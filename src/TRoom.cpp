
#include <QVector3D>
#include "TRoom.h"
#include "TRoomDB.h"
#include <QDebug>

TRoom::TRoom(TRoomDB * pRDB )
: id( 0 )
, mpRoomDB( pRDB )
, area( 0 )
, x( 0 )
, y( 0 )
, z( 0 )
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
, environment( -1 )
, weight(1)
, isLocked( false )
, c( 0 )
, highlight( false )
, highlightColor( QColor( 255,150,0 ) )
, rendered(false)
, min_x( 0 )
, max_x( 0 )
, min_y( 0 )
, max_y( 0 )
{
}

TRoom::~TRoom()
{
    mpRoomDB->__removeRoom( id );
}

int TRoom::hasExitStub(int direction){
    if (exitStubs.contains(direction))
        return 1;
    else
        return 0;
}

void TRoom::setExitStub(int direction, int status){
    if (status)
        exitStubs.append(direction);
    else
        exitStubs.removeOne(direction);
}

int TRoom::getExitWeight( QString cmd )
{
    if( exitWeights.contains( cmd ) )
    {
        return exitWeights[cmd];
    }
    else
        return weight; // NOTE: if no exit weight has been set: exit weight = room weight
}

void TRoom::setWeight( int w )
{
    if( w < 1 ) w = 1;
    weight = w;
}

//bool TRoom::setExit( int to, int dir )
//{
//    TRoom * pR_to = mpRoomDB->getRoom( to );
//    if( !pR_to )
//    {
//        to = -1;
//    }
//    switch( dir )
//    {
//        case DIR_NORTH:
//            north = to;
//            break;
//        case DIR_NORTHEAST:
//            northeast = to;
//            break;
//        case DIR_NORTHWEST:
//            northwest = to;
//            break;
//        case DIR_SOUTH:
//            south = to;
//            break;
//        case DIR_SOUTHEAST:
//            southeast = to;
//            break;
//        case DIR_SOUTHWEST:
//            southwest = to;
//            break;
//        case DIR_EAST:
//            east = to;
//            break;
//        case DIR_WEST:
//            west = to;
//            break;
//        case DIR_UP:
//            up = to;
//            break;
//        case DIR_DOWN:
//            down = to;
//            break;
//        case DIR_IN:
//            in = to;
//            break;
//        case DIR_OUT:
//            out = to;
//            break;
//        default:
//            return false;
//    }
//    return true;
//}

void TRoom::setExitWeight(QString cmd, int w)
{
    exitWeights[cmd] = w;
}

void TRoom::setId( int _id )
{
    id = _id;
}

void TRoom::setArea( int _areaID )
{
    area = _areaID;
    TArea * pA = mpRoomDB->getArea( area );
    if( !pA )
    {
        mpRoomDB->addArea( area );
        pA = mpRoomDB->getArea( area );
        if( !pA )
        {
            QString error = "TRoom::setArea(): No area created! requested area ID=%1. Note: area IDs must be > 0";
            mpRoomDB->mpMap->logError(error);
            return;
        }
    }

    pA->addRoom( id );
    pA->fast_ausgaengeBestimmen(id);
    pA->fast_calcSpan(id);
}

bool TRoom::hasExit( int _id )
{
    if( north == _id )
        return true;
    else if( south == _id )
        return true;
    else if( northwest == _id )
        return true;
    else if( northeast == _id )
        return true;
    else if( southwest == _id )
        return true;
    else if( southeast == _id )
        return true;
    else if( east == _id )
        return true;
    else if( west == _id )
        return true;
    else if( up == _id )
        return true;
    else if( down == _id )
        return true;
    else if( out == _id )
        return true;
    else if( in == _id )
        return true;
    else
        return false;
}

void TRoom::setExitLock(int exit, bool state )
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

