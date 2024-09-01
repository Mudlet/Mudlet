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

// escapes and encodes data to be send over NEW ENVIRON and MNES
QByteArray ClientVariables::prepareNewEnvironData(const QString &arg)
{
    QString ret = arg;

    ret.replace(TN_IAC, qsl("%1%2").arg(TN_IAC, TN_IAC));
    ret.replace(NEW_ENVIRON_ESC, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_ESC));
    ret.replace(NEW_ENVIRON_VAL, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_VAL));
    ret.replace(NEW_ENVIRON_USERVAR, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_USERVAR));
    ret.replace(NEW_ENVIRON_VAR, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_VAR));

    if (!mpHost->mTelnet.getEncoding().isEmpty() && mpHost->mTelnet.getOutgoingDataEncoder()) {
        return mpHost->mTelnet.getOutgoingDataEncoder()->fromUnicode(ret).constData();
    }

    return ret.toLatin1().constData();
}

QString ClientVariables::clientVariableUser() const
{
    return !mpHost->getLogin().isEmpty() ? mpHost->getLogin().trimmed() : QString();
}

QString ClientVariables::clientVariableSystemType() const
{
    QString systemType;

    // "SYSTEMTYPE" Inspired by https://www.rfc-editor.org/rfc/rfc1340.txt
    // Ordering redone to follow general format of TLuaInterpreter::getOs()
#if defined(Q_OS_CYGWIN)
    // Try for this one before Q_OS_WIN32 as both are likely to be defined on
    // a Cygwin platform
    systemType = qsl("CYGWIN");
#elif defined(Q_OS_WIN64)
    // This reflects the build machine rather than the run-time one and our
    // published builds are actually 32-bit ones that can run on either.
    systemType = qsl("WIN64");
#elif defined(Q_OS_WIN32)
    // This reflects the build machine rather than the run-time one and our
    // published builds are actually 32-bit ones that can run on either.
    systemType = qsl("WIN32");
#elif (defined(Q_OS_MACOS))
    systemType = qsl("MACOS");
#elif defined(Q_OS_LINUX)
    systemType = qsl("LINUX");
#elif defined(Q_OS_HURD)
    systemType = qsl("HURD");
#elif (defined(Q_OS_FREEBSD_KERNEL))
    // Defined for BOTH Debian kFreeBSD hybrid with a GNU userland and
    // main FreeBSD so it must be after a Q_OS_FREEBSD check if we needed
    // to tell the different; OTOH only a Debian packager for this, now
    // obsolete hybrid would want to worry about this!
    systemType = qsl("FREEBSD");
#elif defined(Q_OS_NETBSD)
    systemType = qsl("NETBSD");
#elif defined(Q_OS_OPENBSD)
    systemType = qsl("OPENBSD");
#elif defined(Q_OS_BSD4)
    // Generic *nix - must be before unix and after other more specific results
    systemType = qsl("BSD4");
#elif defined(Q_OS_UNIX)
    systemType = qsl("UNIX");
#endif

    return systemType.isEmpty() ? QString() : systemType;
}

QString ClientVariables::clientVariableCharset() const
{
    const QString charsetEncoding = mpHost->mTelnet.getEncoding();

    return !charsetEncoding.isEmpty() ? charsetEncoding : qsl("ASCII");
}

bool ClientVariables::setClientVariableCharset(const QVariant& value, const QVariant& currentValue) {
    QByteArray byteValue = value.toString().toUtf8();
    QByteArray encoding;
    bool updated = false;

    if (byteValue.contains("ASCII")) {
        encoding = "ASCII";
    } else if (byteValue.startsWith("ISO-")) {
        encoding = "ISO " + byteValue.mid(4);
    } else if (byteValue.startsWith("ISO") && !value.toString().startsWith("ISO ")) {
        encoding = "ISO " + byteValue.mid(3);
    } else {
        encoding = byteValue;
    }

    if (encoding != currentValue.toString().toUtf8()) {
        mpHost->mTelnet.setEncoding(encoding, true);
        updated = true;
        qDebug() << "Game changed encoding to" << encoding;
    }

    return updated;
}

