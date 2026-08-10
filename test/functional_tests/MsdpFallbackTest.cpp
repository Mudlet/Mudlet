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

// MSDP is the starter UI's fallback for games with no GMCP, so it is negotiated only when the
// game offers nothing better - and asked for with a session-only setting, so removing or hiding
// the package leaves the player's own MSDP preference exactly as they left it.

#include <QtTest/QtTest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

// One telnet option command, as either side of the conversation sends it.
struct TelnetReply
{
    unsigned char command = 0;
    unsigned char option = 0;

    bool operator==(const TelnetReply& other) const { return command == other.command && option == other.option; }
};

// A game that announces whichever options the test asked for, in the order it asked for them,
// and records how Mudlet answers.
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
    // A second burst, written this many milliseconds after the first. Games do not always fit
    // their announcements into one packet, and a client that only ever sees them arrive together
    // has not been asked the question this feature exists to answer.
    void setLateOffers(const QList<TelnetReply>& offers, const int afterMs)
    {
        mLateOffers = offers;
        mLateOfferDelay = afterMs;
    }
    int connectionCount() const { return mConnectionCount; }

    void clearReplies()
    {
        mReplies.clear();
        mSubnegotiations.clear();
    }
    bool sawReply(const unsigned char command, const unsigned char option) const { return mReplies.contains(TelnetReply{command, option}); }

    bool sawSubnegotiationContaining(const QByteArray& text) const
    {
        for (const QByteArray& payload : mSubnegotiations) {
            if (payload.contains(text)) {
                return true;
            }
        }
        return false;
    }

private slots:
    void onNewConnection()
    {
        mClient = mServer.nextPendingConnection();
        if (!mClient) {
            return;
        }
        ++mConnectionCount;
        // The previous connection's leftovers would otherwise be parsed as this one's replies.
        mBuffer.clear();
        connect(mClient, &QTcpSocket::readyRead, this, &ProtocolOfferServer::onReadyRead);
        connect(mClient, &QTcpSocket::disconnected, mClient, &QObject::deleteLater);

        writeBurst(mClient, mOffers);
        if (!mLateOffers.isEmpty()) {
            // Addressed to this connection's socket, so a timer left over from a dropped connection
            // cannot deliver its burst down the one that replaced it.
            QTimer::singleShot(mLateOfferDelay, this, [this, client = QPointer<QTcpSocket>(mClient)]() {
                if (client) {
                    writeBurst(client, mLateOffers);
                }
            });
        }
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
    }

private:
    // One write, the way a game announces what it speaks.
    static void writeBurst(QTcpSocket* socket, const QList<TelnetReply>& offers)
    {
        QByteArray burst;
        for (const TelnetReply& offer : offers) {
            burst.append(TN_IAC);
            burst.append(static_cast<char>(offer.command));
            burst.append(static_cast<char>(offer.option));
        }
        socket->write(burst);
        socket->flush();
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
                mSubnegotiations.append(mBuffer.mid(i + 3, end - (i + 3)));
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
    int mLateOfferDelay = 0;
    QList<TelnetReply> mReplies;
    QList<QByteArray> mSubnegotiations;
    int mConnectionCount = 0;
};

class MsdpFallbackTest : public QObject
{
    Q_OBJECT

private:
    ProtocolOfferServer* mpServer = nullptr;
    const QString mHostname = qsl("Test-MsdpFallback");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    static constexpr unsigned char kMsdp = static_cast<unsigned char>(OPT_MSDP);
    static constexpr unsigned char kGmcp = static_cast<unsigned char>(OPT_GMCP);
    static constexpr unsigned char kWill = static_cast<unsigned char>(TN_WILL);
    static constexpr unsigned char kWont = static_cast<unsigned char>(TN_WONT);
    static constexpr unsigned char kDo = static_cast<unsigned char>(TN_DO);
    static constexpr unsigned char kDont = static_cast<unsigned char>(TN_DONT);

