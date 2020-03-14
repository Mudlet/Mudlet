-- LuaRocks configuration

rocks_trees = {
   { name = "user", root = home .. "/.luarocks" };
   { name = "system", root = "/home/runner/work/Mudlet/Mudlet/.luarocks" };
}
lua_interpreter = "lua";
variables = {
   LUA_DIR = "/home/runner/work/Mudlet/Mudlet/.lua";
   LUA_BINDIR = "/home/runner/work/Mudlet/Mudlet/.lua/bin";
}
