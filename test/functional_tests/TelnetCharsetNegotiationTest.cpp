/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org     *
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

// End-to-end coverage of RFC 2066 CHARSET negotiation against a server stub built
// from KaVir's protocol handler (protocol.c), which is what the games affected by
// #9434 run: it announces its options in the order Mudlet's KaVir detection looks
// for, cycles TTYPE until a response repeats, and decides whether it may send
// UTF-8 from the MTTS bitvector or a CHARSET acceptance. What Mudlet picks out of
// a CHARSET offer therefore decides what the game sends back, so where that
// matters the tests assert on both ends: the charset the client accepted, and the
// MTTS bitvector it advertises afterwards.
//
// Every test here goes through the extra connection Mudlet makes once per profile
// when it recognises that handshake, as the stub always announces in KaVir's order.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <memory>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// What one client connection told the server. A bitvector of -1 means the client
// never sent an "MTTS <n>" terminal type at all, which has to be distinguished
// from one that does not advertise UTF-8.
struct ConnectionRecord
{
    QStringList terminalTypes;
    int mttsBitvector = -1;
    QByteArray acceptedCharacterSet;
    bool charsetRejected = false;
};

// A minimal KaVir-protocol game server. On the client's WILL TTYPE it writes the
// option announcements in one batch, in KaVir's order, optionally with a trailer
// appended to that same packet. It requests a charset once the client has agreed
// to CHARSET, and cycles TTYPE afterwards so the recorded MTTS reflects the
// encoding the client settled on.
class KaVirServerStub : public QObject
{
    Q_OBJECT

public:
    explicit KaVirServerStub(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &KaVirServerStub::onNewConnection);
    }

    // Port 0: an ephemeral port, so concurrent test runs cannot collide.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    // The charset list as it goes on the wire, separator included, e.g. ";UTF-8".
    void setCharacterSetOffer(const QByteArray& offer) { mCharacterSetOffer = offer; }
    // RFC 2066 does not oblige a game to ask at all.
    void suppressCharacterSetRequest() { mCharacterSetRequestSuppressed = true; }
    // Bytes appended to the option-announcement packet of the given connection,
    // counting from 1.
    void setTrailerForConnection(int connection, const QByteArray& trailer) { mTrailerOverrides.insert(connection, trailer); }

    int connectionCount() const { return static_cast<int>(mConnections.size()); }
    // Connections are numbered from 1, in the order the client made them.
    ConnectionRecord record(int connection) const { return (connection >= 1 && connection <= static_cast<int>(mConnections.size())) ? mConnections.at(connection - 1)->record : ConnectionRecord{}; }
    bool latestTtypeCycleFinished() const { return !mConnections.empty() && mConnections.back()->ttypeCycleFinished; }

private:
    // Per-connection state, so a read that arrives once the next connection is
    // already open cannot be filed against it.
    struct Connection
    {
        QPointer<QTcpSocket> socket;
        QByteArray buffer;
        bool announced = false;
        bool characterSetRequested = false;
        bool ttypeCycleFinished = false;
        int number = 0;
        ConnectionRecord record;
    };

