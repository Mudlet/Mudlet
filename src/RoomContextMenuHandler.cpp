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

#include "RoomContextMenuHandler.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "utils.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>

RoomContextMenuHandler::RoomContextMenuHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool RoomContextMenuHandler::matches(const T2DMap::MapInteractionContext& context) const
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

    if (context.isLabelHighlighted || context.isCustomLineDrawing) {
        return false;
    }

    if (hasCustomLineSelection(context)) {
        return false;
    }

    return true;
}

bool RoomContextMenuHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap) {
        return false;
    }

    if (context.isDialogLocked) {
        return true;
    }

    auto* popup = new QMenu(&mMapWidget);
    popup->setToolTipsVisible(true);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    mMapWidget.registerContextMenu(popup);

    auto* roomDatabase = mMapWidget.mpMap->mpRoomDB.get();
    if (!roomDatabase || roomDatabase->isEmpty()) {
        // No map loaded
        if (!mMapWidget.mpMap->getMmpMapLocation().isEmpty()) {
            //: 2D Mapper context menu (no map found) item. Downloads the shared map offered by the game server via MMP.
            auto downloadMap = new QAction(T2DMap::tr("Download from game"), popup);
            TMap* pMap = mMapWidget.mpMap;
            QObject::connect(downloadMap, &QAction::triggered, &mMapWidget, [pMap]() {
                if (pMap) {
                    pMap->downloadMap();
                }
            });
            popup->addAction(downloadMap);
        }

        //: 2D Mapper context menu (no map found) item
        auto createMap = new QAction(T2DMap::tr("Create new map"), popup);
        QObject::connect(createMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_newMap);
        //: 2D Mapper context menu (no map found) item
        auto loadMap = new QAction(T2DMap::tr("Load map..."), popup);
        QObject::connect(loadMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_loadMap);

        popup->addAction(loadMap);
        popup->addAction(createMap);

        mMapWidget.mPopupMenu = true;
        popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
        mMapWidget.update();

        return true;
    }

    mMapWidget.prepareSingleClickSelection(context);

    bool selectionChanged = false;

    const int previousSelectionSize = mMapWidget.mMultiSelectionSet.size();

    if (context.hasClickedRoom) {
        const bool isRoomAlreadySelected = mMapWidget.mMultiSelectionSet.contains(context.clickedRoomId);

        if (isRoomAlreadySelected) {
            if (mMapWidget.mMultiSelectionHighlightRoomId != context.clickedRoomId) {
                const bool isHighlightMissing = !mMapWidget.mMultiSelectionSet.contains(mMapWidget.mMultiSelectionHighlightRoomId);
                if (previousSelectionSize <= 1 || isHighlightMissing) {
                    mMapWidget.mMultiSelectionHighlightRoomId = context.clickedRoomId;
                }
            }
        } else {
            mMapWidget.clearSelection();
            mMapWidget.mMultiSelectionSet.clear();
            mMapWidget.mMultiSelectionSet.insert(context.clickedRoomId);
            mMapWidget.mMultiSelectionHighlightRoomId = context.clickedRoomId;
            selectionChanged = true;
        }
    }
    // Deliberately no else branch: a right-click that hits no room leaves the
    // selection alone, whatever its size. Mappers select a room (or box-select
    // one from a stack) and then right-click on empty space to edit it, because
    // hitting the room itself is hard when zoomed out (#9915).

    context.multiSelectionSet = &mMapWidget.mMultiSelectionSet;
    context.hasMultiSelection = !mMapWidget.mMultiSelectionSet.isEmpty();

    if (selectionChanged) {
        mMapWidget.updateSelectionWidget();
    }

    const int selectionSize = mMapWidget.mMultiSelectionSet.size();

    if (!context.isMapViewOnly) {
        populateEditModeActions(popup, selectionSize, context.area);
    }

    populateViewModeActions(popup, selectionSize);

    popup->addSeparator();

    const QString viewModeItem = context.isMapViewOnly ?
                                                       //: 2D Mapper context menu (room) item
                                         T2DMap::tr("Switch to editing mode")
                                                       :
                                                       //: 2D Mapper context menu (room) item
                                         T2DMap::tr("Switch to viewing mode");
    auto setMapViewOnly = new QAction(viewModeItem, popup);
    QObject::connect(setMapViewOnly, &QAction::triggered, &mMapWidget, &T2DMap::slot_toggleMapViewOnly);
    popup->addAction(setMapViewOnly);

    mMapWidget.populateUserContextMenus(*popup);

    mMapWidget.mPopupMenu = true;
    popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
    mMapWidget.update();

    return true;
}

