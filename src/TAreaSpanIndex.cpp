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

#include "TAreaSpanIndex.h"

bool TAreaSpanIndex::addRoom(int x, int y, int z)
{
    increment(mXCounts, x);
    increment(mYCounts, y);

    const auto itZLevel = mPerZ.find(z);
    if (itZLevel == mPerZ.end()) {
        ZLevelCounts& zLevel = mPerZ[z];
        increment(zLevel.xCounts, x);
        increment(zLevel.yCounts, y);
        return true;
    }

    // An overall extreme can only move when the extreme of the room's own Z
    // level moves with it, so this answer covers both.
    const Extremes before = extremesOf(itZLevel.value());
    increment(itZLevel.value().xCounts, x);
    increment(itZLevel.value().yCounts, y);
    return !(before == extremesOf(itZLevel.value()));
}

bool TAreaSpanIndex::removeRoom(int x, int y, int z)
{
    const auto itZLevel = mPerZ.find(z);
    if (itZLevel == mPerZ.end()) {
        return false;
    }

    decrement(mXCounts, x);
    decrement(mYCounts, y);
    const Extremes before = extremesOf(itZLevel.value());
    decrement(itZLevel.value().xCounts, x);
    decrement(itZLevel.value().yCounts, y);
    if (itZLevel.value().xCounts.isEmpty()) {
        mPerZ.erase(itZLevel);
        return true;
    }
    return !(before == extremesOf(itZLevel.value()));
}

TAreaSpanIndex::Extremes TAreaSpanIndex::overallExtremes() const
{
    if (mXCounts.isEmpty()) {
        return {};
    }
    return {mXCounts.firstKey(), mXCounts.lastKey(), mYCounts.firstKey(), mYCounts.lastKey()};
}

TAreaSpanIndex::Extremes TAreaSpanIndex::extremesForZ(int z) const
{
    const auto itZLevel = mPerZ.constFind(z);
    if (itZLevel == mPerZ.constEnd()) {
        return {};
    }
    return extremesOf(itZLevel.value());
}

TAreaSpanIndex::Extremes TAreaSpanIndex::extremesOf(const ZLevelCounts& zLevel)
{
    return {zLevel.xCounts.firstKey(), zLevel.xCounts.lastKey(), zLevel.yCounts.firstKey(), zLevel.yCounts.lastKey()};
}

void TAreaSpanIndex::clear()
{
    mPerZ.clear();
    mXCounts.clear();
    mYCounts.clear();
}

void TAreaSpanIndex::increment(QMap<int, int>& counts, int value)
{
    ++counts[value];
}

void TAreaSpanIndex::decrement(QMap<int, int>& counts, int value)
{
    const auto it = counts.find(value);
    if (it == counts.end()) {
        return;
    }

    if (--it.value() <= 0) {
        counts.erase(it);
    }
}
