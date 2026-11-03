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

#include <QObject>

class Host;

// The 2D map of a map widget or embedded mapper that the null view records. It keeps the area that
// T2DMap would show, which T2DMap moves as it paints, so this "paints" once the event loop runs.
class TNullMapView final : public QObject, public TMapViewFrontend
{
public:
    explicit TNullMapView(Host* pHost, QObject* parent = nullptr);
    // A hidden mapper does not paint
    void setShown(bool shown);

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
    int shownAreaId() const override { return mAreaId; }
    std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) override;
    std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int>, qreal zoom, bool exportAllZLevels) override;
    std::pair<bool, QString> show3DView(bool shown) override;
    // As dlgMapper answers while it shows its 2D map, the only one this keeps
    bool showing3DView() const override { return false; }
    void recreate3DView() override {}
    std::pair<bool, QString> shift3DViewCamera(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }
    std::pair<bool, QString> set3DViewCameraPosition(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }

private:
    void schedulePaint();
    void paint();

    Host* mpHost = nullptr;
    bool mShown = false;
    bool mPaintScheduled = false;
    // T2DMap's mAreaID, mLastViewedAreaID, mRoomID and mShiftMode
    int mAreaId = 0;
    int mLastViewedAreaId = -2;
    int mCentredRoomId = 0;
    bool mCentred = false;
};

#endif // MUDLET_TNULLMAPVIEW_H
