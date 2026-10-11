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

#include "ExitScene.h"

#include "ExitAppearance.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QDebug>
#include <QLineF>
#include <QtMath>

#include <cmath>

namespace ExitScene {

namespace {

constexpr int scmDirections[] = {DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST, DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST, DIR_UP, DIR_DOWN, DIR_IN, DIR_OUT};
// The 2D map's door glyphs are sized by a room's spacing across plus its spacing down
constexpr qreal scmDoorRoomSpan = 2.0;
constexpr int scmDiscSides = 16;
constexpr qreal scmMaxDashRepetitions = 10000.0;

bool onCompass(const int direction)
{
    return direction >= DIR_NORTH && direction <= DIR_SOUTHWEST;
}

// One room's spacing in that direction; TMap::scmUnitVectors has y increasing southward
QVector3D step(const int direction)
{
    const QVector3D unit = TMap::scmUnitVectors.value(direction);
    return QVector3D(unit.x(), -unit.y(), unit.z());
}

// Across the line and level, or along x for an upright line
QVector3D sideways(const QVector3D& along)
{
    const QVector3D side = QVector3D::crossProduct(along, QVector3D(0.0f, 0.0f, 1.0f));
    return side.lengthSquared() < 1e-6f ? QVector3D(1.0f, 0.0f, 0.0f) : side.normalized();
}

class Builder
{
public:
    Builder(const TMap& map, const Host& host, const Options& options)
    : mMap(map)
    , mHost(host)
    , mOptions(options)
    , mWidth(options.lineWidth)
    , mHeight(options.lineHeight)
    {
    }

    void addRoom(int roomId, const TRoom& room);
    bool levelShown(int z) const;
    Scene mScene;

private:
    bool drawsExits(const TRoom* pRoom) const;
    bool hasOneWayExitTo(const TRoom& from, int fromId, const TRoom& to, int toId) const;
    int nearerLevel(int a, int b) const;
    float top(float z) const { return z + mHeight / 2.0f; }
    void bar(const QVector3D& from, const QVector3D& to, float width, float height, const QColor& color, int level);
    // phase carries the pattern on from one piece of a line to the next
    void dashedBar(const QVector3D& from, const QVector3D& to, const QList<qreal>& dashes, qreal& phase, const QColor& color, int level);
    void triangle(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QColor& color, int level);
    void arrowhead(const QVector3D& tip, const QVector3D& along, float length, float halfWidth, const QColor& color, int level, const QColor& outline = QColor());
    void disc(const QVector3D& center, float radius, const QColor& color, int level);
    void door(const TRoom& room, const QString& key, const QVector3D& from, const QVector3D& to, int level);
    void customLine(const TRoom& room, const QString& key, const QList<QPointF>& points);

    const TMap& mMap;
    const Host& mHost;
    const Options& mOptions;
    const float mWidth;
    const float mHeight;
    int mRoomId = 0;
};

bool Builder::levelShown(const int z) const
{
    const qint64 above = qint64{z} - mOptions.centerZ;
    return above <= mOptions.levelsAbove && -above <= mOptions.levelsBelow;
}

bool Builder::drawsExits(const TRoom* pRoom) const
{
    return pRoom && !pRoom->isHidden() && pRoom->getArea() == mOptions.areaId && levelShown(pRoom->z());
}

bool Builder::hasOneWayExitTo(const TRoom& from, const int fromId, const TRoom& to, const int toId) const
{
    for (const int direction : scmDirections) {
        if (from.getExit(direction) == toId && !from.customLines.contains(TRoom::dirCodeToShortString(direction)) && to.getExit(TMap::scmReverseDirections.value(direction)) != fromId) {
            return true;
        }
    }
    return false;
}

int Builder::nearerLevel(const int a, const int b) const
{
    const qint64 distanceA = std::abs(qint64{a} - mOptions.centerZ);
    const qint64 distanceB = std::abs(qint64{b} - mOptions.centerZ);
    if (distanceA != distanceB) {
        return distanceA < distanceB ? a : b;
    }
    return qMin(a, b);
}

void Builder::bar(const QVector3D& from, const QVector3D& to, const float width, const float height, const QColor& color, const int level)
{
    if ((to - from).lengthSquared() < 1e-10f) {
        return;
    }
    mScene.segments.append({from, to, width, height, color, level});
}

void Builder::dashedBar(const QVector3D& from, const QVector3D& to, const QList<qreal>& dashes, qreal& phase, const QColor& color, const int level)
{
    const float length = (to - from).length();
    if (length < 1e-5f) {
        return;
    }
    if (dashes.isEmpty()) {
        bar(from, to, mWidth, mHeight, color, level);
        return;
    }
    qreal period = 0.0;
    for (const qreal dash : dashes) {
        period += dash * mWidth;
    }
    if (period <= 0.0) {
        bar(from, to, mWidth, mHeight, color, level);
        return;
    }
    // As Qt's dash stroker does, draw solid a pattern too fine to repeat along the line in reasonable time
    if (!(length / period <= scmMaxDashRepetitions)) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning().nospace() << "ExitScene: the line from room " << mRoomId << " is drawn solid, as its exit lines are too thin to dash";
        }
        bar(from, to, mWidth, mHeight, color, level);
        phase = 0.0;
        return;
    }
    const QVector3D along = (to - from) / length;
    // Walk the pattern from where the last piece left it
    qreal position = -std::fmod(phase, period);
    int index = 0;
    while (position < length) {
        const qreal dashLength = dashes.at(index) * mWidth;
        const bool drawn = (index % 2) == 0;
        const qreal start = qMax(position, 0.0);
        const qreal end = qMin(position + dashLength, static_cast<qreal>(length));
        if (drawn && end > start) {
            bar(from + along * static_cast<float>(start), from + along * static_cast<float>(end), mWidth, mHeight, color, level);
        }
        position += dashLength;
        index = (index + 1) % dashes.size();
    }
    phase = std::fmod(phase + length, period);
}

