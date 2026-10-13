#ifndef MUDLET_MAPATLAS_H
#define MUDLET_MAPATLAS_H

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
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QString>
#include <QVector>

#include <functional>

class TMap;

// What the modern 3D view draws of the map when zoomed out past its rooms: each area as the outline its rooms
// cover, then the world as a graph of areas joined where exits cross between them. Areas have no shared
// coordinates, so everything here is in world coordinates, where an area's rooms are moved by its offset to
// sit at its place in the map's area graph (TAreaGraph). Made once and kept until the map changes.
class MapAtlas
{
public:
    // How much of each way of drawing the map a view shows, from 0 to 1. The rooms show as long as the
    // footprints are not fully in; the footprints give way to the world graph as the areas shrink further
    struct Weights
    {
        float footprint = 0.0f;
        float world = 0.0f;
        bool roomsShown() const { return footprint < 1.0f; }
        // A click picks an area rather than a room
        bool areasPicked() const { return footprint >= 0.5f; }
        // An area is picked by its node rather than its footprint
        bool nodesPicked() const { return world >= 0.5f; }
    };

    struct Area
    {
        int id = 0;
        QString name;
        int roomCount = 0;
        // The color most of its rooms are drawn in
        QColor color;
        // Its place in the world, the middle of its rooms' bounding box there, and how much room it takes
        QPointF center;
        float radius = 0.0f;
        // Added to a room's coordinates to place it in the world
        QPointF offset;
        // In the world, counterclockwise, around every room's cell on every level
        QPolygonF footprint;
        // Where a view flying into the area comes down: the room nearest its middle
        int landingRoomId = 0;
        QPoint landing;
        int landingLevel = 0;
    };

    // Pixels between neighboring rooms: above the first the rooms show alone, by the second they are footprints
    static constexpr float scmRoomsAlonePixels = 2.5f;
    static constexpr float scmFootprintsOnlyPixels = 1.25f;
    // Camera distances: up to the first the rooms show alone however small, so every view the mapper could
    // show before it could be pulled back any further looks as it did; by the second they are footprints
    static constexpr float scmRoomsAloneDistance = 100.0f;
    static constexpr float scmFootprintsOnlyDistance = 200.0f;
    // Pixels across the radius of a typical area: below the first the world graph comes in, by the second it is all there is
    static constexpr float scmWorldFromPixels = 8.0f;
    static constexpr float scmWorldOnlyPixels = 4.0f;

    // roomPixels is how many pixels apart neighboring rooms are; areaPixels how many a typical area's radius spans
    static Weights weightsFor(float roomPixels, float areaPixels, float distance);
    // The convex hull, counterclockwise, without repeated or collinear points
    static QPolygonF hull(QList<QPointF> points);

    // Remakes everything from the map, whose area graph must be refreshed first; environmentColor gives a
    // room color from its environment
    void build(const TMap& map, const std::function<QColor(int)>& environmentColor, const QColor& linkColor);
    void clear();

    const QList<Area>& areas() const { return mAreas; }
    const Area* area(int areaId) const;
    // Half the areas are smaller, in radius
    float typicalRadius() const { return mTypicalRadius; }
    // The area whose footprint, or with nodes whose node, is under this point of the world, or else the one
    // whose edge is nearest within slop, or 0
    int areaAt(const QPointF& point, bool nodes, float slop) const;

    // Three coordinates and four color components a corner, three corners a triangle, all at height 0
    const QVector<float>& footprintVertices() const { return mFootprintVertices; }
    const QVector<float>& footprintColors() const { return mFootprintColors; }
    // The links between areas then the areas' nodes
    const QVector<float>& worldVertices() const { return mWorldVertices; }
    const QVector<float>& worldColors() const { return mWorldColors; }
    // A ring around the area's node, as the one the view is in is marked
    void appendRing(int areaId, const QColor& color, QVector<float>& vertices, QVector<float>& colors) const;
    // A ring around the area's node from inner to outer times its radius
    void appendRing(int areaId, float inner, float outer, const QColor& color, QVector<float>& vertices, QVector<float>& colors) const;
    // How wide the world graph draws the link between two areas
    float linkWidth(int fromAreaId, int toAreaId) const;
    // A band from the rim of one area's node to the rim of the other's, as their link is drawn, or nothing if the nodes overlap
    void appendLink(int fromAreaId, int toAreaId, float width, const QColor& color, QVector<float>& vertices, QVector<float>& colors) const;
    // A band of this width along the area's footprint
    void appendOutline(int areaId, float width, const QColor& color, QVector<float>& vertices, QVector<float>& colors) const;
    // A band of this width from one point of the world to another, with a square end at each point
    static void appendBand(const QPointF& from, const QPointF& to, float width, const QColor& color, QVector<float>& vertices, QVector<float>& colors);

private:
    static float linkWidth(const Area& from, const Area& to);
    static void appendLink(const Area& from, const Area& to, float width, const QColor& color, QVector<float>& vertices, QVector<float>& colors);

    QList<Area> mAreas;
    QHash<int, qsizetype> mAreaIndex;
    float mTypicalRadius = 0.0f;
    QVector<float> mFootprintVertices;
    QVector<float> mFootprintColors;
    QVector<float> mWorldVertices;
    QVector<float> mWorldColors;
};

#endif // MUDLET_MAPATLAS_H
