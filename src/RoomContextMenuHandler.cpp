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

#include "LabelInteractionHandler.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "utils.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>

#include <algorithm>

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

    auto* popup = createMenu(&mMapWidget);
    if (!populateNoMapActions(popup)) {
        mMapWidget.prepareSingleClickSelection(context);

        if (applyClickedRoom(context.hasClickedRoom ? context.clickedRoomId : 0)) {
            mMapWidget.updateSelectionWidget();
        }
        context.multiSelectionSet = &mMapWidget.mMultiSelectionSet;
        context.hasMultiSelection = !mMapWidget.mMultiSelectionSet.isEmpty();

        populateRoomActions(popup, context.area, true);
        if (context.area && !context.isMapViewOnly) {
            populateBelowRoomsLabelActions(popup, *context.area, context.widgetPosition);
        }
    }

    showPopup(popup, context);
    return true;
}

void RoomContextMenuHandler::showPopup(QMenu* popup, const T2DMap::MapInteractionContext& context) const
{
    // Scripts can hide every item; a menu with nothing in it would pop up as an empty sliver.
    const auto actions = popup->actions();
    const bool hasItems = std::any_of(actions.cbegin(), actions.cend(), [](const QAction* action) {
        return !action->isSeparator();
    });
    if (!hasItems) {
        delete popup;
        mMapWidget.update();
        return;
    }

    mMapWidget.mPopupMenu = true;
    popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
    mMapWidget.update();
}

QMenu* RoomContextMenuHandler::menuForOtherView(const int roomId, QWidget* parent)
{
    if (!mMapWidget.mpMap || mMapWidget.mDialogLock) {
        return nullptr;
    }

    auto* popup = createMenu(parent);
    if (!populateNoMapActions(popup)) {
        if (applyClickedRoom(roomId)) {
            mMapWidget.updateSelectionWidget();
            mMapWidget.update();
        }
        TArea* area = nullptr;
        if (auto* room = mMapWidget.mpMap->mpRoomDB->getRoom(roomId)) {
            area = mMapWidget.mpMap->mpRoomDB->getArea(room->getArea());
        }
        populateRoomActions(popup, area, false);
    }
    return popup;
}

QMenu* RoomContextMenuHandler::createMenu(QWidget* parent) const
{
    auto* popup = new QMenu(parent);
    popup->setToolTipsVisible(true);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    mMapWidget.registerContextMenu(popup, parent);
    return popup;
}

bool RoomContextMenuHandler::populateNoMapActions(QMenu* menu) const
{
    TMap* pMap = mMapWidget.mpMap;
    auto* roomDatabase = pMap->mpRoomDB.get();
    if (roomDatabase && !roomDatabase->isEmpty()) {
        return false;
    }

    if (!pMap->getMmpMapLocation().isEmpty() && pMap->menuItemShown(qsl("downloadMap"))) {
        //: 2D Mapper context menu (no map found) item. Downloads the shared map offered by the game server via MMP.
        auto downloadMap = new QAction(T2DMap::tr("Download from game"), menu);
        QObject::connect(downloadMap, &QAction::triggered, &mMapWidget, [pMap]() {
            if (pMap) {
                pMap->downloadMap();
            }
        });
        menu->addAction(downloadMap);
    }

    if (pMap->menuItemShown(qsl("loadMap"))) {
        //: 2D Mapper context menu (no map found) item
        auto loadMap = new QAction(T2DMap::tr("Load map..."), menu);
        QObject::connect(loadMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_loadMap);
        menu->addAction(loadMap);
    }

    if (pMap->menuItemShown(qsl("createMap"))) {
        //: 2D Mapper context menu (no map found) item
        auto createMap = new QAction(T2DMap::tr("Create new map"), menu);
        QObject::connect(createMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_newMap);
        menu->addAction(createMap);
    }
    return true;
}

