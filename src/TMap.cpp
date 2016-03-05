/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "TMap.h"


#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "Host.h"
#include "TArea.h"
#include "TConsole.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include "post_guard.h"


TMap::TMap( Host * pH )
: mpRoomDB( new TRoomDB( this ) )
, mpHost( pH )
, mpM( 0 )
, mpMapper( 0 )
, mMapGraphNeedsUpdate( true )
, mNewMove( true )
, mDefaultVersion( 16 )      // <== replaces CURRENT_MAP_VERSION
                            // THIS, mMinVersion AND mMaxVersion SHOULD BE
                            // REVISED WHEN WE SWITCH FROM A PREVIEW TO A RELEASE VERSION!
                            // Currently:
                            // + TLuaInterpreter::setAreaUserData()
                            // + TLuaInterpreter::setMapUserData() need 17
                            // (for persistant storage of data)
                            // + TArea::rooms as QSet<int> needs 18,
                            // is/was QList<int> in prior versions
                            // + TMap::mRoomIdHash as QHash<QString, int> needs 18, is/was
                            // a single mRoomId in prior versions
, mSaveVersion( mDefaultVersion ) // This needs to be set (for when writing new
                            // map files) as it triggers some version features
                            // that NEED a new map file format to be usable, it
                            // can be changed by control in last tab of profile
                            // preference dialog.
, mMaxVersion( 18 )              // CHECKME: Allow 18 ( mDefaultVersion + 2 ) for testing
, mMinVersion( mDefaultVersion ) // CHECKME: Allow 16 ( mDefaultVersion )
{
    mVersion = mDefaultVersion; // This is overwritten during a map restore and
                                // is the loaded file version
    customEnvColors[257] = mpHost->mRed_2;
    customEnvColors[258] = mpHost->mGreen_2;
    customEnvColors[259] = mpHost->mYellow_2;
    customEnvColors[260] = mpHost->mBlue_2;
    customEnvColors[261] = mpHost->mMagenta_2;
    customEnvColors[262] = mpHost->mCyan_2;
    customEnvColors[263] = mpHost->mWhite_2;
    customEnvColors[264] = mpHost->mBlack_2;
    customEnvColors[265] = mpHost->mLightRed_2;
    customEnvColors[266] = mpHost->mLightGreen_2;
    customEnvColors[267] = mpHost->mLightYellow_2;
    customEnvColors[268] = mpHost->mLightBlue_2;
    customEnvColors[269] = mpHost->mLightMagenta_2;
    customEnvColors[270] = mpHost->mLightCyan_2;
    customEnvColors[271] = mpHost->mLightWhite_2;
    customEnvColors[272] = mpHost->mLightBlack_2;
    unitVectors[1] = QVector3D(0,-1,0);
    unitVectors[2] = QVector3D(1,-1,0);
    unitVectors[3] = QVector3D(-1,-1,0);
    unitVectors[4] = QVector3D(1, 0,0);
    unitVectors[5] = QVector3D(-1,0,0);
    unitVectors[6] = QVector3D(0,1,0);
    unitVectors[7] = QVector3D(1,1,0);
    unitVectors[8] = QVector3D(-1,1,0);
    unitVectors[9] = QVector3D(0,0,1);
    unitVectors[10] = QVector3D(0,0,-1);
    reverseDirections[1] = 6; //contains complementary directions
    reverseDirections[2] = 8;
    reverseDirections[3] = 7;
    reverseDirections[4] = 5;
    reverseDirections[5] = 4;
    reverseDirections[6] = 1;
    reverseDirections[7] = 3;
    reverseDirections[8] = 2;
    reverseDirections[9] = 10;
    reverseDirections[10] = 9;
    reverseDirections[11] = 12;
    reverseDirections[12] = 11;
    m2DPanMode = false;
    mLeftDown = false;
    mRightDown = false;
}

TMap::~TMap() {
    delete mpRoomDB;
}

void TMap::mapClear()
{
    mpRoomDB->clearMapDB();
    mRoomIdHash.clear();
    pixNameTable.clear();
    pixTable.clear();
    envColors.clear();
    customEnvColors.clear();
    mapLabels.clear();
}

void TMap::logError( QString & msg )
{
    QColor orange = QColor(255,128,0);
    QColor black = QColor(0,0,0);
    QString s1 = QString("[MAP ERROR:]%1\n").arg(msg);
    if( mpHost->mpEditorDialog )
    {
        mpHost->mpEditorDialog->mpErrorConsole->printDebug(orange, black, s1 );
    }
}

void TMap::exportMapToDatabase()
{
    QString dbName = QFileDialog::getSaveFileName( 0, "Chose db file name." );
    QString script = QString("exportMapToDatabse([[%1]])").arg(dbName);
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
}

void TMap::importMapFromDatabase()
{
    QString dbName = QFileDialog::getOpenFileName( 0, "Chose db file name." );
    QString script = QString("importMapFromDatabase([[%1]])").arg(dbName);
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
}

bool TMap::setRoomArea( int id, int area, bool isToDeferAreaRelatedRecalculations )
{
    TRoom * pR = mpRoomDB->getRoom( id );
    if( !pR ) {
        QString msg = tr( "RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!" )
                      .arg(id)
                      .arg(area);
        logError(msg);
        return false;
    }

    TArea * pA = mpRoomDB->getArea( area );
    if( ! pA ) {
        // Uh oh, the area doesn't seem to exist as a TArea instance, lets check
        // to see if it exists as a name only:
        if( ! mpRoomDB->getAreaNamesMap().contains( area ) ) {
            // Ah, no it doesn't so moan:
            QString msg = tr( "AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!" )
                          .arg(id)
                          .arg(area);
            logError(msg);
            return false;
        }
        // If got to this point then there is NOT a TArea instance for the given
        // area Id but there is a Name - and the pR->setArea(...) call WILL
        // instantiate the required TArea structure - this seems a bit twisty
        // and convoluted, but it was how previous code was wired up and we need
        // to retain the API for the lua subsystem...
    }

    bool result = pR->setArea( area, isToDeferAreaRelatedRecalculations );
    if( result ) {
        mMapGraphNeedsUpdate = true;
    }
    return result;
}

bool TMap::addRoom( int id )
{
    bool ret = mpRoomDB->addRoom( id );
    if( ret ) mMapGraphNeedsUpdate = true;
    return ret;
}

bool TMap::setRoomCoordinates( int id, int x, int y, int z )
{
    TRoom * pR = mpRoomDB->getRoom( id );
    if( !pR ) return false;

    pR->x = x;
    pR->y = y;
    pR->z = z;

    return true;
}

int compSign(int a, int b){
    return (a < 0) == (b < 0);
}

