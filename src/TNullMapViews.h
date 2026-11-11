#ifndef MUDLET_TNULLMAPVIEWS_H
#define MUDLET_TNULLMAPVIEWS_H

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

#include "TMapViewsFrontend.h"

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSize>

class Host;
class TMap;
class TNullMapViews;

// A secondary map view of a profile with no main window: what TMapViewManager gives its dock and
// what the view's T2DMap keeps of the map. T2DMap moves its zoom as it paints, so this "paints"
// once the event loop runs, as an update() would.
class TNullSecondaryMapView final : public QObject, public TSecondaryMapViewFrontend
{
public:
    // The dock TMapViewManager puts the view in, which only a user can move, hide or resize
    struct Dock
    {
        QString title;
        bool floating = false;
        QSize size;
        bool shown = false;
    };

    TNullSecondaryMapView(Host* pHost, TMap* pMap, TNullMapViews* pViews);

    std::pair<bool, QString> centerOnRoom(int roomId) override;
    std::pair<bool, QString> setZoom(qreal zoom) override;
    int getCurrentAreaId() const override { return mAreaId; }
    int getCenteredRoomId() const override { return mRoomId; }
    qreal getZoom() const override { return mZoom; }
    int getZLevel() const override { return mZLevel; }

    // As T2DMap::switchArea(int)
    void switchArea(int areaId);
    // As TMapView::switchToAnotherArea()
    void switchToAnotherArea();
    void schedulePaint();

    Dock mDock;

private:
    void switchArea(const QString& areaName);
    void paint();

    QPointer<Host> mpHost;
    TMap* mpMap = nullptr;
    TNullMapViews* mpViews = nullptr;
    bool mPaintScheduled = false;
    // T2DMap's mAreaID, mRoomID, mMapCenterZ and xyzoom, as a secondary view starts them
    int mAreaId = 0;
    int mRoomId = 0;
    int mZLevel = 0;
    qreal mZoom;
};

// The secondary map views of a profile with no main window, made, numbered and closed as
// TMapViewManager does
class TNullMapViews final : public QObject, public TMapViewsFrontend
{
public:
    TNullMapViews(Host* pHost, TMap* pMap);

    std::pair<int, QString> createView(int initialAreaId) override;
    std::pair<bool, QString> closeView(int viewId) override;
    int closeAllViews() override;
    TSecondaryMapViewFrontend* view(int viewId) override { return mViews.value(viewId); }
    QList<int> getViewIds() const override { return mViews.keys(); }
    void updateAllViews() override;
    void switchViewsShowingArea(int areaId) override;
    // A null view never shows a 3D view
    void update3DViewAvailability() override {}

    // A GUI view takes its area's new zoom at whatever repaints it next; nothing else repaints a null one
    void paintViewsShowing(int areaId);

    const TNullSecondaryMapView* record(int viewId) const { return mViews.value(viewId); }

private:
    QPointer<Host> mpHost;
    TMap* mpMap = nullptr;
    int mNextViewId = 1;
    QMap<int, TNullSecondaryMapView*> mViews;
};

#endif // MUDLET_TNULLMAPVIEWS_H
