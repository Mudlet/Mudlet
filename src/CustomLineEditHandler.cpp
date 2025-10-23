/***************************************************************************
 *   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
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

#include "CustomLineEditHandler.h"

#include "TMap.h"
#include "TArea.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QLineF>
#include <QMapIterator>
#include <QSetIterator>

#include <cmath>

CustomLineEditHandler::CustomLineEditHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool CustomLineEditHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    const QEvent::Type eventType = context.event->type();

    if (eventType == QEvent::MouseButtonPress) {
        if (context.button != Qt::LeftButton) {
            return false;
        }

        if (context.isMapViewOnly || context.hasMultiSelection || context.isCustomLineDrawing) {
            return false;
        }

        return true;
    }

    if (eventType == QEvent::MouseMove) {
        if (!(context.buttons & Qt::LeftButton)) {
            return false;
        }

        return mMapWidget.mCustomLineSelectedRoom != 0 && mMapWidget.mCustomLineSelectedPoint >= 0;
    }

    if (eventType == QEvent::MouseButtonRelease) {
        if (context.button != Qt::LeftButton) {
            return false;
        }

        return mMapWidget.mCustomLineSelectedPoint >= 0;
    }

    return false;
}

bool CustomLineEditHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event) {
        return false;
    }

    const QEvent::Type eventType = context.event->type();

    if (eventType == QEvent::MouseButtonPress) {
        return handleMousePress(context);
    }

    if (eventType == QEvent::MouseMove) {
        return handleMouseMove(context);
    }

    if (eventType == QEvent::MouseButtonRelease) {
        return handleMouseRelease();
    }

    return false;
}

bool CustomLineEditHandler::handleMousePress(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB || !context.area) {
        resetSelection();
        return false;
    }

    mMapWidget.mCustomLineSelectedPoint = -1;

    const float mx = static_cast<float>(context.mapX);
    const float my = static_cast<float>(context.mapY);
    const QPointF& clickPoint = context.mapPoint;

    QSetIterator<int> roomIterator(context.area->rooms);

    while (roomIterator.hasNext()) {
        const int currentRoomId = roomIterator.next();
        TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(currentRoomId);
        if (!room) {
            continue;
        }

        QMapIterator<QString, QList<QPointF>> lineIterator(room->customLines);
        while (lineIterator.hasNext()) {
            lineIterator.next();
            const QList<QPointF>& linePoints = lineIterator.value();
            if (linePoints.isEmpty()) {
                continue;
            }

            float oldX = 0.0F;
            float oldY = 0.0F;
            float newX = 0.0F;
            float newY = 0.0F;

            for (int pointIndex = 0; pointIndex < linePoints.size(); ++pointIndex) {
                if (pointIndex == 0) {
                    oldX = room->x();
                    oldY = room->y();
                    newX = static_cast<float>(linePoints[0].x());
                    newY = static_cast<float>(linePoints[0].y());
                } else {
                    oldX = newX;
                    oldY = newY;
                    newX = static_cast<float>(linePoints[pointIndex].x());
                    newY = static_cast<float>(linePoints[pointIndex].y());
                }

                if (mMapWidget.mCustomLineSelectedRoom != 0) {
                    if (std::fabs(mx - newX) <= 0.25F && std::fabs(my - newY) <= 0.25F) {
                        mMapWidget.mCustomLineSelectedPoint = pointIndex;
                        return true;
                    }
                }

                const QLineF lineSegment(oldX, oldY, newX, newY);
                const QLineF normalVector = lineSegment.normalVector();

                QLineF testLine1;
                testLine1.setP1(clickPoint);
                testLine1.setAngle(normalVector.angle());
                testLine1.setLength(0.1);

                QLineF testLine2;
                testLine2.setP1(clickPoint);
                testLine2.setAngle(normalVector.angle());
                testLine2.setLength(-0.1);

                QPointF intersectionPoint;
                const auto intersection1 = lineSegment.intersects(testLine1, &intersectionPoint);
                const auto intersection2 = lineSegment.intersects(testLine2, &intersectionPoint);
                if (intersection1 == QLineF::BoundedIntersection || intersection2 == QLineF::BoundedIntersection) {
                    mMapWidget.mCustomLineSelectedRoom = room->getId();
                    mMapWidget.mCustomLineSelectedExit = lineIterator.key();
                    mMapWidget.mCustomLineSelectedPoint = -1;
                    mMapWidget.repaint();
                    return true;
                }
            }
        }
    }

    resetSelection();
    return false;
}

bool CustomLineEditHandler::handleMouseMove(T2DMap::MapInteractionContext& context)
{
    if (!(context.buttons & Qt::LeftButton)) {
        return false;
    }

    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        mMapWidget.mCustomLineSelectedPoint = -1;
        return false;
    }

    TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(mMapWidget.mCustomLineSelectedRoom);
    if (!room) {
        mMapWidget.mCustomLineSelectedPoint = -1;
        return false;
    }

    if (!room->customLines.contains(mMapWidget.mCustomLineSelectedExit)) {
        mMapWidget.mCustomLineSelectedPoint = -1;
        return false;
    }

    QList<QPointF>& points = room->customLines[mMapWidget.mCustomLineSelectedExit];
    if (mMapWidget.mCustomLineSelectedPoint < 0 || mMapWidget.mCustomLineSelectedPoint >= points.size()) {
        mMapWidget.mCustomLineSelectedPoint = -1;
        return false;
    }

    points[mMapWidget.mCustomLineSelectedPoint] = context.mapPoint;
    room->calcRoomDimensions();
    mMapWidget.repaint();
    mMapWidget.mpMap->setUnsaved("CustomLineEditHandler::handleMouseMove");

    return true;
}

bool CustomLineEditHandler::handleMouseRelease()
{
    return false;
}

void CustomLineEditHandler::resetSelection()
{
    mMapWidget.mCustomLineSelectedRoom = 0;
    mMapWidget.mCustomLineSelectedExit.clear();
    mMapWidget.mCustomLineSelectedPoint = -1;
}
