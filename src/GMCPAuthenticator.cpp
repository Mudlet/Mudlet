/***************************************************************************
 *   Copyright (C) 2024 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "GMCPAuthenticator.h"

#include "Host.h"
#include "CredentialManager.h"
#include "OAuthClientFlow.h"
#include "SecureStringUtils.h"
#include "ctelnet.h"
#include "mudlet.h"
#include <QAccessible>
#include <QDebug>
#include <QDesktopServices>
#include <QHash>
#include <QPointer>
#include <QTimer>
#include <QUrl>

namespace {
// Human-readable labels for the provider ids that arrive as lowercase strings on the wire, so the
// browser-handoff messages read naturally ("...to sign in with GitHub").
QString providerDisplayName(const QString& id)
{
    static const QHash<QString, QString> brandNames{
            {qsl("apple"), qsl("Apple")},
            {qsl("discord"), qsl("Discord")},
            {qsl("github"), qsl("GitHub")},
            {qsl("google"), qsl("Google")},
            {qsl("microsoft"), qsl("Microsoft")},
            {qsl("slack"), qsl("Slack")},
            {qsl("twitch"), qsl("Twitch")},
            {qsl("x"), qsl("X")},
    };
    QString display = brandNames.value(id.toLower());
    if (display.isEmpty() && !id.isEmpty()) {
        display = id;
        display[0] = display[0].toUpper();
    }
    return display;
}
} // namespace

GMCPAuthenticator::GMCPAuthenticator(Host* pHost)
: mpHost(pHost)
{
    resetPerConnectionState();
}

void GMCPAuthenticator::resetPerConnectionState()
{
    // mReconnectRejected is deliberately not reset here: it is a one-shot latch consumed by
    // attemptReconnect() on the very next connection, so it must survive the reconnect it triggers.
    mAwaitingReconnectResult = false;
    mReconnectingWithToken = false;
    mAnnouncedTokenSaveThisConnection = false;
    // Start a new auth attempt so any in-flight reconnect-token keychain read from a previous connection
    // is recognised as stale by its callback and ignored.
    ++mAuthAttemptGeneration;
    // A fresh sign-in supersedes any browser dance still in flight from the previous connection.
    cancelClientDrivenOAuth();
}

void GMCPAuthenticator::saveSupportsSet(const QString& packageMessage, const QString& data)
{
    // Clear the cached capabilities up front so a malformed frame (an early return below) cannot leave
    // stale provider/auth lists from a previous connection in place - later reconnect/auth decisions must
    // act only on the current input, never on outdated capabilities.
    mSupportedAuthTypes.clear();
    mOAuthDiscoveryUrl.clear();
    mOAuthClientId.clear();
    mOAuthScopes.clear();
    mOAuthNonceRequired = false;
    // Reset to the version 1 default; a server that speaks version 2 or higher reports the negotiated
    // version below, and its absence (a version 1 server) leaves us correctly acting as version 1.
    mNegotiatedVersion = 1;

    QJsonParseError parseError;
    auto jsonDoc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString() << " at offset " << parseError.offset << ". Received data: \"" << data
                                       << "\"";
        return;
    }
    if (!jsonDoc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got " << (jsonDoc.isArray() ? "array" : jsonDoc.isNull() ? "null" : "unknown type") << ".";
        return;
    }
    auto jsonObj = jsonDoc.object();

    // The server reports the negotiated Char.Login version here; treat a missing, non-numeric, or
    // out-of-range value as version 1 (per the spec, the version is a positive, non-zero integer).
    if (jsonObj.contains("version")) {
        const int reportedVersion = jsonObj["version"].toInt(1);
        // Clamp to the highest version this client implements: the negotiated version is
        // min(client, server), so we never act on - or echo back - a version we do not understand.
        mNegotiatedVersion = qBound(1, reportedVersion, 2);
    }

    if (jsonObj.contains("type")) {
        QJsonArray typesArray = jsonObj["type"].toArray();
        for (const auto& type : std::as_const(typesArray)) {
            // A non-string or empty entry yields an empty QString; drop it so it cannot masquerade as a
            // real method later (an all-empty list would otherwise slip past our isEmpty() guards).
            const QString typeName = type.toString();
            if (typeName.isEmpty()) {
                qWarning().noquote().nospace() << "GMCP " << packageMessage << " - ignoring a malformed (non-string or empty) auth type entry.";
                continue;
            }
            mSupportedAuthTypes.append(typeName);
        }
    }

    // The optional client-driven OAuth fields (the server is itself an OpenID Provider). Per the spec
    // these are only meaningful over an encrypted transport - the flow ends in a Char.Login.AuthCode
    // that carries the authorization code and PKCE verifier together, which must never travel in the
    // clear - so on a cleartext connection ignore them even if a non-conformant server sends them,
    // which transparently steers the sign-in to the server-driven flow.
    if (mpHost->mTelnet.currentlySecure()) {
        mOAuthDiscoveryUrl = jsonObj["location"].toString();
        mOAuthClientId = jsonObj["client_id"].toString();
        if (jsonObj.contains("scopes")) {
            const QJsonArray scopesArray = jsonObj["scopes"].toArray();
            for (const auto& scope : std::as_const(scopesArray)) {
                const QString scopeName = scope.toString();
                if (scopeName.isEmpty()) {
                    qWarning().noquote().nospace() << "GMCP " << packageMessage << " - ignoring a malformed (non-string or empty) OAuth scope entry.";
                    continue;
                }
                mOAuthScopes.append(scopeName);
            }
        }
        mOAuthNonceRequired = jsonObj["nonce"].toBool();
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
#endif
}

bool GMCPAuthenticator::clientDrivenOAuthAvailable() const
{
    // Both fields are only ever stored when the connection is encrypted (see saveSupportsSet), so
    // their presence also means the transport is acceptable for Char.Login.AuthCode.
    return mSupportedAuthTypes.contains(qsl("oauth")) && !mOAuthDiscoveryUrl.isEmpty() && !mOAuthClientId.isEmpty();
}

void GMCPAuthenticator::sendCredentials(bool interactiveHandoff)
{
    auto character = mpHost->getLogin();
    auto password = mpHost->getPass();

    QJsonObject credentials;

    // Autofill stored credentials only when this is not an explicit interactive hand-off and the game
    // actually accepts password-credentials; otherwise this stays an empty object, the deliberate
    // Char.Login.Credentials {} hand-off telling the game to run its own sign-in screen (see
    // selectAuthMethod).
    if (!interactiveHandoff && mSupportedAuthTypes.contains(qsl("password-credentials")) && !character.isEmpty() && !password.isEmpty()) {
        credentials["account"] = character;
        credentials["password"] = password;
        // Echo the negotiated version so the server can confirm both ends agree.
        credentials["version"] = mNegotiatedVersion;
    }

    QJsonDocument doc(credentials);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    QString gmcpMessage = QString::fromUtf8(json);

    // Clear sensitive data from memory as soon as possible
    credentials = QJsonObject();                    // Clear JSON object
    doc = QJsonDocument();                          // Clear document
    SecureStringUtils::secureStringClear(password); // Clear password copy
    SecureStringUtils::secureByteArrayClear(json);  // Clear the plaintext JSON bytes toJson() produced

    // Keep the plaintext and its telnet-cooked form in named buffers so both can be wiped; passing the
    // toStdString()/encodeAndCookBytes() temporaries inline would leave un-scrubbed copies behind.
    std::string plaintext = gmcpMessage.toStdString();
    std::string encoded = mpHost->mTelnet.encodeAndCookBytes(plaintext);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.Credentials ";
    output += encoded;
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

    // Scrub every copy of the secret: the JSON message, the plaintext and encoded payloads, and the
    // assembled telnet frame, which also holds the (encoded) password.
    SecureStringUtils::secureStringClear(gmcpMessage);
    SecureStringUtils::secureStdStringClear(plaintext);
    SecureStringUtils::secureStdStringClear(encoded);
    SecureStringUtils::secureStdStringClear(output);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP credentials";
#endif
}

void GMCPAuthenticator::sendReconnect(const QString& account, QString token)
{
    QJsonObject payload;
    payload[qsl("account")] = account;
    payload[qsl("token")] = token;
    // Echo the negotiated version so the server can confirm both ends agree.
    payload[qsl("version")] = mNegotiatedVersion;
    QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QString gmcpMessage = QString::fromUtf8(json);
    payload = QJsonObject();
    SecureStringUtils::secureByteArrayClear(json);

    // Keep the plaintext and its telnet-cooked form in named buffers so both can be wiped; the
    // toStdString()/encodeAndCookBytes() temporaries would otherwise leave un-scrubbed token copies.
    std::string plaintext = gmcpMessage.toStdString();
    std::string encoded = mpHost->mTelnet.encodeAndCookBytes(plaintext);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.Reconnect ";
    output += encoded;
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

    // The reconnect token is a bearer secret, so scrub every copy once it is sent: the token argument
    // (taken by value and moved in from the caller, so we own the sole copy), the JSON message, the
    // plaintext and encoded payloads, and the assembled telnet frame, which also holds the (encoded) token.
    SecureStringUtils::secureStringClear(token);
    SecureStringUtils::secureStringClear(gmcpMessage);
    SecureStringUtils::secureStdStringClear(plaintext);
    SecureStringUtils::secureStdStringClear(encoded);
    SecureStringUtils::secureStdStringClear(output);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP reconnect for account:" << account;
#endif
}

void GMCPAuthenticator::storeReconnectToken(const QString& account, QString token)
{
    QJsonObject obj;
    obj[qsl("account")] = account;
    obj[qsl("token")] = token;
    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QString payload = QString::fromUtf8(json);
    // Release the JSON object and the plaintext bytes that also hold the token.
    obj = QJsonObject();
    SecureStringUtils::secureByteArrayClear(json);

    // One reconnect token per profile: the fixed "reconnect" key means signing a profile into a second
    // account overwrites the first account's token. A stale-account token is simply rejected on the next
    // connect and falls back to a fresh sign-in, so this is safe under the one-account-per-profile model.
    QPointer<Host> safeHost = mpHost;
    QPointer<CredentialManager> credentialManager = new CredentialManager();
    credentialManager->storePassword(mpHost->getName(), qsl("reconnect"), payload, [safeHost, credentialManager](bool success, const QString& errorMessage) {
        if (!success) {
            qWarning().noquote() << "GMCP Char.Login.Token - failed to store reconnect token:" << errorMessage;
            if (safeHost) {
                //: Shown when the user opted to stay signed in but saving the sign-in token failed, so they will have to sign in again next time.
                safeHost->postMessage(tr("[ WARN ]  - Could not save your sign-in for next time; you may need to sign in again."));
            }
        }
        if (credentialManager) {
            credentialManager->deleteLater();
        }
    });

    // The token is a bearer secret; storePassword has already taken its own copy (inside payload), so
    // scrub both our serialized payload and the caller-owned token argument we took by value.
    SecureStringUtils::secureStringClear(payload);
    SecureStringUtils::secureStringClear(token);
}

void GMCPAuthenticator::forgetSavedSignIn(std::function<void(bool success)> callback)
{
    discardReconnectToken(std::move(callback));
}

void GMCPAuthenticator::discardReconnectToken(std::function<void(bool success)> callback)
{
    QPointer<CredentialManager> credentialManager = new CredentialManager();
    credentialManager->removePassword(mpHost->getName(), qsl("reconnect"), [credentialManager, callback = std::move(callback)](bool success, const QString& errorMessage) {
        // A failed removal leaves a now-invalid bearer token on disk, so make it visible rather than
        // swallowing it - the security intent of this flow is to not keep stale tokens around.
        if (!success) {
            qWarning().noquote() << "GMCP Char.Login - failed to remove stored reconnect token:" << errorMessage;
        }
        if (credentialManager) {
            credentialManager->deleteLater();
        }
        // Report the real outcome so callers (e.g. the preferences UI) only claim success once the token
        // is actually gone, rather than before the asynchronous removal has resolved.
        if (callback) {
            callback(success);
        }
    });
}

void GMCPAuthenticator::handleAuthUrl(const QString& packageMessage, const QString& data)
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString() << " at offset " << parseError.offset << ". Received data: \"" << data
                                       << "\"";
        return;
    }
    if (!doc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got " << (doc.isArray() ? "array" : doc.isNull() ? "null" : "unknown type") << ".";
        return;
    }

    const auto obj = doc.object();
    const auto url = obj[qsl("url")].toString();
    if (url.isEmpty()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Missing 'url' field.";
        return;
    }
    // Optional label so the browser-handoff message can name the provider ("...to sign in with Discord").
    const auto provider = obj[qsl("provider")].toString();

    // The URL arrives unauthenticated over the wire, so only ever hand http(s) links to the OS - never
    // arbitrary scheme handlers (file:, javascript:, etc.), whether we auto-open or offer a manual link.
    const QUrl parsedUrl(url);
    if (!parsedUrl.isValid() || (parsedUrl.scheme() != qsl("http") && parsedUrl.scheme() != qsl("https"))) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Refusing to open sign-in link with unsupported scheme: \"" << url << "\"";
        //: Shown when the game sends a sign-in link with an unsupported or invalid address (not an http/https web link).
        mpHost->postMessage(tr("[ WARN ]  - The game sent an invalid sign-in link; cannot continue."));
        return;
    }

    // This message may be pushed by the server (unsolicited) when the player reaches a browser step on
    // the game's own sign-in screen. Auto-opening a browser is only appropriate as a consequence of the
    // player's own action, so gate it on their having sent input this connection; otherwise offer the
    // link for them to open deliberately, so a misbehaving server cannot pop a browser at an idle player.
    if (mpHost->userSentInputThisConnection()) {
        openSignInUrl(parsedUrl, provider);
        return;
    }

    //: %1 is the sign-in web address the user should open in their browser to sign in.
    mpHost->postMessage(tr("[ INFO ]  - To sign in, open this link in your browser: %1").arg(url));
}

void GMCPAuthenticator::openSignInUrl(const QUrl& url, const QString& provider)
{
    if (!QDesktopServices::openUrl(url)) {
        //: %1 is the sign-in web address the user should open manually in their browser.
        mpHost->postMessage(tr("[ WARN ]  - Could not open your browser. Open this link manually to sign in: %1").arg(url.toString()));
        return;
    }
    announceBrowserHandoff(provider);
}

void GMCPAuthenticator::announceBrowserHandoff(const QString& provider)
{
    const QString display = providerDisplayName(provider);
    //: Shown after the user's browser is launched to complete an OAuth/web sign-in. %1 is the provider name (e.g. Discord).
    const QString message = display.isEmpty() ? tr("[ INFO ]  - Opening your browser to sign in. Complete the login there, then return here.")
                                              : tr("[ INFO ]  - Opening your browser to sign in with %1. Complete the login there, then return here.").arg(display);
    mpHost->postMessage(message);

    // The browser handoff happens with no focused control to announce it, so push an explicit
    // announcement to assistive technology (VoiceOver/NVDA/Orca) instead of relying on the console.
    if (auto* mainWindow = mudlet::self()) {
        QAccessibleAnnouncementEvent announcement(mainWindow, message);
        QAccessible::updateAccessibility(&announcement);
    }
}

void GMCPAuthenticator::startClientDrivenOAuth()
{
    cancelClientDrivenOAuth();

    // Parented to the Host so the flow (and its loopback listener) cannot outlive the profile.
    mpOAuthFlow = new OAuthClientFlow(mpHost);
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::authorizationCaptured, mpHost, [this](const QString& code, const QString& codeVerifier, const QString& redirectUri) {
        sendAuthCode(code, codeVerifier, redirectUri);
    });
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::browserOpened, mpHost, [this]() {
        announceBrowserHandoff(QString());
    });
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::browserOpenFailed, mpHost, [this](const QString& url) {
        //: %1 is the sign-in web address the user should open manually in their browser.
        mpHost->postMessage(tr("[ WARN ]  - Could not open your browser. Open this link manually to sign in: %1").arg(url));
    });
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::flowFailed, mpHost, [this](const QString& logDetail) {
        qWarning().noquote() << "GMCP Char.Login client-driven OAuth failed:" << logDetail;
        mpHost->mTelnet.setDontReconnect(true);
        //: Shown when a browser-based sign-in with the game's own account could not be completed.
        mpHost->postMessage(tr("[ WARN ]  - The browser sign-in could not be completed; reconnect to try again."));
    });

    mpOAuthFlow->start(QUrl(mOAuthDiscoveryUrl), mOAuthClientId, mOAuthScopes, mOAuthNonceRequired);
}

void GMCPAuthenticator::cancelClientDrivenOAuth()
{
    if (mpOAuthFlow) {
        mpOAuthFlow->abort();
        mpOAuthFlow->deleteLater();
        mpOAuthFlow = nullptr;
    }
}

void GMCPAuthenticator::sendAuthCode(QString code, QString codeVerifier, const QString& redirectUri)
{
    // The spec forbids Char.Login.AuthCode on a cleartext connection: the authorization code and PKCE
    // verifier together would let an eavesdropper redeem the code at the provider. The flow only starts
    // on an encrypted connection, but re-check at send time in case the transport changed underneath us.
    if (!mpHost->mTelnet.currentlySecure()) {
        SecureStringUtils::secureStringClear(code);
        SecureStringUtils::secureStringClear(codeVerifier);
        qWarning().noquote() << "GMCP Char.Login.AuthCode - refusing to send the authorization code over an unencrypted connection.";
        mpHost->mTelnet.setDontReconnect(true);
        // Tear down the in-flight flow and its loopback listener immediately: the sign-in is doomed, so
        // there is no reason to keep the temporary listener running.
        cancelClientDrivenOAuth();
        //: Shown when a browser sign-in finished but the game connection is not encrypted, so completing it would be unsafe.
        mpHost->postMessage(tr("[ WARN ]  - Cannot complete the sign-in because the connection is not encrypted."));
        return;
    }

    QJsonObject payload;
    payload[qsl("code")] = code;
    payload[qsl("code_verifier")] = codeVerifier;
    payload[qsl("redirect_uri")] = redirectUri;
    // Echo the negotiated version so the server can confirm both ends agree.
    payload[qsl("version")] = mNegotiatedVersion;
    QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QString gmcpMessage = QString::fromUtf8(json);
    payload = QJsonObject();
    SecureStringUtils::secureByteArrayClear(json);

    // Keep the plaintext and its telnet-cooked form in named buffers so both can be wiped; the
    // toStdString()/encodeAndCookBytes() temporaries would otherwise leave un-scrubbed copies behind.
    std::string plaintext = gmcpMessage.toStdString();
    std::string encoded = mpHost->mTelnet.encodeAndCookBytes(plaintext);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.AuthCode ";
    output += encoded;
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

    // The authorization code and PKCE verifier are single-use secrets; scrub every copy once sent: the
    // code and verifier arguments (both taken by value), the JSON message, the plaintext and encoded
    // payloads, and the assembled telnet frame.
    SecureStringUtils::secureStringClear(code);
    SecureStringUtils::secureStringClear(codeVerifier);
    SecureStringUtils::secureStringClear(gmcpMessage);
    SecureStringUtils::secureStdStringClear(plaintext);
    SecureStringUtils::secureStdStringClear(encoded);
    SecureStringUtils::secureStdStringClear(output);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP AuthCode to complete the client-driven OAuth sign-in";
#endif
}


void GMCPAuthenticator::handleAuthResult(const QString& packageMessage, const QString& data)
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString() << " at offset " << parseError.offset << ". Received data: \"" << data
                                       << "\"";
        return;
    }
    if (!doc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got " << (doc.isArray() ? "array" : doc.isNull() ? "null" : "unknown type") << ".";
        return;
    }
    auto obj = doc.object();

    // some game drivers can parse JSON for true or false, but may not be able to write booleans back
    auto result = obj[qsl("success")];
    bool success = (result.isBool() && result.toBool()) || (result.isString() && result.toString() == "true");
    auto message = obj[qsl("message")].toString();

    // A failed password-less reconnect is not a dead end: discard the stale token and fall back to the
    // normal sign-in (provider picker or credentials) instead of giving up on the connection.
    if (mAwaitingReconnectResult) {
        mAwaitingReconnectResult = false;
        if (success) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
            qDebug() << "GMCP reconnect successful";
#endif
            return;
        }
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP reconnect rejected, reconnecting for a fresh sign-in:" << message;
#endif
        // The saved token is no good. Servers commonly drop the connection right after rejecting a
        // reconnect (and Mudlet will not auto-reconnect from such a fast rejection), which would leave the
        // in-session sign-in picker writing into a dead socket - nothing happens when a provider is
        // chosen. Discard the token and reconnect so the full sign-in runs on a fresh, stable connection.
        // mReconnectRejected makes that next connection skip the now-asynchronously-clearing saved token
        // instead of racing it and looping straight back into another rejected reconnect.
        discardReconnectToken();
        mReconnectRejected = true;
        //: Shown when a saved password-less sign-in is no longer accepted; Mudlet reconnects so the user can sign in again.
        mpHost->postMessage(tr("[ INFO ]  - Your saved sign-in has expired; reconnecting so you can sign in again."));
        QPointer<Host> safeHost = mpHost;
        QTimer::singleShot(0, mpHost, [safeHost]() {
            if (safeHost) {
                safeHost->mTelnet.reconnect();
            }
        });
        return;
    }

    if (success) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP login successful";
#endif
    } else {
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP login failed:" << message;
#endif
        mpHost->mTelnet.setDontReconnect(true);
        if (message.isEmpty()) {
            mpHost->postMessage(tr("[ WARN ]  - Could not log in to the game, is the login information correct?"));
        } else {
            //: %1 shows the reason for failure, could be authentication, etc.
            mpHost->postMessage(tr("[ WARN ]  - Could not log in to the game: %1").arg(message));
        }
    }
}

// controller for GMCP authentication
void GMCPAuthenticator::handleAuthGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Char.Login.Default")) {
        saveSupportsSet(packageMessage, data);

        // A fresh Char.Login.Default starts a new sign-in; reset the per-connection token state before
        // deciding how to authenticate.
        resetPerConnectionState();

        attemptReconnect();
        return;
    }

    if (packageMessage == qsl("Char.Login.URL")) {
        handleAuthUrl(packageMessage, data);
        return;
    }

    if (packageMessage == qsl("Char.Login.Token")) {
        handleAuthToken(packageMessage, data);
        return;
    }

    if (packageMessage == qsl("Char.Login.Result")) {
        handleAuthResult(packageMessage, data);
        return;
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Unknown GMCP auth package:" << packageMessage;
#endif
}

void GMCPAuthenticator::handleAuthToken(const QString& packageMessage, const QString& data)
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        // The payload carries the reconnect token (a bearer secret), so never log its contents - report
        // only the parse error and a non-sensitive length summary.
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString() << " at offset " << parseError.offset << " (withholding "
                                       << data.length() << "-character payload as it may contain a token).";
        return;
    }
    if (!doc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got " << (doc.isArray() ? "array" : doc.isNull() ? "null" : "unknown type") << ".";
        return;
    }

    const auto obj = doc.object();
    const auto account = obj[qsl("account")].toString();
    auto token = obj[qsl("token")].toString();
    if (account.isEmpty() || token.isEmpty()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Missing 'account' or 'token' field.";
        return;
    }

    // The server issues this token at its own discretion - the "remember me" decision belongs to the
    // game's flow, not the client - so we simply persist whatever arrives (overwriting on rotation) and
    // offer a local "forget saved sign-in" control in preferences. Move the token in so
    // storeReconnectToken owns the sole copy and can scrub it after persisting.
    storeReconnectToken(account, std::move(token));

    // Let the player know their sign-in will be remembered - but only the first time this connection,
    // and never on a token-reconnect (where the arriving token is a silent rotation, not a new opt-in).
    if (!mReconnectingWithToken && !mAnnouncedTokenSaveThisConnection) {
        mAnnouncedTokenSaveThisConnection = true;
        //: Shown once after a browser/OAuth sign-in whose reconnect token was saved, so future connects need no sign-in.
        mpHost->postMessage(tr("[ INFO ]  - You'll be signed in automatically next time. Manage this under Preferences, Connection."));
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Stored GMCP reconnect token for account:" << account;
#endif
}

void GMCPAuthenticator::attemptReconnect()
{
    mpHost->mTelnet.cancelLoginTimers();

    // We reconnected after a rejected token (see handleAuthResult) to sign in cleanly. The stored token
    // is being removed asynchronously, so skip it explicitly this once rather than risk reading a
    // not-yet-removed token and looping straight back into another rejected reconnect.
    if (mReconnectRejected) {
        mReconnectRejected = false;
        selectAuthMethod();
        return;
    }

    // A complete stored character name and password is the player's explicit choice of sign-in method,
    // so it outranks a saved reconnect token: typed credentials name the exact character the player
    // wants to play, whereas the token names whatever account last signed in. selectAuthMethod() sends
    // them as its first rung.
    if (mSupportedAuthTypes.contains(qsl("password-credentials")) && !mpHost->getLogin().isEmpty() && !mpHost->getPass().isEmpty()) {
        selectAuthMethod();
        return;
    }

    // Reconnect tokens are part of the version 2 OAuth capability; if the server is not offering oauth
    // there is nothing to reconnect against, so go straight to the normal method selection.
    if (!mSupportedAuthTypes.contains(qsl("oauth"))) {
        selectAuthMethod();
        return;
    }

    QPointer<Host> safeHost = mpHost;
    QPointer<CredentialManager> credentialManager = new CredentialManager();
    // Capture the current auth attempt so a result arriving after a newer Char.Login.Default (which
    // bumps mAuthAttemptGeneration) is dropped rather than driving a sign-in on the wrong connection.
    const auto attemptGeneration = mAuthAttemptGeneration;
    credentialManager->retrievePassword(mpHost->getName(), qsl("reconnect"), [this, safeHost, credentialManager, attemptGeneration](bool success, const QString& value, const QString&) {
        if (credentialManager) {
            credentialManager->deleteLater();
        }
        // The Host (which owns this authenticator) may have gone away while the keychain read was in
        // flight; bail out without touching any members if so.
        if (!safeHost) {
            return;
        }
        // A newer connection started while this read was in flight; its result is stale, so ignore it
        // rather than send a reconnect or pick a method for the connection that superseded it.
        if (attemptGeneration != mAuthAttemptGeneration) {
            return;
        }

        if (success && !value.isEmpty()) {
            const auto doc = QJsonDocument::fromJson(value.toUtf8());
            if (doc.isObject()) {
                const auto account = doc.object()[qsl("account")].toString();
                auto token = doc.object()[qsl("token")].toString();
                if (!account.isEmpty() && !token.isEmpty()) {
                    // This connection is logging in by replaying a saved token, so a Char.Login.Token
                    // that comes back is a silent rotation rather than a first-time save to announce.
                    mReconnectingWithToken = true;
                    mAwaitingReconnectResult = true;
                    // Move the token in so sendReconnect owns the sole copy and can scrub it after sending.
                    sendReconnect(account, std::move(token));
                    return;
                }
            }
        }

        selectAuthMethod();
    });
}

void GMCPAuthenticator::selectAuthMethod()
{
    mpHost->mTelnet.cancelLoginTimers();

    // The game owns the interactive sign-in screen; this client automates around it rather than
    // rendering a pop-up of its own (see the "who owns the screen" design).
    const bool serverOffersPassword = mSupportedAuthTypes.contains(qsl("password-credentials"));
    const bool haveCredentials = !mpHost->getLogin().isEmpty() && !mpHost->getPass().isEmpty();

    // A complete stored character name and password is the player's choice of sign-in method - they
    // typed both into this profile deliberately - so autofill it whenever the game accepts
    // password-credentials. A partial pair cannot be sent and falls through to the rungs below.
    if (serverOffersPassword && haveCredentials) {
        sendCredentials();
        return;
    }

    // When the game is its own OpenID Provider over TLS, the client uniquely can run the client-driven
    // flow end to end (discovery, PKCE, loopback capture), so drive it directly: the browser opens for
    // the sign-in the player came to do, with no pop-up and no password to manage.
    if (clientDrivenOAuthAvailable()) {
        startClientDrivenOAuth();
        return;
    }

    // Otherwise hand off to the game's own interactive sign-in with an empty Char.Login.Credentials {}
    // and let the player choose there - even if the profile has a stored password (the game's screen
    // still accepts it), so a saved password never blocks reaching a provider choice.
    sendCredentials(true);
}
