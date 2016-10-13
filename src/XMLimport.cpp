/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "XMLimport.h"


#include "LuaInterface.h"
#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TTimer.h"
#include "TVar.h"
#include "VarUnit.h"

#include "pre_guard.h"
#include <QStringList>
#include <QDebug>
#include "post_guard.h"


XMLimport::XMLimport( Host * pH )
: mpHost( pH )
, mPackageName( QString() )
, mpTrigger( Q_NULLPTR )
, mpTimer( Q_NULLPTR )
, mpAlias( Q_NULLPTR )
, mpKey( Q_NULLPTR )
, mpAction( Q_NULLPTR )
, mpScript( Q_NULLPTR )
, mpVar( Q_NULLPTR )
, gotTrigger( false )
, gotTimer( false )
, gotAlias( false )
, gotKey( false )
, gotAction( false )
, gotScript( false )
, module( 0 )
, mMaxRoomId( 0 )
, mMaxAreaId( -1 )
{
}

bool XMLimport::importPackage( QIODevice * device, QString packName, int moduleFlag )
{
    mPackageName = packName;
    setDevice( device );

    module = moduleFlag;

    if( ! packName.isEmpty() )
    {
        mpKey = new TKey( 0, mpHost );
        if (module){
            mpKey->mModuleMasterFolder=true;
            mpKey->mModuleMember=true;
        }
        mpKey->setPackageName( mPackageName );
        mpKey->setIsActive( true );
        mpKey->setName( mPackageName );
        mpKey->setIsFolder( true );

        mpTrigger = new TTrigger( 0, mpHost );
        if (module){
            mpTrigger->mModuleMasterFolder=true;
            mpTrigger->mModuleMember=true;
        }
        mpTrigger->setPackageName( mPackageName );
        mpTrigger->setIsActive( true );
        mpTrigger->setName( mPackageName );
        mpTrigger->setIsFolder( true );

        mpTimer = new TTimer( 0, mpHost );
        mpTimer->setIsFolder( true );

        if (module){
            mpTimer->mModuleMasterFolder=true;
            mpTimer->mModuleMember=true;
        }

        mpTimer->setPackageName( mPackageName );
        mpTimer->setName( mPackageName );
        mpTimer->setIsActive( true );

        mpAlias = new TAlias( 0, mpHost );
        if (module){
            mpAlias->mModuleMasterFolder=true;
            mpAlias->mModuleMember=true;
        }
        mpAlias->setPackageName( mPackageName );
        mpAlias->setName( mPackageName );
        mpAlias->setIsFolder( true );
        QString _nothing = "";
        mpAlias->setScript(_nothing);
        mpAlias->setRegexCode("");
        mpAlias->setIsActive( true );

        mpAction = new TAction( 0, mpHost );
        if (module){
            mpAction->mModuleMasterFolder=true;
            mpAction->mModuleMember=true;
        }
        mpAction->setPackageName( mPackageName );
        mpAction->setIsActive( true );
        mpAction->setName( mPackageName );
        mpAction->setIsFolder( true );
        mpScript = new TScript( 0, mpHost );
        if (module){
            mpScript->mModuleMasterFolder=true;
            mpScript->mModuleMember=true;
        }
        mpScript->setPackageName( mPackageName );
        mpScript->setIsActive( true );
        mpScript->setName( mPackageName );
        mpScript->setIsFolder( true );
        mpHost->getTriggerUnit()->registerTrigger( mpTrigger );
        mpHost->getTimerUnit()->registerTimer( mpTimer );
        mpHost->getAliasUnit()->registerAlias( mpAlias );
        mpHost->getActionUnit()->registerAction( mpAction );
        mpHost->getKeyUnit()->registerKey( mpKey );
        mpHost->getScriptUnit()->registerScript( mpScript );
    }
    while( ! atEnd() )
    {
        readNext();

        if( isStartElement() )
        {
            if( name() == "MudletPackage" )// && attributes().value("version") == "1.0")
            {
                readPackage();
            }
            else if( name() == "map" )
            {
                readMap();
                mpHost->mpMap->audit();
            }
            else
            {
                qDebug()<<"ERROR:name="<<name().toString()<<"text:"<<text().toString();
            }
        }
    }

    if( gotTimer && ! packName.isEmpty() )
    {
        mpTimer->setIsActive( true );
        mpTimer->enableTimer( mpTimer->getID() );
    }

    if( gotAlias && ! packName.isEmpty() )
    {
        mpAlias->setIsActive( true );
    }

    if( gotAction && ! packName.isEmpty() )
    {
        mpHost->getActionUnit()->updateToolbar();
    }

    if( ! packName.isEmpty())
    {
       if( ! gotTrigger )
            mpHost->getTriggerUnit()->unregisterTrigger( mpTrigger );
       if( ! gotTimer )
            mpHost->getTimerUnit()->unregisterTimer( mpTimer );
       if( ! gotAlias )
            mpHost->getAliasUnit()->unregisterAlias( mpAlias );
       if( ! gotAction )
            mpHost->getActionUnit()->unregisterAction( mpAction );
       if( ! gotKey )
            mpHost->getKeyUnit()->unregisterKey( mpKey );
       if( ! gotScript )
            mpHost->getScriptUnit()->unregisterScript( mpScript );
    }
    return ! error();
}

