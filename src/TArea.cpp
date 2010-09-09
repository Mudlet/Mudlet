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




#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include <stdlib.h>
#include <QDebug>
#include <QPair>

#define NORTH 12
#define NORTHEAST 1
#define EAST 3
#define SOUTHEAST 4
#define SOUTH 6
#define SOUTHWEST 7
#define WEST 9
#define NORTHWEST 10
#define UP 13
#define DOWN 14

TArea::TArea( TMap * map )
: min_x(0)
, min_y(0)
, min_z(0)
, max_x(0)
, max_y(0)
, max_z(0)
, gridMode( false )
{
    mpMap = map;
}

void TArea::ausgaengeBestimmen()
{
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        if( ! rooms.indexOf( mpMap->rooms[id]->north ) )
        {
            QPair<int, int> p = QPair<int,int>(id, NORTH);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->northeast ) )
        {
            QPair<int, int> p = QPair<int,int>(id, NORTHEAST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->east ) )
        {
            QPair<int, int> p = QPair<int,int>(id, EAST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->southeast ) )
        {
            QPair<int, int> p = QPair<int,int>(id, SOUTHEAST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->south ) )
        {
            QPair<int, int> p = QPair<int,int>(id, SOUTH);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->southwest ) )
        {
            QPair<int, int> p = QPair<int,int>(id, SOUTHWEST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->west ) )
        {
            QPair<int, int> p = QPair<int,int>(id, WEST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->northwest ) )
        {
            QPair<int, int> p = QPair<int,int>(id, NORTHWEST);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->up ) )
        {
            QPair<int, int> p = QPair<int,int>(id, UP);
            exits.insertMulti( id, p );
        }
        if( ! rooms.indexOf( mpMap->rooms[id]->down ) )
        {
            QPair<int, int> p = QPair<int,int>(id, DOWN);
            exits.insertMulti( id, p );
        }
    }
    //qDebug()<<"exits:"<<exits.size();
}

void TArea::calcSpan()
{
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->x;
        if( _m < min_x )
        {
            min_x = _m;
        }
    }
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->y;
        if( _m < min_y )
        {
            min_y = _m;
        }
    }
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->z;
        if( _m < min_z )
        {
            min_z = _m;
        }
    }
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->x;
        if( _m > max_x )
        {
            max_x = _m;
        }
    }
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->y;
        if( _m > max_y )
        {
            max_y = _m;
        }
    }
    for( int i=0; i<rooms.size(); i++ )
    {
        int id = rooms[i];
        int _m = mpMap->rooms[id]->z;
        if( _m > max_z )
        {
            max_z = _m;
        }
        // ebenenliste anlegen
        if( ! ebenen.contains( _m ) )
        {
            ebenen.push_back( _m );
        }
    }

    qreal sx = abs( min_x ) + max_x;
    qreal sy = abs( min_y ) + max_y;
    qreal sz = abs( min_z ) + max_z;
    span = QVector3D( sx, sy, sz );

    for( int k=0; k<ebenen.size(); k++ )
    {
        int _min_x = max_x;
        int _min_y = max_y;
        int _min_z = max_z;
        int _max_x = min_x;
        int _max_y = min_y;
        int _max_z = min_z;

        for( int i=0; i<rooms.size(); i++ )
        {
            int id = rooms[i];
            if( mpMap->rooms[id]->z != ebenen[k]) continue;
            int _m = mpMap->rooms[id]->x;
            if( _m < _min_x )
            {
                _min_x = _m;
            }
            _m = mpMap->rooms[id]->y;
            if( _m < _min_y )
            {
                _min_y = _m;
            }
            _m = mpMap->rooms[id]->z;
            if( _m < _min_z )
            {
                _min_z = _m;
            }
            _m = mpMap->rooms[id]->x;
            if( _m > _max_x )
            {
                max_x = _m;
            }
            _m = mpMap->rooms[id]->y;
            if( _m > _max_y )
            {
                _max_y = _m;
            }
            _m = mpMap->rooms[id]->z;
            if( _m > _max_z )
            {
                _max_z = _m;
            }
        }
        xminEbene[ebenen[k]] = _min_x;
        yminEbene[ebenen[k]] = _min_y;
        zminEbene[ebenen[k]] = _min_z;
        xmaxEbene[ebenen[k]] = _max_x;
        ymaxEbene[ebenen[k]] = _max_y;
        zmaxEbene[ebenen[k]] = _max_z;
    }


}


