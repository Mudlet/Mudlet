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

#include <QMouseEvent>
#include <QRect>

RoomMoveActivationHandler::RoomMoveActivationHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomMoveActivationHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event) {
        return false;
    }

    if (!context.isRoomBeingMoved) {
        return false;
    }

    return context.event->type() == QEvent::MouseButtonPress
        && context.button == Qt::LeftButton;
}

bool RoomMoveActivationHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event || !context.isRoomBeingMoved) {
        return false;
    }

    mMapWidget.mPopupMenu = false;
    mMapWidget.mPick = true;
    mMapWidget.setMouseTracking(false);
    mMapWidget.mRoomBeingMoved = false;
    mMapWidget.mMultiRect = QRect(0, 0, 0, 0);

    context.isRoomBeingMoved = false;

    return false;
}
