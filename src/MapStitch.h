#ifndef MUDLET_MAPSTITCH_H
#define MUDLET_MAPSTITCH_H

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

#include <QColor>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QVector3D>

#include <functional>

class TMap;

// The areas next to the one the modern 3D view shows, placed in its coordinates where the exits between them
// say they lie, so that the view can draw their rooms past its edge; and, for each exit from the viewed level
// into an area that cannot be placed so, what the view shows at the exit instead: the area's name and a few
// of its rooms around the one the exit leads to. Made once for an area and level, until the map changes.
class MapStitch
{
public:
    // A neighbor's rooms are kept this many room spacings, or up to twice that, from the viewed level's rooms
    static constexpr int scmMarginSteps = 10;
    // A neighbor with more than one in this many of its rooms landing on rooms already placed is not placed
    static constexpr int scmOverlapShare = 20;
    // How many of its room spacings around the room an exit leads to a preview reaches, and how many rooms it shows
    static constexpr int scmPreviewSteps = 3;
    static constexpr int scmPreviewRooms = 49;

    // Added to a neighbor's room coordinates to place it in the viewed area's
    struct Offset
    {
        int x = 0;
        int y = 0;
        int z = 0;
        bool operator==(const Offset&) const = default;
        QVector3D vector() const { return QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }
    };

    enum class Placement {
        Stitched,
        // Joined only by up or down exits, which have no compass direction to place it by
        NoCompassLinks,
        Overlaps
    };

    struct Neighbour
    {
        int areaId = 0;
        Offset offset;
        // Compass exits either way between the two areas, and how many of them put it at the offset chosen
        int links = 0;
        int agreeing = 0;
        // Its rooms that would land on rooms of the viewed area, or of a neighbor placed before it
        int collisions = 0;
        Placement placement = Placement::NoCompassLinks;
    };

    struct Room
    {
        int id = 0;
        int areaId = 0;
        // In the viewed area's coordinates, on the viewed level
        QVector3D at;
        QColor color;
    };

    struct PreviewRoom
    {
        // From the room the exit leads to, in that area's room spacings
        QPointF at;
        QColor color;
        bool target = false;
    };

    struct Portal
    {
        int fromRoomId = 0;
        int direction = 0;
        // Where the exit leaves, in the viewed area's coordinates
        QVector3D from;
        int toRoomId = 0;
        int toAreaId = 0;
        QList<PreviewRoom> preview;
    };

    // The offset most of the votes are for; a tie goes to the smallest move, then the lowest x, y and z
    static Offset chooseOffset(const QList<Offset>& votes, int* agreeing = nullptr);
    // The usual length of the area's compass exits, in map units, at least 1
    static int spacingOf(const TMap& map, int areaId);

    // environmentColor gives a room color from its environment
    void build(const TMap& map, int areaId, int level, const std::function<QColor(int)>& environmentColor);
    void clear();

    int areaId() const { return mAreaId; }
    int level() const { return mLevel; }
    int spacing() const { return mSpacing; }
    // Every area joined to this one by a compass, up or down exit either way, most links first
    const QList<Neighbour>& neighbours() const { return mNeighbours; }
    const Neighbour* neighbour(int areaId) const;
    // The rooms of the neighbors placed, on the viewed level and near its rooms
    const QList<Room>& rooms() const { return mRooms; }
    const Room* room(int roomId) const;
    const QList<Portal>& portals() const { return mPortals; }

private:
    int mAreaId = 0;
    int mLevel = 0;
    int mSpacing = 1;
    QList<Neighbour> mNeighbours;
    QHash<int, qsizetype> mNeighbourIndex;
    QList<Room> mRooms;
    QHash<int, qsizetype> mRoomIndex;
    QList<Portal> mPortals;
};

#endif // MUDLET_MAPSTITCH_H
