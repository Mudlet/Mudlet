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


#include "TArea.h"


#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include "post_guard.h"

// Previous direction #defines here did not match the DIR_ defines in TRoom.h,
// but as they are stored in the map file they ought not to be redefined without
// a map file change - however the data that was stored - though wrong was not
// actually used internally so we will just fix it and carry on!  We will use
// the (supplimented by the addition of a code for OTHER) DIR_**** values set
// in the top of TRoom.h.
// FIXME: Modify mapper "painter" code to use "exits" rather than deriving the
// same information each time it is run ???

TArea::TArea(TMap * map , TRoomDB * pRDB )
: min_x(0)
, min_y(0)
, min_z(0)
, max_x(0)
, max_y(0)
, max_z(0)
, gridMode( false )
, isZone( false )
, zoneAreaRef( 0 )
, mpRoomDB( pRDB )
, mIsDirty( false )
{
}

TArea::~TArea()
{
    if( mpRoomDB )
        mpRoomDB->removeArea( (TArea*)this );
    else
        qDebug()<<"ERROR: In TArea::~TArea(), instance has no mpRoomDB";
}

int TArea::getAreaID()
{
    if( mpRoomDB )
        return mpRoomDB->getAreaID( this );
    else
    {
        qDebug()<<"ERROR: TArea::getAreaID() instance has no mpRoomDB, returning -1 as ID";
        return -1;
    }
}

QMap<int,QMap<int,QMultiMap<int,int> > > TArea::koordinatenSystem()
{
    QMap<int,QMap<int,QMultiMap<int,int> > > kS;
    QList<TRoom*> roomList = mpRoomDB->getRoomPtrList();
    for( int i=0; i<roomList.size(); i++ )
    {
        TRoom * pR = roomList[i];
        int id = pR->getId();
        int x = pR->x;
        int y = pR->y;
        int z = pR->z;
        QMap<int,QMultiMap<int,int> > _y;
        QMultiMap<int,int> _z;
        if( ! kS.contains( x ) )
        {
            kS[x] = _y;
        }
        if( ! kS[x].contains( y ) )
        {
            kS[x][y] = _z;
        }
        kS[x][y].insertMulti( z, id );
    }
    //qDebug()<< "kS="<<kS;
    return kS;
}

QList<int> TArea::getRoomsByPosition( int x, int y, int z )
{
    QList<int> dL;
    for( int i=0; i<rooms.size(); i++ )
    {
        TRoom * pR = mpRoomDB->getRoom(rooms[i]);
        if( pR )
        {
            int id = pR->getId();
            int _x = pR->x;
            int _y = pR->y;
            int _z = pR->z;
            if( _x == x && _y == y && _z == z )
            {
                dL.push_back( id );
            }
        }
    }
    return dL;
}

QList<int> TArea::getCollisionNodes()
{
    QList<int> problems;
    QMap<int,QMap<int,QMultiMap<int,int> > > kS = koordinatenSystem();
    QMapIterator<int,QMap<int,QMultiMap<int,int> > > it(kS);
    while (it.hasNext())
    {
        it.next();
        QMap<int,QMultiMap<int,int> > x_val = it.value();
        QMapIterator<int,QMultiMap<int,int> > it2(x_val);
        while (it2.hasNext())
        {
            it2.next();
            QMultiMap<int,int> y_val = it2.value();
            QMapIterator<int,int> it3(y_val);
            QList<int> z_coordinates;
            while (it3.hasNext())
            {
                it3.next();
                int z = it3.key();
                int node = it3.value();

                if( ! z_coordinates.contains( node ) )
                    z_coordinates.append( node );
                else
                {
                    if( ! problems.contains( node ) )
                    {
                        QMultiMap<int, int>::iterator it4 = y_val.find(z);
                        problems.append( it4.value() );
                        //qDebug()<<"problem node="<<node;
                    }
                }
            }
        }
    }
    return problems;
}

