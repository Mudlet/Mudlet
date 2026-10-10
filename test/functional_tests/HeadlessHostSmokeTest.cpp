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

#include <QApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
// After a QtNetwork header, which is what defines QT_NO_SSL
#if !defined(QT_NO_SSL)
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#endif
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

// A profile made without ever starting the main window, so it only has the null
// console view: its Lua, triggers and main console model have to work regardless.
class HeadlessHostSmokeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    const QString mHostname = qsl("Test-Headless-Host-Smoke");
    const QString mConnectingHostname = qsl("Test-Headless-Host-Connect");
    const QString mSecureHostname = qsl("Test-Headless-Host-Secure");
    const QString mHeldLineHostname = qsl("Test-Headless-Host-Held-Line");

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static bool mainBufferHolds(Host* host, const QString& text) { return host->mainConsoleModel().buffer.lineBuffer.join(QChar::LineFeed).contains(text); }

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

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
    }

    void cleanupTestCase()
    {
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_profileRunsLuaAndTriggersWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->consoleFrontend(), "consoleFrontend() must never be null.");

        // pcall keeps the failing assert's message, which compileAndExecuteScript() only logs
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessResult = 'not run'
local ok, err = pcall(function()
  assert(echo("main", "headless echo line\n") == true, "echo to main did not answer true")
  local missingOk, missingMsg = echo("noSuchHeadlessWindow", "text")
  assert(missingOk == nil and type(missingMsg) == "string", "echo to a missing window did not answer nil and a message")
  headlessTriggerHit = 'none'
  local id = tempTrigger("headless fed line", [[headlessTriggerHit = line; echo("main", "headless trigger echo\n")]])
  assert(id, "tempTrigger made no trigger")
  assert(feedTriggers("headless fed line\n") == true, "feedTriggers did not answer true")
end)
headlessResult = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessTriggerHit"), qsl("headless fed line"));
        QVERIFY2(mainBufferHolds(host, qsl("headless echo line")), "echo() to main never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless fed line")), "feedTriggers() never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless trigger echo")), "The trigger's echo never reached the main console model.");
        // Lets work the profile deferred, such as its first-launch timer, run before looking
        QCoreApplication::processEvents();
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making and running the profile created a widget.");
    }

    void test_profileConnectsAndLogsInWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        QTcpSocket* client = nullptr;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&]() {
                received.append(client->readAll());
            });
            client->write("Welcome to the headless test server.\r\n");
        });

        QVERIFY2(HostManager::self()->addHost(mConnectingHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mConnectingHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->setLogin(qsl("headlesshero"));
        host->setPass(qsl("headlesssecret"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessGreeting = 'none'; tempTrigger("headless test server", [[headlessGreeting = line]]))lua")));

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_COMPARE_WITH_TIMEOUT(luaGlobalString(host, "headlessGreeting"), qsl("Welcome to the headless test server."), 10000);
        QVERIFY2(mainBufferHolds(host, qsl("Welcome to the headless test server.")), "The game's line never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("Open connection made")), "The connection's own messages never reached the main console model.");
        // The login and password go out on timers, 2s and then 1s by default
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("headlesshero") && received.contains("headlesssecret"), 10000);

        host->mTelnet.disconnectIt();
        QTRY_COMPARE_WITH_TIMEOUT(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Connecting the profile created a widget.");
    }

    void test_heldLineCommitsBeforeDisconnectWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

        // Prose that runs right up to the wrap column, so undoing server wrap holds it back
        const QByteArray heldLine = "Welcome traveller, the gate stands open and";
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            client->write(heldLine + "\r\n");
            client->disconnectFromHost();
        });

        QVERIFY2(HostManager::self()->addHost(mHeldLineHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHeldLineHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mUndoServerWrap = true;
        host->mUndoServerWrapWidth = static_cast<int>(heldLine.size());
        // Long enough that only the disconnect, not either timer, can commit the held line
        host->mTelnet.setPostingTimeout(60000);
        host->mServerWrapFlushTimer.setInterval(60000);
        QSignalSpy held(&host->mainConsoleModel().mNotifier, &TConsoleModelNotifier::serverWrapLineHeld);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessHeldLine = 'none'; tempTrigger("the gate stands open", [[headlessHeldLine = line]]))lua")));
        QString heldLineAtDisconnect;
        // Scoped to this test, as the profile outlives the locals the slot writes to
        QObject receiver;
        connect(&host->mTelnet, &cTelnet::signal_disconnected, &receiver, [&]() {
            heldLineAtDisconnect = luaGlobalString(host, "headlessHeldLine");
        });

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_VERIFY_WITH_TIMEOUT(!heldLineAtDisconnect.isEmpty(), 10000);
        QCOMPARE(held.count(), 1);
        QCOMPARE(heldLineAtDisconnect, QString::fromUtf8(heldLine));
    }

#if !defined(QT_NO_SSL)
    void test_untrustedCertificateWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");
        if (QSslSocket::activeBackend() != QLatin1String("openssl")) {
            QSKIP("Serving a certificate from an in-memory PEM key is only relied on with the OpenSSL backend.");
        }

        // Self-signed, so the profile refuses it; valid until 2126 so it never fails as expired instead
        static constexpr char certificatePem[] = R"(-----BEGIN CERTIFICATE-----
MIIBfzCCASWgAwIBAgIUNVnXBt8EptvmkFkhxe/ibqLOLRIwCgYIKoZIzj0EAwIw
FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAwOTIxNTA1OFoYDzIxMjYwOTE1
MjE1MDU4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO
PQMBBwNCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qOfbYT0tapBHTFGBkf6NgxBGen
wL5TDeL9g3w57+FWiHtIKUylQhCoNb20o1MwUTAdBgNVHQ4EFgQUdeivXGb0CJyG
TeJIhMFeiKCyPV0wHwYDVR0jBBgwFoAUdeivXGb0CJyGTeJIhMFeiKCyPV0wDwYD
VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEA4ktl+ztKx3yhNaOQRVvo
t7BeROX3QMOcjmtFD1z7gMMCIBCdl4RZ9RpZqyngwieUaEVXNou189dJOrb5/4Iu
62HC
-----END CERTIFICATE-----)";
        static constexpr char keyPem[] = R"(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgrC6UFloRMFbttYkA