void TMap::connectExitStub(int roomId, int dirType)
{
    TRoom * pR = mpRoomDB->getRoom( roomId );
    if( !pR )
    {
        return;
    }
    int area = pR->getArea();
    int minDistance = 999999;
    int minDistanceRoom=0, meanSquareDistance=0;
    if( !unitVectors.contains( dirType ) ) return;
    QVector3D unitVector = unitVectors[dirType];
    int ux = unitVector.x(), uy = unitVector.y(), uz = unitVector.z();
    int rx = pR->x, ry = pR->y, rz = pR->z;
    int dx=0,dy=0,dz=0;
    TArea * pA = mpRoomDB->getArea(area);
    if( !pA )
    {
        return;
    }
    QSetIterator<int> itRoom( pA->getAreaRooms() );
    while( itRoom.hasNext() )
    {
        pR = mpRoomDB->getRoom( itRoom.next() );
        if( !pR )
        {
            continue;
        }
        if( pR->getId() == roomId )
        {
            continue;
        }
        if(uz)
        {
            dz = pR->z-rz;
            if(!compSign(dz,uz) || !dz)
            {
                continue;
            }
        }
        else
        {
            //to avoid lower/upper floors from stealing stubs
            if(pR->z != rz)
            {
                continue;
            }
        }
        if(ux)
        {
            dx = pR->x-rx;
            if (!compSign(dx,ux) || !dx) //we do !dx to make sure we have a component in the desired direction
            {
               continue;
            }
        }
        else
        {
            //to avoid rooms on same plane from stealing stubs
            if((int)pR->x != rx)
            {
                continue;
            }
        }
        if(uy)
        {
            dy = pR->y-ry;
            //if the sign is the SAME here we keep it b/c we flip our y coordinate.
            if (compSign(dy,uy) || !dy)
            {
                continue;
            }
        }
        else
        {
            //to avoid rooms on same plane from stealing stubs
            if(pR->y != ry)
            {
                continue;
            }
        }
        meanSquareDistance=dx*dx+dy*dy+dz*dz;
        if(meanSquareDistance < minDistance)
        {
            minDistanceRoom=pR->getId();
            minDistance=meanSquareDistance;
        }
    }
    if(minDistanceRoom)
    {
        pR = mpRoomDB->getRoom(minDistanceRoom);
        if( !pR )
        {
            return;
        }
        if(pR->exitStubs.contains(reverseDirections[dirType]))
        {
            setExit( roomId, minDistanceRoom, dirType);
            setExit( minDistanceRoom, roomId, reverseDirections[dirType]);
        }
    }
}

int TMap::createNewRoomID()
{
    int _id = 1;
    for( ; ; _id++ )
    {
        if( ! mpRoomDB->getRoom( _id ) )
        {
            return _id;
        }
    }
    return -1;
}

bool TMap::setExit( int from, int to, int dir )
{
    // FIXME: This along with TRoom->setExit need to be unified to a controller.
    TRoom * pR = mpRoomDB->getRoom( from );
    TRoom * pR_to = mpRoomDB->getRoom( to );

    if( !pR ) {
        return false;
    }
    if( !pR_to && to > 0 ) {
        return false;
    }
    if( to < 1 ) {
        to = -1;
    }

    bool ret = true;

    switch( dir ) {
        case DIR_NORTH:
            pR->setNorth(to);
            break;
        case DIR_NORTHEAST:
            pR->setNortheast(to);
            break;
        case DIR_NORTHWEST:
            pR->setNorthwest(to);
            break;
        case DIR_EAST:
            pR->setEast(to);
            break;
        case DIR_WEST:
            pR->setWest(to);
            break;
        case DIR_SOUTH:
            pR->setSouth(to);
            break;
        case DIR_SOUTHEAST:
            pR->setSoutheast(to);
            break;
        case DIR_SOUTHWEST:
            pR->setSouthwest(to);
            break;
        case DIR_UP:
            pR->setUp(to);
            break;
        case DIR_DOWN:
            pR->setDown(to);
            break;
        case DIR_IN:
            pR->setIn(to);
            break;
        case DIR_OUT:
            pR->setOut(to);
            break;
        default:
            ret = false;
    }
    pR->setExitStub(dir, false);
    mMapGraphNeedsUpdate = true;
    TArea * pA = mpRoomDB->getArea( pR->getArea() );
    if( ! pA ) {
        return false;
    }
    pA->determineAreaExitsOfRoom(pR->getId());
    mpRoomDB->updateEntranceMap(pR);
    return ret;
}

void TMap::init( Host * pH )
{
    // init areas
    QElapsedTimer _time;
    _time.start();

    if( mVersion < 14 ) {
        mpRoomDB->initAreasForOldMaps();
    }
    else if( mVersion < 17 ) {
        // The second half of mpRoomDB->initAreasForOldMaps() - needed to fixup
        // all the (TArea *)->areaExits() that were built wrongly previously,
        // calcSpan() may not be required to be done here and now but it is in my
        // sights as a target for revision in the future. Slysven
        QMapIterator<int, TArea *> itArea( mpRoomDB->getAreaMap() );
        while( itArea.hasNext() ) {
            itArea.next();
            itArea.value()->determineAreaExits();
            itArea.value()->calcSpan();
        }
    }
    mpRoomDB->auditRooms();

    if( mVersion <16 ) {
        // convert old style labels, wasn't made version conditional in past but
        // not likely to be an issue in recent map file format versions (say 16+)
        QMapIterator<int, TArea *> itArea( mpRoomDB->getAreaMap() );
        while( itArea.hasNext() ) {
            itArea.next();
            int areaID = itArea.key();
            if( mapLabels.contains(areaID) ) {
                QList<int> labelIDList = mapLabels.value(areaID).keys();
                for( int i=0; i<labelIDList.size(); i++ ) {
                    TMapLabel l = mapLabels.value(areaID).value(labelIDList.at(i));
                    if( l.pix.isNull() ) {
                        int newID = createMapLabel(areaID, l.text, l.pos.x(), l.pos.y(), l.pos.z(), l.fgColor, l.bgColor, true, false, 40.0, 50 );
                        if( newID > -1 ) {
                            QString msg = tr( "[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2." )
                                          .arg(areaID)
                                          .arg(labelIDList.at(i) );
                            mpHost->postMessage(msg);
                            mapLabels[areaID][labelIDList.at(i)] = mapLabels[areaID][newID];
                            deleteMapLabel( areaID, newID );
                        }
                        else {
                            QString msg = tr( "[ WARN ] - CONVERTING: cannot convert old style label, areaID:%1 labelID:%2." )
                                          .arg(areaID)
                                          .arg(labelIDList.at(i));
                            mpHost->postMessage(msg);
                        }
                    }
                    if (    ( l.size.width() >  std::numeric_limits<qreal>::max() )
                         || ( l.size.width() < -std::numeric_limits<qreal>::max() ) ) {
                        mapLabels[areaID][labelIDList[i]].size.setWidth(l.pix.width());
                    }
                    if (    ( l.size.height() >  std::numeric_limits<qreal>::max() )
                         || ( l.size.height() < -std::numeric_limits<qreal>::max() ) ) {
                        mapLabels[areaID][labelIDList[i]].size.setHeight(l.pix.height());
                    }
                }
            }
        }
    }
    qDebug() << "TMap::init() Initialize run time:" << _time.nsecsElapsed() * 1.0e-9 << "sec.";
}



void TMap::setView(float x, float y, float z, float zoom )
{
}

void TMap::tidyMap( int areaID )
{
}

void TMap::solveRoomCollision( int id, int creationDirection, bool PCheck )
{
}

QList<int> TMap::detectRoomCollisions( int id )
{
    QList<int> collList;
    TRoom * pR = mpRoomDB->getRoom( id );
    if( !pR )
    {
        return collList;
    }
    int area = pR->getArea();
    int x = pR->x;
    int y = pR->y;
    int z = pR->z;
    TArea * pA = mpRoomDB->getArea( area );
    if( !pA )
    {
        return collList;
    }

    QSetIterator<int> itRoom( pA->getAreaRooms() );
    while( itRoom.hasNext() )
    {
        int checkRoomId = itRoom.next();
        pR = mpRoomDB->getRoom( checkRoomId );
        if( !pR )
        {
            continue;
        }
        if( pR->x == x && pR->y == y && pR->z == z )
        {
            collList.push_back( checkRoomId );
        }
    }

    return collList;
}

void TMap::astBreitenAnpassung( int id, int id2 )
{
}

void TMap::astHoehenAnpassung( int id, int id2 )
{
}

void TMap::getConnectedNodesGreaterThanX( int id, int min )
{
}

void TMap::getConnectedNodesSmallerThanX( int id, int min )
{
}

void TMap::getConnectedNodesGreaterThanY( int id, int min )
{
}

void TMap::getConnectedNodesSmallerThanY( int id, int min )
{
}

