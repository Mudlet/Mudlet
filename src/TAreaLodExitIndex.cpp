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

#include "TAreaLodExitIndex.h"

#include <algorithm>

void TAreaLodExitIndex::beginRebuild()
{
    mIndex.clear();
    mEntries.clear();
    ++mRebuildCount;
}

// Buckets hold a handful of rooms even on a map where the rebuild had to look
// at a million, so sorting them here costs far less than keeping a million ids
// in order just to have them arrive that way.
void TAreaLodExitIndex::endRebuild()
{
    for (auto& bySpan : mIndex) {
        for (auto& bucket : bySpan) {
            std::sort(bucket.begin(), bucket.end());
        }
    }
    mDirty = false;
}

void TAreaLodExitIndex::insertRoom(const int id, const int z, const int span)
{
    if (!worthStoring(span)) {
        return;
    }
    mIndex[z][span].append(id);
    mEntries.insert(id, Entry{z, span});
}

void TAreaLodExitIndex::eraseEntry(const int id, const Entry& entry)
{
    const auto itZ = mIndex.find(entry.z);
    if (itZ == mIndex.end()) {
        return;
    }
    const auto itBucket = itZ->find(entry.span);
    if (itBucket == itZ->end()) {
        return;
    }
    const auto itRoom = std::lower_bound(itBucket->begin(), itBucket->end(), id);
    if (itRoom != itBucket->end() && *itRoom == id) {
        itBucket->erase(itRoom);
    }
    // Buckets and Z levels are iterated wholesale by the queries, so an empty
    // one left behind would be walked on every frame:
    if (itBucket->isEmpty()) {
        itZ->erase(itBucket);
        if (itZ->isEmpty()) {
            mIndex.erase(itZ);
        }
    }
}

void TAreaLodExitIndex::updateRoom(const int id, const int z, const int span)
{
    const auto itEntry = mEntries.find(id);
    if (itEntry != mEntries.end()) {
        if (itEntry->z == z && itEntry->span == span) {
            return;
        }
        eraseEntry(id, *itEntry);
        mEntries.erase(itEntry);
    }
    if (!worthStoring(span)) {
        return;
    }
    QList<int>& bucket = mIndex[z][span];
    bucket.insert(std::lower_bound(bucket.begin(), bucket.end(), id), id);
    mEntries.insert(id, Entry{z, span});
}

void TAreaLodExitIndex::removeRoom(const int id)
{
    const auto itEntry = mEntries.constFind(id);
    if (itEntry == mEntries.constEnd()) {
        return;
    }
    eraseEntry(id, *itEntry);
    mEntries.erase(itEntry);
}

// Thresholds at or above the always-visible bucket would step past it, and
// the rooms in there may never be skipped:
qsizetype TAreaLodExitIndex::roomCountSpanningBeyond(const int z, const int span) const
{
    Q_ASSERT_X(span >= cMinStoredSpan - 1, "TAreaLodExitIndex", "a threshold this low would miss rooms that are not stored");
    const auto itZ = mIndex.constFind(z);
    if (itZ == mIndex.constEnd()) {
        return 0;
    }
    qsizetype count = 0;
    for (auto itBucket = itZ->upperBound(qMin(span, cAlwaysVisibleSpan - 1)); itBucket != itZ->constEnd(); ++itBucket) {
        count += itBucket->size();
    }
    return count;
}

void TAreaLodExitIndex::appendRoomsSpanningBeyond(const int z, const int span, QList<int>& out) const
{
    Q_ASSERT_X(span >= cMinStoredSpan - 1, "TAreaLodExitIndex", "a threshold this low would miss rooms that are not stored");
    const auto itZ = mIndex.constFind(z);
    if (itZ == mIndex.constEnd()) {
        return;
    }
    for (auto itBucket = itZ->upperBound(qMin(span, cAlwaysVisibleSpan - 1)); itBucket != itZ->constEnd(); ++itBucket) {
        out += *itBucket;
    }
}
