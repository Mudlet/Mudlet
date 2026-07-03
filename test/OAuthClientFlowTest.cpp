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

#include <OAuthClientFlow.h>
#include <QtTest/QtTest>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>

// Serves a static OpenID Connect discovery document over loopback HTTP so the
// flow's QNetworkAccessManager fetch has something real to talk to.
class MiniDiscoveryServer : public QObject
{
public:
    explicit MiniDiscoveryServer(const QString& authorizationEndpoint, QObject* parent = nullptr)
    : QObject(parent)
    {
        mBody = QJsonDocument(QJsonObject{{QStringLiteral("authorization_endpoint"), authorizationEndpoint}}).toJson(QJsonDocument::Compact);
        const bool listening = mServer.listen(QHostAddress::LocalHost, 0);
        if (!listening) {
            qWarning() << "MiniDiscoveryServer failed to bind a loopback port:" << mServer.errorString();
        }
        Q_ASSERT_X(listening, "MiniDiscoveryServer", "failed to bind a loopback port for the test discovery server");
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            while (mServer.hasPendingConnections()) {
                QTcpSocket* socket = mServer.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                    socket->readAll();
                    QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(mBody.size()) + "\r\nConnection: close\r\n\r\n" + mBody;
                    socket->write(response);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QUrl discoveryUrl() const { return QUrl(QStringLiteral("http://127.0.0.1:%1/.well-known/openid-configuration").arg(mServer.serverPort())); }

private:
    QTcpServer mServer;
    QByteArray mBody;
};

// Accepts the discovery connection and then immediately closes it without sending a response,
// so the QNetworkReply error path is guaranteed regardless of how a given host treats a closed
// or refused port.
class MiniClosingServer : public QObject
{
public:
    explicit MiniClosingServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        const bool listening = mServer.listen(QHostAddress::LocalHost, 0);
        if (!listening) {
            qWarning() << "MiniClosingServer failed to bind a loopback port:" << mServer.errorString();
        }
        Q_ASSERT_X(listening, "MiniClosingServer", "failed to bind a loopback port for the test discovery server");
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            while (mServer.hasPendingConnections()) {
                QTcpSocket* socket = mServer.nextPendingConnection();
                socket->abort();
                socket->deleteLater();
            }
        });
    }

    QUrl discoveryUrl() const { return QUrl(QStringLiteral("http://127.0.0.1:%1/.well-known/openid-configuration").arg(mServer.serverPort())); }

private:
    QTcpServer mServer;
};

class OAuthClientFlowTest : public QObject
{
    Q_OBJECT

public slots:
    void captureBrowserUrl(const QUrl& url) { mBrowserUrl = url; }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testCodeVerifierFormat();
    void testCodeVerifierUnique();
    void testCodeChallengeRfc7636Vector();
    void testBuildAuthorizationUrl();
    void testBuildAuthorizationUrlOmitsEmptyNonce();
    void testFullFlowCapturesAuthorizationCode();
    void testStateMismatchFailsFlow();
    void testNonRedirectRequestIgnored();
    void testDiscoveryFetchFailureFailsFlow();
    void testNonLoopbackHttpDiscoveryUrlRejected();
    void testProviderErrorFailsFlow();

private:
    QUrl mBrowserUrl;
};

void OAuthClientFlowTest::initTestCase()
{
    QDesktopServices::setUrlHandler(QStringLiteral("http"), this, "captureBrowserUrl");
}

void OAuthClientFlowTest::cleanupTestCase()
{
    QDesktopServices::unsetUrlHandler(QStringLiteral("http"));
}

void OAuthClientFlowTest::init()
{
    mBrowserUrl.clear();
}


void OAuthClientFlowTest::testCodeVerifierFormat()
{
    const QString verifier = OAuthClientFlow::generateCodeVerifier();
    // RFC 7636 requires 43-128 characters from the unreserved set; 32 random bytes
    // base64url-encoded without padding is exactly 43.
    QCOMPARE(verifier.length(), 43);
    const QRegularExpression unreserved(QStringLiteral("^[A-Za-z0-9\\-._~]+$"));
    QVERIFY(unreserved.match(verifier).hasMatch());
}

void OAuthClientFlowTest::testCodeVerifierUnique()
{
    QVERIFY(OAuthClientFlow::generateCodeVerifier() != OAuthClientFlow::generateCodeVerifier());
}

