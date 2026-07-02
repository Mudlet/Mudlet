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

#include "OAuthClientFlow.h"

#include "SecureStringUtils.h"
#include "utils.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QUrlQuery>

namespace {
QString base64Url(const QByteArray& bytes)
{
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// byteCount must be a multiple of sizeof(quint32); the system generator is the OS CSPRNG.
QString randomUrlSafeToken(int byteCount)
{
    QByteArray bytes(byteCount, '\0');
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(bytes.data()), byteCount / static_cast<int>(sizeof(quint32)));
    return base64Url(bytes);
}

// OAuth endpoints must be https; plain http is permitted solely for loopback addresses so
// self-hosted and test setups work without certificates (mirroring RFC 8252 loopback redirects).
bool acceptableEndpointUrl(const QUrl& url)
{
    if (!url.isValid() || url.host().isEmpty()) {
        return false;
    }
    if (url.scheme() == qsl("https")) {
        return true;
    }
    if (url.scheme() != qsl("http")) {
        return false;
    }
    const QString host = url.host();
    return host == qsl("127.0.0.1") || host == qsl("::1") || host.compare(qsl("localhost"), Qt::CaseInsensitive) == 0;
}
} // namespace

OAuthClientFlow::OAuthClientFlow(QObject* parent)
: QObject(parent)
{
}

OAuthClientFlow::~OAuthClientFlow()
{
    // The verifier is the PKCE secret; make sure it does not outlive the flow in memory.
    SecureStringUtils::secureStringClear(mCodeVerifier);
}

QString OAuthClientFlow::generateCodeVerifier()
{
    // 32 random bytes base64url-encoded without padding gives a 43-character verifier, the RFC 7636
    // minimum length, from the required unreserved character set.
    return randomUrlSafeToken(32);
}

QString OAuthClientFlow::codeChallengeS256(const QString& codeVerifier)
{
    // The verifier is ASCII by construction; RFC 7636 hashes its ASCII bytes.
    return base64Url(QCryptographicHash::hash(codeVerifier.toLatin1(), QCryptographicHash::Sha256));
}

QUrl OAuthClientFlow::buildAuthorizationUrl(
        const QUrl& authorizationEndpoint, const QString& clientId, const QStringList& scopes, const QString& redirectUri, const QString& state, const QString& codeChallenge, const QString& nonce)
{
    QUrl url = authorizationEndpoint;
    QUrlQuery query(url);
    query.addQueryItem(qsl("response_type"), qsl("code"));
    query.addQueryItem(qsl("client_id"), clientId);
    query.addQueryItem(qsl("redirect_uri"), redirectUri);
    query.addQueryItem(qsl("scope"), scopes.join(QChar::Space));
    query.addQueryItem(qsl("state"), state);
    query.addQueryItem(qsl("code_challenge"), codeChallenge);
    query.addQueryItem(qsl("code_challenge_method"), qsl("S256"));
    if (!nonce.isEmpty()) {
        query.addQueryItem(qsl("nonce"), nonce);
    }
    url.setQuery(query);
    return url;
}

// Single-use: create a fresh OAuthClientFlow for each sign-in attempt.
void OAuthClientFlow::start(const QUrl& discoveryUrl, const QString& clientId, const QStringList& scopes, bool includeNonce)
{
    mClientId = clientId;
    // The spec defaults the scopes to ["openid"] when the server sends none.
    mScopes = scopes.isEmpty() ? QStringList{qsl("openid")} : scopes;
    mNonce = includeNonce ? randomUrlSafeToken(16) : QString();

    if (!acceptableEndpointUrl(discoveryUrl)) {
        fail(qsl("discovery URL must be https (or http on loopback): %1").arg(discoveryUrl.toString()));
        return;
    }

    // One deadline covers the whole flow - discovery fetch, browser sign-in, and redirect - so an
    // abandoned attempt cannot leave the loopback listener open indefinitely.
    mTimeoutTimer.setSingleShot(true);
    connect(&mTimeoutTimer, &QTimer::timeout, this, [this]() {
        fail(qsl("timed out waiting for the browser sign-in to complete"));
    });
    mTimeoutTimer.start(std::chrono::minutes(5));

    QNetworkRequest request(discoveryUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = mNetworkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleDiscoveryReply(reply);
    });
}

void OAuthClientFlow::abort()
{
    if (mCompleted) {
        return;
    }
    mCompleted = true;
    cleanup();
    SecureStringUtils::secureStringClear(mCodeVerifier);
}

