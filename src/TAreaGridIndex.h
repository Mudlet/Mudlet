#ifndef MUDLET_TAREA_GRID_INDEX_H
#define MUDLET_TAREA_GRID_INDEX_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <QHash>
#include <QList>
#include <QPair>
#include <QVarLengthArray>

// Three-level spatial index: Z → X → Y → the room IDs in that cell.
// Used by the 2D map renderer to retrieve only the rooms visible in the
// current viewport rather than scanning every room in the area.
class TAreaGridIndex
{
public:
    // The rooms occupying one grid cell. Nearly every cell holds a single
    // room and one Z level of a large area holds a million cells, so the id
    // lives in the cell rather than behind a container's pointer: giving each
    // cell its own QSet meant a scattered allocation to chase per room, which
    // measured as 95% of the time a viewport query took on such a level.
    // A cell never holds the same id twice, which the type no longer says for
    // itself: addRoom() looks before it appends, and the rebuild() overloads
    // take their ids from the keys of a QHash.
    using RoomIds = QVarLengthArray<int, 1>;

    void addRoom(int id, int z, int x, int y);
    void removeRoom(int id, int z, int x, int y);
    void moveRoom(int id, int fromZ, int fromX, int fromY, int toZ, int toX, int toY);

    // Replaces all rooms on the given Z level from a roomId → (x,y) mapping.
    void rebuild(int z, const QHash<int, QPair<int, int>>& roomIdToXY);

    // Replaces the entire index from a pre-bucketed z → (roomId → (x,y)) mapping.
    void rebuild(const QHash<int, QHash<int, QPair<int, int>>>& zToRoomXY);

    // Returns all room IDs at the exact grid cell (z, x, y), in no particular
    // order. Returns a reference to a stable empty cell when it is unoccupied.
    const RoomIds& roomsAt(int z, int x, int y) const;

    // Returns all room IDs whose grid cell lies within the inclusive rectangle
    // [minX..maxX] × [minY..maxY] on the given Z level.
    QList<int> roomsInViewport(int z, int minX, int maxX, int minY, int maxY) const;

    // Like roomsInViewport but also sets the collision flag (true when two or
    // more rooms share the same cell). Avoids a secondary roomsAt() lookup per
    // room in the rendering collect pass.
    QList<QPair<int, bool>> roomsInViewportWithCollisions(int z, int minX, int maxX, int minY, int maxY) const;

    bool isEmpty() const { return mIndex.isEmpty(); }
    void clear()
    {
        mIndex.clear();
        mCachedSize = 0;
        mCachedMemoryEstimate = 0;
    }

    // O(1) — counts are maintained incrementally by add/remove/rebuild.
    int size() const { return mCachedSize; }

    // O(1) — rough memory usage estimate in bytes, for profiling/diagnostics.
    // Refreshed by rebuild() and reset by clear(); may be stale after
    // incremental add/remove/move updates until the next rebuild().
    int memoryEstimateBytes() const { return mCachedMemoryEstimate; }

private:
    int computeMemoryEstimate() const;

    QHash<int, QHash<int, QHash<int, RoomIds>>> mIndex;
    int mCachedSize = 0;
    int mCachedMemoryEstimate = 0;
    static const RoomIds csmEmptyCell;
};

#endif // MUDLET_TAREA_GRID_INDEX_H
