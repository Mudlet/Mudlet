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

#include "CustomLineDrawContextMenuHandler.h"

#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "utils.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>

CustomLineDrawContextMenuHandler::CustomLineDrawContextMenuHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool CustomLineDrawContextMenuHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    if (context.event->type() != QEvent::MouseButtonRelease) {
        return false;
    }

    if (context.button != Qt::RightButton) {
        return false;
    }

    if (!context.isCustomLineDrawing || context.customLinesRoomFrom <= 0) {
        return false;
    }

    return true;
}

bool CustomLineDrawContextMenuHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    if (context.isDialogLocked) {
        return true;
    }

    TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(context.customLinesRoomFrom);
    if (!room) {
        return false;
    }

    auto popup = new QMenu(&mMapWidget);
    popup->setToolTipsVisible(true);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    mMapWidget.registerContextMenu(popup);

    //: 2D Mapper context menu (drawing custom exit line) item
    auto customLineUndoLastPoint = new QAction(T2DMap::tr("Undo"), &mMapWidget);
    //: 2D Mapper context menu (drawing custom exit line) item tooltip
    customLineUndoLastPoint->setToolTip(T2DMap::tr("Undo last point"));
    if (room->customLines.value(context.customLinesRoomExit).count() > 1) {
        QObject::connect(customLineUndoLastPoint, &QAction::triggered, &mMapWidget, &T2DMap::slot_undoCustomLineLastPoint);
    } else {
        customLineUndoLastPoint->setEnabled(false);
    }

    //: 2D Mapper context menu (drawing custom exit line) item
    auto snapToGrid = new QAction(T2DMap::tr("Snap points to grid"), &mMapWidget);
    snapToGrid->setCheckable(true);
    snapToGrid->setChecked(mMapWidget.isSnapCustomLinePointsToGridEnabled());
    QObject::connect(snapToGrid, &QAction::toggled, &mMapWidget, &T2DMap::slot_setSnapCustomLinePointsToGrid);
    //: 2D Mapper context menu (drawing custom exit line) item tooltip
    snapToGrid->setToolTip(utils::richText(T2DMap::tr("Snap current points and keep custom line edits aligned to the map grid")));

    //: 2D Mapper context menu (drawing custom exit line) item
    auto moveLastPoint = new QAction(T2DMap::tr("Move last point to target room"), &mMapWidget);
    if (mMapWidget.canMoveCustomLineLastPointToTargetRoom(*room, context.customLinesRoomExit)) {
        QObject::connect(moveLastPoint, &QAction::triggered, &mMapWidget, &T2DMap::slot_moveCustomLineLastPointToTargetRoom);
        //: 2D Mapper context menu (drawing custom exit line) item tooltip (enabled state)
        moveLastPoint->setToolTip(utils::richText(T2DMap::tr("Snap the final point to the destination room")));
    } else {
        moveLastPoint->setEnabled(false);
        //: 2D Mapper context menu (drawing custom exit line) item tooltip (disabled state)
        moveLastPoint->setToolTip(utils::richText(T2DMap::tr("Select a line with a valid target room and at least one adjustable point")));
    }

    //: 2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)
    auto customLineProperties = new QAction(T2DMap::tr("Properties"), &mMapWidget);
    //: 2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)
    customLineProperties->setText(T2DMap::tr("properties..."));
    //: 2D Mapper context menu (drawing custom exit line) item tooltip
    customLineProperties->setToolTip(utils::richText(T2DMap::tr("Change the properties of this line")));
    QObject::connect(customLineProperties, &QAction::triggered, &mMapWidget, &T2DMap::slot_customLineProperties);

    //: 2D Mapper context menu (drawing custom exit line) item
    auto customLineFinish = new QAction(T2DMap::tr("Finish"), &mMapWidget);
    //: 2D Mapper context menu (drawing custom exit line) item tooltip
    customLineFinish->setToolTip(utils::richText(T2DMap::tr("Finish drawing this line")));
    QObject::connect(customLineFinish, &QAction::triggered, &mMapWidget, &T2DMap::slot_doneCustomLine);

    room->calcRoomDimensions();

    popup->addAction(customLineUndoLastPoint);
    popup->addAction(snapToGrid);
    popup->addAction(moveLastPoint);
    popup->addAction(customLineProperties);
    popup->addAction(customLineFinish);

    mMapWidget.mPopupMenu = true;
    popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
    mMapWidget.update();

    return true;
}
