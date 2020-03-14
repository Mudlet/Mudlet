# lua-compat-5.2

Lua-5.2-style APIs for Lua 5.1.

## What is it

This is a small module that aims to make it easier to write code
in a Lua-5.2-style that is compatible with both Lua 5.1 and Lua 5.2.
This does *not* make Lua 5.1 entirely compatible with Lua 5.2, but
it brings the API closer to that of Lua 5.2.

It includes:

* _For writing Lua_: Lua modules, `compat52` and `compat52.strict`,
  which can be require'd from Lua scripts and run in both Lua 5.1
  and 5.2, plus a backport of `bit32` straight from the Lua 5.2
  sources, adapted to build as a Lua 5.1 module.
* _For writing C_: A C header and file which can be linked to your
  Lua module written in C, providing some functions from the C API
  of Lua 5.2 that do not exist in Lua 5.1, making it easier to write
  C code that compiles with both versions of liblua.

## How to use it

### Lua module

```lua
require("compat52")
```

You have to launch it like this (instead of the usual idiom of storing
the return of `require` in a local variable) because compat52 needs to
make changes to your global environment.

When run under Lua 5.2, this module does nothing.

When run under Lua 5.1, it replaces some of your standard functions and
adds new ones to bring your environment closer to that of Lua 5.2.

You may also use the "strict mode" which removes from Lua 5.1 functions
that were deprecated in 5.2; that is the equivalent of running Lua 5.2
with the LUA_COMPAT_ALL flag disabled:

```lua
require("compat52.strict")
```

The "strict mode" changes the global environment, so it affects all
loaded modules and chunks. If this is undesirable, you can use the
"modular strict mode" which only replaces the environment of the current
file. The usage is slightly different (you have to call the return value
of `require`):

```lua
require("compat52.mstrict")()
```

The effects of `compat52` are still in effect for all chunks, though.

### C code

Add the files `compat-5.2.c` and `compat-5.2.h` to your project and link it
with the rest of your code as usual.

## What's implemented

### Lua

* `load` and `loadfile`
* `table.pack` and `table.unpack`
* `string` patterns may contain embedded zeros
* `string.rep` accepts sep argument
* `string.format` calls `tostring` on arguments for `%s`
* `math.log` accepts base argument
* `xpcall` takes additional arguments
* `pcall` and `xpcall` can execute functions that yield
* metamethods for `pairs` and `ipairs`
* `rawlen` (but `#` still doesn't respect `__len` for tables)
* `collectgarbage`
* `package.searchers`
* `package.searchpath`
* `coroutine` functions dealing with the main coroutine 
* `coroutine.create` accepts functions written in C
* return code of `os.execute`
* `io.write` and `file:write` return file handle
* `io.lines` and `file:lines` accept format arguments (like `io.read`)
* `bit32` (actual backport from the `bit32` library from Lua 5.2,
  also available as a stand-alone rock in the LuaRocks repository)
* `debug.setmetatable` returns object
* `debug.getuservalue` and `debug.setuservalue`
* optional strict mode which removes functions removed or deprecated in
  Lua 5.2, such as `setfenv` and `getfenv`

### C

* `luaL_Reg` (for Lua 5.0)
* `luaL_Unsigned`
* `LUA_FILEHANDLE` (via `lualib.h`) and `luaL_Stream`
* `luaL_addchar` (for Lua 5.0)
* `lua_absindex`
* `lua_arith`
* `lua_compare`
* `lua_tonumberx` and `lua_tointegerx`
* `lua_tounsignedx` and `lua_tounsigned`
* `luaL_checkunsigned` and `luaL_optunsigned`
* `lua_len`, `lua_rawlen`, and `luaL_len`
* `lua_rawgetp` and `lua_rawsetp`
* `lua_copy`
* `lua_getuservalue` and `lua_setuservalue`
* `lua_pushglobaltable`
* `lua_pushunsigned`
* `luaL_testudata`
* `luaL_setfuncs` and `luaL_newlib`
* `luaL_setmetatable`
* `luaL_getsubtable`
* `luaL_traceback`
* `luaL_fileresult`
* `luaL_checkversion` (with empty body, only to avoid compile errors)
* `luaL_tolstring`
* `luaL_requiref`
* `luaL_buffinitsize`, `luaL_prepbuffsize`, and `luaL_pushresultsize`

## What's not implemented

* `_ENV`
* obviously, this does not turn Lua 5.1 into Lua 5.2: syntactic changes
  to the core language, such as the `goto` statement, and semantic
  changes such as ephemeron support for weak tables, remain unavailable.
* `"*L"` format flag for `io.read`, `io.lines`, `file:read`, `file:lines`
* second argument for `os.exit`
* return values of `pipe:close` if `pipe` has been opened by `io.popen`
* `"*"` as second argument for `package.loadlib`
* some functions in the debug library
* anything else missing in the Lua libraries?

## See also

* For more information about compatibility between Lua versions, see
[Compatibility With Lua
Five](http://lua-users.org/wiki/CompatibilityWithLuaFive) at the Lua-Users
wiki
* For Lua-5.1-style APIs under Lua 5.0, see
[Compat-5.1](http://keplerproject.org/compat/)
* for C support in the opposite direction (ie, loading C code using
Lua-5.1-style APIs under Lua 5.2), see
[Twoface](http://corsix.github.io/twoface/)

## Credits

This package contains code written by:

* [The Lua Team](http://www.lua.org)
* Philipp Janda ([@siffiejoe](http://github.com/siffiejoe))
* Tomás Guisasola Gorham ([@tomasguisasola](http://github.com/tomasguisasola))
* Hisham Muhammad ([@hishamhm](http://github.com/hishamhm))
* Renato Maia ([@renatomaia](http://github.com/renatomaia))
