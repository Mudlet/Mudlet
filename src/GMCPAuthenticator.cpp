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

#include "GMCPAuthenticator.h"

#include "Host.h"
#include "ctelnet.h"
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

GMCPAuthenticator::GMCPAuthenticator(Host* pHost)
: mpHost(pHost), mHttpServer(nullptr)
{}

void GMCPAuthenticator::saveSupportsSet(const QString& data)
{
    auto jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    auto jsonObj = jsonDoc.object();

    if (jsonObj.contains("type")) {
        QJsonArray typesArray = jsonObj["type"].toArray();
        for (const auto& type : typesArray) {
            mSupportedAuthTypes.append(type.toString());
        }
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
#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Sent GMCP credentials";
#endif
}

void GMCPAuthenticator::handleAuthResult(const QString& data)
{
    auto doc = QJsonDocument::fromJson(data.toUtf8());
    auto obj = doc.object();

    // some game drivers can parse JSON for true or false, but may not be able to write booleans back
    auto result = obj[qsl("success")];
    bool success = (result.isBool() && result.toBool()) || (result.isString() && result.toString() == "true");
    auto message = obj[qsl("message")].toString();

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
        saveSupportsSet(data);

        if (mSupportedAuthTypes.contains(qsl("password-credentials"))) {
            mpHost->mTelnet.cancelLoginTimers();
            sendCredentials();
        } else {
#if defined(DEBUG_GMCP_AUTHENTICATION)
            qDebug() << "Server does not support credentials authentication and we don't support any other";
#endif
        }
        return;
    }

    if (packageMessage == qsl("Char.Login.Result")) {
        handleAuthResult(data);
        return;
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Unknown GMCP auth package:" << packageMessage;
#endif
}

void GMCPAuthenticator::startLocalServer()
{
    if (!mHttpServer) {
        mHttpServer = new QTcpServer(this);
    }

    connect(mHttpServer, &QTcpServer::newConnection, this, &GMCPAuthenticator::handleIncomingConnection);

    if (!mHttpServer->listen(QHostAddress::LocalHost, 8000)) {
        qDebug() << "Error: Could not start local HTTP server";
        return;
    }

    qDebug() << "Local HTTP server started on port 8000";
}

void GMCPAuthenticator::handleIncomingConnection()
{
    QTcpSocket *clientConnection = mHttpServer->nextPendingConnection();
    connect(clientConnection, &QTcpSocket::disconnected, clientConnection, &QTcpSocket::deleteLater);

    if (!clientConnection->waitForReadyRead(5000)) {
        clientConnection->disconnectFromHost();
        return;
    }

    QByteArray requestData = clientConnection->readAll();
    QString requestString = QString::fromUtf8(requestData);

    QRegExp errorRegex("GET /\\?error=([^&\\s]+)");
    if (errorRegex.indexIn(requestString) != -1) {
        QString error = errorRegex.cap(1);
        mpHost->postMessage(tr("[ WARN ]  - OpenID authentication failed: %1").arg(error));
        stopLocalServer();
        return;
    }

    QRegExp codeRegex("GET /\\?code=([^&\\s]+)&state=([^\\s]+)");
    if (codeRegex.indexIn(requestString) != -1) {
        QString receivedCode = codeRegex.cap(1);
        QString receivedState = codeRegex.cap(2);

        if (receivedState == oidcState) {
            qDebug() << "Received OIDC Authorization Code: " << receivedCode;
            exchangeCodeForToken(receivedCode);
        } else {
            qDebug() << "OIDC state mismatch! Possible CSRF attack.";
        }
    }

    clientConnection->disconnectFromHost();
}

void GMCPAuthenticator::stopLocalServer()
{
    if (mHttpServer) {
        mHttpServer->close();
        delete mHttpServer;
        mHttpServer = nullptr;
        qDebug() << "Local HTTP server stopped";
    }
}

void GMCPAuthenticator::exchangeCodeForToken(const QString& authCode)
{
    QString tokenUrl = getOIDCTokenURL(oidcProvider);
    if (tokenUrl.isEmpty()) {
        qDebug() << "Error: Invalid token URL for provider " << oidcProvider;
        return;
    }

    QUrl url(tokenUrl);
    QUrlQuery postData;
    postData.addQueryItem("client_id", "your-client-id");
    postData.addQueryItem("client_secret", "your-client-secret");
    postData.addQueryItem("code", authCode);
    postData.addQueryItem("grant_type", "authorization_code");
    postData.addQueryItem("redirect_uri", "http://127.0.0.1:8000");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = networkManager.post(request, postData.query().toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument responseJson = QJsonDocument::fromJson(reply->readAll());
            QString idToken = responseJson.object().value("id_token").toString();
            processOIDCToken(idToken);
        } else {
            qDebug() << "OIDC Token Exchange Failed: " << reply->errorString();
        }
        reply->deleteLater();
    });
}

