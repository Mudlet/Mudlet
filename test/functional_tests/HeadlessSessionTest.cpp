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

#include "CredentialManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TGameDetails.h"
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

    void test_aFirstLoadInstallsTheDefaultPackages_data()
    {
        QTest::addColumn<QString>("save");
        QTest::newRow("no save") << QString();
        QTest::newRow("settings copied") << qsl("Copied profile (settings only).xml");
    }

    // As the GUI's first load of a profile does; a later load leaves them be
    void test_aFirstLoadInstallsTheDefaultPackages()
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
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.host());
        const QStringList installed = session.host()->mInstalledPackages;
        QVERIFY2(installed.contains(qsl("echo")) && installed.contains(qsl("generic_mapper")) && installed.contains(qsl("mudlet-base-ui")), qPrintable(installed.join(QLatin1Char(' '))));
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->mInstalledPackages, installed);
    }

    // What the connection dialog stores as it is typed into, and Host reads as it is made; a new profile is
    // made for a host, its TLS box stored as it stands, an existing one keeps what is not given
    void test_connectionDetailsAreStored()
    {
        const QString name = qsl("Headless Details");
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists());
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example"), .port = 4000, .tls = true, .login = qsl("bob"), .password = qsl("hunter2")}),
                 THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4000"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(MudletApp::readProfileData(name, qsl("login")), qsl("bob"));
        QVERIFY2(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("password"))), "The password for the run was stored.");
        QCOMPARE(session.host()->getUrl(), qsl("game.example"));
        QCOMPARE(session.host()->getPort(), 4000);
        QVERIFY(session.host()->mSslTsl);
        QCOMPARE(session.host()->getLogin(), qsl("bob"));
        QCOMPARE(session.host()->getPass(), qsl("hunter2"));
        QVERIFY(session.shutdown(true));

        QCOMPARE(session.start({.profile = name, .connect = false, .port = 4001}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4001"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(session.host()->getLogin(), qsl("bob"));
        QVERIFY(session.host()->getPass().isEmpty());
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false, .tls = false}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Unchecked));
        QVERIFY(!session.host()->mSslTsl);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false, .port = 70000}), THeadlessSession::StartResult::notOpened);
        QVERIFY2(session.errorString().contains(qsl("70000")), qPrintable(session.errorString()));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4001"));

        const QString plain = qsl("Headless Plain Details");
        QCOMPARE(session.start({.profile = plain, .connect = false, .host = qsl(" game.example ")}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(plain, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(plain, qsl("ssl_tsl")), QString::number(Qt::Unchecked));
        QVERIFY(!session.host()->mSslTsl);
        QCOMPARE(session.host()->getPort(), 23);
        QVERIFY(session.shutdown(true));

        // A predefined game's load sets its own address and TLS over the stored ones; what is given wins, as
        // the dialog's fields do, and its TLS box stands as the dialog ticks it for the game. The load's scripts
        // already see the given address, as the dialog's do.
        const bool gameTls = TGameDetails::findGame(qsl("Achaea"))->tlsEnabled;
        QCOMPARE(session.start({.profile = qsl("achaea"), .connect = false, .host = qsl("127.0.0.1"), .port = 4000}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getName(), qsl("Achaea"));
        QCOMPARE(session.host()->getUrl(), qsl("127.0.0.1"));
        QCOMPARE(session.host()->getPort(), 4000);
        QCOMPARE(session.host()->mSslTsl, gameTls);
        QCOMPARE(MudletApp::readProfileData(qsl("Achaea"), qsl("ssl_tsl")), QString::number(gameTls ? Qt::Checked : Qt::Unchecked));
        QVERIFY(session.runLua(qsl("headlessLoadHost = select(1, getConnectionInfo())")));
        QVERIFY(session.shutdown(true));

        const QString scripted = qsl("Headless Load Sees Details");
        QVERIFY(writeProfile(scripted, qsl("headlessLoadHost = select(1, getConnectionInfo())")));
        QCOMPARE(session.start({.profile = scripted, .connect = false, .host = qsl("game.example")}), THeadlessSession::StartResult::started);
        QCOMPARE(luaGlobalString(session.host(), "headlessLoadHost"), qsl("game.example"));
    }

    void test_aNewProfileNeedsAUsableName_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("result");
        QTest::newRow("no host") << qsl("Headless Unmade") << QString() << static_cast<int>(THeadlessSession::StartResult::noSuchProfile);
        QTest::newRow("a character the dialog refuses") << qsl("Headless/Unmade") << qsl("game.example") << static_cast<int>(THeadlessSession::StartResult::invalidName);
        QTest::newRow("the profiles folder itself") << qsl(".") << qsl("game.example") << static_cast<int>(THeadlessSession::StartResult::invalidName);
    }

    void test_aNewProfileNeedsAUsableName()
    {
        QFETCH(QString, name);
        QFETCH(QString, host);
        QFETCH(int, result);
        const QStringList profilesBefore = QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        THeadlessSession session;
        QCOMPARE(static_cast<int>(session.start({.profile = name, .connect = false, .host = host})), result);
        QVERIFY2(session.errorString().contains(name), qPrintable(session.errorString()));
        QVERIFY(!HostManager::self());
        QCOMPARE(QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot), profilesBefore);
    }

    // The stored one answers from the credential store once the profile is made; one given for the run stands
    void test_aPasswordForTheRunOutlivesTheStoredOne()
    {
        const QString name = qsl("Headless Stored Secret");
        QVERIFY(writeProfile(name, QString()));
        // The first pool of the process reads the setting from Mudlet.ini, over anything set before it
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        CredentialManager store;
        std::optional<bool> stored;
        store.storePassword(name, qsl("character"), qsl("stored"), [&stored](const bool success, const QString&) {
            stored = success;
        });
        QTRY_VERIFY_WITH_TIMEOUT(stored.has_value(), 10000);
        QVERIFY(*stored);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .password = qsl("given")}), THeadlessSession::StartResult::started);
        // The profile's own lookup went to the same store first, so it has answered once this one has
        std::optional<bool> answered;
        store.retrievePassword(name, qsl("character"), [&answered](const bool success, QString, const QString&) {
            answered = success;
        });
        QTRY_VERIFY_WITH_TIMEOUT(answered.has_value(), 10000);
        QVERIFY(*answered);
        QCOMPARE(session.host()->getPass(), qsl("given"));
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->getPass(), qsl("stored"), 10000);
    }

    void test_installPackage()
    {
        const QString name = qsl("Headless Package");
        QVERIFY(writeProfile(name, QString()));
        const QString packagePath = mConfigDir.filePath(qsl("headlessPackage.xml"));
        QFile packageFile(packagePath);
        QVERIFY(packageFile.open(QIODevice::WriteOnly));
        packageFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>fromPackage</name><packageName></packageName><script>headlessPackageRan = "yes"</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)");
        packageFile.close();
        THeadlessSession session;
        QVERIFY(!session.installPackage(packagePath));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.installPackage(packagePath), qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("headlessPackage")));
        QCOMPARE(luaGlobalString(session.host(), "headlessPackageRan"), qsl("yes"));
        const QString missing = mConfigDir.filePath(qsl("missing.mpackage"));
        QVERIFY(!session.installPackage(missing));
        QVERIFY2(session.errorString().contains(missing), qPrintable(session.errorString()));
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
        session.shutdown(false);
        QCOMPARE(saveCount(name), 1);
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

    // What the GUI writes into a trigger's line or a prompt's, which is not at the end as print() writes, keeps
    // the lines the GUI shows: an echo into a trigger's line ends with that line
    void test_commandEchoesAndTriggerEchoesReachLocalText()
    {
        const QString name = qsl("Headless Local Text");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Welcome", [[echo(" <tagged>") echo(" <twice>") send("from a trigger")]])
tempTrigger("Second", [[echo(" <second>")]])
tempTrigger("Third", [[echo(" <third>")]]))lua")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QSignalSpy local(&session, &THeadlessSession::localText);
        game->write("Welcome\r\nSecond\r\nThird\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("hp 10> ")), 10000);
        QVERIFY(session.runLua(qsl(R"lua(echo("later\n"))lua")));
        QVERIFY(session.submit(qsl("look")));
        QCOMPARE(outputTexts(local).join(QString()), qsl(" <tagged> <twice>\nfrom a trigger\n <second>\n <third>\nlater\nlook\n"));
        QCOMPARE(local.constFirst().at(0).toString(), name);
    }

    // An open piece ends with its line: the next game line, a deleted line's successor, and the trigger's own
    // line, where echo() and print() from a trigger both land
    void test_localTextEndsAPieceWithItsLine()
    {
        QVERIFY(writeProfile(qsl("Headless Local Lines"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Local Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Alpha", [[echo(" <a>")]])
tempTrigger("Beta", [[echo(" <b>") deleteLine()]])
tempTrigger("Gamma", [[echo(" <c>")]]))lua")));
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(echo("HP: "))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Welcome\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("ok\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Alpha\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Beta\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Gamma\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("cleared") clearWindow() echo("end\n"))lua")));
        QCOMPARE(outputTexts(local).join(QString()), qsl("HP: \nok\n <a>\n <b>\n <c>\ncleared\nend\n"));
    }

    void test_errorConsoleTextIsSent()
    {
        const QString name = qsl("Headless Error Console");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("headlessBoom", function() error("headless boom") end) raiseEvent("headlessBoom"))lua")));
        const QString text = outputTexts(errors).join(QString());
        QVERIFY2(text.contains(qsl("headless boom")), qPrintable(text));
        QCOMPARE(errors.constFirst().at(0).toString(), name);
    }

    void test_aConnectionThatCannotBeMadeIsDisconnected()
    {
        const QString name = qsl("Headless Refused");
        QVERIFY(writeProfile(name, QString()));
        quint16 port = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            port = closed.serverPort();
        }
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(port)).first);

        THeadlessSession session;
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(disconnected.count(), 1, 10000);
        QCOMPARE(disconnected.constFirst().at(0).toString(), name);
        session.shutdown(false);
    }

    // A frontend that opens the profile again asks about that run's close alone
    void test_aStartForgetsAFailedSave()
    {
        const QString name = qsl("Headless Save Failed");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local current = getMudletHomeDir() .. "/current"
  os.rename(current, current .. ".moved")
  local file = io.open(current, "wb")
  file:write("not a folder")
  file:close()
end))lua")));
        session.shutdown(true);
        QVERIFY(session.saveFailed());
        const QString current = qsl("%1/current").arg(MudletApp::getMudletPath(enums::profileHomePath, name));
        QVERIFY(QFile::remove(current));
        QVERIFY(QDir().rename(current + qsl(".moved"), current));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.saveFailed());
        session.shutdown(true);
        QVERIFY(!session.saveFailed());
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
