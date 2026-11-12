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
 * The windows a profile's scripts make, as an app that embeds Mudlet reads them
 * from THeadlessSession to draw them. Each test makes a session of its own, and
 * with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionWindowsTest -V
 */

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TCommandHistory.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

using Kind = THeadlessSession::ScriptWindow::Kind;

class HeadlessSessionWindowsTest : public QObject
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
        if (!session.runLua(qsl("headlessWindowsAnswer = tostring(%1)").arg(expression))) {
            return {};
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessWindowsAnswer");
        const QString answer = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return answer;
    }

    static qsizetype indexOf(const QList<THeadlessSession::ScriptWindow>& windows, const QString& name, const Kind kind)
    {
        for (qsizetype i = 0; i < windows.size(); ++i) {
            if (windows.at(i).name == name && windows.at(i).kind == kind) {
                return i;
            }
        }
        return -1;
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

    void test_eachWindowReadsBackWithItsKindParentAndPlace()
    {
        const QString name = qsl("Headless Windows List");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(session.scriptWindows().isEmpty());
        QVERIFY(!session.scriptWindow(qsl("mc")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mc", 10, 20, 300, 200)
enableCommandLine("mc")
setBackgroundColor("mc", 10, 20, 30, 255)
openUserWindow("uw")
createLabel("uw", "lbl", 5, 6, 70, 80, 1)
createScrollBox("main", "sb", 400, 10, 200, 100)
createTextEdit("sb", "tb", 1, 2, 50, 40)
hpGauge = Geyser.Gauge:new({name = "hp", x = 0, y = 300, width = 200, height = 20})
openMapWidget()
)lua")));

        const QList<THeadlessSession::ScriptWindow> windows = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> mc = session.scriptWindow(qsl("mc"));
        QVERIFY(mc);
        QCOMPARE(mc->kind, Kind::miniConsole);
        QCOMPARE(mc->parent, QString());
        QCOMPARE(mc->geometry, QRect(10, 20, 300, 200));
        QVERIFY(mc->visible && mc->shown);
        QVERIFY(!mc->inDock);
        QCOMPARE(mc->background, QColor(10, 20, 30));
        QCOMPARE(mc->font.pointSize(), 12);

        // Its own command line shares its name
        const std::optional<THeadlessSession::ScriptWindow> line = session.scriptWindow(qsl("mc"), Kind::commandLine);
        QVERIFY(line);
        QCOMPARE(line->parent, qsl("mc"));
        QVERIFY(line->visible && line->shown);

        const std::optional<THeadlessSession::ScriptWindow> uw = session.scriptWindow(qsl("uw"));
        QVERIFY(uw);
        QCOMPARE(uw->kind, Kind::userWindow);
        QCOMPARE(uw->parent, QString());
        QVERIFY(uw->inDock);
        const std::optional<THeadlessSession::ScriptWindow> label = session.scriptWindow(qsl("lbl"));
        QVERIFY(label);
        QCOMPARE(label->kind, Kind::label);
        QCOMPARE(label->parent, qsl("uw"));
        QCOMPARE(label->geometry, QRect(5, 6, 70, 80));
        QCOMPARE(label->background, QColor(32, 32, 32));

        const std::optional<THeadlessSession::ScriptWindow> box = session.scriptWindow(qsl("sb"));
        QVERIFY(box);
        QCOMPARE(box->kind, Kind::scrollBox);
        QCOMPARE(box->geometry, QRect(400, 10, 200, 100));
        const std::optional<THeadlessSession::ScriptWindow> textBox = session.scriptWindow(qsl("tb"));
        QVERIFY(textBox);
        QCOMPARE(textBox->kind, Kind::textBox);
        QCOMPARE(textBox->parent, qsl("sb"));
        QCOMPARE(textBox->geometry, QRect(1, 2, 50, 40));

        // A Geyser gauge is the labels it is made of
        for (const QString& part : {qsl("hp_back"), qsl("hp_front"), qsl("hp_text")}) {
            const std::optional<THeadlessSession::ScriptWindow> gaugeLabel = session.scriptWindow(part);
            QVERIFY2(gaugeLabel, qPrintable(part));
            QCOMPARE(gaugeLabel->kind, Kind::label);
            QCOMPARE(gaugeLabel->parent, QString());
        }
        const std::optional<THeadlessSession::ScriptWindow> back = session.scriptWindow(qsl("hp_back"));
        QVERIFY(back);
        QCOMPARE(back->geometry, QRect(0, 300, 200, 20));

        // The dock openMapWidget() opened, beside the main window's area as a user window is
        const std::optional<THeadlessSession::ScriptWindow> map = session.scriptWindow(qsl("mapper"));
        QVERIFY(map);
        QCOMPARE(map->kind, Kind::mapper);
        QVERIFY(map->inDock);

        // Parents before their children, and the main window's area before the docks
        QVERIFY(indexOf(windows, qsl("uw"), Kind::userWindow) < indexOf(windows, qsl("lbl"), Kind::label));
        QVERIFY(indexOf(windows, qsl("mc"), Kind::miniConsole) < indexOf(windows, qsl("mc"), Kind::commandLine));
        QVERIFY(indexOf(windows, qsl("sb"), Kind::scrollBox) < indexOf(windows, qsl("tb"), Kind::textBox));
        QVERIFY(indexOf(windows, qsl("hp_text"), Kind::label) < indexOf(windows, qsl("uw"), Kind::userWindow));
        QCOMPARE(windows.size(), 10);

        // Hidden, the console's own flag is off, and what is in it is not shown while keeping its own
        QVERIFY(session.runLua(qsl(R"(hideWindow("mc") hideWindow("uw"))")));
        const std::optional<THeadlessSession::ScriptWindow> hiddenMc = session.scriptWindow(qsl("mc"));
        const std::optional<THeadlessSession::ScriptWindow> hiddenLine = session.scriptWindow(qsl("mc"), Kind::commandLine);
        const std::optional<THeadlessSession::ScriptWindow> hiddenLabel = session.scriptWindow(qsl("lbl"));
        QVERIFY(hiddenMc && hiddenLine && hiddenLabel);
        QVERIFY(!hiddenMc->visible);
        QVERIFY(!hiddenMc->shown);
        QVERIFY(hiddenLine->visible);
        QVERIFY(!hiddenLine->shown);
        QVERIFY(hiddenLabel->visible);
        QVERIFY(!hiddenLabel->shown);

        // Made later is drawn over, until a raise or a lower changes it; -1 for one that is not there
        const auto order = [&session](const QString& window) {
            const std::optional<THeadlessSession::ScriptWindow> found = session.scriptWindow(window);
            return found ? found->stackOrder : -1;
        };
        // The docks are stacked apart from the main window's area, which the 5 windows in it count up in
        QCOMPARE(order(qsl("uw")), 0);
        QCOMPARE(order(qsl("mapper")), 1);
        QCOMPARE(order(qsl("hp_text")), 4);
        QVERIFY(order(qsl("hp_back")) >= 0);
        QVERIFY(order(qsl("hp_back")) < order(qsl("hp_front")));
        QVERIFY(session.runLua(qsl(R"(raiseWindow("hp_back"))")));
        QVERIFY(order(qsl("hp_back")) > order(qsl("hp_front")));
        QVERIFY(order(qsl("hp_back")) > order(qsl("mc")));
        QCOMPARE(order(qsl("hp_back")), 4);
        QCOMPARE(order(qsl("mapper")), 1);
        QVERIFY(session.runLua(qsl(R"(lowerWindow("hp_back"))")));
        QCOMPARE(order(qsl("hp_back")), 0);
        QCOMPARE(luaString(session, qsl("raiseWindow('nonesuch')")), qsl("false"));

        QVERIFY(session.shutdown(false));
        QVERIFY(session.scriptWindows().isEmpty());
    }

    void test_eachWindowIsListedInItsOwnParent()
    {
        const QString name = qsl("Headless Windows Parents");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        // A scroll box may share its name with a mini console, made here first so that it is listed first
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("dup", 0, 0, 100, 100)
createScrollBox("main", "dup", 200, 0, 100, 100)
createLabel("dup", "inBox", 1, 2, 10, 10, 1)
openUserWindow("uw")
createMapper(0, 0, 100, 100)
setWindow("uw", "mapper", 40, 50, true)
)lua")));
        const QList<THeadlessSession::ScriptWindow> windows = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> inBox = session.scriptWindow(qsl("inBox"));
        QVERIFY(inBox);
        QCOMPARE(inBox->parent, qsl("dup"));
        QCOMPARE(inBox->parentKind, Kind::scrollBox);
        QVERIFY(indexOf(windows, qsl("dup"), Kind::miniConsole) < indexOf(windows, qsl("dup"), Kind::scrollBox));
        QVERIFY(indexOf(windows, qsl("dup"), Kind::scrollBox) < indexOf(windows, qsl("inBox"), Kind::label));

        // setWindow() moves the mapper to its point in the window it is put in, as QWidget::move() does
        std::optional<THeadlessSession::ScriptWindow> mapper = session.scriptWindow(qsl("mapper"));
        QVERIFY(mapper);
        QCOMPARE(mapper->parent, qsl("uw"));
        QCOMPARE(mapper->parentKind, Kind::userWindow);
        QCOMPARE(mapper->geometry, QRect(40, 50, 100, 100));
        QVERIFY(session.runLua(qsl(R"(setWindow("main", "mapper", 7, 8, true))")));
        mapper = session.scriptWindow(qsl("mapper"));
        QVERIFY(mapper);
        QCOMPARE(mapper->parent, QString());
        QCOMPARE(mapper->geometry, QRect(7, 8, 100, 100));

        // What a deleted scroll box held stays, in its user window, until deferred deletes run
        QVERIFY(session.runLua(qsl(R"(createScrollBox("uw", "gone", 0, 0, 50, 50) createLabel("gone", "orphan", 1, 1, 5, 5, 1) deleteScrollBox("gone"))")));
        const QList<THeadlessSession::ScriptWindow> afterDelete = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> orphan = session.scriptWindow(qsl("orphan"));
        QVERIFY(orphan);
        QCOMPARE(orphan->parent, qsl("uw"));
        QCOMPARE(orphan->parentKind, Kind::userWindow);
        QVERIFY(indexOf(afterDelete, qsl("uw"), Kind::userWindow) < indexOf(afterDelete, qsl("orphan"), Kind::label));
        QVERIFY(session.shutdown(false));
    }

    // What a deleted user window held keeps its names until deferred deletes run, as the dock's deleteLater() takes
    // it on the desktop, but is in no window an app is shown, and a script's later windows of that name stay
    void test_whatADeletedUserWindowHeldIsInNoWindowTheAppSees()
    {
        const QString name = qsl("Headless Windows Held");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        QSignalSpy texts(&session, &THeadlessSession::windowTextChanged);
        const auto namesTold = [](const QSignalSpy& spy) {
            QStringList names;
            for (const QList<QVariant>& emitted : spy) {
                names << emitted.at(1).toStringList();
            }
            return names;
        };
        // The first window whose parent is a deleted window's internal key, which an app must never be given
        const auto underRetiredKey = [&session]() {
            for (const THeadlessSession::ScriptWindow& window : session.scriptWindows()) {
                if (window.parent.startsWith(QChar(0))) {
                    return window.name;
                }
            }
            return QString();
        };
        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("held")
createMiniConsole("held", "heldMini", 0, 0, 100, 100)
createMiniConsole("held", "heldRescued", 0, 0, 100, 100)
createLabel("held", "heldLabel", 0, 0, 10, 10, 1)
createScrollBox("held", "heldBox", 0, 0, 50, 50)
createLabel("heldBox", "heldBoxLabel", 1, 1, 5, 5, 1)
createScrollBox("held", "heldGoneBox", 0, 0, 50, 50)
createLabel("heldGoneBox", "heldGoneBoxLabel", 1, 1, 5, 5, 1)
)lua")));
        QVERIFY(session.scriptWindow(qsl("heldGoneBoxLabel")));
        QVERIFY(changes.wait(1000));
        QCoreApplication::processEvents();
        changes.clear();
        texts.clear();
        QVERIFY(session.runLua(qsl(R"lua(
deleteScrollBox("heldGoneBox")
deleteMiniConsole("held")
echo("heldMini", "after the delete\n")
echo("heldRescued", "before its rescue\n")
setWindow("main", "heldRescued", 0, 0, true)
)lua")));
        const QStringList held{qsl("heldMini"), qsl("heldLabel"), qsl("heldBox"), qsl("heldBoxLabel"), qsl("heldGoneBoxLabel")};
        QCOMPARE(luaString(session, qsl("windowType('heldMini')")), qsl("miniconsole"));
        // Not changes.wait(), whose event loop would run the deferred deletes
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QVERIFY(!changes.isEmpty());
        const QStringList told = namesTold(changes);
        const QStringList textTold = namesTold(texts);
        for (const QString& window : held) {
            QVERIFY2(!session.scriptWindow(window), qPrintable(window));
            QVERIFY2(told.contains(window), qPrintable(window + u' ' + told.join(u',')));
        }
        QVERIFY(!textTold.contains(qsl("heldMini")));
        QVERIFY(textTold.contains(qsl("heldRescued")));
        QVERIFY(!session.windowLines(qsl("heldMini"), 0, 1));
        QVERIFY(session.windowLines(qsl("heldRescued"), 0, 1));
        QCOMPARE(underRetiredKey(), QString());
        std::optional<THeadlessSession::ScriptWindow> rescued = session.scriptWindow(qsl("heldRescued"));
        QVERIFY(rescued);
        QCOMPARE(rescued->parent, QString());

        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("held")
createMiniConsole("held", "heldFresh", 0, 0, 100, 100)
createLabel("held", "heldFreshLabel", 0, 0, 10, 10, 1)
)lua")));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        for (const QString& window : held) {
            QCOMPARE(luaString(session, qsl("windowType('%1')").arg(window)), qsl("nil"));
        }
        for (const QString& window : {qsl("heldFresh"), qsl("heldFreshLabel")}) {
            const std::optional<THeadlessSession::ScriptWindow> fresh = session.scriptWindow(window);
            QVERIFY2(fresh, qPrintable(window));
            QCOMPARE(fresh->parent, qsl("held"));
        }
        QCOMPARE(luaString(session, qsl("windowType('heldFresh')")), qsl("miniconsole"));
        QCOMPARE(luaString(session, qsl("windowType('heldRescued')")), qsl("miniconsole"));
        QVERIFY(session.runLua(qsl(R"(echo("heldFresh", "fresh\n") echo("heldRescued", "still here\n"))")));
        QVERIFY(session.windowLines(qsl("heldFresh"), 0, 1));
        rescued = session.scriptWindow(qsl("heldRescued"));
        QVERIFY(rescued);
        QCOMPARE(rescued->parent, QString());
        QCOMPARE(underRetiredKey(), QString());
        QVERIFY(session.shutdown(false));
    }

    // Columns and rows are counted in the font the main console's text is drawn in, which a profile sheet can give it
    void test_theMainAreaIsCountedInTheMainTextsFont()
    {
        const QString name = qsl("Headless Windows Main Text");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        const auto grid = [&session]() {
            return QSize(luaString(session, qsl("getColumnCount('main')")).toInt(), luaString(session, qsl("getRowCount('main')")).toInt());
        };
        const QSize base = grid();
        QVERIFY(!base.isEmpty());
        QVERIFY(session.runLua(qsl(R"(setProfileStyleSheet("TMainConsole TTextEdit { font-size: 20pt; }"))")));
        const QSize styled = grid();
        QVERIFY2(styled.width() < base.width() && styled.height() < base.height(), qPrintable(qsl("%1x%2 %3x%4").arg(styled.width()).arg(styled.height()).arg(base.width()).arg(base.height())));
        QVERIFY(session.runLua(qsl(R"(setProfileStyleSheet(""))")));
        QCOMPARE(grid(), base);
        QVERIFY(session.shutdown(false));
    }

    void test_aTurnsChangesAreToldOnce()
    {
        const QString name = qsl("Headless Windows Changes");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        const auto toldOnce = [&changes](const QStringList& names) {
            if (!changes.wait(1000)) {
                return false;
            }
            // Anything more this turn would be queued by now
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            const bool once = changes.count() == 1 && changes.at(0).at(0).toString() == qsl("Headless Windows Changes") && changes.at(0).at(1).toStringList() == names;
            changes.clear();
            return once;
        };

        QVERIFY(session.runLua(qsl(R"(createLabel("old", 0, 0, 10, 10, 1))")));
        QVERIFY(toldOnce({qsl("old")}));

        QVERIFY(session.runLua(qsl(R"lua(
createLabel("fresh", 0, 0, 10, 10, 1)
moveWindow("fresh", 50, 60)
resizeWindow("fresh", 30, 40)
hideWindow("fresh")
deleteLabel("old")
)lua")));
        QVERIFY(toldOnce({qsl("fresh"), qsl("old")}));
        QVERIFY(!session.scriptWindow(qsl("old")));
        const std::optional<THeadlessSession::ScriptWindow> fresh = session.scriptWindow(qsl("fresh"));
        QVERIFY(fresh);
        QCOMPARE(fresh->geometry, QRect(50, 60, 30, 40));

        QVERIFY(session.runLua(qsl(R"(moveWindow("fresh", 1, 2))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(resizeWindow("fresh", 5, 6))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(showWindow("fresh"))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(createMiniConsole("mc", 0, 0, 100, 100))")));
        QVERIFY(toldOnce({qsl("mc")}));
        QVERIFY(session.runLua(qsl(R"(setBackgroundColor("mc", 1, 2, 3, 255))")));
        QVERIFY(toldOnce({qsl("mc")}));
        // A dock is not stacked with the main window's area, so raising a window there does not move it
        QVERIFY(session.runLua(qsl(R"(openUserWindow("uw"))")));
        QVERIFY(toldOnce({qsl("uw")}));
        QVERIFY(session.runLua(qsl(R"(raiseWindow("fresh"))")));
        QVERIFY(toldOnce({qsl("fresh"), qsl("mc")}));

        QVERIFY(session.runLua(qsl(R"(moveWindow("fresh", 1, 2) moveWindow("mc", 0, 0))")));
        QVERIFY(!changes.wait(200));

        // The mapper goes with the user window it is in only as deferred deletes run, after the window's own change
        // has been told
        QVERIFY(session.runLua(qsl(R"(openUserWindow("mapWindow") createMapper("mapWindow", 0, 0, 50, 50))")));
        QVERIFY(changes.wait(1000));
        QVERIFY(session.scriptWindow(qsl("mapper")));
        changes.clear();
        QVERIFY(session.runLua(qsl(R"(deleteMiniConsole("mapWindow"))")));
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QVERIFY(changes.count() >= 1);
        QVERIFY(session.scriptWindow(qsl("mapper")));
        changes.clear();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!session.scriptWindow(qsl("mapper")));
        QVERIFY(toldOnce({qsl("mapper")}));

        QVERIFY(session.shutdown(false));
    }

    void test_eachLabelReadsBackWhatItShows()
    {
        const QString name = qsl("Headless Label Content");
        QVERIFY(writeProfile(name));
        QTemporaryDir imageDir;
        const QString image = imageDir.filePath(qsl("red.png"));
        QImage red(4, 4, QImage::Format_ARGB32);
        red.fill(Qt::red);
        QVERIFY(red.save(image));
        THeadlessSession session;
        QVERIFY(!session.labelContent(qsl("lbl")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        QVERIFY(session.runLua(qsl(R"lua(
createLabel("lbl", 0, 0, 100, 20, 1)
createMiniConsole("mc", 0, 100, 100, 100)
hpGauge = Geyser.Gauge:new({name = "hp", x = 0, y = 300, width = 200, height = 20})
hpGauge:setValue(25, 100)
)lua")));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QVERIFY(!session.labelContent(qsl("mc")));
        QVERIFY(!session.labelContent(qsl("nonesuch")));
        std::optional<THeadlessSession::LabelContent> content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->text, QString());

        QVERIFY(session.runLua(qsl(R"(echo("lbl", "<b>hp</b>"))")));
        QVERIFY(changes.wait(1000));
        QCOMPARE(changes.takeFirst().at(1).toStringList(), QStringList{qsl("lbl")});
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->text.contains(qsl("<b>hp</b>")));

        QVERIFY(session.runLua(qsl(R"(setBackgroundImage("lbl", [[%1]]))").arg(image)));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->image, image);
        QVERIFY(!content->svg);
        // A raster image takes the text down, as QLabel::setPixmap() does
        QCOMPARE(content->text, QString());

        QVERIFY(session.runLua(qsl(R"lua(
setLabelToolTip("lbl", "your health", 3)
setLabelCursor("lbl", 2)
setLabelStyleSheet("lbl", "color: red;")
)lua")));
        QVERIFY(changes.wait(1000));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->toolTip, qsl("your health"));
        QCOMPARE(content->toolTipDuration, 3.0);
        QCOMPARE(content->cursor, std::optional(Qt::CrossCursor));
        QCOMPARE(content->styleSheet, qsl("color: red;"));
        QCOMPARE(content->accessibleName, qsl("your health"));
        QVERIFY(!content->accessibleButton);
        QVERIFY(!content->clickThrough);
        changes.clear();
        QVERIFY(session.runLua(qsl(R"(setLabelAccessibleName("lbl", "health"))")));
        QVERIFY(changes.wait(1000));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->accessibleName, qsl("health"));

        // A movie's playback is told as scripts change it
        const QString movie = imageDir.filePath(qsl("two.gif"));
        QFile gif(movie);
        QVERIFY(gif.open(QIODevice::WriteOnly));
        // Two frames of one pixel, as GeyserLabel_spec.lua writes them
        gif.write(QByteArray::fromHex("47494638396101000100800000000000ffffff21ff0b4e45545343415045322e30030100000021f9"
                                      "04000a0000002c000000000100010000020244010021f904000a0000002c00000000010001000002024c01003b"));
        gif.close();
        QVERIFY(session.runLua(qsl(R"(setMovie("lbl", [[%1]]))").arg(movie)));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->movie, movie);
        QVERIFY(!content->moviePaused);
        QCOMPARE(content->movieFrame, -1);
        QCOMPARE(content->movieSpeed, 100);
        QVERIFY(!content->movieSize.isValid());
        QVERIFY(changes.wait(1000));
        changes.clear();
        // Each call is told on its own; the frame to jump to is the one the pause did not stop on
        const auto told = [&](const QString& call) {
            if (!session.runLua(call) || !changes.wait(1000)) {
                return false;
            }
            const bool named = changes.takeFirst().at(1).toStringList() == QStringList{qsl("lbl")};
            changes.clear();
            return named;
        };
        QVERIFY(told(qsl("pauseMovie('lbl')")));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        const int other = 1 - content->movieFrame;
        QVERIFY(told(qsl("setMovieFrame('lbl', %1)").arg(other)));
        QVERIFY(told(qsl("setMovieSpeed('lbl', 50)")));
        QVERIFY(told(qsl("scaleMovie('lbl')")));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        QCOMPARE(content->movieFrame, other);
        QCOMPARE(content->movieSpeed, 50);
        QCOMPARE(content->movieSize, QSize(100, 20));
        QVERIFY(session.runLua(qsl("startMovie('lbl')")));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QVERIFY(!session.labelContent(qsl("lbl"))->moviePaused);
        QCOMPARE(session.labelContent(qsl("lbl"))->movieFrame, -1);
        // A style sheet's empty text is what QLabel::setMovie() left, so QLabel::setText() keeps the movie playing
        QVERIFY(session.runLua(qsl(R"(setLabelStyleSheet("lbl", 'qproperty-text: "";'))")));
        QCOMPARE(session.labelContent(qsl("lbl"))->movie, movie);
        QVERIFY(!session.labelContent(qsl("lbl"))->moviePaused);
        QVERIFY(session.runLua(qsl(R"(setLabelStyleSheet("lbl", ""))")));
        changes.clear();

        // One that does not loop stops by itself on its last frame, and that is told too
        const QString once = imageDir.filePath(qsl("once.gif"));
        QFile onceGif(once);
        QVERIFY(onceGif.open(QIODevice::WriteOnly));
        onceGif.write(QByteArray::fromHex("47494638396101000100800000000000ffffff21f904000a0000002c000000000100010000020244010021f904000a0000002c"
                                          "00000000010001000002024c01003b"));
        onceGif.close();
        QVERIFY(session.runLua(qsl(R"(createLabel("onceLabel", 0, 400, 10, 10, 1) setMovie("onceLabel", [[%1]]))").arg(once)));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QTRY_VERIFY_WITH_TIMEOUT(changes.count() > 0, 3000);
        QCOMPARE(changes.takeFirst().at(1).toStringList(), QStringList{qsl("onceLabel")});
        content = session.labelContent(qsl("onceLabel"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        QCOMPARE(content->movieFrame, 1);

        // A gauge's value is its front label's width
        const std::optional<THeadlessSession::ScriptWindow> back = session.scriptWindow(qsl("hp_back"));
        const std::optional<THeadlessSession::ScriptWindow> front = session.scriptWindow(qsl("hp_front"));
        QVERIFY(back && front);
        QCOMPARE(back->geometry.width(), 200);
        QCOMPARE(front->geometry.width(), 50);
        QVERIFY(session.labelContent(qsl("hp_front")));

        QVERIFY(session.shutdown(false));
        QVERIFY(!session.labelContent(qsl("lbl")));
    }

    void test_userWindowsArePlacedAsTheGuisDocksAre()
    {
        const QString name = qsl("Headless User Window Place");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.setUserWindowGeometry(qsl("uw"), QRect(0, 0, 250, 400), false));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("uw", false, false, "left")
openUserWindow("floater", false, true, "f")
setUserWindowTitle("floater", "Chat")
createLabel("lbl", 0, 0, 10, 10, 1)
resizes = {}
registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, window)
  resizes[#resizes + 1] = string.format("%s %d %d", window, width, height)
end)
)lua")));
        const std::optional<THeadlessSession::ScriptWindow> uw = session.scriptWindow(qsl("uw"));
        QVERIFY(uw && uw->dock);
        QVERIFY(!uw->dock->autoDock);
        QCOMPARE(uw->dock->area, Qt::LeftDockWidgetArea);
        QVERIFY(!uw->dock->floating);
        QCOMPARE(uw->dock->title, qsl("uw"));
        const std::optional<THeadlessSession::ScriptWindow> floater = session.scriptWindow(qsl("floater"));
        QVERIFY(floater && floater->dock);
        QVERIFY(floater->dock->autoDock && floater->dock->floating);
        QCOMPARE(floater->dock->title, qsl("Chat"));
        const std::optional<THeadlessSession::ScriptWindow> label = session.scriptWindow(qsl("lbl"));
        QVERIFY(label);
        QVERIFY(!label->dock);

        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(10, 20, 250, 400), false));
        QCOMPARE(luaString(session, qsl("table.concat(resizes, ';')")), qsl("uw 250 400"));
        QCOMPARE(luaString(session, qsl("table.concat({getUserWindowSize('uw')}, ',')")), qsl("250,400"));
        QCOMPARE(luaString(session, qsl("table.concat({getWindowGeometry('uw')}, ',')")), qsl("10,20,250,400"));
        std::optional<THeadlessSession::ScriptWindow> placed = session.scriptWindow(qsl("uw"));
        QVERIFY(placed && placed->dock);
        QCOMPARE(placed->geometry, QRect(10, 20, 250, 400));
        // Moved, not resized, as a dock's moveEvent() raises nothing
        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(30, 40, 250, 400), true));
        QCOMPARE(luaString(session, qsl("#resizes")), qsl("1"));
        placed = session.scriptWindow(qsl("uw"));
        QVERIFY(placed && placed->dock);
        QVERIFY(placed->dock->floating);
        // The area is what the script asked for, wherever the app docks it
        QCOMPARE(placed->dock->area, Qt::LeftDockWidgetArea);

        QVERIFY(session.closeUserWindow(qsl("uw")));
        QCOMPARE(luaString(session, qsl("windowVisible('uw')")), qsl("false"));
        // A size given while it is closed reaches it as it next shows
        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(0, 0, 300, 300), false));
        QCOMPARE(luaString(session, qsl("#resizes")), qsl("1"));
        QVERIFY(session.runLua(qsl(R"(showWindow("uw"))")));
        QCOMPARE(luaString(session, qsl("resizes[2]")), qsl("uw 300 300"));

        // A handler of the resize that takes the window away, or opens another of its name in its place, leaves it unplaced
        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("gone")
