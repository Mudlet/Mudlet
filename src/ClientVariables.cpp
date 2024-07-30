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

QString ClientVariables::getClientVariableUser()
{
    return !mpHost->getLogin().isEmpty() ? mpHost->getLogin().trimmed() : QString();
}

QString ClientVariables::getClientVariableSystemType()
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

QString ClientVariables::getClientVariableBoldIsBright()
{
    return qsl("1"); // 4.17.2
    //return qsl("%1").arg(mpHost->mBoldIsBright);
}

QString ClientVariables::getClientVariableCharset()
{
    const QString charsetEncoding = mpHost->mTelnet.getEncoding();

    return !charsetEncoding.isEmpty() ? charsetEncoding : qsl("ASCII");
}

QString ClientVariables::getClientVariableClientName()
{
    return qsl("MUDLET");
}

QString ClientVariables::getClientVariableClientVersion()
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

QString ClientVariables::getClientVariableTerminalType()
{
    return qsl("ANSI-TRUECOLOR");
}

QString ClientVariables::getClientVariableMTTS()
{
    int terminalStandards = MTTS_STD_ANSI|MTTS_STD_256_COLORS|MTTS_STD_OSC_COLOR_PALETTE|MTTS_STD_TRUECOLOR;

    if (mpHost->mTelnet.getEncoding() == "UTF-8") {
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

    return qsl("%1").arg(terminalStandards);
}

QString ClientVariables::getClientVariableANSI()
{
    return qsl("1");
}

QString ClientVariables::getClientVariableVT100()
{
    return QString("0");
}

QString ClientVariables::getClientVariable256Colors()
{
    return qsl("1");
}

QString ClientVariables::getClientVariableUTF8()
{
    return mpHost->mTelnet.getEncoding() == "UTF-8" ? qsl("1") : QString();
}

QString ClientVariables::getClientVariableOSCColorPalette()
{
    return qsl("1");
}

QString ClientVariables::getClientVariableScreenReader()
{
    return mpHost->mAdvertiseScreenReader ? qsl("1") : qsl("0");
}

QString ClientVariables::getClientVariableTruecolor()
{
    return qsl("1");
}

QString ClientVariables::getClientVariableTLS()
{
#if !defined(QT_NO_SSL)
    return qsl("1");
#else
    return qsl("0");
#endif
}

QString ClientVariables::getClientVariableLanguage()
{
    return mudlet::self()->getInterfaceLanguage();
}

QString ClientVariables::getClientVariableFont()
{
    return mpHost->getDisplayFont().family();
}

QString ClientVariables::getClientVariableFontSize()
{
    return qsl("%1").arg(mpHost->getDisplayFont().pointSize());
}

QString ClientVariables::getClientVariableWordWrap()
{
    return qsl("%1").arg(mpHost->mWrapAt);
}

QMap<QString, QPair<bool, QString>> ClientVariables::getClientVariableDataMap()
{
    QMap<QString, QPair<bool, QString>> clientVariablesDataMap;
    const bool isUserVar = true;

    // Per https://tintin.mudhalla.net/protocols/mnes/, the variables are limited to the following only.
    // * These will be be requested with NEW_ENVIRON_VAR for the MNES protocol
    // * "IPADDRESS" Intentionally not implemented by Mudlet Makers
    // * These will be used by NEW_ENVIRON as well and be requested with NEW_ENVIRON_USERVAR
    clientVariablesDataMap.insert(qsl("CHARSET"), qMakePair(isUserVar, getClientVariableCharset()));
    clientVariablesDataMap.insert(qsl("CLIENT_NAME"), qMakePair(isUserVar, getClientVariableClientName()));
    clientVariablesDataMap.insert(qsl("CLIENT_VERSION"), qMakePair(isUserVar, getClientVariableClientVersion()));
    clientVariablesDataMap.insert(qsl("MTTS"), qMakePair(isUserVar, getClientVariableMTTS()));
    clientVariablesDataMap.insert(qsl("TERMINAL_TYPE"), qMakePair(isUserVar, getClientVariableTerminalType()));

    if (mpHost->mEnableMNES) {
        return clientVariablesDataMap;
    }

    // Per https://www.rfc-editor.org/rfc/rfc1572.txt, these will be requested with NEW_ENVIRON_USERVAR
    clientVariablesDataMap.insert(qsl("ANSI"), qMakePair(isUserVar, getClientVariableANSI()));
    clientVariablesDataMap.insert(qsl("VT100"), qMakePair(isUserVar, getClientVariableVT100()));
    clientVariablesDataMap.insert(qsl("256_COLORS"), qMakePair(isUserVar, getClientVariable256Colors()));
    clientVariablesDataMap.insert(qsl("UTF-8"), qMakePair(isUserVar, getClientVariableUTF8()));
    clientVariablesDataMap.insert(qsl("OSC_COLOR_PALETTE"), qMakePair(isUserVar, getClientVariableOSCColorPalette()));
    clientVariablesDataMap.insert(qsl("TRUECOLOR"), qMakePair(isUserVar, getClientVariableTruecolor()));
    clientVariablesDataMap.insert(qsl("TLS"), qMakePair(isUserVar, getClientVariableTLS()));
    clientVariablesDataMap.insert(qsl("WORD_WRAP"), qMakePair(isUserVar, getClientVariableWordWrap()));
    clientVariablesDataMap.insert(qsl("BOLD_IS_BRIGHT"), qMakePair(isUserVar, getClientVariableBoldIsBright()));

    if (mShareFont == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("FONT"), qMakePair(isUserVar, getClientVariableFont()));
    }

    if (mShareFontSize == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("FONT_SIZE"), qMakePair(isUserVar, getClientVariableFontSize()));
    }

    if (mShareLanguage == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("LANGUAGE"), qMakePair(isUserVar, getClientVariableLanguage()));
    }

    if (mShareScreenReader == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("SCREEN_READER"), qMakePair(isUserVar, getClientVariableScreenReader()));
    }

    // Per https://www.rfc-editor.org/rfc/rfc1572.txt, "SYSTEMTYPE" is well-known and will be requested with NEW_ENVIRON_VAR
    if (mShareSystemType == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("SYSTEMTYPE"), qMakePair(!isUserVar, getClientVariableSystemType()));
    }

    // Per https://www.rfc-editor.org/rfc/rfc1572.txt, "USER" is well-known and will be requested with NEW_ENVIRON_VAR
    if (mShareUser == ClientVariables::DataSharingBehaviour::Share) {
        clientVariablesDataMap.insert(qsl("USER"), qMakePair(!isUserVar, getClientVariableUser()));
    }

    return clientVariablesDataMap;
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

    const QMap<QString, QPair<bool, QString>> clientVariablesDataMap = getClientVariableDataMap();

    if (clientVariablesDataMap.contains(var)) {
        qDebug() << "We updated NEW_ENVIRON" << var;

        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = clientVariablesDataMap.value(var);
        const bool isUserVar = !mpHost->mEnableMNES && newEnvironData.first;
        const QString val = newEnvironData.second;

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

void ClientVariables::appendAllNewEnvironValues(std::string &output, const bool isUserVar, const QMap<QString, QPair<bool, QString>> &clientVariablesDataMap)
{
    for (auto it = clientVariablesDataMap.begin(); it != clientVariablesDataMap.end(); ++it) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = it.value();

        if (isUserVar != newEnvironData.first) {
            continue;
        }

        const QString val = newEnvironData.second;

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

void ClientVariables::appendNewEnvironValue(std::string &output, const QString &var, const bool isUserVar, const QMap<QString, QPair<bool, QString>> &clientVariablesDataMap)
{
    if (clientVariablesDataMap.contains(var)) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = clientVariablesDataMap.value(var);
        const QString val = newEnvironData.second;

        if (newEnvironData.first != isUserVar) {
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
    const QMap<QString, QPair<bool, QString>> clientVariablesDataMap = getClientVariableDataMap();

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
                appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesDataMap);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesDataMap);
            }

            is_uservar = false;
            is_var = true;
        } else if (transcodedMsg.at(i) == NEW_ENVIRON_USERVAR) {
            if (!var.isEmpty()) {
                appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesDataMap);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesDataMap);
            }

            is_var = false;
            is_uservar = true;
        } else {
            var.append(transcodedMsg.at(i));
        }
    }

    if (!var.isEmpty()) { // Last on the stack variable
        appendNewEnvironValue(output, var, (is_uservar ? true : false), clientVariablesDataMap);
    } else if (is_var || is_uservar) { // Last on the stack VAR or USERVAR with no name
        appendAllNewEnvironValues(output, (is_uservar ? true : false), clientVariablesDataMap);
    } else { // No list specified, send the entire list of defined VAR and USERVAR variables
        appendAllNewEnvironValues(output, false, clientVariablesDataMap);
        appendAllNewEnvironValues(output, true, clientVariablesDataMap);
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

    const QMap<QString, QPair<bool, QString>> clientVariablesDataMap = getClientVariableDataMap();

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;

    for (auto it = clientVariablesDataMap.begin(); it != clientVariablesDataMap.end(); ++it) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = it.value();
        const QString val = newEnvironData.second;

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

void ClientVariables::sendMNESValue(const QString &var, const QMap<QString, QPair<bool, QString>> &clientVariablesDataMap)
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

    if (clientVariablesDataMap.contains(var)) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = clientVariablesDataMap.value(var);
        const QString val = newEnvironData.second;

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

    const QMap<QString, QPair<bool, QString>> clientVariablesDataMap = getClientVariableDataMap();

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
                sendMNESValue(var, clientVariablesDataMap);
                var = QString();
            }

            continue;
        }

        var.append(transcodedMsg.at(i));
    }

    if (!var.isEmpty()) { // Last variable on the stack
        sendMNESValue(var, clientVariablesDataMap);
        return;
    }

    sendAllMNESValues(); // No list specified or only a VAR, send the entire list of defined VAR variables
}

