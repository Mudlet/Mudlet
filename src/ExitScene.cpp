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

#include <QHash>
#include <QLineF>
#include <QtMath>

#include <cmath>

namespace ExitScene {

namespace {

constexpr int scmDirections[] = {DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST, DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST, DIR_UP, DIR_DOWN, DIR_IN, DIR_OUT};
// The 2D map's door glyphs are sized by a room's spacing across plus its spacing down
constexpr qreal scmDoorRoomSpan = 2.0;
// A door swing's widest opening, in rooms' spacing
constexpr qreal scmDoorSwingWidth = 0.3;
// A door bar's distance along its exit line from the room's center, in rooms' spacing, and its length across the line
// and width along it, in lines' widths
constexpr float scmDoorBarInset = 0.32f;
constexpr float scmDoorBarLength = 2.2f;
constexpr float scmDoorBarWidth = 0.5f;
constexpr int scmDiscSides = 16;
// Qt's dash stroker's limit, QDashStroker::repetitionLimit()
constexpr qreal scmMaxDashRepetitions = 10000.0;
// The most characters a glyph line is drawn with; a longer one is drawn as a bar, as one line of a misplaced room
// could otherwise make millions of characters every frame
constexpr qint64 scmMaxGlyphRun = 2048;
// How far a shaft for an up or down exit rises or drops from its room, in levels
constexpr float scmShaftLength = 0.35f;
// The part of a one-way exit's shaft, at its tip, in the one-way arrow's color
constexpr float scmShaftTip = 0.3f;

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
    // Along the exit line from its start, across it to the side
    void doorSwing(int status, const QVector3D& from, const QVector3D& along, const QVector3D& side, float length, float height, int level);
    void customLine(const TRoom& room, const QString& key, const QList<QPointF>& points);
    // The color of a line from room to pOther, or of a stub without pOther
    QColor lineColor(const TRoom& room, const TRoom* pOther);
    void glyph(const QVector3D& at, const QVector3D& along, QChar character, const QColor& color, int level);
    // A line's characters in the cells between its ends, which its rooms are drawn in; false, drawing nothing, for
    // a line too long to draw so
    bool glyphRun(const QVector3D& from, const QVector3D& to, const QColor& color, int level);

