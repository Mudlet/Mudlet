/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
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


#include "Host.h"
#include "TArea.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "XMLimport.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QProgressDialog>
#include <QSslConfiguration>
#include "post_guard.h"


TMap::TMap( Host * pH )
: mpRoomDB( new TRoomDB( this ) )
, mpHost( pH )
, m2DPanMode( false )
, mLeftDown( false )
, mRightDown( false )
, m2DPanXStart( 0.0f )
, m2DPanYStart( 0.0f )
, mTargetID( 0 )
, mpM( Q_NULLPTR )
, mpMapper( Q_NULLPTR )
, mMapGraphNeedsUpdate( true )
, mNewMove( true )
// default map version that new maps will get
, mDefaultVersion( 18 )
// maximum version of the map format that this Mudlet can understand and will
// allow the user to load
, mMaxVersion( 18 )
// minimum version this instance of Mudlet will allow the user to save maps in
, mMinVersion( 16 )
, mIsFileViewingRecommended( false )
, mpNetworkAccessManager( Q_NULLPTR )
, mpProgressDialog( Q_NULLPTR )
, mpNetworkReply( Q_NULLPTR )
, mExpectedFileSize( 0 )
{
    mSaveVersion = mDefaultVersion; // Can not be set initialiser list because of ordering issues (?)
                                    // It needs to be set (for when writing new
                                    // map files) as it triggers some version features
                                    // that NEED a new map file format to be usable, it
                                    // can be changed by control in last tab of profile
                                    // preference dialog.
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

    // According to Qt Docs we should really only have one of these
    // (QNetworkAccessManager) for the whole application, but: each profile's
    // TLuaInterpreter; each profile's ctelnet and now each profile's TMap
    // (was dlgMapper) instance has one...!
    mpNetworkAccessManager = new QNetworkAccessManager( this );

    connect( mpNetworkAccessManager, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( slot_replyFinished( QNetworkReply * ) ) );
}

TMap::~TMap()
{
    delete mpRoomDB;
    if( ! mStoredMessages.isEmpty() ) {
        qWarning() << "TMap::~TMap() Instance being destroyed before it could display some messages,\n"
                   << "messages are:\n"
                   << "------------";
        foreach(QString message, mStoredMessages) {
            qWarning() << message
                       << "\n------------";
        }
    }
}

void TMap::mapClear()
{
    mpRoomDB->clearMapDB();
    envColors.clear();
    mRoomIdHash.clear();
    mTargetID = 0;
    mPathList.clear();
    mDirList.clear();
    mWeightList.clear();
    customEnvColors.clear();
    // Need to restore the default colours:
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
    roomidToIndex.clear();
// Not used:    pixNameTable.clear();
// Not used:    pixTable.clear();
    edgeHash.clear();
    locations.clear();
    mMapGraphNeedsUpdate = true;
    mNewMove = true;
    mapLabels.clear();
    mVersion = mDefaultVersion;
    mUserData.clear();
    // mSaveVersion is not reset - so that any new Mudlet map file saves are to
    // whatever version was previously set/deduced
}

void TMap::logError( QString & msg )
{
    auto orange = QColor(255,128,0);
    auto black = QColor(Qt::black);
    QString s1 = QString("[MAP ERROR:]%1\n").arg(msg);
    if( mpHost->mpEditorDialog )
    {
        mpHost->mpEditorDialog->mpErrorConsole->printDebug(orange, black, s1 );
    }
}

// Not used:
//void TMap::exportMapToDatabase()
//{
//    QString dbName = QFileDialog::getSaveFileName( 0, "Chose db file name." );
//    QString script = QString("exportMapToDatabse([[%1]])").arg(dbName);
//    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
//}

//void TMap::importMapFromDatabase()
//{
//    QString dbName = QFileDialog::getOpenFileName( 0, "Chose db file name." );
//    QString script = QString("importMapFromDatabase([[%1]])").arg(dbName);
//    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
//}

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