void RoomContextMenuHandler::populateEditModeActions(QMenu* menu, int selectionSize, TArea* area) const
{
    if (!menu) {
        return;
    }

    if (selectionSize == 0) {
        const auto [x, y] = mMapWidget.getMousePosition();
        mMapWidget.mContextMenuClickPosition = {x, y};
        //: Menu option to create a new room in the mapper
        auto createRoom = new QAction(T2DMap::tr("Create new room here"), menu);
        QObject::connect(createRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_createRoom);
        menu->addAction(createRoom);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto moveRoom = new QAction(T2DMap::tr("Move"), menu);
        QObject::connect(moveRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_moveRoom);
        menu->addAction(moveRoom);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto roomProperties = new QAction(T2DMap::tr("Configure room..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        roomProperties->setToolTip(utils::richText(T2DMap::tr("Set room's name and color of icon, weight and lock for speed walks, and a symbol to mark special rooms")));
        QObject::connect(roomProperties, &QAction::triggered, &mMapWidget, &T2DMap::slot_showPropertiesDialog);
        menu->addAction(roomProperties);
    }

    if (selectionSize == 1) {
        //: 2D Mapper context menu (room) item
        auto roomExits = new QAction(T2DMap::tr("Set exits..."), menu);
        QObject::connect(roomExits, &QAction::triggered, &mMapWidget, &T2DMap::slot_setExits);
        menu->addAction(roomExits);
    }

    if (selectionSize == 1) {
        //: 2D Mapper context menu (room) item
        auto customExitLine = new QAction(T2DMap::tr("Create exit line..."), menu);
        if (area && !area->gridMode) {
            //: 2D Mapper context menu (room) item tooltip (enabled state)
            customExitLine->setToolTip(utils::richText(T2DMap::tr("Replace an exit line with a custom line")));
            QObject::connect(customExitLine, &QAction::triggered, &mMapWidget, &T2DMap::slot_setCustomLine);
        } else {
            // Disable custom exit lines in grid mode as they aren't visible anyway
            //: 2D Mapper context menu (room) item tooltip (disabled state)
            customExitLine->setToolTip(utils::richText(T2DMap::tr("Custom exit lines are not shown and are not editable in grid mode")));
            customExitLine->setEnabled(false);
        }
        menu->addAction(customExitLine);
    }

    if (selectionSize > 1) {
        //: 2D Mapper context menu (room) item
        auto spreadRooms = new QAction(T2DMap::tr("Spread..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        spreadRooms->setToolTip(utils::richText(T2DMap::tr("Increase map X-Y spacing for the selected group of rooms")));
        QObject::connect(spreadRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_spread);
        menu->addAction(spreadRooms);
    }

    if (selectionSize > 1) {
        //: 2D Mapper context menu (room) item
        auto shrinkRooms = new QAction(T2DMap::tr("Shrink..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        shrinkRooms->setToolTip(utils::richText(T2DMap::tr("Decrease map X-Y spacing for the selected group of rooms")));
        QObject::connect(shrinkRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_shrink);
        menu->addAction(shrinkRooms);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto deleteRoom = new QAction(T2DMap::tr("Delete"), menu);
        QObject::connect(deleteRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_deleteRoom);
        menu->addAction(deleteRoom);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto moveRoomXY = new QAction(T2DMap::tr("Move to position..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        moveRoomXY->setToolTip(utils::richText(T2DMap::tr("Move selected room or group of rooms to the given coordinates in this area")));
        QObject::connect(moveRoomXY, &QAction::triggered, &mMapWidget, &T2DMap::slot_movePosition);
        menu->addAction(moveRoomXY);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto roomArea = new QAction(T2DMap::tr("Move to area..."), menu);
        QObject::connect(roomArea, &QAction::triggered, &mMapWidget, &T2DMap::slot_setArea);
        menu->addAction(roomArea);
    }

    //: 2D Mapper context menu (room) item
    auto configureAreas = new QAction(T2DMap::tr("Configure areas..."), menu);
    //: 2D Mapper context menu (room) item tooltip
    configureAreas->setToolTip(utils::richText(T2DMap::tr("Modify and create new areas.")));
    QObject::connect(configureAreas, &QAction::triggered, &mMapWidget, &T2DMap::slot_configureAreas);
    menu->addAction(configureAreas);

    //: 2D Mapper context menu (room) item
    auto createLabel = new QAction(T2DMap::tr("Create label..."), menu);
    //: 2D Mapper context menu (room) item tooltip
    createLabel->setToolTip(utils::richText(T2DMap::tr("Create label to show text or an image")));
    QObject::connect(createLabel, &QAction::triggered, &mMapWidget, &T2DMap::slot_createLabel);
    menu->addAction(createLabel);

    //: 2D Mapper context menu (area) item
    auto exportAreaImage = new QAction(T2DMap::tr("Export area to image..."), menu);
    //: 2D Mapper context menu (area) item tooltip
    exportAreaImage->setToolTip(utils::richText(T2DMap::tr("Export the current area as an image file")));
    QObject::connect(exportAreaImage, &QAction::triggered, &mMapWidget, &T2DMap::slot_exportAreaToImage);
    menu->addAction(exportAreaImage);
}

void RoomContextMenuHandler::populateViewModeActions(QMenu* menu, int selectionSize) const
{
    if (!menu) {
        return;
    }

    if (selectionSize == 1) {
        //: 2D Mapper context menu (room) item
        auto setPlayerLocation = new QAction(T2DMap::tr("Set player location"), menu);
        //: 2D Mapper context menu (room) item tooltip (enabled state)
        setPlayerLocation->setToolTip(utils::richText(T2DMap::tr("Set the player's current location to here")));
        QObject::connect(setPlayerLocation, &QAction::triggered, &mMapWidget, &T2DMap::slot_setPlayerLocation);
        menu->addAction(setPlayerLocation);
    }
}

bool RoomContextMenuHandler::hasCustomLineSelection(const T2DMap::MapInteractionContext& context) const
{
    if (context.customLineSelectedRoom != 0) {
        return true;
    }

    if (!context.customLineSelectedExit.isEmpty()) {
        return true;
    }

    if (context.customLineSelectedPoint >= 0) {
        return true;
    }

    return false;
}