QString ClientVariables::clientVariableClientName() const
{
    return qsl("MUDLET");
}

QString ClientVariables::clientVariableClientVersion() const
{
    QString clientVersion = APP_VERSION;
    static const auto allInvalidCharacters = QRegularExpression(qsl("[^A-Z,0-9,-,\\/]"));
    static const auto multipleHyphens = QRegularExpression(qsl("-{2,}"));

    if (auto build = mudlet::self()->mAppBuild; build.trimmed().length()) {
        clientVersion.append(build);
    }

    /*
    * The valid characters for termType are more restricted than being ASCII
    * from https://tools.ietf.org/html/rfc1010 (page 29):
    * "A terminal names may be up to 40 characters taken from the set of uppercase
    * letters, digits, and the two punctuation characters hyphen and slash.  It must
    * start with a letter, and end with a letter or digit."
    */
    clientVersion = clientVersion.toUpper()
                                        .replace(QChar('.'), QChar('/'))
                                        .replace(QChar::Space, QChar('-'))
                                        .replace(allInvalidCharacters, QChar('-'))
                                        .replace(multipleHyphens, QChar('-'))
                                        .left(40);

    for (int i = clientVersion.size() - 1; i >= 0; --i) {
        if (clientVersion.at(i).isLetterOrNumber()) {
            clientVersion = clientVersion.left(i + 1);
            break;
        }
    }

    return clientVersion;
}

QString ClientVariables::clientVariableTerminalType() const
{
    return qsl("ANSI-TRUECOLOR");
}

int ClientVariables::clientVariableMTTS() const
{
    int terminalStandards = MTTS_STD_ANSI|MTTS_STD_256_COLORS|MTTS_STD_OSC_COLOR_PALETTE|MTTS_STD_TRUECOLOR;

    if (mpHost->mTelnet.getEncoding() == qsl("UTF-8")) {
        terminalStandards |= MTTS_STD_UTF_8;
    }

    if (mShareScreenReader == ClientVariables::DataSharingBehaviour::Share && mpHost->mAdvertiseScreenReader) {
        terminalStandards |= MTTS_STD_SCREEN_READER;  // Needs an OPT-IN to be enabled
    }

    if (mpHost->mEnableMNES && !mpHost->mForceNewEnvironNegotiationOff) {
        terminalStandards |= MTTS_STD_MNES;
    }

#if !defined(QT_NO_SSL)
    terminalStandards |= MTTS_STD_SSL;
#endif

    return terminalStandards;
}

bool ClientVariables::clientVariableANSI() const
{
    return true;
}

bool ClientVariables::clientVariableVT100() const
{
    return false;
}

bool ClientVariables::clientVariable256Colors() const
{
    return true;
}

bool ClientVariables::clientVariableUTF8() const
{
    return mpHost->mTelnet.getEncoding() == "UTF-8";
}

bool ClientVariables::clientVariableOSCColorPalette() const
{
    return true;
}

bool ClientVariables::clientVariableScreenReader() const
{
    return mpHost->mAdvertiseScreenReader;
}

bool ClientVariables::clientVariableTruecolor() const
{
    return true;
}

bool ClientVariables::clientVariableTLS() const
{
#if !defined(QT_NO_SSL)
    return true;
#else
    return false;
#endif
}

QString ClientVariables::clientVariableLanguage() const
{
    return mudlet::self()->getInterfaceLanguage();
}

int ClientVariables::clientVariableWordWrap() const
{
    return mpHost->mWrapAt;
}

/*
 * Retrieves the current state of all client variables.
 * This function is used to generate the data map that reflects the current values and states
 * of client variables as per the GMCP Client Variables standard. It is crucial for synchronizing
 * the client's state with the server.
 */
