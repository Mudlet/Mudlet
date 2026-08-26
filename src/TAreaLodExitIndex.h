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
 * cAlwaysVisibleSpan bucket instead. A span below two can never beat a
 * threshold the renderer is allowed to ask about (it must pass a
 * maxSkippableSpan of at least 1), so those rooms are not stored at all,
 * which is what keeps the index near-empty on lattice-like maps.
 *
 * The contents are a conservative superset of what will draw: a stored room
 * may still contribute nothing (hidden or dangling destination, room
 * outside the viewport), which costs the renderer its usual per-room tests,
 * but a missing room would lose pixels, so staleness is only ever allowed
 * in the too-many direction. TArea marks the index dirty on any room or
 * exit change and rebuilds it wholesale on the next query - queries only
 * happen while the reduced-detail tier is actually on screen.
 */
class TAreaLodExitIndex
{
public:
    static constexpr int cAlwaysVisibleSpan = std::numeric_limits<int>::max();

    void markDirty()
    {
        mDirty = true;
        mIndex.clear();
    }
    bool needsRebuild() const { return mDirty; }

    void beginRebuild() { mIndex.clear(); }
    void insertRoom(int id, int z, int span);
    void endRebuild() { mDirty = false; }

    // How many rooms on the given Z level have a span beyond the given one.
    // Lets the renderer compare against a viewport query's size without
    // materialising the list.
    int roomCountSpanningBeyond(int z, int span) const;
    void appendRoomsSpanningBeyond(int z, int span, QList<int>& out) const;

private:
    // Outer key: Z level; inner key: span - ordered so a query can start at
    // the first bucket beyond its threshold.
    QHash<int, QMap<int, QList<int>>> mIndex;
    bool mDirty = true;
};

#endif // MUDLET_TAREA_LOD_EXIT_INDEX_H
