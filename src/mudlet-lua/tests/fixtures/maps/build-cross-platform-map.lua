-- Rebuilds cross-platform-map.dat and cross-platform-map.json beside this file, and the cross-platform-map-v<N> pair
-- when this Mudlet can save a format newer than its default; README.md says how to run it
local directory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]") or "."
-- saveMap() and loadMap() take a relative path as relative to the profile folder
if not directory:match("^/") and not directory:match("^%a:") then
  directory = lfs.currentdir() .. "/" .. directory
end
local mapPath = directory .. "/cross-platform-map.dat"
local jsonPath = directory .. "/cross-platform-map.json"
-- Both files hold translated area names, and the specs and the smoke test expect English ones
assert(mudlet.translations.interfacelanguage == "en_US",
       "the interface language is " .. tostring(mudlet.translations.interfacelanguage) .. ", not en_US: see README.md")

local function readFile(path)
  local file = assert(io.open(path, "rb"))
  local contents = file:read("*a")
  file:close()
  return contents
end

deleteMap()
local north = addAreaName("GoldenNorth")
local south = addAreaName("GoldenSouth")
for id = 1, 5 do
  assert(addRoom(id))
  assert(setRoomArea(id, id <= 3 and north or south))
  assert(setRoomName(id, "Room " .. id))
end
assert(setRoomCoordinates(1, 0, 0, 0))
assert(setRoomCoordinates(2, 1, 0, 0))
assert(setRoomCoordinates(3, 1, 1, 0))
assert(setRoomCoordinates(4, 0, 0, -1))
assert(setRoomCoordinates(5, 0, -1, -1))
assert(setExit(1, 2, "e"))
assert(setExit(2, 1, "w"))
assert(setExit(2, 3, "n"))
assert(setExit(3, 2, "s"))
assert(setExit(3, 4, "d"))
assert(setExit(4, 3, "u"))
assert(addSpecialExit(4, 5, "climb down the well"))
assert(addSpecialExit(5, 4, "climb up"))
assert(lockSpecialExit(5, 4, "climb up", true))
assert(lockExit(3, "d", true))
assert(setExitWeight(1, "e", 3))
assert(setDoor(2, "n", 2))
setExitStub(5, "s", true)
assert(setRoomWeight(4, 7))
assert(lockRoom(5, true))
assert(setRoomEnv(1, 300))
assert(setCustomEnvColor(300, 10, 20, 30, 255))
assert(setRoomChar(3, "⌂"))
assert(setRoomIDbyHash(5, "south-hash"))
assert(setRoomUserData(1, "note", "a fountain"))
assert(setAreaUserData(north, "climate", "temperate"))
assert(setMapUserData("golden", "yes"))
assert(addCustomLine(1, {{0.5, 0.25, 0}, {0.75, -1.5, 0}}, "e", "dash line", {200, 100, 50}, true))
assert(createMapLabel(north, "Well", 0.5, 1.5, 0, 240, 230, 140, 20, 40, 60, 30.0, 10, false, false,
                      "Bitstream Vera Sans Mono", 255, 128, false, 200, 10, 10) >= 0)
assert(setRoomBorderColor(1, 1, 2, 3, 200))
assert(setRoomBorderThickness(1, 4))
assert(setGridMode(south, true))
-- Below format 21 the hidden flag, the symbol colour and the 2D zoom are saved as user data and restored from it
assert(setRoomHidden(5, true))
assert(setRoomCharColor(3, 250, 128, 64))
-- setMapZoom() needs a map view, and the headless build's is a stand-in
assert(openMapWidget())
assert(setMapZoom(7.5, north))
assert(saveMap(mapPath))

local normalised = dofile(directory .. "/normalise-map-json.lua")
-- exported from the saved file, as the specs do, not from the map built above
local function exportJson(map, jsonFile)
  assert(loadMap(map))
  assert(saveJsonMap(jsonFile))
  local json = normalised(readFile(jsonFile))
  local file = assert(io.open(jsonFile, "wb"))
  file:write(json)
  file:close()
  echo("wrote " .. map .. " and " .. jsonFile .. "\n")
end

for entry in lfs.dir(directory) do
  local extension = entry:match("^cross%-platform%-map%-v%d+%.(%a+)$")
  if extension == "dat" or extension == "json" then
    assert(os.remove(directory .. "/" .. entry))
  end
end

local function formatVersion(path)
  -- QDataStream writes the leading int32 big-endian
  local b1, b2, b3, b4 = readFile(path):byte(1, 4)
  return ((b1 * 256 + b2) * 256 + b3) * 256 + b4
end
-- TMap::mMaxVersion can be above the default for development, and saveMap() refuses only a format beyond it, with
-- an [ ERROR ] line
local defaultVersion = formatVersion(mapPath)
local newestVersion = defaultVersion
local probePath = getMudletHomeDir() .. "/cross-platform-map-probe.dat"
while saveMap(probePath, newestVersion + 1) do
  newestVersion = newestVersion + 1
end
os.remove(probePath)
local newerMapPath, newerJsonPath
if newestVersion > defaultVersion then
  local newerName = string.format("%s/cross-platform-map-v%d", directory, newestVersion)
  newerMapPath, newerJsonPath = newerName .. ".dat", newerName .. ".json"
  assert(saveMap(newerMapPath, newestVersion))
end

-- Only now, as each export loads a file, and a map loaded from the default format keeps only what that format stores
exportJson(mapPath, jsonPath)
if newerMapPath then
  exportJson(newerMapPath, newerJsonPath)
end
