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

#include "TAreaZLevelIndex.h"

const QSet<int> TAreaZLevelIndex::csmEmptySet;

void TAreaZLevelIndex::addRoom(int id, int z)
{
    mIndex[z].insert(id);
}

void TAreaZLevelIndex::removeRoom(int id, int z)
{
    auto it = mIndex.find(z);
    if (it == mIndex.end()) {
        return;
    }
    it->remove(id);
    if (it->isEmpty()) {
        mIndex.erase(it);
    }
}

void TAreaZLevelIndex::moveRoom(int id, int fromZ, int toZ)
{
    if (fromZ == toZ) {
        return;
    }
    removeRoom(id, fromZ);
    addRoom(id, toZ);
}

void TAreaZLevelIndex::rebuild(const QHash<int, int>& roomIdToZ)
{
    mIndex.clear();
    QHashIterator<int, int> it(roomIdToZ);
    while (it.hasNext()) {
        it.next();
        mIndex[it.value()].insert(it.key());
    }
}

const QSet<int>& TAreaZLevelIndex::roomsForZ(int z) const
{
    const auto it = mIndex.constFind(z);
    if (it == mIndex.constEnd()) {
        return csmEmptySet;
    }
    return *it;
}