QMap<QString, std::tuple<QString, bool, bool, QVariant>> ClientVariables::clientVariablesDataMap()
{
    QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesData;

    auto insertVariables = [&](const auto& variables, bool checkBehaviour = false) {
        for (const auto &key : variables.keys()) {
            if constexpr (std::tuple_size_v<std::decay_t<decltype(variables[key])>> == 6) {
                // Tuple with behaviour
                const auto &[type, updatable, userVar, valueFunction, behaviour, translation] = variables[key];

                if (checkBehaviour && behaviour == ClientVariables::DataSharingBehaviour::Share) {
                    clientVariablesData.insert(key, {type, updatable, userVar, valueFunction()});
                }
            } else {
                // Tuple without behaviour
                const auto &[type, updatable, userVar, valueFunction] = variables[key];
                clientVariablesData.insert(key, {type, updatable, userVar, valueFunction()});
            }
        }
    };

    // Insert MNES variables
    insertVariables(mnesVariablesMap());

    if (!mpHost->mEnableMNES) {
        // Insert non-MNES variables
        insertVariables(nonMNESVariablesMap());

        // Insert protected variables, checking behaviour
        insertVariables(protectedVariablesMap(), true);
    }

    return clientVariablesData;
}

QString ClientVariables::convertVariableValueToString(const QString &type, const QVariant &variableValue) const
{
    if (variableValue.isNull() || type == qsl("null")) {
        return QString();
    } else if (type == qsl("string") || type == qsl("number") || type == qsl("integer")) {
        return variableValue.toString();
    } else if (type == qsl("boolean")) {
        return variableValue.toBool() ? qsl("1") : qsl("0");
    } else if (type == qsl("array")) {
        QJsonArray jsonArray = variableValue.toJsonArray();
        return QString(QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
    } else if (type == qsl("object")) {
        QJsonObject jsonObject = variableValue.toJsonObject();
        return QString(QJsonDocument(jsonObject).toJson(QJsonDocument::Compact));
    }

    // Default case if type doesn't match any expected types
    return QString();
}

// SEND INFO per https://www.rfc-editor.org/rfc/rfc1572
void ClientVariables::sendInfoNewEnvironValue(const QString &var)
{
    if (!mpHost->mTelnet.isNewEnvironEnabled() || mpHost->mForceNewEnvironNegotiationOff) {
        return;
    }

    if (mpHost->mEnableMNES && !isMNESVariable(var)) {
        return;
    }

    if (!newEnvironVariablesRequested.contains(var)) {
        qDebug() << "We did not update NEW_ENVIRON" << var << "because the server did not request it yet";
        return;
    }

    const QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesData = clientVariablesDataMap();

    if (clientVariablesData.contains(var)) {
        qDebug() << "We updated NEW_ENVIRON" << var;

        const auto &[type, updatable, userVar, variableValue] = clientVariablesData.value(var);
        const bool isUserVar = !mpHost->mEnableMNES && userVar;
        const QString val = convertVariableValueToString(type, variableValue);

        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_NEW_ENVIRON;
        output += NEW_ENVIRON_INFO;
        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(var).toStdString();
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
        }

        output += TN_IAC;
        output += TN_SE;
        mpHost->mTelnet.socketOutRaw(output);

        if (mpHost->mEnableMNES) {
            if (!val.isEmpty()) {
                qDebug() << "WE inform NEW_ENVIRON (MNES) VAR" << var << "VAL" << val;
            } else {
                qDebug() << "WE inform NEW_ENVIRON (MNES) VAR" << var << "as an empty VAL";
            }
        } else if (!isUserVar) {
            if (!val.isEmpty()) {
                qDebug() << "WE inform NEW_ENVIRON VAR" << var << "VAL" << val;
            } else {
                qDebug() << "WE inform NEW_ENVIRON VAR" << var << "as an empty VAL";
            }
        } else if (!val.isEmpty()) {
            qDebug() << "WE inform NEW_ENVIRON USERVAR" << var << "VAL" << val;
        } else {
            qDebug() << "WE inform NEW_ENVIRON USERVAR" << var << "as an empty VAL";
        }
    }
}

