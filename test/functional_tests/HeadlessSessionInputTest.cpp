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
 * The controls of the GUI's main window and console toolbar that THeadlessSession gives an app that embeds
 * Mudlet. Each test makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionInputTest -V
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
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

class HeadlessSessionInputTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name, const QString& script = QString())
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessInput</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped())
                               .toUtf8());
        return true;
    }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
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

    void test_theEmergencyStopHoldsAliasesAndEvents()
    {
        const QString name = qsl("Headless Emergency Stop");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessAliasRuns = 0
headlessEventRuns = 0
tempAlias("^ping$", [[headlessAliasRuns = headlessAliasRuns + 1]])
registerAnonymousEventHandler("headlessPing", function() headlessEventRuns = headlessEventRuns + 1 end)
)lua")));
        QVERIFY(!session.scriptsStopped());

        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.scriptsStopped());
        QVERIFY(session.host()->mEmergencyStop);
        QVERIFY(session.submit(qsl("ping")));
        QVERIFY(session.runLua(qsl("raiseEvent('headlessPing')")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRuns"), qsl("0"));
        QCOMPARE(luaGlobalString(session.host(), "headlessEventRuns"), qsl("0"));
        // The state it is in already
        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.scriptsStopped());

        QVERIFY(session.setScriptsStopped(false));
        QVERIFY(!session.scriptsStopped());
        QVERIFY(session.submit(qsl("ping")));
        QVERIFY(session.runLua(qsl("raiseEvent('headlessPing')")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRuns"), qsl("1"));
        QCOMPARE(luaGlobalString(session.host(), "headlessEventRuns"), qsl("1"));
        QVERIFY(session.setScriptsStopped(false));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's button: lifting the stop turns on what it turned off, and nothing a script had turned off
    void test_whatAScriptTurnedOffStaysOffAfterTheEmergencyStop()
    {
        const QString name = qsl("Headless Emergency Stop Kept Off");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessRuns = ""
disableAlias(tostring(tempAlias("^ping$", [[headlessRuns = headlessRuns .. "off alias "]])))
tempAlias("^ding$", [[headlessRuns = headlessRuns .. "alias "]])
disableTrigger(tostring(tempTrigger("pong", [[headlessRuns = headlessRuns .. "off trigger "]])))
tempTrigger("zing", [[headlessRuns = headlessRuns .. "trigger "]])
)lua")));
        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.setScriptsStopped(false));
        QVERIFY(session.submit(qsl("ping")));
        QVERIFY(session.submit(qsl("ding")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("pong\nzing\n"))lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessRuns"), qsl("alias trigger "));
        QVERIFY(session.shutdown(false));
    }

    // What is made or turned on while stopped is live, as a script's temporary items are, so stopping again must
    // turn it off
    void test_theEmergencyStopAppliedAgainStopsWhatCameOnSince()
    {
        const QString name = qsl("Headless Emergency Stop Again");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.runLua(qsl(R"lua(
headlessRuns = ""
tempAlias("^ping$", [[headlessRuns = headlessRuns .. "alias "]])
tempTrigger("pong", [[headlessRuns = headlessRuns .. "trigger "]])
)lua")));
        QVERIFY(session.submit(qsl("ping")));
        QCOMPARE(luaGlobalString(session.host(), "headlessRuns"), qsl("alias "));

        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.scriptsStopped());
        QVERIFY(session.submit(qsl("ping")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("pong\n"))lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessRuns"), qsl("alias "));
        QVERIFY(session.setScriptsStopped(false));
        QVERIFY(session.submit(qsl("ping")));
        QCOMPARE(luaGlobalString(session.host(), "headlessRuns"), qsl("alias alias "));
        QVERIFY(session.shutdown(false));
    }

    void test_theEmergencyStopIsRefusedMidWriteAndWithNoProfile()
    {
        THeadlessSession unstarted;
        QVERIFY(!unstarted.setScriptsStopped(true));
        QCOMPARE(unstarted.errorString(), qsl("No profile is open."));
        QVERIFY(!unstarted.scriptsStopped());

        const QString name = qsl("Headless Emergency Stop Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        std::optional<bool> stoppedMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &stoppedMidWrite](const QString&, const QString& text) {
            if (text == qsl("stop now")) {
                stoppedMidWrite = session.setScriptsStopped(true);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("stop now\n"))lua")));
        QCOMPARE(stoppedMidWrite, std::optional<bool>(false));
        QVERIFY(session.errorString().startsWith(qsl("Refused from a handler of outputLine")));
        QVERIFY(!session.scriptsStopped());
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.setScriptsStopped(false));
        QVERIFY(!session.scriptsStopped());
    }

    void test_theAppComingToTheFrontIsToldToEveryProfile()
    {
        const QString script = qsl(R"lua(
headlessFocus = ""
registerAnonymousEventHandler("sysApplicationFocusChangeEvent", function(_, active)
  headlessFocus = headlessFocus .. tostring(active) .. " "
end)
)lua");
        const QString name = qsl("Headless App Active");
        const QString other = qsl("Headless App Active Other");
        QVERIFY(writeProfile(name, script));
        QVERIFY(writeProfile(other, script));
        THeadlessSession session;
        QVERIFY(session.applicationActive());
        // Only kept, with no profile to tell
        QVERIFY(session.setApplicationActive(false));
        QVERIFY(!session.applicationActive());
        QVERIFY(session.setApplicationActive(true));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), QString());

        QVERIFY(session.setApplicationActive(false));
        QVERIFY(!session.applicationActive());
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), qsl("false "));
        QVERIFY(session.setApplicationActive(false));
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), qsl("false "));
        QVERIFY(session.setApplicationActive(true));
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), qsl("false true "));

        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(session.setApplicationActive(false));
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), qsl("false true false "));
        QCOMPARE(luaGlobalString(pOther, "headlessFocus"), qsl("false "));
        QVERIFY(session.shutdown(false));
    }

    // The handler's saveJsonMap() runs the event loop, where the app comes to the front and leaves it again
    void test_theAppLeavingAndComingBackUnderAHandlerIsToldOnce()
    {
        const QString name = qsl("Headless App Active Nested");
        const QString other = qsl("Headless App Active Nested Other");
        const QString script = qsl(R"lua(
headlessFocus = ""
assert(openMapWidget())
registerAnonymousEventHandler("sysApplicationFocusChangeEvent", function(_, active)
  headlessFocus = headlessFocus .. tostring(active) .. " "
  if headlessPump then
    local path = headlessPump
    headlessPump = nil
    saveJsonMap(path)
  end
end)
)lua");
        QVERIFY(writeProfile(name, script));
        QVERIFY(writeProfile(other, script));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        for (Host* pHost : {session.host(), pOther}) {
            const QString path = mConfigDir.filePath(qsl("%1.json").arg(pHost->getName()));
            QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessPump = [[%1]]").arg(path)));
        }
        std::optional<std::pair<bool, bool>> nested;
        QTimer::singleShot(0, this, [&session, &nested]() {
            const bool cameBack = session.setApplicationActive(true);
            nested = {cameBack, session.setApplicationActive(false)};
        });

        QVERIFY(session.setApplicationActive(false));
        QCOMPARE(nested, (std::optional<std::pair<bool, bool>>({true, true})));
        QVERIFY(!session.applicationActive());
        // The profile told first hears all three; the other only the two the handler's loop sent it
        QStringList heard{luaGlobalString(session.host(), "headlessFocus"), luaGlobalString(pOther, "headlessFocus")};
        heard.sort();
        QCOMPARE(heard, (QStringList{qsl("false true false "), qsl("true false ")}));
        QVERIFY(session.shutdown(false));
    }

    void test_theAppComingToTheFrontIsRefusedMidWrite()
    {
        const QString name = qsl("Headless App Active Mid Write");
        QVERIFY(writeProfile(name, qsl(R"lua(
headlessFocus = ""
registerAnonymousEventHandler("sysApplicationFocusChangeEvent", function(_, active)
  headlessFocus = headlessFocus .. tostring(active) .. " "
end)
)lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        std::optional<bool> setMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &setMidWrite](const QString&, const QString& text) {
            if (text == qsl("leave now")) {
                setMidWrite = session.setApplicationActive(false);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("leave now\n"))lua")));
        QCOMPARE(setMidWrite, std::optional<bool>(false));
        QVERIFY(session.applicationActive());
        QCOMPARE(luaGlobalString(session.host(), "headlessFocus"), QString());
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionInputTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionInputTest)
