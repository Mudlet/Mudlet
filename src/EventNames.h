#ifndef MUDLET_EVENTNAMES_H
#define MUDLET_EVENTNAMES_H

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

#include <QStringList>

namespace eventNames {

enum class Source { Mudlet, Game, Script };

struct Entry
{
    QString name;
    Source source;
};

bool fromGame(const QString& name);

// Leaves out gmcp.*, msdp.*, mssp.* and mxp.*: the game names those, see fromGame()
QStringList systemEvents();

// Read off string literals only, so a name built at runtime is not found.
// Unique, in order of first appearance.
QStringList namesInCode(const QString& code);

} // namespace eventNames

#endif // MUDLET_EVENTNAMES_H