bool TMap::gotoRoom( int r )
{
    mTargetID = r;
    return findPath( mRoomIdHash.value( mpHost->getName() ), r );
}

// As can be seen this only sets the target and start point for a path find
// the speedwalk is instigated by the Host class caller...
bool TMap::gotoRoom( int r1, int r2 )
{
    return findPath( r1, r2 );
}

void TMap::initGraph()
{
    QElapsedTimer _time;
    _time.start();
    locations.clear();
    roomidToIndex.clear();
    g.clear();
    g = mygraph_t();
    unsigned int roomCount=0;
    unsigned int edgeCount=0;
    QSet<unsigned int> unUsableRoomSet;
    // Keep track of the unusuable rather than the useable ones because that is
    // hopefully a MUCH smaller set in normal situations!
    QHashIterator<int, TRoom *> itRoom = mpRoomDB->getRoomMap();
    while( itRoom.hasNext() ) {
        itRoom.next();
        TRoom * pR = itRoom.value();
        if( itRoom.key() < 1 || ! pR || pR->isLocked ) {
            unUsableRoomSet.insert( itRoom.key() );
            continue;
        }

        location l;
        l.pR = pR;
        l.id = itRoom.key();
        // locations is std::vector<location> and (locations.at( k )).id will give room ID value
        locations.push_back( l );
        // Map's usable TRooms (key) to index of entry in locations (for route finding), will lose invalid and unusable (through locking) rooms
        roomidToIndex.insert( itRoom.key(), roomCount++ );
    }

    // Now identify the routes between rooms, and pick out the best edges of parallel ones
    foreach(location l, locations) {
        unsigned int source = l.id;
        TRoom * pSourceR = l.pR;
        QHash<unsigned int, route> bestRoutes;
        // key is target (destination room),
        // value is data we will need to store later,
        QMap<QString, int> exitWeights = pSourceR->getExitWeights();

        int target = pSourceR->getNorth();
        TRoom * pTargetR;
        quint8 direction = DIR_NORTH;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            // In above tests the second test is to eliminate self-edges (they
            // are of no use).  The third test is to eliminate targets that we
            // have already found to be unreachable because they are invalid or
            // locked.
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) { // OK got something that is valid
                route r;
                r.cost = exitWeights.value(QStringLiteral("n"), pTargetR->getWeight() );
                r.direction = direction;
                bestRoutes.insert(target, r);
            }
        }

        target = pSourceR->getEast();
        direction = DIR_EAST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("e"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) { // Ah, this is a better route
                    r.direction = direction;
                    bestRoutes.insert(target, r); // If the second part of conditional is the truth this will replace previous best route to this target
                }
            }
        }

        target = pSourceR->getSouth();
        direction = DIR_SOUTH;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("s"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getWest();
        direction = DIR_WEST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("w"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getUp();
        direction = DIR_UP;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("up"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getDown();
        direction = DIR_DOWN;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("down"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getNortheast();
        direction = DIR_NORTHEAST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("ne"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getSoutheast();
        direction = DIR_SOUTHEAST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("se"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getSouthwest();
        direction = DIR_SOUTHWEST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("sw"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getNorthwest();
        direction = DIR_NORTHWEST;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("nw"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getIn();
        direction = DIR_IN;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("in"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        target = pSourceR->getOut();
        direction = DIR_OUT;
        if( target > 0 && source != target && !unUsableRoomSet.contains(target) && !pSourceR->hasExitLock(direction) ) {
            pTargetR = mpRoomDB->getRoom( target );
            if( pTargetR && ! pTargetR->isLocked ) {
                route r;
                r.cost = exitWeights.value(QStringLiteral("out"), pTargetR->getWeight() );
                if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                    r.direction = direction;
                    bestRoutes.insert(target, r);
                }
            }
        }

        QMapIterator<int, QString> itSpecialExit( pSourceR->getOtherMap() );
        while( itSpecialExit.hasNext() ) {
            itSpecialExit.next();
            if( (itSpecialExit.value()).startsWith( QStringLiteral("1") ) ) {
                continue; // Is a locked exit so forget it...
            }

            target = itSpecialExit.key();
            direction = DIR_OTHER;
            if( target > 0 && source != target && !unUsableRoomSet.contains(target) ) {
                pTargetR = mpRoomDB->getRoom( target );
                if( pTargetR && ! pTargetR->isLocked ) {
                    route r;
                    if( Q_LIKELY( (itSpecialExit.value()).startsWith( QStringLiteral("0")) ) ) {
                        r.specialExitName = itSpecialExit.value().mid(1);
                         }
                    else {
                        r.specialExitName = itSpecialExit.value();
                    }
                    r.cost = exitWeights.value( r.specialExitName, pTargetR->getWeight() );
                    if( ! bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost ) {
                        r.direction = direction;
                        bestRoutes.insert(target, r);
                    }
                }
            }
        } // End of while(itSpecialExit.hasNext())

        // Now we have eliminated possibe duplicate and useless edges we can create and
        // insert the remainder into the BGL graph:
        QHashIterator<unsigned int, route> itRoute = bestRoutes;
        while( itRoute.hasNext() ) {
            itRoute.next();
            edge_descriptor e;
            bool inserted; // This is always going to be false as it gets set if
                           // we had tried to insert a parallel edge into a graph
                           // that does not support them - but we've just been
                           // and disposed of those already!
            tie(e, inserted) = add_edge( roomidToIndex.value( source ),
                                         roomidToIndex.value( itRoute.key() ),
                                         itRoute.value().cost,
                                         g );
            edgeHash.insert( qMakePair(source, itRoute.key()), itRoute.value() );
            // The key is made from the QPair<edgeSourceRoomId, edgeTargetRoomId>...
            edgeCount++;
        }
    } // End of foreach(location l, locations)

    mMapGraphNeedsUpdate = false;
    qDebug() << "TMap::initGraph() INFO: built graph with:" << locations.size() << "(" << roomCount <<") locations(roomCount), and discarded" << unUsableRoomSet.count() << "other NOT useable rooms and found:" << edgeCount << "distinct, usable edges in:" << _time.nsecsElapsed() * 1.0e-9 << "seconds.";
}

bool TMap::findPath( int from, int to )
{
    if( mMapGraphNeedsUpdate ) {
        initGraph();
    }

    QElapsedTimer t;
    t.start();

    mPathList.clear();
    mDirList.clear();
    mWeightList.clear();
    // Clear the previous path data here so that if the following test is
    // passed, the data is empty - and valid for THAT case!

    if( from == to ) {
        return true; // Take a short-cut for trival "already there" case!
    }

    TRoom * pFrom = mpRoomDB->getRoom( from );
    TRoom * pTo = mpRoomDB->getRoom( to );

    if( !pFrom || !pTo ) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: NULL TRoom pointer for start or target rooms!";
        return false;
    }

    bool hasUsableExit = false;

    if( pFrom->getNorth()                        > 0 && ( ! pFrom->hasExitLock( DIR_NORTH ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getSouth()     > 0 && ( ! pFrom->hasExitLock( DIR_SOUTH     ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getWest()      > 0 && ( ! pFrom->hasExitLock( DIR_WEST      ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getEast()      > 0 && ( ! pFrom->hasExitLock( DIR_EAST      ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getUp()        > 0 && ( ! pFrom->hasExitLock( DIR_UP        ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getDown()      > 0 && ( ! pFrom->hasExitLock( DIR_DOWN      ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getNortheast() > 0 && ( ! pFrom->hasExitLock( DIR_NORTHEAST ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getNorthwest() > 0 && ( ! pFrom->hasExitLock( DIR_NORTHWEST ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getSoutheast() > 0 && ( ! pFrom->hasExitLock( DIR_SOUTHEAST ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getSouthwest() > 0 && ( ! pFrom->hasExitLock( DIR_SOUTHWEST ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getIn()        > 0 && ( ! pFrom->hasExitLock( DIR_IN        ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit && pFrom->getOut()       > 0 && ( ! pFrom->hasExitLock( DIR_OUT       ) ) ) {
        hasUsableExit = true;
    }
    if( ! hasUsableExit ) {
        // No available normal exits from this room so check the special ones
        QStringList specialExitCommands = pFrom->getOtherMap().values();
        while( ! specialExitCommands.isEmpty() ) {
            if( specialExitCommands.at(0).mid(0,1)== "0" ) {
                hasUsableExit = true;
                break;
            }
            specialExitCommands.removeFirst();
        }
    }
    if( ! hasUsableExit ) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: no usable exits from start room!";
        return false; // No available exits from the start room so give up!
    }

    if( ! roomidToIndex.contains(from) ) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: start room not in map graph!";
        return false;
        // The start room is NOT one that has been included in the BGL graph
        // probably because it is locked - so no route finding can be done
    }
    vertex start = roomidToIndex.value(from);

    if( ! roomidToIndex.contains(to) ) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: target room not in map graph!";
        return false;
        // The target room is NOT one that has been included in the BGL graph
        // probably because it is locked - so no route finding can be done
    }
    vertex goal = roomidToIndex.value(to);

    std::vector<vertex> p(num_vertices(g));
    // Somehow p is an acending, monotonic series of numbers start at 0, it
    // seems we have a redundent indirection in play there as p[0]=0, p[1]=1,..., p[n]=n ...!
    std::vector<cost> d(num_vertices(g));
    try {
        astar_search( g,
                      start,
                      distance_heuristic<mygraph_t, cost, std::vector<location> >(locations, goal),
                      predecessor_map(&p[0]).distance_map(&d[0]).
                      visitor(astar_goal_visitor<vertex>(goal)) );
    }
    catch( found_goal ) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: time elapsed in A*:" << t.nsecsElapsed() * 1.0e-9 << "seconds.";
             t.restart();
        if( ! roomidToIndex.contains(to) ) {
            qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: target room not in map graph!";
            return false;
        }

        vertex currentVertex = roomidToIndex.value(to);
        unsigned int currentRoomId = (locations.at(currentVertex)).id;

        // We step through the found path BACKWARDS so advance (well retard)
        // the "previous" one first, and it will be the SOURCE vertex for the
        // edge and current will be the TARGET vertex:
        vertex previousVertex = currentVertex;
        do {
            previousVertex = p[currentVertex];
            if( previousVertex == currentVertex ) {
                qDebug() << "TMap::findPath(" << from << "," << to << ") WARN: unable to build a path in:" << t.nsecsElapsed() * 1.0e-9 << "seconds." ;
                mPathList.clear();
                mDirList.clear();
                mWeightList.clear(); // Reset any partial results...
                return false;
            }
            unsigned int previousRoomId = (locations.at(previousVertex)).id;
            QPair<unsigned int, unsigned int> edgeRoomIdPair = qMakePair(previousRoomId, currentRoomId);
            route r = edgeHash.value( edgeRoomIdPair );
            mPathList.prepend( currentRoomId );
            Q_ASSERT_X( r.cost > 0, "TMap::findPath()", "broken path {QPair made from source and target roomIds for a path step NOT found in QHash table of all possible steps.}");
            // Above was found to be triggered by the situation described in:
            // https://bugs.launchpad.net/mudlet/+bug/1263447 on 2015-07-17 but
            // this is because previousVertex was the same as currentVertex after
            // the "previousVertex = p[currentVertex]" operation at the start of
            // the do{} loop - added a test for this so should bail out if it
            // happens - Slysven
            mWeightList.prepend( r.cost );
            switch( r.direction ) {  // TODO: Eventually this can instead drop in I18ned values set by country or user preference!
            case DIR_NORTH:        mDirList.prepend( tr( "n", "This translation converts the direction that DIR_NORTH codes for to a direction string that the MUD server will accept!" ) );      break;
            case DIR_NORTHEAST:    mDirList.prepend( tr( "ne", "This translation converts the direction that DIR_NORTHEAST codes for to a direction string that the MUD server will accept!" ) ); break;
            case DIR_EAST:         mDirList.prepend( tr( "e", "This translation converts the direction that DIR_EAST codes for to a direction string that the MUD server will accept!" ) );       break;
            case DIR_SOUTHEAST:    mDirList.prepend( tr( "se", "This translation converts the direction that DIR_SOUTHEAST codes for to a direction string that the MUD server will accept!" ) ); break;
            case DIR_SOUTH:        mDirList.prepend( tr( "s", "This translation converts the direction that DIR_SOUTH codes for to a direction string that the MUD server will accept!" ) );      break;
            case DIR_SOUTHWEST:    mDirList.prepend( tr( "sw", "This translation converts the direction that DIR_SOUTHWEST codes for to a direction string that the MUD server will accept!" ) ); break;
            case DIR_WEST:         mDirList.prepend( tr( "w", "This translation converts the direction that DIR_WEST codes for to a direction string that the MUD server will accept!" ) );       break;
            case DIR_NORTHWEST:    mDirList.prepend( tr( "nw", "This translation converts the direction that DIR_NORTHWEST codes for to a direction string that the MUD server will accept!" ) ); break;
            case DIR_UP:           mDirList.prepend( tr( "up", "This translation converts the direction that DIR_UP codes for to a direction string that the MUD server will accept!" ) );        break;
            case DIR_DOWN:         mDirList.prepend( tr( "down", "This translation converts the direction that DIR_DOWN codes for to a direction string that the MUD server will accept!" ) );    break;
            case DIR_IN:           mDirList.prepend( tr( "in", "This translation converts the direction that DIR_IN codes for to a direction string that the MUD server will accept!" ) );        break;
            case DIR_OUT:          mDirList.prepend( tr( "out", "This translation converts the direction that DIR_OUT codes for to a direction string that the MUD server will accept!" ) );      break;
            case DIR_OTHER:        mDirList.prepend( r.specialExitName );  break;
            default:               qWarning() << "TMap::findPath(" << from << "," << to << ") WARN: found route between rooms (from Id:" << previousRoomId << ", to Id:" << currentRoomId << ") with an invalid DIR_xxxx code:" << r.direction << " - the path will not be valid!" ;
            }
            currentVertex = previousVertex;
            currentRoomId = previousRoomId;
        } while( currentVertex != start );

        qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: found path in:" << t.nsecsElapsed() * 1.0e-9 << "seconds." ;
        return true;
    }

    qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: did NOT find path in:" << t.nsecsElapsed() * 1.0e-9 << "seconds." ;
    return false;
}

bool TMap::serialize( QDataStream & ofs )
{

    if( mSaveVersion != mVersion ) {
        QString message = tr( "[ ALERT ] - Saving map in a format {%1} that is different than the one it was\n"
                                          "loaded as {%2}. This may be an issue if you want to share the resulting\n"
                                          "map with others relying on the original format." )
                          .arg( mSaveVersion )
                          .arg( mVersion );
        mpHost->mTelnet.postMessage( message );
    }

    if( mSaveVersion != mDefaultVersion ) {
        QString message = tr( "[ WARN ]  - Saving map in a format {%1} that is different than the one\n"
                                          "recommended {%2} baring in mind the build status of the source\n"
                                          "code.  Development code versions may offer the chance to try\n"
                                          "experimental features needing a revised format that could be\n"
                                          "incompatible with existing release code versions.  Conversely\n"
                                          "a release version may allow you to downgrade to save a map in\n"
                                          "a format compatible with others using older versions of MUDLET\n"
                                          "however some features may be crippled or non-operational for\n"
                                          "this version of MUDLET." )
                          .arg( mSaveVersion )
                          .arg( mDefaultVersion );
        mpHost->postMessage( message );
    }

    ofs << mSaveVersion;
    ofs << envColors;
    ofs << mpRoomDB->getAreaNamesMap();
    ofs << customEnvColors;
    ofs << mpRoomDB->hashTable;
    if( mSaveVersion >= 17 ) {
        ofs << mUserData;
    }
    // TODO: Remove when versions < 17 are not an option...
    else {
        if( ! mUserData.isEmpty() ) {
            QString message = tr( "[ ALERT ] - Map User data has been lost in saved map file.  Re-save in a\n"
                                              "format of at least 17 to preserve it before quitting!" )
                                  .arg( mSaveVersion );
            mpHost->mTelnet.postMessage( message );
        }
    }
    // End of TODO:

    ofs << mpRoomDB->getAreaMap().size();
    // serialize area table
    QMapIterator<int, TArea *> itAreaList(mpRoomDB->getAreaMap());
    QList<int> areasWithData; // TODO: Remove when versions < 17 are not an option
    while( itAreaList.hasNext() )
    {
        itAreaList.next();
        int areaID = itAreaList.key();
        TArea * pA = itAreaList.value();
        ofs << areaID;
        if( mSaveVersion >= 18 ) {
            ofs << pA->rooms;
        }
        else {
            // Switched to a (faster) QSet<int> from a QList<int> in version 18
            QList<int> _oldList = pA->rooms.toList();
            ofs << _oldList;
        }
        ofs << pA->ebenen;
        ofs << pA->exits;
        ofs << pA->gridMode;
        ofs << pA->max_x;
        ofs << pA->max_y;
        ofs << pA->max_z;
        ofs << pA->min_x;
        ofs << pA->min_y;
        ofs << pA->min_z;
        ofs << pA->span;
        if( mSaveVersion >= 17) {
            ofs << pA->xmaxEbene;
            ofs << pA->ymaxEbene;
            ofs << pA->xminEbene;
            ofs << pA->yminEbene;
        }
        else { // Recreate the pointless z{min|max}Ebene items
            QMap<int, int> dummyMinMaxEbene;
            QListIterator<int> itZ( pA->ebenen );
            while( itZ.hasNext() ) {
                int dummyEbenValue = itZ.next();
                dummyMinMaxEbene.insert( dummyEbenValue, dummyEbenValue );
            }
            ofs << pA->xmaxEbene;
            ofs << pA->ymaxEbene;
            ofs << dummyMinMaxEbene;
            ofs << pA->xminEbene;
            ofs << pA->yminEbene;
            ofs << dummyMinMaxEbene;
        }
        ofs << pA->pos;
        ofs << pA->isZone;
        ofs << pA->zoneAreaRef;
        if( mSaveVersion >= 17 ) {
            ofs << pA->mUserData;
        }
        // TODO: Remove when versions < 17 are not an option...
        else {
            if( ! pA->mUserData.isEmpty() ) {
                areasWithData.append( areaID );
            }
        }
        // End of TODO:
    }

    // TODO: Remove when versions < 17 are not an option...
    if( ! areasWithData.isEmpty() ) {
        if( areasWithData.size() > 1 ) {
            std::sort( areasWithData.begin(), areasWithData.end() );
        }
        QStringList areaIds;
        do {
            areaIds.append( QString::number( areasWithData.takeFirst() ) );
        } while( ! areasWithData.isEmpty() );

        QString message = tr( "[ ALERT ] - Area User data has been lost in saved map file.  Re-save in a\n"
                                          "format of at least 17 to preserve it before quitting!\n"
                                          "Areas Id affected: %1." )
                              .arg( areaIds.join( tr( ", " ) ) ); // Translatable in case list separators are locale dependendent!
        mpHost->mTelnet.postMessage( message );
    }
    // End of TODO

    if( mSaveVersion >= 18 ) {
        // Revised in version 18 to store mRoomId as a per profile case so that
        // sharing/copying between profiles respects each profile's player
        // location
        ofs << mRoomIdHash;
    }
    else {
        ofs << mRoomIdHash.value( mpHost->getName() );
    }

    ofs << mapLabels.size(); //anzahl der areas
    QMapIterator<int, QMap<int, TMapLabel> > itL1(mapLabels);
    while( itL1.hasNext() )
    {
        itL1.next();
        int i = itL1.key();
        ofs << itL1.value().size();//anzahl der labels pro area
        ofs << itL1.key(); //area id
        QMapIterator<int, TMapLabel> itL2(mapLabels[i]);
        while( itL2.hasNext() )
        {
            itL2.next();
            ofs << itL2.key();//label ID
            TMapLabel label = itL2.value();
            ofs << label.pos;
            ofs << label.pointer;
            ofs << label.size;
            ofs << label.text;
            ofs << label.fgColor;
            ofs << label.bgColor;
            ofs << label.pix;
            ofs << label.noScaling;
            ofs << label.showOnTop;
        }
    }
    QHashIterator<int, TRoom *> it( mpRoomDB->getRoomMap() );
    while( it.hasNext() )
    {

        it.next();
        TRoom * pR = it.value();
        if( ! pR ) {
            qDebug() << "TMap::serialize(...) skipping a room with a NULL TRoom pointer:"
                     << it.key();
            continue;
        }

        ofs << pR->getId();
        ofs << pR->getArea();
        ofs << pR->x;
        ofs << pR->y;
        ofs << pR->z;
        ofs << pR->getNorth();
        ofs << pR->getNortheast();
        ofs << pR->getEast();
        ofs << pR->getSoutheast();
        ofs << pR->getSouth();
        ofs << pR->getSouthwest();
        ofs << pR->getWest();
        ofs << pR->getNorthwest();
        ofs << pR->getUp();
        ofs << pR->getDown();
        ofs << pR->getIn();
        ofs << pR->getOut();
        ofs << pR->environment;
        ofs << pR->getWeight();
        ofs << pR->name;
        ofs << pR->isLocked;
        ofs << pR->getOtherMap();
        ofs << pR->c;
        ofs << pR->userData;
        ofs << pR->customLines;
        ofs << pR->customLinesArrow;
        ofs << pR->customLinesColor;
        ofs << pR->customLinesStyle;
        ofs << pR->exitLocks;
        ofs << pR->exitStubs;
        ofs << pR->getExitWeights();
        ofs << pR->doors;
    }
    return true;
}

bool TMap::restore(QString location)
{
    qDebug() << "TMap::restore(" << location << ") INFO: restoring map of Profile:"
             << mpHost->getName()
             << " URL:"
             << mpHost->getUrl();

    QElapsedTimer _time;
    _time.start();
    QString folder;
    QStringList entries;

    if( location.isEmpty() ) {
        folder = QStringLiteral( "%1/.config/mudlet/profiles/%2/map/" )
                 .arg( QDir::homePath() )
                 .arg( mpHost->getName() );
        QDir dir( folder );
        dir.setSorting( QDir::Time );
        entries = dir.entryList( QDir::Files, QDir::Time );
    }

    bool canRestore = true;
    if( entries.size() || ! location.isEmpty() ) {
        QFile file(   location.isEmpty()
                    ? QStringLiteral( "%1%2" ).arg( folder ).arg( entries.at(0) )
                    : location );

        if( ! file.open( QFile::ReadOnly ) ) {
            QString errMsg = tr( "[ ERROR ] - Unable to open (for reading) map file: \"%1\"!" )
                             .arg( file.fileName() );
            mpHost->postMessage( errMsg );
            return false;
        }

        QDataStream ifs( & file );

        ifs >> mVersion;
//        qDebug()<<"map version:"<<mVersion;
        if( mVersion > mDefaultVersion ) {
            if( QByteArray( APP_BUILD ).isEmpty() ) {
                // This is a release version - should not support any map file versions higher that it was built for
                QString errMsg = tr( "[ ERROR ] - Map file is too new, it's file format (%1) is higher than this version of\n"
                                                 "Mudlet can handle (%2)!" )
                                 .arg( mVersion )
                                 .arg( mDefaultVersion );
                mpHost->postMessage( errMsg );
                QString infoMsg = tr( "[ INFO ]  - You will need to upgrade your Mudlet or find a Map file saved in a\n"
                                                  "format that it CAN read.  As this Mudlet appears to be based on a\n"
                                                  "release version of the source code it is possible that you, or the\n"
                                                  "creator of the Map file, may have been experimenting with a development\n"
                                                  "version that has additional features that need a revised format and,\n"
                                                  "unfortunately the file that has been loaded was from that.  If the\n"
                                                  "map was loaded automatically on profile start up, you be able to recover\n"
                                                  "from this by going to the \"map\" sub-directory of this profile's home\n"
                                                  "directory and selecting a file to load that is NOT the most recent one\n"
                                                  "(which is the one that is selected normally) though it is probable that\n"
                                                  "the stored location of the current map location will be wrong." );
                mpHost->postMessage( infoMsg );
                file.close();
                return false;
            }
            else {
                // Is a development version so check against mMaxVersion
                if( mVersion > mMaxVersion ) {
                    // Oh dear, can't handle THIS
                    QString errMsg = tr( "[ ERROR ] - Map file is too new, it's file format (%1) is higher than this version of\n"
                                                     "Mudlet can handle (%2)!" )
                                     .arg( mVersion )
                                     .arg( mMaxVersion );
                    mpHost->postMessage( errMsg );
                    QString infoMsg = tr( "[ INFO ]  - You will need to upgrade your Mudlet or find a Map file saved in a\n"
                                                      "format that it CAN read.  Even though this Mudlet appears to be based on a\n"
                                                      "development version of the source code it is possible that you, or the\n"
                                                      "creator of the Map file, may have been experimenting with an even more\n"
                                                      "advanced version that has additional features that need a revised format\n"
                                                      "and, unfortunately the file that has been loaded was from that.  If the\n"
                                                      "map was loaded automatically on profile start up, you be able to recover\n"
                                                      "from this by going to the \"map\" sub-directory of this profile's home\n"
                                                      "directory and selecting a file to load that is NOT the most recent one\n"
                                                      "(which is the one that is selected normally) though it is probable that\n"
                                                      "the stored location of the current map location will be wrong." );
                    mpHost->postMessage( infoMsg );
                    file.close();
                    return false;
                }
                else {
                    QString alertMsg = tr( "[ ALERT ] - Map file is using new features, it's file format (%1) is higher than\n"
                                                       "the default for this version of Mudlet (%2)!" )
                                     .arg( mVersion )
                                     .arg( mDefaultVersion );
                    mpHost->postMessage( alertMsg );
                    QString infoMsg = tr( "[ INFO ]  - You are using a new, possibly experimental, map file format which this\n"
                                                      "version of Mudlet CAN read.  This Mudlet appears to be based on a\n"
                                                      "development version of the source code so it is possible that you, or the\n"
                                                      "creator of the Map file, may have been experimenting with this more\n"
                                                      "advanced version that has additional features that need a revised format;\n"
                                                      "beware that this file may not be usable by the current release version\n"
                                                      "of Mudlet that you or others with whom you might share it have!" );
                    mpHost->postMessage( infoMsg );
                    mSaveVersion = mVersion; // Make the save version default to the loaded one
                                             // this means that each session using a particular
                                             // map file version will continue to use it unless
                                             // the user intervenes.
                }
            }
        }
        else if( mVersion < 4 ) {
            QString alertMsg = tr( "[ ALERT ] - Map file is really old, it's file format (%1) is so ancient that\n"
                                               "this version of Mudlet may not gain enough information from\n"
                                               "it but it will try!" )
                               .arg( mVersion );
            mpHost->postMessage( alertMsg );
            QString infoMsg = tr( "[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!\n"
                                              "There is so much data that it DOES NOT have that you could be\n"
                                              "be better off starting again..." );
            mpHost->postMessage( infoMsg );
            canRestore = false;
            mSaveVersion = mVersion; // Make the save version the default one - unless the user intervenes
        }
        else {
            // Less than (but not less than 4) or equal to default version
            QString infoMsg = tr( "[ INFO ]  - Reading map file: \"%1\", format version:%2, please wait..." )
                              .arg( file.fileName() )
                              .arg( mVersion );
            mpHost->postMessage( infoMsg );
            mSaveVersion = mVersion; // Make the save version the default one - unless the user intervenes
        }

        // As all but the room reading have version checks the fact that sub-4
        // files will still be parsed despite canRestore being false is probably OK
        if( mVersion >= 4 ) {
            ifs >> envColors;
            mpRoomDB->restoreAreaMap(ifs);
        }
        if( mVersion >= 5 ) {
            ifs >> customEnvColors;
        }
        if( mVersion >= 7 ) {
            ifs >> mpRoomDB->hashTable;
        }
        if( mVersion >= 17 ) {
            ifs >> mUserData;
        }
        if( mVersion >= 14 ) {
            int areaSize;
            ifs >> areaSize;
            // restore area table
            for( int i=0; i<areaSize; i++ ) {
                TArea * pA = new TArea( this, mpRoomDB );
                int areaID;
                ifs >> areaID;
                if( mVersion >= 18 ) {
                    // In version 18 changed from QList<int> to QSet<int> as the later is
                    // faster in many of the cases where we use it.
                    ifs >> pA->rooms;
                }
                else {
                    QList<int> oldRoomsList;
                    ifs >> oldRoomsList;
                    pA->rooms = oldRoomsList.toSet();
                }
// Can be useful when analysing suspect map files!
//                qDebug() << "TMap::restore(...)" << "Area:" << areaID;
//                qDebug() << "Rooms:" << pA->rooms;
                ifs >> pA->ebenen;
                ifs >> pA->exits;
                ifs >> pA->gridMode;
                ifs >> pA->max_x;
                ifs >> pA->max_y;
                ifs >> pA->max_z;
                ifs >> pA->min_x;
                ifs >> pA->min_y;
                ifs >> pA->min_z;
                ifs >> pA->span;
                if( mVersion >= 17 ) {
                    ifs >> pA->xmaxEbene;
                    ifs >> pA->ymaxEbene;
                    ifs >> pA->xminEbene;
                    ifs >> pA->yminEbene;
                }
                else {
                    QMap<int, int> dummyMinMaxEbene;
                    ifs >> pA->xmaxEbene;
                    ifs >> pA->ymaxEbene;
                    ifs >> dummyMinMaxEbene;
                    ifs >> pA->xminEbene;
                    ifs >> pA->yminEbene;
                    ifs >> dummyMinMaxEbene;
                }
                ifs >> pA->pos;
                ifs >> pA->isZone;
                ifs >> pA->zoneAreaRef;
                if( mVersion >= 17 ) {
                    ifs >> pA->mUserData;
                }
                mpRoomDB->restoreSingleArea( ifs, areaID, pA );
            }
        }

        if( mVersion >= 18 ) {
            // In version 18 we changed to store the "userRoom" for each profile
            // so that when copied/shared between profiles they do not interfere
            // with each other's saved value
            ifs >> mRoomIdHash;
        }
        else if( mVersion >= 12 ) {
            int oldRoomId;
            ifs >> oldRoomId;
            mRoomIdHash[ mpHost->getName() ] = oldRoomId;
        }

        if( mVersion >= 11 ) {
            int size;
            ifs >> size; //size of mapLabels
            int areaLabelCount = 0;
            while( ! ifs.atEnd() && areaLabelCount < size ) {
                int areaID;
                int size_labels;
                ifs >> size_labels;
                ifs >> areaID;
                int labelCount = 0;
                QMap<int, TMapLabel> _map;
                while( ! ifs.atEnd() &&  labelCount < size_labels ) {
                    int labelID;
                    ifs >> labelID;
                    TMapLabel label;
                    if( mVersion >= 12 ) {
                        ifs >> label.pos;
                    }
                    else {
                        QPointF __label_pos;
                        ifs >> __label_pos;
                        label.pos = QVector3D(__label_pos.x(), __label_pos.y(), 0);
                    }
                    ifs >> label.pointer;
                    ifs >> label.size;
                    ifs >> label.text;
                    ifs >> label.fgColor;
                    ifs >> label.bgColor;
                    ifs >> label.pix;
                    if( mVersion >= 15 ) {
                        ifs >> label.noScaling;
                        ifs >> label.showOnTop;
                    }
                    _map.insert( labelID, label );
                    labelCount++;
                }
                mapLabels[areaID] = _map;
                areaLabelCount++;
            }
        }

        while( ! ifs.atEnd() ) {
            int i;
            ifs >> i;
            TRoom * pT = new TRoom(mpRoomDB);
            pT->restore( ifs, i, mVersion );
            mpRoomDB->restoreSingleRoom( ifs, i, pT );
        }

        customEnvColors[257] = mpHost->mRed_2;
        customEnvColors[258] = mpHost->mGreen_2;
        customEnvColors[259] = mpHost->mYellow_2;
        customEnvColors[260] = mpHost->mBlue_2;
        customEnvColors[261] = mpHost->mMagenta_2;
        customEnvColors[262] = mpHost->mCyan_2;
        customEnvColors[263] = mpHost->mWhite_2;
        customEnvColors[264] = mpHost->mBlack_2;
        customEnvColors[265] = mpHost->mLightRed_2;
        customEnvColors[266] = mpHost->mLightGreen_2;
        customEnvColors[267] = mpHost->mLightYellow_2;
        customEnvColors[268] = mpHost->mLightBlue_2;
        customEnvColors[269] = mpHost->mLightMagenta_2;
        customEnvColors[270] = mpHost->mLightCyan_2;
        customEnvColors[271] = mpHost->mLightWhite_2;
        customEnvColors[272] = mpHost->mLightBlack_2;

        qDebug() << "TMap::restore(...) loading time:" << _time.nsecsElapsed() * 1.0e-9 << "sec.";
        QString okMsg = tr( "[  OK  ]  - Sucessfully read map file, will now check some consistancy details." );
        mpHost->postMessage( okMsg );
        if( canRestore ) {
            return true;
        }
    }

    if( ! canRestore || entries.size() == 0 ) {
        QMessageBox msgBox;

        if(    mpHost->mUrl.toLower().contains( "achaea.com" )
            || mpHost->mUrl.toLower().contains( "aetolia.com" )
            || mpHost->mUrl.toLower().contains( "imperian.com" )
            || mpHost->mUrl.toLower().contains( "midkemiaonline.com" )
            || mpHost->mUrl.toLower().contains( "lusternia.com" ) ) {

            msgBox.setText("No map found. Would you like to download the map or start your own?");
            QPushButton *yesButton = msgBox.addButton("Download the map", QMessageBox::ActionRole);
            QPushButton *noButton = msgBox.addButton("Start my own", QMessageBox::ActionRole);
            msgBox.exec();
            init( mpHost );
            if( msgBox.clickedButton() == yesButton ) {
                mpMapper->downloadMap();
            }
            else if( msgBox.clickedButton() == noButton ) {
                ; //No-op to avoid unused "noButton"
            }
        }
        else {
            mpHost->mpMap->init( mpHost );
        }
    }

    return canRestore;//FIXME
}

// Reads the newest map file from the profile and retrieves some stats and data,
// including the current player room - was mRoomId in 12 to pre-18 map files and
// is in mRoomIdHash since then so that it can be reinserted into a map that is
// copied across (if the room STILL exists!  This is to avoid a replacement map
// (copied/shared) from one profile to another from repositioning the other
// player location. Though this is written as a member function it is intended
// also for use to retrieve details from maps from OTHER profiles, importantly
// it does (or should) NOT interact with this TMap instance...!
const bool TMap::retrieveMapFileStats( QString profile, QString * latestFileName = 0, int * fileVersion = 0, int * roomId = 0, int * areaCount = 0, int * roomCount = 0 )
{
    if( profile.isEmpty() ) {
        return false;
    }

    QString folder;
    QStringList entries;
    folder = QStringLiteral( "%1/.config/mudlet/profiles/%2/map/" )
             .arg( QDir::homePath() )
             .arg( profile );
    QDir dir( folder );
    dir.setSorting( QDir::Time );
    entries = dir.entryList( QDir::Files|QDir::NoDotAndDotDot, QDir::Time );

    // As the files are sorted by time this gets the latest one
    QFile file( QStringLiteral( "%1%2" ).arg( folder ).arg( entries.at( 0 ) ) );

    if( ! file.open( QFile::ReadOnly ) ) {
        QString errMsg = tr( "[ ERROR ] - Unable to open (for reading) map file: \"%1\"!" )
                         .arg( file.fileName() );
        mpHost->postMessage( errMsg );
        return false;
    }

    if( latestFileName ) {
        *latestFileName = file.fileName();
    }
    int otherProfileVersion = 0;
    QDataStream ifs( & file );
    ifs >> otherProfileVersion;

    QString infoMsg = tr( "[ INFO ]  - Checking map file: \"%1\", format version:%2, please wait..." )
                      .arg( file.fileName() )
                      .arg( otherProfileVersion );
    mpHost->postMessage( infoMsg );

    if( otherProfileVersion > mDefaultVersion ) {
        if( QByteArray( APP_BUILD ).isEmpty() ) {
            // This is a release version - should not support any map file versions higher that it was built for
            if( fileVersion ) {
                * fileVersion = otherProfileVersion;
            }
            file.close();
            return true;
        }
        else {
            // Is a development version so check against mMaxVersion
            if( otherProfileVersion > mMaxVersion ) {
                // Oh dear, can't handle THIS
                if( fileVersion ) {
                    * fileVersion = otherProfileVersion;
                }
                file.close();
                return true;
            }
            else {
                if( fileVersion ) {
                    * fileVersion = otherProfileVersion;
                }
            }
        }
    }
    else {
        if( fileVersion ) {
            * fileVersion = otherProfileVersion;
        }
    }

    if( otherProfileVersion >= 4 ) {
        // envColorMap
        QMap<int, int> _dummyQMapIntInt;
        ifs >> _dummyQMapIntInt;

        // AreaNamesMap
        QMap<int, QString> _dummyQMapIntQString;
        ifs >> _dummyQMapIntQString;
    }

    if( otherProfileVersion >= 5 ) {
        // customEnvColors
        QMap<int, QColor> _dummyQMapIntQColor;
        ifs >> _dummyQMapIntQColor;
    }

    if( otherProfileVersion >= 7 ) {
        // hashTable
        QHash<QString, int> _dummyQHashQStringInt;
        ifs >> _dummyQHashQStringInt;
    }

    if( otherProfileVersion >= 17 ) {
        // userMapData
        QMap<QString, QString> _dummyQMapQStringQString;
        ifs >> _dummyQMapQStringQString;
    }

    if( otherProfileVersion >= 14 ) {
        int areaSize;
        ifs >> areaSize;
        if( areaCount ) {
            * areaCount = areaSize;
        }
        // read each area
        for( int i = 0; i < areaSize; i++ ) {
            TArea * pA = new TArea( 0, 0 );
            int areaID;
            ifs >> areaID;
            ifs >> pA->rooms;
            ifs >> pA->ebenen;
            ifs >> pA->exits;
            ifs >> pA->gridMode;
            ifs >> pA->max_x;
            ifs >> pA->max_y;
            ifs >> pA->max_z;
            ifs >> pA->min_x;
            ifs >> pA->min_y;
            ifs >> pA->min_z;
            ifs >> pA->span;
            if( otherProfileVersion >= 17 ) {
                ifs >> pA->xmaxEbene;
                ifs >> pA->ymaxEbene;
                ifs >> pA->xminEbene;
                ifs >> pA->yminEbene;
            }
            else {
                QMap<int, int> dummyMinMaxEbene;
                ifs >> pA->xmaxEbene;
                ifs >> pA->ymaxEbene;
                ifs >> dummyMinMaxEbene;
                ifs >> pA->xminEbene;
                ifs >> pA->yminEbene;
                ifs >> dummyMinMaxEbene;
            }
            ifs >> pA->pos;
            ifs >> pA->isZone;
            ifs >> pA->zoneAreaRef;
            if( otherProfileVersion >= 17 ) {
                ifs >> pA->mUserData;
            }
        }
    }

    if( otherProfileVersion >= 18 ) {
        // In version 18 we changed to store the "userRoom" for each profile
        // so that when copied/shared between profiles they do not interfere
        // with each other's saved value
        QHash<QString, int> _dummyQHashQStringInt;
        ifs >> _dummyQHashQStringInt;
        if( roomId ) {
           *roomId = _dummyQHashQStringInt.value( profile );
        }
    }
    else if( otherProfileVersion >= 12 ) {
        int oldRoomId;
        ifs >> oldRoomId;
        if( roomId ) {
           *roomId = oldRoomId;
        }
    }
    else {
        if( roomId ) {
           *roomId = -1; // Not found value
        }
    }

    if( otherProfileVersion >= 11 ) {
        int size;
        ifs >> size; //size of mapLabels
        int areaLabelCount = 0;
        while( ! ifs.atEnd() && areaLabelCount < size ) {
            int areaID;
            int size_labels;
            ifs >> size_labels;
            ifs >> areaID;
            int labelCount = 0;
            while( ! ifs.atEnd() &&  labelCount < size_labels ) {
                int labelID;
                ifs >> labelID;
                TMapLabel label;
                if( otherProfileVersion >= 12 ) {
                    ifs >> label.pos;
                }
                else {
                    QPointF __label_pos;
                    ifs >> __label_pos;
                    label.pos = QVector3D(__label_pos.x(), __label_pos.y(), 0);
                }
                ifs >> label.pointer;
                ifs >> label.size;
                ifs >> label.text;
                ifs >> label.fgColor;
                ifs >> label.bgColor;
                ifs >> label.pix;
                if( otherProfileVersion >= 15 ) {
                    ifs >> label.noScaling;
                    ifs >> label.showOnTop;
                }
                labelCount++;
            }
            areaLabelCount++;
        }
    }

    TRoom * _pT = new TRoom( 0 );
    QSet<int> _dummyRoomIdSet;
    while( ! ifs.atEnd() ) {
        int i;
        ifs >> i;
        _pT->restore( ifs, i, otherProfileVersion );
        // Can't do mpRoomDB->restoreSingleRoom( ifs, i, pT ) as it would mess up
        // this TMap::mpRoomDB
        // So emulate using _dummyRoomIdSet
        if( i > 0 && ! _dummyRoomIdSet.contains( i ) ) {
            _dummyRoomIdSet.insert( i );
        }
    }
    if( roomCount ) {
        *roomCount = _dummyRoomIdSet.count();
    }

    return true;
}

int TMap::createMapLabel(int area, QString text, float x, float y, float z, QColor fg, QColor bg, bool showOnTop, bool noScaling, qreal zoom, int fontSize )
{
    if( ! mpRoomDB->getArea( area ) ) return -1;

    TMapLabel label;
    label.text = text;
    label.bgColor = bg;
    label.bgColor.setAlpha(50);
    label.fgColor = fg;
    label.size = QSizeF(100,100);
    label.pos = QVector3D( x, y, z);
    label.showOnTop = showOnTop;
    label.noScaling = noScaling;

    if( label.text.length() < 1 )
    {
        return -1;
    }
    QRectF lr = QRectF( 0, 0, 1000, 1000 );
    QPixmap pix( lr.size().toSize() );
    pix.fill(QColor(0,0,0,0));
    QPainter lp( &pix );
    lp.fillRect( lr, label.bgColor );
    QPen lpen;
    lpen.setColor( label.fgColor );
    QFont font;
    font.setPointSize(fontSize); //good: font size = 50, zoom = 30.0
    lp.setRenderHint(QPainter::TextAntialiasing, true);
    lp.setPen( lpen );
    lp.setFont(font);
    QRectF br;
    lp.drawText( lr, Qt::AlignLeft|Qt::AlignTop, label.text, &br );

    label.size = br.normalized().size();
    label.pix = pix.copy(br.normalized().topLeft().x(), br.normalized().topLeft().y(), br.normalized().width(), br.normalized().height());
    QSizeF s = QSizeF(label.size.width()/zoom, label.size.height()/zoom);
    label.size = s;
    label.clickSize = s;
    if( ! mpRoomDB->getArea(area) ) return -1;
    int labelID;
    if( !mapLabels.contains( area ) )
    {
        QMap<int, TMapLabel> m;
        m[0] = label;
        mapLabels[area] = m;
    }
    else
    {
        labelID = createMapLabelID( area );
        if( labelID > -1 )
        {
            mapLabels[area].insert(labelID, label);
        }
    }

    if( mpMapper ) mpMapper->mp2dMap->update();
    return labelID;
}

int TMap::createMapImageLabel(int area, QString imagePath, float x, float y, float z, float width, float height, float zoom, bool showOnTop, bool noScaling )
{
    if( ! mpRoomDB->getArea( area ) ) return -1;

    TMapLabel label;
    label.size = QSizeF(width, height);
    label.pos = QVector3D( x, y, z);
    label.showOnTop = showOnTop;
    label.noScaling = noScaling;

    QRectF drawRect = QRectF( 0, 0, width*zoom, height*zoom );
    QPixmap imagePixmap = QPixmap(imagePath);
    QPixmap pix = QPixmap( drawRect.size().toSize() );
    pix.fill(QColor(0,0,0,0));
    QPainter lp( &pix );
    lp.drawPixmap(QPoint(0,0), imagePixmap.scaled(drawRect.size().toSize()));
    label.size = QSizeF(width, height);
    label.pix = pix;
    if( ! mpRoomDB->getArea(area) ) return -1;
    int labelID;
    if( !mapLabels.contains( area ) )
    {
        QMap<int, TMapLabel> m;
        m[0] = label;
        mapLabels[area] = m;
    }
    else
    {
        labelID = createMapLabelID( area );
        if( labelID > -1 )
        {
            mapLabels[area].insert(labelID, label);
        }
    }

    if( mpMapper ) mpMapper->mp2dMap->update();
    return labelID;
}


int TMap::createMapLabelID(int area )
{
    if( mapLabels.contains( area ) )
    {
        QList<int> idList = mapLabels[area].keys();
        int id = 0;
        while( id >= 0 )
        {
            if( !idList.contains( id ) )
            {
                return id;
            }
            id++;
        }
    }
    return -1;
}

void TMap::deleteMapLabel(int area, int labelID )
{
    if( ! mpRoomDB->getArea( area ) ) return;
    if( ! mapLabels.contains( area ) ) return;
    if( ! mapLabels[area].contains( labelID ) ) return;
    mapLabels[area].remove( labelID );
    if( mpMapper ) mpMapper->mp2dMap->update();
}
