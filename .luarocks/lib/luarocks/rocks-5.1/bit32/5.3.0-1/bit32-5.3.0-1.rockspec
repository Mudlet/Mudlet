package = "bit32"

version = "5.3.0-1"

source = {
   url = "git://github.com/keplerproject/lua-compat-5.2.git",
   tag = "bitlib-5.3.0",
}

description = {
   summary = "Lua 5.2 bit manipulation library",
   detailed = [[
      bit32 is the native Lua 5.2 bit manipulation library, in the version
      from Lua 5.3; it is compatible with Lua 5.1, 5.2 and 5.3.
   ]],
   license = "MIT/X11",
   homepage = "http://www.lua.org/manual/5.2/manual.html#6.7",
}

dependencies = {
   "lua >= 5.1, <= 5.3"
}

build = {
   type = "builtin",
   modules = {
      bit32 = {
         sources = { "lbitlib.c" },
         incdirs = { "c-api" },
      }
   }
}
