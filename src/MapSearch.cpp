/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "MapSearch.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QSet>

#include <algorithm>

MapSearch::Inputs MapSearch::inputsOf(const TMap& map)
{
    Inputs inputs;
    inputs.mapGeneration = map.mpRoomDB->mapGeneration();
    inputs.mapRevision = map.mapRevision();
    for (const TArea* pArea : map.mpRoomDB->getAreaMap()) {
        inputs.roomsVersions = inputs.roomsVersions * 31 + pArea->getRoomsVersion();
        // Labels made for this session only, and labels deleted, leave the map revision as it was; the appearance
        // revision would catch them, but scripts bump that far too often for an index of every name
        for (auto label = pArea->mMapLabels.cbegin(); label != pArea->mMapLabels.cend(); ++label) {
            inputs.labels = inputs.labels * 31 + qHash(label.key()) + qHash(label->text);
        }
    }
    return inputs;
}

bool MapSearch::stale(const TMap& map) const
{
    return !map.mpRoomDB ? !mEntries.isEmpty() : mInputs != inputsOf(map);
}

void MapSearch::clear()
{
    mInputs.reset();
    mEntries.clear();
}

bool MapSearch::refresh(const TMap& map)
{
    if (!stale(map)) {
        return false;
    }
    clear();
    if (!map.mpRoomDB) {
        return true;
    }
    mInputs = inputsOf(map);
    const TRoomDB& roomDB = *map.mpRoomDB;
    mEntries.reserve(roomDB.getRoomMap().size() + roomDB.getAreaNamesMap().size());
    // Only an area with a room to show has a place to fly to
    QSet<int> areasShown;
    const QHash<int, TRoom*>& rooms = roomDB.getRoomMap();
    for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
        const TRoom* pRoom = it.value();
        if (pRoom->isHidden()) {
            continue;
        }
        areasShown.insert(pRoom->getArea());
        // Unnamed rooms are kept too, to be found by their ID
        const QString name = pRoom->name.simplified();
        mEntries.append({Kind::Room, it.key(), pRoom->getArea(), name.toCaseFolded(), name});
    }
    const QMap<int, QString>& areaNames = roomDB.getAreaNamesMap();
    for (auto it = areaNames.cbegin(); it != areaNames.cend(); ++it) {
        const QString name = it.value().simplified();
        if (areasShown.contains(it.key()) && roomDB.getAreaMap().contains(it.key()) && !name.isEmpty()) {
            mEntries.append({Kind::Area, it.key(), it.key(), name.toCaseFolded(), name});
        }
    }
    const QMap<int, TArea*>& areas = roomDB.getAreaMap();
    for (auto it = areas.cbegin(); it != areas.cend(); ++it) {
        for (auto label = it.value()->mMapLabels.cbegin(); label != it.value()->mMapLabels.cend(); ++label) {
            const QString text = label->text.simplified();
            if (!text.isEmpty()) {
                mEntries.append({Kind::Label, label.key(), it.key(), text.toCaseFolded(), text});
            }
        }
    }
    return true;
}

QList<MapSearch::Result> MapSearch::find(const QString& text, const int currentAreaId, const bool labels, const qsizetype limit, qsizetype* total) const
{
    const QString folded = text.simplified().toCaseFolded();
    bool numeric = false;
    const int roomId = folded.toInt(&numeric);
    struct Candidate
    {
        Match match;
        qsizetype index;
    };
    QList<Candidate> candidates;
    if (!folded.isEmpty()) {
        for (qsizetype i = 0; i < mEntries.size(); ++i) {
            const Entry& entry = mEntries.at(i);
            if (entry.kind == Kind::Label && !labels) {
                continue;
            }
            if (entry.folded == folded) {
                candidates.append({Match::Exact, i});
            } else if (numeric && entry.kind == Kind::Room && entry.id == roomId) {
                candidates.append({Match::Exact, i});
            } else if (entry.folded.startsWith(folded)) {
                candidates.append({Match::Prefix, i});
            } else if (entry.folded.contains(folded)) {
                candidates.append({Match::Substring, i});
            }
        }
    }
    if (total) {
        *total = candidates.size();
    }
    const auto better = [this, currentAreaId](const Candidate& a, const Candidate& b) {
        if (a.match != b.match) {
            return a.match < b.match;
        }
        const Entry& first = mEntries.at(a.index);
        const Entry& second = mEntries.at(b.index);
        const bool firstHere = first.areaId == currentAreaId;
        const bool secondHere = second.areaId == currentAreaId;
        if (firstHere != secondHere) {
            return firstHere;
        }
        if (first.kind != second.kind) {
            return first.kind < second.kind;
        }
        if (first.folded != second.folded) {
            return first.folded < second.folded;
        }
        if (first.areaId != second.areaId) {
            return first.areaId < second.areaId;
        }
        return first.id < second.id;
    };
    const qsizetype kept = std::min(limit, candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + kept, candidates.end(), better);
    QList<Result> results;
    results.reserve(kept);
    for (qsizetype i = 0; i < kept; ++i) {
        const Entry& entry = mEntries.at(candidates.at(i).index);
        results.append({entry.kind, candidates.at(i).match, entry.id, entry.areaId, entry.name});
    }
    return results;
}
