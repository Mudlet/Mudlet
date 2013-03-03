/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
 *                                                                         *
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
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include "dlgMapper.h"


TMap::TMap( Host * pH )
: mpHost( pH )
, mpM( 0 )
, mpMapper( 0 )
, mMapGraphNeedsUpdate( true )
, mNewMove( true )
, version( 0 )
{
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
    float m2DPanXStart=0;
    float m2DPanYStart=0;
    int mViewArea = 0;
}

#include <QFileDialog>
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

void TMap::setRoomArea( int id, int area )
{
    if( ! rooms.contains( id ) ) return;
    if( areas.contains( rooms[id]->area ) )
    {
        areas[rooms[id]->area]->rooms.removeAll(id);
    }
    if( ! areas.contains( area ) )
    {
        areas[area] = new TArea(this);
    }

    rooms[id]->area = area;
    if( ! areas[area]->rooms.contains( id ) )
        areas[area]->rooms.push_back(id);
    areas[area]->fast_ausgaengeBestimmen(id);
    areas[area]->fast_calcSpan(id);
    mMapGraphNeedsUpdate = true;
}


void TMap::deleteRoom( int id )
{
    if( rooms.contains(id ) && id != 0 )
    {
        QMapIterator<int, TRoom *> it( rooms );
        while( it.hasNext() )
        {
            it.next();
            TRoom * r = it.value();
            if( r->north == id ) r->north = -1;
            if( r->northeast == id ) r->northeast = -1;
            if( r->northwest == id ) r->northwest = -1;
            if( r->east == id ) r->east = -1;
            if( r->west == id ) r->west = -1;
            if( r->south == id ) r->south = -1;
            if( r->southeast == id ) r->southeast = -1;
            if( r->southwest == id ) r->southwest = -1;
            if( r->up == id ) r->up = -1;
            if( r->down == id ) r->down = -1;
            if( r->in == id ) r->in = -1;
            if( r->out == id ) r->out = -1;
            r->other.remove( id );
        }
        TRoom * pR = rooms[id];
        int areaID = pR->area;
        if( areas.contains(areaID) )
        {
            TArea * pA = areas[areaID];

            pA->rooms.removeAll( id );
        }
        rooms.remove( id );
        mMapGraphNeedsUpdate = true;
        delete pR;
    }
    QList<QString> kL = hashTable.keys(id);
    for( int i=0; i<kL.size(); i++ )
    {
        hashTable.remove( kL[i] );
    }
}

void TMap::deleteArea( int id )
{
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
            deleteRoom( rl[i] );
        }
        areas.remove( id );

        mMapGraphNeedsUpdate = true;
    }
    areaNamesMap.remove( id );
}

bool TMap::addRoom( int id )
{
    TRoom * pT = new TRoom;

    if( ! rooms.contains( id ) && id != 0 )
    {
        rooms[id] = pT;
        pT->id = id;
        mMapGraphNeedsUpdate = true;
        return true;
    }
    else
    {
        delete pT;
        return false;
    }
}

bool TMap::setRoomCoordinates( int id, int x, int y, int z )
{
    if( ! rooms.contains( id ) ) return false;

    rooms[id]->x = x;
    rooms[id]->y = y;
    rooms[id]->z = z;

    return true;
}

int compSign(int a, int b){
    return (a < 0) == (b < 0);
}

void TMap::connectExitStub(int roomId, int dirType){
    int area = rooms[roomId]->area;
    int minDistance = 999999;
    int minDistanceRoom=0, meanSquareDistance=0;
    QVector3D unitVector = unitVectors[dirType];
    int ux = unitVector.x(), uy = unitVector.y(), uz = unitVector.z();
    int rx = rooms[roomId]->x, ry = rooms[roomId]->y, rz = rooms[roomId]->z;
    int dx=0,dy=0,dz=0;
    TArea * pA = areas[area];
    for( int i=0; i< pA->rooms.size(); i++ ){
        if (rooms[pA->rooms[i]]->id == roomId)
            continue;
        if (uz){
            dz = (int)rooms[pA->rooms[i]]->z-rz;
            if (!compSign(dz,uz) || !dz)
                continue;
        }
        else{
            //to avoid lower/upper floors from stealing stubs
            if ((int)rooms[pA->rooms[i]]->z != rz)
                continue;
        }
        if (ux){
            dx = (int)rooms[pA->rooms[i]]->x-rx;
            if (!compSign(dx,ux) || !dx) //we do !dx to make sure we have a component in the desired direction
                continue;
        }
        else{
            //to avoid rooms on same plane from stealing stubs
            if ((int)rooms[pA->rooms[i]]->x != rx)
                continue;
        }
        if (uy){
            dy = (int)rooms[pA->rooms[i]]->y-ry;
            //if the sign is the SAME here we keep it b/c we flip our y coordinate.
            if (compSign(dy,uy) || !dy)
                continue;
        }
        else{
            //to avoid rooms on same plane from stealing stubs
            if ((int)rooms[pA->rooms[i]]->y != ry)
                continue;
        }
        meanSquareDistance=dx*dx+dy*dy+dz*dz;
        if (meanSquareDistance < minDistance){
            minDistanceRoom=rooms[pA->rooms[i]]->id;
            minDistance=meanSquareDistance;
        }
    }
    if (minDistanceRoom){
        if (rooms[minDistanceRoom]->exitStubs.contains(reverseDirections[dirType])){
            setExit( roomId, minDistanceRoom, dirType);
            //rooms[roomId]->setExitStub(dirType, 0);
            setExit( minDistanceRoom, roomId, reverseDirections[dirType]);
            //rooms[minDistanceRoom]->setExitStub(reverseDirections[dirType], 0);
        }
    }
}

