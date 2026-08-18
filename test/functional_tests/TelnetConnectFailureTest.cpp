/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// A connection attempt that is refused never reaches the connected state, and Qt only signals a
// disconnection for a socket that did reach it. These connect a real profile to a port nothing is
// listening on and assert that the player is told, that scripts hear about it, and that a profile
// set to reconnect automatically tries again.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A game that says nothing and only counts who arrives. It can be taken off the air and put back
// on the same port, which is how a game that is down and then comes back is played out here.
class SilentGameServer : public QObject
{
    Q_OBJECT

public:
    explicit SilentGameServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &SilentGameServer::onNewConnection);
    }

    // Ephemeral port so parallel worktree runs never collide. The port is kept because close()
    // forgets it and restart() needs it.
    bool start()
    {
        const bool listening = mServer.listen(QHostAddress::LocalHost, 0);
        mPort = mServer.serverPort();
        return listening;
    }
    // The same port again, so a connection that was refused can be retried into a game that is up.
    // Retried for a moment because a connection closed on that port can hold it in TIME_WAIT,
    // which Windows - unlike the platforms that set SO_REUSEADDR - refuses to bind over.
    bool restart()
    {
        QElapsedTimer waited;
        waited.start();
        while (!mServer.listen(QHostAddress::LocalHost, mPort)) {
            if (waited.hasExpired(5000)) {
                return false;
            }
            QTest::qWait(100);
        }
        return true;
    }
    void stop() { mServer.close(); }

    quint16 serverPort() const { return mPort; }
    int connectionCount() const { return mConnectionCount; }

    // Bytes to answer the next connection with. Plain text where TLS is expected is what fails a
    // secure handshake outright, rather than leaving it waiting.
    void setGreeting(const QByteArray& greeting) { mGreeting = greeting; }

    // The game dropping the player, which is how most disconnections actually happen.
    void dropClient()
    {
        if (mClient) {
            mClient->disconnectFromHost();
        }
    }

private slots:
    void onNewConnection()
    {
        QTcpSocket* client = mServer.nextPendingConnection();
        if (!client) {
            return;
        }
        ++mConnectionCount;
        mClient = client;
        connect(client, &QTcpSocket::disconnected, client, &QObject::deleteLater);
        if (!mGreeting.isEmpty()) {
            client->write(mGreeting);
            client->flush();
        }
    }

private:
    QTcpServer mServer;
    QPointer<QTcpSocket> mClient;
    QByteArray mGreeting;
    quint16 mPort = 0;
    int mConnectionCount = 0;
};

class TelnetConnectFailureTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    SilentGameServer* mpServer = nullptr;
    const QString mHostname = qsl("Test-TelnetConnectFailure");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new SilentGameServer(qApp);
        QVERIFY2(mpServer->start(), "SilentGameServer failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        // These assert on messages Mudlet writes, so they have to be the English ones whatever
        // the machine running them is set to.
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // The control for the two below: unless a connection that is made puts its own text on screen
    // and raises its own event where this test can see them, their absence proves nothing.
    void test_aConnectionThatIsMadeIsReportedAndRaisesItsEvent()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(recordEvents(host));

        // Connected again because the profile's first connection was made before there was any
        // script listening for the event it raised.
        QVERIFY2(disconnectAndWait(host), "the profile never noticed its own disconnect");
        host->mTelnet.reconnect();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->connectionCount() > 1;
                         },
                         8000),
                 "the profile did not connect to the stub a second time");

        QVERIFY2(waitForTextInBuffer(host, qsl("connection made")), "a connection that was made put nothing on screen, so this test cannot see connection messages at all");
        QVERIFY2(waitForRecordedEvent(host, qsl("sysConnectionEvent")), "sysConnectionEvent was not seen, so this test cannot see connection events at all");
    }

    // Nothing is listening, so the player is left looking at "Attempting an open connection..."
    // with no idea that it is never going to happen.
    void test_aRefusedConnectionIsReported()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY2(bufferContains(host, qsl("connection made")), "the control text is missing, so an absent failure message would prove nothing");

        const quint16 deadPort = connectToADeadPort(host);
        QVERIFY(deadPort);

        QVERIFY2(waitForTextInBuffer(host, qsl("Unable to connect")), "the failed connection attempt was never reported to the player");

        // On that line and not merely somewhere in the buffer: the message announcing the attempt
        // named the same port a moment earlier, and would answer for this on its own.
        QVERIFY2(lineContaining(host, qsl("Unable to connect")).contains(QString::number(deadPort)), "the failure message does not say which server could not be reached");
    }

    // Scripts learn that the profile is offline from sysDisconnectionEvent, and a connection that
    // was never made leaves them just as offline as one that was lost.
    void test_aRefusedConnectionRaisesTheDisconnectionEvent()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(recordEvents(host));

        QVERIFY(connectToADeadPort(host));

        QVERIFY2(waitForRecordedEvent(host, qsl("sysDisconnectionEvent")), "a connection attempt that failed raised no event, so scripts never hear about it");
    }

    // A game that is down refuses every connection instantly. Without noticing that, "Reconnect
    // automatically" gets one attempt and then gives up for good.
    void test_reconnectAutomaticallyTriesAgainAfterARefusedConnection()
    {
        Host* host = startProfile();
        QVERIFY(host);
        host->mTelnet.setAutoReconnect(true);

        const int connectionsBefore = mpServer->connectionCount();
        // The game goes down, and the profile's attempt to reach it is refused.
        mpServer->stop();
        const quint16 deadPort = connectToADeadPort(host, mpServer->serverPort());
        QVERIFY(deadPort);
        QVERIFY2(waitForTextInBuffer(host, qsl("Unable to connect")), "the connection attempt did not fail, so a retry proves nothing");
        QVERIFY2(waitForTextInBuffer(host, qsl("Trying again")), "no retry was announced to the player");

        // The game comes back up, and nothing in this test asks for another connection.
        QVERIFY2(mpServer->restart(), "the stub could not take its port back");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->connectionCount() > connectionsBefore;
                         },
                         30000),
                 "the profile never tried the game again, so reconnecting automatically is a single attempt");
    }

    // Reconnecting automatically is for a game that comes back, not for one the player has just
    // walked away from.
    void test_aDisconnectDuringTheRetryWaitCallsTheRetryOff()
    {
        Host* host = startProfile();
        QVERIFY(host);
        host->mTelnet.setAutoReconnect(true);

        mpServer->stop();
        QVERIFY(connectToADeadPort(host, mpServer->serverPort()));
        QVERIFY2(waitForTextInBuffer(host, qsl("Trying again")), "no retry was scheduled, so calling one off proves nothing");

        host->mTelnet.disconnectIt();

        // An attempt already under way cannot be called off - neither a name lookup nor a connect
        // in progress can be taken back - so it is given time to run out while the port is still
        // dead. Without this it is that attempt, and not a retry, that reaches the game below.
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                         },
                         20000),
                 "the profile was still trying to connect long after the disconnect");
        QTest::qWait(3s);

        const int connectionsBefore = mpServer->connectionCount();
        QVERIFY2(mpServer->restart(), "the stub could not take its port back");
        QTest::qWait(15s);
        QCOMPARE(mpServer->connectionCount(), connectionsBefore);
    }

    // Every ordinary disconnection reaches the new error handling as well, because Qt reports the
    // remote host closing as a socket error before it signals the disconnection. Those already
    // have their own message and their own event, and must not collect a second of either.
    void test_aGameDroppingTheConnectionIsNotReportedAsAFailureToConnect()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(recordEvents(host));

        mpServer->dropClient();
        QVERIFY2(waitForTextInBuffer(host, qsl("Socket got disconnected")), "the profile never noticed the game dropping the connection");

        QTest::qWait(1s);
        QVERIFY2(!bufferContains(host, qsl("Unable to connect")), "a connection the game dropped was reported as a failure to connect as well");
        QCOMPARE(recordedEventCount(host, qsl("sysDisconnectionEvent")), 1);
    }

    // The failure path must stay out of the way of a disconnect the player asked for, which
    // already has its own messages.
    void test_aDisconnectTheUserAskedForIsNotReportedAsAFailureToConnect()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(recordEvents(host));

        QVERIFY2(disconnectAndWait(host), "the profile never noticed its own disconnect");
        QVERIFY2(waitForTextInBuffer(host, qsl("User Disconnected")), "the disconnect the user asked for was not reported as such");

        QTest::qWait(1s);
        QVERIFY2(!bufferContains(host, qsl("Unable to connect")), "a disconnect the user asked for was reported as a failure to connect as well");
        QCOMPARE(recordedEventCount(host, qsl("sysDisconnectionEvent")), 1);
    }

#if !defined(QT_NO_SSL)
    // A secure connection to a game that speaks plain telnet on that port does reach the game, so
    // Qt signals the disconnection that follows the failed handshake and that is what reports it.
    // The error the handshake raises must not be taken for a connection that was never made.
    void test_aFailedSecureHandshakeIsReportedOnlyOnce()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(recordEvents(host));
        QVERIFY2(disconnectAndWait(host), "the profile never noticed its own disconnect");

        // Counted, not looked for: the disconnect above has already put that text on screen once.
        const int disconnectsBefore = countLines(host, qsl("Socket got disconnected"));

        // Plain telnet where the client is expecting TLS, which is what fails the handshake.
        mpServer->setGreeting(QByteArray("Welcome to the game.\r\n"));
        host->mSslTsl = true;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("recordedEvents = {}"));
        host->mTelnet.connectIt(mLocalhost, mPort);

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return countLines(host, qsl("Socket got disconnected")) > disconnectsBefore;
                         },
                         20000),
                 "the failed handshake was not reported by the disconnection path");
        QTest::qWait(1s);
        QVERIFY2(!bufferContains(host, qsl("Unable to connect")), "a failed handshake was reported as a failure to connect as well");
        QCOMPARE(recordedEventCount(host, qsl("sysDisconnectionEvent")), 1);
    }
