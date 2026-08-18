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

// Which protocols a profile is speaking is settled by negotiation with one game, so a profile
// that reconnects to a different game must arrive knowing nothing. These drive a real profile
// through a game that negotiates and then through one that offers nothing at all, and assert on
// what goes out on the wire to the second game.

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

// One telnet option command, as either side of the conversation sends it.
struct TelnetReply
{
    unsigned char command = 0;
    unsigned char option = 0;

    bool operator==(const TelnetReply& other) const { return command == other.command && option == other.option; }
};

// A game that announces whichever options the test asked for, and records how Mudlet answers.
// Everything it has recorded belongs to the connection it was recorded on, so accepting a new
// one starts the record over.
class ProtocolOfferServer : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolOfferServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &ProtocolOfferServer::onNewConnection);
    }

    // Ephemeral port so parallel worktree runs never collide.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    void setOffers(const QList<TelnetReply>& offers) { mOffers = offers; }
    // Announced once the client has started answering the first burst, rather than alongside it.
    // Games do not always fit their announcements into one packet, and a client that only ever
    // sees them arrive together has not been asked how it treats an option settled later.
    void setLateOffers(const QList<TelnetReply>& offers) { mLateOffers = offers; }
    int connectionCount() const { return mConnectionCount; }

    bool sawReply(const unsigned char command, const unsigned char option) const { return mReplies.contains(TelnetReply{command, option}); }
    bool sawSubnegotiationOf(const unsigned char option) const { return mSubnegotiationOptions.contains(option); }

    void sendRaw(const QByteArray& bytes)
    {
        if (mClient) {
            mClient->write(bytes);
            mClient->flush();
        }
    }

    void dropClient()
    {
        if (mClient) {
            mClient->disconnectFromHost();
        }
    }

private slots:
    void onNewConnection()
    {
        mClient = mServer.nextPendingConnection();
        if (!mClient) {
            return;
        }
        ++mConnectionCount;
        // The previous connection's replies would otherwise be credited to this one. That includes
        // replies still in flight while the old socket was closing, which is why this is done on
        // accepting the new connection rather than by the test before it asks for one.
        mBuffer.clear();
        mReplies.clear();
        mSubnegotiationOptions.clear();
        mLateOffersSent = false;
        connect(mClient, &QTcpSocket::readyRead, this, &ProtocolOfferServer::onReadyRead);
        connect(mClient, &QTcpSocket::disconnected, mClient, &QObject::deleteLater);

        writeBurst(mOffers);
    }

    void onReadyRead()
    {
        // sender(), not mClient: a socket from a previous connection can still emit readyRead
        // after mClient has moved on, and its bytes are not this connection's replies.
        auto* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket || socket != mClient) {
            return;
        }
        mBuffer.append(socket->readAll());
        parseBuffer();

        if (!mLateOffersSent && !mLateOffers.isEmpty()) {
            mLateOffersSent = true;
            writeBurst(mLateOffers);
        }
    }

