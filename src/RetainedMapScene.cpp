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

#include "RetainedMapScene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {
constexpr float scmFar = std::numeric_limits<float>::max();

void grow(QVector3D& low, QVector3D& high, const QVector3D& lowCorner, const QVector3D& highCorner)
{
    low = QVector3D(qMin(low.x(), lowCorner.x()), qMin(low.y(), lowCorner.y()), qMin(low.z(), lowCorner.z()));
    high = QVector3D(qMax(high.x(), highCorner.x()), qMax(high.y(), highCorner.y()), qMax(high.z(), highCorner.z()));
}

// Whether any of the box is on the inner side of every plane, each plane being a, b, c, d of ax + by + cz + d >= 0
bool boxInside(const QList<QVector4D>& planes, const QVector3D& low, const QVector3D& high)
{
    for (const QVector4D& plane : planes) {
        const QVector3D furthest(plane.x() > 0.0f ? high.x() : low.x(), plane.y() > 0.0f ? high.y() : low.y(), plane.z() > 0.0f ? high.z() : low.z());
        if (QVector3D::dotProduct(plane.toVector3D(), furthest) + plane.w() < 0.0f) {
            return false;
        }
    }
    return true;
}

// The planes bounding what a projection shows, as left, right, bottom, top, near, far; the sides moved out by these
// parts of the view's half width and height
QList<QVector4D> frustumPlanes(const QMatrix4x4& projection, const float widen = 0.0f, const float heighten = 0.0f)
{
    const QVector4D x = projection.row(0);
    const QVector4D y = projection.row(1);
    const QVector4D z = projection.row(2);
    const QVector4D w = projection.row(3);
    return {w * (1.0f + widen) + x, w * (1.0f + widen) - x, w * (1.0f + heighten) + y, w * (1.0f + heighten) - y, w + z, w - z};
}

void appendSpan(QVector<float>& to, const QVector<float>& from, const qsizetype first, const qsizetype count)
{
    const qsizetype at = to.size();
    to.resize(at + count);
    std::copy_n(from.constData() + first, count, to.data() + at);
}

// Files items by tile, keeping their order within each; returns where each tile's items start, and after the last
QVector<qsizetype> fileByTile(const QVector<int>& tiles, const int tileCount, QVector<qsizetype>& order)
{
    QVector<qsizetype> starts(tileCount + 1, 0);
    for (const int tile : tiles) {
        ++starts[tile + 1];
    }
    for (int tile = 0; tile < tileCount; ++tile) {
        starts[tile + 1] += starts[tile];
    }
    QVector<qsizetype> next = starts;
    order.resize(tiles.size());
    for (qsizetype index = 0; index < tiles.size(); ++index) {
        order[next[tiles.at(index)]++] = index;
    }
    return starts;
}
} // namespace

RetainedMapScene::RetainedMapScene()
{
    for (auto& layer : mInstances) {
        layer = std::make_shared<RetainedInstances>();
    }
}

int RetainedMapScene::pendingTile(const int level, const QPointF& anchor)
{
    const TileKey key{level, static_cast<int>(std::floor(anchor.y() / scmTileSize)), static_cast<int>(std::floor(anchor.x() / scmTileSize))};
    // Not tryEmplace(), which needs Qt 6.9
    if (const auto found = mPendingTiles.constFind(key); found != mPendingTiles.cend()) {
        return found.value();
    }
    const int tile = static_cast<int>(mPendingTileKeys.size());
    mPendingTiles.insert(key, tile);
    mPendingTileKeys.append(key);
    return tile;
}

void RetainedMapScene::clear()
{
    // Assigned rather than cleared, as clear() would keep what a huge area took for as long as the view is open
    for (auto& pending : mPendingInstances) {
        pending = {};
    }
    mPendingTriangles = {};
    mPendingRooms = {};
    mPendingTileKeys = {};
    mPendingTiles = {};
}

void RetainedMapScene::addRoom(const Room& room)
{
    mPendingRooms.append({pendingTile(qRound(room.center.z()), room.center.toPointF()), room});
}

void RetainedMapScene::addInstance(const Layer layer, const int roomId, const int level, const QPointF& anchor, const CubeInstanceData& instance)
{
    mPendingInstances[static_cast<int>(layer)].append({pendingTile(level, anchor), roomId, instance});
}

