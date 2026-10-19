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

#include "MapAtlas.h"

#include "TArea.h"
#include "TAreaGraph.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QLineF>
#include <QMap>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace {
constexpr int scmNodeSegments = 32;
// A link is this much of the smaller of the two areas' radii wide, but never narrower than half a room
constexpr float scmLinkWidthPerRadius = 0.12f;
constexpr float scmMinimumLinkWidth = 0.5f;
// The ring marking an area lies just outside its node
constexpr float scmRingInner = 1.06f;
constexpr float scmRingOuter = 1.2f;

float smoothStep(const float from, const float to, const float value)
{
    const float t = std::clamp((value - from) / (to - from), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

qreal cross(const QPointF& origin, const QPointF& a, const QPointF& b)
{
    return (a.x() - origin.x()) * (b.y() - origin.y()) - (a.y() - origin.y()) * (b.x() - origin.x());
}

void appendCorner(QVector<float>& vertices, QVector<float>& colors, const QPointF& point, const QColor& color)
{
    vertices << static_cast<float>(point.x()) << static_cast<float>(point.y()) << 0.0f;
    colors << color.redF() << color.greenF() << color.blueF() << 1.0f;
}

void appendTriangle(QVector<float>& vertices, QVector<float>& colors, const QPointF& a, const QPointF& b, const QPointF& c, const QColor& color)
{
    appendCorner(vertices, colors, a, color);
    appendCorner(vertices, colors, b, color);
    appendCorner(vertices, colors, c, color);
}

QPointF onCircle(const QPointF& center, const qreal radius, const int segment)
{
    const qreal angle = 2.0 * std::numbers::pi * segment / scmNodeSegments;
    return center + QPointF(std::cos(angle), std::sin(angle)) * radius;
}

qreal distanceToSegment(const QPointF& point, const QPointF& a, const QPointF& b)
{
    const QPointF along = b - a;
    const qreal lengthSquared = QPointF::dotProduct(along, along);
    const qreal t = lengthSquared > 0.0 ? std::clamp(QPointF::dotProduct(point - a, along) / lengthSquared, 0.0, 1.0) : 0.0;
    return QLineF(point, a + along * t).length();
}
} // namespace

MapAtlas::Weights MapAtlas::weightsFor(const float roomPixels, const float areaPixels, const float distance)
{
    Weights weights;
    weights.footprint = std::min(1.0f - smoothStep(scmFootprintsOnlyPixels, scmRoomsAlonePixels, roomPixels), smoothStep(scmRoomsAloneDistance, scmFootprintsOnlyDistance, distance));
    weights.world = 1.0f - smoothStep(scmWorldOnlyPixels, scmWorldFromPixels, areaPixels);
    return weights;
}

QPolygonF MapAtlas::hull(QList<QPointF> points)
{
    std::sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() < 3) {
        return QPolygonF(points);
    }
    // Andrew's monotone chain: the lower half left to right, then the upper half back
    QList<QPointF> chain;
    chain.reserve(points.size() + 1);
    for (const QPointF& point : std::as_const(points)) {
        while (chain.size() >= 2 && cross(chain.at(chain.size() - 2), chain.last(), point) <= 0.0) {
            chain.removeLast();
        }
        chain.append(point);
    }
    const qsizetype lowerSize = chain.size();
    for (qsizetype i = points.size() - 2; i >= 0; --i) {
        while (chain.size() > lowerSize && cross(chain.at(chain.size() - 2), chain.last(), points.at(i)) <= 0.0) {
            chain.removeLast();
        }
        chain.append(points.at(i));
    }
    // The last is the first again
    chain.removeLast();
    return QPolygonF(chain);
}

void MapAtlas::clear()
{
    mAreas.clear();
    mAreaIndex.clear();
    mTypicalRadius = 0.0f;
    mFootprintVertices.clear();
    mFootprintColors.clear();
    mWorldVertices.clear();
    mWorldColors.clear();
}

