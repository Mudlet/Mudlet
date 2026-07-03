/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

// End-to-end coverage of the client side of the GMCP Char.Login v2 sign-in flow
// (see the Char.Login standard). A minimal GMCP-speaking server stub drives a real
// Mudlet profile and asserts on what the client sends back (Char.Login.Credentials /
// Reconnect) and on the messages it prints, so future changes cannot silently break
// authentication.

#include <QtTest/QtTest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "CredentialManager.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

// A tiny GMCP-capable server: offers GMCP on connect, parses the telnet stream to
// collect the client's GMCP messages, and can push Char.Login frames on demand.
class GmcpServerStub : public QObject
{
    Q_OBJECT

public:
    explicit GmcpServerStub(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &GmcpServerStub::onNewConnection);
    }

    // Bind to an ephemeral port (0) so shared CI runners cannot collide on a fixed port; the caller
    // reads the actual port back via serverPort().
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    bool gmcpEnabled() const { return mGmcpEnabled; }
    QStringList receivedGmcp() const { return mReceivedGmcp; }
    void clearReceived() { mReceivedGmcp.clear(); }

    // Send one GMCP message as IAC SB GMCP <message> IAC SE.
    void sendGmcp(const QString& message)
    {
        if (!mClient) {
            return;
        }
        QByteArray frame;
        frame.append(TN_IAC);
        frame.append(TN_SB);
        frame.append(static_cast<char>(OPT_GMCP));
        frame.append(message.toUtf8());
        frame.append(TN_IAC);
        frame.append(TN_SE);
        mClient->write(frame);
        mClient->flush();
    }

signals:
    void gmcpReceived(const QString& message);

private slots:
    void onNewConnection()
    {
        mClient = mServer.nextPendingConnection();
        if (!mClient) {
            return;
        }
        connect(mClient, &QTcpSocket::readyRead, this, &GmcpServerStub::onReadyRead);
        connect(mClient, &QTcpSocket::disconnected, mClient, &QObject::deleteLater);
        // Offer GMCP; the client answers IAC DO GMCP and sends Core.Hello + Core.Supports.Set.
        QByteArray offer;
        offer.append(TN_IAC);
        offer.append(TN_WILL);
        offer.append(static_cast<char>(OPT_GMCP));
        mClient->write(offer);
        mClient->flush();
    }

    void onReadyRead()
    {
        mBuffer.append(mClient->readAll());
        parseBuffer();
    }

private:
    // Find the index of the IAC that begins an IAC SE, honouring escaped IAC IAC.
    int findSubnegotiationEnd(int from) const
    {
        int j = from;
        while (j + 1 < mBuffer.size()) {
            if (static_cast<unsigned char>(mBuffer.at(j)) == static_cast<unsigned char>(TN_IAC)) {
                const unsigned char next = static_cast<unsigned char>(mBuffer.at(j + 1));
                if (next == static_cast<unsigned char>(TN_IAC)) {
                    j += 2; // escaped 255 inside the subnegotiation
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
                break; // incomplete
            }
            const unsigned char cmd = static_cast<unsigned char>(mBuffer.at(i + 1));
            if (cmd == static_cast<unsigned char>(TN_IAC)) {
                i += 2; // escaped literal 255 in the main stream
                continue;
            }
            if (cmd == static_cast<unsigned char>(TN_WILL) || cmd == static_cast<unsigned char>(TN_WONT) || cmd == static_cast<unsigned char>(TN_DO) || cmd == static_cast<unsigned char>(TN_DONT)) {
                if (i + 2 >= mBuffer.size()) {
                    break; // incomplete
                }
                const unsigned char opt = static_cast<unsigned char>(mBuffer.at(i + 2));
                if (cmd == static_cast<unsigned char>(TN_DO) && opt == OPT_GMCP) {
                    mGmcpEnabled = true;
                }
                i += 3;
                continue;
            }
            if (cmd == static_cast<unsigned char>(TN_SB)) {
                const int end = findSubnegotiationEnd(i + 2);
                if (end == -1) {
                    break; // incomplete subnegotiation
                }
                const unsigned char opt = static_cast<unsigned char>(mBuffer.at(i + 2));
                QByteArray payload = mBuffer.mid(i + 3, end - (i + 3));
                payload.replace(QByteArray(2, TN_IAC), QByteArray(1, TN_IAC)); // unescape IAC IAC
                if (opt == OPT_GMCP) {
                    const QString message = QString::fromUtf8(payload);
                    mReceivedGmcp.append(message);
                    emit gmcpReceived(message);
                }
                i = end + 2;
                continue;
            }
            i += 2; // any other 2-byte IAC command
        }
        mBuffer = mBuffer.mid(i);
    }

    QTcpServer mServer;
    QPointer<QTcpSocket> mClient;
    QByteArray mBuffer;
    QStringList mReceivedGmcp;
    bool mGmcpEnabled = false;
};

class GMCPCharLoginTest : public QObject
{
    Q_OBJECT

private:
    GmcpServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-CharLogin");
    quint16 mPort = 0; // assigned the stub's actual loopback port in init()

private slots:
    void initTestCase()
    {
        // Force CredentialManager to use its deterministic encrypted-file backend rather than the
        // system keychain, so reconnect-token storage/retrieval is synchronous and observable in tests.
        qputenv("MUDLET_TEST_MODE", "1");
        initializeQRCResources();
    }

