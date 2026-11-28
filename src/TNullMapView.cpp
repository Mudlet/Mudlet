/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TNullMapView.h"

#include "Host.h"
#include "MudletApp.h"
#include "RoomAppearance.h"
#include "TAppFrontend.h"
#include "TArea.h"
#include "TEvent.h"
#include "TMap.h"
#include "TNullConsoleFrontend.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mapInfoContributorManager.h"

#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QTimer>

#include <algorithm>
#include <cmath>

TNullMapView::TNullMapView(Host* pHost, QObject* parent, const bool raisesEvents)
: QObject(parent)
, mpHost(pHost)
, mRaisesEvents(raisesEvents)
{
    TMap* pMap = pHost->mpMap.data();
    // The symbol pixmaps are kept by colour and symbol alone; a loaded map's font arrives through this too
    connect(pMap, &TMap::signal_symbolCachesStale, this, [this]() {
        mPainter.flushSymbolPixmapCache();
    });
    connect(pMap, &TMap::signal_roomDeleted, this, &TNullMapView::forgetDeletedRoom);
    // What makes the GUI's dlgMapper repaint its 2D map
    connect(pMap, &TMap::signal_areaChanged, this, &TNullMapView::schedulePaint);
    connect(pMap, &TMap::signal_mapLoaded, this, &TNullMapView::schedulePaint);
    // What has dlgMapper ask T2DMap for an update(), or change what its next frame draws
    connect(pMap, &TMap::signal_areaChanged, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapLoaded, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapperColoursChanged, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapLabelsChanged, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_symbolCachesStale, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_playerRoomStyleChanged, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_playerAreaShowRequested, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_defaultAreaVisibilitySet, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapInfoContributorsChanged, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapRepaintRequested, this, &TNullMapView::requestRepaint);
    connect(pMap, &TMap::signal_mapperSettingChanged, this, [this](const TMap::MapperSetting setting) {
        if (setting == TMap::MapperSetting::RoomSize) {
            mPainter.flushSymbolPixmapCache();
        }
        // The style is the 3D view's alone
        if (setting != TMap::MapperSetting::Style) {
            requestRepaint();
        }
    });
    connect(pMap->mMapInfoContributorManager, &MapInfoContributorManager::signal_contributorsUpdated, this, &TNullMapView::requestRepaint);
}

void TNullMapView::requestRepaint()
{
    if (auto* pFrontend = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()); pFrontend && pFrontend->mMapRepaintRequested) {
        pFrontend->mMapRepaintRequested();
    }
}

void TNullMapView::setViewOnly(const bool viewOnly)
{
    const QPointer<TNullMapView> self(this);
    switchMode(mpHost, viewOnly);
    if (self) {
        requestRepaint();
    }
}

void TNullMapView::announceEditing(Host* pHost)
{
    if (!pHost->mMapViewOnly) {
        switchMode(pHost, false);
    }
}

void TNullMapView::switchMode(Host* pHost, const bool viewOnly)
{
    pHost->mMapViewOnly = viewOnly;
    TEvent mapModeEvent{};
    mapModeEvent.mArgumentList.append(qsl("mapModeChangeEvent"));
    mapModeEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mapModeEvent.mArgumentList.append(viewOnly ? qsl("viewing") : qsl("editing"));
    mapModeEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent(mapModeEvent);
}

void TNullMapView::setShown(const bool shown)
{
    if (shown == mShown) {
        return;
    }
    mShown = shown;
    schedulePaint();
    // The GUI paints a widget as it is shown
    if (shown) {
        requestRepaint();
    }
}

void TNullMapView::schedulePaint()
{
    if (!mShown || mPaintScheduled) {
        return;
    }
    mPaintScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        mPaintScheduled = false;
        paint();
    });
}

// What T2DMap::paintEvent() does to the area it shows: it centres on the player's room once, then
// again whenever that room changes or a centring is asked for (TMap::mNewMove). Only the next render()
// moves the view, as only a paintEvent() moves T2DMap's, so the pointer acts on what was drawn.
void TNullMapView::paint()
{
    if (mShown) {
        followPlayer();
    }
}

