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

#include "ExitAppearance.h"

#include "TMap.h"
#include "TRoom.h"

#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace ExitAppearance {

QColor doorColor(const int status)
{
    if (status == 1) {
        return QColor(10, 155, 10);
    }
    if (status == 2) {
        return QColor(155, 155, 10);
    }
    return QColor(155, 10, 10);
}

QColor oneWayArrowColor()
{
    return QColor(255, 100, 100);
}

QColor contrastColor(const QColor& background)
{
    return background.lightness() > 127 ? QColor(Qt::black) : QColor(Qt::white);
}

std::pair<QLineF, QLineF> doorStrokes(const QLineF& exitLine, const qreal roomSpan)
{
    // A set of numbers that can be converted to "static" type and be frobbed
    // during development:
    const double shortPositionFactor = 0.225;
    const double middlePositionFactor = 0.95;
    const double longPositionFactor = 0.45;
    const double innerThresholdFactor = 0.225;
    const double outerThresholdFactor = 0.45;
    const double middleAngleFactor = 165.0;
    const double middleFiddleFactor = 0.25;
    const double endAngleFactor = 150.0;
    const double endFiddleFactor = 0.50;
    const bool isShortLine = ((exitLine.length() / roomSpan) < innerThresholdFactor);
    const bool isLongLine = ((exitLine.length() / roomSpan) > outerThresholdFactor);
    QLineF line{exitLine};
    if (isShortLine) {
        line.setLength(shortPositionFactor * roomSpan);
    } else if (isLongLine) {
        line.setLength(longPositionFactor * roomSpan);
    } else {
        line.setLength(middlePositionFactor * line.length());
    }
    // The end of "doorLine" is the one nearest the room center
    QLineF doorLine(line.p2(), line.p1());
    // This drags that end outwards - so that the door can be positioned in
    // 1/10 of the overall distance from the end:
    doorLine.setLength(endFiddleFactor * roomSpan);
    // This line starts at the inner end of doorLine and ends at the outer end:
    QLineF subDoorLineA{doorLine.p2(), doorLine.p1()};
    // This swings the outer end anti-clockwise:
    subDoorLineA.setAngle(doorLine.angle() - endAngleFactor);
    // This swings another one in the same configuration clockwise:
    QLineF subDoorLineD{doorLine.p2(), doorLine.p1()};
    subDoorLineD.setAngle(doorLine.angle() + endAngleFactor);

    // Repeat for an intermediate second pair of lines - a smaller factor puts
    // these points nearer to the outer edge - but the amount from the
    // centerline is less:
    doorLine.setLength(middleFiddleFactor * roomSpan);
    QLineF subDoorLineB{doorLine.p2(), doorLine.p1()};
    subDoorLineB.setAngle(doorLine.angle() - middleAngleFactor);
    QLineF subDoorLineC{doorLine.p2(), doorLine.p1()};
    subDoorLineC.setAngle(doorLine.angle() + middleAngleFactor);

    return {QLineF(subDoorLineA.p2(), subDoorLineB.p2()), QLineF(subDoorLineD.p2(), subDoorLineC.p2())};
}

QList<qreal> visibleDashes(const Qt::PenStyle style)
{
    if (style == Qt::SolidLine || style == Qt::NoPen || style == Qt::CustomDashLine) {
        return {};
    }
    // QPen's pattern leaves out the caps, which reach half the width past each end of a dash
    QList<qreal> dashes = QPen(style).dashPattern();
    for (qsizetype i = 0; i < dashes.size(); ++i) {
        dashes[i] = (i % 2) == 0 ? dashes.at(i) + 1.0 : qMax(0.0, dashes.at(i) - 1.0);
    }
    return dashes;
}

