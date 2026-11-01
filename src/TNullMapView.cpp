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
#include "TEvent.h"
#include "TMap.h"
#include "TRoomDB.h"

#include <QTimer>

TNullMapView::TNullMapView(Host* pHost, QObject* parent)
: QObject(parent)
, mpHost(pHost)
{
    // What makes the GUI's dlgMapper repaint its 2D map
    connect(pHost->mpMap.data(), &TMap::signal_areaChanged, this, &TNullMapView::schedulePaint);
    connect(pHost->mpMap.data(), &TMap::signal_mapLoaded, this, &TNullMapView::schedulePaint);
}

void TNullMapView::setShown(const bool shown)
{
    if (shown == mShown) {
        return;
    }
    mShown = shown;
    schedulePaint();
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
    if (mCentred && mCentredRoomId == playerRoomId && !pMap->mNewMove) {
        return;
    }
    mCentred = true;
    pMap->mNewMove = false;
    mCentredRoomId = playerRoomId;
    mAreaId = pPlayerRoom->getArea();
    if (mLastViewedAreaId == mAreaId) {
        return;
    }
    TEvent areaViewedChangedEvent{};
    areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    areaViewedChangedEvent.mArgumentList.append(QString::number(mAreaId));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    areaViewedChangedEvent.mArgumentList.append(QString::number(mLastViewedAreaId));
    areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mLastViewedAreaId = mAreaId;
    mpHost->raiseEvent(areaViewedChangedEvent);
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

// Refuses what T2DMap refuses before it draws, so a script hears what is wrong with its file name
std::pair<bool, QString> TNullMapView::exportAreaToImage(const int areaId, const QString& filePath, std::optional<int>, const qreal zoom, const bool exportAllZLevels)
{
    QList<std::pair<int, QString>> levelExports;
    if (const QString problem = mpHost->mpMap->areaExportProblem(areaId, filePath, zoom, exportAllZLevels, levelExports); !problem.isEmpty()) {
        return {false, problem};
    }
    return {false, qsl("there is no main window to draw the map in")};
}

// As dlgMapper::setMapZoom() and T2DMap::setMapZoom()
std::pair<bool, QString> TNullMapView::setMapZoom(const qreal zoom, const int areaId)
{
    if (const QString error = TMap::zoomError(zoom); !error.isEmpty()) {
        return {false, error};
    }
    const int zoomedAreaId = areaId ? areaId : mAreaId;
    if (!mpHost->mpMap->mpRoomDB->set2DMapZoom(zoomedAreaId, zoom)) {
#if defined(INCLUDE_3DMAPPER)
        // dlgMapper hands T2DMap the shown area in place of 0, so the refusal names it
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(zoomedAreaId))};
#else
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(areaId))};
#endif
    }
    return {true, QString()};
}
