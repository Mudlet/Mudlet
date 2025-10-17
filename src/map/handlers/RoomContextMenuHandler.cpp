#include "map/handlers/RoomContextMenuHandler.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "utils.h"

#include "pre_guard.h"
#include <QAction>
#include <QEvent>
#include <QMap>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>
#include <QStringList>
#include <QSignalMapper>
#include "post_guard.h"

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

    auto* roomDatabase = mMapWidget.mpMap->mpRoomDB;
    if (!roomDatabase || roomDatabase->isEmpty()) {
        // No map loaded
        //: 2D Mapper context menu (no map found) item
        auto createMap = new QAction(T2DMap::tr("Create new map"), &mMapWidget);
        QObject::connect(createMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_newMap);
        //: 2D Mapper context menu (no map found) item
        auto loadMap = new QAction(T2DMap::tr("Load map"), &mMapWidget);
        QObject::connect(loadMap, &QAction::triggered, &mMapWidget, &T2DMap::slot_loadMap);

        popup->addAction(createMap);
        popup->addAction(loadMap);

        mMapWidget.mPopupMenu = true;
        popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));
        mMapWidget.update();

        return true;
    }

    mMapWidget.prepareSingleClickSelection(context);
    const int selectionSize = context.multiSelectionSet ? context.multiSelectionSet->size() : 0;

    if (!context.isMapViewOnly) {
        populateEditModeActions(popup, selectionSize, context.area);
    }

    populateViewModeActions(popup, selectionSize);

    popup->addSeparator();

    const QString viewModeItem = context.isMapViewOnly ?
        //: 2D Mapper context menu (room) item
        T2DMap::tr("Switch to editing mode") :
        //: 2D Mapper context menu (room) item
        T2DMap::tr("Switch to viewing mode");
    auto setMapViewOnly = new QAction(viewModeItem, &mMapWidget);
    QObject::connect(setMapViewOnly, &QAction::triggered, &mMapWidget, &T2DMap::slot_toggleMapViewOnly);
    popup->addAction(setMapViewOnly);

    populateUserMenus(popup);

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
        mMapWidget.mpCreateRoomAction = new QAction(T2DMap::tr("Create new room here"), &mMapWidget);
        QObject::connect(mMapWidget.mpCreateRoomAction.data(), &QAction::triggered, &mMapWidget, &T2DMap::slot_createRoom);
        menu->addAction(mMapWidget.mpCreateRoomAction);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto moveRoom = new QAction(T2DMap::tr("Move"), &mMapWidget);
        QObject::connect(moveRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_moveRoom);
        menu->addAction(moveRoom);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto roomProperties = new QAction(T2DMap::tr("Configure room..."), &mMapWidget);
        //: 2D Mapper context menu (room) item tooltip
        roomProperties->setToolTip(utils::richText(T2DMap::tr("Set room's name and color of icon, weight and lock for speed walks, and a symbol to mark special rooms")));
        QObject::connect(roomProperties, &QAction::triggered, &mMapWidget, &T2DMap::slot_showPropertiesDialog);
        menu->addAction(roomProperties);
    }

    if (selectionSize == 1) {
        //: 2D Mapper context menu (room) item
        auto roomExits = new QAction(T2DMap::tr("Set exits..."), &mMapWidget);
        QObject::connect(roomExits, &QAction::triggered, &mMapWidget, &T2DMap::slot_setExits);
        menu->addAction(roomExits);
    }

    if (selectionSize == 1) {
        //: 2D Mapper context menu (room) item
        auto customExitLine = new QAction(T2DMap::tr("Create exit line..."), &mMapWidget);
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
        auto spreadRooms = new QAction(T2DMap::tr("Spread..."), &mMapWidget);
        //: 2D Mapper context menu (room) item tooltip
        spreadRooms->setToolTip(utils::richText(T2DMap::tr("Increase map X-Y spacing for the selected group of rooms")));
        QObject::connect(spreadRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_spread);
        menu->addAction(spreadRooms);
    }

    if (selectionSize > 1) {
        //: 2D Mapper context menu (room) item
        auto shrinkRooms = new QAction(T2DMap::tr("Shrink..."), &mMapWidget);
        //: 2D Mapper context menu (room) item tooltip
        shrinkRooms->setToolTip(utils::richText(T2DMap::tr("Decrease map X-Y spacing for the selected group of rooms")));
        QObject::connect(shrinkRooms, &QAction::triggered, &mMapWidget, &T2DMap::slot_shrink);
        menu->addAction(shrinkRooms);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto deleteRoom = new QAction(T2DMap::tr("Delete"), &mMapWidget);
        QObject::connect(deleteRoom, &QAction::triggered, &mMapWidget, &T2DMap::slot_deleteRoom);
        menu->addAction(deleteRoom);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto moveRoomXY = new QAction(T2DMap::tr("Move to position..."), &mMapWidget);
        //: 2D Mapper context menu (room) item tooltip
        moveRoomXY->setToolTip(utils::richText(T2DMap::tr("Move selected room or group of rooms to the given coordinates in this area")));
        QObject::connect(moveRoomXY, &QAction::triggered, &mMapWidget, &T2DMap::slot_movePosition);
        menu->addAction(moveRoomXY);
    }

    if (selectionSize > 0) {
        //: 2D Mapper context menu (room) item
        auto roomArea = new QAction(T2DMap::tr("Move to area..."), &mMapWidget);
        QObject::connect(roomArea, &QAction::triggered, &mMapWidget, &T2DMap::slot_setArea);
        menu->addAction(roomArea);
    }

    //: 2D Mapper context menu (room) item
    auto createLabel = new QAction(T2DMap::tr("Create label..."), &mMapWidget);
    //: 2D Mapper context menu (room) item tooltip
    createLabel->setToolTip(utils::richText(T2DMap::tr("Create label to show text or an image")));
    QObject::connect(createLabel, &QAction::triggered, &mMapWidget, &T2DMap::slot_createLabel);
    menu->addAction(createLabel);

    //: 2D Mapper context menu (area) item
    auto exportAreaImage = new QAction(T2DMap::tr("Export area to image..."), &mMapWidget);
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
        auto setPlayerLocation = new QAction(T2DMap::tr("Set player location"), &mMapWidget);
        //: 2D Mapper context menu (room) item tooltip (enabled state)
        setPlayerLocation->setToolTip(utils::richText(T2DMap::tr("Set the player's current location to here")));
        QObject::connect(setPlayerLocation, &QAction::triggered, &mMapWidget, &T2DMap::slot_setPlayerLocation);
        menu->addAction(setPlayerLocation);
    }
}

