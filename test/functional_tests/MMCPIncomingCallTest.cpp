/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * The answering side of MMCP chat: a peer calling this profile's chat server.
 * No Lua spec can reach it - mmcp.startServer, mmcp.accept, mmcp.deny,
 * mmcp.peek, mmcp.request and mmcp.setDoNotDisturb are all left out of the Lua
 * mmcp table - so the server is started from here and the callers are plain
 * sockets speaking the protocol by hand.
 *
 * The profile is staged with auto-accept off, so a call waits to be answered,
 * and with peek requests allowed, so a peer asking who else is connected gets
 * an answer rather than a refusal.
 *
 * Run with: ctest -R MMCPIncomingCallTest -V
 */

#include <QPointer>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MMCP.h"
#include "MMCPClient.h"
#include "MMCPServer.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
const QByteArray csListenerName = QByteArrayLiteral("Listener");

QByteArray frame(const MMCPChatCommand command, const QByteArray& payload = QByteArray())
{
    return static_cast<char>(command) + payload + static_cast<char>(End);
}
} // namespace

// One peer calling in: a socket that has sent the MMCP handshake and keeps
// everything the chat server writes back to it.
class MMCPCaller : public QObject
{
    Q_OBJECT

public:
    explicit MMCPCaller(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mSocket, &QTcpSocket::readyRead, this, [this]() {
            mReceived += mSocket.readAll();
        });
    }

    bool dial(const quint16 port)
    {
        mSocket.connectToHost(QHostAddress::LocalHost, port);
        return mSocket.waitForConnected(5000);
    }

    // "CHAT:<name>\n<address><port in five columns>" in one write, as Mudlet's
    // own client sends it. A five digit port needs no padding, which keeps
    // this clear of how the padding of a shorter one is read back.
    void sendHandshake(const QByteArray& name) { send("CHAT:" + name + "\n127.0.0.112345"); }

    void send(const QByteArray& bytes)
    {
        mSocket.write(bytes);
        mSocket.flush();
    }

    const QByteArray& received() const { return mReceived; }
    void clearReceived() { mReceived.clear(); }

    bool waitToReceive(const QByteArray& needle, const int timeout = 5000)
    {
        return QTest::qWaitFor(
                [this, &needle]() {
                    return mReceived.contains(needle);
                },
                timeout);
    }

    bool waitForHangUp(const int timeout = 5000)
    {
        return QTest::qWaitFor(
                [this]() {
                    return mSocket.state() == QAbstractSocket::UnconnectedState;
                },
                timeout);
    }

    bool connected() const { return mSocket.state() == QAbstractSocket::ConnectedState; }

    void hangUp() { mSocket.disconnectFromHost(); }

private:
    QTcpSocket mSocket;
    QByteArray mReceived;
};

class MMCPIncomingCallTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    quint16 mPort = 0;
    const QString mProfileName = qsl("MMCPIncomingCall-Test");

    MMCPServer* server() const { return mpHost->mMMCPServer; }

    MMCPClient* client(const QString& name) const
    {
        for (const auto& pClient : server()->getClients()) {
            if (pClient && pClient->chatName() == name) {
                return pClient;
            }
        }
        return nullptr;
    }

    bool waitForClient(const QString& name) const
    {
        return QTest::qWaitFor(
                [this, &name]() {
                    return client(name) != nullptr;
                },
                5000);
    }

    bool waitForNoClient(const QString& name) const
    {
        return QTest::qWaitFor(
                [this, &name]() {
                    return client(name) == nullptr;
                },
                5000);
    }

    // Places a call and answers it, leaving a caller that is fully connected
    // and has already had the acceptance and our version string
    MMCPCaller* acceptedCaller(const QByteArray& name)
    {
        auto* caller = new MMCPCaller(this);
        if (!caller->dial(mPort)) {
            return nullptr;
        }
        caller->sendHandshake(name);
        if (!waitForClient(QString::fromLatin1(name)) || !server()->chatAccept(QString::fromLatin1(name)).first) {
            return nullptr;
        }
        if (!caller->waitToReceive(frame(Version, QByteArrayLiteral("Mudlet")).chopped(1))) {
            return nullptr;
        }
        caller->clearReceived();
        return caller;
    }

    void hangUp(MMCPCaller* caller, const QString& name)
    {
        caller->hangUp();
        QVERIFY2(waitForNoClient(name), qPrintable(qsl("%1 is still in the peer list after hanging up").arg(name)));
    }

    bool writeStagedProfile()
    {
        const QString folder = MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        if (!QDir().mkpath(folder)) {
            return false;
        }
        // The shape XMLexport::writeHost() gives the MMCP child of <Host>
        const QByteArray xml =
                QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                  "<!DOCTYPE MudletPackage>\n"
                                  "<MudletPackage version=\"1.001\">\n"
                                  "<HostPackage>\n"
                                  "<Host>\n"
                                  "<MMCP chatName=\"Listener\" chatPort=\"4050\" chatPrefix=\"\" autostartServer=\"no\" allowPeekRequests=\"yes\" prefixEmotes=\"no\" chatMessageNewline=\"yes\" "
                                  "autoAcceptCalls=\"no\" snoopInMain=\"yes\"/>\n"
                                  "</Host>\n"
                                  "</HostPackage>\n"
                                  "</MudletPackage>\n");
        QFile file(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
        return file.open(QIODevice::WriteOnly | QIODevice::Text) && file.write(xml) == xml.size();
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

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(writeStagedProfile(), "could not write the staged profile save");
        mpHost = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(mpHost);
        QVERIFY2(mpHost->mLoadedOk, "the staged profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(mProfileName, false);
        QVERIFY2(!mpHost->getMMCPAutoAcceptCalls() && mpHost->getMMCPAllowPeekRequests(), "the staged profile did not set the MMCP options these cases rely on");
        QCOMPARE(mpHost->getMMCPChatName(), QString::fromLatin1(csListenerName));

        mpHost->initMMCPServer();
        // 0 lets the OS pick, so parallel runs never collide on MMCP's 4050
        QVERIFY(server()->startServer(0).first);
        mPort = server()->serverPort();
        QVERIFY(mPort != 0);
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(profilePath).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aCallWaitsToBeAnswered()
    {
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake("Alpha");
        QVERIFY2(waitForClient(qsl("Alpha")), "the call never showed up in the peer list");
        QVERIFY(client(qsl("Alpha"))->isPending());
        QVERIFY2(caller.received().isEmpty(), "the caller was answered before anybody accepted the call");

        QVERIFY(server()->chatAccept(qsl("Alpha")).first);
        QVERIFY2(caller.waitToReceive("YES:" + csListenerName + "\n"), caller.received().constData());
        // the version follows the acceptance, and a Mudlet peer is told it is
        // talking to Mudlet
        QVERIFY2(caller.waitToReceive(frame(Version, QByteArrayLiteral("Mudlet")).chopped(1)), caller.received().constData());
        QVERIFY(!client(qsl("Alpha"))->isPending());

        // a call that is already answered cannot be answered again
        const auto again = server()->chatAccept(qsl("Alpha"));
        QVERIFY(!again.first);
        QCOMPARE(again.second, qsl("failed to accept incoming call"));
        const auto deny = server()->chatDeny(qsl("Alpha"));
        QVERIFY(!deny.first);
        QCOMPARE(deny.second, qsl("failed to deny incoming call"));
        QVERIFY(caller.connected());

        hangUp(&caller, qsl("Alpha"));
    }

    void test_aDeniedCallIsToldNoAndDropped()
    {
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake("Beta");
        QVERIFY(waitForClient(qsl("Beta")));

        // by id this time, which is what the pending-call notice tells the user
        // to type
        QVERIFY(server()->chatDeny(client(qsl("Beta"))->id()).first);
        QVERIFY2(caller.waitToReceive("NO:" + csListenerName + "\n"), caller.received().constData());
        QVERIFY2(caller.waitForHangUp(), "the denied caller was left connected");
        QVERIFY(waitForNoClient(qsl("Beta")));
    }

    void test_answeringNobodyFailsAndACallerCanGiveUpWaiting()
    {
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake("Gamma");
        QVERIFY(waitForClient(qsl("Gamma")));

        const auto accepted = server()->chatAccept(qsl("Nobody"));
        QVERIFY(!accepted.first);
        QCOMPARE(accepted.second, qsl("failed to accept incoming call"));
        QVERIFY(!server()->chatDeny(qsl("Nobody")).first);
        QVERIFY(client(qsl("Gamma"))->isPending());

        // and a caller who gives up waiting takes the call out of the list
        // before anybody answered it
        caller.hangUp();
        QVERIFY(waitForNoClient(qsl("Gamma")));
    }

    void test_doNotDisturbRefusesACall()
    {
        QVERIFY(!server()->isDoNotDisturb());
        server()->toggleDoNotDisturb();
        QVERIFY(server()->isDoNotDisturb());

        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake("Delta");
        QVERIFY2(caller.waitToReceive("NO:" + csListenerName + "\n"), caller.received().constData());
        QVERIFY2(caller.waitForHangUp(), "the refused caller was left connected");
        QVERIFY(!client(qsl("Delta")));

        server()->toggleDoNotDisturb();
        QVERIFY(!server()->isDoNotDisturb());
    }

    void test_aNameOfUpTo64CharactersIsAccepted()
    {
        const QByteArray name(64, 'n');
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake(name);
        QVERIFY2(waitForClient(QString::fromLatin1(name)), "a 64 character name was refused");
        QVERIFY(server()->chatDeny(QString::fromLatin1(name)).first);
        QVERIFY(caller.waitForHangUp());
    }

    void test_aLongerNameIsRefused()
    {
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.sendHandshake(QByteArray(65, 'n'));
        QVERIFY2(caller.waitToReceive("NO:" + csListenerName + "\n"), caller.received().constData());
        QVERIFY(caller.waitForHangUp());
        QVERIFY(server()->getClients().isEmpty());
    }

    void test_aMalformedHandshakeIsDroppedWithoutAReply_data()
    {
        QTest::addColumn<QByteArray>("handshake");
        QTest::newRow("not a chat call") << QByteArrayLiteral("GET / HTTP/1.0\r\n\r\n");
        QTest::newRow("no room for a port") << QByteArrayLiteral("CHAT:Epsilon\n1234");
        QTest::newRow("a port that is not a number") << QByteArrayLiteral("CHAT:Epsilon\n127.0.0.1porty");
    }

    void test_aMalformedHandshakeIsDroppedWithoutAReply()
    {
        QFETCH(QByteArray, handshake);
        MMCPCaller caller;
        QVERIFY(caller.dial(mPort));
        caller.send(handshake);
        QVERIFY2(caller.waitForHangUp(), "a malformed handshake was left connected");
        QVERIFY2(caller.received().isEmpty(), caller.received().constData());
        QVERIFY(server()->getClients().isEmpty());
    }

    void test_peekAndRequestAskThePeer()
    {
        MMCPCaller* caller = acceptedCaller("Zeta");
        QVERIFY(caller);

        QVERIFY(server()->peek(qsl("Zeta")).first);
        QVERIFY2(caller->waitToReceive(frame(PeekConnections)), caller->received().toHex().constData());
        QVERIFY(server()->request(qsl("Zeta")).first);
        QVERIFY2(caller->waitToReceive(frame(RequestConnections)), caller->received().toHex().constData());

        QCOMPARE(server()->peek(qsl("Nobody")).second, qsl("no client by that name or id"));
        QCOMPARE(server()->request(qsl("Nobody")).second, qsl("no client by that name or id"));

        hangUp(caller, qsl("Zeta"));
    }

    void test_aPeerAloneIsToldThereIsNobodyElse()
    {
        MMCPCaller* caller = acceptedCaller("Eta");
        QVERIFY(caller);

        caller->send(frame(PeekConnections));
        QVERIFY2(caller->waitToReceive(frame(Message, "<CHAT> " + csListenerName + " doesn't have any other connections")), caller->received().constData());

        // a connection request goes unanswered rather than getting an empty list
        caller->clearReceived();
        caller->send(frame(RequestConnections));
        QTest::qWait(300);
        QVERIFY2(caller->received().isEmpty(), caller->received().toHex().constData());

        hangUp(caller, qsl("Eta"));
    }

    void test_aPeerIsToldWhoElseIsConnected()
    {
        MMCPCaller* asking = acceptedCaller("Theta");
        QVERIFY(asking);
        MMCPCaller* other = acceptedCaller("Iota");
        QVERIFY(other);

        asking->send(frame(PeekConnections));
        QVERIFY2(asking->waitToReceive("~Iota~" + QByteArray(1, static_cast<char>(End))), asking->received().constData());
        QVERIFY(asking->received().startsWith(static_cast<char>(PeekList)));
        // only the others: nobody is told about themselves
        QVERIFY(!asking->received().contains("Theta"));

        asking->clearReceived();
        asking->send(frame(RequestConnections));
        QVERIFY2(asking->waitToReceive(QByteArray(1, static_cast<char>(End))), asking->received().toHex().constData());
        QVERIFY(asking->received().startsWith(QByteArray(1, static_cast<char>(ConnectionList)) + "127.0.0.1,"));
        // one address and port pair
        QCOMPARE(asking->received().count(','), 1);

        // a private peer is left out of both
        QVERIFY(server()->chatPrivate(qsl("Iota")).first);
        asking->clearReceived();
        asking->send(frame(PeekConnections));
        QVERIFY2(asking->waitToReceive("doesn't have any other connections"), asking->received().constData());
        asking->clearReceived();
        asking->send(frame(RequestConnections));
        QTest::qWait(300);
        QVERIFY2(asking->received().isEmpty(), asking->received().toHex().constData());

        hangUp(other, qsl("Iota"));
        hangUp(asking, qsl("Theta"));
    }

    void test_servingForwardsChatBetweenPeers()
    {
        MMCPCaller* served = acceptedCaller("Kappa");
        QVERIFY(served);
        MMCPCaller* other = acceptedCaller("Lambda");
        QVERIFY(other);

        // nobody is served yet, so a chat to everybody goes no further
        const QByteArray fromOther = frame(TextEveryone, "Lambda chats to everybody, 'first'\n");
        other->send(fromOther);
        QTest::qWait(300);
        QVERIFY2(served->received().isEmpty(), served->received().constData());

        QVERIFY(server()->serve(qsl("Kappa")).first);
        QVERIFY(served->waitToReceive("You are now being served by " + csListenerName));
        served->clearReceived();

        // a served peer hears everybody else's chat
        const QByteArray again = frame(TextEveryone, "Lambda chats to everybody, 'second'\n");
        other->send(again);
        QVERIFY2(served->waitToReceive(again), served->received().constData());

        // and what a served peer says is passed on to everybody else
        const QByteArray fromServed = frame(TextEveryone, "Kappa chats to everybody, 'third'\n");
        served->clearReceived();
        other->clearReceived();
        served->send(fromServed);
        QVERIFY2(other->waitToReceive(fromServed), other->received().constData());
        // but not back to the peer who said it
        QTest::qWait(200);
        QVERIFY2(!served->received().contains("'third'"), served->received().constData());

        hangUp(other, qsl("Lambda"));
        hangUp(served, qsl("Kappa"));
    }

    // Last, since it takes the server down
    void test_stoppingTheServer()
    {
        QTcpServer occupier;
        QVERIFY(occupier.listen(QHostAddress::Any, 0));
        MMCPServer second(mpHost);
        const auto busy = second.startServer(occupier.serverPort());
        QVERIFY2(!busy.first, "a second server started on a port that is already taken");
        QCOMPARE(busy.second, qsl("unable to start server"));

        QVERIFY(server()->isListening());
        QVERIFY(server()->stopServer().first);
        QVERIFY(!server()->isListening());
        const auto stopped = server()->stopServer();
        QVERIFY(!stopped.first);
        QCOMPARE(stopped.second, qsl("unable to stop server, it is not listening"));

        MMCPCaller caller;
        caller.dial(mPort);
        QVERIFY2(caller.waitForHangUp(2000), "the stopped server still took a call");
    }
};

#include "MMCPIncomingCallTest.moc"
MUDLET_GROUPED_TEST_MAIN(MMCPIncomingCallTest)