void RetainedMapScene::addTriangle(const bool seeThrough, const int level, const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector4D& tint, const bool distant)
{
    const QVector3D middle = (a + b + c) / 3.0f;
    mPendingTriangles.append({pendingTile(level, middle.toPointF()), seeThrough, distant, {a, b, c}, tint});
}

void RetainedMapScene::finish()
{
    const int tileCount = static_cast<int>(mPendingTileKeys.size());
    QVector<int> sorted(tileCount);
    std::iota(sorted.begin(), sorted.end(), 0);
    std::sort(sorted.begin(), sorted.end(), [this](const int a, const int b) {
        return mPendingTileKeys.at(a) < mPendingTileKeys.at(b);
    });
    // Tiles in key order, so by level first
    QVector<int> rank(tileCount);
    for (int index = 0; index < tileCount; ++index) {
        rank[sorted.at(index)] = index;
    }
    const auto tileOf = [&rank](const int pending) {
        return rank.at(pending);
    };

    mTiles = QList<Tile>(tileCount);
    for (int index = 0; index < tileCount; ++index) {
        Tile& tile = mTiles[index];
        tile.level = mPendingTileKeys.at(sorted.at(index)).level;
        tile.low = tile.decalLow = QVector3D(scmFar, scmFar, scmFar);
        tile.high = tile.decalHigh = QVector3D(-scmFar, -scmFar, -scmFar);
    }

    // Tiles are in level order, so filing by this files by level
    QVector<int> levelRanks(tileCount);
    int levelCount = 0;
    for (int index = 0; index < tileCount; ++index) {
        if (index && mTiles.at(index).level != mTiles.at(index - 1).level) {
            ++levelCount;
        }
        levelRanks[index] = levelCount;
    }
    if (tileCount) {
        ++levelCount;
    }
    QVector<int> tiles;
    QVector<int> ranks;
    QVector<qsizetype> order;
    const auto file = [&](const bool byLevel) {
        if (!byLevel) {
            return fileByTile(tiles, tileCount, order);
        }
        ranks.resize(tiles.size());
        for (qsizetype index = 0; index < tiles.size(); ++index) {
            ranks[index] = levelRanks.at(tiles.at(index));
        }
        return fileByTile(ranks, levelCount, order);
    };
    for (int layer = 0; layer < scmLayerCount; ++layer) {
        const QList<PendingInstance>& pending = mPendingInstances.at(layer);
        tiles.resize(pending.size());
        for (qsizetype index = 0; index < pending.size(); ++index) {
            tiles[index] = tileOf(pending.at(index).tile);
        }
        const bool byLevel = keepsOrder(static_cast<Layer>(layer));
        const QVector<qsizetype> starts = file(byLevel);
        QVector<CubeInstanceData> instances;
        instances.reserve(pending.size());
        QVector<int>& rooms = mInstanceRooms[layer];
        rooms.clear();
        rooms.reserve(pending.size());
        QVector<int>& instanceTiles = mInstanceTiles[layer];
        instanceTiles.clear();
        instanceTiles.reserve(pending.size());
        for (const qsizetype index : std::as_const(order)) {
            const PendingInstance& instance = pending.at(index);
            instances.append(instance.instance);
            rooms.append(instance.roomId);
            instanceTiles.append(tiles.at(index));
            // The template spans -1 to 1 each way, so its corners reach as far from the center as each row's sizes add up to
            const QMatrix4x4& transform = instance.instance.transform;
            const QVector3D center = transform.column(3).toVector3D();
            QVector3D reach;
            for (int row = 0; row < 3; ++row) {
                reach[row] = std::abs(transform(row, 0)) + std::abs(transform(row, 1)) + std::abs(transform(row, 2));
            }
            Tile& tile = mTiles[tiles.at(index)];
            grow(tile.low, tile.high, center - reach, center + reach);
        }
        for (int index = 0; index < tileCount && !byLevel; ++index) {
            mTiles[index].first[layer] = starts.at(index);
            mTiles[index].end[layer] = starts.at(index + 1);
        }
        mInstances.at(layer)->setInstances(std::move(instances));
    }

    for (int part = 0; part < 2; ++part) {
        QVector<qsizetype> triangles;
        tiles.clear();
        for (qsizetype index = 0; index < mPendingTriangles.size(); ++index) {
            if (mPendingTriangles.at(index).seeThrough == (part == 1)) {
                triangles.append(index);
                tiles.append(tileOf(mPendingTriangles.at(index).tile));
            }
        }
        const bool byLevel = part == 1;
        const QVector<qsizetype> starts = file(byLevel);
        QVector<float>& vertices = mTriangleVertices[part];
        QVector<float>& colors = mTriangleColors[part];
        QVector<int>& triangleTiles = mTriangleTiles[part];
        QVector<bool>& distant = mTriangleDistant[part];
        vertices.clear();
        colors.clear();
        triangleTiles.clear();
        distant.clear();
        vertices.reserve(triangles.size() * 9);
        colors.reserve(triangles.size() * 12);
        triangleTiles.reserve(triangles.size());
        distant.reserve(triangles.size());
        for (const qsizetype index : std::as_const(order)) {
            const PendingTriangle& triangle = mPendingTriangles.at(triangles.at(index));
            triangleTiles.append(tiles.at(index));
            distant.append(triangle.distant);
            Tile& tile = mTiles[tiles.at(index)];
            for (const QVector3D& corner : triangle.corners) {
                vertices << corner.x() << corner.y() << corner.z();
                colors << triangle.tint.x() << triangle.tint.y() << triangle.tint.z() << triangle.tint.w();
                grow(tile.low, tile.high, corner, corner);
            }
        }
        for (int index = 0; index < tileCount && !byLevel; ++index) {
            mTiles[index].firstTriangle[part] = starts.at(index);
            mTiles[index].endTriangle[part] = starts.at(index + 1);
        }
    }

    tiles.resize(mPendingRooms.size());
    for (qsizetype index = 0; index < mPendingRooms.size(); ++index) {
        tiles[index] = tileOf(mPendingRooms.at(index).first);
    }
    const QVector<qsizetype> roomStarts = fileByTile(tiles, tileCount, order);
    mRooms.clear();
    mRooms.reserve(mPendingRooms.size());
    mRoomOrder.clear();
    mRoomOrder.reserve(mPendingRooms.size());
    mRoomIndex.clear();
    mRoomIndex.reserve(mPendingRooms.size());
    mRoomTile.clear();
    mRoomTile.reserve(mPendingRooms.size());
    mLevelExtents.clear();
    for (const qsizetype index : std::as_const(order)) {
        const Room& room = mPendingRooms.at(index).second;
        const int tileIndex = tiles.at(index);
        Tile& tile = mTiles[tileIndex];
        mRoomIndex.insert(room.id, mRooms.size());
        mRoomTile.insert(room.id, tileIndex);
        mRooms.append(room);
        mRoomOrder.append(index);
        const QVector3D reach(room.decalReach, room.decalReach, room.decalReach);
        grow(tile.decalLow, tile.decalHigh, room.center - reach, room.center + reach);
        tile.longestName = qMax(tile.longestName, room.nameLength);
        tile.furthestNameOffset = qMax(tile.furthestNameOffset, room.nameOffset);

        const QRectF cell(room.center.x() - 0.5f, room.center.y() - 0.5f, 1.0, 1.0);
        const int level = qRound(room.center.z());
        const auto extent = mLevelExtents.find(level);
        if (extent == mLevelExtents.end()) {
            mLevelExtents.insert(level, cell);
        } else {
            *extent = extent->united(cell);
        }
    }
    for (int index = 0; index < tileCount; ++index) {
        mTiles[index].firstRoom = roomStarts.at(index);
        mTiles[index].endRoom = roomStarts.at(index + 1);
    }

    mSeeThroughExitLevels.clear();
    for (const QVector<int>* seeThroughTiles :
         {&mInstanceTiles.at(static_cast<int>(Layer::SeeThroughExitBars)), &mInstanceTiles.at(static_cast<int>(Layer::SeeThroughExitStrokes)), &mTriangleTiles.at(1)}) {
        for (const int tile : *seeThroughTiles) {
            const int level = mTiles.at(tile).level;
            if (!mSeeThroughExitLevels.contains(level)) {
                mSeeThroughExitLevels.append(level);
            }
        }
    }
    std::sort(mSeeThroughExitLevels.begin(), mSeeThroughExitLevels.end());
    clear();
}

