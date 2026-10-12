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

#include "TAreaGraph.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <unordered_map>
#include <vector>

namespace {
// Room units between the bounding circles of two areas.
constexpr double cGap = 4.0;
constexpr double cMinRadius = 2.0;
constexpr int cIterations = 300;
constexpr int cOverlapPasses = 50;
constexpr quint64 cSeed = 0x6d75646c65744147ULL;

// splitmix64, rather than a <random> distribution, whose output differs between standard libraries.
double unitHash(quint64 value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return static_cast<double>(value >> 11) / static_cast<double>(1ULL << 53);
}

double unitHash(int a, int b = 0)
{
    return unitHash(cSeed ^ (static_cast<quint64>(static_cast<quint32>(a)) << 32) ^ static_cast<quint32>(b));
}

QPointF hashedDirection(int a, int b = 0)
{
    const double angle = unitHash(a, b) * 2.0 * std::numbers::pi;
    return {std::cos(angle), std::sin(angle)};
}

// Every pair i < j whose points share a cell of the given size or sit in neighbouring cells.
void collectNearbyPairs(const std::vector<double>& x, const std::vector<double>& y, const double cellSize, std::vector<std::pair<int, int>>& pairs)
{
    pairs.clear();
    const int count = static_cast<int>(x.size());
    const auto cellOf = [cellSize](double v) {
        return static_cast<qint64>(std::floor(v / cellSize));
    };
    const auto cellKey = [](qint64 cx, qint64 cy) {
        return (cx << 32) ^ (cy & 0xffffffffLL);
    };
    std::unordered_map<qint64, std::vector<int>> grid;
    for (int i = 0; i < count; ++i) {
        grid[cellKey(cellOf(x[i]), cellOf(y[i]))].push_back(i);
    }
    for (int i = 0; i < count; ++i) {
        const qint64 cx = cellOf(x[i]);
        const qint64 cy = cellOf(y[i]);
        for (qint64 dx = -1; dx <= 1; ++dx) {
            for (qint64 dy = -1; dy <= 1; ++dy) {
                const auto it = grid.find(cellKey(cx + dx, cy + dy));
                if (it == grid.end()) {
                    continue;
                }
                for (const int j : it->second) {
                    if (j > i) {
                        pairs.emplace_back(i, j);
                    }
                }
            }
        }
    }
}

struct Bounds
{
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    void add(const QPointF& centre, double radius)
    {
        minX = std::min(minX, centre.x() - radius);
        maxX = std::max(maxX, centre.x() + radius);
        minY = std::min(minY, centre.y() - radius);
        maxY = std::max(maxY, centre.y() + radius);
    }
    bool empty() const { return minX > maxX; }
    double width() const { return maxX - minX; }
    double height() const { return maxY - minY; }
};
} // namespace

TAreaGraph::TAreaGraph(TMap* pMap)
: mpMap(pMap)
{
}

void TAreaGraph::refresh()
{
    const unsigned int generation = mpMap->mpRoomDB->mapGeneration();
    const quint64 revision = mpMap->mapRevision();
    const bool structureChanged = !mBuilt || generation != mSeenGeneration || revision != mSeenRevision;
    const bool layoutChanged = !mStoredLayoutRead || mpMap->mUserData.value(cLayoutUserDataKey) != mStoredLayoutText;
    if (!structureChanged && !layoutChanged) {
        return;
    }

    if (layoutChanged) {
        readStoredLayout();
    }
    if (structureChanged) {
        rebuildGraph();
    }
    layoutUnplacedAreas();
    writeStoredLayout();
    mSeenGeneration = generation;
    mSeenRevision = revision;
    mBuilt = true;
}

