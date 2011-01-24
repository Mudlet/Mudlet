
#include <QVector3D>
#include "TRoom.h"

#include <QDebug>

TRoom::TRoom()
: id( 0 )
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
{
}

void TRoom::setWeight( int w )
{
    weight = w;
}

bool TRoom::hasExit( int id )
{
    if( north == id )
        return true;
    else if( south == id )
        return true;
    else if( northwest == id )
        return true;
    else if( northeast == id )
        return true;
    else if( southwest == id )
        return true;
    else if( southeast == id )
        return true;
    else if( east == id )
        return true;
    else if( west == id )
        return true;
    else if( up == id )
        return true;
    else if( down == id )
        return true;
    else if( out == id )
        return true;
    else if( in == id )
        return true;
    else
        return false;
}

void TRoom::setExitLock(int exit, bool state )
{
    qDebug()<<"setExitLock: exit="<<exit<<" state="<<state;
    if( ! state )
    {
        exitLocks.removeAll( exit );
        return;
    }
    switch( exit )
    {
        case DIR_NORTH: exitLocks[DIR_NORTH] = true; break;
        case DIR_NORTHEAST: exitLocks[DIR_NORTHEAST] = true; break;
        case DIR_NORTHWEST: exitLocks[DIR_NORTHWEST] = true; break;
        case DIR_SOUTHEAST: exitLocks[DIR_SOUTHEAST] = true; break;
        case DIR_SOUTHWEST: exitLocks[DIR_SOUTHWEST] = true; break;
        case DIR_SOUTH: exitLocks[DIR_SOUTH] = true; break;
        case DIR_EAST: exitLocks[DIR_EAST] = true; break;
        case DIR_WEST: exitLocks[DIR_WEST] = true; break;
        case DIR_UP: exitLocks[DIR_UP] = true; break;
        case DIR_DOWN: exitLocks[DIR_DOWN] = true; break;
        case DIR_IN: exitLocks[DIR_IN] = true; break;
        case DIR_OUT: exitLocks[DIR_OUT] = true; break;
    }
}

void TRoom::setSpecialExitLock(int to, QString cmd, bool doLock)
{
    if( other.contains( to ) )
    {
        QMapIterator<int, QString> it( other );
        while(it.hasNext() )
        {
            it.next();
            if( it.key() != to ) continue;
            if( it.value().right(1) != cmd ) continue;
            if( doLock && it.value().size() > 0 )
            {
                QString _cmd = it.value();
                _cmd.mid(0,1) = "1";
                other.replace( it.key(), _cmd );
            }
            else if( it.value().size() > 0 )
            {
                QString _cmd = it.value();
                _cmd.mid(0,1) = "0";
                other.replace( it.key(), _cmd );
            }
            return;
        }
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
            if( it.value().right(1) != cmd ) continue;
            if( it.value().size() > 0 )
                return it.value().mid(0,1) == "1";
        }
        return false;
    }
    else
        return false;
}

void TRoom::addSpecialExit( int to, QString cmd )
{
    // replace if this special exit exists, otherwise add
    QMapIterator<int, QString> it( other );
    while(it.hasNext() )
    {
        it.next();
        if( it.key() != to ) continue;
        if( it.value().right(1) != cmd ) continue;
        if( it.value().size() > 0 )
        {
            QString _cmd = cmd;
            _cmd.prepend("0");
            other.replace( to, _cmd );
            _cmd.mid(0,1) = "1";
            other.replace( to, _cmd );
            return;
        }
    }
    // it doesnt exit -> add
    QString _cmd = cmd;
    cmd.prepend("0");
    other.insertMulti( to, cmd );
}


void TRoom::removeSpecialExit( int to, QString cmd )
{
    other.remove(to, cmd.prepend("0"));
    other.remove(to, cmd.prepend("1"));
}







