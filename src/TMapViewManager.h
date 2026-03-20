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

#ifndef MUDLET_TMAPVIEWMANAGER_H
#define MUDLET_TMAPVIEWMANAGER_H

#include <QDockWidget>
#include <QMap>
#include <QObject>
#include <QPointer>

class Host;
class TMap;
class TMapView;

/**
 * Manages the lifecycle of secondary map view windows.
 * Secondary views are view-only map displays that can show different
 * areas/z-levels independently from the main mapper window.
 */
class TMapViewManager : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMapViewManager)
    TMapViewManager(Host* pHost, TMap* pMap);
    ~TMapViewManager() override;

    // View lifecycle
    std::pair<int, QString> createView(int initialAreaId = 0);
    std::pair<bool, QString> closeView(int viewId);
    int closeAllViews();

    // View access
    TMapView* getView(int viewId);
    QList<int> getViewIds() const;
    int getViewCount() const;

    // Bulk operations
    void updateAllViews();

signals:
    void viewCreated(int viewId);
    void viewClosed(int viewId);

private slots:
    void slot_viewClosed();

private:
    // View IDs start at 1; 0 is reserved as an error/invalid indicator in return values
    int mNextViewId = 1;
    QPointer<Host> mpHost;
    QPointer<TMap> mpMap;
    QMap<int, QPointer<QDockWidget>> mDockWidgets;
    QMap<int, QPointer<TMapView>> mViews;
};

#endif // MUDLET_TMAPVIEWMANAGER_H