void OAuthClientFlow::handleDiscoveryReply(QNetworkReply* reply)
{
    reply->deleteLater();
    if (mCompleted) {
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        fail(qsl("could not fetch the OpenID discovery document: %1").arg(reply->errorString()));
        return;
    }

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        fail(qsl("the OpenID discovery document is not a JSON object: %1").arg(parseError.errorString()));
        return;
    }
    const QUrl authorizationEndpoint(doc.object()[qsl("authorization_endpoint")].toString());
    if (!acceptableEndpointUrl(authorizationEndpoint)) {
        fail(qsl("the discovery document has no usable https authorization_endpoint"));
        return;
    }

    if (!mRedirectServer.listen(QHostAddress::LocalHost, 0)) {
        fail(qsl("could not open a loopback port for the browser redirect: %1").arg(mRedirectServer.errorString()));
        return;
    }
    connect(&mRedirectServer, &QTcpServer::newConnection, this, &OAuthClientFlow::handleRedirectConnection);
    mRedirectUri = qsl("http://127.0.0.1:%1/").arg(mRedirectServer.serverPort());

    mCodeVerifier = generateCodeVerifier();
    mState = randomUrlSafeToken(16);
    const QUrl authorizationUrl = buildAuthorizationUrl(authorizationEndpoint, mClientId, mScopes, mRedirectUri, mState, codeChallengeS256(mCodeVerifier), mNonce);

    if (QDesktopServices::openUrl(authorizationUrl)) {
        emit browserOpened(authorizationUrl.toString());
    } else {
        // Keep the listener up: the user can still open the link by hand and complete the sign-in.
        emit browserOpenFailed(authorizationUrl.toString());
    }
}

void OAuthClientFlow::handleRedirectConnection()
{
    while (mRedirectServer.hasPendingConnections()) {
        QTcpSocket* socket = mRedirectServer.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            readRedirectRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QObject::destroyed, this, [this, socket]() {
            mRequestBuffers.remove(socket);
        });
    }
}

void OAuthClientFlow::readRedirectRequest(QTcpSocket* socket)
{
    QByteArray& buffer = mRequestBuffers[socket];
    buffer += socket->readAll();
    const int lineEnd = buffer.indexOf("\r\n");
    if (lineEnd < 0) {
        // A request line has no business being this long; drop the connection rather than buffer more.
        if (buffer.size() > 8192) {
            mRequestBuffers.remove(socket);
            socket->abort();
        }
        return;
    }
    const QByteArray requestLine = buffer.left(lineEnd);
    mRequestBuffers.remove(socket);

    if (mCompleted) {
        respond(socket, "404 Not Found", QString());
        return;
    }

    // "GET /path?query HTTP/1.1" - only the request target matters.
    const QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 2 || parts.at(0) != "GET") {
        respond(socket, "400 Bad Request", QString());
        return;
    }
    const QUrlQuery query{QUrl(QString::fromUtf8(parts.at(1)))};

    // Browsers also ask for things like /favicon.ico; only a request carrying the provider's
    // code or error is the redirect we are waiting for - answer anything else and keep waiting.
    if (!query.hasQueryItem(qsl("code")) && !query.hasQueryItem(qsl("error"))) {
        respond(socket, "404 Not Found", QString());
        return;
    }

    if (query.queryItemValue(qsl("state")) != mState) {
        //: Shown in the user's web browser when a browser sign-in attempt could not be verified as the one Mudlet started.
        respond(socket, "400 Bad Request", tr("This sign-in attempt could not be verified. Please return to Mudlet and try again."));
        fail(qsl("redirect state did not match the authorization request"));
        return;
    }

    if (query.hasQueryItem(qsl("error"))) {
        //: Shown in the user's web browser when the identity provider reported that the sign-in did not complete.
        respond(socket, "200 OK", tr("The sign-in was not completed. You can close this tab and return to Mudlet."));
        fail(qsl("the provider returned an error: %1").arg(query.queryItemValue(qsl("error"))));
        return;
    }

    const QString code = query.queryItemValue(qsl("code"), QUrl::FullyDecoded);
    //: Shown in the user's web browser after a successful browser sign-in.
    respond(socket, "200 OK", tr("You are signed in. You can close this tab and return to Mudlet."));

    mCompleted = true;
    cleanup();
    emit authorizationCaptured(code, mCodeVerifier, mRedirectUri);
    // The receiver has taken (and is responsible for scrubbing) its own copy; drop ours now.
    SecureStringUtils::secureStringClear(mCodeVerifier);
}

void OAuthClientFlow::respond(QTcpSocket* socket, const QByteArray& status, const QString& body)
{
    // Fixed, non-reflected content only: nothing from the request may be echoed into the page.
    const QByteArray content =
            body.isEmpty() ? QByteArray() : qsl("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Mudlet</title></head><body><p>%1</p></body></html>").arg(body.toHtmlEscaped()).toUtf8();
    QByteArray response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    socket->write(response);
    socket->disconnectFromHost();
}

void OAuthClientFlow::fail(const QString& logDetail)
{
    if (mCompleted) {
        return;
    }
    mCompleted = true;
    cleanup();
    SecureStringUtils::secureStringClear(mCodeVerifier);
    emit flowFailed(logDetail);
}

void OAuthClientFlow::cleanup()
{
    mTimeoutTimer.stop();
    if (mRedirectServer.isListening()) {
        mRedirectServer.close();
    }
    mRequestBuffers.clear();
}