void GMCPAuthenticator::processOIDCToken(const QString& idToken)
{
    QJsonObject payload = decodeJWT(idToken);
    if (payload.isEmpty()) {
        qDebug() << "Error: Failed to decode JWT";
        return;
    }

    QString issuer = payload.value("iss").toString();
    QString audience = payload.value("aud").toString();
    qint64 expiration = payload.value("exp").toDouble();
    QString receivedNonce = payload.value("nonce").toString();

    if (issuer != "https://accounts.google.com") {
        qDebug() << "Error: Invalid token issuer!";
        return;
    }

    if (audience != "your-client-id") {
        qDebug() << "Error: Token audience mismatch!";
        return;
    }

    if (QDateTime::currentSecsSinceEpoch() >= expiration) {
        qDebug() << "Error: ID token is expired!";
        return;
    }

    if (receivedNonce != oidcNonce) {
        qDebug() << "Error: Nonce mismatch! Possible replay attack.";
        return;
    }

    qDebug() << "ID Token validated successfully.";
    sendOIDCCredentials(payload.value("email").toString(), idToken);
}

QJsonObject GMCPAuthenticator::decodeJWT(const QString& jwt)
{
    QStringList parts = jwt.split(".");
    if (parts.size() != 3) {
        qDebug() << "Error: Invalid JWT format.";
        return QJsonObject();
    }

    QByteArray payloadData = QByteArray::fromBase64(parts[1].toUtf8());
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payloadData);
    
    if (jsonDoc.isNull()) {
        qDebug() << "Error: Failed to parse JWT payload.";
        return QJsonObject();
    }

    return jsonDoc.object();
}

void GMCPAuthenticator::setOIDCProvider(const QString& provider)
{
    oidcProvider = provider;
}

QString GMCPAuthenticator::getOIDCAuthURL(const QString& provider)
{
    QMap<QString, QString> providerURLs = {
        {"Google", "https://accounts.google.com/o/oauth2/v2/auth"},
        {"Microsoft", "https://login.microsoftonline.com/common/oauth2/v2.0/authorize"},
        {"GitHub", "https://github.com/login/oauth/authorize"},
        {"Steam", "https://steamcommunity.com/openid"},
        {"Apple", "https://appleid.apple.com/auth/authorize"},
        {"Facebook", "https://www.facebook.com/v12.0/dialog/oauth"}
    };
    return providerURLs.value(provider, "");
}

QString GMCPAuthenticator::getOIDCTokenURL(const QString& provider)
{
    QMap<QString, QString> tokenURLs = {
        {"Google", "https://oauth2.googleapis.com/token"},
        {"Microsoft", "https://login.microsoftonline.com/common/oauth2/v2.0/token"},
        {"GitHub", "https://github.com/login/oauth/access_token"},
        {"Steam", "https://steamcommunity.com/openid"},
        {"Apple", "https://appleid.apple.com/auth/token"},
        {"Facebook", "https://graph.facebook.com/v12.0/oauth/access_token"}
    };
    return tokenURLs.value(provider, "");
}

void GMCPAuthenticator::startOIDCAuth(const QString& provider)
{
    setOIDCProvider(provider);
    startLocalServer();

    QString clientId = "your-client-id";
    QString redirectUri = "http://127.0.0.1:8000";
    oidcState = QUuid::createUuid().toString(QUuid::Id128);
    oidcNonce = QUuid::createUuid().toString(QUuid::Id128);
    QString authUrl = getOIDCAuthURL(provider);

    if (authUrl.isEmpty()) {
        qDebug() << "Error: Unsupported OIDC provider!";
        return;
    }

    QUrl url(authUrl);
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", "openid email profile");
    query.addQueryItem("redirect_uri", redirectUri);
    query.addQueryItem("state", oidcState);
    query.addQueryItem("nonce", oidcNonce);

    url.setQuery(query);
    QDesktopServices::openUrl(url);
}
