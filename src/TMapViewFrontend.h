#ifndef MUDLET_TMAPVIEWFRONTEND_H
#define MUDLET_TMAPVIEWFRONTEND_H

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

#include <QSet>
#include <QString>

#include <optional>
#include <utility>

// What core code asks of the mapper drawing a profile's map. dlgMapper implements it; core code
// reaches it through TMap::mapViewFrontend().
class TMapViewFrontend
{
public:
    virtual bool onScreen() const = 0;
    // The progress overlay a map transfer shows inside the mapper. While it is shown, its cancel
    // button calls TMap::slot_downloadCancel() on the mapper's map.
    virtual void showMapProgress(const QString& label, bool cancelable) = 0;
    virtual void setMapProgressLabel(const QString& text) = 0;
    virtual void setMapProgressRange(int minimum, int maximum) = 0;
    virtual void setMapProgressValue(int value) = 0;
    virtual int mapProgressMaximum() const = 0;
    virtual void setMapProgressCancelable(bool cancelable) = 0;
    virtual void hideMapProgress() = 0;
    virtual bool isMapProgressVisible() const = 0;

    // The 2D map's room selection; while a drag is still sizing it, it cannot be cleared.
    virtual bool selectingRooms() const = 0;
    virtual QSet<int> selectedRooms() const = 0;
    virtual int centerSelectedRoom() const = 0;
    virtual void clearRoomSelection() = 0;

    virtual int shownAreaId() const = 0;
    virtual std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) = 0;
    virtual std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels) = 0;

    // Without the 3D mapper compiled in there is no 3D view to show.
    virtual void show3DView(bool shown) = 0;
    virtual bool showing3DView() const = 0;
    // Builds the 3D view afresh, for a change of 3D renderer to take effect.
    virtual void recreate3DView() = 0;

protected:
    // The mapper is a widget whose Qt parent deletes it, so nothing deletes it through this interface.
    ~TMapViewFrontend() = default;
};

#endif // MUDLET_TMAPVIEWFRONTEND_H
