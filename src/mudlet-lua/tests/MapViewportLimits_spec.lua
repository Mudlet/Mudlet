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

-- A label this many rooms across, painted at kCloseZoom's hundred pixels per
-- room on the 600x400 window, is terabytes as a single pixmap, which a build
-- under AddressSanitizer aborts on asking for. That makes the spec bite; the
-- player-facing failure is a smaller label needing gigabytes and running out of
-- memory.
local kHugeLabelRooms = 20000
local kCloseZoom = 4
local kLabelParkZoom = 33

describe("Tests painting a map label many times bigger than the map window", function()
  local areaId, parkAreaId
  local centreRoom, parkRoom
  local originalRoom

  local function makeRoom(areaOfRoom)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaOfRoom)
    setRoomCoordinates(id, 0, 0, 0)
    return id
  end

  setup(function()
    originalRoom = getPlayerRoom()
    closeMapWidget()
    assert.is_true(openMapWidget(0, 0, 600, 400))
    areaId = addAreaName("HugeLabelSpec")
    parkAreaId = addAreaName("HugeLabelSpecPark")
    centreRoom = makeRoom(areaId)
    parkRoom = makeRoom(parkAreaId)
    assert.is_true(setMapZoom(kLabelParkZoom, parkAreaId))
  end)

  teardown(function()
    if originalRoom and originalRoom > 0 then
      centerview(originalRoom)
      pumpEvents(100)
    end
    deleteRoom(centreRoom)
    deleteRoom(parkRoom)
    deleteArea("HugeLabelSpec")
    deleteArea("HugeLabelSpecPark")
    closeMapWidget()
  end)

  local function paintCloseUp()
    assert.is_true(centerview(parkRoom))
    assert.is_true(waitForTheMapperToShow(kLabelParkZoom),
      "the mapper never moved off the area under test, so the paint below would prove nothing")
    assert.is_true(setMapZoom(kCloseZoom, areaId))
    assert.is_true(centerview(centreRoom))
    assert.is_true(waitForTheMapperToShow(kCloseZoom),
      "the mapper never painted the area holding the label")
  end

  it("paints a scaled text label", function()
    -- The text's own size decides a text label's, so measure it to aim the zoom
    local probeId = createMapLabel(areaId, "Huge", 0, 0, 0, 255, 255, 255, 0, 0, 0, 1, 12, false, false)
    local measured = getMapLabel(areaId, probeId)
    deleteMapLabel(areaId, probeId)
    local labelZoom = measured.Width / kHugeLabelRooms
    local height = measured.Height / labelZoom
    local id = createMapLabel(areaId, "Huge", -kHugeLabelRooms / 2, height / 2, 0, 255, 255, 255, 0, 0, 0, labelZoom, 12, false, false)
    finally(function() deleteMapLabel(areaId, id) end)

    paintCloseUp()
    assert.is_not_nil(getMapLabels(areaId)[id])
  end)

  -- A label with a font of its own is redrawn from its text rather than stretched
  it("paints a scaled text label that names its font", function()
    local font = "Bitstream Vera Sans Mono"
    local probeId = createMapLabel(areaId, "Huge", 0, 0, 0, 255, 255, 255, 0, 0, 0, 1, 12, false, false, font)
    local measured = getMapLabel(areaId, probeId)
    deleteMapLabel(areaId, probeId)
    local labelZoom = measured.Width / kHugeLabelRooms
    local height = measured.Height / labelZoom
    local id = createMapLabel(areaId, "Huge", -kHugeLabelRooms / 2, height / 2, 0, 255, 255, 255, 0, 0, 0, labelZoom, 12, false, false, font)
    finally(function() deleteMapLabel(areaId, id) end)

    paintCloseUp()
    assert.is_not_nil(getMapLabels(areaId)[id])
  end)

  it("paints a scaled image label", function()
    local id = createMapImageLabel(areaId, getMudletHomeDir() .. "/nonexistent.png", -kHugeLabelRooms / 2, kHugeLabelRooms / 2, 0,
      kHugeLabelRooms, kHugeLabelRooms, 0.005, true)
    finally(function() deleteMapLabel(areaId, id) end)

    paintCloseUp()
    assert.is_not_nil(getMapLabels(areaId)[id])
  end)

  -- Lua cannot read the mapper's pixels back, but it can read an export's, so
  -- this is where a huge label drawn as nothing at all would show up. An opaque
  -- image covers the whole export, so each labelled export has to differ from
  -- the bare one; the two runs cover the layers under and over the rooms.
  describe("exports an area holding a huge scaled image label", function()
    local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
    assert(specDirectory, "MapViewportLimits_spec.lua has to be run from a file so that it can find its fixtures")
    local opaqueImage = specDirectory .. "/fixtures/images/solid-magenta-4x4.png"
    local barePath = getMudletHomeDir() .. "/HugeLabelSpecBare.png"
    local labelledPath = getMudletHomeDir() .. "/HugeLabelSpecLabelled.png"

    local function readBytes(path)
      local file = io.open(path, "rb")
      if not file then
        return nil
      end
      local bytes = file:read("*a")
      file:close()
      return bytes
    end

    -- The export is written on another thread after exportAreaImage() returns,
    -- and starting a second one before the first has reported back crashes
    -- Mudlet (#10393), so wait for the file to be complete and then a little
    -- longer for the export to report back.
    local function exportAndWait(path)
      os.remove(path)
      assert.is_true(exportAreaImage(areaId, path))
      local bytes = readBytes(path)
      local waitedMs = 0
      while not (bytes and bytes:sub(-8, -5) == "IEND") and waitedMs < 5000 do
        pumpEvents(20)
        waitedMs = waitedMs + 20
        bytes = readBytes(path)
      end
      pumpEvents(100)
      assert(bytes and bytes:sub(-8, -5) == "IEND", "the export was never written to " .. path)
      return bytes
    end

    local function exportWithHugeLabel(showOnTop)
      local id
      finally(function()
        if id then
          deleteMapLabel(areaId, id)
        end
        os.remove(barePath)
        os.remove(labelledPath)
      end)
      local bare = exportAndWait(barePath)
      id = createMapImageLabel(areaId, opaqueImage, -kHugeLabelRooms / 2, kHugeLabelRooms / 2, 0,
        kHugeLabelRooms, kHugeLabelRooms, 0.005, showOnTop)

      local labelled = exportAndWait(labelledPath)
      assert.is_true(bare ~= labelled,
        "the export came out the same with the label as without it, so the label was not drawn")
    end

    it("above the rooms", function()
      exportWithHugeLabel(true)
    end)

    it("below the rooms", function()
      exportWithHugeLabel(false)
    end)
  end)
end)
