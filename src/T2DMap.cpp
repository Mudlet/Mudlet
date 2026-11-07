/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2018-2026 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021-2022 by Piotr Wilczynski - delwing@gmail.com       *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
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


#include "T2DMap.h"


#include "Host.h"
#include "MudletApp.h"
#include "TArea.h"
#include "TConsole.h"
#include "TEvent.h"
#include "CustomLineDrawContextMenuHandler.h"
#include "CustomLineDrawHandler.h"
#include "CustomLineEditContextMenuHandler.h"
#include "CustomLineEditHandler.h"
#include "CustomLineSession.h"
#include "ExitAppearance.h"
#include "LabelInteractionHandler.h"
#include "MiddleMousePanHandler.h"
#include "PanInteractionHandler.h"
#include "RoomAppearance.h"
#include "RoomContextMenuHandler.h"
#include "RoomMoveActivationHandler.h"
#include "RoomMoveDragHandler.h"
#include "SelectionRectangleHandler.h"
#include "TMainConsole.h"
#include "TMapViewManager.h"
#include "TRoom.h" // For DIR_XXX defines
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "dlgRoomExits.h"
#include "dlgRoomProperties.h"
#include "mudlet.h"
#include "utils.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif


#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QFutureWatcher>
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
/* The Devuan package for qt6-base-dev (for Qt 6.8.2) - and presumably
 * Debian and Ubuntu are missing the
 * ./include/QtConcurrent/QtConcurrentTask file that contains the single
 * line:
 * #include "qtconcurrenttask.h" // IWYU pragma: export
 *
 * that is present in the Qt on-line installer install (for Qt 6.10.1)
 * - so building Mudlet using the latter succeed whereas it breaks on the
 * GitHub CI runners (qt 6.9.0). So use the less "pretty" direct include.
 * It appears that this is an upstream Qt bug as the MacOS builds (also
 * for Qt 6.9.0) are failing in the same manner. Yet the Windows build
 * using the later Qt 6.11.0 succeed!*/
#include <qtconcurrenttask.h>
#else
#include <QtConcurrentTask>
#endif
#include <QMap>
#include <QMapIterator>
#include <QMenu>
#include <QStandardPaths>
#include <QTimer>
#include <QtEvents>
#include <QtUiTools>
#include <QWidget>
#include <QAbstractItemView>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>

#include <climits>
#include <cmath>

#include <algorithm>
#include <utility>
#include <vector>

#include "mapInfoContributorManager.h"


// qsls cannot be shared so define a common instance to use when
// there are multiple places where they are used within this file:

// replacement parameter supplied at point of use:
const QString& key_plain = qsl("%1");

namespace {
QStringList getMapperAreaNamesSorted(const QMap<int, QString>& areaNamesMap)
{
    QMap<QString, QString> sortedAreaNames;
    QMapIterator<int, QString> itAreaNames(areaNamesMap);
    while (itAreaNames.hasNext()) {
        itAreaNames.next();
        uint deduplicate = 0;
        QString name;
        do {
            name = qsl("%1+%2").arg(itAreaNames.value().toLower(), QString::number(++deduplicate));
        } while (sortedAreaNames.contains(name));
        sortedAreaNames.insert(name, itAreaNames.value());
    }
    return sortedAreaNames.values();
}
} // namespace

const QString& key_n = qsl("n");
const QString& key_ne = qsl("ne");
const QString& key_e = qsl("e");
const QString& key_se = qsl("se");
const QString& key_s = qsl("s");
const QString& key_sw = qsl("sw");
const QString& key_w = qsl("w");
const QString& key_nw = qsl("nw");
const QString& key_up = qsl("up");
const QString& key_down = qsl("down");
const QString& key_in = qsl("in");
const QString& key_out = qsl("out");

const QString& key_icon_line_solid = qsl(":/icons/solid-line.png");
const QString& key_icon_line_dot = qsl(":/icons/dot-line.png");
const QString& key_icon_line_dash = qsl(":/icons/dash-line.png");
const QString& key_icon_line_dashDot = qsl(":/icons/dash-dot-line.png");
const QString& key_icon_line_dashDotDot = qsl(":/icons/dash-dot-dot-line.png");

const QString& key_dialog_ok_apply = qsl("dialog-ok-apply");
const QString& key_dialog_cancel = qsl("dialog-cancel");

void T2DMap::registerInteractionHandler(IInteractionHandler* handler, int priority)
{
    mInteractionDispatcher.registerHandler(handler, priority);
}

std::optional<int> T2DMap::roomIdAtWidgetPosition(const QPoint& widgetPosition, const TArea* area) const
{
    if (!mpMap || !mpMap->mpRoomDB || !area) {
        return std::nullopt;
    }

    const float fx = ((xspan / 2.0f) - mMapCenterX) * mRoomWidth;
    const float fy = ((yspan / 2.0f) - mMapCenterY) * mRoomHeight;

    const int mx = widgetPosition.x();
    const int my = widgetPosition.y();
    const int mz = mMapCenterZ;

    if (area->gridMode) {
        // Grid mode: snap the click to the nearest grid-cell centre and look
        // it up directly.  The pixel-distance check below breaks at extreme
        // zoom-out where rooms are sub-pixel in size, whereas this works at
        // all zoom levels.
        const int worldX = qRound(static_cast<double>(mx - fx) / mRoomWidth);
        const int worldY = qRound(static_cast<double>(fy - my) / mRoomHeight);
        const TAreaGridIndex::RoomIds& cellRooms = area->getGridIndex().roomsAt(mz, worldX, worldY);
        if (!cellRooms.isEmpty()) {
            return *cellRooms.constBegin();
        }
        return std::nullopt;
    }

    // Non-grid mode: use pixel-distance hit test with a minimum 1-pixel radius
    // so that clicks remain functional even when rooms are very small on screen.
    const int halfW = qMax(1, qRound(mRoomWidth * rSize / 2.0));
    const int halfH = qMax(1, qRound(mRoomHeight * rSize / 2.0));

    QSetIterator<int> roomIterator(area->getRoomsForZ(mz));
    while (roomIterator.hasNext()) {
        const int roomId = roomIterator.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }

        const int rx = room->x() * mRoomWidth + fx;
        const int ry = room->y() * -1 * mRoomHeight + fy;

        if (qAbs(mx - rx) < halfW && qAbs(my - ry) < halfH) {
            return roomId;
        }
    }

    return std::nullopt;
}

QSet<int> T2DMap::roomIdsAtWidgetPosition(const QPoint& widgetPosition, const TArea* area) const
{
    QSet<int> result;

    if (!mpMap || !mpMap->mpRoomDB || !area) {
        return result;
    }

    const float fx = ((xspan / 2.0f) - mMapCenterX) * mRoomWidth;
    const float fy = ((yspan / 2.0f) - mMapCenterY) * mRoomHeight;

    const int mx = widgetPosition.x();
    const int my = widgetPosition.y();
    const int mz = mMapCenterZ;

    if (area->gridMode) {
        // Grid mode: look up the exact cell under the cursor.
        const int worldX = qRound(static_cast<double>(mx - fx) / mRoomWidth);
        const int worldY = qRound(static_cast<double>(fy - my) / mRoomHeight);
        const TAreaGridIndex::RoomIds& cellRooms = area->getGridIndex().roomsAt(mz, worldX, worldY);
        return QSet<int>(cellRooms.constBegin(), cellRooms.constEnd());
    }

    const int halfW = qMax(1, qRound(mRoomWidth * rSize / 2.0));
    const int halfH = qMax(1, qRound(mRoomHeight * rSize / 2.0));

    QSetIterator<int> roomIterator(area->getRoomsForZ(mz));
    while (roomIterator.hasNext()) {
        const int roomId = roomIterator.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }

        const int rx = room->x() * mRoomWidth + fx;
        const int ry = room->y() * -1 * mRoomHeight + fy;

        if (qAbs(mx - rx) < halfW && qAbs(my - ry) < halfH) {
            result.insert(roomId);
        }
    }

    return result;
}

void T2DMap::prepareSingleClickSelection(MapInteractionContext& context)
{
    mMultiRect = QRectF(context.widgetPosition, context.widgetPosition);

    context.hasClickedRoom = false;
    context.clickedRoomId = 0;

    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    auto* area = context.area;
    if (!area) {
        return;
    }

    const auto clickedRoomId = roomIdAtWidgetPosition(context.widgetPosition, area);
    context.hasClickedRoom = clickedRoomId.has_value();
    context.clickedRoomId = clickedRoomId.value_or(0);

    if (context.button == Qt::RightButton) {
        return;
    }

    const float fx = ((xspan / 2.0f) - mMapCenterX) * mRoomWidth;
    const float fy = ((yspan / 2.0f) - mMapCenterY) * mRoomHeight;

    const int mx = context.widgetPosition.x();
    const int my = context.widgetPosition.y();
    const int mz = mMapCenterZ;

    // Build the list of rooms under the cursor.  Grid mode uses the spatial
    // index for an exact cell lookup (works at all zoom levels including
    // extreme zoom-out where rooms are sub-pixel sized).  Non-grid mode uses
    // a pixel-distance test with a minimum 1-pixel radius.
    QList<int> hitRooms;
    if (area->gridMode) {
        const int worldX = qRound(static_cast<double>(mx - fx) / mRoomWidth);
        const int worldY = qRound(static_cast<double>(fy - my) / mRoomHeight);
        for (const int roomId : area->getGridIndex().roomsAt(mz, worldX, worldY)) {
            hitRooms.append(roomId);
        }
    } else {
        const int halfW = qMax(1, qRound(mRoomWidth * rSize / 2.0));
        const int halfH = qMax(1, qRound(mRoomHeight * rSize / 2.0));
        QSetIterator<int> roomIterator(area->getRoomsForZ(mz));
        while (roomIterator.hasNext()) {
            const int roomId = roomIterator.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
            if (!room) {
                continue;
            }
            const int rx = room->x() * mRoomWidth + fx;
            const int ry = room->y() * -1 * mRoomHeight + fy;
            if (qAbs(mx - rx) < halfW && qAbs(my - ry) < halfH) {
                hitRooms.append(roomId);
            }
        }
    }

    addClickedRoomsToSelection(hitRooms, context.modifiers);
}

void T2DMap::addClickedRoomsToSelection(const QList<int>& hitRooms, const Qt::KeyboardModifiers modifiers)
{
    const bool hasShift = modifiers.testFlag(Qt::ShiftModifier);
    const bool hasCtrl = modifiers.testFlag(Qt::ControlModifier);
    for (const int roomId : hitRooms) {
        const bool isAlreadySelected = mMultiSelectionSet.contains(roomId);

        if (hasCtrl) {
            if (isAlreadySelected && !hasShift) {
                mMultiSelectionSet.remove(roomId);
            } else {
                mMultiSelectionSet.insert(roomId);
            }
        } else {
            mMultiSelectionSet.insert(roomId);
        }

        if (!mMultiSelectionSet.empty()) {
            mMultiSelection = false;
        }
    }

    const int selectionSize = mMultiSelectionSet.size();
    switch (selectionSize) {
    case 0:
        mMultiSelectionHighlightRoomId = 0;
        break;
    case 1:
        mMultiSelection = false;
        mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.begin());
        break;
    default:
        mMultiSelection = false;
        getCenterSelection();
        break;
    }
}

void T2DMap::selectRoomFromOtherView(const int roomId, const Qt::KeyboardModifiers modifiers)
{
    // As a click on the 2D map: without Ctrl or Shift it starts a new selection
    if (!modifiers.testFlag(Qt::ControlModifier) && !modifiers.testFlag(Qt::ShiftModifier)) {
        mMultiSelectionSet.clear();
    }
    QList<int> hitRooms;
    if (roomId > 0) {
        hitRooms.append(roomId);
    }
    addClickedRoomsToSelection(hitRooms, modifiers);
    updateSelectionWidget();
    update();
}

void T2DMap::beginRectangleSelection(const int roomId, const Qt::KeyboardModifiers modifiers)
{
    // As SelectionRectangleHandler's press: Ctrl toggles rooms in what was selected before, Shift adds to it
    const bool hasShift = modifiers.testFlag(Qt::ShiftModifier);
    const bool hasCtrl = modifiers.testFlag(Qt::ControlModifier);
    mMultiSelectionBaseSet = mMultiSelectionSet;
    if (!hasCtrl && !hasShift) {
        mMultiSelectionSet.clear();
    }
    mMultiSelectionAnchorSet.clear();
    QList<int> hitRooms;
    if (roomId > 0) {
        hitRooms.append(roomId);
    }
    addClickedRoomsToSelection(hitRooms, modifiers);
    if (hasShift) {
        mMultiSelectionAnchorSet = mMultiSelectionSet;
    }
    updateSelectionWidget();
}

void T2DMap::applyRectangleSelection(const QSet<int>& rectangleSelection, const Qt::KeyboardModifiers modifiers)
{
    const QSet<int> previousSelection = mMultiSelectionSet;
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        mMultiSelectionSet = mMultiSelectionAnchorSet;
        mMultiSelectionSet.unite(rectangleSelection);
    } else if (modifiers.testFlag(Qt::ControlModifier)) {
        QSet<int> toggledSelection = mMultiSelectionBaseSet;
        for (const int roomId : rectangleSelection) {
            if (toggledSelection.contains(roomId)) {
                toggledSelection.remove(roomId);
            } else {
                toggledSelection.insert(roomId);
            }
        }
        mMultiSelectionSet = toggledSelection;
    } else {
        mMultiSelectionSet = rectangleSelection;
    }

    switch (mMultiSelectionSet.size()) {
    case 0:
        mMultiSelectionHighlightRoomId = 0;
        break;
    case 1:
        mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.begin());
        break;
    default:
        getCenterSelection();
        break;
    }

    if (mMultiSelectionSet.size() > 1) {
        // Most moves leave the selection as it was, and rebuilding the list
        // for thousands of rooms takes longer than a mouse move's interval
        if (mMultiSelectionSet != previousSelection || mMultiSelectionListWidget.isHidden()) {
            updateSelectionWidget();
        }
    } else {
        mMultiSelectionListWidget.hide();
    }
}

void T2DMap::endRectangleSelection()
{
    mMultiSelection = false;
    mMultiSelectionAnchorSet.clear();
    mMultiSelectionBaseSet.clear();
    mMultiRect = QRectF();
    updateSelectionWidget();
}

void T2DMap::selectRoomsToMove(const QSet<int>& clickedRoomIds, const int roomId)
{
    if (!mMultiSelectionSet.contains(roomId)) {
        mMultiSelectionSet = clickedRoomIds;
        mMultiSelection = false;
    }
    mMultiSelectionHighlightRoomId = roomId;
}

void T2DMap::moveSelectedRoomsBy(const int dx, const int dy)
{
    if ((!dx && !dy) || !mpMap || !mpMap->mpRoomDB) {
        return;
    }
    auto* roomDb = mpMap->mpRoomDB.get();
    // Rebuilding an area's indexes costs a mouse move's interval on a big map, so it only
    // beats updating them room by room once nearly every room in the area is moving
    const TArea* shownArea = roomDb->getArea(mAreaID);
    const bool rebuildIndexes = shownArea && mMultiSelectionSet.size() * 4 > shownArea->getAreaRooms().size() * 3;
    QSet<int> dirtyAreas;
    for (const int roomId : std::as_const(mMultiSelectionSet)) {
        TRoom* room = roomDb->getRoom(roomId);
        if (!room) {
            continue;
        }

        const int oldX = room->x();
        const int oldY = room->y();
        room->offset(dx, dy, 0);
        if (rebuildIndexes) {
            dirtyAreas.insert(room->getArea());
        } else if (auto* area = roomDb->getArea(room->getArea())) {
            area->moveRoom(roomId, room->z(), oldX, oldY, room->z(), room->x(), room->y());
        }
    }

    for (const int areaId : std::as_const(dirtyAreas)) {
        if (auto* area = roomDb->getArea(areaId)) {
            area->calcSpan();
        }
    }

    mpMap->setUnsaved(__func__);
}

void T2DMap::InteractionDispatcher::registerHandler(IInteractionHandler* handler, int priority)
{
    if (!handler) {
        return;
    }

    for (auto iterator = mHandlers.begin(); iterator != mHandlers.end(); ++iterator) {
        if (iterator->handler == handler) {
            iterator = mHandlers.erase(iterator);
            break;
        }
    }

    HandlerEntry entry;
    entry.priority = priority;
    entry.handler = handler;

    auto insertIterator = mHandlers.begin();
    while (insertIterator != mHandlers.end() && insertIterator->priority >= priority) {
        ++insertIterator;
    }

    mHandlers.insert(insertIterator, entry);
}

void T2DMap::registerContextMenu(QMenu* menu, QWidget* clickTarget)
{
    if (!menu) {
        return;
    }

    mActiveContextMenu = menu;
    mContextMenuClickTarget = clickTarget ? clickTarget : this;

    QObject::connect(menu, &QMenu::aboutToHide, this, [this]() {
        mActiveContextMenu.clear();
    });

    QObject::connect(menu, &QObject::destroyed, this, [this]() {
        mActiveContextMenu.clear();
    });
}

bool T2DMap::InteractionDispatcher::dispatch(MapInteractionContext& context) const
{
    for (const auto& entry : mHandlers) {
        if (!entry.handler) {
            continue;
        }

        if (!entry.handler->matches(context)) {
            continue;
        }

        if (entry.handler->handle(context)) {
            return true;
        }
    }

    return false;
}

