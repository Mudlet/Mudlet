/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2017 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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

    if( ! packName.isEmpty() ) {

        mpKey = new TKey( 0, mpHost );
        if( module ) {
            mpKey->mModuleMasterFolder=true;
            mpKey->mModuleMember=true;
        }
        mpKey->setPackageName( mPackageName );
        mpKey->setIsActive( true );
        mpKey->setName( mPackageName );
        mpKey->setIsFolder( true );

        mpTrigger = new TTrigger( 0, mpHost );
        if( module ) {
            mpTrigger->mModuleMasterFolder=true;
            mpTrigger->mModuleMember=true;
        }
        mpTrigger->setPackageName( mPackageName );
        mpTrigger->setIsActive( true );
        mpTrigger->setName( mPackageName );
        mpTrigger->setIsFolder( true );

        mpTimer = new TTimer( 0, mpHost );
        if( module ) {
            mpTimer->mModuleMasterFolder=true;
            mpTimer->mModuleMember=true;
        }
        mpTimer->setPackageName( mPackageName );
        mpTimer->setIsActive( true );
        mpTimer->setName( mPackageName );
        mpTimer->setIsFolder( true );


        mpAlias = new TAlias( 0, mpHost );
        if( module ) {
            mpAlias->mModuleMasterFolder=true;
            mpAlias->mModuleMember=true;
        }
        mpAlias->setPackageName( mPackageName );
        mpAlias->setIsActive( true );
        mpAlias->setName( mPackageName );
        mpAlias->setScript( QString() );
        mpAlias->setRegexCode( QString() );
        mpAlias->setIsFolder( true );

        mpAction = new TAction( 0, mpHost );
        if( module ) {
            mpAction->mModuleMasterFolder=true;
            mpAction->mModuleMember=true;
        }
        mpAction->setPackageName( mPackageName );
        mpAction->setIsActive( true );
        mpAction->setName( mPackageName );
        mpAction->setIsFolder( true );

        mpScript = new TScript( 0, mpHost );
        if( module ) {
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

    while( ! atEnd() ) {
        readNext();

        if( isStartElement() ) {
            if( name() == "MudletPackage" ) {
                readPackage();
            }
            else if( name() == "map" ) {
                readMap();
                mpHost->mpMap->audit();
            }
            else {
                qDebug().nospace() << "XMLimport::importPackage(...) ERROR: unrecognised element with name: "
                                   << name().toString()
                                   << "and content: "
                                   << text().toString();
            }
        }
    }


    if( ! packName.isEmpty() ) {
        if( ! gotTrigger ) {
             mpHost->getTriggerUnit()->unregisterTrigger( mpTrigger );
             delete mpTrigger;
        }

        if( gotTimer ) { // packName is NOT empty for modules...!
            mpTimer->setIsActive( true );
            mpTimer->enableTimer( mpTimer->getID() );
        }
        else {
            mpHost->getTimerUnit()->unregisterTimer( mpTimer );
            delete mpTimer;
        }

        if( gotAlias ) {
            mpAlias->setIsActive( true );
        }
        else {
            mpHost->getAliasUnit()->unregisterAlias( mpAlias );
            delete mpAlias;
        }

        if( gotAction ) {
            mpHost->getActionUnit()->updateToolbar();
        }
        else {
            mpHost->getActionUnit()->unregisterAction( mpAction );
            delete mpAction;
        }

        if( ! gotKey ) {
             mpHost->getKeyUnit()->unregisterKey( mpKey );
             delete mpKey;
        }

        if( ! gotScript ) {
             mpHost->getScriptUnit()->unregisterScript( mpScript );
             delete mpScript;
        }
    }

    return ! error();
}

void XMLimport::readVariableGroup( TVar *pParent )
{
    TVar * var = new TVar( pParent );
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    QString keyName, value;
    int keyType = 0;
    int valueType;
    while( ! atEnd() ) {
        readNext();
        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == "name" ) {
                keyName = readElementText();
                continue;
            }
            else if( name() == "value" ) {
                value = readElementText();
                continue;
            }
            else if( name() == "keyType" ) {
                keyType = readElementText().toInt() ;
                continue;
            }
            else if ( name() == "valueType" ) {
                valueType = readElementText().toInt();
                var->setName( keyName, keyType );
                var->setValue( value, valueType );
                vu->addSavedVar( var );
                lI->setValue( var );
                continue;
            }
            else if( name() == "VariableGroup" || name() == "Variable" ) {
                readVariableGroup( var );
            }
        }
    }
}