void RoomContextMenuHandler::populateUserMenus(QMenu* menu) const
{
    if (!menu) {
        return;
    }

    // this is placed at the end since it is likely someone will want to hook anywhere
    QMap<QString, QMenu*> userMenus;
    QMapIterator<QString, QStringList> it(mMapWidget.mUserMenus);
    while (it.hasNext()) {
        it.next();
        QStringList menuInfo = it.value();
        const QString displayName = menuInfo[1];
        // Need to give the top-level context menu as the parent so the
        // sub-menus get destroyed at the right time:
        auto userMenu = new QMenu(displayName, menu);
        userMenus.insert(it.key(), userMenu);
    }

    it.toFront();
    while (it.hasNext()) {
        //take care of nested menus now since they're all made
        it.next();
        QStringList menuInfo = it.value();
        const QString menuParent = menuInfo[0];
        if (menuParent.isEmpty()) { //parentless
            menu->addMenu(userMenus[it.key()]);
        } else if (userMenus.contains(menuParent)) { //has a parent
            userMenus[menuParent]->addMenu(userMenus[it.key()]);
        }
    }

    //add our actions
    QMapIterator<QString, QStringList> it2(mMapWidget.mUserActions);
    auto mapper = new QSignalMapper(menu);

    while (it2.hasNext()) {
        it2.next();
        QStringList actionInfo = it2.value();
        const QString menuParentKey = actionInfo.value(1);
        auto action = new QAction(actionInfo.value(2), menu);
        if (menuParentKey.isEmpty()) { //no parent
            menu->addAction(action);
        } else if (auto parentMenu = userMenus.value(menuParentKey, nullptr)) { //has a parent
            parentMenu->addAction(action);
        } else {
            delete action;
            continue;
        }
        mapper->setMapping(action, it2.key());
        // TODO: QSignalMapper is not completely compatible with the functor
        // style of QObject::connect(...) - it has been declared obsolete
        // and should be replaced with lambda functions to perform what the
        // slot method did...
        QObject::connect(action, &QAction::triggered, mapper, qOverload<>(&QSignalMapper::map));
    }
    // In relation to above "TODO" in the meantime we can handle things
    // by a change to one of a group of newer signals with a specific
    // signature:
    QObject::connect(mapper, &QSignalMapper::mappedString, &mMapWidget, &T2DMap::slot_userAction);
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