void OAuthClientFlowTest::testCodeChallengeRfc7636Vector()
{
    // Test vector from RFC 7636 Appendix B.
    const QString verifier = QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    QCOMPARE(OAuthClientFlow::codeChallengeS256(verifier), QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
}

void OAuthClientFlowTest::testBuildAuthorizationUrl()
{
    const QUrl url = OAuthClientFlow::buildAuthorizationUrl(QUrl(QStringLiteral("https://example.com/authorize")),
                                                            QStringLiteral("mud-native-client"),
                                                            {QStringLiteral("openid"), QStringLiteral("profile")},
                                                            QStringLiteral("http://127.0.0.1:49152/"),
                                                            QStringLiteral("test-state"),
                                                            QStringLiteral("test-challenge"),
                                                            QStringLiteral("test-nonce"));
    QVERIFY(url.isValid());
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("example.com"));
    QCOMPARE(url.path(), QStringLiteral("/authorize"));

    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("response_type")), QStringLiteral("code"));
    QCOMPARE(query.queryItemValue(QStringLiteral("client_id")), QStringLiteral("mud-native-client"));
    QCOMPARE(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded), QStringLiteral("http://127.0.0.1:49152/"));
    QCOMPARE(query.queryItemValue(QStringLiteral("scope"), QUrl::FullyDecoded), QStringLiteral("openid profile"));
    QCOMPARE(query.queryItemValue(QStringLiteral("state")), QStringLiteral("test-state"));
    QCOMPARE(query.queryItemValue(QStringLiteral("code_challenge")), QStringLiteral("test-challenge"));
    QCOMPARE(query.queryItemValue(QStringLiteral("code_challenge_method")), QStringLiteral("S256"));
    QCOMPARE(query.queryItemValue(QStringLiteral("nonce")), QStringLiteral("test-nonce"));
}

void OAuthClientFlowTest::testBuildAuthorizationUrlOmitsEmptyNonce()
{
    const QUrl url = OAuthClientFlow::buildAuthorizationUrl(QUrl(QStringLiteral("https://example.com/authorize")),
                                                            QStringLiteral("mud-native-client"),
                                                            {QStringLiteral("openid")},
                                                            QStringLiteral("http://127.0.0.1:49152/"),
                                                            QStringLiteral("test-state"),
                                                            QStringLiteral("test-challenge"),
                                                            QString());
    const QUrlQuery query(url);
    QVERIFY(!query.hasQueryItem(QStringLiteral("nonce")));
}

void OAuthClientFlowTest::testFullFlowCapturesAuthorizationCode()
{
    MiniDiscoveryServer discovery(QStringLiteral("http://127.0.0.1:1/authorize"));
    OAuthClientFlow flow;
    QSignalSpy capturedSpy(&flow, &OAuthClientFlow::authorizationCaptured);
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);
    QSignalSpy openedSpy(&flow, &OAuthClientFlow::browserOpened);

    flow.start(discovery.discoveryUrl(), QStringLiteral("test-client"), {QStringLiteral("openid")}, true);
    QTRY_VERIFY(!mBrowserUrl.isEmpty());
    QCOMPARE(openedSpy.count(), 1);

    const QUrlQuery query(mBrowserUrl);
    QCOMPARE(query.queryItemValue(QStringLiteral("response_type")), QStringLiteral("code"));
    QCOMPARE(query.queryItemValue(QStringLiteral("client_id")), QStringLiteral("test-client"));
    QCOMPARE(query.queryItemValue(QStringLiteral("scope"), QUrl::FullyDecoded), QStringLiteral("openid"));
    QCOMPARE(query.queryItemValue(QStringLiteral("code_challenge_method")), QStringLiteral("S256"));
    QVERIFY(!query.queryItemValue(QStringLiteral("state")).isEmpty());
    QVERIFY(!query.queryItemValue(QStringLiteral("nonce")).isEmpty());
    const QString challenge = query.queryItemValue(QStringLiteral("code_challenge"));
    QVERIFY(!challenge.isEmpty());
    const QUrl redirectUri(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded));
    QCOMPARE(redirectUri.host(), QStringLiteral("127.0.0.1"));
    QVERIFY(redirectUri.port() > 0);

    // Simulate the provider redirecting the browser back to the loopback listener.
    QTcpSocket browser;
    browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
    QVERIFY(browser.waitForConnected(3000));
    browser.write("GET /?code=test-auth-code&state=" + query.queryItemValue(QStringLiteral("state")).toLatin1() + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

    QTRY_COMPARE(capturedSpy.count(), 1);
    const auto args = capturedSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("test-auth-code"));
    QCOMPARE(OAuthClientFlow::codeChallengeS256(args.at(1).toString()), challenge);
    QCOMPARE(args.at(2).toString(), redirectUri.toString());
    QCOMPARE(failedSpy.count(), 0);

    QTRY_VERIFY(browser.bytesAvailable() > 0);
    QVERIFY(browser.readAll().startsWith("HTTP/1.1 200"));
}

