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

    if (!mMapWidget.getCenterSelection()) {
        return false;
    }

    TRoom* referenceRoom = roomDb->getRoom(mMapWidget.mMultiSelectionHighlightRoomId);
    if (!referenceRoom) {
        return false;
    }

    const int dx = qRound(context.mapX) - referenceRoom->x();
    const int dy = qRound(context.mapY) - referenceRoom->y();

    QSetIterator<int> roomIterator = mMapWidget.mMultiSelectionSet;
    while (roomIterator.hasNext()) {
        TRoom* room = roomDb->getRoom(roomIterator.next());
        if (!room) {
            continue;
        }

        room->offset(dx, dy, 0);

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

    mMapWidget.repaint();
    mMapWidget.mpMap->setUnsaved(__func__);

    return true;
}