void XMLimport::readHiddenVariables()
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    while( ! atEnd() ) {
        readNext();
        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == "name" ) {
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
    while( ! atEnd() ) {
        readNext();
        if( isStartElement() ) {
            if( name() == "VariableGroup" || name() == "Variable" ) {
                readVariableGroup( mpVar );
            }
            else if( name() == "HiddenVariables" ) {
                readHiddenVariables();
            }
        }
    }
}

void XMLimport::readMap()
{
    QMultiHash<int, int> tempAreaRoomsHash; // Keys: area id, Values: a room id in that area

    while( ! atEnd() ) {
        readNext();

        if( isStartElement() ) {

            if( name() == "areas" ) {
                mpHost->mpMap->mpRoomDB->clearMapDB();
                mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing area data..." ) );
                mpHost->mpMap->reportProgressToProgressDialog( 0, 3 );
                readAreas();
            }
            else if( name() == "rooms" ) {
                mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing room data..." ) );
                mpHost->mpMap->reportProgressToProgressDialog( 1, 3 );
                readRooms( tempAreaRoomsHash );
            }
            else if( name() == "environments" ) {
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
    while( ! atEnd() ) {
        readNext();

        if( name() == "environment" ) {
            readEnvColor();
        }
    }
}

void XMLimport::readEnvColor()
{

    int id = attributes().value( "id" ).toString().toInt();
    int color = attributes().value( "color" ).toString().toInt();
    mpHost->mpMap->envColors[id] = color;
}

void XMLimport::readAreas()
{
    while( ! atEnd() ) {
        readNext();

        if( name() == "areas" ) {
            break;
        }
        else if( name() == "area" ) {
            readArea();
        }

    }
}

void XMLimport::readArea()
{
    int id = attributes().value( "id" ).toString().toInt();
    QString name = attributes().value( "name" ).toString();
    mpHost->mpMap->mpRoomDB->addArea( id, name );
}

void XMLimport::readRooms( QMultiHash<int, int> & areaRoomsHash )
{

    unsigned int roomCount = 0;
    while( ! atEnd() ) {
        readNext();

        if( Q_LIKELY( isStartElement() ) ) {
            if( Q_LIKELY( name() == QStringLiteral( "room" ) ) ) {
                readRoom( areaRoomsHash, &roomCount );
            }
            else {
                readUnknownMapElement();
            }

        }
        else if( isEndElement() ) {
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

    while( ! atEnd() ) {
        readNext();

        if( Q_UNLIKELY( pT->id < 1 ) ) {
            continue; // Skip further tests on exits as we'd have to throw away
                      // this invalid room and it would mess up the entranceMultiHash
        }
        else if( Q_LIKELY( name() == QStringLiteral( "exit" ) ) ) {
            QString dir = attributes().value( QStringLiteral( "direction" ) ).toString();
            int e = attributes().value( QStringLiteral( "target" ) ).toString().toInt();
            if( dir.isEmpty() ) {
                continue;
            }
            else if( dir == QStringLiteral( "north" ) ) {
                pT->north = e;
            }
            else if( dir == QStringLiteral( "east" ) ) {
                pT->east = e;
            }
            else if( dir == QStringLiteral( "south" ) ) {
                pT->south = e;
            }
            else if( dir == QStringLiteral( "west" ) ) {
                pT->west = e;
            }
            else if( dir == QStringLiteral( "up" ) ) {
                pT->up = e;
            }
            else if( dir == QStringLiteral( "down" ) ) {
                pT->down = e;
            }
            else if( dir == QStringLiteral( "northeast" ) ) {
                pT->northeast = e;
            }
            else if( dir == QStringLiteral( "southwest" ) ) {
                pT->southwest = e;
            }
            else if( dir == QStringLiteral( "southeast" ) ) {
                pT->southeast = e;
            }
            else if( dir == QStringLiteral( "northwest" ) ) {
                pT->northwest = e;
            }
            else if( dir == QStringLiteral( "in" ) ) {
                pT->in = e;
            }
            else if( dir == QStringLiteral( "out" ) ) {
                pT->out = e;
            }
            else {
                // TODO: Handle Special Exits
            }
        }
        else if( name() == QStringLiteral( "coord" ) ) {

            if( attributes().value( "x" ).toString().isEmpty() ) {
                continue;
            }

            pT->x = attributes().value( QStringLiteral( "x" ) ).toString().toInt();
            pT->y = attributes().value( QStringLiteral( "y" ) ).toString().toInt();
            pT->z = attributes().value( QStringLiteral( "z" ) ).toString().toInt();
        }
        else if( Q_UNLIKELY( name().isEmpty() ) ) {

            continue;
        }

        if( isEndElement() ) {
            break;
        }
    }

    if( pT->id > 0 ) {
        if( ( ++( *roomCount ) % 100 == 0 ) ) {
            mpHost->mpMap->reportStringToProgressDialog( tr( "Parsing room data [count: %1]..." ).arg( *roomCount ) );
        }
        areamRoomMultiHash.insert( pT->area, pT->id );
        // We are loading a map so can make some optimisation by setting the
        // third argument as true:
        mpHost->mpMap->mpRoomDB->addRoom( pT->id, pT, true );
        mMaxRoomId = qMax( mMaxRoomId, pT->id ); //Wasn't used but now maintains max Room Id
    }
    else {
        delete pT;
    }
}

void XMLimport::readUnknownMapElement()
{
    while( ! atEnd() ) {

        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            readMap();
        }
    }
}

void XMLimport::readPackage()
{
    while( ! atEnd() ) {
        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "HostPackage" ) {
                readHostPackage();
            }
            else if( name() == "TriggerPackage" ) {
                readTriggerPackage();
            }
            else if( name() == "TimerPackage" ) {
                readTimerPackage();
            }
            else if( name() == "AliasPackage" ) {
                readAliasPackage();
            }
            else if( name() == "ActionPackage" ) {
                readActionPackage();
            }
            else if( name() == "ScriptPackage" ) {
                readScriptPackage();
            }
            else if( name() == "KeyPackage" ) {
                readKeyPackage();
            }
            else if(name() == "HelpPackage" ) {
                readHelpPackage();
            }
            else if( name() == "VariablePackage" ) {
                readVariablePackage();
            }
            else {
                readUnknownPackage();
            }
        }
    }
}

void XMLimport::readHelpPackage()
{

    while( ! atEnd() ) {
        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "helpURL" ) {
                QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert( "helpURL", contents );
            }
        }
    }
}

