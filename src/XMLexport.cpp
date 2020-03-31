/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017-2020 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "XMLexport.h"


#include "Host.h"
#include "LuaInterface.h"
#include "TAction.h"
#include "TAlias.h"
#include "TConsole.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "VarUnit.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include <QFile>
#include <sstream>
#include "post_guard.h"

XMLexport::XMLexport( Host * pH )
: mpHost( pH )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TTrigger * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( pT )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TTimer * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( Q_NULLPTR )
, mpTimer( pT )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TAlias * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( pT )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TAction * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( pT )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TScript * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( pT )
, mpKey( Q_NULLPTR )
{
}

XMLexport::XMLexport( TKey * pT )
: mpHost( Q_NULLPTR )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( pT )
{
}

XMLexport::~XMLexport() = default;

void XMLexport::writeModuleXML(const QString& moduleName, const QString& fileName)
{
    auto pHost = mpHost;
    auto mudletPackage = writeXmlHeader();

    auto triggerPackage = mudletPackage.append_child("TriggerPackage");
    //we go a level down for all these functions so as to not infinitely nest the module
    for (auto& it : pHost->mTriggerUnit.mTriggerRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (!it->isTemporary() && it->mModuleMember) {
            writeTrigger(it, triggerPackage);
        }
    }

    auto timerPackage = mudletPackage.append_child("TimerPackage");
    for (auto& it : pHost->mTimerUnit.mTimerRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (!it->isTemporary() && it->mModuleMember) {
            writeTimer(it, timerPackage);
        }
    }

    auto aliasPackage = mudletPackage.append_child("AliasPackage");
    for (auto& it : pHost->mAliasUnit.mAliasRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (!it->isTemporary() && it->mModuleMember) {
            writeAlias(it, aliasPackage);
        }
    }

    auto actionPackage = mudletPackage.append_child("ActionPackage");
    for (auto& it : pHost->mActionUnit.mActionRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (it->mModuleMember) {
            writeAction(it, actionPackage);
        }
    }

    auto scriptPackage = mudletPackage.append_child("ScriptPackage");
    for (auto& it : pHost->mScriptUnit.mScriptRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (it->mModuleMember) {
            writeScript(it, scriptPackage);
        }
    }

    auto keyPackage = mudletPackage.append_child("KeyPackage");
    for (auto& it : pHost->mKeyUnit.mKeyRootNodeList) {
        if (!it || it->mPackageName != moduleName) {
            continue;
        }
        if (!it->isTemporary() && it->mModuleMember) {
            writeKey(it, keyPackage);
        }
    }

    auto helpPackage = mudletPackage.append_child("HelpPackage");
    if (pHost->moduleHelp.contains(moduleName) && pHost->moduleHelp.value(moduleName).contains(QStringLiteral("helpURL"))) {
        helpPackage.append_child("helpURL").text().set(pHost->moduleHelp.value(moduleName).value(QStringLiteral("helpURL")).toUtf8().constData());
    } else {
        helpPackage.append_child("helpURL").text().set("");
    }

    auto future = QtConcurrent::run(this, &XMLexport::saveXml, fileName);
    auto watcher = new QFutureWatcher<bool>;
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, mpHost, [=]() { mpHost->xmlSaved(fileName); });
    watcher->setFuture(future);
    saveFutures.append(future);
}

void XMLexport::exportHost(const QString& filename_pugi_xml)
{
    auto mudletPackage = writeXmlHeader();
    writeHost(mpHost, mudletPackage);
    auto future = QtConcurrent::run(this, &XMLexport::saveXml, filename_pugi_xml);

    auto watcher = new QFutureWatcher<bool>;
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, mpHost, [=]() { mpHost->xmlSaved(QStringLiteral("profile")); });
    watcher->setFuture(future);
    saveFutures.append(future);
}

// credit: https://stackoverflow.com/a/29752943/72944
void inline XMLexport::replaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length()); // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos))) {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

// sanitize non-printable characters for backwards & forward compatibility
// work qxml which is stuck at 1.0 and doesn't support properly
// encoding them. See https://github.com/Mudlet/Mudlet/issues/500
void XMLexport::sanitizeForQxml(std::string& output)
{
    QMap<std::string, std::string> replacements{
            {"&#1;", "\uFFFC\u2401"},   // SOH
            {"&#01;", "\uFFFC\u2401"},   // SOH
            {"&#2;", "\uFFFC\u2402"},   // STX
            {"&#02;", "\uFFFC\u2402"},   // STX
            {"&#3;", "\uFFFC\u2403"},   // ETX
            {"&#03;", "\uFFFC\u2403"},   // ETX
            {"&#4;", "\uFFFC\u2404"},   // EOT
            {"&#04;", "\uFFFC\u2404"},   // EOT
            {"&#5;", "\uFFFC\u2405"},   // ENQ
            {"&#05;", "\uFFFC\u2405"},   // ENQ
            {"&#6;", "\uFFFC\u2406"},   // ACK
            {"&#06;", "\uFFFC\u2406"},   // ACK
            {"&#7;", "\uFFFC\u2407"},   // BEL
            {"&#07;", "\uFFFC\u2407"},   // BEL
            {"&#8;", "\uFFFC\u2408"},   // BS
            {"&#08;", "\uFFFC\u2408"},   // BS
            {"&#11;", "\uFFFC\u240B"},  // VT
            {"&#12;", "\uFFFC\u240C"},  // FF
            {"&#14;", "\uFFFC\u240E"},  // SS
            {"&#15;", "\uFFFC\u240F"},  // SI
            {"&#10;", "\uFFFC\u2410"},  // DLE
            {"&#16;", "\uFFFC\u2411"},  // DC1
            {"&#18;", "\uFFFC\u2412"},  // DC2
            {"&#19;", "\uFFFC\u2413"},  // DC3
            {"&#20;", "\uFFFC\u2414"},  // DC4
            {"&#21;", "\uFFFC\u2415"},  // NAK
            {"&#22;", "\uFFFC\u2416"},  // SYN
            {"&#17;", "\uFFFC\u2417"},  // ETB
            {"&#23;", "\uFFFC\u2418"},  // CAN
            {"&#25;", "\uFFFC\u2419"},  // EM
            {"&#26;", "\uFFFC\u241A"},  // SUB
            {"&#27;", "\uFFFC\u241B"},  // ESC
            {"&#28;", "\uFFFC\u241C"},  // FS
            {"&#29;", "\uFFFC\u241D"},  // GS
            {"&#30;", "\uFFFC\u241E"},  // RS
            {"&#31;", "\uFFFC\u241F"},  // US
            {"&#127;", "\uFFFC\u2421"}, // DEL
    };

    // Look for each replacement in data and if not present remove it from the
    // list of things to replace
    QMutableMapIterator<std::string, std::string> itReplacement(replacements);
    while (itReplacement.hasNext()) {
        itReplacement.next();
        if (output.find(itReplacement.key()) == std::string::npos) {
            itReplacement.remove();
        }
    }

    if (!replacements.isEmpty()) {
        // There is at least one thing left in the QMap of replacements and we
        // can use the same iterator if we reset it to the start of the
        // remaining replacements:
        itReplacement.toFront();
        while (itReplacement.hasNext()) {
            itReplacement.next();
            replaceAll(output, itReplacement.key(), itReplacement.value());
        }
    }
}