    const TMap& mMap;
    const Host& mHost;
    const Options& mOptions;
    const float mWidth;
    const float mHeight;
    int mRoomId = 0;
    // Index into mScene.glyphs by cell, as doubled coordinates, and level
    QHash<std::pair<std::pair<qint64, qint64>, int>, qsizetype> mGlyphCells;
    // lineColor() by environment
    QHash<int, QColor> mEnvironmentLineColors;
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
        if (!mScene.solidDashRoomId) {
            mScene.solidDashRoomId = mRoomId;
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

void Builder::glyph(const QVector3D& at, const QVector3D& along, const QChar character, const QColor& color, const int level)
{
    const qint64 column = qRound64(at.x() * 2.0);
    const qint64 row = qRound64(at.y() * 2.0);
    const Glyph placed{QVector3D(column / 2.0f, row / 2.0f, at.z()), along, character, color, level};
    const auto key = std::pair{std::pair{column, row}, level};
    if (const auto found = mGlyphCells.constFind(key); found != mGlyphCells.cend()) {
        // A door's character shows over the line it is on, and the first of either is kept
        Glyph& existing = mScene.glyphs[*found];
        if (existing.character.isNull() && !character.isNull()) {
            existing = placed;
        }
        return;
    }
    mGlyphCells.insert(key, mScene.glyphs.size());
    mScene.glyphs.append(placed);
}

bool Builder::glyphRun(const QVector3D& from, const QVector3D& to, const QColor& color, const int level)
{
    const QVector3D line = to - from;
    const qint64 cells = qRound64(2.0 * qMax(std::abs(line.x()), std::abs(line.y())));
    if (cells > scmMaxGlyphRun) {
        return false;
    }
    if (cells < 2) {
        return true;
    }
    const QVector3D along = line.normalized();
    for (qint64 cell = 1; cell < cells; ++cell) {
        glyph(from + line * (static_cast<float>(cell) / static_cast<float>(cells)), along, QChar(), color, level);
    }
    return true;
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
    if (mOptions.glyphs) {
        // In the line's first cell from the room, as glyphRun() steps along it, as a roguelike puts a door in a room's wall
        const qint64 cells = qRound64(2.0 * qMax(std::abs(line.x()), std::abs(line.y())));
        glyph(cells > 0 ? from + line / static_cast<float>(cells) : to, along, QLatin1Char('+'), ExitAppearance::doorColor(status), level);
        return;
    }
    const QVector3D side = sideways(along);
    const auto place = [&](const QPointF& point) {
        return from + along * static_cast<float>(point.x()) + side * static_cast<float>(point.y());
    };
    // A little taller than the line, so that it shows over it
    const float height = mHeight * 1.2f;
    if (mOptions.doorBars) {
        // Within the exit's half, so that it does not meet a door drawn from the other end
        const QVector3D middle = from + along * qMin(scmDoorBarInset, length * 0.7f);
        const QVector3D across = side * (mWidth * scmDoorBarLength / 2.0f);
        bar(middle - across, middle + across, mWidth * scmDoorBarWidth, height, ExitAppearance::doorColor(status), level);
        return;
    }
    if (mOptions.doorSwings) {
        doorSwing(status, from, along, side, length, height, level);
        return;
    }
    const auto [strokeA, strokeB] = ExitAppearance::doorStrokes(QLineF(0.0, 0.0, length, 0.0), scmDoorRoomSpan);
    const QColor color = ExitAppearance::doorColor(status);
    for (const QLineF& stroke : {strokeA, strokeB}) {
        bar(place(stroke.p1()), place(stroke.p2()), mWidth * 1.5f, height, color, level);
    }
}

void Builder::doorSwing(const int status, const QVector3D& from, const QVector3D& along, const QVector3D& side, const float length, const float height, const int level)
{
    const auto place = [&](const QPointF& point) {
        return from + along * static_cast<float>(point.x()) + side * static_cast<float>(point.y());
    };
    // Clear of the room, whose edge is further out on a diagonal from a square room, as the room hides whatever of
    // the door is inside it. About as wide as a room, but within the exit's half, so that it does not meet the
    // swing of a door drawn from the other end
    const qreal across = qMax(std::abs(along.x()), std::abs(along.y()));
    const qreal edge = mOptions.roundRooms || across < 1e-3 ? mOptions.roomSize / 2.0 : mOptions.roomSize / 2.0 / across;
    const qreal start = edge + mWidth * 1.5;
    const qreal space = length - start;
    // Rooms so close that there is no room for a door between them still get one, over them
    const qreal opening = space > 0.0 ? qMin(scmDoorSwingWidth, space * 0.6) : qMin(scmDoorSwingWidth, length * 0.4);
    // So that a locked door's bolt, which reaches a quarter of the opening back toward the room, starts clear of it
    const qreal jamb = space > 0.0 ? start + opening / 4.0 : length * 0.5;
    const QPointF hinge(jamb, -opening / 2.0);
    // Halfway to the ink, so the status still shows without the plan turning gaudy
    const QColor statusColor = ExitAppearance::doorColor(status);
    const QColor ink = mOptions.exitColor.isValid() ? mOptions.exitColor : statusColor;
    const QColor color = QColor::fromRgbF((statusColor.redF() + ink.redF()) / 2.0f, (statusColor.greenF() + ink.greenF()) / 2.0f, (statusColor.blueF() + ink.blueF()) / 2.0f);
    const bool shut = status != 1;
    const QPointF leafEnd = shut ? QPointF(jamb, opening / 2.0) : QPointF(jamb + opening, -opening / 2.0);
    bar(place(hinge), place(leafEnd), mWidth * 2.0f, height, color, level);
    if (status == 3) {
        // A bolt across the shut leaf
        const QPointF middle(jamb, 0.0);
        bar(place(middle - QPointF(opening / 4.0, 0.0)), place(middle + QPointF(opening / 4.0, 0.0)), mWidth * 2.0f, height, color, level);
    }
    // The leaf's sweep, from where it swings open to the far side of the opening
    constexpr int arcSegments = 8;
    QPointF previous = QPointF(jamb + opening, -opening / 2.0);
    for (int segment = 1; segment <= arcSegments; ++segment) {
        const qreal angle = M_PI / 2.0 * segment / arcSegments;
        const QPointF point = hinge + QPointF(std::cos(angle), std::sin(angle)) * opening;
        bar(place(previous), place(point), mWidth * 1.2f, height, color, level);
        previous = point;
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

QColor Builder::lineColor(const TRoom& room, const TRoom* pOther)
{
    if (!mOptions.environmentLines || (pOther && pOther->environment != room.environment)) {
        return mOptions.exitColor;
    }
    if (const auto found = mEnvironmentLineColors.constFind(room.environment); found != mEnvironmentLineColors.cend()) {
        return *found;
    }
    const QColor color = ExitAppearance::standOutColor(RoomAppearance::environmentColor(mMap, mHost, room.environment), mOptions.background);
    mEnvironmentLineColors.insert(room.environment, color);
    return color;
}

void Builder::addRoom(const int roomId, const TRoom& room)
{
    const auto rz = static_cast<float>(room.z());
    const QVector3D center(room.x(), room.y(), rz);
    const int level = room.z();
    const QColor exitColor = mOptions.exitColor;
    mRoomId = roomId;

    if (!mOptions.areaExitsOnly) {
        for (auto it = room.customLines.constBegin(); it != room.customLines.constEnd(); ++it) {
            customLine(room, it.key(), it.value());
        }

        for (const int direction : room.exitStubs) {
            if (!onCompass(direction)) {
                continue;
            }
            const QVector3D end = center + step(direction) * 0.5f;
            door(room, TRoom::dirCodeToShortString(direction), center, end, level);
            if (mOptions.glyphs) {
                glyph(end, step(direction).normalized(), QChar(), exitColor, level);
                continue;
            }
            const QColor stubColor = lineColor(room, nullptr);
            bar(center, end, mWidth, mHeight, stubColor, level);
            disc(QVector3D(end.x(), end.y(), top(rz)), 0.1f, stubColor, level);
        }
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
        const int destinationId = room.getExit(direction);
        if (destinationId <= 0 || room.customLines.contains(TRoom::dirCodeToShortString(direction))) {
            continue;
        }
        const TRoom* pDestination = mMap.mpRoomDB->getRoom(destinationId);
        if (!pDestination || pDestination->isHidden() || (mOptions.areaExitsOnly && pDestination->getArea() == mOptions.areaId)) {
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
            const QVector3D outer = center + step(exit.direction) * arrowScale;
            mScene.areaExits.append({center, outer, exit.destinationId, level});
            if (mOptions.areaExitsOnly) {
                continue;
            }
            const QVector3D along = step(exit.direction).normalized();
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
            bar(center, center + (destination - center) / 4.0f, mWidth, mHeight, lineColor(room, exit.pDestination), level);
            continue;
        }

        // Stairs that do not lead straight up or down would be long slanting lines that read as exits on the
        // map, so each room with one shows a short shaft of its own instead
        const float rise = destination.z() - center.z();
        const bool upOrDown = (exit.direction == DIR_UP && rise > 0.0f) || (exit.direction == DIR_DOWN && rise < 0.0f);
        if (upOrDown && (exit.pDestination->x() != room.x() || exit.pDestination->y() != room.y())) {
            const QVector3D shaftEnd = center + QVector3D(0.0f, 0.0f, std::copysign(scmShaftLength, rise));
            if (!exit.oneWay) {
                bar(center, shaftEnd, mWidth, mHeight, lineColor(room, exit.pDestination), level);
                continue;
            }
            const QVector3D tipStart = shaftEnd + (center - shaftEnd) * scmShaftTip;
            bar(center, tipStart, mWidth, mHeight, lineColor(room, exit.pDestination), level);
            bar(tipStart, shaftEnd, mWidth, mHeight, ExitAppearance::oneWayArrowColor(), level);
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
            if (upOrDown && mOptions.dashedStairs) {
                qreal phase = 0.0;
                dashedBar(center, destination, ExitAppearance::visibleDashes(Qt::DashLine), phase, exitColor, lineLevel);
                continue;
            }
            if (!mOptions.glyphs || rise != 0.0f || !glyphRun(center, destination, exitColor, lineLevel)) {
                bar(center, destination, mWidth, mHeight, lineColor(room, exit.pDestination), lineLevel);
            }
            continue;
        }

        if (!mOptions.glyphs || rise != 0.0f || !glyphRun(center, destination, exitColor, lineLevel)) {
            qreal phase = 0.0;
            dashedBar(center, destination, ExitAppearance::visibleDashes(Qt::DotLine), phase, lineColor(room, exit.pDestination), lineLevel);
        }
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