int TMap::createNewRoomID()
{
    int _id = 1;
    for( ; ; _id++ )
    {
        if( ! rooms.contains( _id ) )
        {
            return _id;
        }
    }
    return -1;
}

bool TMap::setExit( int from, int to, int dir )
{
    if( ! rooms.contains( from ) ) return false;
    if( to > 0 )
    {
        if( ! rooms.contains( to ) ) return false;
    }
    else
    {
        to = -1;
    }

    mPlausaOptOut = 0;

    switch( dir )
    {
        case DIR_NORTH: rooms[from]->north = to; break;
        case DIR_NORTHEAST: rooms[from]->northeast = to; break;
        case DIR_NORTHWEST: rooms[from]->northwest = to; break;
        case DIR_EAST: rooms[from]->east = to; break;
        case DIR_WEST: rooms[from]->west = to; break;
        case DIR_SOUTH: rooms[from]->south = to; break;
        case DIR_SOUTHEAST: rooms[from]->southeast = to; break;
        case DIR_SOUTHWEST: rooms[from]->southwest = to; break;
        case DIR_UP: rooms[from]->up = to; break;
        case DIR_DOWN: rooms[from]->down = to; break;
        case DIR_IN: rooms[from]->in = to; break;
        case DIR_OUT: rooms[from]->out = to; break;
        default: return false;
    }
    rooms[from]->setExitStub(dir, 0);
    mMapGraphNeedsUpdate = true;
    return true;
}