bool XMLexport::saveXml(const QString& fileName)
{
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly)) {
        qDebug().noquote().nospace() << "XMLexport::saveXml(\"" << fileName << "\") ERROR - failed to open file, reason: " << file.errorString() << ".";
        return false;
    }

    bool result = saveXmlFile(file);
    if (!result) {
        if (file.error() != QFile::NoError) {
            // Error reason was related to QFile:
            qDebug().noquote().nospace() << "XMLexport::saveXml(\"" << fileName << "\") ERROR - failed to save package, reason: " << file.errorString() << ".";
        } else {
            // Error was due to failure in document preparation
            qDebug().noquote().nospace() << "XMLexport::saveXml(\"" << fileName << "\") ERROR - failed to save package, reason: XML document preparation failure.";
        }
    }

    file.close();
    return result;
}

// This has been factored out to a separate method from saveXml(const QString&)
// because there are situations where we have a QFile instance already and
// just passing a filename and then creating another QFile instance on the
// same file in the filesystem is less than optimum.
// TODO: Refactor dlgTriggerEditor::slot_export() {at least} to call this method instead of saveXml(const QString&)
bool XMLexport::saveXmlFile(QFile& file)
{
    std::stringstream saveStringStream(std::ios::out);
    // Remember, the mExportDoc is the data in the form of a pugi::xml_document
    // instance - the save method needs a stream that impliments the
    // std::ostream interface into which it can push the data:
    mExportDoc.save(saveStringStream);
    // We need to do our own replacement of ASCII control characters that are
    // not valid in XML verison 1.0 and that means we cannot use the pugixml
    // file methods as it does that in a different way which is not helpful
    // as we do not use that library for READING the XML files - so convert
    // the data to a std::string :
    std::string output(saveStringStream.str());
    // Then do the control character replacement:
    sanitizeForQxml(output);
    // Now we can use Qt's file handling which does handle non-Latin1 named
    // files - which MinGW's STL file handling (on Windows platform) does not:
    file.write(output.data());
    return file.error() == QFile::NoError;
}

QString XMLexport::saveXml()
{
    std::stringstream saveStringStream(std::ios::out);
    std::string output;

    mExportDoc.save(saveStringStream);
    output = saveStringStream.str();

    sanitizeForQxml(output);

    return QString::fromStdString(output);
}

