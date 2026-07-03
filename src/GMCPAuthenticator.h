#ifndef MUDLET_AUTHENTICATOR_H
#define MUDLET_AUTHENTICATOR_H

/***************************************************************************
 *   Copyright (C) 2024 by Vadim Peretokin - vperetokin@gmail.com          *
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

#include "Host.h"
#include "utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>

class OAuthClientFlow;


class GMCPAuthenticator
{
    Q_DECLARE_TR_FUNCTIONS(GMCPAuthenticator)

public:
    explicit GMCPAuthenticator(Host* pHost);
    ~GMCPAuthenticator() = default;

    void saveSupportsSet(const QString& packageMessage, const QString& data);
    // Sends Char.Login.Credentials. With interactiveHandoff true it always sends the empty object {}
    // (the "run your own sign-in screen" hand-off) even when the profile has stored credentials;
    // otherwise it autofills the stored character name and password when the game accepts them.
    void sendCredentials(bool interactiveHandoff = false);
    void handleAuthResult(const QString& packageMessage, const QString& data);
    void handleAuthGMCP(const QString& packageMessage, const QString& data);
    // Clears any stored password-less reconnect token for this profile, so the next connection signs in
    // afresh. Invoked from the profile preferences "Forget saved sign-in" control.
    void forgetSavedSignIn();

private:
    void handleAuthUrl(const QString& packageMessage, const QString& data);
    void openSignInUrl(const QUrl& url, const QString& provider);
    void startClientDrivenOAuth();
    void cancelClientDrivenOAuth();
    void announceBrowserHandoff(const QString& provider);
    void sendAuthCode(const QString& code, QString codeVerifier, const QString& redirectUri);
    void selectAuthMethod();
    void attemptReconnect();
    void sendReconnect(const QString& account, QString token);
    void handleAuthToken(const QString& packageMessage, const QString& data);
    void storeReconnectToken(const QString& account, QString token);
    void discardReconnectToken();
    void resetPerConnectionState();

    bool clientDrivenOAuthAvailable() const;

    Host* mpHost;
    QStringList mSupportedAuthTypes;
    // Version 2 client-driven OAuth capability, advertised by a server that is itself an OpenID
    // Provider. Only populated when the connection is encrypted: the flow's completing
    // Char.Login.AuthCode message must never travel in the clear, so on plain telnet these stay
    // empty and the sign-in transparently uses the server-driven flow instead.
    QString mOAuthDiscoveryUrl;
    QString mOAuthClientId;
    QStringList mOAuthScopes;
    bool mOAuthNonceRequired = false;
    // The in-flight client-driven OAuth flow, if any. Parented to the Host so it cannot outlive the
    // profile; guarded so a new Char.Login.Default aborts a stale attempt before starting over.
    QPointer<OAuthClientFlow> mpOAuthFlow;
    // The negotiated Char.Login protocol version the server reported in Char.Login.Default. Absent (a
    // version 1 server or legacy exchange) is treated as 1; we echo this back on our client->server
    // messages so both ends agree on the version even though base GMCP negotiation is one-directional.
    int mNegotiatedVersion = 1;
    // True while we are waiting for the Char.Login.Result that answers a Char.Login.Reconnect attempt,
    // so a failure can discard the stale token and fall back instead of aborting the login.
    bool mAwaitingReconnectResult = false;
    // True on a connection that logged in by replaying a saved token, so a Char.Login.Token arriving
    // afterwards is a silent rotation rather than a first-time save worth announcing to the user.
    bool mReconnectingWithToken = false;
    // One-shot guard so the "you'll be signed in automatically next time" notice is shown at most once
    // per connection, on the first token we persist.
    bool mAnnouncedTokenSaveThisConnection = false;
    // Set when a reconnect token is rejected and we reconnect for a clean sign-in. The saved token is
    // cleared asynchronously, so this makes the next connection skip it explicitly rather than racing
    // the keychain removal and looping back into another rejected reconnect.
    bool mReconnectRejected = false;
};

#endif // MUDLET_AUTHENTICATOR_H
