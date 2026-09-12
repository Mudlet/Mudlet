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

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <functional>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "CredentialManager.h"
#include "GMCPAuthenticator.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Self-signed loopback certificate, valid until 2126; the client accepts it via Host::mSslIgnoreAll.
static const char* csmTestCertificatePem = R"PEM(-----BEGIN CERTIFICATE-----
MIIDJzCCAg+gAwIBAgIULa4vwGAVOB+r6qtcLMPqwzBlEJgwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDgwNjE0MTM0OFoYDzIxMjYw
NzEzMTQxMzQ4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDEg1HE09f69FW/OLD0jrWEQRbKkSkIkexLfV5OtzbI
ZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5LBEKGP1TTs6C9R+e5hlI6McoKown
ha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon7M4wzVvUO5VIRhFeA2AO6LRVQhyi
9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp4SbuHuXPwgiZ9hYEIX2xAFSR48OB
bjWgqVISptu/s+UkK2XckI42qdxqzwglLIIqjFYJ1HvGqhqV69DeqB0XNw6qp8W2
qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTfG1y8DnnLAgMBAAGjbzBtMB0GA1Ud
DgQWBBSyDWWzo202vFbYncaD2crvY5V2pDAfBgNVHSMEGDAWgBSyDWWzo202vFbY
ncaD2crvY5V2pDAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z
dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAs5nw4GBPPHc9Nc08uLUYTDLkA2XM
WPugjSO7OxUe8NptVh/v4GbeKzQ4FRIF6rca8De15+OOZgIDppRUoy+fd+ncoDan
flw38rIj13XfV/3WF33Uag2xtZG0Hrpu4PFZQyIzr0MwGJJ/v2uRjMiV0CX+rc0L
BJg2JS4oCbNdQpwH81qOktoH8aHirAyLjtm732GQgAGLe0fIBBsb4Dg2ZdvN+TF5
xfKoFfri3H1rwju43zHXmUyCE/RPdIBR8flO6gzdgWAVY0jaixZi1fzQEQuReh2j
d2iZYOFSrVDea41ltrUvRC6q6gxe/REVjj1nCSYU1x44J9DQ6n6ljvJCVw==
-----END CERTIFICATE-----)PEM";

static const char* csmTestPrivateKeyPem = R"PEM(-----BEGIN PRIVATE KEY-----
MIIEwAIBADANBgkqhkiG9w0BAQEFAASCBKowggSmAgEAAoIBAQDEg1HE09f69FW/
OLD0jrWEQRbKkSkIkexLfV5OtzbIZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5
LBEKGP1TTs6C9R+e5hlI6McoKownha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon
7M4wzVvUO5VIRhFeA2AO6LRVQhyi9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp
4SbuHuXPwgiZ9hYEIX2xAFSR48OBbjWgqVISptu/s+UkK2XckI42qdxqzwglLIIq
jFYJ1HvGqhqV69DeqB0XNw6qp8W2qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTf
G1y8DnnLAgMBAAECggEBALRPHebwzfrI2CilttAeZXTdWDEzsifX5K17cd3eBBkp
xVuNShuCupZq9bUNOhl4ghlDPALmpRTFDHp78YKHXWFkLN5CVeoxjL+2Po6fQ4w7
/3zOtWNMYp/q32Kn+4ocjaLT0U+SDs0G6LR7dtGWjAyXQylWiTbu9+OWJ2kXSTlH
QbdtamymoJrrjRTV1HUEq/a3qSHlqTA5/EKIcGeiETq2NR0fZ3NFbe+PLiOSpiNg
uIiVEdsItuZTdINSEzOtMFvRd2od0ITDpMtLG404aGsI4Zisiuhr5naf4DWqK2aL
n9Z/55LSuAdBqvrtJ9XVdtNsFdCRjbIj2R1qqDTFm/kCgYEA6W/+ufDXrhPi8XWS
8+7tlOoUd0jYZL9N+N1hfho21SN3eH5TtNO0b/os/PN/M+5dKeWPtnyzwg49EksF
Es9Z4+lLt/Z+71RDmYqSCwaLhXNKtUZluZmrGHcRogd4hJDYv9icgmpwMK5Hg634
PYCgVYb9C1Wug/mZhgLg7Aw3hn8CgYEA14GxAPViaSszVNRps+a9WVEJklPbPR8U
kAxWTP6n1SdT3Z9HRcHH9inIdTLyC/3ti4+4dc1pDkMrq+MUTjvF8BqN35uzJa7l
6dnsXBmWvB1cIcwQb4SLnDb7jzmiK2uIjMrO54x3+atB83GdvESLOQ/9NAJL/+NX
ILq5kAs2nrUCgYEAqq7/8pceLKNPybttMr0drEenpTx3NNsIORItydWDCD8BiPHd
ZJdzFHk5Uc780EzWg97dQNJXYWmlz+1YjVNdZ57ahW1PjNDxCKBgfn1PoMkW9ArA
MIAisSXGl9GcllmOkl/guB75Xy7fDXIz00xsb3zfIt2IV+k2Dt2l9hJMuyMCgYEA
lv45ZHCJeSJZntANF41NkazjxfCXJaYHJD5goSWztfcOHbOhnlB9qA3yc5s0WA6c
RzJ1jaRUPTf2+0HpUj8zGl2gldFjnb2DPWwA3S7YnAj+Knft9BSsNNGZQ+qfo0h+
rhbTDQ0wanABj25FlEl6OornX29UjH9e5oGtziztIhkCgYEAppTHqOgLiKmeV15d
i850uRyh7X6whywY8gm0VLO+xzCVsCR6CvgZY1MwwFuDwu2d/d5jdJXLpHueQwNU
3HipTI77OuIRv4ykXwPOIemT9VmL/N21CgrckJGA6dYywTnc/JNpOKxdTM9srOyr
Rcsgla9jttJevaHI71x2jLNBaKk=
-----END PRIVATE KEY-----)PEM";

// Hands out QSslSocket connections when asked to, so the stub can offer an encrypted transport.
class GmcpTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit GmcpTcpServer(QObject* parent = nullptr)
    : QTcpServer(parent)
    {
    }

    void setTls(bool tls) { mTls = tls; }

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        if (!mTls) {
            QTcpServer::incomingConnection(socketDescriptor);
            return;
        }
        auto* socket = new QSslSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        socket->setLocalCertificate(QSslCertificate(QByteArray(csmTestCertificatePem)));
        socket->setPrivateKey(QSslKey(QByteArray(csmTestPrivateKeyPem), QSsl::Rsa));
        socket->startServerEncryption();
        // QSslSocket buffers writes queued before the handshake, so this behaves like a plain socket.
        addPendingConnection(socket);
    }

private:
    bool mTls = false;
};

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
    void setTls(bool tls) { mServer.setTls(tls); }

    bool gmcpEnabled() const { return mGmcpEnabled; }
    // So a test asserting on encrypted-transport behaviour cannot silently pass over a plain socket.
    bool clientEncrypted() const
    {
        auto* sslClient = qobject_cast<QSslSocket*>(mClient.data());
        return sslClient && sslClient->isEncrypted();
    }
    QStringList receivedGmcp() const { return mReceivedGmcp; }
    // Client bytes outside telnet sequences - what the timer-driven auto-login types.
    QByteArray receivedText() const { return mReceivedText; }
    void clearReceived()
    {
        mReceivedGmcp.clear();
        mReceivedText.clear();
    }

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

    int countReceived(const QString& packagePrefix) const
    {
        int n = 0;
        for (const QString& msg : mReceivedGmcp) {
            if (msg.startsWith(packagePrefix)) {
                ++n;
            }
        }
        return n;
    }

    // Number of client connections accepted so far. A reconnect (the client dropping and re-opening the
    // socket after a rejected token) increments this, so a test can wait for the follow-on connection.
    int connectionCount() const { return mConnectionCount; }