bool T2DMap::eventFilter(QObject* watched, QEvent* event)
{
    if (mActiveContextMenu && event && event->type() == QEvent::MouseButtonPress) {
        if (auto* activeMenu = mActiveContextMenu.data()) {
            if (watched == activeMenu) {
                return QObject::eventFilter(watched, event);
            }

            if (auto* watchedWidget = qobject_cast<QWidget*>(watched); watchedWidget && activeMenu->isAncestorOf(watchedWidget)) {
                return QObject::eventFilter(watched, event);
            }
        }

        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent) {
            const auto button = mouseEvent->button();
            if (button == Qt::LeftButton || button == Qt::RightButton) {
                const QPoint globalPos = mouseEvent->globalPosition().toPoint();

                // Check if the click is on the menu or any of its visible submenus
                if (auto* activeMenu = mActiveContextMenu.data()) {
                    const QRect menuGlobalGeometry(activeMenu->mapToGlobal(QPoint(0, 0)), activeMenu->size());
                    if (menuGlobalGeometry.contains(globalPos)) {
                        return QObject::eventFilter(watched, event);
                    }

                    for (const auto* action : activeMenu->actions()) {
                        if (auto* subMenu = action->menu(); subMenu && subMenu->isVisible()) {
                            const QRect subMenuGeometry(subMenu->mapToGlobal(QPoint(0, 0)), subMenu->size());
                            if (subMenuGeometry.contains(globalPos)) {
                                return QObject::eventFilter(watched, event);
                            }
                        }
                    }
                }

                // This map is hidden while another view of it, which opened the menu, is shown in its place
                QWidget* target = mContextMenuClickTarget.data();
                const QPoint localPos = target ? target->mapFromGlobal(globalPos) : QPoint();

                if (target && target->rect().contains(localPos)) {
                    auto menu = mActiveContextMenu;
                    mActiveContextMenu.clear();

                    if (menu) {
                        menu->close();
                    }

                    const QPointF localPosF(localPos);
                    const QPointF globalPosF(globalPos);

                    auto* pressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPosF, localPosF, globalPosF, button, button, mouseEvent->modifiers());
                    auto* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, localPosF, localPosF, globalPosF, button, Qt::NoButton, mouseEvent->modifiers());

                    QCoreApplication::postEvent(target, pressEvent);
                    QCoreApplication::postEvent(target, releaseEvent);

                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

const QString& key_icon_dialog_ok_apply = qsl(":/icons/dialog-ok-apply.png");
const QString& key_icon_dialog_cancel = qsl(":/icons/dialog-cancel.png");


T2DMap::T2DMap(QWidget* parent)
: QWidget(parent)
, xyzoom(TMap::scmDefaultXYZoom)
{
    if (auto* app = qApp) {
        // This allows to forward clicks to widget even if popup menu is opened, therefore e.g. one click is enough to close popup and select room
        // No more need to first close popup and then perform click on ob
        app->installEventFilter(this);
    }

    mpRenderThrottleTimer = new QTimer(this);
    mpRenderThrottleTimer->setSingleShot(true);
    connect(mpRenderThrottleTimer, &QTimer::timeout, this, qOverload<>(&T2DMap::update));

    mMultiSelectionListWidget.setParent(this);
    mMultiSelectionListWidget.setColumnCount(2);
    mMultiSelectionListWidget.hideColumn(1);
    QStringList headerLabels;
    headerLabels <<
            //: Room ID in the mapper widget
            tr("ID") <<
            //: Room name in the mapper widget
            tr("Name");
    mMultiSelectionListWidget.setHeaderLabels(headerLabels);
    //: Tooltip for multi-room selection widget in mapper
    mMultiSelectionListWidget.setToolTip(utils::richText(tr("Click to select/deselect rooms. Click headers to sort. "
                                                            "Name column shows only if rooms are named.")));
    mMultiSelectionListWidget.setUniformRowHeights(true);
    mMultiSelectionListWidget.setItemsExpandable(false);
    mMultiSelectionListWidget.setSelectionMode(QAbstractItemView::MultiSelection); // Was ExtendedSelection
    mMultiSelectionListWidget.setRootIsDecorated(false);
    const QSizePolicy multiSelectionSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    mMultiSelectionListWidget.setSizePolicy(multiSelectionSizePolicy);
    mMultiSelectionListWidget.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    mMultiSelectionListWidget.setFrameShape(QFrame::NoFrame);
    mMultiSelectionListWidget.setFrameShadow(QFrame::Plain);
    mMultiSelectionListWidget.header()->setProperty("showSortIndicator", QVariant(true));
    mMultiSelectionListWidget.header()->setSectionsMovable(false);
    mMultiSelectionListWidget.header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mMultiSelectionListWidget.header()->setStretchLastSection(true);
    mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
    mMultiSelectionListWidget.resize(120, 100);
    mMultiSelectionListWidget.move(0, 0);
    mMultiSelectionListWidget.hide();
    connect(&mMultiSelectionListWidget, &QTreeWidget::itemSelectionChanged, this, &T2DMap::slot_roomSelectionChanged);

    mCustomLineSession = std::make_unique<CustomLineSession>(*this);

    // A hands-free pan ends on any other button's press, so this must see every press first.
    // It consumes only middle-button events.
    mMiddleMousePanHandler = std::make_unique<MiddleMousePanHandler>(*this);
    registerInteractionHandler(mMiddleMousePanHandler.get(), 500);

    mCustomLineDrawContextMenuHandler = std::make_unique<CustomLineDrawContextMenuHandler>(*this);
    registerInteractionHandler(mCustomLineDrawContextMenuHandler.get(), 450);

    mCustomLineDrawInteractionHandler = std::make_unique<CustomLineDrawHandler>(*this);
    registerInteractionHandler(mCustomLineDrawInteractionHandler.get(), 400);

    mCustomLineEditContextMenuHandler = std::make_unique<CustomLineEditContextMenuHandler>(*this);
    registerInteractionHandler(mCustomLineEditContextMenuHandler.get(), 360);

    mCustomLineEditInteractionHandler = std::make_unique<CustomLineEditHandler>(*this);
    registerInteractionHandler(mCustomLineEditInteractionHandler.get(), 350);

    mRoomContextMenuHandler = std::make_unique<RoomContextMenuHandler>(*this);
    registerInteractionHandler(mRoomContextMenuHandler.get(), 340);

    mRoomMoveActivationHandler = std::make_unique<RoomMoveActivationHandler>(*this);
    registerInteractionHandler(mRoomMoveActivationHandler.get(), 300);

    mRoomMoveDragHandler = std::make_unique<RoomMoveDragHandler>(*this);
    registerInteractionHandler(mRoomMoveDragHandler.get(), 290);

    mSelectionRectangleInteractionHandler = std::make_unique<SelectionRectangleHandler>(*this);
    registerInteractionHandler(mSelectionRectangleInteractionHandler.get(), 200);

    mLabelInteractionHandler = std::make_unique<LabelInteractionHandler>(*this);
    registerInteractionHandler(mLabelInteractionHandler.get(), 150);

    mPanInteractionHandler = std::make_unique<PanInteractionHandler>(*this);
    registerInteractionHandler(mPanInteractionHandler.get(), 100);
}

T2DMap::~T2DMap()
{
    if (auto* app = qApp) {
        app->removeEventFilter(this);
    }
}

// A dock Qt has no size for takes its content's size hint, and the controls under the
// map alone are far too small for one.
QSize T2DMap::sizeHint() const
{
    return {400, 400};
}

void T2DMap::init()
{
    if (!mpHost || !mpMap) {
        return;
    }

    eSize = mpHost->mLineSize;
    mBSize = mpHost->mRoomBorderSize;
    rSize = mpHost->mRoomSize;
    mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    mShowGrid = mpHost->mMapperShowGrid;
    if (mMapViewOnly != mpHost->mMapViewOnly) {
        // If it was initialised in one state but the stored setting is the
        // opposite then we need to toggle the mode:
        slot_toggleMapViewOnly();
    }
    flushSymbolPixmapCache();
    flushTextLabelPixmapCache();
    mLargeAreaExitArrows = mpHost->getLargeAreaExitArrows();
}

void T2DMap::scheduleRender()
{
    if (!mpRenderThrottleTimer->isActive()) {
        mpRenderThrottleTimer->start(csmRenderThrottleMs);
    }
}

void T2DMap::slot_shiftDown()
{
    mShiftMode = true;
    mMapCenterY--;
    update();
}

// Not used:
//void T2DMap::slot_toggleShiftMode()
//{
//    mShiftMode = !mShiftMode;
//    update();
//}

void T2DMap::slot_shiftUp()
{
    mShiftMode = true;
    mMapCenterY++;
    update();
}

void T2DMap::slot_shiftLeft()
{
    mShiftMode = true;
    mMapCenterX--;
    update();
}

void T2DMap::slot_shiftRight()
{
    mShiftMode = true;
    mMapCenterX++;
    update();
}
void T2DMap::slot_shiftZup()
{
    mShiftMode = true;
    mMapCenterZ++;
    update();
}

void T2DMap::slot_shiftZdown()
{
    mShiftMode = true;
    mMapCenterZ--;
    update();
}

void T2DMap::setCompanion3DView(QOpenGLWidget* view)
{
#if defined(INCLUDE_3DMAPPER)
    mpCompanion3DView = view;
#else
    Q_UNUSED(view)
#endif
}

void T2DMap::set3DViewCenter(const int areaId, const int x, const int y, const int z)
{
#if defined(INCLUDE_3DMAPPER)
    GLWidgetFactory::setViewCenter(mIsSecondaryView ? mpCompanion3DView.data() : mpMap->mpM.data(), areaId, x, y, z);
#else
    Q_UNUSED(areaId)
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(z)
#endif
}

void T2DMap::switchArea(const QString& newAreaName)
{
    Host* pHost = mpHost;
    if (!pHost || !mpMap) {
        return;
    }

    if (mMoveLabel) {
        mMoveLabel = false;
        setMouseTracking(false);
        mLabelHighlighted = false;
        mHelpMsg.clear();
    }

    const int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(playerRoomId);
    int playerAreaID = -2; // Cannot be valid (but -1 can be)!
    if (pPlayerRoom) {
        playerAreaID = pPlayerRoom->getArea();
    }

    QMapIterator<int, QString> it(mpMap->mpRoomDB->getAreaNamesMap());
    TEvent areaViewedChangedEvent{};
    while (it.hasNext()) {
        it.next();
        const int areaID = it.key();
        auto areaName = it.value();
        TArea* area = mpMap->mpRoomDB->getArea(areaID);
        if (area && newAreaName == areaName) {
            if (mAreaID != areaID) {
                // We are changing the viewed area - so change the zoom to the
                // stored zoom value - this way though will reset it to the
                // default if the new area has not previously had a zoom set
                // from the Lua API and some parties have suggested that in
                // this case we should just use the zoom value that we were
                // using in the area we are switching from:
                xyzoom = mpMap->mpRoomDB->get2DMapZoom(areaID);
                areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
                areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
                areaViewedChangedEvent.mArgumentList.append(QString::number(areaID));
                areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
                areaViewedChangedEvent.mArgumentList.append(QString::number(mAreaID));
                areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
                pruneRoomSelectionToArea(areaID);
                mAreaID = areaID;
                mLastViewedAreaID = mAreaID;
            }

            mShiftMode = true;
            area->calcSpan();

            if (mAreaID == playerAreaID) {
                // We are switching back to the area that has the player in it
                // recenter view on that room!
                mMapCenterX = pPlayerRoom->x();
                // Map y coordinates are reversed on 2D map!
                mMapCenterY = -pPlayerRoom->y();
                mMapCenterZ = pPlayerRoom->z();
                xyzoom = mpMap->mpRoomDB->get2DMapZoom(mAreaID);
                repaint();
                set3DViewCenter(mAreaID, mMapCenterX, -mMapCenterY, mMapCenterZ);
                if (!areaViewedChangedEvent.mArgumentList.isEmpty()) {
                    mpHost->raiseEvent(areaViewedChangedEvent);
                }
                // escape early
                return;
            }

            bool validRoomFound = false;
            if (!area->zLevels.contains(mMapCenterZ)) {
                // If the current map z-coordinate value is NOT one that is used
                // for this then get the FIRST room in the area and goto the
                // mathematical midpoint of all the rooms on the same
                // z-coordinate.
                QSetIterator<int> itRoom(area->getAreaRooms());
                // key is z-coordinate, value is count of rooms on that level
                QMap<int, int> roomsCountLevelMap;
                while (itRoom.hasNext()) {
                    const int checkRoomID = itRoom.next();
                    TRoom* room = mpMap->mpRoomDB->getRoom(checkRoomID);
                    if (room) {
                        validRoomFound = true;
                        if (roomsCountLevelMap.contains(room->z())) {
                            ++roomsCountLevelMap[room->z()];
                        } else {
                            roomsCountLevelMap[room->z()] = 1;
                        }
                    }
                }

                if (validRoomFound) {
                    QMapIterator<int, int> itRoomsCount(roomsCountLevelMap);
                    // Start at highest value and work down
                    itRoomsCount.toBack();
                    // This will be Okay as we KNOW there is at least one entry
                    itRoomsCount.previous();
                    int maxRoomCountOnLevel = 0;
                    // Initialisation value, will get overwritten
                    int minLevelWithMaxRoomCount = itRoomsCount.key();
                    // Return to the back so the previous() in the do loop works
                    // correctly
                    itRoomsCount.next();
                    do {
                        itRoomsCount.previous();
                        if (maxRoomCountOnLevel < itRoomsCount.value()) {
                            maxRoomCountOnLevel = itRoomsCount.value();
                            minLevelWithMaxRoomCount = itRoomsCount.key();
                        }
                    } while (itRoomsCount.hasPrevious());

                    // We now have lowest level with the highest number of rooms
                    // Now find the geometry center of the rooms on THAT level
                    // In a similar manner to the getCenterSelection() method
                    float mean_x = 0.0;
                    float mean_y = 0.0;
                    uint processedRoomCount = 0;
                    QSet<TRoom*> roomsToConsider; // Hold on to relevant rooms for
                                                  // following step
                    for (const int roomId : area->getRoomsForZ(minLevelWithMaxRoomCount)) {
                        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
                        if (!room) {
                            continue;
                        }

                        roomsToConsider.insert(room);
                        mean_x += (static_cast<float>(room->x() - mean_x)) / ++processedRoomCount;
                        mean_y += (static_cast<float>(room->y() - mean_y)) / processedRoomCount;
                    }

                    // We now have the position that is the "centre" of the
                    // rooms on this level - just need to find the room nearest
                    // to that:
                    QSetIterator<TRoom*> itpRoom(roomsToConsider);
                    float closestSquareDistance = -1.0;
                    TRoom* closestCenterRoom = nullptr;
                    while (itpRoom.hasNext()) {
                        TRoom* room = itpRoom.next();
                        const QVector2D meanToRoom(static_cast<float>(room->x()) - mean_x, static_cast<float>(room->y()) - mean_y);
                        if (closestSquareDistance < -0.5) {
                            // Test for first time around loop - for initialisation
                            // Don't use an equality to zero test, we are using
                            // floats so need to allow for a little bit of
                            // fuzzyness!
                            closestSquareDistance = meanToRoom.lengthSquared();
                            closestCenterRoom = room;
                        } else {
                            const float currentRoomSquareDistance = meanToRoom.lengthSquared();
                            if (closestSquareDistance > currentRoomSquareDistance) {
                                closestSquareDistance = currentRoomSquareDistance;
                                closestCenterRoom = room;
                            }
                        }
                    }

                    if (closestCenterRoom) {
                        mMapCenterX = closestCenterRoom->x();
                        // Map y coordinates are reversed on 2D map!
                        mMapCenterY = -closestCenterRoom->y();
                        mMapCenterZ = closestCenterRoom->z();
                    } else {
                        mMapCenterX = mMapCenterY = mMapCenterZ = 0;
                    }
                }

                if (!validRoomFound) {
                    //no rooms, go to 0,0,0
                    mMapCenterX = 0;
                    mMapCenterY = 0;
                    mMapCenterZ = 0;
                }
            } else {
                // Else the selected area DOES have rooms on the same
                // z-coordinate. Now find the geometric center of the rooms on
                // the given level, in a similar manner to the
                // getCenterSelection() method
                float mean_x = 0.0;
                float mean_y = 0.0;
                uint processedRoomCount = 0;
                QSet<TRoom*> roomsToConsider; // Hold on to relevant rooms for
                                              // following step
                for (const int roomId : area->getRoomsForZ(mMapCenterZ)) {
                    TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
                    if (!room) {
                        continue;
                    }

                    roomsToConsider.insert(room);
                    mean_x += (static_cast<float>(room->x() - mean_x)) / ++processedRoomCount;
                    mean_y += (static_cast<float>(room->y() - mean_y)) / processedRoomCount;
                }

                // We now have the position that is the "centre" of the
                // rooms on this level - just need to find the room nearest
                // to that:
                QSetIterator<TRoom*> itpRoom(roomsToConsider);
                float closestSquareDistance = -1.0;
                TRoom* closestCenterRoom = nullptr;
                while (itpRoom.hasNext()) {
                    TRoom* room = itpRoom.next();
                    const QVector2D meanToRoom(static_cast<float>(room->x()) - mean_x, static_cast<float>(room->y()) - mean_y);
                    if (closestSquareDistance < -0.5) {
                        // Test for first time around loop - for initialisation
                        // Don't use an equality to zero test, we are using
                        // floats so need to allow for a little bit of
                        // fuzzyness!
                        closestSquareDistance = meanToRoom.lengthSquared();
                        closestCenterRoom = room;
                    } else {
                        const float currentRoomSquareDistance = meanToRoom.lengthSquared();
                        if (closestSquareDistance > currentRoomSquareDistance) {
                            closestSquareDistance = currentRoomSquareDistance;
                            closestCenterRoom = room;
                        }
                    }
                }

                if (closestCenterRoom) {
                    mMapCenterX = closestCenterRoom->x();
                    // Map y coordinates are reversed on 2D map!
                    mMapCenterY = -closestCenterRoom->y();
                }
            }
            xyzoom = mpMap->mpRoomDB->get2DMapZoom(mAreaID);
            repaint();
            set3DViewCenter(mAreaID, mMapCenterX, -mMapCenterY, mMapCenterZ);
            if (!areaViewedChangedEvent.mArgumentList.isEmpty()) {
                mpHost->raiseEvent(areaViewedChangedEvent);
            }
            return;
        }
    }
}

void T2DMap::switchArea(int areaId)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        qWarning() << "T2DMap::switchArea(int) - cannot switch to area" << areaId << "- map or roomDB is null";
        return;
    }

    const QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
    if (areaName.isEmpty()) {
        qWarning() << "T2DMap::switchArea(int) - area" << areaId << "not found in area names map";
        return;
    }
    switchArea(areaName);
}

std::pair<bool, QString> T2DMap::centerview(int roomId)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return {false, qsl("map or roomDB is null")};
    }

    TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return {false, qsl("room %1 not found").arg(roomId)};
    }

    const int areaId = pR->getArea();
    TArea* pArea = mpMap->mpRoomDB->getArea(areaId);
    if (!pArea) {
        return {false, qsl("room %1 references non-existent area %2").arg(roomId).arg(areaId)};
    }

    // For secondary views, don't raise sysMapAreaChanged event
    if (!mIsSecondaryView) {
        TEvent areaViewedChangedEvent{};
        if (mAreaID != areaId) {
            areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            areaViewedChangedEvent.mArgumentList.append(QString::number(areaId));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            areaViewedChangedEvent.mArgumentList.append(QString::number(mAreaID));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        }

        if (!areaViewedChangedEvent.mArgumentList.isEmpty() && mpHost) {
            mpHost->raiseEvent(areaViewedChangedEvent);
        }
    }

    if (mAreaID != areaId) {
        pruneRoomSelectionToArea(areaId);
    }
    mAreaID = areaId;
    mLastViewedAreaID = areaId;
    mRoomID = roomId;
    mMapCenterX = pR->x();
    mMapCenterY = -pR->y(); // Map y coordinates are reversed
    mMapCenterZ = pR->z();
    xyzoom = mpMap->mpRoomDB->get2DMapZoom(areaId);

    isCenterViewCall = true;
    update();
    isCenterViewCall = false;

    return {true, QString()};
}

// Helper that refactors out code to start a speedwalk:
void T2DMap::initiateSpeedWalk(const int speedWalkStartRoomId, const int speedWalkTargetRoomId)
{
    mTargetRoomId = speedWalkTargetRoomId;
    if (mpMap->mpRoomDB->getRoom(speedWalkTargetRoomId)) {
        mpMap->mTargetID = speedWalkTargetRoomId;

        if (mpHost->checkForCustomSpeedwalk()) {
            mpHost->startSpeedWalk(speedWalkStartRoomId, speedWalkTargetRoomId);
        } else if (mpMap->findPath(speedWalkStartRoomId, speedWalkTargetRoomId)) {
            mpHost->startSpeedWalk();
        } else {
            mpHost->mainConsoleView()->printSystemMessage(
                    qsl("%1\n").arg(tr("Mapper: Cannot find a path from %1 to %2 using known exits.").arg(QString::number(speedWalkStartRoomId), QString::number(speedWalkTargetRoomId))));
        }
    }
}

TMapPainter::View T2DMap::painterView() const
{
    TMapPainter::View view;
    view.mpMap = mpMap;
    view.mpHost = mpHost;
    view.mSize = size();
    view.mRoomWidth = mRoomWidth;
    view.mRoomHeight = mRoomHeight;
    view.mRX = mRX;
    view.mRY = mRY;
    view.rSize = rSize;
    view.mBSize = mBSize;
    view.mAreaID = mAreaID;
    view.mMapCenterZ = mMapCenterZ;
    view.mShowRoomID = mShowRoomID;
    view.mMaxRoomIdDigits = mMaxRoomIdDigits;
    view.mBubbleMode = mBubbleMode;
    view.mMapperUseAntiAlias = mMapperUseAntiAlias;
    view.mLargeAreaExitArrows = mLargeAreaExitArrows;
    view.mPHighlight = mPHighlight;
    view.mPick = mPick;
    view.mStartSpeedWalk = mStartSpeedWalk;
    view.mMultiSelectionSet = mMultiSelectionSet;
    view.mCustomLinesRoomTo = mCustomLinesRoomTo;
    view.mCustomLineSelectedRoom = mCustomLineSelectedRoom;
    view.mCustomLineSelectedExit = mCustomLineSelectedExit;
    view.mCustomLineSelectedPoint = mCustomLineSelectedPoint;
    view.mLodExitIndexDisabled = mLodExitIndexDisabled;
    return view;
}

void T2DMap::takePainterResults()
{
    mPick = mPainter.view().mPick;
    mStartSpeedWalk = mPainter.view().mStartSpeedWalk;
    mLodExitIndexRoomsHandedOver = mPainter.lodExitIndexRoomsHandedOver();
}

void T2DMap::addSymbolToPixmapCache(const QString key, const QString text, const QColor symbolColor, const bool gridMode)
{
    mPainter.setView(painterView());
    mPainter.addSymbolToPixmapCache(key, text, symbolColor, gridMode);
}

void T2DMap::drawScaledLabel(QPainter& painter, const QPointF& position, TMapLabel& label, int labelKey, const QRectF& paintRect)
{
    mPainter.setView(painterView());
    mPainter.drawScaledLabel(painter, position, label, labelKey, paintRect);
}

void T2DMap::startPendingSpeedWalk()
{
    if (const auto pendingSpeedWalk = mPainter.takePendingSpeedWalk()) {
        initiateSpeedWalk(pendingSpeedWalk->first, pendingSpeedWalk->second);
    }
}

// Markers are gathered from every drawn room, so hit-test once after the room passes rather than
// from a room loop: below four pixels a room, only the player's room is drawn in full.
void T2DMap::resolveAreaExitClick(QPainter& painter, const QMap<int, QPointF>& areaExitsMap, const int speedWalkStartRoomId)
{
    if (!mPick || !mStartSpeedWalk) {
        return;
    }
    QMapIterator<int, QPointF> it(areaExitsMap);
    while (it.hasNext()) {
        it.next();
        const QPointF roomCenter = it.value();
        const QRectF dr = QRectF(roomCenter.x(), roomCenter.y(), mRoomWidth * rSize, mRoomHeight * rSize);

        // clang-format off
        if (mPHighlight.x() < (dr.x() - mRoomWidth / 3.0)
            || mPHighlight.x() > (dr.x() + mRoomWidth / 3.0)
            || mPHighlight.y() < (dr.y() - mRoomHeight / 3.0)
            || mPHighlight.y() > (dr.y() + mRoomHeight / 3.0)) {
            // clang-format on
            continue;
        }

        mStartSpeedWalk = false;
        // This draws a red circle around the out of area exit that
        // was chosen as the target for the speedwalk, but it is
        // only shown for one paintEvent call and it is not obvious
        // that it is useful, note that there is similar code for a
        // room being clicked on that is WITHIN the area, in drawRoom():
        painter.save();
        const float roomRadius = (0.8 * mRoomWidth) / 2.0;
        QRadialGradient gradient(roomCenter, roomRadius);
        gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
        gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
        gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
        gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
        gradient.setColorAt(0, Qt::white);
        const QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        painter.setBrush(gradient);
        painter.setPen(transparentPen);
        myPath.addEllipse(roomCenter, roomRadius, roomRadius);
        painter.drawPath(myPath);
        painter.restore();

        mPick = false;
        initiateSpeedWalk(speedWalkStartRoomId, it.key());
        return;
    }
}

