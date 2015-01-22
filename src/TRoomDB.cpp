/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2015 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TRoomDB.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "Host.h"

#include "pre_guard.h"
#include <QApplication>
#include <QDebug>
#include <QTime>
#include "post_guard.h"


TRoomDB::TRoomDB( TMap * pMap )
: mpMap( pMap )
, mUnnamedAreaName( qApp->translate( "TRoomDB", "Unnamed Area" ) )
{
}

TRoom * TRoomDB::getRoom( int id )
{
    if (id < 0)
        return 0;
    QHash< int, TRoom * >::iterator i = rooms.find( id );
    if ( i != rooms.end() && i.key() == id )
        return i.value();
    return 0;
}

bool TRoomDB::addRoom( int id )
{
    qDebug()<<"addRoom("<<id<<")";
    if( !rooms.contains( id ) && id > 0 )
    {
        rooms[id] = new TRoom( this );
        rooms[id]->setId(id);
        QHash<int, int> exits = rooms[id]->getExits();
        QList<int> toExits = exits.keys();
        for (int i = 0; i < toExits.size(); i++)
            reverseExitMap.insert(id, toExits[i]);
        return true;
    }
    else
    {
        if( id <= 0 )
        {
            QString error = QString("illegal room id=%1. roomID must be > 0").arg(id);
            mpMap->logError(error);
        }
        return false;
    }
}

bool TRoomDB::addRoom( int id, TRoom * pR )
{
    if( !rooms.contains( id ) && id > 0 && pR )
    {
        rooms[id] = pR;
        pR->setId(id);
        return true;
    }
    else
    {
        return false;
    }
}

// this is call by TRoom destructor only
bool TRoomDB::__removeRoom( int id )
{
    TRoom* pR = getRoom(id);
    if (pR) {
        // FIXME: make a proper exit controller so we don't need to do all these if statements
        QMultiHash<int, int>::iterator i = reverseExitMap.find(id);
        while (i != reverseExitMap.end() && i.key() == id) {
            TRoom* r = getRoom(i.value());
            if (r) {
                if (r->getNorth() == id)
                    r->setNorth(-1);
                if (r->getNortheast() == id)
                    r->setNortheast(-1);
                if (r->getNorthwest() == id)
                    r->setNorthwest(-1);
                if (r->getEast() == id)
                    r->setEast(-1);
                if (r->getWest() == id)
                    r->setWest(-1);
                if (r->getSouth() == id)
                    r->setSouth(-1);
                if (r->getSoutheast() == id)
                    r->setSoutheast(-1);
                if (r->getSouthwest() == id)
                    r->setSouthwest(-1);
                if (r->getUp() == id)
                    r->setUp(-1);
                if (r->getDown() == id)
                    r->setDown(-1);
                if (r->getIn() == id)
                    r->setIn(-1);
                if (r->getOut() == id)
                    r->setOut(-1);
                r->removeAllSpecialExitsToRoom(id);
            }
            ++i;
        }
        rooms.remove(id);
        // FIXME: make hashTable a bimap
        QList<QString> keyList = hashTable.keys();
        QList<int> valueList = hashTable.values();
        for (int i = 0; i < valueList.size(); i++) {
            if (valueList[i] == id) {
                hashTable.remove(keyList[i]);
            }
        }
        int areaID = pR->getArea();
        TArea * pA = getArea( areaID );
        if (pA)
            pA->removeRoom(id);
        reverseExitMap.remove(id);
        // Because we clear the graph in initGraph which will be called
        // if mMapGraphNeedsUpdate is true -- we don't need to
        // remove the vertex using clear_vertex and remove_vertex here
        mpMap->mMapGraphNeedsUpdate = true;
        return true;
    }
    return false;
}

bool TRoomDB::removeRoom( int id )
{
    if( rooms.contains( id ) && id > 0 ) {
        if( mpMap->mRoomId == id ) {
            mpMap->mRoomId = 0;
        }
        if( mpMap->mTargetID == id ) {
            mpMap->mTargetID = 0;
        }
        TRoom * pR = getRoom( id );
        delete pR;
        return true;
    }
    return false;
}