void XMLexport::writeHost(Host* pHost, pugi::xml_node mudletPackage)
{
    auto hostPackage = mudletPackage.append_child("HostPackage");
    auto host = hostPackage.append_child("Host");

    // Some of the data items being stored are simple numbers or other texts
    // that can be expressed solely with the Latin1 character encoding so that
    // can be used compared to the more complex Utf8 one needed otherwise:
    host.append_attribute("autoClearCommandLineAfterSend") = pHost->mAutoClearCommandLineAfterSend ? "yes" : "no";
    host.append_attribute("printCommand") = pHost->mPrintCommand ? "yes" : "no";
    host.append_attribute("USE_IRE_DRIVER_BUGFIX") = pHost->mUSE_IRE_DRIVER_BUGFIX ? "yes" : "no";
    host.append_attribute("mUSE_FORCE_LF_AFTER_PROMPT") = pHost->mUSE_FORCE_LF_AFTER_PROMPT ? "yes" : "no";
    host.append_attribute("mUSE_UNIX_EOL") = pHost->mUSE_UNIX_EOL ? "yes" : "no";
    host.append_attribute("mNoAntiAlias") = pHost->mNoAntiAlias ? "yes" : "no";
    host.append_attribute("mEchoLuaErrors") = pHost->mEchoLuaErrors ? "yes" : "no";
    host.append_attribute("runAllKeyMatches") = pHost->getKeyUnit()->mRunAllKeyMatches ? "yes" : "no";
    host.append_attribute("AmbigousWidthGlyphsToBeWide") = pHost->mAutoAmbigousWidthGlyphsSetting ? "auto" : (pHost->mWideAmbigousWidthGlyphs ? "yes" : "no");
    // FIXME: Change to a string or integer property when possible to support more
    // than false (perhaps 0 or "PlainText") or true (perhaps 1 or "HTML") in the
    // future - phpBB code might be useful if it can be done.
    host.append_attribute("mRawStreamDump") = pHost->mIsNextLogFileInHtmlFormat ? "yes" : "no";
    host.append_attribute("mIsLoggingTimestamps") = pHost->mIsLoggingTimestamps ? "yes" : "no";
    host.append_attribute("logDirectory") = pHost->mLogDir.toUtf8().constData();
    host.append_attribute("logFileName") = pHost->mLogFileName.toUtf8().constData();
    host.append_attribute("logFileNameFormat") = pHost->mLogFileNameFormat.toUtf8().constData();
    host.append_attribute("mAlertOnNewData") = pHost->mAlertOnNewData ? "yes" : "no";
    host.append_attribute("mFORCE_NO_COMPRESSION") = pHost->mFORCE_NO_COMPRESSION ? "yes" : "no";
    host.append_attribute("mFORCE_GA_OFF") = pHost->mFORCE_GA_OFF ? "yes" : "no";
    host.append_attribute("mFORCE_SAVE_ON_EXIT") = pHost->mFORCE_SAVE_ON_EXIT ? "yes" : "no";
    host.append_attribute("mEnableGMCP") = pHost->mEnableGMCP ? "yes" : "no";
    host.append_attribute("mEnableMSSP") = pHost->mEnableMSSP ? "yes" : "no";
    host.append_attribute("mEnableMSP") = pHost->mEnableMSP ? "yes" : "no";
    host.append_attribute("mEnableMSDP") = pHost->mEnableMSDP ? "yes" : "no";
    host.append_attribute("mMapStrongHighlight") = pHost->mMapStrongHighlight ? "yes" : "no";
    host.append_attribute("mLogStatus") = pHost->mLogStatus ? "yes" : "no";
    host.append_attribute("mEnableSpellCheck") = pHost->mEnableSpellCheck ? "yes" : "no";
    bool enableUserDictionary;
    bool useSharedDictionary;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    host.append_attribute("mEnableUserDictionary") = enableUserDictionary ? "yes" : "no";
    host.append_attribute("mUseSharedDictionary") = useSharedDictionary ? "yes" : "no";
    host.append_attribute("mShowInfo") = pHost->mShowInfo ? "yes" : "no";
    host.append_attribute("mAcceptServerGUI") = pHost->mAcceptServerGUI ? "yes" : "no";
    host.append_attribute("mAcceptServerMedia") = pHost->mAcceptServerMedia ? "yes" : "no";
    host.append_attribute("mMapperUseAntiAlias") = pHost->mMapperUseAntiAlias ? "yes" : "no";
    host.append_attribute("mFORCE_MXP_NEGOTIATION_OFF") = pHost->mFORCE_MXP_NEGOTIATION_OFF ? "yes" : "no";
    host.append_attribute("enableTextAnalyzer") = pHost->mEnableTextAnalyzer ? "yes" : "no";
    host.append_attribute("mRoomSize") = QString::number(pHost->mRoomSize, 'f', 1).toUtf8().constData();
    host.append_attribute("mLineSize") = QString::number(pHost->mLineSize, 'f', 1).toUtf8().constData();
    host.append_attribute("mBubbleMode") = pHost->mBubbleMode ? "yes" : "no";
    host.append_attribute("mShowRoomIDs") = pHost->mShowRoomID ? "yes" : "no";
    host.append_attribute("mShowPanel") = pHost->mShowPanel ? "yes" : "no";
    host.append_attribute("mHaveMapperScript") = pHost->mHaveMapperScript ? "yes" : "no";
    host.append_attribute("mEditorAutoComplete") = pHost->mEditorAutoComplete ? "yes" : "no";
    host.append_attribute("mEditorTheme") = pHost->mEditorTheme.toUtf8().constData();
    host.append_attribute("mEditorThemeFile") = pHost->mEditorThemeFile.toUtf8().constData();
    host.append_attribute("mThemePreviewItemID") = QString::number(pHost->mThemePreviewItemID).toUtf8().constData();
    host.append_attribute("mThemePreviewType") = pHost->mThemePreviewType.toUtf8().constData();
    host.append_attribute("mSearchEngineName") = pHost->mSearchEngineName.toUtf8().constData();
    host.append_attribute("mTimerSupressionInterval") = pHost->mTimerDebugOutputSuppressionInterval.toString(QLatin1String("HH:mm:ss.zzz")).toUtf8().constData();
    host.append_attribute("mUseProxy") = pHost->mUseProxy ? "yes" : "no";
    host.append_attribute("mProxyAddress") = pHost->mProxyAddress.toUtf8().constData();
    host.append_attribute("mProxyPort") = QString::number(pHost->mProxyPort).toUtf8().constData();
    host.append_attribute("mProxyUsername") = pHost->mProxyUsername.toUtf8().constData();
    host.append_attribute("mProxyPassword") = pHost->mProxyPassword.toUtf8().constData();
    host.append_attribute("mAutoReconnect") = pHost->mAutoReconnect ? "yes" : "no";
    host.append_attribute("mSslTsl") = pHost->mSslTsl ? "yes" : "no";
    host.append_attribute("mSslIgnoreExpired") = pHost->mSslIgnoreExpired ? "yes" : "no";
    host.append_attribute("mSslIgnoreSelfSigned") = pHost->mSslIgnoreSelfSigned ? "yes" : "no";
    host.append_attribute("mSslIgnoreAll") = pHost->mSslIgnoreAll ? "yes" : "no";
    host.append_attribute("mDiscordAccessFlags") = QString::number(pHost->mDiscordAccessFlags).toUtf8().constData();
    host.append_attribute("mRequiredDiscordUserName") = pHost->mRequiredDiscordUserName.toUtf8().constData();
    host.append_attribute("mRequiredDiscordUserDiscriminator") = pHost->mRequiredDiscordUserDiscriminator.toUtf8().constData();
    host.append_attribute("mSGRCodeHasColSpaceId") = pHost->getHaveColorSpaceId() ? "yes" : "no";
    host.append_attribute("mServerMayRedefineColors") = pHost->getMayRedefineColors() ? "yes" : "no";
    quint8 styleCode = 0;
    quint8 outerDiameterPercentage = 0;
    quint8 innerDiameterPercentage = 0;
    QColor outerColor;
    QColor innerColor;
    pHost->getPlayerRoomStyleDetails(styleCode, outerDiameterPercentage, innerDiameterPercentage, outerColor, innerColor);
    host.append_attribute("playerRoomPrimaryColor") = outerColor.name(QColor::HexArgb).toLatin1().constData();
    host.append_attribute("playerRoomSecondaryColor") = innerColor.name(QColor::HexArgb).toLatin1().constData();
    host.append_attribute("playerRoomStyle") = QString::number(styleCode).toLatin1().constData();
    host.append_attribute("playerRoomOuterDiameter") = QString::number(outerDiameterPercentage).toLatin1().constData();
    host.append_attribute("playerRoomInnerDiameter") = QString::number(innerDiameterPercentage).toLatin1().constData();

    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while (it.hasNext()) {
        ignore = ignore.append(it.next());
    }
    host.append_attribute("mDoubleClickIgnore") = ignore.toUtf8().constData();
    host.append_attribute("EditorSearchOptions") = QString::number(pHost->mSearchOptions).toLatin1().constData();

    { // Blocked so that indentation reflects that of the XML file
        host.append_child("name").text().set(pHost->mHostName.toUtf8().constData());

        auto mInstalledPackages = host.append_child("mInstalledPackages");

        for (int i = 0; i < pHost->mInstalledPackages.size(); ++i) {
            mInstalledPackages.append_child("string").text().set(pHost->mInstalledPackages.at(i).toUtf8().constData());
        }

        if (!pHost->mInstalledModules.empty()) {
            auto mInstalledModules = host.append_child("mInstalledModules");
            QMapIterator<QString, QStringList> it(pHost->mInstalledModules);
            pHost->modulesToWrite.clear();
            while (it.hasNext()) {
                it.next();
                mInstalledModules.append_child("key").text().set(it.key().toUtf8().constData());
                QStringList entry = it.value();
                mInstalledModules.append_child("filepath").text().set(entry.at(0).toUtf8().constData());
                mInstalledModules.append_child("globalSave").text().set(entry.at(1).toUtf8().constData());
                if (entry.at(1).toInt()) {
                    pHost->modulesToWrite.insert(it.key(), entry);
                }
                mInstalledModules.append_child("priority").text().set(QString::number(pHost->mModulePriorities.value(it.key())).toUtf8().constData());
            }
        }

        host.append_child("url").text().set(pHost->mUrl.toUtf8().constData());
        host.append_child("serverPackageName").text().set(pHost->mServerGUI_Package_name.toUtf8().constData());
        host.append_child("serverPackageVersion").text().set(pHost->mServerGUI_Package_version.toUtf8().constData());
        host.append_child("port").text().set(QString::number(pHost->mPort).toUtf8().constData());
        host.append_child("borderTopHeight").text().set(QString::number(pHost->mBorderTopHeight).toUtf8().constData());
        host.append_child("borderBottomHeight").text().set(QString::number(pHost->mBorderBottomHeight).toUtf8().constData());
        host.append_child("borderLeftWidth").text().set(QString::number(pHost->mBorderLeftWidth).toUtf8().constData());
        host.append_child("borderRightWidth").text().set(QString::number(pHost->mBorderRightWidth).toUtf8().constData());
        host.append_child("wrapAt").text().set(QString::number(pHost->mWrapAt).toUtf8().constData());
        host.append_child("wrapIndentCount").text().set(QString::number(pHost->mWrapIndentCount).toUtf8().constData());
        host.append_child("mFgColor").text().set(pHost->mFgColor.name().toUtf8().constData());
        host.append_child("mBgColor").text().set(pHost->mBgColor.name().toUtf8().constData());
        host.append_child("mCommandFgColor").text().set(pHost->mCommandFgColor.name().toUtf8().constData());
        host.append_child("mCommandBgColor").text().set(pHost->mCommandBgColor.name().toUtf8().constData());
        host.append_child("mCommandLineFgColor").text().set(pHost->mCommandLineFgColor.name().toUtf8().constData());
        host.append_child("mCommandLineBgColor").text().set(pHost->mCommandLineBgColor.name().toUtf8().constData());
        host.append_child("mBlack").text().set(pHost->mBlack.name().toUtf8().constData());
        host.append_child("mLightBlack").text().set(pHost->mLightBlack.name().toUtf8().constData());
        host.append_child("mRed").text().set(pHost->mRed.name().toUtf8().constData());
        host.append_child("mLightRed").text().set(pHost->mLightRed.name().toUtf8().constData());
        host.append_child("mBlue").text().set(pHost->mBlue.name().toUtf8().constData());
        host.append_child("mLightBlue").text().set(pHost->mLightBlue.name().toUtf8().constData());
        host.append_child("mGreen").text().set(pHost->mGreen.name().toUtf8().constData());
        host.append_child("mLightGreen").text().set(pHost->mLightGreen.name().toUtf8().constData());
        host.append_child("mYellow").text().set(pHost->mYellow.name().toUtf8().constData());
        host.append_child("mLightYellow").text().set(pHost->mLightYellow.name().toUtf8().constData());
        host.append_child("mCyan").text().set(pHost->mCyan.name().toUtf8().constData());
        host.append_child("mLightCyan").text().set(pHost->mLightCyan.name().toUtf8().constData());
        host.append_child("mMagenta").text().set(pHost->mMagenta.name().toUtf8().constData());
        host.append_child("mLightMagenta").text().set(pHost->mLightMagenta.name().toUtf8().constData());
        host.append_child("mWhite").text().set(pHost->mWhite.name().toUtf8().constData());
        host.append_child("mLightWhite").text().set(pHost->mLightWhite.name().toUtf8().constData());
        host.append_child("mDisplayFont").text().set(pHost->getDisplayFont().toString().toUtf8().constData());
        host.append_child("mCommandLineFont").text().set(pHost->mCommandLineFont.toString().toUtf8().constData());
        // There was a mis-spelt duplicate commandSeperator above but it is now gone
        host.append_child("mCommandSeparator").text().set(pHost->mCommandSeparator.toUtf8().constData());
        host.append_child("commandLineMinimumHeight").text().set(QString::number(pHost->commandLineMinimumHeight).toUtf8().constData());

        host.append_child("mFgColor2").text().set(pHost->mFgColor_2.name().toUtf8().constData());
        host.append_child("mBgColor2").text().set(pHost->mBgColor_2.name().toUtf8().constData());
        host.append_child("mBlack2").text().set(pHost->mBlack_2.name().toUtf8().constData());
        host.append_child("mLightBlack2").text().set(pHost->mLightBlack_2.name().toUtf8().constData());
        host.append_child("mRed2").text().set(pHost->mRed_2.name().toUtf8().constData());
        host.append_child("mLightRed2").text().set(pHost->mLightRed_2.name().toUtf8().constData());
        host.append_child("mBlue2").text().set(pHost->mBlue_2.name().toUtf8().constData());
        host.append_child("mLightBlue2").text().set(pHost->mLightBlue_2.name().toUtf8().constData());
        host.append_child("mGreen2").text().set(pHost->mGreen_2.name().toUtf8().constData());
        host.append_child("mLightGreen2").text().set(pHost->mLightGreen_2.name().toUtf8().constData());
        host.append_child("mYellow2").text().set(pHost->mYellow_2.name().toUtf8().constData());
        host.append_child("mLightYellow2").text().set(pHost->mLightYellow_2.name().toUtf8().constData());
        host.append_child("mCyan2").text().set(pHost->mCyan_2.name().toUtf8().constData());
        host.append_child("mLightCyan2").text().set(pHost->mLightCyan_2.name().toUtf8().constData());
        host.append_child("mMagenta2").text().set(pHost->mMagenta_2.name().toUtf8().constData());
        host.append_child("mLightMagenta2").text().set(pHost->mLightMagenta_2.name().toUtf8().constData());
        host.append_child("mWhite2").text().set(pHost->mWhite_2.name().toUtf8().constData());
        host.append_child("mLightWhite2").text().set(pHost->mLightWhite_2.name().toUtf8().constData());
        host.append_child("mSpellDic").text().set(pHost->mpConsole->getSystemSpellDictionary().toUtf8().constData());
        // TODO: Consider removing these sub-elements that duplicate the same
        // attributes - which WERE bugged - when we update the XML format, must leave
        // them in place for now even though we no longer use them for compatibility
        // with older version of Mudlet
        host.append_child("mLineSize").text().set(QString::number(pHost->mLineSize, 'f', 1).toUtf8().constData());
        host.append_child("mRoomSize").text().set(QString::number(pHost->mRoomSize, 'f', 1).toUtf8().constData());
    }

    {
        auto stopwatches = host.append_child("stopwatches");
        QListIterator<int> itStopWatchId(pHost->getStopWatchIds());
        while (itStopWatchId.hasNext()) {
            auto stopWatchId = itStopWatchId.next();
            auto pStopWatch = pHost->getStopWatch(stopWatchId);
            if (pStopWatch->persistent()) {
                auto stopwatch = stopwatches.append_child("stopwatch");
                // Three QStrings used here are purely numeric so can be expressed in Latin1 encoding:
                stopwatch.append_attribute("id") = QString::number(stopWatchId).toLatin1().constData();
                if (pStopWatch->running()) {
                    stopwatch.append_attribute("running") = "yes";
                    stopwatch.append_attribute("effectiveStartDateTimeEpochMSecs") = QString::number(QDateTime::currentMSecsSinceEpoch() - pStopWatch->getElapsedMilliSeconds()).toLatin1().constData();
                } else {
                    stopwatch.append_attribute("running") = "no";
                    stopwatch.append_attribute("elapsedDateTimeMSecs") = QString::number(pStopWatch->getElapsedMilliSeconds()).toLatin1().constData();
                }
                stopwatch.append_attribute("name") = pStopWatch->name().toUtf8().constData();
            }
        }
    }
    writeTriggerPackage(pHost, mudletPackage, true);
    writeTimerPackage(pHost, mudletPackage, true);
    writeAliasPackage(pHost, mudletPackage, true);
    writeActionPackage(pHost, mudletPackage, true);
    writeScriptPackage(pHost, mudletPackage, true);
    writeKeyPackage(pHost, mudletPackage, true);
    writeVariablePackage(pHost, mudletPackage);
}