void XMLimport::readVariableGroup( TVar *pParent )
{
    TVar * var;
    if( pParent )
    {
        var = new TVar( pParent );
    }
    else
    {
        var = new TVar( );
    }

    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    QString keyName, value;
    int keyType = 0;
    int valueType;
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                keyName = readElementText();
                continue;
            }

            else if( name() == "value")
            {
                value = readElementText();
                continue;
            }
            else if( name() == "keyType" )
            {
                keyType = readElementText().toInt() ;
                continue;
            }
            else if ( name() == "valueType" )
            {
                valueType = readElementText().toInt();
                var->setName( keyName, keyType );
                var->setValue( value, valueType );
                vu->addSavedVar( var );
                lI->setValue( var );
                continue;
            }
            else if( name() == "VariableGroup" )
            {
                readVariableGroup( var );
            }
            else if( name() == "Variable" )
            {
                readVariableGroup( var );
            }
        }
    }
}

void XMLimport::readHiddenVariables()
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                QString var = readElementText();
                vu->addHidden( var );
                continue;
            }
        }
    }
}

void XMLimport::readVariablePackage()
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    mpVar = vu->getBase();
    while( ! atEnd() )
    {
        readNext();
        if( isStartElement() )
        {
            if( name() == "VariableGroup" )
            {
                readVariableGroup( mpVar );
            }
            else if( name() == "Variable" )
            {
                readVariableGroup( mpVar );
            }
            else if ( name() == "HiddenVariables")
            {
                readHiddenVariables( );
            }
        }
    }
}

void XMLimport::readMap()
{
    QMultiHash<int, int> tempAreaRoomsHash; // Keys: area id, Values: a room id in that area

    while( ! atEnd() )
    {
        readNext();

        if( isStartElement() )
        {

            if( name() == "areas" )
            {
                mpHost->mpMap->mpRoomDB->clearMapDB();
                mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing area data..." ) );
                mpHost->mpMap->reportProgressToProgressDialog( 0, 3 );
                readAreas();
            }
            else if( name() == "rooms" )
            {
                mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing room data..." ) );
                mpHost->mpMap->reportProgressToProgressDialog( 1, 3 );
                readRooms( tempAreaRoomsHash );
            }
            else if( name() == "environments" )
            {
                mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing environment data..." ) );
                mpHost->mpMap->reportProgressToProgressDialog( 2, 3 );
                readEnvColors();
            }
            mpHost->mpMap->reportProgressToProgressDialog( 3, 3 );
        }
    }

    mpHost->mpMap->reportStringToProgressDialog( tr( "Assigning rooms to their areas..." ) );
    int roomTotal = tempAreaRoomsHash.count();
    int currentRoomCount = 0;

    QListIterator<int> itAreaWithRooms( tempAreaRoomsHash.uniqueKeys() );
    while( itAreaWithRooms.hasNext() ) {
        int areaId = itAreaWithRooms.next();
        QSet<int> areaRoomsSet = tempAreaRoomsHash.values( areaId ).toSet();

        if( ! mpHost->mpMap->mpRoomDB->areas.contains( areaId ) ) {
            // It is known for map files to have rooms with area Ids that are not in the
            // listed areas - this cures that:
            mpHost->mpMap->mpRoomDB->addArea( areaId );
        }

        mpHost->mpMap->mpRoomDB->setAreaRooms( areaId, areaRoomsSet );
        currentRoomCount += areaRoomsSet.count();
        mpHost->mpMap->reportProgressToProgressDialog( currentRoomCount, roomTotal );
    }
}

void XMLimport::readEnvColors()
{
    while( ! atEnd() )
    {
        readNext();
        if( name() == "environment" )
        {
            readEnvColor();
        }
        /*else
        {
            readUnknownMapElement();
        }*/
    }
}

void XMLimport::readEnvColor()
{

    int id = attributes().value("id").toString().toInt();
    int color = attributes().value("color").toString().toInt();
    mpHost->mpMap->envColors[id] = color;
}

void XMLimport::readAreas()
{
    while( ! atEnd() )
    {
        readNext();
        if( name() == "areas")
        {
            break;
        }
        if( name() == "area" )
        {
            readArea();
        }

    }
}

void XMLimport::readArea()
{
    int id = attributes().value("id").toString().toInt();
    QString name = attributes().value("name").toString();
    mpHost->mpMap->mpRoomDB->addArea( id, name );
}

