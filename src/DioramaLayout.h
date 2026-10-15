/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "EnvironmentKit.h"

#include <QHash>
#include <QList>

// How the diorama map style builds each room as a piece on its board, by the room's EnvironmentKit
namespace DioramaLayout {

using Kit = EnvironmentKit::Kit;

// Where a room stands on the board
struct Place
{
    int roomId = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    Kit kit = Kit::Plain;
};

// A room's piece, its heights on the map's z axis, where each level is one unit above the one below
struct Piece
{
    Kit kit = Kit::Plain;
    // The block's or tile's foot and its flat top
    float bottom = 0.0f;
    float top = 0.0f;
    // The tip of the roof or of the tallest tree, or top where there is neither
    float peak = 0.0f;
    // Across the room's spacing
    float halfWidth = 0.0f;
    // Counted up from the board, in a building that spans several levels; 0 for a piece that stands alone
    int storey = 0;
    int storeys = 1;
};

// The board lies under the viewed level, its top where that level's rooms stand. A room on the viewed level with
// rooms on the levels straight above it is the foot of a tall building, with a storey for each and a roof on top;
// its storeys stand lower than their levels
QHash<int, Piece> layout(const QList<Place>& places, int viewedLevel, float roomSize, float roomHeight);

// How far below its level a room's ground is
float groundBelowLevel(float roomHeight);
// How deep the hole cut in the board for water goes, down to the water's underside
float waterPitDepth(float roomHeight);

} // namespace DioramaLayout