void TMap::init( Host * pH )
{
    // init areas
    QTime _time; _time.start();
    if( version < 14 )
    {
        areas.clear();
        buildAreas();

        // area mit raeumen fuellen
        QMapIterator<int, TRoom *> it( rooms );
        while( it.hasNext() )
        {
            it.next();
            int roomID = it.key();
            int areaID = rooms[roomID]->area;
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
    qDebug()<<" TMap::init() initialize area rooms: run time:"<<_time.elapsed();
    auditRooms();
    // convert old style labels
    QMapIterator<int, TArea *> it( areas );
    while( it.hasNext() )
    {
        it.next();
        TArea * pA = it.value();
        int areaID = it.key();
        if( mapLabels.contains(areaID) )
        {
            QList<int> labelIDList = mapLabels[areaID].keys();
            for( int i=0; i<labelIDList.size(); i++ )
            {
                TMapLabel l = mapLabels[areaID][labelIDList[i]];
                if( l.pix.isNull() )
                {
                    int newID = createMapLabel(areaID, l.text, l.pos.x(), l.pos.y(), l.pos.z(), l.fgColor, l.bgColor, true, false, 40.0, 50 );
                    if( newID > -1 )
                    {
                        cout << "CONVERTING: old style label areaID:"<<areaID<<" labelID:"<< labelIDList[i]<<endl;
                        mapLabels[areaID][labelIDList[i]] = mapLabels[areaID][newID];
                        deleteMapLabel( areaID, newID );
                    }
                    else
                        cout << "ERROR: cannot convert old style label areaID:"<<areaID<<" labelID:"<< labelIDList[i]<<endl;
                }
                if ( ( l.size.width() > std::numeric_limits<qreal>::max() ) || ( l.size.width() < -std::numeric_limits<qreal>::max() ) )
                {
                    mapLabels[areaID][labelIDList[i]].size.setWidth(l.pix.width());
                }
                if ( ( l.size.height() > std::numeric_limits<qreal>::max() ) || ( l.size.height() < -std::numeric_limits<qreal>::max() ) )
                {
                    mapLabels[areaID][labelIDList[i]].size.setHeight(l.pix.height());
                }
            }
        }
    }
}

void TMap::auditRooms()
{
    QTime t; t.start();
    // rooms konsolidieren
    QMapIterator<int, TRoom* > itRooms( rooms );
    while( itRooms.hasNext() )
    {
        itRooms.next();
        TRoom * pR = itRooms.value();
        if( ! rooms.contains(pR->north) ) pR->north = -1;
        if( ! rooms.contains(pR->south) ) pR->south = -1;
        if( ! rooms.contains(pR->northwest) ) pR->northwest = -1;
        if( ! rooms.contains(pR->northeast) ) pR->northeast = -1;
        if( ! rooms.contains(pR->southwest) ) pR->southwest = -1;
        if( ! rooms.contains(pR->southeast) ) pR->southeast = -1;
        if( ! rooms.contains(pR->west) ) pR->west = -1;
        if( ! rooms.contains(pR->east) ) pR->east = -1;
        if( ! rooms.contains(pR->in) ) pR->in = -1;
        if( ! rooms.contains(pR->out) ) pR->out = -1;

        AUDIT_SPECIAL_EXITS: QMapIterator<int, QString> it( pR->other );
        while( it.hasNext() )
        {
            it.next();
            QString _cmd = it.value();
            if( _cmd.size() <= 0 )
            {
                rooms[itRooms.key()]->other.remove( it.key(), it.value() );
                qDebug()<<"AUDIT_SPECIAL_EXITS: roomID:"<<pR->id<<" REMOVING invalid special exit:"<<_cmd;
                goto AUDIT_SPECIAL_EXITS;
            }
            else if( ! ( _cmd.startsWith('1') || _cmd.startsWith('0') ) )
            {
                QString _nc = it.value();
                int _nk = it.key();
                _nc.prepend('0');
                pR->other.remove( it.key(), it.value() );
                pR->other.insertMulti( _nk, _nc );
                qDebug()<<"AUDIT_SPECIAL_EXITS: roomID:"<<pR->id<<" PATCHING invalid special exit:"<<_cmd << " new:"<<_nc;
                goto AUDIT_SPECIAL_EXITS;
            }
        }
    }
    qDebug()<<"audit map: runtime:"<<t.elapsed();
}

void TMap::buildAreas()
{
    QTime _time; _time.start();
    QMapIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {
        it.next();
        int id = it.key();
        if( ! areas.contains( rooms[id]->area ) )
        {
            areas[rooms[id]->area] = new TArea( this );
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
            areas[id] = new TArea( this );
        }
    }
    qDebug()<<"BUILD AREAS run time:"<<_time.elapsed();
}

void TMap::setView(float x, float y, float z, float zoom )
{
    if( rooms.contains(mRoomId) )
    {
//        rooms[mRoomId]->xRot = x;
//        rooms[mRoomId]->yRot = y;
//        rooms[mRoomId]->zRot = z;
//        rooms[mRoomId]->zoom = zoom;
    }
}

void TMap::tidyMap( int areaID )
{
}

void TMap::solveRoomCollision( int id, int creationDirection, bool PCheck )
{
}

QList<int> TMap::detectRoomCollisions( int id )
{
    if( ! rooms.contains(( id ) ) )
    {
        QList<int> l;
        return l;
    }
    int area = rooms[id]->area;
    int x = rooms[id]->x;
    int y = rooms[id]->y;
    int z = rooms[id]->z;
    QList<int> collList;
    if( ! areas.contains( area ) )
    {
        QList<int> l;
        return l;
    }
    TArea * pA = areas[area];
    //pA->rooms;
    for( int i=0; i< pA->rooms.size(); i++ )
    {
        if( rooms[pA->rooms[i]]->x == x && rooms[pA->rooms[i]]->y == y && rooms[pA->rooms[i]]->z == z )
        {
            collList.push_back( pA->rooms[i] );
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

bool TMap::plausabilitaetsCheck( int area )
{
        return true;
}

bool TMap::fixExits( int id, int dir )
{
        return true;
}

bool TMap::fixExits2( int id )
{
        return true;
}


void TMap::getConnectedNodesGreaterThanX( int id, int min )
{
    if( ! rooms.contains(( id ) ) || id < 1 || mTestedNodes.contains(id) || conList.contains(id) )
    {
        return;
    }
    if( rooms[id]->x < min )
    {
        return;
    }

    conList.append( id );
    if( rooms[id]->north > 0 )
    {
        if( rooms.contains( rooms[id]->north ) && rooms[id]->north > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->north, min );
    }
    if( rooms[id]->south > 0 )
    {
        if( rooms.contains( rooms[id]->south ) && rooms[id]->south > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->south, min );
    }
    if( rooms[id]->northwest > 0 )
    {
        if( rooms.contains( rooms[id]->northwest ) && rooms[id]->northwest > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->northwest, min );
    }
    if( rooms[id]->northeast > 0 )
    {
        if( rooms.contains( rooms[id]->northeast ) && rooms[id]->northeast > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->northeast, min );
    }
    if( rooms[id]->southwest > 0 )
    {
        if( rooms.contains( rooms[id]->southwest ) && rooms[id]->southwest > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->southwest, min );
    }
    if( rooms[id]->southeast > 0 )
    {
        if( rooms.contains( rooms[id]->southeast ) && rooms[id]->southeast > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->southeast, min );
    }
    if( rooms[id]->west > 0 )
    {
        if( rooms.contains( rooms[id]->west ) && rooms[id]->west > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->west, min );
    }
    if( rooms[id]->east > 0 )
    {
        if( rooms.contains( rooms[id]->east ) && rooms[id]->east > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->east, min );
    }
    if( rooms[id]->up > 0 )
    {
        if( rooms.contains( rooms[id]->up ) && rooms[id]->up > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->up, min );
    }
    if( rooms[id]->down > 0 )
    {
        if( rooms.contains( rooms[id]->down ) && rooms[id]->down > 0 )
            getConnectedNodesGreaterThanX( rooms[id]->down, min );
    }
}

void TMap::getConnectedNodesSmallerThanX( int id, int min )
{
    if( ! rooms.contains(( id ) ) || id < 1 || mTestedNodes.contains(id) || conList.contains(id) )
    {
        return;
    }
    if( rooms[id]->x > min )
    {
        return;
    }

    conList.append( id );
    if( rooms[id]->north > 0 )
    {
        if( rooms.contains( rooms[id]->north ) && rooms[id]->north > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->north, min );
    }
    if( rooms[id]->south > 0 )
    {
        if( rooms.contains( rooms[id]->south ) && rooms[id]->south > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->south, min );
    }
    if( rooms[id]->northwest > 0 )
    {
        if( rooms.contains( rooms[id]->northwest ) && rooms[id]->northwest > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->northwest, min );
    }
    if( rooms[id]->northeast > 0 )
    {
        if( rooms.contains( rooms[id]->northeast ) && rooms[id]->northeast > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->northeast, min );
    }
    if( rooms[id]->southwest > 0 )
    {
        if( rooms.contains( rooms[id]->southwest ) && rooms[id]->southwest > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->southwest, min );
    }
    if( rooms[id]->southeast > 0 )
    {
        if( rooms.contains( rooms[id]->southeast ) && rooms[id]->southeast > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->southeast, min );
    }
    if( rooms[id]->west > 0 )
    {
        if( rooms.contains( rooms[id]->west ) && rooms[id]->west > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->west, min );
    }
    if( rooms[id]->east > 0 )
    {
        if( rooms.contains( rooms[id]->east ) && rooms[id]->east > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->east, min );
    }
    if( rooms[id]->up > 0 )
    {
        if( rooms.contains( rooms[id]->up ) && rooms[id]->up > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->up, min );
    }
    if( rooms[id]->down > 0 )
    {
        if( rooms.contains( rooms[id]->down ) && rooms[id]->down > 0 )
            getConnectedNodesSmallerThanX( rooms[id]->down, min );
    }
}

void TMap::getConnectedNodesGreaterThanY( int id, int min )
{

    if( ! rooms.contains(( id ) ) || id < 1 || mTestedNodes.contains(id) || conList.contains(id) )
    {
        return;
    }
    if( rooms[id]->y < min )
    {
        return;
    }

    conList.append( id );
    if( rooms[id]->north > 0 )
    {
        if( rooms.contains( rooms[id]->north ) && rooms[id]->north > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->north, min );
    }
    if( rooms[id]->south > 0 )
    {
        if( rooms.contains( rooms[id]->south ) && rooms[id]->south > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->south, min );
    }
    if( rooms[id]->northwest > 0 )
    {
        if( rooms.contains( rooms[id]->northwest ) && rooms[id]->northwest > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->northwest, min );
    }
    if( rooms[id]->northeast > 0 )
    {
        if( rooms.contains( rooms[id]->northeast ) && rooms[id]->northeast > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->northeast, min );
    }
    if( rooms[id]->southwest > 0 )
    {
        if( rooms.contains( rooms[id]->southwest ) && rooms[id]->southwest > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->southwest, min );
    }
    if( rooms[id]->southeast > 0 )
    {
        if( rooms.contains( rooms[id]->southeast ) && rooms[id]->southeast > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->southeast, min );
    }
    if( rooms[id]->west > 0 )
    {
        if( rooms.contains( rooms[id]->west ) && rooms[id]->west > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->west, min );
    }
    if( rooms[id]->east > 0 )
    {
        if( rooms.contains( rooms[id]->east ) && rooms[id]->east > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->east, min );
    }
    if( rooms[id]->up > 0 )
    {
        if( rooms.contains( rooms[id]->up ) && rooms[id]->up > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->up, min );
    }
    if( rooms[id]->down > 0 )
    {
        if( rooms.contains( rooms[id]->down ) && rooms[id]->down > 0 )
            getConnectedNodesGreaterThanY( rooms[id]->down, min );
    }
}

void TMap::getConnectedNodesSmallerThanY( int id, int min )
{

    if( ! rooms.contains(( id ) ) || id < 1 || mTestedNodes.contains(id) || conList.contains(id) )
    {
        return;
    }
    if( rooms[id]->y > min )
    {
        return;
    }

    conList.append( id );
    if( rooms[id]->north > 0 )
    {
        if( rooms.contains( rooms[id]->north ) && rooms[id]->north > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->north, min );
    }
    if( rooms[id]->south > 0 )
    {
        if( rooms.contains( rooms[id]->south ) && rooms[id]->south > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->south, min );
    }
    if( rooms[id]->northwest > 0 )
    {
        if( rooms.contains( rooms[id]->northwest ) && rooms[id]->northwest > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->northwest, min );
    }
    if( rooms[id]->northeast > 0 )
    {
        if( rooms.contains( rooms[id]->northeast ) && rooms[id]->northeast > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->northeast, min );
    }
    if( rooms[id]->southwest > 0 )
    {
        if( rooms.contains( rooms[id]->southwest ) && rooms[id]->southwest > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->southwest, min );
    }
    if( rooms[id]->southeast > 0 )
    {
        if( rooms.contains( rooms[id]->southeast ) && rooms[id]->southeast > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->southeast, min );
    }
    if( rooms[id]->west > 0 )
    {
        if( rooms.contains( rooms[id]->west ) && rooms[id]->west > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->west, min );
    }
    if( rooms[id]->east > 0 )
    {
        if( rooms.contains( rooms[id]->east ) && rooms[id]->east > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->east, min );
    }
    if( rooms[id]->up > 0 )
    {
        if( rooms.contains( rooms[id]->up ) && rooms[id]->up > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->up, min );
    }
    if( rooms[id]->down > 0 )
    {
        if( rooms.contains( rooms[id]->down ) && rooms[id]->down > 0 )
            getConnectedNodesSmallerThanY( rooms[id]->down, min );
    }
}

bool TMap::gotoRoom( int r )
{
    mTargetID = r;
    return findPath( mRoomId, r );
}

bool TMap::gotoRoom( int r1, int r2 )
{
    //mTargetID = r2;
    return findPath( r1, r2 );
}

void TMap::initGraph()
{
    QTime _time; _time.start();
    locations.clear();
    g.clear();
    g = mygraph_t();
    weightmap = get(edge_weight, g);
    QMapIterator<int, TRoom *> it( rooms );
    int roomCount=0;
    int edgeCount=0;
    while( it.hasNext() )
    {

        it.next();
        int i = it.key();
        if( ! rooms.contains( i ) || rooms[i]->isLocked || i == -1 )
        {
            continue;
        }
        roomCount++;
        int roomExits = edgeCount;
        location l;
        l.x = rooms[i]->x;
        l.y = rooms[i]->y;
        l.z = rooms[i]->z;
        locations.push_back( l );


        if( rooms[i]->north != -1 && rooms.contains( rooms[i]->north ) && !rooms[rooms[i]->north]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_NORTH ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->north,
                                             g );
                if( rooms[i]->exitWeights.contains("n"))
                    weightmap[e] = rooms[i]->exitWeights["n"];
                else
                    weightmap[e] = rooms[rooms[i]->north]->weight;
            }
        }
        if( rooms[i]->south != -1 && rooms.contains( rooms[i]->south ) && !rooms[rooms[i]->south]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_SOUTH ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->south,
                                             g );
                if( rooms[i]->exitWeights.contains("s"))
                    weightmap[e] = rooms[i]->exitWeights["s"];
                else
                    weightmap[e] = rooms[rooms[i]->south]->weight;
            }
        }
        if( rooms[i]->northeast != -1 && rooms.contains( rooms[i]->northeast ) && !rooms[rooms[i]->northeast]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_NORTHEAST ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                            rooms[i]->northeast,
                                            g );
                if( rooms[i]->exitWeights.contains("ne"))
                    weightmap[e] = rooms[i]->exitWeights["ne"];
                else
                    weightmap[e] = rooms[rooms[i]->northeast]->weight;

            }
        }
        if( rooms[i]->east != -1 && rooms.contains( rooms[i]->east ) && !rooms[rooms[i]->east]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_EAST ) )
            {
               edgeCount++;
               edge_descriptor e;
               bool inserted;
               tie(e, inserted) = add_edge( i,
                                            rooms[i]->east,
                                            g );
               if( rooms[i]->exitWeights.contains("e"))
                   weightmap[e] = rooms[i]->exitWeights["e"];
               else
                   weightmap[e] = rooms[rooms[i]->east]->weight;
            }
        }
        if( rooms[i]->west != -1 && rooms.contains( rooms[i]->west ) && !rooms[rooms[i]->west]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_WEST ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->west,
                                             g );
                if( rooms[i]->exitWeights.contains("w"))
                    weightmap[e] = rooms[i]->exitWeights["w"];
                else
                    weightmap[e] = rooms[rooms[i]->west]->weight;
            }
        }
        if( rooms[i]->southwest != -1 && rooms.contains( rooms[i]->southwest ) && !rooms[rooms[i]->southwest]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_SOUTHWEST ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->southwest,
                                             g );
                if( rooms[i]->exitWeights.contains("sw"))
                    weightmap[e] = rooms[i]->exitWeights["sw"];
                else
                    weightmap[e] = rooms[rooms[i]->southwest]->weight;
            }
        }
        if( rooms[i]->southeast != -1 && rooms.contains( rooms[i]->southeast ) && !rooms[rooms[i]->southeast]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_SOUTHEAST ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->southeast,
                                             g );
                if( rooms[i]->exitWeights.contains("se"))
                    weightmap[e] = rooms[i]->exitWeights["se"];
                else
                    weightmap[e] = rooms[rooms[i]->southeast]->weight;
            }
        }
        if( rooms[i]->northwest != -1 && rooms.contains( rooms[i]->northwest ) && !rooms[rooms[i]->northwest]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_NORTHWEST ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->northwest,
                                             g );
                if( rooms[i]->exitWeights.contains("nw"))
                    weightmap[e] = rooms[i]->exitWeights["nw"];
                else
                    weightmap[e] = rooms[rooms[i]->northwest]->weight;
            }
        }
        if( rooms[i]->up != -1 && rooms.contains( rooms[i]->up ) && !rooms[rooms[i]->up]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_UP ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->up,
                                             g );
                if( rooms[i]->exitWeights.contains("up"))
                    weightmap[e] = rooms[i]->exitWeights["up"];
                else
                    weightmap[e] = rooms[rooms[i]->up]->weight;
            }
        }
        if( rooms[i]->down != -1 && rooms.contains( rooms[i]->down ) && !rooms[rooms[i]->down]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_DOWN ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->down,
                                             g );
                if( rooms[i]->exitWeights.contains("down"))
                    weightmap[e] = rooms[i]->exitWeights["down"];
                else
                    weightmap[e] = rooms[rooms[i]->down]->weight;
            }
        }
        if( rooms[i]->in != -1 && rooms.contains( rooms[i]->in ) && !rooms[rooms[i]->in]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_IN ) )
            {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->in,
                                             g );
                if( rooms[i]->exitWeights.contains("in"))
                    weightmap[e] = rooms[i]->exitWeights["in"];
                else
                    weightmap[e] = rooms[rooms[i]->in]->weight;
            }
        }
        if( rooms[i]->out != -1 && rooms.contains( rooms[i]->out ) && !rooms[rooms[i]->out]->isLocked )
        {
            if( ! rooms[i]->hasExitLock( DIR_OUT ) )
            {
                 edgeCount++;
                 edge_descriptor e;
                 bool inserted;
                 tie(e, inserted) = add_edge( i,
                                              rooms[i]->out,
                                              g );
                 if( rooms[i]->exitWeights.contains("out"))
                     weightmap[e] = rooms[i]->exitWeights["out"];
                 else
                     weightmap[e] = rooms[rooms[i]->out]->weight;
            }
        }
        if( rooms[i]->other.size() > 0 )
        {
            QMapIterator<int, QString> it( rooms[i]->other );
            while( it.hasNext() )
            {
                it.next();
                int _id = it.key();
                QString _cmd = it.value();
                if( _cmd.size()>0 ) _cmd.remove(0,1);//strip special exit lock information

                // FIXME: double check if the special exit id really exists in the room db as a workaround
                if( rooms.contains( _id ) && !rooms[i]->hasSpecialExitLock( _id, it.value() ) )
                {
                    edgeCount++;
                    edge_descriptor e;
                    bool inserted;
                    tie(e, inserted) = add_edge( i,
                                                 _id,
                                                 g );
                    if( rooms[i]->exitWeights.contains(_cmd))
                    {
                        weightmap[e] = rooms[i]->exitWeights[_cmd];
                    }
                    else
                    {
                        weightmap[e] = rooms[_id]->weight;
                    }
                }
            }
        }
        if( roomExits == edgeCount ) locations.pop_back();
    }

    mMapGraphNeedsUpdate = false;
    qDebug()<<"initGraph: nodes: "<<locations.size()<<"/"<<roomCount<<" edges:"<<edgeCount<<" run time:"<<_time.elapsed();
}