private:
    // One write, the way a game announces what it speaks.
    void writeBurst(const QList<TelnetReply>& offers)
    {
        if (!mClient || offers.isEmpty()) {
            return;
        }
        QByteArray burst;
        for (const TelnetReply& offer : offers) {
            burst.append(TN_IAC);
            burst.append(static_cast<char>(offer.command));
            burst.append(static_cast<char>(offer.option));
        }
        mClient->write(burst);
        mClient->flush();
    }

    int findSubnegotiationEnd(int from) const
    {
        int j = from;
        while (j + 1 < mBuffer.size()) {
            if (static_cast<unsigned char>(mBuffer.at(j)) == static_cast<unsigned char>(TN_IAC)) {
                const unsigned char next = static_cast<unsigned char>(mBuffer.at(j + 1));
                if (next == static_cast<unsigned char>(TN_IAC)) {
                    j += 2;
                    continue;
                }
                if (next == static_cast<unsigned char>(TN_SE)) {
                    return j;
                }
            }
            ++j;
        }
        return -1;
    }

    void parseBuffer()
    {
        int i = 0;
        while (i < mBuffer.size()) {
            if (static_cast<unsigned char>(mBuffer.at(i)) != static_cast<unsigned char>(TN_IAC)) {
                ++i;
                continue;
            }
            if (i + 1 >= mBuffer.size()) {
                break;
            }
            const unsigned char command = static_cast<unsigned char>(mBuffer.at(i + 1));
            if (command == static_cast<unsigned char>(TN_IAC)) {
                i += 2;
                continue;
            }
            if (command == static_cast<unsigned char>(TN_WILL) || command == static_cast<unsigned char>(TN_WONT) || command == static_cast<unsigned char>(TN_DO)
                || command == static_cast<unsigned char>(TN_DONT)) {
                if (i + 2 >= mBuffer.size()) {
                    break;
                }
                mReplies.append(TelnetReply{command, static_cast<unsigned char>(mBuffer.at(i + 2))});
                i += 3;
                continue;
            }
            if (command == static_cast<unsigned char>(TN_SB)) {
                const int end = findSubnegotiationEnd(i + 2);
                if (end == -1) {
                    break;
                }
                mSubnegotiationOptions.insert(static_cast<unsigned char>(mBuffer.at(i + 2)));
                i = end + 2;
                continue;
            }
            i += 2;
        }
        mBuffer = mBuffer.mid(i);
    }

    QTcpServer mServer;
    QPointer<QTcpSocket> mClient;
    QByteArray mBuffer;
    QList<TelnetReply> mOffers;
    QList<TelnetReply> mLateOffers;
    bool mLateOffersSent = false;
    QList<TelnetReply> mReplies;
    QSet<unsigned char> mSubnegotiationOptions;
    int mConnectionCount = 0;
};

class TelnetReconnectStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    ProtocolOfferServer* mpServer = nullptr;
    const QString mHostname = qsl("Test-TelnetReconnectState");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    static constexpr unsigned char kWill = static_cast<unsigned char>(TN_WILL);
    static constexpr unsigned char kDo = static_cast<unsigned char>(TN_DO);
    static constexpr unsigned char kDont = static_cast<unsigned char>(TN_DONT);
    static constexpr unsigned char kGmcp = static_cast<unsigned char>(OPT_GMCP);
    static constexpr unsigned char kAtcp = static_cast<unsigned char>(OPT_ATCP);
    static constexpr unsigned char kMsdp = static_cast<unsigned char>(OPT_MSDP);
    static constexpr unsigned char kMssp = static_cast<unsigned char>(OPT_MSSP);
    static constexpr unsigned char kMsp = static_cast<unsigned char>(OPT_MSP);
    static constexpr unsigned char kMxp = static_cast<unsigned char>(OPT_MXP);
    static constexpr unsigned char kCharset = static_cast<unsigned char>(OPT_CHARSET);
    static constexpr unsigned char kNewEnviron = static_cast<unsigned char>(OPT_NEW_ENVIRON);
    static constexpr unsigned char k102 = static_cast<unsigned char>(OPT_102);
    static constexpr unsigned char kCompress = static_cast<unsigned char>(OPT_COMPRESS);
    static constexpr unsigned char kCompress2 = static_cast<unsigned char>(OPT_COMPRESS2);

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
        mpServer = new ProtocolOfferServer(qApp);
        QVERIFY2(mpServer->start(), "ProtocolOfferServer failed to bind a loopback port");
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

    // A GMCP game earlier in the session must not leave Mudlet sending GMCP subnegotiations at a
    // game that never asked for any. The game drops the connection here, which is how a player
    // ends up reconnecting in the first place.
    void test_gmcpIsNotSpokenAtTheGameAfterAGmcpGame()
    {
        // GMCP is announced only once Mudlet has started answering the opening burst, so this is
        // not only about options that arrive before it has finished negotiating.
        mpServer->setOffers({{kWill, kMssp}});
        mpServer->setLateOffers({{kWill, kGmcp}});
        Host* host = startProfile();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDo, kGmcp), "the game's GMCP offer was never accepted, so this proves nothing about leaking it");
        QVERIFY2(host->mTelnet.isGMCPEnabled(), "GMCP was accepted on the wire but the profile does not think it is running");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->sawSubnegotiationOf(kGmcp);
                         },
                         8000),
                 "no GMCP ever reached the first game");

        // The next game offers nothing at all.
        mpServer->setOffers({});
        mpServer->setLateOffers({});
        QVERIFY(reconnectAfterServerDrop(host));

        QVERIFY2(!host->mTelnet.isGMCPEnabled(), "the profile thinks GMCP is running on a game that never offered it");
        QVERIFY2(!luaSucceeds(host, qsl(R"(sendGMCP("Test.Ping"))")), "sendGMCP() was accepted for a game that never negotiated GMCP");
        QTest::qWait(500ms);
        QVERIFY2(!mpServer->sawSubnegotiationOf(kGmcp), "GMCP was sent to a game that never offered it");
    }

    // Nothing negotiated with one game may be left standing for the next one, so the whole set is
    // checked and not only GMCP.
    void test_everyNegotiatedProtocolIsForgottenOnReconnect()
    {
        Host* host = startProfile();
        QVERIFY(host);

        // MSDP is off by default, and this needs a game whose every offer is accepted.
        host->mEnableMSDP = true;
        mpServer->setOffers({{kWill, kGmcp}, {kWill, kMsdp}, {kWill, kMssp}, {kWill, kMsp}, {kWill, kMxp}, {kWill, kCharset}, {kWill, kNewEnviron}, {kWill, k102}});
        QVERIFY(reconnect(host));

        cTelnet& telnet = host->mTelnet;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return telnet.isGMCPEnabled() && telnet.isMSDPEnabled() && telnet.isMSSPEnabled() && telnet.isMSPEnabled() && telnet.isMXPEnabled() && telnet.isCHARSETEnabled()
                                    && telnet.isNewEnvironEnabled() && telnet.isChannel102Enabled();
                         },
                         8000),
                 "the game's offers were not all accepted, so the reconnect below proves nothing");

        mpServer->setOffers({}); // the next game offers nothing at all
        QVERIFY(reconnect(host));

        QVERIFY2(!telnet.isGMCPEnabled(), "GMCP survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isMSDPEnabled(), "MSDP survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isMSSPEnabled(), "MSSP survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isMSPEnabled(), "MSP survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isMXPEnabled(), "MXP survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isCHARSETEnabled(), "CHARSET survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isNewEnvironEnabled(), "NEW_ENVIRON survived a reconnect to a game that never offered it");
        QVERIFY2(!telnet.isChannel102Enabled(), "channel 102 survived a reconnect to a game that never offered it");
    }

    // ATCP is only ever accepted for a profile with GMCP switched off, so it needs its own game.
    void test_atcpIsForgottenOnReconnect()
    {
        Host* host = startProfile();
        QVERIFY(host);

        host->mEnableGMCP = false;
        mpServer->setOffers({{kWill, kAtcp}});
        QVERIFY(reconnect(host));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mTelnet.isATCPEnabled();
                         },
                         8000),
                 "the game's ATCP offer was not accepted, so the reconnect below proves nothing");

        mpServer->setOffers({});
        QVERIFY(reconnect(host));
        QVERIFY2(!host->mTelnet.isATCPEnabled(), "ATCP survived a reconnect to a game that never offered it");
    }

    // Negotiating MCCP arms a scan for the compression start sequence in the incoming byte stream.
    // Left armed, the next game can trip it with bytes it means as a plain subnegotiation, and
    // Mudlet then inflates everything after them - so the game's text is eaten, and refusing the
    // option is how Mudlet says it tried and failed.
    void test_mccpV2IsNotExpectedFromAGameThatNeverNegotiatedIt()
    {
        Host* host = startProfile();
        QVERIFY(host);

        mpServer->setOffers({{kWill, kCompress2}});
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDo, kCompress2), "MCCP v2 was not negotiated, so the reconnect below proves nothing");

        mpServer->setOffers({});
        QVERIFY(reconnect(host));

        QByteArray startSequence;
        startSequence.append(TN_IAC).append(TN_SB).append(kCompress2).append(TN_IAC).append(TN_SE);
        mpServer->sendRaw(startSequence + "this is not a compressed stream\r\n");

        // The empty subnegotiation ahead of it is well-formed telnet, so the game gets to say its
        // piece in plain text - and that it arrived is what makes the absence of a refusal below
        // mean something.
        QVERIFY2(waitForTextInBuffer(host, qsl("this is not a compressed stream")), "the game's plain text never arrived");
        QVERIFY2(!mpServer->sawReply(kDont, kCompress2), "Mudlet tried to decompress a game that had never negotiated MCCP v2");
    }

    // v1 is a separate flag, and is refused as a separate telnet option, so a leak of it hides
    // behind any assertion about v2.
    void test_mccpV1IsNotExpectedFromAGameThatNeverNegotiatedIt()
    {
        Host* host = startProfile();
        QVERIFY(host);

        mpServer->setOffers({{kWill, kCompress}});
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDo, kCompress), "MCCP v1 was not negotiated, so the reconnect below proves nothing");

        mpServer->setOffers({});
        QVERIFY(reconnect(host));

        // v1's start sequence is not valid telnet (it ends WILL SE rather than IAC SE), so it is
        // terminated by hand to leave the parser somewhere sane afterwards.
        QByteArray startSequence;
        startSequence.append(TN_IAC).append(TN_SB).append(kCompress).append(TN_WILL).append(TN_SE);
        QByteArray terminator;
        terminator.append(TN_IAC).append(TN_SE);
        mpServer->sendRaw(startSequence + "this is not a compressed stream" + terminator);

        QTest::qWait(1s);
        QVERIFY2(!mpServer->sawReply(kDont, kCompress), "Mudlet tried to decompress a game that had never negotiated MCCP v1");
    }

    // MXP tags are built up across incoming bytes, and the tag builder is asked only whether the
    // MXP processor is on - not whether this game negotiated MXP. An unfinished tag left over from
    // the last game therefore gets flushed into the next game's output at the first escape code.
    void test_aHalfBuiltMxpTagDoesNotSpillIntoTheNextGame()
    {
        Host* host = startProfile();
        QVERIFY(host);

        mpServer->setOffers({{kWill, kMxp}});
        QVERIFY(reconnect(host));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mTelnet.isMXPEnabled();
                         },
                         8000),
                 "MXP was not negotiated, so this proves nothing");

        // A tag the game starts and never finishes, with no newline after it - in the mode this
        // game negotiated, a newline would close it.
        mpServer->sendRaw("<b");
        QTest::qWait(200ms);

        mpServer->setOffers({});
        QVERIFY(reconnect(host));

        // An escape code is what makes Mudlet give up on a tag it is part way through building.
        mpServer->sendRaw("\033[1mplain text from the next game\r\n");
        QVERIFY2(waitForTextInBuffer(host, qsl("plain text from the next game")), "the next game's text never arrived");
        QVERIFY2(!bufferContains(host, qsl("<b")), "an unfinished tag from the previous game was printed into this one's output");
    }

    // A secure port is something one game advertised about itself. Offered to the next game it is
    // wrong, and accepting the offer writes that port into the profile.
    void test_theSecurePortOneGameAdvertisedIsNotOfferedForTheNext()
    {
        Host* host = startProfile();
        QVERIFY(host);

        // The advertisement is recorded either way; this only keeps the modal offer off the
        // screen, which a test cannot answer.
        host->mAskTlsAvailable = false;
        mpServer->setOffers({{kWill, kMssp}});
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDo, kMssp), "MSSP was not negotiated, so this proves nothing");

        QByteArray mssp;
        mssp.append(TN_IAC).append(TN_SB).append(kMssp);
        mssp.append(MSSP_VAR).append("HOSTNAME").append(MSSP_VAL).append("first.example.com");
        mssp.append(MSSP_VAR).append("TLS").append(MSSP_VAL).append("9999");
        mssp.append(TN_IAC).append(TN_SE);
        mpServer->sendRaw(mssp);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mMSSPTlsPort == 9999;
                         },
                         8000),
                 "the game's advertised secure port was never recorded, so this proves nothing");

        mpServer->setOffers({});
        QVERIFY(reconnect(host));

        QVERIFY2(host->mMSSPTlsPort == 0, "a secure port advertised by an earlier game is still on offer for one that never mentioned it");
        QVERIFY2(host->mMSSPHostName.isEmpty(), "an earlier game's MSSP hostname is still what the secure port offer is checked against");
    }

    // The reverse hazard: which protocols the player is willing to accept is theirs, lives on the
    // Host, and must survive the reconnect that clears what was negotiated.
    void test_theProfilesOwnProtocolPreferencesSurviveAReconnect()
    {
        Host* host = startProfile();
        QVERIFY(host);

        host->mEnableGMCP = true;
        host->mEnableMSDP = true;
        host->mEnableMSP = false;
        mpServer->setOffers({{kWill, kGmcp}, {kWill, kMsdp}, {kWill, kMsp}});
        QVERIFY(reconnect(host));
        QVERIFY(waitForReply(kDo, kGmcp));

        QVERIFY(reconnect(host));

        QVERIFY2(host->mEnableGMCP, "the profile's GMCP preference was cleared along with the negotiated state");
        QVERIFY2(host->mEnableMSDP, "the profile's MSDP preference was cleared along with the negotiated state");
        QVERIFY2(!host->mEnableMSP, "the profile's MSP preference was overwritten");

        // Forgetting what was negotiated must not stop the same game negotiating it again.
        QVERIFY2(waitForReply(kDo, kGmcp), "GMCP was not negotiated again on reconnecting to the same game");
        QVERIFY2(waitForReply(kDo, kMsdp), "MSDP was not negotiated again on reconnecting to the same game");
        QVERIFY2(waitForReply(kDont, kMsp), "a protocol the player had switched off was accepted");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mTelnet.isGMCPEnabled() && host->mTelnet.isMSDPEnabled();
                         },
                         8000),
                 "the profile does not think the renegotiated protocols are running");
        QVERIFY2(!host->mTelnet.isMSPEnabled(), "MSP was enabled despite the player switching it off");
    }