void TAreaGraph::rebuildGraph()
{
    mNodes.clear();
    mLinks.clear();

    const QMap<int, QString>& names = mpMap->mpRoomDB->getAreaNamesMap();
    const QMap<int, TArea*>& areas = mpMap->mpRoomDB->getAreaMap();
    for (auto it = areas.cbegin(); it != areas.cend(); ++it) {
        // The default area exists in every map; only show it once something is in it.
        if (it.key() == -1 && it.value()->getAreaRooms().isEmpty()) {
            continue;
        }
        Node node;
        node.areaId = it.key();
        node.name = names.value(it.key());
        mNodes.insert(it.key(), node);
    }

    const QHash<int, TRoom*>& rooms = mpMap->mpRoomDB->getRoomMap();
    QMap<QPair<int, int>, Link> links;
    for (TRoom* pR : rooms) {
        const int areaId = pR->getArea();
        auto itNode = mNodes.find(areaId);
        if (itNode == mNodes.end()) {
            continue;
        }
        Node& node = itNode.value();
        if (node.roomCount == 0) {
            node.minX = node.maxX = pR->x();
            node.minY = node.maxY = pR->y();
            node.minZ = node.maxZ = pR->z();
        } else {
            node.minX = std::min(node.minX, pR->x());
            node.maxX = std::max(node.maxX, pR->x());
            node.minY = std::min(node.minY, pR->y());
            node.maxY = std::max(node.maxY, pR->y());
            node.minZ = std::min(node.minZ, pR->z());
            node.maxZ = std::max(node.maxZ, pR->z());
        }
        ++node.roomCount;

        const int fromId = pR->getId();
        const auto addExit = [&](const int toId) {
            if (toId <= 0) {
                return;
            }
            const TRoom* pTo = rooms.value(toId);
            if (!pTo) {
                return;
            }
            const int toArea = pTo->getArea();
            if (toArea == areaId || !mNodes.contains(toArea)) {
                return;
            }
            Link& link = links[qMakePair(std::min(areaId, toArea), std::max(areaId, toArea))];
            link.fromArea = std::min(areaId, toArea);
            link.toArea = std::max(areaId, toArea);
            ++link.exitCount;
            link.roomPairs.append(qMakePair(fromId, toId));
        };
        for (const int toId : {pR->getNorth(),
                               pR->getNortheast(),
                               pR->getEast(),
                               pR->getSoutheast(),
                               pR->getSouth(),
                               pR->getSouthwest(),
                               pR->getWest(),
                               pR->getNorthwest(),
                               pR->getUp(),
                               pR->getDown(),
                               pR->getIn(),
                               pR->getOut()}) {
            addExit(toId);
        }
        for (const int toId : pR->getSpecialExits()) {
            addExit(toId);
        }
    }

    mLinks.reserve(links.size());
    for (Link& link : links) {
        std::sort(link.roomPairs.begin(), link.roomPairs.end());
        mLinks.append(link);
    }
}

void TAreaGraph::readStoredLayout()
{
    mStored.clear();
    mStoredLayoutText = mpMap->mUserData.value(cLayoutUserDataKey);
    mStoredLayoutRead = true;
    if (mStoredLayoutText.isEmpty()) {
        return;
    }

    const QJsonObject areas = QJsonDocument::fromJson(mStoredLayoutText.toUtf8()).object().value(qsl("areas")).toObject();
    for (auto it = areas.constBegin(); it != areas.constEnd(); ++it) {
        bool isNumber = false;
        const int areaId = it.key().toInt(&isNumber);
        const QJsonObject entry = it.value().toObject();
        const double x = entry.value(qsl("x")).toDouble(qQNaN());
        const double y = entry.value(qsl("y")).toDouble(qQNaN());
        if (!isNumber || !(std::abs(x) <= cMaxCoordinate) || !(std::abs(y) <= cMaxCoordinate)) {
            continue;
        }
        mStored.insert(areaId, {QPointF(x, y), entry.value(qsl("pinned")).toBool()});
    }
}

void TAreaGraph::writeStoredLayout()
{
    QJsonObject areas;
    for (Node& node : mNodes) {
        const StoredPosition stored = mStored.value(node.areaId);
        node.position = stored.position;
        node.pinned = stored.pinned;
        QJsonObject entry{{qsl("x"), stored.position.x()}, {qsl("y"), stored.position.y()}};
        if (stored.pinned) {
            entry.insert(qsl("pinned"), true);
        }
        areas.insert(QString::number(node.areaId), entry);
    }

    // Areas deleted since are dropped, so a recycled id is laid out afresh.
    for (auto it = mStored.begin(); it != mStored.end();) {
        it = mNodes.contains(it.key()) ? std::next(it) : mStored.erase(it);
    }

    if (mNodes.isEmpty()) {
        mpMap->mUserData.remove(cLayoutUserDataKey);
        mStoredLayoutText.clear();
        return;
    }
    const QJsonObject root{{qsl("version"), 1}, {qsl("areas"), areas}};
    mStoredLayoutText = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    mpMap->mUserData.insert(cLayoutUserDataKey, mStoredLayoutText);
}

