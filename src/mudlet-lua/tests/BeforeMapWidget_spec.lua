-- Specs that need a profile with no mapper at all. Once the map widget is
-- opened, the mapper behind it lives for the rest of the session and
-- closeMapWidget() only hides it; an embedded mapper from createMapper() or
-- Geyser.Mapper persists the same way. So this file is named to sort ahead of
-- every spec that makes one: busted runs its files in sorted order.

-- First in the first file to touch the map: loading, clearing or opening a
-- map each copies the profile's sixteen ANSI colours into it again
describe("Tests the environment colours a profile starts out with", function()
  it("starts environments 257 to 272 out as the profile's ANSI colours", function()
    local atStart = getCustomEnvColorTable()
    deleteMap()
    local copiedAgain = getCustomEnvColorTable()
    for env = 257, 272 do
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
