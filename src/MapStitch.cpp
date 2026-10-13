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

#include "MapStitch.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QMap>
#include <QSet>
#include <algorithm>
#include <cstdlib>
#include <tuple>

namespace {
constexpr int scmCompassDirections[] = {DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST, DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST};
// The exits drawn as a stub at the edge of the area, which is where a portal goes
constexpr int scmStubDirections[] = {DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST, DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST, DIR_UP, DIR_DOWN};

struct Cell
{
    int x = 0;
    int y = 0;
    int z = 0;
    bool operator==(const Cell&) const = default;
    friend size_t qHash(const Cell& cell, const size_t seed = 0) { return qHashMulti(seed, cell.x, cell.y, cell.z); }
};

// One room spacing that way, in map coordinates, where north is +y
Cell stepOf(const int direction, const int spacing)
{
    const QVector3D unit = TMap::scmUnitVectors.value(direction);
    return {static_cast<int>(unit.x()) * spacing, -static_cast<int>(unit.y()) * spacing, static_cast<int>(unit.z()) * spacing};
}

int floorDiv(const int value, const int divisor)
{
    const int quotient = value / divisor;
    return (value % divisor != 0 && (value < 0) != (divisor < 0)) ? quotient - 1 : quotient;
}

const TRoom* shownRoom(const TMap& map, const int roomId)
{
    const TRoom* pRoom = map.mpRoomDB->getRoom(roomId);
    return pRoom && !pRoom->isHidden() ? pRoom : nullptr;
}
} // namespace

MapStitch::Offset MapStitch::chooseOffset(const QList<Offset>& votes, int* agreeing)
{
    QList<std::pair<Offset, int>> counts;
    for (const Offset& vote : votes) {
        auto found = std::find_if(counts.begin(), counts.end(), [&vote](const std::pair<Offset, int>& count) {
            return count.first == vote;
        });
        if (found == counts.end()) {
            counts.append({vote, 1});
        } else {
            ++found->second;
        }
    }
    const auto better = [](const std::pair<Offset, int>& a, const std::pair<Offset, int>& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        const int aMove = std::abs(a.first.x) + std::abs(a.first.y) + std::abs(a.first.z);
        const int bMove = std::abs(b.first.x) + std::abs(b.first.y) + std::abs(b.first.z);
        if (aMove != bMove) {
            return aMove < bMove;
        }
        return std::tie(a.first.x, a.first.y, a.first.z) < std::tie(b.first.x, b.first.y, b.first.z);
    };
    std::pair<Offset, int> best;
    for (const std::pair<Offset, int>& count : std::as_const(counts)) {
        if (best.second == 0 || better(count, best)) {
            best = count;
        }
    }
    if (agreeing) {
        *agreeing = best.second;
    }
    return best.first;
}

int MapStitch::spacingOf(const TMap& map, const int areaId)
{
    const TArea* pArea = map.mpRoomDB ? map.mpRoomDB->getArea(areaId) : nullptr;
    if (!pArea) {
        return 1;
    }
    QList<int> lengths;
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pRoom = map.mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }
        for (const int direction : scmCompassDirections) {
            const TRoom* pTo = map.mpRoomDB->getRoom(pRoom->getExit(direction));
            if (!pTo || pTo->getArea() != areaId || pTo->z() != pRoom->z()) {
                continue;
            }
            const int length = std::max(std::abs(pTo->x() - pRoom->x()), std::abs(pTo->y() - pRoom->y()));
            if (length > 0) {
                lengths.append(length);
            }
        }
    }
    if (lengths.isEmpty()) {
        return 1;
    }
    auto middle = lengths.begin() + lengths.size() / 2;
    std::nth_element(lengths.begin(), middle, lengths.end());
    return *middle;
}

void MapStitch::clear()
{
    mAreaId = 0;
    mLevel = 0;
    mSpacing = 1;
    mNeighbours.clear();
    mNeighbourIndex.clear();
    mRooms.clear();
    mRoomIndex.clear();
    mPortals.clear();
}

const MapStitch::Neighbour* MapStitch::neighbour(const int areaId) const
{
    const auto found = mNeighbourIndex.constFind(areaId);
    return found == mNeighbourIndex.cend() ? nullptr : &mNeighbours.at(*found);
}

const MapStitch::Room* MapStitch::room(const int roomId) const
{
    const auto found = mRoomIndex.constFind(roomId);
    return found == mRoomIndex.cend() ? nullptr : &mRooms.at(*found);
}

