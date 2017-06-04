/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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
            if (!(*it)->isTempTrigger() && (*it)->mModuleMember) {
                if (!writeTrigger(*it)) {
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
            if (!(*it)->isTempTimer() && (*it)->mModuleMember) {
                if (!writeTimer(*it)) {
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
            if (!(*it)->isTempAlias() && (*it)->mModuleMember) {
                if (!writeAlias(*it)) {
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
                if (!writeAction(*it)) {
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
                if (!writeScript(*it)) {
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
            if (!(*it)->isTempKey() && (*it)->mModuleMember) {
                if (!writeKey(*it)) {
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

bool XMLexport::exportHost(QIODevice* device)
{
    bool isOk = true;
    setDevice(device);

    writeStartDocument();
    if (hasError()) {
        return false;
    }

    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement("MudletPackage");
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement("HostPackage");
    if (!writeHost(mpHost)) {
        isOk = false;
    }
    writeEndElement(); // </HostPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeHost(Host* pHost)
{
    bool isOk = true;
    writeStartElement("Host");

    writeAttribute("autoClearCommandLineAfterSend", pHost->mAutoClearCommandLineAfterSend ? "yes" : "no");
    writeAttribute("disableAutoCompletion", pHost->mDisableAutoCompletion ? "yes" : "no");
    writeAttribute("printCommand", pHost->mPrintCommand ? "yes" : "no");
    writeAttribute("USE_IRE_DRIVER_BUGFIX", pHost->mUSE_IRE_DRIVER_BUGFIX ? "yes" : "no");
    writeAttribute("mUSE_FORCE_LF_AFTER_PROMPT", pHost->mUSE_FORCE_LF_AFTER_PROMPT ? "yes" : "no");
    writeAttribute("mUSE_UNIX_EOL", pHost->mUSE_UNIX_EOL ? "yes" : "no");
    writeAttribute("mNoAntiAlias", pHost->mNoAntiAlias ? "yes" : "no");
    writeAttribute("mEchoLuaErrors", pHost->mEchoLuaErrors ? "yes" : "no");
    // FIXME: Change to a string or integer property when possible to support more
    // than false (perhaps 0 or "PlainText") or true (perhaps 1 or "HTML") in the
    // future - phpBB code might be useful if it can be done.
    writeAttribute("mRawStreamDump", pHost->mIsNextLogFileInHtmlFormat ? "yes" : "no");
    writeAttribute("mIsLoggingTimestamps", pHost->mIsLoggingTimestamps ? "yes" : "no");
    writeAttribute("mAlertOnNewData", pHost->mAlertOnNewData ? "yes" : "no");
    writeAttribute("mFORCE_NO_COMPRESSION", pHost->mFORCE_NO_COMPRESSION ? "yes" : "no");
    writeAttribute("mFORCE_GA_OFF", pHost->mFORCE_GA_OFF ? "yes" : "no");
    writeAttribute("mFORCE_SAVE_ON_EXIT", pHost->mFORCE_SAVE_ON_EXIT ? "yes" : "no");
    writeAttribute("mEnableGMCP", pHost->mEnableGMCP ? "yes" : "no");
    writeAttribute("mEnableMSDP", pHost->mEnableMSDP ? "yes" : "no");
    writeAttribute("mMapStrongHighlight", pHost->mMapStrongHighlight ? "yes" : "no");
    writeAttribute("mLogStatus", pHost->mLogStatus ? "yes" : "no");
    writeAttribute("mEnableSpellCheck", pHost->mEnableSpellCheck ? "yes" : "no");
    writeAttribute("mShowInfo", pHost->mShowInfo ? "yes" : "no");
    writeAttribute("mAcceptServerGUI", pHost->mAcceptServerGUI ? "yes" : "no");
    writeAttribute("mMapperUseAntiAlias", pHost->mMapperUseAntiAlias ? "yes" : "no");
    writeAttribute("mFORCE_MXP_NEGOTIATION_OFF", pHost->mFORCE_MXP_NEGOTIATION_OFF ? "yes" : "no");
    writeAttribute("mRoomSize", QString::number(pHost->mRoomSize, 'f', 1));
    writeAttribute("mLineSize", QString::number(pHost->mLineSize, 'f', 1));
    writeAttribute("mBubbleMode", pHost->mBubbleMode ? "yes" : "no");
    writeAttribute("mShowRoomIDs", pHost->mShowRoomID ? "yes" : "no");
    writeAttribute("mShowPanel", pHost->mShowPanel ? "yes" : "no");
    writeAttribute("mHaveMapperScript", pHost->mHaveMapperScript ? "yes" : "no");
    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while (it.hasNext()) {
        ignore = ignore.append(it.next());
    }
    writeAttribute("mDoubleClickIgnore", ignore);

    { // Blocked so that indentation reflects that of the XML file
        writeTextElement("name", pHost->mHostName);

        writeStartElement("mInstalledPackages");
        for (int i = 0; i < pHost->mInstalledPackages.size(); ++i) {
            writeTextElement("string", pHost->mInstalledPackages.at(i));
        }
        writeEndElement(); // </mInstalledPackages>

        if (pHost->mInstalledModules.size()) {
            writeStartElement("mInstalledModules");
            QMapIterator<QString, QStringList> it(pHost->mInstalledModules);
            pHost->modulesToWrite.clear();
            while (it.hasNext()) {
                it.next();
                writeTextElement("key", it.key());
                QStringList entry = it.value();
                writeTextElement("filepath", entry.at(0));
                writeTextElement("globalSave", entry.at(1));
                if (entry.at(1).toInt()) {
                    pHost->modulesToWrite.insert(it.key(), entry);
                }
                writeTextElement("priority", QString::number(pHost->mModulePriorities.value(it.key())));
            }
            writeEndElement(); // </mInstalledModules>
        }
// CHECK: Do we need:
//        else {
//            writeEmptyElement( "mInstalledModules" ); // i.e. <mInstalledModules />
//        }

        writeTextElement("url", pHost->mUrl);
        writeTextElement("serverPackageName", pHost->mServerGUI_Package_name);
        writeTextElement("serverPackageVersion", QString::number(pHost->mServerGUI_Package_version));
        writeTextElement("port", QString::number(pHost->mPort));
        writeTextElement("borderTopHeight", QString::number(pHost->mBorderTopHeight));
        writeTextElement("borderBottomHeight", QString::number(pHost->mBorderBottomHeight));
        writeTextElement("borderLeftWidth", QString::number(pHost->mBorderLeftWidth));
        writeTextElement("borderRightWidth", QString::number(pHost->mBorderRightWidth));
        writeTextElement("wrapAt", QString::number(pHost->mWrapAt));
        writeTextElement("wrapIndentCount", QString::number(pHost->mWrapIndentCount));
        writeTextElement("mFgColor", pHost->mFgColor.name());
        writeTextElement("mBgColor", pHost->mBgColor.name());
        writeTextElement("mCommandFgColor", pHost->mCommandFgColor.name());
        writeTextElement("mCommandBgColor", pHost->mCommandBgColor.name());
        writeTextElement("mCommandLineFgColor", pHost->mCommandLineFgColor.name());
        writeTextElement("mCommandLineBgColor", pHost->mCommandLineBgColor.name());
        writeTextElement("mBlack", pHost->mBlack.name());
        writeTextElement("mLightBlack", pHost->mLightBlack.name());
        writeTextElement("mRed", pHost->mRed.name());
        writeTextElement("mLightRed", pHost->mLightRed.name());
        writeTextElement("mBlue", pHost->mBlue.name());
        writeTextElement("mLightBlue", pHost->mLightBlue.name());
        writeTextElement("mGreen", pHost->mGreen.name());
        writeTextElement("mLightGreen", pHost->mLightGreen.name());
        writeTextElement("mYellow", pHost->mYellow.name());
        writeTextElement("mLightYellow", pHost->mLightYellow.name());
        writeTextElement("mCyan", pHost->mCyan.name());
        writeTextElement("mLightCyan", pHost->mLightCyan.name());
        writeTextElement("mMagenta", pHost->mMagenta.name());
        writeTextElement("mLightMagenta", pHost->mLightMagenta.name());
        writeTextElement("mWhite", pHost->mWhite.name());
        writeTextElement("mLightWhite", pHost->mLightWhite.name());
        writeTextElement("mDisplayFont", pHost->mDisplayFont.toString());
        writeTextElement("mCommandLineFont", pHost->mCommandLineFont.toString());
        // There was a mis-spelt duplicate commandSeperator above but it is now gone
        writeTextElement("mCommandSeparator", pHost->mCommandSeparator);
        writeTextElement("commandLineMinimumHeight", QString::number(pHost->commandLineMinimumHeight));

        writeTextElement("mFgColor2", pHost->mFgColor_2.name());
        writeTextElement("mBgColor2", pHost->mBgColor_2.name());
        writeTextElement("mBlack2", pHost->mBlack_2.name());
        writeTextElement("mLightBlack2", pHost->mLightBlack_2.name());
        writeTextElement("mRed2", pHost->mRed_2.name());
        writeTextElement("mLightRed2", pHost->mLightRed_2.name());
        writeTextElement("mBlue2", pHost->mBlue_2.name());
        writeTextElement("mLightBlue2", pHost->mLightBlue_2.name());
        writeTextElement("mGreen2", pHost->mGreen_2.name());
        writeTextElement("mLightGreen2", pHost->mLightGreen_2.name());
        writeTextElement("mYellow2", pHost->mYellow_2.name());
        writeTextElement("mLightYellow2", pHost->mLightYellow_2.name());
        writeTextElement("mCyan2", pHost->mCyan_2.name());
        writeTextElement("mLightCyan2", pHost->mLightCyan_2.name());
        writeTextElement("mMagenta2", pHost->mMagenta_2.name());
        writeTextElement("mLightMagenta2", pHost->mLightMagenta_2.name());
        writeTextElement("mWhite2", pHost->mWhite_2.name());
        writeTextElement("mLightWhite2", pHost->mLightWhite_2.name());
        writeTextElement("mSpellDic", pHost->mSpellDic);
        // TODO: Consider removing these sub-elements that duplicate the same
        // attributes - which WERE bugged - when we update the XML format, must leave
        // them in place for now even though we no longer use them for compatibility
        // with older version of Mudlet
        writeTextElement("mLineSize", QString::number(pHost->mLineSize, 'f', 1));
        writeTextElement("mRoomSize", QString::number(pHost->mRoomSize, 'f', 1));

        writeEndElement(); // </Host>
    }

    writeEndElement(); // </HostPackage>

    if (hasError()) {
        isOk = false;
    }

    // Use if() to block each XXXXPackage element to limit scope of iterator so
    // we can use more of the same code in each block - and to escape quicker on
    // error...
    if (isOk) {
        writeStartElement("TriggerPackage");
        for (auto it = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); isOk && it != pHost->mTriggerUnit.mTriggerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTempTrigger()) {
                if (!writeTrigger(*it)) {
                    isOk = false;
                }
            }
        }
        writeEndElement(); // </TriggerPackage>
    }

    if (isOk) {
        writeStartElement("TimerPackage");
        for (auto it = pHost->mTimerUnit.mTimerRootNodeList.begin(); isOk && it != pHost->mTimerUnit.mTimerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTempTimer()) {
                if (!writeTimer(*it)) {
                    isOk = false;
                }
            }
        }
        writeEndElement(); // </TimerPackage>
    }

    if (isOk) {
        writeStartElement("AliasPackage");
        for (auto it = pHost->mAliasUnit.mAliasRootNodeList.begin(); isOk && it != pHost->mAliasUnit.mAliasRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!(*it)->isTempAlias()) {
                if (!writeAlias(*it)) {
                    isOk = false;
                }
            }
        }
        writeEndElement(); // </AliasPackage>
    }

    if (isOk) {
        writeStartElement("ActionPackage");
        for (auto it = pHost->mActionUnit.mActionRootNodeList.begin(); isOk && it != pHost->mActionUnit.mActionRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!writeAction(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </ActionPackage>
    }

    if (isOk) {
        writeStartElement("ScriptPackage");
        for (auto it = pHost->mScriptUnit.mScriptRootNodeList.begin(); isOk && it != pHost->mScriptUnit.mScriptRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->mModuleMember) {
                continue;
            }
            if (!writeScript(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </ScriptPackage>
    }

    if (isOk) {
        writeStartElement("KeyPackage");
        for( auto it = pHost->mKeyUnit.mKeyRootNodeList.begin(); isOk && it != pHost->mKeyUnit.mKeyRootNodeList.end(); ++it ) {
            if( ! (*it) || (*it)->isTempKey() || (*it)->mModuleMember) {
                continue;
            }
            if (!writeKey(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </KeyPackage>
    }

    if (isOk) {
        writeStartElement("VariablePackage");
        LuaInterface* lI = pHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        //do hidden variables first
        { // Blocked so that indentation reflects that of the XML file
            writeStartElement("HiddenVariables");
            QSetIterator<QString> itHiddenVariableName(vu->hiddenByUser);
            while (itHiddenVariableName.hasNext()) {
                writeTextElement("name", itHiddenVariableName.next());
            }
            writeEndElement(); // </HiddenVariables>
        }

        TVar* base = vu->getBase();
        if (!base) {
            lI->getVars(false);
            base = vu->getBase();
        }

        if (base) {
            QListIterator<TVar*> itVariable(base->getChildren(false));
            while (isOk && itVariable.hasNext()) {
                if (!writeVariable(itVariable.next(), lI, vu)) {
                    isOk = false;
                }
            }
        }
        writeEndElement(); // </VariablePackage>
    }

    return (isOk && (!hasError()));
}

bool XMLexport::writeVariable(TVar* pVar, LuaInterface* pLuaInterface, VarUnit* pVariableUnit)
{
    bool isOk = true;
    if (pVariableUnit->isSaved(pVar)) {
        if (pVar->getValueType() == LUA_TTABLE) {
            writeStartElement("VariableGroup");
            writeTextElement("name", pVar->getName());
            writeTextElement("keyType", QString::number(pVar->getKeyType()));
            writeTextElement("value", pLuaInterface->getValue(pVar));
            writeTextElement("valueType", QString::number(pVar->getValueType()));
            QListIterator<TVar*> itNestedVariable(pVar->getChildren(false));
            while (isOk && itNestedVariable.hasNext()) {
                if (!writeVariable(itNestedVariable.next(), pLuaInterface, pVariableUnit)) {
                    isOk = false;
                }
            }
            writeEndElement(); // </VariableGroup>
        } else {
            writeStartElement("Variable");
            writeTextElement("name", pVar->getName());
            writeTextElement("keyType", QString::number(pVar->getKeyType()));
            writeTextElement("value", pLuaInterface->getValue(pVar));
            writeTextElement("valueType", QString::number(pVar->getValueType()));
            writeEndElement(); // </Variable>
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
            if (!(*it) || (*it)->isTempTrigger()) {
                continue;
            }
            if (!writeTrigger(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </TriggerPackage>
    }

    if (isOk) {
        writeStartElement("TimerPackage");
        for (auto it = pHost->mTimerUnit.mTimerRootNodeList.begin(); isOk && it != pHost->mTimerUnit.mTimerRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->isTempTimer()) {
                continue;
            }
            if (!writeTimer(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </TimerPackage>
    }

    if (isOk) {
        writeStartElement("AliasPackage");
        for (auto it = pHost->mAliasUnit.mAliasRootNodeList.begin(); isOk && it != pHost->mAliasUnit.mAliasRootNodeList.end(); ++it) {
            if (!(*it) || (*it)->isTempAlias()) {
                continue;
            }
            if (!writeAlias(*it)) {
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
            if (!writeAction(*it)) {
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
            if (!writeScript(*it)) {
                isOk = false;
            }
        }
        writeEndElement(); // </ScriptPackage>
    }

    if( isOk ) {
        writeStartElement( "KeyPackage" );
        for( auto it = pHost->mKeyUnit.mKeyRootNodeList.begin(); isOk && it != pHost->mKeyUnit.mKeyRootNodeList.end(); ++it ) {
            if( ! (*it) || (*it)->isTempKey() ) {
                continue;
            }
            if (!writeKey(*it)) {
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
    bool isOk = writeTrigger(mpTrigger);
    writeEndElement(); // </TriggerPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}


bool XMLexport::writeTrigger(TTrigger* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "TriggerGroup" : "Trigger");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");
        writeAttribute("isTempTrigger", pT->mIsTempTrigger ? "yes" : "no");
        writeAttribute("isMultiline", pT->mIsMultiline ? "yes" : "no");
        writeAttribute("isPerlSlashGOption", pT->mPerlSlashGOption ? "yes" : "no");
        writeAttribute("isColorizerTrigger", pT->mIsColorizerTrigger ? "yes" : "no");
        writeAttribute("isFilterTrigger", pT->mFilterTrigger ? "yes" : "no");
        writeAttribute("isSoundTrigger", pT->mSoundTrigger ? "yes" : "no");
        writeAttribute("isColorTrigger", pT->mColorTrigger ? "yes" : "no");
        writeAttribute("isColorTriggerFg", pT->mColorTriggerFg ? "yes" : "no");
        writeAttribute("isColorTriggerBg", pT->mColorTriggerBg ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeScriptElement(pT->mScript);
            writeTextElement("triggerType", QString::number(pT->mTriggerType));
            writeTextElement("conditonLineDelta", QString::number(pT->mConditionLineDelta));
            writeTextElement("mStayOpen", QString::number(pT->mStayOpen));
            writeTextElement("mCommand", pT->mCommand);
            writeTextElement("packageName", pT->mPackageName);
            writeTextElement("mFgColor", pT->mFgColor.name());
            writeTextElement("mBgColor", pT->mBgColor.name());
            writeTextElement("mSoundFile", pT->mSoundFile);
            writeTextElement("colorTriggerFgColor", pT->mColorTriggerFgColor.name());
            writeTextElement("colorTriggerBgColor", pT->mColorTriggerBgColor.name());

            // TODO: The next bit could be revised for a new - not BACKWARD COMPATIBLE form:
            // int elementCount = qMin( pTt->mRegexCodeList.size(), pT->mRegexCodePropertyList.size() ):
            // writeStartElement( "RegexList" );
            // writeAttribute( "size", QString::number( elementCount ) );
            // for( int i = 0; i < elementCount; ++i ) {
            //     writeEmptyElement( "RegexCode" );
            //     writeAttribute( "name", pT->mRegexCodeList.at(i) );
            //     writeAttribute( "type", pT->mRegexCodePropertyList.at(i) );
            // }
            // writeEndElement(); // </RegexList>

            writeStartElement("regexCodeList");
            for (int i = 0; i < pT->mRegexCodeList.size(); ++i) {
                writeTextElement("string", pT->mRegexCodeList.at(i));
            }
            writeEndElement(); // </regexCodeList>

            writeStartElement("regexCodePropertyList");
            for (int i : pT->mRegexCodePropertyList) {
                writeTextElement("integer", QString::number(i));
            }
            writeEndElement(); // </regexCodePropertyList>
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeTrigger(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a "&& (! pT->mModuleMasterFolder)"
        writeEndElement(); // </TriggerGroup> or </Trigger>
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
    bool isOk = writeAlias(mpAlias);
    writeEndElement(); // </AliasPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeAlias(TAlias* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "AliasGroup" : "Alias");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeScriptElement(pT->mScript);
            writeTextElement("command", pT->mCommand);
            writeTextElement("packageName", pT->mPackageName);
            writeTextElement("regex", pT->mRegexCode);
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeAlias(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a (! pT->mModuleMasterFolder)
        writeEndElement(); // </AliasGroup> or </Alias>
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
    bool isOk = writeAction(mpAction);
    writeEndElement(); // </ActionPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeAction(TAction* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "ActionGroup" : "Action");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");
        writeAttribute("isPushButton", pT->mIsPushDownButton ? "yes" : "no");
        writeAttribute("isFlatButton", pT->mButtonFlat ? "yes" : "no");
        writeAttribute("useCustomLayout", pT->mUseCustomLayout ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeTextElement("packageName", pT->mPackageName);
            writeScriptElement(pT->mScript);
            writeTextElement("css", pT->css);
            writeTextElement("commandButtonUp", pT->mCommandButtonUp);
            writeTextElement("commandButtonDown", pT->mCommandButtonDown);
            writeTextElement("icon", pT->mIcon);
            writeTextElement("orientation", QString::number(pT->mOrientation));
            writeTextElement("location", QString::number(pT->mLocation));
            writeTextElement("posX", QString::number(pT->mPosX));
            writeTextElement("posY", QString::number(pT->mPosY));
            // We now use a boolean but file must use original "1" (false)
            // or "2" (true) for backward compatibility
            writeTextElement("mButtonState", QString::number(pT->mButtonState ? 2 : 1));
            writeTextElement("sizeX", QString::number(pT->mSizeX));
            writeTextElement("sizeY", QString::number(pT->mSizeY));
            writeTextElement("buttonColumn", QString::number(pT->mButtonColumns));
            writeTextElement("buttonRotation", QString::number(pT->mButtonRotation));
            writeTextElement("buttonColor", pT->mButtonColor.name());
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeAction(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a "&& (! pT->mModuleMasterFolder)"
        writeEndElement(); // </ActionGroup> or </Action>
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
    bool isOk = writeTimer(mpTimer);
    writeEndElement(); // </TimerPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeTimer(TTimer* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "TimerGroup" : "Timer");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");
        writeAttribute("isTempTimer", pT->mIsTempTimer ? "yes" : "no");
        writeAttribute("isOffsetTimer", pT->isOffsetTimer() ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeScriptElement(pT->mScript);
            writeTextElement("command", pT->mCommand);
            writeTextElement("packageName", pT->mPackageName);
            writeTextElement("time", pT->mTime.toString("hh:mm:ss.zzz"));
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeTimer(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a "&& (! pT->mModuleMasterFolder)"
        writeEndElement(); // </TimerGroup> or </Timer>
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
    bool isOk = writeScript(mpScript);
    writeEndElement(); // </ScriptPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeScript(TScript* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "ScriptGroup" : "Script");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeTextElement("packageName", pT->mPackageName);
            writeScriptElement(pT->mScript);

            writeStartElement("eventHandlerList");
            for (int i = 0; i < pT->mEventHandlerList.size(); ++i) {
                writeTextElement("string", pT->mEventHandlerList.at(i));
            }
            writeEndElement(); // </eventHandlerList>
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); it++) {
        if (!writeScript(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a "&& (! pT->mModuleMasterFolder)"
        writeEndElement(); // </ScriptGroup> or </Script>
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
    bool isOk = writeKey(mpKey);
    writeEndElement(); // </KeyPackage>

    writeEndElement(); // </MudletPackage>
    writeEndDocument();

    return (isOk && (!hasError()));
}

bool XMLexport::writeKey(TKey* pT)
{
    bool isOk = true;
    if (!pT->mModuleMasterFolder && pT->exportItem) {
        writeStartElement(pT->mIsFolder ? "KeyGroup" : "Key");

        writeAttribute("isActive", pT->shouldBeActive() ? "yes" : "no");
        writeAttribute("isFolder", pT->mIsFolder ? "yes" : "no");

        { // Blocked so that indentation reflects that of the XML file
            writeTextElement("name", pT->mName);
            writeTextElement("packageName", pT->mPackageName);
            writeScriptElement(pT->mScript);
            writeTextElement("command", pT->mCommand);
            writeTextElement("keyCode", QString::number(pT->mKeyCode));
            writeTextElement("keyModifier", QString::number(pT->mKeyModifier));
        }

        isOk = !hasError();
    }

    for (auto it = pT->mpMyChildrenList->begin(); isOk && it != pT->mpMyChildrenList->end(); ++it) {
        if (!writeKey(*it)) {
            isOk = false;
        }
    }

    if (pT->exportItem) {  // CHECK: doesn't it also need a "&& (! pT->mModuleMasterFolder)"
        writeEndElement(); // </KeyGroup> or </Key>
    }

    return (isOk && (!hasError()));
}

bool XMLexport::writeScriptElement(const QString & script)
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
    writeTextElement(QLatin1String("script"), localScript);
}
