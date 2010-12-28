
#include <QVector3D>
#include "TRoom.h"


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