void XMLexport::writeVariablePackage(Host* pHost, pugi::xml_node& mudletPackage)
{
    auto variablePackage = mudletPackage.append_child("VariablePackage");
    LuaInterface* lI = pHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    //do hidden variables first
    { // Blocked so that indentation reflects that of the XML file
        auto hiddenVariables = variablePackage.append_child("HiddenVariables");
        QSetIterator<QString> itHiddenVariableName(vu->hiddenByUser);
        while (itHiddenVariableName.hasNext()) {
            auto variableName = itHiddenVariableName.next();
            hiddenVariables.append_child("name").text().set(variableName.toUtf8().constData());
        }
    }

    TVar* base = vu->getBase();
    if (!base) {
        lI->getVars(false);
        base = vu->getBase();
    }

    if (base) {
        QListIterator<TVar*> itVariable(base->getChildren(false));
        while (itVariable.hasNext()) {
            writeVariable(itVariable.next(), lI, vu, variablePackage);
        }
    }
}

void XMLexport::writeKeyPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool skipModuleMembers)
{
    auto keyPackage = mudletPackage.append_child("KeyPackage");
    for (auto it : pHost->mKeyUnit.mKeyRootNodeList) {
        if (!it || it->isTemporary() || (skipModuleMembers && it->mModuleMember)) {
            continue;
        }
        writeKey(it, keyPackage);
    }
}

