#ifndef MUDLET_TNULLMAPVIEW_H
#define MUDLET_TNULLMAPVIEW_H

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

#include "TMapViewFrontend.h"
#include "utils.h"

#include "TMapPainter.h"

#include <QObject>

class Host;
class QPainter;
class QSize;

// The 2D map of a map widget or embedded mapper that the null view records. It keeps the area that
// T2DMap would show, which T2DMap moves as it paints, so this "paints" once the event loop runs.
class TNullMapView final : public QObject, public TMapViewFrontend
{
public:
    explicit TNullMapView(Host* pHost, QObject* parent = nullptr);
    // A hidden mapper does not paint
    void setShown(bool shown);
    // What T2DMap would draw at that size, whether or not the mapper is shown, raising sysMapAreaChanged as it does
    void render(QPainter&, const QSize&);

    bool onScreen() const override { return false; }
    void showMapProgress(const QString&, bool) override {}
    void setMapProgressLabel(const QString&) override {}
    void setMapProgressRange(int, int) override {}
    void setMapProgressValue(int) override {}
    int mapProgressMaximum() const override { return 0; }
    void setMapProgressCancelable(bool) override {}
    void hideMapProgress() override {}
    bool isMapProgressVisible() const override { return false; }
    // Only a user selects rooms
    bool selectingRooms() const override { return false; }
    QSet<int> selectedRooms() const override { return {}; }
    int centerSelectedRoom() const override { return 0; }
    void clearRoomSelection() override {}
    int shownAreaId() const override { return mView.mAreaID; }
    std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) override;
    std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels) override;
    std::pair<bool, QString> show3DView(bool shown) override;
    // As dlgMapper answers while it shows its 2D map, the only one this keeps
    bool showing3DView() const override { return false; }
    void recreate3DView() override {}
    std::pair<bool, QString> shift3DViewCamera(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }
    std::pair<bool, QString> set3DViewCameraPosition(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }

private:
    void schedulePaint();
    void paint();
    void requestRepaint();
    void raiseAreaChanged(int areaId, int lastViewedAreaId);

    Host* mpHost = nullptr;
    bool mShown = false;
    bool mPaintScheduled = false;
    // What T2DMap keeps between frames: its area, the area last raised for, the room it centred on, whether it
    // has, and where
    TMapPainter::View mView;
    // paint() centred on the player without a size, so the next render() centres again as T2DMap would, which
    // for a small area may be on the area
    bool mRecentre = false;
    TMapPainter mPainter;
};

#endif // MUDLET_TNULLMAPVIEW_H