    void init()
    {
        mpServer = new GmcpServerStub(qApp);
        QVERIFY2(mpServer->start(), "GmcpServerStub failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // Start each test from a clean credential state so a reconnect token saved by an earlier test
        // cannot leak into one that expects none (which would make the client replay it instead).
        CredentialManager::removeCredential(mHostname, qsl("reconnect"));
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // ---- Char.Login.Credentials / hand-off ---------------------------------

    void testStoredCredentialsAutofillWithVersionEcho()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send Char.Login.Credentials");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("player"));
        QCOMPARE(sent.value(qsl("password")).toString(), qsl("secret"));
        QCOMPARE(sent.value(qsl("version")).toInt(), 2);
    }

    void testAbsentServerVersionEchoesVersionOne()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        // No "version" field: the client must treat the session as version 1.
        mpServer->sendGmcp(qsl("Char.Login.Default {\"type\": [\"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send Char.Login.Credentials");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("player"));
        QCOMPARE(sent.value(qsl("version")).toInt(), 1);
    }

    void testNoCredentialsHandsOffWithEmptyCredentials()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not hand off with Char.Login.Credentials");
        QVERIFY2(sent.isEmpty(), "hand-off Char.Login.Credentials should be an empty object");
    }

    void testPartialCredentialsAreNotSent()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(QString()); // password missing - a partial pair must never be sent

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not respond");
        QVERIFY2(sent.isEmpty(), "a partial credential pair must not be autofilled");
    }

    void testClientDrivenOAuthFieldsIgnoredOnCleartext()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->clearReceived();
        // location/client_id must be ignored on a cleartext connection, so the client
        // hands off interactively rather than starting the client-driven OAuth flow.
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"], "
                               "\"location\": \"https://example.com/.well-known/openid-configuration\", \"client_id\": \"abc\"}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not hand off");
        QVERIFY2(sent.isEmpty(), "client-driven OAuth fields must be ignored on cleartext, yielding an empty hand-off");
    }

    // ---- Char.Login.URL safety ---------------------------------------------

    void testAuthUrlWithUnsupportedSchemeIsRejected()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"file:///etc/passwd\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("invalid sign-in link")), "an unsupported-scheme sign-in URL should be rejected");
    }

    void testUnpromptedAuthUrlIsOfferedNotOpened()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        // On a fresh connection the user has sent no input, so the client must not
        // auto-open the browser; it offers the link to open deliberately instead.
        QVERIFY2(!host->userSentInputThisConnection(), "precondition: no user input yet");
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("To sign in, open this link")), "an unprompted URL should be offered as a link");
    }

    // ---- Char.Login.Token ---------------------------------------------------

    void testTokenIsPersistedAndAnnounced()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("signed in automatically next time")), "saving a reconnect token should be announced once");
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the reconnect token should be persisted");
    }

    // ---- Char.Login.Reconnect ----------------------------------------------

    void testSavedTokenIsReplayedOnReconnect()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        // Pre-seed a saved token as if a previous sign-in had stored one.
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"token\": \"saved-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("acct:char"));
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("saved-token"));
        QCOMPARE(sent.value(qsl("version")).toInt(), 2);
    }

    void testRejectedReconnectTokenIsDiscarded()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"token\": \"stale-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        // The server rejects the reconnect; the client must discard the token and say so.
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("saved sign-in has expired")), "a rejected reconnect should be reported");
    }

    // ---- Char.Login.Result --------------------------------------------------

    void testFailedResultReportsError()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send credentials");

        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Invalid credentials\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("Could not log in to the game")), "a failed result should be reported to the user");
    }

private:
    // Drive the GUI to create/connect a profile, then wait for GMCP to negotiate.
    Host* connectAndNegotiate()
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
            QTest::keyClicks(QApplication::focusWidget(), qsl("localhost"));
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy loaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!loaded.wait(5000)) {
            qWarning("Profile took too long to load");
            return nullptr;
        }
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            qWarning("No active host");
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            qWarning("Could not connect to the stub");
            return nullptr;
        }
        // Wait until the client has answered our GMCP offer (IAC DO GMCP) so that
        // Char.Login frames we push afterwards are processed.
        const bool negotiated = QTest::qWaitFor(
                [this]() {
                    return mpServer->gmcpEnabled();
                },
                3000);
        if (!negotiated) {
            qWarning("GMCP was not negotiated");
        }
        return host;
    }

    // Wait until the client sends a GMCP message whose package matches, returning its JSON body.
    bool waitForClientGmcp(const QString& packagePrefix, QJsonObject& out, int timeoutMs = 5000)
    {
        QString match;
        const bool ok = QTest::qWaitFor(
                [this, &packagePrefix, &match]() {
                    for (const QString& msg : mpServer->receivedGmcp()) {
                        if (msg.startsWith(packagePrefix)) {
                            match = msg;
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
        if (!ok) {
            qWarning() << "waitForClientGmcp timed out waiting for" << packagePrefix << "- received so far:" << mpServer->receivedGmcp();
            return false;
        }
        const int space = match.indexOf(QChar::Space);
        const QString body = (space == -1) ? QString() : match.mid(space + 1).trimmed();
        out = QJsonDocument::fromJson(body.toUtf8()).object();
        return true;
    }

    bool waitForConsoleContains(Host* host, const QString& substring, int timeoutMs = 4000)
    {
        if (!host || !host->mpConsole) {
            return false;
        }
        auto& buffer = host->mpConsole->buffer;
        return QTest::qWaitFor(
                [&]() {
                    QString all;
                    for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
                        all.append(buffer.line(i));
                        all.append(QChar::Space);
                    }
                    return all.contains(substring);
                },
                timeoutMs);
    }

    bool waitForStoredToken(Host* host, const QString& expectedToken, int timeoutMs = 4000)
    {
        if (!host) {
            return false;
        }
        return QTest::qWaitFor(
                [&]() {
                    const QString stored = CredentialManager::retrieveCredential(host->getName(), qsl("reconnect"));
                    return stored.contains(expectedToken);
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
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

#include "GMCPCharLoginTest.moc"
QTEST_MAIN(GMCPCharLoginTest)
