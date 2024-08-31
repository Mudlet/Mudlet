#ifndef MUDLET_CLIENTVARIABLES_H
#define MUDLET_CLIENTVARIABLES_H

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


#include "Host.h"
#include "utils.h"
#include "mudlet.h"

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

        QString getClientVariableCharset();
        QString getClientVariableClientName();
        QString getClientVariableClientVersion();
        QString getClientVariableTerminalType();
        QString getClientVariableMTTS();
        QString getClientVariableANSI();
        QString getClientVariableVT100();
        QString getClientVariable256Colors();
        QString getClientVariableUTF8();
        QString getClientVariableOSCColorPalette();
        QString getClientVariableTruecolor();
        QString getClientVariableTLS();
        QString getClientVariableWordWrap();

        QMap<QString, QPair<bool, QString>> getClientVariableDataMap();
        void resetClientVariablesRequested() { clientVariablesRequested.clear(); }
        void resetNewEnvironVariablesRequested() { newEnvironVariablesRequested.clear(); }
        QString getClientVariablesRequestedPurpose(const QString& key) { return clientVariablesRequested.contains(key) ? clientVariablesRequested[key] : QString(); }
        QString getClientVariablesTranslation(const QString& key) { 
            const auto protectedVariables = protectedVariablesMap();

            if (protectedVariables.contains(key)) {
                const auto &[updatable, behaviour, translation] = protectedVariables[key];
                return translation;
            }

            return QString(); 
        }

        bool isMNESVariable(const QString&);
        void sendIsNewEnvironValues(const QByteArray&);
        void sendAllMNESValues();
        void sendMNESValue(const QString&, const QMap<QString, QPair<bool, QString>>&);
        void sendIsMNESValues(const QByteArray&);
        void sendInfoNewEnvironValue(const QString&);

        void sendClientVariablesUpdate(const QString& data, ClientVariables::Source source);
        void handleClientVariablesGMCP(const QString& packageMessage, const QString& data);

        enum class DataSharingBehaviour {
            DoNotShare,
            Share,
            Block
        };
        Q_ENUM(DataSharingBehaviour)
    
        DataSharingBehaviour mShareLanguage = DataSharingBehaviour::DoNotShare;
        DataSharingBehaviour mShareScreenReader = DataSharingBehaviour::DoNotShare;
        DataSharingBehaviour mShareSystemType = DataSharingBehaviour::DoNotShare;
        DataSharingBehaviour mShareUser = DataSharingBehaviour::DoNotShare;
    private:
        // https://tintin.mudhalla.net/protocols/mtts/
        const int MTTS_STD_ANSI = 1; // Client supports all common ANSI color codes.
        const int MTTS_STD_VT100 = 2; // Client supports all common VT100 codes.
        const int MTTS_STD_UTF_8 = 4; // Client is using UTF-8 character encoding.
        const int MTTS_STD_256_COLORS = 8; // Client supports all 256 color codes.
        const int MTTS_STD_MOUSE_TRACKING = 16; // Client supports xterm mouse tracking.
        const int MTTS_STD_OSC_COLOR_PALETTE = 32; // Client supports the OSC color palette.
        const int MTTS_STD_SCREEN_READER = 64; // Client is using a screen reader.
        const int MTTS_STD_PROXY = 128; // Client is a proxy allowing different users to connect from the same IP address.
        const int MTTS_STD_TRUECOLOR = 256; // Client supports truecolor codes using semicolon notation.
        const int MTTS_STD_MNES = 512; // Client supports the Mud New Environment Standard for information exchange.
        const int MTTS_STD_MSLP = 1024; // Client supports the Mud Server Link Protocol for clickable link handling.
        const int MTTS_STD_SSL = 2048; // Client supports SSL for data encryption, preferably TLS 1.3 or higher.

        // https://www.rfc-editor.org/rfc/rfc1572.txt && https://tintin.mudhalla.net/protocols/mnes/
        const char NEW_ENVIRON_IS = 0;
        const char NEW_ENVIRON_SEND = 1;
        const char NEW_ENVIRON_INFO = 2;
        const char NEW_ENVIRON_VAR = 0;
        const char NEW_ENVIRON_VAL = 1;
        const char NEW_ENVIRON_ESC = 2;
        const char NEW_ENVIRON_USERVAR = 3;

        QMap<QString, bool> mnesVariablesMap() const {
            // {variable, updatable}
            return {
                {qsl("CHARSET"), true},
                {qsl("CLIENT_NAME"), false},
                {qsl("CLIENT_VERSION"), false},
                {qsl("MTTS"), false},
                {qsl("TERMINAL_TYPE"), false}
            };
        }
        QMap<QString, bool> nonMNESVariablesMap() const {
            // {variable, updatable}
            return {
                {qsl("256_COLORS"), false},
                {qsl("ANSI"), false},
                {qsl("OSC_COLOR_PALETTE"), false},
                {qsl("UTF-8"), false},
                {qsl("TLS"), false},
                {qsl("TRUECOLOR"), false},
                {qsl("VT100"), false},
                {qsl("WORD_WRAP"), false}
            };
        }
        QMap<QString, bool> nonProtectedVariablesMap() const {
            QMap<QString, bool> nonProtectedVariables = nonMNESVariablesMap();
            QMap<QString, bool> mnesVariables = mnesVariablesMap();

            for (auto it = mnesVariables.constBegin(); it != mnesVariables.constEnd(); ++it) {
                nonProtectedVariables.insert(it.key(), it.value());
            }

            // {variable, updatable}
            return nonProtectedVariables;
        }
        QMap<QString, std::tuple<bool, ClientVariables::DataSharingBehaviour, QString>> protectedVariablesMap() {
            // {variable, {updatable, behaviour, translation}}
            return {
                {qsl("LANGUAGE"), {false, mShareLanguage, tr("Language")}},
                {qsl("SCREEN_READER"), {false, mShareScreenReader, tr("Screen Reader Use")}},
                {qsl("SYSTEMTYPE"), {false, mShareSystemType, tr("Operating System Type")}},
                {qsl("USER"), {false, mShareUser, tr("Character Name")}}
            };
        }
        QByteArray prepareNewEnvironData(const QString&);
        // Protected Client Variables
        QString getClientVariableUser();
        QString getClientVariableSystemType();
        QString getClientVariableScreenReader();
        QString getClientVariableLanguage();

        void appendAllNewEnvironValues(std::string&, const bool, const QMap<QString, QPair<bool, QString>>&);
        void appendNewEnvironValue(std::string&, const QString&, const bool, const QMap<QString, QPair<bool, QString>>&);
        void sendClientVariablesList();
        bool updateClientVariable(const QString&, const QString&, QMap<QString, QPair<bool, QString>>&);

        Host* mpHost;
        QMap<QString, QString> clientVariablesRequested;
        QSet<QString> newEnvironVariablesRequested;
    };

    #endif // MUDLET_CLIENTVARIABLES_H
    