void TArea::determineAreaExitsOfRoom( int id )
{
    if( ! mpRoomDB ) {
        return;
    }

    TRoom * pR = mpRoomDB->getRoom(id);
    if( ! pR ) {
        return;
    }

    exits.remove(id);
    QSetIterator<QPair<quint8, quint64> > itNormalExitSet = pR->getNormalExits();
    while( itNormalExitSet.hasNext() ) {
        QPair<quint8, quint64> _exit = itNormalExitSet.next();
        if( rooms.indexOf( _exit.second ) < 0 ) {
            exits.insertMulti( id, qMakePair( _exit.second, _exit.first ) );
        }
    }

    QSetIterator<QPair<QString, quint64> > itSpecialExitSet = pR->getSpecialExits();
    while( itSpecialExitSet.hasNext() ) {
        QPair<QString, quint64> _exit = itSpecialExitSet.next();
        if( rooms.indexOf( _exit.second ) < 0 ) {
            exits.insertMulti( id, qMakePair( _exit.second, DIR_OTHER ) );
        }
    }
}

void TArea::determineAreaExits()
{
    QElapsedTimer _time; // Needed to time for wait cursor
    bool isLongTime = false;
    _time.start();
    exits.clear();
    for( int i=0; i<rooms.size(); i++ ) {
        TRoom * pR = mpRoomDB->getRoom(rooms.at(i));
        if( ! pR ) {
            continue;
        }

        QSetIterator<QPair<quint8, quint64> > itNormalExitSet = pR->getNormalExits();
        while( itNormalExitSet.hasNext() ) {
            if( (! isLongTime ) && _time.hasExpired(100) ) {
                isLongTime = true;
                qApp->setOverrideCursor( Qt::WaitCursor );
            }
            QPair<quint8, quint64> _exit = itNormalExitSet.next();
            if( rooms.indexOf( _exit.second ) < 0 ) {
                exits.insertMulti( rooms.at(i), qMakePair( _exit.second, _exit.first ) );
            }
        }

        QSetIterator<QPair<QString, quint64> > itSpecialExitSet = pR->getSpecialExits();
        while( itSpecialExitSet.hasNext() ) {
            if( (! isLongTime ) && _time.hasExpired(100) ) {
                isLongTime = true;
                qApp->setOverrideCursor( Qt::WaitCursor );
            }
            QPair<QString, quint64> _exit = itSpecialExitSet.next();
            if( rooms.indexOf( _exit.second ) < 0 ) {
                exits.insertMulti( rooms.at(i), qMakePair( _exit.second, DIR_OTHER ) );
            }
        }
    }
    if( isLongTime ) {
        qApp->restoreOverrideCursor();
        isLongTime = false;
    }
#if defined(DEBUG_TIMING)
    qDebug( "TArea::determineAreaExits() Area Id: %i, run time (%i rooms): %i milli-Seconds.", mpRoomDB->getAreaID(this), rooms.size(), _time.elapsed() );
#endif
}

void TArea::fast_calcSpan( int id )
{

    TRoom * pR = mpRoomDB->getRoom(id);
    if( !pR ) return;

    int x = pR->x;
    int y = pR->y;
    int z = pR->z;
    if( x > max_x ) max_x = x;
    if( x < min_x ) min_x = x;
    if( y > max_y ) max_y = y;
    if( y < min_y ) min_y = y;
    if( z > max_z ) max_z = z;
    if( z < min_z ) min_z = z;
}

void TArea::addRoom( int id )
{
    TRoom * pR = mpRoomDB->getRoom( id );
    if( pR )
    {
        if( !rooms.contains( id ) )
        {
            rooms.append( id );
        }
        else
        {
            qDebug()<<"TArea::addRoom("<<id<<") No creation! room already exists";
        }
    }
    else
    {
        QString error = QString("roomID=%1 does not exist, can't set properties of non-existent rooms").arg(id);
    }
}

