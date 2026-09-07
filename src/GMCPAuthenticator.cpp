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
#include "UntrustedText.h"
#include "ctelnet.h"
#include "mudlet.h"
#include <QAccessible>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <chrono>
#include <optional>

using namespace std::chrono_literals;

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

// Decodes a field the standard declares boolean, in every form it permits: a JSON boolean, the
// strings "true"/"false" or "1"/"0" (case-insensitive, surrounding whitespace ignored), or the numbers
// 1/0. Several MUD drivers have no JSON boolean in their serializer at all - LDMud's json_serialize()
// among them - which is why the string and number forms exist at all.
//
// A value in none of those forms is not a boolean, and returns nullopt so the caller applies that
// field's documented default. Keeping "absent" distinct from "false" is the point: the defaults here
// are either true or the transport the token arrived on, never a flat false, so collapsing the two
// would quietly answer a security-bearing question wrong on the transport that has protection to lose.
std::optional<bool> decodeWireBool(const QJsonValue& value)
{
    if (value.isBool()) {
        return value.toBool();
    }
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (number == 1) {
            return true;
        }
        if (number == 0) {
            return false;
        }
        return std::nullopt;
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == qsl("true") || text == qsl("1")) {
            return true;
        }
        if (text == qsl("false") || text == qsl("0")) {
            return false;
        }
    }
    return std::nullopt;
}

// The transport requirement recorded in a stored sign-in entry. Missing means the entry predates the
// field being stored, which is the ordinary case once per profile after an upgrade. Present but
// undecodable means the protected store holds a value Mudlet never wrote - corruption, or another
// writer - sitting next to a bearer token about to be replayed, so unlike the missing case it is worth
// reporting. Both read strictly: never widen a stored token's exposure on a value we could not read.
bool readStoredTransportRequirement(const QJsonObject& entry)
{
    const auto stored = entry[qsl("secure_only")];
    const auto decoded = decodeWireBool(stored);
    if (!decoded.has_value() && !stored.isUndefined()) {
        qWarning().noquote().nospace() << "GMCP Char.Login - the stored sign-in's 'secure_only' is of type " << stored.type()
                                       << ", which is not a boolean in any form; treating the saved token as replayable only over an encrypted connection.";
    }
    return decoded.value_or(true);
}

// The two credential-store keys a profile's saved sign-in occupies: non-secret metadata under one, the
// bearer token under the other. Every write goes metadata-then-token and every removal goes
// token-then-metadata, each step conditional on the one before, so a store failure part way through can
// only ever leave the harmless half without the secret - never the secret without the metadata, which is
// the half the preferences "Forget saved sign-in" control keys off.
//
// Functions rather than repeated literals: the token key is named at five call sites, and a typo in one
// of them would silently write where nothing ever reads.
QString metadataKey()
{
    return qsl("reconnect");
}

// Holds the raw token and nothing else, so the secret never passes through a QJsonDocument - whose
// parsed copy lives in heap storage Qt does not expose and cannot be zeroed.
QString tokenKey()
{
    return qsl("reconnect-token");
}

} // namespace

GMCPAuthenticator::GMCPAuthenticator(Host* pHost)
: mpHost(pHost)
, mpStoreReconciler(new SignInStoreReconciler([this](SignInStoreReconciler::Operation op, QString payload, SignInStoreReconciler::Done done) {
    performStoreOperation(op, std::move(payload), std::move(done));
}))
{
    resetPerConnectionState();
    // mTelnet is declared before this authenticator in Host, so it is already constructed here.
    QObject::connect(&pHost->mTelnet, &cTelnet::signal_connected, pHost, [this]() {
        resetForNewConnection();
    });
    QObject::connect(&pHost->mTelnet, &cTelnet::signal_disconnected, pHost, [this]() {
        resetForNewConnection();
    });
}

void GMCPAuthenticator::resetForNewConnection()
{
    // Anything still in flight belongs to the connection that started it: a deferred attempt would
    // cancel the new connection's login timers and sign in with capabilities the new server never
    // advertised, and a credential read landing there would replay a token nobody asked for.
    ++mSignInScheduleGeneration;
    ++mAuthAttemptGeneration;
    mSignInAttemptPending = false;
    mLastSignInAttempt.invalidate();
    mUnpromptedBrowserOpenAvailable = true;
}

void GMCPAuthenticator::resetPerConnectionState()
{
    // Reset the whole per-connection state as a unit. mReconnectRejected and mAuthAttemptGeneration are
    // deliberately outside mConn (see their declarations) - the latch survives this reset, the counter
    // only ever increments.
    mConn = PerConnectionState{};
    // Start a new auth attempt so any in-flight reconnect-token keychain read from a previous connection
    // is recognised as stale by its callback and ignored.
    ++mAuthAttemptGeneration;
    // A fresh sign-in supersedes any browser dance still in flight from the previous connection.
    cancelClientDrivenOAuth();
}

