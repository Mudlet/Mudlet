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

#include "RoomMoveActivationHandler.h"

#include "pre_guard.h"
#include <QMouseEvent>
#include <QRect>
#include "post_guard.h"

RoomMoveActivationHandler::RoomMoveActivationHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomMoveActivationHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress: {
        if (context.button != Qt::LeftButton) {
            return false;
        }

        if (context.isMapViewOnly) {
            return false;
        }

        if (!context.multiSelectionSet || context.multiSelectionSet->isEmpty()) {
            return false;
        }

        if (!context.area) {
            return false;
        }

        if (context.modifiers.testFlag(Qt::ShiftModifier) || context.modifiers.testFlag(Qt::ControlModifier)
            || context.modifiers.testFlag(Qt::AltModifier)) {
            return false;
        }

        const auto clickedRoomId = mMapWidget.roomIdAtWidgetPosition(context.widgetPosition, context.area);
        return clickedRoomId.has_value() && context.multiSelectionSet->contains(clickedRoomId.value());
    }
    case QEvent::MouseButtonRelease:
        return context.button == Qt::LeftButton && context.isRoomBeingMoved;
    default:
        return false;
    }
}

bool RoomMoveActivationHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress: {
        if (!context.area) {
            return false;
        }

        const auto clickedRoomId = mMapWidget.roomIdAtWidgetPosition(context.widgetPosition, context.area);
        if (!clickedRoomId.has_value()) {
            return false;
        }

        mMapWidget.mPopupMenu = false;
        mMapWidget.mPick = false;
        mMapWidget.mRoomBeingMoved = true;
        mMapWidget.mMultiRect = QRect(0, 0, 0, 0);
        mMapWidget.mNewMoveAction = false;
        mMapWidget.setMouseTracking(true);

        if (mMapWidget.mMultiSelectionSet.contains(clickedRoomId.value())) {
            mMapWidget.mMultiSelectionHighlightRoomId = clickedRoomId.value();
        }

        context.isRoomBeingMoved = true;

        return true;
    }
    case QEvent::MouseButtonRelease:
        mMapWidget.mPopupMenu = false;
        mMapWidget.setMouseTracking(false);
        mMapWidget.mRoomBeingMoved = false;
        mMapWidget.mMultiRect = QRect(0, 0, 0, 0);
        context.isRoomBeingMoved = false;

        return true;
    default:
        return false;
    }
}