bool TMap::findPath( int from, int to )
{
     if( mMapGraphNeedsUpdate )
     {
        initGraph();
     }

     vertex start = from;//mRoomId;
     vertex goal = to;//mTargetID;
     if( ! rooms.contains( start ) || ! rooms.contains( goal ) )
     {
         return false;
     }
     vector<mygraph_t::vertex_descriptor> p(num_vertices(g));
     vector<cost> d(num_vertices(g));
     try
     {
         astar_search( g,
                       start,
                       distance_heuristic<mygraph_t, cost, std::vector<location> >(locations, goal),
                       predecessor_map(&p[0]).distance_map(&d[0]).
                       visitor(astar_goal_visitor<vertex>(goal)) );
     }
     catch( found_goal fg )
     {
         list<vertex> shortest_path;
         for(vertex v = goal; ; v = p[v])
         {
             cout << "assembling path: v="<<v<<endl;
             if( ! rooms.contains( v ) )
            {
                 cout<<"ERROR path assembly: path room not in map!"<<endl;
                 return false;
             }
             shortest_path.push_front(v);
             if(p[v] == v) break;
         }
         cout << "Shortest path from " << rooms[start]->id << " to "
              << rooms[goal]->id << ": ";
         list<vertex>::iterator spi = shortest_path.begin();
         cout << rooms[start]->id;
         mPathList.clear();
         mDirList.clear();
         int curRoom = start;
         for( ++spi; spi != shortest_path.end(); ++spi )
         {
             if( ! rooms.contains( *spi ) )
             {
                 cout << "ERROR: path not possible. curRoom not in map!" << endl;
                 mPathList.clear();
                 mDirList.clear();
                 return false;
             }
             cout <<" spi:"<<*spi<<" curRoom:"<< curRoom << endl;//" -> ";
             mPathList.push_back( *spi );
             if( rooms[curRoom]->north == rooms[*spi]->id )
             {
                 mDirList.push_back("n");
             }
             else if( rooms[curRoom]->northeast == rooms[*spi]->id )
             {
                 mDirList.push_back("ne");
             }
             else if( rooms[curRoom]->northwest == rooms[*spi]->id )
             {
                 mDirList.push_back("nw");
             }
             else if( rooms[curRoom]->southeast == rooms[*spi]->id )
             {
                 mDirList.push_back("se");
             }
             else if( rooms[curRoom]->southwest == rooms[*spi]->id )
             {
                 mDirList.push_back("sw");
             }
             else if( rooms[curRoom]->south == rooms[*spi]->id )
             {
                 mDirList.push_back("s");
             }
             else if( rooms[curRoom]->east == rooms[*spi]->id )
             {
                 mDirList.push_back("e");
             }
             else if( rooms[curRoom]->west == rooms[*spi]->id )
             {
                 mDirList.push_back("w");
             }
             else if( rooms[curRoom]->up == rooms[*spi]->id )
             {
                 mDirList.push_back("up");
             }
             else if( rooms[curRoom]->down == rooms[*spi]->id )
             {
                 mDirList.push_back("down");
             }
             else if( rooms[curRoom]->in == rooms[*spi]->id )
             {
                 mDirList.push_back("in");
             }
             else if( rooms[curRoom]->out == rooms[*spi]->id )
             {
                 mDirList.push_back("out");
             }
             else if( rooms[curRoom]->other.size() > 0 )
             {
                 QMapIterator<int, QString> it( rooms[curRoom]->other );
                 while( it.hasNext() )
                 {
                     it.next();
                     if( it.key() == rooms[*spi]->id )
                     {
                         QString _cmd = it.value();
                         if( _cmd.size() > 0 && (_cmd.startsWith('0')))
                         {
                             _cmd = _cmd.mid(1);
                             mDirList.push_back( _cmd );
                             qDebug()<<" adding special exit: roomID:"<<rooms[curRoom]->id<<" OPEN special exit:"<<_cmd;
                         }
                         else if( _cmd.startsWith('1'))
                         {
                             qDebug()<<"NOT adding roomID:"<<rooms[curRoom]->id<<" LOCKED special exit:"<<_cmd;
                         }
                         else
                             qDebug()<<"ERROR adding roomID:"<<rooms[curRoom]->id<<" special exit:"<<_cmd;
                     }
                 }
             }

             qDebug()<<"added to DirList:"<<mDirList.back();
             curRoom = *spi;
         }

         return true;
     }

     return false;
}

