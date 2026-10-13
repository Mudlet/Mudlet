#ifndef MUDLET_MAPROUTE_H
#define MUDLET_MAPROUTE_H

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

#include <QHash>
#include <QList>

#include <optional>

class TMap;
class TRoomDB;

// A route from the player's room to a room, or to the nearest room of an area that can be reached, found by the
// map's own pathfinding, with its weights, locks and exit rules. Found again only once the player has moved or
// the map has changed since.
class MapRoute
{
public:
    enum class Status {
        NoDestination,
        Found,
        // The player is in the room, or the area, the route was to
        Arrived,
        Unreachable,
        NoPlayerRoom,
        // The room or area the route was to is no longer in the map
        NoSuchDestination,
        // The map's pathfinding graph is being built, by a script's exit weight filter; try again later
        Busy
    };

    // A stretch of the route in one area. An area left and entered again has a run each time
    struct AreaRun
    {
        int areaId = 0;
        // The moves that arrive in a room of the area
        int steps = 0;
        bool operator==(const AreaRun&) const = default;
    };

    void setDestinationRoom(int roomId);
    void setDestinationArea(int areaId);
    void clear();
    bool hasDestination() const { return mDestinationRoomId || mDestinationAreaId; }
    int destinationRoomId() const { return mDestinationRoomId; }
    int destinationAreaId() const { return mDestinationAreaId; }

    // Whether the player moved, the destination changed or the map changed since the route was last found
    bool stale(const TMap& map) const;
    // Finds the route again if it is stale; true if it did. Can run the exit weight filter script, so not
    // to be called while painting
    bool refresh(TMap& map);

    Status status() const { return mStatus; }
    // From the player's room to the destination, both included; empty unless Found, or Busy finding a found route again
    const QList<int>& rooms() const { return mRooms; }
    // The room the route ends in, which for an area is the nearest of its rooms that can be reached
    int endRoomId() const { return mRooms.isEmpty() ? 0 : mRooms.last(); }
    int totalWeight() const { return mTotalWeight; }
    int steps() const { return mRooms.isEmpty() ? 0 : static_cast<int>(mRooms.size()) - 1; }
    const QList<AreaRun>& areaRuns() const { return mAreaRuns; }
    // The steps the route takes in each area, over all its runs there
    const QHash<int, int>& stepsByArea() const { return mStepsByArea; }

    static QList<AreaRun> areaRunsOf(TRoomDB& roomDB, const QList<int>& rooms);

private:
    struct Inputs
    {
        int playerRoomId = 0;
        int destinationRoomId = 0;
        int destinationAreaId = 0;
        quint64 mapRevision = 0;
        quint64 exitsRevision = 0;
        unsigned int mapGeneration = 0;
        quint64 pathGraphRevision = 0;
        bool graphNeedsUpdate = false;
        bool operator==(const Inputs&) const = default;
    };
    Inputs inputsOf(const TMap& map) const;
    // The map's findPath(), but leaving its last path as it was for the speedwalk or script that asked for it
    static bool search(TMap& map, int from, int to, QList<int>& rooms, int& weight);
    void setResult(Status status, TRoomDB* roomDB = nullptr, const QList<int>& rooms = {}, int weight = 0);

    int mDestinationRoomId = 0;
    int mDestinationAreaId = 0;
    std::optional<Inputs> mInputs;
    Status mStatus = Status::NoDestination;
    QList<int> mRooms;
    int mTotalWeight = 0;
    QList<AreaRun> mAreaRuns;
    QHash<int, int> mStepsByArea;
};

#endif // MUDLET_MAPROUTE_H
