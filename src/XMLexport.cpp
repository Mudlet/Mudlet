/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn  ( KoehnHeiko@googlemail.com )      *
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

using namespace std;

XMLexport::XMLexport( Host * pH )
: mpHost( pH )
, mType( "Host" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( Host * pT, bool b )
: mpHost( pT )
, mType( "GenericPackage" )
{
    setAutoFormatting(true);
}


XMLexport::XMLexport( TTrigger * pT )
: mpTrigger( pT )
, mType( "Trigger" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TTimer * pT )
: mpTimer( pT )
, mType( "Timer" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TAlias * pT )
: mpAlias( pT )
, mType( "Alias" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TAction * pT )
: mpAction( pT )
, mType( "Action" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TScript * pT )
: mpScript( pT )
, mType( "Script" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TKey * pT )
: mpKey( pT )
, mType( "Key" )
{
    setAutoFormatting(true);
}

bool XMLexport::writeModuleXML( QIODevice * device, QString moduleName){
    setDevice(device);
    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");

    writeStartElement( "MudletPackage" );
    writeAttribute("version", "1.0");

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
    writeAttribute("version", "1.0");

    writeStartElement( "HostPackage" );
    writeHost( mpHost );
    writeEndElement();

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeHost( Host * pT )
{
    writeStartElement( "Host" );

    writeAttribute( "autoClearCommandLineAfterSend", pT->mAutoClearCommandLineAfterSend ? "yes" : "no" );
    writeAttribute( "disableAutoCompletion", pT->mDisableAutoCompletion ? "yes" : "no" );
    writeAttribute( "printCommand", pT->mPrintCommand ? "yes" : "no" );
    writeAttribute( "USE_IRE_DRIVER_BUGFIX", pT->mUSE_IRE_DRIVER_BUGFIX ? "yes" : "no" );
    writeAttribute( "mUSE_FORCE_LF_AFTER_PROMPT", pT->mUSE_FORCE_LF_AFTER_PROMPT ? "yes" : "no" );
    writeAttribute( "mUSE_UNIX_EOL", pT->mUSE_UNIX_EOL ? "yes" : "no" );
    writeAttribute( "mNoAntiAlias", pT->mNoAntiAlias ? "yes" : "no" );
    writeAttribute( "mRawStreamDump", pT->mRawStreamDump ? "yes" : "no" );
    writeAttribute( "mAlertOnNewData", pT->mAlertOnNewData ? "yes" : "no" );
    writeAttribute( "mFORCE_NO_COMPRESSION", pT->mFORCE_NO_COMPRESSION ? "yes" : "no" );
    writeAttribute( "mFORCE_GA_OFF", pT->mFORCE_GA_OFF ? "yes" : "no" );
    writeAttribute( "mFORCE_SAVE_ON_EXIT", pT->mFORCE_SAVE_ON_EXIT ? "yes" : "no" );
    writeAttribute( "mEnableGMCP", pT->mEnableGMCP ? "yes" : "no" );
    writeAttribute( "mMapStrongHighlight", pT->mMapStrongHighlight ? "yes" : "no" );
    writeAttribute( "mLogStatus", pT->mLogStatus ? "yes" : "no" );
    writeAttribute( "mEnableSpellCheck", pT->mEnableSpellCheck ? "yes" : "no" );
    writeAttribute( "mShowInfo", pT->mShowInfo ? "yes" : "no" );
    writeAttribute( "mAcceptServerGUI", pT->mAcceptServerGUI ? "yes" : "no" );
    writeAttribute( "mMapperUseAntiAlias", pT->mMapperUseAntiAlias ? "yes" : "no" );
    writeAttribute( "mFORCE_MXP_NEGOTIATION_OFF", pT->mFORCE_MXP_NEGOTIATION_OFF ? "yes" : "no" );
    writeAttribute( "mRoomSize", QString::number(pT->mRoomSize));
    writeAttribute( "mLineSize", QString::number(pT->mLineSize));
    writeAttribute( "mBubbleMode", pT->mBubbleMode ? "yes" : "no");
    writeAttribute( "mShowRoomIDs", pT->mShowRoomID ? "yes" : "no");
    writeAttribute( "mShowPanel", pT->mShowPanel ? "yes" : "no");
    writeAttribute( "mHaveMapperScript", pT->mHaveMapperScript ? "yes" : "no");
    QString ignore;
    QSetIterator<QChar> it(pT->mDoubleClickIgnore);
    while( it.hasNext() )
    {
        ignore = ignore.append(it.next());
    }
    writeAttribute( "mDoubleClickIgnore", ignore);

    writeTextElement( "name", pT->mHostName );
    //writeTextElement( "login", pT->mLogin );
    //writeTextElement( "pass", pT->mPass );
    writeStartElement( "mInstalledPackages" );
    for( int i=0; i<pT->mInstalledPackages.size(); i++ )
    {
        writeTextElement( "string", pT->mInstalledPackages[i] );
    }
    writeEndElement();
    if (pT->mInstalledModules.size()){
        writeStartElement( "mInstalledModules" );
        QMapIterator<QString, QStringList> it(pT->mInstalledModules);
        pT->modulesToWrite.clear();
        while( it.hasNext() )
        {
            it.next();
            writeTextElement("key", it.key());
            QStringList entry = it.value();
            writeTextElement("filepath", entry[0]);
            writeTextElement("globalSave", entry[1]);
            if (entry[1].toInt()){
                pT->modulesToWrite[it.key()] = entry;
            }
            writeTextElement("priority", QString::number(pT->mModulePriorities[it.key()]));
        }
        writeEndElement();
    }
    writeTextElement( "url", pT->mUrl );
    writeTextElement( "serverPackageName", pT->mServerGUI_Package_name );
    writeTextElement( "serverPackageVersion", QString::number(pT->mServerGUI_Package_version ) );
    writeTextElement( "port", QString::number(pT->mPort) );
    writeTextElement( "borderTopHeight", QString::number(pT->mBorderTopHeight) );
    writeTextElement( "borderBottomHeight", QString::number(pT->mBorderBottomHeight) );
    writeTextElement( "borderLeftWidth", QString::number(pT->mBorderLeftWidth) );
    writeTextElement( "borderRightWidth", QString::number(pT->mBorderRightWidth) );
    writeTextElement( "wrapAt", QString::number(pT->mWrapAt) );
    writeTextElement( "wrapIndentCount", QString::number(pT->mWrapIndentCount) );
    writeTextElement( "commandSeperator", pT->mCommandSeperator );
    writeTextElement( "mFgColor", pT->mFgColor.name() );
    writeTextElement( "mBgColor", pT->mBgColor.name() );
    writeTextElement( "mCommandFgColor", pT->mCommandFgColor.name() );
    writeTextElement( "mCommandBgColor", pT->mCommandBgColor.name() );
    writeTextElement( "mCommandLineFgColor", pT->mCommandLineFgColor.name() );
    writeTextElement( "mCommandLineBgColor", pT->mCommandLineBgColor.name() );
    writeTextElement( "mBlack", pT->mBlack.name() );
    writeTextElement( "mLightBlack", pT->mLightBlack.name() );
    writeTextElement( "mRed", pT->mRed.name() );
    writeTextElement( "mLightRed", pT->mLightRed.name() );
    writeTextElement( "mBlue", pT->mBlue.name() );
    writeTextElement( "mLightBlue", pT->mLightBlue.name() );
    writeTextElement( "mGreen", pT->mGreen.name() );
    writeTextElement( "mLightGreen", pT->mLightGreen.name() );
    writeTextElement( "mYellow", pT->mYellow.name() );
    writeTextElement( "mLightYellow", pT->mLightYellow.name() );
    writeTextElement( "mCyan", pT->mCyan.name() );
    writeTextElement( "mLightCyan", pT->mLightCyan.name() );
    writeTextElement( "mMagenta", pT->mMagenta.name() );
    writeTextElement( "mLightMagenta", pT->mLightMagenta.name() );
    writeTextElement( "mWhite", pT->mWhite.name() );
    writeTextElement( "mLightWhite", pT->mLightWhite.name() );
    writeTextElement( "mDisplayFont", pT->mDisplayFont.toString() );
    writeTextElement( "mCommandLineFont", pT->mCommandLineFont.toString() );
    writeTextElement( "mCommandSeparator", pT->mCommandSeparator );
    writeTextElement( "commandLineMinimumHeight", QString::number(pT->commandLineMinimumHeight) );

    writeTextElement( "mFgColor2", pT->mFgColor_2.name() );
    writeTextElement( "mBgColor2", pT->mBgColor_2.name() );
    writeTextElement( "mBlack2", pT->mBlack_2.name() );
    writeTextElement( "mLightBlack2", pT->mLightBlack_2.name() );
    writeTextElement( "mRed2", pT->mRed_2.name() );
    writeTextElement( "mLightRed2", pT->mLightRed_2.name() );
    writeTextElement( "mBlue2", pT->mBlue_2.name() );
    writeTextElement( "mLightBlue2", pT->mLightBlue_2.name() );
    writeTextElement( "mGreen2", pT->mGreen_2.name() );
    writeTextElement( "mLightGreen2", pT->mLightGreen_2.name() );
    writeTextElement( "mYellow2", pT->mYellow_2.name() );
    writeTextElement( "mLightYellow2", pT->mLightYellow_2.name() );
    writeTextElement( "mCyan2", pT->mCyan_2.name() );
    writeTextElement( "mLightCyan2", pT->mLightCyan_2.name() );
    writeTextElement( "mMagenta2", pT->mMagenta_2.name() );
    writeTextElement( "mLightMagenta2", pT->mLightMagenta_2.name() );
    writeTextElement( "mWhite2", pT->mWhite_2.name() );
    writeTextElement( "mLightWhite2", pT->mLightWhite_2.name() );
    writeTextElement( "mSpellDic", pT->mSpellDic );
    writeTextElement( "mLineSize", QString::number(pT->mLineSize) );
    writeTextElement( "mRoomSize", QString::number(pT->mRoomSize) );
    writeEndElement(); // end Host tag
    writeEndElement(); // end HostPackage tag

    writeStartElement( "TriggerPackage" );
    bool ret = true;
    typedef list<TTrigger *>::const_iterator ItTriggerUnit;
    for( ItTriggerUnit it1 = pT->mTriggerUnit.mTriggerRootNodeList.begin(); it1 != pT->mTriggerUnit.mTriggerRootNodeList.end(); it1++)
    {
        TTrigger * pChildTrigger = *it1;
        if( ! pChildTrigger || pChildTrigger->mModuleMember) continue;
        if( ! pChildTrigger->isTempTrigger())
        {
            ret = writeTrigger( pChildTrigger );
        }
    }
    writeEndElement(); //end trigger package tag

    writeStartElement("TimerPackage");
    typedef list<TTimer *>::const_iterator ItTimerUnit;
    for( ItTimerUnit it2 = pT->mTimerUnit.mTimerRootNodeList.begin(); it2 != pT->mTimerUnit.mTimerRootNodeList.end(); it2++)
    {
        TTimer * pChildTimer = *it2;
        if (pChildTimer->mModuleMember) continue;
        if( ! pChildTimer->isTempTimer())
        {
            ret = writeTimer( pChildTimer );
        }
    }
    writeEndElement();

    writeStartElement("AliasPackage");
    typedef list<TAlias *>::const_iterator ItAliasUnit;
    for( ItAliasUnit it3 = pT->mAliasUnit.mAliasRootNodeList.begin(); it3 != pT->mAliasUnit.mAliasRootNodeList.end(); it3++)
    {
        TAlias * pChildAlias = *it3;
        if (pChildAlias->mModuleMember) continue;
        if( ! pChildAlias->isTempAlias())
        {
            ret = writeAlias( pChildAlias );
        }
    }
    writeEndElement();

    writeStartElement("ActionPackage");
    typedef list<TAction *>::const_iterator ItActionUnit;
    for( ItActionUnit it4 = pT->mActionUnit.mActionRootNodeList.begin(); it4 != pT->mActionUnit.mActionRootNodeList.end(); it4++)
    {
        TAction * pChildAction = *it4;
        if (pChildAction->mModuleMember) continue;
        ret = writeAction( pChildAction );
    }
    writeEndElement();

    writeStartElement("ScriptPackage");
    typedef list<TScript *>::const_iterator ItScriptUnit;
    for( ItScriptUnit it5 = pT->mScriptUnit.mScriptRootNodeList.begin(); it5 != pT->mScriptUnit.mScriptRootNodeList.end(); it5++)
    {
        TScript * pChildScript = *it5;
        if (pChildScript->mModuleMember) continue;
        ret = writeScript( pChildScript );
    }
    writeEndElement();

    writeStartElement("KeyPackage");
    typedef list<TKey *>::const_iterator ItKeyUnit;
    for( ItKeyUnit it6 = pT->mKeyUnit.mKeyRootNodeList.begin(); it6 != pT->mKeyUnit.mKeyRootNodeList.end(); it6++)
    {
        TKey * pChildKey = *it6;
        if (pChildKey->mModuleMember) continue;
        ret = writeKey( pChildKey );
    }
    writeEndElement();

    writeStartElement("VariablePackage");
    LuaInterface * lI = pT->getLuaInterface();
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
        QListIterator<TVar *> it7( base->getChildren(0) );
        while( it7.hasNext() )
        {
            TVar * var = it7.next();
            writeVariable( var, lI, vu );
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
            QListIterator<TVar *> it( var->getChildren(0) );
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
    writeAttribute("version", "1.0");

    writeGenericPackage( mpHost );

    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeGenericPackage( Host * pT )
{
    writeStartElement( "TriggerPackage" );
    bool ret = true;
    typedef list<TTrigger *>::const_iterator ItTriggerUnit;
    for( ItTriggerUnit it1 = pT->mTriggerUnit.mTriggerRootNodeList.begin(); it1 != pT->mTriggerUnit.mTriggerRootNodeList.end(); it1++)
    {
        TTrigger * pChildTrigger = *it1;
        if( ! pChildTrigger ) continue;
        if( ! pChildTrigger->isTempTrigger())
        {
            ret = writeTrigger( pChildTrigger );
        }
    }
    writeEndElement(); //end trigger package tag

    writeStartElement("TimerPackage");
    typedef list<TTimer *>::const_iterator ItTimerUnit;
    for( ItTimerUnit it2 = pT->mTimerUnit.mTimerRootNodeList.begin(); it2 != pT->mTimerUnit.mTimerRootNodeList.end(); it2++)
    {
        TTimer * pChildTimer = *it2;
        if( ! pChildTimer->isTempTimer())
        {
            ret = writeTimer( pChildTimer );
        }
    }
    writeEndElement();

    writeStartElement("AliasPackage");
    typedef list<TAlias *>::const_iterator ItAliasUnit;
    for( ItAliasUnit it3 = pT->mAliasUnit.mAliasRootNodeList.begin(); it3 != pT->mAliasUnit.mAliasRootNodeList.end(); it3++)
    {
        TAlias * pChildAlias = *it3;
        if( ! pChildAlias->isTempAlias())
        {
            ret = writeAlias( pChildAlias );
        }
    }
    writeEndElement();

    writeStartElement("ActionPackage");
    typedef list<TAction *>::const_iterator ItActionUnit;
    for( ItActionUnit it4 = pT->mActionUnit.mActionRootNodeList.begin(); it4 != pT->mActionUnit.mActionRootNodeList.end(); it4++)
    {
        TAction * pChildAction = *it4;
            ret = writeAction( pChildAction );
    }
    writeEndElement();

    writeStartElement("ScriptPackage");
    typedef list<TScript *>::const_iterator ItScriptUnit;
    for( ItScriptUnit it5 = pT->mScriptUnit.mScriptRootNodeList.begin(); it5 != pT->mScriptUnit.mScriptRootNodeList.end(); it5++)
    {
        TScript * pChildScript = *it5;
            ret = writeScript( pChildScript );
    }
    writeEndElement();

    writeStartElement("KeyPackage");
    typedef list<TKey *>::const_iterator ItKeyUnit;
    for( ItKeyUnit it6 = pT->mKeyUnit.mKeyRootNodeList.begin(); it6 != pT->mKeyUnit.mKeyRootNodeList.end(); it6++)
    {
        TKey * pChildKey = *it6;
            ret = writeKey( pChildKey );
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
    writeAttribute("version", "1.0");

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
    writeAttribute("version", "1.0");

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
    writeAttribute("version", "1.0");

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
    writeTextElement( "mButtonState", QString::number(pT->mButtonState) );
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
    writeAttribute("version", "1.0");

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
    writeAttribute("version", "1.0");

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
    writeAttribute("version", "1.0");

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

