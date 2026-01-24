/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "TMapViewManager.h"

#include "Host.h"
#include "mudlet.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TMapView.h"
#include "TRoomDB.h"
#include "utils.h"

TMapViewManager::TMapViewManager(Host* pHost, TMap* pMap)
: QObject(pMap)
, mpHost(pHost)
, mpMap(pMap)
{
    Q_ASSERT(pHost);
    Q_ASSERT(pMap);
}

TMapViewManager::~TMapViewManager()
{
    closeAllViews();
}

std::pair<int, QString> TMapViewManager::createView(int initialAreaId)
{
    if (!mpHost || !mpMap) {
        return {0, qsl("no valid host or map")};
    }

    // Validate area ID if provided
    if (initialAreaId > 0 && mpMap->mpRoomDB && !mpMap->mpRoomDB->getAreaNamesMap().contains(initialAreaId)) {
        return {0, qsl("area %1 does not exist").arg(initialAreaId)};
    }

    const int viewId = mNextViewId++;
    const QString hostName = mpHost->getName();

    //: Title for a secondary map view window, %1 is the view number, %2 is the profile name
    auto* dockWidget = new QDockWidget(tr("Map View %1 - %2").arg(viewId).arg(hostName));
    dockWidget->setObjectName(qsl("dockMapView_%1_%2").arg(hostName).arg(viewId));

    auto* mapView = new TMapView(viewId, mpHost, mpMap, dockWidget);
    dockWidget->setWidget(mapView);

    if (initialAreaId > 0) {
        mapView->setArea(initialAreaId);
    }

    mDockWidgets[viewId] = dockWidget;
    mViews[viewId] = mapView;

    connect(dockWidget, &QDockWidget::destroyed, this, &TMapViewManager::slot_viewClosed);

    mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    dockWidget->setFloating(true);
    dockWidget->resize(400, 500);
    dockWidget->show();
    dockWidget->raise();

    emit viewCreated(viewId);

    return {viewId, QString()};
}

std::pair<bool, QString> TMapViewManager::closeView(int viewId)
{
    auto dockIt = mDockWidgets.find(viewId);
    if (dockIt == mDockWidgets.end() || !dockIt.value()) {
        return {false, qsl("view %1 not found").arg(viewId)};
    }

    QDockWidget* dockWidget = dockIt.value();

    mDockWidgets.remove(viewId);
    mViews.remove(viewId);

    // Note: deleteLater() will trigger the destroyed signal, which is connected to slot_viewClosed().
    // Since we already removed from tracking maps above, slot_viewClosed() will find viewId == 0
    // and skip emitting viewClosed again.
    dockWidget->deleteLater();

    emit viewClosed(viewId);

    return {true, QString()};
}

int TMapViewManager::closeAllViews()
{
    const QList<int> viewIds = mDockWidgets.keys();
    int closedCount = 0;

    for (int viewId : viewIds) {
        auto [success, msg] = closeView(viewId);
        if (success) {
            ++closedCount;
        } else {
            qWarning() << "TMapViewManager::closeAllViews() - failed to close view" << viewId << ":" << msg;
        }
    }

    return closedCount;
}

TMapView* TMapViewManager::getView(int viewId)
{
    auto it = mViews.find(viewId);
    if (it != mViews.end() && it.value()) {
        return it.value();
    }
    return nullptr;
}

QList<int> TMapViewManager::getViewIds() const
{
    return mViews.keys();
}

int TMapViewManager::getViewCount() const
{
    return mViews.size();
}

void TMapViewManager::updateAllViews()
{
    for (const auto& [viewId, view] : mViews.asKeyValueRange()) {
        Q_UNUSED(viewId)
        if (view && view->get2DMap()) {
            view->get2DMap()->update();
            view->updateAreaComboBox();
        }
    }
}

void TMapViewManager::slot_viewClosed()
{
    auto* dockWidget = qobject_cast<QDockWidget*>(sender());
    if (!dockWidget) {
        qWarning() << "TMapViewManager::slot_viewClosed() - sender is not a QDockWidget";
        return;
    }

    const int viewId = mDockWidgets.key(dockWidget, 0);
    if (viewId > 0) {
        mDockWidgets.remove(viewId);
        mViews.remove(viewId);
        emit viewClosed(viewId);
    } else {
        qWarning() << "TMapViewManager::slot_viewClosed() - dock widget not found in tracking map";
    }
}
