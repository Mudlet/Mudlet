-- A build with USE_STATIC_LUA_MODULES links these into Mudlet and puts them in package.preload, for a
-- platform that cannot dlopen them. Any other build loads them from package.cpath, so these pend
-- there, unless MUDLET_EXPECT_STATIC_LUA_MODULES=1 says the build under test must have them.
local modules = { "lfs", "lpeg", "rex_pcre2", "lua-utf8", "brimworks.zip", "yajl" }
local expectStatic = os.getenv("MUDLET_EXPECT_STATIC_LUA_MODULES") == "1"

local function shallowCopy(t)
  local copy = {}
  for k, v in pairs(t) do
    copy[k] = v
  end
  return copy
end

-- A second luaopen_* call refills the module's table and its metatables, undoing what Mudlet put
-- there after the first (the guarded self methods), so put them all back
local function saveModuleState()
  local saved = {}
  local function save(t)
    if type(t) == "table" and not saved[t] then
      saved[t] = shallowCopy(t)
    end
  end
  save(package.loaded)
  for _, loaded in pairs(package.loaded) do
    save(loaded)
  end
  for key, value in pairs(debug.getregistry()) do
    if type(key) == "string" and type(value) == "table" then
      save(value)
      save(value.__index)
    end
  end
  -- Only changed fields are written, so _G keeps pairs and the rest throughout
  return function()
    for t, copy in pairs(saved) do
      for key, value in pairs(t) do
        if copy[key] ~= value then
          t[key] = copy[key]
        end
      end
      for key, value in pairs(copy) do
        if t[key] ~= value then
          t[key] = value
        end
      end
    end
  end
end

describe("Lua C modules linked into Mudlet", function()
  local anyPreloaded = false
  for _, name in ipairs(modules) do
    anyPreloaded = anyPreloaded or package.preload[name] ~= nil
  end
  -- One pending test, not one per module, keeps the pending count steady
  if not anyPreloaded and not expectStatic then
    it("are not in this build", function()
      pending("this build loads them from package.cpath")
    end)
    return
  end

  for _, name in ipairs(modules) do
    describe(name, function()
      it("is in package.preload as a C function", function()
        assert.is_function(package.preload[name])
        assert.equals("C", debug.getinfo(package.preload[name], "S").what)
        assert.is_not_nil(package.loaded[name], name .. " should have loaded at startup")
      end)

      it("loads with nothing on package.path or package.cpath", function()
        local path, cpath = package.path, package.cpath
        local restore = saveModuleState()
        finally(function()
          package.path, package.cpath = path, cpath
          restore()
        end)
        package.path, package.cpath = "", ""
        package.loaded[name] = nil

        local ok, module = pcall(require, name)

        assert.is_true(ok, tostring(module))
        assert.is_table(module)
      end)
    end)
  end
end)
