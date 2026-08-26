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

void TAreaLodExitIndex::insertRoom(const int id, const int z, const int span)
{
    mIndex[z][span].append(id);
}

int TAreaLodExitIndex::roomCountSpanningBeyond(const int z, const int span) const
{
    const auto itZ = mIndex.constFind(z);
    if (itZ == mIndex.constEnd()) {
        return 0;
    }
    int count = 0;
    for (auto itBucket = itZ->upperBound(span); itBucket != itZ->constEnd(); ++itBucket) {
        count += itBucket->size();
    }
    return count;
}

void TAreaLodExitIndex::appendRoomsSpanningBeyond(const int z, const int span, QList<int>& out) const
{
    const auto itZ = mIndex.constFind(z);
    if (itZ == mIndex.constEnd()) {
        return;
    }
    for (auto itBucket = itZ->upperBound(span); itBucket != itZ->constEnd(); ++itBucket) {
        out += *itBucket;
    }
}