void XMLimport::readUnknownPackage()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownPackage(): Error: UNKNOWN Package Element name: "
                           << name().toString()
                           << "content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readPackage();
        }
    }
}

void XMLimport::readUnknownHostElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Host Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readHostPackage( mpHost );
        }
    }
}

void XMLimport::readUnknownTriggerElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Trigger Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readTriggerPackage();
        }
    }
}

void XMLimport::readUnknownTimerElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Timer Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readTimerPackage();
        }
    }
}

void XMLimport::readUnknownAliasElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Alias Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readAliasPackage();
        }
    }
}

void XMLimport::readUnknownActionElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Action Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readActionPackage();
        }
    }
}

void XMLimport::readUnknownScriptElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Script Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readScriptPackage();
        }
    }
}

void XMLimport::readUnknownKeyElement()
{

    while( ! atEnd() ) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() Error: UNKNOWN Key Package Element, name: "
                           << name().toString()
                           << " content: "
                           << text().toString();

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            readKeyPackage();
        }
    }
}

void XMLimport::readHostPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "Host" ) {
                readHostPackage( mpHost );
            }
            else {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readHostPackage( Host * pT )
{
    pT->mAutoClearCommandLineAfterSend = ( attributes().value( "autoClearCommandLineAfterSend" ) == "yes" );
    pT->mDisableAutoCompletion = ( attributes().value( "disableAutoCompletion" ) == "yes" );
    pT->mPrintCommand = ( attributes().value( "printCommand" ) == "yes" );
    pT->set_USE_IRE_DRIVER_BUGFIX( attributes().value( "USE_IRE_DRIVER_BUGFIX" ) == "yes" );
    pT->mUSE_FORCE_LF_AFTER_PROMPT = ( attributes().value( "mUSE_FORCE_LF_AFTER_PROMPT" ) == "yes" );
    pT->mUSE_UNIX_EOL = ( attributes().value( "mUSE_UNIX_EOL" ) == "yes" );
    pT->mNoAntiAlias = ( attributes().value( "mNoAntiAlias" ) == "yes" );
    pT->mEchoLuaErrors = ( attributes().value( "mEchoLuaErrors" ) == "yes" );
    pT->mIsNextLogFileInHtmlFormat = ( attributes().value( "mRawStreamDump" ) == "yes" );
    pT->mAlertOnNewData = ( attributes().value( "mAlertOnNewData" ) == "yes" );
    pT->mFORCE_NO_COMPRESSION = ( attributes().value( "mFORCE_NO_COMPRESSION" ) == "yes" );
    pT->mFORCE_GA_OFF = ( attributes().value( "mFORCE_GA_OFF" ) == "yes" );
    pT->mFORCE_SAVE_ON_EXIT = ( attributes().value( "mFORCE_SAVE_ON_EXIT" ) == "yes" );
    pT->mEnableGMCP = ( attributes().value( "mEnableGMCP" ) == "yes" );
    pT->mEnableMSDP = ( attributes().value( "mEnableMSDP" ) == "yes" );
    pT->mMapStrongHighlight = ( attributes().value( "mMapStrongHighlight" ) == "yes" );
    pT->mLogStatus = ( attributes().value( "mLogStatus" ) == "yes" );
    pT->mEnableSpellCheck = ( attributes().value( "mEnableSpellCheck" ) == "yes" );
    pT->mShowInfo = ( attributes().value( "mShowInfo" ) == "yes" );
    pT->mAcceptServerGUI = ( attributes().value( "mAcceptServerGUI" ) == "yes" );
    pT->mMapperUseAntiAlias = ( attributes().value( "mMapperUseAntiAlias" ) == "yes" );
    pT->mFORCE_MXP_NEGOTIATION_OFF = ( attributes().value( "mFORCE_MXP_NEGOTIATION_OFF" ) == "yes" );
    pT->mRoomSize = attributes().value( "mRoomSize" ).toString().toFloat();
    if( ! pT->mRoomSize ) {
        pT->mRoomSize= 0.5f; // Same value as is in Host class initalizer list
    }
    pT->mLineSize = attributes().value( "mLineSize" ).toString().toFloat();
    if( !pT->mLineSize ) {
        pT->mLineSize= 1.0f; // Same value as is in Host class initalizer list
    }
    pT->mBubbleMode = ( attributes().value( "mBubbleMode" ) == "yes" );
    pT->mShowRoomID = ( attributes().value( "mShowRoomIDs" ) == "yes" );
    pT->mShowPanel = ( attributes().value( "mShowPanel" ) == "yes" );
    pT->mHaveMapperScript = ( attributes().value( "mHaveMapperScript" ) == "yes" );
    QStringRef ignore = attributes().value( "mDoubleClickIgnore" );
    for( int i = 0, total = ignore.size(); i < total; ++i ) {
        pT->mDoubleClickIgnore.insert( ignore.at( i ) );
    }

    while( ! atEnd() ) {
        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->mHostName = readElementText();
            }
            else if( name() == "mInstalledModules" ) {

                QMap<QString, QStringList> entry;

                readModulesDetailsMap( entry );

                QMapIterator<QString, QStringList> it( entry );
                while( it.hasNext() ) {
                    it.next();
                    QStringList moduleList;
                    QStringList entryList = it.value();
                    moduleList << entryList.at(0);
                    moduleList << entryList.at(1);
                    pT->mInstalledModules[it.key()] = moduleList;
                    pT->mModulePriorities[it.key()] = entryList.at(2).toInt();
                }
            }
            else if( name() == "mInstalledPackages" ) {
                readStringList( pT->mInstalledPackages );
            }
            else if( name() =="url" ) {
                pT->mUrl = readElementText();
            }
            else if( name() =="serverPackageName" ) {
                pT->mServerGUI_Package_name = readElementText();
            }
            else if( name() == "serverPackageVersion" ) {
                pT->mServerGUI_Package_version = readElementText().toInt();
            }
            else if( name() == "port" ) {
                pT->mPort = readElementText().toInt();
            }
            else if( name() == "borderTopHeight" ) {
                pT->mBorderTopHeight = readElementText().toInt();
            }
            else if( name() == "commandLineMinimumHeight" ) {
                pT->commandLineMinimumHeight = readElementText().toInt();
            }
            else if( name() == "borderBottomHeight" ) {
                pT->mBorderBottomHeight = readElementText().toInt();
            }
            else if( name() == "borderLeftWidth" ) {
                pT->mBorderLeftWidth = readElementText().toInt();
            }
            else if( name() == "borderRightWidth" ) {
                pT->mBorderRightWidth = readElementText().toInt();
            }
            else if( name() == "wrapAt" ) {
                pT->mWrapAt = readElementText().toInt();
            }
            else if( name() == "wrapIndentCount" ) {
                pT->mWrapIndentCount = readElementText().toInt();
            }
            else if( name() == "mCommandSeparator" ) {
                pT->mCommandSeparator = readElementText();
            }
            else if( name() == "mCommandLineFgColor" ) {
                pT->mCommandLineFgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mCommandLineBgColor" ) {
                pT->mCommandLineBgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mFgColor" ) {
                pT->mFgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mBgColor" ) {
                pT->mBgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mCommandFgColor" ) {
                pT->mCommandFgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mCommandBgColor" ) {
                pT->mCommandBgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mBlack" ) {
                pT->mBlack.setNamedColor( readElementText() );
            }
            else if( name() == "mLightBlack" ) {
                pT->mLightBlack.setNamedColor( readElementText() );
            }
            else if( name() == "mRed" ) {
                pT->mRed.setNamedColor( readElementText() );
            }
            else if( name() == "mLightRed" ) {
                pT->mLightRed.setNamedColor( readElementText() );
            }
            else if( name() == "mBlue" ) {
                pT->mBlue.setNamedColor( readElementText() );
            }
            else if( name() == "mLightBlue" ) {
                pT->mLightBlue.setNamedColor( readElementText() );
            }
            else if( name() == "mGreen" ) {
                pT->mGreen.setNamedColor( readElementText() );
            }
            else if( name() == "mLightGreen" ) {
                pT->mLightGreen.setNamedColor( readElementText() );
            }
            else if( name() == "mYellow" ) {
                pT->mYellow.setNamedColor( readElementText() );
            }
            else if( name() == "mLightYellow" ) {
                pT->mLightYellow.setNamedColor( readElementText() );
            }
            else if( name() == "mCyan" ) {
                pT->mCyan.setNamedColor( readElementText() );
            }
            else if( name() == "mLightCyan" ) {
                pT->mLightCyan.setNamedColor( readElementText() );
            }
            else if( name() == "mMagenta" ) {
                pT->mMagenta.setNamedColor( readElementText() );
            }
            else if( name() == "mLightMagenta" ) {
                pT->mLightMagenta.setNamedColor( readElementText() );
            }
            else if( name() == "mWhite" ) {
                pT->mWhite.setNamedColor( readElementText() );
            }
            else if( name() == "mLightWhite" ) {
                pT->mLightWhite.setNamedColor( readElementText() );
            }
            else if( name() == "mDisplayFont" ) {
                pT->mDisplayFont.fromString( readElementText() );
                pT->mDisplayFont.setFixedPitch( true );
            }
            else if( name() == "mCommandLineFont" ) {
                pT->mCommandLineFont.fromString( readElementText() );
            }
            else if( name() == "commandSeperator" ) {
                // TODO: Ignore this misspelled duplicate, can remove it if we
                // check the Xml format version and revise it under something
                // > 1.0 in the future.
                Q_UNUSED( readElementText() );
            }
            else if( name() == "mFgColor2" ) {
                pT->mFgColor_2.setNamedColor( readElementText() );
            }
            else if( name() == "mBgColor2" ) {
                pT->mBgColor_2.setNamedColor( readElementText() );
            }
            else if( name() == "mBlack2" ) {
                pT->mBlack_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightBlack2" ) {
                pT->mLightBlack_2.setNamedColor( readElementText() );
            }
            else if( name() == "mRed2" ) {
                pT->mRed_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightRed2" ) {
                pT->mLightRed_2.setNamedColor( readElementText() );
            }
            else if( name() == "mBlue2" ) {
                pT->mBlue_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightBlue2" ) {
                pT->mLightBlue_2.setNamedColor( readElementText() );
            }
            else if( name() == "mGreen2" ) {
                pT->mGreen_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightGreen2" ) {
                pT->mLightGreen_2.setNamedColor( readElementText() );
            }
            else if( name() == "mYellow2" ) {
                pT->mYellow_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightYellow2" ) {
                pT->mLightYellow_2.setNamedColor( readElementText() );
            }
            else if( name() == "mCyan2" ) {
                pT->mCyan_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightCyan2" ) {
                pT->mLightCyan_2.setNamedColor( readElementText() );
            }
            else if( name() == "mMagenta2" ) {
                pT->mMagenta_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightMagenta2" ) {
                pT->mLightMagenta_2.setNamedColor( readElementText() );
            }
            else if( name() == "mWhite2" ) {
                pT->mWhite_2.setNamedColor( readElementText() );
            }
            else if( name() == "mLightWhite2" ) {
                pT->mLightWhite_2.setNamedColor( readElementText() );
            }
            else if( name() == "mSpellDic" ) {
                pT->mSpellDic = readElementText();
            }
            else if( name() == "mLineSize" || name() == "mRoomSize" ) {
                Q_UNUSED( readElementText() );
                // TODO: This and mLineSize can be dropped when we add Xml format
                // version checking and update the format as these are duplicates
                // of attributes that were incorrected read in the parent
                // <Host ...> element as integers {they are stored as decimals
                // but for the first one at least it is a decimal number n,
                // where 0.1 <= n <= 1.1 so was being read as "0" for all but
                // the greatest 2 values where it was read as "1"!}
            }
            else {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readTriggerPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == "TriggerGroup" || name() == "Trigger" ) {
                gotTrigger = true;
                readTriggerGroup( mPackageName.isEmpty() ? 0 : mpTrigger );
            }
            else {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readTriggerGroup( TTrigger * pParent )
{
    TTrigger * pT = new TTrigger( pParent, mpHost );

    if( module ) {
        pT->mModuleMember = true;
    }

    mpHost->getTriggerUnit()->registerTrigger( pT );

    pT->setIsActive( attributes().value( "isActive" ) == "yes" );
    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    pT->mIsTempTrigger = ( attributes().value( "isTempTrigger" ) == "yes" );
    pT->mIsMultiline = ( attributes().value( "isMultiline" ) == "yes" );
    pT->mPerlSlashGOption = ( attributes().value( "isPerlSlashGOption" ) == "yes" );
    pT->mIsColorizerTrigger = ( attributes().value( "isColorizerTrigger" ) == "yes" );
    pT->mFilterTrigger = ( attributes().value( "isFilterTrigger" ) == "yes" );
    pT->mSoundTrigger = ( attributes().value( "isSoundTrigger" ) == "yes" );
    pT->mColorTrigger = ( attributes().value( "isColorTrigger" ) == "yes" );
    pT->mColorTriggerBg = ( attributes().value( "isColorTriggerBg" ) == "yes" );
    pT->mColorTriggerFg = ( attributes().value( "isColorTriggerFg" ) == "yes" );

    QString tempScript;
    while( ! atEnd() ) {
        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->setName( readElementText() );
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "triggerType" ) {
                pT->mTriggerType = readElementText().toInt();
            }
            else if( name() == "conditonLineDelta" ) {
                pT->mConditionLineDelta = readElementText().toInt();
            }
            else if( name() == "mStayOpen" ) {
                pT->mStayOpen = readElementText().toInt();
            }
            else if( name() == "mCommand" ) {
                pT->mCommand = readElementText();
            }
            else if( name() == "mFgColor" ) {
                pT->mFgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mBgColor" ) {
                pT->mBgColor.setNamedColor( readElementText() );
            }
            else if( name() == "colorTriggerFgColor" ) {
                pT->mColorTriggerFgColor.setNamedColor( readElementText() );
            }
            else if( name() == "colorTriggerBgColor" ) {
                pT->mColorTriggerBgColor.setNamedColor( readElementText() );
            }
            else if( name() == "mSoundFile" ) {
                pT->mSoundFile = readElementText();
            }
            else if( name() == "regexCodeList" ) {
                readStringList( pT->mRegexCodeList );
            }
            else if( name() == "regexCodePropertyList" ) {
                readIntegerList( pT->mRegexCodePropertyList );
                continue;
            }
            else if( name() == "TriggerGroup" || name() == "Trigger" ) {
                readTriggerGroup( pT );
            }
            else {
                readUnknownTriggerElement();
            }
        }
    }

    if( ! pT->setRegexCodeList( pT->mRegexCodeList, pT->mRegexCodePropertyList ) ) {
        qDebug().nospace() << "XMLimport::readTriggerGroup(...): Error: can not initialize pattern list for trigger: "
                           << pT->getName();
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readTriggerGroup(...): Error: can not compile trigger script for: "
                           << pT->getName();
    }
}

void XMLimport::readTimerPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "TimerGroup" || name() == "Timer" ) {
                gotTimer = true;
                readTimerGroup( mPackageName.isEmpty() ? 0 : mpTimer );
            }
            else {
                readUnknownTimerElement();
            }
        }
    }
}

