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
