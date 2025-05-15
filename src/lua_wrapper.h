#ifndef LUA_WRAPPER_H

extern "C" {
    #if defined(__MINGW64__)
        #include <lua5.1/lua.h>
        #include <lua5.1/lauxlib.h>
        #include <lua5.1/lualib.h>
    #else
        #include <lua.h>
        #include <lauxlib.h>
        #include <lualib.h>
    #endif
}

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif
#pragma message("Using LUA version: " TOSTRING(LUA_VERSION))

#endif