void RetainedMapScene::cull(const View& view)
{
    mView = view;
    const QList<QVector4D> planes = frustumPlanes(view.projection);
    for (Tile& tile : mTiles) {
        tile.drawn = tile.low.x() <= tile.high.x() && boxInside(planes, tile.low, tile.high);
        // Judged for tiles out of view too, as their rooms' text may reach into it. The nearest corner stands for
        // the nearest room; a tile reaching behind the camera is near enough
        float nearest = scmFar;
        for (int corner = 0; corner < 8; ++corner) {
            const QVector3D point(corner & 1 ? tile.high.x() : tile.low.x(), corner & 2 ? tile.high.y() : tile.low.y(), corner & 4 ? tile.high.z() : tile.low.z());
            nearest = qMin(nearest, -view.eye.map(point).z());
        }
        tile.detailed = nearest <= 0.0f || view.roomSize / (view.unitsPerPixelPerDepth * nearest) >= view.detailPixels;
    }
}

QList<InstanceRun> RetainedMapScene::runs(const Layer layer, const std::optional<int> level, const QSet<int>& excludedRooms) const
{
    const int index = static_cast<int>(layer);
    QList<InstanceRun> runs;
    if (keepsOrder(layer)) {
        const QVector<int>& tiles = mInstanceTiles.at(index);
        const QVector<int>& rooms = mInstanceRooms.at(index);
        const auto [first, end] = levelSpan(tiles, level);
        for (qsizetype position = first; position < end; ++position) {
            if (!mTiles.at(tiles.at(position)).drawn || (!excludedRooms.isEmpty() && excludedRooms.contains(rooms.at(position)))) {
                continue;
            }
            if (!runs.isEmpty() && runs.constLast().first + runs.constLast().count == position) {
                ++runs.last().count;
            } else {
                runs.append({position, 1});
            }
        }
        return runs;
    }
    // Level outlines are detail; the hovered room's, drawn separately, is not
    const bool detailOnly = layer == Layer::Outlines;
    for (const Tile& tile : mTiles) {
        if (!tile.drawn || (detailOnly && !tile.detailed) || (level && tile.level != *level)) {
            continue;
        }
        const qsizetype count = tile.end.at(index) - tile.first.at(index);
        if (!count) {
            continue;
        }
        if (!runs.isEmpty() && runs.constLast().first + runs.constLast().count == tile.first.at(index)) {
            runs.last().count += count;
        } else {
            runs.append({tile.first.at(index), count});
        }
    }
    if (excludedRooms.isEmpty() || runs.isEmpty()) {
        return runs;
    }

    QList<qsizetype> cuts;
    const QVector<int>& rooms = mInstanceRooms.at(index);
    for (const int roomId : excludedRooms) {
        const auto tileIndex = mRoomTile.constFind(roomId);
        if (tileIndex == mRoomTile.cend()) {
            continue;
        }
        const Tile& tile = mTiles.at(*tileIndex);
        for (qsizetype position = tile.first.at(index); position < tile.end.at(index); ++position) {
            if (rooms.at(position) == roomId) {
                cuts.append(position);
            }
        }
    }
    if (cuts.isEmpty()) {
        return runs;
    }
    std::sort(cuts.begin(), cuts.end());
    QList<InstanceRun> kept;
    auto cut = cuts.cbegin();
    for (const InstanceRun& run : std::as_const(runs)) {
        qsizetype from = run.first;
        const qsizetype to = run.first + run.count;
        while (cut != cuts.cend() && *cut < to) {
            if (*cut >= from) {
                if (*cut > from) {
                    kept.append({from, *cut - from});
                }
                from = *cut + 1;
            }
            ++cut;
        }
        if (to > from) {
            kept.append({from, to - from});
        }
    }
    return kept;
}