private:
    bool waitForReply(const unsigned char command, const unsigned char option)
    {
        return QTest::qWaitFor(
                [&]() {
                    return mpServer->sawReply(command, option);
                },
                8000);
    }

    bool bufferContains(Host* host, const QString& text)
    {
        TBuffer& buffer = host->mpConsole->buffer;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    bool waitForTextInBuffer(Host* host, const QString& text)
    {
        return QTest::qWaitFor(
                [&]() {
                    return bufferContains(host, text);
                },
                8000);
    }

    // Answers rather than asserting: a QVERIFY here would only return from this helper, leaving
    // the test slot to run on and bury the real failure under timeouts.
    [[nodiscard]] bool reconnect(Host* host)
    {
        const int before = mpServer->connectionCount();
        QSignalSpy connected(&host->mTelnet, &cTelnet::signal_connected);
        host->mTelnet.disconnectIt();
        host->mTelnet.reconnect();
        return waitForConnection(before, connected);
    }

    // The way a player actually gets here: the game drops them and they connect again.
    [[nodiscard]] bool reconnectAfterServerDrop(Host* host)
    {
        const int before = mpServer->connectionCount();
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);
        mpServer->dropClient();
        if (!QTest::qWaitFor(
                    [&]() {
                        return !disconnected.isEmpty();
                    },
                    8000)) {
            qWarning("the profile never noticed the game dropping the connection");
            return false;
        }

        QSignalSpy connected(&host->mTelnet, &cTelnet::signal_connected);
        host->mTelnet.reconnect();
        return waitForConnection(before, connected);
    }

    // Both ends have to agree that the new connection is up: the stub has to have accepted it
    // before its record means anything, and asking Mudlet about a connection it has not made yet
    // gets answers that are about not being connected.
    bool waitForConnection(const int before, QSignalSpy& connected)
    {
        const bool reconnected = QTest::qWaitFor(
                [&]() {
                    return mpServer->connectionCount() > before && !connected.isEmpty();
                },
                8000);
        if (!reconnected) {
            qWarning("the profile did not reconnect to the stub");
        }
        return reconnected;
    }

    // Whether the script ran to completion, which for a Lua API call that refuses is what says it
    // refused: warnArgumentValue() returns nil, so the assert() fails.
    bool luaSucceeds(Host* host, const QString& call) { return host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(%1)").arg(call)); }

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

#include "TelnetReconnectStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetReconnectStateTest)