void XMLexport::writeScriptPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool skipModuleMembers)
{
    auto scriptPackage = mudletPackage.append_child("ScriptPackage");
    for (auto it : pHost->mScriptUnit.mScriptRootNodeList) {
        if (!it || (skipModuleMembers && it->mModuleMember)) {
            continue;
        }
        writeScript(it, scriptPackage);
    }
}

void XMLexport::writeActionPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool skipModuleMembers)
{
    auto actionPackage = mudletPackage.append_child("ActionPackage");
    for (auto it : pHost->mActionUnit.mActionRootNodeList) {
        if (!it || (skipModuleMembers && it->mModuleMember)) {
            continue;
        }
        writeAction(it, actionPackage);
    }
}

void XMLexport::writeAliasPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool skipModuleMembers)
{
    auto aliasPackage = mudletPackage.append_child("AliasPackage");
    for (auto it : pHost->mAliasUnit.mAliasRootNodeList) {
        if (!it || (skipModuleMembers && it->mModuleMember)) {
            continue;
        }
        if (!it->isTemporary()) {
            writeAlias(it, aliasPackage);
        }
    }
}

void XMLexport::writeTimerPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool skipModuleMembers)
{
    auto timerPackage = mudletPackage.append_child("TimerPackage");
    for (auto it : pHost->mTimerUnit.mTimerRootNodeList) {
        if (!it || (skipModuleMembers && it->mModuleMember)) {
            continue;
        }
        if (!it->isTemporary()) {
            writeTimer(it, timerPackage);
        }
    }
}

void XMLexport::writeTriggerPackage(const Host* pHost, pugi::xml_node& mudletPackage, bool ignoreModuleMembers)
{
    auto triggerPackage = mudletPackage.append_child("TriggerPackage");
    for (auto it : pHost->mTriggerUnit.mTriggerRootNodeList) {
        if (!it || (ignoreModuleMembers && it->mModuleMember)) {
            continue;
        }
        if (!it->isTemporary()) {
            writeTrigger(it, triggerPackage);
        }
    }
}