void TNullMapView::followPlayer()
{
    TMap* pMap = mpHost->mpMap.data();
    TRoomDB* pRoomDB = pMap->mpRoomDB.get();
    int playerRoomId = pMap->mRoomIdHash.value(pMap->mProfileName);
    TRoom* pPlayerRoom = pRoomDB->getRoom(playerRoomId);
    if (!pPlayerRoom && !pRoomDB->isEmpty()) {
        // T2DMap takes whichever room the hash gives first
        playerRoomId = pRoomDB->getRoomMap().constBegin().key();
        pPlayerRoom = pRoomDB->getRoom(playerRoomId);
    }
    if (!pPlayerRoom) {
        return;
    }
    const bool newMove = takeNewMove();
    if ((mRecentre || mView.mShiftMode) && centredRoomId() == playerRoomId && !newMove) {
        return;
    }
    followRoom(playerRoomId, pPlayerRoom->getArea());
}

// The script's map and the app's both follow a move, but TMap::mNewMove is one flag, which the first to see clears
bool TNullMapView::takeNewMove()
{
    TMap* pMap = mpHost->mpMap.data();
    if (pMap->mNewMove) {
        pMap->mNewMove = false;
        ++pMap->mNewMoves;
    }
    const bool newMove = mNewMovesTaken != pMap->mNewMoves;
    mNewMovesTaken = pMap->mNewMoves;
    return newMove;
}

// Off the player until the player moves again: a move not yet followed was asked for before this
void TNullMapView::leavePlayer()
{
    mRecentre = false;
    mFollowing = false;
    takeNewMove();
}

void TNullMapView::followRoom(const int roomId, const int areaId)
{
    if (shownAreaId() != areaId) {
        TMapPainter::pruneSelectionToArea(mpHost->mpMap.data(), mView.mMultiSelectionSet, mView.mMultiSelectionHighlightRoomId, areaId);
    }
    mRecentre = true;
    mFollowing = true;
    mFollowedRoomId = roomId;
    mFollowedAreaId = areaId;
    if (mView.mLastViewedAreaID == areaId) {
        return;
    }
    const int lastViewedAreaId = mView.mLastViewedAreaID;
    mView.mLastViewedAreaID = areaId;
    raiseAreaChanged(areaId, lastViewedAreaId);
}

int TNullMapView::playerRoomId() const
{
    const TMap* pMap = mpHost->mpMap.data();
    TRoomDB* pRoomDB = pMap->mpRoomDB.get();
    const int playerRoomId = pMap->mRoomIdHash.value(pMap->mProfileName);
    if (pRoomDB->getRoom(playerRoomId) || pRoomDB->isEmpty()) {
        return playerRoomId;
    }
    return pRoomDB->getRoomMap().constBegin().key();
}

bool TNullMapView::follow()
{
    const int roomId = playerRoomId();
    const TRoom* pRoom = mpHost->mpMap->mpRoomDB->getRoom(roomId);
    if (!pRoom) {
        return false;
    }
    const QPointer<TNullMapView> self(this);
    followRoom(roomId, pRoom->getArea());
    // A handler of sysMapAreaChanged may have closed the map
    if (self) {
        requestRepaint();
    }
    return true;
}

qreal TNullMapView::maxZoom() const
{
    qreal most = 30.0;
    if (const TArea* pArea = mpHost->mpMap->mpRoomDB->getArea(mView.mAreaID)) {
        most = qMax(most, 1.5 * (qMax(pArea->max_x - pArea->min_x, pArea->max_y - pArea->min_y) + 1));
    }
    return qMax(most, mPendingZoom ? mPendingZoom->xyzoom : mView.xyzoom);
}

