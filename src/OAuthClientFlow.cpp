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

#include "utils.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
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
} // namespace

OAuthClientFlow::OAuthClientFlow(QObject* parent)
: QObject(parent)
{
}

OAuthClientFlow::~OAuthClientFlow() = default;

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

void OAuthClientFlow::start(const QUrl& discoveryUrl, const QString& clientId, const QStringList& scopes, bool includeNonce)
{
    Q_UNUSED(discoveryUrl)
    Q_UNUSED(clientId)
    Q_UNUSED(scopes)
    Q_UNUSED(includeNonce)
}

void OAuthClientFlow::abort() {}

void OAuthClientFlow::handleDiscoveryReply(QNetworkReply* reply)
{
    Q_UNUSED(reply)
}

void OAuthClientFlow::handleRedirectConnection() {}

void OAuthClientFlow::readRedirectRequest(QTcpSocket* socket)
{
    Q_UNUSED(socket)
}

void OAuthClientFlow::respond(QTcpSocket* socket, const QByteArray& status, const QString& body)
{
    Q_UNUSED(socket)
    Q_UNUSED(status)
    Q_UNUSED(body)
}

void OAuthClientFlow::fail(const QString& logDetail)
{
    Q_UNUSED(logDetail)
}

void OAuthClientFlow::cleanup() {}