void XMLimport::readTimerGroup( TTimer * pParent )
{
    TTimer * pT = new TTimer( pParent, mpHost );

    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    pT->mIsTempTimer = ( attributes().value( "isTempTimer" ) == "yes" );

    mpHost->getTimerUnit()->registerTimer( pT );
    pT->setShouldBeActive( ( attributes().value( "isActive" ) == "yes" ) );

    if( module ) {
        pT->mModuleMember = true;
    }

    QString tempScript;
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->setName( readElementText() );
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "command" ) {
                pT->mCommand = readElementText();
            }
            else if( name() == "time" ) {
                pT->setTime( QTime::fromString( readElementText(), "hh:mm:ss.zzz" ) );
            }
            else if( name() == "TimerGroup" || name() == "Timer" ) {
                readTimerGroup( pT );
            }
            else {
                readUnknownTimerElement();
            }
        }
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readTimerGroup(...): Error: can not compile timer script for: "
                           << pT->getName();
    }

    mudlet::self()->registerTimer( pT, pT->mpTimer );

    if( ! pT->mpParent && pT->shouldBeActive() ) {
        pT->setIsActive( true );
        pT->enableTimer( pT->getID() );
    }

}

void XMLimport::readAliasPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "AliasGroup" || name() == "Alias" ) {
                gotAlias = true;
                readAliasGroup( mPackageName.isEmpty() ? 0 : mpAlias );
            }
            else {
                readUnknownAliasElement();
            }
        }
    }
}