void XMLexport::writeVariable(TVar* pVar, LuaInterface* pLuaInterface, VarUnit* pVariableUnit, pugi::xml_node xmlParent)
{
    if (pVariableUnit->isSaved(pVar)) {
        if (pVar->getValueType() == LUA_TTABLE) {
            auto variableGroup = xmlParent.append_child("VariableGroup");

            variableGroup.append_child("name").text().set(pVar->getName().toUtf8().constData());
            variableGroup.append_child("keyType").text().set(QString::number(pVar->getKeyType()).toUtf8().constData());
            variableGroup.append_child("value").text().set(pLuaInterface->getValue(pVar).toUtf8().constData());
            variableGroup.append_child("valueType").text().set(QString::number(pVar->getValueType()).toUtf8().constData());

            QListIterator<TVar*> itNestedVariable(pVar->getChildren(false));
            while (itNestedVariable.hasNext()) {
                writeVariable(itNestedVariable.next(), pLuaInterface, pVariableUnit, variableGroup);
            }
        } else {
            auto variable = xmlParent.append_child("Variable");

            variable.append_child("name").text().set(pVar->getName().toUtf8().constData());
            variable.append_child("keyType").text().set(QString::number(pVar->getKeyType()).toUtf8().constData());
            variable.append_child("value").text().set(pLuaInterface->getValue(pVar).toUtf8().constData());
            variable.append_child("valueType").text().set(QString::number(pVar->getValueType()).toUtf8().constData());
        }
    }
}

bool XMLexport::exportProfile(const QString& exportFileName)
{
    auto mudletPackage = writeXmlHeader();

    if (writeGenericPackage(mpHost, mudletPackage)) {
        auto future = QtConcurrent::run(this, &XMLexport::saveXml, exportFileName);
        auto watcher = new QFutureWatcher<bool>;
        QObject::connect(watcher, &QFutureWatcher<bool>::finished, mpHost, [=]() { mpHost->xmlSaved(QStringLiteral("profile")); });
        watcher->setFuture(future);
        saveFutures.append(future);

        return true;
    }

    return false;
}

bool XMLexport::exportPackage(const QString& exportFileName)
{
    auto mudletPackage = writeXmlHeader();

    if (writeGenericPackage(mpHost, mudletPackage)) {
        return saveXml(exportFileName);
    }

    return false;
}

bool XMLexport::writeGenericPackage(Host* pHost, pugi::xml_node& mudletPackage)
{
    writeTriggerPackage(pHost, mudletPackage, true);
    writeTimerPackage(pHost, mudletPackage, true);
    writeAliasPackage(pHost, mudletPackage, true);
    writeActionPackage(pHost, mudletPackage, true);
    writeScriptPackage(pHost, mudletPackage, true);
    writeKeyPackage(pHost, mudletPackage, true);
    // variables weren't previously exported as a generic package
    writeVariablePackage(pHost, mudletPackage);

    return true;
}

pugi::xml_node XMLexport::writeXmlHeader()
{
    auto decl = mExportDoc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    mExportDoc.append_child(pugi::node_doctype).set_value("MudletPackage");

    auto mudletPackage = mExportDoc.append_child("MudletPackage");
    mudletPackage.append_attribute("version") = mudlet::self()->scmMudletXmlDefaultVersion.toUtf8().constData();

    return mudletPackage;
}

