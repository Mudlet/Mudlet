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
 * Index of the rooms whose exits can still produce pixels in the 2D
 * renderer's reduced-detail tier, bucketed by how far their longest exit
 * reaches. At the furthest zooms nearly every exit on a large level is
 * shorter than a room blob and is dropped without being gathered, so the
 * renderer wants the handful of rooms that beat that cut handed over
 * without visiting all the others to ask.
 *
 * Each room is stored under its "span": the largest per-axis (Chebyshev)
 * distance in room units from the room to any same-area destination of its
 * 2D-plane exits. Rooms with something that draws at any zoom - an exit
 * into another area (fixed-size marker) or a 2D-plane exit stub - go in the
 * cAlwaysVisibleSpan bucket instead. A span below cMinStoredSpan can never
 * beat a threshold the renderer is allowed to ask about, so those rooms are
 * not stored at all, which is what keeps the index near-empty on
 * lattice-like maps.
 *
 * Entries may be a superset of the rooms that will actually draw - a stored
 * room can still contribute nothing (hidden or dangling destination, room
 * outside the viewport), which costs the renderer its usual per-room tests.
 * A missing room would lose pixels, so every caller has to keep staleness
 * on the too-many side. Note that this covers only what paintRoomExits
 * draws as plain lines from the room's own position: custom exit lines
 * start anywhere and are found through the area's separate custom-line
 * index, which the renderer has to consult as well.
 *
 * Rebuilding is O(rooms in the area) with a destination lookup per exit, so
 * TArea keeps entries current one room at a time as exits and rooms change
 * and only rebuilds wholesale when a whole area is recomputed. Bucket
 * contents stay sorted by room id so that the order the renderer paints in
 * does not depend on how the entries got there.
 */
class TAreaLodExitIndex
{
public:
    // Rooms that draw something at any zoom, so no threshold may skip them.
    static constexpr int cAlwaysVisibleSpan = std::numeric_limits<int>::max();
    // The renderer must pass a threshold of at least 1 and only asks for
    // rooms strictly beyond it, so nothing below this can ever be returned.
    static constexpr int cMinStoredSpan = 2;

    void markDirty()
    {
        mDirty = true;
        mIndex.clear();
        mEntries.clear();
    }
    bool needsRebuild() const { return mDirty; }
    // Wholesale rebuilds so far. Only of interest to the tests, which use it
    // to prove that an ordinary map edit re-files the one room it touched
    // rather than paying for the whole area again.
    quint32 rebuildCount() const { return mRebuildCount; }

    // Wholesale rebuild. insertRoom() takes ids in any order; endRebuild()
    // puts the buckets back in order.
    void beginRebuild();
    void insertRoom(int id, int z, int span);
    void endRebuild();

    // Re-file one room under a freshly computed span, or drop it if the span
    // no longer qualifies. Costs a lookup plus an insertion into one bucket.
    void updateRoom(int id, int z, int span);
    void removeRoom(int id);

    // Rooms on the given Z level with a span beyond the given one. The count
    // lets the renderer compare against a viewport query's size without
    // materialising the list.
    qsizetype roomCountSpanningBeyond(int z, int span) const;
    void appendRoomsSpanningBeyond(int z, int span, QList<int>& out) const;

private:
    // Where one stored room currently sits, so updateRoom() and removeRoom()
    // can find its bucket without searching every one of them.
    struct Entry
    {
        int z = 0;
        int span = 0;
    };

    static bool worthStoring(const int span) { return span >= cMinStoredSpan; }
    void eraseEntry(int id, const Entry&);

    // Outer key: Z level; inner key: span - ordered so a query can start at
    // the first bucket beyond its threshold.
    QHash<int, QMap<int, QList<int>>> mIndex;
    QHash<int, Entry> mEntries;
    quint32 mRebuildCount = 0;
    bool mDirty = true;
};

#endif // MUDLET_TAREA_LOD_EXIT_INDEX_H
