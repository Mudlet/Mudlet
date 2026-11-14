#include <stdio.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

int main(void)
{
    lua_State* L = luaL_newstate();
    if (!L) {
        fputs("luaL_newstate failed\n", stderr);
        return 1;
    }
    luaL_openlibs(L);
    /* A full collection traverses every weak table, whatever luaL_openlibs's collector steps reached */
    if (luaL_dostring(L, "local weak = setmetatable({}, {__mode = 'kv'}); collectgarbage()")) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }
    lua_close(L);
    puts("luaL_openlibs and a full collection ran");
    return 0;
}
