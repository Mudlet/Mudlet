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
