-- Specs that need a profile whose map widget has never been opened. Once any
-- spec opens it, the mapper behind it lives for the rest of the run and
-- closeMapWidget() only hides it, so this file is named to sort ahead of every
-- spec that opens one: busted runs its files in sorted order.

describe("Tests secondary map views in a profile that never opened the map widget", function()
  local areaId, roomId

  setup(function()
    local _, message = closeMapWidget()
    assert.are.equal("no map widget found to close", message,
      "an earlier spec opened the map widget, so this file no longer tests a profile without one")

    areaId = addAreaName("BeforeMapWidgetSpec")
    roomId = createRoomID()
    assert.is_true(addRoom(roomId))
    assert.is_true(setRoomArea(roomId, areaId))
  end)

  teardown(function()
    deleteRoom(roomId)
    deleteArea(areaId)
  end)

  it("creates, paints and closes a view without the main mapper", function()
    local viewId = createMapView(areaId)
    assert.is_number(viewId)
    finally(function() closeMapView(viewId) end)

    pumpEvents(200)

    assert.are.equal(areaId, getMapViewInfo(viewId).areaId)
  end)
end)