void ClientVariables::appendAllNewEnvironValues(std::string &output, const bool isUserVar, const QMap<QString, std::tuple<QString, bool, bool, QVariant>> &clientVariablesData)
{
    for (auto it = clientVariablesData.begin(); it != clientVariablesData.end(); ++it) {
        const auto &[type, updatable, userVar, variableValue] = it.value();

        if (isUserVar != userVar) {
            continue;
        }

        const QString val = convertVariableValueToString(type, variableValue);

        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(it.key()).toStdString();
        newEnvironVariablesRequested.insert(it.key());
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
        }

        if (!isUserVar) {
            if (!val.isEmpty()) {
                qDebug() << "WE send NEW_ENVIRON VAR" << it.key() << "VAL" << val;
            } else {
                qDebug() << "WE send NEW_ENVIRON VAR" << it.key() << "as an empty VAL";
            }
        } else if (!val.isEmpty()) {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << it.key() << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << it.key() << "as an empty VAL";
        }
    }
}

void ClientVariables::appendNewEnvironValue(std::string &output, const QString &var, const bool isUserVar, const QMap<QString, std::tuple<QString, bool, bool, QVariant>> &clientVariablesData)
{
    if (clientVariablesData.contains(var)) {
        const auto &[type, updatable, userVar, variableValue] = clientVariablesData[var];
        const QString val = convertVariableValueToString(type, variableValue);

        if (isUserVar != userVar) {
            // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
            // USERVAR, or IAC SE) then that variable is undefined.
            output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
            output += prepareNewEnvironData(var).toStdString();
            newEnvironVariablesRequested.insert(var);

            if (!isUserVar) {
                qDebug() << "WE send NEW_ENVIRON VAR" << var << "with no VAL because we don't maintain it as VAR (use USERVAR!)";
            } else {
                qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "with no VAL because we don't maintain it as USERVAR (use VAR!)";
            }
        } else {
            output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
            output += prepareNewEnvironData(var).toStdString();
            newEnvironVariablesRequested.insert(var);
            output += NEW_ENVIRON_VAL;

            // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
            // variable is defined, but has no value.
            if (!val.isEmpty()) {
                output += prepareNewEnvironData(val).toStdString();

                if (!isUserVar) {
                    qDebug() << "WE send NEW_ENVIRON VAR" << var << "VAL" << val;
                } else {
                    qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "VAL" << val;
                }
            } else if (!isUserVar) {
                qDebug() << "WE send NEW_ENVIRON VAR" << var << "as an empty VAL";
            } else {
                qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "as an empty VAL";
            }
        }
    } else {
        // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
        // USERVAR, or IAC SE) then that variable is undefined.
        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(var).toStdString();

        if (!isUserVar) {
            qDebug() << "WE send NEW_ENVIRON VAR" << var << "with no VAL because we don't maintain it";
        } else {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "with no VAL because we don't maintain it";
        }
    }
}

// SEND IS per https://www.rfc-editor.org/rfc/rfc1572
void ClientVariables::sendIsNewEnvironValues(const QByteArray& payload)
{
    const QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesData = clientVariablesDataMap();

    QString transcodedMsg;

    if (mpHost->mTelnet.getOutOfBandDataIncomingCodec()) {
        // Message is encoded
        transcodedMsg = mpHost->mTelnet.getOutOfBandDataIncomingCodec()->toUnicode(payload);
    } else {
        // Message is in ASCII (though this can handle Utf-8):
        transcodedMsg = payload;
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;

    bool is_uservar = false;
    bool is_var = false;
    QString var;

    for (int i = 0; i < transcodedMsg.size(); ++i) {
        if (!i && transcodedMsg.at(i) == NEW_ENVIRON_SEND) {
            continue;
        } else if (!i) {
            return; // Invalid response;
        }

        if (transcodedMsg.at(i) == NEW_ENVIRON_VAR) {
            if (!var.isEmpty()) {
                appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesData);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesData);
            }

            is_uservar = false;
            is_var = true;
        } else if (transcodedMsg.at(i) == NEW_ENVIRON_USERVAR) {
            if (!var.isEmpty()) {
                appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesData);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesData);
            }

            is_var = false;
            is_uservar = true;
        } else {
            var.append(transcodedMsg.at(i));
        }
    }

    if (!var.isEmpty()) { // Last on the stack variable
        appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesData);
    } else if (is_var || is_uservar) { // Last on the stack VAR or USERVAR with no name
        appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesData);
    } else { // No list specified, send the entire list of defined VAR and USERVAR variables
        appendAllNewEnvironValues(output, false, clientVariablesData);
        appendAllNewEnvironValues(output, true, clientVariablesData);
    }

    output += TN_IAC;
    output += TN_SE;
    mpHost->mTelnet.socketOutRaw(output);
}