void GMCPAuthenticator::saveSupportsSet(const QString& packageMessage, const QString& data)
{
    // Clear cached capabilities up front so a malformed frame (an early return below) cannot leave stale
    // provider/auth lists from a previous connection driving later reconnect/auth decisions.
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
    // non-positive value as version 1 (per the spec, the version is a positive, non-zero integer). A
    // value above what this client implements is clamped down by the qBound below, not treated as 1.
    if (jsonObj.contains(qsl("version"))) {
        const int reportedVersion = jsonObj[qsl("version")].toInt(1);
        // Clamp to the highest version this client implements: the negotiated version is
        // min(client, server), so we never act on - or echo back - a version we do not understand.
        mNegotiatedVersion = qBound(1, reportedVersion, 2);
    }

    if (jsonObj.contains(qsl("type"))) {
        QJsonArray typesArray = jsonObj[qsl("type")].toArray();
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

    // The optional client-driven OAuth fields (the server is itself an OpenID Provider) are honoured only
    // on an encrypted transport: the completing Char.Login.AuthCode carries the code and PKCE verifier
    // together and must never travel in the clear. On cleartext, ignore them and fall back to the
    // server-driven flow.
    if (mpHost->mTelnet.currentlySecure()) {
        mOAuthDiscoveryUrl = jsonObj[qsl("location")].toString();
        mOAuthClientId = jsonObj[qsl("client_id")].toString();
        if (jsonObj.contains(qsl("scopes"))) {
            const QJsonArray scopesArray = jsonObj[qsl("scopes")].toArray();
            for (const auto& scope : std::as_const(scopesArray)) {
                const QString scopeName = scope.toString();
                if (scopeName.isEmpty()) {
                    qWarning().noquote().nospace() << "GMCP " << packageMessage << " - ignoring a malformed (non-string or empty) OAuth scope entry.";
                    continue;
                }
                mOAuthScopes.append(scopeName);
            }
        }
        mOAuthNonceRequired = jsonObj[qsl("nonce")].toBool();
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
#endif
}

void GMCPAuthenticator::addCommonFields(QJsonObject& payload) const
{
    // Echo the negotiated version so the server can confirm both ends agree.
    payload[qsl("version")] = mNegotiatedVersion;
    // Whether a Char.Login.Token minted on this connection would actually be written to protected
    // storage and replayed later - which is what lets the game decide whether it can honestly offer to
    // remember this player before it writes its sign-in screen.
    //
    // Constant true for Mudlet: CredentialManager writes to the system keychain, or to its own encrypted
    // file store when the install is portable and when a keychain write fails. The standard asks per
    // connection because a client that has no store at all has to say so; ours always has one to try.
    //
    // A version 1 server gets the fields too. An unknown member is inert to it, and we already echo
    // `version` - itself a version 2 addition - to such servers today. Only the hand-off is special-cased
    // (see sendCredentials), because there the empty object is load-bearing rather than incidental.
    //
    // That is a statement of intent rather than a guarantee the write lands: a store that fails anyway is
    // reported to the player by storeReconnectToken. The standard puts no obligation on a client that
    // answered true and then could not save - only on one that answered false, which must then discard a
    // token that arrives. Qt serialises a real JSON boolean, the form the standard prefers.
    payload[qsl("token_storage")] = true;
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
    // actually accepts password-credentials; otherwise this is the deliberate hand-off telling the game
    // to run its own sign-in screen (see selectAuthMethod).
    if (!interactiveHandoff && mSupportedAuthTypes.contains(qsl("password-credentials")) && !character.isEmpty() && !password.isEmpty()) {
        credentials[qsl("account")] = character;
        credentials[qsl("password")] = password;
        addCommonFields(credentials);
    } else if (mNegotiatedVersion >= 2) {
        // A version 2 hand-off is identified by carrying no account, not by being literally {}, so the
        // common fields ride on it - and carrying token_storage here is the whole reason to send it:
        // this message reaches the game before it writes a line of its sign-in screen, which is the last
        // moment at which it can still decide whether to offer to remember this player. Version 1
        // predates both that rule and the field, so its hand-off stays the bare {} object that a version
        // 1 server may still be testing for literally.
        addCommonFields(credentials);
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

bool GMCPAuthenticator::sendReconnect(const QString& account, QString token, bool secureOnly)
{
    // The token signs in to the account without the player's password, and is replayed on whatever
    // transport is live now rather than the one it was earned on: in the clear that hands the account
    // to anyone on the path. Whether that matters is the issuing server's call, recorded in the token's
    // own requirement when it was minted - so a game that mints and accepts tokens on plain telnet can
    // use them there. The requirement is the server's to set rather than an invariant enforced here: a
    // server that mints over TLS and sends an explicit secure_only false has asked for its own token to
    // be replayable in the clear, and gets that. What is never done is inferring the permissive answer
    // where the server did not give one - see handleAuthToken for the defaulting.
    if (secureOnly && !mpHost->mTelnet.currentlySecure()) {
        SecureStringUtils::secureStringClear(token);
        qWarning().noquote() << "GMCP Char.Login.Reconnect - refusing to replay the saved sign-in token over an unencrypted connection.";
        //: Shown when a saved password-less sign-in cannot be reused because this connection to the game is not encrypted.
        mpHost->postMessage(tr("[ WARN ]  - Not using your saved sign-in because this connection is not encrypted; please sign in again."));
        return false;
    }

    QJsonObject payload;
    payload[qsl("account")] = account;
    payload[qsl("token")] = token;
    // Redundant here - a client replaying a token evidently stores them - but sending the common fields
    // uniformly is simpler than special-casing this one message, and the standard forbids a server from
    // requiring the field here anyway.
    addCommonFields(payload);
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
    return true;
}

void GMCPAuthenticator::performStoreOperation(SignInStoreReconciler::Operation op, QString payload, SignInStoreReconciler::Done done)
{
    using Operation = SignInStoreReconciler::Operation;
    const bool onMetadata = (op == Operation::WriteMetadata || op == Operation::RemoveMetadata);
    const QString key = onMetadata ? metadataKey() : tokenKey();

    QPointer<CredentialManager> credentialManager = new CredentialManager();
    auto onDone = [credentialManager, done = std::move(done)](bool ok, const QString& error) {
        if (credentialManager) {
            credentialManager->deleteLater();
        }
        done(ok, error);
    };

    if (op == Operation::WriteMetadata || op == Operation::WriteToken) {
        credentialManager->storePassword(mpHost->getName(), key, payload, std::move(onDone));
        // Our copy is released here. For the token this is the only copy this class ever held - the
        // reconciler moved it in - so zeroing it is real; CredentialManager's own copy is subject to
        // the implicit-sharing caveat documented there and is not ours to clear.
        SecureStringUtils::secureStringClear(payload);
        return;
    }
    credentialManager->removePassword(mpHost->getName(), key, std::move(onDone));
}

void GMCPAuthenticator::storeReconnectToken(const QString& account, QString token, bool secureOnly)
{
    SignInStoreReconciler::Intent intent;
    intent.shape = SignInStoreReconciler::Shape::Full;
    intent.account = account;
    // Keep the provider with the entry so a later connection can resume this provider's browser
    // sign-in if the token has expired or been revoked by then.
    intent.provider = mConn.accountProvider;
    // Stored with the token rather than recomputed at replay time: the requirement belongs to the
    // connection that minted it, which by then is long gone.
    intent.secureOnly = secureOnly;
    intent.token = std::move(token);

    // Whether a successful save is worth telling the player about at all, decided now: both answers
    // belong to the moment the token arrived, and the next Char.Login.Default resets mConn while the
    // save is still in flight. A rotation is not a new opt-in; and a token this connection could not
    // replay - scoped to an encrypted transport this connection is not - would make the promise
    // knowably false as it was written.
    const bool worthAnnouncing = !mConn.reconnectingWithToken && !(secureOnly && !mpHost->mTelnet.currentlySecure());
    // Which sign-in attempt this save belongs to, so its announcement is deduplicated against that
    // attempt rather than whichever is live when the write lands; see mAnnouncedSaveForAttempt.
    const auto attemptGeneration = mAuthAttemptGeneration;

    // safeHost stands in for `this` as well: Host owns this authenticator, and the reconciler that
    // invokes this completion dies with the authenticator, so `this` is valid whenever safeHost is.
    QPointer<Host> safeHost = mpHost;
    mpStoreReconciler->setIntent(
            std::move(intent), [this, safeHost, worthAnnouncing, attemptGeneration](SignInStoreReconciler::Outcome outcome, SignInStoreReconciler::Operation failedAt, QString error) {
                using Outcome = SignInStoreReconciler::Outcome;
                using Operation = SignInStoreReconciler::Operation;
                // Log before the liveness check: a save can resolve, or time out, after the profile has
                // closed, and that is precisely when the record of a token that never reached disk matters.
                if (outcome == Outcome::Failed) {
                    qWarning().noquote() << "GMCP Char.Login.Token - failed to store the" << (failedAt == Operation::WriteToken ? "reconnect token:" : "sign-in details:") << error;
                }
                if (!safeHost) {
                    return;
                }
                switch (outcome) {
                case Outcome::Superseded:
                    // A newer request owns the outcome now. Saying anything here would be the stale warning
                    // or the duplicate announcement the reconciler exists to prevent.
                    return;
                case Outcome::Failed:
                    //: Shown when the user opted to stay signed in but saving the sign-in token failed, so they will have to sign in again next time.
                    safeHost->postMessage(tr("[ WARN ]  - Could not save your sign-in for next time; you may need to sign in again."));
                    return;
                case Outcome::Reached:
                    // Announced only once the token itself has landed, and at most once per sign-in attempt.
                    if (!worthAnnouncing || mAnnouncedSaveForAttempt == attemptGeneration) {
                        return;
                    }
                    mAnnouncedSaveForAttempt = attemptGeneration;
                    //: Shown once after a browser/OAuth sign-in whose reconnect token was saved, so future connects need no sign-in.
                    //: "Privacy and security" is the name of a page in the preferences dialog; translate it the same way there.
                    safeHost->postMessage(tr("[ INFO ]  - You'll be signed in automatically next time. Manage this under Preferences, Privacy and security."));
                    return;
                }
            });
}

void GMCPAuthenticator::sendResume(const QString& account, const QString& provider)
{
    QJsonObject payload;
    payload[qsl("account")] = account;
    payload[qsl("provider")] = provider;
    addCommonFields(payload);
    const QString gmcpMessage = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.Credentials ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

    const QString display = providerDisplayName(provider);
    //: Shown when Mudlet asks the game to restart the browser sign-in with the remembered provider; %1 is the provider name (e.g. Discord).
    mpHost->postMessage(tr("[ INFO ]  - Resuming your %1 sign-in with the game.").arg(display.isEmpty() ? provider : display));

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP resume (Credentials{account, provider}) for provider:" << provider;
#endif
}

void GMCPAuthenticator::storeResumeHint(const QString& account, const QString& provider)
{
    SignInStoreReconciler::Intent intent;
    intent.shape = SignInStoreReconciler::Shape::Hint;
    intent.account = account;
    intent.provider = provider;

    QPointer<Host> safeHost = mpHost;
    mpStoreReconciler->setIntent(std::move(intent), [this, safeHost](SignInStoreReconciler::Outcome outcome, SignInStoreReconciler::Operation, QString error) {
        if (outcome != SignInStoreReconciler::Outcome::Failed) {
            return;
        }
        qWarning().noquote() << "GMCP Char.Login - failed to keep the sign-in resume hint after a rejected token:" << error;
        if (!safeHost) {
            return;
        }
        // The dead token may still be stored. Drop the whole entry rather than leave it replayable;
        // losing the resume hint only costs the player one provider menu.
        discardReconnectToken();
    });
}

void GMCPAuthenticator::forgetSavedSignIn(std::function<void(bool success)> callback)
{
    discardReconnectToken(std::move(callback));
}

void GMCPAuthenticator::discardReconnectToken(std::function<void(bool success)> callback)
{
    // An intent's default shape is Absent.
    mpStoreReconciler->setIntent(SignInStoreReconciler::Intent{}, [callback = std::move(callback)](SignInStoreReconciler::Outcome outcome, SignInStoreReconciler::Operation failedAt, QString error) {
        using Outcome = SignInStoreReconciler::Outcome;
        using Operation = SignInStoreReconciler::Operation;
        if (outcome == Outcome::Failed) {
            // A failed removal may leave a now-invalid bearer token on disk, so make it visible rather
            // than swallowing it.
            qWarning().noquote() << "GMCP Char.Login - failed to remove the stored" << (failedAt == Operation::RemoveToken ? "reconnect token:" : "sign-in:") << error;
        }
        // Report the real outcome so callers (e.g. the preferences UI) only claim success once
        // everything is actually gone. A superseded forget did not complete as asked - it takes a
        // token arriving in the same instant - and is reported as such.
        if (callback) {
            callback(outcome == Outcome::Reached);
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
    // Also remembered (and persisted alongside any reconnect token) so a later connection can resume
    // this provider's sign-in without the player re-answering "which provider did I use here?".
    const auto provider = obj[qsl("provider")].toString();
    if (!provider.isEmpty()) {
        mConn.accountProvider = provider;
    }

    // The URL arrives unauthenticated over the wire, so only ever hand http(s) links to the OS - never
    // arbitrary scheme handlers (file:, javascript:, etc.), whether we auto-open or offer a manual link.
    const QUrl parsedUrl(url);
    if (!parsedUrl.isValid() || (parsedUrl.scheme() != qsl("http") && parsedUrl.scheme() != qsl("https"))) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Refusing to open sign-in link with unsupported scheme: \"" << url << "\"";
        //: Shown when the game sends a sign-in link with an unsupported or invalid address (not an http/https web link).
        mpHost->postMessage(tr("[ WARN ]  - The game sent an invalid sign-in link; cannot continue."));
        return;
    }

    // Char.Login.URL can arrive at any moment in a session, so it only auto-opens against user input.
    offerOrOpenSignInUrl(parsedUrl, provider, false);
}

void GMCPAuthenticator::offerOrOpenSignInUrl(const QUrl& url, const QString& provider, bool answersTheGamesSignInOffer)
{
    const bool mayOpen = mpHost->userSentInputThisConnection() || (answersTheGamesSignInOffer && mUnpromptedBrowserOpenAvailable);
    if (mayOpen) {
        if (openSignInUrl(url, provider)) {
            mUnpromptedBrowserOpenAvailable = false;
            mpHost->setUserSentInputThisConnection(false);
        }
        return;
    }

    //: %1 is the sign-in web address the user should open in their browser to sign in.
    mpHost->postMessage(tr("[ INFO ]  - To sign in, open this link in your browser: %1").arg(UntrustedText::forTarget(url.toString())));
}

bool GMCPAuthenticator::openSignInUrl(const QUrl& url, const QString& provider)
{
    if (!QDesktopServices::openUrl(url)) {
        //: %1 is the sign-in web address the user should open manually in their browser.
        mpHost->postMessage(tr("[ WARN ]  - Could not open your browser. Open this link manually to sign in: %1").arg(UntrustedText::forTarget(url.toString())));
        return false;
    }
    announceBrowserHandoff(provider);
    return true;
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
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::authorizationCaptured, mpHost, [this](const QString& code, const QString& codeVerifier, const QString& redirectUri, const QString& nonce) {
        sendAuthCode(code, codeVerifier, redirectUri, nonce);
    });
    // This flow only starts from the game's own sign-in offer, so connecting is itself the request.
    QObject::connect(mpOAuthFlow, &OAuthClientFlow::authorizationUrlReady, mpHost, [this](const QUrl& authorizationUrl) {
        offerOrOpenSignInUrl(authorizationUrl, QString(), true);
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

void GMCPAuthenticator::sendAuthCode(QString code, QString codeVerifier, const QString& redirectUri, QString nonce)
{
    // The spec forbids Char.Login.AuthCode on a cleartext connection: the authorization code and PKCE
    // verifier together would let an eavesdropper redeem the code at the provider. The flow only starts
    // on an encrypted connection, but re-check at send time in case the transport changed underneath us.
    if (!mpHost->mTelnet.currentlySecure()) {
        SecureStringUtils::secureStringClear(code);
        SecureStringUtils::secureStringClear(codeVerifier);
        SecureStringUtils::secureStringClear(nonce);
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
    // The server, not this client, receives and validates the ID token, so it is the only party that
    // can check the nonce claim - and only against the value chosen here.
    if (!nonce.isEmpty()) {
        payload[qsl("nonce")] = nonce;
    } else if (mOAuthNonceRequired) {
        qWarning().noquote() << "GMCP Char.Login.AuthCode - the server asked for a nonce but none was generated, so it cannot verify the ID token's nonce claim.";
    }
    addCommonFields(payload);
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
    SecureStringUtils::secureStringClear(nonce);
    SecureStringUtils::secureStringClear(gmcpMessage);
    SecureStringUtils::secureStdStringClear(plaintext);
    SecureStringUtils::secureStdStringClear(encoded);
    SecureStringUtils::secureStdStringClear(output);

    // The flow has done its job: the loopback listener already closed when the code was captured, and the
    // only remaining step is the server's Char.Login.Result. Tear the flow down now - deleteLater makes
    // this safe from within its own authorizationCaptured handler - rather than leaving the object and
    // its state resident until the next Char.Login.Default or profile teardown.
    cancelClientDrivenOAuth();

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

    // A failed password-less reconnect is not a dead end: retryOrDropRejectedToken() first checks
    // whether another instance rotated the token (and replays it if so), and only a genuinely dead
    // token is dropped - falling back to autofilled credentials, a provider resume, or handing off to
    // the game's own sign-in screen, rather than giving up on the connection.
    if (mConn.awaitingReconnectResult) {
        mConn.awaitingReconnectResult = false;
        if (success) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
            qDebug() << "GMCP reconnect successful";
#endif
            return;
        }
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP reconnect rejected:" << message;
#endif
        // Before treating the saved token as dead, check whether another running instance sharing this
        // profile's keychain rotated it while ours was in flight - destroying its fresh token here
        // would sign that instance out too.
        retryOrDropRejectedToken();
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

void GMCPAuthenticator::retryOrDropRejectedToken()
{
    QPointer<Host> safeHost = mpHost;
    const auto attemptGeneration = mAuthAttemptGeneration;
    // Capture the per-connection facts this recovery needs before awaiting the keychain: a
    // Char.Login.Default arriving while the read is in flight resets mConn, and the callback would then
    // have no hash to recognise the rejected token by and no account or provider to keep a resume hint
    // from - silently downgrading the drop below into discarding the whole entry.
    const auto sentTokenHash = mConn.sentReconnectTokenHash;
    const auto retriedRotatedToken = mConn.retriedRotatedToken;
    const auto reconnectAccount = mConn.reconnectAccount;
    const auto accountProvider = mConn.accountProvider;
    // Latch synchronously rather than when the read returns; see mReconnectRejected's declaration.
    mReconnectRejected = true;
    readStoredSignInEntry([this, safeHost, attemptGeneration, sentTokenHash, retriedRotatedToken, reconnectAccount, accountProvider](bool success, StoredSignIn entry, unsigned int) {
        if (!safeHost) {
            SecureStringUtils::secureStringClear(entry.token);
            return;
        }
        // A newer sign-in attempt began while the read was in flight; it owns the connection now, so this
        // recovery must not send anything or reconnect. It may still rewrite the stored entry, but only
        // on positive evidence that the rejected token is the one stored - see the drop below.
        const bool superseded = (attemptGeneration != mAuthAttemptGeneration);

        bool rejectedTokenStillStored = false;

        // Shared-store rotation check: if the stored token no longer hashes to what this connection
        // sent, another running instance rotated it (single-use) - replay the fresh one once, rather
        // than discarding its token. A rejection whose stored token still matches (a genuinely dead
        // token, or a non-rotation rejection) falls through to drop-and-re-sign-in below.
        if (!retriedRotatedToken && success && !entry.account.isEmpty() && !entry.token.isEmpty()) {
            QByteArray tokenBytes = entry.token.toUtf8();
            const QByteArray storedHash = QCryptographicHash::hash(tokenBytes, QCryptographicHash::Sha256);
            SecureStringUtils::secureByteArrayClear(tokenBytes);
            if (!sentTokenHash.isEmpty() && storedHash != sentTokenHash) {
                if (superseded) {
                    // The newer attempt reads the store for itself, so leave the other instance's fresh
                    // token in place and let it decide.
                    SecureStringUtils::secureStringClear(entry.token);
                    return;
                }
#if defined(DEBUG_GMCP_AUTHENTICATION)
                qDebug() << "GMCP reconnect token was rotated by another instance; replaying the fresh token";
#endif
                if (sendReconnect(entry.account, std::move(entry.token), entry.secureOnly)) {
                    mConn.retriedRotatedToken = true;
                    mConn.sentReconnectTokenHash = storedHash;
                    mConn.reconnectingWithToken = true;
                    mConn.awaitingReconnectResult = true;
                    mConn.reconnectAccount = entry.account;
                    // This attempt is replaying a live token rather than recovering from a dead one, so
                    // release the latch: the next Char.Login.Default is an ordinary sign-in again.
                    mReconnectRejected = false;
                    return;
                }
                // The other instance's token is live, so leave the stored entry alone. The rejection
                // latch stays armed: this connection cannot use a token at all.
                selectAuthMethod();
                return;
            }
            // Reaching here means the stored token is not a rotation. That only counts as evidence the
            // rejected token is still stored when this connection recorded what it sent.
            rejectedTokenStillStored = !sentTokenHash.isEmpty();
        }
        // Scrub on every path that did not move the token into sendReconnect.
        SecureStringUtils::secureStringClear(entry.token);

        // A superseded recovery rewrites the entry only when this read positively saw the rejected token
        // still stored. Without that evidence the newer attempt may already have saved its own fresh
        // token, and a token-less rewrite here would erase it and force another browser sign-in.
        if (superseded && !rejectedTokenStillStored) {
            // Leaving the rejected token stored means re-arming the latch: the Char.Login.Default that
            // superseded this recovery already consumed it, so without this the next one would be free
            // to replay a token the server has already rejected.
            mReconnectRejected = true;
            return;
        }
        // The token really is dead. Keep the account+provider resume hint (dropping only the token) so
        // the next attempt restarts the same provider's browser sign-in with no menu.
        dropTokenKeepResumeHint(reconnectAccount, accountProvider);
        if (superseded) {
            return;
        }
        // Servers commonly drop the connection right after rejecting a reconnect, so the fresh sign-in
        // needs a fresh, stable connection.
        //: Shown when a saved password-less sign-in is no longer accepted; Mudlet reconnects so the user can sign in again.
        mpHost->postMessage(tr("[ INFO ]  - Your saved sign-in has expired; reconnecting so you can sign in again."));
        QTimer::singleShot(0ms, mpHost, [safeHost]() {
            if (safeHost) {
                safeHost->mTelnet.reconnect();
            }
        });
    });
}

void GMCPAuthenticator::dropTokenKeepResumeHint(const QString& account, const QString& provider)
{
    // Without a remembered provider there is nothing to resume, so remove the whole entry.
    if (account.isEmpty() || provider.isEmpty()) {
        discardReconnectToken();
        return;
    }
    storeResumeHint(account, provider);
}

// controller for GMCP authentication
void GMCPAuthenticator::handleAuthGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Char.Login.Default")) {
        saveSupportsSet(packageMessage, data);

        // Every rung of the sign-in needs a type to act on, so a frame naming none can only reach the
        // interactive hand-off - and getting there cancels the timer-driven username/password
        // auto-login, the only thing that can sign such a game in. Returning above the reset leaves an
        // attempt already running on this connection to finish.
        if (mSupportedAuthTypes.isEmpty()) {
            // A throttled burst is served by one attempt using the capabilities the last frame left
            // behind, and this frame leaves none - so drop an attempt the burst already armed rather
            // than let it cancel the timers a second later.
            ++mSignInScheduleGeneration;
            mSignInAttemptPending = false;
#if defined(DEBUG_GMCP_AUTHENTICATION)
            qDebug() << "GMCP Char.Login.Default named no auth type; leaving the sign-in to the standard auto-login";
#endif
            return;
        }

        // A fresh Char.Login.Default starts a new sign-in; reset the per-connection token state before
        // deciding how to authenticate.
        resetPerConnectionState();

        scheduleSignInAttempt();
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
    // The payload carries a bearer token; parse from an owned buffer and scrub it once parsed so the
    // token does not linger in an unscrubbed temporary.
    QByteArray payloadBytes = data.toUtf8();
    auto doc = QJsonDocument::fromJson(payloadBytes, &parseError);
    SecureStringUtils::secureByteArrayClear(payloadBytes);
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
        // token may hold a bearer secret even when account is empty (a malformed frame); scrub it on
        // this early return too, matching every other token-parsing site in this file.
        SecureStringUtils::secureStringClear(token);
        return;
    }

    // Whether this token may only ever be replayed on an encrypted transport. Absent - or in a form
    // that is not a boolean at all - means it inherits the transport it arrived on: a token minted over
    // TLS is encrypted-only, one minted in the clear may be replayed either way. A server that mints
    // over TLS but accepts replays in the clear has to say so with an explicit false, so on the transport
    // where a token has any protection to lose, silence can only ever keep it.
    const auto declaredSecureOnly = obj[qsl("secure_only")];
    const auto decodedSecureOnly = decodeWireBool(declaredSecureOnly);
    // A value we could not decode is treated as absent, which is what the standard requires - but unlike
    // a genuinely absent field it means the server tried to state a transport requirement in a form no
    // conformant client can read. Nothing else would report that, and the server operator is the only
    // person who can fix it. Say what the fallback actually did, too: the defaulted answer is written to
    // the store as a definite boolean, so on a cleartext connection an unreadable requirement becomes a
    // durable "replayable in the clear" that every later read then honours without further complaint.
    if (!decodedSecureOnly.has_value() && !declaredSecureOnly.isUndefined()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - a 'secure_only' value of type " << declaredSecureOnly.type()
                                       << " is not a boolean in any form the standard permits, so it is being ignored. The token is stored as "
                                       << (mpHost->mTelnet.currentlySecure() ? "replayable only over an encrypted connection" : "replayable over an unencrypted connection")
                                       << ", inherited from the transport it arrived on.";
    }
    const bool secureOnly = decodedSecureOnly.value_or(mpHost->mTelnet.currentlySecure());

    // The server issues this token at its own discretion - the "remember me" decision belongs to the
    // game's flow, not the client - so we simply persist whatever arrives (overwriting on rotation) and
    // offer a local "forget saved sign-in" control in preferences. Move the token in so
    // storeReconnectToken owns the sole copy and can scrub it after persisting.
    // Announcing that the sign-in was saved belongs to the save's own callback, not here: the store is
    // asynchronous, so a message posted at this point would promise a save that may still fail.
    storeReconnectToken(account, std::move(token), secureOnly);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Stored GMCP reconnect token for account:" << account;
#endif
}

void GMCPAuthenticator::scheduleSignInAttempt()
{
    if (mSignInAttemptPending) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP Char.Login.Default arrived while a sign-in attempt was already scheduled; folding it into that attempt";
#endif
        return;
    }
    if (!mLastSignInAttempt.isValid() || mLastSignInAttempt.durationElapsed() >= scmSignInAttemptInterval) {
        mLastSignInAttempt.start();
        attemptReconnect();
        return;
    }

    // Inside the throttle window: serve the whole burst with one attempt when it closes, using the
    // capabilities the last frame left behind, and drop it if that connection has gone by then.
    mSignInAttemptPending = true;
    const auto scheduleGeneration = mSignInScheduleGeneration;
    QTimer::singleShot(scmSignInAttemptInterval - mLastSignInAttempt.durationElapsed(), mpHost, [this, scheduleGeneration]() {
        if (scheduleGeneration != mSignInScheduleGeneration) {
            return;
        }
        mSignInAttemptPending = false;
        mLastSignInAttempt.start();
        attemptReconnect();
    });
}

void GMCPAuthenticator::attemptReconnect()
{
    mpHost->mTelnet.cancelLoginTimers();

    // We reconnected after a rejected token (see handleAuthResult) to sign in cleanly. The stored entry
    // is being rewritten asynchronously into a token-less resume hint, so read it but never replay a
    // token this once - a not-yet-rewritten entry must not loop us into another rejected reconnect.
    if (mReconnectRejected) {
        mReconnectRejected = false;
        readStoredSignIn(false);
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

    // Reconnect tokens and the provider resume are part of the version 2 OAuth capability; if the
    // server is not offering oauth there is nothing to replay or resume against, so go straight to the
    // normal method selection.
    if (!mSupportedAuthTypes.contains(qsl("oauth"))) {
        selectAuthMethod();
        return;
    }

    readStoredSignIn(true);
}

void GMCPAuthenticator::readStoredSignInEntry(std::function<void(bool success, StoredSignIn entry, unsigned int attemptGeneration)> callback)
{
    QPointer<Host> safeHost = mpHost;
    QPointer<CredentialManager> metadataReader = new CredentialManager();
    const auto attemptGeneration = mAuthAttemptGeneration;
    metadataReader->retrievePassword(
            mpHost->getName(), metadataKey(), [safeHost, metadataReader, attemptGeneration, callback = std::move(callback)](bool success, QString value, const QString& errorMessage) mutable {
                if (metadataReader) {
                    metadataReader->deleteLater();
                }
                if (!safeHost) {
                    SecureStringUtils::secureStringClear(value);
                    return;
                }
                // A read failure is indistinguishable from "no stored sign-in" to the caller - both end
                // at an interactive sign-in - so log it here to make an unreadable token visible rather
                // than silently downgrading the player with no clue.
                if (!success) {
                    qWarning().noquote() << "GMCP Char.Login - could not read the stored sign-in; falling back to interactive sign-in:" << errorMessage;
                    callback(false, StoredSignIn{}, attemptGeneration);
                    return;
                }
                StoredSignIn entry;
                if (value.isEmpty()) {
                    callback(true, entry, attemptGeneration);
                    return;
                }
                // An entry written before the split still carries the token inline, so parse from an
                // owned buffer and scrub every owned copy on every path, corrupt entries included.
                QByteArray valueBytes = value.toUtf8();
                const auto doc = QJsonDocument::fromJson(valueBytes);
                SecureStringUtils::secureByteArrayClear(valueBytes);
                SecureStringUtils::secureStringClear(value);
                if (!doc.isObject()) {
                    callback(true, entry, attemptGeneration);
                    return;
                }
                const auto obj = doc.object();
                entry.account = obj[qsl("account")].toString();
                entry.provider = obj[qsl("provider")].toString();
                entry.secureOnly = readStoredTransportRequirement(obj);
                entry.token = obj[qsl("token")].toString();
                // An inline token wins over the token key. Only a Mudlet from before the split writes
                // one, and every split-format save rewrites the metadata without it - so an inline token
                // sitting beside a token key means that instance rotated the token more recently than
                // any split write, and its copy is the live one.
                if (!entry.token.isEmpty() || entry.account.isEmpty()) {
                    callback(true, std::move(entry), attemptGeneration);
                    return;
                }
                QPointer<CredentialManager> tokenReader = new CredentialManager();
                tokenReader->retrievePassword(
                        safeHost->getName(),
                        tokenKey(),
                        [safeHost, tokenReader, entry = std::move(entry), attemptGeneration, callback = std::move(callback)](bool tokenSuccess, QString tokenValue, const QString& tokenError) mutable {
                            if (tokenReader) {
                                tokenReader->deleteLater();
                            }
                            if (!safeHost) {
                                SecureStringUtils::secureStringClear(tokenValue);
                                return;
                            }
                            if (tokenSuccess) {
                                entry.token = std::move(tokenValue);
                            } else {
                                // CredentialManager cannot tell an absent key from a failed read, and the common
                                // reason to land here is the ordinary one: this entry is a resume hint with no token
                                // to find. A genuinely broken store fails the metadata read above and warns there,
                                // so this stays a debug note rather than claiming a failure that may not have happened.
                                qDebug().noquote() << "GMCP Char.Login - no saved token was read; using the stored sign-in as a resume hint only:" << tokenError;
                            }
                            callback(true, std::move(entry), attemptGeneration);
                        });
            });
}

void GMCPAuthenticator::readStoredSignIn(bool allowToken)
{
    QPointer<Host> safeHost = mpHost;
    readStoredSignInEntry([this, safeHost, allowToken](bool, StoredSignIn entry, unsigned int attemptGeneration) {
        // The Host (which owns this authenticator) may have gone away while the read was in flight.
        if (!safeHost) {
            SecureStringUtils::secureStringClear(entry.token);
            return;
        }
        // A newer connection started while this read was in flight; its result is stale, so ignore it
        // rather than send a reconnect or pick a method for the connection that superseded it.
        if (attemptGeneration != mAuthAttemptGeneration) {
            SecureStringUtils::secureStringClear(entry.token);
            return;
        }
        if (!entry.provider.isEmpty()) {
            mConn.accountProvider = entry.provider;
        }
        if (allowToken && !entry.account.isEmpty() && !entry.token.isEmpty()) {
            // Remember only a hash of what we send: if the reconnect is rejected, comparing it against a
            // fresh read tells a dead token apart from one another running instance (sharing this
            // profile's keychain) rotated while ours was in flight.
            QByteArray tokenBytes = entry.token.toUtf8();
            const QByteArray sentHash = QCryptographicHash::hash(tokenBytes, QCryptographicHash::Sha256);
            SecureStringUtils::secureByteArrayClear(tokenBytes);
            // Move the token in so sendReconnect owns the sole copy and can scrub it either way.
            if (sendReconnect(entry.account, std::move(entry.token), entry.secureOnly)) {
                // This connection is logging in by replaying a saved token, so a Char.Login.Token that
                // comes back is a silent rotation rather than a first-time save to announce.
                mConn.reconnectingWithToken = true;
                mConn.awaitingReconnectResult = true;
                mConn.reconnectAccount = entry.account;
                mConn.sentReconnectTokenHash = sentHash;
                return;
            }
            // Not sent, so nothing awaits a result on it; fall through to resume or hand-off.
        }
        // Scrub on every remaining path: the token is still live here whenever sendReconnect was not
        // reached or refused.
        SecureStringUtils::secureStringClear(entry.token);
        if (!entry.account.isEmpty() && !entry.provider.isEmpty()) {
            // No usable token, but we remember how this account signs in: ask the game to restart that
            // provider's browser sign-in rather than fall to a provider menu.
            sendResume(entry.account, entry.provider);
            return;
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

    // Otherwise hand off to the game's own interactive sign-in (sendCredentials picks the hand-off shape
    // for the negotiated version) and let the player choose there - even if the profile has a stored password (the game's screen
    // still accepts it), so a saved password never blocks reaching a provider choice.
    sendCredentials(true);
}