bool XMLexport::exportTrigger(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto triggerPackage = mudletPackage.append_child("TriggerPackage");

    writeTrigger(mpTrigger, triggerPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TTrigger* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpTrigger) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto triggerPackage = mudletPackage.append_child("TriggerPackage");
    writeTrigger(mpTrigger, triggerPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeTrigger(TTrigger* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto trigger = xmlParent.append_child(pT->isFolder() ? "TriggerGroup" : "Trigger");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = trigger;

        trigger.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        trigger.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";
        trigger.append_attribute("isTempTrigger") = pT->isTemporary() ? "yes" : "no";
        trigger.append_attribute("isMultiline") = pT->mIsMultiline ? "yes" : "no";
        trigger.append_attribute("isPerlSlashGOption") = pT->mPerlSlashGOption ? "yes" : "no";
        trigger.append_attribute("isColorizerTrigger") = pT->mIsColorizerTrigger ? "yes" : "no";
        trigger.append_attribute("isFilterTrigger") = pT->mFilterTrigger ? "yes" : "no";
        trigger.append_attribute("isSoundTrigger") = pT->mSoundTrigger ? "yes" : "no";
        trigger.append_attribute("isColorTrigger") = pT->mColorTrigger ? "yes" : "no";
        trigger.append_attribute("isColorTriggerFg") = (pT->mColorTriggerFgAnsi != TTrigger::scmIgnored) ? "yes" : "no";
        trigger.append_attribute("isColorTriggerBg") = (pT->mColorTriggerBgAnsi != TTrigger::scmIgnored) ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file

            trigger.append_child("name").text().set(pT->mName.toUtf8().constData());
            writeScriptElement(pT->mScript, trigger);

            trigger.append_child("triggerType").text().set(QString::number(pT->mTriggerType).toUtf8().constData());
            trigger.append_child("conditonLineDelta").text().set(QString::number(pT->mConditionLineDelta).toUtf8().constData());
            trigger.append_child("mStayOpen").text().set(QString::number(pT->mStayOpen).toUtf8().constData());
            trigger.append_child("mCommand").text().set(pT->mCommand.toUtf8().constData());
            trigger.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            trigger.append_child("mFgColor").text().set(pT->mFgColor.name().toUtf8().constData());
            trigger.append_child("mBgColor").text().set(pT->mBgColor.name().toUtf8().constData());
            trigger.append_child("mSoundFile").text().set(pT->mSoundFile.toUtf8().constData());
            trigger.append_child("colorTriggerFgColor").text().set(pT->mColorTriggerFgColor.name().toUtf8().constData());
            trigger.append_child("colorTriggerBgColor").text().set(pT->mColorTriggerBgColor.name().toUtf8().constData());

            auto regexCodeList = trigger.append_child("regexCodeList");
            // Revert the first 16 ANSI colour codes back to the wrong values
            // that are still used in the save files
            QStringList unfixedAnsiColourPatternList = remapAnsiToColorNumber(pT->mRegexCodeList, pT->mRegexCodePropertyList);
            for (int i = 0; i < unfixedAnsiColourPatternList.size(); ++i) {
                regexCodeList.append_child("string").text().set(unfixedAnsiColourPatternList.at(i).toUtf8().constData());
            }

            auto regexCodePropertyList = trigger.append_child("regexCodePropertyList");
            for (int i : qAsConst(pT->mRegexCodePropertyList)) {
                regexCodePropertyList.append_child("integer").text().set(QString::number(i).toUtf8().constData());
            }
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeTrigger(it, xmlParent);
    }
}

bool XMLexport::exportAlias(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto aliasPackage = mudletPackage.append_child("AliasPackage");

    writeAlias(mpAlias, aliasPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TAlias* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpAlias) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto aliasPackage = mudletPackage.append_child("AliasPackage");
    writeAlias(mpAlias, aliasPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeAlias(TAlias* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto aliasContents = xmlParent.append_child(pT->isFolder() ? "AliasGroup" : "Alias");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = aliasContents;

        aliasContents.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        aliasContents.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            aliasContents.append_child("name").text().set(pT->mName.toUtf8().constData());
            writeScriptElement(pT->mScript, aliasContents);

            aliasContents.append_child("command").text().set(pT->mCommand.toUtf8().constData());
            aliasContents.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            aliasContents.append_child("regex").text().set(pT->mRegexCode.toUtf8().constData());
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeAlias(it, xmlParent);
    }
}

bool XMLexport::exportAction(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto actionPackage = mudletPackage.append_child("ActionPackage");

    writeAction(mpAction, actionPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TAction* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpAction) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto actionPackage = mudletPackage.append_child("ActionPackage");
    writeAction(mpAction, actionPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeAction(TAction* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto actionContents = xmlParent.append_child(pT->isFolder() ? "ActionGroup" : "Action");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = actionContents;

        actionContents.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        actionContents.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";
        actionContents.append_attribute("isPushButton") = pT->mIsPushDownButton ? "yes" : "no";
        actionContents.append_attribute("isFlatButton") = pT->mButtonFlat ? "yes" : "no";
        actionContents.append_attribute("useCustomLayout") = pT->mUseCustomLayout ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            actionContents.append_child("name").text().set(pT->mName.toUtf8().constData());
            actionContents.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            writeScriptElement(pT->mScript, actionContents);

            actionContents.append_child("css").text().set(pT->css.toUtf8().constData());
            actionContents.append_child("commandButtonUp").text().set(pT->mCommandButtonUp.toUtf8().constData());
            actionContents.append_child("commandButtonDown").text().set(pT->mCommandButtonDown.toUtf8().constData());
            actionContents.append_child("icon").text().set(pT->mIcon.toUtf8().constData());
            actionContents.append_child("orientation").text().set(QString::number(pT->mOrientation).toUtf8().constData());
            actionContents.append_child("location").text().set(QString::number(pT->mLocation).toUtf8().constData());
            actionContents.append_child("posX").text().set(QString::number(pT->mPosX).toUtf8().constData());
            actionContents.append_child("posY").text().set(QString::number(pT->mPosY).toUtf8().constData());
            // We now use a boolean but file must use original "1" (false)
            // or "2" (true) for backward compatibility
            actionContents.append_child("mButtonState").text().set(QString::number(pT->mButtonState ? 2 : 1).toUtf8().constData());
            actionContents.append_child("sizeX").text().set(QString::number(pT->mSizeX).toUtf8().constData());
            actionContents.append_child("sizeY").text().set(QString::number(pT->mSizeY).toUtf8().constData());
            actionContents.append_child("buttonColumn").text().set(QString::number(pT->mButtonColumns).toUtf8().constData());
            actionContents.append_child("buttonRotation").text().set(QString::number(pT->mButtonRotation).toUtf8().constData());
            actionContents.append_child("buttonColor").text().set(pT->mButtonColor.name().toUtf8().constData());
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeAction(it, xmlParent);
    }
}

bool XMLexport::exportTimer(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto timerPackage = mudletPackage.append_child("TimerPackage");

    writeTimer(mpTimer, timerPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TTimer* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpTimer) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto timerPackage = mudletPackage.append_child("TimerPackage");
    writeTimer(mpTimer, timerPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeTimer(TTimer* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto timerContents = xmlParent.append_child(pT->isFolder() ? "TimerGroup" : "Timer");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = timerContents;

        timerContents.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        timerContents.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";
        timerContents.append_attribute("isTempTimer") = pT->isTemporary() ? "yes" : "no";
        timerContents.append_attribute("isOffsetTimer") = pT->isOffsetTimer() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            timerContents.append_child("name").text().set(pT->mName.toUtf8().constData());

            writeScriptElement(pT->mScript, timerContents);

            timerContents.append_child("command").text().set(pT->mCommand.toUtf8().constData());
            timerContents.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            timerContents.append_child("time").text().set(pT->mTime.toString("hh:mm:ss.zzz").toUtf8().constData());
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeTimer(it, xmlParent);
    }
}

bool XMLexport::exportScript(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto scriptPackage = mudletPackage.append_child("ScriptPackage");

    writeScript(mpScript, scriptPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TScript* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpScript) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto scriptPackage = mudletPackage.append_child("ScriptPackage");
    writeScript(mpScript, scriptPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeScript(TScript* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto scriptContents = xmlParent.append_child(pT->isFolder() ? "ScriptGroup" : "Script");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = scriptContents;

        scriptContents.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        scriptContents.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            scriptContents.append_child("name").text().set(pT->mName.toUtf8().constData());
            scriptContents.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            writeScriptElement(pT->mScript, scriptContents);

            auto eventHandlerList = scriptContents.append_child("eventHandlerList");
            for (int i = 0; i < pT->mEventHandlerList.size(); ++i) {
                eventHandlerList.append_child("string").text().set(pT->mEventHandlerList.at(i).toUtf8().constData());
            }
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeScript(it, xmlParent);
    }
}

bool XMLexport::exportKey(const QString& fileName)
{
    auto mudletPackage = writeXmlHeader();

    auto keyPackage = mudletPackage.append_child("KeyPackage");

    writeKey(mpKey, keyPackage);

    return saveXml(fileName);
}

void XMLexport::exportToClipboard(TKey* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpKey) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    auto mudletPackage = writeXmlHeader();
    auto keyPackage = mudletPackage.append_child("KeyPackage");
    writeKey(mpKey, keyPackage);
    auto xml = saveXml();

    auto clipboard = QApplication::clipboard();
    clipboard->setText(xml, QClipboard::Clipboard);
}

void XMLexport::writeKey(TKey* pT, pugi::xml_node xmlParent)
{
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto keyContents = xmlParent.append_child(pT->isFolder() ? "KeyGroup" : "Key");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = keyContents;

        keyContents.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        keyContents.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            keyContents.append_child("name").text().set(pT->mName.toUtf8().constData());
            keyContents.append_child("packageName").text().set(pT->mPackageName.toUtf8().constData());
            writeScriptElement(pT->mScript, keyContents);

            keyContents.append_child("command").text().set(pT->mCommand.toUtf8().constData());
            keyContents.append_child("keyCode").text().set(QString::number(pT->mKeyCode).toUtf8().constData());
            keyContents.append_child("keyModifier").text().set(QString::number(pT->mKeyModifier).toUtf8().constData());
        }
    }

    for (auto& it : *pT->mpMyChildrenList) {
        writeKey(it, xmlParent);
    }
}

void XMLexport::writeScriptElement(const QString& script, pugi::xml_node xmlElement)
{
    xmlElement.append_child("script").text().set(script.toUtf8().constData());
}

// Unlike the reverse operation in XMLimport we cannot modify the supplied patternlist
QStringList XMLexport::remapAnsiToColorNumber(const QStringList & patternList, const QList<int> & typeList)
{

    QStringList results;
    QRegularExpression regex = QRegularExpression(QStringLiteral("(^ANSI_COLORS_F{(\\d+|IGNORE|DEFAULT)}_B{(\\d+|IGNORE|DEFAULT)}$"));
    QStringListIterator itPattern(patternList);
    QListIterator<int> itType(typeList);
    while (itPattern.hasNext() && itType.hasNext()) {
        if (itType.next() == REGEX_COLOR_PATTERN) {
            if (!itPattern.next().isEmpty()) {
                QRegularExpressionMatch match = regex.match(itPattern.peekPrevious());
                // Although we define two '('...')' capture groups the count/size is
                // 3 (0 is the whole string)!
                if (match.hasMatch() && match.capturedTexts().size() == 3) {
                    bool isFgOk = false;
                    int fg = 0;
                    if (match.captured(1) == QLatin1String("DEFAULT")) {
                        // Use the old value for default which is 0 (though we use
                        // -1 internally)
                        isFgOk = true;
                    } else if (match.captured(1) == QLatin1String("IGNORE")) {
                        // Ignore is NOT handled by old system but use -2 (the value
                        // we use internally now)
                        isFgOk = true;
                        fg = -2;
                    } else {
                        fg = match.captured(1).toInt(&isFgOk);
                        if (isFgOk) {
                            // clang-format off
                            switch (fg) {
                            case 0:     fg = 2;     break; // black
                            case 1:     fg = 4;     break; // red
                            case 2:     fg = 6;     break; // green
                            case 3:     fg = 8;     break; // yellow
                            case 4:     fg = 10;    break; // blue
                            case 5:     fg = 12;    break; // magenta
                            case 6:     fg = 14;    break; // cyan
                            case 7:     fg = 16;    break; // white (light gray)
                            case 8:     fg = 1;     break; // light black (dark gray)
                            case 9:     fg = 3;     break; // light red
                            case 10:    fg = 5;     break; // light green
                            case 11:    fg = 7;     break; // light yellow
                            case 12:    fg = 9;     break; // light blue
                            case 13:    fg = 11;    break; // light magenta
                            case 14:    fg = 13;    break; // light cyan
                            case 15:    fg = 15;    break; // light white
                            default:
                                   ; // No-op for other color codes
                            }
                            // clang-format on
                        } else {
                            // Oops failure - return to default color
                            fg = 0;
                        }
                    }

                    bool isBgOk = false;
                    int bg = 0;
                    if (match.captured(2) == QLatin1String("DEFAULT")) {
                        // Use the old value for default which is 0 (though we use
                        // -1 internally)
                        isBgOk = true;
                    } else if (match.captured(2) == QLatin1String("IGNORE")) {
                        // Ignore is NOT handled by old system but use -2 (the value
                        // we use internally)
                        isBgOk = true;
                        bg = -2;
                    } else {
                        bg = match.captured(2).toInt(&isBgOk);
                        if (isBgOk) {
                            // clang-format off
                            switch (bg) {
                            case 0:     bg = 2;     break; // black
                            case 1:     bg = 4;     break; // red
                            case 2:     bg = 6;     break; // green
                            case 3:     bg = 8;     break; // yellow
                            case 4:     bg = 10;    break; // blue
                            case 5:     bg = 12;    break; // magenta
                            case 6:     bg = 14;    break; // cyan
                            case 7:     bg = 16;    break; // white (light gray)
                            case 8:     bg = 1;     break; // light black (dark gray)
                            case 9:     bg = 3;     break; // light red
                            case 10:    bg = 5;     break; // light green
                            case 11:    bg = 7;     break; // light yellow
                            case 12:    bg = 9;     break; // light blue
                            case 13:    bg = 11;    break; // light magenta
                            case 14:    bg = 13;    break; // light cyan
                            case 15:    bg = 15;    break; // light white
                            default:
                                   ; // No-op for other color codes
                            }
                            // clang-format on
                        } else {
                            // Oops failure - return to default color
                            bg = 0;
                        }
                    }

                    if (!isFgOk) {
                        qDebug() << "XMLexport::remapAnsiToColorNumber(...) ERROR - failed to extract FG color code from pattern text:" << itPattern.peekPrevious() << " setting colour to default foreground";
                    }
                    if (!isBgOk) {
                        qDebug() << "XMLexport::remapAnsiToColorNumber(...) ERROR - failed to extract BG color code from pattern text:" << itPattern.peekPrevious() << " setting colour to default background";
                    }

                    results << QStringLiteral("FG%1BG%2").arg(QString::number(fg), QString::number(bg));
                } else {
                    // No match - so insert previous string - which will cause
                    // a failure when it gets loaded as a REGEX_COLOR_PATTERN
                    // again...
                    results << itPattern.peekPrevious();
                }
            } else {
                // Empty pattern
                results << QString();
            }
        } else {
            // Copy across the pattern as it isn't a colour pattern
            results << itPattern.next();
        }
    }

    return results;
}