bool ClientVariables::isMNESVariable(const QString &var)
{
    return mnesVariablesMap().contains(var);
}

void ClientVariables::sendAllMNESValues()
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    const QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesData = clientVariablesDataMap();

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;

    for (auto it = clientVariablesData.begin(); it != clientVariablesData.end(); ++it) {
        const auto &[type, updatable, userVar, variableValue] = it.value();
        const QString val = convertVariableValueToString(type, variableValue);

        output += NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(it.key()).toStdString();
        newEnvironVariablesRequested.insert(it.key());
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << it.key() << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << it.key() << "as an empty VAL";
        }
    }

    output += TN_IAC;
    output += TN_SE;
    mpHost->mTelnet.socketOutRaw(output);
}

void ClientVariables::sendMNESValue(const QString &var, const QMap<QString, std::tuple<QString, bool, bool, QVariant>> &clientVariablesData)
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    if (!isMNESVariable(var)) {
        return;
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;
    output += NEW_ENVIRON_VAR;

    if (clientVariablesData.contains(var)) {
        const auto &[type, updatable, userVar, variableValue] = clientVariablesData[var];
        const QString val = convertVariableValueToString(type, variableValue);

        output += prepareNewEnvironData(var).toStdString();
        newEnvironVariablesRequested.insert(var);
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << var << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << var << "as an empty VAL";
        }
    } else {
        // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
        // USERVAR, or IAC SE) then that variable is undefined.
        output += prepareNewEnvironData(var).toStdString();
        output += NEW_ENVIRON_VAL;

        qDebug() << "WE send that we do not maintain NEW_ENVIRON (MNES) VAR" << var;
    }

    output += TN_IAC;
    output += TN_SE;
    mpHost->mTelnet.socketOutRaw(output);
}

void ClientVariables::sendIsMNESValues(const QByteArray& payload)
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    const QMap<QString, std::tuple<QString, bool, bool, QVariant>> clientVariablesData = clientVariablesDataMap();

    QString transcodedMsg;

    if (mpHost->mTelnet.getOutOfBandDataIncomingCodec()) {
        // Message is encoded
        transcodedMsg = mpHost->mTelnet.getOutOfBandDataIncomingCodec()->toUnicode(payload);
    } else {
        // Message is in ASCII (though this can handle Utf-8):
        transcodedMsg = payload;
    }

    QString var;

    for (int i = 0; i < transcodedMsg.size(); ++i) {
        if (!i && transcodedMsg.at(i) == NEW_ENVIRON_SEND) {
            continue;
        } else if (!i) {
            return; // Invalid response;
        }

        if (transcodedMsg.at(i) == NEW_ENVIRON_VAR) {
            if (!var.isEmpty()) {
                sendMNESValue(var, clientVariablesData);
                var = QString();
            }

            continue;
        }

        var.append(transcodedMsg.at(i));
    }

    if (!var.isEmpty()) { // Last variable on the stack
        sendMNESValue(var, clientVariablesData);
        return;
    }

    sendAllMNESValues(); // No list specified or only a VAR, send the entire list of defined VAR variables
}

/*
 * Sends a list of all client variables to the server as required by the GMCP Client Variables standard.
 * This function is typically called during the initialization phase or when the server requests the list
 * of variables currently available on the client. The list includes the name, type, availability, and
 * updatability of each variable.
 */
