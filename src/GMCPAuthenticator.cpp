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
#include "SecureStringUtils.h"
#include "ctelnet.h"
#include <QDebug>

GMCPAuthenticator::GMCPAuthenticator(Host* pHost)
: mpHost(pHost)
{}

void GMCPAuthenticator::saveSupportsSet(const QString& packageMessage, const QString& data)
{
    QJsonParseError parseError;
    auto jsonDoc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString()
                                       << " at offset " << parseError.offset << ". Received data: \"" << data << "\"";
        return;
    }
    if (!jsonDoc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got "
                                       << (jsonDoc.isArray() ? "array" : jsonDoc.isNull() ? "null" : "unknown type") << ".";
        return;
    }
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
    
    // Clear sensitive data from memory as soon as possible
    credentials = QJsonObject(); // Clear JSON object
    doc = QJsonDocument();       // Clear document
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


void GMCPAuthenticator::handleAuthResult(const QString& packageMessage, const QString& data)
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Failed to parse JSON: " << parseError.errorString()
                                       << " at offset " << parseError.offset << ". Received data: \"" << data << "\"";
        return;
    }
    if (!doc.isObject()) {
        qWarning().noquote().nospace() << "GMCP " << packageMessage << " - Expected JSON object but got "
                                       << (doc.isArray() ? "array" : doc.isNull() ? "null" : "unknown type") << ".";
        return;
    }
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
        saveSupportsSet(packageMessage, data);

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
        handleAuthResult(packageMessage, data);
        return;
    }

#if defined(DEBUG_GMCP_AUTHENTICATION)
    qDebug() << "Unknown GMCP auth package:" << packageMessage;
#endif
}