// Returns whether the selection changed
bool RoomContextMenuHandler::applyClickedRoom(const int clickedRoomId)
{
    // Deliberately nothing for a right-click that hits no room: it leaves the
    // selection alone, whatever its size. Mappers select a room (or box-select
    // one from a stack) and then right-click on empty space to edit it, because
    // hitting the room itself is hard when zoomed out (#9915).
    if (clickedRoomId <= 0) {
        return false;
    }

    if (mMapWidget.mMultiSelectionSet.contains(clickedRoomId)) {
        if (mMapWidget.mMultiSelectionHighlightRoomId != clickedRoomId) {
            const bool isHighlightMissing = !mMapWidget.mMultiSelectionSet.contains(mMapWidget.mMultiSelectionHighlightRoomId);
            if (mMapWidget.mMultiSelectionSet.size() <= 1 || isHighlightMissing) {
                mMapWidget.mMultiSelectionHighlightRoomId = clickedRoomId;
            }
        }
        return false;
    }

    mMapWidget.clearSelection();
    mMapWidget.mMultiSelectionSet.clear();
    mMapWidget.mMultiSelectionSet.insert(clickedRoomId);
    mMapWidget.mMultiSelectionHighlightRoomId = clickedRoomId;
    return true;
}

void RoomContextMenuHandler::populateRoomActions(QMenu* menu, TArea* area, const bool includePointerActions) const
{
    const int selectionSize = mMapWidget.mMultiSelectionSet.size();

    if (!mMapWidget.mMapViewOnly) {
        populateEditModeActions(menu, selectionSize, area, includePointerActions);
    }

    populateViewModeActions(menu, selectionSize);

    menu->addSeparator();

    if (mMapWidget.mpMap->menuItemShown(qsl("switchMapMode"))) {
        const QString viewModeItem = mMapWidget.mMapViewOnly ?
                                                             //: 2D Mapper context menu (room) item
                                             T2DMap::tr("Switch to editing mode")
                                                             :
                                                             //: 2D Mapper context menu (room) item
                                             T2DMap::tr("Switch to viewing mode");
        auto setMapViewOnly = new QAction(viewModeItem, menu);
        QObject::connect(setMapViewOnly, &QAction::triggered, &mMapWidget, &T2DMap::slot_toggleMapViewOnly);
        menu->addAction(setMapViewOnly);
    }

    mMapWidget.populateUserContextMenus(*menu);
}