void paintExitMarkers(QPainter& painter, QPen& pen, const TRoom& room, const float rx, const float ry, const qreal roomWidth, const qreal roomHeight, const QColor& roomColor, const bool cosmeticPen)
{
    // Change these from const to static to tweak them while running in a debugger...!
    const float allInsideTipOffsetFactor = 1 / 20.0f;
    const float upDownXOrYFactor = 1 / 3.1f;
    const float inOuterXFactor = 1 / 4.5f;
    const float inUpDownYFactor = 1 / 7.0f;
    const float outOuterXFactor = 1 / 2.2f;
    const float outUpDownYFactor = 1 / 5.5f;
    const float outInterXFactor = 1 / 3.5f;
    const float outerRealDoorPenThicknessFactor = 0.050f;
    const float outerStubDoorPenThicknessFactor = 0.025f;
    const float innerRealDoorPenThicknessFactor = 0.025f;
    const float innerStubDoorPenThicknessFactor = 0.0125f;

    const QColor lc = contrastColor(roomColor);
    pen = painter.pen();
    pen.setColor(lc);
    pen.setCosmetic(cosmeticPen);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    QPen innerPen = pen;
    painter.save();

    QBrush innerBrush = painter.brush();
    innerBrush.setStyle(Qt::NoBrush);

    // Paints one marker's polygons, filled and outlined as the exit (or stub) and any door on it require
    const auto paintMarker = [&](const QList<QPolygonF>& polygons, const QString& doorKey, const bool realExit) {
        bool isDoor = true;
        QBrush brush = painter.brush();
        const int doorStatus = room.doors.value(doorKey);
        switch (doorStatus) {
        case 1:
        case 2:
        case 3:
            brush.setColor(doorColor(doorStatus));
            innerPen.setColor(doorColor(doorStatus));
            break;
        default:
            brush.setColor(lc);
            isDoor = false;
        }
        if (realExit) {
            pen.setWidthF(roomWidth * outerRealDoorPenThicknessFactor);
            innerPen.setWidthF(roomWidth * innerRealDoorPenThicknessFactor);
            brush.setStyle(Qt::Dense4Pattern);
        } else {
            pen.setWidthF(roomWidth * outerStubDoorPenThicknessFactor);
            innerPen.setWidthF(roomWidth * innerStubDoorPenThicknessFactor);
            brush.setStyle(Qt::DiagCrossPattern);
        }
        painter.setPen(pen);
        painter.setBrush(brush);
        for (const QPolygonF& polygon : polygons) {
            painter.drawPolygon(polygon);
        }
        if (isDoor) {
            // Draw a narrower outline on top of the existing one if there is
            // a door - to help emphasis the coloured door if the brush from
            // the main one is not obvious given the main room colour.
            painter.setPen(innerPen);
            painter.setBrush(innerBrush);
            for (const QPolygonF& polygon : polygons) {
                painter.drawPolygon(polygon);
            }
        }
    };

    if (room.getUp() > 0 || room.exitStubs.contains(DIR_UP)) {
        QPolygonF poly_up;
        poly_up.append(QPointF(rx, ry + (roomHeight * allInsideTipOffsetFactor)));
        poly_up.append(QPointF(rx - (roomWidth * upDownXOrYFactor), ry + (roomHeight * upDownXOrYFactor)));
        poly_up.append(QPointF(rx + (roomWidth * upDownXOrYFactor), ry + (roomHeight * upDownXOrYFactor)));
        paintMarker({poly_up}, qsl("up"), room.getUp() > 0);
    }

    if (room.getDown() > 0 || room.exitStubs.contains(DIR_DOWN)) {
        QPolygonF poly_down;
        poly_down.append(QPointF(rx, ry - (roomHeight * allInsideTipOffsetFactor)));
        poly_down.append(QPointF(rx - (roomWidth * upDownXOrYFactor), ry - (roomHeight * upDownXOrYFactor)));
        poly_down.append(QPointF(rx + (roomWidth * upDownXOrYFactor), ry - (roomHeight * upDownXOrYFactor)));
        paintMarker({poly_down}, qsl("down"), room.getDown() > 0);
    }

    if (room.getIn() > 0 || room.exitStubs.contains(DIR_IN)) {
        QPolygonF poly_in_left;
        QPolygonF poly_in_right;
        poly_in_left.append(QPointF(rx - (roomWidth * allInsideTipOffsetFactor), ry));
        poly_in_left.append(QPointF(rx - (roomWidth * inOuterXFactor), ry + (roomHeight * inUpDownYFactor)));
        poly_in_left.append(QPointF(rx - (roomWidth * inOuterXFactor), ry - (roomHeight * inUpDownYFactor)));
        poly_in_right.append(QPointF(rx + (roomWidth * allInsideTipOffsetFactor), ry));
        poly_in_right.append(QPointF(rx + (roomWidth * inOuterXFactor), ry + (roomHeight * inUpDownYFactor)));
        poly_in_right.append(QPointF(rx + (roomWidth * inOuterXFactor), ry - (roomHeight * inUpDownYFactor)));
        paintMarker({poly_in_left, poly_in_right}, qsl("in"), room.getIn() > 0);
    }

    if (room.getOut() > 0 || room.exitStubs.contains(DIR_OUT)) {
        QPolygonF poly_out_left;
        QPolygonF poly_out_right;
        poly_out_left.append(QPointF(rx - (roomWidth * outOuterXFactor), ry));
        poly_out_left.append(QPointF(rx - (roomWidth * outInterXFactor), ry + (roomHeight * outUpDownYFactor)));
        poly_out_left.append(QPointF(rx - (roomWidth * outInterXFactor), ry - (roomHeight * outUpDownYFactor)));
        poly_out_right.append(QPointF(rx + (roomWidth * outOuterXFactor), ry));
        poly_out_right.append(QPointF(rx + (roomWidth * outInterXFactor), ry + (roomHeight * outUpDownYFactor)));
        poly_out_right.append(QPointF(rx + (roomWidth * outInterXFactor), ry - (roomHeight * outUpDownYFactor)));
        paintMarker({poly_out_left, poly_out_right}, qsl("out"), room.getOut() > 0);
    }

    painter.restore();
}

} // namespace ExitAppearance