void XMLimport::readRooms( QMultiHash<int, int> & areaRoomsHash )
{

    unsigned int roomCount = 0;
    while( ! atEnd() )
    {
        readNext();

        if( Q_LIKELY( isStartElement() ) )
        {
            if( Q_LIKELY( name() == QStringLiteral( "room" ) ) )
            {
                readRoom( areaRoomsHash, &roomCount );
            }

            else
            {
                readUnknownMapElement();
            }
        }
        else if( isEndElement() )
        {
            break;
        }
    }
}

// This is a CPU/Time hog without the non-default (true) third argument to
// TRoomDB::addRoom(...)
void XMLimport::readRoom( QMultiHash<int, int> & areamRoomMultiHash, unsigned int * roomCount )
{
    TRoom * pT = new TRoom( mpHost->mpMap->mpRoomDB );
    pT->id = attributes().value( QStringLiteral( "id" ) ).toString().toInt();
    pT->area = attributes().value( QStringLiteral( "area" ) ).toString().toInt();
    pT->name = attributes().value( QStringLiteral( "title" ) ).toString();
    pT->environment = attributes().value( QStringLiteral( "environment" ) ).toString().toInt();

    while( ! atEnd() )
    {
        readNext();

        if( Q_UNLIKELY( pT->id < 1 ) )
        {
            continue; // Skip further tests on exits as we'd have to throw away
                      // this invalid room and it would mess up the entranceMultiHash
        }
        else if( Q_LIKELY( name() == QStringLiteral( "exit" ) ) )
        {
            QString dir = attributes().value( QStringLiteral( "direction" ) ).toString();
            int e = attributes().value( QStringLiteral( "target" ) ).toString().toInt();
            if( dir.isEmpty() )
            {
                continue;
            }
            else if( dir == QStringLiteral( "north" ) )
            {
                pT->north = e;
            }
            else if( dir == QStringLiteral( "east" ) )
            {
                pT->east = e;
            }
            else if( dir == QStringLiteral( "south" ) )
            {
                pT->south = e;
            }
            else if( dir == QStringLiteral( "west" ) )
            {
                pT->west = e;
            }
            else if( dir == QStringLiteral( "up" ) )
            {
                pT->up = e;
            }
            else if( dir == QStringLiteral( "down" ) )
            {
                pT->down = e;
            }
            else if( dir == QStringLiteral( "northeast" ) )
            {
                pT->northeast = e;
            }
            else if( dir == QStringLiteral( "southwest" ) )
            {
                pT->southwest = e;
            }
            else if( dir == QStringLiteral( "southeast" ) )
            {
                pT->southeast = e;
            }
            else if( dir == QStringLiteral( "northwest" ) )
            {
                pT->northwest = e;
            }
            else if( dir == QStringLiteral( "in" ) )
            {
                pT->in = e;
            }
            else if( dir == QStringLiteral( "out" ) )
            {
                pT->out = e;
            }
            else
            {
                // TODO: Handle Special Exits
            }
            continue;
        }
        else if( name() == QStringLiteral( "coord" ) )
        {
            if( attributes().value("x").toString().isEmpty() )
            {
                continue;
            }

            pT->x = attributes().value( QStringLiteral( "x" ) ).toString().toInt();
            pT->y = attributes().value( QStringLiteral( "y" ) ).toString().toInt();
            pT->z = attributes().value( QStringLiteral( "z" ) ).toString().toInt();
            continue;
        }
        else if( Q_UNLIKELY( name().isEmpty() ) )
        {
            continue;
        }

        if( isEndElement() )
        {
            break;
        }
    }

    if( pT->id > 0 )
    {
        if( ( ++( *roomCount ) % 100 == 0 ) )
        {
            mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing room data [count: %1]..." ).arg( *roomCount ) );
        }
        areamRoomMultiHash.insert( pT->area, pT->id );
        // We are loading a map so can make some optimisation by setting the
        // third argument as true:
        mpHost->mpMap->mpRoomDB->addRoom( pT->id, pT, true );
        mMaxRoomId = qMax( mMaxRoomId, pT->id ); //Wasn't used but now maintains max Room Id
    }
    else
    {
        delete pT;
    }
}


void XMLimport::readUnknownMapElement()
{
    while( ! atEnd() )
    {

        readNext();

        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readMap();
        }
    }
}

// Not used:
//void XMLimport::readUnknownRoomElement()
//{
//    while( ! atEnd() )
//    {

//        readNext();
//        qDebug() << "[ERROR]: UNKNOWN room element: name="
//                 << name().toString();

//        if( isEndElement() )
//        {
//            break;
//        }

//        if( isStartElement() )
//        {
//            readRoom();
//        }
//    }
//}



