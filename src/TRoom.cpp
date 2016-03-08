/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TRoom.h"


#include "TArea.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QStringBuilder>
#include <QElapsedTimer>
#include "post_guard.h"


TRoom::TRoom(TRoomDB * pRDB)
: x( 0 )
, y( 0 )
, z( 0 )
, environment( -1 )
, isLocked( false )
, min_x( 0 )
, min_y( 0 )
, max_x( 0 )
, max_y( 0 )
, c( 0 )
, highlight( false )
, highlightColor( QColor( 255,150,0 ) )
, rendered(false)
, id( 0 )
, area( -1 )
, weight(1)
, north( -1 )
, northeast( -1 )
, east( -1 )
, southeast( -1 )
, south( -1 )
, southwest( -1 )
, west( -1 )
, northwest( -1 )
, up( -1 )
, down( -1 )
, in( -1 )
, out( -1 )
, mpRoomDB( pRDB )
{
}

TRoom::~TRoom()
{
    static double cumulativeMean = 0.0;
    static quint64 runCount = 0 ;
    QElapsedTimer timer;
    timer.start();
    mpRoomDB->__removeRoom( id );
    quint64 thisTime = timer.nsecsElapsed();
    cumulativeMean += ( ( (thisTime * 1.0e-9) - cumulativeMean) / ++runCount );
    if( runCount % 1000 == 0 ) {
        qDebug()<<"TRoom::~TRoom() took" << thisTime * 1.0e-9 << "Sec this time and after" << runCount <<"times the average is" << cumulativeMean << "Sec.";
    }
}

bool TRoom::hasExitStub(int direction)
{
    if (exitStubs.contains(direction))
        return true;
    else
        return false;
}

void TRoom::setExitStub(int direction, bool status)
{
    if(status)
    {
        if( ! hasExit(direction) )
        {
            if( ! exitStubs.contains(direction) )
            {
                // Previous code did not check for an existing entry for given
                // direction and QList container permits duplicates of same value!
                exitStubs.append(direction);
            }
        }
        else
        {
            QString error = QString("Set exit stub in given direction in RoomID(%1) - there is already an exit there!").arg( id );
            mpRoomDB->mpMap->logError(error);
        }
    }
    else
        exitStubs.removeAll(direction);
}

int TRoom::getExitWeight(const QString& cmd )
{
    if( exitWeights.contains( cmd ) )
    {
        return exitWeights[cmd];
    }
    else
        return weight; // NOTE: if no exit weight has been set: exit weight = room weight
}

// NOTE: needed so dialogRoomExit code can tell if an exit weight has been set
// now that they are private!
bool TRoom::hasExitWeight(const QString& cmd )
{
    if( exitWeights.contains( cmd ) )
    {
        if( exitWeights.value( cmd ) > 0 )
            return true;
        else
            return false;
    }
    else
        return false;
}

void TRoom::setWeight( int w )
{
    if( w < 1 ) w = 1;
    weight = w;
}

// Previous implimentations did not allow for REMOVAL of an exit weight (by
// setting it to zero)
void TRoom::setExitWeight(const QString& cmd, int w )
{
    if( w > 0 )
    {
        exitWeights[ cmd ] = w;
        if( mpRoomDB && mpRoomDB->mpMap )
            mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
    }
    else if( exitWeights.contains( cmd ) )
    {
        exitWeights.remove( cmd );
        if( mpRoomDB && mpRoomDB->mpMap )
            mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
    }
}

// Declared in header but was missing!
// Uses lower case initials: n,ne,e,se,s,sw,w,nw
//
// also: up, down, in, out or any unprefixed special exit command
// all of which can be stored but aren't (yet?) showable on the 2D mapper
void TRoom::setDoor(const QString& cmd, int doorStatus)
{
    if( doorStatus > 0 && doorStatus <=3 )
        doors[cmd] = doorStatus;
    else if( doors.contains( cmd ) && doorStatus == 0 )
        doors.remove( cmd );
}

int TRoom::getDoor(const QString& cmd )
{
    return doors.value( cmd, 0 );
    // Second argument is the result if cmd is not in the doors QMap
}

void TRoom::setId( int _id )
{
    id = _id;
}

