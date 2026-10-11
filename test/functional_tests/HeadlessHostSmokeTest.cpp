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
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSvgRenderer>
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
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TRoomDB.h"

#include "GroupedTest.h"

// A profile made without ever starting the main window, so it only has the null
// console view: its Lua, triggers and main console model have to work regardless.
class HeadlessHostSmokeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the media switches and the profile pool in the app; here nothing else would.
    // The media switches come first, as there, so they outlive the profiles that reach them.
    std::unique_ptr<MudletMedia> mpMedia;
    std::unique_ptr<HostManager> mpHostManager;
    const QString mHostname = qsl("Test-Headless-Host-Smoke");
    const QString mConnectingHostname = qsl("Test-Headless-Host-Connect");
    const QString mSecureHostname = qsl("Test-Headless-Host-Secure");
    const QString mHeldLineHostname = qsl("Test-Headless-Host-Held-Line");
    const QString mWindowsHostname = qsl("Test-Headless-Host-Windows");

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static bool mainBufferHolds(Host* host, const QString& text) { return host->mainConsoleModel().buffer.lineBuffer.join(QChar::LineFeed).contains(text); }

    // The shape a game serves for an MMP map download
    static QByteArray xmlMap()
    {
        return QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<map>
 <areas>
  <area id="1" name="Headless Xml Area"/>
 </areas>
 <rooms>
  <room id="3" area="1" title="Headless Xml Room" environment="2">
   <coord x="0" y="0" z="0"/>
  </room>
 </rooms>
</map>
)");
    }

    // A binary map of the profile's own, holding just room roomId; the profile's map is left empty
    static QByteArray binaryMapOf(Host* host, const int roomId)
    {
        TMap* map = host->mpMap.data();
        map->mapClear();
        const int areaId = map->mpRoomDB->addArea(qsl("Headless Binary Area"));
        map->addRoom(roomId);
        map->setRoomArea(roomId, areaId);
        QByteArray serialized;
        QDataStream out(&serialized, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        map->serialize(out);
        map->mapClear();
        return serialized;
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

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpMedia = std::make_unique<MudletMedia>();
        mpHostManager = std::make_unique<HostManager>();
    }

    void cleanupTestCase()
    {
        mpHostManager.reset();
        mpMedia.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_profileRunsLuaAndTriggersWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

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

    void test_windowsMadeWithNoMainWindowHaveModels()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mWindowsHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysLabelDeleted"), qsl("onHeadlessDeleted"));
        host->registerAnonymousEventHandler(qsl("sysMiniConsoleDeleted"), qsl("onHeadlessDeleted"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessWindows = 'not run'
headlessDeleted = {}
function onHeadlessDeleted(_, name) headlessDeleted[#headlessDeleted + 1] = name end
local ok, err = pcall(function()
  assert(createMiniConsole("headlessMini", 10, 20, 300, 200) == true, "createMiniConsole did not answer true")
  assert(windowType("headlessMini") == "miniconsole", "the miniconsole has no type")
  assert(echo("headlessMini", "mini line\n") == true, "echo to the miniconsole did not answer true")
  local again, againMsg = createMiniConsole("headlessMini", 0, 0, 10, 10)
  assert(again == false and againMsg:find("already exists"), "a second miniconsole of that name was not refused")

  assert(openUserWindow("headlessWindow") == true, "openUserWindow did not answer true")
  assert(windowType("headlessWindow") == "userwindow", "the user window has no type")
  assert(echo("headlessWindow", "window line\n") == true, "echo to the user window did not answer true")
  local placed, placedMsg = openUserWindow("headlessWindow", false, false, "sideways")
  assert(placed == nil and placedMsg:find("docking option"), "an unknown docking area was not refused")
  assert(windowType("headlessWindow") == "userwindow", "refusing the area took the user window away")
  assert(createMiniConsole("headlessWindow", "headlessNested", 0, 0, 50, 50) == true, "a miniconsole could not go in the user window")
  assert(createLabel("headlessWindow", "headlessNestedLabel", 0, 0, 50, 50, 1) == true, "a label could not go in the user window")
  local orphan, orphanMsg = createMiniConsole("noSuchHeadlessWindow", "headlessOrphan", 0, 0, 50, 50)
  assert(orphan == false and orphanMsg:find("not found"), "a miniconsole went into a window that does not exist")

  assert(createLabel("headlessLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(windowType("headlessLabel") == "label", "the label has no type")
  assert(echo("headlessLabel", "label text") == true, "echo to the label did not answer true")
  local clash, clashMsg = createLabel("headlessMini", 0, 0, 10, 10, 1)
  assert(clash == false and clashMsg:find("already exists"), "a label took the name of a miniconsole")

  createBuffer("headlessBuffer")
  assert(windowType("headlessBuffer") == "buffer", "the buffer has no type")
  assert(echo("headlessBuffer", "buffer line\n") == true, "echo to the buffer did not answer true")

  assert(deleteLabel("headlessLabel") == true, "deleteLabel did not answer true")
  assert(windowType("headlessLabel") == nil, "the deleted label still has a type")
  assert(echo("headlessLabel", "text") == nil, "echo to the deleted label did not fail")
  local gone, goneMsg = deleteLabel("headlessLabel")
  assert(gone == false and goneMsg:find("not found"), "deleting a missing label did not fail")

  assert(deleteMiniConsole("headlessMini") == true, "deleteMiniConsole did not answer true")
  assert(windowType("headlessMini") == nil, "the deleted miniconsole still has a type")
  assert(echo("headlessMini", "text") == nil, "echo to the deleted miniconsole did not fail")

  assert(deleteMiniConsole("headlessWindow") == true, "deleting the user window did not answer true")
  assert(windowType("headlessWindow") == nil, "the deleted user window still has a type")
  assert(windowType("headlessNested") == nil, "a miniconsole in the user window outlived it")
  assert(windowType("headlessNestedLabel") == nil, "a label in the user window outlived it")
end)
headlessWindows = ok and 'ok' or tostring(err)
headlessDeletedNames = table.concat(headlessDeleted, ",")
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessWindows"), qsl("ok"));
        // Only what a script deleted, as the GUI raises nothing for what goes with a user window
        QCOMPARE(luaGlobalString(host, "headlessDeletedNames"), qsl("headlessLabel,headlessMini,headlessWindow"));

        const TConsoleModel* buffer = host->windowRegistry().subConsoleModel(qsl("headlessBuffer"));
        QVERIFY2(buffer, "The buffer has no model.");
        QVERIFY(buffer->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("buffer line")));
        QVERIFY2(!mainBufferHolds(host, qsl("buffer line")), "Text echoed to the buffer reached the main console.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the windows created a widget.");
    }

    void test_labelTextReachesModelWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createLabel("headlessTextLabel", 5, 6, 70, 80, 1); echo("headlessTextLabel", "label words"))lua")));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("headlessTextLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mText, qsl("label words"));
        QCOMPARE(label->mGeometry, QRect(5, 6, 70, 80));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessLabelText = getLabelText("headlessTextLabel"))lua")));
        QCOMPARE(luaGlobalString(host, "headlessLabelText"), qsl("label words"));
    }

    // The trim that raises sysBufferShrinkEvent runs inside TBuffer::append(), and echo goes on using the
    // console's model after that returns, so a handler deleting the console must not free it there and then.
    void test_consoleDeletedByItsOwnShrinkEventWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");
        host->registerAnonymousEventHandler(qsl("sysBufferShrinkEvent"), qsl("onHeadlessShrink"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessShrinks = 0
function onHeadlessShrink(_, name)
  if name == "headlessShrinkBuffer" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkBuffer")
  elseif name == "headlessShrinkNested" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkWindow")
  end
end
local lines = string.rep("shrink line\n", 150)
createBuffer("headlessShrinkBuffer")
setConsoleBufferSize("headlessShrinkBuffer", 100, 10)
headlessShrinkEcho = tostring(echo("headlessShrinkBuffer", lines))
openUserWindow("headlessShrinkWindow")
createMiniConsole("headlessShrinkWindow", "headlessShrinkNested", 0, 0, 50, 50)
setConsoleBufferSize("headlessShrinkNested", 100, 10)
headlessShrinkNestedEcho = tostring(echo("headlessShrinkNested", lines))
headlessShrinkLeft = table.concat({tostring(windowType("headlessShrinkBuffer")), tostring(windowType("headlessShrinkWindow")), tostring(windowType("headlessShrinkNested"))}, ",")
)lua")));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(luaGlobalString(host, "headlessShrinks").toInt() >= 2, "A shrink event never reached the handler, so nothing was deleted mid-echo.");
        QCOMPARE(luaGlobalString(host, "headlessShrinkEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkNestedEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkLeft"), qsl("nil,nil,nil"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createBuffer("headlessShrinkBuffer"); echo("headlessShrinkBuffer", "made again\n"))lua")));
        const TConsoleModel* remade = host->windowRegistry().subConsoleModel(qsl("headlessShrinkBuffer"));
        QVERIFY2(remade, "A buffer of the deleted one's name could not be made again.");
        QVERIFY(remade->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("made again")));
    }

    // As MXP_spec.lua's checks of the same tags in the GUI: a frame is a miniconsole under its name, and
    // what the game sends between <DEST> and </DEST> goes there and not into the main window.
    void test_mxpFrameTakesDestTextWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFrames = 'not run'
local ok, err = pcall(function()
  setConfig("specialForceMXPProcessorOn", true)
  headlessDestTriggered = "no"
  local id = tempTrigger("MXPDEST", [[headlessDestTriggered = "yes"]])
  feedTriggers([[<FRAME Name="headlessFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  assert(windowType("headlessFrame") == "miniconsole", "the frame has no console")
  feedTriggers([[<DEST headlessFrame>MXPDEST first</DEST>]] .. "\n")
  feedTriggers([[<DEST headlessFrame>MXPDEST second</DEST>]] .. "\n")
  headlessFrameLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  feedTriggers([[<DEST headlessFrame EOF>MXPDEST after clear</DEST>]] .. "\n")
  headlessClearedLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  killTrigger(id)
  feedTriggers([[<FRAME headlessFrame ACTION="close">]] .. "\n")
  assert(windowType("headlessFrame") == nil, "the closed frame still has a console")
  setConfig("specialForceMXPProcessorOn", false)
end)
headlessFrames = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFrames"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessFrameLines"), qsl("MXPDEST first|MXPDEST second|"));
        QCOMPARE(luaGlobalString(host, "headlessClearedLines"), qsl("MXPDEST after clear|"));
        QVERIFY2(!mainBufferHolds(host, qsl("MXPDEST")), "Text sent to the frame reached the main console.");
        QCOMPARE(luaGlobalString(host, "headlessDestTriggered"), qsl("no"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The frame created a widget.");
    }

    void test_profileConnectsAndLogsInWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

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
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

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
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
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

    // A deleted window's model outlives the call that deleted it, as a real view's widget does, and goes
    // when deferred deletes run; twice, as each release has to queue the next.
    void test_deletedWindowModelGoesWithDeferredDeletes()
    {
        const QString hostname = qsl("Test-Headless-Host-Release");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        for (int round = 0; round < 2; ++round) {
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(createMiniConsole('releasedMini', 0, 0, 10, 10))")));
            TConsoleModel* model = host->windowRegistry().subConsoleModel(qsl("releasedMini"));
            QVERIFY2(model, "The mini console has no model.");
            const QPointer<TConsoleModelNotifier> notifier = &model->mNotifier;
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(deleteMiniConsole('releasedMini'))")));
            QVERIFY2(notifier, "The model went before deferred deletes ran.");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY2(!notifier, "The model outlived deferred deletes.");
        }
    }

    void test_profileGetsNullAppViewWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        TAppFrontend* app = TAppFrontend::instance();
        QVERIFY2(app, "TAppFrontend::instance() must never be null.");
        QVERIFY(!app->getActiveHost());
        QCOMPARE(app->profileTabIndex(mHostname), -1);
        QVERIFY(app->getOpenFileName(qsl("title"), QDir::tempPath()).isEmpty());
        QVERIFY(!app->quitting());

        // Scripts get this text once the command bindings stop checking for a main window themselves
        const TAppFrontend::CommandRequest request{.name = qsl("headless command")};
        QString error = qsl("left over from an earlier call");
        QCOMPARE(app->addAddonCommand(request, nullptr, QString(), error), -1);
        QCOMPARE(error, qsl("mudlet instance not available"));
        error = qsl("left over from an earlier call");
        QCOMPARE(app->setAddonCommandPulse(1, true, qsl("red"), qsl("blue"), 500, nullptr, error), false);
        QCOMPARE(error, qsl("mudlet instance not available"));

        const QString hostname = qsl("Test-Headless-Host-App-View");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // Each of these calls the app view without asking whether there is one
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAppResult = 'not run'
local ok, err = pcall(function()
  local tab, tabMsg = getProfileTabNumber()
  assert(tab == nil and type(tabMsg) == "string", "getProfileTabNumber did not answer nil and a message")
  assert(openWebPage("about:blank") == false, "openWebPage did not answer false")
  assert(loadWindowLayout() == false, "loadWindowLayout did not answer false")
  assert(showNotification("headless title", "headless text") == true, "showNotification did not answer true")
end)
headlessAppResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAppResult"), qsl("ok"));
    }

    void test_scriptsSeeNoMainWindowAsTheyDidBeforeTheNullAppView()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Answers");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // addCommand and setCommandPulse take this answer from the null view, removeCommand from its own check
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAnswersResult = 'not run'
local ok, err = pcall(function()
  local missing = "mudlet instance not available"
  local id, idMsg = addCommand({name = "Headless", menuPath = "Headless"})
  assert(id == nil and idMsg == missing, "addCommand answered " .. tostring(id) .. ", " .. tostring(idMsg))
  local pulse, pulseMsg = setCommandPulse(1, true)
  assert(pulse == nil and pulseMsg == missing, "setCommandPulse answered " .. tostring(pulse) .. ", " .. tostring(pulseMsg))
  local removed, removedMsg = removeCommand(1)
  assert(removed == nil and removedMsg == missing, "removeCommand answered " .. tostring(removed) .. ", " .. tostring(removedMsg))
  assert(invokeFileDialog(true, "headless") == "", "invokeFileDialog did not answer an empty string")
  alert(0)
  local shown, shownMsg = showToolBar("headless missing toolbar")
  assert(shown == nil and type(shownMsg) == "string", "showToolBar did not answer nil and a message")
end)
headlessAnswersResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAnswersResult"), qsl("ok"));
    }

    // As the GUI answers with no saved settings
    void test_mainWindowSettingsRoundTripWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Settings");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingsResult = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local n = select("#", ...)
    local got = ...
    assert(n == 1 and got == want, label .. " answered " .. n .. " values: " .. tostring(got))
  end
  local key = "showTabConnectionIndicators"
  check("getConfig default", false, getConfig(key))
  check("setConfig true", true, setConfig(key, true))
  check("getConfig after true", true, getConfig(key))
  check("setConfig false", true, setConfig(key, false))
  check("getConfig after false", false, getConfig(key))
end)
headlessSettingsResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingsResult"), qsl("ok"));
    }

    // As Other_spec.lua pins for the GUI: one event per real change, none for a write of the value held
    void test_settingChangesRaiseEventsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Setting-Events");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysSettingChanged"), qsl("onHeadlessSettingChanged"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingEvents = 'not run'
local events = {}
function onHeadlessSettingChanged(_, key, value)
  events[#events + 1] = {key = key, value = value, readBack = getConfig(key)}
end
local ok, err = pcall(function()
  local keys = {"muteMediaAPI", "muteMediaGame", "compactInputLine", "mapperPanelVisible", "enableClosedCaption", "advertiseScreenReader", "announceIncomingText"}
  for _, key in ipairs(keys) do
    local start = getConfig(key)
    for _, step in ipairs({{not start, 1}, {not start, 0}, {start, 1}}) do
      local value, want = step[1], step[2]
      events = {}
      assert(setConfig(key, value) == true, "setConfig " .. key .. " refused")
      assert(#events == want, key .. " set to " .. tostring(value) .. " raised " .. #events .. " events, not " .. want)
      if want == 1 then
        local event = events[1]
        assert(event.key == key and event.value == value and event.readBack == value,
          key .. " raised " .. tostring(event.key) .. " = " .. tostring(event.value) .. ", read back " .. tostring(event.readBack))
      end
    end
  end
end)
headlessSettingEvents = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingEvents"), qsl("ok"));
    }

    // As the GUI answers the same files
    void test_imageSizeWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Image-Size");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        QTemporaryDir images;
        QVERIFY(images.isValid());
        const auto writeFile = [&images](const QString& name, const QByteArray& content) {
            QFile file(images.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
        };
        QVERIFY(QImage(37, 21, QImage::Format_ARGB32).save(images.filePath(qsl("raster.png"))));
        QVERIFY(writeFile(qsl("document.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"48\"/>")));
        // QImage reads this by its content, which the SVG renderer cannot
        QVERIFY(QImage(5, 9, QImage::Format_ARGB32).save(images.filePath(qsl("raster.svg")), "PNG"));
        // Not named .svgz, so only its content says it is gzipped
        QVERIFY(writeFile(
                qsl("gzipped.svg"),
                QByteArrayLiteral(
                        "\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff\xb3\x29\x2e\x4b\x57\xa8\xc8\xcd\xc9\x2b\xb6\x55\xca\x28\x29\x29\xb0\xd2\xd7\x2f\x2f\x2f\xd7\x2b\x37\xd6\xcb\x2f\x4a\xd7\x37\x32\x30"
                        "\x30\xd0\x07\xaa\x50\x52\x28\xcf\x4c\x29\xc9\xb0\x55\x32\x31\x50\x52\xc8\x48\xcd\x4c\xcf\x28\xb1\x55\x32\x36\x50\xd2\xb7\x03\x00\x54\x4e\x04\xa0\x40\x00\x00\x00")));

        const QString missing = images.filePath(qsl("missing.png"));
        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("imageDir = \"%1/\"\nmissingRefusal = \"couldn't retrieve image size, is the location '%2' correct?\"\n").arg(images.path(), missing) + qsl(R"lua(
headlessImageSize = 'not run'
local ok, err = pcall(function()
  local function check(label, wantA, wantB, ...)
    local n = select("#", ...)
    local a, b = ...
    assert(n == 2 and a == wantA and b == wantB, label .. " answered " .. n .. " values: " .. tostring(a) .. ", " .. tostring(b))
  end
  check("PNG", 37, 21, getImageSize(imageDir .. "raster.png"))
  check("SVG", 64, 48, getImageSize(imageDir .. "document.svg"))
  check("raster named .svg", 5, 9, getImageSize(imageDir .. "raster.svg"))
  check("gzipped SVG", 40, 30, getImageSize(imageDir .. "gzipped.svg"))
  check("missing file", nil, missingRefusal, getImageSize(imageDir .. "missing.png"))
  check("empty path", nil, "image location cannot be an empty string", getImageSize(""))
end)
headlessImageSize = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessImageSize"), qsl("ok"));

        // QImage reads SVG too where the qsvg plugin is deployed, which would hide a broken helper from getImageSize
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("document.svg"))));
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("gzipped.svg"))));
        QVERIFY(!TLabelModel::svgCandidate(images.filePath(qsl("raster.svg"))));
        QSvgRenderer renderer;
        QVERIFY(TLabelModel::loadSvg(renderer, images.filePath(qsl("gzipped.svg"))));
        QCOMPARE(renderer.defaultSize(), QSize(40, 30));
    }

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

    // As MapDownloadTest's checks of the same downloads in the GUI: the map is stored and loaded,
    // and scripts get sysMapDownloadEvent, whichever kind of map the game serves
    void test_mapDownloadLoadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        // Nothing renders map progress with no main window, so a warning about it would be a false alarm
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Download");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        TMap* map = host->mpMap.data();
        const QByteArray binaryMap = binaryMapOf(host, 7);
        QVERIFY(!binaryMap.isEmpty());

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            // The request line names the map, and it comes first
            connect(
                    client,
                    &QTcpSocket::readyRead,
                    client,
                    [client, &binaryMap]() {
                        const QByteArray body = client->readAll().startsWith("GET /map.xml ") ? xmlMap() : binaryMap;
                        client->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                        client->disconnectFromHost();
                    },
                    Qt::SingleShotConnection);
        });
        host->registerAnonymousEventHandler(qsl("sysMapDownloadEvent"), qsl("onHeadlessMapDownload"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMapDownloads = 0; function onHeadlessMapDownload() headlessMapDownloads = headlessMapDownloads + 1 end")));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.xml").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The download did not start.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(map->mpRoomDB->getRoom(3), "The downloaded XML map was not loaded.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("1"));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.dat").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The second download did not start, so the first one never finished.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(!mainBufferHolds(host, qsl("failure in parsing")), "The downloaded binary map was reported as unreadable.");
        QVERIFY2(map->mpRoomDB->getRoom(7), "The downloaded binary map was not loaded.");
        QVERIFY2(!map->mpRoomDB->getRoom(3), "The binary map was loaded over the XML one rather than in its place.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("2"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Downloading a map created a widget.");
    }

    // As Mapper_spec.lua's checks of saveMap and loadMap in the GUI
    void test_loadMapReadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Load");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());
        QFile xmlFile(mapDir.filePath(qsl("headless.xml")));
        QVERIFY(xmlFile.open(QIODevice::WriteOnly));
        xmlFile.write(xmlMap());
        xmlFile.close();
        QFile binaryFile(mapDir.filePath(qsl("headless.dat")));
        QVERIFY(binaryFile.open(QIODevice::WriteOnly));
        binaryFile.write(binaryMapOf(host, 7));
        binaryFile.close();

        const QString script = qsl("headlessMapDir = [[%1]]\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLoad = 'not run'
local ok, err = pcall(function()
  local loaded, loadedMsg = loadMap(headlessMapDir .. "/headless.dat")
  assert(loaded == true, "loadMap of a binary map answered " .. tostring(loaded) .. ", " .. tostring(loadedMsg))
  assert(roomExists(7), "loadMap did not load the binary map")
  local imported, importedMsg = loadMap(headlessMapDir .. "/headless.xml")
  assert(imported == true, "loadMap of an XML map answered " .. tostring(imported) .. ", " .. tostring(importedMsg))
  assert(roomExists(3) and not roomExists(7), "loadMap did not replace the map with the XML one")
  assert(loadMap(headlessMapDir .. "/nosuchmap.dat") == false, "loadMap of a missing binary map did not answer false")
  local missing, missingMsg = loadMap(headlessMapDir .. "/nosuchmap.xml")
  assert(missing == nil and missingMsg:find("was not found"), "loadMap of a missing XML map answered " .. tostring(missing) .. ", " .. tostring(missingMsg))
end)
headlessMapLoad = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLoad"), qsl("ok"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Loading a map created a widget.");
    }
    // As the GUI answers once loadMap has made its mapper; with no main window the model is the open map
    void test_playerRoomAndJsonMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Lua");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());

        const QString script = qsl("headlessMapDir = [[%1]]\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLua = 'not run'
local ok, err = pcall(function()
  local function answers(...) return select("#", ...), ... end
  local count, first, second = answers(getPlayerRoom())
  assert(count == 2 and first == nil and second == "the player does not have a valid roomID set",
    "getPlayerRoom with no player room answered " .. tostring(first) .. ", " .. tostring(second))

  local area = addAreaName("Headless Lua Area")
  local kept = createRoomID()
  addRoom(kept); setRoomArea(kept, area)
  local moved = createRoomID()
  addRoom(moved); setRoomArea(moved, area)
  assert(saveMap(headlessMapDir .. "/rooms.dat") == true, "saveMap did not save the map")
  assert(loadMap(headlessMapDir .. "/rooms.dat") == true, "loadMap did not load the map")

  count, first, second = answers(centerview(moved))
  assert(count == 1 and first == true, "centerview answered " .. tostring(first) .. ", " .. tostring(second))
  count, first, second = answers(getPlayerRoom())
  assert(count == 1 and first == moved, "getPlayerRoom after centerview answered " .. tostring(first) .. ", " .. tostring(second))

  count, first, second = answers(saveJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "saveJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  deleteRoom(kept)
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "loadJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  assert(roomExists(kept), "loadJsonMap did not read back the map saveJsonMap wrote")
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/nosuchmap.json"))
  assert(count == 2 and first == nil and tostring(second):find("could not open file", 1, true),
    "loadJsonMap of a missing file answered " .. tostring(first) .. ", " .. tostring(second))
end)
headlessMapLua = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLua"), qsl("ok"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }

    // As the GUI answers with its mapper up and no secondary map view open
    void test_mapperSettingsAndViewsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Map-Config");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const int area = host->mpMap->mpRoomDB->addArea(qsl("Headless Config Area"));
        QVERIFY(host->mpMap->mpRoomDB->set2DMapZoom(area, 7.5));

        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("headlessArea = %1\nnotAnOption = \"'show3dMapView' isn't a valid configuration option\"\n").arg(area) + qsl(R"lua(
headlessMapConfig = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local got = {n = select("#", ...), ...}
    local same = got.n == want.n
    for i = 1, math.max(got.n, want.n) do
      same = same and got[i] == want[i]
    end
    assert(same, label .. " answered " .. got.n .. " values: " .. tostring(got[1]) .. ", " .. tostring(got[2]))
  end
  local function refused(message) return {n = 2, nil, message} end
  local function answered(value) return {n = 1, value} end

  local room = createRoomID()
  addRoom(room); setRoomArea(room, headlessArea)
  local ids = getMapViewIds()
  assert(type(ids) == "table" and next(ids) == nil, "getMapViewIds answered " .. tostring(ids))
  check("closeAllMapViews", answered(0), closeAllMapViews())
  local function counted(...) return select("#", ...), ... end
  local selectionCount, selection = counted(getMapSelection())
  assert(selectionCount == 1 and type(selection) == "table" and next(selection) == nil, "getMapSelection answered " .. selectionCount .. " values: " .. tostring(selection))
  check("clearMapSelection", answered(false), clearMapSelection())
  check("closeMapView", refused("view 99 not found"), closeMapView(99))
  check("getMapViewInfo", refused("view 99 not found"), getMapViewInfo(99))
  check("centerview in a view", refused("view 99 not found"), centerview(room, 99))
  check("getMapZoom in a view", refused("view 99 not found"), getMapZoom(headlessArea, 99))
  check("setMapZoom in a view", refused("view 99 not found"), setMapZoom(5, headlessArea, 99))
  check("createMapView", refused("no view manager available"), createMapView(headlessArea))

  check("getMapZoom of an area", answered(7.5), getMapZoom(headlessArea))
  check("getMapZoom of no such area", refused("number 9999 is not a valid areaID"), getMapZoom(9999))
  check("getMapZoom of the shown area", refused("no active mapper"), getMapZoom())
  check("setMapZoom of an area", answered(true), setMapZoom(12.25, headlessArea))
  check("getMapZoom after setMapZoom", answered(12.25), getMapZoom(headlessArea))
  check("setMapZoom too small", refused("zoom 2 is invalid, it must be at least 3"), setMapZoom(2, headlessArea))
  check("setMapZoom not finite", refused("zoom nan is invalid, it must be a finite number"), setMapZoom(0/0))
  check("setMapZoom of no such area", refused("number 9999 is not a valid areaID"), setMapZoom(5, 9999))
  check("setMapZoom of the shown area", refused("no active mapper"), setMapZoom(5))
  check("getMapZoom after refusals", answered(12.25), getMapZoom(headlessArea))
  check("setDefaultAreaVisible", answered(true), setDefaultAreaVisible(false))

  check("setConfig mapRoomSize", answered(true), setConfig("mapRoomSize", 6))
  check("getConfig mapRoomSize", answered(6), getConfig("mapRoomSize"))
  check("setConfig mapRoomSize 0", refused("mapRoomSize must be at least 1, got 0"), setConfig("mapRoomSize", 0))
  check("setConfig mapExitSize", answered(true), setConfig("mapExitSize", 3.5))
  check("getConfig mapExitSize", answered(3.5), getConfig("mapExitSize"))
  for _, key in ipairs({"mapRoundRooms", "showRoomIdsOnMap", "mapShowGrid"}) do
    check("setConfig " .. key, answered(true), setConfig(key, true))
    check("getConfig " .. key, answered(true), getConfig(key))
  end
  check("setConfig mapShowRoomBorders", answered(true), setConfig("mapShowRoomBorders", false))
  check("getConfig mapShowRoomBorders", answered(false), getConfig("mapShowRoomBorders"))
  check("setConfig showMapInfo", answered(true), setConfig("showMapInfo", "Headless"))
  check("setConfig mapInfoColor", answered(true), setConfig("mapInfoColor", {1, 2, 3}))
  local color = getConfig("mapInfoColor")
  assert(color[1] == 1 and color[2] == 2 and color[3] == 3 and color[4] == 255, "getConfig mapInfoColor did not read back the color set")
  check("setConfig mapInfoColor 'x'", refused("mapInfoColor requires a table {r, g, b} or {r, g, b, a}"), setConfig("mapInfoColor", "x"))
  check("getConfig showUpperLowerLevels default", answered(true), getConfig("showUpperLowerLevels"))
  check("setConfig showUpperLowerLevels", answered(true), setConfig("showUpperLowerLevels", false))
  check("getConfig showUpperLowerLevels", answered(false), getConfig("showUpperLowerLevels"))
  -- This drives the mapper widget, as in the GUI before its mapper exists
  check("setConfig show3dMapView", refused(notAnOption), setConfig("show3dMapView", false))
end)
headlessMapConfig = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapConfig"), qsl("ok"));
        QVERIFY2(!host->mpMap->getDefaultAreaShown(), "setDefaultAreaVisible(false) did not hide the default area.");
        QVERIFY2(host->mMapInfoContributors.contains(qsl("Headless")), "setConfig showMapInfo did not add the contributor.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }
};

#include "HeadlessHostSmokeTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessHostSmokeTest)