void OAuthClientFlowTest::testStateMismatchFailsFlow()
{
    MiniDiscoveryServer discovery(QStringLiteral("http://127.0.0.1:1/authorize"));
    OAuthClientFlow flow;
    QSignalSpy capturedSpy(&flow, &OAuthClientFlow::authorizationCaptured);
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);

    flow.start(discovery.discoveryUrl(), QStringLiteral("test-client"), {QStringLiteral("openid")}, false);
    QTRY_VERIFY(!mBrowserUrl.isEmpty());
    const QUrlQuery query(mBrowserUrl);
    const QUrl redirectUri(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded));

    QTcpSocket browser;
    browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
    QVERIFY(browser.waitForConnected(3000));
    browser.write(QByteArray("GET /?code=test-auth-code&state=wrong-state HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"));

    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(capturedSpy.count(), 0);

    QTRY_VERIFY(browser.bytesAvailable() > 0);
    QVERIFY(browser.readAll().startsWith("HTTP/1.1 400"));
}

void OAuthClientFlowTest::testNonRedirectRequestIgnored()
{
    MiniDiscoveryServer discovery(QStringLiteral("http://127.0.0.1:1/authorize"));
    OAuthClientFlow flow;
    QSignalSpy capturedSpy(&flow, &OAuthClientFlow::authorizationCaptured);
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);

    flow.start(discovery.discoveryUrl(), QStringLiteral("test-client"), {QStringLiteral("openid")}, false);
    QTRY_VERIFY(!mBrowserUrl.isEmpty());
    const QUrlQuery query(mBrowserUrl);
    const QUrl redirectUri(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded));

    // A browser side-request (no code/error) must be answered without ending the flow.
    QTcpSocket sideRequest;
    sideRequest.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
    QVERIFY(sideRequest.waitForConnected(3000));
    sideRequest.write(QByteArray("GET /favicon.ico HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"));
    QTRY_VERIFY(sideRequest.bytesAvailable() > 0);
    QVERIFY(sideRequest.readAll().startsWith("HTTP/1.1 404"));
    QCOMPARE(failedSpy.count(), 0);

    // The real redirect still completes afterwards.
    QTcpSocket browser;
    browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
    QVERIFY(browser.waitForConnected(3000));
    browser.write("GET /?code=test-auth-code&state=" + query.queryItemValue(QStringLiteral("state")).toLatin1() + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
    QTRY_COMPARE(capturedSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
}

void OAuthClientFlowTest::testDiscoveryFetchFailureFailsFlow()
{
    OAuthClientFlow flow;
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);
    // A server that accepts the connection then closes it immediately guarantees the discovery
    // fetch fails deterministically, rather than relying on a host refusing a particular port.
    MiniClosingServer discovery;
    flow.start(discovery.discoveryUrl(), QStringLiteral("test-client"), {QStringLiteral("openid")}, false);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 15000);
}

void OAuthClientFlowTest::testNonLoopbackHttpDiscoveryUrlRejected()
{
    OAuthClientFlow flow;
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);
    // Plain http is only acceptable for loopback hosts; anything else must be refused
    // before any network activity happens.
    flow.start(QUrl(QStringLiteral("http://example.com/.well-known/openid-configuration")), QStringLiteral("test-client"), {QStringLiteral("openid")}, false);
    QCOMPARE(failedSpy.count(), 1);
}

void OAuthClientFlowTest::testProviderErrorFailsFlow()
{
    MiniDiscoveryServer discovery(QStringLiteral("http://127.0.0.1:1/authorize"));
    OAuthClientFlow flow;
    QSignalSpy capturedSpy(&flow, &OAuthClientFlow::authorizationCaptured);
    QSignalSpy failedSpy(&flow, &OAuthClientFlow::flowFailed);

    flow.start(discovery.discoveryUrl(), QStringLiteral("test-client"), {QStringLiteral("openid")}, false);
    QTRY_VERIFY(!mBrowserUrl.isEmpty());
    const QUrlQuery query(mBrowserUrl);
    const QUrl redirectUri(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded));

    QTcpSocket browser;
    browser.connectToHost(redirectUri.host(), static_cast<quint16>(redirectUri.port()));
    QVERIFY(browser.waitForConnected(3000));
    browser.write("GET /?error=access_denied&state=" + query.queryItemValue(QStringLiteral("state")).toLatin1() + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(capturedSpy.count(), 0);
}

#include "OAuthClientFlowTest.moc"
QTEST_MAIN(OAuthClientFlowTest)