private slots:
    void onNewConnection()
    {
        QTcpSocket* socket = mServer.nextPendingConnection();
        if (!socket) {
            return;
        }
        auto connection = std::make_unique<Connection>();
        connection->socket = socket;
        connection->number = static_cast<int>(mConnections.size()) + 1;
        Connection* state = connection.get();
        mConnections.push_back(std::move(connection));

        connect(socket, &QTcpSocket::readyRead, this, [this, state]() {
            if (!state->socket) {
                return;
            }
            state->buffer.append(state->socket->readAll());
            parseBuffer(*state);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        write(*state, QByteArray{} + static_cast<char>(TN_IAC) + static_cast<char>(TN_DO) + static_cast<char>(OPT_TERMINAL_TYPE));
    }

private:
    void write(const Connection& connection, const QByteArray& data)
    {
        if (!connection.socket) {
            return;
        }
        connection.socket->write(data);
        connection.socket->flush();
    }

    QByteArray subnegotiation(char option, const QByteArray& payload) const
    {
        return QByteArray{} + static_cast<char>(TN_IAC) + static_cast<char>(TN_SB) + option + payload + static_cast<char>(TN_IAC) + static_cast<char>(TN_SE);
    }

    void announceOptions(Connection& connection)
    {
        // KaVir's Negotiate(): everything in one write. Together with the DO
        // TERMINAL_TYPE sent on connect, this is the announcement order
        // cTelnet::trackKaVirNegotiation() matches on.
        QByteArray out;
        for (const auto& announcement : {std::pair<char, char>{TN_DO, OPT_NAWS},
                                         {TN_WILL, static_cast<char>(OPT_CHARSET)},
                                         {TN_WILL, static_cast<char>(OPT_MSDP)},
                                         {TN_WILL, static_cast<char>(OPT_MSSP)},
                                         {TN_WILL, static_cast<char>(OPT_ATCP)},
                                         {TN_WILL, static_cast<char>(OPT_MSP)},
                                         {TN_WILL, static_cast<char>(OPT_MXP)}}) {
            out.append(static_cast<char>(TN_IAC)).append(announcement.first).append(announcement.second);
        }
        out.append(mTrailerOverrides.value(connection.number));
        write(connection, out);
    }

    void requestCharacterSet(Connection& connection)
    {
        if (mCharacterSetRequestSuppressed) {
            requestTerminalType(connection);
            return;
        }
        write(connection, subnegotiation(static_cast<char>(OPT_CHARSET), QByteArray{}.append(static_cast<char>(CHARSET_REQUEST)).append(mCharacterSetOffer)));
    }

    void requestTerminalType(Connection& connection) { write(connection, subnegotiation(static_cast<char>(OPT_TERMINAL_TYPE), QByteArray{}.append(static_cast<char>(TNSB_SEND)))); }

    void handleOption(Connection& connection, unsigned char command, unsigned char option)
    {
        if (command == static_cast<unsigned char>(TN_WILL) && option == OPT_TERMINAL_TYPE && !connection.announced) {
            connection.announced = true;
            announceOptions(connection);
        } else if (command == static_cast<unsigned char>(TN_DO) && option == OPT_CHARSET && !connection.characterSetRequested) {
            connection.characterSetRequested = true;
            requestCharacterSet(connection);
        }
    }

    void handleSubnegotiation(Connection& connection, unsigned char option, const QByteArray& payload)
    {
        ConnectionRecord& record = connection.record;

        if (option == OPT_TERMINAL_TYPE && payload.startsWith(static_cast<char>(TNSB_IS))) {
            const QString terminalType = QString::fromLatin1(payload.mid(1));
            const bool repeated = !record.terminalTypes.isEmpty() && record.terminalTypes.last() == terminalType;
            record.terminalTypes.append(terminalType);
            if (terminalType.startsWith(qsl("MTTS "))) {
                record.mttsBitvector = QStringView{terminalType}.mid(5).toInt();
            }
            // KaVir stops cycling once a response repeats the previous one.
            if (repeated) {
                connection.ttypeCycleFinished = true;
            } else {
                requestTerminalType(connection);
            }
            return;
        }

        if (option == OPT_CHARSET) {
            if (payload.startsWith(static_cast<char>(CHARSET_ACCEPTED))) {
                record.acceptedCharacterSet = payload.mid(1);
            } else if (payload.startsWith(static_cast<char>(CHARSET_REJECTED))) {
                record.charsetRejected = true;
            }
            // Only now is the client's encoding settled, so cycle TTYPE and get an
            // MTTS bitvector that reflects it.
            requestTerminalType(connection);
        }
    }

    // Index of the IAC that begins an IAC SE, skipping escaped IAC IAC pairs.
    int findSubnegotiationEnd(const QByteArray& buffer, int from) const
    {
        int j = from;
        while (j + 1 < buffer.size()) {
            if (static_cast<unsigned char>(buffer.at(j)) == static_cast<unsigned char>(TN_IAC)) {
                const unsigned char next = static_cast<unsigned char>(buffer.at(j + 1));
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

    void parseBuffer(Connection& connection)
    {
        QByteArray& buffer = connection.buffer;
        int i = 0;
        while (i < buffer.size()) {
            if (static_cast<unsigned char>(buffer.at(i)) != static_cast<unsigned char>(TN_IAC)) {
                ++i;
                continue;
            }
            if (i + 1 >= buffer.size()) {
                break;
            }
            const unsigned char command = static_cast<unsigned char>(buffer.at(i + 1));
            if (command == static_cast<unsigned char>(TN_IAC)) {
                i += 2;
                continue;
            }
            if (command == static_cast<unsigned char>(TN_WILL) || command == static_cast<unsigned char>(TN_WONT) || command == static_cast<unsigned char>(TN_DO)
                || command == static_cast<unsigned char>(TN_DONT)) {
                if (i + 2 >= buffer.size()) {
                    break;
                }
                handleOption(connection, command, static_cast<unsigned char>(buffer.at(i + 2)));
                i += 3;
                continue;
            }
            if (command == static_cast<unsigned char>(TN_SB)) {
                const int end = findSubnegotiationEnd(buffer, i + 2);
                if (end == -1) {
                    break;
                }
                QByteArray payload = buffer.mid(i + 3, end - (i + 3));
                payload.replace(QByteArray(2, TN_IAC), QByteArray(1, TN_IAC));
                handleSubnegotiation(connection, static_cast<unsigned char>(buffer.at(i + 2)), payload);
                i = end + 2;
                continue;
            }
            i += 2;
        }
        buffer = buffer.mid(i);
    }

    QTcpServer mServer;
    QByteArray mCharacterSetOffer = ";UTF-8";
    bool mCharacterSetRequestSuppressed = false;
    QHash<int, QByteArray> mTrailerOverrides;
    std::vector<std::unique_ptr<Connection>> mConnections;
};

class TelnetCharsetNegotiationTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    KaVirServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-Telnet-Charset");

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
        mpServer = new KaVirServerStub(qApp);
        QVERIFY2(mpServer->start(), "KaVirServerStub failed to bind a loopback port");
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    // A game that lists several charsets it can speak must not pull a UTF-8
    // profile down to whichever one it happened to list first: the reply picks
    // UTF-8 wherever it sits in the offer, so the game keeps sending UTF-8
    // (issue #9434).
    void test_offeredEncodingInUseIsKept_data()
    {
        QTest::addColumn<QByteArray>("offer");
        QTest::addColumn<QByteArray>("expectedAcceptance");

        QTest::newRow("listed after another") << QByteArray(";ISO-8859-1;UTF-8") << QByteArray("UTF-8");
        QTest::newRow("listed first") << QByteArray(";UTF-8;ISO-8859-1") << QByteArray("UTF-8");
        QTest::newRow("listed after ASCII") << QByteArray(";US-ASCII;UTF-8") << QByteArray("UTF-8");
        // RFC 2066 acceptances name a charset from the request, so the game's own
        // spelling of it goes back out:
        QTest::newRow("spelled in lower case") << QByteArray(";iso-8859-1;utf-8") << QByteArray("utf-8");
    }

    void test_offeredEncodingInUseIsKept()
    {
        QFETCH(QByteArray, offer);
        QFETCH(QByteArray, expectedAcceptance);
        mpServer->setCharacterSetOffer(offer);

        Host* host = connectAndNegotiate();
        QVERIFY(host);

        const ConnectionRecord record = mpServer->record(mpServer->connectionCount());
        QCOMPARE(record.acceptedCharacterSet, expectedAcceptance);
        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("UTF-8"));
        QVERIFY2(host->readProfileData(qsl("encoding")).isEmpty(), "the profile's encoding was rewritten even though it did not change");
        verifyAdvertisedUtf8(record, true);
    }

    // The other side of that: a game offering only one charset Mudlet can use
    // still gets it accepted, the encoding really does change and is saved, and
    // the game is told UTF-8 is off.
    void test_singleOfferedCharsetIsStillAccepted()
    {
        mpServer->setCharacterSetOffer(";ISO-8859-1");

        Host* host = connectAndNegotiate();
        QVERIFY(host);

        const ConnectionRecord record = mpServer->record(mpServer->connectionCount());
        QCOMPARE(record.acceptedCharacterSet, QByteArray("ISO-8859-1"));
        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("ISO 8859-1"));
        QCOMPARE(host->readProfileData(qsl("encoding")), qsl("ISO 8859-1"));
        verifyAdvertisedUtf8(record, false);
    }

    // Nothing Mudlet can use: the offer is rejected rather than answered with
    // something that was never on it.
    void test_unusableOfferIsRejected()
    {
        mpServer->setCharacterSetOffer(";NOT-A-CHARSET");

        Host* host = connectAndNegotiate();
        QVERIFY(host);

        const ConnectionRecord record = mpServer->record(mpServer->connectionCount());
        QVERIFY2(record.charsetRejected, "an offer of nothing usable should have been rejected");
        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("UTF-8"));
    }

    // A request with no separator and no names at all: rejected, and above all
    // survived - reading a separator out of an empty offer used to run off the
    // end of the payload.
    void test_emptyOfferIsRejected()
    {
        mpServer->setCharacterSetOffer(QByteArray());

        Host* host = connectAndNegotiate();
        QVERIFY(host);

        const ConnectionRecord record = mpServer->record(mpServer->connectionCount());
        QVERIFY2(record.charsetRejected, "an empty offer should have been rejected");
        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("UTF-8"));
    }

    // Spotting KaVir's handshake makes Mudlet reconnect once per profile. The rest
    // of the packet that triggered it belongs to the connection being dropped, so
    // a CHARSET request sitting in that tail must not set the encoding the fresh
    // connection then negotiates and advertises with.
    void test_reconnectIgnoresDroppedConnectionsCharsetRequest()
    {
        mpServer->setTrailerForConnection(1, charsetRequestTrailer());
        // The connection that survives never asks about charsets, so only the
        // dropped connection's request could change the encoding.
        mpServer->suppressCharacterSetRequest();

        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QCOMPARE(mpServer->connectionCount(), 2);

        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("UTF-8"));
        QVERIFY2(host->readProfileData(qsl("encoding")).isEmpty(), "the dropped connection's charset was saved as the profile encoding");
        const ConnectionRecord dropped = mpServer->record(1);
        QVERIFY2(dropped.acceptedCharacterSet.isEmpty() && !dropped.charsetRejected, "the connection being dropped went on negotiating charsets");
        const ConnectionRecord live = mpServer->record(2);
        verifyAdvertisedUtf8(live, true);
        // What the reconnect is for: the version number the game wants in TTYPE.
        QVERIFY2(!live.terminalTypes.isEmpty() && live.terminalTypes.first().startsWith(qsl("MUDLET ")),
                 qPrintable(qsl("the reconnected client reported terminal types %1, without a version number").arg(live.terminalTypes.join(qsl(", ")))));
    }

    // The control for the case above: the very same trailer, in the very same
    // packet, on the connection that is not being dropped - it is acted on, so
    // that test cannot pass by the trailer never arriving.
    void test_trailerOnTheLiveConnectionIsApplied()
    {
        mpServer->setTrailerForConnection(2, charsetRequestTrailer());
        mpServer->suppressCharacterSetRequest();

        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QCOMPARE(mpServer->connectionCount(), 2);

        QCOMPARE(host->mTelnet.getEncoding(), QByteArray("ISO 8859-1"));
    }

    // Data handed to feedTelnet() is not the connection's, so a KaVir handshake in
    // it neither stands in for one nor may swallow what follows it.
    void test_fedTelnetDataIsNotTruncatedByTheHandshake()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        // As it would be for a profile that has not met a KaVir game yet:
        host->mPromptedForVersionInTTYPE = false;

        QByteArray fed;
        for (const auto& announcement : {std::pair<char, char>{TN_DO, OPT_TERMINAL_TYPE},
                                         {TN_DO, OPT_NAWS},
                                         {TN_WILL, static_cast<char>(OPT_CHARSET)},
                                         {TN_WILL, static_cast<char>(OPT_MSDP)},
                                         {TN_WILL, static_cast<char>(OPT_MSSP)},
                                         {TN_WILL, static_cast<char>(OPT_ATCP)},
                                         {TN_WILL, static_cast<char>(OPT_MSP)},
                                         {TN_WILL, static_cast<char>(OPT_MXP)}}) {
            fed.append(static_cast<char>(TN_IAC)).append(announcement.first).append(announcement.second);
        }
        fed.append("FED_TAIL_MARKER\r\n");
        host->mTelnet.loopbackTest(fed);

        QVERIFY2(waitForBufferToContain(host, qsl("FED_TAIL_MARKER")), "text fed after a KaVir handshake was dropped instead of displayed");
    }

