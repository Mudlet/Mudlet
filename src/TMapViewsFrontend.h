#ifndef MUDLET_TMAPVIEWSFRONTEND_H
#define MUDLET_TMAPVIEWSFRONTEND_H

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

#include <QList>
#include <QString>

#include <utility>

// One of the view-only map windows a profile can open beside its mapper, as core code drives it.
// TMapView implements it.
class TSecondaryMapViewFrontend
{
public:
    virtual std::pair<bool, QString> centerOnRoom(int roomId) = 0;
    virtual std::pair<bool, QString> setZoom(qreal zoom) = 0;
    virtual int getCurrentAreaId() const = 0;
    virtual int getCenteredRoomId() const = 0;
    virtual qreal getZoom() const = 0;
    virtual int getZLevel() const = 0;

protected:
    // A view is a widget its dock deletes, so nothing deletes it through this interface.
    ~TSecondaryMapViewFrontend() = default;
};

// The secondary map views as core code drives them. TMapViewManager implements it; core code
// reaches it through TMap::mapViewsFrontend().
class TMapViewsFrontend
{
public:
    // A view id of 0 means no view was made, and the QString says why.
    virtual std::pair<int, QString> createView(int initialAreaId) = 0;
    virtual std::pair<bool, QString> closeView(int viewId) = 0;
    virtual int closeAllViews() = 0;
    // Null for an id with no open view.
    virtual TSecondaryMapViewFrontend* view(int viewId) = 0;
    virtual QList<int> getViewIds() const = 0;
    virtual void updateAllViews() = 0;
    // Moves every view still showing areaId to another area, for after that area is deleted.
    virtual void switchViewsShowingArea(int areaId) = 0;

protected:
    // The manager is a child of its TMap, which deletes it, so nothing deletes it through this interface.
    ~TMapViewsFrontend() = default;
};

#endif // MUDLET_TMAPVIEWSFRONTEND_H
