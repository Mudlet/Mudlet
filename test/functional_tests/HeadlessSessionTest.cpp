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
 * THeadlessSession is what a frontend with no window - the mudlet-headless
 * runner, a phone app - drives a profile through. Each test makes a session of
 * its own, and with it the process's one profile pool, so none of them shares a
 * profile with another.
 *
 * Run with: ctest -R HeadlessSessionTest -V
 */

#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>
#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TelnetServerStub.h"

#include "GroupedTest.h"

class HeadlessSessionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name, const QString& script)
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
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSession</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped())
                               .toUtf8());
        return true;
    }

    // Opened binary so Windows writes no \r
    static QString recordingScript(const QString& recordPath)
    {
        return qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local file = io.open([[%1]], "ab")
  file:write("sysExitEvent\n")
  file:close()
end))lua")
                .arg(recordPath);
    }

    static QString readFile(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    // Each one the map load tries and turns down runs the event loop
    static bool writeUnreadableMaps(const QString& name, const int count)
    {
        const QString mapFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        if (!QDir().mkpath(mapFolder)) {
            return false;
        }
        for (int map = 0; map < count; ++map) {
            QFile mapFile(qsl("%1/2020-01-0%2#00-00-00map.dat").arg(mapFolder).arg(map + 1));
            if (!mapFile.open(QIODevice::WriteOnly) || mapFile.write(QByteArray(64, 'x')) != 64) {
                return false;
            }
        }
        return true;
    }

    static qsizetype saveCount(const QString& name) { return QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static QStringList outputTexts(const QSignalSpy& spy)
    {
        QStringList texts;
        for (const auto& arguments : spy) {
            texts << arguments.at(1).toString();
        }
        return texts;
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

    // A name the frontend got wrong can be asked for again
    void test_unknownProfileIsRefused()
    {
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("No Such Headless Profile"), .connect = false}), THeadlessSession::StartResult::noSuchProfile);
        QVERIFY(session.errorString().contains(qsl("No Such Headless Profile")));
        QVERIFY(!session.host());
        QVERIFY(!session.submit(qsl("look")));
        QVERIFY(!session.runLua(qsl("x = 1")));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QVERIFY(!HostManager::self());
        QVERIFY(writeProfile(qsl("Headless Second Try"), QString()));
        QCOMPARE(session.start({.profile = qsl("Headless Second Try"), .connect = false}), THeadlessSession::StartResult::started);
    }

    // Mudlet.ini is read by the first pool alone, so the media mutes go from each pool to the next, as through
    // a name the frontend got wrong
    void test_theMediaMutesOutliveThePool()
    {
        const QString name = qsl("Headless Muted");
        QVERIFY(writeProfile(name, QString()));
        {
            const HostManager firstPool;
        }
        const auto unmute = qScopeGuard([]() {
            HostManager pool;
            pool.media().setApiMuted(false);
            pool.media().setGameMuted(false);
        });
        {
            HostManager pool;
            pool.media().setApiMuted(true);
            pool.media().setGameMuted(true);
        }
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Muted Typo"), .connect = false}), THeadlessSession::StartResult::noSuchProfile);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("headlessMutes = tostring(getConfig('muteMediaAPI')) .. ' ' .. tostring(getConfig('muteMediaGame'))")));
        QCOMPARE(luaGlobalString(session.host(), "headlessMutes"), qsl("true true"));
        QVERIFY(session.shutdown(false));
    }

    void test_profileNameIsMatchedIgnoringCase()
    {
        QVERIFY(writeProfile(qsl("Headless Case"), QString()));
        THeadlessSession session;
        QSignalSpy loaded(&session, &THeadlessSession::profileLoaded);
        QCOMPARE(session.start({.profile = qsl("headless case"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(loaded.count(), 1);
        QCOMPARE(loaded.at(0).at(0).toString(), qsl("Headless Case"));
        QVERIFY(session.host());
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
    }

    void test_aProfileNeverSavedIsRefused_data()
    {
        QTest::addColumn<QString>("save");
        QTest::newRow("no save") << QString();
        QTest::newRow("settings copied") << qsl("Copied profile (settings only).xml");
    }

    // The GUI installs a game's default packages on the first load, and a save made here would stop it
    void test_aProfileNeverSavedIsRefused()
    {
        QFETCH(QString, save);
        const QString name = save.isEmpty() ? qsl("Headless Never Saved") : qsl("Headless Settings Copied");
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        if (!save.isEmpty()) {
            QFile copied(qsl("%1/%2").arg(saveFolder, save));
            QVERIFY(copied.open(QIODevice::WriteOnly));
            copied.write(R"(<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE MudletPackage><MudletPackage version="1.001"><HostPackage><Host></Host></HostPackage></MudletPackage>)");
        }
        const qsizetype savesBefore = saveCount(name);
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::notSetUp);
        QVERIFY(!session.errorString().isEmpty());
        QVERIFY(!HostManager::self());
        session.shutdown(true);
        QCOMPARE(saveCount(name), savesBefore);
    }

    void test_aSecondPoolIsRefused()
    {
        QVERIFY(writeProfile(qsl("Headless Second Pool"), QString()));
        auto pool = std::make_unique<HostManager>();
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Second Pool"), .connect = false}), THeadlessSession::StartResult::notOpened);
        QVERIFY(!session.host());
        QVERIFY2(HostManager::self() == pool.get(), "The refused session took the pool it did not make.");
    }

    void test_unreadableSaveIsLoadFailed()
    {
        const QString name = qsl("Headless Corrupt");
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write("<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host");
        saveFile.close();
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy loaded(&session, &THeadlessSession::profileLoaded);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::loadFailed);
        QVERIFY(!session.errorString().isEmpty());
        QCOMPARE(loaded.count(), 0);
        // The GUI connects such a profile too, but there the player sees why it has no scripts
        QTest::qWait(1000);
        QVERIFY2(!server.hasPendingConnections() && !server.clientConnected(), "A profile whose save did not load was connected to its game.");
        // Its scripts never started, and what it holds now would be saved over its last good save and map
        QVERIFY(!session.runLua(qsl("headlessCorruptRan = true")));
        QVERIFY2(session.errorString().contains(qsl("takes no commands")), qPrintable(session.errorString()));
        QVERIFY(!session.submit(qsl("look")));
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        const QStringList mapsBefore = QDir(mapsFolder).entryList(QDir::Files);
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files), mapsBefore);

        // The same session, given a profile that loads, takes commands and saves it
        const QString good = qsl("Headless After Corrupt");
        QVERIFY(writeProfile(good, QString()));
        QCOMPARE(session.start({.profile = good, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.runLua(qsl("headlessAfterCorruptRan = true")), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(good), 2);
    }

    // The scripts read before the save broke off still run; what they open is no part of the failed load
    void test_aProfileThatAFailedLoadOpenedIsSaved()
    {
        const QString name = qsl("Headless Corrupt Opener");
        const QString other = qsl("Headless Opened By The Corrupt");
        QVERIFY(writeProfile(other, QString()));
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>opener</name><packageName></packageName><script>loadProfile("%1", true)</script><eventHandlerList/></Script></ScriptPackage>
<HostPackage><Host)")
                               .arg(other)
                               .toUtf8());
        saveFile.close();

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::loadFailed);
        QVERIFY2(HostManager::self()->getHost(other), "The script read before the save broke off did not run.");
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(saveCount(other), 2);
    }

    void test_anUnwantedConnectIsCalledOff_data()
    {
        QTest::addColumn<bool>("partlyLoaded");
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("save partly loaded") << true << false;
        QTest::newRow("save partly loaded, session deleted during the load") << true << true;
        QTest::newRow("session deleted during the load") << false << true;
    }

    // Some of a save can load before the part that does not, and a sysLoadEvent handler there can run the
    // event loop until the connection is made: the GUI keeps it, but here it is dropped, as is that of a
    // profile whose session has gone
    void test_anUnwantedConnectIsCalledOff()
    {
        QFETCH(const bool, partlyLoaded);
        QFETCH(const bool, deleteSession);
        const QString name = qsl("Headless Unwanted Connect %1%2").arg(partlyLoaded).arg(deleteSession);
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        const QString script = qsl(R"lua(registerAnonymousEventHandler("sysLoadEvent", function()
  for i = 1, 1000 do
    if select(3, getConnectionInfo()) then
      headlessConnected = "yes"
      return
    end
    loadMap()
  end
end))lua");
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSession</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
%2)")
                               .arg(script.toHtmlEscaped(), partlyLoaded ? qsl("<HostPackage><Host") : qsl("<HostPackage><Host></Host></HostPackage></MudletPackage>"))
                               .toUtf8());
        saveFile.close();
        QVERIFY(writeUnreadableMaps(name, 1));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QObject context;
        if (deleteSession) {
            QTimer::singleShot(0, &context, [pSession]() {
                delete pSession;
            });
        }
        const auto started = pSession->start({.profile = name, .connect = true});
        QCOMPARE(started, deleteSession ? THeadlessSession::StartResult::notOpened : THeadlessSession::StartResult::loadFailed);
        // A deleted session's pool goes from a timer, so its profile is still here
        const QPointer<Host> pHost(HostManager::self()->getHost(name));
        QVERIFY(pHost);
        QCOMPARE(luaGlobalString(pHost, "headlessConnected"), qsl("yes"));
        QVERIFY2(!std::get<2>(pHost->mTelnet.getConnectionInfo()), "An unwanted connection was left up.");
        if (deleteSession) {
            QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
            return;
        }
        QTRY_VERIFY_WITH_TIMEOUT(!server.clientConnected(), 5000);
        QTest::qWait(500);
        QVERIFY(!server.clientConnected());
        pSession->shutdown(false);
        delete pSession;
    }

    void test_gameLinesReachOutputLine()
    {
        const QString name = qsl("Headless Game");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("Welcome to the headless session test."));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);

        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Welcome to the headless session test.")), 10000);
        const auto welcome = output.at(outputTexts(output).indexOf(qsl("Welcome to the headless session test.")));
        QCOMPARE(welcome.at(0).toString(), name);
        QCOMPARE(welcome.at(2).toBool(), false);
    }

    // A prompt the game ends with GA, and commands as the command line sends them, a trailing line feed
    // making an empty one as there
    void test_promptsAndCommandsReachTheGame()
    {
        const QString name = qsl("Headless Prompt");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("Welcome\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("hp 10> ")), 10000);
        const auto prompt = output.at(outputTexts(output).indexOf(qsl("hp 10> ")));
        QCOMPARE(prompt.at(2).toBool(), true);
        QCOMPARE(output.at(outputTexts(output).indexOf(qsl("Welcome"))).at(2).toBool(), false);

        QVERIFY(session.submit(qsl("north\n")));
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("north\r\n\r\n"), 10000);
    }

    // Lines a trigger gags still go out as the game sent them, as --mirror copies them
    void test_fedLinesReachOutputLineInOrder()
    {
        QVERIFY(writeProfile(qsl("Headless Fed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Fed"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("second", [[deleteLine()]]) feedTriggers("first fed\nsecond fed\nthird fed\n"))lua")));
        QCOMPARE(outputTexts(output), (QStringList{qsl("first fed"), qsl("second fed"), qsl("third fed")}));
    }

    void test_linesFromTheLoadReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless Load Lines"), qsl(R"lua(feedTriggers("fed by the script\n")
registerAnonymousEventHandler("sysLoadEvent", function() feedTriggers("fed by sysLoadEvent\n") end))lua")));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = qsl("Headless Load Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed by the script"), qsl("fed by sysLoadEvent")}));
    }

    // Another profile a script opens, and what echo() adds, are not the game's lines of the session's profile
    void test_onlyTheProfilesGameLinesReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless Own"), QString()));
        QVERIFY(writeProfile(qsl("Headless Other"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Own"), .connect = false}), THeadlessSession::StartResult::started);
        Host* pOther = HostManager::self()->openProfile(qsl("Headless Other"), false);
        QVERIFY(pOther);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(feedTriggers("fed to the other\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("echoed\n") feedTriggers("fed to its own\n"))lua")));
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed to its own")}));
    }

    void test_submitRunsAliases()
    {
        QVERIFY(writeProfile(qsl("Headless Submit"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Submit"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless (\\w+)$", [[headlessAliasHit = matches[2]; headlessAliasCount = (headlessAliasCount or 0) + 1]]))lua")));
        QVERIFY(session.submit(qsl("headless ping")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasHit"), qsl("ping"));
        // As a paste into the GUI's command line, each line its own command
        QVERIFY(session.submit(qsl("headless one\nheadless two")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasHit"), qsl("two"));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasCount"), qsl("3"));
        // As a Windows paste, which the GUI's command line never hands over
        QVERIFY(session.submit(qsl("headless three\r\nheadless four")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasCount"), qsl("5"));
    }

    void test_runLuaReportsErrors()
    {
        QVERIFY(writeProfile(qsl("Headless Lua"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Lua"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("headlessLua = 'ran'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessLua"), qsl("ran"));
        QVERIFY(!session.runLua(qsl("this is not lua")));
        QVERIFY2(session.errorString().contains(qsl("near")), qPrintable(session.errorString()));
        QVERIFY(!session.runLua(qsl("error('headless failure')")));
        QCOMPARE(session.errorString(), qsl(R"lua([string "error('headless failure')"]:1: headless failure)lua"));
        QVERIFY(!session.runLua(qsl("error({})")));
        QCOMPARE(session.errorString(), qsl("no error message available from Lua"));
        QVERIFY(!session.runLua(QString()));
        QVERIFY(!session.errorString().contains(qsl("headless failure")));
    }

    void test_closeProfileRequestsQuit()
    {
        QVERIFY(writeProfile(qsl("Headless Close"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Close"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        QVERIFY(!session.host());
        QVERIFY(!session.submit(qsl("look")));
    }

    void test_shutdownSavesOnlyWhenAsked_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    void test_shutdownSavesOnlyWhenAsked()
    {
        QFETCH(bool, save);
        const QString name = save ? qsl("Headless Shutdown Save") : qsl("Headless Shutdown No Save");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.shutdown(save);
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), save ? 2 : 1);
    }

    void test_shutdownDoesNotRequestQuit_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    // The quit is queued, so one owed to the shut down profile would end the next start()'s
    void test_shutdownDoesNotRequestQuit()
    {
        QFETCH(bool, save);
        QVERIFY(writeProfile(qsl("Headless Shutdown Quiet"), QString()));
        THeadlessSession session;
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = qsl("Headless Shutdown Quiet"), .connect = false}), THeadlessSession::StartResult::started);
        session.shutdown(save);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        QCOMPARE(session.start({.profile = qsl("Headless Shutdown Quiet"), .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        session.shutdown(false);
    }

    // As a frontend that runs the event loop itself would, opening the profile again once it has closed
    void test_aQuitFromAnEarlierRunIsDropped()
    {
        QVERIFY(writeProfile(qsl("Headless Close Restart"), QString()));
        THeadlessSession session;
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = qsl("Headless Close Restart"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pFirst(session.host());
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QDeadlineTimer deadline(10000);
        while (pFirst && !deadline.hasExpired()) {
            QCoreApplication::processEvents();
        }
        QVERIFY(!pFirst);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("Headless Close Restart"), .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.host());
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        session.shutdown(false);
    }

    void test_exitLinesReachOutputLine_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    void test_exitLinesReachOutputLine()
    {
        QFETCH(bool, save);
        QVERIFY(writeProfile(qsl("Headless Exit Lines"), qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() feedTriggers("fed by sysExitEvent\n") end))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Exit Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        session.shutdown(save);
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed by sysExitEvent")}));
    }

    // Nothing runs the event loop after the runner's shutdown, where the pool lets such a profile go
    void test_shutdownDeletesAProfileTakenOutOfThePool()
    {
        QVERIFY(writeProfile(qsl("Headless Exit Deletes"), QString()));
        // Named to come after the session's, so the closing pass meets it after the handler deleted it
        QVERIFY(writeProfile(qsl("Headless Exit Removed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Exit Deletes"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pRemoved(HostManager::self()->openProfile(qsl("Headless Exit Removed"), false));
        QVERIFY(pRemoved);
        // Stands in for a frontend's slot that a sysExitEvent handler reaches
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() deleteFromPool("Headless Exit Removed") end))lua")));
        session.shutdown(true);
        QVERIFY2(!pRemoved, "The profile taken out of the pool outlived the shutdown.");
    }

    void test_thePoolOutlivesAProfileBeingLetGo_data()
    {
        QTest::addColumn<bool>("ownProfile");
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("another profile, shut down unsaved") << false << false;
        QTest::newRow("the session's profile, shut down unsaved") << true << false;
        QTest::newRow("another profile, session deleted") << false << true;
    }

    // One a sysExitEvent handler takes out of the pool is let go from a timer once the handler has returned,
    // and its deletion still reaches the pool
    void test_thePoolOutlivesAProfileBeingLetGo()
    {
        QFETCH(bool, ownProfile);
        QFETCH(bool, deleteSession);
        const QString name = qsl("Headless Let Go %1 %2").arg(ownProfile).arg(deleteSession);
        const QString other = qsl("Headless Let Go Other %1 %2").arg(ownProfile).arg(deleteSession);
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeProfile(other, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pOther(HostManager::self()->openProfile(other, false));
        QVERIFY(pOther);
        const QString letGo = ownProfile ? name : other;
        Host* pLetGoHost = ownProfile ? pSession->host() : pOther.data();
        const QPointer<Host> pLetGo(pLetGoHost);
        lua_register(pLetGoHost->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        QVERIFY(pLetGoHost->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() deleteFromPool("%1") end))lua").arg(letGo)));
        QVERIFY(HostManager::self()->requestProfileClose(letGo));
        QVERIFY(HostManager::releasingProfile());
        if (deleteSession) {
            delete pSession;
            QVERIFY2(HostManager::self() || !pLetGo, "The pool went before the profile it was letting go.");
            QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
            return;
        }
        QVERIFY(pSession->shutdown(false));
        QVERIFY2(!pLetGo, "A profile being let go outlived the shutdown.");
        QVERIFY(!pSession->host());
        delete pSession;
    }

    // echo() of this phrase commits the OSC 8 examples as game lines, with no trigger pass
    void test_linesWithNoTriggerPassReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless No Pass"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless No Pass"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(qsl(R"lua(echo("!osc8-" .. "docs"))lua")));
        QVERIFY2(outputTexts(output).contains(qsl("Docs: https://wiki.mudlet.org/w/Area_51#OSC_8:_Hyperlink_Protocol")), qPrintable(outputTexts(output).join(QLatin1Char('\n'))));
    }

    // A map that will not read takes a turn of the event loop, where a frontend's timer could run a script
    // before the profile's own have started, or delete the profile under the load
    void test_callsDuringTheLoadAreRefused()
    {
        const QString name = qsl("Headless Load Shutdown");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeUnreadableMaps(name, 1));
        THeadlessSession session;
        bool askedDuringTheLoad = false;
        bool ranLua = true;
        bool submitted = true;
        bool shutDown = true;
        QString refusal;
        QTimer::singleShot(0, &session, [&]() {
            askedDuringTheLoad = true;
            ranLua = session.runLua(qsl("headlessDuringLoad = 'yes'"));
            submitted = session.submit(qsl("look"));
            shutDown = session.shutdown(true);
            refusal = session.errorString();
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(askedDuringTheLoad);
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QVERIFY(!shutDown);
        QCOMPARE(refusal, qsl("Refused while a profile loads; call it once that has returned."));
        QVERIFY(session.host());
        QVERIFY(HostManager::self());
        QVERIFY(session.runLua(qsl("headlessAfterLoad = 'yes'")));
    }

    // As an app's screen closing while it loads
    void test_deletingTheSessionDuringTheLoad()
    {
        const QString name = qsl("Headless Load Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));
        QVERIFY(writeUnreadableMaps(name, 1));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::notOpened);
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    // As one a frontend gives its map: nothing of the profile's own is on the stack, but the pump is
    void test_shutdownDuringAMapOperationIsRefused()
    {
        const QString name = qsl("Headless Map Shutdown");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        bool shutDown = true;
        QTimer::singleShot(0, &session, [&session, &shutDown]() {
            shutDown = session.shutdown(true);
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        session.host()->mpMap->restore(QString());
        QVERIFY(!shutDown);
        QVERIFY(HostManager::self());
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // As a frontend's function bound into Lua would: the profile's Lua state is still running
    void test_shutdownFromUnderAScriptIsRefused()
    {
        static THeadlessSession* spSession = nullptr;
        QVERIFY(writeProfile(qsl("Headless Lua Shutdown"), QString()));
        THeadlessSession session;
        spSession = &session;
        QCOMPARE(session.start({.profile = qsl("Headless Lua Shutdown"), .connect = false}), THeadlessSession::StartResult::started);
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "shutdownSession", [](lua_State*) -> int {
            spSession->shutdown(true);
            return 0;
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl("shutdownSession() headlessAfterShutdown = string.rep('x', 3)")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAfterShutdown"), qsl("xxx"));
        QVERIFY(HostManager::self());
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // As a profile's close does while its save waits, which a frontend's timer can run in the middle of
    void test_shutdownDuringANestedEventLoopIsRefused()
    {
        QVERIFY(writeProfile(qsl("Headless Nested Loop"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Nested Loop"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->getLuaInterpreter()->enterNestedEventLoop();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(!session.shutdown(true));
        QVERIFY(!session.errorString().isEmpty());
        QVERIFY(HostManager::self());
        session.host()->getLuaInterpreter()->leaveNestedEventLoop();
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // Run in the middle of the close, the reset would rebuild the Lua state its exit handlers and save are using
    void test_aResetAskedForFirstEndsBeforeTheClose()
    {
        const QString name = qsl("Headless Reset Shutdown");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("resetProfile()")));
        session.shutdown(true);
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
    }

    // The profile closes with its reset still asked for, which it then never runs
    void test_shutdownAfterAResetAndACloseFromTheSameScript()
    {
        const QString name = qsl("Headless Reset Close");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("resetProfile() closeProfile()")));
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("loaded\nsysExitEvent\n"));
    }

    void test_deletingTheSessionWithAResetAskedForLetsTheResetEnd()
    {
        const QString name = qsl("Headless Reset Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QVERIFY(session.runLua(qsl("resetProfile()")));
        }
        THeadlessSession next;
        QCOMPARE(next.start({.profile = name, .connect = false}), THeadlessSession::StartResult::poolClosing);
        QVERIFY(!next.errorString().isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(next.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
    }

    // As an app's screen closing while it waits: the profiles close unsaved
    void test_deletingTheSessionWhileShutdownWaitsForAReset()
    {
        const QString name = qsl("Headless Reset Wait Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(pSession->runLua(qsl("resetProfile()")));
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(!pSession->shutdown(true));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    void test_deletingTheSessionUnderShutdownsExitHandlers_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    // As above, while an exit handler runs the map's pump: the save, if asked for, goes ahead
    void test_deletingTheSessionUnderShutdownsExitHandlers()
    {
        QFETCH(const bool, save);
        const QString name = save ? qsl("Headless Save Delete") : qsl("Headless No Save Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")
                                     .arg(recordingScript(recordPath))));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(pSession->shutdown(save));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), save ? 2 : 1);
    }

    void test_aStartWhileTheProfilesCloseIsPoolClosing_data()
    {
        QTest::addColumn<int>("close");
        QTest::newRow("deleted") << 0;
        QTest::newRow("shut down, saved") << 1;
        QTest::newRow("shut down, unsaved") << 2;
        QTest::newRow("shut down after a reset") << 3;
    }

    // Until the last one has gone, as its exit handlers, and a shutdown's wait for a reset, may run the event loop
    void test_aStartWhileTheProfilesCloseIsPoolClosing()
    {
        QFETCH(const int, close);
        const QString name = qsl("Headless Closing Start %1").arg(close);
        QVERIFY(writeProfile(name, close == 3 ? QString() : qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        if (close == 3) {
            QVERIFY(pSession->runLua(qsl("resetProfile()")));
        }
        THeadlessSession next;
        std::optional<THeadlessSession::StartResult> started;
        bool ranLua = true;
        bool shutDown = true;
        QObject context;
        QTimer::singleShot(0, &context, [pSession, &next, &started, &ranLua, &shutDown, name]() {
            started = next.start({.profile = name, .connect = false});
            ranLua = pSession->runLua(qsl("headlessRan = 'yes'"));
            shutDown = pSession->shutdown(true);
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        if (close) {
            QVERIFY(pSession->shutdown(close != 2));
        }
        delete pSession;
        QVERIFY(!HostManager::self());
        QCOMPARE(started, std::optional(THeadlessSession::StartResult::poolClosing));
        QVERIFY(!ranLua);
        QVERIFY(!shutDown);
    }

    void test_callsDuringAShutdownAreRefused_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("while it waits for a reset") << true;
        QTest::newRow("under its exit handlers") << false;
    }

    // As a frontend's timer closing the profiles the other way, or sending to them, while they close
    void test_callsDuringAShutdownAreRefused()
    {
        QFETCH(const bool, save);
        const QString name = save ? qsl("Headless Nested Shutdown") : qsl("Headless Nested Shutdown Exit");
        QVERIFY(writeProfile(name, save ? QString() : qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        if (save) {
            QVERIFY(session.runLua(qsl("resetProfile()")));
        } else {
            QVERIFY(writeUnreadableMaps(name, 2));
        }
        std::optional<bool> nested;
        bool ranLua = true;
        bool submitted = true;
        QObject context;
        QTimer::singleShot(0, &context, [&session, &nested, &ranLua, &submitted, save]() {
            nested = session.shutdown(!save);
            ranLua = session.runLua(qsl("headlessRan = 'yes'"));
            submitted = session.submit(qsl("north"));
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.shutdown(save));
        QCOMPARE(nested, std::optional(false));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QVERIFY(!HostManager::self());
        QCOMPARE(saveCount(name), save ? 2 : 1);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.shutdown(false));
        QVERIFY(!HostManager::self());
    }

    // As a frontend's timer closing its screen while a script runs the map's pump
    void test_deletingTheSessionWhileRunLuaPumpsEvents()
    {
        const QString name = qsl("Headless RunLua Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(pSession->host());
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(!pSession->runLua(qsl("loadMap() headlessAfterDelete = 'yes' error('after the delete')")));
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("yes"));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(saveCount(name), 1);
    }

    // As above, from an alias
    void test_deletingTheSessionWhileSubmitPumpsEvents()
    {
        const QString name = qsl("Headless Submit Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(pSession->runLua(qsl(R"lua(tempAlias("^pump$", [[loadMap()]]) tempAlias("^after$", [[headlessAfterDelete = "yes"]]))lua")));
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(pSession->host());
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(pSession->submit(qsl("pump\nafter")));
        QVERIFY(pHost);
        QVERIFY(luaGlobalString(pHost, "headlessAfterDelete").isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(saveCount(name), 1);
    }

    void test_aClosedProfileTakesNoMoreCommands_data()
    {
        QTest::addColumn<int>("closedBy");
        QTest::newRow("a script, before the call") << 0;
        QTest::newRow("the first line's alias") << 1;
        QTest::newRow("the frontend, in the event loop the first line's alias ran") << 2;
    }

    // Its aliases and connection stop once it has closed, before it is deleted, so what is left of a paste goes nowhere
    void test_aClosedProfileTakesNoMoreCommands()
    {
        QFETCH(const int, closedBy);
        const QString name = qsl("Headless Submit Closing %1").arg(closedBy);
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^first$", [[%1]]) tempAlias("^after$", [[headlessAfter = "yes"]]))lua").arg(closedBy == 1 ? qsl("closeProfile()") : qsl("loadMap()"))));
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(session.host());
        if (closedBy == 0) {
            QVERIFY(session.runLua(qsl("closeProfile()")));
        } else if (closedBy == 2) {
            QTimer::singleShot(0, this, [name]() {
                if (HostManager* pPool = HostManager::self()) {
                    pPool->requestProfileClose(name);
                }
            });
        }
        QVERIFY(!session.submit(qsl("first\nafter")));
        QCOMPARE(session.errorString(), qsl("The profile has closed, so it takes no more commands."));
        QVERIFY(pHost);
        QVERIFY(pHost->isClosingDown());
        QVERIFY(luaGlobalString(pHost, "headlessAfter").isEmpty());
        QVERIFY(!session.runLua(qsl("headlessAfter = 'yes'")));
        QCOMPARE(session.errorString(), qsl("The profile has closed, so it takes no more commands."));
        QTRY_VERIFY_WITH_TIMEOUT(!session.host(), 10000);
        QVERIFY(session.shutdown(false));
    }

    // Deleting the pool would close the Lua state the script is still running on
    void test_deletingTheSessionFromUnderAScriptClosesTheProfileOnceItEnds()
    {
        static THeadlessSession* spSession = nullptr;
        QVERIFY(writeProfile(qsl("Headless Lua Delete"), QString()));
        QObject owner;
        spSession = new THeadlessSession(&owner);
        QCOMPARE(spSession->start({.profile = qsl("Headless Lua Delete"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(spSession->host());
        lua_register(pHost->getLuaInterpreter()->getLuaGlobalState(), "deleteSession", [](lua_State*) -> int {
            delete spSession;
            spSession = nullptr;
            return 0;
        });
        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("deleteSession() headlessAfterDelete = string.rep('x', 3)")));
        QVERIFY(pHost);
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("xxx"));
        // Then it closes, so that another session can start
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        THeadlessSession next;
        QCOMPARE(next.start({.profile = qsl("Headless Lua Delete"), .connect = false}), THeadlessSession::StartResult::started);
    }

    // The buffer is still committing the line the handler was handed, with no script running
    void test_deletingTheSessionFromAGameLineHandler()
    {
        const QString name = qsl("Headless Game Line Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        // The trigger runs once the line is handed out, so it shows the commit finished
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
tempTrigger("outer line", function()
  local file = io.open([[%2]], "ab")
  file:write("trigger\n")
  file:close()
end))lua")
                                     .arg(recordingScript(recordPath), recordPath)));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("outer line"));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        connect(pSession, &THeadlessSession::outputLine, this, [pSession](const QString&, const QString& text, bool) {
            if (text == qsl("outer line")) {
                delete pSession;
            }
        });
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        QCOMPARE(readFile(recordPath), qsl("trigger\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    // Not what a handler may do, but the session's own part holds: deleted from a timer in the loop, its pool
    // waits for the line's commit to end
    void test_deletingTheSessionInAnOutputLineHandlersEventLoop()
    {
        const QString name = qsl("Headless Nested Loop Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
tempTrigger("outer line", function()
  local file = io.open([[%2]], "ab")
  file:write("trigger\n")
  file:close()
end))lua")
                                     .arg(recordingScript(recordPath), recordPath)));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("outer line"));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        bool sessionGoneInTheLoop = false;
        bool poolKeptThroughTheLoop = false;
        connect(pSession,
                &THeadlessSession::outputLine,
                this,
                [this, pSession, pAlive = QPointer(pSession), &sessionGoneInTheLoop, &poolKeptThroughTheLoop](const QString&, const QString& text, bool) {
                    if (text != qsl("outer line")) {
                        return;
                    }
                    QTimer::singleShot(0, this, [pSession]() {
                        delete pSession;
                    });
                    QEventLoop loop;
                    QTimer::singleShot(200, &loop, &QEventLoop::quit);
                    loop.exec();
                    sessionGoneInTheLoop = !pAlive;
                    poolKeptThroughTheLoop = HostManager::self() != nullptr;
                });
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(sessionGoneInTheLoop);
        QVERIFY(poolKeptThroughTheLoop);
        QVERIFY(!pHost);
        QCOMPARE(readFile(recordPath), qsl("trigger\nsysExitEvent\n"));
    }

    void test_deletingTheSessionFromAProfileLoadedHandler()
    {
        const QString name = qsl("Headless Loaded Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QString profileAfterDelete;
        connect(pSession, &THeadlessSession::profileLoaded, this, [pSession, &profileAfterDelete](const QString& profile) {
            delete pSession;
            profileAfterDelete = profile;
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(profileAfterDelete, name);
        QVERIFY(!HostManager::self());
    }

    void test_deletingTheSessionFromAnOutputLineHandler_data()
    {
        QTest::addColumn<bool>("throughSession");
        QTest::newRow("runLua()") << true;
        QTest::newRow("a script of the profile's own") << false;
    }

    // As above, from under the script that fed the line and, if the session ran it, that call
    void test_deletingTheSessionFromAnOutputLineHandler()
    {
        QFETCH(const bool, throughSession);
        const QString name = throughSession ? qsl("Headless Handler Delete") : qsl("Headless Handler Delete Own");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        QString profileAfterDelete;
        connect(pSession, &THeadlessSession::outputLine, this, [pSession, &profileAfterDelete](const QString& profile, const QString& text, bool) {
            if (text == qsl("outer line")) {
                delete pSession;
                profileAfterDelete = profile;
            }
        });
        const QString script = qsl(R"lua(feedTriggers("outer line\n") headlessAfterDelete = "yes")lua");
        QVERIFY(throughSession ? pSession->runLua(script) : pHost->getLuaInterpreter()->compileAndExecuteScript(script));
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("yes"));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        QCOMPARE(profileAfterDelete, name);
    }

    // A line fed from a handler would be committed in the middle of the one it is handed, and a shutdown
    // there would delete the buffer that is committing it
    void test_outputLineHandlersCannotReenterTheProfile()
    {
        QVERIFY(writeProfile(qsl("Headless Reentry"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Reentry"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless", [[headlessAliasRan = "yes"]]))lua")));
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        bool ranLua = true;
        bool submitted = true;
        QString refusal;
        // One with no profiles of its own has nothing to refuse
        THeadlessSession unstarted;
        bool unstartedShutDown = false;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &ranLua, &submitted, &refusal, &unstarted, &unstartedShutDown](const QString&, const QString& text, bool) {
            if (text == qsl("outer line")) {
                ranLua = session.runLua(qsl(R"lua(feedTriggers("inner line\n"))lua"));
                submitted = session.submit(qsl("headless"));
                session.shutdown(true);
                refusal = session.errorString();
                unstartedShutDown = unstarted.shutdown(false);
            }
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("outer line\n") feedTriggers("after\n"))lua")));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QCOMPARE(refusal, qsl("Refused while a line is being handed out; queue the call instead."));
        QVERIFY(unstartedShutDown);
        QVERIFY(HostManager::self());
        QCOMPARE(outputTexts(output), (QStringList{qsl("outer line"), qsl("after")}));
        QVERIFY(luaGlobalString(session.host(), "headlessAliasRan").isEmpty());
        QVERIFY(session.submit(qsl("headless")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRan"), qsl("yes"));
    }
};

#include "HeadlessSessionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionTest)