void MapAtlas::build(const TMap& map, const std::function<QColor(int)>& environmentColor, const QColor& linkColor)
{
    clear();
    const TAreaGraph* pGraph = map.areaGraph();
    if (!pGraph || !map.mpRoomDB) {
        return;
    }

    for (const TAreaGraph::Node& node : pGraph->nodes()) {
        const TArea* pArea = node.roomCount > 0 ? map.mpRoomDB->getArea(node.areaId) : nullptr;
        if (!pArea) {
            continue;
        }
        const QPointF boxCenter((node.minX + node.maxX) / 2.0, (node.minY + node.maxY) / 2.0);
        Area area;
        area.id = node.areaId;
        area.name = node.name;
        area.center = node.position;
        area.radius = static_cast<float>(node.radius);
        area.offset = node.position - boxCenter;

        // Only a row's ends can be on the hull
        QMap<int, std::pair<int, int>> rows;
        QHash<int, int> environments;
        qreal nearestLanding = std::numeric_limits<qreal>::max();
        for (const int roomId : pArea->getAreaRooms()) {
            const TRoom* pR = map.mpRoomDB->getRoom(roomId);
            if (!pR || pR->isHidden()) {
                continue;
            }
            ++area.roomCount;
            const auto row = rows.find(pR->y());
            if (row == rows.end()) {
                rows.insert(pR->y(), {pR->x(), pR->x()});
            } else {
                row->first = qMin(row->first, pR->x());
                row->second = qMax(row->second, pR->x());
            }
            ++environments[pR->environment];
            const qreal fromMiddle = QLineF(boxCenter, QPointF(pR->x(), pR->y())).length();
            // Ties go to the lowest ID, as the rooms come in no particular order
            if (fromMiddle < nearestLanding || (fromMiddle == nearestLanding && roomId < area.landingRoomId)) {
                nearestLanding = fromMiddle;
                area.landingRoomId = roomId;
                area.landing = QPoint(pR->x(), pR->y());
                area.landingLevel = pR->z();
            }
        }
        if (rows.isEmpty()) {
            continue;
        }

        int environment = 0;
        int mostRooms = 0;
        for (auto it = environments.cbegin(); it != environments.cend(); ++it) {
            if (it.value() > mostRooms || (it.value() == mostRooms && it.key() < environment)) {
                mostRooms = it.value();
                environment = it.key();
            }
        }
        area.color = environmentColor(environment);

        QList<QPointF> corners;
        corners.reserve(rows.size() * 4);
        for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
            const qreal y = it.key();
            for (const qreal x : {it->first - 0.5, it->second + 0.5}) {
                corners.append(QPointF(x, y - 0.5) + area.offset);
                corners.append(QPointF(x, y + 0.5) + area.offset);
            }
        }
        area.footprint = hull(corners);
        for (qsizetype i = 1; i + 1 < area.footprint.size(); ++i) {
            appendTriangle(mFootprintVertices, mFootprintColors, area.footprint.at(0), area.footprint.at(i), area.footprint.at(i + 1), area.color);
        }

        mAreaIndex.insert(area.id, mAreas.size());
        mAreas.append(area);
    }
    if (mAreas.isEmpty()) {
        return;
    }

    QList<float> radii;
    radii.reserve(mAreas.size());
    for (const Area& area : std::as_const(mAreas)) {
        radii.append(area.radius);
    }
    std::nth_element(radii.begin(), radii.begin() + radii.size() / 2, radii.end());
    mTypicalRadius = radii.at(radii.size() / 2);

    // From rim to rim, so that no link lies under a node
    for (const TAreaGraph::Link& link : pGraph->links()) {
        const Area* pFrom = area(link.fromArea);
        const Area* pTo = area(link.toArea);
        if (!pFrom || !pTo) {
            continue;
        }
        const QLineF between(pFrom->center, pTo->center);
        const qreal length = between.length();
        if (length <= pFrom->radius + pTo->radius) {
            continue;
        }
        const QPointF direction = (pTo->center - pFrom->center) / length;
        const QPointF start = pFrom->center + direction * pFrom->radius;
        const QPointF end = pTo->center - direction * pTo->radius;
        const qreal width = qMax(scmMinimumLinkWidth, scmLinkWidthPerRadius * qMin(pFrom->radius, pTo->radius));
        const QPointF side = QPointF(-direction.y(), direction.x()) * (width / 2.0);
        appendTriangle(mWorldVertices, mWorldColors, start + side, end + side, end - side, linkColor);
        appendTriangle(mWorldVertices, mWorldColors, start + side, end - side, start - side, linkColor);
    }
    for (const Area& area : std::as_const(mAreas)) {
        for (int segment = 0; segment < scmNodeSegments; ++segment) {
            appendTriangle(mWorldVertices, mWorldColors, area.center, onCircle(area.center, area.radius, segment), onCircle(area.center, area.radius, segment + 1), area.color);
        }
    }
}

const MapAtlas::Area* MapAtlas::area(const int areaId) const
{
    const auto it = mAreaIndex.constFind(areaId);
    return it == mAreaIndex.cend() ? nullptr : &mAreas.at(*it);
}

int MapAtlas::areaAt(const QPointF& point, const bool nodes, const float slop) const
{
    int nearestArea = 0;
    // How far outside its shape the point is: negative inside a node, 0 inside a footprint
    qreal nearest = slop;
    for (const Area& area : mAreas) {
        qreal outside = 0.0;
        if (nodes) {
            outside = QLineF(point, area.center).length() - area.radius;
        } else {
            const QPolygonF& shape = area.footprint;
            bool inside = shape.size() >= 3;
            outside = std::numeric_limits<qreal>::max();
            for (qsizetype i = 0; i < shape.size(); ++i) {
                const QPointF& a = shape.at(i);
                const QPointF& b = shape.at((i + 1) % shape.size());
                inside = inside && cross(a, b, point) >= 0.0;
                outside = qMin(outside, distanceToSegment(point, a, b));
            }
            if (inside) {
                outside = 0.0;
            }
        }
        if (outside < nearest || (outside == nearest && !nearestArea)) {
            nearest = outside;
            nearestArea = area.id;
        }
    }
    return nearestArea;
}

void MapAtlas::appendRing(const int areaId, const QColor& color, QVector<float>& vertices, QVector<float>& colors) const
{
    const Area* pArea = area(areaId);
    if (!pArea) {
        return;
    }
    for (int segment = 0; segment < scmNodeSegments; ++segment) {
        const QPointF innerFrom = onCircle(pArea->center, pArea->radius * scmRingInner, segment);
        const QPointF innerTo = onCircle(pArea->center, pArea->radius * scmRingInner, segment + 1);
        const QPointF outerFrom = onCircle(pArea->center, pArea->radius * scmRingOuter, segment);
        const QPointF outerTo = onCircle(pArea->center, pArea->radius * scmRingOuter, segment + 1);
        appendTriangle(vertices, colors, innerFrom, outerFrom, outerTo, color);
        appendTriangle(vertices, colors, innerFrom, outerTo, innerTo, color);
    }
}
