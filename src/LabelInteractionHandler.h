/***************************************************************************
 *   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
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

#ifndef MUDLET_LABELINTERACTIONHANDLER_H
#define MUDLET_LABELINTERACTIONHANDLER_H

#include "T2DMap.h"

class TArea;

class LabelInteractionHandler : public T2DMap::IInteractionHandler
{
public:
    explicit LabelInteractionHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

    // The labels drawn below the rooms at this point on the level shown, the one drawn on top first
    static QList<int> belowRoomsLabelsAt(const T2DMap& mapWidget, const TArea& area, const QPoint& widgetPosition);

private:
    bool handleMousePress(T2DMap::MapInteractionContext& context) const;
    bool handleMouseMove(T2DMap::MapInteractionContext& context) const;
    bool handleMouseRelease(T2DMap::MapInteractionContext& context) const;

    T2DMap& mMapWidget;
};

#endif // MUDLET_LABELINTERACTIONHANDLER_H