mx8Y3UWOzQ/JtyVfFEEK23/u4iGhRANCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qO
fbYT0tapBHTFGBkf6NgxBGenwL5TDeL9g3w57+FWiHtIKUylQhCoNb20
-----END PRIVATE KEY-----)";
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.setLocalCertificate(QSslCertificate(QByteArray(certificatePem)));
        configuration.setPrivateKey(QSslKey(QByteArray(keyPem), QSsl::Ec));
        QSslServer server;
        server.setSslConfiguration(configuration);
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");

        QVERIFY2(HostManager::self()->addHost(mSecureHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mSecureHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mSslTsl = true;
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);

        // Not 127.0.0.1: a secure connect dials the name the lookup returns, and Windows
        // reverse-resolves 127.0.0.1 to the machine's own name, whose addresses this server is not on
        host->mTelnet.connectIt(qsl("localhost"), server.serverPort());

        // The app opens the profile's connection preferences here, which needs a main window
        QTRY_VERIFY_WITH_TIMEOUT(!disconnected.isEmpty(), 10000);
        QVERIFY2(!host->mTelnet.getSslErrors().isEmpty(), "The connection did not fail on the certificate.");
        QVERIFY2(mainBufferHolds(host, qsl("self-signed")), "Why the connection was refused never reached the main console model.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The refused connection created a widget.");
    }
#endif

    void test_profileWithNoSettingsStoreTakesTheDefaults()
    {
        if (MudletApp::getQSettings()) {
            QSKIP("A settings store exists with no main window, so there are no defaults for having none to check.");
        }

        const QString hostname = qsl("Test-Headless-Host-Defaults");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no settings store.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->mMapperCenterSmallAreas);
    }
};

#include "HeadlessHostSmokeTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessHostSmokeTest)
