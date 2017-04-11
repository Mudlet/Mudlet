/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TTimer.h"
#include "TVar.h"
#include "VarUnit.h"


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

bool XMLexport::writeModuleXML( QIODevice * device, QString moduleName){
    setDevice(device);
    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "TriggerPackage" );
    Host * pT = mpHost;
    bool ret = true;
    int nodesWritten = 0;
    //we go a level down for all these functions so as to not infinitely nest the module
    typedef list<TTrigger *>::const_iterator ItTriggerUnit;
    for( ItTriggerUnit it1 = pT->mTriggerUnit.mTriggerRootNodeList.begin(); it1 != pT->mTriggerUnit.mTriggerRootNodeList.end(); it1++ ) {
        TTrigger * pChildTrigger = *it1;
        if( ! pChildTrigger || pChildTrigger->mPackageName != moduleName ) continue;
        if( ! pChildTrigger->isTempTrigger() && pChildTrigger->mModuleMember ) {
            bool result = writeTrigger( pChildTrigger );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;

    writeStartElement( "TimerPackage" );
    typedef list<TTimer *>::const_iterator ItTimerUnit;
    for( ItTimerUnit it2 = pT->mTimerUnit.mTimerRootNodeList.begin(); it2 != pT->mTimerUnit.mTimerRootNodeList.end(); it2++ ) {
        TTimer * pChildTimer = *it2;
        if( ! pChildTimer || pChildTimer->mPackageName != moduleName ) continue;
        if( ! pChildTimer->isTempTimer() && pChildTimer->mModuleMember ) {
            bool result = writeTimer( pChildTimer );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;

    writeStartElement( "AliasPackage" );
    typedef list<TAlias *>::const_iterator ItAliasUnit;
    for( ItAliasUnit it3 = pT->mAliasUnit.mAliasRootNodeList.begin(); it3 != pT->mAliasUnit.mAliasRootNodeList.end(); it3++ ) {
        TAlias * pChildAlias = *it3;
        if( ! pChildAlias || pChildAlias->mPackageName != moduleName ) continue;
        if( ! pChildAlias->isTempAlias() && pChildAlias->mModuleMember ) {
            bool result = writeAlias( pChildAlias );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;

    writeStartElement( "ActionPackage" );
    typedef list<TAction *>::const_iterator ItActionUnit;
    for( ItActionUnit it4 = pT->mActionUnit.mActionRootNodeList.begin(); it4 != pT->mActionUnit.mActionRootNodeList.end(); it4++ ) {
        TAction * pChildAction = *it4;
        if( ! pChildAction || pChildAction->mPackageName != moduleName ) continue;
        if( pChildAction->mModuleMember ) {
            bool result = writeAction( pChildAction );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;

    writeStartElement( "ScriptPackage" );
    typedef list<TScript *>::const_iterator ItScriptUnit;
    for( ItScriptUnit it5 = pT->mScriptUnit.mScriptRootNodeList.begin(); it5 != pT->mScriptUnit.mScriptRootNodeList.end(); it5++ ) {
        TScript * pChildScript = *it5;
        if( ! pChildScript || pChildScript->mPackageName != moduleName ) continue;
        if( pChildScript->mModuleMember ) {
            bool result = writeScript( pChildScript );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;

    writeStartElement( "KeyPackage" );
    typedef list<TKey *>::const_iterator ItKeyUnit;
    for( ItKeyUnit it6 = pT->mKeyUnit.mKeyRootNodeList.begin(); it6 != pT->mKeyUnit.mKeyRootNodeList.end(); it6++ ) {
        TKey * pChildKey = *it6;
        if( ! pChildKey || pChildKey->mPackageName != moduleName ) continue;
        if( pChildKey->mModuleMember ) {
            bool result = writeKey( pChildKey );
            if( ! result )
                ret = false;
            nodesWritten+=1;
        }
    }
    if( ! nodesWritten )
        writeEndElement(); //end trigger package tag
    nodesWritten=0;
    writeStartElement( "HelpPackage" );
    if( mpHost->moduleHelp.contains(moduleName) && mpHost->moduleHelp[moduleName].contains("helpURL") )
        writeTextElement( "helpURL", mpHost->moduleHelp[moduleName]["helpURL"]);
    else
        writeTextElement( "helpURL", "");
    writeEndElement(); //end trigger package tag

    writeEndElement();//end hostpackage
    writeEndElement();//MudletPackage
    writeEndDocument();
    return ret;
}

bool XMLexport::exportHost( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "HostPackage" );
    writeHost( mpHost );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeHost( Host * pHost )
{
    writeStartElement( "Host" );

    writeAttribute( "autoClearCommandLineAfterSend", pHost->mAutoClearCommandLineAfterSend ? "yes" : "no" );
    writeAttribute( "disableAutoCompletion", pHost->mDisableAutoCompletion ? "yes" : "no" );
    writeAttribute( "printCommand", pHost->mPrintCommand ? "yes" : "no" );
    writeAttribute( "USE_IRE_DRIVER_BUGFIX", pHost->mUSE_IRE_DRIVER_BUGFIX ? "yes" : "no" );
    writeAttribute( "mUSE_FORCE_LF_AFTER_PROMPT", pHost->mUSE_FORCE_LF_AFTER_PROMPT ? "yes" : "no" );
    writeAttribute( "mUSE_UNIX_EOL", pHost->mUSE_UNIX_EOL ? "yes" : "no" );
    writeAttribute( "mNoAntiAlias", pHost->mNoAntiAlias ? "yes" : "no" );
    // FIXME: Change to a string or integer property when possible to support more
    // than false (perhaps 0 or "PlainText") or true (perhaps 1 or "HTML") in the
    // future - phpBB code might be useful if it can be done.
    writeAttribute( "mRawStreamDump", pHost->mIsNextLogFileInHtmlFormat ? "yes" : "no" );
    writeAttribute( "mAlertOnNewData", pHost->mAlertOnNewData ? "yes" : "no" );
    writeAttribute( "mFORCE_NO_COMPRESSION", pHost->mFORCE_NO_COMPRESSION ? "yes" : "no" );
    writeAttribute( "mFORCE_GA_OFF", pHost->mFORCE_GA_OFF ? "yes" : "no" );
    writeAttribute( "mFORCE_SAVE_ON_EXIT", pHost->mFORCE_SAVE_ON_EXIT ? "yes" : "no" );
    writeAttribute( "mEnableGMCP", pHost->mEnableGMCP ? "yes" : "no" );
    writeAttribute( "mEnableMSDP", pHost->mEnableMSDP ? "yes" : "no" );
    writeAttribute( "mMapStrongHighlight", pHost->mMapStrongHighlight ? "yes" : "no" );
    writeAttribute( "mLogStatus", pHost->mLogStatus ? "yes" : "no" );
    writeAttribute( "mEnableSpellCheck", pHost->mEnableSpellCheck ? "yes" : "no" );
    writeAttribute( "mShowInfo", pHost->mShowInfo ? "yes" : "no" );
    writeAttribute( "mAcceptServerGUI", pHost->mAcceptServerGUI ? "yes" : "no" );
    writeAttribute( "mMapperUseAntiAlias", pHost->mMapperUseAntiAlias ? "yes" : "no" );
    writeAttribute( "mFORCE_MXP_NEGOTIATION_OFF", pHost->mFORCE_MXP_NEGOTIATION_OFF ? "yes" : "no" );
    writeAttribute( "mRoomSize", QString::number(pHost->mRoomSize));
    writeAttribute( "mLineSize", QString::number(pHost->mLineSize));
    writeAttribute( "mBubbleMode", pHost->mBubbleMode ? "yes" : "no");
    writeAttribute( "mShowRoomIDs", pHost->mShowRoomID ? "yes" : "no");
    writeAttribute( "mShowPanel", pHost->mShowPanel ? "yes" : "no");
    writeAttribute( "mHaveMapperScript", pHost->mHaveMapperScript ? "yes" : "no");
    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while( it.hasNext() )
    {
        ignore = ignore.append(it.next());
    }
    writeAttribute( "mDoubleClickIgnore", ignore);

    writeTextElement( "name", pHost->mHostName );
    //writeTextElement( "login", pHost->mLogin );
    //writeTextElement( "pass", pHost->mPass );
    writeStartElement( "mInstalledPackages" );
    for( int i=0; i<pHost->mInstalledPackages.size(); i++ )
    {
        writeTextElement( "string", pHost->mInstalledPackages[i] );
    }
    writeEndElement();
    if (pHost->mInstalledModules.size()){
        writeStartElement( "mInstalledModules" );
        QMapIterator<QString, QStringList> it(pHost->mInstalledModules);
        pHost->modulesToWrite.clear();
        while( it.hasNext() )
        {
            it.next();
            writeTextElement("key", it.key());
            QStringList entry = it.value();
            writeTextElement("filepath", entry[0]);
            writeTextElement("globalSave", entry[1]);
            if (entry[1].toInt()){
                pHost->modulesToWrite[it.key()] = entry;
            }
            writeTextElement("priority", QString::number(pHost->mModulePriorities[it.key()]));
        }
        writeEndElement();
    }
    writeTextElement( "url", pHost->mUrl );
    writeTextElement( "serverPackageName", pHost->mServerGUI_Package_name );
    writeTextElement( "serverPackageVersion", QString::number(pHost->mServerGUI_Package_version ) );
    writeTextElement( "port", QString::number(pHost->mPort) );
    writeTextElement( "borderTopHeight", QString::number(pHost->mBorderTopHeight) );
    writeTextElement( "borderBottomHeight", QString::number(pHost->mBorderBottomHeight) );
    writeTextElement( "borderLeftWidth", QString::number(pHost->mBorderLeftWidth) );
    writeTextElement( "borderRightWidth", QString::number(pHost->mBorderRightWidth) );
    writeTextElement( "wrapAt", QString::number(pHost->mWrapAt) );
    writeTextElement( "wrapIndentCount", QString::number(pHost->mWrapIndentCount) );
    writeTextElement( "mFgColor", pHost->mFgColor.name() );
    writeTextElement( "mBgColor", pHost->mBgColor.name() );
    writeTextElement( "mCommandFgColor", pHost->mCommandFgColor.name() );
    writeTextElement( "mCommandBgColor", pHost->mCommandBgColor.name() );
    writeTextElement( "mCommandLineFgColor", pHost->mCommandLineFgColor.name() );
    writeTextElement( "mCommandLineBgColor", pHost->mCommandLineBgColor.name() );
    writeTextElement( "mBlack", pHost->mBlack.name() );
    writeTextElement( "mLightBlack", pHost->mLightBlack.name() );
    writeTextElement( "mRed", pHost->mRed.name() );
    writeTextElement( "mLightRed", pHost->mLightRed.name() );
    writeTextElement( "mBlue", pHost->mBlue.name() );
    writeTextElement( "mLightBlue", pHost->mLightBlue.name() );
    writeTextElement( "mGreen", pHost->mGreen.name() );
    writeTextElement( "mLightGreen", pHost->mLightGreen.name() );
    writeTextElement( "mYellow", pHost->mYellow.name() );
    writeTextElement( "mLightYellow", pHost->mLightYellow.name() );
    writeTextElement( "mCyan", pHost->mCyan.name() );
    writeTextElement( "mLightCyan", pHost->mLightCyan.name() );
    writeTextElement( "mMagenta", pHost->mMagenta.name() );
    writeTextElement( "mLightMagenta", pHost->mLightMagenta.name() );
    writeTextElement( "mWhite", pHost->mWhite.name() );
    writeTextElement( "mLightWhite", pHost->mLightWhite.name() );
    writeTextElement( "mDisplayFont", pHost->mDisplayFont.toString() );
    writeTextElement( "mCommandLineFont", pHost->mCommandLineFont.toString() );
    // There was a mis-spelt duplicate commandSeperator above but it is now gone
    writeTextElement( "mCommandSeparator", pHost->mCommandSeparator );
    writeTextElement( "commandLineMinimumHeight", QString::number(pHost->commandLineMinimumHeight) );

    writeTextElement( "mFgColor2", pHost->mFgColor_2.name() );
    writeTextElement( "mBgColor2", pHost->mBgColor_2.name() );
    writeTextElement( "mBlack2", pHost->mBlack_2.name() );
    writeTextElement( "mLightBlack2", pHost->mLightBlack_2.name() );
    writeTextElement( "mRed2", pHost->mRed_2.name() );
    writeTextElement( "mLightRed2", pHost->mLightRed_2.name() );
    writeTextElement( "mBlue2", pHost->mBlue_2.name() );
    writeTextElement( "mLightBlue2", pHost->mLightBlue_2.name() );
    writeTextElement( "mGreen2", pHost->mGreen_2.name() );
    writeTextElement( "mLightGreen2", pHost->mLightGreen_2.name() );
    writeTextElement( "mYellow2", pHost->mYellow_2.name() );
    writeTextElement( "mLightYellow2", pHost->mLightYellow_2.name() );
    writeTextElement( "mCyan2", pHost->mCyan_2.name() );
    writeTextElement( "mLightCyan2", pHost->mLightCyan_2.name() );
    writeTextElement( "mMagenta2", pHost->mMagenta_2.name() );
    writeTextElement( "mLightMagenta2", pHost->mLightMagenta_2.name() );
    writeTextElement( "mWhite2", pHost->mWhite_2.name() );
    writeTextElement( "mLightWhite2", pHost->mLightWhite_2.name() );
    writeTextElement( "mSpellDic", pHost->mSpellDic );
    // TODO: Consider removing these sub-elements that duplicate the same
    // attributes - which WERE bugged - when we update the XML format, must leave
    // them in place for now even though we no longer use them for compatibility
    // with older version of Mudlet
    writeTextElement( "mLineSize", QString::number(pHost->mLineSize, 'f', 1) );
    writeTextElement( "mRoomSize", QString::number(pHost->mRoomSize, 'f', 1) );

    writeEndElement(); // end Host tag
    writeEndElement(); // end HostPackage tag

    writeStartElement( "TriggerPackage" );
    bool ret = true;
    typedef list<TTrigger *>::const_iterator ItTriggerUnit;
    for( ItTriggerUnit it1 = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); it1 != pHost->mTriggerUnit.mTriggerRootNodeList.end(); it1++)
    {
        TTrigger * pChildTrigger = *it1;
        if( ! pChildTrigger || pChildTrigger->mModuleMember) continue;
        if( ! pChildTrigger->isTempTrigger())
        {
            if( ! writeTrigger( pChildTrigger ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement(); //end trigger package tag

    writeStartElement("TimerPackage");
    typedef list<TTimer *>::const_iterator ItTimerUnit;
    for( ItTimerUnit it2 = pHost->mTimerUnit.mTimerRootNodeList.begin(); it2 != pHost->mTimerUnit.mTimerRootNodeList.end(); it2++)
    {
        TTimer * pChildTimer = *it2;
        if (pChildTimer->mModuleMember) continue;
        if( ! pChildTimer->isTempTimer())
        {
            if( ! writeTimer( pChildTimer ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement();

    writeStartElement("AliasPackage");
    typedef list<TAlias *>::const_iterator ItAliasUnit;
    for( ItAliasUnit it3 = pHost->mAliasUnit.mAliasRootNodeList.begin(); it3 != pHost->mAliasUnit.mAliasRootNodeList.end(); it3++)
    {
        TAlias * pChildAlias = *it3;
        if (pChildAlias->mModuleMember) continue;
        if( ! pChildAlias->isTempAlias())
        {
            if( ! writeAlias( pChildAlias ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement();

    writeStartElement("ActionPackage");
    typedef list<TAction *>::const_iterator ItActionUnit;
    for( ItActionUnit it4 = pHost->mActionUnit.mActionRootNodeList.begin(); it4 != pHost->mActionUnit.mActionRootNodeList.end(); it4++)
    {
        TAction * pChildAction = *it4;
        if (pChildAction->mModuleMember) continue;
        if( ! writeAction( pChildAction ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    writeStartElement("ScriptPackage");
    typedef list<TScript *>::const_iterator ItScriptUnit;
    for( ItScriptUnit it5 = pHost->mScriptUnit.mScriptRootNodeList.begin(); it5 != pHost->mScriptUnit.mScriptRootNodeList.end(); it5++)
    {
        TScript * pChildScript = *it5;
        if (pChildScript->mModuleMember) continue;
        if( ! writeScript( pChildScript ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    writeStartElement("KeyPackage");
    typedef list<TKey *>::const_iterator ItKeyUnit;
    for( ItKeyUnit it6 = pHost->mKeyUnit.mKeyRootNodeList.begin(); it6 != pHost->mKeyUnit.mKeyRootNodeList.end(); it6++)
    {
        TKey * pChildKey = *it6;
        if (pChildKey->mModuleMember) continue;
        if( ! writeKey( pChildKey ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    writeStartElement("VariablePackage");
    LuaInterface * lI = pHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    //do hidden variables first
    writeStartElement("HiddenVariables");
    QSetIterator<QString> it8( vu->hiddenByUser );
    while( it8.hasNext() )
    {
        writeTextElement( "name", it8.next() );
    }
    writeEndElement();
    TVar * base = vu->getBase();
    if ( !base )
    {
        lI->getVars( false );
        base = vu->getBase();
    }
    if ( base )
    {
        QListIterator<TVar *> it7( base->getChildren(false) );
        while( it7.hasNext() )
        {
            TVar * var = it7.next();
            if( ! writeVariable( var, lI, vu ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement();

    return ret;
}

bool XMLexport::writeVariable( TVar * var, LuaInterface * lI, VarUnit * vu )
{
    if ( var->getValueType() == LUA_TTABLE )
    {
        if ( vu->isSaved( var ) )
        {
            writeStartElement( "VariableGroup" );
            writeTextElement( "name", var->getName() );
            writeTextElement( "keyType", QString::number( var->getKeyType() ) );
            writeTextElement( "value", lI->getValue( var ) );
            writeTextElement( "valueType", QString::number( var->getValueType() ) );
            QListIterator<TVar *> it( var->getChildren(false) );
            while( it.hasNext() )
            {
                TVar * var = it.next();
                writeVariable( var, lI, vu );
            }
            writeEndElement();
        }
    }
    else
    {
        if ( vu->isSaved( var ) )
        {
            writeStartElement( "Variable" );
            writeTextElement( "name", var->getName() );
            writeTextElement( "keyType", QString::number( var->getKeyType() ) );
            writeTextElement( "value", lI->getValue( var ) );
            writeTextElement( "valueType", QString::number( var->getValueType() ) );
            writeEndElement();
        }
    }
    return true;
}

bool XMLexport::exportGenericPackage( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeGenericPackage( mpHost );

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeGenericPackage( Host * pHost )
{
    writeStartElement( "TriggerPackage" );
    bool ret = true;
    typedef list<TTrigger *>::const_iterator ItTriggerUnit;
    for( ItTriggerUnit it1 = pHost->mTriggerUnit.mTriggerRootNodeList.begin(); it1 != pHost->mTriggerUnit.mTriggerRootNodeList.end(); it1++)
    {
        TTrigger * pChildTrigger = *it1;
        if( ! pChildTrigger ) continue;
        if( ! pChildTrigger->isTempTrigger())
        {
            if( ! writeTrigger( pChildTrigger ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement(); //end trigger package tag

    writeStartElement("TimerPackage");
    typedef list<TTimer *>::const_iterator ItTimerUnit;
    for( ItTimerUnit it2 = pHost->mTimerUnit.mTimerRootNodeList.begin(); it2 != pHost->mTimerUnit.mTimerRootNodeList.end(); it2++)
    {
        TTimer * pChildTimer = *it2;
        if( ! pChildTimer->isTempTimer())
        {
            if( ! writeTimer( pChildTimer ) )
            {
                ret = false;
            }
        }
    }
    writeEndElement();

    writeStartElement("AliasPackage");
    typedef list<TAlias *>::const_iterator ItAliasUnit;
    for( ItAliasUnit it3 = pHost->mAliasUnit.mAliasRootNodeList.begin(); it3 != pHost->mAliasUnit.mAliasRootNodeList.end(); it3++)
    {
        TAlias * pChildAlias = *it3;
        if( ! pChildAlias->isTempAlias())
        {
            if( ! writeAlias( pChildAlias ) )
            {
                ret =false;
            }
        }
    }
    writeEndElement();

    writeStartElement("ActionPackage");
    typedef list<TAction *>::const_iterator ItActionUnit;
    for( ItActionUnit it4 = pHost->mActionUnit.mActionRootNodeList.begin(); it4 != pHost->mActionUnit.mActionRootNodeList.end(); it4++)
    {
        TAction * pChildAction = *it4;
        if( ! writeAction( pChildAction ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    writeStartElement("ScriptPackage");
    typedef list<TScript *>::const_iterator ItScriptUnit;
    for( ItScriptUnit it5 = pHost->mScriptUnit.mScriptRootNodeList.begin(); it5 != pHost->mScriptUnit.mScriptRootNodeList.end(); it5++)
    {
        TScript * pChildScript = *it5;
        if( ! writeScript( pChildScript ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    writeStartElement("KeyPackage");
    typedef list<TKey *>::const_iterator ItKeyUnit;
    for( ItKeyUnit it6 = pHost->mKeyUnit.mKeyRootNodeList.begin(); it6 != pHost->mKeyUnit.mKeyRootNodeList.end(); it6++)
    {
        TKey * pChildKey = *it6;
        if( ! writeKey( pChildKey ) )
        {
            ret = false;
        }
    }
    writeEndElement();

    return ret;
}



bool XMLexport::exportTrigger( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "TriggerPackage" );
    writeTrigger( mpTrigger );
    writeEndElement();//TriggerPackage

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}


bool XMLexport::writeTrigger( TTrigger * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    //qDebug()<<"trigger written"<<pT->mModuleMember;
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "TriggerGroup";
    }
    else
    {
        tag = "Trigger";
    }
    writeStartElement( tag );
    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );
    writeAttribute( "isTempTrigger", pT->mIsTempTrigger ? "yes" : "no" );
    writeAttribute( "isMultiline", pT->mIsMultiline ? "yes" : "no" );
    writeAttribute( "isPerlSlashGOption", pT->mPerlSlashGOption ? "yes" : "no" );
    writeAttribute( "isColorizerTrigger", pT->mIsColorizerTrigger ? "yes" : "no" );
    writeAttribute( "isFilterTrigger", pT->mFilterTrigger ? "yes" : "no" );
    writeAttribute( "isSoundTrigger", pT->mSoundTrigger ? "yes" : "no" );
    writeAttribute( "isColorTrigger", pT->mColorTrigger ? "yes" : "no" );
    writeAttribute( "isColorTriggerFg", pT->mColorTriggerFg ? "yes" : "no" );
    writeAttribute( "isColorTriggerBg", pT->mColorTriggerBg ? "yes" : "no" );


    writeTextElement( "name", pT->mName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "triggerType", QString::number( pT->mTriggerType ) );
    writeTextElement( "conditonLineDelta", QString::number( pT->mConditionLineDelta ) );
    writeTextElement( "mStayOpen", QString::number( pT->mStayOpen ) );
    writeTextElement( "mCommand", pT->mCommand );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "mFgColor", pT->mFgColor.name() );
    writeTextElement( "mBgColor", pT->mBgColor.name() );
    writeTextElement( "mSoundFile", pT->mSoundFile );
    writeTextElement( "colorTriggerFgColor", pT->mColorTriggerFgColor.name() );
    writeTextElement( "colorTriggerBgColor", pT->mColorTriggerBgColor.name() );

// TODO: The next bit could be revised for a new - not BACKWARD COMPATIBLE form
// if/when we get 'version' checking enforced in the 'MudletPackage' element:
//    int elementCount = qMin( pTt->mRegexCodeList.size(), pT->mRegexCodePropertyList.size() ):
//    writeStartElement( "RegexList" );
//    writeAttribute( "size", QString::number( elementCount ) );
//    for( int i = 0; i < elementCount; ++i ) {
//        writeEmptyElement( "RegexCode" );
//        writeAttribute( "index", QString::number( i ) );
//        writeAttribute( "type", QString::number( pT->mRegexCodePropertyList.at(i) ) );
//        writeAttribute( "data", pT->mRegexCodeList.at(i) );
//    }
//    writeEndElement(); // </RegexList>

    writeStartElement( "regexCodeList" );
    for( int i=0; i<pT->mRegexCodeList.size(); i++ )
    {
        writeTextElement( "string", pT->mRegexCodeList[i] );
    }
    writeEndElement();

    writeStartElement( "regexCodePropertyList" );
    for( int i=0; i<pT->mRegexCodePropertyList.size(); i++ )
    {
        writeTextElement( "integer", QString::number( pT->mRegexCodePropertyList[i] ) );
    }
    writeEndElement();
    }
    typedef list<TTrigger *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
      //  if (pChild->mModuleMember) continue;
        writeTrigger( pChild );
    }
    if (pT->exportItem)
        writeEndElement();

    return true;
}


bool XMLexport::exportAlias( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "AliasPackage" );
    writeAlias( mpAlias );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeAlias( TAlias * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "AliasGroup";
    }
    else
    {
        tag = "Alias";
    }

    writeStartElement( tag );

    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );

    writeTextElement( "name", pT->mName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "command", pT->mCommand );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "regex", pT->mRegexCode );
}
    typedef list<TAlias *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TAlias * pChild = *it;
        writeAlias( pChild );
    }
    if (pT->exportItem)
        writeEndElement();

    return true;
}

bool XMLexport::exportAction( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "ActionPackage" );
    writeAction( mpAction );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeAction( TAction * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "ActionGroup";
    }
    else
    {
        tag = "Action";
    }

    writeStartElement( tag );

    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );
    writeAttribute( "isPushButton", pT->mIsPushDownButton ? "yes" : "no" );
    writeAttribute( "isFlatButton", pT->mButtonFlat ? "yes" : "no" );
    writeAttribute( "useCustomLayout", pT->mUseCustomLayout ? "yes" : "no" );

    writeTextElement( "name", pT->mName );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "css", pT->css );
    writeTextElement( "commandButtonUp", pT->mCommandButtonUp );
    writeTextElement( "commandButtonDown", pT->mCommandButtonDown );
    writeTextElement( "icon", pT->mIcon );
    writeTextElement( "orientation", QString::number(pT->mOrientation) );
    writeTextElement( "location", QString::number(pT->mLocation) );
    writeTextElement( "posX", QString::number(pT->mPosX) );
    writeTextElement( "posY", QString::number(pT->mPosY) );
    // We now use a boolean but file must use original "1" (false)
    // or "2" (true) for backward compatibility
    writeTextElement( "mButtonState", QString::number( pT->mButtonState ? 2 : 1 ) );
    writeTextElement( "sizeX", QString::number(pT->mSizeX) );
    writeTextElement( "sizeY", QString::number(pT->mSizeY) );
    writeTextElement( "buttonColumn", QString::number(pT->mButtonColumns) );
    writeTextElement( "buttonRotation", QString::number(pT->mButtonRotation) );
    writeTextElement( "buttonColor", pT->mButtonColor.name() );
}
    typedef list<TAction *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        writeAction( pChild );
    }
    if (pT->exportItem)
        writeEndElement();

    return true;
}

bool XMLexport::exportTimer( QIODevice * device )
{

    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "TimerPackage" );
    writeTimer( mpTimer );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeTimer( TTimer * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "TimerGroup";
    }
    else
    {
        tag = "Timer";
    }

    writeStartElement( tag );

    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );
    writeAttribute( "isTempTimer", pT->mIsTempTimer ? "yes" : "no" );
    writeAttribute( "isOffsetTimer", pT->isOffsetTimer() ? "yes" : "no" );

    writeTextElement( "name", pT->mName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "command", pT->mCommand );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "time", pT->mTime.toString( "hh:mm:ss.zzz" ) );
}
    typedef list<TTimer *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TTimer * pChild = *it;
        writeTimer( pChild );
    }
    if (pT->exportItem)
        writeEndElement();

    return true;
}


bool XMLexport::exportScript( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "ScriptPackage" );
    writeScript( mpScript );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeScript( TScript * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "ScriptGroup";
    }
    else
    {
        tag = "Script";
    }

    writeStartElement( tag );

    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );

    writeTextElement( "name", pT->mName );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "script", pT->mScript );

    writeStartElement( "eventHandlerList" );
    for( int i=0; i<pT->mEventHandlerList.size(); i++ )
    {
        writeTextElement( "string", pT->mEventHandlerList[i] );
    }
    writeEndElement();
}
    typedef list<TScript *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TScript * pChild = *it;
        writeScript( pChild );
    }
    if (pT->exportItem)
        writeEndElement();

    return true;
}


bool XMLexport::exportKey( QIODevice * device )
{
    setDevice(device);

    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute(QStringLiteral("version"), mudlet::self()->scmMudletXmlDefaultVersion);

    writeStartElement( "KeyPackage" );
    writeKey( mpKey );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeKey( TKey * pT )
{
    if (!pT->mModuleMasterFolder && pT->exportItem){
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "KeyGroup";
    }
    else
    {
        tag = "Key";
    }

    writeStartElement( tag );

    writeAttribute( "isActive", pT->shouldBeActive() ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );

    writeTextElement( "name", pT->mName );
    writeTextElement( "packageName", pT->mPackageName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "command", pT->mCommand );
    writeTextElement( "keyCode", QString::number( pT->mKeyCode ) );
    writeTextElement( "keyModifier", QString::number( pT->mKeyModifier ) );
}
    typedef list<TKey *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TKey * pChild = *it;
        writeKey( pChild );
    }
    if (pT->exportItem)
        writeEndElement();


    return true;
}
