#ifndef MUDLET_LUALOCATION_H
#define MUDLET_LUALOCATION_H
/***************************************************************************
 *   Copyright (C) 2014 by Stephen Lyons - slysven@virginmedia.com         *
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
 
/*
 * THIS FILE IS MODIFIED/REWRITTED EACH TIME THAT THE QMAKE OR CMAKE PROJECT
 * FILES ARE PARSED - USER CHANGES TO THIS FILE WILL BE OVERWRITTEN!
 */
 
/*
 * It should NOT be copied to a shared, readonly, location when the other,
 * actual Lua files in this directory are installed on systems which support
 * such a situation, e.g. most *nixes with a /usr/share part in their file
 * system; its purpose is to permit the Mudlet application to differentiate
 * between: a development version of those Lua files, in the collection of
 * source files for this project and a normal working set installed into the
 * system for normal use.  For systems/installations where two such sets are
 * not maintained, or for release builds in package form in a distribution and
 * used by others, the physical absence of this file at run-time will cleanly
 * disable the ability to choose to use development versions of the core Mudlet
 * Lua files.
 */
 
#include <QString>
 
const QString sourceLuaHeaderPathFile = QStringLiteral( "/usr/local/src/Mudlet2/tester/src/mudlet-lua/lua/luaLocation.h" );
 
#endif // MUDLET_LUALOCATION_H
