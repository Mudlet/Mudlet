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

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>

#include <chrono>
#include <functional>

class OAuthClientFlow;


class GMCPAuthenticator
{
    Q_DECLARE_TR_FUNCTIONS(GMCPAuthenticator)

public:
    explicit GMCPAuthenticator(Host* pHost);
    ~GMCPAuthenticator() = default;

    void saveSupportsSet(const QString& packageMessage, const QString& data);
    // Sends Char.Login.Credentials. With interactiveHandoff true it always sends the "run your own
    // sign-in screen" hand-off even when the profile has stored credentials; otherwise it autofills the
    // stored character name and password when the game accepts them. A version 2 hand-off is the
    // message with no account rather than a literally empty object - it carries the common fields (see
    // addCommonFields) - while a version 1 one stays {}.
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
    // The single place where a sign-in web address may reach the system browser, for both the
    // server-driven (Char.Login.URL) and the client-driven flow. Auto-opens only against evidence that
    // the player wants to sign in and consumes it, otherwise offers the address as a link.
    // answersTheGamesSignInOffer marks an address reached from Char.Login.Default, where connecting is
    // itself that evidence - once per connection.
    void offerOrOpenSignInUrl(const QUrl& url, const QString& provider, bool answersTheGamesSignInOffer);
    bool openSignInUrl(const QUrl& url, const QString& provider);
    void startClientDrivenOAuth();
    void cancelClientDrivenOAuth();
    void announceBrowserHandoff(const QString& provider);
    void sendAuthCode(QString code, QString codeVerifier, const QString& redirectUri, QString nonce);
    void selectAuthMethod();
    void scheduleSignInAttempt();
    void attemptReconnect();
    // A profile's saved sign-in, assembled from both credential keys. token is empty when there is
    // none to replay; it is a bearer secret, so whoever receives one scrubs it on every path.
    struct StoredSignIn
    {
        QString account;
        QString provider;
        // Strict unless the store says otherwise, so an entry whose requirement is missing or
        // unreadable never widens a stored token's exposure.
        bool secureOnly = true;
        QString token;
    };
    // Reads the stored sign-in as one entry, following the token wherever it lives: inline in the
    // metadata for an entry written before the split, otherwise under its own key. Reports the auth
    // attempt current when the read began rather than acting on it, because the two callers want
    // different things from a stale result - one drops it, the other still has to decide whether it
    // may rewrite the store. The callback is never invoked at all if the Host goes away while a read is
    // in flight.
    void readStoredSignInEntry(std::function<void(bool success, StoredSignIn entry, unsigned int attemptGeneration)> callback);
    // Reads the stored sign-in entry ({account, provider?, token?}) and acts on it: replay the token
    // (when allowToken), else send the resume form for a remembered provider, else fall through to
    // selectAuthMethod(). allowToken is false on the connection straight after a rejection, so a
    // not-yet-rewritten entry cannot loop us back into another rejected reconnect.
    void readStoredSignIn(bool allowToken);
    // Returns false when it refused to send: the token is a bearer secret, and one the issuing server
    // scoped to an encrypted transport (secureOnly) never goes out in the clear. It is scrubbed either
    // way, and only a true return means a result is awaited.
    bool sendReconnect(const QString& account, QString token, bool secureOnly);
    // Sends the resume form of Char.Login.Credentials: an account and provider plus the common fields, no
    // password - asking the game to restart the browser sign-in for the provider remembered from an
    // earlier Char.Login.URL. The absence of a password (not the presence of provider) distinguishes it.
    void sendResume(const QString& account, const QString& provider);
    void handleAuthToken(const QString& packageMessage, const QString& data);
    // secureOnly is the token's transport requirement, stored with it because it belongs to the
    // connection that minted the token rather than to whichever one later replays it.
    void storeReconnectToken(const QString& account, QString token, bool secureOnly);
    // After a rejected reconnect: re-reads the store first - another Mudlet instance sharing this
    // profile's keychain may have rotated the (single-use) token, in which case the fresh token is
    // replayed once instead of destroyed. Only a genuinely dead token is dropped, keeping the
    // account+provider resume hint so the next sign-in needs no provider menu.
    void retryOrDropRejectedToken();
    // Takes the account and provider explicitly: the caller captures them before its keychain read, so
    // a Char.Login.Default arriving mid-read cannot clear mConn and turn this into a full discard.
    void dropTokenKeepResumeHint(const QString& account, const QString& provider);
    // Rewrites the stored entry as {account, provider} with no token: enough to resume later, nothing
    // any longer a bearer secret.
    void storeResumeHint(const QString& account, const QString& provider);
    void discardReconnectToken(std::function<void(bool success)> callback = {});
    void resetPerConnectionState();
    // Per socket connection, unlike resetPerConnectionState() which runs per Char.Login.Default.
    void resetForNewConnection();

    // Adds the two fields every client->server Char.Login message may carry: the negotiated version we
    // are acting on, and token_storage - whether a reconnect token minted on this connection would
    // actually be kept.
    void addCommonFields(QJsonObject& payload) const;

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
    // messages (Credentials, Reconnect, resume, AuthCode) so both ends agree on the version even though
    // base GMCP negotiation is one-directional. The one exception is the hand-off on a version 1
    // exchange, which stays the bare Char.Login.Credentials {} that a version 1 server may be testing
    // for literally - version 2 defined the hand-off as the message with no account instead, so from
    // there on it carries the common fields like any other.
    int mNegotiatedVersion = 1;

