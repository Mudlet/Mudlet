#ifndef MUDLET_TAREA_SPAN_INDEX_H
#define MUDLET_TAREA_SPAN_INDEX_H

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

#include <QList>
#include <QMap>

/*
 * Maintains an area's coordinate extremes - overall and per Z level - as rooms
 * are added, moved and removed, so that none of those operations has to rescan
 * the area's rooms.
 *
 * Coordinates are bucketed into sorted count maps keyed by the coordinate
 * value, so the number of entries is the number of *distinct* coordinates
 * rather than the number of rooms, and an extreme is the first or last key of
 * the relevant map.
 *
 * The y coordinates are those of TArea::min_y and friends, i.e. the negated
 * TRoom::y(), so that callers do not have to flip the sense of the answers.
 */
class TAreaSpanIndex
{
public:
    // The lowest and highest coordinate in use, either overall or on one Z
    // level. Only meaningful for a level that has rooms.
    struct Extremes
    {
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;

        bool operator==(const Extremes& other) const { return minX == other.minX && maxX == other.maxX && minY == other.minY && maxY == other.maxY; }
    };

    // Both return whether the extremes of the affected Z level moved, so that
    // callers can skip republishing them - during a map load that is the case
    // for all but a handful of the rooms.
    bool addRoom(int x, int y, int z);
    bool removeRoom(int x, int y, int z);
    void clear();

    bool empty() const { return mPerZ.isEmpty(); }
    bool hasZ(int z) const { return mPerZ.contains(z); }
    // Ascending, as TArea::zLevels is expected (and saved) sorted.
    QList<int> zLevels() const { return mPerZ.keys(); }

    // All four answer with zeroes when there is nothing to measure, which is
    // never a meaningful extreme, so callers check empty() / hasZ() first.
    int minZ() const { return mPerZ.isEmpty() ? 0 : mPerZ.firstKey(); }
    int maxZ() const { return mPerZ.isEmpty() ? 0 : mPerZ.lastKey(); }
    Extremes overallExtremes() const;
    Extremes extremesForZ(int z) const;

private:
    struct ZLevelCounts
    {
        QMap<int, int> xCounts;
        QMap<int, int> yCounts;
    };

    static Extremes extremesOf(const ZLevelCounts& zLevel);
    static void increment(QMap<int, int>& counts, int value);
    static void decrement(QMap<int, int>& counts, int value);

    QMap<int, ZLevelCounts> mPerZ;
    QMap<int, int> mXCounts;
    QMap<int, int> mYCounts;
};

#endif // MUDLET_TAREA_SPAN_INDEX_H
