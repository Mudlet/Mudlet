-- Run in a mudlet-headless whose mudlet_core carries Mudlet's Lua files and the Lua C modules itself
-- (USE_BUILTIN_MUDLET_LUA and USE_BUNDLED_LUA_MODULES), with neither on disk, by
-- .github/workflows/core-mobile-build.yml: each has to have come from the library, and to work.

assert(Geyser, "Mudlet's own Lua files did not load")
assert(not luaGlobalPath:find("^:"), "the Lua files were not unpacked, but read from " .. luaGlobalPath)
assert(lfs.attributes(luaGlobalPath .. "/translations/mudlet-lua.json", "mode") == "file")

for _, name in ipairs({"lfs", "brimworks.zip", "rex_pcre2", "luasql.sqlite3", "lua-utf8", "yajl", "lpeg"}) do
  assert(type(package.preload[name]) == "function", name .. " is not built in")
end

assert(lfs.attributes(getMudletHomeDir(), "mode") == "directory")
assert(rex.match("abc123", [[\d+]]) == "123")
assert(rex_pcre == rex)
assert(utf8.len("h\195\164h") == 3)
assert(yajl.to_value([[{"answer": 42}]]).answer == 42)
assert(lpeg.match(lpeg.R("09") ^ 1, "42x") == 3)
assert(db:create("coremobilesmoke", {things = {"name"}}))

-- A package is a zip archive, which Mudlet's Lua opens with the zip module and unpacks with lfs
local archive = "src/mudlet-lua/tests/fixtures/packages/mudlet-spec-manifestonly.mpackage"
local unpacked = getMudletHomeDir() .. "/coremobilesmoke"
assert(lfs.mkdir(unpacked))
assert(unzip(archive, unpacked))
assert(lfs.attributes(unpacked .. "/config.lua", "mode") == "file")
