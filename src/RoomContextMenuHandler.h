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

#pragma once

#include "T2DMap.h"

class QMenu;
class TArea;

class RoomContextMenuHandler : public T2DMap::IInteractionHandler
{
public:
    explicit RoomContextMenuHandler(T2DMap& mapWidget);

    bool matches(const T2DMap::MapInteractionContext& context) const override;
    bool handle(T2DMap::MapInteractionContext& context) override;

    // The same menu for a right-click on roomId (0 for none) in another view of this map, such as the
    // 3D view, without the actions that need a pointer position on the 2D map; nullptr while a mapper
    // dialog is open. The caller pops it up; it deletes itself on close.
    QMenu* menuForOtherView(int roomId, QWidget* parent);

private:
    QMenu* createMenu(QWidget* parent) const;
    bool populateNoMapActions(QMenu* menu) const;
    bool applyClickedRoom(int clickedRoomId);
    void populateRoomActions(QMenu* menu, TArea* area, bool includePointerActions) const;
    void populateEditModeActions(QMenu* menu, int selectionSize, TArea* area, bool includePointerActions) const;
    void populateViewModeActions(QMenu* menu, int selectionSize) const;
    bool hasCustomLineSelection(const T2DMap::MapInteractionContext& context) const;

    T2DMap& mMapWidget;
};
