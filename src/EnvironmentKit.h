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

#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>
#include <utility>

class Host;
class QColor;
class TMap;

// What kind of ground an environment is, for the 3D map styles that draw rooms by it rather than by color
// alone. A game names the kit for an environment, and until it does one is guessed from the environment's color
namespace EnvironmentKit {

enum class Kit { Plain, Water, Road, Forest, Building };

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

} // namespace EnvironmentKit