void TArea::calcSpan()
{
    QElapsedTimer _time; // Needed to time for wait cursor
    bool isLongTime = false;
    _time.start();

    xminEbene.clear();
    yminEbene.clear();
    zminEbene.clear();
    xmaxEbene.clear();
    ymaxEbene.clear();
    zmaxEbene.clear();
    if( rooms.size() > 0 )
    {
        int id = rooms[0];
        TRoom * pR = mpRoomDB->getRoom( id );
        if( !pR ) return;
        min_x = pR->x;
        max_x = min_x;
        min_y = pR->y*-1;
        max_y = min_y;
        min_z = pR->z;
        max_z = min_z;
    }

    for( int i=0; i<rooms.size(); i++ )
    {
        if( (! isLongTime ) && _time.hasExpired(100) ) {
            isLongTime = true;
            qApp->setOverrideCursor( Qt::WaitCursor );
        }
        int id = rooms[i];
        TRoom * pR = mpRoomDB->getRoom( id );
        if( !pR ) continue;
        int _m_xmin = pR->x;
        if( _m_xmin < min_x )
            min_x = _m_xmin;
        int _m_ymin = pR->y*-1;
        if( _m_ymin < min_y )
            min_y = _m_ymin;
        int _m_zmin = pR->z;
        if( _m_zmin < min_z )
        {
            min_z = _m_zmin;
            if( ! ebenen.contains( _m_zmin ) )
            {
                ebenen.push_back( _m_zmin );
            }
        }
        int _m_xmax = pR->x;
        if( _m_xmax > max_x )
            max_x = _m_xmax;
        int _m_ymax = pR->y*-1;
        if( _m_ymax > max_y )
            max_y = _m_ymax;
        int _m_zmax = pR->z;
        if( _m_zmax > max_z )
        {
            max_z = _m_zmax;
        }
        // ebenenliste anlegen
        if( ! ebenen.contains( _m_zmax ) )
        {
            ebenen.push_back( _m_zmax );
        }
    }

    for( int k=0; k<ebenen.size(); k++ )
    {
        if( (! isLongTime ) && _time.hasExpired(100) ) {
            isLongTime = true;
            qApp->setOverrideCursor( Qt::WaitCursor );
        }
        // For each of the (used) z-axis values that has been put into the list "ebenen"
        int _min_x;
        int _min_y;
        int _min_z;
        int _max_x;
        int _max_y;
        int _max_z;
        bool minAndMaxsInitialized = false;

        if( rooms.size() > 0 )
        {
            int id = rooms[0];
            TRoom * pR = mpRoomDB->getRoom( id );
            if( !pR )
                continue;
            _min_x = pR->x;
            _max_x = _min_x;
            _min_y = pR->y*-1;
            _max_y = _min_y;
            _min_z = pR->z;
            _max_z = _min_z;
            minAndMaxsInitialized = true;
        }

        for( int i=0; i<rooms.size(); i++ )
        {
            int id = rooms[i];
            TRoom * pR = mpRoomDB->getRoom( id );
            if( !pR )
                continue;   // Not a valid room so ignore
            if( pR->z != ebenen[k])
                continue;   // Room is not on the z-axis value level that we currently are working with
            if( ! minAndMaxsInitialized )
            {  // Will get here if FIRST room (in rooms[]) was not a valid one
                _min_x = pR->x;
                _max_x = _min_x;
                _min_y = pR->y*-1;
                _max_y = _min_y;
                _min_z = pR->z;
                _max_z = _min_z;
                minAndMaxsInitialized = true;
            }
            else
            {
                int _m = pR->x;
                if( _m < _min_x )
                    _min_x = _m;
                if( _m > _max_x )
                    _max_x = _m;

                _m = pR->y*-1;
                if( _m < _min_y )
                   _min_y = _m;
                if( _m > _max_y )
                   _max_y = _m;

                // This bit is pointless, we already known that pR->z will be the
                // fixed z-axis value that ebenen[k] holds so _min_z and _max_z will
                // also have that value.
                _m = pR->z;
                if( _m < _min_z )
                   _min_z = _m;
                if( _m > _max_z )
                   _max_z = _m;
            }
        }
        if( minAndMaxsInitialized )
        {
            xminEbene[ebenen[k]] = _min_x;
            yminEbene[ebenen[k]] = _min_y;
            zminEbene[ebenen[k]] = _min_z;
            xmaxEbene[ebenen[k]] = _max_x;
            ymaxEbene[ebenen[k]] = _max_y;
            zmaxEbene[ebenen[k]] = _max_z;
        }
    }
    if( isLongTime ) {
        qApp->restoreOverrideCursor();
        isLongTime = false;
    }
#if defined(DEBUG_TIMING)
    qDebug( "TArea::calcSpan() Area Id: %i, run time (%i rooms): %i milli-Seconds.", mpRoomDB->getAreaID(this), rooms.size(), _time.elapsed() );
#endif
}