// Revised to use a QCache to hold QPixmap * to generated images for room symbols
void T2DMap::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    if (!mpMap || mpHost.isNull()) {
        return;
    }
    QElapsedTimer renderTimer;
    renderTimer.start();

    // Single phase timer restarted at each phase boundary.
    QElapsedTimer phaseTimer;
    phaseTimer.start();
    qreal phase1Time = 0, phase2Time = 0, phase3Time = 0, phase4Time = 0, phase5Time = 0, phase6Time = 0, phase7Time = 0;

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    // This is needed so that clicking on an area exit can instigate a
    // speed-walk to the room in the linked to area...
    QMap<int, QPointF> areaExitsMap;

    const float widgetWidth = width();
    const float widgetHeight = height();

    if (widgetWidth < 10 || widgetHeight < 10) {
        return;
    }

    static bool oldBubbleMode = false;
    if (oldBubbleMode != mBubbleMode) {
        // If the round/square room selection has changed this will invalidate
        // all the previously generated pixmaps:
        flushSymbolPixmapCache();
        oldBubbleMode = mBubbleMode;
    }

    mPainter.resetSymbolFontSize();
    mMapSymbolFont = mpMap->mMapSymbolFont;
    mMapSymbolFont.setBold(false);
    mMapSymbolFont.setItalic(false);
    mMapSymbolFont.setUnderline(false);
    mMapSymbolFont.setOverline(false);
    mMapSymbolFont.setStrikeOut(false);

    // the room name's font defaults to the symbol's
    // but may be overridden
    auto mapNameFont = mpMap->mMapSymbolFont;
    const QString fontName = mpMap->mUserData.value(ROOM_UI_NAMEFONT);
    if (!fontName.isEmpty()) {
        QFont font;
        if (font.fromString(fontName)) {
            mapNameFont = font;
        }
    }
    mapNameFont.setBold(false);
    mapNameFont.setItalic(false);
    mapNameFont.setUnderline(false);
    mapNameFont.setOverline(false);
    mapNameFont.setStrikeOut(false);

    QList<ExitToPaint> exitList;
    int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(playerRoomId);

    // try and set the player to a room if we don't have a known location
    if (!pPlayerRoom && !mpMap->mpRoomDB->isEmpty()) {
        const int randomRoom = mpMap->mpRoomDB->getRoomMap().constBegin().key();
        pPlayerRoom = mpMap->mpRoomDB->getRoom(randomRoom);
        playerRoomId = pPlayerRoom->getId();
    }
    // no rooms at all, let's show an information message instead
    if (!pPlayerRoom) {
        if (mSuppressEmptyStateMessage) {
            return;
        }
        painter.save();
        painter.fillRect(0, 0, width(), height(), Qt::transparent);
        auto font(painter.font());
        font.setPointSize(10);
        painter.setFont(font);

        QString message;
        if (mpMap->mpRoomDB) {
            if (mpMap->mpRoomDB->isEmpty()) {
                message = tr("No rooms in the map - load another one, or start mapping from scratch to begin.");
            } else {
                message = tr("You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.", nullptr, mpMap->mpRoomDB->size());
            }
        } else {
            message = tr("You do not have a map yet - load one, or start mapping from scratch to begin.");
        }

        painter.drawText(0, 0, widgetWidth, widgetHeight, Qt::AlignCenter | Qt::TextWordWrap, message);
        painter.restore();
        return;
    }

    phase1Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // This is only a safety check to avoid a probably impossible condition, we
    // have already established that pPlayerRoom is valid so for it to NOT be
    // in a valid area is unlikely:
    if (Q_UNLIKELY(!mpMap->mpRoomDB->getArea(pPlayerRoom->getArea()))) {
        return;
    }

    if (mRoomID != playerRoomId && mShiftMode) {
        // Reset the flag if it is set but current player room (playerRoomId)
        // is not the same as was last used for that (mRoomID):
        mShiftMode = false;
    }

    // We declare this here but only populate it if needed below and then send
    // it at the end of the paintEvent:
    TEvent areaViewedChangedEvent{};

    // Secondary views don't follow the player - they maintain their own independent view
    // Track if we're auto-centering on the player (vs user manually panning)
    const bool centeringOnPlayer = !mIsSecondaryView && ((!mPick && !mShiftMode) || mpMap->mNewMove);

    if (centeringOnPlayer) {
        mShiftMode = true;
        // that's of interest only here because the map editor is here ->
        // map might not be updated, thus I force a map update on centerview()
        // with mNewRoom
        mpMap->mNewMove = false;

        mRoomID = playerRoomId;
        const int playerAreaID = pPlayerRoom->getArea();
        if (mAreaID != playerAreaID) {
            pruneRoomSelectionToArea(playerAreaID);
        }
        mAreaID = playerAreaID;
        if (mLastViewedAreaID != mAreaID) {
            areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            areaViewedChangedEvent.mArgumentList.append(QString::number(mAreaID));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            areaViewedChangedEvent.mArgumentList.append(QString::number(mLastViewedAreaID));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            mLastViewedAreaID = mAreaID;
        }

        mMapCenterX = pPlayerRoom->x();
        mMapCenterY = pPlayerRoom->y() * -1;
        mMapCenterZ = pPlayerRoom->z();
    }

    TArea* pDrawnArea = mpMap->mpRoomDB->getArea(mAreaID);
    if (!pDrawnArea) {
        return;
    }

    phase2Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    xyzoom = pDrawnArea->get2DMapZoom();
    if (widgetWidth > widgetHeight) {
        xspan = xyzoom * (widgetWidth / widgetHeight);
        yspan = xyzoom;
    } else {
        xspan = xyzoom;
        yspan = xyzoom * (widgetHeight / widgetWidth);
    }

    // Center map on area when it fits entirely in viewport, but only when
    // following player movement - not during manual panning or editing.
    // Off by default; opt in via the mapCenterSmallAreas key in Mudlet.ini.
    if (mpHost->mMapperCenterSmallAreas && centeringOnPlayer && !mRoomBeingMoved && !mMultiSelection) {
        const int zLevel = mMapCenterZ;

        // Get area bounds for current Z level (use overall bounds as fallback)
        const qreal areaMinX = pDrawnArea->xminForZ.value(zLevel, pDrawnArea->min_x);
        const qreal areaMaxX = pDrawnArea->xmaxForZ.value(zLevel, pDrawnArea->max_x);
        // Y is inverted in map coordinates
        const qreal areaMinY = -pDrawnArea->ymaxForZ.value(zLevel, pDrawnArea->max_y);
        const qreal areaMaxY = -pDrawnArea->yminForZ.value(zLevel, pDrawnArea->min_y);

        const qreal areaWidth = areaMaxX - areaMinX;
        const qreal areaHeight = areaMaxY - areaMinY;

        if (areaWidth <= xspan && areaHeight <= yspan) {
            mMapCenterX = (areaMinX + areaMaxX) / 2.0;
            mMapCenterY = -((areaMinY + areaMaxY) / 2.0);
        }
    }

    mRoomWidth = widgetWidth / xspan;
    mRoomHeight = widgetHeight / yspan;

    if (!qFuzzyCompare(1.0f + mPrevRoomWidth, 1.0f + mRoomWidth) || !qFuzzyCompare(1.0f + mPrevRoomHeight, 1.0f + mRoomHeight)) {
        flushSymbolPixmapCache();
        flushTextLabelPixmapCache();
        mPrevRoomWidth = mRoomWidth;
        mPrevRoomHeight = mRoomHeight;
    }

    // Not qRound(): it returns int, overflowing before the clamp.
    mRX = TMapPainter::clampedToIntRange(std::round(mRoomWidth * ((xspan / 2.0) - mMapCenterX)));
    mRY = TMapPainter::clampedToIntRange(std::round(mRoomHeight * ((yspan / 2.0) - mMapCenterY)));
    const QRect roomBounds = viewportRoomBounds(mRX, mRY, mRoomWidth, mRoomHeight, widgetWidth, widgetHeight);
    QFont roomVNumFont = mpMap->mMapSymbolFont;

    bool isFontBigEnoughToShowRoomVnum = false;
    if (mShowRoomID) {
        // Find out the number of digits the biggest room Id number in the area
        // needs; also useful for the room selection widget. Rescanning every
        // room here on every single paint made a large area's room-ID display
        // cost more than everything else in the frame put together, so this is
        // only redone when the area or its room membership has actually moved
        // on since the value was last cached.
        if (mCachedRoomIdDigitsVersion != pDrawnArea->getRoomsVersion()) {
            int maxUsedRoomId = 0;
            QSetIterator<int> itRoomId(pDrawnArea->getAreaRooms());
            while (itRoomId.hasNext()) {
                maxUsedRoomId = qMax(maxUsedRoomId, itRoomId.next());
            }
            mMaxRoomIdDigits = static_cast<quint8>(QString::number(maxUsedRoomId).length());
            mCachedRoomIdDigitsVersion = pDrawnArea->getRoomsVersion();
        }

        QRectF roomTestRect;
        if (pDrawnArea->gridMode) {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth), static_cast<qreal>(mRoomHeight));
        } else {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth) * rSize, static_cast<qreal>(mRoomHeight) * rSize);
        }
        static quint8 const roomVnumMargin = 10;
        roomVNumFont.setBold(true);

        // QFont::PreferOutline will help to select a font that will scale to any
        // size - which is important for good rendering over a range of sizes
        // QFont::PreferAntialias will look better - except perhaps at very small
        // sizes (but we prevent that by checking in the method call afterwards):
        roomVNumFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));

        isFontBigEnoughToShowRoomVnum = TMapPainter::sizeFontToFitTextInRect(roomVNumFont, roomTestRect, qsl("8").repeated(mMaxRoomIdDigits), roomVnumMargin);
    }
    mPainter.setView(painterView());

    bool showRoomNames = mpMap->getRoomNamesShown() && !pDrawnArea->gridMode;
    if (showRoomNames) {
        /*
         * Like above, except that we use the room height as the font size.
         */
        mapNameFont.setBold(true);

        mapNameFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));

        const double sizeAdjust = 0;
        mapNameFont.setPointSizeF(static_cast<qreal>(mRoomWidth) * rSize * pow(1.1, sizeAdjust) / 2.0);
        showRoomNames = (mapNameFont.pointSizeF() > 3.0);
    }

    const int zLevel = mMapCenterZ;

    const float exitWidth = 1 / eSize * mRoomWidth * rSize;

    painter.fillRect(0, 0, width(), height(), mpHost->mBgColor_2);

    auto pen = painter.pen();
    pen.setColor(mpHost->mFgColor_2);
    pen.setWidthF(exitWidth);
    painter.setRenderHint(QPainter::Antialiasing, mMapperUseAntiAlias);
    painter.setPen(pen);

    if (mShowGrid && mRoomWidth > 0.0f && mRoomHeight > 0.0f) {
        painter.save();

        QColor gridColor = mpHost->mMapGridColor;

        QPen gridPen(gridColor);
        const qreal gridWidth = static_cast<qreal>(mpHost->mMapGridLineSize) * mRoomWidth / 50.0;
        gridPen.setWidthF(gridWidth);
        gridPen.setCosmetic(mMapperUseAntiAlias);
        painter.setPen(gridPen);

        const qreal visibleMinX = mMapCenterX - xspan / 2.0;
        const qreal visibleMaxX = mMapCenterX + xspan / 2.0;
        const qreal visibleMinY = -((yspan / 2.0) + mMapCenterY);
        const qreal visibleMaxY = (yspan / 2.0) - mMapCenterY;

        const int startX = static_cast<int>(std::floor(visibleMinX));
        const int endX = static_cast<int>(std::ceil(visibleMaxX));
        for (int gridX = startX; gridX <= endX; ++gridX) {
            const qreal widgetX = static_cast<qreal>(gridX) * static_cast<qreal>(mRoomWidth) + static_cast<qreal>(mRX);
            painter.drawLine(QPointF(widgetX, 0.0), QPointF(widgetX, static_cast<qreal>(widgetHeight)));
        }

        const int startY = static_cast<int>(std::floor(visibleMinY));
        const int endY = static_cast<int>(std::ceil(visibleMaxY));
        for (int gridY = startY; gridY <= endY; ++gridY) {
            const qreal widgetY = static_cast<qreal>(gridY) * -static_cast<qreal>(mRoomHeight) + static_cast<qreal>(mRY);
            painter.drawLine(QPointF(0.0, widgetY), QPointF(static_cast<qreal>(widgetWidth), widgetY));
        }

        painter.restore();
    }

    phase3Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw label sizing or group selection box
    if (mSizeLabel) {
        painter.fillRect(mMultiRect, QColor(250, 190, 0, 190));
    } else {
        painter.fillRect(mMultiRect, QColor(190, 190, 190, 60));
    }

    QPointF playerRoomOnWidgetCoordinates;
    bool isPlayerRoomVisible = false;
    // QPoint doesn't work here as the key as it can't be hashed...!
    QSet<QPair<int, int>> usedRoomPositions;

    if (mudlet::self()->mDrawUpperLowerLevels) {
        const TAreaGridIndex& gridIndex = pDrawnArea->getGridIndex();
        // Zoomed out over the whole area, walking the index costs more than the flat room set.
        // The area keeps its y extremes negated, unlike roomBounds.
        const bool viewportHoldsArea =
                roomBounds.left() <= pDrawnArea->min_x && roomBounds.right() >= pDrawnArea->max_x && roomBounds.top() <= -pDrawnArea->max_y && roomBounds.bottom() >= -pDrawnArea->min_y;
        auto neighbouringLevelRooms = [&](const int z) {
            if (viewportHoldsArea) {
                const QSet<int>& rooms = pDrawnArea->getRoomsForZ(z);
                return QList<int>(rooms.cbegin(), rooms.cend());
            }
            return gridIndex.roomsInViewport(z, roomBounds.left(), roomBounds.right(), roomBounds.top(), roomBounds.bottom());
        };
        const QList<int> lowerLevelRooms = neighbouringLevelRooms(zLevel - 1);
        for (const int currentAreaRoom : lowerLevelRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx >= 0 && ry >= 0 && rx <= widgetWidth && ry <= widgetHeight) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(mpHost->mLowerLevelColor);
                if (mBubbleMode) {
                    const float roomRadius = 0.5 * rSize * mRoomWidth;
                    const QPointF roomCenter = QPointF(rx - (roomRadius * rSize * 0.5), ry + (roomRadius * rSize * 0.5));
                    QPainterPath diameterPath;
                    diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                    painter.drawPath(diameterPath);
                } else {
                    painter.drawRect(rx - (mRoomWidth * rSize * 0.8), ry - (mRoomHeight * rSize * 0.2), mRoomWidth * rSize, mRoomHeight * rSize);
                }
                painter.restore();
            }
        }

        const QList<int> upperLevelRooms = neighbouringLevelRooms(zLevel + 1);
        for (const int currentAreaRoom : upperLevelRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx >= 0 && ry >= 0 && rx <= widgetWidth && ry <= widgetHeight) {
                painter.save();
                painter.setPen(QPen(mpHost->mUpperLevelColor, 1));
                painter.setBrush(Qt::transparent);
                if (mBubbleMode) {
                    const float roomRadius = 0.5 * rSize * mRoomWidth;
                    const QPointF roomCenter = QPointF(rx + (roomRadius * rSize * 0.5), ry - (roomRadius * rSize * 0.5));
                    QPainterPath diameterPath;
                    diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                    painter.drawPath(diameterPath);
                } else {
                    painter.drawRect(rx - (mRoomWidth * rSize * 0.2), ry - (mRoomHeight * rSize * 0.8), mRoomWidth * rSize, mRoomHeight * rSize);
                }
                painter.restore();
            }
        }
    }

    // Draw the ("background") labels that are on the bottom of the map:
    mPainter.startTextLabelFrame();
    QMutableMapIterator<int, TMapLabel> itMapLabel(pDrawnArea->mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        TMapLabel& mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != mMapCenterZ) {
            continue;
        }
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * mRoomWidth + mRX;
        const int labelY = mapLabel.pos.y() * mRoomHeight * -1 + mRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * mRoomWidth));
        const int labelHeight = abs(qRound(mapLabel.size.height() * mRoomHeight));
        if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
            continue;
        }

        QRectF labelPaintRectangle = QRect(mapLabel.pos.x() * mRoomWidth + mRX, mapLabel.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
        if (!mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                mPainter.drawScaledLabel(painter, labelPosition, mapLabel, itMapLabel.key(), labelPaintRectangle);
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
        }

        if (mapLabel.highlight) {
            TMapPainter::paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    phase4Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // draw room exits
    qreal timeViewportQuery = 0.0;
    QList<int> viewportRooms;
    if (!pDrawnArea->gridMode) {
        // Shared by the exit and room passes, as it's the costliest part of each. Not for grid mode: it
        // paints no exits and needs the collision flag only the other query returns.
        QElapsedTimer viewportQueryTimer;
        viewportQueryTimer.start();
        viewportRooms = pDrawnArea->getGridIndex().roomsInViewport(zLevel, roomBounds.left(), roomBounds.right(), roomBounds.top(), roomBounds.bottom());
        timeViewportQuery = viewportQueryTimer.nsecsElapsed() / 1000000.0;
        mPainter.paintRoomExits(painter, pen, exitList, pDrawnArea, zLevel, roomBounds, viewportRooms, exitWidth, areaExitsMap);
    }

    phase5Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw rooms on selected z-level - use batch rendering for grid mode
    // Room collection time is printed at the end, keeping qDebug() overhead (large on Windows) out of phase6Time.
    QString roomLoopProfileOutput;
    QElapsedTimer phase6Sub;
    phase6Sub.start();
    // Only the full-detail loop layers its rooms, and only with a highlight in view
    bool layerRooms = false;
    // Gathered before drawing so each room is looked up once
    QList<QPair<TRoom*, QPointF>> drawnRooms;
    bool isPlayerRoomBodyDrawn = false;
    painter.save(); // prevent room-drawing state from leaking into labels/info box
    if (pDrawnArea->gridMode) {
        // Grid mode: use spatial grid index for O(visible cells) rendering
        mPainter.drawGridModeRooms(painter,
                                   pDrawnArea,
                                   zLevel,
                                   playerRoomId,
                                   mRX,
                                   mRY,
                                   mRoomWidth,
                                   mRoomHeight,
                                   widgetWidth,
                                   widgetHeight,
                                   roomVNumFont,
                                   isPlayerRoomVisible,
                                   playerRoomOnWidgetCoordinates,
                                   isPlayerRoomBodyDrawn,
                                   isFontBigEnoughToShowRoomVnum,
                                   &roomLoopProfileOutput);
    } else if (mRoomWidth < TMapPainter::cLodPixelThreshold) {
        mPainter.drawNonGridModeRoomsLod(
                painter, pDrawnArea, zLevel, playerRoomId, viewportRooms, widgetWidth, widgetHeight, isPlayerRoomVisible, playerRoomOnWidgetCoordinates, isPlayerRoomBodyDrawn, &roomLoopProfileOutput);
    } else {
        // Non-grid mode: use full-featured per-room rendering
        drawnRooms.reserve(viewportRooms.size());
        for (const int currentAreaRoom : viewportRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
                continue;
            }

            layerRooms = layerRooms || (room->highlight && !room->isHidden());
            if (playerRoomId == currentAreaRoom) {
                // We defer drawing THIS (the player's room) until the end
                isPlayerRoomVisible = true;
                playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
            } else {
                drawnRooms.append({room, QPointF(rx, ry)});
            }
        }
        for (const auto& [room, roomCenter] : std::as_const(drawnRooms)) {
            const QPair<int, int> roomPos{room->x(), room->y()};
            const bool roomCollision = usedRoomPositions.contains(roomPos);
            usedRoomPositions.insert(roomPos);
            mPainter.drawRoom(painter,
                              roomVNumFont,
                              mapNameFont,
                              pen,
                              room,
                              pDrawnArea->gridMode,
                              isFontBigEnoughToShowRoomVnum,
                              showRoomNames,
                              playerRoomId,
                              static_cast<float>(roomCenter.x()),
                              static_cast<float>(roomCenter.y()),
                              roomCollision,
                              layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole);
        }
    }
    painter.restore(); // end of room-drawing save

    qreal timePhase6RoomsDraw = phase6Sub.nsecsElapsed() / 1000000.0;
    phase6Sub.restart();

    qreal timePhase6DrawRoom = 0.0;
    qreal timePhase6Gradient = 0.0;
    painter.save(); // prevent player-room drawing state from leaking into labels/info box
    if (isPlayerRoomVisible) {
        // Only the full-detail non-grid loop fills usedRoomPositions. The index also counts off-screen and hidden rooms.
        const bool roomCollision = (pDrawnArea->gridMode || mRoomWidth < TMapPainter::cLodPixelThreshold) ? pDrawnArea->getGridIndex().roomsAt(zLevel, pPlayerRoom->x(), pPlayerRoom->y()).size() > 1
                                                                                                          : usedRoomPositions.contains({pPlayerRoom->x(), pPlayerRoom->y()});
        const QList<RoomDrawPass> playerRoomPasses =
                isPlayerRoomBodyDrawn ? QList<RoomDrawPass>{RoomDrawPass::Highlight, RoomDrawPass::Labels} : QList<RoomDrawPass>{layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole};
        for (const RoomDrawPass pass : playerRoomPasses) {
            mPainter.drawRoom(painter,
                              roomVNumFont,
                              mapNameFont,
                              pen,
                              pPlayerRoom,
                              pDrawnArea->gridMode,
                              isFontBigEnoughToShowRoomVnum,
                              showRoomNames,
                              playerRoomId,
                              static_cast<float>(playerRoomOnWidgetCoordinates.x()),
                              static_cast<float>(playerRoomOnWidgetCoordinates.y()),
                              roomCollision,
                              pass);
        }
        if (layerRooms) {
            drawnRooms.append({pPlayerRoom, playerRoomOnWidgetCoordinates});
        }
        timePhase6DrawRoom = phase6Sub.nsecsElapsed() / 1000000.0;
        phase6Sub.restart();
    }
    if (layerRooms) {
        mPainter.drawRoomHighlightsAndLabels(painter, roomVNumFont, mapNameFont, pen, drawnRooms, pDrawnArea->gridMode, isFontBigEnoughToShowRoomVnum, showRoomNames, playerRoomId);
        timePhase6RoomsDraw += phase6Sub.nsecsElapsed() / 1000000.0;
        phase6Sub.restart();
    }
    if (isPlayerRoomVisible) {
        painter.save();
        const QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        // Use the room+border diagonal so 100% fully encompasses the square
        // (including corners), not just touching the midpoints of the sides
        const int borderWidth = qMax(1, static_cast<int>(1.0 / mBSize * mRoomWidth * rSize));
        const double roomVisualSize = static_cast<double>(mRoomWidth) * rSize + 2.0 * borderWidth;
        const double roomRadius = RoomAppearance::playerRoomRadius(mpMap->mPlayerRoomOuterDiameterPercentage, roomVisualSize);
        QRadialGradient gradient(playerRoomOnWidgetCoordinates, roomRadius);
        if (mpHost->mMapStrongHighlight) {
            // Never set, no means to except via XMLImport, as dlgMapper class's
            // slot_toggleStrongHighlight is not wired up to anything
            const QRectF dr = QRectF(playerRoomOnWidgetCoordinates.x() - (static_cast<double>(mRoomWidth) * rSize) / 2.0,
                                     playerRoomOnWidgetCoordinates.y() - (static_cast<double>(mRoomHeight) * rSize) / 2.0,
                                     static_cast<double>(mRoomWidth) * rSize,
                                     static_cast<double>(mRoomHeight) * rSize);
            painter.fillRect(dr, RoomAppearance::strongHighlightFillColor());

            gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
            gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
            gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
            gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
            gradient.setColorAt(0, Qt::white);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        } else {
            gradient.setStops(mPlayerRoomColorGradientStops);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        }
        painter.drawPath(myPath);
        painter.restore();
        timePhase6Gradient = phase6Sub.nsecsElapsed() / 1000000.0;
    }
    painter.restore(); // end of player-room save

    takePainterResults();
    startPendingSpeedWalk();
    resolveAreaExitClick(painter, areaExitsMap, playerRoomId);

    phase6Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw the ("foreground") labels that are on the top of the map:
    itMapLabel.toFront();
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        TMapLabel& mapLabel = itMapLabel.value();

        if (mapLabel.pos.z() != mMapCenterZ) {
            continue;
        }
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * mRoomWidth + mRX;
        const int labelY = mapLabel.pos.y() * mRoomHeight * -1 + mRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * mRoomWidth));
        const int labelHeight = abs(qRound(mapLabel.size.height() * mRoomHeight));

        if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
            continue;
        }
        QRectF labelPaintRectangle = QRect(mapLabel.pos.x() * mRoomWidth + mRX, mapLabel.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
        if (mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                mPainter.drawScaledLabel(painter, labelPosition, mapLabel, itMapLabel.key(), labelPaintRectangle);
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
        }
        if (mapLabel.highlight) {
            TMapPainter::paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    // Draw an indication of the central room of a multi-room selection.
    // Similar code was used to indicate target of custom exit line selected for
    // editing but this could not be done there because gridmode areas don't hit
    // that bit of code and later rooms would overwrite the target...
    if (mMultiSelectionHighlightRoomId > 0 && mMultiSelectionSet.size() > 1) {
        TRoom* pR_multiSelectionHighlight = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
        if (pR_multiSelectionHighlight) {
            const float r_mSx = pR_multiSelectionHighlight->x() * mRoomWidth + mRX;
            const float r_mSy = pR_multiSelectionHighlight->y() * -1 * mRoomHeight + mRY;
            const QPen savePen = painter.pen();
            const QBrush saveBrush = painter.brush();
            const float roomRadius = mRoomWidth * 1.2;
            const float roomDiagonal = mRoomWidth * 1.2;
            const QPointF roomCenter = QPointF(r_mSx, r_mSy);

            QPen yellowPen(QColor(255, 255, 50, 192)); // Quarter opaque yellow pen
            yellowPen.setWidth(mRoomWidth * 0.1);
            QPainterPath myPath;
            painter.setPen(yellowPen);
            painter.setBrush(Qt::NoBrush);
            myPath.addEllipse(roomCenter, roomRadius, roomRadius);
            myPath.addEllipse(roomCenter, roomRadius / 2.0, roomRadius / 2.0);
            myPath.moveTo(r_mSx - roomDiagonal, r_mSy - roomDiagonal);
            myPath.lineTo(r_mSx + roomDiagonal, r_mSy + roomDiagonal);
            myPath.moveTo(r_mSx + roomDiagonal, r_mSy - roomDiagonal);
            myPath.lineTo(r_mSx - roomDiagonal, r_mSy + roomDiagonal);
            painter.drawPath(myPath);
            painter.setPen(savePen);
            painter.setBrush(saveBrush);
        }
    }

    if (mMiddleMousePanHandler) {
        mMiddleMousePanHandler->renderIndicator(painter);
    }

    QColor infoColor;
    if (mpHost->mBgColor_2.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }

    // The info box always shows the player room unless rooms are explicitly
    // selected.  playerRoomId is fresh each frame; mRoomID is only updated when
    // centeringOnPlayer fires and can be stale or zero before first centering.
    int roomID = playerRoomId;
    if (!mMultiSelectionSet.empty() && mMultiSelectionHighlightRoomId > 0 && mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId)) {
        roomID = mMultiSelectionHighlightRoomId;
    }

    int xOffset = 10;
    if (mMultiSelectionListWidget.isVisible()) {
        xOffset += mMultiSelectionListWidget.x() + mMultiSelectionListWidget.rect().width();
    }

    phase7Time = phaseTimer.nsecsElapsed() / 1000000.0;

    if (qEnvironmentVariableIsSet("MUDLET_PROFILE_MAP")) {
        // Accumulate all profiling text into ONE string then emit a single
        // qDebug() call.  On Windows each qDebug() call invokes OutputDebugString
        // which can take 5-30 ms when a debugger is attached; batching to one
        // call reduces that to a fixed per-frame cost of one call.
        QString profileOut;
        QDebug pdbg(&profileOut);
        pdbg.nospace().noquote() << "viewportQuery: " << timeViewportQuery << "ms (phase5, shared with phase6)\n";
        if (!roomLoopProfileOutput.isEmpty()) {
            pdbg << roomLoopProfileOutput << "\n";
        }
        pdbg << "phase6 subs: roomsDraw=" << timePhase6RoomsDraw << " playerRoom=" << timePhase6DrawRoom << " gradient=" << timePhase6Gradient << " playerVisible=" << isPlayerRoomVisible << "\n";
        // Comprehensive infoBox diagnostic: shows every condition paintMapInfo
        // checks so we can see exactly why it produces blank output.
        auto* mgr = mpMap->mMapInfoContributorManager;
        const bool roomIDValid = mpMap->mpRoomDB->getRoom(roomID) != nullptr;
        const bool contribOk = mgr != nullptr && mpHost->mMapInfoContributors.intersects(QSet<QString>(mgr->getContributorKeys().constBegin(), mgr->getContributorKeys().constEnd()));
        pdbg << "infoBox: roomID=" << roomID << " player=" << playerRoomId << " mRoomID=" << mRoomID << " areaID=" << mAreaID << " selectionSize=" << static_cast<int>(mMultiSelectionSet.size())
             << " xOffset=" << xOffset << " listWidgetVisible=" << mMultiSelectionListWidget.isVisible() << " listWidgetGeom=" << mMultiSelectionListWidget.x() << "+"
             << mMultiSelectionListWidget.rect().width() << " widgetWidth=" << width() << " shift=" << mShiftMode << " centerView=" << isCenterViewCall << " playerVisible=" << isPlayerRoomVisible
             << " roomValid=" << roomIDValid << " contribOk=" << contribOk << " painterHasClip=" << painter.hasClipping() << " painterClipRect=" << painter.clipRegion().boundingRect()
             << " fontHeight=" << mFontHeight << "\n"
             << "paintEvent: total=" << renderTimer.elapsed() << " p1=" << phase1Time << " p2=" << phase2Time << " p3=" << phase3Time << " p4=" << phase4Time << " p5=" << phase5Time
             << " p6=" << phase6Time << " p7=" << phase7Time;
        qDebug().noquote() << profileOut;
    }

    dlgMapper::paintMapInfo(renderTimer, painter, mpHost, mpMap, roomID, mAreaID, mMultiSelectionSet.size(), infoColor, xOffset, 20, width(), mFontHeight);

    // The area combobox belongs to the main mapper, which a secondary view may exist without
    if (!mIsSecondaryView && mpMap->mapper()) {
        static bool isAreaWidgetValid = true; // Remember between uses
        QFont _f = mpMap->mapper()->comboBox_showArea->font();
        if (isAreaWidgetValid) {
            if (mAreaID == -1                       // the map being shown is the "default" area
                && !mpMap->getDefaultAreaShown()) { // the area widget is not showing the "default" area

                isAreaWidgetValid = false; // So the widget CANNOT indicate the correct area
                // Set the area widget to indicate the area widget is NOT
                // showing valid text - so make it italic and crossed out
                _f.setItalic(true);
                _f.setUnderline(true);
                _f.setStrikeOut(true);
                _f.setOverline(true);
            }
        } else {
            if (!(mAreaID == -1 && !mpMap->getDefaultAreaShown())) {
                isAreaWidgetValid = true; // So the widget CAN now indicate the correct area
                // Reset to normal
                _f.setItalic(false);
                _f.setUnderline(false);
                _f.setStrikeOut(false);
                _f.setOverline(false);
            }
        }

        mpMap->mapper()->comboBox_showArea->setFont(_f);
    }

    if (!mHelpMsg.isEmpty()) {
        painter.setPen(QColor(255, 155, 50));
        QFont _f = painter.font();
        const QFont _f2 = _f;
        _f.setPointSize(12); // 20 was a little large
        _f.setBold(true);
        painter.setFont(_f);
        const QRect _r = QRect(0, 0, widgetWidth, widgetHeight);
        painter.drawText(_r, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, mHelpMsg);
        // Now draw text centered at bottom, so it does not clash with info window
        painter.setFont(_f2);
    }
    // Save the current map zoom for this area
    pDrawnArea->set2DMapZoom(xyzoom);

    // Check and send view change event if necessary
    if (!areaViewedChangedEvent.mArgumentList.isEmpty()) {
        mpHost->raiseEvent(areaViewedChangedEvent);
    }
}