void XMLimport::readPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "HostPackage" )
            {
                readHostPackage();
                continue;
            }
            if( name() == "TriggerPackage" )
            {
                readTriggerPackage();
                continue;
            }
            else if( name() == "TimerPackage" )
            {
                readTimerPackage();
                continue;
            }
            else if( name() == "AliasPackage" )
            {
                readAliasPackage();
                continue;
            }
            else if( name() == "ActionPackage" )
            {
                readActionPackage();
                continue;
            }
            else if( name() == "ScriptPackage" )
            {
                readScriptPackage();
                continue;
            }
            else if( name() == "KeyPackage" )
            {
                readKeyPackage();
                continue;
            }
            else if(name() == "HelpPackage"){
                readHelpPackage();
                continue;
            }
            else if( name() == "VariablePackage" )
            {
                readVariablePackage();
            }
            else
            {
                readUnknownPackage();
            }
        }
    }
}

void XMLimport::readHelpPackage(){
    while( ! atEnd() )
    {
        readNext();
        if(isEndElement())
        {
            break;
        }
        if( isStartElement() )
        {
            if( name() == "helpURL" )
            {
                QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert("helpURL", contents);
                continue;
            }
        }
    }
}


void XMLimport::readUnknownPackage()
{
    while( ! atEnd() )
    {

        readNext();
        qDebug()<<"[ERROR]: UNKNOWN readUnknownPackage() Package Element:"<<name().toString()<<"text:"<<text().toString();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readPackage();
        }
    }
}

void XMLimport::readUnknownHostElement()
{
    while( ! atEnd() )
    {

        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Host Package Element:name="<<name().toString()<<"text:"<<text().toString();

        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readHostPackage( mpHost );
        }
    }
}


void XMLimport::readUnknownTriggerElement()
{
    while( ! atEnd() )
    {

        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Trigger Package Element:name="<<name().toString()<<"text:"<<text().toString();

        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readTriggerPackage();
        }
    }
}

void XMLimport::readUnknownTimerElement()
{
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readTimerPackage();
        }
    }
}

void XMLimport::readUnknownAliasElement()
{
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readAliasPackage();
        }
    }
}

void XMLimport::readUnknownActionElement()
{
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readActionPackage();
        }
    }
}

void XMLimport::readUnknownScriptElement()
{
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readScriptPackage();
        }
    }
}

void XMLimport::readUnknownKeyElement()
{
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            readKeyPackage();
        }
    }
}



