#ifndef MUDLET_TAREA_ZLEVEL_INDEX_H
#define MUDLET_TAREA_ZLEVEL_INDEX_H

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
#include <QSet>

/*
 * Maintains an incrementally-updated reverse index of room IDs by Z level.
 * Allows roomsForZ() to return only the rooms on a given level without
 * scanning the full area room set.
 */
class TAreaZLevelIndex
{
public:
    void addRoom(int id, int z);
    void removeRoom(int id, int z);
    void moveRoom(int id, int fromZ, int toZ);

    // Replaces the entire index from a roomId → Z mapping.
    void rebuild(const QHash<int, int>& roomIdToZ);

    // Returns all room IDs on the given Z level, or a stable empty set.
    const QSet<int>& roomsForZ(int z) const;

    bool isEmpty() const { return mIndex.isEmpty(); }
    void clear() { mIndex.clear(); }

private:
    QHash<int, QSet<int>> mIndex;
    static const QSet<int> csmEmptySet;
};

#endif // MUDLET_TAREA_ZLEVEL_INDEX_H