// The second optional argument delays area related recaluclations when true
// until called with false (the default) - it records the "dirty" areas so that
// the affected areas can be identified.
// The caller, should set the argument true for all but the last when working
// through a list of rooms.
// There IS a theoretical risk that if the last called room "doesn't exist" then
// the area related recalculations won't get done - so had better provide an
// alternative means to do them as a fault recovery
bool TRoom::setArea( int areaID, bool isToDeferAreaRelatedRecalculations )
{
    static QSet<TArea *> dirtyAreas;
    TArea * pA = mpRoomDB->getArea( areaID );
    if( ! pA ) { // There is no TArea instance with that _areaID
        mpRoomDB->addArea( areaID ); // So try and make it
        pA = mpRoomDB->getArea( areaID );
        if( ! pA ) { // Oh, dear THAT didn't work
            QString error = qApp->translate( "TRoom", "No area created!  Requested area ID=%1. Note: Area IDs must be > 0" ).arg( areaID );
            mpRoomDB->mpMap->logError(error);
            return false;
        }
    }

    //remove from the old area
    TArea * pA2 = mpRoomDB->getArea( area );
    if( pA2 ) {
        pA2->removeRoom( id, isToDeferAreaRelatedRecalculations );
        // Ah, all rooms in the OLD area that led to the room now become area
        // exits for that OLD area {so must run determineAreaExits() for the
        // old area after the room has moved to the new area see other
        // "if( pA2 )" below} - other exits that led to the room from other
        // areas are still "out of area exits" UNLESS the room moves to the SAME
        // area that the other exits are in.
        dirtyAreas.insert( pA2 ); // Add to local store of dirty areas
        pA2->mIsDirty = true; // Flag the area itself in case soemthing goes
                              // wrong on last room in a series
    }
    else {
        QString error = qApp->translate( "TRoom", "Warning: When setting the Area for Room (Id: %1) it did not have a current area!").arg( id );
        mpRoomDB->mpMap->logError( error );
    }

    area = areaID;
    pA->addRoom( id );

    dirtyAreas.insert( pA );
    pA->mIsDirty = true;

    if( ! isToDeferAreaRelatedRecalculations ) {
        QSetIterator<TArea *> itpArea = dirtyAreas;
        while( itpArea.hasNext() ) {
            TArea * pArea = itpArea.next();
            pArea->calcSpan();
            pArea->determineAreaExits();
            pArea->mIsDirty = false;
        }
        dirtyAreas.clear();
    }

    return true;
}

bool TRoom::setExit( int to, int direction )
{
    // FIXME: This along with TRoom->setExit need to be unified to a controller.
    switch(direction){
    case DIR_NORTH:     north     = to; break;
    case DIR_NORTHEAST: northeast = to; break;
    case DIR_NORTHWEST: northwest = to; break;
    case DIR_EAST:      east      = to; break;
    case DIR_WEST:      west      = to; break;
    case DIR_SOUTH:     south     = to; break;
    case DIR_SOUTHEAST: southeast = to; break;
    case DIR_SOUTHWEST: southwest = to; break;
    case DIR_UP:        up        = to; break;
    case DIR_DOWN:      down      = to; break;
    case DIR_IN:        in        = to; break;
    case DIR_OUT:       out       = to; break;
    default: return false;
    }
    mpRoomDB->updateEntranceMap(this);
    return true;
}

bool TRoom::hasExit( int direction )
{
    switch(direction){
    case DIR_NORTH:     if(north     != -1) return true; break;
    case DIR_NORTHEAST: if(northeast != -1) return true; break;
    case DIR_NORTHWEST: if(northwest != -1) return true; break;
    case DIR_EAST:      if(east      != -1) return true; break;
    case DIR_WEST:      if(west      != -1) return true; break;
    case DIR_SOUTH:     if(south     != -1) return true; break;
    case DIR_SOUTHEAST: if(southeast != -1) return true; break;
    case DIR_SOUTHWEST: if(southwest != -1) return true; break;
    case DIR_UP:        if(up        != -1) return true; break;
    case DIR_DOWN:      if(down      != -1) return true; break;
    case DIR_IN:        if(in        != -1) return true; break;
    case DIR_OUT:       if(out       != -1) return true;
    }
    return false;
}

int TRoom::getExit( int direction )
{
    switch(direction){
    case DIR_NORTH:     return north    ; break;
    case DIR_NORTHEAST: return northeast; break;
    case DIR_NORTHWEST: return northwest; break;
    case DIR_EAST:      return east     ; break;
    case DIR_WEST:      return west     ; break;
    case DIR_SOUTH:     return south    ; break;
    case DIR_SOUTHEAST: return southeast; break;
    case DIR_SOUTHWEST: return southwest; break;
    case DIR_UP:        return up       ; break;
    case DIR_DOWN:      return down     ; break;
    case DIR_IN:        return in       ; break;
    case DIR_OUT:       return out      ;
    }
    return -1;
}

