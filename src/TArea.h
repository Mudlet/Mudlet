#ifndef MUDLET_TAREA_H
#define MUDLET_TAREA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2020-2023, 2025 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
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


#include "TAreaGridIndex.h"
#include "TAreaLodExitIndex.h"
#include "TAreaSpanIndex.h"
#include "TAreaZLevelIndex.h"
#include "TMap.h"

#include "TMapLabel.h"

#include <QList>
#include <QMap>
#include <QPair>
#include <QVector3D>

class QJsonArray;
class QJsonObject;

class TRoomDB;

class TArea
{
    Q_DECLARE_TR_FUNCTIONS(TArea) // Needed so we can use tr() even though TArea is NOT derived from QObject

    friend bool TMap::serialize(QDataStream&, int);
    friend bool TMap::restore(QString);
    friend bool TMap::retrieveMapFileStats(QString, QString*, int*, int*, qsizetype*, qsizetype*);

public:
    TArea(TMap*, TRoomDB*);
    ~TArea();
    int getAreaID();
    void addRoom(int id);
    const QSet<int>& getAreaRooms() const { return rooms; }
    const QList<int> getAreaExitRoomIds() const { return mAreaExits.uniqueKeys(); }
    const QMultiMap<int, QPair<QString, int>> getAreaExitRoomData() const;
    // Keeps the Z-level index, the grid index and the area extremes in step
    // when a room this area already holds moves to new coordinates; callers
    // must use this rather than updating any of them on their own.  A room
    // joining or leaving the area, including one being deleted, goes through
    // addRoom()/removeRoom() instead.
    void moveRoom(int id, int fromZ, int fromX, int fromY, int toZ, int toX, int toY);
    // Returns the set of room IDs on the given Z level.  The returned reference
    // is stable for the lifetime of the index (an internal empty set is used
    // for Z levels with no rooms), so it can be safely iterated immediately.
    const QSet<int>& getRoomsForZ(int z) const { return mZLevelIndex.roomsForZ(z); }
    // Returns a const reference to the grid index for read-only access by the renderer.
    const TAreaGridIndex& getGridIndex() const { return mGridIndex; }
    // Returns the rooms on the given Z level that have custom exit lines. Such
    // a line can run right across the level, so the renderer has to consider
    // its room even when the room itself is nowhere near the viewport and a
    // viewport query would never hand it over.
    // The set can still be a superset: calcRoomDimensions() drops a room that
    // has lost its last custom line, but the exit removal paths that never
    // recompute a room's dimensions leave their entry behind until
    // removeRoom() or calcSpan() drops it, which costs a cull test rather than
    // a missing line.
    const QSet<int>& getCustomLineRoomsForZ(int z) const { return mCustomLineIndex.roomsForZ(z); }
    // Rooms on the given Z level whose exits can still draw something in the
    // 2D renderer's reduced-detail tier once exits spanning no more than
    // maxSkippableSpan room units per axis are dropped. maxSkippableSpan must
    // be at least 1. A superset - the renderer still runs its usual per-room
    // tests on each one - but never misses a room: see TAreaLodExitIndex.
    QList<int> lodVisibleExitRooms(int z, int maxSkippableSpan) const;
    // How many rooms the above would return, so the renderer can compare
    // against a viewport query without materialising the list.
    int lodVisibleExitRoomCount(int z, int maxSkippableSpan) const;
    void markLodExitIndexDirty() { mLodExitIndex.markDirty(); }
    // Records that one of this area's rooms has custom exit lines.
    void addRoomWithCustomLines(int id, int z);
    // Drops a room that no longer has any. The room stays in every other index
    // this area holds, so this is not a counterpart to removeRoom().
    void removeRoomWithCustomLines(int id, int z);
    void calcSpan();
    void determineAreaExits();
    void determineAreaExitsOfRoom(int);
    // Recomputes the area exit records of this area's rooms that have an exit
    // to the given room, which is what changes when that room joins or leaves
    // this area.
    void refreshAreaExitsToRoom(int);
    void removeRoom(int);
    // List of coordinate triples (x,y,z) where there are multiple rooms
    QList<std::tuple<int, int, int>> getCollisionNodes();
    QList<int> getRoomsByPosition(int x, int y, int z);
    /*
     * Outer key: z coordinate,
     * Middle key: y coordinate
     * Inner key: x coordinate
     * Inner value: is roomId:
     */
    QMap<int, QMap<int, QMultiMap<int, int>>> koordinatenSystem();
    int createLabelId() const;
    void writeJsonArea(QJsonArray&) const;
    std::pair<int, QString> readJsonArea(const QJsonArray&, const int);
    QList<int> getPermanentLabelIds() const;
    bool hasPermanentLabels() const;
    qreal get2DMapZoom() const { return mLast2DMapZoom; }
    void set2DMapZoom(const qreal zoom);
    void clean();


