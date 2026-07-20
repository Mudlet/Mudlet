#ifndef MUDLET_OAUTHCLIENTFLOW_H
#define MUDLET_OAUTHCLIENTFLOW_H

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

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTimer>
#include <QUrl>

class QNetworkReply;
class QTcpSocket;

// Runs the client-driven GMCP Char.Login v2 OAuth flow: fetches the server's OpenID Connect
// discovery document, opens the provider's authorization URL in the system browser with a PKCE
// (S256) challenge, and captures the authorization code on a loopback (RFC 8252) redirect
// listener. The token exchange itself stays on the game server - this class only produces the
// {code, code_verifier, redirect_uri} triple that GMCPAuthenticator sends as Char.Login.AuthCode.
class OAuthClientFlow : public QObject
{
    Q_OBJECT

public:
    explicit OAuthClientFlow(QObject* parent = nullptr);
    ~OAuthClientFlow() override;

    void start(const QUrl& discoveryUrl, const QString& clientId, const QStringList& scopes, bool includeNonce);
    void abort();

    static QString generateCodeVerifier();
    static QString codeChallengeS256(const QString& codeVerifier);
    static QUrl buildAuthorizationUrl(const QUrl& authorizationEndpoint,
                                      const QString& clientId,
                                      const QStringList& scopes,
                                      const QString& redirectUri,
                                      const QString& state,
                                      const QString& codeChallenge,
                                      const QString& nonce);

signals:
    void authorizationCaptured(const QString& code, const QString& codeVerifier, const QString& redirectUri);
    void browserOpened(const QString& url);
    void browserOpenFailed(const QString& url);
    void flowFailed(const QString& logDetail);

private:
    void handleDiscoveryReply(QNetworkReply* reply);
    void handleRedirectConnection();
    void readRedirectRequest(QTcpSocket* socket);
    void respond(QTcpSocket* socket, const QByteArray& status, const QString& body);
    void fail(const QString& logDetail);
    void cleanup();

    QNetworkAccessManager mNetworkManager;
    QTcpServer mRedirectServer;
    QTimer mTimeoutTimer;
    QHash<QTcpSocket*, QByteArray> mRequestBuffers;
    QString mClientId;
    QStringList mScopes;
    QString mCodeVerifier;
    QString mState;
    QString mNonce;
    QString mRedirectUri;
    // Latches once the flow has reached a terminal outcome (code captured, failed, or aborted) so a
    // late redirect request or the shared timeout cannot produce a second, contradictory signal.
    bool mCompleted = false;
    // Set by start() to enforce this class's single-use contract: a second start() is refused.
    bool mStarted = false;
};

#endif // MUDLET_OAUTHCLIENTFLOW_H