private:
    bool waitForBufferToContain(Host* host, const QString& text)
    {
        return QTest::qWaitFor(
                [host, &text]() {
                    for (int i = 0; i <= host->mpConsole->buffer.getLastLineNumber(); ++i) {
                        if (host->mpConsole->buffer.line(i).contains(text)) {
                            return true;
                        }
                    }
                    return false;
                },
                5000);
    }

    // An RFC 2066 request for a charset Mudlet can use but is not using.
    QByteArray charsetRequestTrailer() const
    {
        QByteArray trailer;
        trailer.append(static_cast<char>(TN_IAC)).append(static_cast<char>(TN_SB)).append(static_cast<char>(OPT_CHARSET));
        trailer.append(static_cast<char>(CHARSET_REQUEST)).append(";ISO-8859-1");
        trailer.append(static_cast<char>(TN_IAC)).append(static_cast<char>(TN_SE));
        return trailer;
    }

    // MTTS bit 4 is what a KaVir game reads to decide whether it may send anything
    // outside ASCII, so this is the client-visible consequence of the charset
    // Mudlet settled on.
    void verifyAdvertisedUtf8(const ConnectionRecord& record, bool expected)
    {
        QVERIFY2(record.mttsBitvector >= 0, "the client never sent an MTTS terminal type");
        const bool advertised = record.mttsBitvector & MTTS_STD_UTF_8;
        QVERIFY2(advertised == expected,
                 qPrintable(qsl("MTTS bitvector %1 advertises UTF-8: %2, expected %3").arg(QString::number(record.mttsBitvector), QVariant(advertised).toString(), QVariant(expected).toString())));
    }

    // Drives the profile dialog the way a user would, then waits for the telnet
    // negotiation to settle - including the extra connection Mudlet makes when it
    // recognises KaVir's handshake.
    Host* connectAndNegotiate()
    {
        const QString port = QString::number(mpServer->serverPort());
        Host* host = TestProfile::create(mHostname, qsl("localhost"), port);
        if (!host) {
            qWarning("No active host");
            return nullptr;
        }

        if (!QTest::qWaitFor(
                    [this]() {
                        return mpServer->connectionCount() >= 2 && mpServer->latestTtypeCycleFinished();
                    },
                    15000)) {
            qWarning("The client did not reconnect and complete its telnet negotiation");
            return nullptr;
        }
        return host;
    }

    void deleteProfileDirectory()
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TelnetCharsetNegotiationTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetCharsetNegotiationTest)