signals:
    void gmcpReceived(const QString& message);

private slots:
    void onNewConnection()
    {
        mClient = mServer.nextPendingConnection();
        if (!mClient) {
            return;
        }
        ++mConnectionCount;
        mGmcpEnabled = false; // renegotiated per connection
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
                mReceivedText.append(mBuffer.at(i));
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

    GmcpTcpServer mServer;
    QPointer<QTcpSocket> mClient;
    QByteArray mBuffer;
    QStringList mReceivedGmcp;
    QByteArray mReceivedText;
    bool mGmcpEnabled = false;
    int mConnectionCount = 0;
};

// The auto-login delays live in a QSettings file shared by every case in this binary, so they have to
// go back however a QVERIFY leaves the test body.
class ScopedAutoLoginDelays
{
public:
    ScopedAutoLoginDelays(int usernameMs, int passwordMs)
    : mpSettings(mudlet::getQSettings())
    , mSavedUsername(mpSettings->value(qsl("autoLoginUsernameDelay")))
    , mSavedPassword(mpSettings->value(qsl("autoLoginPasswordDelay")))
    {
        mpSettings->setValue(qsl("autoLoginUsernameDelay"), usernameMs);
        mpSettings->setValue(qsl("autoLoginPasswordDelay"), passwordMs);
    }

    ~ScopedAutoLoginDelays()
    {
        restore(qsl("autoLoginUsernameDelay"), mSavedUsername);
        restore(qsl("autoLoginPasswordDelay"), mSavedPassword);
    }

private:
    void restore(const QString& key, const QVariant& saved) { saved.isValid() ? mpSettings->setValue(key, saved) : mpSettings->remove(key); }

    QSettings* mpSettings;
    QVariant mSavedUsername;
    QVariant mSavedPassword;
};