QList<RetainedMapScene::DecalRoom>
RetainedMapScene::decalRooms(const qreal marginPixels, const float nameReachPerCharacter, const float nameReach, const QSet<int>& selection, const QSet<int>& always) const
{
    const float widen = static_cast<float>(2.0 * marginPixels / qMax(1.0, mView.viewport.width()));
    const float heighten = static_cast<float>(2.0 * marginPixels / qMax(1.0, mView.viewport.height()));
    QList<QVector4D> planes = frustumPlanes(mView.projection, widen, heighten);
    // Decals are drawn whatever their depth, as text is placed only where the whole of it is in front of the camera
    planes.resize(4);

    QList<std::pair<qsizetype, DecalRoom>> found;
    for (const Tile& tile : mTiles) {
        if (tile.firstRoom == tile.endRoom) {
            continue;
        }
        const float names = tile.longestName > 0 ? tile.furthestNameOffset + nameReach + tile.longestName * nameReachPerCharacter / 2.0f : 0.0f;
        const QVector3D reach(names, names, 0.0f);
        if (!boxInside(planes, tile.decalLow - reach, tile.decalHigh + reach)) {
            continue;
        }
        for (qsizetype index = tile.firstRoom; index < tile.endRoom; ++index) {
            const Room& room = mRooms.at(index);
            if (tile.detailed || room.highlighted || selection.contains(room.id)) {
                found.append(std::make_pair(mRoomOrder.at(index), DecalRoom{room.id, tile.detailed}));
            }
        }
    }
    for (const int roomId : always) {
        const auto index = mRoomIndex.constFind(roomId);
        if (index == mRoomIndex.cend()) {
            continue;
        }
        const bool listed = std::any_of(found.cbegin(), found.cend(), [roomId](const auto& entry) {
            return entry.second.id == roomId;
        });
        if (!listed) {
            found.append(std::make_pair(mRoomOrder.at(*index), DecalRoom{roomId, mTiles.at(mRoomTile.value(roomId)).detailed}));
        }
    }
    std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    QList<DecalRoom> rooms;
    rooms.reserve(found.size());
    for (const auto& [order, room] : std::as_const(found)) {
        rooms.append(room);
    }
    return rooms;
}