#endif

private:
    // Takes a port that nothing is listening on - by binding one and letting it go - and points
    // the profile at it. Answers the port, or 0 if one could not be had.
    quint16 connectToADeadPort(Host* host, const quint16 knownDeadPort = 0)
    {
        quint16 deadPort = knownDeadPort;
        if (!deadPort) {
            QTcpServer reserver;
            if (!reserver.listen(QHostAddress::LocalHost, 0)) {
                qWarning("could not bind a port to then abandon");
                return 0;
            }
            deadPort = reserver.serverPort();
            reserver.close();
        }

        // A port let go is not a port nobody else can take, and a test asking about a connection
        // that failed has to know that this one does.
        QTcpSocket probe;
        probe.connectToHost(QHostAddress::LocalHost, deadPort);
        if (probe.waitForConnected(2000)) {
            qWarning("something is listening on the port picked to be refused");
            return 0;
        }

        if (!disconnectAndWait(host)) {
            qWarning("the profile never noticed its own disconnect");
            return 0;
        }
        // That disconnect raised a sysDisconnectionEvent of its own, and left behind it would be
        // taken for the one the failure below is being asked for.
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("recordedEvents = {}"));
        host->mTelnet.connectIt(qsl("127.0.0.1"), deadPort);
        return deadPort;
    }

    [[nodiscard]] bool disconnectAndWait(Host* host)
    {
        if (host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState) {
            return true;
        }
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);
        host->mTelnet.disconnectIt();
        return QTest::qWaitFor(
                [&]() {
                    return !disconnected.isEmpty();
                },
                8000);
    }

    // Events are only visible to a script, so one is asked to count every event it hears. Counted
    // rather than noted, because "reported twice" is as much a failure here as "not reported".
    [[nodiscard]] bool recordEvents(Host* host)
    {
        return host->getLuaInterpreter()->compileAndExecuteScript(qsl("recordedEvents = {}\n"
                                                                      "function recordEvent(event) recordedEvents[event] = (recordedEvents[event] or 0) + 1 end\n"
                                                                      "registerAnonymousEventHandler(\"sysConnectionEvent\", \"recordEvent\")\n"
                                                                      "registerAnonymousEventHandler(\"sysDisconnectionEvent\", \"recordEvent\")\n"));
    }

    // compileAndExecuteScript() only answers whether the script ran, so the count is arrived at by
    // asking successively larger yes/no questions about it.
    int recordedEventCount(Host* host, const QString& event)
    {
        int count = 0;
        while (host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert((recordedEvents[\"%1\"] or 0) > %2)").arg(event, QString::number(count)))) {
            ++count;
        }
        return count;
    }

    bool waitForRecordedEvent(Host* host, const QString& event)
    {
        return QTest::qWaitFor(
                [&]() {
                    return recordedEventCount(host, event) > 0;
                },
                8000);
    }

    // Case-insensitively, because the message for a connection that was made reads "Open
    // connection made" or "Connection made" depending on whether the build can do TLS.
    bool bufferContains(Host* host, const QString& text)
    {
        TBuffer& buffer = host->mpConsole->buffer;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i).contains(text, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    int countLines(Host* host, const QString& text)
    {
        int found = 0;
        TBuffer& buffer = host->mpConsole->buffer;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i).contains(text, Qt::CaseInsensitive)) {
                ++found;
            }
        }
        return found;
    }

    QString lineContaining(Host* host, const QString& text)
    {
        TBuffer& buffer = host->mpConsole->buffer;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i).contains(text, Qt::CaseInsensitive)) {
                return buffer.line(i);
            }
        }
        return QString();
    }

    bool waitForTextInBuffer(Host* host, const QString& text)
    {
        return QTest::qWaitFor(
                [&]() {
                    return bufferContains(host, text);
                },
                20000);
    }

    // Mirrors the helper the other functional tests use.
    Host* startProfile()
    {
        const QString port = QString::number(mPort);
        Host* host = TestProfile::create(mHostname, mLocalhost, port);
        if (!host) {
            qWarning("no active host available for the test");
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            qWarning("could not connect to the stub");
            return nullptr;
        }
        return host;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TelnetConnectFailureTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetConnectFailureTest)