void Builder::triangle(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QColor& color, const int level)
{
    mScene.triangles.append({a, b, c, color, level});
}

void Builder::arrowhead(const QVector3D& tip, const QVector3D& along, const float length, const float halfWidth, const QColor& color, const int level, const QColor& outline)
{
    const QVector3D base = tip - along * length;
    QList<QVector3D> wings{sideways(along)};
    const bool upright = std::abs(along.z()) > 0.99f;
    if (upright) {
        // Upright: a second, crossed head so that it shows from any side
        wings.append(QVector3D(0.0f, 1.0f, 0.0f));
    }
    for (const QVector3D& wing : wings) {
        const QVector3D left = base + wing * halfWidth;
        const QVector3D right = base - wing * halfWidth;
        if (outline.isValid()) {
            // Grown about its incenter by about half a line width, as the 2D map's 2px outline does
            const float sideLength = std::hypot(length, halfWidth);
            const float perimeter = 2.0f * sideLength + 2.0f * halfWidth;
            const float inradius = (length * halfWidth) / (perimeter / 2.0f);
            const QVector3D incenter = (tip * (2.0f * halfWidth) + left * sideLength + right * sideLength) / perimeter;
            const float grow = (inradius + mWidth / 2.0f) / inradius;
            triangle(incenter + (tip - incenter) * grow, incenter + (left - incenter) * grow, incenter + (right - incenter) * grow, outline, level);
        }
        // Lifted a little off its own outline, so that the depth test does not hide it; an upright
        // one gets a fill on each face, as it is seen from either side
        QVector3D normal = QVector3D::crossProduct(along, wing).normalized();
        if (normal.z() < 0.0f) {
            normal = -normal;
        }
        const QVector3D lift = normal * (mHeight / 10.0f);
        triangle(tip + lift, left + lift, right + lift, color, level);
        if (upright) {
            triangle(tip - lift, left - lift, right - lift, color, level);
        }
    }
}

void Builder::disc(const QVector3D& center, const float radius, const QColor& color, const int level)
{
    for (int side = 0; side < scmDiscSides; ++side) {
        const qreal angleA = 2.0 * M_PI * side / scmDiscSides;
        const qreal angleB = 2.0 * M_PI * (side + 1) / scmDiscSides;
        triangle(center, center + QVector3D(radius * std::cos(angleA), radius * std::sin(angleA), 0.0f), center + QVector3D(radius * std::cos(angleB), radius * std::sin(angleB), 0.0f), color, level);
    }
}

void Builder::door(const TRoom& room, const QString& key, const QVector3D& from, const QVector3D& to, const int level)
{
    const int status = room.doors.value(key);
    if (status <= 0) {
        return;
    }
    const QVector3D line = to - from;
    const float length = line.length();
    if (length < 1e-5f) {
        return;
    }
    // Worked out flat along the line, as the 2D map does, then stood on it
    const QVector3D along = line / length;
    const QVector3D side = sideways(along);
    const auto place = [&](const QPointF& point) {
        return from + along * static_cast<float>(point.x()) + side * static_cast<float>(point.y());
    };
    const auto [strokeA, strokeB] = ExitAppearance::doorStrokes(QLineF(0.0, 0.0, length, 0.0), scmDoorRoomSpan);
    const QColor color = ExitAppearance::doorColor(status);
    // A little taller than the line, so that it shows over it
    const float height = mHeight * 1.2f;
    for (const QLineF& stroke : {strokeA, strokeB}) {
        bar(place(stroke.p1()), place(stroke.p2()), mWidth * 1.5f, height, color, level);
    }
}