void ClientVariables::sendClientVariablesList() {
    if (!mpHost->mEnableGMCP) {
        return;
    }

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

            if (behaviour != ClientVariables::DataSharingBehaviour::Block) {
                const bool available = (behaviour == ClientVariables::DataSharingBehaviour::Share);
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

bool ClientVariables::updateClientVariable(const QString& key, const QString& value, QMap<QString, QPair<bool, QString>>& clientVariablesDataMap) {
    bool updated = false;

    if (key == "CHARSET") {
        bool updated = false;
        QByteArray byteValue = value.toUtf8();
        QByteArray encoding;

        if (byteValue.contains(QByteArray("ASCII"))) {
            encoding = QByteArray("ASCII");

            if (encoding != clientVariablesDataMap[key].second.toUtf8()) {
                mpHost->mTelnet.setEncoding(encoding, true); // Force variants of ASCII to ASCII
                updated = true;
            }
        } else if (byteValue.startsWith("ISO-")) {
            encoding = QByteArray("ISO " + byteValue.mid(4));

            if (encoding != clientVariablesDataMap[key].second.toUtf8()) {
                mpHost->mTelnet.setEncoding(encoding, true); // Align with TEncodingTable::csmEncodings
                updated = true;
            }
        } else if (byteValue.startsWith("ISO") && !value.startsWith("ISO ")) {
            encoding = QByteArray("ISO " + byteValue.mid(3));

            if (encoding != clientVariablesDataMap[key].second.toUtf8()) {
                mpHost->mTelnet.setEncoding(encoding, true); // Align with TEncodingTable::csmEncodings
                updated = true;
            }
        } else {
            encoding = byteValue;

            if (encoding != clientVariablesDataMap[key].second.toUtf8()) {
                mpHost->mTelnet.setEncoding(encoding, true);
                updated = true;
            }
        }

        if (updated) {
            qDebug() << "Game changed encoding to" << encoding;
        }
    } else if (key == "BOLD_IS_BRIGHT") {
        bool updated = false;

        if (value != clientVariablesDataMap[key].second) {
            if (value.toInt() == Qt::Unchecked) {
                //mpHost->mBoldIsBright = Qt::Unchecked;
                updated = true;
            } else if (value.toInt() == Qt::Checked) {
                //mpHost->mBoldIsBright = Qt::Checked;
                updated = true;
            } else {
                //mpHost->mBoldIsBright = Qt::PartiallyChecked;
                updated = true;
            }
        }

        if (updated) {
            qDebug() << "Game changed boldIsBright to" << value;
        }
    }

    return updated;
}

void ClientVariables::sendClientVariablesUpdate(const QString& data, ClientVariables::Source source) {
    if (!mpHost->mEnableGMCP) {
        return;
    }

    QJsonObject response;
    QStringList requested;

    const auto nonProtectedVariables = nonProtectedVariablesMap();
    const auto protectedVariables = protectedVariablesMap();
    auto clientVariablesDataMap = getClientVariableDataMap(); // client variable values
    QStringList sources = {"request", "server", "client"};  // initiated by
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();  // current timestamp

    auto addResponse = [&](const QString& key, bool available, bool updatable, bool requested = false, const QString& value = QString()) {
        QJsonObject obj{
            {"available", available},
            {"updatable", updatable}
        };

        if (source == ClientVariables::SourceRequest) {
            clientVariablesRequested.insert(key);
            obj["requested"] = requested;
        }

        if (!value.isEmpty()) {
            bool successful = false;

            if (source == ClientVariables::SourceServer) {
                if (updateClientVariable(key, value, clientVariablesDataMap)) {
                    clientVariablesDataMap = getClientVariableDataMap(); // refresh
                }

                successful = (available && updatable && value == clientVariablesDataMap[key].second);
                obj["success"] = successful;
            }

            if (successful) {
                obj["value"] = clientVariablesDataMap[key].second;
            } else {
                obj["value"] = value;
            }
        }

        obj["source"] = sources[source];
        obj["timestamp"] = timestamp;
        response[key] = obj;
    };

    const QString process = (source == ClientVariables::SourceClient ? "[ \"" + data + "\" ]" : data);
    auto doc = QJsonDocument::fromJson(process.toUtf8());

    if (doc.isNull()) {
        qWarning() << "ClientVariables::sendClientVariablesUpdate: Invalid JSON data";
        return;
    }

    if (doc.isArray()) {
        QJsonArray jsonArray = doc.array();

        for (const QJsonValue& value : jsonArray) {
            if (value.isString()) {
                QString key = value.toString();
                bool available = !mpHost->mEnableMNES; // Default availability state

                if (source != ClientVariables::SourceRequest && !clientVariablesRequested.contains(key)) {
                    qDebug() << "We did not update client variable" << key << "because the server did not request it yet";
                    continue;
                }

                if (protectedVariables.contains(key)) {
                    const auto &[updatable, behaviour, translation] = protectedVariables[key];

                    if (behaviour == ClientVariables::DataSharingBehaviour::Block) {
                        addResponse(key, false, false);
                    } else {
                        available = (behaviour == ClientVariables::DataSharingBehaviour::Share);

                        if (!available && source == ClientVariables::SourceRequest) {
                            requested << translation;
                        }

                        if (clientVariablesDataMap.contains(key)) {
                            addResponse(key, available, updatable, !available, clientVariablesDataMap[key].second);
                        } else {
                            addResponse(key, available, updatable, !available);
                        }
                    }
                } else if (nonProtectedVariables.contains(key)) {
                    if (clientVariablesDataMap.contains(key)) {
                        addResponse(key, true, nonProtectedVariables[key], false, clientVariablesDataMap[key].second);
                    } else {
                        addResponse(key, true, nonProtectedVariables[key]);
                    }
                } else {
                    addResponse(key, false, false);
                }
            } else {
                qWarning() << "ClientVariables::sendClientVariablesUpdate: Array element is not a string";
            }
        }
    } else if (doc.isObject()) {
        QJsonObject jsonObj = doc.object();

        for (QJsonObject::ConstIterator it = jsonObj.constBegin(); it != jsonObj.constEnd(); ++it) {
            QString key = it.key();
            QString value = it.value().toString();
            bool available = !mpHost->mEnableMNES; // Default availability state

            if (source != ClientVariables::SourceRequest && !clientVariablesRequested.contains(key)) {
                qDebug() << "We did not update client variable" << key << "because the server did not request it yet";
                continue;
            }

            if (protectedVariables.contains(key)) {
                const auto &[updatable, behaviour, translation] = protectedVariables[key];

                if (behaviour == ClientVariables::DataSharingBehaviour::Block) {
                    addResponse(key, false, false);
                } else {
                    available = (behaviour == ClientVariables::DataSharingBehaviour::Share);

                    if (!available && source == ClientVariables::SourceRequest) {
                        requested << translation;
                    }

                    if (clientVariablesDataMap.contains(key)) {
                        addResponse(key, available, updatable, !available, value);
                    } else {
                        addResponse(key, available, updatable, !available);
                    }
                }
            } else if (nonProtectedVariables.contains(key)) {
                if (clientVariablesDataMap.contains(key)) {
                    addResponse(key, true, nonProtectedVariables[key], false, value);
                } else {
                    addResponse(key, true, nonProtectedVariables[key]);
                }
            } else {
                addResponse(key, false, false);
            }
        }
    } else {
        qWarning() << "ClientVariables::sendClientVariablesUpdate: Unsupported JSON format";
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
        const QString text = tr("\n        --> Control sharing preferences by clicking here for the Sharing tab in Settings <--\n\n\n");
        QStringList commandList;
        QStringList hintList;
        bool useCurrentFormat = true;

        commandList << "showSettingsTab(\"tab_sharing\")";
        hintList << tr("Open the Sharing tab of the Settings menu");

        const QString info1 = tr("[ INFO ]  - To customize the experience, the game or a script requests you consent to share:");
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
