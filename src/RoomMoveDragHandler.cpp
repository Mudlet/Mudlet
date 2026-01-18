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

#include "RoomMoveDragHandler.h"

#include "TArea.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QMouseEvent>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QtGlobal>

RoomMoveDragHandler::RoomMoveDragHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomMoveDragHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event) {
        return false;
    }

    if (!context.isRoomBeingMoved || context.isSizingLabel || !context.hasMultiSelection) {
        return false;
    }

    if (!context.multiSelectionSet || context.multiSelectionSet->isEmpty()) {
        return false;
    }

    if (!context.buttons.testFlag(Qt::LeftButton) && !mMapWidget.mRoomMoveViaContextMenu) {
        return false;
    }

    return context.event->type() == QEvent::MouseMove;
}

bool RoomMoveDragHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event || context.event->type() != QEvent::MouseMove) {
        return false;
    }

    if (!context.isRoomBeingMoved || context.isSizingLabel || !context.multiSelectionSet || context.multiSelectionSet->isEmpty()) {
        return false;
    }

    if (!context.buttons.testFlag(Qt::LeftButton) && !mMapWidget.mRoomMoveViaContextMenu) {
        return false;
    }

    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    mMapWidget.mMultiRect = QRect(0, 0, 0, 0);

    auto* roomDb = mMapWidget.mpMap->mpRoomDB;
    if (!roomDb->getRoom(mMapWidget.mRoomID)) {
        return false;
    }

    if (!roomDb->getArea(mMapWidget.mAreaID)) {
        return false;
    }

    if (!mMapWidget.mHasRoomMoveLastMapPoint) {
        mMapWidget.mRoomMoveLastMapPoint = QPointF(qRound(context.mapX), qRound(context.mapY));
        mMapWidget.mHasRoomMoveLastMapPoint = true;
    }

    const qreal deltaX = context.mapPoint.x() - mMapWidget.mRoomMoveLastMapPoint.x();
    const qreal deltaY = context.mapPoint.y() - mMapWidget.mRoomMoveLastMapPoint.y();

    const int dx = qRound(deltaX);
    const int dy = qRound(deltaY);

    if (!dx && !dy) {
        return true;
    }

    mMapWidget.mRoomMoveLastMapPoint.setX(mMapWidget.mRoomMoveLastMapPoint.x() + dx);
    mMapWidget.mRoomMoveLastMapPoint.setY(mMapWidget.mRoomMoveLastMapPoint.y() + dy);

    QSet<int> dirtyAreas;
    QSetIterator<int> roomIterator = mMapWidget.mMultiSelectionSet;
    while (roomIterator.hasNext()) {
        TRoom* room = roomDb->getRoom(roomIterator.next());
        if (!room) {
            continue;
        }

        room->offset(dx, dy, 0);
        dirtyAreas.insert(room->getArea());

        QMapIterator<QString, QList<QPointF>> customLineIterator(room->customLines);
        QMap<QString, QList<QPointF>> updatedLines;
        while (customLineIterator.hasNext()) {
            customLineIterator.next();
            QList<QPointF> points = customLineIterator.value();
            for (auto& point : points) {
                const QPointF originalPoint = point;
                point.setX(static_cast<float>(originalPoint.x() + dx));
                point.setY(static_cast<float>(originalPoint.y() + dy));
            }
            updatedLines.insert(customLineIterator.key(), points);
        }
        room->customLines = updatedLines;
        room->calcRoomDimensions();
    }

    QSetIterator<int> areaIterator(dirtyAreas);
    while (areaIterator.hasNext()) {
        if (auto* area = roomDb->getArea(areaIterator.next())) {
            area->calcSpan();
        }
    }

    mMapWidget.repaint();
    mMapWidget.mpMap->setUnsaved(__func__);

    return true;
}
