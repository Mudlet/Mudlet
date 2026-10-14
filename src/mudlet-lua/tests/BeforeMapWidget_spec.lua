-- Specs that need a profile with no mapper at all. Once the map widget is
-- opened, the mapper behind it lives for the rest of the session and
-- closeMapWidget() only hides it; an embedded mapper from createMapper() or
-- Geyser.Mapper persists the same way. So this file is named to sort ahead of
-- every spec that makes one: busted runs its files in sorted order.

-- First in the first file to touch the map: loading, clearing or opening a
-- map each copies the profile's sixteen ANSI colours into it again, and so
-- does reading a saved profile, so this bites only on a profile that was new
-- when it was loaded, as CI's is. Earlier specs save it during the run, so
-- saved files existing now don't mean it was loaded from one
describe("Tests the environment colours a profile starts out with", function()
  it("starts environments 257 to 272 out as the profile's ANSI colours", function()
    local atStart = getCustomEnvColorTable()
    -- a new profile's defaults, in the order the map files them
    local defaults = {
      {128, 0, 0, 255}, {0, 128, 0, 255}, {128, 128, 0, 255}, {0, 0, 128, 255},
      {128, 0, 128, 255}, {0, 128, 128, 255}, {192, 192, 192, 255}, {0, 0, 0, 255},
      {255, 0, 0, 255}, {0, 255, 0, 255}, {255, 255, 0, 255}, {0, 0, 255, 255},
      {255, 0, 255, 255}, {0, 255, 255, 255}, {255, 255, 255, 255}, {128, 128, 128, 255},
    }
    assert.is_true(deleteMap())
    local copiedAgain = getCustomEnvColorTable()
    for env = 257, 272 do
      assert.are.same(defaults[env - 256], atStart[env], "environment " .. env)
      assert.are.same(copiedAgain[env], atStart[env], "environment " .. env)
    end
  end)
end)

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

-- A profile with no console view answers these from the model instead (HeadlessHostSmokeTest)
describe("Tests the map functions that wait for a mapper in a profile without one", function()
  local jsonPath = getMudletHomeDir() .. "/before_map_widget.json"
  local areaId, roomId

  setup(function()
    local _, message = getMapZoom()
    assert.are.equal("no active mapper", message, "this profile already has a mapper, so this file can no longer test a profile without one")

    areaId = addAreaName("BeforeMapWidgetSpecRefusals")
    roomId = createRoomID()
    assert.is_true(addRoom(roomId))
    assert.is_true(setRoomArea(roomId, areaId))
  end)

  teardown(function()
    os.remove(jsonPath)
    if roomId then
      deleteRoom(roomId)
      deleteArea(areaId)
    end
  end)

  it("refuses them although the map holds a room", function()
    assert.are.same({nil, "you haven't opened a map yet"}, {getPlayerRoom()})
    assert.are.same({nil, "you haven't opened a map yet"}, {centerview(roomId)})
    assert.are.same({nil, "no map present or loaded"}, {saveJsonMap(jsonPath)})
    assert.are.same({nil, "no map present or loaded"}, {loadJsonMap(jsonPath)})
  end)

  it("refuses the mapper settings and an area's zoom", function()
    assert.are.same({false}, {setDefaultAreaVisible(true)})
    assert.are.same({nil, "no active mapper"}, {getMapZoom(areaId)})
    assert.are.same({nil, "'mapRoomSize' isn't a valid configuration option"}, {setConfig("mapRoomSize", getConfig("mapRoomSize"))})
    assert.are.same({nil, "'mapExitSize' isn't a valid configuration option"}, {setConfig("mapExitSize", getConfig("mapExitSize"))})
  end)

  it("lists no secondary map views", function()
    assert.are.same({{}}, {getMapViewIds()})
    assert.are.same({0}, {closeAllMapViews()})
  end)

  it("refuses the map selection", function()
    assert.are.same({nil, "no map present or loaded"}, {getMapSelection()})
    assert.are.same({nil, "no map present or loaded"}, {clearMapSelection()})
  end)
end)
