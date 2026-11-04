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

#include "TNullMapViews.h"

#include "Host.h"
#include "TArea.h"
#include "TEvent.h"
#include "TMap.h"
#include "TRoomDB.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

TNullSecondaryMapView::TNullSecondaryMapView(Host* pHost, TMap* pMap, TNullMapViews* pViews)
: QObject(pViews)
, mpHost(pHost)
, mpMap(pMap)
, mpViews(pViews)
, mZoom(TMap::scmDefaultXYZoom)
{
    // What makes TMapView repaint its 2D map
    connect(pMap, &TMap::signal_areaChanged, this, [this](const int areaId) {
        if (areaId == -1 || areaId == mAreaId) {
            schedulePaint();
        }
    });
    connect(pMap, &TMap::signal_symbolCachesStale, this, &TNullSecondaryMapView::schedulePaint);
}

void TNullSecondaryMapView::schedulePaint()
{
    if (mPaintScheduled) {
        return;
    }
    mPaintScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        mPaintScheduled = false;
        paint();
    });
}

// What T2DMap::paintEvent() does to a secondary view's zoom, which it takes from the area it
// draws once the map has a room to draw around
void TNullSecondaryMapView::paint()
{
    TRoomDB* pRoomDB = mpMap->mpRoomDB.get();
    TRoom* pPlayerRoom = pRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
    if (!pPlayerRoom && !pRoomDB->isEmpty()) {
        pPlayerRoom = pRoomDB->getRoom(pRoomDB->getRoomMap().constBegin().key());
    }
    if (!pPlayerRoom || !pRoomDB->getArea(pPlayerRoom->getArea())) {
        return;
    }
    if (TArea* pArea = pRoomDB->getArea(mAreaId)) {
        mZoom = pArea->get2DMapZoom();
    }
}

void TNullSecondaryMapView::switchArea(const int areaId)
{
    const QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
    if (areaName.isEmpty()) {
        qWarning() << "TNullSecondaryMapView::switchArea(int) - area" << areaId << "not found in area names map";
        return;
    }
    switchArea(areaName);
}

// As T2DMap::switchArea(const QString&), which takes the first area of that name
void TNullSecondaryMapView::switchArea(const QString& areaName)
{
    if (!mpHost) {
        return;
    }
    TRoomDB* pRoomDB = mpMap->mpRoomDB.get();
    TRoom* pPlayerRoom = pRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
    for (const auto& [areaId, name] : pRoomDB->getAreaNamesMap().asKeyValueRange()) {
        TArea* pArea = pRoomDB->getArea(areaId);
        if (!pArea || name != areaName) {
            continue;
        }
        const int previousAreaId = mAreaId;
        mAreaId = areaId;
        pArea->calcSpan();
        if (pPlayerRoom && pPlayerRoom->getArea() == mAreaId) {
            mZLevel = pPlayerRoom->z();
        } else if (!pArea->zLevels.contains(mZLevel)) {
            // The level with the most rooms, the highest of those that tie
            QMap<int, int> roomCountByLevel;
            for (const int roomId : pArea->getAreaRooms()) {
                if (TRoom* pRoom = pRoomDB->getRoom(roomId)) {
                    ++roomCountByLevel[pRoom->z()];
                }
            }
            int mostRooms = 0;
            mZLevel = 0;
            for (const auto& [level, count] : roomCountByLevel.asKeyValueRange()) {
                if (count >= mostRooms) {
                    mostRooms = count;
                    mZLevel = level;
                }
            }
        }
        mZoom = pRoomDB->get2DMapZoom(mAreaId);
        if (previousAreaId != mAreaId) {
            TEvent areaViewedChangedEvent{};
            areaViewedChangedEvent.mArgumentList.append(qsl("sysMapAreaChanged"));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            areaViewedChangedEvent.mArgumentList.append(QString::number(mAreaId));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            areaViewedChangedEvent.mArgumentList.append(QString::number(previousAreaId));
            areaViewedChangedEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            mpHost->raiseEvent(areaViewedChangedEvent);
        }
        return;
    }
}

void TNullSecondaryMapView::switchToAnotherArea()
{
    const QMap<int, QString>& areaNames = mpMap->mpRoomDB->getAreaNamesMap();
    // TMapView's area list, sorted ignoring case, of which it picks the entry it shows or else the first
    QMap<QString, QString> listedNames;
    for (const auto& [areaId, areaName] : areaNames.asKeyValueRange()) {
        if (areaId == -1 && !mpMap->getDefaultAreaShown()) {
            continue;
        }
        listedNames.insert(areaName.toLower(), areaName);
    }
    if (listedNames.isEmpty()) {
        switchArea(mpMap->getDefaultAreaName());
        return;
    }
    const QString shownName = areaNames.value(mAreaId);
    const QString pickedName = !shownName.isEmpty() && listedNames.value(shownName.toLower()) == shownName ? shownName : listedNames.first();
    if (!areaNames.key(pickedName, 0)) {
        qWarning() << "TNullSecondaryMapView::switchToAnotherArea() - area" << pickedName << "not found in area names map";
        return;
    }
    switchArea(pickedName);
}

