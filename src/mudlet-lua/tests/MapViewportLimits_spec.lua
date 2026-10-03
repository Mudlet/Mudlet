-- These cases hang rather than fail when they regress: the run times out
-- instead of reporting.

local kFarX = 2147483647
-- More occupied columns than any viewport here asks for, so the scan probes the
-- columns it wants rather than walking every column the area has. The margin is
-- wide because how many it wants tracks the map window's width.
local kColumns = 2000
-- One pixel per room on the 600x400 map window below, which puts the viewport's
-- right-hand bound onto the coordinate limit once it is clamped.
local kOnePixelPerRoomZoom = 400
-- Far enough out that the span overflows a float.
local kSpanOverflowingZoom = 1e40
-- Distinct from each other, from the zooms above, and from the 20 an untouched
-- area carries, so the mapper reporting either is the mapper showing the one
-- area that has it.
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
    -- Tolerant rather than asserting: an earlier spec may have left the widget
    -- open, and failing here would leak the rooms below to every spec after.
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

  -- The no-argument getMapZoom() reports the area the mapper is SHOWING, and only
  -- a repaint moves it onto the area a centring asked for, so waiting for it to
  -- name a zoom is waiting for a paint of that area.
  local function waitForTheMapperToShow(zoom)
    local waitedMs = 0
    while getMapZoom() ~= zoom and waitedMs < 5000 do
      updateMap()
      pumpEvents(10)
      waitedMs = waitedMs + 10
    end
    return getMapZoom() == zoom
  end

  -- Parking on another area first is what makes the wait mean something: from an
  -- area the mapper already shows, the reported zoom changes the moment it is
  -- stored, paint or no paint. Storing the zoom before the centring is what makes
  -- the paint waited for the paint at that zoom.
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