QHash<int, int> TRoom::getExits()
{
    // key is room id we exit to, value is type of exit. 0 is normal, 1 is special
    QHash<int, int> exitList;
    if (north != -1)
        exitList[north] = 0;
    if (northeast != -1)
        exitList[northeast] = 0;
    if (east != -1)
        exitList[east] = 0;
    if (southeast != -1)
        exitList[southeast] = 0;
    if (south != -1)
        exitList[south] = 0;
    if (southwest != -1)
        exitList[southwest] = 0;
    if (west != -1)
        exitList[west] = 0;
    if (northwest != -1)
        exitList[northwest] = 0;
    if (up != -1)
        exitList[up] = 0;
    if (down != -1)
        exitList[down] = 0;
    if (in != -1)
        exitList[in] = 0;
    if (out != -1)
        exitList[out] = 0;
    QMapIterator<int, QString> it(other);
    while (it.hasNext()) {
        it.next();
        exitList[it.key()] = 1;
    }
    return exitList;
}

void TRoom::setExitLock( int exit, bool state )
{
    if( ! state )
    {
        exitLocks.removeAll( exit );
        return;
    }
    switch( exit )
    {
        case DIR_NORTH: exitLocks.push_back(DIR_NORTH); break;
        case DIR_NORTHEAST: exitLocks.push_back(DIR_NORTHEAST); break;
        case DIR_NORTHWEST: exitLocks.push_back(DIR_NORTHWEST); break;
        case DIR_SOUTHEAST: exitLocks.push_back(DIR_SOUTHEAST); break;
        case DIR_SOUTHWEST: exitLocks.push_back(DIR_SOUTHWEST); break;
        case DIR_SOUTH: exitLocks.push_back(DIR_SOUTH); break;
        case DIR_EAST: exitLocks.push_back(DIR_EAST); break;
        case DIR_WEST: exitLocks.push_back(DIR_WEST); break;
        case DIR_UP: exitLocks.push_back(DIR_UP); break;
        case DIR_DOWN: exitLocks.push_back(DIR_DOWN); break;
        case DIR_IN: exitLocks.push_back(DIR_IN); break;
        case DIR_OUT: exitLocks.push_back(DIR_OUT); break;
    }
}

// The need for "to" seems superflous here, cmd is the decisive factor
void TRoom::setSpecialExitLock(int to, const QString& cmd, bool doLock)
{
    QMapIterator<int, QString> it( other );
    while( it.hasNext() )
    {
        it.next();
        if( it.key() != to )
            continue;
        if( it.value().size() < 1 )
            continue;
        if( it.value().mid(1) != cmd )
        {
            if( it.value() != cmd )
            {
                continue;
            }
        }
        if( doLock )
        {
            QString _cmd = it.value();
            _cmd.replace( 0, 1, '1' );
            other.replace( to, _cmd );
        }
        else
        {
            QString _cmd = it.value();
            _cmd.replace( 0, 1, '0');
            other.replace( to, _cmd );
        }
        return;
    }
}

bool TRoom::setSpecialExitLock(const QString& cmd, bool doLock)
{
    QMutableMapIterator<int, QString> it( other );
    while( it.hasNext() )
    {
        it.next();

        if( ! it.value().size() )
            continue;

        if( it.value().mid(1) != cmd )
        { // This value doesn't match, just check the old (obsolete) form without a lock state prefix
            if( it.value() != cmd )
            { // No match with or WITHOUT lock prefix, so move on to next value
                continue;
            }
            else
            {  // Got a match WITHOUT a '0'|'1' prefix (used now to encode lock state) so add it on
                QString _cmd = it.value();
                if( doLock )
                    _cmd.prepend( '1' );
                else
                    _cmd.prepend( '0' );
                it.setValue( _cmd ); // We can change the value as we are using the Mutable iterator...
                return true;
            }
        }
        else
        { // Found it!
            QString _cmd = it.value();
            if( doLock )
                _cmd.replace( 0, 1, '1' );
            else
                _cmd.replace( 0, 1, '0');
            it.setValue( _cmd );
            return true;
        }
    }
    return false;
}

bool TRoom::hasExitLock( int exit )
{
    return exitLocks.contains(exit);
}

// 0=offen 1=zu
bool TRoom::hasSpecialExitLock( int to, const QString& cmd )
{
    if( other.contains( to ) )
    {
        QMapIterator<int, QString> it( other );
        while( it.hasNext() )
        {
            it.next();
            if( it.key() != to )
                continue;
            if( it.value().size() < 2 )
                continue;
            return it.value().mid(0,1) == "1";
        }
        return false;
    }
    else
        return false;
}

