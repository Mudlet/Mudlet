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
#include "SecureStringUtils.h"
#include "ctelnet.h"
#include "mudlet.h"
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QHash>
#include <QLabel>
#include <QPointer>
#include <QUrl>
#include <QVBoxLayout>

GMCPAuthenticator::GMCPAuthenticator(Host* pHost)
: mpHost(pHost)
{
}

void GMCPAuthenticator::saveSupportsSet(const QString& packageMessage, const QString& data)
{
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

    mSupportedAuthTypes.clear();
    if (jsonObj.contains("type")) {
        QJsonArray typesArray = jsonObj["type"].toArray();
        for (const auto& type : std::as_const(typesArray)) {
            mSupportedAuthTypes.append(type.toString());
        }
    }

    mOAuthProviders.clear();
    if (jsonObj.contains("providers")) {
        const QJsonArray providersArray = jsonObj["providers"].toArray();
        for (const auto& provider : std::as_const(providersArray)) {
            mOAuthProviders.append(provider.toString());
        }
    }

    // Remember (persistently) that this game presents a Char.Login sign-in choice, i.e. it offers both
    // OAuth and password-credentials. This gates the profile's "always use character name and password"
    // preference so that option is only ever shown for games that actually use this flow. It is sticky:
    // once seen it stays set so the preference (and its undo) remains available even if a later
    // connection advertises fewer methods.
    if (mSupportedAuthTypes.contains(qsl("oauth")) && mSupportedAuthTypes.contains(qsl("password-credentials"))) {
        mpHost->mSeenCharLoginSignInChoice = true;
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
#endif
}

void GMCPAuthenticator::sendCredentials()
{
    auto character = mpHost->getLogin();
    auto password = mpHost->getPass();

    QJsonObject credentials;

    if (!character.isEmpty() && !password.isEmpty()) {
        credentials["account"] = character;
        credentials["password"] = password;
    }

    QJsonDocument doc(credentials);
    QString gmcpMessage = doc.toJson(QJsonDocument::Compact);

    // Clear sensitive data from memory as soon as possible
    credentials = QJsonObject();                    // Clear JSON object
    doc = QJsonDocument();                          // Clear document
    SecureStringUtils::secureStringClear(password); // Clear password copy

    // Build and send the GMCP message
    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.Credentials ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    // Send credentials to server
    mpHost->mTelnet.socketOutRaw(output);

    // Clear message from memory
    SecureStringUtils::secureStringClear(gmcpMessage);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP credentials";
#endif
}


void GMCPAuthenticator::promptForOAuthProvider()
{
    //: Title of the dialog asking which provider to use for a web/OAuth sign-in.
    const QString title = tr("Choose sign-in provider");
    //: Label prompting the user to pick a provider to sign in with.
    const QString label = tr("Sign in with:");

    // Server providers arrive as lowercase ids (e.g. "github"); show human-readable labels so screen
    // readers announce them clearly, while keeping a map back to the id we must send over the wire.
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

    auto* dialog = new QDialog(mudlet::self());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setWindowTitle(title);

    auto* layout = new QVBoxLayout(dialog);
    auto* promptLabel = new QLabel(label, dialog);
    layout->addWidget(promptLabel);

    auto* combo = new QComboBox(dialog);
    promptLabel->setBuddy(combo);
    for (const auto& id : std::as_const(mOAuthProviders)) {
        QString display = brandNames.value(id.toLower());
        if (display.isEmpty()) {
            display = id;
            if (!display.isEmpty()) {
                display[0] = display[0].toUpper();
            }
        }
        combo->addItem(display, id);
    }

    // When the server also accepts classic login, offer it in the same picker beneath the providers so a
    // player with a traditional character name and password can pick it instead of an external provider.
    if (mSupportedAuthTypes.contains(qsl("password-credentials")) && combo->count() > 0) {
        combo->insertSeparator(combo->count());
        //: Provider-picker entry for signing in with a game-native character name and password instead of an external provider.
        combo->addItem(tr("Character name and password"), qsl("password-credentials"));
    }
    layout->addWidget(combo);

    //: Checkbox letting the user stay signed in on this device for future connections.
    auto* rememberMe = new QCheckBox(tr("Remember me on this device"), dialog);
    layout->addWidget(rememberMe);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    QObject::connect(dialog, &QDialog::accepted, dialog, [this, combo, rememberMe]() {
        const QString choice = combo->currentData().toString();
        if (choice.isEmpty()) {
            return;
        }
        if (choice == qsl("password-credentials")) {
            // "Remember me" here means "always sign in this way": persist the choice so future connects
            // skip this picker entirely (see selectAuthMethod). Classic login never mints a reconnect
            // token, so drop any previously stored one.
            if (rememberMe->isChecked()) {
                mpHost->mUseCharacterNamePasswordLogin = true;
                discardReconnectToken();
            }
            sendCredentials();
            return;
        }
        mTokenPersistEnabled = rememberMe->isChecked();
        sendOAuth(choice);
    });
    QObject::connect(dialog, &QDialog::rejected, mpHost, [this]() {
        mpHost->mTelnet.setDontReconnect(true);
        //: Shown when the user dismisses the provider-selection dialog instead of signing in.
        mpHost->postMessage(tr("[ INFO ]  - Sign-in cancelled."));
    });
    dialog->open();
}

void GMCPAuthenticator::sendOAuth(const QString& provider)
{
    QJsonObject payload;
    payload[qsl("provider")] = provider;
    QString gmcpMessage = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.OAuth ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP OAuth provider:" << provider;
#endif
}

void GMCPAuthenticator::sendReconnect(const QString& account, const QString& token)
{
    QJsonObject payload;
    payload[qsl("account")] = account;
    payload[qsl("token")] = token;
    QString gmcpMessage = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    payload = QJsonObject();

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Char.Login.Reconnect ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    mpHost->mTelnet.socketOutRaw(output);

    // The reconnect token is a bearer secret, so scrub our copy of the message once it is sent.
    SecureStringUtils::secureStringClear(gmcpMessage);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP reconnect for account:" << account;
#endif
}

void GMCPAuthenticator::storeReconnectToken(const QString& account, const QString& token)
{
    QJsonObject obj;
    obj[qsl("account")] = account;
    obj[qsl("token")] = token;
    const QString payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QPointer<CredentialManager> credentialManager = new CredentialManager();
    credentialManager->storePassword(mpHost->getName(), qsl("reconnect"), payload, [credentialManager](bool success, const QString& errorMessage) {
        if (!success) {
            qWarning().noquote() << "GMCP Char.Login.Token - failed to store reconnect token:" << errorMessage;
        }
        if (credentialManager) {
            credentialManager->deleteLater();
        }
    });
}

void GMCPAuthenticator::discardReconnectToken()
{
    mTokenPersistEnabled = false;

    QPointer<CredentialManager> credentialManager = new CredentialManager();
    credentialManager->removePassword(mpHost->getName(), qsl("reconnect"), [credentialManager](bool, const QString&) {
        if (credentialManager) {
            credentialManager->deleteLater();
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

    const auto url = doc.object()[qsl("url")].toString();
    if (url.isEmpty()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Missing 'url' field.";
        return;
    }

    // The URL arrives unauthenticated over the wire and is opened without a user click, so only hand
    // http(s) links to the OS to avoid launching arbitrary scheme handlers (file:, javascript:, etc.).
    const QUrl parsedUrl(url);
    if (!parsedUrl.isValid() || (parsedUrl.scheme() != qsl("http") && parsedUrl.scheme() != qsl("https"))) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Refusing to open sign-in link with unsupported scheme: \"" << url << "\"";
        mpHost->postMessage(tr("[ WARN ]  - The game sent an invalid sign-in link; cannot continue."));
        return;
    }

    if (!QDesktopServices::openUrl(parsedUrl)) {
        //: %1 is the sign-in web address the user should open manually in their browser.
        mpHost->postMessage(tr("[ WARN ]  - Could not open your browser. Open this link manually to sign in: %1").arg(url));
        return;
    }

    //: Shown after the user's browser is launched to complete an OAuth/web sign-in.
    const QString message = tr("[ INFO ]  - Opening your browser to sign in. Complete the login there, then return here.");
    mpHost->postMessage(message);

    // The browser handoff happens with no focused control to announce it, so push an explicit
    // announcement to assistive technology (VoiceOver/NVDA/Orca) instead of relying on the console.
    if (auto* mainWindow = mudlet::self()) {
        QAccessibleAnnouncementEvent announcement(mainWindow, message);
        QAccessible::updateAccessibility(&announcement);
    }
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
    // normal sign-in (browser pairing or credentials) instead of giving up on the connection.
    if (mAwaitingReconnectResult) {
        mAwaitingReconnectResult = false;
        if (success) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
            qDebug() << "GMCP reconnect successful";
#endif
            return;
        }
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP reconnect rejected, falling back:" << message;
#endif
        discardReconnectToken();
        //: Shown when a saved password-less sign-in is no longer accepted and the user must sign in again.
        mpHost->postMessage(tr("[ INFO ]  - Your saved sign-in has expired; please sign in again."));
        selectAuthMethod();
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
        mAwaitingReconnectResult = false;
        mTokenPersistEnabled = false;

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
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString() << " at offset " << parseError.offset << ". Received data: \"" << data
                                       << "\"";
        return;
    }
    if (!doc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got " << (doc.isArray() ? "array" : doc.isNull() ? "null" : "unknown type") << ".";
        return;
    }

    const auto obj = doc.object();
    const auto account = obj[qsl("account")].toString();
    const auto token = obj[qsl("token")].toString();
    if (account.isEmpty() || token.isEmpty()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Missing 'account' or 'token' field.";
        return;
    }

    // Only persist when the user opted into "remember me" this sign-in, or when we are rotating a token
    // that was already remembered (a saved token was loaded at connect). Otherwise drop it on the floor.
    if (!mTokenPersistEnabled) {
#if defined(DEBUG_GMCP_AUTHENTICATION)
        qDebug() << "GMCP reconnect token discarded - persistence not enabled for account:" << account;
#endif
        return;
    }

    storeReconnectToken(account, token);

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Stored GMCP reconnect token for account:" << account;
#endif
}

void GMCPAuthenticator::attemptReconnect()
{
    mpHost->mTelnet.cancelLoginTimers();

    // A standing "always use character name and password" preference takes precedence over any saved
    // OAuth reconnect token: hand off to method selection, which will send the credentials.
    if (mpHost->mUseCharacterNamePasswordLogin && mSupportedAuthTypes.contains(qsl("password-credentials"))) {
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
    credentialManager->retrievePassword(mpHost->getName(), qsl("reconnect"), [this, safeHost, credentialManager](bool success, const QString& value, const QString&) {
        if (credentialManager) {
            credentialManager->deleteLater();
        }
        // The Host (which owns this authenticator) may have gone away while the keychain read was in
        // flight; bail out without touching any members if so.
        if (!safeHost) {
            return;
        }

        if (success && !value.isEmpty()) {
            const auto doc = QJsonDocument::fromJson(value.toUtf8());
            if (doc.isObject()) {
                const auto account = doc.object()[qsl("account")].toString();
                const auto token = doc.object()[qsl("token")].toString();
                if (!account.isEmpty() && !token.isEmpty()) {
                    // A remembered token implies the user already opted in, so keep persistence enabled
                    // to capture any rotated token the server sends back.
                    mTokenPersistEnabled = true;
                    mAwaitingReconnectResult = true;
                    sendReconnect(account, token);
                    return;
                }
            }
        }

        selectAuthMethod();
    });
}

void GMCPAuthenticator::selectAuthMethod()
{
    const bool haveCredentials = !mpHost->getLogin().isEmpty() && !mpHost->getPass().isEmpty();

    // If the user chose to always sign in with a character name and password, honour that whenever the
    // server offers it, skipping the provider picker regardless of the server's preferred order.
    if (mpHost->mUseCharacterNamePasswordLogin && mSupportedAuthTypes.contains(qsl("password-credentials"))) {
        mpHost->mTelnet.cancelLoginTimers();
        sendCredentials();
        return;
    }

    // mSupportedAuthTypes preserves the server's advertised order (see saveSupportsSet), so the
    // first method we can satisfy is the server's most-preferred one.
    for (const auto& type : std::as_const(mSupportedAuthTypes)) {
        if (type == qsl("oauth")) {
            // We can only drive the server-driven flow when the server told us which providers it
            // accepts; without that list fall through to the next advertised method.
            if (!mOAuthProviders.isEmpty()) {
                mpHost->mTelnet.cancelLoginTimers();
                promptForOAuthProvider();
                return;
            }
        } else if (type == qsl("password-credentials") && haveCredentials) {
            mpHost->mTelnet.cancelLoginTimers();
            sendCredentials();
            return;
        }
    }

    // The loop only sends password-credentials when we have a stored login/password. If the server
    // offers it but we have none, send anyway so the server can prompt for or reject the credentials.
    if (mSupportedAuthTypes.contains(qsl("password-credentials"))) {
        mpHost->mTelnet.cancelLoginTimers();
        sendCredentials();
    } else {
        // Nothing the server offered is satisfiable; tell the user rather than stalling silently.
        mpHost->mTelnet.cancelLoginTimers();
        mpHost->mTelnet.setDontReconnect(true);
        mpHost->postMessage(tr("[ WARN ]  - The game offered no sign-in method this client supports."));
    }
}
