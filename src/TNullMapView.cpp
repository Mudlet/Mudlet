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
#include "RoomAppearance.h"
#include "TAppFrontend.h"
#include "TEvent.h"
#include "TMap.h"
#include "TNullConsoleFrontend.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mapInfoContributorManager.h"

#include <QTimer>

TNullMapView::TNullMapView(Host* pHost, QObject* parent)
: QObject(parent)
, mpHost(pHost)
{
    TMap* pMap = pHost->mpMap.data();
    // The symbol pixmaps are kept by colour and symbol alone; a loaded map's font arrives through this too
    connect(pMap, &TMap::signal_symbolCachesStale, this, [this]() {
        mPainter.flushSymbolPixmapCache();
    });
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
// again whenever that room changes or a centring is asked for (TMap::mNewMove)
void TNullMapView::paint()
{
    if (!mShown) {
        return;
    }
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
    if (mView.mShiftMode && mView.mRoomID == playerRoomId && !pMap->mNewMove) {
        return;
    }
    mView.mShiftMode = true;
    mRecentre = true;
    pMap->mNewMove = false;
    mView.mRoomID = playerRoomId;
    mView.mAreaID = pPlayerRoom->getArea();
    mView.mMapCenterX = pPlayerRoom->x();
    mView.mMapCenterY = pPlayerRoom->y() * -1;
    mView.mMapCenterZ = pPlayerRoom->z();
    if (mView.mLastViewedAreaID == mView.mAreaID) {
        return;
    }
    const int lastViewedAreaId = mView.mLastViewedAreaID;
    mView.mLastViewedAreaID = mView.mAreaID;
    raiseAreaChanged(mView.mAreaID, lastViewedAreaId);
}

void TNullMapView::raiseAreaChanged(const int areaId, const int lastViewedAreaId)
{
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
void TNullMapView::render(QPainter& painter, const QSize& size)
{
    TMap* pMap = mpHost->mpMap.data();
    TMapPainter::View view = mView;
    view.mpMap = pMap;
    view.mpHost = mpHost;
    view.mSize = size;
    view.eSize = mpHost->mLineSize;
    view.mBSize = mpHost->mRoomBorderSize;
    view.rSize = mpHost->mRoomSize;
    view.mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    view.mShowGrid = mpHost->mMapperShowGrid;
    view.mShowRoomID = mpHost->mShowRoomID;
    view.mBubbleMode = mpHost->mBubbleMode;
    view.mLargeAreaExitArrows = mpHost->getLargeAreaExitArrows();
    view.mDrawUpperLowerLevels = TAppFrontend::instance()->drawUpperLowerLevels();
    view.mPlayerRoomColorGradientStops = RoomAppearance::playerRoomStops(pMap->mPlayerRoomStyle, pMap->mPlayerRoomInnerDiameterPercentage, pMap->mPlayerRoomInnerColor, pMap->mPlayerRoomOuterColor);
    if (mRecentre) {
        view.mShiftMode = false;
        mRecentre = false;
    }
    TMapPainter::PaintResult result;
    // A map info contributor's Lua reads the area shown, as getMapZoom() does, so finds the one this frame shows
    mPainter.paintView(painter, view, result, [this](const TMapPainter::View& frame) {
        mView = frame;
    });
    mView = result.view;
    if (result.areaChanged) {
        raiseAreaChanged(result.areaChanged->first, result.areaChanged->second);
    }
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
    view.mAreaID = mView.mAreaID;
    if (const TRoom* pCentredRoom = mpHost->mpMap->mpRoomDB->getRoom(mView.mRoomID)) {
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
std::pair<bool, QString> TNullMapView::setMapZoom(const qreal zoom, const int areaId)
{
    if (const QString error = TMap::zoomError(zoom); !error.isEmpty()) {
        return {false, error};
    }
    const int zoomedAreaId = areaId ? areaId : mView.mAreaID;
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