void ClientVariables::sendClientVariablesList() {
    if (!mpHost->mEnableGMCP) {
        return;
    }

    QJsonArray clientVariablesList;

    // Helper lambda to add variables to the list
    auto addVariablesToList = [&](const auto& variables, bool checkBehaviour = false) {
        for (const auto& key : variables.keys()) {
            if constexpr (std::tuple_size_v<std::decay_t<decltype(variables[key])>> == 6) {
                // Tuple with behaviour
                const auto &[type, updatable, userVar, variableValue, behaviour, translation] = variables[key];

                if (checkBehaviour && behaviour == ClientVariables::DataSharingBehaviour::Block) {
                    continue;
                }

                QJsonObject variable;
                variable[qsl("name")] = key;
                variable[qsl("type")] = type;
                variable[qsl("available")] = (checkBehaviour ? (behaviour == ClientVariables::DataSharingBehaviour::Share) : true);
                variable[qsl("updatable")] = updatable;
                clientVariablesList.append(variable);
            } else {
                // Tuple without behaviour
                const auto &[type, updatable, userVar, variableValue] = variables[key];
                QJsonObject variable;
                variable[qsl("name")] = key;
                variable[qsl("type")] = type;
                variable[qsl("available")] = true;
                variable[qsl("updatable")] = updatable;
                clientVariablesList.append(variable);
            }
        }
    };

    // Add MNES variables
    addVariablesToList(mnesVariablesMap());

    if (!mpHost->mEnableMNES) {
        // Add non-MNES variables
        addVariablesToList(nonMNESVariablesMap());

        // Add protected variables, checking behaviour
        addVariablesToList(protectedVariablesMap(), true);
    }

    if (!clientVariablesList.isEmpty()) {
        QJsonDocument doc(clientVariablesList);
        QString gmcpMessage = "Client.Variables.List " + doc.toJson(QJsonDocument::Compact);

        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_GMCP;
        output += mpHost->mTelnet.encodeAndCookBytes(gmcpMessage.toStdString());
        output += TN_IAC;
        output += TN_SE;

        // Send variables to server
        mpHost->mTelnet.socketOutRaw(output);
        qDebug() << "Sent client variables list";
    }
}

bool ClientVariables::setClientVariable(const QString& key, const QVariant& value, QMap<QString, std::tuple<QString, bool, bool, QVariant>>& clientVariablesData) {
    bool updated = false;

    if (key == qsl("CHARSET")) {
        const auto variableValue = std::get<ClientVariables::TupleValue>(clientVariablesData[key]);
        updated = setClientVariableCharset(value, variableValue);
    }

    return updated;
}

QJsonValue ClientVariables::convertValueToJson(const QVariant& value) {
    if (value.typeName() == qsl("QString")) {
        return value.toString();
    } else if (value.typeName() == qsl("double")) {
        return value.toDouble();
    } else if (value.typeName() == qsl("int")) {
        return value.toInt();
    } else if (value.typeName() == qsl("bool")) {
        return value.toBool();
    } else if (value.typeName() == qsl("QJsonArray")) {
        return value.toJsonArray();
    } else if (value.typeName() == qsl("QJsonObject")) {
        return value.toJsonObject();
    }

    return QJsonValue();
}

/*
 * Sends updates of client variables to the server as specified by the GMCP Client Variables standard.
 * This function is called when client variables have been modified and need to be synchronized with the server.
 * It constructs a GMCP-compliant message containing the updated variables, their values, and additional metadata.
 */
