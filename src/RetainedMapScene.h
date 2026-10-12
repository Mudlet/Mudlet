#ifndef MUDLET_RETAINEDMAPSCENE_H
#define MUDLET_RETAINEDMAPSCENE_H

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

#include "GeometryManager.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QMatrix4x4>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QVector3D>
#include <QVector4D>
#include <array>
#include <compare>
#include <memory>
#include <optional>
#include <utility>

// An area's rooms and exits as the modern 3D view draws them, kept from frame to frame. Everything is filed
// in square tiles of the map, by level, so that a frame draws only the tiles in view and leaves the detail
// off those too far away to show it; what a frame draws of a layer is a few runs of one GPU buffer.
// It is built afresh, but uploads only what changed, when anything it was made from changes.
class RetainedMapScene
{
public:
    enum class Layer { Borders, Outlines, Rooms, SeeThroughCubes, ExitBars, SeeThroughExitBars };
    static constexpr int scmLayerCount = 6;
    // In map units
    static constexpr int scmTileSize = 16;

    struct Room
    {
        int id = 0;
        // Its center, on its level
        QVector3D center;
        // How far, flat on the map, what is drawn on it reaches from its center
        float decalReach = 0.0f;
        // Its name's length and how far the name is moved from its usual place, for how far the name reaches
        int nameLength = 0;
        float nameOffset = 0.0f;
        // Shown in a tile too far away for detail
        bool highlighted = false;
    };

    // What one frame sees
    struct View
    {
        // Map to clip coordinates
        QMatrix4x4 projection;
        // Map to eye coordinates, in which the camera looks down -z
        QMatrix4x4 eye;
        QSizeF viewport;
        // How many map units a pixel spans, for each unit of eye depth
        float unitsPerPixelPerDepth = 0.0f;
        float roomSize = 0.0f;
        // A tile whose nearest room is narrower than this many pixels is drawn without detail
        float detailPixels = 0.0f;
    };

    struct DecalRoom
    {
        int id = 0;
        bool detailed = true;
    };

    RetainedMapScene();

    void clear();
    void addRoom(const Room& room);
    void addInstance(Layer layer, int roomId, int level, const QPointF& anchor, const CubeInstanceData& instance);
    void addTriangle(bool seeThrough, int level, const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector4D& tint);
    // Files what was added in its tiles and marks what differs from before for upload
    void finish();

    void cull(const View& view);
    // The runs of the layer's instances in the tiles in view, on that level if given, less those of the
    // excluded rooms, which the frame draws itself
    QList<InstanceRun> runs(Layer layer, std::optional<int> level, const QSet<int>& excludedRooms) const;
    // The rooms in tiles near enough to the view for what is drawn on them to show, in the order they were
    // added: those of tiles without detail only if highlighted or selected, and always those given
    QList<DecalRoom> decalRooms(qreal marginPixels, float nameReachPerCharacter, float nameReach, const QSet<int>& selection, const QSet<int>& always) const;
    // The triangles of the tiles in view with detail, on that level if given
    void appendTriangles(bool seeThrough, std::optional<int> level, QVector<float>& vertices, QVector<float>& colors) const;

    std::shared_ptr<RetainedInstances> instances(Layer layer) const { return mInstances.at(static_cast<int>(layer)); }
    const QMap<int, QRectF>& levelExtents() const { return mLevelExtents; }
    // Levels with see-through exit bars or triangles, lowest first
    const QList<int>& seeThroughExitLevels() const { return mSeeThroughExitLevels; }
    bool hasExits() const;
    bool contains(int roomId) const { return mRoomIndex.contains(roomId); }
    int tileCount() const { return static_cast<int>(mTiles.size()); }
    int tilesDrawn() const;
    // With the GL context current
    void releaseBuffers();

private:
    struct TileKey
    {
        int level = 0;
        int row = 0;
        int column = 0;
        auto operator<=>(const TileKey&) const = default;
        friend size_t qHash(const TileKey& key, const size_t seed = 0) { return qHashMulti(seed, key.level, key.row, key.column); }
    };
    struct Tile
    {
        int level = 0;
        // Around everything drawn in it
        QVector3D low;
        QVector3D high;
        // Around its rooms' decals
        QVector3D decalLow;
        QVector3D decalHigh;
        int longestName = 0;
        float furthestNameOffset = 0.0f;
        std::array<qsizetype, scmLayerCount> first{};
        std::array<qsizetype, scmLayerCount> end{};
        qsizetype firstRoom = 0;
        qsizetype endRoom = 0;
        std::array<qsizetype, 2> firstTriangle{};
        std::array<qsizetype, 2> endTriangle{};
        bool drawn = true;
        bool detailed = true;
    };
    // Pending items name their tile by the order its key was first seen in
    struct PendingInstance
    {
        int tile = 0;
        int roomId = 0;
        CubeInstanceData instance;
    };
    struct PendingTriangle
    {
        int tile = 0;
        bool seeThrough = false;
        std::array<QVector3D, 3> corners;
        QVector4D tint;
    };

    int pendingTile(int level, const QPointF& anchor);
    // See-through layers blend in the order they are drawn, so they are filed by level alone, in the order they were added
    static bool keepsOrder(Layer layer) { return layer == Layer::SeeThroughCubes || layer == Layer::SeeThroughExitBars; }
    // Where the items of the level, or of all levels, are among items filed by level
    std::pair<qsizetype, qsizetype> levelSpan(const QVector<int>& tiles, std::optional<int> level) const;

    std::array<QList<PendingInstance>, scmLayerCount> mPendingInstances;
    QList<PendingTriangle> mPendingTriangles;
    QList<std::pair<int, Room>> mPendingRooms;
    QList<TileKey> mPendingTileKeys;
    QHash<TileKey, int> mPendingTiles;

    QList<Tile> mTiles;
    std::array<std::shared_ptr<RetainedInstances>, scmLayerCount> mInstances;
    // Which room each instance belongs to, 0 for exits
    std::array<QVector<int>, scmLayerCount> mInstanceRooms;
    // Which tile each instance is in
    std::array<QVector<int>, scmLayerCount> mInstanceTiles;
    // Laid out by tile, each with the order it was added in
    QList<Room> mRooms;
    QVector<qsizetype> mRoomOrder;
    QHash<int, qsizetype> mRoomIndex;
    QHash<int, int> mRoomTile;
    // Opaque, then see-through: three corners of three coordinates and three of four color components each
    std::array<QVector<float>, 2> mTriangleVertices;
    std::array<QVector<float>, 2> mTriangleColors;
    std::array<QVector<int>, 2> mTriangleTiles;
    QMap<int, QRectF> mLevelExtents;
    QList<int> mSeeThroughExitLevels;
    View mView;
};

#endif // MUDLET_RETAINEDMAPSCENE_H