openUserWindow("again")
registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, window)
  if window == "gone" or (window == "again" and width == 222) then
    deleteMiniConsole(window)
  end
  if window == "again" and width == 222 then
    openUserWindow("again")
  end
end)
)lua")));
        QVERIFY(!session.setUserWindowGeometry(qsl("gone"), QRect(0, 0, 111, 111), false));
        QVERIFY(session.errorString().contains(qsl("taken away")));
        QVERIFY(!session.scriptWindow(qsl("gone")));
        QVERIFY(!session.setUserWindowGeometry(qsl("again"), QRect(0, 0, 222, 222), false));
        QVERIFY(session.errorString().contains(qsl("made again")));
        const std::optional<THeadlessSession::ScriptWindow> again = session.scriptWindow(qsl("again"));
        QVERIFY(again);
        QVERIFY(again->geometry.size() != QSize(222, 222));

        QVERIFY(!session.setUserWindowGeometry(qsl("lbl"), QRect(0, 0, 5, 5), false));
        QVERIFY(session.errorString().contains(qsl("lbl")));
        QVERIFY(!session.closeUserWindow(qsl("nonesuch")));
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.closeUserWindow(qsl("uw")));
    }

    void test_aUserWindowsSheetFindsItsTextByTheGuisName()
    {
        const QString name = qsl("HeadlessPaneSheet");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"(openUserWindow("uw") setFontSize("uw", 10))")));
        // The TConsole constructor names the upper pane, which a rule may pick by that name
        QVERIFY(session.runLua(qsl(R"(setUserWindowStyleSheet("uw", "TTextEdit#upperPane_HeadlessPaneSheet_uw { font-size: 21pt; }"))")));
        QCOMPARE(luaString(session, qsl(R"(getFontSize("uw"))")), qsl("21"));
        QVERIFY(session.runLua(qsl(R"(setUserWindowStyleSheet("uw", "TTextEdit#upperPane_HeadlessPaneSheet_other { font-size: 14pt; }"))")));
        QCOMPARE(luaString(session, qsl(R"(getFontSize("uw"))")), qsl("10"));
        QVERIFY(session.shutdown(false));
    }

    void test_aMiniConsoleItsFontEventDeletesIsNotMadeFurther()
    {
        const QString name = qsl("Headless Mini Console Font Event");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
registerAnonymousEventHandler("sysFontChangeEvent", function(_, window)
  if window == "gone" then deleteMiniConsole("gone") end
  if window == "again" and not madeAgain then madeAgain = true deleteMiniConsole("again") createMiniConsole("again", 0, 0, 50, 50) end
end)
createMiniConsole("gone", 0, 0, 100, 100)
createMiniConsole("again", 0, 0, 100, 100)
)lua")));
        QCOMPARE(indexOf(session.scriptWindows(), qsl("gone"), Kind::miniConsole), -1);
        const qsizetype again = indexOf(session.scriptWindows(), qsl("again"), Kind::miniConsole);
        QVERIFY(again >= 0);
        QCOMPARE(session.scriptWindows().at(again).geometry.size(), QSize(50, 50));
        QVERIFY(session.shutdown(false));
    }

    void test_labelsRunTheirCallbacksAsTheGuisMouseDoes()
    {
        const QString name = qsl("Headless Label Pointer");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QVERIFY(!session.pointer(qsl("lbl"), {.type = Type::press, .position = QPointF(5, 6)}));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        const QString noting = qsl(R"lua(
got = {}
function note(kind)
  return function(event)
    if not event then
      got[#got + 1] = kind
    elseif kind == "wheel" then
      got[#got + 1] = string.format("%s %d %d %d", kind, event.x, event.y, event.angleDeltaY)
    else
      got[#got + 1] = string.format("%s %s %d %d %d %d", kind, tostring(event.button), event.x, event.y, event.globalX, event.globalY)
    end
  end
end
function noteAll(label)
  createLabel(label, 100, 50, 80, 40, 1)
  setLabelClickCallback(label, note("click"))
  setLabelDoubleClickCallback(label, note("double"))
  setLabelReleaseCallback(label, note("release"))
  setLabelMoveCallback(label, note("move"))
  setLabelWheelCallback(label, note("wheel"))
  setLabelOnEnter(label, note("enter"))
  setLabelOnLeave(label, note("leave"))
end
)lua");
        QVERIFY(session.runLua(noting));
        QVERIFY(session.runLua(qsl(R"lua(
noteAll("lbl")
-- Docks, which are beside the main window's area and so under nothing in it
openUserWindow("uw")
openMapWidget()
createLabel("plain", 300, 50, 80, 40, 1)
createLabel("glass", 100, 50, 80, 40, 1)
enableClickthrough("glass")
createLabel("button", 500, 50, 80, 40, 1)
setLabelClickCallback("button", note("click"))
)lua")));
        const auto got = [&session]() {
            const QString all = luaString(session, qsl("table.concat(got, ';')"));
            session.runLua(qsl("got = {}"));
            return all;
        };

        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::press, .position = QPointF(5, 6), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(got(), qsl("enter nil 5 6 105 56;click LeftButton 5 6 105 56"));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::release, .position = QPointF(5, 6), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::doubleClick, .position = QPointF(7, 8), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(9, 10)}));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, 120)}));
        QCOMPARE(got(), qsl("release LeftButton 5 6 105 56;double RightButton 7 8 107 58;move NoButton 9 10 109 60;wheel 1 2 120"));

        // The pointer is over one label at a time
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::enter, .position = QPointF(1, 1)}));
        QVERIFY(session.pointer(qsl("plain"), {.type = Type::move, .position = QPointF(1, 1)}));
        QCOMPARE(got(), qsl("leave"));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::leave}));
        QCOMPARE(got(), QString());
        QVERIFY(session.pointer(qsl("plain"), {.type = Type::press, .position = QPointF(1, 1), .button = Qt::LeftButton}));
        QCOMPARE(got(), QString());

        // A click-through label lets the one under it have the click, not a dock made since
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::press, .position = QPointF(2, 3), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("enter nil 2 3 102 53;click LeftButton 2 3 102 53"));
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::release, .position = QPointF(2, 3), .button = Qt::LeftButton}));
        // A leave says the pointer went, not from where, so it leaves the label the pointer was over through it
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::leave, .position = QPointF(210, 10)}));
        QCOMPARE(got(), qsl("release LeftButton 2 3 102 53;leave"));
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::move, .position = QPointF(2, 3)}));
        QCOMPARE(got(), qsl("enter nil 2 3 102 53;move NoButton 2 3 102 53"));

        // With no double-click callback of its own, a double-click clicks
        QVERIFY(session.pointer(qsl("button"), {.type = Type::doubleClick, .position = QPointF(3, 4), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(got(), qsl("leave;click LeftButton 3 4 503 54"));

        // The GUI's mouse cannot reach a hidden window, nor a point outside one; a leave is taken from either
        QVERIFY(session.runLua(qsl(R"(hideWindow("button"))")));
        QVERIFY(!session.pointer(qsl("button"), {.type = Type::press, .position = QPointF(3, 4), .button = Qt::LeftButton}));
        QVERIFY(session.errorString().contains(qsl("not shown")));
        QVERIFY(session.pointer(qsl("button"), {.type = Type::leave}));
        QVERIFY(!session.pointer(qsl("lbl"), {.type = Type::press, .position = QPointF(80, 5), .button = Qt::LeftButton}));
        QVERIFY(session.errorString().contains(qsl("lbl")));
        QVERIFY(!session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(5, -1)}));
        QCOMPARE(got(), QString());

        // A label deleted and made again under its name is not the one the pointer entered
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(1, 1)}));
        QCOMPARE(got(), qsl("enter nil 1 1 101 51;move NoButton 1 1 101 51"));
        QVERIFY(session.runLua(qsl(R"(deleteLabel("lbl") noteAll("lbl"))")));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(2, 2)}));
        QCOMPARE(got(), qsl("enter nil 2 2 102 52;move NoButton 2 2 102 52"));

        // Qt's press goes to the label it found under the pointer, which deleteLater() keeps, though its enter
        // deleted it and made another under its name
        QVERIFY(session.runLua(qsl(R"lua(
createLabel("swap", 300, 200, 80, 40, 1)
setLabelClickCallback("swap", note("old.click"))
setLabelOnEnter("swap", function()
  deleteLabel("swap")
  createLabel("swap", 300, 200, 80, 40, 1)
  setLabelClickCallback("swap", note("new.click"))
end)
)lua")));
        QVERIFY(session.pointer(qsl("swap"), {.type = Type::press, .position = QPointF(1, 1), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("leave;old.click LeftButton 1 1 301 201"));

        // A label in a scroll box is placed from that box, not from an earlier mini console of the same name
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("dup", 0, 300, 100, 100)
createScrollBox("main", "dup", 200, 300, 100, 100)
createLabel("dup", "inBox", 1, 2, 10, 10, 1)
setLabelClickCallback("inBox", note("click"))
)lua")));
        QVERIFY(session.pointer(qsl("inBox"), {.type = Type::press, .position = QPointF(3, 4), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("click LeftButton 3 4 204 306"));

        QVERIFY(!session.pointer(qsl("nonesuch"), {.type = Type::press}));
        QVERIFY(session.errorString().contains(qsl("nonesuch")));
        QVERIFY(session.shutdown(false));

        // Nor is one of the next run, though the pointer was over its name as the last run ended; it is made as
        // many labels in as that one was, so only the run tells them apart
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(noting));
        QVERIFY(session.runLua(qsl(R"(for i = 1, 4 do createLabel("pad" .. i, 0, 0, 1, 1, 1) end noteAll("lbl"))")));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(3, 3)}));
        QCOMPARE(got(), qsl("enter nil 3 3 103 53;move NoButton 3 3 103 53"));
        QVERIFY(session.shutdown(false));
    }

    void test_aHeldButtonKeepsTheLabelThatTookThePress()
    {
        const QString name = qsl("Headless Label Grab");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY2(session.runLua(qsl(R"lua(
got = {}
local function note(kind)
  return function(event)
    got[#got + 1] = event and string.format("%s %d %d", kind, event.x, event.y) or kind
  end
end
local function noteAll(label, x, y)
  createLabel(label, x, y, 80, 40, 1)
  for _, kind in ipairs({"Click", "Release", "Move"}) do
    _G["setLabel" .. kind .. "Callback"](label, note(label .. "." .. kind:lower()))
  end
  setLabelOnEnter(label, note(label .. ".enter"))
  setLabelOnLeave(label, note(label .. ".leave"))
end
noteAll("drag", 100, 50)
noteAll("other", 180, 50)
noteAll("under", 300, 50)
noteAll("toggle", 300, 50)
setLabelClickCallback("toggle", function()
  got[#got + 1] = "toggle.click"
  hideWindow("toggle")
end)
ac = Adjustable.Container:new({name = "ac", x = 400, y = 300, width = 200, height = 150})
finished = 0
registerAnonymousEventHandler("AdjustableContainerRepositionFinish", function()
  finished = finished + 1
end)
)lua")),
                 qPrintable(session.errorString()));
        const auto got = [&session]() {
            const QString all = luaString(session, qsl("table.concat(got, ';')"));
            session.runLua(qsl("got = {}"));
            return all;
        };

        // As Qt's implicit grab, the label that took the press has the moves and the release wherever they are,
        // and no other label is entered meanwhile
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::press, .position = QPointF(5, 6), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::move, .position = QPointF(90, 5), .buttons = Qt::LeftButton}));
        QVERIFY(!session.pointer(qsl("other"), {.type = Type::move, .position = QPointF(10, 5), .buttons = Qt::LeftButton}));
        QVERIFY(session.errorString().contains(qsl("drag")));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::move, .position = QPointF(-5, 50), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::release, .position = QPointF(90, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("drag.enter 5 6;drag.click 5 6;drag.move 90 5;drag.move -5 50;drag.release 90 5;drag.leave"));
        QVERIFY(session.pointer(qsl("other"), {.type = Type::move, .position = QPointF(10, 5)}));
        QCOMPARE(got(), qsl("other.enter 10 5;other.move 10 5"));
        // With no button held, the point must be in the window again
        QVERIFY(!session.pointer(qsl("drag"), {.type = Type::move, .position = QPointF(85, 5)}));

        // A label its own press hid lost the grab, as Qt drops it, so what the button held goes nowhere, and it has
        // had its leave
        QVERIFY(session.pointer(qsl("toggle"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(got(), qsl("other.leave;toggle.enter 5 5;toggle.click"));
        QVERIFY(session.pointer(qsl("toggle"), {.type = Type::move, .position = QPointF(6, 5), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("toggle"), {.type = Type::release, .position = QPointF(6, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("toggle.leave"));
        QVERIFY(session.pointer(qsl("under"), {.type = Type::move, .position = QPointF(6, 5)}));
        QCOMPARE(got(), qsl("under.enter 6 5;under.move 6 5"));
        // A press holds its own button, whether or not the app gave it among the buttons
        QVERIFY(session.pointer(qsl("other"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("other"), {.type = Type::move, .position = QPointF(-50, 5), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("other"), {.type = Type::release, .position = QPointF(-50, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("under.leave;other.enter 5 5;other.click 5 5;other.move -50 5;other.release -50 5;other.leave"));

        // A Geyser container dragged by its title past the label's edge moves, and its release ends the drag
        const std::optional<THeadlessSession::ScriptWindow> title = session.scriptWindow(qsl("acadjLabel"));
        QVERIFY(title);
        const int width = title->geometry.width();
        QVERIFY(session.pointer(qsl("acadjLabel"), {.type = Type::press, .position = QPointF(width / 2, 20), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("acadjLabel"), {.type = Type::move, .position = QPointF(width + 50, 20), .buttons = Qt::LeftButton}));
        const QString moved = luaString(session, qsl("ac:get_x()"));
        QVERIFY2(moved.toInt() > 400, qPrintable(moved));
        QVERIFY(session.pointer(qsl("acadjLabel"), {.type = Type::release, .position = QPointF(width + 50, 20), .button = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("finished")), qsl("1"));
        // and a hover with no button held no longer drags it
        QVERIFY(session.pointer(qsl("acadjLabel"), {.type = Type::move, .position = QPointF(width / 2 + 30, 20)}));
        QCOMPARE(luaString(session, qsl("ac:get_x()")), moved);
        QVERIFY(session.shutdown(false));
    }

    void test_theGrabEndsAndIsLostOnlyAsQtsIs()
    {
        const QString name = qsl("Headless Label Grab Ends");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY2(session.runLua(qsl(R"lua(
got = {}
local function note(kind)
  return function(event)
    got[#got + 1] = event and string.format("%s %d %d", kind, event.x, event.y) or kind
  end
end
function noteAll(label, x, y)
  createLabel(label, x, y, 80, 40, 1)
  for _, kind in ipairs({"Click", "Release", "Move", "Wheel"}) do
    _G["setLabel" .. kind .. "Callback"](label, note(label .. "." .. kind:lower()))
  end
  setLabelOnEnter(label, note(label .. ".enter"))
  setLabelOnLeave(label, note(label .. ".leave"))
end
noteAll("drag", 100, 50)
noteAll("other", 180, 50)
noteAll("lbl", 100, 150)
createLabel("glass", 100, 150, 300, 40, 1)
enableClickthrough("glass")
noteAll("flip", 100, 250)
setLabelClickCallback("flip", function()
  got[#got + 1] = "flip.click"
  hideWindow("flip")
  showWindow("flip")
end)
noteAll("remade", 300, 250)
setLabelClickCallback("remade", function()
  got[#got + 1] = "remade.click"
  deleteLabel("remade")
  noteAll("remade", 300, 250)
end)
noteAll("again", 500, 250)
setLabelReleaseCallback("again", function()
  got[#got + 1] = "again.release"
  deleteLabel("again")
  noteAll("again", 500, 250)
end)
noteAll("btn", 300, 350)
openUserWindow("uw")
noteAll("moved", 500, 350)
noteAll("slot", 100, 450)
createLabel("overlay", 110, 460, 60, 30, 1)
enableClickthrough("overlay")
setLabelClickCallback("slot", function()
  got[#got + 1] = "slot.click"
  deleteLabel("overlay")
end)
setLabelClickCallback("moved", function()
  got[#got + 1] = "moved.click"
  setWindow("uw", "moved", 0, 0, true)
end)
createScrollBox("box", 600, 450, 150, 80)
createLabel("box", "boxed", 10, 10, 80, 40, 1)
for _, kind in ipairs({"Release", "Move"}) do
  _G["setLabel" .. kind .. "Callback"]("boxed", note("boxed." .. kind:lower()))
end
setLabelOnEnter("boxed", note("boxed.enter"))
setLabelOnLeave("boxed", note("boxed.leave"))
setLabelClickCallback("boxed", function()
  got[#got + 1] = "boxed.click"
  hideWindow("box")
end)
)lua")),
                 qPrintable(session.errorString()));
        const auto got = [&session]() {
            const QString all = luaString(session, qsl("table.concat(got, ';')"));
            session.runLua(qsl("got = {}"));
            return all;
        };

        // While the grab holds, Qt sends no enter or leave, a wheel goes where the pointer is without entering it,
        // and only a release leaving no button held ends it
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::press, .position = QPointF(5, 6), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("other"), {.type = Type::enter, .position = QPointF(10, 5)}));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::leave}));
        QVERIFY(session.pointer(qsl("other"), {.type = Type::wheel, .position = QPointF(10, 5), .buttons = Qt::LeftButton, .angleDelta = QPoint(0, 120)}));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::move, .position = QPointF(-5, 50), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("drag"), {.type = Type::release, .position = QPointF(-5, 50), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("drag.enter 5 6;drag.click 5 6;other.wheel 10 5;drag.move -5 50;drag.release -5 50;drag.leave"));

        // A press through a click-through label is the grab of what took it, which the app may name too
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::move, .position = QPointF(150, 5), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("lbl"), {.type = Type::move, .position = QPointF(160, 5), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::release, .position = QPointF(150, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("lbl.enter 5 5;lbl.click 5 5;lbl.move 150 5;lbl.move 160 5;lbl.release 150 5;lbl.leave"));
        // even after the press's callback deleted it, from where it was at the press
        QVERIFY(session.pointer(qsl("overlay"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY2(session.pointer(qsl("overlay"), {.type = Type::move, .position = QPointF(50, 50), .buttons = Qt::LeftButton}), qPrintable(session.errorString()));
        // and an enter or a leave naming it is taken and runs nothing, but a wheel has no window to turn over
        QVERIFY2(session.pointer(qsl("overlay"), {.type = Type::leave}), qPrintable(session.errorString()));
        QVERIFY2(session.pointer(qsl("overlay"), {.type = Type::enter, .position = QPointF(20, 20)}), qPrintable(session.errorString()));
        QVERIFY(!session.pointer(qsl("overlay"), {.type = Type::wheel, .position = QPointF(20, 20), .buttons = Qt::LeftButton, .angleDelta = QPoint(0, 120)}));
        QCOMPARE(session.errorString(), qsl("There is no window \"overlay\"."));
        QVERIFY2(session.pointer(qsl("overlay"), {.type = Type::release, .position = QPointF(40, 40), .button = Qt::LeftButton}), qPrintable(session.errorString()));
        QCOMPARE(got(), qsl("slot.enter 15 15;slot.click;slot.move 60 60;slot.release 50 50;slot.leave"));

        // A label that hid and showed again in its own press, or was made again, has lost the grab
        QVERIFY(session.pointer(qsl("flip"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("flip"), {.type = Type::release, .position = QPointF(5, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("flip.enter 5 5;flip.click"));
        QVERIFY(session.pointer(qsl("remade"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("remade"), {.type = Type::release, .position = QPointF(5, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("flip.leave;remade.enter 5 5;remade.click"));
        QVERIFY(session.pointer(qsl("moved"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("moved"), {.type = Type::release, .position = QPointF(5, 5), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("moved"), {.type = Type::leave}));
        QCOMPARE(got(), qsl("moved.enter 5 5;moved.click;moved.leave"));
        // but one keeps it as the window it is in hides, as Qt's grab is on the label, which is left as the release
        // ends it
        QVERIFY(session.pointer(qsl("boxed"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("boxed"), {.type = Type::move, .position = QPointF(5, -12), .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("boxed"), {.type = Type::release, .position = QPointF(5, -12), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("boxed.enter 5 5;boxed.click;boxed.move 5 -12;boxed.release 5 -12;boxed.leave"));

        // The label a release's callback made again under the name was never entered, so is not left
        QVERIFY(session.pointer(qsl("again"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("again"), {.type = Type::release, .position = QPointF(90, 5), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("again.enter 5 5;again.click 5 5;again.release"));

        // A label that hid under the pointer has its leave even from a call that is refused
        QVERIFY(session.pointer(qsl("btn"), {.type = Type::move, .position = QPointF(5, 5)}));
        QCOMPARE(got(), qsl("btn.enter 5 5;btn.move 5 5"));
        QVERIFY(session.runLua(qsl(R"(hideWindow("btn"))")));
        QVERIFY(!session.pointer(qsl("btn"), {.type = Type::move, .position = QPointF(6, 5)}));
        QVERIFY(session.errorString().contains(qsl("not shown")));
        QCOMPARE(got(), qsl("btn.leave"));
        QVERIFY(session.shutdown(false));
    }

    void test_consolesRaiseTheGuisMouseEvents()
    {
        const QString name = qsl("Headless Console Pointer");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        const QString making = qsl(R"lua(
got = {}
local function note(event, button, x, y, window)
  got[#got + 1] = string.format("%s %d %d %d %s", event == "sysWindowMousePressEvent" and "press" or "release", button, x, y, window)
end
registerAnonymousEventHandler("sysWindowMousePressEvent", note)
registerAnonymousEventHandler("sysWindowMouseReleaseEvent", note)
createMiniConsole("mc", 10, 20, 200, 100)
for i = 1, 50 do
  echo("mc", "line " .. i .. "\n")
end
openUserWindow("uw")
createLabel("bare", 400, 300, 50, 50, 1)
createLabel("uw", "inUw", 30, 40, 50, 50, 1)
openMapWidget()
createLabel("glass", 600, 400, 50, 50, 1)
enableClickthrough("glass")
createLabel("clicky", 300, 300, 50, 50, 1)
setLabelClickCallback("clicky", function() end)
createLabel("cover", 300, 300, 50, 50, 1)
createScrollBox("sb", 100, 300, 100, 100)
createMiniConsole("sb", "sbmc", 0, 0, 100, 100)
createLabel("overSb", 100, 300, 50, 50, 1)
)lua");
        QVERIFY2(session.runLua(making), qPrintable(session.errorString()));
        const auto got = [&session]() {
            const QString all = luaString(session, qsl("table.concat(got, ';')"));
            session.runLua(qsl("got = {}"));
            return all;
        };

        // TTextEdit gives a mini console's press to the console it is in, and opens its menu for a right-button
        // release, which it does not raise
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::press, .position = QPointF(12, 30), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::release, .position = QPointF(12, 30), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::press, .position = QPointF(12, 30), .button = Qt::MiddleButton, .buttons = Qt::MiddleButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::release, .position = QPointF(12, 30), .button = Qt::MiddleButton}));
        QCOMPARE(got(), qsl("press 2 22 50 main;press 3 22 50 main;release 3 22 50 main"));
        // as on a user window's text and the main console's, wherever the grab has the release, and with no press
        // before it
        QVERIFY(session.pointer(qsl("uw"), {.type = Type::press, .position = QPointF(5, 6), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("uw"), {.type = Type::release, .position = QPointF(-500, 6), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(7, 8), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::release, .position = QPointF(7, 8), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::release, .position = QPointF(7, 8), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("press 2 5 6 uw;press 2 7 8 main"));
        // The main console's borders and a scroll box's own area have no menu, so theirs is raised, over the text
        // too where the grab has it, and one the text's grab has over a border is not
        QVERIFY(session.runLua(qsl("setBorderTop(40)")));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(7, 8), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::release, .position = QPointF(7, 100), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(7, 100), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::release, .position = QPointF(7, 8), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("sb"), {.type = Type::press, .position = QPointF(60, 60), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("sb"), {.type = Type::release, .position = QPointF(60, 60), .button = Qt::RightButton}));
        QVERIFY(session.runLua(qsl("setBorderTop(0)")));
        QCOMPARE(got(), qsl("press 2 7 8 main;release 2 7 100 main;press 2 7 100 main;press 2 160 360 main;release 2 160 360 main"));
        QVERIFY(session.pointer(qsl("uw"), {.type = Type::press, .position = QPointF(5, 6), .button = Qt::ExtraButton24, .buttons = Qt::ExtraButton24}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::doubleClick, .position = QPointF(7, 8), .button = Qt::MiddleButton, .buttons = Qt::MiddleButton}));
        QCOMPARE(got(), qsl("press 27 5 6 uw;press 3 7 8 main"));
        // A label with no callback for it leaves the press to the console under it
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("inUw"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("inUw"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::move, .position = QPointF(1, 2)}));
        QCOMPARE(got(), qsl("press 1 401 302 main;release 1 401 302 main;press 1 31 42 uw;release 1 31 42 uw"));
        // The console that took the press has the release wherever it is, as Qt's implicit grab gives it
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::press, .position = QPointF(12, 30), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::release, .position = QPointF(250, 30), .button = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(7, 8), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::release, .position = QPointF(900, 10), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("press 1 22 50 main;release 1 260 50 main;press 1 7 8 main;release 1 900 10 main"));
        // A click-through label with nothing under it in the main window's area leaves the press to the main
        // console, not to a dock
        QVERIFY(session.pointer(qsl("glass"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(got(), qsl("press 1 601 402 main"));
        // A right-button release opens the menu of the console's text the bare label covers, and is not raised
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("inUw"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("press 2 401 302 main"));
        // but where a label with a click callback is what it covers there, the release goes on
        QVERIFY(session.pointer(qsl("cover"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("release 2 301 302 main"));
        // A release the grab took away from the label is tested where it is: over a label with a click callback
        // that a bare one covers it goes on, and over the main console's text the menu opens
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::release, .position = QPointF(-99, 2), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("bare"), {.type = Type::release, .position = QPointF(-30, 2), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("press 2 401 302 main;release 2 301 302 main;press 2 401 302 main"));
        // A scroll box's mini console under a bare label opens its menu
        QVERIFY(session.pointer(qsl("overSb"), {.type = Type::press, .position = QPointF(10, 10), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("overSb"), {.type = Type::release, .position = QPointF(10, 10), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("press 2 110 310 main"));

        // As the Lua window functions, no name is the main console too, and the name is as it is spelt
        QVERIFY(session.pointer(QString(), {.type = Type::press, .position = QPointF(7, 8), .button = Qt::MiddleButton, .buttons = Qt::MiddleButton}));
        QCOMPARE(got(), qsl("press 3 7 8 main"));
        QVERIFY(!session.pointer(qsl("Main"), {.type = Type::press, .position = QPointF(7, 8), .button = Qt::MiddleButton}));
        QVERIFY(session.errorString().contains(qsl("Main")));
        QVERIFY(!session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(800, 8), .button = Qt::MiddleButton}));
        QCOMPARE(got(), QString());

        const int end = luaString(session, qsl("getScroll('mc')")).toInt();
        QVERIFY(end > 3);
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, 120)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), end - 3);
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .modifiers = Qt::ShiftModifier, .angleDelta = QPoint(0, 120)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), end - 4);
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, -240)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), end);
        QCOMPARE(got(), QString());
        // Too little to scroll a line, which a console made again under the name does not inherit
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, 16)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), end);
        QVERIFY(session.runLua(qsl(R"(deleteMiniConsole("mc") createMiniConsole("mc", 10, 20, 200, 100) for i = 1, 50 do echo("mc", "line " .. i .. "\n") end)")));
        const int remade = luaString(session, qsl("getScroll('mc')")).toInt();
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, 8)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), remade);
        QVERIFY(session.shutdown(false));

        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY2(session.runLua(making), qPrintable(session.errorString()));
        const int again = luaString(session, qsl("getScroll('mc')")).toInt();
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .angleDelta = QPoint(0, 8)}));
        QCOMPARE(luaString(session, qsl("getScroll('mc')")).toInt(), again);
        QVERIFY(session.shutdown(false));
    }

    void test_consolesKeepAndLoseTheGrabAsQtsDo()
    {
        const QString name = qsl("Headless Console Grab");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY2(session.runLua(qsl(R"lua(
got = {}
local function note(event, button, x, y, window)
  got[#got + 1] = string.format("%s %d %d %d %s", event == "sysWindowMousePressEvent" and "press" or "release", button, x, y, window)
end
registerAnonymousEventHandler("sysWindowMousePressEvent", note)
registerAnonymousEventHandler("sysWindowMouseReleaseEvent", note)
openUserWindow("uw")
createLabel("uw", "inUw", 30, 40, 50, 50, 1)
createLabel("throughClicky", 500, 100, 50, 50, 1)
setLabelClickCallback("throughClicky", function() end)
enableClickthrough("throughClicky")
createLabel("overThrough", 500, 100, 50, 50, 1)
createLabel("hiddenClicky", 600, 100, 50, 50, 1)
setLabelClickCallback("hiddenClicky", function() end)
hideWindow("hiddenClicky")
createLabel("overHidden", 600, 100, 50, 50, 1)
createMiniConsole("mc", 10, 20, 200, 100)
createMiniConsole("mc2", 10, 400, 200, 100)
createMiniConsole("mc3", 250, 400, 100, 50)
createMiniConsole("mc4", 400, 400, 100, 50)
createMiniConsole("mc5", 250, 520, 100, 50)
createScrollBox("sb", 550, 400, 100, 100)
createScrollBox("sb2", 700, 400, 100, 100)
createScrollBox("sb3", 850, 400, 100, 100)
-- What the next press raised does to the window it came from
remake = nil
registerAnonymousEventHandler("sysWindowMousePressEvent", function()
  local what = remake
  remake = nil
  if what == "mc2" then
    deleteMiniConsole("mc2")
    createMiniConsole("mc2", 10, 400, 200, 100)
  elseif what == "sb2" then
    deleteScrollBox("sb2")
    createScrollBox("sb2", 700, 400, 100, 100)
  elseif what == "mc4" or what == "sb3" then
    setWindow("uw", what, 0, 0, true)
  elseif what == "mc5" then
    hideWindow(what)
  elseif what then
    hideWindow(what)
    showWindow(what)
  end
end)
createLabel("hover", 300, 300, 50, 50, 1)
setLabelClickCallback("hover", function() end)
setLabelReleaseCallback("hover", function() end)
setLabelOnEnter("hover", function() got[#got + 1] = "hover.enter" end)
setLabelOnLeave("hover", function() got[#got + 1] = "hover.leave" end)
)lua")),
                 qPrintable(session.errorString()));
        const auto got = [&session]() {
            const QString all = luaString(session, qsl("table.concat(got, ';')"));
            session.runLua(qsl("got = {}"));
            return all;
        };

        // A mini console or scroll box made again in what its press raised has lost the grab, so its release goes
        // nowhere, but one hidden, even to show again, or moved keeps it, as Qt's grab is on a widget inside it, and
        // its release goes to the window it is now in
        for (const QString& window : {qsl("mc2"), qsl("mc3"), qsl("mc4"), qsl("mc5"), qsl("sb"), qsl("sb2"), qsl("sb3")}) {
            QVERIFY(session.runLua(qsl("remake = '%1'").arg(window)));
            QVERIFY(session.pointer(window, {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
            QVERIFY(session.pointer(window, {.type = Type::release, .position = QPointF(300, 5), .button = Qt::LeftButton}));
        }
        QCOMPARE(got(),
                 qsl("press 1 15 405 main;press 1 255 405 main;release 1 550 405 main;press 1 405 405 main;release 1 300 5 uw;press 1 255 525 main;release 1 550 525 main;"
                     "press 1 555 405 main;release 1 850 405 main;press 1 705 405 main;press 1 855 405 main;release 1 300 5 uw"));

        // A wheel over a console while a label has the grab does not leave that label
        QVERIFY(session.pointer(qsl("hover"), {.type = Type::press, .position = QPointF(1, 1), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::wheel, .position = QPointF(1, 2), .buttons = Qt::LeftButton, .angleDelta = QPoint(0, 120)}));
        QVERIFY(session.pointer(qsl("hover"), {.type = Type::release, .position = QPointF(1, 1), .button = Qt::LeftButton}));
        QCOMPARE(got(), qsl("hover.enter"));

        // The menu's hit test passes over a click-through label and a hidden one, as QWidget::childAt() does
        QVERIFY(session.pointer(qsl("overThrough"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("overThrough"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("overHidden"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("overHidden"), {.type = Type::release, .position = QPointF(1, 2), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("hover.leave;press 2 501 102 main;press 2 601 102 main"));
        // and finds nothing outside the window the label is in, so a release the grab took there goes on
        QVERIFY(session.pointer(qsl("inUw"), {.type = Type::press, .position = QPointF(1, 2), .button = Qt::RightButton, .buttons = Qt::RightButton}));
        QVERIFY(session.pointer(qsl("inUw"), {.type = Type::release, .position = QPointF(1000, 2), .button = Qt::RightButton}));
        QCOMPARE(got(), qsl("press 2 31 42 uw;release 2 1030 42 uw"));
        QVERIFY(session.shutdown(false));
    }

    void test_getMousePositionIsWhereTheAppsPointerLastWas()
    {
        const QString name = qsl("Headless Mouse Position");
        QVERIFY(writeProfile(name));
        using Type = THeadlessSession::WindowPointerEvent::Type;
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"(createMiniConsole("mc", 10, 20, 200, 100))")));
        QCOMPARE(luaString(session, qsl("getMousePosition()")), qsl("nil"));
        QCOMPARE(luaString(session, qsl("select(2, getMousePosition())")), qsl("the profile has no main window"));

        QVERIFY(session.pointer(qsl("mc"), {.type = Type::move, .position = QPointF(5, 5)}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("15,25"));
        QVERIFY(session.pointer(qsl("main"), {.type = Type::press, .position = QPointF(300, 400), .button = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("300,400"));
        // A leave says the pointer went, not where
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::leave}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("300,400"));
        // A drag goes on past the window that took the press
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::move, .position = QPointF(250, 5), .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("260,25"));
        // and on, as the pointer does, after the window has lost the grab
        QVERIFY(session.runLua(qsl(R"(deleteMiniConsole("mc") createMiniConsole("mc", 10, 20, 200, 100))")));
        QVERIFY(session.pointer(qsl("mc"), {.type = Type::move, .position = QPointF(30, 5), .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("40,25"));
        // from where the window was at the press, after its own click callback deleted it
        QVERIFY(session.runLua(qsl(R"(createLabel("item", 100, 50, 80, 40, 1) setLabelClickCallback("item", function() deleteLabel("item") end))")));
        QVERIFY(session.pointer(qsl("item"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("105,55"));
        QVERIFY(session.pointer(qsl("item"), {.type = Type::move, .position = QPointF(300, 200), .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("400,250"));
        QVERIFY(session.pointer(qsl("item"), {.type = Type::release, .position = QPointF(310, 210), .button = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("410,260"));
        // or from where the click-through label the press went through was, after the press deleted both
        QVERIFY(session.runLua(qsl(R"(createLabel("u", 100, 300, 80, 40, 1) createLabel("g", 110, 310, 60, 30, 1) enableClickthrough("g")
setLabelClickCallback("u", function() deleteLabel("u") deleteLabel("g") end))")));
        QVERIFY(session.pointer(qsl("g"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("115,315"));
        QVERIFY(session.pointer(qsl("g"), {.type = Type::move, .position = QPointF(50, 50), .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("160,360"));
        QVERIFY(session.pointer(qsl("u"), {.type = Type::move, .position = QPointF(70, 70), .buttons = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("170,370"));
        QVERIFY(session.pointer(qsl("g"), {.type = Type::release, .position = QPointF(40, 40), .button = Qt::LeftButton}));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("150,350"));
        // and from there too, after the press deleted only the click-through label, as what took it keeps the grab
        QVERIFY(session.runLua(qsl(R"(createLabel("u", 100, 300, 80, 40, 1) createLabel("g", 110, 310, 60, 30, 1) enableClickthrough("g")
setLabelClickCallback("u", function() deleteLabel("g") end))")));
        QVERIFY(session.pointer(qsl("g"), {.type = Type::press, .position = QPointF(5, 5), .button = Qt::LeftButton, .buttons = Qt::LeftButton}));
        QVERIFY2(session.pointer(qsl("g"), {.type = Type::move, .position = QPointF(30, 20), .buttons = Qt::LeftButton}), qPrintable(session.errorString()));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("140,330"));
        // An enter naming it is a point the pointer took, as for any window while the grab holds, and a leave is not
        QVERIFY(session.runLua(qsl(R"(seen = {} setLabelOnEnter("u", function() seen[#seen + 1] = "u.enter" end)
setLabelOnLeave("u", function() seen[#seen + 1] = "u.leave" end) setLabelWheelCallback("u", function() seen[#seen + 1] = "u.wheel" end))")));
        QVERIFY2(session.pointer(qsl("g"), {.type = Type::leave}), qPrintable(session.errorString()));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("140,330"));
        QVERIFY2(session.pointer(qsl("g"), {.type = Type::enter, .position = QPointF(25, 15)}), qPrintable(session.errorString()));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("135,325"));
        QVERIFY(!session.pointer(qsl("g"), {.type = Type::wheel, .position = QPointF(20, 20), .buttons = Qt::LeftButton, .angleDelta = QPoint(0, 120)}));
        QCOMPARE(session.errorString(), qsl("There is no window \"g\"."));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("135,325"));
        QCOMPARE(luaString(session, qsl("table.concat(seen, ';')")), QString());
        QVERIFY2(session.pointer(qsl("g"), {.type = Type::release, .position = QPointF(20, 10), .button = Qt::LeftButton}), qPrintable(session.errorString()));
        QCOMPARE(luaString(session, qsl("table.concat({getMousePosition()}, ',')")), qsl("130,320"));
        QVERIFY(session.shutdown(false));
    }

    void test_aConsolesScrollIsReadAndTold()
    {
        const QString name = qsl("Headless Windows Scroll");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
createLabel("lbl", 0, 300, 100, 20, 1)
for i = 1, 40 do echo("mini", "line " .. i .. "\n") end
)lua")));
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        const auto mini = [&session]() {
            return *session.scriptWindow(qsl("mini"));
        };
        const auto luaScroll = [&session]() {
            return luaString(session, qsl("getScroll('mini')")).toInt();
        };
        const QSize whole = mini().grid;
        QVERIFY(whole.width() > 0 && whole.height() > 0);
        QCOMPARE(whole.width(), luaString(session, qsl("getColumnCount('mini')")).toInt());
        QCOMPARE(whole.height(), luaString(session, qsl("getRowCount('mini')")).toInt());
        QVERIFY(!mini().scrolledBackTo);
        QVERIFY(!mini().scrollBar);
        QVERIFY(!mini().timeStamps);
        // The other kinds have none of it
        QVERIFY(!session.scriptWindow(qsl("lbl"))->scrolledBackTo);
        QCOMPARE(session.scriptWindow(qsl("lbl"))->grid, QSize());

        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        const auto told = [&changes]() {
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            QStringList all;
            for (const QList<QVariant>& change : std::as_const(changes)) {
                all << change.at(1).toStringList().join(QLatin1Char(','));
            }
            changes.clear();
            return all.join(QLatin1Char(';'));
        };

        // Scrolled back, the GUI splits the console, so its upper pane has fewer rows
        QVERIFY(session.runLua(qsl("scrollTo('mini', 3)")));
        QCOMPARE(told(), qsl("mini"));
        QCOMPARE(mini().scrolledBackTo, std::optional<int>(3));
        QCOMPARE(*mini().scrolledBackTo, luaScroll());
        QVERIFY(mini().grid.height() < whole.height());
        QCOMPARE(mini().grid.height(), luaString(session, qsl("getRowCount('mini')")).toInt());
        QVERIFY(session.runLua(qsl("scrollTo('mini', 3)")));
        QCOMPARE(told(), QString());
        QVERIFY(session.runLua(qsl("scrollTo('mini', 7)")));
        QCOMPARE(told(), qsl("mini"));
        QCOMPARE(mini().scrolledBackTo, std::optional<int>(7));

        // Back to following new lines
        QVERIFY(session.runLua(qsl("scrollTo('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!mini().scrolledBackTo);
        QCOMPARE(mini().grid, whole);

        // The app's wheel
        using Type = THeadlessSession::WindowPointerEvent::Type;
        QVERIFY(session.pointer(qsl("mini"), {.type = Type::wheel, .position = QPointF(20, 20), .angleDelta = QPoint(0, 120)}));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(mini().scrolledBackTo);
        QCOMPARE(*mini().scrolledBackTo, luaScroll());
        QCOMPARE(*mini().scrolledBackTo, luaString(session, qsl("getLineCount('mini')")).toInt() - 3);

        // A clear closes the split
        QVERIFY(session.runLua(qsl("clearWindow('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!mini().scrolledBackTo);
        QCOMPARE(mini().grid, whole);

        // A trim of the oldest rows moves the row it is scrolled back to, as a script reads it
        QVERIFY(session.runLua(qsl("setConsoleBufferSize('mini', 100, 10) for i = 1, 95 do echo('mini', 'row ' .. i .. '\\n') end scrollTo('mini', 50)")));
        QCOMPARE(told(), qsl("mini"));
        QCOMPARE(mini().scrolledBackTo, std::optional<int>(50));
        QVERIFY(session.runLua(qsl("for i = 96, 120 do echo('mini', 'row ' .. i .. '\\n') end")));
        QVERIFY(luaString(session, qsl("getLineCount('mini')")).toInt() < 120);
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(mini().scrolledBackTo);
        QVERIFY(*mini().scrolledBackTo < 50);
        QCOMPARE(*mini().scrolledBackTo, luaScroll());
        // and what a deleteLine() leaves it past is bounded as a script's is
        QVERIFY(session.runLua(qsl("scrollTo('mini', getLineCount('mini') - 1) for i = 1, 4 do moveCursor('mini', 0, getLineCount('mini')) deleteLine('mini') end")));
        told();
        QVERIFY(mini().scrolledBackTo);
        QCOMPARE(*mini().scrolledBackTo, luaScroll());

        // Rows deleted from the end take the row it is scrolled back to with them
        QVERIFY(session.runLua(qsl("scrollTo('mini', getLineCount('mini') - 1)")));
        told();
        const int beforeDeleting = *mini().scrolledBackTo;
        QVERIFY(session.runLua(qsl("for i = 1, 4 do moveCursor('mini', 0, getLineCount('mini')) deleteLine('mini') end")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(*mini().scrolledBackTo < beforeDeleting);
        QCOMPARE(*mini().scrolledBackTo, luaScroll());

        // A console told not to scroll follows new lines
        QVERIFY(session.runLua(qsl("disableScrolling('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!mini().scrolledBackTo);
        QVERIFY(session.runLua(qsl("enableScrolling('mini')")));
        QCOMPARE(told(), QString());

        QVERIFY(session.runLua(qsl("enableScrollBar('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(mini().scrollBar);
        QVERIFY(session.runLua(qsl("enableScrollBar('mini')")));
        QCOMPARE(told(), QString());
        QVERIFY(session.runLua(qsl("disableScrollBar('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!mini().scrollBar);

        QVERIFY(session.runLua(qsl("enableTimeStamps('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(mini().timeStamps);
        QVERIFY(session.runLua(qsl("disableTimeStamps('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!mini().timeStamps);
        QVERIFY(session.shutdown(false));
    }

    void test_theCommandLinesScriptsMakeAreTypedInAndSent()
    {
        const QString name = qsl("Headless Windows Command Lines");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.windowCommandLineText(qsl("own")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
enableCommandLine("mini")
createCommandLine("own", 0, 300, 200, 30)
headlessSent, headlessActed = {}, {}
registerAnonymousEventHandler("sysDataSendRequest", function(_, command) headlessSent[#headlessSent + 1] = command end)
)lua")));
        const auto sent = [&session]() {
            return luaString(session, qsl("table.concat(headlessSent, '|')"));
        };
        const auto miniRows = [&session]() {
            return luaString(session, qsl("table.concat(getLines('mini', 0, getLineCount('mini')), '|')"));
        };
        // The notifier's queued call, then the session's queued relay of it
        const auto settle = []() {
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
        };
        // As the profile saves it
        const auto savedHistory = [&session](const QString& line) {
            Host* pHost = session.host();
            emit pHost->signal_saveCommandLinesHistory();
            QStringList history;
            QString error;
            TCommandHistory::read(MudletApp::getMudletPath(enums::profileDataItemPath, pHost->getName(), std::get<0>(pHost->getCmdLineSettings(enums::SubCommandLine, line))), history, error);
            return history;
        };
        QVERIFY(indexOf(session.scriptWindows(), qsl("own"), Kind::commandLine) >= 0);
        QVERIFY(indexOf(session.scriptWindows(), qsl("mini"), Kind::commandLine) >= 0);
        for (const QString& other : {QString(), qsl("main"), qsl("nope")}) {
            QVERIFY(!session.windowCommandLineText(other));
            QVERIFY(!session.setWindowCommandLineText(other, qsl("x")));
            QVERIFY(session.errorString().contains(qsl("command line")));
            QVERIFY(!session.submitWindowCommandLine(other, qsl("x")));
        }
        QCOMPARE(sent(), QString());
        QCOMPARE(session.windowCommandLineText(qsl("own")), std::optional<QString>(QString()));

        QSignalSpy changed(&session, &THeadlessSession::windowCommandLineTextChanged);
        QVERIFY(session.setWindowCommandLineText(qsl("own"), qsl("typed")));
        QCOMPARE(luaString(session, qsl("getCmdLine('own')")), qsl("typed"));
        QCOMPARE(luaString(session, qsl("getCmdLine()")), QString());
        settle();
        QCOMPARE(changed.count(), 0);

        QVERIFY(session.runLua(qsl("printCmdLine('own', 'a') appendCmdLine('own', 'b') printCmdLine('mini', 'm') printCmdLine('mini', 'm')")));
        QCOMPARE(session.windowCommandLineText(qsl("own")), std::optional<QString>(qsl("ab")));
        QCOMPARE(changed.count(), 0);
        settle();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).toString(), name);
        QCOMPARE(changed.at(0).at(1).toStringList(), QStringList({qsl("mini"), qsl("own")}));
        changed.clear();

        // Each line through the aliases to the game; the profile keeps a sent line unless told to clear it
        QVERIFY(session.submitWindowCommandLine(qsl("own"), qsl("north\nsouth")));
        QCOMPARE(sent(), qsl("north|south"));
        QCOMPARE(session.windowCommandLineText(qsl("own")), std::optional<QString>(qsl("north\nsouth")));
        // The whole line joins that line's own history, but not while the game hides what is typed; the file has a
        // line of its own for each of the line's lines, after the blank one the history starts with
        QCOMPARE(savedHistory(qsl("own")).join(qsl("|")), qsl("|north|south"));
        session.host()->setRemoteEchoingActive(true);
        QVERIFY(session.submitWindowCommandLine(qsl("own"), qsl("secret")));
        session.host()->setRemoteEchoingActive(false);
        QCOMPARE(savedHistory(qsl("own")).join(qsl("|")), qsl("|north|south"));
        QVERIFY(session.setWindowCommandLineText(qsl("own"), qsl("north\nsouth")));
        QCOMPARE(sent(), qsl("north|south|secret"));
        QCOMPARE(miniRows(), QString());
        // A console's own line echoes what it sent in its console
        QVERIFY(session.submitWindowCommandLine(qsl("mini"), qsl("look")));
        QCOMPARE(sent(), qsl("north|south|secret|look"));
        QCOMPARE(miniRows(), qsl("look"));
        settle();
        QCOMPARE(changed.count(), 0);

        session.host()->mAutoClearCommandLineAfterSend = true;
        QVERIFY(session.runLua(qsl(R"lua(
local function act(text) headlessActed[#headlessActed + 1] = text .. "=" .. getCmdLine("own") end
setCmdLineAction("own", act)
setCmdLineAction("mini", act)
)lua")));
        QVERIFY(session.submitWindowCommandLine(qsl("own"), qsl("one\ntwo")));
        QVERIFY(session.submitWindowCommandLine(qsl("mini"), qsl("three")));
        QCOMPARE(luaString(session, qsl("table.concat(headlessActed, '|')")), qsl("one=one\ntwo|two=one\ntwo|three="));
        QCOMPARE(sent(), qsl("north|south|secret|look"));
        QCOMPARE(miniRows(), qsl("look"));
        QCOMPARE(session.windowCommandLineText(qsl("own")), std::optional<QString>(QString()));
        QCOMPARE(session.windowCommandLineText(qsl("mini")), std::optional<QString>(QString()));
        settle();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(1).toStringList(), QStringList({qsl("mini"), qsl("own")}));

        // The app's line may hold text Mudlet's copy has not, so a write that leaves the copy as it was is told too
        changed.clear();
        QVERIFY(session.runLua(qsl("clearCmdLine('own')")));
        settle();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(1).toStringList(), QStringList({qsl("own")}));

        // A player cannot type on a line that is not shown, so neither can the app; a script still writes to it
        const QString sentBefore = sent();
        const QStringList hiders = {qsl("disableCommandLine('mini')"), qsl("hideWindow('mini')"), qsl("hideWindow('own')")};
        const QStringList showers = {qsl("enableCommandLine('mini')"), qsl("showWindow('mini')"), qsl("showWindow('own')")};
        const QStringList hidden = {qsl("mini"), qsl("mini"), qsl("own")};
        for (qsizetype i = 0; i < hiders.size(); ++i) {
            QVERIFY(session.runLua(hiders.at(i)));
            QVERIFY2(!session.setWindowCommandLineText(hidden.at(i), qsl("unseen")), qPrintable(hiders.at(i)));
            QCOMPARE(session.errorString(), qsl("The command line \"%1\" is not shown, so nothing can be typed on it.").arg(hidden.at(i)));
            QVERIFY2(!session.submitWindowCommandLine(hidden.at(i), qsl("unseen")), qPrintable(hiders.at(i)));
            QCOMPARE(session.windowCommandLineText(hidden.at(i)), std::optional<QString>(QString()));
            QVERIFY(session.runLua(qsl("printCmdLine('%1', 'by a script')").arg(hidden.at(i))));
            QCOMPARE(session.windowCommandLineText(hidden.at(i)), std::optional<QString>(qsl("by a script")));
            QVERIFY(session.runLua(showers.at(i)));
            QVERIFY2(session.setWindowCommandLineText(hidden.at(i), QString()), qPrintable(showers.at(i)));
        }
        QCOMPARE(sent(), sentBefore);
        QCOMPARE(luaString(session, qsl("table.concat(headlessActed, '|')")), qsl("one=one\ntwo|two=one\ntwo|three="));

        // A line its own function deletes takes no more commands
        QVERIFY(session.runLua(qsl("setCmdLineAction('own', function(text) headlessActed = {text} deleteCommandLine('own') end)")));
        QVERIFY(session.submitWindowCommandLine(qsl("own"), qsl("first\nsecond")));
        QCOMPARE(luaString(session, qsl("table.concat(headlessActed, '|')")), qsl("first"));
        QVERIFY(!session.windowCommandLineText(qsl("own")));
        // Nor does one it makes again under the name, which has no function to take them
        QVERIFY(session.runLua(qsl(R"lua(
createCommandLine("own", 0, 300, 200, 30)
setCmdLineAction("own", function(text) headlessActed = {text} deleteCommandLine("own") createCommandLine("own", 0, 300, 200, 30) end)
)lua")));
        const QString sentBeforeReplacement = sent();
        QVERIFY(session.submitWindowCommandLine(qsl("own"), qsl("first\nsecond")));
        QCOMPARE(luaString(session, qsl("table.concat(headlessActed, '|')")), qsl("first"));
        QCOMPARE(sent(), sentBeforeReplacement);
        QCOMPARE(session.windowCommandLineText(qsl("own")), std::optional<QString>(QString()));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionWindowsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionWindowsTest)