// T2DMap's PanInteractionHandler moves the centre by the drag over the room size; zoomAtPoint() keeps the point under
// the pointer in place, here worked out for any factor rather than in 7% notches
bool TNullMapView::move(const QPointF& pan, const qreal zoom, const QPointF& about)
{
    TArea* pArea = mpHost->mpMap->mpRoomDB->getArea(mView.mAreaID);
    if (!pArea || !mLaidOut || !(zoom > 0.0) || !qIsFinite(zoom)) {
        return false;
    }
    const qreal side = qMin(mView.mSize.width(), mView.mSize.height());
    PendingZoom pending = mPendingZoom.value_or(PendingZoom{mView.xyzoom, mView.mMapCenterX, mView.mMapCenterY});
    const qreal roomSize = side / pending.xyzoom;
    pending.centreX -= laidOut(pan).x() / roomSize;
    pending.centreY -= laidOut(pan).y() / roomSize;
    const qreal newZoom = qBound(TMap::scmMinXYZoom, pending.xyzoom / zoom, maxZoom());
    const QPointF fromMiddle = laidOut(about - QPointF(mRenderedSize.width() / 2.0, mRenderedSize.height() / 2.0));
    const bool zoomed = !qFuzzyCompare(newZoom, pending.xyzoom);
    if (zoomed) {
        const qreal newRoomSize = side / newZoom;
        pending.centreX += fromMiddle.x() / roomSize - fromMiddle.x() / newRoomSize;
        pending.centreY += fromMiddle.y() / roomSize - fromMiddle.y() / newRoomSize;
        pending.xyzoom = newZoom;
        pArea->set2DMapZoom(newZoom);
        mPainter.flushSymbolPixmapCache();
        mPainter.flushTextLabelPixmapCache();
    }
    mPendingZoom = pending;
    mView.mPick = false;
    // A zoom about the middle keeps following the player, and a centring the player's move asked for
    if (!pan.isNull() || (zoomed && !fromMiddle.isNull())) {
        // As showRoom(): still in shift mode for the player's room, so the next frame keeps this centre
        leavePlayer();
        mView.mShiftMode = true;
        mView.mRoomID = playerRoomId();
    }
    requestRepaint();
    return true;
}

void TNullMapView::collectDrawnRooms()
{
    mDrawnRooms.clear();
    TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    const TArea* pArea = mLaidOut ? pRoomDB->getArea(mView.mAreaID) : nullptr;
    if (!pArea) {
        return;
    }
    // As roomsAtPoint() places a room
    const qreal fx = ((mView.xspan / 2.0) - mView.mMapCenterX) * mView.mRoomWidth;
    const qreal fy = ((mView.yspan / 2.0) - mView.mMapCenterY) * mView.mRoomHeight;
    const QRectF frame = QRectF(QPointF(0, 0), QSizeF(mView.mSize)).adjusted(-mView.mRoomWidth, -mView.mRoomHeight, mView.mRoomWidth, mView.mRoomHeight);
    for (const int roomId : pArea->getRoomsForZ(mView.mMapCenterZ)) {
        const TRoom* pRoom = pRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }
        const QPointF position(pRoom->x() * mView.mRoomWidth + fx, -pRoom->y() * mView.mRoomHeight + fy);
        if (frame.contains(position)) {
            mDrawnRooms.append(DrawnRoom{.id = roomId, .position = position / mLayoutScale, .name = pRoom->name});
        }
    }
    if (mDrawnRooms.size() <= maxDrawnRooms) {
        return;
    }
    const QPointF middle(mRenderedSize.width() / 2.0, mRenderedSize.height() / 2.0);
    const auto fromMiddle = [&middle](const DrawnRoom& room) {
        const QPointF offset = room.position - middle;
        return QPointF::dotProduct(offset, offset);
    };
    std::partial_sort(mDrawnRooms.begin(), mDrawnRooms.begin() + maxDrawnRooms, mDrawnRooms.end(), [&fromMiddle](const DrawnRoom& a, const DrawnRoom& b) {
        return fromMiddle(a) < fromMiddle(b);
    });
    mDrawnRooms.resize(maxDrawnRooms);
}

void TNullMapView::raiseAreaChanged(const int areaId, const int lastViewedAreaId)
{
    if (!mRaisesEvents) {
        return;
    }
    TEvent areaViewedChangedEvent{};
    areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    areaViewedChangedEvent.mArgumentList.append(QString::number(areaId));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    areaViewedChangedEvent.mArgumentList.append(QString::number(lastViewedAreaId));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(areaViewedChangedEvent);
}