void T2DMap::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        // No map loaded!
        event->ignore();
        return;
    }
    if (mDialogLock || (event->buttons() != Qt::LeftButton)) {
        event->ignore();
        return;
    }

    mPHighlight = event->pos();
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
    event->accept();
}

void T2DMap::createLabel(QRectF labelRectangle)
{
    mpDlgMapLabel = new dlgMapLabel(this);
    mHelpMsg.clear();

    auto pArea = mpMap->mpRoomDB->getArea(mAreaID);
    if (!pArea) {
        return;
    }
    const int labelId = pArea->createLabelId();

    // The map can be replaced while this non-modal dialog is open, so re-find the area by id rather than
    // keep the pointer, and check the map generation, since a new map reuses area and label ids.
    const int areaId = mAreaID;
    const unsigned int mapGeneration = mpMap->mpRoomDB->mapGeneration();
    connect(mpDlgMapLabel, &dlgMapLabel::updated, this, [this, labelRectangle, labelId, areaId, mapGeneration]() {
        if (mpMap->mpRoomDB->mapGeneration() != mapGeneration) {
            return;
        }
        if (auto pLabelArea = mpMap->mpRoomDB->getArea(areaId)) {
            updateMapLabel(labelRectangle, labelId, pLabelArea);
        }
    });

    connect(mpDlgMapLabel, &dlgMapLabel::rejected, this, [this, labelId, areaId, mapGeneration]() {
        if (mpMap->mpRoomDB->mapGeneration() == mapGeneration) {
            if (auto pLabelArea = mpMap->mpRoomDB->getArea(areaId)) {
                pLabelArea->mMapLabels.remove(labelId);
            }
        }
        update();
    });

    mpDlgMapLabel->show();
    mpDlgMapLabel->raise();
    mpDlgMapLabel->updated();
}

void T2DMap::updateMapLabel(QRectF labelRectangle, int labelId, TArea* pArea)
{
    TMapLabel label;
    QFont font;
    QString imagePath;
    if (mpDlgMapLabel->isTextLabel()) {
        label.text = mpDlgMapLabel->getText();
        label.fgColor = mpDlgMapLabel->getFgColor();
        font = mpDlgMapLabel->getFont();
        label.font = font;
    } else {
        label.text.clear();
        imagePath = mpDlgMapLabel->getImagePath();
    }
    label.bgColor = mpDlgMapLabel->getBgColor();
    label.outlineColor = mpDlgMapLabel->getOutlineColor();
    label.showOnTop = mpDlgMapLabel->isOnTop();
    label.noScaling = mpDlgMapLabel->noScale();

    const QRectF box = labelRectangle.normalized();
    QPixmap pixmap(static_cast<int>(box.width()), static_cast<int>(box.height()));
    pixmap.fill(Qt::transparent);
    QRect drawRectangle = box.toRect();
    drawRectangle.moveTo(0, 0);
    QPainter lp(&pixmap);
    lp.setRenderHint(QPainter::Antialiasing, mMapperUseAntiAlias);
    lp.fillRect(drawRectangle, label.bgColor);
    int labelWidth = drawRectangle.width();
    int labelHeight = drawRectangle.height();

    if (mpDlgMapLabel->isTextLabel()) {
        lp.setFont(font);
        QRectF br;

        if (label.fgColor != label.outlineColor) {
            QPen outlinePen(label.outlineColor);
            outlinePen.setCosmetic(mMapperUseAntiAlias);
            outlinePen.setWidth(1);
            lp.setPen(outlinePen);

            // Draw the outline by offsetting the text slightly in all directions
            lp.drawText(QRect(-1, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignCenter, label.text, &br);
            lp.drawText(QRect(1, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignCenter, label.text, &br);
            lp.drawText(QRect(0, -1, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignCenter, label.text, &br);
            lp.drawText(QRect(0, 1, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignCenter, label.text, &br);
        }

        // Draw the main text on top
        lp.setPen(label.fgColor);
        lp.drawText(QRect(0, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignCenter, label.text, &br);
    } else {
        const QPixmap imagePixmap = QPixmap(imagePath).scaled(drawRectangle.size(), mpDlgMapLabel->stretchImage() ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio);
        auto point = mpDlgMapLabel->stretchImage() ? QPoint(0, 0) : pixmap.rect().center() - imagePixmap.rect().center();
        lp.drawPixmap(point, imagePixmap);
    }

    label.pix = pixmap.copy(drawRectangle);
    const float mx = (box.left() / mRoomWidth) + mMapCenterX - (xspan / 2.0);
    const float my = (yspan / 2.0) - (box.top() / mRoomHeight) - mMapCenterY;

    const float mx2 = (box.right() / mRoomWidth) + mMapCenterX - (xspan / 2.0);
    const float my2 = (yspan / 2.0) - (box.bottom() / mRoomHeight) - mMapCenterY;
    label.pos = QVector3D(mx, my, mMapCenterZ);
    label.size = QRectF(QPointF(mx, my), QPointF(mx2, my2)).normalized().size();

    if (Q_LIKELY(labelId >= 0)) {
        pArea->mMapLabels.insert(labelId, label);
        mpMap->updateArea(mAreaID);
        if (!label.temporary) {
            mpMap->setUnsaved(__func__);
        }
    }
}

void T2DMap::mouseReleaseEvent(QMouseEvent* event)
{
    if (!mpMap) {
        event->ignore();
        return;
    }

    auto context = buildInteractionContext(event);
    mInteractionDispatcher.dispatch(context);
    event->accept();
}

// In a window without the command line the focus would fall to a panel button instead, which keeps arrows and Space
// for itself; the map handles no keys, so they go on to the mapper around it, which hands them to the command line
void T2DMap::takeFocusAwayFromTheCommandLine(const Qt::FocusReason reason)
{
    TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr;
    if (pConsole && window() != pConsole->window()) {
        setFocus(reason);
    }
}

bool T2DMap::event(QEvent* event)
{
    // NOTE: the widget has the focus only in a window without the command line,
    // as it is more comfortable for the user to always have focus on the command
    // line, and it handles no key events itself

    if (event->type() == QEvent::WindowActivate && !window()->focusWidget()) {
        takeFocusAwayFromTheCommandLine(Qt::ActiveWindowFocusReason);
    } else if (event->type() == QEvent::KeyPress) {
        //        auto* ke = static_cast<QKeyEvent*>(event);
        //        if (ke->key() == Qt::Key_Delete ) {
        //            if (mCustomLineSelectedRoom != 0  ) {
        //                if (mpMap->rooms.contains(mCustomLineSelectedRoom)) {
        //                    TRoom * pR = mpMap->rooms[mCustomLineSelectedRoom];
        //                    if (pR->customLines.contains( mCustomLineSelectedExit)) {
        //                        pR->customLines.remove(mCustomLineSelectedExit);
        //                        repaint();
        //                        mCustomLineSelectedRoom = 0;
        //                        mCustomLineSelectedExit = "";
        //                        mCustomLineSelectedPoint = -1;
        //                        return QWidget::event(event);
        //                    }
        //                }
        //            }
        //        }
    } else if (event->type() == QEvent::Resize) { // Tweak the room selection widget to fit
        resizeMultiSelectionWidget();
    }
    return QWidget::event(event);
}

void T2DMap::mousePressEvent(QMouseEvent* event)
{
    if (!mpMap) {
        event->ignore();
        return;
    }
    auto context = buildInteractionContext(event);

    mInteractionDispatcher.dispatch(context);

    mudlet::self()->activateProfile(mpHost);
    mNewMoveAction = true;
    TEvent sysMapWindowMousePressEvent{};
    sysMapWindowMousePressEvent.mArgumentList.append(QLatin1String("sysMapWindowMousePressEvent"));
    sysMapWindowMousePressEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(sysMapWindowMousePressEvent);
    takeFocusAwayFromTheCommandLine(Qt::MouseFocusReason);

    updateSelectionWidget();
    update();
    event->accept();
}

T2DMap::MapInteractionContext T2DMap::buildInteractionContext(QMouseEvent* event)
{
    MapInteractionContext context;
    context.event = event;
    context.isMapViewOnly = mMapViewOnly;
    context.multiSelectionSet = &mMultiSelectionSet;
    context.hasMultiSelection = !mMultiSelectionSet.isEmpty();
    context.isMultiSelectionActive = mMultiSelection;
    context.isSizingLabel = mSizeLabel;
    context.isLabelHighlighted = mLabelHighlighted;
    context.isRoomBeingMoved = mRoomBeingMoved;
    context.isMoveLabelActive = mMoveLabel;
    context.isCustomLineDrawing = mCustomLinesRoomFrom > 0;
    context.isDialogLocked = mDialogLock;
    context.customLinesRoomFrom = mCustomLinesRoomFrom;
    context.customLinesRoomExit = mCustomLinesRoomExit;
    context.customLineSelectedRoom = mCustomLineSelectedRoom;
    context.customLineSelectedExit = mCustomLineSelectedExit;
    context.customLineSelectedPoint = mCustomLineSelectedPoint;
    context.hasClickedRoom = false;
    context.clickedRoomId = 0;

    if (!mpMap || !mpMap->mpRoomDB) {
        return context;
    }

    context.area = mpMap->mpRoomDB->getArea(mAreaID);

    if (!event) {
        return context;
    }

    context.buttons = event->buttons();
    context.button = event->button();
    context.modifiers = event->modifiers();
    context.widgetPosition = event->pos();
    context.widgetPositionF = event->position();

    const qreal roomWidth = static_cast<qreal>(mRoomWidth);
    const qreal roomHeight = static_cast<qreal>(mRoomHeight);
    const qreal halfXSpan = static_cast<qreal>(xspan) / 2.0;
    const qreal halfYSpan = static_cast<qreal>(yspan) / 2.0;

    if (!qFuzzyIsNull(roomWidth)) {
        context.mapX = static_cast<qreal>(context.widgetPosition.x()) / roomWidth + mMapCenterX - halfXSpan;
    }

    if (!qFuzzyIsNull(roomHeight)) {
        context.mapY = halfYSpan - static_cast<qreal>(context.widgetPosition.y()) / roomHeight - mMapCenterY;
    }

    context.mapPoint = QPointF(context.mapX, context.mapY);

    return context;
}

void T2DMap::updateSelectionWidget()
{
    // display room selection list widget if more than 1 room has been selected
    // -> user can manually change current selection if rooms are overlapping
    if (mMultiSelectionSet.size() > 1) {
        // We don't want to cause calls to slot_roomSelectionChanged() here!
        mMultiSelectionListWidget.blockSignals(true);
        mIsSelectionSorting = mMultiSelectionListWidget.isSortingEnabled();
        mIsSelectionSortByNames = (mMultiSelectionListWidget.sortColumn() == 1);
        mMultiSelectionListWidget.clear();
        // The rows are rebuilt from mMultiSelectionSet, which deleted rooms have already left
        mRoomsToDropFromSelectionWidget.clear();
        // Do NOT sort while inserting items!
        mMultiSelectionListWidget.setSortingEnabled(false);
        QSetIterator<int> itRoom = mMultiSelectionSet;
        mIsSelectionUsingNames = false;
        while (itRoom.hasNext()) {
            auto _item = new QTreeWidgetItem;
            const int multiSelectionRoomId = itRoom.next();
            _item->setText(0, key_plain.arg(multiSelectionRoomId, mMaxRoomIdDigits));
            _item->setTextAlignment(0, Qt::AlignRight);
            TRoom* pR_multiSelection = mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
            if (pR_multiSelection) {
                const QString multiSelectionRoomName = pR_multiSelection->name;
                if (!multiSelectionRoomName.isEmpty()) {
                    _item->setText(1, multiSelectionRoomName);
                    _item->setTextAlignment(1, Qt::AlignLeft);
                    mIsSelectionUsingNames = true;
                }
            }
            mMultiSelectionListWidget.addTopLevelItem(_item);
        }
        mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames);
        // Can't sort if nothing to sort on, switch to sorting by room number
        if ((!mIsSelectionUsingNames) && mIsSelectionSortByNames && mIsSelectionSorting) {
            mIsSelectionSortByNames = false;
        }
        mMultiSelectionListWidget.sortByColumn(mIsSelectionSortByNames ? 1 : 0, Qt::AscendingOrder);
        mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
        resizeMultiSelectionWidget();
        mMultiSelectionListWidget.selectAll();
        mMultiSelectionListWidget.blockSignals(false);
        mMultiSelectionListWidget.show();
    } else {
        mMultiSelectionListWidget.hide();
    }
    update();
}

// returns the current mouse position as X, Y coordinates on the map
std::pair<int, int> T2DMap::getMousePosition()
{
    const QPoint mousePosition = this->mapFromGlobal(QCursor::pos());

    const float mx = (mousePosition.x() / mRoomWidth) + mMapCenterX - (xspan / 2.0);
    const float my = (yspan / 2.0) - (mousePosition.y() / mRoomHeight) - mMapCenterY;

    return {std::round(mx), std::round(my)};
}

void T2DMap::slot_createRoom()
{
    if (!mpHost) {
        return;
    }

    auto roomID = mpMap->createNewRoomID();
    if (!mpMap->addRoom(roomID)) {
        return;
    }

    mpMap->setRoomArea(roomID, mAreaID);
    mpMap->setRoomCoordinates(roomID, mContextMenuClickPosition.x, mContextMenuClickPosition.y, mMapCenterZ);

    mpMap->mMapGraphNeedsUpdate = true;
    isCenterViewCall = true;
    mpMap->updateArea(mAreaID);
    isCenterViewCall = false;
    mpMap->setUnsaved(__func__);
}

// Used both by "Properties..." context menu item for existing lines AND
// during drawing new ones.
void T2DMap::slot_customLineProperties()
{
    QString exit;
    TRoom* room;

    if (mCustomLineSelectedRoom > 0) {
        room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        exit = mCustomLineSelectedExit;
    } else {
        room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        exit = mCustomLinesRoomExit;
    }

    if (room) {
        if (exit.isEmpty()) {
            qDebug("T2DMap::slot_customLineProperties() called but no exit is selected...");
            return;
        }
        if (room->customLines.contains(exit)) {
            QUiLoader loader;

            QFile file(qsl(":/ui/custom_lines_properties.ui"));
            if (!file.open(QFile::ReadOnly)) {
                qWarning() << "T2DMap: failed to open custom_lines_properties.ui for reading:" << file.errorString();
                return;
            }
            auto* dialog = qobject_cast<QDialog*>(loader.load(&file, this));
            file.close();
            if (!dialog) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to create the dialog!");
                return;
            }
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_custom_exit_properties.png")));
            auto* le_toId = dialog->findChild<QLineEdit*>(qsl("toId"));
            auto* le_fromId = dialog->findChild<QLineEdit*>(qsl("fromId"));
            auto* le_cmd = dialog->findChild<QLineEdit*>(qsl("cmd"));

            mpCurrentLineStyle = dialog->findChild<QComboBox*>(qsl("lineStyle"));
            mpCurrentLineColor = dialog->findChild<QPushButton*>(qsl("lineColor"));
            mpCurrentLineArrow = dialog->findChild<QCheckBox*>(qsl("arrow"));
            if (!le_toId || !le_cmd || !le_fromId || !mpCurrentLineStyle || !mpCurrentLineColor || !mpCurrentLineArrow) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to find an element in the dialog!");
                return;
            }
            le_cmd->setText(exit);
            le_fromId->setText(QString::number(room->getId()));
            if (const int destination = TMapPainter::customLineDestination(*room, exit); destination || room->getSpecialExits().contains(exit)) {
                le_toId->setText(QString::number(destination));
            } else {
                qWarning().noquote().nospace() << "T2DMap::slot_customLineProperties() WARNING - missing no exit \"" << exit
                                               << "\" to be associated with a custom exit line with that designation in room id " << room->getId();
            }

            mpCurrentLineStyle->setIconSize(QSize(48, 24));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_solid)), tr("Solid line"), static_cast<int>(Qt::SolidLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dot)), tr("Dot line"), static_cast<int>(Qt::DotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dash)), tr("Dash line"), static_cast<int>(Qt::DashLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDot)), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDotDot)), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
            Qt::PenStyle const lineStyle = room->customLinesStyle.value(exit);
            mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(lineStyle)));

            mpCurrentLineArrow->setChecked(room->customLinesArrow.value(exit));
            mCurrentLineColor = room->customLinesColor.value(exit);

            mpCurrentLineColor->setStyleSheet(mudlet::self()->mBG_ONLY_STYLESHEET.arg(mCurrentLineColor.name()));
            connect(mpCurrentLineColor, &QAbstractButton::clicked, this, &T2DMap::slot_customLineColor);
            dialog->adjustSize();

            // The dialog leaves scripts running, and one may delete the room or its line, or load
            // another map (a JSON load swaps in a new room database) meanwhile
            connect(dialog, &QDialog::accepted, this, [this, roomId = room->getId(), exit, roomDB = mpMap->mpRoomDB.get(), mapGeneration = mpMap->mpRoomDB->mapGeneration()]() {
                if (mpMap->mpRoomDB.get() != roomDB || mpMap->mpRoomDB->mapGeneration() != mapGeneration) {
                    return;
                }
                TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
                if (!room || !room->customLines.contains(exit)) {
                    return;
                }
                mCurrentLineStyle = static_cast<Qt::PenStyle>(mpCurrentLineStyle->currentData().toInt());
                room->customLinesStyle[exit] = mCurrentLineStyle;
                room->customLinesColor[exit] = mCurrentLineColor;
                room->customLinesArrow[exit] = mpCurrentLineArrow->checkState();
                mCurrentLineArrow = mpCurrentLineArrow->checkState();

                repaint();
                mpMap->setUnsaved(__func__);
            });
            dialog->show();
            dialog->raise();
        }
    } else {
        qDebug("T2DMap::slot_customLineProperties() called but no line is selected...");
    }
}


