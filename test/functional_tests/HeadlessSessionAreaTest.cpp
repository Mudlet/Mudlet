/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * The main window's area an app that embeds Mudlet gives THeadlessSession, and
 * what it is told of the main console's grid and where its text goes. Each test
 * makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionAreaTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionAreaTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)"));
        return true;
    }

    static QString luaString(THeadlessSession& session, const QString& expression)
    {
        if (!session.runLua(qsl("headlessAreaAnswer = tostring(%1)").arg(expression))) {
            return {};
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessAreaAnswer");
        const QString answer = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return answer;
    }

    // calcFontSize() of the main console's font, as it is now
    static QSize cell(THeadlessSession& session) { return {luaString(session, qsl("select(1, calcFontSize())")).toInt(), luaString(session, qsl("select(2, calcFontSize())")).toInt()}; }

    static QString mainWindowSize(THeadlessSession& session) { return luaString(session, qsl("table.concat({getMainWindowSize()}, 'x')")); }

    static QSize grid(THeadlessSession& session) { return {luaString(session, qsl("getColumnCount()")).toInt(), luaString(session, qsl("getRowCount()")).toInt()}; }

    static QVariantList area(const QSize& grid, const QRect& textRect) { return {grid, textRect}; }

    static QList<QVariantList> areasSent(const QSignalSpy& spy)
    {
        QList<QVariantList> areas;
        for (const auto& arguments : spy) {
            areas << arguments.mid(1);
        }
        return areas;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void test_aMainWindowGivenInPixelsHoldsWhatItsFontFits()
    {
        const QString name = qsl("Headless Area Pixels");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.setMainWindowSize(QSize(800, 600)));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.mainConsoleGrid(), QSize());
        QCOMPARE(session.mainConsoleTextRect(), QRect());
        QSignalSpy areas(&session, &THeadlessSession::mainConsoleAreaChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(100);
        QCOMPARE(areas.count(), 0);

        const QSize cellSize = cell(session);
        QVERIFY(cellSize.width() > 0 && cellSize.height() > 0);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QCOMPARE(mainWindowSize(session), qsl("800x600"));
        const QSize expected(800 / cellSize.width(), 600 / cellSize.height());
        QCOMPARE(grid(session), expected);
        QCOMPARE(session.mainConsoleGrid(), expected);
        QCOMPARE(session.mainConsoleTextRect(), QRect(0, 0, 800, 600));
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(expected, QRect(0, 0, 800, 600))}));

        // A size that holds no character, or more than NAWS can carry, is refused and changes nothing
        for (const QSize& refused : {QSize(0, 600), QSize(800, 0), QSize(-1, -1), QSize(cellSize.width() * 65536, 600)}) {
            QVERIFY(!session.setMainWindowSize(refused));
            QVERIFY2(session.errorString().contains(qsl("from 1 to 65535")), qPrintable(session.errorString()));
        }
        QCOMPARE(mainWindowSize(session), qsl("800x600"));

        // The later of the two holds
        QVERIFY(session.setScreenSize(100, 30));
        QCOMPARE(grid(session), QSize(100, 30));
        QCOMPARE(mainWindowSize(session), qsl("%1x%2").arg(100 * cellSize.width()).arg(30 * cellSize.height()));
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QCOMPARE(grid(session), expected);
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.mainConsoleGrid(), QSize());
    }

    void test_aBorderSendsWhereTheTextGoesOncePerTurn()
    {
        const QString name = qsl("Headless Area Borders");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QSize cellSize = cell(session);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy areas(&session, &THeadlessSession::mainConsoleAreaChanged);
        QTest::qWait(100);
        areas.clear();

        QVERIFY(session.runLua(qsl("setBorderTop(50)")));
        const QSize topGrid(800 / cellSize.width(), 550 / cellSize.height());
        QCOMPARE(session.mainConsoleGrid(), topGrid);
        QCOMPARE(session.mainConsoleTextRect(), QRect(0, 50, 800, 550));
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(topGrid, QRect(0, 50, 800, 550))}));
        QCOMPARE(mainWindowSize(session), qsl("800x600"));
        areas.clear();

        QVERIFY(session.runLua(qsl("setBorderLeft(16) setBorderLeft(32)")));
        const QSize leftGrid(768 / cellSize.width(), 550 / cellSize.height());
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(leftGrid, QRect(32, 50, 768, 550))}));
        QTest::qWait(100);
        QCOMPARE(areas.count(), 1);
        areas.clear();

        // Taken back in the same turn, it is as it was, so nothing is sent
        QVERIFY(session.runLua(qsl("setBorderRight(40) setBorderRight(0)")));
        QTest::qWait(100);
        QCOMPARE(areas.count(), 0);

        // As a resize of the GUI's main window, scripts are told what the borders leave of it
        QVERIFY(session.runLua(qsl("headlessAreaResizes = '' registerAnonymousEventHandler('sysWindowResizeEvent', function(_, width, height, name) headlessAreaResizes = headlessAreaResizes .. width "
                                   ".. 'x' .. height .. ' ' .. name .. ';' end)")));
        QVERIFY(session.setMainWindowSize(QSize(1000, 600)));
        QCOMPARE(luaString(session, qsl("headlessAreaResizes")), qsl("968x550 main;"));
        QCOMPARE(session.mainConsoleTextRect(), QRect(32, 50, 968, 550));

        // Borders wider than the area leave the text no room and the grid at its floor
        QVERIFY(session.runLua(qsl("setBorderLeft(2000)")));
        QCOMPARE(session.mainConsoleTextRect(), QRect(2000, 50, 0, 550));
        QCOMPARE(grid(session), QSize(40, 550 / cellSize.height()));
        QCOMPARE(luaString(session, qsl("headlessAreaResizes")), qsl("968x550 main;0x550 main;"));
        QVERIFY(session.shutdown(false));
    }

    void test_aFontChangeKeepsWhatTheAppGave()
    {
        const QString name = qsl("Headless Area Font");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy areas(&session, &THeadlessSession::mainConsoleAreaChanged);
        QTest::qWait(100);
        areas.clear();

        // In pixels, the area stays and the grid is what the new font fits in it
        QVERIFY(session.runLua(qsl("setFontSize(20)")));
        const QSize bigCell = cell(session);
        const QSize bigGrid(800 / bigCell.width(), 600 / bigCell.height());
        QCOMPARE(mainWindowSize(session), qsl("800x600"));
        QCOMPARE(grid(session), bigGrid);
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(bigGrid, QRect(0, 0, 800, 600))}));
        areas.clear();

        // In characters, as a terminal gives it, the grid stays and the area is counted again in the new font
        QVERIFY(session.setScreenSize(100, 30));
        QVERIFY(session.runLua(qsl("setFontSize(12)")));
        const QSize smallCell = cell(session);
        QCOMPARE(grid(session), QSize(100, 30));
        QCOMPARE(mainWindowSize(session), qsl("%1x%2").arg(100 * smallCell.width()).arg(30 * smallCell.height()));
        // Both in one turn, so one is sent
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(QSize(100, 30), QRect(0, 0, 100 * smallCell.width(), 30 * smallCell.height()))}));
        QVERIFY(session.shutdown(false));
    }

    void test_aScriptSetsItsOwnProfilesSize()
    {
        const QString name = qsl("Headless Area Script");
        const QString other = qsl("Headless Area Script Other");
        QVERIFY(writeProfile(name));
        QVERIFY(writeProfile(other));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("loadProfile('%1', true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        const std::optional<QSize> otherSize = pOther->mainWindowSize();
        QVERIFY(otherSize && *otherSize != QSize(640, 480));
        QSignalSpy requests(&session, &THeadlessSession::mainWindowResizeRequested);
        QSignalSpy areas(&session, &THeadlessSession::mainConsoleAreaChanged);
        QVERIFY(session.runLua(qsl(R"lua(
headlessAreaResizes = {}
registerAnonymousEventHandler("sysWindowResizeEvent", function(_, width, height, window)
  headlessAreaResizes[#headlessAreaResizes + 1] = string.format("%dx%d %s", width, height, window)
end)
)lua")));

        QCOMPARE(luaString(session, qsl("setMainWindowSize(640, 480)")), qsl("true"));
        QCOMPARE(mainWindowSize(session), qsl("640x480"));
        QCOMPARE(luaString(session, qsl("table.concat(headlessAreaResizes, ';')")), qsl("640x480 main"));
        const QSize cellSize = cell(session);
        const QSize expected(std::max(40, 640 / cellSize.width()), 480 / cellSize.height());
        QCOMPARE(grid(session), expected);
        QTRY_COMPARE(requests.count(), 1);
        QCOMPARE(requests.at(0), (QVariantList{name, QSize(640, 480)}));
        QTRY_COMPARE(areasSent(areas), (QList<QVariantList>{area(expected, QRect(0, 0, 640, 480))}));
        QCOMPARE(pOther->mainWindowSize(), otherSize);

        // What the app's own call refuses is refused here too, and the app is not asked for it
        for (const QString& refused : {qsl("0, 0"), qsl("-640, 480"), qsl("640, 0"), qsl("%1, 480").arg(cellSize.width() * 65536)}) {
            const QString answer = luaString(session, qsl("(function() local ok, err = setMainWindowSize(%1) return tostring(ok) .. '|' .. tostring(err) end)()").arg(refused));
            QVERIFY2(answer.startsWith(qsl("nil|a main window of")) && answer.endsWith(qsl("each must be from 1 to 65535")), qPrintable(answer));
        }
        QCOMPARE(mainWindowSize(session), qsl("640x480"));
        QCOMPARE(grid(session), expected);

        // Asked again for the size it has, the app is told, and the profile's scripts are not
        QCOMPARE(luaString(session, qsl("setMainWindowSize(640, 480)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 2);
        QCOMPARE(luaString(session, qsl("#headlessAreaResizes")), qsl("1"));

        // Another profile's is its own, and not this session's to tell
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("setMainWindowSize(320, 240)")));
        QCOMPARE(pOther->mainWindowSize(), std::optional(QSize(320, 240)));
        QCOMPARE(mainWindowSize(session), qsl("640x480"));
        QTest::qWait(100);
        QCOMPARE(requests.count(), 2);
        QCOMPARE(areas.count(), 1);
        QVERIFY(session.shutdown(false));
    }

    // The size an app that cannot take the one a script asks for gives back, from the request's handler, next
    // turn, or later as a window system's would, in milliseconds; -1 is from the handler
    void test_aScriptAskingAgainForASizeTheAppCannotGiveSettles_data()
    {
        QTest::addColumn<int>("delay");
        QTest::newRow("from the handler") << -1;
        QTest::newRow("next turn") << 0;
        QTest::newRow("later") << 50;
    }

    // A resize handler that asks again is answered by the size the app gave, as the GUI's is by a window manager
    // holding the window
    void test_aScriptAskingAgainForASizeTheAppCannotGiveSettles()
    {
        QFETCH(const int, delay);
        const QString name = qsl("Headless Area Script Asks Again %1").arg(delay);
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSize terminal(80, 30);
        QVERIFY(session.setScreenSize(terminal.width(), terminal.height()));
        int answers = 0;
        connect(&session, &THeadlessSession::mainWindowResizeRequested, this, [&session, &terminal, &answers, delay](const QString&, const QSize&) {
            ++answers;
            if (delay < 0) {
                session.setScreenSize(terminal.width(), terminal.height());
                return;
            }
            QTimer::singleShot(delay, &session, [&session, &terminal] {
                session.setScreenSize(terminal.width(), terminal.height());
            });
        });
        QVERIFY(session.runLua(qsl(R"lua(
headlessAreaResizes = {}
registerAnonymousEventHandler("sysWindowResizeEvent", function(_, width, height, window)
  if #headlessAreaResizes < 10 then
    headlessAreaResizes[#headlessAreaResizes + 1] = string.format("%dx%d %s", width, height, window)
  end
  if width < 1600 then
    setMainWindowSize(1600, 900)
  end
end)
)lua")));
        const QSize cellSize = cell(session);
        const auto resized = [&cellSize](const QSize& cells) {
            return qsl("%1x%2 main").arg(cells.width() * cellSize.width()).arg(cells.height() * cellSize.height());
        };

        QCOMPARE(luaString(session, qsl("setMainWindowSize(1600, 900)")), qsl("true"));
        // A loop would go on answering for as long as the event loop runs
        QTest::qWait(500);
        QCOMPARE(answers, 1);
        QCOMPARE(luaString(session, qsl("table.concat(headlessAreaResizes, ';')")), (QStringList{qsl("1600x900 main"), resized(terminal)}.join(QLatin1Char(';'))));
        QCOMPARE(grid(session), terminal);

        // Later, too, from a timer
        QVERIFY(session.runLua(qsl("tempTimer(0, function() headlessAreaLater = setMainWindowSize(1600, 900) end)")));
        QTest::qWait(200);
        QCOMPARE(luaString(session, qsl("headlessAreaLater")), qsl("true"));
        QCOMPARE(answers, 1);
        QCOMPARE(grid(session), terminal);

        // A size the app gives of its own accord is a new one, which a script may ask to change once more
        QVERIFY(session.runLua(qsl("headlessAreaResizes = {}")));
        terminal = QSize(100, 30);
        QVERIFY(session.setScreenSize(terminal.width(), terminal.height()));
        QTest::qWait(500);
        QCOMPARE(answers, 2);
        QCOMPARE(luaString(session, qsl("table.concat(headlessAreaResizes, ';')")), (QStringList{resized(terminal), qsl("1600x900 main"), resized(terminal)}.join(QLatin1Char(';'))));
        QCOMPARE(grid(session), terminal);
        QVERIFY(session.shutdown(false));
    }

    // A script that asks for another size after one the app answered is heard, as in the GUI, whether the app
    // answered it with a size of its own or with the one asked for
    void test_aScriptAskingForAnotherSizeIsHeardAgain()
    {
        const QString name = qsl("Headless Area Script Asks Another");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy requests(&session, &THeadlessSession::mainWindowResizeRequested);
        bool honours = false;
        const QSize smallest(640, 480);
        connect(&session, &THeadlessSession::mainWindowResizeRequested, this, [&session, &honours, &smallest](const QString&, const QSize& pixels) {
            session.setMainWindowSize(honours ? pixels : pixels.expandedTo(smallest));
        });

        // A window no smaller than 640x480, which answers each request with the size it took
        QCOMPARE(luaString(session, qsl("setMainWindowSize(320, 200)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 1);
        QCOMPARE(mainWindowSize(session), qsl("640x480"));
        QCOMPARE(luaString(session, qsl("setMainWindowSize(1024, 768)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 2);
        QCOMPARE(mainWindowSize(session), qsl("1024x768"));
        QCOMPARE(luaString(session, qsl("setMainWindowSize(320, 200)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 3);
        QCOMPARE(mainWindowSize(session), qsl("640x480"));

        // Asked twice in one turn, the answer to the first, which the second has replaced, is not laid out
        QVERIFY(session.runLua(qsl(R"lua(
headlessAreaResizes = {}
registerAnonymousEventHandler("sysWindowResizeEvent", function(_, width, height, window)
  headlessAreaResizes[#headlessAreaResizes + 1] = string.format("%dx%d %s", width, height, window)
end)
setMainWindowSize(400, 300)
setMainWindowSize(1100, 700)
)lua")));
        QTRY_COMPARE(requests.count(), 5);
        QTest::qWait(100);
        QCOMPARE(mainWindowSize(session), qsl("1100x700"));
        QCOMPARE(luaString(session, qsl("table.concat(headlessAreaResizes, ';')")), qsl("400x300 main;1100x700 main"));

        // A window that takes any size and says so, and a script that measures and puts back what it had, as UI_spec's do
        honours = true;
        requests.clear();
        QCOMPARE(luaString(session, qsl("setMainWindowSize(1000, 700)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 1);
        QCOMPARE(mainWindowSize(session), qsl("1000x700"));
        QVERIFY(session.runLua(qsl("local width, height = getMainWindowSize() setMainWindowSize(800, 600) setMainWindowSize(width, height)")));
        QTRY_COMPARE(requests.count(), 3);
        QTest::qWait(100);
        QCOMPARE(requests.count(), 3);
        QCOMPARE(requests.at(1), (QVariantList{name, QSize(800, 600)}));
        QCOMPARE(requests.at(2), (QVariantList{name, QSize(1000, 700)}));
        QCOMPARE(mainWindowSize(session), qsl("1000x700"));
        QVERIFY(session.shutdown(false));
    }

    // An app that takes a size it can have and says nothing has the next size it gives taken as the answer
    void test_anAppThatDoesNotAnswerHasItsNextSizeTakenAsTheAnswer()
    {
        const QString name = qsl("Headless Area Silent App");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy requests(&session, &THeadlessSession::mainWindowResizeRequested);
        const QSize smallest(640, 480);
        connect(&session, &THeadlessSession::mainWindowResizeRequested, this, [&session, &smallest](const QString&, const QSize& pixels) {
            if (pixels.width() < smallest.width() || pixels.height() < smallest.height()) {
                session.setMainWindowSize(pixels.expandedTo(smallest));
            }
        });
        QVERIFY(session.setMainWindowSize(QSize(1000, 700)));
        const QSize cellSize = cell(session);

        // The answer to a request the next has replaced is not laid out, though the next is not answered
        QVERIFY(session.runLua(qsl("setMainWindowSize(320, 200) setMainWindowSize(1000, 700)")));
        QTRY_COMPARE(requests.count(), 2);
        QTest::qWait(100);
        QCOMPARE(mainWindowSize(session), qsl("1000x700"));
        QCOMPARE(grid(session), QSize(std::max(40, 1000 / cellSize.width()), 700 / cellSize.height()));

        // A size it took silently is answered by the next it gives, so the app is not asked for it again till it
        // gives another
        QCOMPARE(luaString(session, qsl("setMainWindowSize(1024, 768)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 3);
        QVERIFY(session.setMainWindowSize(QSize(900, 700)));
        for (int ask = 0; ask < 2; ++ask) {
            QCOMPARE(luaString(session, qsl("setMainWindowSize(1024, 768)")), qsl("true"));
            QTest::qWait(50);
            QCOMPARE(requests.count(), 3);
            QCOMPARE(mainWindowSize(session), qsl("900x700"));
        }
        QVERIFY(session.setMainWindowSize(QSize(950, 700)));
        QCOMPARE(luaString(session, qsl("setMainWindowSize(1024, 768)")), qsl("true"));
        QTRY_COMPARE(requests.count(), 4);
        QCOMPARE(mainWindowSize(session), qsl("1024x768"));
        QVERIFY(session.shutdown(false));
    }

    // With nothing connected to mainWindowResizeRequested nothing answers, so a size the app gives is not taken
    // as an answer and a resize handler keeping to a size keeps it after each one
    void test_anAppThatDoesNotListenLeavesAScriptItsSize()
    {
        const QString name = qsl("Headless Area Deaf App");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
registerAnonymousEventHandler("sysWindowResizeEvent", function(_, width, height)
  if width ~= 1200 or height ~= 800 then
    setMainWindowSize(1200, 800)
  end
end)
)lua")));
        const QSize cellSize = cell(session);
        const QSize kept(std::max(40, 1200 / cellSize.width()), 800 / cellSize.height());
        for (const QSize& given : {QSize(1000, 700), QSize(900, 650), QSize(950, 600)}) {
            QVERIFY(session.setMainWindowSize(given));
            QCOMPARE(mainWindowSize(session), qsl("1200x800"));
            QCOMPARE(grid(session), kept);
            // The request's relay, which finds nothing to answer it
            QTest::qWait(50);
        }
        QVERIFY(session.shutdown(false));
    }

    void test_aMainWindowSizeIsRefusedMidWrite()
    {
        const QString name = qsl("Headless Area Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        std::optional<bool> setMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &setMidWrite](const QString&, const QString& text) {
            if (text == qsl("headless area line")) {
                setMidWrite = session.setMainWindowSize(QSize(640, 480));
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless area line\n"))lua")));
        QCOMPARE(setMidWrite, std::optional(false));
        QCOMPARE(mainWindowSize(session), qsl("800x600"));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionAreaTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionAreaTest)
