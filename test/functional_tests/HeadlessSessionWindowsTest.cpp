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
};

#include "HeadlessSessionWindowsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionWindowsTest)