double TAreaGraph::radiusOf(int areaId) const
{
    const Node node = mNodes.value(areaId);
    if (node.roomCount == 0) {
        return cMinRadius;
    }
    return std::max(cMinRadius, 0.5 * std::hypot(node.maxX - node.minX + 1, node.maxY - node.minY + 1));
}

void TAreaGraph::layoutUnplacedAreas()
{
    const bool anyUnplaced = std::any_of(mNodes.keyBegin(), mNodes.keyEnd(), [this](int id) {
        return !mStored.contains(id);
    });
    if (!anyUnplaced) {
        return;
    }

    QHash<int, QList<int>> adjacency;
    for (const Link& link : std::as_const(mLinks)) {
        adjacency[link.fromArea].append(link.toArea);
        adjacency[link.toArea].append(link.fromArea);
    }

    QHash<int, bool> fixed;
    for (auto it = mStored.cbegin(); it != mStored.cend(); ++it) {
        fixed.insert(it.key(), true);
    }

    // Connected components in ascending order of their lowest area id, each in BFS order.
    QList<QList<int>> components;
    QSet<int> visited;
    for (const int start : mNodes.keys()) {
        if (visited.contains(start)) {
            continue;
        }
        QList<int> component{start};
        visited.insert(start);
        for (qsizetype i = 0; i < component.size(); ++i) {
            QList<int> neighbours = adjacency.value(component.at(i));
            std::sort(neighbours.begin(), neighbours.end());
            for (const int neighbour : neighbours) {
                if (!visited.contains(neighbour)) {
                    visited.insert(neighbour);
                    component.append(neighbour);
                }
            }
        }
        components.append(component);
    }

    QList<QList<int>> freshComponents;
    for (const QList<int>& component : std::as_const(components)) {
        const bool hasFixed = std::any_of(component.cbegin(), component.cend(), [&fixed](int id) {
            return fixed.contains(id);
        });
        const bool hasUnplaced = std::any_of(component.cbegin(), component.cend(), [this](int id) {
            return !mStored.contains(id);
        });
        if (!hasUnplaced) {
            continue;
        }
        if (hasFixed) {
            // Grow outward from what is already placed, so the existing world does not move.
            QList<int> pending;
            for (const int id : component) {
                if (!mStored.contains(id)) {
                    pending.append(id);
                }
            }
            while (!pending.isEmpty()) {
                QList<int> deferred;
                for (const int id : std::as_const(pending)) {
                    QPointF sum;
                    int placedNeighbours = 0;
                    double neighbourRadius = 0.0;
                    for (const int neighbour : adjacency.value(id)) {
                        if (mStored.contains(neighbour)) {
                            sum += mStored.value(neighbour).position;
                            neighbourRadius = std::max(neighbourRadius, radiusOf(neighbour));
                            ++placedNeighbours;
                        }
                    }
                    if (!placedNeighbours) {
                        deferred.append(id);
                        continue;
                    }
                    const QPointF offset = hashedDirection(id) * (radiusOf(id) + neighbourRadius + cGap);
                    mStored.insert(id, {sum / placedNeighbours + offset, false});
                }
                // A component is connected, so each round places at least one area.
                pending = deferred;
            }
            simulate(component, fixed, adjacency);
        } else {
            double spread = 0.0;
            for (const int id : component) {
                spread += std::pow(2.0 * radiusOf(id) + cGap, 2);
            }
            spread = std::sqrt(spread);
            for (const int id : component) {
                const double distance = std::sqrt(unitHash(id, 1)) * spread * 0.5;
                mStored.insert(id, {hashedDirection(id) * distance, false});
            }
            simulate(component, fixed, adjacency);
            freshComponents.append(component);
        }
    }

    if (!freshComponents.isEmpty()) {
        packComponents(freshComponents);
    }
    // Areas grown from placed ones, and the packed rows, can land on areas of other components.
    removeOverlaps(mNodes.keys(), fixed);
}