// With the settings T2DMap::init() and dlgMapper take from the profile and the main window
void TNullMapView::render(QPainter& painter, const QSize& size, const qreal layoutScale)
{
    // The repaint a player's move asks for can come before the paint() it schedules, and a hidden view schedules
    // none; TMapPainter would then centre on the player itself, clearing the move for the other view
    followPlayer();
    TMap* pMap = mpHost->mpMap.data();
    const qreal scale = qIsFinite(layoutScale) ? qBound(1.0, layoutScale, 16.0) : 1.0;
    TMapPainter::View view = mView;
    view.mpMap = pMap;
    view.mpHost = mpHost;
    view.mSize = (QSizeF(size) * scale).toSize();
    view.eSize = mpHost->mLineSize;
    view.mBSize = mpHost->mRoomBorderSize;
    view.rSize = mpHost->mRoomSize;
    view.mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    view.mShowGrid = mpHost->mMapperShowGrid;
    view.mShowRoomID = mpHost->mShowRoomID;
    view.mBubbleMode = mpHost->mBubbleMode;
    view.mLargeAreaExitArrows = mpHost->getLargeAreaExitArrows();
    view.mDirectionalExitLines = TMapPainter::directionalExitLinesEnabled();
    view.mDrawUpperLowerLevels = TAppFrontend::instance()->drawUpperLowerLevels();
    view.mPlayerRoomColorGradientStops = RoomAppearance::playerRoomStops(pMap->mPlayerRoomStyle, pMap->mPlayerRoomInnerDiameterPercentage, pMap->mPlayerRoomInnerColor, pMap->mPlayerRoomOuterColor);
    if (mPendingZoom) {
        view.xyzoom = mPendingZoom->xyzoom;
        view.mMapCenterX = mPendingZoom->centreX;
        view.mMapCenterY = mPendingZoom->centreY;
        mPendingZoom.reset();
    }
    if (mRecentre) {
        view.mShiftMode = false;
        mRecentre = false;
    }
    // Set again only by a frame that lays the rooms out
    view.mRoomWidth = 0.0f;
    TMapPainter::PaintResult result;
    painter.save();
    painter.scale(1.0 / scale, 1.0 / scale);
    // A map info contributor's Lua reads the area shown, as getMapZoom() does, so finds the one this frame shows
    mPainter.paintView(painter, view, result, [this](const TMapPainter::View& frame) {
        mView = frame;
    });
    painter.restore();
    mView = result.view;
    mDrawnAreaId = mView.mAreaID;
    mDrawnLevel = mView.mMapCenterZ;
    mDrawnZoom = mView.xyzoom;
    mRenderedSize = size;
    mLayoutScale = scale;
    mMapInfo = result.mapInfo;
    mLaidOut = mView.mRoomWidth > 0.0f;
    collectDrawnRooms();
    if (result.areaChanged) {
        raiseAreaChanged(result.areaChanged->first, result.areaChanged->second);
    }
}

// The view T2DMap is left with once the player pans it to the room and clicks it: the area and level of the room,
// centred on it, the room selected, and still in shift mode for the player's room, so the next frame keeps it
bool TNullMapView::showRoom(const int roomId)
{
    if (!centreOnRoom(roomId)) {
        return false;
    }
    mView.mMultiSelectionSet = {roomId};
    mView.mMultiSelectionHighlightRoomId = roomId;
    return true;
}

bool TNullMapView::centreOnRoom(const int roomId)
{
    TMap* pMap = mpHost->mpMap.data();
    TRoomDB* pRoomDB = pMap->mpRoomDB.get();
    const TRoom* pRoom = pRoomDB->getRoom(roomId);
    if (!pRoom) {
        return false;
    }
    // As paint() and TMapPainter take the player's room, which shift mode is kept for
    int playerRoomId = pMap->mRoomIdHash.value(pMap->mProfileName);
    if (!pRoomDB->getRoom(playerRoomId)) {
        playerRoomId = pRoomDB->getRoomMap().constBegin().key();
    }
    leavePlayer();
    mPendingZoom.reset();
    mView.mAreaID = pRoom->getArea();
    mView.mMapCenterX = pRoom->x();
    mView.mMapCenterY = -pRoom->y();
    mView.mMapCenterZ = pRoom->z();
    mView.mShiftMode = true;
    mView.mPick = false;
    mView.mRoomID = playerRoomId;
    requestRepaint();
    return true;
}

