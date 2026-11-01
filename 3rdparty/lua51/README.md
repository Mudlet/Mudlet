# Lua 5.1.5

The library sources of [Lua 5.1.5](https://www.lua.org/ftp/lua-5.1.5.tar.gz)
(sha256 `2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333`),
without the stand-alone `lua` and `luac`. Built by default except on a Linux
without Debian's multiarch layout; `-DUSE_BUNDLED_LUA=OFF` (or the environment
`WITH_BUNDLED_LUA=NO`) turns it off, and Mudlet then links the system's Lua 5.1
and makes every check the write barrier saves.

Every change from upstream is listed here, and is marked `MUDLET` in the source
or is a call of one of the `luaC_watch` macros.

## Changes

- `ldo.c`: the fix for CVE-2014-5461, a stack overflow in vararg functions
  (Debian's `debian/patches/0004`).
- `luaconf.h`: the default `package.path` and `package.cpath` of the system Lua
  each platform's Mudlet links with `USE_BUNDLED_LUA` off - Debian's on Linux
  (`LUA_MULTIARCH`, set by CMake), MSYS2's on Windows, upstream's on macOS.
- `loslib.c`: on iOS, whose SDK marks `system()` unavailable, `os.execute()`
  answers as `system()` does with no command processor - `0` with no command,
  and with one `-1`, POSIX's value for a shell that could not be started - so
  scripts see a non-zero status, as for any command that fails. Lua 5.4's
  `l_system()` gives the same values under `LUA_USE_IOS`, which CMake defines
  when `CMAKE_SYSTEM_NAME` is `iOS`.
- The write barrier, below: `lua.h`, `lgc.h`, `lstate.h`, `lstate.c`, `lvm.c`,
  `lapi.c` and `loadlib.c`.

## Write barrier

Between trigger fires Mudlet relies on a few facts about the Lua state: that
the globals table still carries its `__index`/`__newindex` handlers, that
`matches`, `multimatches` and `line` are still absent where Mudlet cleared
them, and that the empty table it reuses as `matches` is still empty with no
metatable. A script can change any of them through a path no Lua-level hook
sees: a C module, `rawset()`, `table.insert()`, a kept reference,
`debug.setmetatable()`, `setfenv(0, ...)`, or bytecode. So without help, every
fire has to look again.

`lua_watch(L, idx, 1)` marks the table or string at `idx` with bit 7 of its GC
header, which the collector never touches, and returns a pointer to a counter.
The counter moves on:

- a store into a watched table, or under a watched string key into any table -
  `luaV_settable()` (every assignment, `lua_settable()` and `lua_setfield()`),
  `OP_SETLIST`, `lua_rawset()` and `lua_rawseti()`, which between them are
  every store the API and the VM make into an existing table
- `lua_setmetatable()` on a watched table, or on one whose metatable is watched
- `lua_setfenv()` on a thread, and `lua_replace()` of `LUA_GLOBALSINDEX` or
  `LUA_REGISTRYINDEX`

C modules make their stores through these functions too. A library with a Lua
core of its own does not, so once `package.loadlib()` or `require()` loads one -
on Windows, once either loads any library while one is in the process -
`lua_watch()` returns NULL for that state from then on, and the counter moves
once so that whoever holds it looks again. A core whose symbols the library
hides, or that it loads only after its `luaopen_` function returns, is not seen;
Lua 5.1 does not support two cores sharing a state anyway, as each compares
tables against its own empty node. The collector clearing a weak table makes no
store either, so Mudlet stops trusting the counter once the registry or the
globals metatable has a metatable.

A watched string is also fixed, so it is never collected and recreated
unwatched. Mudlet watches the registry, the globals metatable, the empty
`matches` table and the three names. When the counter has not moved since a
fact was last checked, or moved only by a store of Mudlet's own, the fact still
holds and Mudlet skips the check.