    QSet<int> rooms; // rooms of this area
    // TODO: These next 2 members have not been used for some time - if at all
    // - maybe they can go?
    QVector3D pos; // pos auf der map und 0 punkt des area internen koordinatensystems
    QVector3D span;
    int min_x = 0;
    int min_y = 0;
    int min_z = 0;
    int max_x = 0;
    int max_y = 0;
    int max_z = 0;
    // Key = z-level, Value = the relevant x or y extreme:
    QMap<int, int> xminForZ;
    QMap<int, int> xmaxForZ;
    QMap<int, int> yminForZ;
    QMap<int, int> ymaxForZ;
    QList<int> zLevels; // The z-levels that have rooms, in ascending order
    bool gridMode = false;
    bool isZone = false;
    int zoneAreaRef = 0;
    TRoomDB* mpRoomDB = nullptr;
    bool mIsDirty = false;
    QMap<QString, QString> mUserData;
    QMap<int, TMapLabel> mMapLabels;


private:
    TArea() { qFatal("FATAL: illegal default constructor use of TArea()"); }


    void readJsonUserData(const QJsonObject& obj);
    void writeJsonUserData(QJsonObject&) const;

    void readJsonLabels(const QJsonObject&);
    void writeJsonLabels(QJsonObject&) const;

    void writeJsonLabel(QJsonArray&, const int, const TMapLabel*) const;
    void readJsonLabel(const QJsonObject&);

    QSizeF readJsonSize(const QJsonObject&, const QString&) const;
    void writeJsonSize(QJsonObject&, const QString&, const QSizeF&) const;

    void writeTwinValues(QJsonObject&, const QString&, const QPointF&) const;

    QVector3D readJson3DCoordinates(const QJsonObject&, const QString&) const;
    void writeJson3DCoordinates(QJsonObject&, const QString&, const QVector3D&) const;

    void publishSpan();
    void publishSpanForZ(int z);
    void publishOverallSpan();

    QList<QByteArray> convertImageToBase64Data(const QPixmap&) const;
    QPixmap convertBase64DataToImage(const QList<QByteArray>&) const;


    // Supplied by C'tor and now needed to pass an error message upwards:
    TMap* mpMap = nullptr;
    // Rooms that border on this area:
    // key=in_area room id, pair.first=out_of_area room id pair.second=direction
    // Made private as we may change implementation detail
    QMultiMap<int, QPair<int, int>> mAreaExits;

    // Per-Z-level room index. Maintained incrementally alongside TArea::rooms.
    // Allows paintEvent to iterate only the rooms on a given Z level, avoiding
    // an O(N-total) scan just to find which rooms match the current Z.
    TAreaZLevelIndex mZLevelIndex;
    // Per-(z,x,y) grid index for efficient viewport queries in grid mode.
    TAreaGridIndex mGridIndex;
    // Per-Z-level index of the rooms that have custom exit lines, kept because
    // those are the rooms a viewport query can miss and still owe pixels for.
    TAreaZLevelIndex mCustomLineIndex;
    // Source of truth for the public extremes above (min_x, xminForZ, zLevels
    // and friends), which stay plain members because the map file format
    // stores them and a lot of code reads them directly.
    TAreaSpanIndex mSpanIndex;
    // Unlike the indexes above this one is rebuilt lazily inside the const
    // queries - the renderer only holds a const TArea* and most maps never
    // show the reduced-detail tier - hence mutable.
    mutable TAreaLodExitIndex mLodExitIndex;

    void rebuildLodExitIndex() const;

    // In use this has a minimum of 3.0 and a default of 20.0, the latter will
    // be applied in the constructor initialisation list:
    qreal mLast2DMapZoom = 0.0;
};

#endif // MUDLET_TAREA_H