void T2DMap::slot_customLineAddPoint()
{
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if (!room) {
        return;
    }
    // The menu stays open while scripts run, so the line may have been removed or shortened since
    const QList<QPointF> line = room->customLines.value(mCustomLineSelectedExit);
    if (mCustomLineSelectedPoint < 0 || mCustomLineSelectedPoint >= line.size()) {
        return;
    }

    QLineF segment;
    if (mCustomLineSelectedPoint > 0) {
        segment = QLineF(line.at(mCustomLineSelectedPoint - 1), line.at(mCustomLineSelectedPoint));
    } else {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction
        // The first segment of custom line stick out half of the distance
        // between two rooms at a map unit vector distance apart. so an added
        // point inserted before the first must be placed halfway between
        // the offset point and the previous first point
        QPointF customLineStartPoint;
        if (mCustomLineSelectedExit == key_n) {
            customLineStartPoint = QPointF(room->x(), room->y() + 0.5);
        } else if (mCustomLineSelectedExit == key_s) {
            customLineStartPoint = QPointF(room->x(), room->y() - 0.5);
        } else if (mCustomLineSelectedExit == key_e) {
            customLineStartPoint = QPointF(room->x() + 0.5, room->y());
        } else if (mCustomLineSelectedExit == key_w) {
            customLineStartPoint = QPointF(room->x() - 0.5, room->y());
        } else if (mCustomLineSelectedExit == key_ne) {
            customLineStartPoint = QPointF(room->x() + 0.5, room->y() + 0.5);
        } else if (mCustomLineSelectedExit == key_nw) {
            customLineStartPoint = QPointF(room->x() - 0.5, room->y() + 0.5);
        } else if (mCustomLineSelectedExit == key_se) {
            customLineStartPoint = QPointF(room->x() + 0.5, room->y() - 0.5);
        } else if (mCustomLineSelectedExit == key_sw) {
            customLineStartPoint = QPointF(room->x() - 0.5, room->y() - 0.5);
        } else {
            customLineStartPoint = QPointF(room->x(), room->y());
        }
        segment = QLineF(customLineStartPoint, line.at(0));
    }
    segment.setLength(segment.length() / 2.0);
    QPointF newPoint = segment.p2();
    if (isSnapCustomLinePointsToGridEnabled()) {
        newPoint = snapPointToGrid(newPoint);
    }
    room->customLines[mCustomLineSelectedExit].insert(mCustomLineSelectedPoint, newPoint);
    mCustomLineSelectedPoint++;
    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process:
    room->calcRoomDimensions();
    repaint();
    mpMap->setUnsaved(__func__);
}


void T2DMap::slot_setSnapCustomLinePointsToGrid(bool enabled)
{
    if (mCustomLineSession) {
        mCustomLineSession->setSnapToGridEnabled(enabled);
    }
}


void T2DMap::slot_customLineRemovePoint()
{
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if (!room) {
        return;
    }
    // The menu stays open while scripts run, so the line may have been removed or shortened since
    const auto pointCount = room->customLines.value(mCustomLineSelectedExit).size();
    if (mCustomLineSelectedPoint < 0 || mCustomLineSelectedPoint >= pointCount) {
        return;
    }

    if (mCustomLineSelectedPoint > 0) {
        room->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
        mCustomLineSelectedPoint--;
    } else if (pointCount > 1) {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction.  We can only
        // allow its deletion if there is at least another one left.
        room->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
    }
    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process:
    room->calcRoomDimensions();
    repaint();
    mpMap->setUnsaved(__func__);
}


bool T2DMap::isSnapCustomLinePointsToGridEnabled() const
{
    return mCustomLineSession && mCustomLineSession->isSnapToGridEnabled();
}


QPointF T2DMap::snapPointToGrid(const QPointF& point) const
{
    if (mCustomLineSession) {
        return mCustomLineSession->snapPointToGrid(point);
    }

    return point;
}


bool T2DMap::canMoveSelectedCustomLineLastPointToTargetRoom() const
{
    return mCustomLineSession && mCustomLineSession->canMoveSelectedCustomLineLastPointToTargetRoom();
}


bool T2DMap::canMoveCustomLineLastPointToTargetRoom(const TRoom& room, const QString& exitKey) const
{
    return mCustomLineSession && mCustomLineSession->canMoveCustomLineLastPointToTargetRoom(room, exitKey);
}


void T2DMap::slot_moveCustomLineLastPointToTargetRoom()
{
    if (mCustomLineSession) {
        mCustomLineSession->moveCustomLineLastPointToTargetRoom();
    }
}


void T2DMap::slot_undoCustomLineLastPoint()
{
    if (mCustomLinesRoomFrom > 0) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        if (room) {
            if (!room->customLines.value(mCustomLinesRoomExit).isEmpty()) {
                room->customLines[mCustomLinesRoomExit].pop_back();
            }
            room->calcRoomDimensions();
        }
        repaint();
        mpMap->setUnsaved(__func__);
    }
}

void T2DMap::slot_doneCustomLine()
{
    if (mCustomLineSession) {
        mCustomLineSession->clearOriginalPoints();
    }

    if (mpCustomLinesDialog) {
        mpCustomLinesDialog->accept();
        mpCustomLinesDialog = nullptr;
    }
    mHelpMsg = "";
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    if (!mMultiSelectionSet.empty()) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (room) {
            room->calcRoomDimensions();
        }
    }
    mpMap->updateArea(mAreaID);
    mpMap->setUnsaved(__func__);
}

void T2DMap::slot_deleteCustomExitLine()
{
    if (mCustomLineSelectedRoom > 0) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (room) {
            if (mCustomLineSession) {
                mCustomLineSession->clearOriginalPoints();
            }

            room->customLinesArrow.remove(mCustomLineSelectedExit);
            room->customLinesColor.remove(mCustomLineSelectedExit);
            room->customLinesStyle.remove(mCustomLineSelectedExit);
            room->customLines.remove(mCustomLineSelectedExit);
            mCustomLineSelectedRoom = 0;
            mCustomLineSelectedExit = "";
            mCustomLineSelectedPoint = -1;
            repaint();
            mpMap->setUnsaved(__func__);
            room->calcRoomDimensions();
            TArea* area = mpMap->mpRoomDB->getArea(room->getArea());
            if (area) {
                area->calcSpan();
            }
        }
    }
}

void T2DMap::slot_moveLabel()
{
    mMoveLabel = true;
    setMouseTracking(true);
    //: 2D Mapper big, bottom of screen help message when moving a label
    mHelpMsg = tr("Click to finish moving the label.");

    auto [mx, my] = getMousePosition();
    auto* area = mpMap->mpRoomDB->getArea(mAreaID);
    if (area) {
        for (auto& mapLabel : area->mMapLabels) {
            if (mapLabel.highlight) {
                mapLabel.pos = QVector3D(static_cast<float>(mx), static_cast<float>(my), static_cast<float>(mMapCenterZ));
            }
        }
    }

    update();
}

void T2DMap::slot_deleteLabel()
{
    auto pA = mpMap->mpRoomDB->getArea(mAreaID);
    if (!pA || pA->mMapLabels.isEmpty()) {
        return;
    }

    bool updateNeeded = false;
    bool saveNeeded = false;
    QMutableMapIterator<int, TMapLabel> itMapLabel(pA->mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        auto label = itMapLabel.value();
        if (qRound(label.pos.z()) != mMapCenterZ) {
            continue;
        }
        if (label.highlight) {
            itMapLabel.remove();
            updateNeeded = true;
            if (!label.temporary) {
                saveNeeded = true;
            }
        }
    }

    if (updateNeeded) {
        mpMap->updateArea(mAreaID);
        if (saveNeeded) {
            mpMap->setUnsaved(__func__);
        }
    }
}

void T2DMap::slot_editLabel() {}

void T2DMap::slot_setPlayerLocation()
{
    if (mMultiSelectionSet.size() != 1) {
        return; // Was <= 1 but that can't be right, and >1 doesn't seem right either
    }

    const int _newRoomId = *(mMultiSelectionSet.constBegin());
    if (auto* pR = mpMap->mpRoomDB->getRoom(_newRoomId)) {
        // Read before the event: its Lua handlers can delete the room (or the whole map)
        const int areaId = pR->getArea();
        // No need to check it is a DIFFERENT room - that is taken care of by en/dis-abling the control
        mpMap->mRoomIdHash[mpMap->mProfileName] = _newRoomId;
        mpMap->mNewMove = true;
        TEvent manualSetEvent{};
        manualSetEvent.mArgumentList.append(QLatin1String("sysManualLocationSetEvent"));
        manualSetEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        manualSetEvent.mArgumentList.append(QString::number(_newRoomId));
        manualSetEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(manualSetEvent);
        mpMap->updatePlayerArea(areaId);
    }
}

void T2DMap::slot_toggleMapViewOnly()
{
    if (mpHost) {
        // If the local state did not match the profile stored state (in Host)
        // then we get called once from init() - this will toggle the state to
        // match:
        mMapViewOnly = !mMapViewOnly;
        // In the init() case this is a no-op, otherwise it ensures the profile
        // state matches the local copy (so it gets saved with the profile):
        mpHost->mMapViewOnly = mMapViewOnly;
        TEvent mapModeEvent{};
        mapModeEvent.mArgumentList.append(QLatin1String("mapModeChangeEvent"));
        mapModeEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mapModeEvent.mArgumentList.append(mMapViewOnly ? qsl("viewing") : qsl("editing"));
        mapModeEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mapModeEvent);

        update();
    }
}

void T2DMap::populateUserContextMenus(QMenu& menu)
{
    if (!mpMap) {
        return;
    }
    struct UserEntry
    {
        int priority;
        bool menu;
        QString uniqueName;
        QString parent;
    };
    QMap<QString, QMenu*> userMenus;
    QList<UserEntry> entries;
    for (const auto& [uniqueName, menuInfo] : std::as_const(mpMap->mUserMenus).asKeyValueRange()) {
        userMenus.insert(uniqueName, new QMenu(menuInfo.value(1), &menu));
        entries.append({mpMap->mUserMenuPriorities.value(uniqueName), true, uniqueName, menuInfo.value(0)});
    }
    for (const auto& [uniqueName, actionInfo] : std::as_const(mpMap->mUserActions).asKeyValueRange()) {
        entries.append({mpMap->mUserMenuPriorities.value(uniqueName), false, uniqueName, actionInfo.value(1)});
    }
    // Stable: ties keep the default order of menus, then events, each by unique name
    std::stable_sort(entries.begin(), entries.end(), [](const UserEntry& a, const UserEntry& b) {
        return a.priority < b.priority;
    });

    for (const UserEntry& entry : std::as_const(entries)) {
        const QString& uniqueName = entry.uniqueName;
        const QString& menuParentKey = entry.parent;
        if (entry.menu) {
            QMenu* childMenu = userMenus.value(uniqueName);
            if (menuParentKey.isEmpty()) {
                menu.addMenu(childMenu);
            } else if (auto* parentMenu = userMenus.value(menuParentKey, nullptr)) {
                parentMenu->addMenu(childMenu);
            }
            continue;
        }

        const QString displayText = mpMap->mUserActions.value(uniqueName).value(2);

        auto* action = new QAction(displayText, &menu);
        if (menuParentKey.isEmpty()) {
            menu.addAction(action);
        } else if (auto* parentMenu = userMenus.value(menuParentKey, nullptr)) {
            parentMenu->addAction(action);
        } else {
            delete action;
            continue;
        }

        QObject::connect(action, &QAction::triggered, this, [this, uniqueName](bool) {
            slot_userAction(uniqueName);
        });
    }
}

void T2DMap::slot_userAction(QString uniqueName)
{
    if (!mpMap) {
        return;
    }
    // The menu stays open while scripts run, so removeMapEvent() may have taken the item away since
    const QStringList userEvent = mpMap->mUserActions.value(uniqueName);
    if (userEvent.isEmpty()) {
        return;
    }
    TEvent event{};
    event.mArgumentList.append(userEvent[0]);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(uniqueName);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    QSetIterator<int> itRoom(mMultiSelectionSet);
    if (itRoom.hasNext()) {
        while (itRoom.hasNext()) {
            event.mArgumentList.append(QString::number(itRoom.next()));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        }
        mpHost->raiseEvent(event);
    } else {
        event.mArgumentList.append(uniqueName);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        for (int i = 0; i < userEvent.size(); i++) {
            event.mArgumentList.append(userEvent[i]);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }
        mpHost->raiseEvent(event);
    }
}

void T2DMap::slot_movePosition()
{
    if (!getCenterSelection()) {
        return;
    }

    const int startRoomId = mMultiSelectionHighlightRoomId;
    TRoom* pR_start = mpMap->mpRoomDB->getRoom(startRoomId);
    // pR has already been validated by getCenterSelection() but add explicit check
    if (!pR_start) {
        return;
    }

    auto dialog = new QDialog(this);
    auto gridLayout = new QGridLayout;
    // Do NOT try to set the Qt::WA_DeleteOnClose attribute on the dialogue,
    // because we want to read the details from the QLineEdits on it after the
    // user has clicked "OK" on the dialog - and setting that flag will cause
    // it (and those QLineEdits) to be destroyed by the time the QDialog:exec()
    // call returns which is before we've got those details!
    dialog->setLayout(gridLayout);
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    dialog->setContentsMargins(0, 0, 0, 0);
    auto pLEx = new QLineEdit(dialog);
    auto pLEy = new QLineEdit(dialog);
    auto pLEz = new QLineEdit(dialog);

    pLEx->setText(QString::number(pR_start->x()));
    pLEy->setText(QString::number(pR_start->y()));
    pLEz->setText(QString::number(pR_start->z()));
    QLabel* pLa0 = new QLabel(tr("Move the selection, centered on the highlighted room (%1) to:",
                                 // Intentional comment to separate arguments
                                 "%1 is a room number")
                                      .arg(mMultiSelectionHighlightRoomId));
    pLa0->setWordWrap(true);
    // Record the starting coordinates - can be a help when working out how to move a block of rooms!
    QLabel* pLa1 = new QLabel(tr("x coordinate (was %1):").arg(pR_start->x()));
    QLabel* pLa2 = new QLabel(tr("y coordinate (was %1):").arg(pR_start->y()));
    QLabel* pLa3 = new QLabel(tr("z coordinate (was %1):").arg(pR_start->z()));
    gridLayout->addWidget(pLa0, 0, 0, 1, 2, Qt::AlignCenter);
    gridLayout->addWidget(pLa1, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEx, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    gridLayout->addWidget(pLa2, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEy, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
    gridLayout->addWidget(pLa3, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEz, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto pButtonBar = new QWidget(dialog);

    auto boxLayout = new QHBoxLayout;
    pButtonBar->setLayout(boxLayout);
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    auto pB_ok = new QPushButton(pButtonBar);
    //: dialog (room(s) move) button
    pB_ok->setText(tr("OK"));
    boxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    //: dialog (room(s) move) button
    pB_abort->setText(tr("Cancel"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    boxLayout->addWidget(pB_abort);
    gridLayout->addWidget(pButtonBar, 4, 0, 1, 2, Qt::AlignCenter);

    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        pB_ok->setIcon(QIcon::fromTheme(key_dialog_ok_apply, QIcon(key_icon_dialog_ok_apply)));
        pB_abort->setIcon(QIcon::fromTheme(key_dialog_cancel, QIcon(key_icon_dialog_cancel)));
    }

    const bool accepted = dialog->exec() == QDialog::Accepted;
    // The dialog's event loop runs scripts, which may delete the room
    pR_start = mpMap->mpRoomDB->getRoom(startRoomId);
    if (accepted && pR_start) {
        const int dx = pLEx->text().toInt() - pR_start->x();
        const int dy = pLEy->text().toInt() - pR_start->y();
        const int dz = pLEz->text().toInt() - pR_start->z();

        mMultiRect = QRect(0, 0, 0, 0);

        QSetIterator<int> itRoom = mMultiSelectionSet;
        QSet<int> dirtyAreas;
        while (itRoom.hasNext()) {
            TRoom* room = mpMap->mpRoomDB->getRoom(itRoom.next());
            if (!room) {
                continue;
            }

            room->offset(dx, dy, dz);
            dirtyAreas.insert(room->getArea());
        }
        QSetIterator<int> itAreaID(dirtyAreas);
        while (itAreaID.hasNext()) {
            auto pArea = mpMap->mpRoomDB->getArea(itAreaID.next());
            if (pArea) {
                pArea->calcSpan();
            }
        }
        mpMap->setUnsaved(__func__);
    }
    dialog->deleteLater();
    repaint();
}


void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);
    mNewMoveAction = true;
    mRoomMoveViaContextMenu = true;
    mHasRoomMoveLastMapPoint = false;
    mHelpMsg = tr("Click to finish moving the selected room(s).");
    update();
}

void T2DMap::slot_showPropertiesDialog()
{
    // Counts and reports the existing properties used in ALL the selected rooms
    // if more than one has been selected (and sorts by their frequency).
    // Allows the existing symbols to be deleted (by clearing all the displayed letters)

    // No need to show dialog if no rooms are selected
    if (mMultiSelectionSet.empty()) {
        return;
    }

    // No need to show if dialog is already shown
    if (mpDlgRoomProperties) {
        return;
    }

    bool isAtLeastOneRoom = false;
    QSetIterator<int> itRoom = mMultiSelectionSet;
    QSet<int> roomIdsSet;

    QHash<QString, int> usedNames;
    QHash<int, int> usedColors;
    QHash<QString, int> usedSymbols;
    QHash<int, int> usedWeights; // key is weight, value is count of uses
    QHash<bool, int> usedLockStatus;
    int hiddenRoomCount = 0;

    while (itRoom.hasNext()) {
        const int roomId = itRoom.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }
        roomIdsSet.insert(roomId);
        isAtLeastOneRoom = true;

        // Scan and count all the different names used
        if (!room->name.isEmpty()) {
            const QString thisName = QString(room->name);
            if (!thisName.isEmpty()) {
                if (usedNames.contains(thisName)) {
                    (usedNames[thisName])++;
                } else {
                    usedNames[thisName] = 1;
                }
            }
        }

        // Scan and count all the different room colors used
        const int thisColor = room->environment;
        if (usedColors.contains(thisColor)) {
            (usedColors[thisColor])++;
        } else {
            usedColors[thisColor] = 1;
        }

        // Scan and count all the different symbols used
        const QString thisSymbol = QString(room->mSymbol);
        if (usedSymbols.contains(thisSymbol)) {
            (usedSymbols[thisSymbol])++;
        } else {
            usedSymbols[thisSymbol] = 1;
        }

        // Scan and count all the different weights used
        const int thisWeight = room->getWeight();
        if (thisWeight > 0) {
            if (usedWeights.contains(thisWeight)) {
                (usedWeights[thisWeight])++;
            } else {
                usedWeights[thisWeight] = 1;
            }
        }

        // Scan and count all the different lock status used
        const bool thisLockStatus = room->isLocked;
        if (usedLockStatus.contains(thisLockStatus)) {
            (usedLockStatus[thisLockStatus])++;
        } else {
            usedLockStatus[thisLockStatus] = 1;
        }

        // Count the hidden rooms
        if (room->isHidden()) {
            ++hiddenRoomCount;
        }
    }

    // No need to show dialog if no rooms were found
    if (!isAtLeastOneRoom) {
        return;
    }

    mpDlgRoomProperties = new dlgRoomProperties(mpHost, this);
    mpDlgRoomProperties->init(usedNames, usedColors, usedSymbols, usedWeights, usedLockStatus, hiddenRoomCount, roomIdsSet);
    mpDlgRoomProperties->show();
    mpDlgRoomProperties->raise();
    connect(mpDlgRoomProperties, &dlgRoomProperties::signal_save_symbol, this, &T2DMap::slot_setRoomProperties);
    connect(mpDlgRoomProperties, &dlgRoomProperties::signal_preview_border, this, &T2DMap::slot_previewBorderProperties);
    connect(mpDlgRoomProperties, &QDialog::finished, this, [=, this]() {
        mpDlgRoomProperties = nullptr;
    });
}


void T2DMap::slot_setRoomProperties(bool changeName,
                                    QString newName,
                                    bool changeRoomColor,
                                    int newRoomColor,
                                    bool changeSymbol,
                                    QString newSymbol,
                                    bool changeSymbolColor,
                                    QColor newSymbolColor,
                                    bool changeWeight,
                                    int newWeight,
                                    bool changeLockStatus,
                                    std::optional<bool> newLockStatus,
                                    bool changeHiddenStatus,
                                    std::optional<bool> newHiddenStatus,
                                    bool changeBorderColor,
                                    QColor newBorderColor,
                                    bool changeBorderThickness,
                                    int newBorderThickness,
                                    QSet<int> roomIds)
{
    if (newName.isEmpty()) {
        newName = QString();
    } else {
        // 10.0 is the maximum supported by all the Qt versions (5.14+) we
        // handle/use/allow - by normalising the symbol we can ensure that
        // all the entered ones are decomposed and recomposed in a
        // "standard" way and will have the same sequence of codepoints:
        newName = newName.normalized(QString::NormalizationForm_C, QChar::Unicode_10_0);
    }

    if (newSymbol.isEmpty()) {
        newSymbol = QString();
    } else {
        // 10.0 is the maximum supported by all the Qt versions (5.14+) we
        // handle/use/allow - by normalising the symbol we can ensure that
        // all the entered ones are decomposed and recomposed in a
        // "standard" way and will have the same sequence of codepoints:
        newSymbol = newSymbol.normalized(QString::NormalizationForm_C, QChar::Unicode_10_0);
    }

    // The rooms' own area, or -1 (every area) when they span several: they may have been picked on the 3D view
    // while this map, hidden, shows another
    std::optional<int> editedAreaId;
    for (const int roomId : std::as_const(roomIds)) {
        // The dialog is not modal, so rooms may have gone while it was open
        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }
        editedAreaId = !editedAreaId || *editedAreaId == room->getArea() ? room->getArea() : -1;
        if (changeName) {
            room->name = newName;
        }
        if (changeRoomColor) {
            room->environment = newRoomColor;
        }
        if (changeSymbol) {
            room->mSymbol = newSymbol;
        }
        if (changeSymbolColor) {
            room->mSymbolColor = newSymbolColor;
        }
        if (changeWeight) {
            room->setWeight(newWeight);
        }
        if (changeLockStatus && newLockStatus.has_value()) {
            room->isLocked = newLockStatus.value();
        }
        if (changeHiddenStatus && newHiddenStatus.has_value()) {
            room->setHidden(newHiddenStatus.value());
        }
        if (changeBorderColor) {
            room->mBorderColor = newBorderColor;
        }
        if (changeBorderThickness) {
            room->mBorderThickness = newBorderThickness;
        }
    }
    if (changeWeight || changeLockStatus) {
        mpMap->mMapGraphNeedsUpdate = true;
    }
    mpMap->updateArea(editedAreaId.value_or(mAreaID));
    mpMap->setUnsaved(__func__);
}

void T2DMap::slot_previewBorderProperties(QSet<int> roomIds)
{
    Q_UNUSED(roomIds)
    repaint();
    update();
    // The 3D view keeps what it drew of the area until told the area changed
    mpMap->updateArea(-1);
}

void T2DMap::slot_setImage() {}

void T2DMap::slot_deleteRoom()
{
    mpMap->mpRoomDB->removeRoom(mMultiSelectionSet);
    // mMultiSelectionSet gets cleared as rooms are removed by
    // TRoomDB::removeRoom() so no need to clear it here!
    mMultiRect = QRect(0, 0, 0, 0);
    mMultiSelectionListWidget.clear();
    mMultiSelectionListWidget.hide();
    repaint();
    mpMap->setUnsaved(__func__);
}