// Serves a static OpenID Connect discovery document over loopback http, which
// OAuthClientFlow::acceptableEndpointUrl() permits, so no second certificate is needed.
class DiscoveryServerStub : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryServerStub(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            while (mServer.hasPendingConnections()) {
                QTcpSocket* socket = mServer.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                    mRequests[socket] += socket->readAll();
                    if (!mRequests.value(socket).contains("\r\n\r\n")) {
                        return;
                    }
                    mRequests.remove(socket);
                    const QByteArray body = QJsonDocument(QJsonObject{{qsl("authorization_endpoint"), authorizationEndpoint()}}).toJson(QJsonDocument::Compact);
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QObject::destroyed, this, [this, socket]() {
                    mRequests.remove(socket);
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    QString discoveryUrl() const { return qsl("http://127.0.0.1:%1/.well-known/openid-configuration").arg(mServer.serverPort()); }
    QString authorizationEndpoint() const { return qsl("http://127.0.0.1:%1/authorize").arg(mServer.serverPort()); }

private:
    QTcpServer mServer;
    QHash<QTcpSocket*, QByteArray> mRequests;
};

class GMCPCharLoginTest : public QObject
{
    Q_OBJECT

public slots:
    // Registered as the http/https URL handler so a sign-in address the client auto-opens routes here
    // instead of launching a real browser during the test.
    void captureOpenedUrl(const QUrl& url) { mOpenedUrls.append(url); }

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    GmcpServerStub* mpServer = nullptr;
    DiscoveryServerStub* mpDiscovery = nullptr;
    const QString mHostname = qsl("Test-CharLogin");
    quint16 mPort = 0; // assigned the stub's actual loopback port in init()
    QList<QUrl> mOpenedUrls;

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

        // Intercept browser opens so an auto-opened Char.Login.URL does not launch a real browser.
        QDesktopServices::setUrlHandler(qsl("http"), this, "captureOpenedUrl");
        QDesktopServices::setUrlHandler(qsl("https"), this, "captureOpenedUrl");
        // Force CredentialManager to use its deterministic encrypted-file backend rather than the
        // system keychain, so reconnect-token storage/retrieval is synchronous and observable in tests.
        qputenv("MUDLET_TEST_MODE", "1");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new GmcpServerStub(qApp);
        QVERIFY2(mpServer->start(), "GmcpServerStub failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mOpenedUrls.clear();
        // Start each test from a clean credential state so a reconnect token saved by an earlier test
        // cannot leak into one that expects none (which would make the client replay it instead).
        // Clearing the blocking directory here as well as in cleanup() is what makes a hard death - a CI
        // timeout, a sanitizer abort - survivable: the credential store lives under AppConfigLocation,
        // which follows XDG_CONFIG_HOME only on Linux, so on macOS and Windows it outlives this run's
        // temporary config directory, and removeCredential below cannot delete a directory.
        removeBlockingCredentialDirectory();
        CredentialManager::removeCredential(mHostname, qsl("reconnect"));
        CredentialManager::removeCredential(mHostname, qsl("reconnect-token"));
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        // A test that blocked the credential store with a directory must not leave it standing, even
        // when it failed part way through: every later test's save would be blocked too.
        removeBlockingCredentialDirectory();
        delete mpServer;
        mpServer = nullptr;
        delete mpDiscovery;
        mpDiscovery = nullptr;
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
        // A real JSON boolean, not the string "true": Qt can serialise one, so the leniency the
        // standard allows for driver-limited peers is not ours to spend.
        QCOMPARE(sent.value(qsl("token_storage")), QJsonValue(true));
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
        QCOMPARE(sent.value(qsl("token_storage")), QJsonValue(true));
    }

    void testNoCredentialsHandsOffToTheGamesSignInScreen()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not hand off with Char.Login.Credentials");
        QVERIFY2(isVersionTwoHandoff(sent), qPrintable(qsl("expected the version 2 hand-off, got %1").arg(describe(sent))));
    }

    void testVersionOneHandoffStaysABareEmptyObject()
    {
        // Keeping the bare {} on a version 1 exchange is a compatibility choice, not a protocol rule:
        // the empty hand-off is itself a version 2 addition, so no version 1 server was specified to
        // expect one. Mudlet has sent {} to such servers since before the standard, and some may have
        // been written against that, so this pins the behaviour rather than the standard.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not hand off with Char.Login.Credentials");
        QVERIFY2(sent.isEmpty(), qPrintable(qsl("a version 1 hand-off must stay the bare {} object, got %1").arg(describe(sent))));
    }

    void testDefaultWithoutAuthTypesLeavesTimerAutoLoginAlone_data()
    {
        QTest::addColumn<QString>("frame");
        // Federation 2 sends the first of these.
        QTest::newRow("no type key") << qsl("Char.Login.Default {}");
        QTest::newRow("empty type array") << qsl("Char.Login.Default {\"type\": []}");
        QTest::newRow("every type entry malformed") << qsl("Char.Login.Default {\"type\": [\"\", 5, null]}");
        QTest::newRow("unparseable payload") << qsl("Char.Login.Default {");
    }

    void testDefaultWithoutAuthTypesLeavesTimerAutoLoginAlone()
    {
        // A Char.Login.Default naming no usable method cannot be driven over GMCP, so the timer-driven
        // auto-login has to survive the frame and type the credentials itself.
        QFETCH(QString, frame);
        // mTimerLogin is already running by now - it starts when the socket connects - so this budget
        // has to cover the GMCP round trip below as well.
        ScopedAutoLoginDelays delays(1500, 200);

        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        // Without this the assertions below would also pass on a runner slow enough that the timer beat
        // the frame, which would prove nothing about how the frame was handled.
        QVERIFY2(!mpServer->receivedText().contains("player\r\n"), "the auto-login fired before the test frame was sent");

        mpServer->clearReceived();
        mpServer->sendGmcp(frame);

        // Wait for whichever comes first: the hand-off a build that drives this frame sends (which
        // arrives in milliseconds), or the name the timer types. That way a regression fails on the
        // line below rather than after the full timeout.
        const bool typedName = QTest::qWaitFor(
                [this]() {
                    return mpServer->countReceived(qsl("Char.Login.Credentials")) > 0 || mpServer->receivedText().contains("player\r\n");
                },
                8000);
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Credentials")), 0);
        QVERIFY2(typedName, "the timer auto-login did not send the character name");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpServer->receivedText().contains("secret\r\n");
                         },
                         4000),
                 "the timer auto-login did not send the password");
        const QByteArray typed = mpServer->receivedText();
        QVERIFY2(typed.indexOf("player\r\n") < typed.indexOf("secret\r\n"), "the password was typed before the character name");
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
        QVERIFY2(isVersionTwoHandoff(sent), qPrintable(qsl("a partial credential pair must not be autofilled, got %1").arg(describe(sent))));
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
        QVERIFY2(isVersionTwoHandoff(sent), qPrintable(qsl("client-driven OAuth fields must be ignored on cleartext, yielding a hand-off, got %1").arg(describe(sent))));
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
        // Asserted separately rather than as one sentence, which the console wraps across lines. The page
        // name is pinned because it can silently drift away from where the "Forget saved sign-in" control
        // actually lives - which is exactly how the notice came to name the wrong page before.
        QVERIFY2(waitForConsoleContainsUnwrapped(host, qsl("Manage this under Preferences, Privacy and security.")), "the notice should name the preferences page that manages the saved sign-in");
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("account")).toString() == qsl("acct:char");
                                        }),
                 "the reconnect metadata should be persisted with the announced account");
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should be persisted under its own key");

        // A server may mint repeatedly on one sign-in; the player only needs telling once.
        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"second-token\"}"));
        QVERIFY2(waitForStoredToken(host, qsl("second-token")), "the second token should overwrite the first");
        QCOMPARE(consoleOccurrences(host, qsl("signed in automatically next time")), 1);
    }

    void testASavedTokenGoesToItsOwnKey()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return CredentialManager::retrieveCredential(host->getName(), qsl("reconnect-token")) == qsl("opaque-token");
                         },
                         4000),
                 "the token should be stored under its own key, verbatim");
        const QJsonObject metadata = readStoredReconnect(host);
        QCOMPARE(metadata.value(qsl("account")).toString(), qsl("acct:char"));
        QVERIFY2(!metadata.contains(qsl("token")), "the metadata must not carry the token any more");
        QCOMPARE(metadata.value(qsl("secure_only")), QJsonValue(false));
    }

    void testATornSaveLeavesAResumeHintAndNoPromise()
    {
        // Block only the token key's file. The metadata write still lands, so what survives is a resume
        // hint - a state the read path already handles - and the player is told the save failed rather
        // than promised an automatic sign-in that could never happen.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin\", \"provider\": \"discord\"}"));
        QVERIFY(waitForConsoleContains(host, qsl("To sign in, open this link")));

        const QString tokenPath = reconnectCredentialPath(host->getName(), qsl("reconnect-token"));
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect-token"), qsl("seed")));
        QVERIFY2(QFileInfo::exists(tokenPath), qPrintable(qsl("the credential store no longer files entries at %1").arg(tokenPath)));
        QVERIFY(CredentialManager::removeCredential(host->getName(), qsl("reconnect-token")));
        QVERIFY(QDir().mkpath(tokenPath));

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));

        QVERIFY2(waitForConsoleContains(host, qsl("Could not save your sign-in")), "a torn save should be reported to the player");
        QVERIFY2(!waitForConsoleContains(host, qsl("signed in automatically next time"), 500), "a torn save must not promise an automatic sign-in");
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("account")).toString() == qsl("acct:char") && entry.value(qsl("provider")).toString() == qsl("discord")
                                                   && !entry.contains(qsl("token"));
                                        }),
                 "the metadata write should have landed, leaving a resume hint");
    }

    void testATokenMintedOverTlsIsAnnounced()
    {
        // The ordinary encrypted sign-in: secure_only defaults to true from the transport, and this
        // connection satisfies it, so the promise holds and is worth making. This is the case that
        // pins the transport half of worthAnnouncing - drop it and reduce the test to the cleartext
        // ones and the notice could stop appearing on TLS entirely without anything noticing.
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        QVERIFY2(host->mTelnet.currentlySecure(), "precondition: this connection is encrypted");

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));

        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("secure_only")) == QJsonValue(true);
                                        }),
                 "a token minted over TLS should be stored as encrypted-only");
        QVERIFY2(waitForConsoleContains(host, qsl("signed in automatically next time")), "a token this connection is able to replay should be announced");
    }

    void testATokenThisTransportCannotReplayIsNotAnnounced()
    {
        // Minted in the clear but scoped by the server to an encrypted transport. Every later connect
        // on this transport refuses it (see sendReconnect), so promising an automatic sign-in would be
        // knowably false at the moment it was written. The token is still stored: the requirement may
        // be met by some future connection.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\", \"secure_only\": true}"));

        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should still be stored under its own key");
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("secure_only")) == QJsonValue(true);
                                        }),
                 "the metadata should carry the requirement the server set");
        QVERIFY2(!waitForConsoleContains(host, qsl("signed in automatically next time"), 500), "a token this transport cannot replay must not be announced as one that will be");
    }

    void testAFailedSaveIsNotAnnouncedAsASuccess()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);

        // Block the save by putting a directory exactly where the credential file has to be written -
        // surgical, removed again by cleanup(), and behaves the same on every platform, unlike revoking
        // write permission, which is a no-op for root in a container and for an Administrator on Windows.
        // Seeding a real credential first proves the computed path is the one
        // actually in use, so a change to the storage scheme fails this test rather than quietly
        // blocking nothing and letting it pass for the wrong reason.
        const QString credentialPath = reconnectCredentialPath(host->getName(), qsl("reconnect"));
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("seed")));
        QVERIFY2(QFileInfo::exists(credentialPath), qPrintable(qsl("the credential store no longer files entries at %1").arg(credentialPath)));
        QVERIFY(CredentialManager::removeCredential(host->getName(), qsl("reconnect")));
        QVERIFY(QDir().mkpath(credentialPath));

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));

        QVERIFY2(waitForConsoleContains(host, qsl("Could not save your sign-in")), "a save that failed should be reported to the player");
        // Waits for a message that must never arrive, rather than sampling once: the file-backed store
        // resolving inline is an implementation detail of MUDLET_TEST_MODE, and this assertion should not
        // quietly become a race if that ever changes.
        QVERIFY2(!waitForConsoleContains(host, qsl("signed in automatically next time"), 500), "a failed save must not also be announced as a success");
    }

    void testTokenMintedInTheClearIsReplayedInTheClear()
    {
        // Issue #10585 end to end: remember-me was unusable on a plain telnet game, because a token
        // the server had explicitly marked replayable on either transport was stored and then refused
        // on every later connection. secure_only arrives as a JSON string here because the server that
        // found this (LDMud) has no JSON boolean to send.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->sendGmcp(qsl("Char.Login.Token {\"secure_only\": \"false\", \"token\": \"opaque-token\", \"account\": \"acct:char\"}"));
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("secure_only")) == QJsonValue(false);
                                        }),
                 "the token's transport requirement should be stored in the metadata");
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should be stored under its own key");

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "a token the server marked replayable in the clear should be replayed");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("opaque-token"));
    }

    void testAbsentSecureOnlyInheritsTheIssuingTransport_data()
    {
        QTest::addColumn<bool>("encrypted");
        QTest::newRow("minted in the clear") << false;
        QTest::newRow("minted over TLS") << true;
    }

    void testAbsentSecureOnlyInheritsTheIssuingTransport()
    {
        QFETCH(bool, encrypted);
        Host* host = connectAndNegotiate(encrypted);
        QVERIFY(host);
        QCOMPARE(host->mTelnet.currentlySecure(), encrypted);

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));
        QVERIFY2(waitForStoredReconnect(host,
                                        [encrypted](const QJsonObject& entry) {
                                            return entry.value(qsl("secure_only")) == QJsonValue(encrypted);
                                        }),
                 "an absent secure_only should be defaulted from the transport the token arrived on");
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should be stored under its own key");
    }

    void testSecureOnlyIsDecodedInEveryFormTheStandardAllows_data()
    {
        QTest::addColumn<QString>("literal");
        QTest::addColumn<bool>("encrypted");
        QTest::addColumn<bool>("secureOnly");
        QTest::addColumn<bool>("decodable");
        // Every row is minted on the transport whose inherited default is the OPPOSITE of what it
        // expects, so no row can pass unless the value was really decoded. Run them all over TLS and the
        // true-expecting rows would pass against a decoder that always returned "undecodable".
        //
        // Decodable false, minted over TLS, where an undecoded value would have inherited true:
        QTest::newRow("JSON false") << qsl("false") << true << false << true;
        QTest::newRow("string false") << qsl("\"false\"") << true << false << true;
        QTest::newRow("string FALSE") << qsl("\"FALSE\"") << true << false << true;
        QTest::newRow("string zero") << qsl("\"0\"") << true << false << true;
        QTest::newRow("number zero") << qsl("0") << true << false << true;
        // Decodable true, minted in the clear, where an undecoded value would have inherited false:
        QTest::newRow("JSON true") << qsl("true") << false << true << true;
        QTest::newRow("string true") << qsl("\"true\"") << false << true << true;
        QTest::newRow("padded string True") << qsl("\" True \"") << false << true << true;
        QTest::newRow("number one") << qsl("1") << false << true << true;
        // Undecodable is absent, never a guess: each of these must land on the transport's own default,
        // which is only demonstrated by pinning it in both directions.
        QTest::newRow("unrecognised string") << qsl("\"maybe\"") << true << true << false;
        QTest::newRow("null") << qsl("null") << true << true << false;
        QTest::newRow("array") << qsl("[false]") << true << true << false;
        QTest::newRow("out-of-range number") << qsl("2") << false << false << false;
        QTest::newRow("fractional number") << qsl("1.5") << false << false << false;
        QTest::newRow("object") << qsl("{}") << false << false << false;
    }

    void testSecureOnlyIsDecodedInEveryFormTheStandardAllows()
    {
        QFETCH(QString, literal);
        QFETCH(bool, encrypted);
        QFETCH(bool, secureOnly);
        QFETCH(bool, decodable);
        Host* host = connectAndNegotiate(encrypted);
        QVERIFY(host);
        QCOMPARE(host->mTelnet.currentlySecure(), encrypted);
        if (!decodable) {
            // ignoreMessage fails the test if the message never arrives, so this asserts the diagnostic
            // rather than merely silencing it: a value no conformant client can read is the one thing the
            // server operator needs told, and nothing else would tell them.
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("'secure_only' value of type .* is not a boolean")));
        }

        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\", \"secure_only\": %1}").arg(literal));
        QVERIFY2(waitForStoredReconnect(host,
                                        [secureOnly](const QJsonObject& entry) {
                                            return entry.value(qsl("secure_only")) == QJsonValue(secureOnly);
                                        }),
                 qPrintable(qsl("secure_only %1 should have been stored as %2").arg(literal, secureOnly ? qsl("true") : qsl("false"))));
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should be stored under its own key");
    }

    // ---- Char.Login.Reconnect ----------------------------------------------

    void testSavedTokenIsReplayedOnReconnect()
    {
        Host* host = connectAndNegotiate(true);
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
        QCOMPARE(sent.value(qsl("token_storage")), QJsonValue(true));

        // A token arriving on a connection that signed in by replaying one is a silent rotation, not a
        // fresh opt-in, so it is saved without telling the player they will be remembered next time -
        // they already were.
        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"rotated-token\"}"));
        QVERIFY2(waitForStoredToken(host, qsl("rotated-token")), "the rotated token should still be persisted");
        QVERIFY2(!waitForConsoleContains(host, qsl("signed in automatically next time"), 500), "a silent rotation must not be announced as a new opt-in");
    }

    void testTokenStoredUnderItsOwnKeyIsReplayed()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"secure_only\": true}"), qsl("split-token")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "a token stored under its own key should be replayed");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("acct:char"));
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("split-token"));
    }

    void testInlineTokenWinsOverTheTokenKey()
    {
        // Only a Mudlet from before the split writes an inline token, and every split-format save
        // rewrites the metadata without one - so an inline token found beside a token key means that
        // instance rotated more recently, and its token is the live one.
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"token\": \"inline-token\", \"secure_only\": true}"), qsl("stale-key-token")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay a token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("inline-token"));
    }

    void testMetadataWithoutATokenSendsTheResumeForm()
    {
        // A metadata entry with no token key at all is a resume hint, exactly as a token-less inline
        // entry has always been.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\"}")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send the resume form");
        QCOMPARE(sent.value(qsl("provider")).toString(), qsl("discord"));
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    void testReconnectAcceptedAsTheIntegerOneKeepsTheToken_data()
    {
        QTest::addColumn<QString>("successLiteral");
        // Issue #10622: the standard declares success boolean and permits all three encodings. Reading
        // only a JSON true or the string "true" made an integer 1 a failure - and a failed reconnect
        // runs the recovery, which tells the player their sign-in expired and deletes a token the
        // server had just accepted. The drivers that send 1 are the ones with no JSON boolean, which is
        // the same limitation that motivated secure_only's string form.
        QTest::newRow("integer one") << qsl("1");
        QTest::newRow("string one") << qsl("\"1\"");
        QTest::newRow("string TRUE") << qsl("\"TRUE\"");
        QTest::newRow("JSON true") << qsl("true");
    }

    void testReconnectAcceptedAsTheIntegerOneKeepsTheToken()
    {
        QFETCH(QString, successLiteral);
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"secure_only\": true}"), qsl("saved-token")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": %1}").arg(successLiteral));

        QVERIFY2(!waitForConsoleContains(host, qsl("saved sign-in has expired"), 1000), "an accepted reconnect must not be reported as expired");
        QVERIFY2(CredentialManager::retrieveCredential(host->getName(), qsl("reconnect-token")) == qsl("saved-token"), "an accepted reconnect must not destroy the token the server just accepted");
    }

    void testSavedTokenIsNotReplayedOverCleartext()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"token\": \"saved-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "the sign-in should fall back to the interactive hand-off");
        QVERIFY2(isVersionTwoHandoff(sent), qPrintable(qsl("the fall-back must be the hand-off, got %1").arg(describe(sent))));
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
        QVERIFY2(waitForConsoleContains(host, qsl("not encrypted")), "the user should be told why their saved sign-in was not used");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "refusing to send the token must not destroy it");

        // Nothing awaits a reconnect result, so an ordinary failed sign-in must not be mistaken for a
        // rejected token - that would rewrite or delete the stored entry the player still needs.
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Invalid credentials\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("Could not log in to the game")), "a failed interactive sign-in should be reported as one");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "the stored sign-in must survive an unrelated login failure");
    }

    void testStoredTransportRequirementGatesReplay_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::addColumn<bool>("replayed");
        QTest::newRow("replayable in the clear") << qsl("{\"account\": \"acct:char\", \"token\": \"saved-token\", \"secure_only\": false}") << true;
        QTest::newRow("encrypted only") << qsl("{\"account\": \"acct:char\", \"token\": \"saved-token\", \"secure_only\": true}") << false;
        // Written by a Mudlet from before the requirement was stored: keep the strict reading, so
        // upgrading never widens the exposure of a token already on disk. It gains the field on the
        // next rotation.
        QTest::newRow("no requirement stored") << qsl("{\"account\": \"acct:char\", \"token\": \"saved-token\"}") << false;
    }

    void testStoredTransportRequirementGatesReplay()
    {
        QFETCH(QString, entry);
        QFETCH(bool, replayed);
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), entry));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        if (replayed) {
            QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "a token this transport is allowed to carry should have been replayed");
            QCOMPARE(sent.value(qsl("token")).toString(), qsl("saved-token"));
            return;
        }
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "a token this transport may not carry should fall through to the hand-off");
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
        QVERIFY2(waitForConsoleContains(host, qsl("not encrypted")), "the user should be told why their saved sign-in was not used");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "a token refused on this transport must not be destroyed");
    }

    void testCleartextTokenFallsBackToProviderResume()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"saved-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send the resume form");
        QCOMPARE(sent.value(qsl("provider")).toString(), qsl("discord"));
        QVERIFY2(!sent.contains(qsl("token")), "the resume form must not carry the token");
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    void testRejectedReconnectTokenIsDiscarded()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        // No provider in the entry: with nothing to resume, rejection removes the entry entirely.
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"token\": \"stale-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        // The server rejects the reconnect; the client must discard the token and say so.
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("saved sign-in has expired")), "a rejected reconnect should be reported");
        QVERIFY2(waitForNoStoredReconnect(host), "a rejected token with no resume hint should be removed from storage");
    }

    void testRejectedReconnectKeepsResumeHint()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        // Provider present: rejection must drop only the token, keeping {account, provider} so the
        // next sign-in can resume the same provider without a menu.
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"stale-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("saved sign-in has expired")), "a rejected reconnect should be reported");
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("account")).toString() == qsl("acct:char") && entry.value(qsl("provider")).toString() == qsl("discord")
                                                   && !entry.contains(qsl("token"));
                                        }),
                 "rejection should rewrite the entry as an {account, provider} resume hint with no leftover token");
    }

    void testDroppingADeadTokenRemovesTheTokenKey()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"secure_only\": true}"), qsl("dead-token")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return CredentialManager::retrieveCredential(host->getName(), qsl("reconnect-token")).isEmpty();
                         },
                         4000),
                 "a dead token must not survive under its own key");
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("provider")).toString() == qsl("discord") && !entry.contains(qsl("token"));
                                        }),
                 "the resume hint should remain");
    }

    void testForgettingTheSavedSignInRemovesBothKeys()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"secure_only\": false}"), qsl("forget-me")));

        bool reported = false;
        bool removed = false;
        host->mpAuth->forgetSavedSignIn([&](bool success) {
            reported = true;
            removed = success;
        });

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return reported;
                         },
                         4000),
                 "forgetSavedSignIn never reported an outcome");
        QVERIFY2(removed, "forgetting a saved sign-in should report success");
        QVERIFY2(CredentialManager::retrieveCredential(host->getName(), qsl("reconnect-token")).isEmpty(), "the token key should be gone");
        QVERIFY2(CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "the metadata key should be gone");
    }

    void testAFailedTokenRemovalKeepsTheWholeEntry()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"secure_only\": false}"), qsl("forget-me")));

        // Block the token key's own file so its removal cannot succeed. Removing the metadata anyway
        // would strand the token: preferences only offers "Forget saved sign-in" when the metadata key
        // exists, so the entry has to survive whole for the player to be able to try again.
        const QString tokenPath = reconnectCredentialPath(host->getName(), qsl("reconnect-token"));
        QVERIFY2(QFileInfo::exists(tokenPath), qPrintable(qsl("the credential store no longer files entries at %1").arg(tokenPath)));
        QVERIFY(CredentialManager::removeCredential(host->getName(), qsl("reconnect-token")));
        QVERIFY(QDir().mkpath(tokenPath));

        bool reported = false;
        bool removed = true;
        host->mpAuth->forgetSavedSignIn([&](bool success) {
            reported = true;
            removed = success;
        });

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return reported;
                         },
                         4000),
                 "forgetSavedSignIn never reported an outcome");
        QVERIFY2(!removed, "a failed token removal must not be reported as a success");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(),
                 "the metadata must survive a failed token removal, or preferences can never offer to remove the token again");
    }

    void testResumeSentWhenTokenAbsentButProviderRemembered()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        // A resume hint left behind by an earlier rejection: no token, provider remembered.
        const QString hintJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), hintJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send the resume form");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("acct:char"));
        QCOMPARE(sent.value(qsl("provider")).toString(), qsl("discord"));
        QVERIFY2(!sent.contains(qsl("password")), "the resume form must not carry a password");
        QCOMPARE(sent.value(qsl("version")).toInt(), 2);
        QCOMPARE(sent.value(qsl("token_storage")), QJsonValue(true));
    }

    void testProviderFromUrlIsPersistedWithToken()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        // The provider named on Char.Login.URL must be remembered and stored with the token that
        // follows, so a later connection can resume this provider's sign-in.
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin\", \"provider\": \"discord\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("To sign in, open this link")), "the unprompted URL should be offered as a link");
        mpServer->sendGmcp(qsl("Char.Login.Token {\"account\": \"acct:char\", \"token\": \"opaque-token\"}"));
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("account")).toString() == qsl("acct:char") && entry.value(qsl("provider")).toString() == qsl("discord");
                                        }),
                 "the metadata should be persisted together with the provider learned from Char.Login.URL");
        QVERIFY2(waitForStoredToken(host, qsl("opaque-token")), "the token should be persisted under its own key");
    }

    void testRotatedTokenIsReplayedNotDiscarded()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-A\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-A"));

        // Another running instance sharing this profile's store rotates the (single-use) token while
        // ours is in flight; our replay of token-A is therefore rejected.
        const QString rotatedJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-B\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), rotatedJson));
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));

        // The client must notice the store changed and replay the fresh token instead of destroying it.
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the rotated token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-B"));
        const QJsonObject stored = readStoredReconnect(host);
        QCOMPARE(stored.value(qsl("token")).toString(), qsl("token-B"));
        QCOMPARE(stored.value(qsl("provider")).toString(), qsl("discord"));
        QCOMPARE(stored.value(qsl("account")).toString(), qsl("acct:char"));

        // Reject token-B too. The one-shot retriedRotatedToken guard allows at most one rotation retry
        // per connection, so this second rejection must NOT trigger a third Char.Login.Reconnect - the
        // token is dropped (rewritten to a resume hint) and the client re-signs-in instead.
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("saved sign-in has expired")), "the second rejection should be reported, not retried");
        QTest::qWait(300ms);
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    void testRotatedTokenIsReplayedInTheClearWhenTheServerAllowsIt()
    {
        // The rotation path reads the stored requirement for itself. On a plain-telnet game that mints
        // replayable-in-the-clear tokens - the configuration issue #10585 was reported from - a second
        // Mudlet instance sharing the store rotates the single-use token, and the retry has to honour
        // that stored false rather than refuse on transport grounds.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("{\"account\": \"acct:char\", \"token\": \"token-A\", \"secure_only\": false}")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-A"));

        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-B\", \"secure_only\": false}")));
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));

        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "the rotated token should be replayed on a transport its requirement allows");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-B"));
    }

    void testRotationIsDetectedWhenTheRotatedTokenIsUnderItsOwnKey()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"secure_only\": true}"), qsl("token-A")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-A"));

        // Another instance sharing the store rotates the single-use token, in split format.
        QVERIFY(seedSplitSignIn(host->getName(), qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"secure_only\": true}"), qsl("token-B")));
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));

        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "the rotated token under its own key should be replayed, not discarded");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-B"));
    }

    void testRotatedTokenRefusedOnTransportGroundsIsLeftAlone_data()
    {
        QTest::addColumn<QString>("rotatedEntry");
        QTest::newRow("explicitly encrypted-only") << qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-B\", \"secure_only\": true}");
        // An instance running an older Mudlet shares the store and writes no requirement at all. The
        // rotation path has to read that strictly for itself, exactly as readStoredSignIn does.
        QTest::newRow("written by a Mudlet that stored no requirement") << qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-B\"}");
    }

    void testRotatedTokenRefusedOnTransportGroundsIsLeftAlone()
    {
        QFETCH(QString, rotatedEntry);
        // The mirror of the case above: the other instance's fresh token requires encryption that this
        // connection does not have. It must not be replayed, and - because it is another instance's live
        // token, not a dead one - must not be destroyed either.
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        QVERIFY2(!host->mTelnet.currentlySecure(), "precondition: this connection is unencrypted");
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("{\"account\": \"acct:char\", \"token\": \"token-A\", \"secure_only\": false}")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-A"));

        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), rotatedEntry));
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));

        QVERIFY2(waitForConsoleContains(host, qsl("not encrypted")), "the user should be told why the rotated token was not used");
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
        QCOMPARE(readStoredReconnect(host).value(qsl("token")).toString(), qsl("token-B"));
    }

    void testRotationReplayClearsTheRejectionLatch()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-A\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-A"));

        // Another instance rotates the token, so our replay of token-A is rejected and the client replays
        // the fresh token-B rather than discarding it.
        const QString rotatedJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"token-B\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), rotatedJson));
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the rotated token");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-B"));

        // Replaying a rotated token is an ordinary sign-in, not a recovery from a dead one, so the
        // rejection latch must have been released again: a following Char.Login.Default may replay the
        // stored token. Were the latch left set, this would come back as the token-less resume form and
        // the player would face a browser sign-in despite holding a good token.
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "the rejection latch leaked past the rotation replay");
        QCOMPARE(sent.value(qsl("token")).toString(), qsl("token-B"));
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Credentials")), 0);
    }

    void testCorruptStoredEntryFallsThroughToHandoff()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        // A corrupt (non-JSON) reconnect entry must not stall the sign-in: the client cannot read a
        // token or provider from it, so it falls through to the interactive hand-off.
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), qsl("this is not valid json {")));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "a corrupt stored entry should fall through to the hand-off");
        QVERIFY2(isVersionTwoHandoff(sent), qPrintable(qsl("the fall-through must be the hand-off, not a reconnect or a partial replay, got %1").arg(describe(sent))));
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    // ---- Version negotiation clamp -----------------------------------------

    void testTooHighServerVersionIsClampedToTwo()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        // A server claiming a version above what this client implements must be clamped to 2, not echoed
        // verbatim - the client must never claim to speak a version it does not implement.
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 5, \"type\": [\"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send credentials");
        QCOMPARE(sent.value(qsl("version")).toInt(), 2);
    }

    void testNonPositiveServerVersionIsTreatedAsOne()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 0, \"type\": [\"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not send credentials");
        QCOMPARE(sent.value(qsl("version")).toInt(), 1);
    }

    // ---- Precedence: stored credentials outrank a saved token --------------

    void testStoredCredentialsOutrankSavedToken()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        // Both a saved reconnect token AND stored character name/password are present. The player's
        // typed credentials name the exact character, so they must win: the client sends
        // Char.Login.Credentials and never replays the token.
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"saved-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not autofill stored credentials");
        QCOMPARE(sent.value(qsl("account")).toString(), qsl("player"));
        QCOMPARE(sent.value(qsl("password")).toString(), qsl("secret"));
        QTest::qWait(300ms);
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    // ---- Char.Login.URL positive auto-open ---------------------------------

    void testPromptedAuthUrlIsAutoOpened()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        // Simulate the player having acted on the game's sign-in screen this connection; an unsolicited
        // Char.Login.URL is then a consequence of their input and must be auto-opened in the browser.
        host->setUserSentInputThisConnection(true);
        mOpenedUrls.clear();
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin\", \"provider\": \"discord\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("Opening your browser to sign in with Discord")), "a prompted URL should be auto-opened with a provider-labelled handoff");
        QCOMPARE(mOpenedUrls, QList<QUrl>{QUrl(qsl("https://example.com/signin"))});
    }

    void testRepeatedAuthUrlsOpenOneBrowserPerUserAction()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setUserSentInputThisConnection(true);
        mOpenedUrls.clear();

        for (int i = 1; i <= 5; ++i) {
            mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin%1\"}").arg(i));
        }
        // GMCP frames are handled in order, so a reply to this one proves all five were processed.
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "client did not work through the pushed sign-in addresses");
        QCOMPARE(mOpenedUrls, QList<QUrl>{QUrl(qsl("https://example.com/signin1"))});

        // A further player action re-arms it: a rate limit, not a one-per-connection cap.
        host->setUserSentInputThisConnection(true);
        mpServer->sendGmcp(qsl("Char.Login.URL {\"url\": \"https://example.com/signin6\"}"));
        QTRY_COMPARE(mOpenedUrls.size(), 2);
        QCOMPARE(mOpenedUrls.at(1), QUrl(qsl("https://example.com/signin6")));
    }

    // ---- Post-rejection loop guard (allowToken == false) -------------------

    void testReconnectAfterRejectionDoesNotReplayToken()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        const QString tokenJson = qsl("{\"account\": \"acct:char\", \"provider\": \"discord\", \"token\": \"stale-token\"}");
        QVERIFY(CredentialManager::storeCredential(host->getName(), qsl("reconnect"), tokenJson));

        const int firstConnection = mpServer->connectionCount();
        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Reconnect"), sent), "client did not replay the saved token");

        // Reject the token. The client rewrites the entry to a resume hint and reconnects. On that
        // follow-on connection the mReconnectRejected latch makes readStoredSignIn(false) run - it must
        // NOT replay a token (even though the async rewrite may not have landed yet), or a rejected
        // reconnect could loop. It sends the resume form instead.
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Reconnect token expired\"}"));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mpServer->connectionCount() > firstConnection && mpServer->gmcpEnabled();
                         },
                         8000),
                 "client did not reconnect and renegotiate GMCP after the rejection");

        mpServer->clearReceived();
        mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "the post-rejection connection should send the resume form");
        QCOMPARE(sent.value(qsl("provider")).toString(), qsl("discord"));
        QVERIFY2(!sent.contains(qsl("password")), "the resume form carries no password");
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
    }

    // NOTE: the superseded-callback (mAuthAttemptGeneration) path in readStoredSignIn and
    // retryOrDropRejectedToken - taken when a newer Char.Login.Default arrives before the reconnect-token
    // keychain read resolves - is intentionally NOT covered here. It is not deterministically testable
    // with the current harness: in test/portable mode CredentialManager reads credentials synchronously
    // and inline (see CredentialManager::retrievePassword), so a read always completes before any
    // superseding Char.Login.Default can arrive, and the race never occurs. Exercising it would require an
    // injectable, genuinely-asynchronous credential manager.
    //
    // Worth the seam if anyone revisits this: in readStoredSignIn the path is a bare early return, but in
    // retryOrDropRejectedToken it decides whether to rewrite the stored entry and whether to re-arm
    // mReconnectRejected. Getting either wrong loses a player's freshly saved token or lets a rejected one
    // be replayed, and neither failure is reachable by hand.

    // ---- Client-driven OAuth (Char.Login.AuthCode) -------------------------

    void testClientDrivenOAuthOpensOneBrowserPerConnection()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        QVERIFY2(!host->userSentInputThisConnection(), "precondition: no user input yet");
        startDiscoveryServer();
        mOpenedUrls.clear();

        // Connecting is itself the request, so the first offer opens the browser with nothing typed.
        mpServer->sendGmcp(clientDrivenDefault());
        QTRY_COMPARE(mOpenedUrls.size(), 1);

        for (int i = 0; i < 4; ++i) {
            mpServer->sendGmcp(clientDrivenDefault());
        }
        QVERIFY2(waitForConsoleContains(host, qsl("To sign in, open this link")), "a re-offered client-driven sign-in should be offered as a link");
        QCOMPARE(mOpenedUrls.size(), 1);
    }

    void testAuthCodeCarriesTheNonceTheServerAskedFor()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        startDiscoveryServer();
        mOpenedUrls.clear();
        mpServer->clearReceived();

        mpServer->sendGmcp(clientDrivenDefault());
        QTRY_VERIFY(!mOpenedUrls.isEmpty());
        const QUrlQuery authorizationQuery(mOpenedUrls.first());
        const QString nonce = authorizationQuery.queryItemValue(qsl("nonce"));
        QVERIFY2(!nonce.isEmpty(), "the authorization request should carry a nonce when the server asked for one");

        // Play the identity provider: send the browser's redirect back to the loopback listener.
        const QUrl redirectUri(authorizationQuery.queryItemValue(qsl("redirect_uri"), QUrl::FullyDecoded));
        QTcpSocket browser;
        browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
        QVERIFY(browser.waitForConnected(3000));
        browser.write("GET /?code=test-auth-code&state=" + authorizationQuery.queryItemValue(qsl("state")).toLatin1() + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.AuthCode"), sent), "client did not complete the client-driven sign-in");
        QCOMPARE(sent.value(qsl("code")).toString(), qsl("test-auth-code"));
        QVERIFY(!sent.value(qsl("code_verifier")).toString().isEmpty());
        QCOMPARE(sent.value(qsl("redirect_uri")).toString(), redirectUri.toString());
        QCOMPARE(sent.value(qsl("nonce")).toString(), nonce);
        // The common fields ride on AuthCode too. Asserted here because this is the only test that
        // completes a client-driven sign-in, and so the only place the message's shape is observable.
        QCOMPARE(sent.value(qsl("version")).toInt(), 2);
        QCOMPARE(sent.value(qsl("token_storage")), QJsonValue(true));
    }

    void testTheLegacyNonceKeyStillRequestsANonce()
    {
        // Issue #10623: Mudlet read "nonce" where the standard's field is "nonce_required". The code
        // now reads the standard's name, but a server written against the old behaviour must keep
        // working, so the old key is still honoured when the standard's is absent.
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        startDiscoveryServer();
        mOpenedUrls.clear();

        mpServer->sendGmcp(clientDrivenDefaultWithLegacyNonceKey());
        QTRY_VERIFY(!mOpenedUrls.isEmpty());

        const QUrlQuery authorizationQuery(mOpenedUrls.first());
        QVERIFY2(!authorizationQuery.queryItemValue(qsl("nonce")).isEmpty(), "the legacy nonce key should still request a nonce");
    }

    void testAuthCodeOmitsTheNonceWhenTheServerDidNotAskForIt()
    {
        Host* host = connectAndNegotiate(true);
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());
        startDiscoveryServer();
        mOpenedUrls.clear();
        mpServer->clearReceived();

        mpServer->sendGmcp(clientDrivenDefault(false));
        QTRY_VERIFY(!mOpenedUrls.isEmpty());
        const QUrlQuery authorizationQuery(mOpenedUrls.first());
        QVERIFY2(!authorizationQuery.hasQueryItem(qsl("nonce")), "no nonce should be requested from the provider either");

        const QUrl redirectUri(authorizationQuery.queryItemValue(qsl("redirect_uri"), QUrl::FullyDecoded));
        QTcpSocket browser;
        browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
        QVERIFY(browser.waitForConnected(3000));
        browser.write("GET /?code=test-auth-code&state=" + authorizationQuery.queryItemValue(qsl("state")).toLatin1() + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.AuthCode"), sent), "client did not complete the client-driven sign-in");
        QVERIFY2(!sent.contains(qsl("nonce")), "an empty nonce must be left out rather than sent as an empty string");
    }

    // ---- Char.Login.Default flood ------------------------------------------

    void testDefaultFloodIsThrottled()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(QString());
        host->setPass(QString());

        mpServer->clearReceived();
        for (int i = 0; i < 200; ++i) {
            mpServer->sendGmcp(qsl("Char.Login.Default {\"version\": 2, \"type\": [\"oauth\", \"password-credentials\"]}"));
        }

        QJsonObject sent;
        QVERIFY2(waitForClientGmcp(qsl("Char.Login.Credentials"), sent), "the first frame should still be answered straight away");

        // Cost is bounded by wall clock, not by how much the server sent: one immediate attempt plus
        // one when the window closes, not 200. A range, so a loaded runner slipping into the next
        // window does not flake, and a lower bound because a re-offer must still be answered.
        QTest::qWait(2500ms);
        const int attempts = mpServer->countReceived(qsl("Char.Login.Credentials"));
        QVERIFY2(attempts >= 2, qPrintable(qsl("a throttled burst must still be answered, saw %1 attempts").arg(attempts)));
        QVERIFY2(attempts <= 4, qPrintable(qsl("200 frames should not buy 200 sign-in attempts, saw %1").arg(attempts)));
    }

    void testTypelessDefaultDropsAnAlreadyArmedAttempt()
    {
        Host* host = connectAndNegotiate();
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(qsl("secret"));

        mpServer->clearReceived();
        // One burst: the first frame is answered straight away, the second falls inside the throttle
        // window and arms an attempt for when it closes, and the third leaves no capabilities for that
        // attempt to act on.
        const auto typed = qsl("Char.Login.Default {\"version\": 2, \"type\": [\"password-credentials\"]}");
        mpServer->sendGmcp(typed);
        mpServer->sendGmcp(typed);
        mpServer->sendGmcp(qsl("Char.Login.Default {}"));

        QTest::qWait(2500ms);
        const int attempts = mpServer->countReceived(qsl("Char.Login.Credentials"));
        QCOMPARE(attempts, 1);
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
    void startDiscoveryServer()
    {
        mpDiscovery = new DiscoveryServerStub();
        QVERIFY(mpDiscovery->start());
    }

    // Advertises the client-driven OAuth capability, which the client only honours over TLS. The field
    // is nonce_required, deliberately named apart from the string nonce that Char.Login.URL and
    // Char.Login.AuthCode carry.
    QString clientDrivenDefault(bool requestNonce = true) const
    {
        return qsl(R"(Char.Login.Default {"version": 2, "type": ["oauth"], "location": "%1", "client_id": "test-client", "nonce_required": %2})")
                .arg(mpDiscovery->discoveryUrl(), requestNonce ? qsl("true") : qsl("false"));
    }

    // The key Mudlet read before issue #10623. Still honoured so a server written against the old
    // behaviour keeps working.
    QString clientDrivenDefaultWithLegacyNonceKey() const
    {
        return qsl(R"(Char.Login.Default {"version": 2, "type": ["oauth"], "location": "%1", "client_id": "test-client", "nonce": true})").arg(mpDiscovery->discoveryUrl());
    }

    // Drive the GUI to create/connect a profile, then wait for GMCP to negotiate. Reaching TLS by
    // reconnecting rather than creating the profile encrypted is what makes this deterministic:
    // mSslTsl and mSslIgnoreAll are set on a live Host, before the attempt that reads them starts.
    Host* connectAndNegotiate(bool secure = false)
    {
        Host* host = createProfileAndConnect();
        if (!host || !secure) {
            return host;
        }
        host->mSslTsl = true;
        host->mSslIgnoreAll = true; // the stub's certificate is self-signed
        mpServer->setTls(true);
        const int plainConnection = mpServer->connectionCount();
        host->mTelnet.reconnect();
        if (!waitForNegotiatedConnection(plainConnection)) {
            return nullptr;
        }
        if (!mpServer->clientEncrypted()) {
            qWarning("The connection did not complete a TLS handshake");
            return nullptr;
        }
        return host;
    }

    Host* createProfileAndConnect()
    {
        const QString port = QString::number(mPort);
        // A fresh mudlet and profile per test on an instrumented, loaded runner is slow.
        Host* host = TestProfile::create(mHostname, qsl("localhost"), port, 20s);
        if (!host) {
            qWarning("No active host");
            return nullptr;
        }
        return waitForNegotiatedConnection(0) ? host : nullptr;
    }

    // Also waits for the client to answer our GMCP offer, so frames pushed afterwards are processed.
    bool waitForNegotiatedConnection(int afterConnectionCount)
    {
        const bool connected = QTest::qWaitFor(
                [this, afterConnectionCount]() {
                    return mpServer->connectionCount() > afterConnectionCount && mpServer->gmcpEnabled();
                },
                15000);
        if (!connected) {
            qWarning("Could not connect to the stub, or GMCP was not negotiated");
        }
        return connected;
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

    static int consoleOccurrences(Host* host, const QString& substring)
    {
        if (!host || !host->mpConsole) {
            return 0;
        }
        auto& buffer = host->mpConsole->buffer;
        int seen = 0;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i).contains(substring)) {
                ++seen;
            }
        }
        return seen;
    }

    static bool consoleContains(Host* host, const QString& substring)
    {
        if (!host || !host->mpConsole) {
            return false;
        }
        auto& buffer = host->mpConsole->buffer;
        QString all;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            all.append(buffer.line(i));
            all.append(QChar::Space);
        }
        return all.contains(substring);
    }

    // Matches ignoring every space and line break on both sides, so an assertion on a whole sentence does
    // not depend on where the console happened to wrap it. Use it only where the wording itself is the
    // thing under test; waitForConsoleContains is the right tool for a short distinctive phrase.
    bool waitForConsoleContainsUnwrapped(Host* host, const QString& sentence, int timeoutMs = 4000)
    {
        static const QRegularExpression whitespace(qsl("\\s+"));
        const QString needle = QString(sentence).remove(whitespace);
        return QTest::qWaitFor(
                [&]() {
                    if (!host || !host->mpConsole) {
                        return false;
                    }
                    auto& buffer = host->mpConsole->buffer;
                    QString all;
                    for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
                        all.append(buffer.line(i));
                    }
                    return all.remove(whitespace).contains(needle);
                },
                timeoutMs);
    }

    bool waitForConsoleContains(Host* host, const QString& substring, int timeoutMs = 4000)
    {
        return QTest::qWaitFor(
                [&]() {
                    return consoleContains(host, substring);
                },
                timeoutMs);
    }

    void removeBlockingCredentialDirectory()
    {
        for (const auto& key : {qsl("reconnect"), qsl("reconnect-token")}) {
            QDir blockedCredential(reconnectCredentialPath(mHostname, key));
            if (blockedCredential.exists()) {
                blockedCredential.removeRecursively();
            }
        }
    }

    // Where the file-backed credential store files this profile's entry under the given key. Blocking
    // that exact path is how a test makes a save fail; see testAFailedSaveIsNotAnnouncedAsASuccess and
    // testATornSaveLeavesAResumeHintAndNoPromise.
    static QString reconnectCredentialPath(const QString& profileName, const QString& key)
    {
        return qsl("%1/profiles/%2/passwords/%3").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), utils::sanitizeForPath(profileName), utils::sanitizeForPath(key));
    }

    // Seeds the split storage format: metadata under "reconnect", the token under its own key. The
    // inline-JSON seeds elsewhere in this file are the legacy format on purpose - they are what a
    // Mudlet from before the split wrote, and the read path still has to understand them.
    static bool seedSplitSignIn(const QString& profileName, const QString& metadataJson, const QString& token)
    {
        return CredentialManager::storeCredential(profileName, qsl("reconnect"), metadataJson) && CredentialManager::storeCredential(profileName, qsl("reconnect-token"), token);
    }

    static QString describe(const QJsonObject& obj) { return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)); }

    // The version 2 interactive hand-off is identified by the absence of account, not by the payload
    // being literally {}: the common fields ride on it, and token_storage riding there is the whole
    // point - it reaches the game before the game writes a line of its sign-in screen.
    static bool isVersionTwoHandoff(const QJsonObject& sent)
    {
        return !sent.contains(qsl("account")) && !sent.contains(qsl("password")) && sent.value(qsl("version")).toInt() == 2 && sent.value(qsl("token_storage")) == QJsonValue(true);
    }

    // Parse the stored reconnect entry as JSON so tests can assert its exact shape rather than
    // matching loose substrings (a rewritten or malformed entry could otherwise pass).
    static QJsonObject readStoredReconnect(Host* host)
    {
        const QString stored = CredentialManager::retrieveCredential(host->getName(), qsl("reconnect"));
        return QJsonDocument::fromJson(stored.toUtf8()).object();
    }

    // Wait until the parsed reconnect entry satisfies the predicate.
    bool waitForStoredReconnect(Host* host, const std::function<bool(const QJsonObject&)>& predicate, int timeoutMs = 4000)
    {
        if (!host) {
            return false;
        }
        return QTest::qWaitFor(
                [&]() {
                    return predicate(readStoredReconnect(host));
                },
                timeoutMs);
    }

    // Wait until the token key holds exactly this value. The token has its own key now (see
    // storeReconnectToken), so a test asserting on a freshly-saved token's value checks this rather than
    // the metadata read above.
    bool waitForStoredToken(Host* host, const QString& expected, int timeoutMs = 4000)
    {
        if (!host) {
            return false;
        }
        return QTest::qWaitFor(
                [&]() {
                    return CredentialManager::retrieveCredential(host->getName(), qsl("reconnect-token")) == expected;
                },
                timeoutMs);
    }

    // Wait until the reconnect entry has been removed from storage entirely.
    bool waitForNoStoredReconnect(Host* host, int timeoutMs = 4000)
    {
        if (!host) {
            return false;
        }
        return QTest::qWaitFor(
                [&]() {
                    return CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty();
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

#include "GMCPCharLoginTest.moc"
MUDLET_GROUPED_TEST_MAIN(GMCPCharLoginTest)