bool TRoomDB::removeArea( int id )
{
    if( areas.contains( id ) ) {
        TArea * pA = areas.value(id);
        QList<int> rl;
        for( int i=0; i< pA->rooms.size(); i++ ) {
            rl.push_back( pA->rooms.at(i) );
        }
        for( int i=rl.size()-1; i >= 0; i-- ) {
            removeRoom( rl.at(i) );
        }
        areaNamesMap.remove( id );
        areas.remove( id );

        mpMap->mMapGraphNeedsUpdate = true;
        return true;
    }
    return false;
}

bool TRoomDB::removeArea( QString name )
{
    if( areaNamesMap.values().contains( name ) )
    {
        return removeArea( areaNamesMap.key( name ) );
    }
    return false;
}

void TRoomDB::removeArea( TArea * pA )
{
    removeArea( getAreaID(pA) );
}

int TRoomDB::getAreaID( TArea * pA )
{
    return areas.key(pA);
}

void TRoomDB::buildAreas()
{
    QTime _time; _time.start();
    QHashIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {
       it.next();
       int id = it.key();
       TRoom * pR = getRoom(id);
       if( !pR ) continue;
       TArea * pA = getArea(pR->getArea());
       if( !pA )
       {
           areas[pR->getArea()] = new TArea( mpMap, this );
       }
    }

    // if the area has been created without any rooms add the area ID
    QMapIterator<int, QString> it2( areaNamesMap );
    while( it2.hasNext() )
    {
       it2.next();
       int id = it2.key();
       if( ! areas.contains( id ) )
       {
           areas[id] = new TArea( mpMap, this );
       }
    }
    qDebug()<<"BUILD AREAS run time:"<<_time.elapsed();
}


const QList<TRoom *> TRoomDB::getRoomPtrList()
{
    return rooms.values();
}

QList<int> TRoomDB::getRoomIDList()
{
    return rooms.keys();
}

TArea * TRoomDB::getArea( int id )
{
    //area id of -1 is a room in the "void", 0 is a failure
    if( id > 0 || id == -1 ) {
        return areas.value( id, 0 );
    }
    else {
        return 0;
    }
}

bool TRoomDB::setAreaName( int areaID, QString name )
{
    if( areaID < 1 ) {
        qWarning( "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Suspect areaID: %n supplied.", areaID );
        return false;
    }
    if( name.isEmpty() ) {
        qWarning( "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Empty name supplied." );
        return false;
    }
    else if( areaNamesMap.values().count(name) > 0 ) {
        // That name is already IN the areaNamesMap
        if( areaNamesMap.value( areaID ) == name ) {
            // The trivial case, the given areaID already IS that name
            return true;
        }
        else {
            qWarning( "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Duplicate name supplied \"%s\"- that is not a good idea!", name.toUtf8().constData() );
            return false;
        }
    }
    areaNamesMap[areaID] = name;
    return true;
}

bool TRoomDB::addArea( int id )
{
    if( ! areas.contains( id ) ) {
        areas[id] = new TArea( mpMap, this );
        if( ! areaNamesMap.contains( id ) ) {
            // Must provide a name for this new area
            QString newAreaName = mUnnamedAreaName;
            if( areaNamesMap.values().contains( newAreaName ) ) {
                // We already have an "unnamed area"
                uint deduplicateSuffix = 0;
                do {
                    newAreaName = QStringLiteral( "%1_%2" ).arg( mUnnamedAreaName ).arg( ++deduplicateSuffix, 3, 10, QLatin1Char('0') );
                } while ( areaNamesMap.values().contains( newAreaName ) );
            }
            areaNamesMap.insert( id, newAreaName );
        }
        return true;
    }
    else {
        QString error = qApp->translate( "TRoomDB", "Area with ID=%1 already exists!" ).arg(id);
        mpMap->logError(error);
        return false;
    }
}

