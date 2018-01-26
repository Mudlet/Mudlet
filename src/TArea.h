#ifndef MUDLET_TAREA_H
#define MUDLET_TAREA_H

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

#include "pre_guard.h"
#include <QList>
#include <QMap>
#include <QPair>
#include <QVector3D>
#include "post_guard.h"

class TRoomDB;


class TArea
{
    Q_DECLARE_TR_FUNCTIONS(TArea) // Needed so we can use tr() even though TArea is NOT derived from QObject

    friend bool TMap::serialize(QDataStream&);
    friend bool TMap::restore(QString, bool);
    friend bool TMap::retrieveMapFileStats(QString, QString*, int*, int*, int*, int*);

public:
    TArea(TMap*, TRoomDB*);
    ~TArea();
    int getAreaID();
    void addRoom(int id);
    const QSet<int>& getAreaRooms() const { return rooms; }
    const QList<int> getAreaExitRoomIds() const { return exits.uniqueKeys(); }
    const QMultiMap<int, QPair<QString, int>> getAreaExitRoomData() const;
    void calcSpan();
    void fast_calcSpan(int);
    void determineAreaExits();
    void determineAreaExitsOfRoom(int);
    void removeRoom(int, bool isToDeferAreaRelatedRecalculations = false);
    QList<int> getCollisionNodes();
    QList<int> getRoomsByPosition(int x, int y, int z);
    QMap<int, QMap<int, QMultiMap<int, int>>> koordinatenSystem();


    QSet<int> rooms; // rooms of this area
    QVector3D pos;   // pos auf der map und 0 punkt des area internen koordinatensystems
    QVector3D span;
    int min_x;
    int min_y;
    int min_z;
    int max_x;
    int max_y;
    int max_z;
    // Key = z-level, Value = the relevent x or y extreme:
    QMap<int, int> xminEbene;
    QMap<int, int> xmaxEbene;
    QMap<int, int> yminEbene;
    QMap<int, int> ymaxEbene;
// Pointless:
//    QMap<int, int> zminEbene;
//    QMap<int, int> zmaxEbene;
    QList<int> ebenen; // The z-levels that ARE used, not guaranteed to be in order
    bool gridMode;
    bool isZone;
    int zoneAreaRef;
    TRoomDB* mpRoomDB;
    bool mIsDirty;
    QMap<QString, QString> mUserData;


private:
    TArea() { qFatal("FATAL: illegal default constructor use of TArea()"); };
    // QMap<int, TMapLabel> labelMap;

    TMap* mpMap; // Supplied by C'tor and now needed to pass an error message upwards
    QMultiMap<int, QPair<int, int>> exits;
    // rooms that border on this area:
    // key=in_area room id, pair.first=out_of_area room id pair.second=direction
    // Made private as we may change implementation detail
};

// - gezeichnet werden erstmal die areas
//   unter berücksichtigung der an die area angrenzenden edges
// - der span der area ist unterschiedlich

#endif // MUDLET_TAREA_H