// Original addSpecialExit...() code had limitation that it used the "to" room
// as part of the things to look for to identify a particular special exit
// indeed the use of the "to" room as the key for the "other" exit map does seem
// a poorer choice than the "command" which is currently the value item...
// FIXME: swap key/value items in (TRoom *)->other<int, QString> map?
// Changing to setSpecialExit(), "to" values less than 1 remove exit...
void TRoom::setSpecialExit( int to, const QString& cmd )
{
    QString _strippedCmd;
    QString _prefix= "";

    if( cmd.startsWith('0') || cmd.startsWith('1') )
    {
        _strippedCmd = cmd.mid(1);
        _prefix = cmd.mid(0,1);
    }
    else
    {
        _strippedCmd = cmd;
    }

    if( _strippedCmd.isEmpty() )
        return; // Refuse to create an unnamed special exit!!!
    // replace if this special exit exists, otherwise add
    QMutableMapIterator<int, QString> it( other );
    while(it.hasNext() )
    {
        it.next();
        if( ! it.value().size() )
            continue;

        if( Q_LIKELY( it.value().startsWith('0') || it.value().startsWith('1') ) )
        {
            if( it.value().mid(1) != _strippedCmd )
                continue;
            else
            { // Found the matching command, preserve the existing lock state
              // unless overriden in command and also the old destination to
              // note which areas are affected
                if( _prefix.isEmpty() )
                {
                    _prefix = it.value().mid(0,1);
                }
                it.remove(); // Despite this being a "Mutable" iterator it does
                             // NOT allow us to change the KEY - we only can
                             // remove the entry to add-in a new one later.
                break;
            }
        }
        else
        {
            if( it.value() != _strippedCmd )
                continue;
            else
            { // Found the matching command, but this is an old one with no lock state prefix
                if( _prefix.isEmpty() )
                    _prefix = '0'; // Assume default unlock case if not set
                it.remove();
                break;
            }

        }
    }
    // Have definately removed the existing case of this command
    // Now add it to map if wanted

    if( to > 1 )
    {
        if( _prefix.isEmpty() )
            _prefix = '0';

        QString finalCmd = _prefix % _strippedCmd;
        other.insertMulti(to, finalCmd);
    }
    else
    { // Clean up related data:
        customLinesArrow.remove( _strippedCmd );
        customLinesColor.remove( _strippedCmd );
        customLinesStyle.remove( _strippedCmd );
        customLines.remove( _strippedCmd );
        exitWeights.remove( _strippedCmd );
        doors.remove( _strippedCmd );
    }

    TArea * pA = mpRoomDB->getArea( area );
    if( pA )
    {
        pA->determineAreaExitsOfRoom( id );
        // This updates the (TArea *)->exits map even for exit REMOVALS
    }
    mpRoomDB->updateEntranceMap(this);
    mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
}

void TRoom::clearSpecialExits()
{
    other.clear();
    mpRoomDB->updateEntranceMap(this);
    mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
}

void TRoom::removeAllSpecialExitsToRoom( int _id )
{
    QList<int> keyList = other.keys();
    QList<QString> valList = other.values();
    for( int i=0; i<keyList.size(); i++ )
    {
        if( keyList[i] == _id )
        {
            // guaranteed to be in synch according to Qt docs
            other.remove(keyList[i], valList[i]);
        }
    }
    TArea * pA = mpRoomDB->getArea( area );
    if( pA )
    {
        pA->determineAreaExitsOfRoom( id );
    }
    mpRoomDB->updateEntranceMap(this);
    mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
}

void TRoom::calcRoomDimensions()
{
    if( customLines.size() < 1 ) return;
    min_x = 0.0;
    min_y = 0.0;
    max_x = 0.0;
    max_y = 0.0;
    bool needInit = true;

    QMapIterator<QString, QList<QPointF> > it(customLines);
    while( it.hasNext() )
    {
        it.next();
        const QList<QPointF> & _pL= it.value();
        if( _pL.size() < 1 ) continue;
        if( needInit )
        {
            needInit = false;
            min_x = _pL[0].x();
            max_x = min_x;
            min_y = _pL[0].y();
            max_y = min_y;
        }
        for( int i=0; i<_pL.size(); i++ )
        {
            qreal _x = _pL[i].x();
            qreal _y = _pL[i].y();
            if( _x < min_x )
                min_x = _x;
            if( _x > max_x )
                max_x = _x;
            if( _y < min_y )
                min_y = _y;
            if( _y > max_y )
                max_y = _y;
        }
    }
}

