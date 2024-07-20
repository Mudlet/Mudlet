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

#pragma once

#include "Host.h"
#include "utils.h"

#include "pre_guard.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
#include "post_guard.h"


class ClientVariables : public QObject
{
    Q_OBJECT
 
    public:
        Q_DISABLE_COPY(ClientVariables)
        explicit ClientVariables(Host* pHost);
        ~ClientVariables() = default;

        enum Source {
            SourceRequest = 0,
            SourceServer = 1,
            SourceClient = 2};

        void sendClientVariablesUpdate(const QString& data, ClientVariables::Source source);
        void handleClientVariablesGMCP(const QString& packageMessage, const QString& data);

    private:
        QMap<QString, bool> mnesVariablesMap() const {
            return {
                {"CHARSET", true},
                {"CLIENT_NAME", false},
                {"CLIENT_VERSION", false},
                {"MTTS", false},
                {"TERMINAL_TYPE", false}
            };
        }
        QMap<QString, bool> nonMNESVariablesMap() const {
            return {
                {"ANSI", true},
                {"VT100", false},
                {"256_COLORS", false},
                {"UTF-8", false},
                {"OSC_COLOR_PALETTE", false},
                {"TRUECOLOR", false},
                {"TLS", false},
                {"WORD_WRAP", true},
            };
        }
        QMap<QString, bool> nonProtectedVariablesMap() const {
            QMap<QString, bool> nonProtectedVariables = nonMNESVariablesMap();

            for (auto it = mnesVariablesMap().constBegin(); it != mnesVariablesMap().constEnd(); ++it) {
                nonProtectedVariables.insert(it.key(), it.value());
            }

            return nonProtectedVariables;
        }
        QMap<QString, std::tuple<bool, Host::DataSharingBehaviour, QString>> protectedVariablesMap() {
            return {
                {"FONT", {false, mpHost->mShareFont, tr("font")}},
                {"FONT_SIZE", {false, mpHost->mShareFontSize, tr("font size")}},
                {"LANGUAGE", {false, mpHost->mShareLanguage, tr("language")}},
                {"SYSTEMTYPE", {false, mpHost->mShareSystemType, tr("operating system type")}},
                {"SCREEN_READER", {false, mpHost->mShareScreenReader, tr("screen reader use")}},
                {"USER", {false, mpHost->mShareUser, tr("character name")}}
            };
        }
        void sendClientVariablesList();
        void sendClientVariablesResponse(const QString& data);

        Host* mpHost;
};