void T2DMap::slot_spread()
{
    if (mMultiSelectionSet.size() < 2) { // nothing to do!
        return;
    }

    const int centerRoomId = mMultiSelectionHighlightRoomId;
    TRoom* pR_centerRoom = mpMap->mpRoomDB->getRoom(centerRoomId);
    if (!pR_centerRoom) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    const int spread = QInputDialog::getInt(this,
                                            tr("Spread out rooms"),
                                            tr("Increase the spacing of\n"
                                               "the selected rooms,\n"
                                               "centered on the\n"
                                               "highlighted room by a\n"
                                               "factor of:"),
                                            5,    // Initial value
                                            1,    // Minimum value
                                            1000, // Maximum value
                                            1,    // Step
                                            &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    // The dialog's event loop runs scripts, which may delete the room
    pR_centerRoom = mpMap->mpRoomDB->getRoom(centerRoomId);
    if (!pR_centerRoom) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    const int dx = pR_centerRoom->x();
    const int dy = pR_centerRoom->y();
    const int areaID = pR_centerRoom->getArea();
    auto pArea = mpMap->mpRoomDB->getArea(areaID);
    bool doneSomething = false;
    QSetIterator<int> itSelectionRoom = mMultiSelectionSet;
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }

        doneSomething = true;
        pMovingR->setCoordinates(((pMovingR->x() - dx) * spread + dx), ((pMovingR->y() - dy) * spread + dy), pMovingR->z());
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                const QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>((movingPoint.x() - dx) * spread + dx));
                customLinePoint.setY(static_cast<float>((movingPoint.y() - dy) * spread + dy));
            }
            newCustomLinePointsMap.insert(itCustomLine.key(), customLinePoints);
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    if (doneSomething) {
        if (pArea) {
            pArea->mIsDirty = true;
            pArea->clean();
        }
        repaint();
        mpMap->setUnsaved(__func__);
    }
}

void T2DMap::slot_shrink()
{
    if (mMultiSelectionSet.size() < 2) { // nothing to do!
        return;
    }

    const int centerRoomId = mMultiSelectionHighlightRoomId;
    TRoom* pR_centerRoom = mpMap->mpRoomDB->getRoom(centerRoomId);
    if (!pR_centerRoom) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    const int spread = QInputDialog::getInt(this,
                                            tr("Shrink in rooms"),
                                            tr("Decrease the spacing of\n"
                                               "the selected rooms,\n"
                                               "centered on the\n"
                                               "highlighted room by a\n"
                                               "factor of:"),
                                            5,    // Initial value
                                            1,    // Minimum value
                                            1000, // Maximum value
                                            1,    // Step
                                            &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    // The dialog's event loop runs scripts, which may delete the room
    pR_centerRoom = mpMap->mpRoomDB->getRoom(centerRoomId);
    if (!pR_centerRoom) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    const int dx = pR_centerRoom->x();
    const int dy = pR_centerRoom->y();
    const int areaID = pR_centerRoom->getArea();
    auto pArea = mpMap->mpRoomDB->getArea(areaID);
    bool doneSomething = false;
    QSetIterator<int> itSelectionRoom(mMultiSelectionSet);
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }

        doneSomething = true;
        pMovingR->setCoordinates(((pMovingR->x() - dx) / spread + dx), ((pMovingR->y() - dy) / spread + dy), pMovingR->z());
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                const QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>((movingPoint.x() - dx) / spread + dx));
                customLinePoint.setY(static_cast<float>((movingPoint.y() - dy) / spread + dy));
            }
            newCustomLinePointsMap.insert(itCustomLine.key(), customLinePoints);
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    if (doneSomething) {
        if (pArea) {
            pArea->mIsDirty = true;
            pArea->clean();
        }
        repaint();
        mpMap->setUnsaved(__func__);
    }
}

void T2DMap::slot_setExits()
{
    if (mMultiSelectionSet.empty()) {
        return;
    }
    if (mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId)) {
        auto pD = new dlgRoomExits(mpHost, mMultiSelectionHighlightRoomId, this);
        pD->show();
        pD->raise();
        pD->setAttribute(Qt::WA_DeleteOnClose);
    }
}


void T2DMap::slot_setUserData() {}

void T2DMap::slot_loadMap()
{
    if (!mpHost) {
        return;
    }

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName())).toString();


    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Load Mudlet map"),
                                                          lastDir,
                                                          tr("Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)",
                                                             // Intentional comment to separate arguments
                                                             "Do not change extensions (in braces) or the ;;s as they are used programmatically"));
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    if (fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        mpHost->importMapFile(fileName);
    } else {
        mpHost->loadMapFile(fileName);
    }
}

void T2DMap::slot_newMap()
{
    if (!mpHost) {
        return;
    }

    auto roomID = mpMap->createNewRoomID();

    if (!mpMap->addRoom(roomID)) {
        return;
    }

    mpMap->setRoomArea(roomID, -1);
    mpMap->setRoomCoordinates(roomID, 0, 0, 0);
    mpMap->mMapGraphNeedsUpdate = true;

    mpMap->mRoomIdHash[mpMap->mProfileName] = roomID;
    mpMap->mNewMove = true;
    if (mMapViewOnly) {
        slot_toggleMapViewOnly();
    }

    isCenterViewCall = true;
    mpMap->updateArea(-1);
    isCenterViewCall = false;
    mpMap->setUnsaved(__func__);
    if (mpMap->mapper()) {
        mpMap->mapper()->resetAreaComboBoxToPlayerRoomArea();
    }
}

void T2DMap::slot_setArea()
{
    QUiLoader loader;

    QFile file(":/ui/set_room_area.ui");
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "T2DMap: failed to open set_room_area.ui for reading:" << file.errorString();
        return;
    }
    auto* set_room_area_dialog = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!set_room_area_dialog) {
        return;
    }
    set_room_area_dialog->setAttribute(Qt::WA_DeleteOnClose);
    arealist_combobox = set_room_area_dialog->findChild<QComboBox*>("arealist_combobox");

    if (!arealist_combobox) {
        return;
    }

    auto label_info = set_room_area_dialog->findChild<QLabel*>("label_info");
    auto font = QFont();
    font.setPointSize(font.pointSize() - 1);
    label_info->setFont(font);
    arealist_combobox->setInsertPolicy(QComboBox::NoInsert);

    const QMap<int, QString>& areaNamesMap = mpMap->mpRoomDB->getAreaNamesMap();
    const QStringList sortedAreaList = getMapperAreaNamesSorted(areaNamesMap);
    for (const QString& areaName : std::as_const(sortedAreaList)) {
        const int areaId = areaNamesMap.key(areaName);
        arealist_combobox->addItem(qsl("%1 (%2)").arg(areaName, QString::number(areaId)), QString::number(areaId));
    }

    connect(arealist_combobox, &QComboBox::currentTextChanged, this, [=, this](const QString newText) {
        auto buttonBox = set_room_area_dialog->findChild<QDialogButtonBox*>("buttonBox");
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!newText.trimmed().isEmpty());
        if (!newText.trimmed().isEmpty() && arealist_combobox->findText(newText.trimmed(), Qt::MatchExactly) == -1 && !sortedAreaList.contains(newText.trimmed())) {
            label_info->setText(tr("This will create new area: %1").arg(arealist_combobox->currentText()));
        } else {
            label_info->clear();
        }
    });

    connect(set_room_area_dialog, &QDialog::accepted, [=, this]() {
        int newAreaId;
        if (arealist_combobox->findText(arealist_combobox->currentText(), Qt::MatchExactly) != -1) {
            newAreaId = arealist_combobox->itemData(arealist_combobox->currentIndex()).toInt();
        } else if (sortedAreaList.contains(arealist_combobox->currentText().trimmed())) {
            newAreaId = mpMap->mpRoomDB->getAreaNamesMap().key(arealist_combobox->currentText());
        } else {
            auto newAreaName = arealist_combobox->currentText().trimmed();
            newAreaId = mpMap->mpRoomDB->addArea(newAreaName);
            if (!newAreaId) {
                mpMap->postMessage(tr("[ ERROR ] - Unable to add \"%1\" as an area to the map.\n"
                                      "See the \"[MAP ERROR:]\" message for the reason.",
                                      // Intentional separator between argument
                                      "The '[MAP ERROR:]' text here should be the same as that used for the translation of \"[MAP ERROR:] %1\" in the 'TMap::logError(...)' function.")
                                           .arg(newAreaName));
                return;
            }
            mpMap->postMessage(tr("[  OK  ]  - Added \"%1\" (%2) area to map.").arg(newAreaName, QString::number(newAreaId)));
            mpMap->setUnsaved(__func__);

            if (mpMap->mapper()) {
                mpMap->mapper()->updateAreaComboBox();
            }
        }
        mMultiRect = QRect(0, 0, 0, 0);
        QSetIterator<int> itSelectedRoom = mMultiSelectionSet;
        while (itSelectedRoom.hasNext()) {
            mpMap->setRoomArea(itSelectedRoom.next(), newAreaId);
        }
        if (!mMultiSelectionSet.isEmpty()) {
            const auto& targetAreaName = mpMap->mpRoomDB->getAreaNamesMap().value(newAreaId);
            if (!mIsSecondaryView && mpMap->mapper()) {
                mpMap->mapper()->comboBox_showArea->setCurrentText(targetAreaName);
            }
            switchArea(targetAreaName);
            // The rooms are still selected, so land on them rather than on
            // whichever room switchArea() would otherwise have picked:
            TRoom* pCenterRoom = getCenterSelection() ? mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId) : nullptr;
            if (pCenterRoom) {
                mMapCenterX = pCenterRoom->x();
                mMapCenterY = -pCenterRoom->y();
                mMapCenterZ = pCenterRoom->z();
                set3DViewCenter(newAreaId, mMapCenterX, -mMapCenterY, mMapCenterZ);
            }
        }
        update();
    });

    set_room_area_dialog->show();
    set_room_area_dialog->raise();

    if (!mIsSecondaryView && mpMap->mapper()) {
        arealist_combobox->setCurrentIndex(mpMap->mapper()->getCurrentShownAreaIndex());
    } else {
        arealist_combobox->setCurrentIndex(arealist_combobox->findData(QString::number(mAreaID)));
    }
}


void T2DMap::slot_configureAreas()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Configure Areas"));

    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto* listWidget = new QListWidget(dialog);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(listWidget);

    auto repopulate = [this, listWidget]() {
        listWidget->clear();
        const QMap<int, QString>& areaNamesMap = mpMap->mpRoomDB->getAreaNamesMap();
        const QStringList sortedAreaList = getMapperAreaNamesSorted(areaNamesMap);

        for (const QString& areaName : std::as_const(sortedAreaList)) {
            const int areaId = areaNamesMap.key(areaName);
            auto* item = new QListWidgetItem(qsl("%1 (%2)").arg(areaName, QString::number(areaId)), listWidget);
            item->setData(Qt::UserRole, areaId);
        }
    };

    repopulate();

    // mAreaID is the area the map is actually showing; the mapper's dropdown
    // index can't be used here as it omits the default area when hidden,
    // while this list always includes it.
    for (int i = 0; i < listWidget->count(); ++i) {
        auto* it = listWidget->item(i);
        if (it && it->data(Qt::UserRole).toInt() == mAreaID) {
            listWidget->setCurrentRow(i);
            listWidget->scrollToItem(it, QAbstractItemView::PositionAtCenter);
            break;
        }
    }

    auto* buttonBar = new QWidget(dialog);
    auto* hbox = new QHBoxLayout;
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(8);
    buttonBar->setLayout(hbox);
    //: "Configure Areas" buttons: create new area
    auto* createBtn = new QPushButton(tr("Create"), buttonBar);
    //: "Configure Areas" buttons: rename existing area
    auto* renameBtn = new QPushButton(tr("Rename"), buttonBar);
    //: "Configure Areas" buttons: delete existing area
    auto* deleteBtn = new QPushButton(tr("Delete"), buttonBar);
    //: "Configure Areas" buttons: close the dialog
    auto* closeBtn = new QPushButton(tr("Close"), buttonBar);
    const bool initialHasSelection = listWidget->currentItem() != nullptr;
    const int initialAreaId = initialHasSelection ? listWidget->currentItem()->data(Qt::UserRole).toInt() : 0;
    const bool initialRenameEnabled = initialHasSelection && initialAreaId != -1;
    const bool initialDeleteEnabled = initialHasSelection && initialAreaId != -1;
    renameBtn->setEnabled(initialRenameEnabled);
    deleteBtn->setEnabled(initialDeleteEnabled);
    hbox->addStretch(1);
    hbox->addWidget(createBtn);
    hbox->addWidget(renameBtn);
    hbox->addWidget(deleteBtn);
    hbox->addWidget(closeBtn);
    layout->addWidget(buttonBar);

    connect(listWidget, &QListWidget::currentItemChanged, this, [renameBtn, deleteBtn, this](QListWidgetItem* current, QListWidgetItem*) {
        if (!current) {
            renameBtn->setEnabled(false);
            deleteBtn->setEnabled(false);
            return;
        }

        const int areaId = current->data(Qt::UserRole).toInt();
        // default area (-1) must not be renamed or deleted
        const bool enabled = (areaId != -1);
        renameBtn->setEnabled(enabled);
        deleteBtn->setEnabled(enabled);
    });

    connect(renameBtn, &QPushButton::clicked, this, [this, dialog, listWidget, repopulate]() {
        auto* item = listWidget->currentItem();
        if (!item) {
            return;
        }

        if (!mpMap || !mpMap->mpRoomDB) {
            return;
        }

        const int areaId = item->data(Qt::UserRole).toInt();
        const QString currentName = mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        bool ok = false;
        //: Dialog title for renaming an area
        const QString newName = QInputDialog::getText(dialog, tr("Rename area"), tr("New name:"), QLineEdit::Normal, currentName, &ok).trimmed();
        if (!ok || newName.isEmpty() || newName == currentName) {
            return;
        }

        const bool renamed = mpMap->mpRoomDB->setAreaName(areaId, newName);
        if (!renamed) {
            //: Warning message shown when renaming an area fails.
            QMessageBox::warning(dialog, tr("Rename failed"), tr("Unable to rename area. Name may be invalid or already in use."));
            return;
        }
        mpMap->setUnsaved(__func__);

        repopulate();
        for (int i = 0; i < listWidget->count(); ++i) {
            auto* it = listWidget->item(i);
            if (it && it->data(Qt::UserRole).toInt() == areaId) {
                listWidget->setCurrentRow(i);
                listWidget->scrollToItem(it, QAbstractItemView::PositionAtCenter);
                break;
            }
        }

        // Refresh every dropdown that lists area names - the primary
        // mapper's and every secondary view's, not just whichever one
        // opened this dialog.
        if (mpMap && mpMap->mapper()) {
            mpMap->mapper()->updateAreaComboBox();
            // Only follow the rename into the dropdown if it's the area the
            // primary mapper is actually showing - otherwise this would
            // move the dropdown to an area the map isn't displaying.
            if (mpMap->mapper()->mp2dMap && areaId == mpMap->mapper()->mp2dMap->getAreaId() && mpMap->mapper()->comboBox_showArea) {
                mpMap->mapper()->comboBox_showArea->setCurrentText(newName);
            }
        }
        if (mpMap && mpMap->getViewManager()) {
            mpMap->getViewManager()->updateAllViews();
        }
    });

    connect(createBtn, &QPushButton::clicked, this, [this, dialog, listWidget, repopulate]() {
        bool ok = false;
        //: Dialog title for creating a new area
        const QString name = QInputDialog::getText(dialog, tr("Create area"), tr("Name:"), QLineEdit::Normal, QString(), &ok).trimmed();
        if (!ok || name.isEmpty()) {
            return;
        }

        if (!mpMap || !mpMap->mpRoomDB) {
            return;
        }

        const int newAreaId = mpMap->mpRoomDB->addArea(name);
        if (!newAreaId) {
            //: Warning message shown when creating a new area fails.
            QMessageBox::warning(dialog, tr("Create failed"), tr("Unable to create area. Name may be invalid or already in use."));
            return;
        }
        mpMap->setUnsaved(__func__);

        repopulate();
        for (int i = 0; i < listWidget->count(); ++i) {
            auto* it = listWidget->item(i);
            if (it && it->data(Qt::UserRole).toInt() == newAreaId) {
                listWidget->setCurrentRow(i);
                listWidget->scrollToItem(it, QAbstractItemView::PositionAtCenter);
                break;
            }
        }

        if (mpMap && mpMap->mapper()) {
            mpMap->mapper()->updateAreaComboBox();
        }
        if (mpMap && mpMap->getViewManager()) {
            mpMap->getViewManager()->updateAllViews();
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [this, dialog, listWidget, repopulate]() {
        auto* item = listWidget->currentItem();
        if (!item) {
            return;
        }

        if (!mpMap || !mpMap->mpRoomDB) {
            return;
        }

        const int areaId = item->data(Qt::UserRole).toInt();
        if (areaId == -1) {
            //: Warning message shown when trying to delete the default area.
            QMessageBox::warning(dialog, tr("Delete failed"), tr("The default area cannot be deleted."));
            return;
        }

        // Delete immediately without confirmation
        const bool removed = mpMap->mpRoomDB->removeArea(areaId);
        if (!removed) {
            //: Warning message shown when trying to delete an area fails.
            QMessageBox::warning(dialog, tr("Delete failed"), tr("Unable to delete area."));
            return;
        }
        mpMap->setUnsaved(__func__);

        repopulate();

        // Refresh every dropdown that lists area names - the primary
        // mapper's and every secondary view's, not just whichever one
        // opened this dialog.
        if (mpMap->mapper()) {
            mpMap->mapper()->updateAreaComboBox();
        }
        if (mpMap->getViewManager()) {
            mpMap->getViewManager()->updateAllViews();
        }

        // Every map that was showing the deleted area needs to move off it -
        // paintEvent() can't draw an area that no longer exists - not just
        // whichever view opened this dialog.
        if (mpMap->mapper() && mpMap->mapper()->mp2dMap && mpMap->mapper()->mp2dMap->getAreaId() == areaId) {
            auto* comboBox = mpMap->mapper()->comboBox_showArea;
            if (comboBox && comboBox->count() > 0) {
                mpMap->mapper()->slot_switchArea(comboBox->currentIndex());
            } else {
                mpMap->mapper()->mp2dMap->switchArea(mpMap->getDefaultAreaName());
            }
        }
        if (mpMap->getViewManager()) {
            mpMap->getViewManager()->switchViewsShowingArea(areaId);
        }
        update();
    });

    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->resize(400, 320);
    dialog->show();
    dialog->raise();
}


void T2DMap::mouseMoveEvent(QMouseEvent* event)
{
    if (!mpMap) {
        event->ignore();
        return;
    }

    auto context = buildInteractionContext(event);
    mInteractionDispatcher.dispatch(context);
    event->accept();
}

// Replacement for getTopLeftCenter - determines a room closest to geometrical
// mean of all the room selected, the result is stored in the class member
// mMultiSelectionHighlightRoomId and this returns true on successfully finding
// such a room
bool T2DMap::getCenterSelection()
{
    mMultiSelectionHighlightRoomId = 0;
    if (mMultiSelectionSet.isEmpty()) {
        return false;
    }

    QSetIterator<int> itRoom = mMultiSelectionSet;
    float mean_x = 0.0;
    float mean_y = 0.0;
    float mean_z = 0.0;
    uint processedRoomCount = 0;
    while (itRoom.hasNext()) {
        const int currentRoomId = itRoom.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
        if (!room) {
            continue;
        }

        mean_x += (static_cast<float>(room->x() - mean_x)) / ++processedRoomCount;
        mean_y += (static_cast<float>(room->y() - mean_y)) / processedRoomCount;
        mean_z += (static_cast<float>(room->z() - mean_z)) / processedRoomCount;
    }

    if (processedRoomCount) {
        itRoom.toFront();
        float closestSquareDistance = -1.0;
        while (itRoom.hasNext()) {
            const int currentRoomId = itRoom.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
            if (!room) {
                continue;
            }

            const QVector3D meanToRoom(static_cast<float>(room->x()) - mean_x, static_cast<float>(room->y()) - mean_y, static_cast<float>(room->z()) - mean_z);
            if (closestSquareDistance < -0.5) {
                // Don't use an equality to zero test, we are using floats so
                // need to allow for a little bit of fuzzzyness!
                closestSquareDistance = meanToRoom.lengthSquared();
                mMultiSelectionHighlightRoomId = currentRoomId;
            } else {
                const float currentRoomSquareDistance = meanToRoom.lengthSquared();
                if (closestSquareDistance > currentRoomSquareDistance) {
                    closestSquareDistance = currentRoomSquareDistance;
                    mMultiSelectionHighlightRoomId = currentRoomId;
                }
            }
        }
        return true;
    }
    return false;
}

void T2DMap::wheelEvent(QWheelEvent* e)
{
    // If the mouse wheel is scrolling up and down through the
    // mMultiSelectionListWidget the wheelevents from that get passed up to here
    // when the end of the list is reached (i.e when it rejects those events)
    // - so that the mapper window doesn't THEN zoom in or out, swallow the
    // events, i.e. accept() them and return before hitting the zoom altering
    // code that follows.
    // However the event "pos()" depends on the widget it came from so we have
    // to use "globalPos()" instead and see how it lies in relation to the child
    // widget:
    const QRect selectionListWidgetGlobalRect = QRect(mapToGlobal(mMultiSelectionListWidget.frameRect().topLeft()), mapToGlobal(mMultiSelectionListWidget.frameRect().bottomRight()));
    if (mMultiSelectionListWidget.isVisible() && selectionListWidgetGlobalRect.contains(e->globalPosition().toPoint())) {
        e->accept();
        return;
    }

    // A secondary view never centred on a room has no mRoomID but must still zoom
    TArea* pArea = mpMap->mpRoomDB->getArea(mAreaID);
    if (!pArea) {
        e->ignore();
        return;
    }

    // Increase rate if control key down - it makes scrolling through
    // a large number of items in a listwidget's contents easier (that happens
    // automagically) AND this make it easier to zoom in and out on LARGE area
    // maps
    const QPoint delta{e->angleDelta()};
    const int yDelta = qRound(delta.y() * (e->modifiers() & Qt::ControlModifier ? 5.0 : 1.0) / (8.0 * 15.0));
    if (yDelta) {
        mPick = false;
        const qreal oldZoom = xyzoom;
        // If invert zoom is enabled, use the traditional (inverted) behavior
        // Otherwise, use modern behavior (non-inverted)
        const int adjustedYDelta = mudlet::self()->invertMapZoom() ? yDelta : -yDelta;
        xyzoom = qMax(TMap::scmMinXYZoom, xyzoom * pow(1.07, adjustedYDelta));
        pArea->set2DMapZoom(xyzoom);

        if (!qFuzzyCompare(1.0 + oldZoom, 1.0 + xyzoom)) {
            const float widgetWidth = width();
            const float widgetHeight = height();
            float xs = 1.0;
            float ys = 1.0;
            if (widgetWidth > 10 && widgetHeight > 10) {
                if (widgetWidth > widgetHeight) {
                    xs = (widgetWidth / widgetHeight);
                } else {
                    ys = (widgetHeight / widgetWidth);
                }
            }

            // mouse pos within the widget
            const QPointF pos = e->position();

            // Position of the mouse within the map, scaled -1 .. +1
            // i.e. if the mouse is in the center, nothing changes
            const float dx = 2.0 * pos.x() / widgetWidth - 1.0;
            const float dy = 2.0 * pos.y() / widgetHeight - 1.0;

            // now shift the origin by that, scaled by the difference in
            // zoom factors. Thus the point under the mouse stays in place.
            mMapCenterX += dx * (oldZoom - xyzoom) / 2.0 * xs;
            mMapCenterY += dy * (oldZoom - xyzoom) / 2.0 * ys;

            flushSymbolPixmapCache();
            flushTextLabelPixmapCache();
            scheduleRender();
        }
        e->accept();
        return;
    }

    e->ignore();
}

std::pair<bool, QString> T2DMap::setMapZoom(const qreal zoom, const int areaId)
{
    // Check that everything is setup correctly
    if (!mpMap || !mpMap->mpRoomDB) {
        // This should be unreachable from the TLuaInterpreter but this cover
        // any future usage:
        return {false, qsl("no map loaded or no active mapper")};
    }

    if (const QString error = TMap::zoomError(zoom); !error.isEmpty()) {
        return {false, error};
    }

    TArea* pArea = nullptr;
    auto areaToChangeId = areaId ? areaId : mAreaID;
    // An area has been supplied - though it could be the current one:
    pArea = mpMap->mpRoomDB->getArea(areaToChangeId);
    if (!pArea) {
        // That area does not exist
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(areaId))};
    }

    auto existingZoom = pArea->get2DMapZoom();
    if (qFuzzyCompare(1.0 + existingZoom, 1.0 + zoom)) {
        // There is no detectable difference between the existing and supplied
        // zooms so do nothing, sucessfully:
        return {true, QString()};
    }

    // Store the zoom to use the next time we view that area:
    pArea->set2DMapZoom(zoom);
    if (areaId && (areaId != mAreaID)) {
        // An area was supplied but it is not the area that is being viewed
        return {true, QString()};
    }

    // We are adjusting the zoom for the currently viewed area so redraw it
    flushSymbolPixmapCache();
    flushTextLabelPixmapCache();
    update();
    return {true, QString()};
}

