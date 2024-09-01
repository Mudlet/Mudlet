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

    enum TupleIndex {
        TupleType = 0,
        TupleUpdatable = 1,
        TupleUserVar = 2,
        TupleValue = 3,
        TupleBehaviour = 4,
        TupleTranslation = 5};
    
    QString clientVariableCharset() const;
    QString clientVariableClientName() const;
    QString clientVariableClientVersion() const;
    QString clientVariableTerminalType() const;
    int clientVariableMTTS() const;
    bool clientVariableANSI() const;
    bool clientVariableVT100() const;
    bool clientVariable256Colors() const;
    bool clientVariableUTF8() const;
    bool clientVariableOSCColorPalette() const;
    bool clientVariableTruecolor() const;
    bool clientVariableTLS() const;
    int clientVariableWordWrap() const;

    QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesDataMap();
    void resetClientVariablesRequested() { clientVariablesRequested.clear(); }
    void resetNewEnvironVariablesRequested() { newEnvironVariablesRequested.clear(); }
    QString clientVariablesRequestedPurpose(const QString& key) { return clientVariablesRequested.contains(key) ? clientVariablesRequested[key] : QString(); }
    QString clientVariablesTranslation(const QString& key) { 
        const auto protectedVariables = protectedVariablesMap();

        if (protectedVariables.contains(key)) {
            return std::get<ClientVariables::TupleTranslation>(protectedVariables[key]);
        }

        return QString(); 
    }

    bool isMNESVariable(const QString&);
    void sendIsNewEnvironValues(const QByteArray&);
    void sendAllMNESValues();
    void sendMNESValue(const QString&, const QMap<QString, std::tuple<QString, bool, bool, QVariant>>&);
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

    QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>>> mnesVariablesMap() const {
        // Per https://tintin.mudhalla.net/protocols/mnes/, the variables are limited to the following only.
        // * These will be be requested with NEW_ENVIRON_VAR for the MNES protocol
        // * "IPADDRESS" Intentionally not implemented by Mudlet Makers
        // * These will be used by NEW_ENVIRON as well and be requested with NEW_ENVIRON_USERVAR

        // {variable, {type, updatable, userVar, valueFunction}}
        return {
            {qsl("CHARSET"), {qsl("string"), true, false, [this]() { return QVariant(clientVariableCharset()); }}},
            {qsl("CLIENT_NAME"), {qsl("string"), false, false, [this]() { return QVariant(clientVariableClientName()); }}},
            {qsl("CLIENT_VERSION"), {qsl("string"), false, false, [this]() { return QVariant(clientVariableClientVersion()); }}},
            {qsl("MTTS"), {qsl("integer"), false, false, [this]() { return QVariant(clientVariableMTTS()); }}},
            {qsl("TERMINAL_TYPE"), {qsl("string"), false, false, [this]() { return QVariant(clientVariableTerminalType()); }}}
        };
    }

    QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>>> nonMNESVariablesMap() const {
        // Per https://www.rfc-editor.org/rfc/rfc1572.txt, these will be requested with NEW_ENVIRON_USERVAR

        // {variable, {type, updatable, userVar, valueFunction}}
        return {
            {qsl("256_COLORS"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariable256Colors()); }}},
            {qsl("ANSI"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableANSI()); }}},
            {qsl("OSC_COLOR_PALETTE"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableOSCColorPalette()); }}},
            {qsl("UTF-8"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableUTF8()); }}},
            {qsl("TLS"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableTLS()); }}},
            {qsl("TRUECOLOR"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableTruecolor()); }}},
            {qsl("VT100"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableVT100()); }}},
            {qsl("WORD_WRAP"), {qsl("integer"), false, false, [this]() { return QVariant(clientVariableWordWrap()); }}}
        };
    }

    QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>>> nonProtectedVariablesMap() const {
        QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>>> nonProtectedVariables = nonMNESVariablesMap();
        QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>>> mnesVariables = mnesVariablesMap();

        for (auto it = mnesVariables.constBegin(); it != mnesVariables.constEnd(); ++it) {
            nonProtectedVariables.insert(it.key(), it.value());
        }

        // {variable, {type, updatable, userVar, valueFunction}}
        return nonProtectedVariables;
    }

    QMap<QString, std::tuple<QString, bool, bool, std::function<QVariant()>, ClientVariables::DataSharingBehaviour, QString>> protectedVariablesMap() {
        // Per https://www.rfc-editor.org/rfc/rfc1572.txt, "SYSTEMTYPE" is well-known and will be requested with NEW_ENVIRON_VAR
        // Per https://www.rfc-editor.org/rfc/rfc1572.txt, "USER" is well-known and will be requested with NEW_ENVIRON_VAR

        // {variable, {type, updatable, userVar, valueFunction, behaviour, translation}}
        return {
            {qsl("LANGUAGE"), {qsl("string"), false, false, [this]() { return QVariant(clientVariableLanguage()); }, mShareLanguage, tr("Language")}},
            {qsl("SCREEN_READER"), {qsl("boolean"), false, false, [this]() { return QVariant(clientVariableScreenReader()); }, mShareScreenReader, tr("Screen Reader Use")}},
            {qsl("SYSTEMTYPE"), {qsl("string"), false, true, [this]() { return QVariant(clientVariableSystemType()); }, mShareSystemType, tr("Operating System Type")}},
            {qsl("USER"), {qsl("string"), false, true, [this]() { return QVariant(clientVariableUser()); }, mShareUser, tr("Character Name")}}
        };
    }

    QString convertVariableValueToString(const QString &, const QVariant &) const;
    QByteArray prepareNewEnvironData(const QString&);
    bool setClientVariableCharset(const QVariant&, const QVariant&);

    // Protected Client Variables
    QString clientVariableUser() const;
    QString clientVariableSystemType() const;
    bool clientVariableScreenReader() const;
    QString clientVariableLanguage() const;

    void appendAllNewEnvironValues(std::string&, const bool, const QMap<QString, std::tuple<QString, bool, bool, QVariant>>&);
    void appendNewEnvironValue(std::string&, const QString&, const bool, const QMap<QString, std::tuple<QString, bool, bool, QVariant>>&);
    void sendClientVariablesList();

    bool setClientVariable(const QString&, const QVariant&, QMap<QString, std::tuple<QString, bool, bool, QVariant>>&);
    QJsonValue convertValueToJson(const QVariant&);

    Host* mpHost;
    QMap<QString, QString> clientVariablesRequested;
    QSet<QString> newEnvironVariablesRequested;
};

#endif // MUDLET_CLIENTVARIABLES_H