bool TNullMapView::showLevel(const int level)
{
    const TArea* pArea = mpHost->mpMap->mpRoomDB->getArea(mView.mAreaID);
    if (!pArea || !mLaidOut || !pArea->zLevels.contains(level)) {
        return false;
    }
    // As move(): in shift mode for the player's room, so the next frame keeps this level
    leavePlayer();
    mView.mMapCenterZ = level;
    mView.mShiftMode = true;
    mView.mPick = false;
    mView.mRoomID = playerRoomId();
    requestRepaint();
    return true;
}

bool TNullMapView::showArea(const int areaId)
{
    TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    const TArea* pArea = pRoomDB->getArea(areaId);
    if (!pArea || pArea->zLevels.isEmpty()) {
        return false;
    }
    const TRoom* pPlayerRoom = pRoomDB->getRoom(playerRoomId());
    if (pPlayerRoom && pPlayerRoom->getArea() == areaId) {
        return follow();
    }
    int level = mView.mMapCenterZ;
    if (!pArea->zLevels.contains(level)) {
        // As T2DMap::switchArea(): the lowest of the levels with the most rooms
        qsizetype most = 0;
        for (const int z : pArea->zLevels) {
            if (const qsizetype count = pArea->getRoomsForZ(z).size(); count > most) {
                most = count;
                level = z;
            }
        }
    }
    const QSet<int>& roomIds = pArea->getRoomsForZ(level);
    QList<const TRoom*> rooms;
    double meanX = 0.0;
    double meanY = 0.0;
    for (const int roomId : roomIds) {
        if (const TRoom* pRoom = pRoomDB->getRoom(roomId)) {
            rooms.append(pRoom);
            meanX += (pRoom->x() - meanX) / rooms.size();
            meanY += (pRoom->y() - meanY) / rooms.size();
        }
    }
    const auto nearest = std::min_element(rooms.cbegin(), rooms.cend(), [meanX, meanY](const TRoom* a, const TRoom* b) {
        return std::hypot(a->x() - meanX, a->y() - meanY) < std::hypot(b->x() - meanX, b->y() - meanY);
    });
    if (nearest == rooms.cend()) {
        return false;
    }
    if (mView.mAreaID != areaId) {
        TMapPainter::pruneSelectionToArea(mpHost->mpMap.data(), mView.mMultiSelectionSet, mView.mMultiSelectionHighlightRoomId, areaId);
    }
    // As showRoom(), without selecting the room
    leavePlayer();
    mPendingZoom.reset();
    mView.mAreaID = areaId;
    mView.mMapCenterX = (*nearest)->x();
    mView.mMapCenterY = -(*nearest)->y();
    mView.mMapCenterZ = level;
    mView.mShiftMode = true;
    mView.mPick = false;
    mView.mRoomID = playerRoomId();
    if (mView.mLastViewedAreaID != areaId) {
        const int lastViewedAreaId = mView.mLastViewedAreaID;
        mView.mLastViewedAreaID = areaId;
        raiseAreaChanged(areaId, lastViewedAreaId);
    }
    requestRepaint();
    return true;
}

QList<int> TNullMapView::drawnLevels() const
{
    const TArea* pArea = mpHost->mpMap->mpRoomDB->getArea(mDrawnAreaId);
    return pArea && mLaidOut ? pArea->zLevels : QList<int>();
}

bool TNullMapView::showsArea() const
{
    return mLaidOut && mpHost->mpMap->mpRoomDB->getArea(mView.mAreaID);
}

// As T2DMap::forgetDeletedRoom() does to the selection
void TNullMapView::forgetDeletedRoom(const int roomId)
{
    if (!mView.mMultiSelectionSet.remove(roomId)) {
        return;
    }
    if (mView.mMultiSelectionHighlightRoomId == roomId) {
        mView.mMultiSelectionHighlightRoomId = TMapPainter::selectionCentreRoom(mpHost->mpMap.data(), mView.mMultiSelectionSet);
    }
    requestRepaint();
}