/* bool - N/U: no return value created or used */
void TRoom::restore( QDataStream & ifs, int roomID, int version )
{

    id = roomID;
    ifs >> area;
// Can be useful when analysing suspect map files!
//     qDebug() << "TRoom::restore(...," << roomID << ",...) has AreaId:" << area;
    ifs >> x;
    ifs >> y;
    ifs >> z;
    ifs >> north;
    ifs >> northeast;
    ifs >> east;
    ifs >> southeast;
    ifs >> south;
    ifs >> southwest;
    ifs >> west;
    ifs >> northwest;
    ifs >> up;
    ifs >> down;
    ifs >> in;
    ifs >> out;
    ifs >> environment;
    ifs >> weight;

    // force room weight >= 1 otherwise pathfinding choses random pathes.
    if( weight < 1 )
    {
        weight = 1;
    }

    if( version < 8 )
    {
        float f1,f2,f3,f4;
        ifs >> f1;//rooms[i]->xRot;
        ifs >> f2;//rooms[i]->yRot;
        ifs >> f3;//rooms[i]->zRot;
        ifs >> f4;//rooms[i]->zoom;
    }
    ifs >> name;
    ifs >> isLocked;
    if( version >= 6 )
    {
        ifs >> other;
    }
    if( version >= 9 )
    {
        ifs >> c;
    }
    if( version >= 10 )
    {
        ifs >> userData;
    }
    if( version >= 11 )
    {
        ifs >> customLines;
        ifs >> customLinesArrow;
        ifs >> customLinesColor;
        ifs >> customLinesStyle;
        ifs >> exitLocks;
    }
    if( version >= 13 )
    {
        ifs >> exitStubs;
    }
    if( version >= 16 )
    {
        ifs >> exitWeights;
        ifs >> doors;
    }
    calcRoomDimensions();
}

void TRoom::auditExits()
{
    if( north != -1 && ! mpRoomDB->getRoom(north) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"north\"", id);
        north = -1;
    }
    if( south != -1 && ! mpRoomDB->getRoom(south) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"south\"", id);
        south = -1;
    }
    if( northwest != -1 && ! mpRoomDB->getRoom(northwest) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"northwest\"", id);
        northwest = -1;
    }
    if( northeast != -1 && ! mpRoomDB->getRoom(northeast) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"northeast\"", id);
        northeast = -1;
    }
    if( southwest != -1 && ! mpRoomDB->getRoom(southwest) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"southwest\"", id);
        southwest = -1;
    }
    if( southeast != -1 && ! mpRoomDB->getRoom(southeast) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"southeast\"", id);
        southeast = -1;
    }
    if( west != -1 && ! mpRoomDB->getRoom(west) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"west\"", id);
        west = -1;
    }
    if( east != -1 && ! mpRoomDB->getRoom(east) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"east\"", id);
        east = -1;
    }
    if( in != -1 && ! mpRoomDB->getRoom(in) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"in\"", id);
        in = -1;
    }
    if( out != -1 && ! mpRoomDB->getRoom(out) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"out\"", id);
        out = -1;
    }
    if( up != -1 && ! mpRoomDB->getRoom(up) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"up\"", id);
        up = -1;
    }
    if( down != -1 && ! mpRoomDB->getRoom(down) )
    {
        qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (general) exit: \"down\"", id);
        down = -1;
    }
    // These last two were missing!

// If we use the Mutable iterator we don't have to restart after a deletion
    QMutableMapIterator<int, QString> it( other );
    while( it.hasNext() )
    {
        it.next();
        QString _cmd = it.value();
        if( _cmd.size() <= 0 )
        {
            qWarning("TRoom::auditExits() WARNING: roomID:%6i REMOVING invalid (special) exit to %i.", id, it.key());
            // If size is less than or equal to 0 then there is nothing to print!!!
            other.remove( it.key(), it.value() );
        }
        else if( ! ( _cmd.startsWith('1') || _cmd.startsWith('0') ) )
        {
            QString _nc = it.value();
            int _nk = it.key();
            _nc.prepend('0');
            // Old, prepatched special exit could not have a lock
            other.remove( it.key(), it.value() );
            other.insertMulti( _nk, _nc );
            qWarning("TRoom::auditExits() WARNING: roomID:%6i PATCHING invalid (special) exit to %i, was:%s now:%s.",
                     id,
                     _nk,
                     qPrintable(_cmd),
                     qPrintable(_nc));
        }
    }
}