void XMLimport::readAliasGroup( TAlias * pParent )
{
    TAlias * pT = new TAlias( pParent, mpHost );

    mpHost->getAliasUnit()->registerAlias( pT );
    pT->setIsActive( attributes().value( "isActive" ) == "yes" );
    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    if( module ) {
        pT->mModuleMember = true;
    }

    QString tempScript;
    while( ! atEnd() ) {
        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->setName( readElementText() );
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "command" ) {
                pT->mCommand = readElementText();
            }
            else if( name() == "regex" ) {
                pT->setRegexCode( readElementText() );
            }
            else if( name() == "AliasGroup" || name() == "Alias" ) {
                readAliasGroup( pT );
            }
            else {
                readUnknownAliasElement();
            }
        }
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readAliasGroup(...): Error: can not compile timer script for: "
                           << pT->getName();
    }

}

void XMLimport::readActionPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "ActionGroup" || name() == "Action" ) {
                gotAction = true;
                readActionGroup( mPackageName.isEmpty() ? 0 : mpAction );
            }
            else {
                readUnknownActionElement();
            }
        }
    }
}

void XMLimport::readActionGroup( TAction * pParent )
{
    TAction * pT = new TAction( pParent, mpHost );

    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    pT->mIsPushDownButton = ( attributes().value( "isPushButton" ) == "yes" );
    pT->mButtonFlat = ( attributes().value( "isFlatButton" ) == "yes" );
    pT->mUseCustomLayout = ( attributes().value( "useCustomLayout" ) == "yes" );
    mpHost->getActionUnit()->registerAction( pT );
    pT->setIsActive( attributes().value( "isActive" ) == "yes" );

    if( module ) {
        pT->mModuleMember = true;
    }

    QString tempScript;
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->mName = readElementText();
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "css" ) {
                pT->css = readElementText();
            }
            else if( name() == "commandButtonUp" ) {
                pT->mCommandButtonUp = readElementText();
            }
            else if( name() == "commandButtonDown" ) {
                pT->mCommandButtonDown = readElementText();
            }
            else if( name() == "icon" ) {
                pT->mIcon = readElementText();
            }
            else if( name() == "orientation" ) {
                pT->mOrientation = readElementText().toInt();
            }
            else if( name() == "location" ) {
                pT->mLocation = readElementText().toInt();
            }
            else if( name() == "buttonRotation" ) {
                pT->mButtonRotation = readElementText().toInt();
            }
            else if( name() == "sizeX" ) {
                pT->mSizeX = readElementText().toInt();
            }
            else if( name() == "sizeY" ) {
                pT->mSizeY = readElementText().toInt();
            }
            else if( name() == "mButtonState" ) {
                pT->mButtonState = readElementText().toInt();
            }
            else if( name() == "buttonColor" ) {
                pT->mButtonColor.setNamedColor( readElementText() );
            }
            else if( name() == "buttonColumn" ) {
                pT->mButtonColumns = readElementText().toInt();
            }
            else if( name() == "posX" ) {
                pT->mPosX = readElementText().toInt();
            }
            else if( name() == "posY" ) {
                pT->mPosY = readElementText().toInt();
            }
            else if( name() == "ActionGroup" || name() == "Action" ) {
                readActionGroup( pT );
            }
            else {
                readUnknownActionElement();
            }
        }
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readActionGroup(...): Error: can not compile trigger script for: "
                           << pT->getName();
    }
}

