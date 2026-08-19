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

// A connect starts with an asynchronous name lookup, and the server it is for lives in members
// that the next connect overwrites. These drive a second connect in while the first one's lookup
// is still outstanding - what a script calling connectToServer() during the profile's own connect
// does - and assert that the connection lands on the server that was asked for last.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>
#include <limits>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"


#include "GroupedTest.h"

// A game that says nothing and only counts who reaches it. Bound to 127.0.0.1 by name rather
// than to every interface, so the test can name a loopback address that is not this one and know
// that nothing there will answer.
class ConnectionCountingServer : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionCountingServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &ConnectionCountingServer::onNewConnection);
    }

    // Ephemeral port so parallel worktree runs never collide.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }
    int connectionCount() const { return mConnectionCount; }

private slots:
    void onNewConnection()
    {
        auto* client = mServer.nextPendingConnection();
        if (!client) {
            return;
        }
        ++mConnectionCount;
        connect(client, &QTcpSocket::disconnected, client, &QObject::deleteLater);
    }

private:
    QTcpServer mServer;
    int mConnectionCount = 0;
};

class TelnetConnectLookupRaceTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    ConnectionCountingServer* mpServer = nullptr;
    const QString mHostname = qsl("Test-TelnetConnectLookupRace");
    const QString mLocalhost = qsl("localhost");
    // A loopback address the stub is not listening on. Naming it rather than 127.0.0.1 is what
    // makes a connect meant for it distinguishable from one meant for the stub; whether the
    // platform can route to it does not matter, because either way nothing answers there.
    const QString mOtherLoopback = qsl("127.0.0.2");
    // TEST-NET-1, reserved for documentation and so routed nowhere. A connect aimed here hangs in
    // ConnectingState rather than being refused, which is what makes acting on a lookup nobody is
    // waiting on tell against a build that does act on it.
    const QString mBlackhole = qsl("192.0.2.1");
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
        mpServer = new ConnectionCountingServer(qApp);
        QVERIFY2(mpServer->start(), "ConnectionCountingServer failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
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

    // The event a superseded connect leaves behind: its lookup is answered after a later connect
    // has moved the server on, so it names a host nobody asked for any more while the port beside
    // it has already changed. Acted on, it dials that pairing and the connect that was actually
    // wanted is refused for a socket that is already busy. Injected rather than raced for, since
    // which of two outstanding lookups is answered first is up to the resolver.
    void test_aLookupASecondConnectSupersededIsNotActedOn()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(disconnectProfile(host));

        QVERIFY2(waitForConnection(host,
                                   [&]() {
                                       host->mTelnet.connectIt(mLocalhost, mPort);
                                       host->mTelnet.slot_socketHostFound(supersededLookup());
                                   }),
                 "a lookup left over from a superseded connect took the connection with it");
        // The drop deliberately leaves the "still looking up" flag alone, which the tab's
        // connection indicator reads, so it has to have been cleared by the wanted result.
        QCOMPARE(host->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState);
    }

    // The same result arriving when there is no connect outstanding at all. Nothing is waiting on
    // it, so acting on it would dial a game the profile has finished with.
    void test_aLookupThatOutlivesTheConnectEntirelyIsNotActedOn()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(disconnectProfile(host));

        const int before = mpServer->connectionCount();
        host->mTelnet.slot_socketHostFound(supersededLookup());
        QTest::qWait(2000);
        QCOMPARE(mpServer->connectionCount(), before);
        QCOMPARE(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
    }

    // Disconnecting while the name lookup is outstanding has no socket to close, so the result
    // arriving afterwards is the only thing left that could still make the connection.
    void test_disconnectingDuringTheLookupCallsTheConnectOff()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(disconnectProfile(host));

        const int before = mpServer->connectionCount();
        host->mTelnet.connectIt(mLocalhost, mPort);
        host->mTelnet.disconnectIt();
        QTest::qWait(2000);
        QCOMPARE(mpServer->connectionCount(), before);
    }

    // Two connects in one turn of the event loop leave two lookups outstanding at once, which is
    // what a script calling connectToServer() during the profile's own connect does.
    void test_aSecondConnectDuringTheFirstsLookupStillConnects()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(disconnectProfile(host));

        QVERIFY2(waitForConnection(host,
                                   [&]() {
                                       host->mTelnet.connectIt(mOtherLoopback, mPort);
                                       host->mTelnet.connectIt(mLocalhost, mPort);
                                   }),
                 "the connect issued while the first lookup was outstanding never reached the game");
    }

    // The guard that drops the superseded lookup must not be left standing, or the connect after
    // it is dropped as well and the profile can never connect again.
    void test_anOrdinaryConnectStillWorksAfterASupersededOne()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(disconnectProfile(host));

        QVERIFY2(waitForConnection(host,
                                   [&]() {
                                       host->mTelnet.connectIt(mOtherLoopback, mPort);
                                       host->mTelnet.connectIt(mLocalhost, mPort);
                                   }),
                 "the racing pair never connected, so this proves nothing about what follows");

        QVERIFY(disconnectProfile(host));
        QVERIFY2(waitForConnection(host,
                                   [&]() {
                                       host->mTelnet.connectIt(mLocalhost, mPort);
                                   }),
                 "a plain connect after a superseded lookup never reached the game");
    }

private:
    // What the resolver hands back for a connect that a later one has replaced: a good answer,
    // for the wrong server, carrying the id of a lookup nobody is waiting on any more.
    QHostInfo supersededLookup() const
    {
        QHostInfo lookup;
        lookup.setLookupId(std::numeric_limits<int>::max());
        lookup.setHostName(mBlackhole);
        lookup.setAddresses({QHostAddress(mBlackhole)});
        return lookup;
    }

    // Answers rather than asserting: a QVERIFY here would only return from this helper, leaving
    // the test slot to run on and bury the real failure under timeouts.
    [[nodiscard]] bool disconnectProfile(Host* host)
    {
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);
        host->mTelnet.disconnectIt();
        const bool done = QTest::qWaitFor(
                [&]() {
                    return !disconnected.isEmpty();
                },
                8000);
        if (!done) {
            qWarning("the profile never disconnected from the stub");
        }
        return done;
    }

    // Both ends have to agree the connection is up: asking Mudlet about one it has not made yet
    // gets answers that are about not being connected.
    [[nodiscard]] bool waitForConnection(Host* host, const std::function<void()>& connectAction = {})
    {
        const int before = mpServer->connectionCount();
        QSignalSpy connected(&host->mTelnet, &cTelnet::signal_connected);
        if (connectAction) {
            connectAction();
        }
        const bool made = QTest::qWaitFor(
                [&]() {
                    return mpServer->connectionCount() > before && !connected.isEmpty();
                },
                15000);
        if (!made) {
            qWarning("the profile did not connect to the stub");
        }
        return made;
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
        if (!connected.wait(5000)) {
            qWarning("could not connect to the stub");
            return nullptr;
        }
        host->mEchoLuaErrors = true;
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

#include "TelnetConnectLookupRaceTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetConnectLookupRaceTest)