void Builder::customLine(const TRoom& room, const QString& key, const QList<QPointF>& points)
{
    if (points.isEmpty()) {
        return;
    }
    const Qt::PenStyle style = room.customLinesStyle.value(key, Qt::SolidLine);
    const QColor color = room.customLinesColor.value(key, QColor(Qt::red));
    const int level = room.z();
    const auto rz = static_cast<float>(room.z());
    const QVector3D origin(room.x(), room.y(), rz);

    int direction = 0;
    for (const int candidate : scmDirections) {
        if (TRoom::dirCodeToShortString(candidate) == key) {
            direction = candidate;
            break;
        }
    }
    const bool xyPlaneExit = onCompass(direction);

    QList<QVector3D> polyline{origin, xyPlaneExit ? origin + step(direction) * 0.5f : origin};
    for (const QPointF& point : points) {
        polyline.append(QVector3D(point.x(), point.y(), rz));
    }

    // Down to (or up to) a destination on another level that the line ends over
    const int destinationId = direction ? room.getExit(direction) : room.getSpecialExits().value(key);
    if (const TRoom* pDestination = mMap.mpRoomDB->getRoom(destinationId); pDestination && pDestination->getArea() == mOptions.areaId && pDestination->z() != room.z() && !pDestination->isHidden()) {
        const QVector3D last = polyline.constLast();
        if (std::abs(last.x() - pDestination->x()) <= 0.5f && std::abs(last.y() - pDestination->y()) <= 0.5f) {
            polyline.append(QVector3D(last.x(), last.y(), pDestination->z()));
        }
    }

    if (style != Qt::NoPen) {
        const QList<qreal> dashes = ExitAppearance::visibleDashes(style);
        qreal phase = 0.0;
        for (qsizetype i = 1; i < polyline.size(); ++i) {
            dashedBar(polyline.at(i - 1), polyline.at(i), dashes, phase, color, level);
            if (dashes.isEmpty() && i + 1 < polyline.size()) {
                // Rounds the joint, as the 2D map's round join does
                disc(QVector3D(polyline.at(i).x(), polyline.at(i).y(), top(polyline.at(i).z())), mWidth / 2.0f, color, level);
            }
        }
    }

    // The arrow is at the line's last drawn point on its own level, as on the 2D map
    const qsizetype lastOnLevel = 2 + points.size() - 1;
    if (room.customLinesArrow.value(key)) {
        const QVector3D tip = polyline.at(lastOnLevel);
        const QVector3D back = polyline.at(lastOnLevel - 1);
        if ((tip - back).lengthSquared() > 1e-10f) {
            arrowhead(QVector3D(tip.x(), tip.y(), top(tip.z())), (tip - back).normalized(), mWidth * 5.0f, mWidth * 2.0f, color, level);
        }
    }

    // Non-XY-plane exits have their first two points coincident
    if (xyPlaneExit) {
        door(room, key, polyline.at(0), polyline.at(1), level);
    } else {
        door(room, key, polyline.at(1), polyline.at(2), level);
    }
}