void TNullMapView::clearRoomSelection()
{
    if (!mView.mMultiSelectionSet.isEmpty()) {
        mView.mMultiSelectionSet.clear();
        mView.mMultiSelectionHighlightRoomId = 0;
        requestRepaint();
    }
}

// SelectionRectangleHandler's press, with no rectangle as there is no drag to size it, then T2DMap's
void TNullMapView::press(const QPoint& position, const Qt::MouseButton button, const Qt::KeyboardModifiers modifiers)
{
    TRoomDB* pRoomDB = mpHost->mpMap->mpRoomDB.get();
    if (button == Qt::LeftButton && !modifiers.testFlag(Qt::AltModifier) && mLaidOut && pRoomDB->getRoom(mView.mRoomID)) {
        if (const TArea* pArea = pRoomDB->getArea(mView.mAreaID)) {
            if (!modifiers.testFlag(Qt::ControlModifier) && !modifiers.testFlag(Qt::ShiftModifier)) {
                mView.mMultiSelectionSet.clear();
            }
            bool multiSelection = false;
            const QList<int> rooms = TMapPainter::roomsAtPoint(mView, *pArea, laidOut(position).toPoint());
            TMapPainter::addRoomsToSelection(mpHost->mpMap.data(), mView.mMultiSelectionSet, multiSelection, mView.mMultiSelectionHighlightRoomId, rooms, modifiers);
        }
    }
    if (mRaisesEvents) {
        TMapPainter::raiseMousePressEvent(mpHost);
    }
    requestRepaint();
}

// T2DMap repaints at once, and drawing the room under the click is what resolves it; this frame is the
// one last drawn, not one that has followed the player since
void TNullMapView::doubleClick(const QPoint& position)
{
    TMapPainter::View view = mView;
    view.mIsSecondaryView = true;
    view.mPHighlight = laidOut(position).toPoint();
    view.mPick = true;
    view.mStartSpeedWalk = true;
    view.mKeepZoom = mLaidOut;
    QImage frame(view.mSize, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&frame);
    TMapPainter::PaintResult result;
    mPainter.paintView(painter, view, result);
    // The painter's symbol pixmaps are now at this frame's room size, so the next draw must know to remake them
    mView.mPrevRoomWidth = result.view.mPrevRoomWidth;
    mView.mPrevRoomHeight = result.view.mPrevRoomHeight;
    if (result.pendingSpeedWalk) {
        TMapPainter::startSpeedWalk(mpHost, mpHost->mpMap.data(), result.pendingSpeedWalk->first, result.pendingSpeedWalk->second);
    }
    requestRepaint();
}

bool TNullMapView::wheel(const QPointF& position, const QPoint& angleDelta, const Qt::KeyboardModifiers modifiers)
{
    TArea* pArea = mpHost->mpMap->mpRoomDB->getArea(mView.mAreaID);
    if (!pArea) {
        return false;
    }
    // The main window reads this once at its start; with none, it is read where it is used
    const bool invertZoom = MudletApp::getQSettings()->value(qsl("invertMapZoom"), false).toBool();
    PendingZoom pending = mPendingZoom.value_or(PendingZoom{mView.xyzoom, mView.mMapCenterX, mView.mMapCenterY});
    const std::optional<bool> zoomed = TMapPainter::zoomAtPoint(*pArea, pending.xyzoom, pending.centreX, pending.centreY, mView.mSize, laidOut(position), angleDelta, modifiers, invertZoom);
    if (!zoomed) {
        return false;
    }
    mView.mPick = false;
    if (*zoomed) {
        // zoomAtPoint() keeps the point under the pointer, so a turn off the middle leaves the player, unless the
        // player's move has asked for a centring the next render() does
        if (!mRecentre && position != QPointF(mRenderedSize.width() / 2.0, mRenderedSize.height() / 2.0)) {
            mFollowing = false;
        }
        mPendingZoom = pending;
        mPainter.flushSymbolPixmapCache();
        mPainter.flushTextLabelPixmapCache();
        requestRepaint();
    }
    return true;
}

std::pair<bool, QString> TNullMapView::show3DView(const bool shown)
{
    if (shown) {
        return {false, qsl("there is no main window to show the 3D map view in")};
    }
    // As dlgMapper does, so a profile saved with the 3D view showing opens without it
    mpHost->mShow3DView = false;
    return {true, QString()};
}

