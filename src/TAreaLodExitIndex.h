#ifndef MUDLET_TAREA_LOD_EXIT_INDEX_H
#define MUDLET_TAREA_LOD_EXIT_INDEX_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
#include <QMap>

#include <limits>

/*
 * Rooms whose exits can still produce pixels in the 2D renderer's reduced-detail tier, so at far
 * zooms (where most exits are shorter than a room blob) it need not visit every room.
 *
 * Bucketed by "span": the largest Chebyshev distance, in room units, to any same-area destination
 * of the room's 2D-plane exits. Rooms with an other-area exit or a 2D exit stub draw at any zoom and
 * go in cAlwaysVisibleSpan. Spans below cMinStoredSpan are not stored, keeping the index near-empty
 * on lattice-like maps.
 *
 * Entries may be a superset of what draws but never a subset (that loses pixels), so callers must
 * keep staleness on the too-many side. Custom exit lines are not covered: the renderer must also
 * consult the area's custom-line index.
 *
 * A rebuild is O(rooms in the area), so TArea updates one room at a time and rebuilds only when a
 * whole area is recomputed. Buckets stay sorted by room id so paint order doesn't depend on history.
 */
class TAreaLodExitIndex
{
public:
    // Rooms drawn at any zoom; no threshold may skip them.
    static constexpr int cAlwaysVisibleSpan = std::numeric_limits<int>::max();
    // Thresholds are >= 1 and queries return spans strictly beyond, so lower spans are never returned.
    static constexpr int cMinStoredSpan = 2;

    void markDirty()
    {
        mDirty = true;
        mIndex.clear();
        mEntries.clear();
    }
    bool needsRebuild() const { return mDirty; }
    // For tests.
    quint32 rebuildCount() const { return mRebuildCount; }

    // insertRoom() takes ids in any order; endRebuild() sorts the buckets.
    void beginRebuild();
    void insertRoom(int id, int z, int span);
    void endRebuild();

    // Drops the room if the span no longer qualifies.
    void updateRoom(int id, int z, int span);
    void removeRoom(int id);

    // The count lets the renderer compare against a viewport query's size without building the list.
    qsizetype roomCountSpanningBeyond(int z, int span) const;
    void appendRoomsSpanningBeyond(int z, int span, QList<int>& out) const;

private:
    // Locates a room's bucket for updateRoom() and removeRoom().
    struct Entry
    {
        int z = 0;
        int span = 0;
    };

    static bool worthStoring(const int span) { return span >= cMinStoredSpan; }
    void eraseEntry(int id, const Entry&);

    // Z -> span -> room ids; a QMap so a query can start at the first bucket beyond its threshold.
    QHash<int, QMap<int, QList<int>>> mIndex;
    QHash<int, Entry> mEntries;
    quint32 mRebuildCount = 0;
    bool mDirty = true;
};

#endif // MUDLET_TAREA_LOD_EXIT_INDEX_H
