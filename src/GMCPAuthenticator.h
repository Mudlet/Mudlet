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

#include <functional>

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
    // afresh. Invoked from the profile preferences "Forget saved sign-in" control. The optional callback
    // reports whether the (asynchronous) keychain removal actually succeeded, so callers do not report
    // success before the token is gone.
    void forgetSavedSignIn(std::function<void(bool success)> callback = {});

private:
    void handleAuthUrl(const QString& packageMessage, const QString& data);
    void openSignInUrl(const QUrl& url, const QString& provider);
    void startClientDrivenOAuth();
    void cancelClientDrivenOAuth();
    void announceBrowserHandoff(const QString& provider);
    void sendAuthCode(QString code, QString codeVerifier, const QString& redirectUri);
    void selectAuthMethod();
    void attemptReconnect();
    // Reads the stored sign-in entry ({account, provider?, token?}) and acts on it: replay the token
    // (when allowToken), else send the resume form for a remembered provider, else fall through to
    // selectAuthMethod(). allowToken is false on the connection straight after a rejection, so a
    // not-yet-rewritten entry cannot loop us back into another rejected reconnect.
    void readStoredSignIn(bool allowToken);
    void sendReconnect(const QString& account, QString token);
    // Sends the resume form of Char.Login.Credentials: {account, provider}, no password - asking the
    // game to restart the browser sign-in for the provider remembered from an earlier Char.Login.URL.
    void sendResume(const QString& account, const QString& provider);
    void handleAuthToken(const QString& packageMessage, const QString& data);
    void storeReconnectToken(const QString& account, QString token);
    // After a rejected reconnect: re-reads the store first - another Mudlet instance sharing this
    // profile's keychain may have rotated the (single-use) token, in which case the fresh token is
    // replayed once instead of destroyed. Only a genuinely dead token is dropped, keeping the
    // account+provider resume hint so the next sign-in needs no provider menu.
    void retryOrDropRejectedToken();
    void dropTokenKeepResumeHint();
    // Rewrites the stored entry as {account, provider} with no token: enough to resume later, nothing
    // any longer a bearer secret.
    void storeResumeHint(const QString& account, const QString& provider);
    void discardReconnectToken(std::function<void(bool success)> callback = {});
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
    // Incremented on every per-connection auth reset (each Char.Login.Default). The asynchronous
    // reconnect-token keychain read captures the value current when it started and re-checks it in its
    // callback, so a result arriving after a newer connection began is discarded instead of driving a
    // sign-in on the wrong attempt.
    unsigned int mAuthAttemptGeneration = 0;
    // The provider this profile's account signs in with, learned from the stored sign-in entry or from
    // the provider field of a Char.Login.URL this connection. Persisted alongside the token so a later
    // connection can resume the same provider's browser sign-in without a provider menu.
    QString mAccountProvider;
    // The account whose token this connection replayed, kept so a rejection can rewrite the stored
    // entry into a resume hint for that same account.
    QString mReconnectAccount;
    // SHA-256 of the token this connection replayed. On rejection the store is re-read and compared
    // against this, so a token rotated by another running instance (shared keychain) is replayed rather
    // than destroyed. Only the hash is held - never the token itself.
    QByteArray mSentReconnectTokenHash;
    // One-shot guard: at most one rotated-token retry per connection, so two instances sharing a store
    // cannot ping-pong retries indefinitely.
    bool mRetriedRotatedToken = false;
};

#endif // MUDLET_AUTHENTICATOR_H
