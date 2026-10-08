-- Specs that need a profile with no mapper at all. Once the map widget is
-- opened, the mapper behind it lives for the rest of the session and
-- closeMapWidget() only hides it; an embedded mapper from createMapper() or
-- Geyser.Mapper persists the same way. So this file is named to sort ahead of
-- every spec that makes one: busted runs its files in sorted order.

describe("Tests secondary map views in a profile without a mapper", function()
  local areaId, roomId

  setup(function()
    -- getMapZoom() answers this exactly when TMap::mpMapper is null
    local _, message = getMapZoom()
    assert.are.equal("no active mapper", message,
      "this profile already has a mapper (an earlier spec or an earlier run made one), so this file can no longer test a profile without one")

    areaId = addAreaName("BeforeMapWidgetSpec")
    roomId = createRoomID()
    assert.is_true(addRoom(roomId))
    assert.is_true(setRoomArea(roomId, areaId))
  end)

  teardown(function()
    if roomId then
      deleteRoom(roomId)
      deleteArea(areaId)
    end
  end)

  it("creates, paints and closes a view without the main mapper", function()
    local viewId = createMapView(areaId)
    assert.is_number(viewId)
    finally(function() closeMapView(viewId) end)

    pumpEvents(200)

    assert.are.equal(areaId, getMapViewInfo(viewId).areaId)
  end)
end)

-- These settings live on the Host or on mudlet, and a mapper made later picks
-- them up, so a startup script can set them before there is one
describe("Tests setConfig for the map keys in a profile without a mapper", function()
  local saved = {}
  local keys = {"mapRoomSize", "mapExitSize", "mapRoundRooms", "showRoomIdsOnMap", "mapShowGrid",
                "mapShowRoomBorders", "showUpperLowerLevels", "mapInfoColor"}

  setup(function()
    local _, message = getMapZoom()
    assert.are.equal("no active mapper", message, "this profile already has a mapper, so this file can no longer test a profile without one")
    for _, key in ipairs(keys) do
      saved[key] = getConfig(key)
    end
  end)

  teardown(function()
    for _, key in ipairs(keys) do
      pcall(setConfig, key, saved[key])
    end
  end)

  it("sets and reads back each map key", function()
    local wanted = {
      mapRoomSize = saved.mapRoomSize == 7 and 8 or 7,
      mapExitSize = saved.mapExitSize == 4 and 6 or 4,
      mapRoundRooms = not saved.mapRoundRooms,
      showRoomIdsOnMap = not saved.showRoomIdsOnMap,
      mapShowGrid = not saved.mapShowGrid,
      mapShowRoomBorders = not saved.mapShowRoomBorders,
      showUpperLowerLevels = not saved.showUpperLowerLevels,
      mapInfoColor = {10, 20, 30, 40},
    }
    for _, key in ipairs(keys) do
      local ok, err = setConfig(key, wanted[key])
      assert.is_true(ok, key .. " was refused: " .. tostring(err))
      assert.are.same(wanted[key], getConfig(key), key .. " did not read back")
    end
  end)

  it("shows and hides a map info contributor", function()
    local name = next(getMapInfo())
    assert.is_string(name, "the map has no info contributors to toggle")
    local wasShown = getMapInfo()[name]
    finally(function()
      setConfig(wasShown and "showMapInfo" or "hideMapInfo", name)
    end)
    assert.is_true(setConfig("hideMapInfo", name))
    assert.is_false(getMapInfo()[name])
    assert.is_true(setConfig("showMapInfo", name))
    assert.is_true(getMapInfo()[name])
  end)

  -- stored for the mapper made later to open in; a build without the 3D
  -- mapper does not know the key at all
  it("stores show3dMapView until there is a mapper", function()
    local ok, err = setConfig("show3dMapView", true)
    if err == "'show3dMapView' isn't a valid configuration option" then
      return
    end
    -- false again, or the mapper a later spec opens would come up in 3D
    finally(function()
      setConfig("show3dMapView", false)
    end)
    assert.is_true(ok, tostring(err))
    assert.is_true(getConfig("show3dMapView"))
    assert.is_true(setConfig("show3dMapView", false))
    assert.is_false(getConfig("show3dMapView"))
  end)
end)
