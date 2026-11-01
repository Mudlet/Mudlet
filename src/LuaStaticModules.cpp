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

#include "LuaStaticModules.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif

int luaopen_lfs(lua_State*);
int luaopen_lpeg(lua_State*);
int luaopen_rex_pcre2(lua_State*);
int luaopen_utf8(lua_State*);
int luaopen_brimworks_zip(lua_State*);
int luaopen_yajl(lua_State*);
}

void registerStaticLuaModules(lua_State* L)
{
    // The names scripts pass to require()
    static constexpr luaL_Reg modules[] = {
            {"lfs", luaopen_lfs},
            {"lpeg", luaopen_lpeg},
            {"rex_pcre2", luaopen_rex_pcre2},
            {"lua-utf8", luaopen_utf8},
            {"brimworks.zip", luaopen_brimworks_zip},
            {"yajl", luaopen_yajl},
    };

    lua_getglobal(L, LUA_LOADLIBNAME);
    lua_getfield(L, -1, "preload");
    for (const auto& [name, open] : modules) {
        lua_pushcfunction(L, open);
        lua_setfield(L, -2, name);
    }
    lua_pop(L, 2);
}