void XMLimport::readScriptPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "ScriptGroup" || name() == "Script" ) {
                gotScript = true;
                readScriptGroup( mPackageName.isEmpty() ? 0 : mpScript );
            }
            else {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readScriptGroup( TScript * pParent )
{
    TScript * pT = new TScript( pParent, mpHost );

    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    mpHost->getScriptUnit()->registerScript( pT );
    pT->setIsActive( attributes().value( "isActive" ) == "yes" );

    if( module ) {
        pT->mModuleMember = true;
    }

    QString tempScript;
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->mName = readElementText();
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "eventHandlerList" ) {
                readStringList( pT->mEventHandlerList );
                pT->setEventHandlerList( pT->mEventHandlerList );
            }
            else if( name() == "ScriptGroup" || name() == "Script" ) {
                readScriptGroup( pT );
            }
            else {
                readUnknownScriptElement();
            }
        }
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readScriptGroup(...): Error: can not compile trigger script for: "
                           << pT->getName();
    }
}

void XMLimport::readKeyPackage()
{
    while( ! atEnd() ) {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "KeyGroup" || name() == "Key" ) {
                gotKey = true;
                readKeyGroup( mPackageName.isEmpty() ? 0 : mpKey );
            }
            else {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readKeyGroup( TKey * pParent )
{
    TKey * pT = new TKey( pParent, mpHost );

    pT->mIsFolder = ( attributes().value( "isFolder" ) == "yes" );
    mpHost->getKeyUnit()->registerKey( pT );
    pT->setIsActive( attributes().value( "isActive" ) == "yes" );

    if( module ) {
        pT->mModuleMember = true;
    }

    QString tempScript;
    while( ! atEnd() )
    {

        readNext();
        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "name" ) {
                pT->mName = readElementText();
            }
            else if( name() == "packageName" ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == "script" ) {
                tempScript = readElementText();
            }
            else if( name() == "command" ) {
                pT->mCommand = readElementText();
            }
            else if( name() == "keyCode" ) {
                pT->mKeyCode = readElementText().toInt();
            }
            else if( name() == "keyModifier" ) {
                pT->mKeyModifier = readElementText().toInt();
            }
            else if( name() == "KeyGroup" || name() == "Key" ) {
                readKeyGroup( pT );
            }
            else {
                readUnknownKeyElement();
            }
        }
    }

    if( ! pT->setScript( tempScript ) ) {
        qDebug().nospace() << "XMLimport::readKeyGroup(...): Error: can not compile trigger script for: "
                           << pT->getName();
    }
}