void T2DMap::setRoomSize(double f)
{
    rSize = f;
    if (mpHost) {
        mpHost->mRoomSize = f;
    }
    flushSymbolPixmapCache();
    flushTextLabelPixmapCache();
    update();
    mpMap->setUnsaved(__func__);
}

void T2DMap::setExitSize(double f)
{
    eSize = f;
    if (mpHost) {
        mpHost->mLineSize = f;
    }
    update();
}

void T2DMap::setBorderSize(double f)
{
    mBSize = f;
    if (mpHost) {
        mpHost->mRoomBorderSize = f;
    }
    update();
}

void T2DMap::slot_setCustomLine()
{
    if (mMultiSelectionSet.isEmpty()) {
        return;
    }
    TRoom* room = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
    if (!room) {
        return;
    }

    if (mpCustomLinesDialog) {
        // Refuse to create another instance if one is already present!
        // Just show it...
        mpCustomLinesDialog->raise();
        return;
    }

    QUiLoader loader;

    QFile file(":/ui/custom_lines.ui");
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "T2DMap: failed to open custom_lines.ui for reading:" << file.errorString();
        return;
    }
    auto* dialog = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!dialog) {
        return;
    }
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_custom_exit.png")));
    mCustomLinesRoomFrom = mMultiSelectionHighlightRoomId;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    auto* button = dialog->findChild<QPushButton*>(key_nw);
    auto* specialExits = dialog->findChild<QTreeWidget*>("specialExits");
    mpCurrentLineStyle = dialog->findChild<QComboBox*>("lineStyle");
    mpCurrentLineColor = dialog->findChild<QPushButton*>("lineColor");
    mpCurrentLineArrow = dialog->findChild<QCheckBox*>("arrow");
    if (!button || !specialExits || !mpCurrentLineColor || !mpCurrentLineStyle || !mpCurrentLineArrow) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "nw" exit line button or another element of the dialog!)");
        return;
    }
    if (room->getNorthwest() <= 0) {
        button->setCheckable(false);
        button->setDisabled(true);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_nw));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_n);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "n" exit line button!)");
        return;
    }
    if (room->getNorth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_n));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_ne);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "ne" exit line button!)");
        return;
    }
    if (room->getNortheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_ne));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_up);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "up" exit line button!)");
        return;
    }
    if (room->getUp() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_up));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_w);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "w" exit line button!)");
        return;
    }
    if (room->getWest() <= 0) {
        button->setCheckable(false);
        button->setDisabled(true);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_w));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_e);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "e" exit line button!)");
        return;
    }
    if (room->getEast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_e));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_down);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "down" exit line button!)");
        return;
    }
    if (room->getDown() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_down));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_sw);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "sw" exit line button!)");
        return;
    }
    if (room->getSouthwest() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_sw));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_s);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "s" exit line button!)");
        return;
    }
    if (room->getSouth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_s));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_se);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "se" exit line button!)");
        return;
    }
    if (room->getSoutheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_se));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_in);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "in" exit line button!)");
        return;
    }
    if (room->getIn() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_in));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_out);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "out" exit line button!)");
        return;
    }
    if (room->getOut() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_out));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    QMapIterator<QString, int> it(room->getSpecialExits());
    while (it.hasNext()) {
        it.next();
        const int id_to = it.value();
        const QString dir = it.key();
        auto pI = new QTreeWidgetItem(specialExits);
        if (room->customLines.contains(dir)) {
            pI->setCheckState(0, Qt::Checked);
        } else {
            pI->setCheckState(0, Qt::Unchecked);
        }
        pI->setTextAlignment(0, Qt::AlignHCenter);
        pI->setText(1, QString::number(id_to));
        pI->setTextAlignment(1, Qt::AlignRight);
        pI->setText(2, dir);
        pI->setTextAlignment(2, Qt::AlignLeft);
    }

    button = dialog->findChild<QPushButton*>("button_cancel");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "cancel" button!)");
        return;
    }
    connect(button, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(dialog, &QDialog::rejected, this, &T2DMap::slot_cancelCustomLineDialog);

    mpCurrentLineStyle->setIconSize(QSize(48, 24));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_solid)), tr("Solid line"), static_cast<int>(Qt::SolidLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dot)), tr("Dot line"), static_cast<int>(Qt::DotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dash)), tr("Dash line"), static_cast<int>(Qt::DashLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDot)), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDotDot)), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
    mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(mCurrentLineStyle)));

    mpCurrentLineArrow->setChecked(mCurrentLineArrow);
    mpCurrentLineColor->setStyleSheet(mudlet::self()->mBG_ONLY_STYLESHEET.arg(mCurrentLineColor.name()));
    connect(specialExits, &QTreeWidget::itemClicked, this, &T2DMap::slot_setCustomLine2B);
    connect(mpCurrentLineColor, &QAbstractButton::clicked, this, &T2DMap::slot_customLineColor);
    dialog->adjustSize();
    mpCustomLinesDialog = dialog; // Don't assign the pointer value to the class member until ready to go
    mpCustomLinesDialog->show();
    mpCustomLinesDialog->raise();
    mDialogLock = true; // Prevent any line drawing until dialog has been used
}

void T2DMap::slot_customLineColor()
{
    QColor color;
    if (mCurrentLineColor.isValid()) {
        color = QColorDialog::getColor(mCurrentLineColor, this);
    } else {
        color = QColorDialog::getColor(mpHost->mFgColor_2, this);
    }

    if (color.isValid()) {
        mCurrentLineColor = color;
        mpCurrentLineColor->setStyleSheet(mudlet::self()->mBG_ONLY_STYLESHEET.arg(color.name()));
    }
}

// Called by dialog's reject event which is caused at least by "X" button on
// title bar and by ESC keypress...
void T2DMap::slot_cancelCustomLineDialog()
{
    if (mCustomLineSession) {
        mCustomLineSession->clearOriginalPoints();
    }

    mpCustomLinesDialog->deleteLater();
    mpCustomLinesDialog = nullptr;
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    mDialogLock = false;
}

bool T2DMap::checkButtonIsForGivenDirection(const QPushButton* pButton, const QString& eKey, const int& roomId)
{
    if (pButton == mpCustomLinesDialog->findChild<QPushButton*>(eKey)) {
        mCustomLinesRoomTo = roomId;
        mCustomLinesRoomExit = eKey;
        return true;
    }

    return false;
}

void T2DMap::slot_setCustomLine2()
{
    auto pButton = qobject_cast<QPushButton*>(sender());
    if (!pButton) {
        if (mpCustomLinesDialog) {
            mpCustomLinesDialog->reject();
        } else {
            // This is needed to escape from custom line exit drawing mode if
            // the dialog has disappeared, not likely I think/hope
            mCustomLinesRoomFrom = 0;
            mCustomLinesRoomTo = 0;
            mCustomLinesRoomExit.clear();
            mDialogLock = false;
        }
        return;
    }

    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mDialogLock = false;
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if (!room) {
        return;
    }
    // The button texts are going to be subject to translation so we need to go
    // for their addresses instead:
    bool isFound = checkButtonIsForGivenDirection(pButton, key_nw, room->getNorthwest());

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_n, room->getNorth());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_ne, room->getNortheast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_up, room->getUp());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_w, room->getWest());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_e, room->getEast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_down, room->getDown());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_sw, room->getSouthwest());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_s, room->getSouth());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_se, room->getSoutheast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_in, room->getIn());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_out, room->getOut());
    }

    if (!isFound) {
        qWarning() << "T2DMap::slot_setCustomLine2() ERROR - unable to identify exit to use for the button with text:" << pButton->text() << "!)";
        return;
    }

    QList<QPointF> const list;
    room->customLines[mCustomLinesRoomExit] = list;
    // Nothing on this path calls calcRoomDimensions(), which would otherwise index the (still empty) line.
    room->indexCustomLines();
    //    qDebug("T2DMap::slot_setCustomLine2() NORMAL EXIT: %s", qPrintable(exitKey));
    room->customLinesColor[mCustomLinesRoomExit] = mCurrentLineColor;
    /*
     *    qDebug("   COLOR(r,g,b): %i,%i,%i",
     *            mCurrentLineColor.red(),
     *            mCurrentLineColor.green(),
     *            mCurrentLineColor.blue() );
     */
    room->customLinesStyle[mCustomLinesRoomExit] = mCurrentLineStyle;
    //    qDebug("   LINE STYLE: %d", mCurrentLineStyle);
    room->customLinesArrow[mCustomLinesRoomExit] = mCurrentLineArrow;
    //    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");
    mpMap->setUnsaved(__func__);

    //: 2D Mapper big, bottom of screen help message
    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    update();
}

void T2DMap::slot_setCustomLine2B(QTreeWidgetItem* special_exit, int column)
{
    Q_UNUSED(column)
    if (!special_exit) {
        return;
    }
    const QString exit = special_exit->text(2);
    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mCustomLinesRoomExit = exit;
    mCustomLinesRoomTo = special_exit->text(1).toInt(); // Wasn't being set !
    mDialogLock = false;
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if (!room) {
        return;
    }
    QList<QPointF> const _list;
    room->customLines[exit] = _list;
    // Nothing on this path calls calcRoomDimensions(), which would otherwise index the (still empty) line.
    room->indexCustomLines();
    //    qDebug("T2DMap::slot_setCustomLine2B() SPECIAL EXIT: %s", qPrintable(exit));
    room->customLinesColor[exit] = mCurrentLineColor;
    /*
     *     qDebug("   COLOR(r,g,b): %i,%i,%i",
     *            mCurrentLineColor.red(),
     *            mCurrentLineColor.green(),
     *            mCurrentLineColor.blue() );
     */
    room->customLinesStyle[exit] = mCurrentLineStyle;
    //    qDebug("   LINE STYLE: %d", mCurrentLineStyle);
    room->customLinesArrow[exit] = mCurrentLineArrow;
    //    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");
    mpMap->setUnsaved(__func__);
    //: 2D Mapper big, bottom of screen help message
    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    update();
}

void T2DMap::slot_createLabel()
{
    if (!mpMap->mpRoomDB->getArea(mAreaID)) {
        return;
    }

    //: 2D Mapper big, bottom of screen help message
    mHelpMsg = tr("Left-click and drag a square for the size and position of your label");
    mSizeLabel = true;
    mMultiSelection = true;
    update();
}

void T2DMap::slot_roomSelectionChanged()
{
    QList<QTreeWidgetItem*> const selection = mMultiSelectionListWidget.selectedItems();
    mMultiSelectionSet.clear();
    for (auto treeWidgetItem : selection) {
        const int currentRoomId = treeWidgetItem->text(0).toInt();
        mMultiSelectionSet.insert(currentRoomId);
    }
    switch (mMultiSelectionSet.size()) {
    case 0:
        mMultiSelectionHighlightRoomId = 0;
        break;
    case 1:
        mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.constBegin());
        break;
    default:
        getCenterSelection();
    }
    update();
}

void T2DMap::resizeMultiSelectionWidget()
{
    int newWidth;
    if (mIsSelectionUsingNames) {
        if (width() <= 750) {
            newWidth = 160;
        } else if (width() <= 890) { // 750-890 => 160-300
            newWidth = 160 + width() - 750;
        } else { // 890+ => 300
            newWidth = 300;
        }
    } else {
        if (width() <= 300) { // 0 - 300 => 0 - 120
            newWidth = 2 * width() / 3;
        } else { // 300+ => 120
            newWidth = 120;
        }
    }
    int _newHeight = 300;
    if (mMultiSelectionListWidget.topLevelItemCount() > 0) {
        QTreeWidgetItem* rowItem = mMultiSelectionListWidget.topLevelItem(1);
        // The following factors are tweaks to ensure that the widget shows all
        // the rows, as the header seems bigger than the value returned, static values
        // used to enable values to be changed by debugger at runtime!
        static const float headerFactor = 1.2;
        static const float rowFactor = 1.0;
        _newHeight = headerFactor * mMultiSelectionListWidget.header()->height();
        if (rowItem) { // Have some data rows - and we have forced them to be the same height:
            _newHeight += rowFactor * mMultiSelectionListWidget.topLevelItemCount() * mMultiSelectionListWidget.visualItemRect(rowItem).height();
        }
    }
    if (_newHeight < height()) {
        mMultiSelectionListWidget.resize(newWidth, _newHeight);
    } else {
        mMultiSelectionListWidget.resize(newWidth, height());
    }
}

void T2DMap::setPlayerRoomStyle(const int type)
{
    if (!mpMap) {
        return;
    }

    mPlayerRoomColorGradientStops = RoomAppearance::playerRoomStops(type, mpMap->mPlayerRoomInnerDiameterPercentage, mpMap->mPlayerRoomInnerColor, mpMap->mPlayerRoomOuterColor);
}

void T2DMap::clearSelection()
{
    if (!mMultiSelection && !mMultiSelectionSet.isEmpty()) {
        mMultiSelectionSet.clear();
        mMultiSelectionHighlightRoomId = 0;
        hideSelectionWidget();
        update();
    }
}

void T2DMap::forgetDeletedRoom(const int roomId)
{
    // A drag still in progress rebuilds the selection from these on its next move
    mMultiSelectionBaseSet.remove(roomId);
    mMultiSelectionAnchorSet.remove(roomId);
    if (mCustomLinesRoomFrom == roomId || mCustomLinesRoomTo == roomId) {
        // The id can be reused, and the next click would add a point to that room;
        // deleting the room the line leads to has already removed its exit
        if (mpCustomLinesDialog) {
            mpCustomLinesDialog->reject();
        } else {
            mCustomLinesRoomFrom = 0;
            mCustomLinesRoomTo = 0;
            mCustomLinesRoomExit.clear();
        }
        mHelpMsg.clear();
        update();
    }
    if (mCustomLineSelectedRoom == roomId) {
        mCustomLineSelectedRoom = 0;
        mCustomLineSelectedExit.clear();
        mCustomLineSelectedPoint = -1;
        update();
    }
    // Queued so that deleting many rooms walks the list once, not once per room.
    // A hidden list is rebuilt before it is shown again.
    if (!mMultiSelectionListWidget.isHidden() && mMultiSelectionListWidget.topLevelItemCount() > 0) {
        if (mRoomsToDropFromSelectionWidget.isEmpty()) {
            QTimer::singleShot(0, this, &T2DMap::pruneDeletedRoomsFromSelectionWidget);
        }
        mRoomsToDropFromSelectionWidget.insert(roomId);
    }
    if (!mMultiSelectionSet.remove(roomId)) {
        return;
    }
    if (mMultiSelectionHighlightRoomId == roomId) {
        switch (mMultiSelectionSet.size()) {
        case 0:
            mMultiSelectionHighlightRoomId = 0;
            break;
        case 1:
            mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.constBegin());
            break;
        default:
            getCenterSelection();
        }
    }
    update();
}

void T2DMap::pruneDeletedRoomsFromSelectionWidget()
{
    // By id, not by getRoom(): a script may already have made a new room with a deleted one's id
    const QSet<int> deletedRoomIds = mRoomsToDropFromSelectionWidget;
    mRoomsToDropFromSelectionWidget.clear();
    bool rowsRemoved = false;
    // Removing a selected row would otherwise rebuild mMultiSelectionSet from the rows left
    mMultiSelectionListWidget.blockSignals(true);
    for (int row = mMultiSelectionListWidget.topLevelItemCount() - 1; row >= 0; --row) {
        if (deletedRoomIds.contains(mMultiSelectionListWidget.topLevelItem(row)->text(0).toInt())) {
            delete mMultiSelectionListWidget.takeTopLevelItem(row);
            rowsRemoved = true;
        }
    }
    mMultiSelectionListWidget.blockSignals(false);
    if (mMultiSelectionListWidget.topLevelItemCount() < 2) {
        hideSelectionWidget();
        return;
    }
    if (!rowsRemoved) {
        return;
    }
    mIsSelectionUsingNames = false;
    for (int row = 0; row < mMultiSelectionListWidget.topLevelItemCount(); ++row) {
        if (!mMultiSelectionListWidget.topLevelItem(row)->text(1).isEmpty()) {
            mIsSelectionUsingNames = true;
            break;
        }
    }
    mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames);
    resizeMultiSelectionWidget();
}

void T2DMap::hideSelectionWidget()
{
    mMultiSelectionListWidget.hide();
    // clear() makes the widget emit itemSelectionChanged() which would in turn
    // call update() - not something we want when this is reached from within
    // paintEvent():
    mMultiSelectionListWidget.blockSignals(true);
    mMultiSelectionListWidget.clear();
    mMultiSelectionListWidget.blockSignals(false);
}

// Drops the selected rooms that are not in the area that is about to be shown.
// Without this a selection would survive an area change but be off-screen, so
// anything done to it - deleting those rooms, say - would happen unseen.
void T2DMap::pruneRoomSelectionToArea(const int areaId)
{
    if (mMultiSelectionSet.isEmpty() || !mpMap || !mpMap->mpRoomDB) {
        return;
    }

    QSet<int> roomsInNewArea;
    for (const int roomId : std::as_const(mMultiSelectionSet)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (pRoom && pRoom->getArea() == areaId) {
            roomsInNewArea.insert(roomId);
        }
    }

    if (roomsInNewArea.size() == mMultiSelectionSet.size()) {
        // Everything selected has come along to the new area - as happens when
        // the rooms themselves were just moved into it:
        return;
    }

    mMultiSelectionSet = roomsInNewArea;
    if (!mMultiSelectionSet.contains(mMultiSelectionHighlightRoomId)) {
        switch (mMultiSelectionSet.size()) {
        case 0:
            mMultiSelectionHighlightRoomId = 0;
            break;
        case 1:
            mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.constBegin());
            break;
        default:
            getCenterSelection();
        }
    }

    // The listing is now stale; it gets rebuilt by updateSelectionWidget() on
    // the next mouse interaction with what, if anything, is still selected:
    hideSelectionWidget();
}