void TArea::removeRoom( int room )
{
//    QTime time;
//    time.start();
//    TRoom * pR = mpRoomDB->getRoom( room );
    rooms.removeOne( room );
    exits.remove( room );
//    qDebug()<<"Area removal took"<<time.elapsed(); // Is room not Area!
//// FIXME: The following recalculations are probably required despite being
//// commented out by another coder! - SlySven
//    int x = pR->x;
//    int y = pR->y*-1;
//    int z = pR->z;
//    int xmin=x;int ymin=y;int zmin=z;
//    int xmax=x;int ymax=y;int zmax=z;
//    if ( xminEbene.contains(z) )
//        xmin = xminEbene[z];
//    if ( yminEbene.contains(z))
//        ymin = yminEbene[z];
//    if ( zminEbene.contains(z))
//        zmin = zminEbene[z];
//    if ( xmaxEbene.contains(z))
//        xmax = xmaxEbene[z];
//    if ( ymaxEbene.contains(z))
//        ymax = ymaxEbene[z];
//    if ( zmaxEbene.contains(z))
//        zmax = zmaxEbene[z];
//    if( xmin == x || xmax == x || ymax == y ||
//        ymin == y || zmin == z || zmax == z)
//        calcSpan();
}

// Reconstruct the area exit data in a format that actually makes sense - only
// needed until the TRoom & TArea classes can be restructured to store exits
// using the exit direction as a key and the to room as a value instead of vice-versa
const QMultiMap<int, QPair<QString, int> > TArea::getAreaExitRoomData() const
{
    QMultiMap<int, QPair<QString, int> > results;
    QSet<int> roomsWithOtherAreaSpecialExits;

    QMapIterator<int, QPair<int, int> > itAreaExit = exits;
    // First parse the normal exits and also find the rooms where there is at
    // least one special area exit
    while( itAreaExit.hasNext() ) {
        itAreaExit.next();
        QPair<QString, int> exitData;
        exitData.second = itAreaExit.value().first;
        switch( itAreaExit.value().second ) {
            case DIR_NORTH:     exitData.first = QString("north");         break;
            case DIR_NORTHEAST: exitData.first = QString("northeast");     break;
            case DIR_NORTHWEST: exitData.first = QString("northwest");     break;
            case DIR_SOUTH:     exitData.first = QString("south");         break;
            case DIR_WEST:      exitData.first = QString("west");          break;
            case DIR_EAST:      exitData.first = QString("east");          break;
            case DIR_SOUTHEAST: exitData.first = QString("southeast");     break;
            case DIR_SOUTHWEST: exitData.first = QString("southwest");     break;
            case DIR_UP:        exitData.first = QString("up");            break;
            case DIR_DOWN:      exitData.first = QString("down");          break;
            case DIR_IN:        exitData.first = QString("in");            break;
            case DIR_OUT:       exitData.first = QString("out");           break;
            case DIR_OTHER:     roomsWithOtherAreaSpecialExits.insert(itAreaExit.key());   break;
            default:
                qWarning("TArea::getAreaExitRoomData() Warning: unrecognised exit code %1 found for exit from room %2 to room %3.",
                         itAreaExit.value().second, itAreaExit.key(), itAreaExit.value().first );
        }
        if( ! exitData.first.isEmpty() ) {
            results.insert( itAreaExit.key(), exitData );
        }
    }
    // Now have to find the special area exits in the rooms where we know there
    // IS one
    QSetIterator<int> itRoomWithOtherAreaSpecialExit = roomsWithOtherAreaSpecialExits;
    while( itRoomWithOtherAreaSpecialExit.hasNext() ) {
        int fromRoomId = itRoomWithOtherAreaSpecialExit.next();
        TRoom * pFromRoom = mpRoomDB->getRoom( fromRoomId );
        if( pFromRoom ) {
            QMapIterator<int, QString> itOtherExit = pFromRoom->getOtherMap();
            while( itOtherExit.hasNext() ) {
                itOtherExit.next();
                QPair<QString, int> exitData;
                exitData.second = itOtherExit.key();
                TRoom * pToRoom = mpRoomDB->getRoom( exitData.second );
                if( pToRoom && mpRoomDB->getArea( pToRoom->getArea() ) != this ) {
                    // Note that pToRoom->getArea() is misnamed, should be getAreaId() !
                    if( itOtherExit.value().mid(0,1) == QStringLiteral("0") || itOtherExit.value().mid(0,1) == QStringLiteral("1") ) {
                        exitData.first = itOtherExit.value().mid(1);
                    }
                    else {
                        exitData.first = itOtherExit.value();
                    }
                    if( ! exitData.first.isEmpty() ) {
                        results.insert( fromRoomId, exitData );
                    }
                }
            }
        }
    }
    return results;
}