void TRoom::setSpecialExitLock(int to, QString cmd, bool doLock)
{
    QMapIterator<int, QString> it( other );
    while(it.hasNext() )
    {
        it.next();
        if( it.key() != to ) continue;
        if( it.value().size() < 1 ) continue;
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

bool TRoom::hasExitLock( int exit )
{
    return exitLocks.contains(exit);
}

// 0=offen 1=zu
bool TRoom::hasSpecialExitLock(int to, QString cmd)
{
    if( other.contains( to ) )
    {
        QMapIterator<int, QString> it( other );
        while(it.hasNext() )
        {
            it.next();
            if( it.key() != to ) continue;
            if( it.value().size() < 2 ) continue;
            return it.value().mid(0,1) == "1";
        }
        return false;
    }
    else
        return false;
}

void TRoom::addSpecialExit( int to, QString cmd )
{
    QString _cmd;
    // replace if this special exit exists, otherwise add
    QMapIterator<int, QString> it( other );
    while(it.hasNext() )
    {
        it.next();
        if( it.key() != to ) continue;
        if( it.value().size() > 0 )
        {
            QString _cmd;
            if( cmd.startsWith('0') || cmd.startsWith('1') )
            {
                _cmd = cmd;
            }
            else
            {
                _cmd.prepend("0");
                _cmd.append( cmd );
            }

            other.replace( to, _cmd );
            goto UPDATE_AREAS;
        }
    }
    // it doesnt exit -> add

    if( cmd.startsWith('0') || cmd.startsWith('1') )
    {
        _cmd = cmd;
    }
    else
    {
        _cmd.prepend("0");
        _cmd.append( cmd );
    }
    other.insertMulti( to, _cmd );

UPDATE_AREAS: TArea * pA = mpRoomDB->getArea( getArea() );
    if( pA )
    {
        pA->fast_ausgaengeBestimmen(getId());
    }

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
}

void TRoom::calcRoomDimensions()
{
    QMapIterator<QString, QList<QPointF> > it(customLines);
    while( it.hasNext() )
    {
        it.next();
        const QString & _e = it.key();
        const QList<QPointF> & _pL= it.value();
        if( _pL.size() < 1 ) continue;
        if( min_x == min_y == max_x == max_y == 1 )
        {
            min_x = _pL[0].x();
            max_x = min_x;
            min_y = _pL[0].y();
            max_y = min_y;
        }
        for( int i=0; i<_pL.size(); i++ )
        {
            qreal _x = _pL[i].x();
            qreal _y = _pL[i].y();
            if( min_x > _x )
                min_x = _x;
            if( max_x < _x )
                max_x = _x;
            if( min_y > _y )
                min_y = _y;
            if( max_y < _y )
                max_y = _y;
        }
        qDebug()<<"custom lines span: x("<<min_x<<"/"<<max_x<<") y("<<min_y<<"/"<<max_y<<")";
    }
}

#include <QDataStream>

bool TRoom::restore( QDataStream & ifs, int i, int version )
{

    id = i;
    ifs >> area;
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
    if( ! mpRoomDB->getRoom(north) ) north = -1;
    if( ! mpRoomDB->getRoom(south) ) south = -1;
    if( ! mpRoomDB->getRoom(northwest) ) northwest = -1;
    if( ! mpRoomDB->getRoom(northeast) ) northeast = -1;
    if( ! mpRoomDB->getRoom(southwest) ) southwest = -1;
    if( ! mpRoomDB->getRoom(southeast) ) southeast = -1;
    if( ! mpRoomDB->getRoom(west) ) west = -1;
    if( ! mpRoomDB->getRoom(east) ) east = -1;
    if( ! mpRoomDB->getRoom(in) ) in = -1;
    if( ! mpRoomDB->getRoom(out) ) out = -1;

    AUDIT_SPECIAL_EXITS: QMapIterator<int, QString> it( other );
    while( it.hasNext() )
    {
        it.next();
        QString _cmd = it.value();
        if( _cmd.size() <= 0 )
        {
            other.remove( it.key(), it.value() );
            qDebug()<<"AUDIT_SPECIAL_EXITS: roomID:"<<id<<" REMOVING invalid special exit:"<<_cmd;
            goto AUDIT_SPECIAL_EXITS;
        }
        else if( ! ( _cmd.startsWith('1') || _cmd.startsWith('0') ) )
        {
            QString _nc = it.value();
            int _nk = it.key();
            _nc.prepend('0');
            other.remove( it.key(), it.value() );
            other.insertMulti( _nk, _nc );
            qDebug()<<"AUDIT_SPECIAL_EXITS: roomID:"<<id<<" PATCHING invalid special exit:"<<_cmd << " new:"<<_nc;
            goto AUDIT_SPECIAL_EXITS;
        }
    }
}



