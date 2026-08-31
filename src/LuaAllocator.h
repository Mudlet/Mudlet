#ifndef MUDLET_LUAALLOCATOR_H
#define MUDLET_LUAALLOCATOR_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

struct lua_State;

// Creates a lua_State backed by Mudlet's size-classed allocator instead of the
// one malloc/free per Lua object that luaL_newstate() installs. Otherwise
// equivalent to luaL_newstate(): no standard library is opened and the same
// panic function is installed.
lua_State* mudletNewLuaState();

#endif // MUDLET_LUAALLOCATOR_H
