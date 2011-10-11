
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
, highlight( false )
, highlightColor( QColor( 255,150,0 ) )
, rendered(false)
{
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
            return;
        }
    }
    // it doesnt exit -> add
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
    other.insertMulti( to, cmd );
}


void TRoom::removeSpecialExit( int to, QString cmd )
{
    other.remove(to, cmd.prepend("0"));
    other.remove(to, cmd.prepend("1"));
}







