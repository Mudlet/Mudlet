-- Specs for the globals defined in lua/LuaGlobal.lua.

local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
assert(specDirectory, "LuaGlobal_spec.lua has to be run from a file so that it can find its fixtures")
local fixtureDirectory = specDirectory .. "/fixtures/packages"

-- lfs.rmdir only removes empty folders, so take the tree unzip() wrote apart
-- from the leaves up. Names are gathered before anything is removed so the
-- directory is not mutated while lfs.dir is still walking it.
local function removeTree(path)
  local mode = lfs.attributes(path, "mode")
  if not mode then
    return
  end
  if mode == "directory" then
    local names = {}
    for entry in lfs.dir(path) do
      if entry ~= "." and entry ~= ".." then
        names[#names + 1] = entry
      end
    end
    for _, name in ipairs(names) do
      removeTree(path .. "/" .. name)
    end
    lfs.rmdir(path)
  else
    os.remove(path)
  end
end

local function fileContents(path)
  local handle = io.open(path, "rb")
  if not handle then
    return nil
  end
  local contents = handle:read("*a")
  handle:close()
  return contents
end

describe("Tests the functionality of unzip", function()
  -- unzip() drives whichever zip library Mudlet preloaded as the global `zip`:
  -- lua-zip (brimworks) on every official build, luazip (Kepler) on the dev
  -- builds that fall back to it (see TLuaInterpreter.cpp). Both have to unpack
  -- the same tree, so this runs against the real module rather than a mock. The
  -- resources fixture is used because it carries a nested folder as well as
  -- top-level files, so one extraction exercises both the directory and the
  -- file paths through the function.
  local archivePath = fixtureDirectory .. "/mudlet-spec-resources.mpackage"
  -- unzip() joins dest and each entry name as-is, so dest ends in a separator -
  -- the documented "/tmp/out/" calling shape. The folder is not named
  -- mudlet-spec-* so it does not trip Package_spec's leftover-folder guard if a
  -- teardown ever fails to run.
  local destinationRoot = getMudletHomeDir() .. "/busted-unzip-spec"
  local destination = destinationRoot .. "/"

  setup(function()
    -- a nil zip module is a broken environment, not a reason the assertions
    -- below should quietly pass; lua-zip is a required rock on every platform
    -- the suite runs on
    assert.is_table(zip, "the zip module is missing, so unzip() cannot run")
    removeTree(destinationRoot)
    -- unzip() writes into dest but does not create dest itself
    lfs.mkdir(destinationRoot)
    -- With the pre-fix body this raises under brimworks ("attempt to call
    -- method 'files'"), failing every spec in the block; the point of the fix is
    -- that it now returns having written the tree out.
    unzip(archivePath, destination)
  end)

  teardown(function()
    removeTree(destinationRoot)
  end)

  it("extracts the archive's top-level files", function()
    assert.is_not_nil(lfs.attributes(destination .. "config.lua", "mode"), "config.lua was not extracted")
    assert.is_not_nil(lfs.attributes(destination .. "mudlet-spec-resources.xml", "mode"), "the package XML was not extracted")
  end)

  it("recreates the archive's nested folders and their files", function()
    assert.equals("directory", lfs.attributes(destination .. "resources", "mode"))
    assert.equals("directory", lfs.attributes(destination .. "resources/nested", "mode"))
    assert.is_not_nil(lfs.attributes(destination .. "resources/spec-note.txt", "mode"), "resources/spec-note.txt was not extracted")
    assert.is_not_nil(lfs.attributes(destination .. "resources/nested/spec-nested.txt", "mode"), "the nested file was not extracted")
  end)

  it("writes each entry's contents out", function()
    local note = fileContents(destination .. "resources/spec-note.txt")
    assert.is_string(note)
    assert.is_not_nil(note:find("mudlet-spec-resources fixture resource", 1, true), tostring(note))
    local nested = fileContents(destination .. "resources/nested/spec-nested.txt")
    assert.is_string(nested)
    assert.is_not_nil(nested:find("in a nested folder", 1, true), tostring(nested))
  end)
end)

describe("Tests unzip on an archive that cannot be opened", function()
  local destinationRoot = getMudletHomeDir() .. "/busted-unzip-missing"
  local destination = destinationRoot .. "/"

  teardown(function()
    removeTree(destinationRoot)
  end)

  it("returns without raising when the archive is not there", function()
    -- zip.open() answers nil + message for a missing file, so unzip() prints the
    -- error and returns early. This is the path that hid the brimworks breakage
    -- for years: a missing archive never reaches the files()/read("*a") idioms
    -- that only Kepler understands, so the function looked like it merely failed
    -- quietly rather than raising on every real archive.
    local ok = pcall(unzip, fixtureDirectory .. "/mudlet-spec-there-is-no-such-archive.mpackage", destination)
    assert.is_true(ok, "unzip() raised on a missing archive instead of returning quietly")
  end)
end)
