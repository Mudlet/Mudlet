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

#include <QColor>
#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>
#include <utility>

class Host;
class TMap;

// What the diorama map style builds a room as: its environment decides the piece's shape as well as its color.
// A game names the kit for an environment, and until it does one is guessed from the environment's color
namespace DioramaKit {

enum class Kit {
    // A block, as the other styles draw a room
    Plain,
    // Sunk below the board, flat and glossy
    Water,
    // Flat paving, level with the board
    Road,
    // A grass tile with a few small trees on it
    Forest,
    // A block with a pitched roof
    Building
};

// The lower-case name setMapEnvironmentKit() takes
QString name(Kit kit);
std::optional<Kit> fromName(const QString& name);
QStringList names();

// Blue is water, green forest, grey, brown and tan road, anything else a building. Nearly black is a road and
// nearly white a building, as neither has a hue to go by
Kit guess(const QColor& environmentColor);

// The map user data key the kits a game set are saved under, as a JSON object of environment ID to kit name
QString userDataKey();
// Those set in the map's user data; an entry that cannot be read is left out
QHash<int, Kit> stored(const QMap<QString, QString>& mapUserData);
// Saves kits in the map's user data, removing the key when there are none
void store(QMap<QString, QString>& mapUserData, const QHash<int, Kit>& kits);

// The kit for an environment, and whether the map names it rather than it being guessed from the environment's color
std::pair<Kit, bool> forEnvironment(const TMap& map, const Host& host, int environmentId);

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

} // namespace DioramaKit