std::pair<bool, QString> TNullSecondaryMapView::centerOnRoom(const int roomId)
{
    TRoomDB* pRoomDB = mpMap->mpRoomDB.get();
    TRoom* pRoom = pRoomDB->getRoom(roomId);
    if (!pRoom) {
        return {false, qsl("room %1 not found").arg(roomId)};
    }
    const int areaId = pRoom->getArea();
    if (!pRoomDB->getArea(areaId)) {
        return {false, qsl("room %1 references non-existent area %2").arg(roomId).arg(areaId)};
    }
    mAreaId = areaId;
    mRoomId = roomId;
    mZLevel = pRoom->z();
    mZoom = pRoomDB->get2DMapZoom(areaId);
    schedulePaint();
    return {true, QString()};
}

// As T2DMap::setMapZoom() for the area the view shows, which sets that area's zoom for the view to
// take as it paints
std::pair<bool, QString> TNullSecondaryMapView::setZoom(const qreal zoom)
{
    if (const QString error = TMap::zoomError(zoom); !error.isEmpty()) {
        return {false, error};
    }
    TArea* pArea = mpMap->mpRoomDB->getArea(mAreaId);
    if (!pArea) {
        return {false, qsl("number %1 is not a valid areaID").arg(QString::number(mAreaId))};
    }
    if (qFuzzyCompare(1.0 + pArea->get2DMapZoom(), 1.0 + zoom)) {
        return {true, QString()};
    }
    pArea->set2DMapZoom(zoom);
    mpViews->paintViewsShowing(mAreaId);
    return {true, QString()};
}

TNullMapViews::TNullMapViews(Host* pHost, TMap* pMap)
: QObject(pMap)
, mpHost(pHost)
, mpMap(pMap)
{
}

std::pair<int, QString> TNullMapViews::createView(const int initialAreaId)
{
    if (!mpHost || !mpMap) {
        return {0, qsl("no valid host or map")};
    }
    if (initialAreaId > 0 && !mpMap->mpRoomDB->getAreaNamesMap().contains(initialAreaId)) {
        return {0, qsl("area %1 does not exist").arg(initialAreaId)};
    }

    const int viewId = mNextViewId++;
    //: Title for a secondary map view window, %1 is the view number, %2 is the profile name
    const QString title = QCoreApplication::translate("TMapViewManager", "Map View %1 - %2").arg(viewId).arg(mpHost->getName());
    auto* pView = new TNullSecondaryMapView(mpHost, mpMap, this);
    pView->mDock = {title, true, QSize(400, 500), true};
    // Before the view is listed, as the sysMapAreaChanged this can raise comes before TMapViewManager lists it
    if (initialAreaId > 0) {
        pView->switchArea(initialAreaId);
    } else if (initialAreaId == 0) {
        if (const auto playerAreaId = mpMap->playerAreaId()) {
            pView->switchArea(*playerAreaId);
        }
    }
    mViews.insert(viewId, pView);
    // The dock showing paints the view
    pView->schedulePaint();
    return {viewId, QString()};
}

std::pair<bool, QString> TNullMapViews::closeView(const int viewId)
{
    TNullSecondaryMapView* pView = mViews.take(viewId);
    if (!pView) {
        return {false, qsl("view %1 not found").arg(viewId)};
    }
    // A sysMapAreaChanged handler can close the view raising it
    pView->deleteLater();
    return {true, QString()};
}

int TNullMapViews::closeAllViews()
{
    int closedCount = 0;
    for (const int viewId : mViews.keys()) {
        if (closeView(viewId).first) {
            ++closedCount;
        }
    }
    return closedCount;
}

void TNullMapViews::updateAllViews()
{
    for (TNullSecondaryMapView* pView : std::as_const(mViews)) {
        pView->schedulePaint();
    }
}

void TNullMapViews::paintViewsShowing(const int areaId)
{
    for (TNullSecondaryMapView* pView : std::as_const(mViews)) {
        if (pView->getCurrentAreaId() == areaId) {
            pView->schedulePaint();
        }
    }
}

void TNullMapViews::switchViewsShowingArea(const int areaId)
{
    // Each switch raises sysMapAreaChanged, whose handlers can open and close views
    for (const int viewId : mViews.keys()) {
        if (TNullSecondaryMapView* pView = mViews.value(viewId); pView && pView->getCurrentAreaId() == areaId) {
            pView->switchToAnotherArea();
        }
    }
}