void RetainedMapScene::appendTriangles(const bool seeThrough, const std::optional<int> level, QVector<float>& vertices, QVector<float>& colors) const
{
    const int part = seeThrough ? 1 : 0;
    const auto append = [&](const qsizetype first, const qsizetype count) {
        if (count) {
            appendSpan(vertices, mTriangleVertices.at(part), first * 9, count * 9);
            appendSpan(colors, mTriangleColors.at(part), first * 12, count * 12);
        }
    };
    const QVector<bool>& distant = mTriangleDistant.at(part);
    const auto appendDistant = [&](const qsizetype first, const qsizetype end) {
        qsizetype runStart = first;
        for (qsizetype index = first; index < end; ++index) {
            if (!distant.at(index)) {
                append(runStart, index - runStart);
                runStart = index + 1;
            }
        }
        append(runStart, end - runStart);
    };
    if (seeThrough) {
        const QVector<int>& tiles = mTriangleTiles.at(part);
        const auto [first, end] = levelSpan(tiles, level);
        qsizetype runStart = first;
        for (qsizetype index = first; index < end; ++index) {
            const Tile& tile = mTiles.at(tiles.at(index));
            if (!tile.drawn || (!tile.detailed && !distant.at(index))) {
                append(runStart, index - runStart);
                runStart = index + 1;
            }
        }
        append(runStart, end - runStart);
        return;
    }
    for (const Tile& tile : mTiles) {
        if (!tile.drawn || (level && tile.level != *level)) {
            continue;
        }
        if (tile.detailed) {
            append(tile.firstTriangle.at(part), tile.endTriangle.at(part) - tile.firstTriangle.at(part));
        } else {
            appendDistant(tile.firstTriangle.at(part), tile.endTriangle.at(part));
        }
    }
}

std::pair<qsizetype, qsizetype> RetainedMapScene::levelSpan(const QVector<int>& tiles, const std::optional<int> level) const
{
    if (!level) {
        return {0, tiles.size()};
    }
    const auto first = std::lower_bound(tiles.cbegin(), tiles.cend(), *level, [this](const int tile, const int wanted) {
        return mTiles.at(tile).level < wanted;
    });
    const auto end = std::upper_bound(first, tiles.cend(), *level, [this](const int wanted, const int tile) {
        return wanted < mTiles.at(tile).level;
    });
    return {first - tiles.cbegin(), end - tiles.cbegin()};
}

bool RetainedMapScene::hasExits() const
{
    for (const Layer layer : {Layer::ExitBars, Layer::SeeThroughExitBars, Layer::ExitStrokes, Layer::SeeThroughExitStrokes}) {
        if (!mInstances.at(static_cast<int>(layer))->instances.isEmpty()) {
            return true;
        }
    }
    return !mTriangleVertices.at(0).isEmpty() || !mTriangleVertices.at(1).isEmpty();
}

int RetainedMapScene::tilesDrawn() const
{
    return static_cast<int>(std::count_if(mTiles.cbegin(), mTiles.cend(), [](const Tile& tile) {
        return tile.drawn;
    }));
}

void RetainedMapScene::releaseBuffers()
{
    for (const auto& layer : mInstances) {
        if (layer->buffer.isCreated()) {
            layer->buffer.destroy();
        }
        layer->resized = true;
    }
}