    // Sign-in/token state for a single sign-in attempt, reset as one unit on every Char.Login.Default
    // (see resetPerConnectionState), so a field added here cannot be forgotten. The persistent
    // mReconnectRejected latch and mAuthAttemptGeneration counter deliberately sit outside the struct;
    // the capability fields above (mSupportedAuthTypes, mOAuth*, mNegotiatedVersion) reset separately in
    // saveSupportsSet, keyed to the advertisement rather than the sign-in attempt.
    struct PerConnectionState
    {
        // True while awaiting the Char.Login.Result that answers a Char.Login.Reconnect, so a failure
        // can recover (rotation retry, or drop-and-resume) instead of aborting the login.
        bool awaitingReconnectResult = false;
        // True on a connection that logged in by replaying a saved token, so a Char.Login.Token arriving
        // afterwards is a silent rotation rather than a first-time save worth announcing to the user.
        bool reconnectingWithToken = false;
        // One-shot guard: at most one rotated-token retry per connection, so two instances sharing a
        // store cannot ping-pong retries indefinitely.
        bool retriedRotatedToken = false;
        // The provider this profile's account signs in with, learned from the stored sign-in entry or
        // from the provider field of a Char.Login.URL this connection. Persisted alongside the token so
        // a later connection can resume the same provider's browser sign-in without a provider menu.
        QString accountProvider;
        // The account whose token this connection replayed, kept so a rejection can rewrite the stored
        // entry into a resume hint for that same account.
        QString reconnectAccount;
        // SHA-256 of the token this connection replayed. On rejection the store is re-read and compared
        // against this, so a token rotated by another running instance (shared keychain) is replayed
        // rather than destroyed. Only the hash is held - never the token itself.
        QByteArray sentReconnectTokenHash;
    };
    PerConnectionState mConn;

    // Set when a reconnect token is rejected, before the keychain read that decides what to do about it.
    // Deliberately NOT part of mConn: attemptReconnect() consumes it on the next Char.Login.Default, so it
    // must survive the per-connection reset that Default performs. The saved token is cleared
    // asynchronously, so this makes the next attempt skip a token replay rather than racing the keychain
    // rewrite and looping back into another rejected reconnect. That next Default usually arrives on the
    // connection we reconnect to, but a server is also permitted to re-offer one on this connection
    // instead, and Char.Login 2 forbids replaying a token rejected on it - hence latching synchronously at
    // the rejection rather than when the read returns.
    //
    // Consumed in one place (attemptReconnect()) but cleared or re-armed in two others, so audit all three
    // together: retryOrDropRejectedToken() clears it when it replays a live rotated token, and re-arms it
    // when a superseded recovery leaves the rejected token stored - by then the superseding Default has
    // already consumed the latch, so without re-arming the Default after that could replay the dead token.
    bool mReconnectRejected = false;
    // Incremented on every per-connection auth reset - each Char.Login.Default, and each socket connect
    // or disconnect. The asynchronous reconnect-token keychain read captures the value current when it
    // started and re-checks it in its callback, so a result arriving after a newer connection began is
    // discarded instead of driving a sign-in on the wrong attempt. Only work that would *act* on the
    // connection is gated this way: the token save's announcement deliberately is not, since it reports
    // what became of the stored token rather than driving anything. Not part of mConn: it must
    // monotonically increase, never reset.
    unsigned int mAuthAttemptGeneration = 0;
    // The sign-in attempt whose successful token save has already been announced, so the "you'll be
    // signed in automatically next time" notice is shown at most once per attempt, on the first token
    // that attempt actually persists.
    //
    // Deliberately NOT part of mConn, unlike the other one-shot guards there: the announcement is made
    // from the asynchronous save callback, so a save resolving after a newer Char.Login.Default would
    // otherwise consume - or be suppressed by - a flag belonging to the attempt that replaced it.
    // Stamping it with the generation keeps each attempt's announcement its own. Zero matches no
    // attempt, since the constructor's reset makes the first generation 1.
    unsigned int mAnnouncedSaveForAttempt = 0;

    // A server can pack thousands of Char.Login.Default frames into one packet and every sign-in
    // attempt reads the credential store. Throttling bounds that cost by wall clock rather than by how
    // much the server sent, and unlike a hard per-connection cap never refuses a legitimate re-offer.
    inline static constexpr std::chrono::milliseconds scmSignInAttemptInterval = std::chrono::seconds(1);
    QElapsedTimer mLastSignInAttempt;
    bool mSignInAttemptPending = false;
    // Bumped whenever a connection begins or ends, so a deferred attempt from a previous one is dropped.
    unsigned int mSignInScheduleGeneration = 0;
    // One automatic browser hand-off per connection for an address reached from the game's sign-in
    // offer, so a server cannot turn a burst of frames into a burst of tabs.
    bool mUnpromptedBrowserOpenAvailable = true;
};

#endif // MUDLET_AUTHENTICATOR_H