    // Comfortably past the hold in cTelnet's MSDP_OFFER_HOLD, so a test that expects no
    // acceptance has actually outlived the wait rather than raced it.
    static constexpr auto kPastTheHold = 3000ms;

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new ProtocolOfferServer(qApp);
        QVERIFY2(mpServer->start(), "ProtocolOfferServer failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
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

    // ---- the fallback itself -----------------------------------------------

    void test_aGameWithNoGmcpGetsMsdpNegotiated()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDo, kMsdp), "a game offering MSDP and no GMCP was never answered DO MSDP");
        QVERIFY2(host->mTelnet.isMSDPEnabled(), "MSDP was accepted on the wire but the profile does not think it is running");
        QVERIFY2(!mpServer->sawReply(kDont, kMsdp), "the offer was refused as well as accepted");

        // Accepting is only worth anything if the data actually starts flowing, and the accept
        // runs from a timer callback, so this is also what says a deferred sysProtocolEnabled
        // still reaches the starter UI's subscription.
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->sawSubnegotiationContaining("LIST");
                         },
                         8000),
                 "the MSDP handshake never reached the game");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->sawSubnegotiationContaining("REPORT");
                         },
                         8000),
                 "the starter UI never asked the game to report its vitals over MSDP");
        QVERIFY2(mpServer->sawSubnegotiationContaining("HEALTH"), "the vitals variables were missing from the REPORT request");
    }

    // The whole point of holding the answer: a game whose GMCP offer is in a later packet than its
    // MSDP one still gets MSDP refused. With the two in one packet the wait is never really tested,
    // because GMCP has always settled before the timer could fire.
    void test_aGmcpOfferInALaterPacketStillWinsTheRace()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        mpServer->setLateOffers({{kWill, kGmcp}}, 600);
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDont, kMsdp), "MSDP was never refused on a game whose GMCP offer came in a later packet");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "MSDP was answered before the game's GMCP offer had a chance to arrive");
        QVERIFY2(host->mTelnet.isGMCPEnabled(), "GMCP was not negotiated at all, so this proves nothing about preferring it");
    }

    // A held offer belongs to the connection it arrived on. Answering it afterwards would send an
    // unsolicited DO MSDP to whichever game came next.
    void test_anOfferHeldWhenTheConnectionDropsIsNeverAnswered()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(waitForReply(kDo, kMsdp));

        // Reconnect and drop again while the next offer is still being held.
        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QTest::qWait(200ms);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "the offer was already answered, so this never reaches the case it is about");

        mpServer->setOffers({}); // the next game offers nothing at all
        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "an offer held over from the dropped connection was answered to a game that never made one");
        QVERIFY2(!host->mTelnet.isMSDPEnabled(), "the profile thinks MSDP is running on a game that never offered it");
    }

    // Whether GMCP is running belongs to one connection: a GMCP game earlier in the session must
    // not go on suppressing the fallback for a game that has no GMCP at all.
    void test_anEarlierGmcpGameDoesNotSuppressTheNextGamesMsdp()
    {
        mpServer->setOffers({{kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(waitForReply(kDo, kGmcp));

        mpServer->setOffers({{kWill, kMsdp}});
        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDo, kMsdp), "a GMCP game from an earlier connection was still suppressing the MSDP fallback");
    }

    // Withdrawing an offer leaves nothing to answer, so the wait must not go on to accept it.
    void test_anOfferWithdrawnDuringTheWaitIsDropped()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        mpServer->setLateOffers({{kWont, kMsdp}}, 300);
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "MSDP was negotiated with a game that had withdrawn the offer");
        QVERIFY2(!host->mTelnet.isMSDPEnabled(), "the profile thinks MSDP is running after the game withdrew it");
    }

    void test_aGameOfferingGmcpTooKeepsMsdpOff()
    {
        // MSDP first: the answer to it cannot be decided until the rest of the burst has been read.
        mpServer->setOffers({{kWill, kMsdp}, {kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDont, kMsdp), "a game offering both protocols was never answered DONT MSDP");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "MSDP was negotiated on a game that offers GMCP");
        QVERIFY2(!host->mTelnet.isMSDPEnabled(), "the profile thinks MSDP is running on a GMCP game");
        QVERIFY2(host->mTelnet.isGMCPEnabled(), "GMCP was not negotiated at all, so this proves nothing about preferring it");
    }

    void test_gmcpArrivingFirstRefusesMsdpStraightAway()
    {
        mpServer->setOffers({{kWill, kGmcp}, {kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        // Nothing left to wait for once GMCP is settled, so the refusal has to be immediate rather
        // than held for the grace period an undecided offer gets.
        QVERIFY2(waitForReply(kDo, kGmcp), "GMCP was not negotiated at all, so this proves nothing about preferring it");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->sawReply(kDont, kMsdp);
                         },
                         1000),
                 "MSDP was held back even though GMCP was already settled when the offer arrived");
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "MSDP was negotiated despite GMCP already being on");
    }

    // A game asking us to speak MSDP, rather than offering to send it, is answered the other way
    // round - WILL/WONT, not DO/DONT - and has to reach the same decision.
    void test_aGameAskingUsToSpeakMsdpIsAnsweredTheSameWay()
    {
        mpServer->setOffers({{kDo, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kWill, kMsdp), "a request to speak MSDP on a GMCP-less game was never accepted");
        QVERIFY2(host->mTelnet.isMSDPEnabled(), "MSDP was accepted but the profile does not think it is running");
    }

    // Asking both ways is two telnet requests, and telnet expects an answer to each: holding one
    // must not swallow the other.
    void test_aGameAskingBothWaysGetsBothAnswered()
    {
        mpServer->setOffers({{kWill, kMsdp}, {kDo, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDo, kMsdp), "the game's offer to send MSDP went unanswered");
        QVERIFY2(waitForReply(kWill, kMsdp), "the game's request that we send MSDP went unanswered");
        QVERIFY2(host->mTelnet.isMSDPEnabled(), "MSDP was accepted but the profile does not think it is running");
    }

    void test_aGameAskingBothWaysGetsBothRefusedWhenItOffersGmcp()
    {
        mpServer->setOffers({{kWill, kMsdp}, {kDo, kMsdp}, {kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kDont, kMsdp), "the game's offer to send MSDP went unanswered");
        QVERIFY2(waitForReply(kWont, kMsdp), "the game's request that we send MSDP went unanswered");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp) && !mpServer->sawReply(kWill, kMsdp), "MSDP was negotiated on a game that offers GMCP");
    }

    void test_aGameAskingUsToSpeakMsdpIsStillRefusedWhenItOffersGmcp()
    {
        mpServer->setOffers({{kDo, kMsdp}, {kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(waitForReply(kWont, kMsdp), "a request to speak MSDP was never refused on a GMCP game");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kWill, kMsdp), "we offered to speak MSDP on a game that offers GMCP");
    }

    // ---- the player's own preference ---------------------------------------

    void test_aPlayerWhoTurnedMsdpOnKeepsItEvenOnGmcpGames()
    {
        mpServer->setOffers({{kWill, kMsdp}, {kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        host->mEnableMSDP = true;
        mpServer->clearReplies();
        QVERIFY(reconnect(host));

        QVERIFY2(waitForReply(kDo, kMsdp), "an explicitly enabled MSDP was refused because the game also offers GMCP");
        QVERIFY2(!mpServer->sawReply(kDont, kMsdp), "the player's own MSDP preference was overruled");
        QVERIFY2(host->mTelnet.isMSDPEnabled(), "MSDP was accepted but the profile does not think it is running");
    }

    // With GMCP switched off there is nothing better to wait for, so the fallback takes the offer.
    void test_aPlayerWhoTurnedGmcpOffStillGetsTheMsdpFallback()
    {
        mpServer->setOffers({{kWill, kMsdp}, {kWill, kGmcp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        host->mEnableGMCP = false;
        mpServer->clearReplies();
        QVERIFY(reconnect(host));

        QVERIFY2(waitForReply(kDo, kMsdp), "MSDP was refused in favour of a GMCP the player had switched off");
        QVERIFY2(!host->mTelnet.isGMCPEnabled(), "GMCP negotiated anyway, so this proves nothing");
    }

    void test_theStarterUiNeverTouchesThePlayersMsdpPreference()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);

        QVERIFY2(!host->mEnableMSDP, "the starter UI switched the profile's own MSDP preference on");
        QVERIFY2(luaTrue(host, qsl("getConfig('enableMSDPFallback')")), "the starter UI did not ask for the MSDP fallback");

        // The fallback belongs to the session, not the profile, so it must not be written out with
        // it. Anchored on the attribute name because the package's own script text, which says
        // "enableMSDPFallback", is exported into the same file.
        const QString savedProfile = savedProfileText(host);
        QVERIFY2(savedProfile.contains(qsl("mEnableMSDP=\"no\"")), "the saved profile does not record the MSDP preference at all, so the rest of this proves nothing");
        QVERIFY2(!savedProfile.contains(qsl("mEnableMSDPFallback=\"")), "the session-only MSDP fallback was saved into the profile");
    }

    // ---- hiding and uninstalling -------------------------------------------

    void test_hidingTheStarterUiStopsAskingForMsdp()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(waitForReply(kDo, kMsdp));

        QVERIFY(runLua(host, qsl("BaseUI.hide()")));
        QVERIFY2(luaTrue(host, qsl("not getConfig('enableMSDPFallback')")), "\"baseui hide\" left the MSDP fallback asked for");

        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDont, kMsdp), "a hidden starter UI still had MSDP negotiated for it");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "a hidden starter UI still had MSDP negotiated for it");

        QVERIFY(runLua(host, qsl("BaseUI.show()")));
        QVERIFY2(luaTrue(host, qsl("getConfig('enableMSDPFallback')")), "\"baseui show\" did not ask for the MSDP fallback again");
        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDo, kMsdp), "MSDP did not come back after \"baseui show\"");
    }

    void test_uninstallingTheStarterUiLeavesNoMsdpBehind()
    {
        mpServer->setOffers({{kWill, kMsdp}});
        Host* host = startProfileWithStarterUi();
        QVERIFY(host);
        QVERIFY(waitForReply(kDo, kMsdp));

        QVERIFY(host->uninstallPackage(qsl("mudlet-base-ui"), enums::PackageModuleType::Package));
        QVERIFY2(!host->mInstalledPackages.contains(qsl("mudlet-base-ui")), "the starter UI is still installed");

        // Nothing left asking for it this session, and nothing persisted either: the preference the
        // player would see in Preferences is untouched, and the profile on disk says so.
        QVERIFY2(luaTrue(host, qsl("not getConfig('enableMSDPFallback')")), "uninstalling left the MSDP fallback asked for");
        QVERIFY2(!host->mEnableMSDP, "uninstalling left the profile's MSDP preference switched on");
        const QString savedProfile = savedProfileText(host);
        QVERIFY2(savedProfile.contains(qsl("mEnableMSDP=\"no\"")), "uninstalling left MSDP switched on in the saved profile");
        QVERIFY2(!savedProfile.contains(qsl("mEnableMSDPFallback=\"")), "the session-only MSDP fallback was saved into the profile");

        mpServer->clearReplies();
        QVERIFY(reconnect(host));
        QVERIFY2(waitForReply(kDont, kMsdp), "MSDP was still negotiated for a starter UI that is no longer installed");
        QTest::qWait(kPastTheHold);
        QVERIFY2(!mpServer->sawReply(kDo, kMsdp), "MSDP was still negotiated for a starter UI that is no longer installed");
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

    // Answers rather than asserting: a QVERIFY here would only return from this helper, leaving the
    // test slot to run on and bury the real failure under timeouts.
    [[nodiscard]] bool reconnect(Host* host)
    {
        const int before = mpServer->connectionCount();
        host->mTelnet.disconnectIt();
        host->mTelnet.reconnect();
        const bool reconnected = QTest::qWaitFor(
                [&]() {
                    return mpServer->connectionCount() > before;
                },
                8000);
        if (!reconnected) {
            qWarning("the profile did not reconnect to the stub");
        }
        return reconnected;
    }

    // Saves the profile and hands back the XML written to disk, empty if that did not work out.
    // Reading the file rather than the Host is what proves a setting does or does not outlive the
    // session.
    QString savedProfileText(Host* host)
    {
        const auto [saved, path, error] = host->saveProfile(QString(), qsl("MsdpFallbackTest"));
        if (!saved) {
            qWarning("%s", qPrintable(qsl("could not save the profile: %1").arg(error)));
            return QString();
        }
        // The XML is written off the main thread, so the file is only complete once no save is
        // outstanding - and a package change may have queued one of its own behind ours.
        if (!QTest::qWaitFor(
                    [host]() {
                        return !host->currentlySavingProfile();
                    },
                    15000)) {
            qWarning("the profile save did not finish, so the file on disk proves nothing");
            return QString();
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning("%s", qPrintable(qsl("could not read the saved profile at %1").arg(path)));
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }

    Host* startProfileWithStarterUi()
    {
        startProfile();
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        host->mEchoLuaErrors = true;
        // Installed by hand only when the preinstall gate did not, so this test says nothing
        // about who counts as a new user.
        if (!host->mInstalledPackages.contains(qsl("mudlet-base-ui"))) {
            auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
            if (!installed) {
                qWarning("%s", qPrintable(qsl("could not install the starter UI: %1").arg(message)));
                return nullptr;
            }
        }
        if (!luaTrue(host, qsl("type(BaseUI) == 'table' and not BaseUI.dormant()"))) {
            qWarning("the starter UI did not load, or loaded dormant");
            return nullptr;
        }
        // The package may only have been installed after that first connection negotiated, so
        // every assertion is about a connection made once it is definitely loaded.
        mpServer->clearReplies();
        return reconnect(host) ? host : nullptr;
    }

    // Mirrors the helper the other functional tests use.
    void startProfile()
    {
        const QString port = QString::number(mPort);
        QTimer::singleShot(0, qApp, [this, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy loaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!loaded.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QFAIL("Could not connect to the stub.");
        }
    }

    bool runLua(Host* host, const QString& script) { return host->getLuaInterpreter()->compileAndExecuteScript(script); }

    bool luaTrue(Host* host, const QString& expression)
    {
        if (!runLua(host, qsl("__msdpProbe = not not (%1)").arg(expression))) {
            qWarning("%s", qPrintable(qsl("probe did not compile: %1").arg(expression)));
            return false;
        }
        const bool result = runLua(host, qsl("assert(__msdpProbe)"));
        if (!result) {
            qWarning("%s", qPrintable(qsl("probe is false: %1").arg(expression)));
        }
        return result;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

static void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "MsdpFallbackTest.moc"
QTEST_MAIN(MsdpFallbackTest)