void MapStitch::build(const TMap& map, const int areaId, const int level, const std::function<QColor(int)>& environmentColor)
{
    clear();
    mAreaId = areaId;
    mLevel = level;
    TArea* pArea = map.mpRoomDB ? map.mpRoomDB->getArea(areaId) : nullptr;
    if (!pArea) {
        return;
    }
    mSpacing = spacingOf(map, areaId);

    // Each compass exit either way between the areas is a vote for where the other area lies
    QMap<int, QList<Offset>> votes;
    QSet<int> joined;
    const auto vote = [&](const TRoom& inArea, const TRoom& outside, const int direction, const int sign) {
        const Cell step = stepOf(direction, mSpacing);
        votes[outside.getArea()].append({inArea.x() + sign * step.x - outside.x(), inArea.y() + sign * step.y - outside.y(), inArea.z() + sign * step.z - outside.z()});
    };
    for (const int roomId : pArea->getAreaExitRoomIds()) {
        const TRoom* pRoom = shownRoom(map, roomId);
        if (!pRoom) {
            continue;
        }
        for (const int direction : scmStubDirections) {
            const TRoom* pTo = shownRoom(map, pRoom->getExit(direction));
            if (!pTo || pTo->getArea() == areaId) {
                continue;
            }
            joined.insert(pTo->getArea());
            if (direction != DIR_UP && direction != DIR_DOWN) {
                vote(*pRoom, *pTo, direction, 1);
            }
        }
    }
    for (auto it = map.mpRoomDB->getAreaMap().cbegin(); it != map.mpRoomDB->getAreaMap().cend(); ++it) {
        if (it.key() == areaId || !it.value()) {
            continue;
        }
        for (const int roomId : it.value()->getAreaExitRoomIds()) {
            const TRoom* pRoom = shownRoom(map, roomId);
            if (!pRoom) {
                continue;
            }
            for (const int direction : scmStubDirections) {
                const TRoom* pTo = shownRoom(map, pRoom->getExit(direction));
                if (!pTo || pTo->getArea() != areaId) {
                    continue;
                }
                joined.insert(it.key());
                if (direction != DIR_UP && direction != DIR_DOWN) {
                    vote(*pTo, *pRoom, direction, -1);
                }
            }
        }
    }

    for (const int neighbourId : std::as_const(joined)) {
        Neighbour neighbour;
        neighbour.areaId = neighbourId;
        const QList<Offset> areaVotes = votes.value(neighbourId);
        neighbour.links = static_cast<int>(areaVotes.size());
        if (!areaVotes.isEmpty()) {
            neighbour.offset = chooseOffset(areaVotes, &neighbour.agreeing);
        }
        mNeighbours.append(neighbour);
    }
    std::sort(mNeighbours.begin(), mNeighbours.end(), [](const Neighbour& a, const Neighbour& b) {
        return a.links != b.links ? a.links > b.links : a.areaId < b.areaId;
    });

    // Placed in that order, each kept off the rooms of the area and of the neighbors placed before it
    QSet<Cell> occupied;
    for (const int roomId : pArea->getAreaRooms()) {
        if (const TRoom* pRoom = shownRoom(map, roomId)) {
            occupied.insert({pRoom->x(), pRoom->y(), pRoom->z()});
        }
    }
    QSet<int> dropped;
    QList<std::pair<const Neighbour*, const TArea*>> placed;
    for (qsizetype i = 0; i < mNeighbours.size(); ++i) {
        Neighbour& neighbour = mNeighbours[i];
        mNeighbourIndex.insert(neighbour.areaId, i);
        const TArea* pNeighbourArea = map.mpRoomDB->getArea(neighbour.areaId);
        if (!pNeighbourArea || !neighbour.links) {
            continue;
        }
        QList<int> landing;
        QList<Cell> cells;
        for (const int roomId : pNeighbourArea->getAreaRooms()) {
            const TRoom* pRoom = shownRoom(map, roomId);
            if (!pRoom) {
                continue;
            }
            const Cell cell{pRoom->x() + neighbour.offset.x, pRoom->y() + neighbour.offset.y, pRoom->z() + neighbour.offset.z};
            if (occupied.contains(cell)) {
                landing.append(roomId);
            } else {
                cells.append(cell);
            }
        }
        neighbour.collisions = static_cast<int>(landing.size());
        if (static_cast<qsizetype>(neighbour.collisions) * scmOverlapShare > landing.size() + cells.size()) {
            neighbour.placement = Placement::Overlaps;
            continue;
        }
        neighbour.placement = Placement::Stitched;
        for (const Cell& cell : std::as_const(cells)) {
            occupied.insert(cell);
        }
        for (const int roomId : std::as_const(landing)) {
            dropped.insert(roomId);
        }
        placed.append({&neighbour, pNeighbourArea});
    }

    // Near the viewed level's rooms: in the same square of this size as one of them, or in one of the eight around it
    const int margin = scmMarginSteps * mSpacing;
    QSet<Cell> nearSquares;
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pRoom = shownRoom(map, roomId);
        if (!pRoom || pRoom->z() != level) {
            continue;
        }
        const int column = floorDiv(pRoom->x(), margin);
        const int row = floorDiv(pRoom->y(), margin);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                nearSquares.insert({column + dx, row + dy, 0});
            }
        }
    }
    for (const auto& [pNeighbour, pNeighbourArea] : std::as_const(placed)) {
        const Offset offset = pNeighbour->offset;
        for (const int roomId : pNeighbourArea->getAreaRooms()) {
            const TRoom* pRoom = shownRoom(map, roomId);
            if (!pRoom || pRoom->z() + offset.z != level || dropped.contains(roomId)) {
                continue;
            }
            const int x = pRoom->x() + offset.x;
            const int y = pRoom->y() + offset.y;
            if (!nearSquares.contains({floorDiv(x, margin), floorDiv(y, margin), 0})) {
                continue;
            }
            mRooms.append({roomId, pNeighbour->areaId, QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(level)), environmentColor(pRoom->environment)});
        }
    }
    std::sort(mRooms.begin(), mRooms.end(), [](const Room& a, const Room& b) {
        return a.id < b.id;
    });
    for (qsizetype i = 0; i < mRooms.size(); ++i) {
        mRoomIndex.insert(mRooms.at(i).id, i);
    }

    // The rooms of an area around a room, found by squares of the preview's reach, made once an area needs them
    struct PreviewIndex
    {
        int spacing = 1;
        int reach = 1;
        QHash<Cell, QList<int>> squares;
    };
    QHash<int, PreviewIndex> previewIndexes;
    const auto previewIndex = [&](const int previewAreaId) -> const PreviewIndex& {
        auto found = previewIndexes.find(previewAreaId);
        if (found != previewIndexes.end()) {
            return *found;
        }
        PreviewIndex index;
        index.spacing = spacingOf(map, previewAreaId);
        index.reach = scmPreviewSteps * index.spacing;
        if (const TArea* pPreviewArea = map.mpRoomDB->getArea(previewAreaId)) {
            for (const int roomId : pPreviewArea->getAreaRooms()) {
                if (const TRoom* pRoom = shownRoom(map, roomId)) {
                    index.squares[{floorDiv(pRoom->x(), index.reach + 1), floorDiv(pRoom->y(), index.reach + 1), pRoom->z()}].append(roomId);
                }
            }
        }
        return *previewIndexes.insert(previewAreaId, index);
    };

    for (const int roomId : pArea->getAreaExitRoomIds()) {
        const TRoom* pRoom = shownRoom(map, roomId);
        if (!pRoom || pRoom->z() != level) {
            continue;
        }
        for (const int direction : scmStubDirections) {
            const int toRoomId = pRoom->getExit(direction);
            const TRoom* pTo = shownRoom(map, toRoomId);
            // An exit with a custom line has no stub to put a portal on
            if (!pTo || pTo->getArea() == areaId || pRoom->customLines.contains(TRoom::dirCodeToShortString(direction))) {
                continue;
            }
            if (const Room* pPlaced = room(toRoomId); pPlaced && direction != DIR_UP && direction != DIR_DOWN) {
                const Cell step = stepOf(direction, mSpacing);
                if (pPlaced->at == QVector3D(static_cast<float>(pRoom->x() + step.x), static_cast<float>(pRoom->y() + step.y), static_cast<float>(level))) {
                    continue;
                }
            }
            Portal portal;
            portal.fromRoomId = roomId;
            portal.direction = direction;
            portal.from = QVector3D(static_cast<float>(pRoom->x()), static_cast<float>(pRoom->y()), static_cast<float>(pRoom->z()));
            portal.toRoomId = toRoomId;
            portal.toAreaId = pTo->getArea();
            const PreviewIndex& index = previewIndex(portal.toAreaId);
            const int column = floorDiv(pTo->x(), index.reach + 1);
            const int row = floorDiv(pTo->y(), index.reach + 1);
            QList<std::pair<std::pair<int, int>, PreviewRoom>> near;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    for (const int nearId : index.squares.value({column + dx, row + dy, pTo->z()})) {
                        const TRoom* pNear = map.mpRoomDB->getRoom(nearId);
                        const int distance = std::max(std::abs(pNear->x() - pTo->x()), std::abs(pNear->y() - pTo->y()));
                        if (distance > index.reach) {
                            continue;
                        }
                        const QPointF at(static_cast<qreal>(pNear->x() - pTo->x()) / index.spacing, static_cast<qreal>(pNear->y() - pTo->y()) / index.spacing);
                        near.append({{distance, nearId}, {at, environmentColor(pNear->environment), nearId == toRoomId}});
                    }
                }
            }
            std::sort(near.begin(), near.end(), [](const auto& a, const auto& b) {
                return a.first < b.first;
            });
            for (qsizetype i = 0; i < near.size() && i < scmPreviewRooms; ++i) {
                portal.preview.append(near.at(i).second);
            }
            mPortals.append(portal);
        }
    }
}