int TRoomDB::createNewAreaID()
{
    int id = 1;
    while( areas.contains( id ) ) {
        id++;
    }
    return id;
}

int TRoomDB::addArea( QString name )
{
    // reject it if area name already exists or is empty
    if( name.isEmpty() ) {
        QString error = qApp->translate( "TRoomDB", "An Unnamed Area is (no longer) permitted!" );
        mpMap->logError(error);
        return 0;
    }
    else if( areaNamesMap.values().contains( name ) ) {
        QString error = qApp->translate( "TRoomDB", "An area called %1 already exists!" ).arg(name);
        mpMap->logError(error);
        return 0;
    }

    int areaID = createNewAreaID();
    if( addArea( areaID ) ) {
        areaNamesMap[areaID] = name;
        // This will overwrite the "Unnamed Area_###" that addArea( areaID )
        // will generate - but that is fine.
        return areaID;
    }
    else {
        return 0; //fail
    }
}

// this func is called by the xml map importer
// NOTE: we no longer accept duplicate IDs or duplicate area names
//       duplicate definitions are ignored
//       Unless the area name is empty, in which case we provide one!
bool TRoomDB::addArea( int id, QString name )
{
    if(   ( ( ! name.isEmpty() ) && areaNamesMap.values().contains( name ) )
       || areaNamesMap.keys().contains( id ) ) {
        return false;
    }
    else if( addArea( id ) ) {
        // This will generate an "Unnamed Area_###" area name which we should
        // overwrite only if we have a name!
        if( ! name.isEmpty() ) {
            areaNamesMap[id] = name;
        }
        return true;
    }
    else {
        return false;
    }
}

const QList<TArea *> TRoomDB::getAreaPtrList()
{
    return areas.values();
}

QList<int> TRoomDB::getAreaIDList()
{
    return areas.keys();
}

void TRoomDB::auditRooms()
{
    QTime t; t.start();
    // rooms konsolidieren
    QHashIterator<int, TRoom* > itRooms( rooms );
    while( itRooms.hasNext() )
    {
        itRooms.next();
        TRoom * pR = itRooms.value();
        pR->auditExits();

    }
    qDebug()<<"audit map: runtime:"<<t.elapsed();
}

void TRoomDB::initAreasForOldMaps()
{
    buildAreas();

    // area mit raeumen fuellen
    QHashIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {
        it.next();
        int roomID = it.key();
        int areaID = rooms[roomID]->getArea();
        if( areas.contains(areaID)) areas[areaID]->rooms.push_back(roomID);
    }
    QMapIterator<int, TArea *> it2( areas );
    while( it2.hasNext() )
    {
        it2.next();
        TArea * pA = it2.value();
        pA->determineAreaExits();
        pA->calcSpan();
    }
}

void TRoomDB::clearMapDB()
{
    QList<TRoom*> rPtrL = getRoomPtrList();
    rooms.clear();
    areaNamesMap.clear();
    hashTable.clear();
    for(int i=0; i<rPtrL.size(); i++ )
    {
        delete rPtrL[i];
    }
    assert( rooms.size() == 0 );

    QList<TArea*> areaList = getAreaPtrList();
    for( int i=0; i<areaList.size(); i++ )
    {
        delete areaList[i];
    }
    assert( areas.size() == 0 );


}

