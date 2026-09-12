-- The 2D mapper works out which rooms could be on screen by inverting the
-- transform that puts them there, and the coordinates that come back are
-- clamped into an int because that is what a room coordinate is. Two ordinary
-- things push a bound onto the clamp: a map that holds a room out at the edge
-- of the coordinate space, and a zoom far enough out that the span it divides
-- by stops being a finite number. Either way the scan is asked for a range
-- ending at the largest int there is, and a scan that cannot get past that
-- last column never comes back - the paint never finishes and the client stops
-- dead. So each example here hangs rather than fails when it regresses, and
-- the run times out instead of reporting.

local kFarX = 2147483647
-- More occupied columns than any viewport here asks for, so the scan probes
-- the columns it wants rather than walking every column the area has: it is
-- the probing route that stops terminating.
local kColumns = 600
-- One pixel per room on the 600x400 map window below, which is where the
-- viewport's right-hand bound lands exactly on the coordinate limit. The
-- scroll wheel reaches it.
local kOnePixelPerRoomZoom = 400

describe("Tests 2D map painting at the limits of the coordinate space", function()
  local areaId
  local nearRoom, farRoom
  local roomIds = {}

  setup(function()
    assert.is_true(openMapWidget(0, 0, 600, 400))
    areaId = addAreaName("ViewportLimitsSpec")

    local function makeRoom(x)
      local id = createRoomID()
      addRoom(id)
      setRoomArea(id, areaId)
      setRoomCoordinates(id, x, 0, 0)
      roomIds[#roomIds + 1] = id
      return id
    end

    for x = 0, kColumns - 1 do
      makeRoom(x)
    end
    nearRoom = roomIds[1]
    farRoom = makeRoom(kFarX)
  end)

  teardown(function()
    for _, id in ipairs(roomIds) do
      deleteRoom(id)
    end
    deleteArea("ViewportLimitsSpec")
    -- Mapper_spec.lua opens the map widget for itself and asserts that it did,
    -- so leave it as this file found it.
    closeMapWidget()
  end)

  -- The repaint a zoom or a centring asks for is deferred onto the event loop,
  -- so it only happens once the loop gets a turn. A scan that never returns
  -- takes the pump with it and nothing below this line runs again.
  local function repaint()
    updateMap()
    pumpEvents(300)
  end

  it("paints an area holding a room at the coordinate limit", function()
    assert.is_true(centerview(farRoom))
    repaint()
    assert.are.equal(farRoom, getPlayerRoom())
  end)

  it("paints that room at a zoom of one pixel per room", function()
    assert.is_true(centerview(farRoom))
    repaint()
    assert.is_true(setMapZoom(kOnePixelPerRoomZoom, areaId))
    repaint()
    assert.are.equal(kOnePixelPerRoomZoom, getMapZoom(areaId))
  end)

  it("paints at a zoom far enough out to overflow the viewport span", function()
    finally(function()
      setMapZoom(20, areaId)
      repaint()
    end)
    assert.is_true(centerview(nearRoom))
    repaint()
    assert.is_true(setMapZoom(1e40, areaId))
    repaint()
    assert.are.equal(1e40, getMapZoom(areaId))
  end)
end)
