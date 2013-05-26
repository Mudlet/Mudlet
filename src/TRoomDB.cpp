/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


TRoomDB::TRoomDB( TMap * pMap )
: mpMap( pMap )
{
}

TRoom * TRoomDB::getRoom( int id )
{
    if( rooms.contains( id ) && id > 0 )
    {
        return rooms[id];
    }
    else
    {
        return 0;
    }
}

bool TRoomDB::addRoom( int id )
{
    qDebug()<<"addRoom("<<id<<")";
    if( !rooms.contains( id ) && id > 0 )
    {
        rooms[id] = new TRoom( this );
        rooms[id]->setId(id);
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
    if( rooms.contains(id ) && id > 0 )
    {

        TRoom * pR = getRoom( id );
        if( !pR ) return false;
        QMapIterator<int, TRoom *> it( rooms );
        while( it.hasNext() )
        {
            it.next();
            TRoom * r = it.value();
            if( r->getNorth() == id ) r->setNorth(-1);
            if( r->getNortheast() == id ) r->setNortheast(-1);
            if( r->getNorthwest() == id ) r->setNorthwest(-1);
            if( r->getEast() == id ) r->setEast(-1);
            if( r->getWest() == id ) r->setWest(-1);
            if( r->getSouth() == id ) r->setSouth(-1);
            if( r->getSoutheast() == id ) r->setSoutheast(-1);
            if( r->getSouthwest() == id ) r->setSouthwest(-1);
            if( r->getUp() == id ) r->setUp(-1);
            if( r->getDown() == id ) r->setDown(-1);
            if( r->getIn() == id ) r->setIn(-1);
            if( r->getOut() == id ) r->setOut(-1);
            r->removeAllSpecialExitsToRoom( id );
        }
        int areaID = pR->getArea();
        TArea * pA = getArea( areaID );
        if( !pA ) return false;
        pA->rooms.removeAll( id );
        pA->exits.remove( id ); //note: this removes *all* keys=id
        mpMap->mMapGraphNeedsUpdate = true;
    }
    QList<QString> keyList = hashTable.keys();
    QList<int> valueList = hashTable.values();
    for( int i=0; i<valueList.size(); i++ )
    {
        if( valueList[i] == id )
        {
            hashTable.remove( keyList[i] );
        }
    }
    return true;
}


// called by TRoom destructor
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

bool TRoomDB::removeArea( int id )
{
    areaNamesMap.remove( id );
    if( areas.contains( id ) )
    {
        TArea * pA = areas[id];
        QList<int> rl;
        for( int i=0; i< pA->rooms.size(); i++ )
        {
            rl.push_back( pA->rooms[i] );
        }
        for( int i=0; i<rl.size(); i++ )
        {
            removeRoom( rl[i] );
        }
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
    QMapIterator<int, TRoom *> it( rooms );
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

int TRoomDB::getArea( TArea * pA )
{
    if( pA && areas.values().contains(pA) )
    {
        return areas.key(pA);
    }
    else
        return -1;
}

TArea * TRoomDB::getArea( int id )
{
    if( areas.contains( id ) && id > 0 )
    {
        return areas[id];
    }
    else
    {
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
        QString error = QString("can't create area with ID=%1").arg(id);
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
    return -1;
}

int TRoomDB::addArea( QString name )
{
    // area name already exists
    if( areaNamesMap.values().contains( name ) ) return -1;

    int areaID = createNewAreaID();
    if( addArea( areaID ) )
    {
        areaNamesMap[areaID] = name;
        return areaID;
    }
    else
        return -1; //fail
}

// this func is called by the xml map importer
// NOTE: we no longer accept duplicate IDs or duplicate area names
//       duplicate definitions are ignored
bool TRoomDB::addArea( int id, QString name )
{
    if( areaNamesMap.values().contains(name) ) return false;
    if( areaNamesMap.keys().contains(id) ) return false;
    if( addArea( id ) )
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
    QTime t; t.start();
    // rooms konsolidieren
    QMapIterator<int, TRoom* > itRooms( rooms );
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
    QMapIterator<int, TRoom *> it( rooms );
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
        pA->ausgaengeBestimmen();
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
    ifs >> areaNamesMap;
}

void TRoomDB::restoreSingleArea(QDataStream & ifs, int areaID, TArea * pA )
{
    areas[areaID] = pA;
}

void TRoomDB::restoreSingleRoom(QDataStream & ifs, int i, TRoom *pT)
{
    rooms[i] = pT;
}