// Draws with the settings T2DMap takes from the profile, at the level of the room it centres on
std::pair<bool, QString> TNullMapView::exportAreaToImage(const int areaId, const QString& filePath, std::optional<int> zLevel, const qreal zoom, const bool exportAllZLevels)
{
    TMapPainter::View view;
    view.mpMap = mpHost->mpMap.data();
    view.mpHost = mpHost;
    view.mBSize = mpHost->mRoomBorderSize;
    view.rSize = mpHost->mRoomSize;
    view.mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    view.mShowRoomID = mpHost->mShowRoomID;
    view.mBubbleMode = mpHost->mBubbleMode;
    view.mLargeAreaExitArrows = mpHost->getLargeAreaExitArrows();
    view.mDirectionalExitLines = TMapPainter::directionalExitLinesEnabled();
    view.mAreaID = shownAreaId();
    if (const TRoom* pCentredRoom = mpHost->mpMap->mpRoomDB->getRoom(centredRoomId())) {
        view.mMapCenterZ = pCentredRoom->z();
    }
    // A map info contributor's Lua can export while a frame is being drawn, which goes on with its own view
    const TMapPainter::View frame = mPainter.view();
    mPainter.setView(view);
    auto result = mPainter.exportArea(areaId, filePath, zLevel, zoom, exportAllZLevels, this);
    mPainter.setView(frame);
    return result;
}

// As dlgMapper::setMapZoom() and T2DMap::setMapZoom()
// As T2DMap::centerview() for the main view: the view pans to the room as a drag would, so paint() leaves it there
// until the player's room changes
std::pair<bool, QString> TNullMapView::centerViewOn(const int roomId)
{
    TMap* pMap = mpHost->mpMap.data();
    TRoomDB* pRoomDB = pMap->mpRoomDB.get();
    const TRoom* pRoom = pRoomDB->getRoom(roomId);
    if (!pRoom) {
        return {false, qsl("room %1 not found").arg(roomId)};
    }
    const int areaId = pRoom->getArea();
    if (!pRoomDB->getArea(areaId)) {
        return {false, qsl("room %1 references non-existent area %2").arg(roomId).arg(areaId)};
    }
    // The room paint() compares against, with T2DMap's fallback to the map's first room
    int playerRoomId = pMap->mRoomIdHash.value(pMap->mProfileName);
    if (!pRoomDB->getRoom(playerRoomId) && !pRoomDB->isEmpty()) {
        playerRoomId = pRoomDB->getRoomMap().constBegin().key();
    }
    if (shownAreaId() != areaId) {
        TMapPainter::pruneSelectionToArea(pMap, mView.mMultiSelectionSet, mView.mMultiSelectionHighlightRoomId, areaId);
    }
    leavePlayer();
    mPendingZoom.reset();
    mView.mAreaID = areaId;
    mView.mRoomID = playerRoomId;
    mView.mShiftMode = true;
    mView.mMapCenterX = pRoom->x();
    mView.mMapCenterY = -pRoom->y();
    mView.mMapCenterZ = pRoom->z();
    mView.xyzoom = pRoomDB->get2DMapZoom(areaId);
    if (mView.mLastViewedAreaID != areaId) {
        const int lastViewedAreaId = mView.mLastViewedAreaID;
        mView.mLastViewedAreaID = areaId;
        raiseAreaChanged(areaId, lastViewedAreaId);
    }
    requestRepaint();
    return {true, QString()};
}

std::pair<bool, QString> TNullMapView::setMapZoom(const qreal zoom, const int areaId)
{
    if (const QString error = TMap::zoomError(zoom); !error.isEmpty()) {
        return {false, error};
    }
    const int zoomedAreaId = areaId ? areaId : shownAreaId();
    if (!mpHost->mpMap->mpRoomDB->set2DMapZoom(zoomedAreaId, zoom)) {
#if defined(INCLUDE_3DMAPPER)
        // dlgMapper hands T2DMap the shown area in place of 0, so the refusal names it
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(zoomedAreaId))};
#else
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(areaId))};
#endif
    }
    requestRepaint();
    return {true, QString()};
}
