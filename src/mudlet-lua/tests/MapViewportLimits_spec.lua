-- The 2D mapper works out which rooms could be on screen by inverting the
-- transform that puts them there, and the coordinates that come back are
-- clamped into an int because that is what a room coordinate is. Two ordinary
-- things push a bound onto the clamp. A map holding a room out at the edge of
-- the coordinate space asks for a range that ends at the largest int there is,
-- and a scan that cannot get past that last column never comes back - the paint
-- never finishes and the client stops dead. A zoom far enough out that the span
-- it divides by stops being a finite number puts both bounds on one end of that
-- space instead, which draws nothing at all where 5.0.1 drew the whole map into
-- one pixel. So the first of these hangs rather than fails when it regresses,
-- and the run times out instead of reporting.

local kFarX = 2147483647
-- More occupied columns than any viewport here asks for, so the scan probes the
-- columns it wants rather than walking every column the area has: it is the
-- probing route that stops terminating. How many columns a viewport wants is
-- about half the map window's width in rooms, so the margin over that has to be
-- wide enough that no platform's window chrome can close it.
local kColumns = 2000
-- One pixel per room on the 600x400 map window below, which puts the viewport's
-- right-hand bound past the coordinate limit and so, once it is clamped, onto
-- it, over a range narrow enough for the scan to probe. The scroll wheel
-- reaches this zoom.
local kOnePixelPerRoomZoom = 400
-- Far enough out that the span overflows a float. That setMapZoom() accepts it
-- is the status quo rather than the requirement here - nothing validates an
-- upper bound - and what this pins is what the mapper does with it once it has.
local kSpanOverflowingZoom = 1e40
-- An ordinary zoom, and the one the parking area below is kept at. Both are
-- distinct from each other, from the zooms above, and from the 20 an untouched
-- area carries - so the mapper reporting either of them is the mapper showing
-- the one area that has it, which only a repaint can make true.
local kOrdinaryZoom = 21
local kParkZoom = 31

describe("Tests 2D map painting at the limits of the coordinate space", function()
  local areaId, parkAreaId
  local nearRoom, farRoom, parkRoom
  local originalRoom
  local roomIds = {}

  local function makeRoom(areaOfRoom, x)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaOfRoom)
    setRoomCoordinates(id, x, 0, 0)
    roomIds[#roomIds + 1] = id
    return id
  end

  setup(function()
    originalRoom = getPlayerRoom()
    -- Tolerant rather than asserting: a spec that ran before this one may have
    -- left the widget open, and failing here would leave the rooms below and
    -- the widget itself to every spec that runs after.
    closeMapWidget()
    assert.is_true(openMapWidget(0, 0, 600, 400))

    areaId = addAreaName("ViewportLimitsSpec")
    parkAreaId = addAreaName("ViewportLimitsSpecPark")

    for x = 0, kColumns - 1 do
      makeRoom(areaId, x)
    end
    nearRoom = roomIds[1]
    farRoom = makeRoom(areaId, kFarX)
    parkRoom = makeRoom(parkAreaId, 0)

    assert.is_true(setMapZoom(kParkZoom, parkAreaId))
    assert.is_true(setMapZoom(kOrdinaryZoom, areaId))
  end)

  teardown(function()
    -- Back to the room the profile was standing in before any of its rooms go,
    -- or every spec after this one starts with the player on a deleted room.
    if originalRoom and originalRoom > 0 then
      centerview(originalRoom)
      pumpEvents(100)
    end
    for _, id in ipairs(roomIds) do
      deleteRoom(id)
    end
    deleteArea("ViewportLimitsSpec")
    deleteArea("ViewportLimitsSpecPark")
    -- Mapper_spec.lua opens the map widget for itself and asserts that it did,
    -- so leave it as this file found it.
    closeMapWidget()
  end)

  -- The no-argument getMapZoom() reports the area the mapper is SHOWING, and
  -- only a repaint moves it onto the area a centring asked for - so waiting for
  -- it to name a zoom is waiting for a paint of that area to have happened. A
  -- scan that never returns takes the pump with it and nothing below runs again.
  local function waitForTheMapperToShow(zoom)
    local waitedMs = 0
    while getMapZoom() ~= zoom and waitedMs < 5000 do
      updateMap()
      pumpEvents(10)
      waitedMs = waitedMs + 10
    end
    return getMapZoom() == zoom
  end

  -- Parking on another area first is what makes the wait below mean something:
  -- from an area the mapper is already showing, the zoom it reports changes the
  -- moment the zoom is stored, paint or no paint. Storing the zoom before the
  -- centring is what makes the paint that is waited for the paint at that zoom.
  local function showAtZoom(roomId, zoom)
    assert.is_true(centerview(parkRoom))
    assert.is_true(waitForTheMapperToShow(kParkZoom),
      "the mapper never moved off the area under test, so the paint below would prove nothing")
    assert.is_true(setMapZoom(zoom, areaId))
    assert.is_true(centerview(roomId))
    assert.is_true(waitForTheMapperToShow(zoom),
      ("the mapper never painted the area under test at a zoom of %g"):format(zoom))
  end

  it("paints an area holding a room at the coordinate limit", function()
    showAtZoom(farRoom, kOrdinaryZoom)
    assert.are.equal(farRoom, getPlayerRoom())
  end)

  it("paints that room at a zoom of one pixel per room", function()
    showAtZoom(farRoom, kOnePixelPerRoomZoom)
    assert.are.equal(kOnePixelPerRoomZoom, getMapZoom(areaId))
  end)

  it("paints at a zoom far enough out to overflow the viewport span", function()
    finally(function()
      setMapZoom(kOrdinaryZoom, areaId)
      updateMap()
      pumpEvents(100)
    end)
    showAtZoom(nearRoom, kSpanOverflowingZoom)
    assert.are.equal(kSpanOverflowingZoom, getMapZoom(areaId))
  end)
end)