void RoomContextMenuHandler::populateEditModeActions(QMenu* menu, int selectionSize, TArea* area, const bool includePointerActions) const
{
    if (!menu) {
        return;
    }

    const auto shown = [this](const QString& itemName) {
        return mMapWidget.mpMap->menuItemShown(itemName);
    };

    if (selectionSize == 0 && includePointerActions && shown(qsl("createRoom"))) {
        const auto [x, y] = mMapWidget.getMousePosition();
        mMapWidget.mContextMenuClickPosition = {x, y};
        //: Menu option to create a new room in the mapper
        auto createRoom = new QAction(T2DMap::tr("Create new room here"), menu);
        QObject::connect(createRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_createRoom);
        menu->addAction(createRoom);
    }

    if (selectionSize > 0 && includePointerActions && shown(qsl("moveRoom"))) {
        //: 2D Mapper context menu (room) item
        auto moveRoom = new QAction(T2DMap::tr("Move"), menu);
        QObject::connect(moveRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_moveRoom);
        menu->addAction(moveRoom);
    }

    if (selectionSize > 0 && shown(qsl("configureRoom"))) {
        //: 2D Mapper context menu (room) item
        auto roomProperties = new QAction(T2DMap::tr("Configure room..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        roomProperties->setToolTip(utils::richText(T2DMap::tr("Set room's name and color of icon, weight and lock for speed walks, and a symbol to mark special rooms")));
        QObject::connect(roomProperties, &QAction::triggered, &mMapWidget, &T2DMap::slot_showPropertiesDialog);
        menu->addAction(roomProperties);
    }

    if (selectionSize == 1 && shown(qsl("setExits"))) {
        //: 2D Mapper context menu (room) item
        auto roomExits = new QAction(T2DMap::tr("Set exits..."), menu);
        QObject::connect(roomExits, &QAction::triggered, &mMapWidget, &T2DMap::slot_setExits);
        menu->addAction(roomExits);
    }

    if (selectionSize == 1 && includePointerActions && shown(qsl("createExitLine"))) {
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

    if (selectionSize > 1 && shown(qsl("spreadRooms"))) {
        //: 2D Mapper context menu (room) item
        auto spreadRooms = new QAction(T2DMap::tr("Spread..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        spreadRooms->setToolTip(utils::richText(T2DMap::tr("Increase map X-Y spacing for the selected group of rooms")));
        QObject::connect(spreadRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_spread);
        menu->addAction(spreadRooms);
    }

    if (selectionSize > 1 && shown(qsl("shrinkRooms"))) {
        //: 2D Mapper context menu (room) item
        auto shrinkRooms = new QAction(T2DMap::tr("Shrink..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        shrinkRooms->setToolTip(utils::richText(T2DMap::tr("Decrease map X-Y spacing for the selected group of rooms")));
        QObject::connect(shrinkRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_shrink);
        menu->addAction(shrinkRooms);
    }

    if (selectionSize > 0 && shown(qsl("deleteRoom"))) {
        //: 2D Mapper context menu (room) item
        auto deleteRoom = new QAction(T2DMap::tr("Delete"), menu);
        QObject::connect(deleteRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_deleteRoom);
        menu->addAction(deleteRoom);
    }

    if (selectionSize > 0 && shown(qsl("moveToPosition"))) {
        //: 2D Mapper context menu (room) item
        auto moveRoomXY = new QAction(T2DMap::tr("Move to position..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        moveRoomXY->setToolTip(utils::richText(T2DMap::tr("Move selected room or group of rooms to the given coordinates in this area")));
        QObject::connect(moveRoomXY, &QAction::triggered, &mMapWidget, &T2DMap::slot_movePosition);
        menu->addAction(moveRoomXY);
    }

    if (selectionSize > 0 && shown(qsl("moveToArea"))) {
        //: 2D Mapper context menu (room) item
        auto roomArea = new QAction(T2DMap::tr("Move to area..."), menu);
        QObject::connect(roomArea, &QAction::triggered, &mMapWidget, &T2DMap::slot_setArea);
        menu->addAction(roomArea);
    }

    if (shown(qsl("configureAreas"))) {
        //: 2D Mapper context menu (room) item
        auto configureAreas = new QAction(T2DMap::tr("Configure areas..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        configureAreas->setToolTip(utils::richText(T2DMap::tr("Modify and create new areas.")));
        QObject::connect(configureAreas, &QAction::triggered, &mMapWidget, &T2DMap::slot_configureAreas);
        menu->addAction(configureAreas);
    }

    if (!includePointerActions) {
        return;
    }

    if (shown(qsl("createLabel"))) {
        //: 2D Mapper context menu (room) item
        auto createLabel = new QAction(T2DMap::tr("Create label..."), menu);
        //: 2D Mapper context menu (room) item tooltip
        createLabel->setToolTip(utils::richText(T2DMap::tr("Create label to show text or an image")));
        QObject::connect(createLabel, &QAction::triggered, &mMapWidget, &T2DMap::slot_createLabel);
        menu->addAction(createLabel);
    }

    if (shown(qsl("exportAreaImage"))) {
        //: 2D Mapper context menu (area) item
        auto exportAreaImage = new QAction(T2DMap::tr("Export area to image..."), menu);
        //: 2D Mapper context menu (area) item tooltip
        exportAreaImage->setToolTip(utils::richText(T2DMap::tr("Export the current area as an image file")));
        QObject::connect(exportAreaImage, &QAction::triggered, &mMapWidget, &T2DMap::slot_exportAreaToImage);
        menu->addAction(exportAreaImage);
    }
}

void RoomContextMenuHandler::populateViewModeActions(QMenu* menu, int selectionSize) const
{
    if (!menu) {
        return;
    }

    if (selectionSize == 1 && mMapWidget.mpMap->menuItemShown(qsl("setPlayerLocation"))) {
        //: 2D Mapper context menu (room) item
        auto setPlayerLocation = new QAction(T2DMap::tr("Set player location"), menu);
        //: 2D Mapper context menu (room) item tooltip (enabled state)
        setPlayerLocation->setToolTip(utils::richText(T2DMap::tr("Set the player's current location to here")));
        QObject::connect(setPlayerLocation, &QAction::triggered, &mMapWidget, &T2DMap::slot_setPlayerLocation);
        menu->addAction(setPlayerLocation);
    }
}

// A room takes clicks from a label drawn below it, so a label lying wholly under rooms is only reachable from here
void RoomContextMenuHandler::populateBelowRoomsLabelActions(QMenu* menu, const TArea& area, const QPoint& widgetPosition) const
{
    const QList<int> labelIds = LabelInteractionHandler::belowRoomsLabelsAt(mMapWidget, area, widgetPosition);
    if (labelIds.isEmpty()) {
        return;
    }

    menu->addSeparator();
    if (labelIds.size() == 1) {
        //: 2D Mapper context menu (room) item, for the label drawn under the room that was clicked
        const QString moveText = T2DMap::tr("Move label");
        //: 2D Mapper context menu (room) item, for the label drawn under the room that was clicked
        const QString deleteText = T2DMap::tr("Delete label");
        addLabelActions(menu, labelIds.first(), moveText, deleteText);
        return;
    }

    for (const int labelId : labelIds) {
        QString text = area.mMapLabels.value(labelId).text.simplified();
        if (text.size() > 30) {
            text = text.left(29) + QChar(0x2026);
        }
        QString title;
        if (text.isEmpty()) {
            //: 2D Mapper context menu (room) submenu for an image label drawn under the room that was clicked; %1 is the label's id
            title = T2DMap::tr("Image label %1").arg(labelId);
        } else {
            //: 2D Mapper context menu (room) submenu for a text label drawn under the room that was clicked; %1 is the label's text
            title = T2DMap::tr("Label: %1").arg(text);
        }
        auto* labelMenu = menu->addMenu(title.replace(QLatin1Char('&'), qsl("&&")));
        //: 2D Mapper context menu (label) item
        const QString moveText = T2DMap::tr("Move");
        //: 2D Mapper context menu (label) item
        const QString deleteText = T2DMap::tr("Delete");
        addLabelActions(labelMenu, labelId, moveText, deleteText);
    }
}

void RoomContextMenuHandler::addLabelActions(QMenu* menu, const int labelId, const QString& moveText, const QString& deleteText) const
{
    const int areaId = mMapWidget.mAreaID;
    const auto pickLabel = [this, areaId, labelId]() {
        auto* pArea = mMapWidget.mpMap->mpRoomDB->getArea(areaId);
        if (mMapWidget.mAreaID != areaId || !pArea || !pArea->mMapLabels.contains(labelId)) {
            return false;
        }
        for (auto&& [key, label] : pArea->mMapLabels.asKeyValueRange()) {
            label.highlight = key == labelId;
        }
        mMapWidget.mLabelHighlighted = true;
        return true;
    };

    auto* moveLabel = new QAction(moveText, menu);
    QObject::connect(moveLabel, &QAction::triggered, &mMapWidget, [this, pickLabel]() {
        if (pickLabel()) {
            mMapWidget.slot_moveLabel();
        }
    });
    menu->addAction(moveLabel);

    auto* deleteLabel = new QAction(deleteText, menu);
    QObject::connect(deleteLabel, &QAction::triggered, &mMapWidget, [this, pickLabel]() {
        if (pickLabel()) {
            mMapWidget.slot_deleteLabel();
            mMapWidget.mLabelHighlighted = false;
        }
    });
    menu->addAction(deleteLabel);
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