void Builder::addRoom(const int roomId, const TRoom& room)
{
    const auto rz = static_cast<float>(room.z());
    const QVector3D center(room.x(), room.y(), rz);
    const int level = room.z();
    const QColor exitColor = mOptions.exitColor;
    mRoomId = roomId;

    for (auto it = room.customLines.constBegin(); it != room.customLines.constEnd(); ++it) {
        customLine(room, it.key(), it.value());
    }

    for (const int direction : room.exitStubs) {
        if (!onCompass(direction)) {
            continue;
        }
        const QVector3D end = center + step(direction) * 0.5f;
        door(room, TRoom::dirCodeToShortString(direction), center, end, level);
        bar(center, end, mWidth, mHeight, exitColor, level);
        disc(QVector3D(end.x(), end.y(), top(rz)), 0.1f, exitColor, level);
    }

    struct Exit
    {
        int direction;
        const TRoom* pDestination;
        int destinationId;
        bool oneWay;
    };
    QList<Exit> exits;
    for (const int direction : scmDirections) {
        const QString key = TRoom::dirCodeToShortString(direction);
        if (room.customLines.contains(key)) {
            continue;
        }
        const int destinationId = room.getExit(direction);
        if (destinationId <= 0) {
            continue;
        }
        const TRoom* pDestination = mMap.mpRoomDB->getRoom(destinationId);
        if (!pDestination || pDestination->isHidden()) {
            continue;
        }
        const bool oneWay = pDestination->getExit(TMap::scmReverseDirections.value(direction)) != roomId;
        exits.append({direction, pDestination, destinationId, oneWay});
    }

    const float arrowScale = mOptions.largeAreaExitArrows ? 2.0f : 1.0f;
    const QColor outlineColor = ExitAppearance::contrastColor(mOptions.background);
    for (const Exit& exit : exits) {
        const QString key = TRoom::dirCodeToShortString(exit.direction);
        const bool inOrOut = exit.direction == DIR_IN || exit.direction == DIR_OUT;

        if (exit.pDestination->getArea() != mOptions.areaId) {
            if (inOrOut) {
                // Nowhere on this map to point to; the room's marker shows it
                continue;
            }
            const QVector3D along = step(exit.direction).normalized();
            const QVector3D outer = center + step(exit.direction) * arrowScale;
            const QColor color = RoomAppearance::environmentColor(mMap, mHost, exit.pDestination->environment);
            // The outline is lower, so that the line shows over it from above
            bar(center, outer, mWidth * 2.0f, mHeight * 0.7f, outlineColor, level);
            bar(center, outer, mWidth, mHeight, color, level);
            const float length = mOptions.largeAreaExitArrows ? 0.8f : mWidth * 5.0f;
            const float halfWidth = mOptions.largeAreaExitArrows ? 0.3f : mWidth * 2.0f;
            arrowhead(QVector3D(outer.x(), outer.y(), top(outer.z())), along, length, halfWidth, color, level, outlineColor);
            if (onCompass(exit.direction)) {
                door(room, key, center, center + (outer - center) / 2.0f, level);
            }
            continue;
        }

        const QVector3D destination(exit.pDestination->x(), exit.pDestination->y(), exit.pDestination->z());
        const int lineLevel = nearerLevel(level, exit.pDestination->z());
        if (onCompass(exit.direction)) {
            door(room, key, center, center + (destination - center) / 2.0f, level);
        }

        if (inOrOut && mOptions.inOutStubs) {
            bar(center, center + (destination - center) / 4.0f, mWidth, mHeight, exitColor, level);
            continue;
        }

        bool oneWayToDestination = false;
        for (const Exit& other : exits) {
            if (other.oneWay && other.destinationId == exit.destinationId) {
                oneWayToDestination = true;
                break;
            }
        }
        if (!oneWayToDestination) {
            const QString reverseKey = TRoom::dirCodeToShortString(TMap::scmReverseDirections.value(exit.direction));
            // A one-way exit of its own to this room has the other end draw a dotted line instead
            const bool otherEndDraws = drawsExits(exit.pDestination) && !exit.pDestination->customLines.contains(reverseKey) && !hasOneWayExitTo(*exit.pDestination, exit.destinationId, room, roomId);
            if (otherEndDraws && exit.destinationId < roomId) {
                continue;
            }
            bar(center, destination, mWidth, mHeight, exitColor, lineLevel);
            continue;
        }

        qreal phase = 0.0;
        dashedBar(center, destination, ExitAppearance::visibleDashes(Qt::DotLine), phase, exitColor, lineLevel);
        const QVector3D line = destination - center;
        if (line.lengthSquared() > 1e-10f) {
            const QVector3D along = line.normalized();
            const QVector3D middle = center + line / 2.0f + along * (mWidth * 2.5f);
            arrowhead(QVector3D(middle.x(), middle.y(), top(middle.z())), along, mWidth * 5.0f, mWidth * 2.0f, ExitAppearance::oneWayArrowColor(), lineLevel);
        }
    }
}

} // namespace

Scene build(const TMap& map, const Host& host, const Options& options)
{
    Builder builder(map, host, options);
    const TArea* pArea = map.mpRoomDB->getArea(options.areaId);
    if (!pArea) {
        return {};
    }
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pRoom = map.mpRoomDB->getRoom(roomId);
        if (!pRoom || pRoom->isHidden() || !builder.levelShown(pRoom->z())) {
            continue;
        }
        builder.addRoom(roomId, *pRoom);
    }
    return builder.mScene;
}

} // namespace ExitScene
