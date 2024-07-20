/***************************************************************************
 *   Copyright (C) 2024 by Mike Conley - mike.conley@stickmud.com          *
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

#include "ClientVariables.h"

#include "Host.h"
#include "ctelnet.h"
#include <QDateTime>
#include <QDebug>

ClientVariables::ClientVariables(Host* pHost) : mpHost(pHost) {}

void ClientVariables::sendClientVariablesList() {
    QJsonObject clientVariablesList;

    const auto mnesVariables = mnesVariablesMap();

    for (auto it = mnesVariables.constBegin(); it != mnesVariables.constEnd(); ++it) {
        clientVariablesList[it.key()] = QJsonObject{
            {"available", true},
            {"updatable", it.value()}
        };
    }

    if (!mpHost->mEnableMNES) {
        const auto nonMNESVariables = nonMNESVariablesMap();

        for (auto it = nonMNESVariables.constBegin(); it != nonMNESVariables.constEnd(); ++it) {
            clientVariablesList[it.key()] = QJsonObject{
                {"available", true},
                {"updatable", it.value()}
            };
        }

        const auto protectedVariables = protectedVariablesMap();

        for (auto it = protectedVariables.constBegin(); it != protectedVariables.constEnd(); ++it) {
            const auto &[updatable, behaviour, translation] = it.value();

            if (behaviour != Host::DataSharingBehaviour::Block) {
                const bool available = (behaviour == Host::DataSharingBehaviour::OptIn);
                clientVariablesList[it.key()] = QJsonObject{
                    {"available", available},
                    {"updatable", updatable}
                };
            }
        }
    }

    if (!clientVariablesList.isEmpty()) {
        QJsonDocument doc(clientVariablesList);
        QString gmcpMessage = doc.toJson(QJsonDocument::Compact);

        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_GMCP;
        output += "Client.Variables.List ";
        output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
        output += TN_IAC;
        output += TN_SE;

        // Send variables to server
        mpHost->mTelnet.socketOutRaw(output);
        qDebug() << "Sent client variables list";
    }
}

void ClientVariables::sendClientVariablesUpdate(const QString& data, ClientVariables::Source source) {
    QJsonObject response;
    QStringList requested;

    const auto nonProtectedVariables = nonProtectedVariablesMap();
    const auto protectedVariables = protectedVariablesMap();
    const auto newEnvironDataMap = mpHost->mTelnet.getNewEnvironDataMap(); // client variable values
    QStringList sources = {"request", "server", "client"};  // initiated by
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();  // current timestamp

    auto addResponse = [&](const QString& key, bool available, bool updatable, bool requested = false, const QString& value = QString()) {
        QJsonObject obj{
            {"available", available},
            {"updatable", updatable},
            {"requested", requested},
            {"source", sources[source]},
            {"timestamp", timestamp}
        };

        if (!value.isEmpty()) {
            if (source == ClientVariables::SourceServer) {
                obj["success"] = (available && updatable && value == newEnvironDataMap[key].second);
            }

            obj["value"] = value;
        }

        response[key] = obj;
    };

    auto doc = QJsonDocument::fromJson(data.toUtf8());
    auto jsonObj = doc.object();

    for (QJsonObject::ConstIterator it = jsonObj.constBegin(); it != jsonObj.constEnd(); ++it) {
        bool available = !mpHost->mEnableMNES; // Default availability state
        const QString key = (doc.isArray() ? it.key() : it.value()[0]).toString();
        const QString value = (doc.isArray() ? QString() : it.value()[1]).toString();

        if (protectedVariables.contains(key)) {
            const auto &[updatable, behaviour, translation] = protectedVariables[key];

            if (behaviour == Host::DataSharingBehaviour::Block) {
                addResponse(key, false, false);
            } else {
                available = (behaviour == Host::DataSharingBehaviour::OptIn);

                if (!available && source == ClientVariables::SourceRequest) {
                    requested << translation;
                }

                if (newEnvironDataMap.contains(key)) {
                    addResponse(key, available, updatable, !available, value);
                } else {
                    addResponse(key, available, updatable, !available);
                }
            }
        } else if (nonProtectedVariables.contains(key)) {
            if (newEnvironDataMap.contains(key)) {
                addResponse(key, true, nonProtectedVariables[key], false, value);
            } else {
                addResponse(key, true, nonProtectedVariables[key]);
            }
        } else {
            addResponse(key, false, false);
        }
    }

    QJsonDocument doc2(response);
    QString gmcpMessage = doc2.toJson(QJsonDocument::Compact);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Client.Variables.Update ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    // Send response to server
    mpHost->mTelnet.socketOutRaw(output);
    qDebug() << "Sent client variables response";

    // Post message about client variable settings
    if (source == ClientVariables::SourceRequest && !response.isEmpty()) {
        const QString text = tr("\n        --> Control sharing preferences by clicking here for the Sharing tab in Settings <--\n\n\n");
        QStringList commandList;
        QStringList hintList;
        bool useCurrentFormat = true;

        commandList << "showSettingsTab(\"tab_sharing\")";
        hintList << tr("Open the Sharing tab of the Settings menu");

        const QString info1 = tr("[ INFO ]  - To customize the experience, the server requests you consent to share:");
        mpHost->mTelnet.postMessage(info1);
        const QString info2 = tr("[ INFO ]  - %1").arg(requested.join(", "));
        mpHost->mTelnet.postMessage(info2);
        mpHost->mpConsole->echoLink(text, commandList, hintList, useCurrentFormat);
    }
}

// controller for client variables
void ClientVariables::handleClientVariablesGMCP(const QString& packageMessage, const QString& data)
{
    const QString package = packageMessage.toLower(); // Don't change original variable

    if (package == qsl("client.variables.default")) {
        sendClientVariablesList();
    } else if (package == qsl("client.variables.request")) {
        sendClientVariablesUpdate(data, ClientVariables::SourceRequest);
    } else if (package == qsl("client.variables.set")) {
        sendClientVariablesUpdate(data, ClientVariables::SourceServer);
    } else {
        qDebug() << "Unknown GMCP client variables package:" << packageMessage;
    }
}