void TAreaGraph::packComponents(QList<QList<int>> freshComponents)
{
    QSet<int> fresh;
    for (const QList<int>& component : std::as_const(freshComponents)) {
        for (const int id : component) {
            fresh.insert(id);
        }
    }
    bool anyPlaced = false;
    Bounds placedBounds;
    for (const int id : mNodes.keys()) {
        if (!fresh.contains(id) && mStored.contains(id)) {
            anyPlaced = true;
            placedBounds.add(mStored.value(id).position, radiusOf(id));
        }
    }

    // Shelf-pack the new components in rows, below whatever was already placed.
    std::stable_sort(freshComponents.begin(), freshComponents.end(), [](const QList<int>& a, const QList<int>& b) {
        return a.size() > b.size();
    });
    QList<Bounds> bounds;
    double totalArea = 0.0;
    double widest = 0.0;
    for (const QList<int>& component : std::as_const(freshComponents)) {
        Bounds b;
        for (const int id : component) {
            b.add(mStored.value(id).position, radiusOf(id));
        }
        bounds.append(b);
        totalArea += (b.width() + cGap) * (b.height() + cGap);
        widest = std::max(widest, b.width());
    }
    const double rowLimit = std::max({widest, std::sqrt(totalArea) * 1.5, anyPlaced ? placedBounds.width() : 0.0});
    const double startX = anyPlaced ? placedBounds.minX : 0.0;
    double cursorX = startX;
    double cursorY = anyPlaced ? placedBounds.minY - 2.0 * cGap : 0.0;
    double rowHeight = 0.0;
    for (qsizetype i = 0; i < freshComponents.size(); ++i) {
        const Bounds& b = bounds.at(i);
        if (cursorX > startX && cursorX - startX + b.width() > rowLimit) {
            cursorX = startX;
            cursorY -= rowHeight + cGap;
            rowHeight = 0.0;
        }
        const QPointF shift(cursorX - b.minX, cursorY - b.maxY);
        for (const int id : freshComponents.at(i)) {
            mStored[id].position += shift;
        }
        cursorX += b.width() + cGap;
        rowHeight = std::max(rowHeight, b.height());
    }
}

