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

#include "DioramaLayout.h"

#include <QSet>

namespace DioramaLayout {

namespace {
// In rooms' heights, above the ground a room stands on
constexpr float scmWaterDepth = 0.15f;
constexpr float scmWaterThickness = 0.06f;
constexpr float scmRoadThickness = 0.045f;
constexpr float scmGrassThickness = 0.09f;
constexpr float scmTreeHeight = 1.0f;
constexpr float scmBuildingHeight = 1.5f;
constexpr float scmRoofHeight = 0.85f;
// Across rooms' spacing: flat tiles meet their neighbours, as paving and water do
constexpr float scmTileHalfWidth = 0.5f;
constexpr float scmGrassHalfWidth = 0.48f;
} // namespace

float groundBelowLevel(const float roomHeight)
{
    return roomHeight / 2.0f;
}

float waterPitDepth(const float roomHeight)
{
    return (scmWaterDepth + scmWaterThickness) * roomHeight;
}

QHash<int, Piece> layout(const QList<Place>& places, const int viewedLevel, const float roomSize, const float roomHeight)
{
    QHash<std::pair<int, int>, QSet<int>> levelsAt;
    for (const Place& place : places) {
        levelsAt[{place.x, place.y}].insert(place.z);
    }
    const float ground = groundBelowLevel(roomHeight);
    QHash<int, Piece> pieces;
    pieces.reserve(places.size());
    for (const Place& place : places) {
        Piece piece;
        piece.kit = place.kit;
        const float foot = static_cast<float>(place.z) - ground;
        const QSet<int> levels = levelsAt.value({place.x, place.y});
        int storeys = 0;
        if (place.z >= viewedLevel && levels.contains(viewedLevel)) {
            while (levels.contains(viewedLevel + storeys)) {
                ++storeys;
            }
        }
        if (storeys > 1 && place.z < viewedLevel + storeys) {
            piece.kit = Kit::Building;
            piece.storey = place.z - viewedLevel;
            piece.storeys = storeys;
            piece.halfWidth = roomSize / 2.0f;
            // Storeys a building's height apart rather than a level apart, which would make a tower tower over everything
            const float base = static_cast<float>(viewedLevel) - ground;
            piece.bottom = base + static_cast<float>(piece.storey) * scmBuildingHeight * roomHeight;
            piece.top = piece.bottom + scmBuildingHeight * roomHeight;
            piece.peak = piece.storey == storeys - 1 ? piece.top + scmRoofHeight * roomHeight : piece.top;
            pieces.insert(place.roomId, piece);
            continue;
        }
        switch (place.kit) {
        case Kit::Water:
            piece.halfWidth = scmTileHalfWidth;
            piece.top = foot - scmWaterDepth * roomHeight;
            piece.bottom = piece.top - scmWaterThickness * roomHeight;
            piece.peak = piece.top;
            break;
        case Kit::Road:
            piece.halfWidth = scmTileHalfWidth;
            piece.bottom = foot;
            piece.top = foot + scmRoadThickness * roomHeight;
            piece.peak = piece.top;
            break;
        case Kit::Forest:
            piece.halfWidth = scmGrassHalfWidth;
            piece.bottom = foot;
            piece.top = foot + scmGrassThickness * roomHeight;
            piece.peak = piece.top + scmTreeHeight * roomHeight;
            break;
        case Kit::Building:
            piece.halfWidth = roomSize / 2.0f;
            piece.bottom = foot;
            piece.top = foot + scmBuildingHeight * roomHeight;
            piece.peak = piece.top + scmRoofHeight * roomHeight;
            break;
        case Kit::Plain:
            piece.halfWidth = roomSize / 2.0f;
            piece.bottom = foot;
            piece.top = foot + roomHeight;
            piece.peak = piece.top;
            break;
        }
        pieces.insert(place.roomId, piece);
    }
    return pieces;
}

} // namespace DioramaLayout
