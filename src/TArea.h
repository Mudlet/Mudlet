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



#ifndef TAREA_H
#define TAREA_H

#include <QList>
#include <QMultiMap>
#include <QPair>
#include <QSizeF>
#include <QColor>
#include <QPixmap>
#include <QVector3D>
#include "TRoomDB.h"

class TMap;
class TRoomDB;


class TArea
{
public:

    TArea( TMap *, TRoomDB * );
    ~TArea();
    int getAreaID();
    void addRoom( int id );
    const QList<int> & getAreaRooms() const { return rooms; }
    const QList<int> getAreaExits() const { return exits.uniqueKeys(); }
    void calcSpan();
    void fast_calcSpan(int);
    void fast_ausgaengeBestimmen(int);
    QList<int> getCollisionNodes();
    QList<int> getRoomsByPosition( int x, int y, int z );
    QMap<int,QMap<int,QMultiMap<int,int> > > koordinatenSystem();
    void ausgaengeBestimmen();
    QMultiMap<int, QPair<int, int> > exits; // rooms that border on this area: key=in_area room id, pair.first=out_of_area room id pair.second=direction
    QList<int> rooms; // rooms of this area
    QVector3D pos; // pos auf der map und 0 punkt des area internen koordinatensystems
    QVector3D span;
    int min_x;
    int min_y;
    int min_z;
    int max_x;
    int max_y;
    int max_z;
    QMap<int, int> xminEbene;
    QMap<int, int> xmaxEbene;
    QMap<int, int> yminEbene;
    QMap<int, int> ymaxEbene;
    QMap<int, int> zminEbene;
    QMap<int, int> zmaxEbene;
    QList<int> ebenen;
    bool gridMode;
    bool isZone;
    int zoneAreaRef;
    TRoomDB * mpRoomDB;

private:
    TArea(){qFatal("FATAL: illegal default constructor use of TArea()");};
    //QMap<int, TMapLabel> labelMap;

};

// - gezeichnet werden erstmal die areas
//   unter berücksichtigung der an die area angrenzenden edges
// - der span der area ist unterschiedlich

#endif // TAREA_H