bool TMap::serialize( QDataStream & ofs )
{
    version = 16;
    ofs << version;
    ofs << envColors;
    ofs << areaNamesMap;
    ofs << customEnvColors;
    ofs << hashTable;

    ofs << areas.size();
    // serialize area table
    QMapIterator<int, TArea *> itAreaList(areas);
    while( itAreaList.hasNext() )
    {
        itAreaList.next();
        int areaID = itAreaList.key();
        TArea * pA = itAreaList.value();
        ofs << areaID;
        ofs << pA->rooms;
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
        ofs << pA->xmaxEbene;
        ofs << pA->ymaxEbene;
        ofs << pA->zmaxEbene;
        ofs << pA->xminEbene;
        ofs << pA->yminEbene;
        ofs << pA->zminEbene;
        ofs << pA->pos;
        ofs << pA->isZone;
        ofs << pA->zoneAreaRef;
    }

    if (mRoomId)
        ofs << mRoomId;
    else{
        mRoomId = 0;
        ofs << mRoomId;
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
            int ii = itL2.key();
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
    QMapIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {

        it.next();
        int i = it.key();
        ofs <<  rooms[i]->id;
        ofs << rooms[i]->area;
        ofs << rooms[i]->x;
        ofs << rooms[i]->y;
        ofs << rooms[i]->z;
        ofs << rooms[i]->north;
        ofs << rooms[i]->northeast;
        ofs << rooms[i]->east;
        ofs << rooms[i]->southeast;
        ofs << rooms[i]->south;
        ofs << rooms[i]->southwest;
        ofs << rooms[i]->west;
        ofs << rooms[i]->northwest;
        ofs << rooms[i]->up;
        ofs << rooms[i]->down;
        ofs << rooms[i]->in;
        ofs << rooms[i]->out;
        ofs << rooms[i]->environment;
        ofs << rooms[i]->weight;
//        ofs << rooms[i]->xRot;
//        ofs << rooms[i]->yRot;
//        ofs << rooms[i]->zRot;
//        ofs << rooms[i]->zoom;
        ofs << rooms[i]->name;
        ofs << rooms[i]->isLocked;
        ofs << rooms[i]->other;
        ofs << rooms[i]->c;
        ofs << rooms[i]->userData;
        ofs << rooms[i]->customLines;
        ofs << rooms[i]->customLinesArrow;
        ofs << rooms[i]->customLinesColor;
        ofs << rooms[i]->customLinesStyle;
        ofs << rooms[i]->exitLocks;
        ofs << rooms[i]->exitStubs;
        ofs << rooms[i]->exitWeights;
        ofs << rooms[i]->doors;
    }

    return true;
}

#include <QDir>

bool TMap::restore(QString location)
{
    qDebug()<<"restoring map of profile:"<<mpHost->getName()<<" url:"<<mpHost->getUrl();
    QTime _time; _time.start();
    QString folder;
    QStringList entries;
    qDebug()<<"RESTORING MAP";

    if (location == "") {
        folder = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName()+"/map/";
        QDir dir( folder );
        dir.setSorting(QDir::Time);
        entries = dir.entryList( QDir::Files, QDir::Time );
        for( int i=0;i<entries.size(); i++ )
            qDebug()<<i<<"#"<<entries[i];
    }

    bool canRestore = true;
    if( entries.size() > 0 || location != "")
    {
        QFile file((location == "") ? folder+entries[0] : location);
        if (!file.open( QFile::ReadOnly ))
            return false;
        qDebug()<<"[LOADING MAP]:"<<file.fileName();
        QDataStream ifs( & file );

        ifs >> version;
        qDebug()<<"map version:"<<version;
        if( version >= 3 )
        {
            ifs >> envColors;
        }
        else
        {
            canRestore = false;
        }
        if( version >= 4 )
        {
            ifs >> areaNamesMap;
        }
        else
        {
            canRestore = false;
        }
        if( version >= 5 )
        {
            ifs >> customEnvColors;
        }
        if( version > 6 )
        {
            ifs >> hashTable;
        }
        if( version >= 14 )
        {
            int areaSize;
            ifs >> areaSize;
            // restore area table
            for( int i=0; i<areaSize; i++ )
            {
                TArea * pA = new TArea( this );
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
                ifs >> pA->xmaxEbene;
                ifs >> pA->ymaxEbene;
                ifs >> pA->zmaxEbene;
                ifs >> pA->xminEbene;
                ifs >> pA->yminEbene;
                ifs >> pA->zminEbene;
                qDebug()<<"areaID:"<<areaID<<" rooms:"<<pA->rooms<<" exits:"<<pA->exits<<" xmaxEbene:"<<pA->xmaxEbene<<" ymaxEbene:"<<pA->ymaxEbene;
                ifs >> pA->pos;
                ifs >> pA->isZone;
                ifs >> pA->zoneAreaRef;
                areas[areaID] = pA;
            }
        }

        if( version >= 12 )
        {
            ifs >> mRoomId;
        }
        if( version >= 11 )
        {
            int size;
            ifs >> size; //size of mapLabels
            int areaLabelCount = 0;
            while( ! ifs.atEnd() && areaLabelCount < size )
            {
                int areaID;
                int size_labels;
                ifs >> size_labels;
                ifs >> areaID;
                int labelCount = 0;
                QMap<int, TMapLabel> _map;
                while( ! ifs.atEnd() &&  labelCount < size_labels )
                {
                    int labelID;
                    ifs >> labelID;
                    TMapLabel label;
                    if( version >= 12 )
                    {
                        ifs >> label.pos;
                    }
                    else
                    {
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
                    if( version >= 15 )
                    {
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
        while( ! ifs.atEnd() )
        {
            int i;
            ifs >> i;
            TRoom * pT = new TRoom;
            rooms[i] = pT;
            rooms[i]->id = i;
            ifs >> rooms[i]->area;
            ifs >> rooms[i]->x;
            ifs >> rooms[i]->y;
            ifs >> rooms[i]->z;
            ifs >> rooms[i]->north;
            ifs >> rooms[i]->northeast;
            ifs >> rooms[i]->east;
            ifs >> rooms[i]->southeast;
            ifs >> rooms[i]->south;
            ifs >> rooms[i]->southwest;
            ifs >> rooms[i]->west;
            ifs >> rooms[i]->northwest;
            ifs >> rooms[i]->up;
            ifs >> rooms[i]->down;
            ifs >> rooms[i]->in;
            ifs >> rooms[i]->out;
            ifs >> rooms[i]->environment;
            ifs >> rooms[i]->weight;
            if( version < 8 )
            {
                float f1,f2,f3,f4;
                ifs >> f1;//rooms[i]->xRot;
                ifs >> f2;//rooms[i]->yRot;
                ifs >> f3;//rooms[i]->zRot;
                ifs >> f4;//rooms[i]->zoom;
            }
            ifs >> rooms[i]->name;
            ifs >> rooms[i]->isLocked;
            if( version >= 6 )
            {
                ifs >> rooms[i]->other;
            }
            if( version >= 9 )
            {
                ifs >> rooms[i]->c;
            }
            if( version >= 10 )
            {
                ifs >> rooms[i]->userData;
            }
            if( version >= 11 )
            {
                ifs >> rooms[i]->customLines;
                ifs >> rooms[i]->customLinesArrow;
                ifs >> rooms[i]->customLinesColor;
                ifs >> rooms[i]->customLinesStyle;
                ifs >> rooms[i]->exitLocks;
            }
            if( version >= 13 )
            {
                ifs >> rooms[i]->exitStubs;
            }
            if( version >= 16 )
            {
                ifs >> rooms[i]->exitWeights;
                ifs >> rooms[i]->doors;
            }
            rooms[i]->calcRoomDimensions();
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
        qDebug()<<"LOADED rooms:"<<rooms.size()<<" loading time:"<<_time.elapsed();
        if( canRestore )
        {
            return true;
        }
    }
    if( ! canRestore || entries.size() == 0 )
    {
        QMessageBox msgBox;

        if( mpHost->mUrl.toLower().contains( "achaea.com" )
            || mpHost->mUrl.toLower().contains( "aetolia.com" )
            || mpHost->mUrl.toLower().contains( "imperian.com" )
            || mpHost->mUrl.toLower().contains( "midkemiaonline.com" )
            || mpHost->mUrl.toLower().contains( "lusternia.com" ) )
        {
            msgBox.setText("No map found. Would you like to download the map or start your own?");
            QPushButton *yesButton = msgBox.addButton("Download the map", QMessageBox::ActionRole);
            QPushButton *noButton = msgBox.addButton("Start my own", QMessageBox::ActionRole);
            msgBox.exec();
            init( mpHost );
            if (msgBox.clickedButton() == yesButton) {
                mpMapper->downloadMap();
            }
        }
        else
        {
            mpHost->mpMap->init( mpHost );
        }
    }
    return canRestore;//FIXME
}


int TMap::createMapLabel(int area, QString text, float x, float y, float z, QColor fg, QColor bg, bool showOnTop, bool noScaling, qreal zoom, int fontSize )
{
    if( ! areas.contains( area ) ) return -1;

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
    if( !areas.contains(area) ) return -1;
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
    if( ! areas.contains( area ) ) return -1;

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
    if( !areas.contains(area) ) return -1;
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
    if( ! areas.contains( area ) ) return;
    if( ! mapLabels.contains( area ) ) return;
    if( ! mapLabels[area].contains( labelID ) ) return;
    mapLabels[area].remove( labelID );
    if( mpMapper ) mpMapper->mp2dMap->update();
}