void XMLimport::readTriggerPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "TriggerGroup" )
            {
                gotTrigger = true;
                if( mPackageName.isEmpty() )
                    readTriggerGroup(0);
                else
                    readTriggerGroup(mpTrigger);
            }
            else if( name() == "Trigger" )
            {
                gotTrigger = true;
                if( mPackageName.isEmpty() )
                    readTriggerGroup(0);
                else
                    readTriggerGroup(mpTrigger);
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readHostPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "Host" )
            {
                readHostPackage( mpHost );
            }

            else
            {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readHostPackage( Host * pT )
{
    pT->mAutoClearCommandLineAfterSend = ( attributes().value("autoClearCommandLineAfterSend") == "yes" );
    pT->mDisableAutoCompletion = ( attributes().value("disableAutoCompletion") == "yes" );
    pT->mPrintCommand = ( attributes().value("printCommand") == "yes" );
    pT->set_USE_IRE_DRIVER_BUGFIX( attributes().value("USE_IRE_DRIVER_BUGFIX") == "yes" );
    pT->mUSE_FORCE_LF_AFTER_PROMPT = ( attributes().value("mUSE_FORCE_LF_AFTER_PROMPT") == "yes" );
    pT->mUSE_UNIX_EOL = ( attributes().value("mUSE_UNIX_EOL") == "yes" );
    pT->mNoAntiAlias = ( attributes().value("mNoAntiAlias") == "yes" );
    pT->mIsNextLogFileInHtmlFormat = ( attributes().value("mRawStreamDump") == "yes" );
    pT->mAlertOnNewData = ( attributes().value("mAlertOnNewData") == "yes" );
    pT->mFORCE_NO_COMPRESSION = ( attributes().value("mFORCE_NO_COMPRESSION") == "yes" );
    pT->mFORCE_GA_OFF = ( attributes().value("mFORCE_GA_OFF") == "yes" );
    pT->mFORCE_SAVE_ON_EXIT = ( attributes().value("mFORCE_SAVE_ON_EXIT") == "yes" );
    pT->mEnableGMCP = ( attributes().value("mEnableGMCP") == "yes" );
    pT->mEnableMSDP = ( attributes().value("mEnableMSDP") == "yes" );
    pT->mMapStrongHighlight = ( attributes().value("mMapStrongHighlight") == "yes" );
    pT->mLogStatus = ( attributes().value("mLogStatus") == "yes" );
    pT->mEnableSpellCheck = ( attributes().value("mEnableSpellCheck") == "yes" );
    pT->mShowInfo = ( attributes().value("mShowInfo") == "yes" );
    pT->mAcceptServerGUI = ( attributes().value("mAcceptServerGUI") == "yes" );
    pT->mMapperUseAntiAlias = ( attributes().value("mMapperUseAntiAlias") == "yes" );
    pT->mFORCE_MXP_NEGOTIATION_OFF = ( attributes().value("mFORCE_MXP_NEGOTIATION_OFF") == "yes" );
    pT->mRoomSize = attributes().value("mRoomSize").toString().toInt();
    if (!pT->mRoomSize)
        pT->mRoomSize=3;
    pT->mLineSize = attributes().value("mLineSize").toString().toInt();
    if (!pT->mLineSize)
        pT->mLineSize=1;
    pT->mBubbleMode = ( attributes().value("mBubbleMode") == "yes" );
    pT->mShowRoomID = ( attributes().value("mShowRoomIDs") == "yes" );
    pT->mShowPanel = ( attributes().value("mShowPanel") == "yes" );
    pT->mHaveMapperScript = ( attributes().value("mHaveMapperScript") == "yes");
    QStringRef ignore = attributes().value("mDoubleClickIgnore");
    for(int i=0;i<ignore.size();i++)
        pT->mDoubleClickIgnore.insert( ignore.at( i ) );

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                pT->mHostName = readElementText();
                continue;
            }
            else if( name() == "mInstalledModules")
            {
                QMap<QString, QStringList> entry;
                readMapList(entry);
                QMapIterator<QString, QStringList> it(entry);
                while (it.hasNext()){
                    it.next();
                    QStringList moduleList;
                    QStringList entryList = it.value();
                    moduleList << entryList[0];
                    moduleList << entryList[1];
                    pT->mInstalledModules[it.key()] = moduleList;
                    pT->mModulePriorities[it.key()] = entryList[2].toInt();

                }
                //readMapList( pT->mInstalledModules );
                continue;
            }
            else if( name() == "mInstalledPackages")
            {
                readStringList( pT->mInstalledPackages );
                continue;
            }
            else if( name() =="url" )
            {
                pT->mUrl = readElementText();
                continue;
            }
            else if( name() =="serverPackageName" )
            {
                pT->mServerGUI_Package_name = readElementText();
                continue;
            }
            else if( name() == "serverPackageVersion")
            {
                pT->mServerGUI_Package_version = readElementText().toInt();
                continue;
            }
            else if( name() == "port")
            {
                pT->mPort = readElementText().toInt();
                continue;
            }
            else if( name() == "borderTopHeight")
            {
                pT->mBorderTopHeight = readElementText().toInt();
                continue;
            }
            else if( name() == "commandLineMinimumHeight")
            {
                pT->commandLineMinimumHeight = readElementText().toInt();
                continue;
            }
            else if( name() == "borderBottomHeight")
            {
                pT->mBorderBottomHeight = readElementText().toInt();
                continue;
            }
            else if( name() == "borderLeftWidth")
            {
                pT->mBorderLeftWidth = readElementText().toInt();
                continue;
            }
            else if( name() == "borderRightWidth")
            {
                pT->mBorderRightWidth = readElementText().toInt();
                continue;
            }
            else if( name() == "wrapAt")
            {
                pT->mWrapAt = readElementText().toInt();
                continue;
            }
            else if( name() == "wrapIndentCount" )
            {
                pT->mWrapIndentCount = readElementText().toInt();
                continue;
            }
            else if( name() == "mCommandSeparator" )
            {
                pT->mCommandSeparator = readElementText();
                continue;
            }
            else if( name() == "mCommandLineFgColor")
            {
                pT->mCommandLineFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mCommandLineBgColor")
            {
                pT->mCommandLineBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mFgColor")
            {
                pT->mFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBgColor")
            {
                pT->mBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mCommandFgColor")
            {
                pT->mCommandFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mCommandBgColor")
            {
                pT->mCommandBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBlack")
            {
                pT->mBlack.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightBlack")
            {
                pT->mLightBlack.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mRed")
            {
                pT->mRed.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightRed")
            {
                pT->mLightRed.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBlue")
            {
                pT->mBlue.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightBlue")
            {
                pT->mLightBlue.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mGreen")
            {
                pT->mGreen.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightGreen")
            {
                pT->mLightGreen.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mYellow")
            {
                pT->mYellow.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightYellow")
            {
                pT->mLightYellow.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mCyan")
            {
                pT->mCyan.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightCyan")
            {
                pT->mLightCyan.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mMagenta")
            {
                pT->mMagenta.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightMagenta")
            {
                pT->mLightMagenta.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mWhite")
            {
                pT->mWhite.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightWhite")
            {
                pT->mLightWhite.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mDisplayFont")
            {
                pT->mDisplayFont.fromString( readElementText() );
                pT->mDisplayFont.setFixedPitch( true );
//                pT->mDisplayFont.setWordSpacing( 0 );
//                pT->mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, 0 );
                continue;
            }
            else if( name() == "mCommandLineFont")
            {
                pT->mCommandLineFont.fromString( readElementText() );
                continue;
            }
            else if( name() == "mFgColor2")
            {
                pT->mFgColor_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBgColor2")
            {
                pT->mBgColor_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBlack2")
            {
                pT->mBlack_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightBlack2")
            {
                pT->mLightBlack_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mRed2")
            {
                pT->mRed_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightRed2")
            {
                pT->mLightRed_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBlue2")
            {
                pT->mBlue_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightBlue2")
            {
                pT->mLightBlue_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mGreen2")
            {
                pT->mGreen_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightGreen2")
            {
                pT->mLightGreen_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mYellow2")
            {
                pT->mYellow_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightYellow2")
            {
                pT->mLightYellow_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mCyan2")
            {
                pT->mCyan_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightCyan2")
            {
                pT->mLightCyan_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mMagenta2")
            {
                pT->mMagenta_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightMagenta2")
            {
                pT->mLightMagenta_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mWhite2")
            {
                pT->mWhite_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mLightWhite2")
            {
                pT->mLightWhite_2.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mSpellDic")
            {
                pT->mSpellDic = readElementText();
                continue;
            }
            else if( name() == "mRoomSize" )
            {
                pT->mRoomSize = readElementText().toDouble();
            }
            else if( name() == "mLineSize" )
            {
                pT->mLineSize = readElementText().toDouble();
            }
            else
            {
                readUnknownHostElement();
            }
        }
    }
}


void XMLimport::readTriggerGroup( TTrigger * pParent )
{
    TTrigger * pT;
    if( pParent )
    {
        pT = new TTrigger( pParent, mpHost );
    }
    else
    {
        pT = new TTrigger( 0, mpHost );
    }
    if (module){
        pT->mModuleMember = true;
    }

    mpHost->getTriggerUnit()->registerTrigger( pT );

    pT->setIsActive( (attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsTempTrigger = ( attributes().value("isTempTrigger") == "yes" );
    pT->mIsMultiline = ( attributes().value("isMultiline") == "yes" );
    pT->mPerlSlashGOption = ( attributes().value("isPerlSlashGOption") == "yes" );
    pT->mIsColorizerTrigger = ( attributes().value("isColorizerTrigger") == "yes" );
    pT->mFilterTrigger = ( attributes().value("isFilterTrigger") == "yes" );
    pT->mSoundTrigger = ( attributes().value("isSoundTrigger") == "yes" );
    pT->mColorTrigger = ( attributes().value("isColorTrigger") == "yes" );
    pT->mColorTriggerBg = ( attributes().value("isColorTriggerBg") == "yes" );
    pT->mColorTriggerFg = ( attributes().value("isColorTriggerFg") == "yes" );

    while( ! atEnd() )
    {
        readNext();
        //qDebug()<<"[INFO] element:"<<name().toString()<<" text:"<<text().toString();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {

                pT->setName( readElementText() );
                continue;
            }

            else if( name() == "script")
            {
                pT->mScript = readElementText();
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "triggerType" )
            {
                pT->mTriggerType = readElementText().toInt();
                continue;
            }
            else if( name() == "conditonLineDelta" )
            {
                pT->mConditionLineDelta = readElementText().toInt();
                continue;
            }
            else if( name() == "mStayOpen" )
            {
                pT->mStayOpen = readElementText().toInt();
                continue;
            }
            else if( name() == "mCommand" )
            {
                pT->mCommand = readElementText();
                continue;
            }

            else if( name() == "mFgColor")
            {
                pT->mFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBgColor")
            {
                pT->mBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "colorTriggerFgColor")
            {
                pT->mColorTriggerFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "colorTriggerBgColor")
            {
                pT->mColorTriggerBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mSoundFile" )
            {
                pT->mSoundFile = readElementText();
                continue;
            }
            else if( name() == "regexCodeList")
            {
                readStringList( pT->mRegexCodeList );
                continue;
            }
            else if( name() == "regexCodePropertyList" )
            {
                readIntegerList( pT->mRegexCodePropertyList );
                continue;
            }

            else if( name() == "TriggerGroup" )
            {
                readTriggerGroup( pT );
            }
            else if( name() == "Trigger" )
            {
                readTriggerGroup( pT );
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }

    if( ! pT->setRegexCodeList( pT->mRegexCodeList, pT->mRegexCodePropertyList ) )
    {
        qDebug()<<"IMPORT: ERROR: cant initialize pattern list for trigger "<<pT->getName();
    }
    QString script = pT->mScript;
    if( ! pT->setScript( script ) )
    {
        qDebug()<<"IMPORT: ERROR: trigger script "<< pT->getName()<<" does not compile";
    }
}

void XMLimport::readTimerPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "TimerGroup" )
            {
                gotTimer = true;
                if( mPackageName.isEmpty() )
                    readTimerGroup(0);
                else
                    readTimerGroup(mpTimer);
            }
            else if( name() == "Timer" )
            {
                gotTimer = true;
                if( mPackageName.isEmpty() )
                    readTimerGroup(0);
                else
                    readTimerGroup(mpTimer);
            }
            else
            {
                readUnknownTimerElement();
            }
        }
    }
}

void XMLimport::readTimerGroup( TTimer * pParent )
{
    TTimer * pT;

    QTime _time(10,0,0,0);

    if( pParent )
    {
        pT = new TTimer( pParent, mpHost );
    }
    else
    {
        pT = new TTimer( 0, mpHost );
    }
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsTempTimer = ( attributes().value("isTempTimer") == "yes" );

    mpHost->getTimerUnit()->registerTimer( pT );
    pT->setShouldBeActive( ( attributes().value("isActive") == "yes" ) );

// N/U:     bool isOffsetTimer = ( attributes().value("isOffsetTimer") == "yes" );


    if (module)
        pT->mModuleMember = true;

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {

                pT->setName( readElementText() );
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "time")
            {
                QString timeString = readElementText();
                QTime time = QTime::fromString( timeString, "hh:mm:ss.zzz" );
                pT->setTime( time );
                continue;
            }
            else if( name() == "TimerGroup" )
            {
                readTimerGroup( pT );
            }
            else if( name() == "Timer" )
            {
                readTimerGroup( pT );
            }
            else
            {
                readUnknownTimerElement();
            }
        }
    }



    mudlet::self()->registerTimer( pT, pT->mpTimer );

    //if( ( ! isOffsetTimer ) && ( ! pT->isFolder() ) && ( pT->shouldBeActive() ) )
    if( ! pT->mpParent && pT->shouldBeActive() )
    {
        pT->setIsActive( true );
        pT->enableTimer( pT->getID() );
//        if( pT->canBeUnlocked( 0 ) )
//        {
//            if( pT->activate() )
//            {
//                pT->mpTimer->start();
//            }
//            else
//            {
//                pT->deactivate();
//                pT->mpTimer->stop();
//            }
//        }
    }
    else
    {
//        qDebug()<<"NOT enabling Timer name:"<<pT->getName();
        //pT->disableTimer( pT->getID() );
        //pT->deactivate();
        //pT->mpTimer->stop();
        //pT->setIsActive( pT->shouldBeActive() );
    }

}

void XMLimport::readAliasPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "AliasGroup" )
            {
                gotAlias = true;
                if( mPackageName.isEmpty() )
                    readAliasGroup(0);
                else
                    readAliasGroup(mpAlias);

            }
            else if( name() == "Alias" )
            {
                gotAlias = true;
                if( mPackageName.isEmpty() )
                    readAliasGroup(0);
                else
                    readAliasGroup(mpAlias);
            }
            else
            {
                readUnknownAliasElement();
            }
        }
    }
}

void XMLimport::readAliasGroup( TAlias * pParent )
{
    TAlias * pT;
    if( pParent )
    {
        pT = new TAlias( pParent, mpHost );
    }
    else
    {
        pT = new TAlias( 0, mpHost );
    }

    mpHost->getAliasUnit()->registerAlias( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    if (module)
        pT->mModuleMember = true;

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                pT->setName( readElementText() );
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "regex")
            {
                pT->setRegexCode( readElementText() );
                continue;
            }
            else if( name() == "AliasGroup" )
            {
                readAliasGroup( pT );
            }
            else if( name() == "Alias" )
            {
                readAliasGroup( pT );
            }
            else
            {
                readUnknownAliasElement();
            }
        }
    }


}

void XMLimport::readActionPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        if( isStartElement() )
        {
            if( name() == "ActionGroup" )
            {
                gotAction = true;
                if( mPackageName.isEmpty() )
                    readActionGroup(0);
                else
                    readActionGroup(mpAction);
            }
            else if( name() == "Action" )
            {
                gotAction = true;
                if( mPackageName.isEmpty() )
                    readActionGroup(0);
                else
                    readActionGroup(mpAction);
            }
            else
            {
                readUnknownActionElement();
            }
        }
    }
}

void XMLimport::readActionGroup( TAction * pParent )
{
    TAction * pT;
    if( pParent )
    {
        pT = new TAction( pParent, mpHost );
    }
    else
    {
        pT = new TAction( 0, mpHost );
    }
    mpHost->getActionUnit()->registerAction( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsPushDownButton = ( attributes().value("isPushButton") == "yes" );
    pT->mButtonFlat = ( attributes().value("isFlatButton") == "yes" );
    pT->mUseCustomLayout = ( attributes().value("useCustomLayout") == "yes" );
    if (module)
        pT->mModuleMember = true;

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "css")
            {
                pT->css = readElementText();
                continue;
            }
            else if( name() == "commandButtonUp")
            {
                pT->mCommandButtonUp = readElementText();
                continue;
            }
            else if( name() == "commandButtonDown")
            {
                pT->mCommandButtonDown = readElementText();
                continue;
            }
            else if( name() == "icon")
            {
                pT->mIcon = readElementText();
                continue;
            }
            else if( name() == "orientation")
            {
                pT->mOrientation = readElementText().toInt();
                continue;
            }
            else if( name() == "location")
            {
                pT->mLocation = readElementText().toInt();
                continue;
            }

            else if( name() == "buttonRotation")
            {
                pT->mButtonRotation = readElementText().toInt();
                continue;
            }
            else if( name() == "sizeX")
            {
                pT->mSizeX = readElementText().toInt();
                continue;
            }
            else if( name() == "sizeY")
            {
                pT->mSizeY = readElementText().toInt();
                continue;
            }
            else if( name() == "mButtonState")
            {
                pT->mButtonState = readElementText().toInt();
                continue;
            }
            else if( name() == "buttonColor")
            {
                pT->mButtonColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "buttonColumn")
            {
                pT->mButtonColumns = readElementText().toInt();
                continue;
            }

            else if( name() == "posX")
            {
                pT->mPosX = readElementText().toInt();
                continue;
            }
            else if( name() == "posY")
            {
                pT->mPosY = readElementText().toInt();
                continue;
            }

            else if( name() == "ActionGroup" )
            {
                readActionGroup( pT );
            }
            else if( name() == "Action" )
            {
                readActionGroup( pT );
            }
            else
            {
                readUnknownActionElement();
            }
        }
    }


}

void XMLimport::readScriptPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "ScriptGroup" )
            {
                gotScript = true;
                if( mPackageName.isEmpty() )
                    readScriptGroup(0);
                else
                    readScriptGroup(mpScript);
            }
            else if( name() == "Script" )
            {
                gotScript = true;
                if( mPackageName.isEmpty() )
                    readScriptGroup(0);
                else
                    readScriptGroup(mpScript);
            }
            else
            {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readScriptGroup( TScript * pParent )
{
    TScript * pT;
    if( pParent )
    {
        pT = new TScript( pParent, mpHost );
    }
    else
    {
        pT = new TScript( 0, mpHost );
    }
    mpHost->getScriptUnit()->registerScript( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    if (module)
        pT->mModuleMember = true;

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "eventHandlerList")
            {
                readStringList( pT->mEventHandlerList );
                pT->setEventHandlerList( pT->mEventHandlerList );
                continue;
            }
            else if( name() == "ScriptGroup" )
            {
                readScriptGroup( pT );
            }
            else if( name() == "Script" )
            {
                readScriptGroup( pT );
            }
            else
            {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readKeyPackage()
{
    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }

        if( isStartElement() )
        {
            if( name() == "KeyGroup" )
            {
                gotKey = true;
                if( mPackageName.isEmpty() )
                    readKeyGroup(0);
                else
                    readKeyGroup(mpKey);

            }
            else if( name() == "Key" )
            {
                gotKey = true;
                if( mPackageName.isEmpty() )
                    readKeyGroup(0);
                else
                    readKeyGroup(mpKey);
            }
            else
            {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readKeyGroup( TKey * pParent )
{
    TKey * pT;
    if( pParent )
    {
        pT = new TKey( pParent, mpHost );
    }
    else
    {
        pT = new TKey( 0, mpHost );
    }
    mpHost->getKeyUnit()->registerKey( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    if (module)
        pT->mModuleMember = true;

    while( ! atEnd() )
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                continue;
            }
            else if( name() == "packageName")
            {
                pT->mPackageName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "keyCode" )
            {
                pT->mKeyCode = readElementText().toInt();
                continue;
            }
            else if( name() == "keyModifier" )
            {
                pT->mKeyModifier = readElementText().toInt();
                continue;
            }

            else if( name() == "KeyGroup" )
            {
                readKeyGroup( pT );
            }
            else if( name() == "Key" )
            {
                readKeyGroup( pT );
            }
            else
            {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readMapList( QMap<QString, QStringList> & map)
{
    QString key;
    QStringList entry;
    while( ! atEnd() )
    {
        readNext();

        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "key")
            {
                key = readElementText();
            }
            else if (name() == "filepath"){
                entry << readElementText();
                //map[key] = readElementText();
            }
            else if (name() == "globalSave"){
                entry << readElementText();
            }
            else if (name() == "priority"){
                entry << readElementText();
                map[key] = entry;
                entry.clear();

            }
            else
            {
                readUnknownHostElement();
            }
        }
    }
}


void XMLimport::readStringList( QStringList & list )
{
    while( ! atEnd() )
    {
        readNext();

        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "string")
            {
                list << readElementText();
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readIntegerList( QList<int> & list )
{
    while( ! atEnd() )
    {
        readNext();

        if( isEndElement() ) break;

        if( isStartElement() )
        {
            if( name() == "integer")
            {
                bool ok = false;
                int num;
                num = readElementText().toInt( &ok, 10 );
                if( ok )
                {
                    list << num;
                }
                else
                {
                    qFatal("FATAL ERROR: reading package property list contained invalid elements");
                }
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}
