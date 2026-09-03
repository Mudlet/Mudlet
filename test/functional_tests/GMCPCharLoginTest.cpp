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

#include <QFileInfo>
#include <QSettings>
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
#include <QUrlQuery>
#include <functional>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "CredentialManager.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

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
        CredentialManager::removeCredential(mHostname, qsl("reconnect"));
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
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
        QVERIFY2(waitForStoredReconnect(host,
                                        [](const QJsonObject& entry) {
                                            return entry.value(qsl("account")).toString() == qsl("acct:char") && entry.value(qsl("token")).toString() == qsl("opaque-token");
                                        }),
                 "the reconnect token should be persisted with the announced account and token");
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
        QVERIFY2(sent.isEmpty(), "the fall-back must be the empty {} hand-off");
        QCOMPARE(mpServer->countReceived(qsl("Char.Login.Reconnect")), 0);
        QVERIFY2(waitForConsoleContains(host, qsl("not encrypted")), "the user should be told why their saved sign-in was not used");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "refusing to send the token must not destroy it");

        // Nothing awaits a reconnect result, so an ordinary failed sign-in must not be mistaken for a
        // rejected token - that would rewrite or delete the stored entry the player still needs.
        mpServer->sendGmcp(qsl("Char.Login.Result {\"success\": false, \"message\": \"Invalid credentials\"}"));
        QVERIFY2(waitForConsoleContains(host, qsl("Could not log in to the game")), "a failed interactive sign-in should be reported as one");
        QVERIFY2(!CredentialManager::retrieveCredential(host->getName(), qsl("reconnect")).isEmpty(), "the stored sign-in must survive an unrelated login failure");
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
                                            return entry.value(qsl("account")).toString() == qsl("acct:char") && entry.value(qsl("token")).toString() == qsl("opaque-token")
                                                   && entry.value(qsl("provider")).toString() == qsl("discord");
                                        }),
                 "the token should be persisted together with the provider learned from Char.Login.URL");
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
        QVERIFY2(sent.isEmpty(), "the fall-through must be the empty {} hand-off, not a reconnect or a partial replay");
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

    // Advertises the client-driven OAuth capability, which the client only honours over TLS.
    QString clientDrivenDefault(bool requestNonce = true) const
    {
        return qsl(R"(Char.Login.Default {"version": 2, "type": ["oauth"], "location": "%1", "client_id": "test-client", "nonce": %2})")
                .arg(mpDiscovery->discoveryUrl(), requestNonce ? qsl("true") : qsl("false"));
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
