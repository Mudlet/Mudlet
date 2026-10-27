# Lua 5.1.5

The library sources of [Lua 5.1.5](https://www.lua.org/ftp/lua-5.1.5.tar.gz)
(sha256 `2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333`),
without the stand-alone `lua` and `luac`. Built unless CMake is given
`-DUSE_BUNDLED_LUA=OFF` (or the environment `WITH_BUNDLED_LUA=NO`), or by
default on a Linux without Debian's multiarch layout; Mudlet then links the
system's Lua 5.1.

Every change from upstream is listed here, and is marked `MUDLET` in the source.

## Changes

- `ldo.c`: Debian's fix for a stack overflow in vararg functions
  (`debian/patches/0004`), which every Linux build of Mudlet already ran with.
- `luaconf.h`: the default `package.path` and `package.cpath` of the Lua each
  platform's Mudlet linked before - Debian's on Linux (`LUA_MULTIARCH`, set by
  CMake), MSYS2's on Windows, upstream's on macOS.
