/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TVar.h"
#include "VarUnit.h"
#include "mudlet.h"

using namespace std;

XMLexport::XMLexport( Host * pH )
: mpHost( pH )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpKey( Q_NULLPTR )
{
    setAutoFormatting(true);
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
    setAutoFormatting(true);
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
    setAutoFormatting(true);
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
    setAutoFormatting(true);
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
    setAutoFormatting(true);
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
    setAutoFormatting(true);
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
    setAutoFormatting(true);
}

bool XMLexport::writeModuleXML(QIODevice* device, QString moduleName)
{
    Host* pHost = mpHost;
    bool isOk = true;
    bool isNodeWritten = false;

    setDevice(device);

    writeStartDocument();
    // Assume that if there is a file writing problem it will show up on first
    // write to file...
    if (hasError() || !pHost) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    if (isOk) {
        writeStartElement("TriggerPackage");
        //we go a level down for all these functions so as to not infinitely nest the module
        for (auto it = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); isOk && it != pHost->mTriggerUnit.mTriggerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if (!(*it)->isTemporary() && (*it)->mModuleMember) {
                if (!writeTrigger(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </TriggerPackage>
        }
    }

    if (isOk) {
        writeStartElement("TimerPackage");
        for (auto it = pHost->mTimerUnit.mTimerRootNodeList.begin(); isOk && it != pHost->mTimerUnit.mTimerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if (!(*it)->isTemporary() && (*it)->mModuleMember) {
                if (!writeTimer(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </TimerPackage>
        }
    }

    if (isOk) {
        writeStartElement("AliasPackage");
        for (auto it = pHost->mAliasUnit.mAliasRootNodeList.begin(); isOk && it != pHost->mAliasUnit.mAliasRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if (!(*it)->isTemporary() && (*it)->mModuleMember) {
                if (!writeAlias(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </AliasPackage>
        }
    }

    if (isOk) {
        writeStartElement("ActionPackage");
        for (auto it = pHost->mActionUnit.mActionRootNodeList.begin(); isOk && it != pHost->mActionUnit.mActionRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if ((*it)->mModuleMember) {
                if (!writeAction(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </ActionPackage>
        }
    }

    if (isOk) {
        writeStartElement("ScriptPackage");
        for (auto it = pHost->mScriptUnit.mScriptRootNodeList.begin(); isOk && it != pHost->mScriptUnit.mScriptRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if ((*it)->mModuleMember) {
                if (!writeScript(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </ScriptPackage>
        }
    }

    if (isOk) {
        writeStartElement("KeyPackage");
        for (auto it = pHost->mKeyUnit.mKeyRootNodeList.begin(); isOk && it != pHost->mKeyUnit.mKeyRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mPackageName != moduleName) {
                continue;
            }
            if (!(*it)->isTemporary() && (*it)->mModuleMember) {
                if (!writeKey(*it, pugi::xml_node())) {
                    isOk = false;
                } else {
                    isNodeWritten = true;
                }
            }
        }
        if (isNodeWritten) {
            isNodeWritten = false;
        } else {
            writeEndElement(); // </KeyPackage>
        }
    }

    if (isOk) {
        writeStartElement("HelpPackage");
        if (pHost->moduleHelp.contains(moduleName) && pHost->moduleHelp.value(moduleName).contains("helpURL")) {
            writeTextElement("helpURL", pHost->moduleHelp.value(moduleName).value("helpURL"));
        } else {
            writeEmptyElement("helpURL");
        }
        writeEndElement(); // </HelpPackage>
    }

    //     writeEndElement();//end hostpackage - NOT NEEDED HERE!
    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportHost(QIODevice* device, const QString& filename_pugi_xml)
{
    Q_UNUSED(device)

    auto mMudletPackageNode = mExportDoc.append_child("MudletPackage");
    mMudletPackageNode.append_attribute("version") = mudlet::self()->scmMudletXmlDefaultVersion.toLocal8Bit().data();

    if (writeHost(mpHost, mMudletPackageNode)) {
        mExportDoc.save_file(filename_pugi_xml.toLocal8Bit().data());
        return true;
    }

    return false;
}

void XMLexport::showXmlDebug()
{
    std::cout << "Document:\n";
    mExportDoc.save(std::cout);
    std::cout << endl;
}

bool XMLexport::writeHost(Host *pHost, pugi::xml_node mMudletPackageNode)
{
    bool isOk = true;

    auto hostPackage = mMudletPackageNode.append_child("HostPackage");
    auto host = hostPackage.append_child("Host");

    host.append_attribute("autoClearCommandLineAfterSend") = pHost->mAutoClearCommandLineAfterSend ? "yes" : "no";
    host.append_attribute("disableAutoCompletion") = pHost->mDisableAutoCompletion ? "yes" : "no";
    host.append_attribute("printCommand") = pHost->mPrintCommand ? "yes" : "no";
    host.append_attribute("USE_IRE_DRIVER_BUGFIX") = pHost->mUSE_IRE_DRIVER_BUGFIX ? "yes" : "no";
    host.append_attribute("mUSE_FORCE_LF_AFTER_PROMPT") = pHost->mUSE_FORCE_LF_AFTER_PROMPT ? "yes" : "no";
    host.append_attribute("mUSE_UNIX_EOL") = pHost->mUSE_UNIX_EOL ? "yes" : "no";
    host.append_attribute("mNoAntiAlias") = pHost->mNoAntiAlias ? "yes" : "no";
    host.append_attribute("mEchoLuaErrors") = pHost->mEchoLuaErrors ? "yes" : "no";
    // FIXME: Change to a string or integer property when possible to support more
    // than false (perhaps 0 or "PlainText") or true (perhaps 1 or "HTML") in the
    // future - phpBB code might be useful if it can be done.
    host.append_attribute("mRawStreamDump") = pHost->mIsNextLogFileInHtmlFormat ? "yes" : "no";
    host.append_attribute("mIsLoggingTimestamps") = pHost->mIsLoggingTimestamps ? "yes" : "no";
    host.append_attribute("mAlertOnNewData") = pHost->mAlertOnNewData ? "yes" : "no";
    host.append_attribute("mFORCE_NO_COMPRESSION") = pHost->mFORCE_NO_COMPRESSION ? "yes" : "no";
    host.append_attribute("mFORCE_GA_OFF") = pHost->mFORCE_GA_OFF ? "yes" : "no";
    host.append_attribute("mFORCE_SAVE_ON_EXIT") = pHost->mFORCE_SAVE_ON_EXIT ? "yes" : "no";
    host.append_attribute("mEnableGMCP") = pHost->mEnableGMCP ? "yes" : "no";
    host.append_attribute("mEnableMSDP") = pHost->mEnableMSDP ? "yes" : "no";
    host.append_attribute("mMapStrongHighlight") = pHost->mMapStrongHighlight ? "yes" : "no";
    host.append_attribute("mLogStatus") = pHost->mLogStatus ? "yes" : "no";
    host.append_attribute("mEnableSpellCheck") = pHost->mEnableSpellCheck ? "yes" : "no";
    host.append_attribute("mShowInfo") = pHost->mShowInfo ? "yes" : "no";
    host.append_attribute("mAcceptServerGUI") = pHost->mAcceptServerGUI ? "yes" : "no";
    host.append_attribute("mMapperUseAntiAlias") = pHost->mMapperUseAntiAlias ? "yes" : "no";
    host.append_attribute("mFORCE_MXP_NEGOTIATION_OFF") = pHost->mFORCE_MXP_NEGOTIATION_OFF ? "yes" : "no";
    host.append_attribute("mRoomSize") = QString::number(pHost->mRoomSize, 'f', 1).toLocal8Bit().data();
    host.append_attribute("mLineSize") = QString::number(pHost->mLineSize, 'f', 1).toLocal8Bit().data();
    host.append_attribute("mBubbleMode") = pHost->mBubbleMode ? "yes" : "no";
    host.append_attribute("mShowRoomIDs") = pHost->mShowRoomID ? "yes" : "no";
    host.append_attribute("mShowPanel") = pHost->mShowPanel ? "yes" : "no";
    host.append_attribute("mHaveMapperScript") = pHost->mHaveMapperScript ? "yes" : "no";
    host.append_attribute("mEditorTheme") = pHost->mEditorTheme.toLocal8Bit().data();
    host.append_attribute("mEditorThemeFile") = pHost->mEditorThemeFile.toLocal8Bit().data();
    host.append_attribute("mThemePreviewItemID") = QString::number(pHost->mThemePreviewItemID).toLocal8Bit().data();
    host.append_attribute("mThemePreviewType") = pHost->mThemePreviewType.toLocal8Bit().data();
    host.append_attribute("mSearchEngineName") = pHost->mSearchEngineName.toLocal8Bit().data();

    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while (it.hasNext()) {
        ignore = ignore.append(it.next());
    }
    host.append_attribute("mDoubleClickIgnore") = ignore.toLocal8Bit().data();

    { // Blocked so that indentation reflects that of the XML file
        host.append_child("name").text().set(pHost->mHostName.toLocal8Bit().data());

        auto mInstalledPackages = host.append_child("mInstalledPackages");

        for (int i = 0; i < pHost->mInstalledPackages.size(); ++i) {
            mInstalledPackages.append_child("string").text().set(pHost->mInstalledPackages.at(i).toLocal8Bit().data());
        }

        if (pHost->mInstalledModules.size()) {
            auto mInstalledModules = host.append_child("mInstalledModules");
            QMapIterator<QString, QStringList> it(pHost->mInstalledModules);
            pHost->modulesToWrite.clear();
            while (it.hasNext()) {
                it.next();
                mInstalledModules.append_child("key").text().set(it.key().toLocal8Bit().data());
                QStringList entry = it.value();
                mInstalledModules.append_child("filepath").text().set(entry.at(0).toLocal8Bit().data());
                mInstalledModules.append_child("globalSave").text().set(entry.at(1).toLocal8Bit().data());
                if (entry.at(1).toInt()) {
                    pHost->modulesToWrite.insert(it.key(), entry);
                }
                mInstalledModules.append_child("priority").text().set(QString::number(pHost->mModulePriorities.value(it.key())).toLocal8Bit().data());
            }
        }

        host.append_child("url").text().set(pHost->mUrl.toLocal8Bit().data());
        host.append_child("serverPackageName").text().set(pHost->mServerGUI_Package_name.toLocal8Bit().data());
        host.append_child("serverPackageVersion").text().set(QString::number(pHost->mServerGUI_Package_version).toLocal8Bit().data());
        host.append_child("port").text().set(QString::number(pHost->mPort).toLocal8Bit().data());
        host.append_child("borderTopHeight").text().set(QString::number(pHost->mBorderTopHeight).toLocal8Bit().data());
        host.append_child("borderBottomHeight").text().set(QString::number(pHost->mBorderBottomHeight).toLocal8Bit().data());
        host.append_child("borderLeftWidth").text().set(QString::number(pHost->mBorderLeftWidth).toLocal8Bit().data());
        host.append_child("borderRightWidth").text().set(QString::number(pHost->mBorderRightWidth).toLocal8Bit().data());
        host.append_child("wrapAt").text().set(QString::number(pHost->mWrapAt).toLocal8Bit().data());
        host.append_child("wrapIndentCount").text().set(QString::number(pHost->mWrapIndentCount).toLocal8Bit().data());
        host.append_child("mFgColor").text().set(pHost->mFgColor.name().toLocal8Bit().data());
        host.append_child("mBgColor").text().set(pHost->mBgColor.name().toLocal8Bit().data());
        host.append_child("mCommandFgColor").text().set(pHost->mCommandFgColor.name().toLocal8Bit().data());
        host.append_child("mCommandBgColor").text().set(pHost->mCommandBgColor.name().toLocal8Bit().data());
        host.append_child("mCommandLineFgColor").text().set(pHost->mCommandLineFgColor.name().toLocal8Bit().data());
        host.append_child("mCommandLineBgColor").text().set(pHost->mCommandLineBgColor.name().toLocal8Bit().data());
        host.append_child("mBlack").text().set(pHost->mBlack.name().toLocal8Bit().data());
        host.append_child("mLightBlack").text().set(pHost->mLightBlack.name().toLocal8Bit().data());
        host.append_child("mRed").text().set(pHost->mRed.name().toLocal8Bit().data());
        host.append_child("mLightRed").text().set(pHost->mLightRed.name().toLocal8Bit().data());
        host.append_child("mBlue").text().set(pHost->mBlue.name().toLocal8Bit().data());
        host.append_child("mLightBlue").text().set(pHost->mLightBlue.name().toLocal8Bit().data());
        host.append_child("mGreen").text().set(pHost->mGreen.name().toLocal8Bit().data());
        host.append_child("mLightGreen").text().set(pHost->mLightGreen.name().toLocal8Bit().data());
        host.append_child("mYellow").text().set(pHost->mYellow.name().toLocal8Bit().data());
        host.append_child("mLightYellow").text().set(pHost->mLightYellow.name().toLocal8Bit().data());
        host.append_child("mCyan").text().set(pHost->mCyan.name().toLocal8Bit().data());
        host.append_child("mLightCyan").text().set(pHost->mLightCyan.name().toLocal8Bit().data());
        host.append_child("mMagenta").text().set(pHost->mMagenta.name().toLocal8Bit().data());
        host.append_child("mLightMagenta").text().set(pHost->mLightMagenta.name().toLocal8Bit().data());
        host.append_child("mWhite").text().set(pHost->mWhite.name().toLocal8Bit().data());
        host.append_child("mLightWhite").text().set(pHost->mLightWhite.name().toLocal8Bit().data());
        host.append_child("mDisplayFont").text().set(pHost->mDisplayFont.toString().toLocal8Bit().data());
        host.append_child("mCommandLineFont").text().set(pHost->mCommandLineFont.toString().toLocal8Bit().data());
        // There was a mis-spelt duplicate commandSeperator above but it is now gone
        host.append_child("mCommandSeparator").text().set(pHost->mCommandSeparator.toLocal8Bit().data());
        host.append_child("commandLineMinimumHeight").text().set(QString::number(pHost->commandLineMinimumHeight).toLocal8Bit().data());

        host.append_child("mFgColor2").text().set(pHost->mFgColor_2.name().toLocal8Bit().data());
        host.append_child("mBgColor2").text().set(pHost->mBgColor_2.name().toLocal8Bit().data());
        host.append_child("mBlack2").text().set(pHost->mBlack_2.name().toLocal8Bit().data());
        host.append_child("mLightBlack2").text().set(pHost->mLightBlack_2.name().toLocal8Bit().data());
        host.append_child("mRed2").text().set(pHost->mRed_2.name().toLocal8Bit().data());
        host.append_child("mLightRed2").text().set(pHost->mLightRed_2.name().toLocal8Bit().data());
        host.append_child("mBlue2").text().set(pHost->mBlue_2.name().toLocal8Bit().data());
        host.append_child("mLightBlue2").text().set(pHost->mLightBlue_2.name().toLocal8Bit().data());
        host.append_child("mGreen2").text().set(pHost->mGreen_2.name().toLocal8Bit().data());
        host.append_child("mLightGreen2").text().set(pHost->mLightGreen_2.name().toLocal8Bit().data());
        host.append_child("mYellow2").text().set(pHost->mYellow_2.name().toLocal8Bit().data());
        host.append_child("mLightYellow2").text().set(pHost->mLightYellow_2.name().toLocal8Bit().data());
        host.append_child("mCyan2").text().set(pHost->mCyan_2.name().toLocal8Bit().data());
        host.append_child("mLightCyan2").text().set(pHost->mLightCyan_2.name().toLocal8Bit().data());
        host.append_child("mMagenta2").text().set(pHost->mMagenta_2.name().toLocal8Bit().data());
        host.append_child("mLightMagenta2").text().set(pHost->mLightMagenta_2.name().toLocal8Bit().data());
        host.append_child("mWhite2").text().set(pHost->mWhite_2.name().toLocal8Bit().data());
        host.append_child("mLightWhite2").text().set(pHost->mLightWhite_2.name().toLocal8Bit().data());
        host.append_child("mSpellDic").text().set(pHost->mSpellDic.toLocal8Bit().data());
        // TODO: Consider removing these sub-elements that duplicate the same
        // attributes - which WERE bugged - when we update the XML format, must leave
        // them in place for now even though we no longer use them for compatibility
        // with older version of Mudlet
        host.append_child("mLineSize").text().set(QString::number(pHost->mLineSize, 'f', 1).toLocal8Bit().data());
        host.append_child("mRoomSize").text().set(QString::number(pHost->mRoomSize, 'f', 1).toLocal8Bit().data());
    }

    // Use if() to block each XXXXPackage element to limit scope of iterator so
    // we can use more of the same code in each block - and to escape quicker on
    // error...
    if (isOk) {
        auto triggerPackageNode = mMudletPackageNode.append_child("TriggerPackage");
        for (auto it = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); isOk && it != pHost->mTriggerUnit.mTriggerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTemporary()) {
                if (!writeTrigger(*it, triggerPackageNode)) {
                    isOk = false;
                }
            }
        }
    }

    if (isOk) {
        auto timerPackageNode = mMudletPackageNode.append_child("TimerPackage");
        for (auto it = pHost->mTimerUnit.mTimerRootNodeList.begin(); isOk && it != pHost->mTimerUnit.mTimerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTemporary()) {
                if (!writeTimer(*it, timerPackageNode)) {
                    isOk = false;
                }
            }
        }
    }

    if (isOk) {
        auto aliasPackageNode = mMudletPackageNode.append_child("AliasPackage");
        for (auto it = pHost->mAliasUnit.mAliasRootNodeList.begin(); isOk && it != pHost->mAliasUnit.mAliasRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTemporary()) {
                if (!writeAlias(*it, aliasPackageNode)) {
                    isOk = false;
                }
            }
        }
    }

    if (isOk) {
        auto actionPackageNode = mMudletPackageNode.append_child("ActionPackage");
        for (auto it = pHost->mActionUnit.mActionRootNodeList.begin(); isOk && it != pHost->mActionUnit.mActionRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!writeAction(*it, actionPackageNode)) {
                isOk = false;
            }
        }
    }

    if (isOk) {
        auto scriptPackageNode = mMudletPackageNode.append_child("ScriptPackage");
        for (auto it = pHost->mScriptUnit.mScriptRootNodeList.begin(); isOk && it != pHost->mScriptUnit.mScriptRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!writeScript(*it, scriptPackageNode)) {
                isOk = false;
            }
        }
    }

    if (isOk) {
        auto keyPackageNode = mMudletPackageNode.append_child("KeyPackage");
        for( auto it = pHost->mKeyUnit.mKeyRootNodeList.begin(); isOk && it != pHost->mKeyUnit.mKeyRootNodeList.end(); ++it ) {
            if( ! (*it) || (*it)->isTemporary() || (*it)->mModuleMember) {
                continue;
            }
            if (!writeKey(*it, keyPackageNode)) {
                isOk = false;
            }
        }
    }

    if (isOk) {
        auto variablePackageNode = mMudletPackageNode.append_child("VariablePackage");
        LuaInterface* lI = pHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        //do hidden variables first
        { // Blocked so that indentation reflects that of the XML file
            auto hiddenVariablesNode = variablePackageNode.append_child("HiddenVariables");
            QSetIterator<QString> itHiddenVariableName(vu->hiddenByUser);
            while (itHiddenVariableName.hasNext()) {
                auto variableName = itHiddenVariableName.next();
                hiddenVariablesNode.append_child("name").text().set(variableName.toLocal8Bit().data());
            }
        }

        TVar* base = vu->getBase();
        if (!base) {
            lI->getVars(false);
            base = vu->getBase();
        }

        if (base) {
            QListIterator<TVar*> itVariable(base->getChildren(false));
            while (isOk && itVariable.hasNext()) {
                if (!writeVariable(itVariable.next(), lI, vu, variablePackageNode)) {
                    isOk = false;
                }
            }
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::writeVariable(TVar *pVar, LuaInterface *pLuaInterface, VarUnit *pVariableUnit, pugi::xml_node xmlParent)
{
    bool isOk = true;
    if (pVariableUnit->isSaved(pVar)) {
        if (pVar->getValueType() == LUA_TTABLE) {
            auto variableGroupNode = xmlParent.append_child("VariableGroup");

            variableGroupNode.append_child("name").text().set(pVar->getName().toLocal8Bit().data());
            variableGroupNode.append_child("keyType").text().set(QString::number(pVar->getKeyType()).toLocal8Bit().data());
            variableGroupNode.append_child("value").text().set(pLuaInterface->getValue(pVar).toLocal8Bit().data());
            variableGroupNode.append_child("valueType").text().set(QString::number(pVar->getValueType()).toLocal8Bit().data());

            QListIterator<TVar*> itNestedVariable(pVar->getChildren(false));
            while (isOk && itNestedVariable.hasNext()) {
                if (!writeVariable(itNestedVariable.next(), pLuaInterface, pVariableUnit, variableGroupNode)) {
                    isOk = false;
                }
            }
        } else {
            auto variableNode = xmlParent.append_child("Variable");

            variableNode.append_child("name").text().set(pVar->getName().toLocal8Bit().data());
            variableNode.append_child("keyType").text().set(QString::number(pVar->getKeyType()).toLocal8Bit().data());
            variableNode.append_child("value").text().set(pLuaInterface->getValue(pVar).toLocal8Bit().data());
            variableNode.append_child("valueType").text().set(QString::number(pVar->getValueType()).toLocal8Bit().data());
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportGenericPackage(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    bool isOk = writeGenericPackage(mpHost);

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeGenericPackage(Host* pHost)
{
    bool isOk = true;
    if (isOk) {
        writeStartElement("TriggerPackage");
        for (auto it = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); isOk && it != pHost->mTriggerUnit.mTriggerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->isTemporary()) {
                continue;
            }
            if (!writeTrigger(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </TriggerPackage>
    }

    if (isOk) {
        writeStartElement("TimerPackage");
        for (auto it = pHost->mTimerUnit.mTimerRootNodeList.begin(); isOk && it != pHost->mTimerUnit.mTimerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->isTemporary()) {
                continue;
            }
            if (!writeTimer(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </TimerPackage>
    }

    if (isOk) {
        writeStartElement("AliasPackage");
        for (auto it = pHost->mAliasUnit.mAliasRootNodeList.begin(); isOk && it != pHost->mAliasUnit.mAliasRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->isTemporary()) {
                continue;
            }
            if (!writeAlias(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </AliasPackage>
    }

    if (isOk) {
        writeStartElement("ActionPackage");
        for (auto it = pHost->mActionUnit.mActionRootNodeList.begin(); isOk && it != pHost->mActionUnit.mActionRootNodeList.end(); ++it) {
            if (!(*it)) {
                continue;
            }
            if (!writeAction(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </ActionPackage>
    }

    if (isOk) {
        writeStartElement("ScriptPackage");
        for (auto it = pHost->mScriptUnit.mScriptRootNodeList.begin(); isOk && it != pHost->mScriptUnit.mScriptRootNodeList.end(); ++it) {
            if (!(*it)) {
                continue;
            }
            if (!writeScript(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </ScriptPackage>
    }

    if( isOk ) {
        writeStartElement( "KeyPackage" );
        for( auto it = pHost->mKeyUnit.mKeyRootNodeList.begin(); isOk && it != pHost->mKeyUnit.mKeyRootNodeList.end(); ++it ) {
            if( ! (*it) || (*it)->isTemporary() ) {
                continue;
            }
            if (!writeKey(*it, pugi::xml_node())) {
                isOk = false;
            }
        }
        writeEndElement(); // </KeyPackage>
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportTrigger(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("TriggerPackage");
    bool isOk = writeTrigger(mpTrigger, pugi::xml_node());
    writeEndElement(); // </TriggerPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TTrigger* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpTrigger) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("TriggerPackage");
    bool isOk = writeTrigger(mpTrigger, pugi::xml_node());
    writeEndElement(); //TriggerPackage

    writeEndElement(); //MudletPackage
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeTrigger(TTrigger *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;
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
        trigger.append_attribute("isColorTriggerFg") = pT->mColorTriggerFg ? "yes" : "no";
        trigger.append_attribute("isColorTriggerBg") = pT->mColorTriggerBg ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file

            trigger.append_child("name").text().set(pT->mName.toLocal8Bit().data());
            writeScriptElement(pT->mScript, trigger);

            trigger.append_child("triggerType").text().set(QString::number(pT->mTriggerType).toLocal8Bit().data());
            trigger.append_child("conditonLineDelta").text().set(QString::number(pT->mConditionLineDelta).toLocal8Bit().data());
            trigger.append_child("mStayOpen").text().set(QString::number(pT->mStayOpen).toLocal8Bit().data());
            trigger.append_child("mCommand").text().set(pT->mCommand.toLocal8Bit().data());
            trigger.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            trigger.append_child("mFgColor").text().set(pT->mFgColor.name().toLocal8Bit().data());
            trigger.append_child("mBgColor").text().set(pT->mBgColor.name().toLocal8Bit().data());
            trigger.append_child("mSoundFile").text().set(pT->mSoundFile.toLocal8Bit().data());
            trigger.append_child("colorTriggerFgColor").text().set(pT->mColorTriggerFgColor.name().toLocal8Bit().data());
            trigger.append_child("colorTriggerBgColor").text().set(pT->mColorTriggerBgColor.name().toLocal8Bit().data());

            auto regexCodeList = trigger.append_child("regexCodeList");
            for (int i = 0; i < pT->mRegexCodeList.size(); ++i) {
                regexCodeList.append_child("string").text().set(pT->mRegexCodeList.at(i).toLocal8Bit().data());
            }

            auto regexCodePropertyList = trigger.append_child("regexCodePropertyList");
            for (int i : pT->mRegexCodePropertyList) {
                regexCodePropertyList.append_child("integer").text().set(QString::number(i).toLocal8Bit().data());
            }
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeTrigger(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportAlias(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("AliasPackage");
    bool isOk = writeAlias(mpAlias, pugi::xml_node());
    writeEndElement(); // </AliasPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TAlias* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpAlias) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    // autoFormatting is set to true in constructor

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("AliasPackage");
    bool isOk = writeAlias(mpAlias, pugi::xml_node());
    writeEndElement(); // </AliasPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeAlias(TAlias *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto aliasContentsNode = xmlParent.append_child(pT->isFolder() ? "AliasGroup" : "Alias");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = aliasContentsNode;

        aliasContentsNode.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        aliasContentsNode.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            aliasContentsNode.append_child("name").text().set(pT->mName.toLocal8Bit().data());
            writeScriptElement(pT->mScript, aliasContentsNode);

            aliasContentsNode.append_child("command").text().set(pT->mCommand.toLocal8Bit().data());
            aliasContentsNode.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            aliasContentsNode.append_child("regex").text().set(pT->mRegexCode.toLocal8Bit().data());
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeAlias(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportAction(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("ActionPackage");
    bool isOk = writeAction(mpAction, pugi::xml_node());
    writeEndElement(); // </ActionPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TAction* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpAction) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    // autoFormatting is set to true in constructor

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("ActionPackage");
    bool isOk = writeAction(mpAction, pugi::xml_node());
    writeEndElement(); // </ActionPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeAction(TAction *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;    
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto actionContentsNode = xmlParent.append_child(pT->isFolder() ? "ActionGroup" : "Action");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = actionContentsNode;

        actionContentsNode.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        actionContentsNode.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";
        actionContentsNode.append_attribute("isPushButton") = pT->mIsPushDownButton ? "yes" : "no";
        actionContentsNode.append_attribute("isFlatButton") = pT->mButtonFlat ? "yes" : "no";
        actionContentsNode.append_attribute("useCustomLayout") = pT->mUseCustomLayout ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            actionContentsNode.append_child("name").text().set(pT->mName.toLocal8Bit().data());
            actionContentsNode.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            writeScriptElement(pT->mScript, actionContentsNode);

            actionContentsNode.append_child("css").text().set(pT->css.toLocal8Bit().data());
            actionContentsNode.append_child("commandButtonUp").text().set(pT->mCommandButtonUp.toLocal8Bit().data());
            actionContentsNode.append_child("commandButtonDown").text().set(pT->mCommandButtonDown.toLocal8Bit().data());
            actionContentsNode.append_child("icon").text().set(pT->mIcon.toLocal8Bit().data());
            actionContentsNode.append_child("orientation").text().set(QString::number(pT->mOrientation).toLocal8Bit().data());
            actionContentsNode.append_child("location").text().set(QString::number(pT->mLocation).toLocal8Bit().data());
            actionContentsNode.append_child("posX").text().set(QString::number(pT->mPosX).toLocal8Bit().data());
            actionContentsNode.append_child("posY").text().set(QString::number(pT->mPosY).toLocal8Bit().data());
            // We now use a boolean but file must use original "1" (false)
            // or "2" (true) for backward compatibility
            actionContentsNode.append_child("mButtonState").text().set(QString::number(pT->mButtonState ? 2 : 1).toLocal8Bit().data());
            actionContentsNode.append_child("sizeX").text().set(QString::number(pT->mSizeX).toLocal8Bit().data());
            actionContentsNode.append_child("sizeY").text().set(QString::number(pT->mSizeY).toLocal8Bit().data());
            actionContentsNode.append_child("buttonColumn").text().set(QString::number(pT->mButtonColumns).toLocal8Bit().data());
            actionContentsNode.append_child("buttonRotation").text().set(QString::number(pT->mButtonRotation).toLocal8Bit().data());
            actionContentsNode.append_child("buttonColor").text().set(pT->mButtonColor.name().toLocal8Bit().data());
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeAction(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportTimer(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("TimerPackage");
    bool isOk = writeTimer(mpTimer, pugi::xml_node());
    writeEndElement(); // </TimerPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TTimer* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpTimer) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    // autoFormatting is set to true in constructor

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("TimerPackage");
    bool isOk = writeTimer(mpTimer, pugi::xml_node());
    writeEndElement(); // </TimerPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeTimer(TTimer *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto timerContentsNode = xmlParent.append_child(pT->isFolder() ? "TimerGroup" : "Timer");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = timerContentsNode;

        timerContentsNode.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        timerContentsNode.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";
        timerContentsNode.append_attribute("isTempTimer") = pT->isTemporary() ? "yes" : "no";
        timerContentsNode.append_attribute("isOffsetTimer") = pT->isOffsetTimer() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            timerContentsNode.append_child("name").text().set(pT->mName.toLocal8Bit().data());

            writeScriptElement(pT->mScript, timerContentsNode);

            timerContentsNode.append_child("command").text().set(pT->mCommand.toLocal8Bit().data());
            timerContentsNode.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            timerContentsNode.append_child("time").text().set(pT->mTime.toString("hh:mm:ss.zzz").toLocal8Bit().data());
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeTimer(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportScript(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("ScriptPackage");
    bool isOk = writeScript(mpScript, pugi::xml_node());
    writeEndElement(); // </ScriptPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TScript* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpScript) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    // autoFormatting is set to true in constructor

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("ScriptPackage");
    bool isOk = writeScript(mpScript, pugi::xml_node());
    writeEndElement(); // </ScriptPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeScript(TScript *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;    
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto scriptContentsNode = xmlParent.append_child(pT->isFolder() ? "ScriptGroup" : "Script");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = scriptContentsNode;

        scriptContentsNode.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        scriptContentsNode.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            scriptContentsNode.append_child("name").text().set(pT->mName.toLocal8Bit().data());
            scriptContentsNode.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            writeScriptElement(pT->mScript, scriptContentsNode);

            auto eventHandlerList = scriptContentsNode.append_child("eventHandlerList");
            for (int i = 0; i < pT->mEventHandlerList.size(); ++i) {
                eventHandlerList.append_child("string").text().set(pT->mEventHandlerList.at(i).toLocal8Bit().data());
            }
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); it++) {
        if (!writeScript(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::exportKey(QIODevice* device)
{
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("KeyPackage");
    bool isOk = writeKey(mpKey, pugi::xml_node());
    writeEndElement(); // </KeyPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::exportToClipboard(TKey* pT)
{
    // The use of pT is a cludge - it was already used in the previously invoked
    // in this XMLexport instance's constructor (and stored in mpKey) and it
    // is only used here for its signature.
    Q_UNUSED(pT);

    // autoFormatting is set to true in constructor

    QBuffer xmlBuffer;
    // set the device explicitly so QXmlStreamWriter knows where to write to
    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::WriteOnly);

    writeStartDocument();
    if (hasError()) {
        xmlBuffer.close();
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("KeyPackage");
    bool isOk = writeKey(mpKey, pugi::xml_node());
    writeEndElement(); // </KeyPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    if (!isOk || hasError()) {
        xmlBuffer.close();
        return false;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(QString(xmlBuffer.buffer()), QClipboard::Clipboard);

    xmlBuffer.close();
    return true;
}

bool XMLexport::writeKey(TKey *pT, pugi::xml_node xmlParent)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        auto keyContentsNode = xmlParent.append_child(pT->isFolder() ? "KeyGroup" : "Key");

        // set the xml parent to the trigger we just made so nested triggers will get written to it
        xmlParent = keyContentsNode;

        keyContentsNode.append_attribute("isActive") = pT->shouldBeActive() ? "yes" : "no";
        keyContentsNode.append_attribute("isFolder") = pT->isFolder() ? "yes" : "no";

        { // Blocked so that indentation reflects that of the XML file
            keyContentsNode.append_child("name").text().set(pT->mName.toLocal8Bit().data());
            keyContentsNode.append_child("packageName").text().set(pT->mPackageName.toLocal8Bit().data());
            writeScriptElement(pT->mScript, keyContentsNode);

            keyContentsNode.append_child("command").text().set(pT->mCommand.toLocal8Bit().data());
            keyContentsNode.append_child("keyCode").text().set(QString::number(pT->mKeyCode).toLocal8Bit().data());
            keyContentsNode.append_child("keyModifier").text().set(QString::number(pT->mKeyModifier).toLocal8Bit().data());
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeKey(*it, xmlParent)) {
            isOk = false;
        }
    }

    return (isOk && (!hasError()));
}

bool XMLexport::writeScriptElement(const QString &script, pugi::xml_node xmlElement)
{
    QString localScript = script;
    localScript.replace(QChar('\x01'), QStringLiteral("\xFFFC\x2401")); // SOH
    localScript.replace(QChar('\x02'), QStringLiteral("\xFFFC\x2402")); // STX
    localScript.replace(QChar('\x03'), QStringLiteral("\xFFFC\x2403")); // ETX
    localScript.replace(QChar('\x04'), QStringLiteral("\xFFFC\x2404")); // EOT
    localScript.replace(QChar('\x05'), QStringLiteral("\xFFFC\x2405")); // ENQ
    localScript.replace(QChar('\x06'), QStringLiteral("\xFFFC\x2406")); // ACK
    localScript.replace(QChar('\x07'), QStringLiteral("\xFFFC\x2407")); // BEL
    localScript.replace(QChar('\x08'), QStringLiteral("\xFFFC\x2408")); // BS
    localScript.replace(QChar('\x0B'), QStringLiteral("\xFFFC\x240B")); // VT
    localScript.replace(QChar('\x0C'), QStringLiteral("\xFFFC\x240C")); // FF
    localScript.replace(QChar('\x0E'), QStringLiteral("\xFFFC\x240E")); // SS
    localScript.replace(QChar('\x0F'), QStringLiteral("\xFFFC\x240F")); // SI
    localScript.replace(QChar('\x10'), QStringLiteral("\xFFFC\x2410")); // DLE
    localScript.replace(QChar('\x11'), QStringLiteral("\xFFFC\x2411")); // DC1
    localScript.replace(QChar('\x12'), QStringLiteral("\xFFFC\x2412")); // DC2
    localScript.replace(QChar('\x13'), QStringLiteral("\xFFFC\x2413")); // DC3
    localScript.replace(QChar('\x14'), QStringLiteral("\xFFFC\x2414")); // DC4
    localScript.replace(QChar('\x15'), QStringLiteral("\xFFFC\x2415")); // NAK
    localScript.replace(QChar('\x16'), QStringLiteral("\xFFFC\x2416")); // SYN
    localScript.replace(QChar('\x17'), QStringLiteral("\xFFFC\x2417")); // ETB
    localScript.replace(QChar('\x18'), QStringLiteral("\xFFFC\x2418")); // CAN
    localScript.replace(QChar('\x19'), QStringLiteral("\xFFFC\x2419")); // EM
    localScript.replace(QChar('\x1A'), QStringLiteral("\xFFFC\x241A")); // SUB
    localScript.replace(QChar('\x1B'), QStringLiteral("\xFFFC\x241B")); // ESC
    localScript.replace(QChar('\x1C'), QStringLiteral("\xFFFC\x241C")); // FS
    localScript.replace(QChar('\x1D'), QStringLiteral("\xFFFC\x241D")); // GS
    localScript.replace(QChar('\x1E'), QStringLiteral("\xFFFC\x241E")); // RS
    localScript.replace(QChar('\x1F'), QStringLiteral("\xFFFC\x241F")); // US
    localScript.replace(QChar('\x7F'), QStringLiteral("\xFFFC\x2421")); // DEL
    xmlElement.append_child("script").text().set(localScript.toLocal8Bit().data());

    return (!hasError());
}