void ClientVariables::sendClientVariablesUpdate(const QString& data, ClientVariables::Source source) {
    if (source == ClientVariables::SourceClient) {
        sendInfoNewEnvironValue(data);
    }

    if (!mpHost->mEnableGMCP) {
        return;
    }

    QJsonArray response;
    QMap<QString, QString> requested;

    const auto nonProtectedVariables = nonProtectedVariablesMap();
    const auto protectedVariables = protectedVariablesMap();
    auto clientVariablesData = clientVariablesDataMap(); // client variable values
    QStringList sources = {qsl("request"), qsl("server"), qsl("client")};  // initiated by
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();  // current timestamp

    auto addResponse = [&](const QString& key, bool available, bool updatable, bool requested = false, const QVariant& value = QVariant(), const QString& purpose = QString()) {
        QJsonObject obj{
            {qsl("name"), key},
            {qsl("available"), available},
            {qsl("updatable"), updatable},
            {qsl("source"), sources[source]},
            {qsl("timestamp"), timestamp}
        };

        if (source == ClientVariables::SourceRequest) {
            clientVariablesRequested.insert(key, purpose);
            obj[qsl("requested")] = requested;
        }

        if (!value.isNull()) {
            const auto variableValue = std::get<ClientVariables::TupleValue>(clientVariablesData[key]);
    
            if (source == ClientVariables::SourceServer) {
                bool successful = false;

                if (setClientVariable(key, value, clientVariablesData)) {
                    clientVariablesData = clientVariablesDataMap(); // refresh
                }

                successful = (available && updatable && value == variableValue);

                obj[qsl("success")] = successful;
                obj[qsl("value")] = successful ? convertValueToJson(variableValue) : convertValueToJson(value);
            } else if (available) {
                obj[qsl("value")] = convertValueToJson(variableValue);             
            }
        }

        response.append(obj);
    };

    QString process;

    if (source == ClientVariables::SourceClient) {
        process = qsl("[{\"name\" : \"%1\"}]").arg(data);
    } else {
        process = data;
    }

    auto doc = QJsonDocument::fromJson(process.toUtf8());

    if (!doc.isArray()) {
        qWarning() << "ClientVariables::sendClientVariablesUpdate: Invalid or unsupported JSON data";
        return;
    }

    QJsonArray jsonArray = doc.array();

    for (const QJsonValue& jsonValue : jsonArray) {
        if (!jsonValue.isObject()) {
            qWarning() << "ClientVariables::sendClientVariablesUpdate: Array element is not an object";
            return;             
        }

        QJsonObject jsonObj = jsonValue.toObject();
        QString key = jsonObj.value("name").toString();
        QString purpose = jsonObj.value("purpose").toString().left(250); // Trim purpose to 250 characters
        QString value = jsonObj.value("value").toString();

        if (key.isEmpty()) {
            qWarning() << "ClientVariables::sendClientVariablesUpdate: Missing or invalid 'name' field";
            continue;
        }

        if (source != ClientVariables::SourceRequest && !clientVariablesRequested.contains(key)) {
            qDebug() << "We did not update client variable" << key << "because the server did not request it yet";
            continue;
        }

        bool available = !mpHost->mEnableMNES; // Default availability state

        if (protectedVariables.contains(key)) {
            const auto &[type, updatable, userVar, variableValue, behaviour, translation] = protectedVariables[key];

            if (behaviour == ClientVariables::DataSharingBehaviour::Block) {
                addResponse(key, false, false);
            } else {
                available = (behaviour == ClientVariables::DataSharingBehaviour::Share);

                if (!available && source == ClientVariables::SourceRequest) {
                    requested.insert(translation, purpose);
                }

                if (clientVariablesData.contains(key)) {
                    addResponse(key, available, updatable, !available, value);
                } else {
                    addResponse(key, available, updatable, !available, value, purpose);
                }
            }
        } else if (nonProtectedVariables.contains(key)) {
            const auto &[type, updatable, userVar, variableValue] = nonProtectedVariables[key];

            if (clientVariablesData.contains(key)) {
                addResponse(key, true, updatable, false, value);
            } else {
                addResponse(key, true, updatable);
            }
        } else {
            addResponse(key, false, false);
        }
    }

    if (source == ClientVariables::SourceClient && response.isEmpty()) {
        return;
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

    // Post message about client variable settings
    if (source == ClientVariables::SourceRequest && !requested.isEmpty()) {
        const QString text = tr(">> Click here for the Data tab in Settings to control sharing preferences");
        QStringList commandList;
        QStringList hintList;
        bool useCurrentFormat = true;

        commandList << "showSettingsTab(\"tab_data\")";
        hintList << tr("Open the Data tab of the Settings menu");

        QStringList actionList;

        actionList << tr("[ ALERT ] - To enhance gameplay, the server or script is requesting the following:\n");

        for (auto i = requested.cbegin(), end = requested.cend(); i != end; ++i) {
            actionList << tr("   Data: %1").arg(i.key());

            if (!i.value().isEmpty()) {
                actionList << tr("Purpose: %1").arg(i.value());
            }
        }

        mpHost->mTelnet.postMessage(actionList.join("\n"));
        mpHost->mTelnet.postMessage("\n");
        mpHost->mpConsole->echoLink(text, commandList, hintList, useCurrentFormat);
        mpHost->mTelnet.postMessage("\n ");
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
