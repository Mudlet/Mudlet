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

#include "TClientVariables.h"

#include "Host.h"
#include "ctelnet.h"
#include <QDebug>

TClientVariables::TClientVariables(Host* pHost) : mpHost(pHost) {}

void TClientVariables::sendClientVariablesList()
{
    QJsonObject variables;
 
    QSet<QString> mnesVariables{"CHARSET", "CLIENT_NAME", "CLIENT_VERSION", "MTTS", "TERMINAL_TYPE"};
    QSet<QString> otherDirectVariables{"ANSI", "VT100", "256_COLORS", "UTF-8", "OSC_COLOR_PALETTE", "TRUECOLOR", "TLS", "WORD_WRAP"};

    for (const auto &varName : mnesVariables) {
        variables[varName] = true;
    }

    if (!mpHost->mEnableMNES) {
        for (const auto &varName : otherDirectVariables) {
            variables[varName] = true;
        }

        QMap<QString, std::function<Host::DataSharingBehaviour()>> specialRequestVariables{
            {"FONT", [this] { return mpHost->mShareFont; }},
            {"FONT_SIZE", [this] { return mpHost->mShareFontSize; }},
            {"LANGUAGE", [this] { return mpHost->mShareLanguage; }},
            {"SYSTEMTYPE", [this] { return mpHost->mShareSystemType; }},
            {"SCREEN_READER", [this] { return mpHost->mShareScreenReader; }},
            {"USER", [this] { return mpHost->mShareUser; }}
        };

        for (const auto &varName : specialRequestVariables.keys()) { // Needs an OPT-IN to be available
            Host::DataSharingBehaviour behaviour = specialRequestVariables[varName]();

            if (behaviour != Host::DataSharingBehaviour::Block) {
                variables[varName] = (behaviour == Host::DataSharingBehaviour::OptIn ? true : false);
            }
        }
    }

    QJsonDocument doc(variables);
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

void TClientVariables::sendClientVariablesResponse(const QString& data) {
    QJsonObject response;
    QStringList requested;
    QSet<QString> directVariables{"CHARSET", "CLIENT_NAME", "CLIENT_VERSION", "MTTS", "TERMINAL_TYPE", "ANSI", "VT100", "256_COLORS", "UTF-8", "OSC_COLOR_PALETTE", "TRUECOLOR", "TLS", "WORD_WRAP"};
    QMap<QString, std::function<Host::DataSharingBehaviour()>> specialRequestVariables{
        {"FONT", [this] { return mpHost->mShareFont; }},
        {"FONT_SIZE", [this] { return mpHost->mShareFontSize; }},
        {"LANGUAGE", [this] { return mpHost->mShareLanguage; }},
        {"SYSTEMTYPE", [this] { return mpHost->mShareSystemType; }},
        {"SCREEN_READER", [this] { return mpHost->mShareScreenReader; }},
        {"USER", [this] { return mpHost->mShareUser; }}
    };
    QMap<QString, QString> specialRequestTranslations{
        {"USER", tr("character name")},
        {"SYSTEMTYPE", tr("operating system type")},
        {"SCREEN_READER", tr("screen reader use")},
        {"LANGUAGE", tr("language")},
        {"FONT", tr("font")},
        {"FONT_SIZE", tr("font size")}
    };

    auto doc = QJsonDocument::fromJson(data.toUtf8());
    auto array = doc.array();

    for (const QJsonValue& value : array) {
        if (value.isString()) {
            QString stringValue = value.toString();
            bool available = !mpHost->mEnableMNES; // Default availability state
            bool isRequested = specialRequestVariables.contains(stringValue) || directVariables.contains(stringValue);

            if (specialRequestVariables.contains(stringValue)) {
                Host::DataSharingBehaviour behaviour = specialRequestVariables[stringValue]();

                if (behaviour == Host::DataSharingBehaviour::Block) {
                    response[stringValue] = QJsonObject{{"requested", false}, {"available", available}};                
                }

                available = (behaviour == Host::DataSharingBehaviour::OptIn ? true : false);

                if (!available) {
                    requested << specialRequestTranslations[stringValue];
                }

                response[stringValue] = QJsonObject{{"requested", true}, {"available", available}};
            } else if (directVariables.contains(stringValue)) {
                response[stringValue] = QJsonObject{{"requested", false}, {"available", available}};
            }
        }
    }

    QJsonDocument doc2(response);
    QString gmcpMessage = doc2.toJson(QJsonDocument::Compact);

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Client.Variables.Response ";
    output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
    output += TN_IAC;
    output += TN_SE;

    // Send response to server
    mpHost->mTelnet.socketOutRaw(output);
    qDebug() << "Sent client variables response";

    // Post message about client variable settings
    if (!response.isEmpty()) {
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

void TClientVariables::sendClientVariableInfo(const QString& variable) {
    QJsonObject info;

    QMap<QString, std::function<Host::DataSharingBehaviour()>> specialRequestVariables{
        {"FONT", [this] { return mpHost->mShareFont; }},
        {"FONT_SIZE", [this] { return mpHost->mShareFontSize; }},
        {"LANGUAGE", [this] { return mpHost->mShareLanguage; }},
        {"SCREEN_READER", [this] { return mpHost->mShareScreenReader; }},
        {"SYSTEMTYPE", [this] { return mpHost->mShareSystemType; }},
        {"USER", [this] { return mpHost->mShareUser; }}
    };

    Host::DataSharingBehaviour behaviour = specialRequestVariables[variable](); // Needs an OPT-IN to be available

    if (behaviour == Host::DataSharingBehaviour::OptIn) {
        const QMap<QString, QPair<bool, QString>> newEnvironDataMap = getNewEnvironDataMap();

        if (newEnvironDataMap.contains(var)) {
            // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
            const QPair<bool, QString> newEnvironData = newEnvironDataMap.value(var);
            const QString val = newEnvironData.second;

            info[variable] = QJsonObject{{"available", true}, {"value", val}};
        } else {
            info[variable] = QJsonObject{{"available", true}};
        }
    }

    if (!info.isEmpty()) {
        QJsonDocument doc(info);
        QString gmcpMessage = doc.toJson(QJsonDocument::Compact);

        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_GMCP;
        output += "Client.Variables.Info ";
        output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
        output += TN_IAC;
        output += TN_SE;

        // Send variables to server
        mpHost->mTelnet.socketOutRaw(output);
        qDebug() << "Sent client variables info";
    }
}

// controller for client variables
void TClientVariables::handleClientVariablesGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Client.Variables.Default")) {
        sendClientVariablesList();
    } else if (packageMessage == qsl("Client.Variables.Request")) {
        sendClientVariablesResponse(data);
    } else {
        qDebug() << "Unknown GMCP client variables package:" << packageMessage;
    }
}
