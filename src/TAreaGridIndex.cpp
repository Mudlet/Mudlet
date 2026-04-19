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

#include "TAreaGridIndex.h"

const QSet<int> TAreaGridIndex::csmEmptySet;

void TAreaGridIndex::addRoom(int id, int z, int x, int y)
{
    auto& cell = mIndex[z][x][y];
    if (!cell.contains(id)) {
        cell.insert(id);
        ++mCachedSize;
    }
}

void TAreaGridIndex::removeRoom(int id, int z, int x, int y)
{
    auto zIt = mIndex.find(z);
    if (zIt == mIndex.end()) {
        return;
    }
    auto xIt = zIt->find(x);
    if (xIt == zIt->end()) {
        return;
    }
    auto yIt = xIt->find(y);
    if (yIt == xIt->end()) {
        return;
    }
    if (yIt->remove(id)) {
        --mCachedSize;
    }
    if (yIt->isEmpty()) {
        xIt->erase(yIt);
    }
    if (xIt->isEmpty()) {
        zIt->erase(xIt);
    }
    if (zIt->isEmpty()) {
        mIndex.erase(zIt);
    }
}

void TAreaGridIndex::moveRoom(int id, int fromZ, int fromX, int fromY, int toZ, int toX, int toY)
{
    if (fromZ == toZ && fromX == toX && fromY == toY) {
        return;
    }
    removeRoom(id, fromZ, fromX, fromY);
    addRoom(id, toZ, toX, toY);
}

void TAreaGridIndex::rebuild(int z, const QHash<int, QPair<int, int>>& roomIdToXY)
{
    // Subtract the room count for the Z level being replaced.
    const auto zIt = mIndex.constFind(z);
    if (zIt != mIndex.constEnd()) {
        for (const auto& xMap : *zIt) {
            for (const auto& cell : xMap) {
                mCachedSize -= cell.size();
            }
        }
    }
    mIndex.remove(z);

    // Insert the new rooms.  Each key in roomIdToXY is a unique room ID.
    mCachedSize += roomIdToXY.size();
    QHashIterator<int, QPair<int, int>> it(roomIdToXY);
    while (it.hasNext()) {
        it.next();
        mIndex[z][it.value().first][it.value().second].insert(it.key());
    }
    mCachedMemoryEstimate = computeMemoryEstimate();
}

void TAreaGridIndex::rebuild(const QHash<int, QHash<int, QPair<int, int>>>& zToRoomXY)
{
    // Full rebuild: bypass the per-Z path so computeMemoryEstimate() is called
    // only once rather than once per Z level.
    mIndex.clear();
    mCachedSize = 0;
    for (auto itZ = zToRoomXY.constBegin(); itZ != zToRoomXY.constEnd(); ++itZ) {
        const int z = itZ.key();
        for (auto it = itZ.value().constBegin(); it != itZ.value().constEnd(); ++it) {
            mIndex[z][it.value().first][it.value().second].insert(it.key());
        }
        mCachedSize += itZ.value().size();
    }
    mCachedMemoryEstimate = computeMemoryEstimate();
}

const QSet<int>& TAreaGridIndex::roomsAt(int z, int x, int y) const
{
    const auto zIt = mIndex.constFind(z);
    if (zIt == mIndex.constEnd()) {
        return csmEmptySet;
    }
    const auto xIt = zIt->constFind(x);
    if (xIt == zIt->constEnd()) {
        return csmEmptySet;
    }
    const auto yIt = xIt->constFind(y);
    if (yIt == xIt->constEnd()) {
        return csmEmptySet;
    }
    return *yIt;
}

QList<int> TAreaGridIndex::roomsInViewport(int z, int minX, int maxX, int minY, int maxY) const
{
    QList<int> result;
    const auto zIt = mIndex.constFind(z);
    if (zIt == mIndex.constEnd()) {
        return result;
    }
    const auto& xyMap = *zIt;
    for (int x = minX; x <= maxX; ++x) {
        const auto xIt = xyMap.constFind(x);
        if (xIt == xyMap.constEnd()) {
            continue;
        }
        const auto& yMap = *xIt;
        for (int y = minY; y <= maxY; ++y) {
            const auto yIt = yMap.constFind(y);
            if (yIt == yMap.constEnd()) {
                continue;
            }
            for (const int roomId : *yIt) {
                result.append(roomId);
            }
        }
    }
    return result;
}

QList<QPair<int, bool>> TAreaGridIndex::roomsInViewportWithCollisions(int z, int minX, int maxX, int minY, int maxY) const
{
    QList<QPair<int, bool>> result;
    const auto zIt = mIndex.constFind(z);
    if (zIt == mIndex.constEnd()) {
        return result;
    }
    const auto& xyMap = *zIt;
    for (int x = minX; x <= maxX; ++x) {
        const auto xIt = xyMap.constFind(x);
        if (xIt == xyMap.constEnd()) {
            continue;
        }
        const auto& yMap = *xIt;
        for (int y = minY; y <= maxY; ++y) {
            const auto yIt = yMap.constFind(y);
            if (yIt == yMap.constEnd()) {
                continue;
            }
            const bool collision = yIt->size() > 1;
            for (const int roomId : *yIt) {
                result.append({roomId, collision});
            }
        }
    }
    return result;
}

int TAreaGridIndex::computeMemoryEstimate() const
{
    // Count the number of containers at each nesting level to give a
    // better-than-nothing estimate.  Qt QHash internals allocate roughly
    // 48 bytes of base overhead plus ~16 bytes per stored entry at a typical
    // load factor.  These constants are intentionally conservative.
    constexpr int kHashBase = 48;
    constexpr int kHashEntry = 16;

    int zCount = mIndex.size();
    int xCount = 0;
    int yCount = 0;
    int roomCount = 0;
    for (auto zIt = mIndex.constBegin(); zIt != mIndex.constEnd(); ++zIt) {
        xCount += zIt->size();
        for (auto xIt = zIt->constBegin(); xIt != zIt->constEnd(); ++xIt) {
            yCount += xIt->size();
            for (auto yIt = xIt->constBegin(); yIt != xIt->constEnd(); ++yIt) {
                roomCount += yIt->size();
            }
        }
    }
    return zCount * (kHashBase + kHashEntry) + xCount * (kHashBase + kHashEntry) + yCount * (kHashBase + kHashEntry) + roomCount * static_cast<int>(sizeof(int));
}