// Fruchterman-Reingold in its grid form - areas repel only within twice their ideal distance, which
// keeps a step near linear in the number of areas - with each pair's ideal distance set by the two
// areas' sizes. Only areas not in fixed move.
void TAreaGraph::simulate(const QList<int>& component, const QHash<int, bool>& fixed, const QHash<int, QList<int>>& adjacency)
{
    const int count = static_cast<int>(component.size());
    std::vector<double> x(count);
    std::vector<double> y(count);
    std::vector<double> radius(count);
    std::vector<char> movable(count);
    QHash<int, int> indexOf;
    bool anyMovable = false;
    double largestRadius = 0.0;
    for (int i = 0; i < count; ++i) {
        const int id = component.at(i);
        indexOf.insert(id, i);
        const QPointF position = mStored.value(id).position;
        x[i] = position.x();
        y[i] = position.y();
        radius[i] = radiusOf(id);
        largestRadius = std::max(largestRadius, radius[i]);
        movable[i] = !fixed.contains(id);
        anyMovable = anyMovable || movable[i];
    }
    if (!anyMovable || count < 2) {
        return;
    }

    std::vector<std::pair<int, int>> edges;
    for (int i = 0; i < count; ++i) {
        for (const int neighbour : adjacency.value(component.at(i))) {
            const int j = indexOf.value(neighbour, -1);
            if (j > i) {
                edges.emplace_back(i, j);
            }
        }
    }

    double spread = 0.0;
    for (const double r : radius) {
        spread += std::pow(2.0 * r + cGap, 2);
    }
    const double startTemperature = std::max(cGap, std::sqrt(spread) * 0.25);
    const double cellSize = 2.0 * (2.0 * largestRadius + cGap);

    std::vector<double> moveX(count);
    std::vector<double> moveY(count);
    std::vector<std::pair<int, int>> pairs;
    for (int iteration = 0; iteration < cIterations; ++iteration) {
        std::fill(moveX.begin(), moveX.end(), 0.0);
        std::fill(moveY.begin(), moveY.end(), 0.0);
        collectNearbyPairs(x, y, cellSize, pairs);
        for (const auto& [i, j] : pairs) {
            double dx = x[j] - x[i];
            double dy = y[j] - y[i];
            double distance = std::hypot(dx, dy);
            if (distance < 1e-3) {
                const QPointF direction = hashedDirection(component.at(i), component.at(j));
                dx = direction.x() * 1e-3;
                dy = direction.y() * 1e-3;
                distance = 1e-3;
            }
            const double ideal = radius[i] + radius[j] + cGap;
            if (distance >= 2.0 * ideal) {
                continue;
            }
            const double force = ideal * ideal / (distance * distance);
            moveX[i] -= dx * force;
            moveY[i] -= dy * force;
            moveX[j] += dx * force;
            moveY[j] += dy * force;
        }
        for (const auto& [i, j] : edges) {
            const double dx = x[j] - x[i];
            const double dy = y[j] - y[i];
            const double distance = std::hypot(dx, dy);
            const double force = distance / (radius[i] + radius[j] + cGap);
            moveX[i] += dx * force;
            moveY[i] += dy * force;
            moveX[j] -= dx * force;
            moveY[j] -= dy * force;
        }

        const double temperature = startTemperature * (1.0 - static_cast<double>(iteration) / cIterations) + 0.1;
        for (int i = 0; i < count; ++i) {
            const double magnitude = std::hypot(moveX[i], moveY[i]);
            if (movable[i] && magnitude > 0.0) {
                const double step = std::min(magnitude, temperature) / magnitude;
                x[i] += moveX[i] * step;
                y[i] += moveY[i] * step;
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        if (movable[i]) {
            mStored[component.at(i)].position = QPointF(x[i], y[i]);
        }
    }
    removeOverlaps(component, fixed);
}

void TAreaGraph::removeOverlaps(const QList<int>& ids, const QHash<int, bool>& fixed)
{
    const int count = static_cast<int>(ids.size());
    std::vector<double> x(count);
    std::vector<double> y(count);
    std::vector<double> radius(count);
    std::vector<char> movable(count);
    double largestRadius = 0.0;
    for (int i = 0; i < count; ++i) {
        const QPointF position = mStored.value(ids.at(i)).position;
        x[i] = position.x();
        y[i] = position.y();
        radius[i] = radiusOf(ids.at(i));
        largestRadius = std::max(largestRadius, radius[i]);
        movable[i] = !fixed.contains(ids.at(i));
    }
    const double cellSize = 2.0 * (2.0 * largestRadius + cGap);

    std::vector<std::pair<int, int>> pairs;
    for (int pass = 0; pass < cOverlapPasses; ++pass) {
        bool moved = false;
        collectNearbyPairs(x, y, cellSize, pairs);
        for (const auto& [i, j] : pairs) {
            if (!movable[i] && !movable[j]) {
                continue;
            }
            double dx = x[j] - x[i];
            double dy = y[j] - y[i];
            const double distance = std::hypot(dx, dy);
            const double wanted = radius[i] + radius[j] + cGap * 0.5;
            if (distance >= wanted) {
                continue;
            }
            if (distance < 1e-3) {
                const QPointF direction = hashedDirection(ids.at(i), ids.at(j));
                dx = direction.x();
                dy = direction.y();
            } else {
                dx /= distance;
                dy /= distance;
            }
            const double overlap = wanted - distance;
            const double shareI = movable[i] ? (movable[j] ? 0.5 : 1.0) : 0.0;
            const double shareJ = 1.0 - shareI;
            x[i] -= dx * overlap * shareI;
            y[i] -= dy * overlap * shareI;
            x[j] += dx * overlap * shareJ;
            y[j] += dy * overlap * shareJ;
            moved = true;
        }
        if (!moved) {
            break;
        }
    }

    for (int i = 0; i < count; ++i) {
        if (movable[i]) {
            mStored[ids.at(i)].position = QPointF(x[i], y[i]);
        }
    }
}

bool TAreaGraph::setPosition(int areaId, const QPointF& position)
{
    refresh();
    if (!mNodes.contains(areaId) || !(std::abs(position.x()) <= cMaxCoordinate) || !(std::abs(position.y()) <= cMaxCoordinate)) {
        return false;
    }
    mStored.insert(areaId, {position, true});
    writeStoredLayout();
    mpMap->setUnsaved(__func__);
    // Moving an area changes no rooms or exits, so the graph built above stays valid.
    mSeenRevision = mpMap->mapRevision();
    return true;
}

void TAreaGraph::resetLayout(bool keepPinned)
{
    refresh();
    for (auto it = mStored.begin(); it != mStored.end();) {
        it = (keepPinned && it.value().pinned) ? std::next(it) : mStored.erase(it);
    }
    layoutUnplacedAreas();
    writeStoredLayout();
    mpMap->setUnsaved(__func__);
    mSeenRevision = mpMap->mapRevision();
}