void XMLimport::readModulesDetailsMap( QMap<QString, QStringList> & pMap )
{
    QString key;
    QStringList entry;
    while( ! atEnd() ) {

        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "key" ) {
                key = readElementText();
            }
            else if( name() == "filepath" ) {
                entry << readElementText();
            }
            else if( name() == "globalSave" ) {
                entry << readElementText();
            }
            else if( name() == "priority" ) {
                // The last expected detail for the entry - so store this completed entry into the QMap
                entry << readElementText();
                pMap[key] = entry;
                entry.clear();
            }
            else {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readStringList( QStringList & list )
{
    while( ! atEnd() ) {

        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "string" ) {
                list << readElementText();
            }
            else {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readIntegerList( QList<int> & list )
{
    while( ! atEnd() ) {

        readNext();

        if( isEndElement() ) {
            break;
        }
        else if( isStartElement() ) {
            if( name() == "integer" ) {
                QString numberText = readElementText();
                bool ok = false;
                int num = numberText.toInt( &ok, 10 );
                if( ! numberText.isEmpty() && ok ) {
                    list << num;
                }
                else {
                    qWarning().nospace() << "XMLimport::readIntegerList(...) Error: unable to convert: "
                                         << numberText
                                         << "when reading package property list - this is an in invalid element!";
                }
            }
            else {
                readUnknownTriggerElement();
            }
        }
    }
}