int TMap::createNewRoomID( int minimumId )
{
    int _id = 0;
    if( minimumId > 0 ) {
        _id = minimumId - 1;
    }

    do {
        ; // Empty loop as increment done in test
    } while( mpRoomDB->getRoom( ++_id ) );

    return _id;
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

void TMap::audit()
{
    // init areas
    QElapsedTimer _time;
    _time.start();

    { // Blocked - just to limit the scope of infoMsg...!
        QString infoMsg = tr( "[ INFO ]  - Auditing of a loaded/imported/downloaded map starting..." );
        postMessage( infoMsg );
    }

    // The old mpRoomDB->initAreasForOldMaps() was a subset of these checks

    QHash<int, int> roomRemapping; // These are populated by the auditRooms(...)
    QHash<int, int> areaRemapping; // call and contain "Keys" of old ids and
                                   // "Values" of new ids to use in their stead

    if( mVersion <16 ) {
        // convert old style labels, wasn't made version conditional in past but
        // not likely to be an issue in recent map file format versions (say 16+)

        QMapIterator<int, TArea *> itArea( mpRoomDB->getAreaMap() );
        while( itArea.hasNext() ) {
            itArea.next();
            int areaID = itArea.key();
            if( mapLabels.contains(areaID) ) {
                QList<int> labelIDList = mapLabels.value(areaID).keys();
                for(int & i : labelIDList) {
                    TMapLabel l = mapLabels.value(areaID).value(i);
                    if( l.pix.isNull() ) {
                        int newID = createMapLabel(areaID, l.text, l.pos.x(), l.pos.y(), l.pos.z(), l.fgColor, l.bgColor, true, false, 40.0, 50 );
                        if( newID > -1 ) {
                            if( mudlet::self()->getAuditErrorsToConsoleEnabled() ) {
                                QString msg = tr( "[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2." )
                                              .arg(areaID)
                                              .arg(i );
                                postMessage(msg);
                            }
                            appendAreaErrorMsg( areaID, tr( "[ INFO ] - Converting old style label id: %1." )
                                                            .arg( i ) );
                            mapLabels[areaID][i] = mapLabels[areaID][newID];
                            deleteMapLabel( areaID, newID );
                        }
                        else {
                            if( mudlet::self()->getAuditErrorsToConsoleEnabled() ) {
                                QString msg = tr( "[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2." )
                                              .arg(areaID)
                                              .arg(i);
                                postMessage(msg);
                            }
                            appendAreaErrorMsg( areaID, tr( "[ WARN ] - CONVERTING: cannot convert old style label with id: %1." )
                                                            .arg( i ) );
                        }
                    }
                    if (    ( l.size.width() >  std::numeric_limits<qreal>::max() )
                         || ( l.size.width() < -std::numeric_limits<qreal>::max() ) ) {
                        mapLabels[areaID][i].size.setWidth(l.pix.width());
                    }
                    if (    ( l.size.height() >  std::numeric_limits<qreal>::max() )
                         || ( l.size.height() < -std::numeric_limits<qreal>::max() ) ) {
                        mapLabels[areaID][i].size.setHeight(l.pix.height());
                    }
                }
            }
        }
    }

    mpRoomDB->auditRooms( roomRemapping, areaRemapping );

    // The second half of old mpRoomDB->initAreasForOldMaps() - needed to fixup
    // all the (TArea *)->areaExits() that were built wrongly previously,
    // calcSpan() may not be required to be done here and now but it is in my
    // sights as a target for revision in the future. Slysven
    QMapIterator<int, TArea *> itArea( mpRoomDB->getAreaMap() );
    while( itArea.hasNext() ) {
        itArea.next();
        itArea.value()->determineAreaExits();
        itArea.value()->calcSpan();
        itArea.value()->mIsDirty = false;
    }

    { // Blocked - just to limit the scope of infoMsg...!
        QString infoMsg = tr( "[  OK  ]  - Auditing of map completed (%1s). Enjoy your game..." )
                              .arg( _time.nsecsElapsed() * 1.0e-9, 0, 'f', 2 );
        postMessage( infoMsg );
        appendErrorMsg( infoMsg );
    }
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

// Not used:
//void TMap::astBreitenAnpassung( int id, int id2 )
//{
//}

//void TMap::astHoehenAnpassung( int id, int id2 )
//{
//}

//void TMap::getConnectedNodesGreaterThanX( int id, int min )
//{
//}

//void TMap::getConnectedNodesSmallerThanX( int id, int min )
//{
//}

//void TMap::getConnectedNodesGreaterThanY( int id, int min )
//{
//}

//void TMap::getConnectedNodesSmallerThanY( int id, int min )
//{
//}

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
            default:               qWarning() << "TMap::findPath(" << from << "," << to << ") WARN: found route between rooms (from id:" << previousRoomId << ", to id:" << currentRoomId << ") with an invalid DIR_xxxx code:" << r.direction << " - the path will not be valid!" ;
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
        appendErrorMsgWithNoLf( message, false );
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
        appendErrorMsgWithNoLf( message, false );
        postMessage( message );
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
            appendErrorMsgWithNoLf( message, false );
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
            int areaId = areasWithData.takeFirst();
            areaIds.append( QString::number( areaId ) );
            appendAreaErrorMsg( areaId, tr( "[ ALERT ] - User data for this area has been lost in saved map file.  Re-save in a\n"
                                                        "format of at least 17 to preserve it before quitting!\n" ) );
        } while( ! areasWithData.isEmpty() );

        QString message = tr( "[ ALERT ] - Area User data has been lost in saved map file.  Re-save in a\n"
                                          "format of at least 17 to preserve it before quitting!\n"
                                          "Areas id affected: %1." )
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

bool TMap::restore( QString location )
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
        folder = QStringLiteral("%1/.config/mudlet/profiles/%2/map/").arg(QDir::homePath(), mpHost->getName());
        QDir dir( folder );
        dir.setSorting( QDir::Time );
        entries = dir.entryList( QDir::Files, QDir::Time );
    }

    bool canRestore = true;
    if( entries.size() || ! location.isEmpty() ) {
        QFile file(   location.isEmpty()
                    ? QStringLiteral( "%1%2" ).arg(folder, entries.at(0) )
                    : location );

        if( ! file.open( QFile::ReadOnly ) ) {
            QString errMsg = tr( "[ ERROR ] - Unable to open (for reading) map file: \"%1\"!" )
                             .arg( file.fileName() );
            appendErrorMsg( errMsg, false );
            postMessage( errMsg );
            return false;
        }

        QDataStream ifs( & file );
        ifs >> mVersion;
        if( mVersion > mMaxVersion ) {
            QString errMsg = tr( "[ ERROR ] - Map file is too new, it's file format (%1) is higher than this version of\n"
                                 "Mudlet can handle (%2)!  The file is:\n\"%3\"." )
                    .arg( mVersion )
                    .arg( mMaxVersion )
                    .arg( file.fileName() );
            appendErrorMsgWithNoLf( errMsg );
            postMessage( errMsg );
            QString infoMsg = tr( "[ INFO ]  - You will need to upgrade your Mudlet or find a map file saved in an\n"
                                  "older format." );
            appendErrorMsgWithNoLf( infoMsg );
            postMessage( infoMsg );
            file.close();
            return false;
        } else if( mVersion < 4 ) {
            QString alertMsg = tr( "[ ALERT ] - Map file is really old, it's file format (%1) is so ancient that\n"
                                               "this version of Mudlet may not gain enough information from\n"
                                               "it but it will try!  The file is: \"%2\"." )
                               .arg( mVersion )
                               .arg( file.fileName() );
            appendErrorMsgWithNoLf( alertMsg, false );
            postMessage( alertMsg );
            QString infoMsg = tr( "[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!\n"
                                              "There is so much data that it DOES NOT have that you could be\n"
                                              "be better off starting again..." );
            appendErrorMsgWithNoLf( infoMsg, false );
            postMessage( infoMsg );
            canRestore = false;
            mSaveVersion = mVersion; // Make the save version the default one - unless the user intervenes
        }
        else {
            // Less than (but not less than 4) or equal to default version
            QString infoMsg = tr( "[ INFO ]  - Reading map (format version:%1) file:\n\"%2\",\nplease wait..." )
                                  .arg( mVersion )
                                  .arg( file.fileName() );
            appendErrorMsg( tr( "[ INFO ]  - Reading map (format version:%1) file: \"%2\"." )
                                .arg( mVersion )
                                .arg( file.fileName() ), false );
            postMessage( infoMsg );
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
                auto pA = new TArea( this, mpRoomDB );
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
                mpRoomDB->restoreSingleArea( areaID, pA );
            }
        }

        if( ! mpRoomDB->getAreaMap().keys().contains( -1 ) ) {
            auto pDefaultA = new TArea( this, mpRoomDB );
            mpRoomDB->restoreSingleArea( -1, pDefaultA );
            QString defaultAreaInsertionMsg = tr( "[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an\n"
                                                              "area) not found, adding reserved -1 id." );
            appendErrorMsgWithNoLf( defaultAreaInsertionMsg, false );
            if( mudlet::self()->getAuditErrorsToConsoleEnabled() ) {
                postMessage( defaultAreaInsertionMsg );
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
            auto pT = new TRoom(mpRoomDB);
            pT->restore( ifs, i, mVersion );
            mpRoomDB->restoreSingleRoom( i, pT );
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

        QString okMsg = tr( "[ INFO ]  - Sucessfully read the map file (%1s), checking some\n"
                                        "consistency details..." )
                            .arg( _time.nsecsElapsed() * 1.0e-9, 0, 'f', 2 );

        postMessage( okMsg );
        appendErrorMsgWithNoLf( okMsg );
        if( canRestore ) {
            return true;
        }
    }

    if( ! canRestore || entries.size() == 0 ) {
        QMessageBox msgBox;

        if( mpHost->mUrl.contains( QStringLiteral( "achaea.com" ), Qt::CaseInsensitive )
         || mpHost->mUrl.contains( QStringLiteral( "aetolia.com" ), Qt::CaseInsensitive )
         || mpHost->mUrl.contains( QStringLiteral( "imperian.com" ), Qt::CaseInsensitive )
         || mpHost->mUrl.contains( QStringLiteral( "lusternia.com" ), Qt::CaseInsensitive ) ) {

            msgBox.setText( tr( "No map found. Would you like to download the map or start your own?" ) );
            QPushButton *yesButton = msgBox.addButton( tr( "Download the map" ), QMessageBox::ActionRole );
            QPushButton *noButton = msgBox.addButton( tr( "Start my own" ), QMessageBox::ActionRole );
            msgBox.exec();
            if( msgBox.clickedButton() == yesButton ) {
                downloadMap();
            }
            else if( msgBox.clickedButton() == noButton ) {
                ; //No-op to avoid unused "noButton"
            }
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
bool TMap::retrieveMapFileStats( QString profile, QString * latestFileName = 0, int * fileVersion = 0, int * roomId = 0, int * areaCount = 0, int * roomCount = 0 )
{
    if( profile.isEmpty() ) {
        return false;
    }

    QString folder;
    QStringList entries;
    folder = QStringLiteral("%1/.config/mudlet/profiles/%2/map/").arg(QDir::homePath(), profile);
    QDir dir( folder );
    dir.setSorting( QDir::Time );
    entries = dir.entryList( QDir::Files|QDir::NoDotAndDotDot, QDir::Time );
    
    if ( entries.isEmpty() ) {
        return false;
    }

    // As the files are sorted by time this gets the latest one
    QFile file( QStringLiteral( "%1%2" ).arg(folder, entries.at( 0) ) );

    if( ! file.open( QFile::ReadOnly ) ) {
        QString errMsg = tr( "[ ERROR ] - Unable to open (for reading) map file: \"%1\"!" )
                         .arg( file.fileName() );
        appendErrorMsg( errMsg, false );
        postMessage( errMsg );
        return false;
    }

    if( latestFileName ) {
        *latestFileName = file.fileName();
    }
    int otherProfileVersion = 0;
    QDataStream ifs( & file );
    ifs >> otherProfileVersion;

    QString infoMsg = tr( "[ INFO ]  - Checking map file: \"%1\", format version:%2..." )
                      .arg( file.fileName() )
                      .arg( otherProfileVersion );
    appendErrorMsg( infoMsg, false );
    if( mudlet::self()->getAuditErrorsToConsoleEnabled() ) {
        postMessage( infoMsg );
    }

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
            TArea pA( 0, 0 );
            int areaID;
            ifs >> areaID;
            ifs >> pA.rooms;
            ifs >> pA.ebenen;
            ifs >> pA.exits;
            ifs >> pA.gridMode;
            ifs >> pA.max_x;
            ifs >> pA.max_y;
            ifs >> pA.max_z;
            ifs >> pA.min_x;
            ifs >> pA.min_y;
            ifs >> pA.min_z;
            ifs >> pA.span;
            if( otherProfileVersion >= 17 ) {
                ifs >> pA.xmaxEbene;
                ifs >> pA.ymaxEbene;
                ifs >> pA.xminEbene;
                ifs >> pA.yminEbene;
            }
            else {
                QMap<int, int> dummyMinMaxEbene;
                ifs >> pA.xmaxEbene;
                ifs >> pA.ymaxEbene;
                ifs >> dummyMinMaxEbene;
                ifs >> pA.xminEbene;
                ifs >> pA.yminEbene;
                ifs >> dummyMinMaxEbene;
            }
            ifs >> pA.pos;
            ifs >> pA.isZone;
            ifs >> pA.zoneAreaRef;
            if( otherProfileVersion >= 17 ) {
                ifs >> pA.mUserData;
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

    TRoom _pT(0);
    QSet<int> _dummyRoomIdSet;
    while( ! ifs.atEnd() ) {
        int i;
        ifs >> i;
        _pT.restore( ifs, i, otherProfileVersion );
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
    pix.fill(Qt::transparent);
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
    if( ! mpRoomDB->getArea(area) ) { return -1; }

    int label_id;

    // No labels exist for this area, so start from zero.
    if (!mapLabels.contains(area)) {
        QMap<int, TMapLabel> m;
        label_id = 0;
        m[label_id] = label;
        mapLabels[area] = m;
    } else {
        label_id = createMapLabelID(area);
        if (label_id > -1) {
            mapLabels[area].insert(label_id, label);
        }
    }

    if (mpMapper) {
        mpMapper->mp2dMap->update();
    }
    return label_id;
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
    pix.fill(Qt::transparent);
    QPainter lp( &pix );
    lp.drawPixmap(QPoint(0,0), imagePixmap.scaled(drawRect.size().toSize()));
    label.size = QSizeF(width, height);
    label.pix = pix;
    if (!mpRoomDB->getArea(area)) {
        return -1;
    }

    int label_id;

    // No labels exist for this area, so start from zero.
    if (!mapLabels.contains(area)) {
        QMap<int, TMapLabel> m;
        label_id = 0;
        m[label_id] = label;
        mapLabels[area] = m;
    } else {
        label_id = createMapLabelID(area);
        if (label_id > -1) {
            mapLabels[area].insert(label_id, label);
        }
    }

    if (mpMapper) {
        mpMapper->mp2dMap->update();
    }
    return label_id;
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

void TMap::postMessage( const QString text )
{
    mStoredMessages.append( text );
    Host * pHost = mpHost;
    if( pHost ) {
        while( ! mStoredMessages.isEmpty() ) {
            pHost->postMessage( mStoredMessages.takeFirst() );
        }
    }
}

// Used by the 2D mapper to send view center coordinates to 3D one
void TMap::set3DViewCenter( const int areaId, const int xPos, const int yPos, const int zPos )
{
    mpM->setViewCenter( areaId, xPos, yPos, zPos );
}

void TMap::appendRoomErrorMsg( const int roomId, const QString msg, const bool isToSetFileViewingRecommended )
{
    mMapAuditRoomErrors[ roomId ].append( msg );
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendAreaErrorMsg( const int areaId, const QString msg, const bool isToSetFileViewingRecommended )
{
    mMapAuditAreaErrors[ areaId ].append( msg );
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendErrorMsg( const QString msg, const bool isToSetFileViewingRecommended )
{
    mMapAuditErrors.append( msg );
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendErrorMsgWithNoLf( const QString msg, const bool isToSetFileViewingRecommended )
{
    QString text = msg;
    text.replace( QChar::LineFeed, QChar::Space );
    mMapAuditErrors.append( text );
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

const QString TMap::createFileHeaderLine( const QString title, const QChar fillChar )
{
    QString text;
    if( title.length() <= 76 ) {
        text = QStringLiteral("%1 %2 %1\n").arg(QString(fillChar).repeated((78 - title.length()) / 2), title);
    }
    else {
        text = title;
        text.append( QChar::LineFeed );
    }
    return text;
}

void TMap::pushErrorMessagesToFile( const QString title, const bool isACleanup )
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        qWarning() << "TMap::pushErrorMessagesToFile( ... ) ERROR: called with a NULL HOST pointer - something is wrong!";
        return;
    }

    // Replacement storage locations:
    QMap<int, QList<QString> >  mapAuditRoomErrors; // Key is room number (where renumbered is the original one), Value is the errors, appended as they are found
    QMap<int, QList<QString> >  mapAuditAreaErrors; // As for the Room ones but with key as the area number
    QList<QString>              mapAuditErrors;     // For the whole map
    // Switch message storage locations to freeze them so we can dump them to
    // file; according to Qt documentation "Swaps XXX other with this XXX. This
    // operation is very fast and never fails."
    mapAuditErrors.swap( mMapAuditErrors );
    mapAuditAreaErrors.swap( mMapAuditAreaErrors );
    mapAuditRoomErrors.swap( mMapAuditRoomErrors );

    if(    mapAuditErrors.isEmpty()
        && mapAuditAreaErrors.isEmpty()
        && mapAuditRoomErrors.isEmpty()
        && isACleanup ) {

        mIsFileViewingRecommended = false;
        return; // Nothing to do
    }

    pHost->mErrorLogStream << createFileHeaderLine( title, QLatin1Char( '#' ) );
    pHost->mErrorLogStream << createFileHeaderLine( tr( "Map issues" ), QLatin1Char( '=' ) );
    QListIterator<QString> itMapMsg( mapAuditErrors );
    while( itMapMsg.hasNext() ) {
        pHost->mErrorLogStream << itMapMsg.next() << QLatin1Char( '\n' );;
    }

    pHost->mErrorLogStream << createFileHeaderLine( tr( "Area issues" ), QLatin1Char( '=' ) );
    QMapIterator<int, QList<QString> > itAreasMsg( mapAuditAreaErrors );
    while( itAreasMsg.hasNext() ) {
        itAreasMsg.next();
        QString titleText;
        if( ! mpRoomDB->getAreaNamesMap().value( itAreasMsg.key() ).isEmpty() ) {
            titleText = tr( "Area id: %1 \"%2\"" )
                            .arg( itAreasMsg.key() )
                            .arg( mpRoomDB->getAreaNamesMap().value( itAreasMsg.key() ) );
        }
        else {
            titleText = tr( "Area id: %1" ).arg( itAreasMsg.key() );
        }
        pHost->mErrorLogStream << createFileHeaderLine( titleText, QLatin1Char( '-' ) );
        QListIterator<QString> itMapAreaMsg( itAreasMsg.value() );
        while( itMapAreaMsg.hasNext() ) {
            pHost->mErrorLogStream << itMapAreaMsg.next() << QLatin1Char( '\n' );
        }
    }

    pHost->mErrorLogStream << createFileHeaderLine( tr( "Room issues" ), QLatin1Char( '=' ) );
    QMapIterator<int, QList<QString> > itRoomsMsg( mapAuditRoomErrors );
    while( itRoomsMsg.hasNext() ) {
        itRoomsMsg.next();
        QString titleText;
        TRoom * pR = mpRoomDB->getRoom( itRoomsMsg.key() );
        if( pR && ! pR->name.isEmpty() ) {
            titleText = tr( "Room id: %1 \"%2\"" )
                            .arg( itRoomsMsg.key() )
                            .arg( pR->name );
        }
        else {
            titleText = tr( "Room id: %1" ).arg( itRoomsMsg.key() );
        }
        pHost->mErrorLogStream << createFileHeaderLine( titleText, QLatin1Char( '-' ) );
        QListIterator<QString> itMapRoomMsg( itRoomsMsg.value() );
        while( itMapRoomMsg.hasNext() ) {
            pHost->mErrorLogStream << itMapRoomMsg.next() << QLatin1Char( '\n' );;
        }
    }

    pHost->mErrorLogStream << createFileHeaderLine( tr( "End of report" ), QLatin1Char( '#' ) );
    pHost->mErrorLogStream.flush();
    mapAuditErrors.clear();
    mapAuditAreaErrors.clear();
    mapAuditRoomErrors.clear();
    if( mIsFileViewingRecommended && (! mudlet::self()->getAuditErrorsToConsoleEnabled() ) ) {
        postMessage( tr( "[ ALERT ] - At least one thing was detected during that last map operation\n"
                         "that it is recommended that you review the most recent report in the file:\n"
                         "\"%1\"\n"
                         "- look for the (last) report with the title:\n"
                         "\"%2\"." )
                         .arg( QStringLiteral( "%1/.config/mudlet/profiles/%2/log/errors.txt" )
                                               .arg( QDir::homePath(), mpHost->getName() ), title ) );
    }
    else if( mIsFileViewingRecommended && mudlet::self()->getAuditErrorsToConsoleEnabled() ) {
        postMessage( tr( "[ INFO ]  - The equivalent to the above information about that last map\n"
                         "operation has been saved for review as the most recent report in the file:\n"
                         "\"%1\"\n"
                         "- look for the (last) report with the title:\n"
                         "\"%2\"." )
                         .arg( QStringLiteral( "%1/.config/mudlet/profiles/%2/log/errors.txt" )
                                               .arg( QDir::homePath(), mpHost->getName() ), title ) );
    }

    mIsFileViewingRecommended = false;
}

void TMap::downloadMap( const QString * remoteUrl, const QString * localFileName )
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        return;
    }

    // Incidentally this should address: https://bugs.launchpad.net/mudlet/+bug/852861
    if( ! mXmlImportMutex.tryLock( 0 ) ) {
        QString warnMsg = QStringLiteral( "[ WARN ]  - Attempt made to download an XML map when one has already been\n"
                                                      "requested or is being imported from a local file - wait for that\n"
                                                      "operation to complete (if it cannot be canceled) before retrying!" );
        postMessage( warnMsg );
        return;
    }

    // We have the mutex locked - MUST unlock it when done under ALL circumstances
    QUrl url;

    if( ! remoteUrl || remoteUrl->isEmpty() ) {
        // TODO: Provide a per profile means to specify a "user settable" default Url...
        url = QUrl::fromUserInput( QStringLiteral( "https://www.%1/maps/map.xml" ).arg( pHost->mUrl ) );
    }
    else {
        url = QUrl::fromUserInput( *remoteUrl );
    }

    if( ! url.isValid() ) {
        QString errMsg = QStringLiteral( "[ WARN ]  - Attempt made to download an XML from an invalid URL.  The URL was:\n"
                                                     "%1\n"
                                                     "and the error message (may contain technical details) was:"
                                                     "\"%2\"." )
                         .arg( url.toString(), url.errorString() );
        postMessage( errMsg );
        mXmlImportMutex.unlock();
        return;
    }

    if( ! localFileName || localFileName->isEmpty() ) {
        mLocalMapFileName = QStringLiteral( "%1/.config/mudlet/profiles/%2/map.xml" )
                            .arg( QDir::homePath(), pHost->getName() );
    }
    else {
        mLocalMapFileName = *localFileName;
    }

    QNetworkRequest request = QNetworkRequest( url );
    // This should prevent similar problems to those mentioned in:
    // https://bugs.launchpad.net/mudlet/+bug/1366781 although the fix for THAT
    // is elsewhere and is to be inserted separately to the changeset that
    // placed this code here:
    request.setRawHeader( QByteArray( "User-Agent" ),
                          QByteArray( QStringLiteral( "Mozilla/5.0 (Mudlet/%1%2)" )
                                      .arg( APP_VERSION, APP_BUILD )
                                      .toUtf8().constData() ) );

#ifndef QT_NO_OPENSSL
    if( url.scheme() == QStringLiteral( "https" ) ) {
        QSslConfiguration config( QSslConfiguration::defaultConfiguration() );
        request.setSslConfiguration( config );
    }
#endif

    // Unfortunately we do not seem to get a file size from the IRE servers so
    // estimate it from current figures + 10% as of now (2016/10) - using previous
    // 4M that was used before for other cases:
    mExpectedFileSize = 4000000;
    if(      url.toString().contains( QStringLiteral( "achaea.com" ), Qt::CaseInsensitive ) ) {
        mExpectedFileSize = qRound( 1.1f * 4706442 );
    }
    else if( url.toString().contains( QStringLiteral( "aetolia.com" ), Qt::CaseInsensitive ) ) {
        mExpectedFileSize = qRound( 1.1f * 5695407 );
    }
    else if( url.toString().contains( QStringLiteral( "imperian.com" ), Qt::CaseInsensitive ) ) {
        mExpectedFileSize = qRound( 1.1f * 4997166 );
    }
    else if( url.toString().contains( QStringLiteral( "lusternia.com" ), Qt::CaseInsensitive ) ) {
        mExpectedFileSize = qRound( 1.1f * 4842063 );
    }

    QString infoMsg = tr( "[ INFO ]  - Map download initiated, please wait..." );
    postMessage( infoMsg );
    qApp->processEvents();
    // Attempts to ensure INFO message gets shown before download is initiated!

    mpNetworkReply = mpNetworkAccessManager->get( QNetworkRequest( QUrl( url ) ) );
    // Using zero for both min and max values should cause the bar to oscillate
    // until the first update
    mpProgressDialog = new QProgressDialog( tr( "Downloading XML map file for use in %1..." ).arg( pHost->getName() ), tr( "Abort" ), 0, 0 );
    mpProgressDialog->setWindowTitle( tr( "Map download" ) );
    mpProgressDialog->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_map_download.png" ) ) );
    mpProgressDialog->setMinimumWidth( 300 );
    mpProgressDialog->setAutoClose( false );
    mpProgressDialog->setAutoReset( false );
    mpProgressDialog->setMinimumDuration( 0 ); // Normally waits for 4 seconds before showing

    connect(mpNetworkReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( slot_setDownloadProgress( qint64, qint64 ) ) );
// Not used:    connect(mpNetworkReply, SIGNAL( readyRead() ), this, SLOT( slot_readyRead() ) );
    connect(mpNetworkReply, SIGNAL( error(QNetworkReply::NetworkError) ), this, SLOT( slot_downloadError( QNetworkReply::NetworkError ) ) );
// Not used:    connect(mpNetworkReply, SIGNAL( sslErrors( QList<QSslError> ) ), this, SLOT( slot_sslErrors( QList<QSslError> ) ) );
    connect(mpProgressDialog, SIGNAL( canceled() ), this, SLOT( slot_downloadCancel() ) );

    mpProgressDialog->show();
}

// Called from TLuaInterpreter::loadFile() or dlgProfilePreferences's "loadMap"
// both via TConsole::importMap( QFile & ) - it is intended to prevent
// readXmlMapFile( QFile & ) from being used more than once at a time and to
// prevent the above callers from using that when a map download is in progress!
// errMsg if, non-null is for a suitable structured error message to return to
// the TLuaInterpreter::loadFile(...) usage and is also needed to suppress the
// error message to the console
bool TMap::importMap( QFile & file, QString * errMsg )
{
    if( ! mXmlImportMutex.tryLock( 0 ) ) {
        if( errMsg ) {
            *errMsg = tr( "loadMap: unable to perform request, a map is already being downloaded or\n"
                          "imported at user request." );
        }
        else {
            QString warnMsg = QStringLiteral( "[ WARN ]  - Attempt made to import an XML map when one is already being\n"
                                                          "downloaded or is being imported from a local file - wait for that\n"
                                                          "operation to complete (if it cannot be canceled) before retrying!" );
            postMessage( warnMsg );
        }
        return false;
    }
    // We have the mutex and MUST unlock it when we are done

    bool result = readXmlMapFile( file, errMsg );

    // Finally release the lock on the XMLimporter
    mXmlImportMutex.unlock();

    return result;
}

bool TMap::readXmlMapFile( QFile & file, QString * errMsg )
{
    Host * pHost = mpHost;
    bool isLocalImport = false;
    if( ! pHost ) {
        return false;
    }

    if( ! mpProgressDialog ) {
        // This is the local import case - which has not got a progress dialog
        // until now:
        isLocalImport = true;
        mpProgressDialog = new QProgressDialog( tr( "Importing XML map file for use in %1..." ).arg( pHost->getName() ), QString(), 0, 0 );
        mpProgressDialog->setWindowTitle( tr( "Map import" ) );
        mpProgressDialog->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_map_download.png" ) ) );
        mpProgressDialog->setMinimumWidth( 300 );
        mpProgressDialog->setAutoClose( false );
        mpProgressDialog->setAutoReset( false );
        mpProgressDialog->setMinimumDuration( 0 ); // Normally waits for 4 seconds before showing
    }
    else {
        ; // This is the download file case which is a no-op
    }

    // It is NOW safe to delete the map as we are in a position to load one
    mapClear();

    XMLimport reader( pHost );
    bool result = reader.importPackage( & file );

    // probably not needed for the download but might be
    // needed for local file case:
    mpMapper->mp2dMap->init();
    // No need to call audit() as XMLimport::importPackage() does it!
    // audit() produces the successful ending [ OK ] message...!
    mpMapper->updateAreaComboBox();
    if( result ) {
        mpMapper->resetAreaComboBoxToPlayerRoomArea();
    }
    else {
        // Failed...
        if( errMsg ) {
            * errMsg = tr( "loadMap: failure to import XML map file, further information may be available\n"
                           "in main console!" );
        }
    }

    if( isLocalImport ) {
        // clean-up
        mpProgressDialog->deleteLater();
        mpProgressDialog = 0;
    }
    mpMapper->show();

    return result;
}

void TMap::slot_setDownloadProgress( qint64 got, qint64 tot )
{
    if( ! mpProgressDialog ) {
        return;
    }

    if( ! mpProgressDialog->maximum() ) {
        // First call, range has not been set;
        mpProgressDialog->setRange( 0, mExpectedFileSize );
    }
    else if( tot != -1 && mpProgressDialog->maximum() != static_cast<int>( tot ) ) {
        // tot will stuck at -1 when we do not know how big the download is
        // which seems to be the case for the IRE MUDS - *sigh* - Slysven
        mpProgressDialog->setRange( 0, static_cast<int>(tot) );
    }

    mpProgressDialog->setValue( static_cast<int>( got ) );
}

void TMap::slot_downloadCancel()
{
    QString alertMsg = tr( "[ ALERT ] - Map download was canceled, on user's request." );
    postMessage( alertMsg );
    if( mpProgressDialog ) {
        mpProgressDialog->deleteLater();
        mpProgressDialog = Q_NULLPTR; // Must reset this so it can be reused
    }
    if( mpNetworkReply ) {
        mpNetworkReply->abort(); // Will indirectly cause error() AND replyFinished signals to be sent
    }
}

void TMap::slot_downloadError( QNetworkReply::NetworkError error )
{
    if( ! mpNetworkReply ) {
        return;
    }

    if( error != QNetworkReply::OperationCanceledError ) {
        // No point in reporting Cancel as that is handled elsewhere
        QString errMsg = tr( "[ ERROR ] - Map download encountered an error:\n%1." ).arg( mpNetworkReply->errorString() );
        postMessage( errMsg );
    }
}

void TMap::slot_replyFinished( QNetworkReply * reply )
{
    if( reply != mpNetworkReply ) {
        qWarning() << "TMap::slot_replyFinished( QNetworkReply * ) ERROR - received argument was not the expected stored pointer.";
    }

    if( reply->error() != QNetworkReply::NoError ) {
        if( reply->error() != QNetworkReply::OperationCanceledError ) {
            // Don't post an error for the cancel case - it has already been done
            QString alertMsg = tr( "[ ALERT ] - Map download failed, error reported was:\n%1.").arg( reply->errorString() );
            postMessage( alertMsg );
        }
        // else was QNetworkReply::OperationCanceledError and we already handle
        // THAT in slot_downloadCancel()
    }
    else {
        QFile file( mLocalMapFileName );
        if( ! file.open( QFile::WriteOnly ) ) {
            QString alertMsg = tr( "[ ALERT ] - Map download failed, unable to open destination file:\n%1.").arg( mLocalMapFileName );
            postMessage( alertMsg );
        }
        else {
            // The QNetworkReply is Ok here:
            if( file.write( reply->readAll() ) == -1 ) {
                QString alertMsg = tr( "[ ALERT ] - Map download failed, unable to write destination file:\n%1.").arg( mLocalMapFileName );
                postMessage( alertMsg );
            }
            else {
                file.flush();
                file.close();

                if( file.open(QFile::ReadOnly | QFile::Text) ) {

                    QString infoMsg = tr(    "[ INFO ]  - ... map downloaded and stored, now parsing it..." );
                    postMessage( infoMsg );

                    Host * pHost = mpHost;
                    if( ! pHost ) {
                        qWarning() << "TMap::slot_replyFinished( QNetworkReply * ) ERROR - NULL Host pointer - something is really wrong!";
                        mXmlImportMutex.unlock();
                        return;
                    }

                    // Since the download is complete but we do not offer to
                    // cancel the required post-processing we should now hide
                    // the cancel/abort button:
                    mpProgressDialog->setCancelButton( Q_NULLPTR );

                    // The action to parse the XML file has been refactored to
                    // a separate method so that it can be shared with the
                    // direct importation of a local copy of a map file.

                    if( readXmlMapFile( file ) ) {
                        TEvent mapDownloadEvent;
                        mapDownloadEvent.mArgumentList.append(QLatin1String("sysMapDownloadEvent"));
                        mapDownloadEvent.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
                        pHost->raiseEvent( mapDownloadEvent );
                    }
                    else {
                        // Failure in parse file...
                        QString alertMsg = tr( "[ ERROR ] - Map download problem, failure in parsing destination file:\n%1.").arg( mLocalMapFileName );
                        postMessage( alertMsg );
                    }
                    file.close();
                }
                else {
                    QString alertMsg = tr( "[ ERROR ] - Map download problem, unable to read destination file:\n%1.").arg( mLocalMapFileName );
                    postMessage( alertMsg );
                }
            }
        }
    }
    reply->deleteLater();
    mpNetworkReply = Q_NULLPTR;

    // We don't delete the progress dialog until here as we now use it to inform
    // about post-download operations

    mpProgressDialog->deleteLater();
    mpProgressDialog = Q_NULLPTR; // Must reset this so it can be reused

    mLocalMapFileName.clear();
    mExpectedFileSize = 0;

    // We have finished with the XMLimporter so must release the lock on it
    mXmlImportMutex.unlock();
}

void TMap::reportStringToProgressDialog( const QString text )
{
    if( mpProgressDialog ) {
        mpProgressDialog->setLabelText( text );
        // Needed to make the changed text show, it does increase the overall
        // time a little but as the main usage is when parsing XML room data
        // and that can take MORE THAN A MINUTE the activity is essential to
        // inform the user that something IS happening...
        qApp->processEvents();
    }
}

void TMap::reportProgressToProgressDialog( const int current, const int maximum )
{
    if( mpProgressDialog ) {
        if( mpProgressDialog->maximum() != maximum  ) {
            mpProgressDialog->setMaximum( maximum );
        }
        mpProgressDialog->setValue( current );
    }
}
