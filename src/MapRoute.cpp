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

#include "MapRoute.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <algorithm>

void MapRoute::setDestinationRoom(const int roomId)
{
    clear();
    mDestinationRoomId = roomId;
}

void MapRoute::setDestinationArea(const int areaId)
{
    clear();
    mDestinationAreaId = areaId;
}

void MapRoute::clear()
{
    mDestinationRoomId = 0;
    mDestinationAreaId = 0;
    mInputs.reset();
    setResult(Status::NoDestination);
}

MapRoute::Inputs MapRoute::inputsOf(const TMap& map) const
{
    Inputs inputs;
    inputs.playerRoomId = map.mRoomIdHash.value(map.mProfileName);
    inputs.destinationRoomId = mDestinationRoomId;
    inputs.destinationAreaId = mDestinationAreaId;
    inputs.mapRevision = map.mapRevision();
    inputs.pathGraphRevision = map.pathGraphRevision();
    inputs.graphNeedsUpdate = map.mMapGraphNeedsUpdate;
    if (map.mpRoomDB) {
        inputs.exitsRevision = map.mpRoomDB->exitsRevision();
        inputs.mapGeneration = map.mpRoomDB->mapGeneration();
    }
    return inputs;
}

bool MapRoute::stale(const TMap& map) const
{
    return hasDestination() && (!mInputs || *mInputs != inputsOf(map));
}

bool MapRoute::search(TMap& map, const int from, const int to, QList<int>& rooms, int& weight)
{
    const QList<int> pathList = map.mPathList;
    const QList<QString> dirList = map.mDirList;
    const QList<int> weightList = map.mWeightList;
    const bool found = map.findPath(from, to);
    if (found) {
        rooms = QList<int>{from} + map.mPathList;
        weight = 0;
        for (const int stepWeight : std::as_const(map.mWeightList)) {
            weight += stepWeight;
        }
    }
    map.mPathList = pathList;
    map.mDirList = dirList;
    map.mWeightList = weightList;
    return found;
}

bool MapRoute::refresh(TMap& map)
{
    if (!stale(map)) {
        return false;
    }
    if (!map.mpRoomDB) {
        clear();
        return true;
    }
    // Room and area IDs are not kept from one map to the next
    if (mInputs && mInputs->mapGeneration != map.mpRoomDB->mapGeneration()) {
        clear();
        return true;
    }
    if (map.graphBuildInProgress()) {
        // The route found last stays drawn, and stale, to be found again
        mStatus = Status::Busy;
        return true;
    }

    TRoomDB& roomDB = *map.mpRoomDB;
    const int playerRoomId = map.mRoomIdHash.value(map.mProfileName);
    const TRoom* pPlayerRoom = roomDB.getRoom(playerRoomId);
    if (mDestinationRoomId && !roomDB.getRoom(mDestinationRoomId)) {
        setResult(Status::NoSuchDestination);
    } else if (mDestinationAreaId && !roomDB.getArea(mDestinationAreaId)) {
        setResult(Status::NoSuchDestination);
    } else if (!pPlayerRoom) {
        setResult(Status::NoPlayerRoom);
    } else if (playerRoomId == mDestinationRoomId || pPlayerRoom->getArea() == mDestinationAreaId) {
        setResult(Status::Arrived);
    } else {
        QList<int> targets;
        if (mDestinationRoomId) {
            targets.append(mDestinationRoomId);
        } else {
            // Any route into the area comes in by an exit from outside it, so the nearest of its rooms is one of these
            const QMultiHash<int, int>& entrances = roomDB.getEntranceHash();
            for (const int roomId : roomDB.getArea(mDestinationAreaId)->getAreaRooms()) {
                for (auto it = entrances.constFind(roomId); it != entrances.cend() && it.key() == roomId; ++it) {
                    const TRoom* pFrom = roomDB.getRoom(it.value());
                    if (pFrom && pFrom->getArea() != mDestinationAreaId) {
                        targets.append(roomId);
                        break;
                    }
                }
            }
            std::sort(targets.begin(), targets.end());
        }
        QList<int> bestRooms;
        int bestWeight = 0;
        for (const int target : std::as_const(targets)) {
            QList<int> rooms;
            int weight = 0;
            if (!search(map, playerRoomId, target, rooms, weight)) {
                continue;
            }
            if (bestRooms.isEmpty() || weight < bestWeight || (weight == bestWeight && rooms.size() < bestRooms.size())) {
                bestRooms = rooms;
                bestWeight = weight;
            }
        }
        if (bestRooms.isEmpty()) {
            setResult(Status::Unreachable);
        } else {
            setResult(Status::Found, &roomDB, bestRooms, bestWeight);
        }
    }
    // After the search, which may have built the pathfinding graph
    mInputs = inputsOf(map);
    return true;
}

void MapRoute::setResult(const Status status, TRoomDB* roomDB, const QList<int>& rooms, const int weight)
{
    mStatus = status;
    mRooms = status == Status::Found ? rooms : QList<int>();
    mTotalWeight = status == Status::Found ? weight : 0;
    mAreaRuns = roomDB && status == Status::Found ? areaRunsOf(*roomDB, rooms) : QList<AreaRun>();
    mStepsByArea.clear();
    for (const AreaRun& run : std::as_const(mAreaRuns)) {
        mStepsByArea[run.areaId] += run.steps;
    }
}

QList<MapRoute::AreaRun> MapRoute::areaRunsOf(TRoomDB& roomDB, const QList<int>& rooms)
{
    QList<AreaRun> runs;
    for (qsizetype i = 0; i < rooms.size(); ++i) {
        const TRoom* pR = roomDB.getRoom(rooms.at(i));
        if (!pR) {
            continue;
        }
        if (runs.isEmpty() || runs.last().areaId != pR->getArea()) {
            runs.append({pR->getArea(), 0});
        }
        // The first room is where the route starts, which no step arrives in
        if (i > 0) {
            ++runs.last().steps;
        }
    }
    return runs;
}
