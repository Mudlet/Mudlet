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
#include <QMimeData>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
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

    static QStringList texts(const QSignalSpy& spy)
    {
        QStringList result;
        for (const auto& arguments : spy) {
            result << arguments.at(1).toString();
        }
        return result;
    }

    static bool listenAsTheGame(QTcpServer& server, const QString& name)
    {
        return server.listen(QHostAddress::LocalHost) && MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first
               && MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first;
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

    void test_aReplayIsRecordedAsTheGuisButtonDoes()
    {
        const QString name = qsl("Headless Replay Recording");
        QVERIFY(writeProfile(name));
        QTcpServer server;
        QVERIFY(listenAsTheGame(server, name));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(!session.setReplayRecording(true));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());

        QVERIFY(!session.setReplayRecording(false));
        QCOMPARE(session.errorString(), qsl("No replay is being recorded."));
        QVERIFY(!session.replayRecording());
        local.clear();
        QVERIFY(session.setReplayRecording(true));
        QVERIFY(session.replayRecording());
        QVERIFY(!session.setReplayRecording(true));
        QCOMPARE(session.errorString(), qsl("A replay is being recorded already."));
        const QString started = qsl("System Message: Replay recording has started. File: ");
        const QString startedText = texts(local).join(QString());
        QVERIFY2(startedText.startsWith(started) && startedText.endsWith(QChar::LineFeed), qPrintable(startedText));
        const QString fileName = startedText.mid(started.size()).chopped(1);
        QCOMPARE(QFileInfo(fileName).absoluteDir(), QDir(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name)));

        game->write("hello\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("hello")), 10000);
        local.clear();
        QVERIFY(session.setReplayRecording(false));
        QVERIFY(!session.replayRecording());
        QCOMPARE(texts(local).join(QString()), qsl("System Message: Replay recording has been stopped. File: %1\n").arg(fileName));
        QVERIFY(QFileInfo::exists(fileName));
        game->write("not recorded\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("not recorded")), 10000);

        QVERIFY(session.runLua(qsl("headlessReplay = tostring(loadReplay([[%1]]))").arg(fileName)));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplay"), qsl("true"));
        QTRY_COMPARE_WITH_TIMEOUT(texts(output).count(qsl("hello")), 2, 10000);
        QCOMPARE(texts(output).count(qsl("not recorded")), 1);
        QVERIFY(session.shutdown(false));
    }

    void test_aReplayRecordingIsSavedWhenTheGameHangsUp()
    {
        const QString name = qsl("Headless Replay Recording Hung Up");
        QVERIFY(writeProfile(name));
        QTcpServer server;
        QVERIFY(listenAsTheGame(server, name));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.setReplayRecording(true));
        game->write("before the end\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("before the end")), 10000);
        game->disconnectFromHost();
        QTRY_VERIFY_WITH_TIMEOUT(!session.replayRecording(), 10000);
        QVERIFY(!session.setReplayRecording(false));

        const QStringList saved = QDir(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name)).entryList(QStringList{qsl("*.dat")}, QDir::Files);
        QCOMPARE(saved.size(), 1);
        QVERIFY(session.runLua(qsl("headlessReplay = tostring(loadReplay([[%1]]))").arg(saved.first())));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplay"), qsl("true"));
        QTRY_COMPARE_WITH_TIMEOUT(texts(output).count(qsl("before the end")), 2, 10000);
        QVERIFY(session.shutdown(false));
    }

    // The file is named for the second it starts in, which an app's recordings can share
    void test_replayRecordingsInOneSecondAreKeptApart()
    {
        const QString name = qsl("Headless Replay Recordings Apart");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        local.clear();
        for (int i = 0; i < 3; ++i) {
            QVERIFY(session.setReplayRecording(true));
            QVERIFY(session.setReplayRecording(false));
        }
        const QStringList saved = QDir(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name)).entryList(QStringList{qsl("*.dat")}, QDir::Files);
        QVERIFY2(saved.size() == 3, qPrintable(saved.join(qsl(", ")) + QChar::LineFeed + texts(local).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    void test_aRecordingNameTakenAlreadyIsSuffixedInTheMessages()
    {
        const QString name = qsl("Headless Replay Recording Suffixed");
        QVERIFY(writeProfile(name));
        const QDir logs(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name));
        QVERIFY(logs.mkpath(logs.absolutePath()));
        // Every second the recording could start in, so the plain name is always taken
        const QDateTime now = QDateTime::currentDateTime();
        QStringList taken;
        for (int second = 0; second < 30; ++second) {
            const QString takenName = logs.absoluteFilePath(qsl("%1.dat").arg(now.addSecs(second).toString(qsl("yyyy-MM-dd#HH-mm-ss"))));
            QFile placeholder(takenName);
            QVERIFY(placeholder.open(QIODevice::WriteOnly));
            QCOMPARE(placeholder.write("earlier"), 7);
            taken << takenName;
        }
        THeadlessSession session;
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        local.clear();
        QVERIFY(session.setReplayRecording(true));
        const QString started = qsl("System Message: Replay recording has started. File: ");
        const QString startedText = texts(local).join(QString());
        QVERIFY2(startedText.startsWith(started) && startedText.endsWith(qsl("-2.dat\n")), qPrintable(startedText));
        const QString fileName = startedText.mid(started.size()).chopped(1);
        QVERIFY2(taken.contains(fileName.chopped(qsl("-2.dat").size()) + qsl(".dat")), qPrintable(fileName));
        local.clear();
        QVERIFY(session.setReplayRecording(false));
        QCOMPARE(texts(local).join(QString()), qsl("System Message: Replay recording has been stopped. File: %1\n").arg(fileName));
        QVERIFY(QFileInfo::exists(fileName));
        for (const QString& takenName : taken) {
            QFile placeholder(takenName);
            QVERIFY(placeholder.open(QIODevice::ReadOnly));
            QCOMPARE(placeholder.readAll(), QByteArray("earlier"));
        }
        QVERIFY(session.shutdown(false));
    }

    void test_aReplayRecordingThatCannotBeOpenedSaysWhy()
    {
        const QString name = qsl("Headless Replay Recording Unopened");
        QVERIFY(writeProfile(name));
        {
            // A file where the log folder would be
            QFile blocker(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name));
            QVERIFY(blocker.open(QIODevice::WriteOnly));
        }
        THeadlessSession session;
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        local.clear();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("failed to open replay file for writing")));
        QVERIFY(!session.setReplayRecording(true));
        QVERIFY(!session.replayRecording());
        QVERIFY2(session.errorString().startsWith(qsl("The replay recording file could not be opened: ")), qPrintable(session.errorString()));
        const QString failed = texts(local).join(QString());
        QVERIFY2(failed.startsWith(qsl("System Message: Failed to open replay recording file for writing: ")), qPrintable(failed));
        QVERIFY(session.shutdown(false));
    }

    void test_aDropOnTheMainConsoleIsToldToScripts()
    {
        const QString name = qsl("Headless Drop");
        QVERIFY(writeProfile(name, qsl(R"lua(
headlessDrops = ""
for _, name in ipairs({"sysDropEvent", "sysDropUrlEvent"}) do
  registerAnonymousEventHandler(name, function(event, item, kind, x, y, console)
    headlessDrops = headlessDrops .. string.format("%s|%s|%s|%s|%s|%s\n", event, item, kind, tostring(x), tostring(y), console)
  end)
end
)lua")));
        QTemporaryDir dropped;
        QVERIFY(dropped.isValid());
        const QString notes = qsl("%1/notes.txt").arg(dropped.path());
        {
            QFile file(notes);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        THeadlessSession session;
        QMimeData empty;
        QVERIFY(!session.dropOnMainConsole(empty, QPoint()));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const auto drops = [&session]() {
            const QString drops = luaGlobalString(session.host(), "headlessDrops");
            session.runLua(qsl("headlessDrops = ''"));
            return drops;
        };

        QVERIFY(!session.dropOnMainConsole(empty, QPoint(1, 2)));
        QCOMPARE(session.errorString(), qsl("The drop holds neither files nor text, which the main console does not take."));
        QCOMPARE(drops(), QString());

        QMimeData link;
        // Not http, which Mudlet's own handler would download as a package
        link.setText(qsl("telnet://example.org:23"));
        QVERIFY(session.dropOnMainConsole(link, QPoint(12, 34)));
        QCOMPARE(drops(), qsl("sysDropUrlEvent|telnet://example.org:23|telnet|12|34|main\n"));

        QMimeData files;
        files.setUrls({QUrl::fromLocalFile(notes)});
        QVERIFY(session.dropOnMainConsole(files, QPoint(7, 8)));
        // QMimeData gives a file's URL as its text when it has none of its own, so the GUI raises both
        QCOMPARE(drops(), qsl("sysDropEvent|%1|txt|7|8|main\nsysDropUrlEvent|%2|file|7|8|main\n").arg(notes, QUrl::fromLocalFile(notes).toString()));

        QMimeData both;
        both.setUrls({QUrl::fromLocalFile(notes), QUrl::fromLocalFile(qsl("%1/missing.txt").arg(dropped.path()))});
        both.setText(qsl("telnet://example.org:23"));
        QVERIFY(session.dropOnMainConsole(both, QPoint(5, 6)));
        QCOMPARE(drops(), qsl("sysDropEvent|%1|txt|5|6|main\nsysDropUrlEvent|telnet://example.org:23|telnet|5|6|main\n").arg(notes));

        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.dropOnMainConsole(both, QPoint(5, 6)));
        QVERIFY(session.setScriptsStopped(false));
        QCOMPARE(drops(), QString());
        QVERIFY(session.shutdown(false));
    }

    void test_theMuteButtonsAreForEveryProfile()
    {
        using SoundSource = THeadlessSession::SoundSource;
        const QString name = qsl("Headless Mute");
        const QString other = qsl("Headless Mute Other");
        const QString script = qsl(R"lua(
headlessSettings = ""
registerAnonymousEventHandler("sysSettingChanged", function(_, setting, value)
  if setting:find("^muteMedia") then
    headlessSettings = headlessSettings .. setting .. "=" .. tostring(value) .. " "
  end
end)
)lua");
        QVERIFY(writeProfile(name, script));
        QVERIFY(writeProfile(other, script));
        THeadlessSession session;
        QStringList heard;
        connect(&session, &THeadlessSession::soundsMutedChanged, this, [&heard](const SoundSource source, const bool muted) {
            heard.append(qsl("%1 %2").arg(source == SoundSource::scripts ? qsl("scripts") : qsl("game"), muted ? qsl("muted") : qsl("unmuted")));
        });
        QVERIFY(!session.setSoundsMuted(SoundSource::scripts, true));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QVERIFY(!session.toggleAllSoundsMuted());
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.soundsMuted(SoundSource::scripts));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        const auto settings = [&session]() {
            const QString settings = luaGlobalString(session.host(), "headlessSettings");
            session.runLua(qsl("headlessSettings = ''"));
            return settings;
        };

        QVERIFY(session.setSoundsMuted(SoundSource::scripts, true));
        QVERIFY(session.soundsMuted(SoundSource::scripts));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        QVERIFY(heard.isEmpty());
        QTRY_COMPARE(heard, (QStringList{qsl("scripts muted")}));
        QCOMPARE(settings(), qsl("muteMediaAPI=true "));
        heard.clear();
        QVERIFY(session.setSoundsMuted(SoundSource::scripts, true));
        QTRY_COMPARE(heard, (QStringList{qsl("scripts muted")}));
        QCOMPARE(settings(), QString());

        heard.clear();
        QVERIFY(session.toggleAllSoundsMuted());
        QVERIFY(session.soundsMuted(SoundSource::game));
        QTRY_COMPARE(heard, (QStringList{qsl("game muted")}));
        QCOMPARE(settings(), qsl("muteMediaGame=true "));
        heard.clear();
        QVERIFY(session.toggleAllSoundsMuted());
        QVERIFY(!session.soundsMuted(SoundSource::scripts));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        QTRY_COMPARE(heard, (QStringList{qsl("scripts unmuted"), qsl("game unmuted")}));
        QCOMPARE(settings(), qsl("muteMediaAPI=false muteMediaGame=false "));
        heard.clear();
        QVERIFY(session.toggleAllSoundsMuted());
        QTRY_COMPARE(heard, (QStringList{qsl("scripts muted"), qsl("game muted")}));
        QCOMPARE(settings(), qsl("muteMediaAPI=true muteMediaGame=true "));

        heard.clear();
        QVERIFY(session.runLua(qsl(R"lua(setConfig("muteMediaGame", false))lua")));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        QTRY_COMPARE(heard, (QStringList{qsl("game unmuted")}));
        QCOMPARE(settings(), qsl("muteMediaGame=false "));

        // Only one session's profiles to tell, as there is only one pool a process
        THeadlessSession second;
        QCOMPARE(second.start({.profile = name, .connect = false}), THeadlessSession::StartResult::notOpened);
        QCOMPARE(second.errorString(), qsl("A profile pool already exists in this process."));
        QVERIFY(!second.setSoundsMuted(SoundSource::scripts, false));

        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        heard.clear();
        QVERIFY(session.setSoundsMuted(SoundSource::game, true));
        QTRY_COMPARE(heard, (QStringList{qsl("game muted")}));
        QCOMPARE(settings(), qsl("muteMediaGame=true "));
        QCOMPARE(luaGlobalString(pOther, "headlessSettings"), qsl("muteMediaGame=true "));
        heard.clear();
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(setConfig("muteMediaGame", false))lua")));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        QTRY_COMPARE(heard, (QStringList{qsl("game unmuted")}));
        QCOMPARE(settings(), qsl("muteMediaGame=false "));

        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        QVERIFY(!session.host());
        heard.clear();
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(setConfig("muteMediaGame", true))lua")));
        QTest::qWait(100);
        QCOMPARE(heard, QStringList());
        QVERIFY(!session.soundsMuted(SoundSource::game));

        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(setConfig("muteMediaAPI", false) setConfig("muteMediaGame", false))lua")));
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.soundsMuted(SoundSource::scripts));
    }

    void test_theMuteButtonsAreRefusedMidWrite()
    {
        using SoundSource = THeadlessSession::SoundSource;
        const QString name = qsl("Headless Mute Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        std::optional<bool> setMidWrite;
        std::optional<bool> toggledMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &setMidWrite, &toggledMidWrite](const QString&, const QString& text) {
            if (text == qsl("quiet now")) {
                setMidWrite = session.setSoundsMuted(SoundSource::game, true);
                toggledMidWrite = session.toggleAllSoundsMuted();
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("quiet now\n"))lua")));
        QCOMPARE(setMidWrite, std::optional<bool>(false));
        QCOMPARE(toggledMidWrite, std::optional<bool>(false));
        QVERIFY(!session.soundsMuted(SoundSource::game));
        QVERIFY(!session.soundsMuted(SoundSource::scripts));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionInputTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionInputTest)
