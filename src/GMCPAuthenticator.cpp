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

#include <QDebug>
#include "Host.h"
#include "ctelnet.h"

GMCPAuthenticator::GMCPAuthenticator(Host* pHost) : mpHost(pHost) {}

void GMCPAuthenticator::saveSupportsSet(const QString& data)
{
    auto jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    auto jsonObj = jsonDoc.object();

    if (jsonObj.contains("types")) {
        QJsonArray typesArray = jsonObj["types"].toArray();
        for (const auto& type : typesArray) {
            mSupportedAuthTypes.append(type.toString());
        }
    }

    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
}

void GMCPAuthenticator::sendCredentials()
{
    auto character = mpHost->getLogin();
    auto password = mpHost->getPass();
    if (character.isEmpty() || password.isEmpty()) {
        qDebug() << "No login or password set in connection settings. Cannot authenticate with GMCP.";
        return;
    }

    QJsonObject credentials;
    credentials["character"] = character;
    credentials["password"] = password;

    QJsonDocument doc(credentials);
    QString gmcpMessage = doc.toJson(QJsonDocument::Compact);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Client.Authenticate.Credentials ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    // Send credentials to server
    mpHost->mTelnet.socketOutRaw(output);
}


void GMCPAuthenticator::handleAuthResult(const QString& data)
{
    auto doc = QJsonDocument::fromJson(data.toUtf8());
    auto obj = doc.object();

    bool success = obj["success"].toBool();
    auto message = obj["message"].toString();

    if (success) {
        qDebug() << "GMCP login successful";
    } else {
        qDebug() << "GMCP login failed:" << message;
    }
}

// controller for GMCP authentication
void GMCPAuthenticator::handleAuthGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Client.Authenticate.Default")) {
        saveSupportsSet(data);

        if (mSupportedAuthTypes.contains(qsl("credentials"))) {
            sendCredentials();
        } else {
            qDebug() << "Server does not support credentials authentication and we don't support any other";
        }
    }

    else if (packageMessage == qsl("Client.Authenticate.Result")) {
        handleAuthResult(data);

    } else {
        qDebug() << "Unknown GMCP auth package:" << packageMessage;
    }
}
