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

#include "pre_guard.h"
#include <QDebug>
#include <QElapsedTimer>
#include "post_guard.h"


TRoomDB::TRoomDB( TMap * pMap )
: mpMap( pMap )
, mpTempRoomDeletionList( 0 )
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
        // there is no point to update the entranceMap here, as the room has no exit information
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

bool TRoomDB::addRoom( int id, TRoom * pR, bool isMapLoading )
{
    if( !rooms.contains( id ) && id > 0 && pR )
    {
        rooms[id] = pR;
        pR->setId(id);
        updateEntranceMap(pR, isMapLoading);
        return true;
    }
    else
    {
        return false;
    }
}

void TRoomDB::deleteValuesFromEntranceMap( int value )
{
    QList<int> keyList = entranceMap.keys();
    QList<int> valueList = entranceMap.values();
    QList<uint> deleteEntries;
    uint index = valueList.indexOf( value );
    while ( index != -1 ) {
        deleteEntries.append( index );
        index = valueList.indexOf( value, index + 1 );
    }
    for (int i = deleteEntries.size() - 1; i >= 0; --i ) {
        entranceMap.remove( keyList.at(deleteEntries.at(i)), valueList.at(deleteEntries.at(i)) );
    }
}

void TRoomDB::deleteValuesFromEntranceMap( QSet<int> & valueSet )
{
    QElapsedTimer timer;
    timer.start();
    QList<int> keyList = entranceMap.keys();
    QList<int> valueList = entranceMap.values();
    QList<uint> deleteEntries;
    foreach(int roomId, valueSet) {
        int index = valueList.indexOf( roomId );
        while (index >= 0 ) {
            deleteEntries.append( index );
            index = valueList.indexOf( roomId, index + 1 );
        }
    }
    for (uint i = 0; i < deleteEntries.size(); ++i) {
        entranceMap.remove( keyList.at(deleteEntries.at(i)), valueList.at(deleteEntries.at(i)) );
    }
    qDebug() << "TRoomDB::deleteValuesFromEntranceMap() with a list of:" << valueSet.size() << "items, run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}

void TRoomDB::updateEntranceMap(int id)
{
    TRoom * pR = getRoom(id);
    updateEntranceMap(pR);
}

void TRoomDB::updateEntranceMap(TRoom * pR, bool isMapLoading)
{
    static bool showDebug = false; // Enable this at runtime (set a breakpoint on it) for debugging!

    // entranceMap maps the room to rooms it has a viable exit to. So if room b and c both have
    // an exit to room a, upon deleting room a we want a map that allows us to find
    // room b and c efficiently.
    // So we create a mapping like: {room_a: room_b, room_a: room_c}. This allows us to delete
    // rooms and know which other rooms are impacted by this change in a single lookup.
    if(pR){
        int id = pR->getId();
        QHash<int, int> exits = pR->getExits();
        QList<int> toExits = exits.keys();
        QString values;
        // to update this we need to iterate the entire entranceMap and remove invalid
        // connections. I'm not sure if this is efficient for every update, and given
        // that we check for rooms existance when the map is used, we'll deal with
        // possible spurious exits for now.
        // entranceMap.remove(id); // <== not what is wanted
        // We need to remove all values == id NOT keys == id, so try and do that
        // now. SlySven

        if( ! isMapLoading ) {
            deleteValuesFromEntranceMap( id ); // When LOADING a map, will never need to do this
        }
        for (int i = 0; i < toExits.size(); i++) {
            if( showDebug ) {
                values.append( QStringLiteral("%1,").arg(toExits.at(i)) );
            }
            entranceMap.insert(toExits.at(i), id);
        }
        if( showDebug ) {
            if( ! values.isEmpty() ) {
                values.chop(1);
            }
            if( values.isEmpty() ) {
                qDebug( "TRoomDB::updateEntranceMap(TRoom * pR) called for room with Id:%i, it is not an Entrance for any Rooms.", id );
            }
            else {
                qDebug( "TRoomDB::updateEntranceMap(TRoom * pR) called for room with Id:%i, it is an Entrance for Room(s): %s.", id, values.toLatin1().constData() );
            }
        }
    }
}

// this is call by TRoom destructor only
bool TRoomDB::__removeRoom( int id )
{
    static QMultiHash<int, int> _entranceMap; // Make it persistant - for multiple room deletions
    static bool isBulkDelete = false;
    // Gets set / reset by mpTempRoomDeletionList being non-null, used to setup
    // _entranceMap the first time around for multi-room deletions

    TRoom * pR = getRoom(id);
    // This will FAIL during map deletion as TRoomDB::rooms has already been
    // zapped, so can use to skip everything...
    if (pR) {
        if( mpTempRoomDeletionList && mpTempRoomDeletionList->size() > 1 ) { // We are deleting multiple rooms
            if( ! isBulkDelete ) {
                _entranceMap = entranceMap;
                _entranceMap.detach(); // MUST take a deep copy of the data
                isBulkDelete = true; // But only do it the first time for a bulk delete
            }
        }
        else { // We are deleting a single room
            if( isBulkDelete ) { // Last time was a bulk delete but it isn't one now
                isBulkDelete = false;
            }
            _entranceMap.clear();
            _entranceMap = entranceMap; // Refresh our local copy
            _entranceMap.detach(); // MUST take a deep copy of the data
        }

        // FIXME: make a proper exit controller so we don't need to do all these if statements
        // Remove the links from the rooms entering this room
        QMultiHash<int, int>::const_iterator i = _entranceMap.find(id);
        // The removeAllSpecialExitsToRoom below modifies the entranceMap - and
        // it is unsafe to modify (use copy operations on) something that an STL
        // iterator is active on - see "Implicit sharing iterator problem" in
        // "Container Class | Qt 5.x Core" - this is now avoid by taking a deep
        // copy and iterating through that instead whilst modifying the original
        while (i != entranceMap.end() && i.key() == id) {
            if( i.value() == id || mpTempRoomDeletionList && mpTempRoomDeletionList->size() > 1 && mpTempRoomDeletionList->contains( i.value() ) ) {
                ++i;
                continue; // Bypass rooms we know are also to be deleted
            }
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
        if( ( ! mpTempRoomDeletionList ) || mpTempRoomDeletionList->size() == 1 ) { // if NOT deleting multiple rooms
            entranceMap.remove(id); // Only removes matching keys
            deleteValuesFromEntranceMap(id); // Needed to remove matching values
        }
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
    if( rooms.contains(id ) && id > 0 )
    {
        TRoom * pR = getRoom( id );
        delete pR;
        return true;
    }
    return false;
}

void TRoomDB::removeRoom( QList<int> & ids )
{
    QElapsedTimer timer;
    timer.start();
    QSet<int> deletedRoomIds;
    mpTempRoomDeletionList = &ids; // Will activate "bulk room deletion" code
                                   // When used by TLuaInterpreter::deleteArea()
                                   // via removeArea(int) the list of rooms to
                                   // delete - as suppplied by the reference
                                   // type argument IS NOT CONSTANT - it is
                                   // ALTERED by TArea::removeRoom( int room )
                                   // for each room that is removed
    quint64 roomcount = mpTempRoomDeletionList->size();
    while( ! mpTempRoomDeletionList->isEmpty() ) {
        int deleteRoomId = mpTempRoomDeletionList->first();
        TRoom * pR = getRoom( deleteRoomId );
        if( pR ) {
            deletedRoomIds.insert( deleteRoomId );
            delete pR;
        }
    }
    foreach(int deleteRoomId, deletedRoomIds ) {
        entranceMap.remove( deleteRoomId ); // This has been deferred from __removeRoom()
    }
    deleteValuesFromEntranceMap( deletedRoomIds );
    mpTempRoomDeletionList->clear();
    mpTempRoomDeletionList=0;
    qDebug() << "TRoomDB::removeRoom(QList<int>) run time for" << roomcount << "rooms:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}

bool TRoomDB::removeArea( int id )
{
    if( areas.contains( id ) ) {
        TArea * pA = areas.value( id );
        if( ! rooms.isEmpty() ) {
            removeRoom( pA->rooms ); // During map deletion rooms will already
                                     // have been cleared so this would not
                                     // be wanted to be done in that case.
        }
        areaNamesMap.remove( id ); // During map deletion areaNamesMap will
                                   // already have been cleared !!!
        areas.remove( id ); // This means areas.clear() is not needed during map
                            // deletion

        mpMap->mMapGraphNeedsUpdate = true;
        return true;
    }
    return false;
}

bool TRoomDB::removeArea( QString name )
{
    if( areaNamesMap.values().contains( name ) ) {
        return removeArea( areaNamesMap.key( name ) ); // i.e. call the removeArea(int) method
    }
    else {
        return false;
    }
}

void TRoomDB::removeArea( TArea * pA )
{
    if( ! pA ) {
        qWarning( "TRoomDB::removeArea(TArea *) Warning - attempt to remove an area with a NULL TArea pointer!" );
        return;
    }

    int areaId = areas.key(pA, 0);
    if( areaId == areas.key(pA, -1) ) {
        // By testing twice with different default keys to return if value NOT
        // found, we can be certain we have an actual valid value
        removeArea( areaId );
    }
    else {
        qWarning( "TRoomDB::removeArea(TArea *) Warning - attempt to remove an area NOT in TRoomDB::areas!" );
    }
}

int TRoomDB::getAreaID( TArea * pA )
{
    return areas.key(pA);
}

void TRoomDB::buildAreas()
{
    QElapsedTimer timer;
    timer.start();
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
    qDebug() << "TRoomDB::buildAreas() run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}


const QList<TRoom *> TRoomDB::getRoomPtrList()
{
    return rooms.values();
}

QList<int> TRoomDB::getRoomIDList()
{
    return rooms.keys();
}

// NOTE: This is MIS-NAMED and redundent as it returns the int ID of a given TArea
// pointer and there is the method getAreaID(TArea *) here as well that does it!
//int TRoomDB::getArea( TArea * pA )
//{
//    if( pA && areas.values().contains(pA) )
//    {
//        return areas.key(pA);
//    }
//    else
//        return -1;
//}

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

void TRoomDB::setAreaName( int areaID, QString name )
{
    areaNamesMap[areaID] = name;
}

bool TRoomDB::addArea( int id )
{
    if( !areas.contains( id ) )
    {
        areas[id] = new TArea( mpMap, this );
        return true;
    }
    else
    {
        QString error = QString("Area with ID=%1 already exists!").arg(id);
        mpMap->logError(error);
        return false;
    }
}

int TRoomDB::createNewAreaID()
{
    int _id = 1;
    for( ; ; _id++ )
    {
        if( !areas.contains(_id) )
        {
            return _id;
        }
    }
    return 0;
}

int TRoomDB::addArea( QString name )
{
    // area name already exists
    if( areaNamesMap.values().contains( name ) ) return 0;

    int areaID = createNewAreaID();
    if( addArea( areaID ) )
    {
        areaNamesMap[areaID] = name;
        return areaID;
    }
    else
        return 0; //fail
}

// this func is called by the xml map importer
// NOTE: we no longer accept duplicate IDs or duplicate area names
//       duplicate definitions are ignored
bool TRoomDB::addArea( int id, QString name )
{
    if( areaNamesMap.values().contains(name) ) return false;
    if( areaNamesMap.keys().contains(id) ) return false;
    bool ret = addArea( id );
    if( ret ) // Wrong check for error (==0 was being made)
    {
        areaNamesMap[id] = name;
        return true;
    }
    return false;

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
    QElapsedTimer timer;
    timer.start();
    // rooms konsolidieren
    QHashIterator<int, TRoom* > itRooms( rooms );
    while( itRooms.hasNext() )
    {
        itRooms.next();
        TRoom * pR = itRooms.value();
        pR->auditExits();

    }
    qDebug() << "TRoomDB::auditRooms() run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
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
    QElapsedTimer timer;
    timer.start();
    QList<TRoom*> rPtrL = getRoomPtrList();
    rooms.clear(); // Prevents any further use of TRoomDB::getRoom(int) !!!
    entranceMap.clear();
    areaNamesMap.clear();
    hashTable.clear();
    for( uint i=0; i<rPtrL.size(); i++ )
    {
        delete rPtrL.at(i); // Uses the internally held value of the room Id
                            // (TRoom::id) to call TRoomDB::__removeRoom(id)
    }
//    assert( rooms.size() == 0 ); // Pointless as rooms.clear() will have achieved the test condition

    QList<TArea*> areaList = getAreaPtrList();
    for( uint i=0; i<areaList.size(); i++ )
    {
        delete areaList.at(i);
    }
    assert( areas.size() == 0 );
    qDebug() << "TRoomDB::clearMapDB() run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}


void TRoomDB::restoreAreaMap( QDataStream & ifs )
{
    ifs >> areaNamesMap;
}

void TRoomDB::restoreSingleArea(QDataStream & ifs, int areaID, TArea * pA )
{
    areas[areaID] = pA;
}

void TRoomDB::restoreSingleRoom(QDataStream & ifs, int i, TRoom *pT)
{
    addRoom(i, pT, true);
}
