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

#include "TMap.h"
#include "TRoomDB.h"

#include <QMouseEvent>
#include <QPointF>
#include <QRect>
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

    auto* roomDb = mMapWidget.mpMap->mpRoomDB.get();
    if (!roomDb->getRoom(mMapWidget.mRoomID)) {
        return false;
    }

    const TArea* shownArea = roomDb->getArea(mMapWidget.mAreaID);
    if (!shownArea) {
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

    mMapWidget.moveSelectedRoomsBy(dx, dy);
    mMapWidget.repaint();

    return true;
}