void TRoomDB::restoreAreaMap( QDataStream & ifs )
{
    QMap<int, QString> areaNamesMapWithPossibleEmptyOrDuplicateItems;
    ifs >> areaNamesMapWithPossibleEmptyOrDuplicateItems;
    areaNamesMap.clear(); // Following code assumes areaNamesMap is empty but
                          // under some situations this has not been the case...

    // Validate names: name nameless areas and rename duplicates
    QMultiMap<QString, QString> renamedMap; // For warning message, holds renamed area map
    QMapIterator<int, QString> itArea = areaNamesMapWithPossibleEmptyOrDuplicateItems;
    bool isMatchingSuffixAlreadyPresent = false;
    bool isEmptyAreaNamePresent = false;
    while( itArea.hasNext() ) {
        itArea.next();
        QString nonEmptyAreaName;
        if( itArea.value().isEmpty() ) {
            isEmptyAreaNamePresent = true;
            nonEmptyAreaName = mUnnamedAreaName;
            // Will trip following if more than one
        }
        else {
            nonEmptyAreaName = itArea.value();
        }
        if( areaNamesMap.values().contains( nonEmptyAreaName ) ) {
            // Oh dear, we have a duplicate
            if( nonEmptyAreaName.contains( QRegExp( "_\d\d\d$" ) ) ) {
                // the areaName already is of form "something_###" where # is a
                // digit, have to strip that off and remember so warning message
                // can include advice on this change
                isMatchingSuffixAlreadyPresent = true;
                nonEmptyAreaName.chop(4); // Take off existing suffix
            }
            uint deduplicateSuffix = 0;
            QString replacementName;
            do {
                replacementName = QStringLiteral( "%1_%2" ).arg(nonEmptyAreaName).arg(++deduplicateSuffix, 3, 10, QLatin1Char('0') );
            } while ( areaNamesMap.values().contains( replacementName ) );
            if( ( ! itArea.value().isEmpty() ) && ( ! renamedMap.contains( itArea.value() ) ) ) {
                // if the renamedMap does not contain the first, unaltered value
                // that a subsequent match has been found for, then include it
                // in the data so the user can see the UNCHANGED one as well
                // Only have to do this once for each duplicate area name, hence
                // the test conditions above
                renamedMap.insert( itArea.value(), itArea.value() );
            }
            renamedMap.insert( itArea.value(), replacementName );
            areaNamesMap.insert( itArea.key(), replacementName );
        }
        else {
            if( itArea.value().isEmpty() ) {
                renamedMap.insert( itArea.value(), nonEmptyAreaName );
            }
            areaNamesMap.insert( itArea.key(), nonEmptyAreaName );
        }
    }
    if( renamedMap.size() || isEmptyAreaNamePresent ) {
        QString alertText;
        QString informativeText;
        QString extraTextForMatchingSuffixAlreadyUsed;
        QString detailText;
        if( isMatchingSuffixAlreadyPresent ) {
            extraTextForMatchingSuffixAlreadyUsed = qApp->translate( "TRoomDB", "It has been detected that \"_###\" form suffixes have already been used, for "
                                                                                "simplicity in the renaming algorithm these will have been removed and possibly "
                                                                                "changed as Mudlet sorts this matter out, if a number assigned in this way "
                                                                                "<b>is</b> important to you, you can change it back, provided you rename the area "
                                                                                "that has been allocated the suffix that was wanted first...!</p>" );
        }
        if( renamedMap.size() ) {
            detailText = qApp->translate( "TRoomDB", "[  OK  ]  - The changes made are:\n"
                                                                 "(ID) \"old name\" ==> \"new name\"\n" );
            QMapIterator<QString, QString> itRemappedNames = renamedMap;
            itRemappedNames.toBack();
            // Seems to look better if we iterate through backwards!
            while( itRemappedNames.hasPrevious() ) {
                itRemappedNames.previous();
                detailText.append( QStringLiteral( "(%1) \"%2\" ==> \"%3\"\n" )
                                   .arg( areaNamesMap.key( itRemappedNames.value() ) )
                                   .arg( itRemappedNames.key().isEmpty() ? qApp->translate( "TRoomDB", "<nothing>" ) : itRemappedNames.key() )
                                   .arg( itRemappedNames.value() ) );
            }
            detailText.chop(1); // Trim last "\n" off
        }
        if( renamedMap.size() && isEmptyAreaNamePresent ) {
            // At least one unnamed area and at least one duplicate area name
            // - may be the same items
            alertText = qApp->translate( "TRoomDB", "[ ALERT ] - Empty and duplicate area names detected in Map file!" );
            informativeText = qApp->translate( "TRoomDB", "[ INFO ]  - Due to some situations not being checked in the past,  Mudlet had\n"
                                                                      "allowed the map to have more than one area with the same or no name.\n"
                                                                      "These make some things confusing and are now disallowed.\n"
                                                                      "  To resolve these cases, an area without a name here (or created in\n"
                                                                      "the future) will automatically be assigned the name \"%1\".\n"
                                                                      "  Duplicated area names will cause all but the first encountered one\n"
                                                                      "to gain a \"_###\" style suffix where each \"###\" is an increasing\n"
                                                                      "number; you may wish to change these, perhaps by replacing them with\n"
                                                                      "a \"(sub-area name)\" but it is entirely up to you how you do this,\n"
                                                                      "other then you will not be able to set one area's name to that of\n"
                                                                      "another that exists at the time.\n"
                                                                      "  If there were more than one area without a name then all but the\n"
                                                                      "first will also gain a suffix in this manner.\n"
                                                                      "%2")
                              .arg( mUnnamedAreaName )
                              .arg( extraTextForMatchingSuffixAlreadyUsed );
        }
        else if( renamedMap.size() ) {
            // Duplicates but no unnnamed area
            alertText = qApp->translate( "TRoomDB", "[ ALERT ] - Duplicate area names detected in the Map file!" );
            informativeText = qApp->translate( "TRoomDB", "[ INFO ]  - Due to some situations not being checked in the past, Mudlet had\n"
                                                                      "allowed the user to have more than one area with the same name.\n"
                                                                      "These make some things confusing and are now disallowed.\n"
                                                                      "  Duplicated area names will cause all but the first encountered one\n"
                                                                      "to gain a \"_###\" style suffix where each \"###\" is an increasing\n"
                                                                      "number; you may wish to change these, perhaps by replacing them with\n"
                                                                      "a \"(sub-area name)\" but it is entirely up to you how you do this,\n"
                                                                      "other then you will not be able to set one area's name to that of\n"
                                                                      "another that exists at the time.\n"
                                                                      "  If there were more than one area without a name then all but the\n"
                                                                      "first will also gain a suffix in this manner.\n"
                                                                      "%1)")
                              .arg( extraTextForMatchingSuffixAlreadyUsed );
        }
        else {
            // A single unnamed area found
            alertText = qApp->translate( "TRoomDB", "[ ALERT ] - An empty area name was detected in the Map file!" );
            // Use OK for this one because it is the last part and indicates the
            // sucessful end of something, whereas INFO is an intermediate step
            informativeText = qApp->translate( "TRoomDB", "[  OK  ]  - Due to some situations not being checked in the past, Mudlet had\n"
                                                                      "allowed the map to have an area with no name. This can make some\n"
                                                                      "things confusing and is now disallowed.\n"
                                                                      "  To resolve this case, the area without a name here (or one created\n"
                                                                      "in the future) will automatically be assigned the name \"%1\".\n"
                                                                      "  If this happens more then once the duplication of area names will\n"
                                                                      "cause all but the first encountered one to gain a \"_###\" style\n"
                                                                      "suffix where each \"###\" is an increasing number; you may wish to\n"
                                                                      "change these, perhaps by adding more meaningful area names but it is\n"
                                                                      "entirely up to you what is used, other then you will not be able to\n"
                                                                      "set one area's name to that of another that exists at the time.")
                              .arg( mUnnamedAreaName );
        }
        mpMap->mpHost->mTelnet.postMessage( alertText );
        mpMap->mpHost->mTelnet.postMessage( informativeText );
        if( ! detailText.isEmpty() ) {
            mpMap->mpHost->mTelnet.postMessage( detailText );
        }
    }
}

void TRoomDB::restoreSingleArea(QDataStream & ifs, int areaID, TArea * pA )
{
    areas[areaID] = pA;
}

void TRoomDB::restoreSingleRoom(QDataStream & ifs, int i, TRoom *pT)
{
    rooms[i] = pT;
}