std::pair<bool, QString> T2DMap::exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels)
{
    if (!mpMap || mpHost.isNull()) {
        return {false, qsl("Map not initialized")};
    }

    QList<std::pair<int, QString>> levelExports;
    if (const QString problem = mpMap->areaExportProblem(areaId, filePath, zoom, exportAllZLevels, levelExports); !problem.isEmpty()) {
        return {false, problem};
    }

    // Handle exporting all Z levels if requested
    if (exportAllZLevels) {
        for (const auto& [currentZLevel, levelFileName] : levelExports) {
            // Recursively call this function for each Z level (without exportAllZLevels flag)
            auto [success, message] = exportAreaToImage(areaId, levelFileName, currentZLevel, zoom, false);
            if (!success) {
                return {false, qsl("Failed to export Z level %1: %2").arg(currentZLevel).arg(message)};
            }
        }

        return {true, {}};
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(areaId);
    // Calculate area bounds
    pArea->calcSpan();

    const int areaWidth = pArea->max_x - pArea->min_x + 1;
    const int areaHeight = pArea->max_y - pArea->min_y + 1;


    if (areaWidth <= 0 || areaHeight <= 0) {
        return {false, qsl("Area %1 has invalid dimensions").arg(areaId)};
    }

    // Calculate room size based on area dimensions - auto-sizing approach
    // Aim for reasonable room sizes that fit well regardless of area size
    const int targetImageDimension = 1024; // Target around 1024 pixels for the larger dimension
    const int maxDimension = qMax(areaWidth, areaHeight);
    const int roomSize = targetImageDimension / maxDimension;
    const int minRoomSize = 8;
    const int maxRoomSize = 50; // Prevent rooms from being too large
    const int finalRoomSize = qBound(minRoomSize, roomSize, maxRoomSize);

    // Calculate image size based on actual area content
    const int padding = finalRoomSize * 2;
    const int imageWidth = areaWidth * finalRoomSize + padding * 2;
    const int imageHeight = areaHeight * finalRoomSize + padding * 2;

    // Calculate offsets to position rooms correctly in the image
    // Map the area bounds to the image with padding, accounting for Y-axis flip
    const int exportRX = padding - (pArea->min_x * finalRoomSize);
    const int exportRY = padding - (pArea->min_y * finalRoomSize);


    // Create high-quality image for crisp exports using specified zoom
    QPixmap pixmap(imageWidth * zoom, imageHeight * zoom);
    pixmap.setDevicePixelRatio(zoom);
    pixmap.fill(mpHost->mBgColor_2);

    QPainter painter(&pixmap);
    if (!painter.isActive()) {
        return {false, qsl("Failed to create image painter")};
    }

    // Configure painter
    if (mMapperUseAntiAlias) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
    }

    // Store original coordinate system values to restore later
    const int originalRX = mRX;
    const int originalRY = mRY;
    const float originalRoomWidth = mRoomWidth;
    const float originalRoomHeight = mRoomHeight;

    // Set class member variables to export coordinate system (Qt handles scaling via devicePixelRatio)
    mRX = exportRX;
    mRY = exportRY;
    // Set mRoomWidth/mRoomHeight for high-quality rendering (Qt handles scaling)
    mRoomWidth = finalRoomSize;
    mRoomHeight = finalRoomSize;

    // Set up fonts and pens - use similar approach to paintEvent
    QFont roomVNumFont = mpMap->mMapSymbolFont;
    QFont mapNameFont = mpMap->mMapSymbolFont;

    // Get room size factors from host
    const float eSize = mpHost->mLineSize;
    const float rSize = mpHost->mRoomSize;

    // Calculate font size based on room dimensions
    const int fontPixelSize = qMax(6, static_cast<int>(mRoomHeight * rSize / 4.0));
    roomVNumFont.setPixelSize(fontPixelSize);
    mapNameFont.setPixelSize(fontPixelSize);

    // Reset symbol font size to match paintEvent behavior (line 1185)
    mPainter.resetSymbolFontSize();

    // Flush symbol cache to ensure symbols are regenerated with export dimensions
    flushSymbolPixmapCache();
    mPainter.setView(painterView());

    // Calculate if room IDs should be legible based on export room size
    // Similar to logic in paintEvent, but using export dimensions
    QRectF roomRect(0, 0, finalRoomSize, finalRoomSize);
    QFont testFont = roomVNumFont;
    bool areRoomIdsLegible = mShowRoomID && TMapPainter::sizeFontToFitTextInRect(testFont, roomRect, qsl("12345"), 10, 7.0);

    QPen pen;
    pen.setColor(mpHost->mFgColor_2);
    // Use the same exitWidth calculation as paintEvent (Qt handles scaling via devicePixelRatio)
    const float exitWidth = 1 / eSize * finalRoomSize * rSize;
    pen.setWidthF(exitWidth);

    // Use the same Z-level filtering and drawing approach as paintEvent
    const int exportZLevel = zLevel.has_value() ? zLevel.value() : mMapCenterZ;

    // Draw the ("background") labels that are on the bottom of the map - same as paintEvent
    QMutableMapIterator<int, TMapLabel> itMapLabel(pArea->mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        auto mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != exportZLevel) {
            continue;
        }
        // Use export coordinate system for label positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * finalRoomSize + exportRX;
        const int labelY = mapLabel.pos.y() * finalRoomSize * -1 + exportRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * finalRoomSize));
        const int labelHeight = abs(qRound(mapLabel.size.height() * finalRoomSize));
        if (!((0 < labelX || 0 < labelX + labelWidth) && (imageWidth > labelX || imageWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (imageHeight > labelY || imageHeight > labelY + labelHeight))) {
            continue;
        }

        QRectF labelPaintRectangle = QRect(labelX, labelY, labelWidth, labelHeight);
        if (!mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                TMapPainter::drawPixmapScaledTo(painter, QRectF(labelPosition, labelPaintRectangle.size()), mapLabel.pix);
                mapLabel.clickSize = QSizeF(labelPaintRectangle.width(), labelPaintRectangle.height());
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
            pArea->mMapLabels[itMapLabel.key()] = mapLabel;
        }

        if (mapLabel.highlight) {
            TMapPainter::paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    QList<int> exitList;
    QList<int> oneWayExits;

    // Build exit lists from rooms on current Z-level (like paintEvent does)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }
        exitList << roomId;
    }


    // Now draw rooms on Z-level and below (like paintEvent does)
    int roomsDrawn = 0;
    int roomsSkipped = 0;

    // First pass: draw rooms on level below (like paintEvent shadow rooms)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel - 1)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        if (rx >= 0 && ry >= 0 && rx <= imageWidth && ry <= imageHeight) {
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(mpHost->mLowerLevelColor));

            // Draw shadow room using the same approach as paintEvent (lines 1426-1442)
            const qreal scaledRoomSize = static_cast<qreal>(finalRoomSize) * rSize;
            if (mBubbleMode) {
                const QPointF roomCenter(rx, ry);
                const qreal roomRadius = scaledRoomSize / 2.0;
                QPainterPath diameterPath;
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            } else {
                const QRectF shadowRect(rx - (scaledRoomSize * 0.8), ry - (scaledRoomSize * 0.2), scaledRoomSize, scaledRoomSize);
                painter.drawRect(shadowRect);
            }
            painter.restore();
            roomsDrawn++;
        }
    }

    // Second pass: draw rooms on level above (like paintEvent upper level rooms)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel + 1)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        if (rx >= 0 && ry >= 0 && rx <= imageWidth && ry <= imageHeight) {
            painter.save();
            painter.setPen(QPen(mpHost->mUpperLevelColor, 1));
            painter.setBrush(Qt::transparent);

            // Draw outline room using the same approach as paintEvent (lines 1456-1471)
            if (mBubbleMode) {
                const float roomRadius = 0.5 * rSize * finalRoomSize;
                const QPointF roomCenter = QPointF(rx + (roomRadius * rSize * 0.5), ry - (roomRadius * rSize * 0.5));
                QPainterPath diameterPath;
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            } else {
                painter.drawRect(rx - (finalRoomSize * rSize * 0.2), ry - (finalRoomSize * rSize * 0.8), finalRoomSize * rSize, finalRoomSize * rSize);
            }
            painter.restore();
            roomsDrawn++;
        }
    }

    // Third pass: draw room exits (moved to correct position after upper-level rooms)
    if (!pArea->gridMode) {
        // paintRoomExits uses width() and height() for visibility filtering, but we need to use imageWidth/imageHeight
        // Since we can't easily override those methods, we'll create a custom export-specific exit drawing

        // Draw exits using similar logic to paintRoomExits but with export-specific coordinates
        QSetIterator<int> itRoom2(pArea->getRoomsForZ(exportZLevel));
        while (itRoom2.hasNext()) {
            const int roomId = itRoom2.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
            if (!room) {
                continue;
            }

            const float rx = room->x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
            const float ry = room->y() * -1 * finalRoomSize + (padding - (pArea->min_y * finalRoomSize));

            // Skip rooms that are outside the image bounds
            if (rx < 0 || ry < 0 || rx > imageWidth || ry > imageHeight) {
                continue;
            }

            room->rendered = true;

            // First, draw custom exit lines (if any) - exact logic from paintRoomExits
            if (!room->customLines.empty()) {
                painter.save();
                QMapIterator<QString, QList<QPointF>> itk(room->customLines);
                while (itk.hasNext()) {
                    itk.next();
                    const QString exitKey = itk.key();
                    QColor customLineColor;
                    if (roomId == mCustomLineSelectedRoom && exitKey == mCustomLineSelectedExit) {
                        customLineColor = QColor(255, 155, 55);
                    } else {
                        customLineColor = room->customLinesColor.value(exitKey, Qt::red);
                    }

                    const QPointF origin = QPointF(rx, ry);

                    // Calculate fixed offset point based on exit direction (same logic as paintRoomExits)
                    QPointF fixedOffsetPoint;
                    bool isXYPlainExit = false;
                    if (exitKey == "n") {
                        fixedOffsetPoint = QPointF(rx, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "ne") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "e") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry);
                        isXYPlainExit = true;
                    } else if (exitKey == "se") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "s") {
                        fixedOffsetPoint = QPointF(rx, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "sw") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "w") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry);
                        isXYPlainExit = true;
                    } else if (exitKey == "nw") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else {
                        fixedOffsetPoint = QPointF(rx, ry);
                    }

                    QPen customLinePen = painter.pen();
                    customLinePen.setCosmetic(mMapperUseAntiAlias);
                    customLinePen.setWidthF(exitWidth);
                    customLinePen.setCapStyle(Qt::RoundCap);
                    customLinePen.setJoinStyle(Qt::RoundJoin);
                    customLinePen.setColor(customLineColor);
                    customLinePen.setStyle(room->customLinesStyle.value(exitKey, Qt::SolidLine));

                    QVector<QPointF> polyLinePoints;
                    const QList<QPointF> customLinePoints = itk.value();
                    QLineF doorLineSegment;

                    if (!customLinePoints.empty()) {
                        painter.setPen(customLinePen);
                        polyLinePoints << origin;
                        polyLinePoints << fixedOffsetPoint;

                        // Transform custom line points (same as original: customLinePoint.x() * mRoomWidth + mRX)
                        for (const QPointF& customLinePoint : customLinePoints) {
                            const float pointX = customLinePoint.x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
                            const float pointY = customLinePoint.y() * finalRoomSize * -1 + (padding - (pArea->min_y * finalRoomSize));
                            polyLinePoints << QPointF(pointX, pointY);
                        }

                        // Determine door line segment for door drawing (same logic as original)
                        if (polyLinePoints.size() > 2) {
                            if (isXYPlainExit) {
                                doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                            } else {
                                // Non-XY-Plane exits have the first two points being coincident:
                                doorLineSegment = QLineF{polyLinePoints.at(1), polyLinePoints.at(2)};
                            }
                        } else {
                            // There must be 2 points ...
                            doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                        }

                        // Draw the custom polyline
                        painter.drawPolyline(polyLinePoints.data(), polyLinePoints.size());

                        // Draw arrow if needed (same logic as original)
                        if (room->customLinesArrow.value(exitKey, false)) {
                            QLineF arrowLine = QLineF(polyLinePoints.last(), polyLinePoints.at(polyLinePoints.size() - 2));
                            arrowLine.setLength(exitWidth * 5.0);

                            QLineF arrowHead1 = arrowLine;
                            arrowHead1.setLength(exitWidth * 3.0);
                            arrowHead1.setAngle(arrowLine.angle() + 20);

                            QLineF arrowHead2 = arrowLine;
                            arrowHead2.setLength(exitWidth * 3.0);
                            arrowHead2.setAngle(arrowLine.angle() - 20);

                            painter.drawLine(arrowHead1);
                            painter.drawLine(arrowHead2);
                        }

                        // Draw door if exists on this custom line
                        if (room->doors.value(exitKey, 0) > 0) {
                            mPainter.drawDoor(painter, *room, exitKey, doorLineSegment);
                        }
                    }
                }
                painter.restore();
            }

            // Now draw regular exits - use exact same logic as paintRoomExits
            QList<int> roomExitList;
            QList<int> roomOneWayExits;

            // Helper function to add exit and check for one-way
            auto addExitAndCheckOneWay = [&](const QString& dirKey, int exitRoomId) {
                if (exitRoomId > 0) {
                    roomExitList.push_back(exitRoomId);
                    TRoom* exitRoom = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (exitRoom) {
                        // Check if it's a one-way exit (return direction doesn't point back)
                        bool isOneWay = false;
                        if (dirKey == "n" && exitRoom->getSouth() != roomId)
                            isOneWay = true;
                        else if (dirKey == "s" && exitRoom->getNorth() != roomId)
                            isOneWay = true;
                        else if (dirKey == "e" && exitRoom->getWest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "w" && exitRoom->getEast() != roomId)
                            isOneWay = true;
                        else if (dirKey == "ne" && exitRoom->getSouthwest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "nw" && exitRoom->getSoutheast() != roomId)
                            isOneWay = true;
                        else if (dirKey == "se" && exitRoom->getNorthwest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "sw" && exitRoom->getNortheast() != roomId)
                            isOneWay = true;

                        if (isOneWay) {
                            roomOneWayExits.push_back(exitRoomId);
                        }
                    }
                }
            };

            // Only add regular exits for directions that don't have custom lines (like paintRoomExits)
            if (!room->customLines.empty()) {
                // This room has custom lines - only add regular exits for directions without custom lines
                if (!room->customLines.contains("n")) {
                    addExitAndCheckOneWay("n", room->getNorth());
                }
                if (!room->customLines.contains("ne")) {
                    addExitAndCheckOneWay("ne", room->getNortheast());
                }
                if (!room->customLines.contains("e")) {
                    addExitAndCheckOneWay("e", room->getEast());
                }
                if (!room->customLines.contains("se")) {
                    addExitAndCheckOneWay("se", room->getSoutheast());
                }
                if (!room->customLines.contains("s")) {
                    addExitAndCheckOneWay("s", room->getSouth());
                }
                if (!room->customLines.contains("sw")) {
                    addExitAndCheckOneWay("sw", room->getSouthwest());
                }
                if (!room->customLines.contains("w")) {
                    addExitAndCheckOneWay("w", room->getWest());
                }
                if (!room->customLines.contains("nw")) {
                    addExitAndCheckOneWay("nw", room->getNorthwest());
                }
            } else {
                // This room has no custom lines - add all regular exits (only 8 cardinal/diagonal directions)
                addExitAndCheckOneWay("n", room->getNorth());
                addExitAndCheckOneWay("s", room->getSouth());
                addExitAndCheckOneWay("e", room->getEast());
                addExitAndCheckOneWay("w", room->getWest());
                addExitAndCheckOneWay("ne", room->getNortheast());
                addExitAndCheckOneWay("nw", room->getNorthwest());
                addExitAndCheckOneWay("se", room->getSoutheast());
                addExitAndCheckOneWay("sw", room->getSouthwest());
            }

            // Draw regular exit lines to destination rooms, leaving out those a straight custom line already draws
            for (const int exitRoomId : roomExitList) {
                if (exitRoomId <= 0)
                    continue;

                TRoom* exitRoom = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (!exitRoom)
                    continue;

                // Determine if this is an area exit (like original paintRoomExits)
                const bool areaExit = exitRoom->getArea() != areaId;
                const float exitArrowScale = (mLargeAreaExitArrows ? 2.0f : 1.0f);

                QLineF exitLine;
                QString doorKey;

                // Determine door key and exit line based on direction
                if (room->getNorth() == exitRoomId) {
                    doorKey = "n";
                    if (areaExit) {
                        exitLine = QLineF(rx, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSouth() == exitRoomId) {
                    doorKey = "s";
                    if (areaExit) {
                        exitLine = QLineF(rx, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getEast() == exitRoomId) {
                    doorKey = "e";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry, rx, ry);
                    }
                } else if (room->getWest() == exitRoomId) {
                    doorKey = "w";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry, rx, ry);
                    }
                } else if (room->getNortheast() == exitRoomId) {
                    doorKey = "ne";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getNorthwest() == exitRoomId) {
                    doorKey = "nw";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSoutheast() == exitRoomId) {
                    doorKey = "se";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSouthwest() == exitRoomId) {
                    doorKey = "sw";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                }

                if (!areaExit) {
                    // Same-area exit: draw line to destination room (original logic)
                    const float exitRoomX = exitRoom->x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
                    const float exitRoomY = exitRoom->y() * -1 * finalRoomSize + (padding - (pArea->min_y * finalRoomSize));

                    // Skip if destination room is outside the image bounds
                    if (exitRoomX < 0 || exitRoomY < 0 || exitRoomX > imageWidth || exitRoomY > imageHeight) {
                        continue;
                    }

                    exitLine = QLineF(rx, ry, exitRoomX, exitRoomY);

                    if (roomOneWayExits.contains(exitRoomId)) {
                        // Draw one-way exit with dashed line
                        QPen oneWayPen = pen;
                        oneWayPen.setStyle(Qt::DashLine);
                        painter.setPen(oneWayPen);
                        painter.drawLine(exitLine);
                    } else {
                        // Also for a line left out: the door drawn below takes its width from this pen
                        painter.setPen(pen);
                        if (!TMapPainter::customLineRunsStraightTo(*room, *exitRoom) && !(exitRoom->z() == exportZLevel && TMapPainter::customLineRunsStraightTo(*exitRoom, *room))) {
                            painter.drawLine(exitLine);
                        }
                    }

                } else {
                    // Area exit: draw directional stub with arrow (like original paintRoomExits)
                    painter.save();
                    QPen areaPen = pen;
                    areaPen.setWidthF(exitWidth);
                    areaPen.setCapStyle(Qt::RoundCap);
                    areaPen.setCosmetic(mMapperUseAntiAlias);
                    areaPen.setColor(mpMap->getColor(exitRoomId));

                    // Draw outline first for visibility on matching backgrounds
                    QPen outlinePen = areaPen;
                    outlinePen.setWidthF(exitWidth + 2.0);
                    outlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                    painter.setPen(outlinePen);
                    painter.drawLine(exitLine);

                    // Draw colored line on top
                    painter.setPen(areaPen);
                    painter.drawLine(exitLine);

                    // Draw arrow head (same logic as original paintRoomExits lines 2298-2333)
                    QLineF arrowLine = QLineF(exitLine);
                    if (mLargeAreaExitArrows) {
                        arrowLine.setLength((finalRoomSize * 2.0) * 0.4);
                    } else {
                        arrowLine.setLength(exitWidth * 5.0);
                    }
                    const QPointF arrowTip = arrowLine.p1();
                    const QPointF arrowBase = arrowLine.p2();
                    const QLineF arrowBaseLine = QLineF(arrowLine);
                    const qreal arrowAngle = arrowBaseLine.angle() - 90.0;

                    QLineF arrowSide;
                    arrowSide.setP1(arrowBase);
                    arrowSide.setAngle(arrowAngle);
                    if (mLargeAreaExitArrows) {
                        arrowSide.setLength((finalRoomSize * 2.0) * 0.15);
                    } else {
                        arrowSide.setLength(exitWidth * 2.0);
                    }
                    const QPointF arrowLeft = arrowSide.p2();
                    arrowSide.setAngle(arrowSide.angle() + 180.0);
                    const QPointF arrowRight = arrowSide.p2();

                    QPolygonF arrowHead;
                    arrowHead.append(arrowTip);
                    arrowHead.append(arrowLeft);
                    arrowHead.append(arrowRight);

                    QBrush arrowBrush = painter.brush();
                    arrowBrush.setColor(mpMap->getColor(exitRoomId));
                    arrowBrush.setStyle(Qt::SolidPattern);
                    QPen arrowPen = painter.pen();
                    arrowPen.setJoinStyle(Qt::RoundJoin);
                    arrowPen.setCapStyle(Qt::RoundCap);
                    arrowPen.setCosmetic(mMapperUseAntiAlias);

                    // Draw outline first for visibility
                    QPen arrowOutlinePen = arrowPen;
                    arrowOutlinePen.setWidthF(2.0);
                    arrowOutlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                    painter.setPen(arrowOutlinePen);
                    painter.setBrush(Qt::NoBrush);
                    painter.drawPolygon(arrowHead);

                    // Draw filled arrow head on top
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(arrowBrush);
                    painter.drawPolygon(arrowHead);
                    painter.restore();
                }

                // Draw door if exists on this exit (both area and same-area exits)
                if (!doorKey.isEmpty() && room->doors.value(doorKey, 0) > 0) {
                    // Apply half-length reduction like original paintRoomExits
                    QLineF halfExitLine = exitLine;
                    halfExitLine.setLength(halfExitLine.length() / 2.0);
                    mPainter.drawDoor(painter, *room, doorKey, halfExitLine);
                }
            }
        }
    }

    // Fourth pass: draw main rooms on current level using existing drawRoom method
    const QList<int> roomsOnLevel = pArea->getRoomsForZ(exportZLevel).values();
    const bool layerRooms = mPainter.anyRoomHighlighted(roomsOnLevel);
    QList<QPair<TRoom*, QPointF>> layeredRooms;
    for (const int roomId : roomsOnLevel) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            roomsSkipped++;
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        // Debug: Check which rooms are being filtered out
        if (rx < 0 || ry < 0 || rx > imageWidth || ry > imageHeight) {
            roomsSkipped++;
            continue;
        }

        // Use the existing drawRoom method!
        mPainter.drawRoom(painter,
                          roomVNumFont,
                          mapNameFont,
                          pen,
                          pRoom,
                          pArea->gridMode,
                          areRoomIdsLegible,
                          false /* showRoomNames */,
                          -1 /* speedWalkStartRoomId */,
                          rx,
                          ry,
                          false /* showRoomCollision */,
                          layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole);
        if (layerRooms) {
            layeredRooms.append({pRoom, QPointF(rx, ry)});
        }

        roomsDrawn++;
    }
    if (layerRooms) {
        mPainter.drawRoomHighlightsAndLabels(painter, roomVNumFont, mapNameFont, pen, layeredRooms, pArea->gridMode, areRoomIdsLegible, false /* showRoomNames */, -1 /* speedWalkStartRoomId */);
    }
    takePainterResults();
    startPendingSpeedWalk();


    // Draw the ("foreground") labels that are on the top of the map - same as paintEvent
    itMapLabel.toFront(); // Reset the iterator
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        auto mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != exportZLevel) {
            continue;
        }
        // Use export coordinate system for label positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const int labelX = mapLabel.pos.x() * finalRoomSize + exportRX;
        const int labelY = mapLabel.pos.y() * finalRoomSize * -1 + exportRY;
        const int labelWidth = abs(qRound(mapLabel.size.width() * finalRoomSize));
        const int labelHeight = abs(qRound(mapLabel.size.height() * finalRoomSize));

        if (!((0 < labelX || 0 < labelX + labelWidth) && (imageWidth > labelX || imageWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (imageHeight > labelY || imageHeight > labelY + labelHeight))) {
            continue;
        }
        QRectF labelPaintRectangle = QRect(labelX, labelY, labelWidth, labelHeight);
        if (mapLabel.showOnTop) {
            QPointF labelPosition(labelX, labelY);
            if (!mapLabel.noScaling) {
                TMapPainter::drawPixmapScaledTo(painter, QRectF(labelPosition, labelPaintRectangle.size()), mapLabel.pix);
                mapLabel.clickSize = QSizeF(labelPaintRectangle.width(), labelPaintRectangle.height());
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSize(mapLabel.pix.width(), mapLabel.pix.height());
            }
            pArea->mMapLabels[itMapLabel.key()] = mapLabel;
        }
        if (mapLabel.highlight) {
            TMapPainter::paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    painter.end();

    // Restore the original coordinate system
    mRX = originalRX;
    mRY = originalRY;
    mRoomWidth = originalRoomWidth;
    mRoomHeight = originalRoomHeight;

    // Each export has a watcher of its own: several can be in flight at once,
    // from calls made back to back or one per z level
    auto* pWatcher = new QFutureWatcher<std::pair<bool, QString>>(this);
    connect(pWatcher, &QFutureWatcher<std::pair<bool, QString>>::finished, this, [this, pWatcher, profileName = mpHost->getName()]() {
        const auto [saved, errorMessage] = pWatcher->result();
        if (!saved) {
            // This view can outlive its profile, which may have closed before the save reported back
            if (mpHost) {
                mpHost->postMessage(tr("[MAP]: %1").arg(errorMessage));
            } else {
                qWarning().noquote().nospace() << "T2DMap::exportAreaToImage() WARNING - profile \"" << profileName << "\" closed before this export finished: " << errorMessage;
            }
        }
        pWatcher->deleteLater();
    });

    auto future = QtConcurrent::task(&T2DMap::performImageSave).withArguments(this, pixmap, filePath, TMap::exportImageFormat(filePath)).spawn();
    pWatcher->setFuture(future);

    return {true, {}};
}

std::pair<bool, QString> T2DMap::performImageSave(const QPixmap& pixmap, const QString& filePath, const QString& format)
{
    if (!pixmap.save(filePath, format.toLocal8Bit().data())) {
        return {false, qsl("Failed to save image to %1").arg(filePath)};
    }

    return {true, {}};
}

void T2DMap::slot_exportAreaToImage()
{
    if (!mpMap || mpHost.isNull()) {
        return;
    }

    QString areaName = tr("Unknown Area");
    TArea* pArea = mpMap->mpRoomDB->getArea(mAreaID);
    if (pArea) {
        areaName = mpMap->mpRoomDB->getAreaNamesMap().value(mAreaID, areaName);
    }

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastExportAreaImageDirectory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();

    QString defaultFileName;
    if (!areaName.isEmpty()) {
        // Use sanitized area name for filename
        defaultFileName = qsl("%1.png").arg(MudletApp::sanitizeForPath(areaName));
    } else {
        // Fall back to area ID if no area name
        defaultFileName = qsl("area_%1.png").arg(mAreaID);
    }

    QString fullPath = qsl("%1/%2").arg(lastDir, defaultFileName);
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Area %1 to Image").arg(areaName), fullPath, tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tiff);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    // Remember the directory for next time
    QString selectedDir = QFileInfo(filePath).absolutePath();
    settings.setValue("lastExportAreaImageDirectory", selectedDir);

    auto result = exportAreaToImage(mAreaID, filePath);

    if (!result.first) {
        mpHost->postMessage(tr("[MAP]: Export failed - %1").arg(result.second));
    }
}
