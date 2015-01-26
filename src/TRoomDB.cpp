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


// Define for testing TRoomDB::deleteArea(...) and TRoomDB::deleteRooms(...)!
#define DEBUG_TIMING

#include "TRoomDB.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"

#include "pre_guard.h"
#include <QApplication>
#include <QElapsedTimer>
#include "post_guard.h"


TRoomDB::TRoomDB( TMap * pMap )
: mpMap( pMap )
, mpDeletionRooms( 0 )
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
    qDebug( "TRoomDB::addRoom(%i) called.", id );
    if( id > 0 && ! rooms.contains( id ) ) {
        TRoom * pR = new TRoom( this );
        if( ! pR ) {
            QString error = qApp->translate( "TRoomDB", "Unable to create room id (%1) - out of memory?").arg(id);
            mpMap->logError(error);
            return false;
        }
        rooms[id] = pR;
        pR->setId(id);
        return true;
    }
    else {
        QString error;
        if( id <= 0 ) {
            error = qApp->translate( "TRoomDB", "Illegal room id (%1) - it must be greater than zero!").arg(id);
        }
        else {
            error = qApp->translate( "TRoomDB", "Duplicate room id (%1) - there is already a room with that number!").arg(id);
        }
        mpMap->logError(error);
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
    TRoom * pR = getRoom(id);
    if( pR ) {
        // FIXME: make a proper exit controller so we don't need to do all these if statements

        // Delete the Exits that lead TO THIS room, use the stored entrance data
        // so we can identify the rooms that have exits leading here - this is
        // the primary reason for maintaining Entrance data otherwise we'd have
        // to check EVERY exit for EVERY room! 8-) SlySven
        QSet<QPair<quint8, quint64> > _normalEntrances(pR->getNormalEntrances());
        _normalEntrances.detach();
        QSetIterator<QPair<quint8, quint64> > itNormalEntrances = _normalEntrances;
        while( itNormalEntrances.hasNext() ) {
            QPair<quint8, quint64> entranceFrom = itNormalEntrances.next();
            if(      entranceFrom.second == id
                || ( mpDeletionRooms && mpDeletionRooms->contains( entranceFrom.second ) ) ) {
                continue; // Ignore circular entrances/exits in THIS room
                          // or other rooms being deleted!!!
            }
            TRoom * pR_from = getRoom( entranceFrom.second );
            if( pR_from ) {
                pR_from->setExit( -1, entranceFrom.first );
                // This call WILL modify pR->getNormalEntrances() that we might
                // otherwise iterate over directly - this is a no-no for many Qt
                // iterators, even the mutable ones (they expect to be making
                // the changes!) - hence the explicit deep copying above caused
                // by making a copy and then using "detach()"
            }
        }
        QSet<QPair<QString,quint64> > _specialEntrances( pR->getSpecialEntrances() );
        _specialEntrances.detach();
        QSetIterator<QPair<QString,quint64> > itSpecialEntrances = _specialEntrances;
        while( itSpecialEntrances.hasNext() ) {
            QPair<QString, quint64> entranceFrom = itSpecialEntrances.next();
            if(      entranceFrom.second == id
                || ( mpDeletionRooms && mpDeletionRooms->contains( entranceFrom.second ) ) ) {
                continue; // Ignore circular entrances/exits in THIS room
                          // or other rooms being deleted!!!
            }
            TRoom * pR_from = getRoom( entranceFrom.second );
            if( pR_from ) {
                pR_from->setSpecialExit( -1, entranceFrom.first );
            }
        }
        // And EXITS from THIS room are entrances in other rooms so clear THIS
        // room's exits to erase those ENTRANCE details from OTHER rooms:
        QSet<QPair<quint8, quint64> > _normalExits(pR->getNormalExits());
        _normalEntrances.detach();
        QSetIterator<QPair<quint8, quint64> > itNormalExits = _normalExits;
        while( itNormalExits.hasNext() ) {
            QPair<quint8, quint64> exitTo = itNormalExits.next();
            if(       exitTo.second == id
                || ( mpDeletionRooms && mpDeletionRooms->contains( exitTo.second ) ) ) {
                continue; // Ignore circular entrances/exits in THIS room
                          // or other rooms being deleted!!!
            }
            pR->setExit( -1, exitTo.first );
        }
        QSet<QPair<QString, quint64> > _specialExits(pR->getSpecialExits());
        _specialExits.detach();
        QSetIterator<QPair<QString, quint64> > itSpecialExits = _specialExits;
        while( itSpecialExits.hasNext() ) {
            QPair<QString, quint64> exitTo = itSpecialExits.next();
            if(       exitTo.second == id
                || ( mpDeletionRooms && mpDeletionRooms->contains( exitTo.second ) ) ) {
                continue; // Ignore circular entrances/exits in THIS room
                          // or other rooms being deleted!!!
            }
            pR->setSpecialExit( -1, exitTo.first );
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
        if(pA) {
            pA->removeRoom(id);
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
    if( id > 0 && rooms.contains(id) ) {
        TRoom * pR = getRoom( id );
        delete pR;
        return true;
    }
    return false;
}

void TRoomDB::removeRooms( QList<int> & ids )
{
#if defined(DEBUG_TIMING)
    static bool isToShowTime = true; // Set to true within debugger to control the display of this
    QElapsedTimer timer;
    timer.start();
#endif
    mpDeletionRooms = new QSet<int>( ids.toSet() );
#if defined(DEBUG_TIMING)
    quint64 roomCount = mpDeletionRooms->size();
#endif
    QSetIterator<int> itRoom = *mpDeletionRooms;
    while( itRoom.hasNext() ) {
        removeRoom( itRoom.next() ); // This will update other area's rooms about exit changes
    }
    delete mpDeletionRooms;
    mpDeletionRooms = 0;
#if defined(DEBUG_TIMING)
    if( isToShowTime ) {
        qint64 elapsed = timer.elapsed();
        double elaspedTime = elapsed / 1000.0;
        double unitTime = 0.0;
        if(roomCount) {
            unitTime = elapsed / (double)(roomCount);
        }
        // Can't get the printf format string character right to present
        // meaningful data - so let's let Qt sort it out
        qDebug()<<"TRoomDB::removeRooms(...) - (" << roomCount << ") Rooms destruction took" << elaspedTime << "seconds in total. This averages to" << unitTime << "milli-seconds per room.";
    }
#endif
}

bool TRoomDB::removeArea( int id )
{
#if defined(DEBUG_TIMING)
    static bool isToShowTime = true; // Set to true within debugger to control the display of this
    QElapsedTimer timer;
    timer.start();
#endif
    areaNamesMap.remove( id );
    if( areas.contains( id ) ) {
        TArea * pA = areas[id];
        mpDeletionRooms = new QSet<int>( pA->rooms.toSet() );
#if defined(DEBUG_TIMING)
        quint64 roomCount = mpDeletionRooms->size();
#endif
        QSetIterator<int> itRoom = *mpDeletionRooms;
        while( itRoom.hasNext() ) {
            removeRoom( itRoom.next() ); // This will update other area's rooms about exit changes
        }
        delete mpDeletionRooms;
        mpDeletionRooms = 0;
        areas.remove( id );
        mpMap->mMapGraphNeedsUpdate = true;
#if defined(DEBUG_TIMING)
        if( isToShowTime ) {
            qint64 elapsed = timer.elapsed();
            double elaspedTime = elapsed / 1000.0;
            double unitTime = 0.0;
            if(roomCount) {
                unitTime = elapsed / (double)(roomCount);
            }
            // Can't get the printf format string character right to present
            // meaningful data - so let's let Qt sort it out
            qDebug()<<"TRoomDB::removeArea(" << id << ") - (" << roomCount << ") Rooms destruction took" << elaspedTime << "seconds in total. This averages to" << unitTime << "milli-seconds per room.";
        }
#endif
        return true;
    }
#if defined(DEBUG_TIMING)
        if( isToShowTime ) {
            qDebug( "TRoomDB::removeArea(%i) - No area found to destroy.", id );
        }
#endif
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
    QElapsedTimer _time;
    _time.start();
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
    qDebug( "TRoomDB::buildAreas(): run time: %i milli-Seconds.", _time.elapsed());
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
    QElapsedTimer t;
    t.start();
    bool ok = true;
    mpTempAllNormalEntrances = new QMultiHash<quint64, QPair<quint8, quint64> >;  // key is toRoomdId, value.first is direction code, value.second is fromRoomId
    mpTempAllSpecialEntrances = new QMultiHash<quint64, QPair<QString, quint64> >;// key is toRoomId, value.first is exit name, value.second is fromRoomId
    if( ( ! mpTempAllNormalEntrances ) || ( ! mpTempAllSpecialEntrances ) ) {
        qCritical( "TRoomDB::auditRooms() Critical error: unable to create temporary structures to validate map route data." );
        ok = false;
    }

    // rooms konsolidieren
    QHashIterator<int, TRoom * > itRooms( rooms );
    while( itRooms.hasNext() ) {
        itRooms.next();
        TRoom * pR = itRooms.value();
        pR->auditExits();
    }

    if( ok && mpMap->version < 17 ) {
        // Right now build entrance data
        itRooms.toFront();
        while( itRooms.hasNext() ) {
            itRooms.next();
            TRoom * pR = itRooms.value();
            QList<QPair<quint8, quint64> > normalRoomEntrancesList = mpTempAllNormalEntrances->values( pR->getId() );
            for( uint i = 0; i < normalRoomEntrancesList.size(); i++ ) {
                pR->setEntrance( normalRoomEntrancesList.at(i) );
            }
            QList<QPair<QString, quint64> > specialRoomEntrancesList = mpTempAllSpecialEntrances->values( pR->getId() );
            for( uint i = 0; i < specialRoomEntrancesList.size(); i++ ) {
                pR->setEntrance( specialRoomEntrancesList.at(i) );
            }
        }
    }

    if( ok ) {
        delete mpTempAllNormalEntrances;
        delete mpTempAllSpecialEntrances;
    }
    qDebug( "TRoomDB::auditRooms() audit Rooms took: %i milli-Seconds.", t.elapsed() );
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
    ifs >> areaNamesMap;
}

void TRoomDB::restoreSingleArea(QDataStream & ifs, int areaID, TArea * pA )
{
    areas[areaID] = pA;
}

void TRoomDB::restoreSingleRoom(QDataStream & ifs, TRoom * pR )
{
    Q_UNUSED(ifs);

    if( ! pR ) {
        return;
    }
    rooms[ pR->getId() ] = pR;
}
