-- This block must stay first in the file: once a later spec calls
-- openMapWidget(), the widget persists for the rest of the session and the
-- pre-widget state becomes unreachable.
describe("Tests map events and menus before the map widget is opened", function()
  it("should return an empty table when nothing is registered yet", function()
    assert.are.same({}, getMapEvents())
    assert.are.same({}, getMapMenus())
  end)

  it("should register and remove a map event before the widget is opened", function()
    assert.is_true(addMapEvent("preWidgetEvent", "myEvent", "", "Pre-widget Event"))

    local events = getMapEvents()
    assert.is_not_nil(events.preWidgetEvent)
    assert.are.equal("myEvent", events.preWidgetEvent["event name"])
    assert.are.equal("Pre-widget Event", events.preWidgetEvent["display name"])

    assert.is_true(removeMapEvent("preWidgetEvent"))
    assert.is_nil(getMapEvents().preWidgetEvent)
  end)

  it("should register and remove a map menu before the widget is opened", function()
    assert.is_true(addMapMenu("PreWidgetMenu"))
    assert.are.equal("top-level", getMapMenus()["PreWidgetMenu"])

    assert.is_true(removeMapMenu("PreWidgetMenu"))
    assert.is_nil(getMapMenus()["PreWidgetMenu"])
  end)

  -- nothing can destroy the map widget again once it exists, so a second
  -- runTests in the same session inherits one and these two have nothing left
  -- to observe
  it("should report that there is no map widget to read a title from", function()
    local title, err = getMapWindowTitle()
    if title then
      pending("the map widget is already open in this session")
      return
    end
    assert.are.equal("no floating/dockable type map window found", err)
  end)

  it("should report that there is no map widget to read a geometry from", function()
    local x, err = getMapWidgetGeometry()
    if x then
      pending("the map widget is already open in this session")
      return
    end
    assert.are.equal("no floating/dockable type map window found", err)
  end)

  it("should retain a registration for when the widget opens later", function()
    assert.is_true(addMapEvent("preWidgetKeptEvent", "myEvent", "", "Kept Event"))
  end)
end)

describe("Tests custom map event and menu functions", function()

  setup(function()
    openMapWidget()
  end)

  after_each(function()
    removeMapEvent("testEvent1")
    removeMapEvent("testEvent2")
    removeMapMenu("TestMenu")
    removeMapMenu("TestSubMenu")
  end)

  describe("Tests that pre-widget registrations survive opening the widget", function()
    it("should still list an event registered before the widget was opened", function()
      local events = getMapEvents()
      assert.is_not_nil(events.preWidgetKeptEvent)
      assert.are.equal("myEvent", events.preWidgetKeptEvent["event name"])
      assert.are.equal("Kept Event", events.preWidgetKeptEvent["display name"])
      removeMapEvent("preWidgetKeptEvent")
    end)
  end)

  describe("Tests addMapEvent and getMapEvents", function()
    it("should add a top-level map event", function()
      addMapEvent("testEvent1", "myEvent", "", "Test Event 1")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("myEvent", events.testEvent1["event name"])
      assert.are.equal("", events.testEvent1["parent"])
      assert.are.equal("Test Event 1", events.testEvent1["display name"])
    end)

    it("should add a map event under a menu", function()
      addMapMenu("TestMenu")
      addMapEvent("testEvent1", "myEvent", "TestMenu", "Test Event 1")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("TestMenu", events.testEvent1["parent"])
    end)

    it("should use unique name as display text when not provided", function()
      addMapEvent("testEvent1", "myEvent")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("testEvent1", events.testEvent1["display name"])
    end)
  end)

  describe("Tests addMapMenu and getMapMenus", function()
    it("should add a top-level menu", function()
      addMapMenu("TestMenu")

      local menus = getMapMenus()
      assert.are.equal("top-level", menus["TestMenu"])
    end)

    it("should add a nested submenu", function()
      addMapMenu("TestMenu")
      addMapMenu("TestSubMenu", "TestMenu")

      local menus = getMapMenus()
      assert.are.equal("top-level", menus["TestMenu"])
      assert.are.equal("TestMenu", menus["TestSubMenu"])
    end)

    it("should key menus by display name by default", function()
      addMapMenu("TestMenu", nil, "Test Display Name")

      local menus = getMapMenus()
      assert.are.equal("top-level", menus["Test Display Name"])
      assert.is_nil(menus["TestMenu"])
    end)

    it("should treat a nil argument like no argument", function()
      addMapMenu("TestMenu")

      local menus = getMapMenus(nil)
      assert.are.equal("top-level", menus["TestMenu"])
    end)

    it("should key menus by unique name when requested", function()
      addMapMenu("TestMenu", nil, "Test Display Name")
      addMapMenu("TestSubMenu", "TestMenu", "Sub Display Name")

      local menus = getMapMenus(true)
      assert.is_table(menus["TestMenu"])
      assert.are.equal("Test Display Name", menus["TestMenu"]["display name"])
      assert.are.equal("top-level", menus["TestMenu"]["parent"])
      assert.is_table(menus["TestSubMenu"])
      assert.are.equal("Sub Display Name", menus["TestSubMenu"]["display name"])
      assert.are.equal("TestMenu", menus["TestSubMenu"]["parent"])
    end)

    it("should let getMapEvents parents be resolved via getMapMenus(true)", function()
      addMapMenu("TestMenu", nil, "Test Display Name")
      addMapEvent("testEvent1", "myEvent", "TestMenu", "Test Event 1")

      local events = getMapEvents()
      local menus = getMapMenus(true)
      assert.is_not_nil(menus[events.testEvent1.parent])
      assert.are.equal("Test Display Name", menus[events.testEvent1.parent]["display name"])
    end)
  end)

  describe("Tests removeMapEvent", function()
    it("should remove an event", function()
      addMapEvent("testEvent1", "myEvent", "", "Test Event 1")
      removeMapEvent("testEvent1")

      local events = getMapEvents()
      assert.is_nil(events.testEvent1)
    end)
  end)

  describe("Tests removeMapMenu", function()
    it("should remove a menu and its children", function()
      addMapMenu("TestMenu")
      addMapMenu("TestSubMenu", "TestMenu")
      removeMapMenu("TestMenu")

      local menus = getMapMenus()
      assert.is_nil(menus["TestMenu"])
      assert.is_nil(menus["TestSubMenu"])
    end)
  end)

end)

describe("Tests per-room border functions", function()

  local testRoomId

  setup(function()
    -- Create a test area and room
    local areaId = addAreaName("TestBorderArea")
    testRoomId = createRoomID()
    addRoom(testRoomId)
    setRoomArea(testRoomId, areaId)
    setRoomCoordinates(testRoomId, 0, 0, 0)
  end)

  teardown(function()
    -- Clean up test room and area
    deleteRoom(testRoomId)
    deleteArea("TestBorderArea")
  end)

  describe("Tests setRoomBorderColor", function()
    it("should set border color with RGB", function()
      local result = setRoomBorderColor(testRoomId, 255, 0, 0)
      assert.is_true(result)

      local r, g, b, a = getRoomBorderColor(testRoomId)
      assert.are.equal(255, r)
      assert.are.equal(0, g)
      assert.are.equal(0, b)
      assert.are.equal(255, a) -- default alpha
    end)

    it("should set border color with RGBA", function()
      local result = setRoomBorderColor(testRoomId, 0, 255, 0, 128)
      assert.is_true(result)

      local r, g, b, a = getRoomBorderColor(testRoomId)
      assert.are.equal(0, r)
      assert.are.equal(255, g)
      assert.are.equal(0, b)
      assert.are.equal(128, a)
    end)

    it("should return nil for invalid color values", function()
      local result, err = setRoomBorderColor(testRoomId, 256, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)

      result, err = setRoomBorderColor(testRoomId, -1, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for invalid room ID", function()
      local result, err = setRoomBorderColor(-999, 255, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

  describe("Tests getRoomBorderColor", function()
    it("should return nil when no custom color is set", function()
      clearRoomBorderColor(testRoomId)
      local result = getRoomBorderColor(testRoomId)
      assert.is_nil(result)
    end)
  end)

  describe("Tests clearRoomBorderColor", function()
    it("should clear the border color", function()
      setRoomBorderColor(testRoomId, 255, 0, 0)
      local result = clearRoomBorderColor(testRoomId)
      assert.is_true(result)
      assert.is_nil(getRoomBorderColor(testRoomId))
    end)
  end)

  describe("Tests setRoomBorderThickness", function()
    it("should set valid thickness", function()
      local result = setRoomBorderThickness(testRoomId, 5)
      assert.is_true(result)
      assert.are.equal(5, getRoomBorderThickness(testRoomId))
    end)

    it("should return nil for thickness below 1", function()
      local result, err = setRoomBorderThickness(testRoomId, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for thickness above 10", function()
      local result, err = setRoomBorderThickness(testRoomId, 11)
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

  describe("Tests getRoomBorderThickness", function()
    it("should return nil when using global default", function()
      clearRoomBorderThickness(testRoomId)
      local result = getRoomBorderThickness(testRoomId)
      assert.is_nil(result)
    end)
  end)

  describe("Tests clearRoomBorderThickness", function()
    it("should clear the thickness", function()
      setRoomBorderThickness(testRoomId, 3)
      local result = clearRoomBorderThickness(testRoomId)
      assert.is_true(result)
      assert.is_nil(getRoomBorderThickness(testRoomId))
    end)
  end)

end)

describe("Tests addRoom", function()

  it("should return true when the room is created", function()
    local roomID = createRoomID()
    assert.is_true(addRoom(roomID))
    deleteRoom(roomID)
  end)

  it("should report the requested areaID when it does not exist", function()
    local roomID = createRoomID()
    local ok, err = addRoom(roomID, 987654)
    assert.is_nil(ok)
    assert.is_string(err)
    assert.is_not_nil(err:find("areaID 987654", 1, true), err)
    -- the room is still created, parked in the default area (-1)
    assert.are.equal(-1, getRoomArea(roomID))
    deleteRoom(roomID)
  end)

end)

describe("Tests map info functions", function()

  describe("Tests getMapInfo", function()
    it("should return a table with contributor states", function()
      local info = getMapInfo()
      assert.is_table(info)
      -- "Short" is a built-in contributor and should exist
      assert.is_not_nil(info["Short"])
    end)

    it("should reflect enabled/disabled state", function()
      enableMapInfo("Short")
      local info = getMapInfo()
      assert.is_true(info["Short"])

      disableMapInfo("Short")
      info = getMapInfo()
      assert.is_false(info["Short"])

      -- Re-enable for clean state
      enableMapInfo("Short")
    end)
  end)

  describe("Tests enableMapInfo and disableMapInfo", function()
    it("should return nil for non-existent contributor", function()
      local result, err = enableMapInfo("NonExistentContributor")
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for non-existent contributor on disable", function()
      local result, err = disableMapInfo("NonExistentContributor")
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

end)

describe("Tests searchRoom", function()

  local testRoomId
  local missingRoomId = 999999999

  setup(function()
    local areaId = addAreaName("TestSearchRoomArea")
    testRoomId = createRoomID()
    addRoom(testRoomId)
    setRoomArea(testRoomId, areaId)
    setRoomName(testRoomId, "SearchRoomSpecRoom")
  end)

  teardown(function()
    deleteRoom(testRoomId)
    deleteArea("TestSearchRoomArea")
  end)

  it("should return the room name for an existing room ID", function()
    local result = searchRoom(testRoomId)
    assert.are.equal("SearchRoomSpecRoom", result)
  end)

  it("should return nil and a message for a non-existent room ID", function()
    assert.is_false(roomExists(missingRoomId))
    local result, err = searchRoom(missingRoomId)
    assert.is_nil(result)
    assert.is_string(err)
    assert.is_truthy(err:find("not a valid roomID", 1, true))
  end)

  it("should return a table of matches for a name search", function()
    local result = searchRoom("SearchRoomSpecRoom")
    assert.is_table(result)
    assert.are.equal("SearchRoomSpecRoom", result[testRoomId])
  end)

  it("should return an empty table for a name search with no matches", function()
    local result = searchRoom("NoSuchRoomNameAnywhere")
    assert.is_table(result)
    assert.is_nil(next(result))
  end)

  it("should treat a numeric string as a room ID and return nil and a message when it does not exist", function()
    local result, err = searchRoom(tostring(missingRoomId))
    assert.is_nil(result)
    assert.is_string(err)
    assert.is_truthy(err:find("not a valid roomID", 1, true))
  end)

end)

-- A shared in-memory fixture: three areas and ten rooms wired into a
-- pathfinding diamond, a cross-area link, a special exit and a pair of sandbox
-- rooms used for the mutation-heavy tests. Everything is torn down at the end.
describe("Tests mapper functions against a shared fixture", function()

  local missingRoomId = 990000001
  local missingAreaId = 990000002

  local areaAlpha, areaBeta, areaGamma
  local rA1, rA2, rA3, rA4, rA5
  local rB1, rB2, rG1
  local rSandA, rSandB

  setup(function()
    -- The mapper widget is required for zoom, views, player room, export and
    -- map info repaint paths; it persists once opened. Asserted so a headless
    -- failure surfaces here rather than as dozens of downstream failures.
    assert.is_true(openMapWidget())

    areaAlpha = addAreaName("MapperSpecAlpha")
    areaBeta = addAreaName("MapperSpecBeta")
    areaGamma = addAreaName("MapperSpecGamma")

    local function makeRoom(area, x, y, z)
      local id = createRoomID()
      addRoom(id)
      setRoomArea(id, area)
      setRoomCoordinates(id, x, y, z)
      return id
    end

    rA1 = makeRoom(areaAlpha, 0, 0, 0)
    rA2 = makeRoom(areaAlpha, 1, 0, 0)
    rA3 = makeRoom(areaAlpha, 2, 0, 0)
    rA4 = makeRoom(areaAlpha, 1, -1, 0)
    rA5 = makeRoom(areaAlpha, 2, -1, 0)
    rSandA = makeRoom(areaAlpha, 0, -3, 0)
    rSandB = makeRoom(areaAlpha, 1, -3, 0)
    rB1 = makeRoom(areaBeta, 0, 0, 1)
    rB2 = makeRoom(areaBeta, 1, 0, 1)
    rG1 = makeRoom(areaGamma, 0, 0, 2)

    -- Pathfinding diamond: a 2-hop east route and a 3-hop south route between
    -- rA1 and rA3, both bidirectional.
    setExit(rA1, rA2, "east"); setExit(rA2, rA1, "west")
    setExit(rA2, rA3, "east"); setExit(rA3, rA2, "west")
    setExit(rA1, rA4, "south"); setExit(rA4, rA1, "north")
    setExit(rA4, rA5, "east"); setExit(rA5, rA4, "west")
    setExit(rA5, rA3, "north"); setExit(rA3, rA5, "south")
    -- Cross-area link into Beta.
    setExit(rA3, rB1, "up"); setExit(rB1, rA3, "down")
    setExit(rB1, rB2, "east"); setExit(rB2, rB1, "west")
    -- Gamma is only reachable through a special exit from rB2.
    addSpecialExit(rB2, rG1, "enter gate")

    -- Sandbox rooms carry the mutation-heavy exits so the diamond stays clean.
    setExit(rSandA, rSandB, "east"); setExit(rSandB, rSandA, "west")
    addSpecialExit(rSandA, rSandB, "wibble")
    setExitStub(rSandB, "north", true)
  end)

  teardown(function()
    closeAllMapViews()
    os.remove(getMudletHomeDir() .. "/mapper_spec_export.png")
    for _, id in ipairs({rA1, rA2, rA3, rA4, rA5, rSandA, rSandB, rB1, rB2, rG1}) do
      deleteRoom(id)
    end
    deleteArea("MapperSpecAlpha")
    deleteArea("MapperSpecBeta")
    deleteArea("MapperSpecGamma")
  end)

  -- saveJsonMap/loadJsonMap are intentionally not covered here: the JSON export
  -- runs through a progress dialog and bulk import, and loadJsonMap replaces and
  -- re-initialises the entire map, which is incompatible with this shared
  -- fixture. JSON persistence is exercised by the C++ MapRoundTripTest instead.

  describe("Tests area listing and naming", function()
    it("getAreaTable maps every area name to its ID", function()
      local areas = getAreaTable()
      assert.is_table(areas)
      assert.are.equal(areaAlpha, areas["MapperSpecAlpha"])
      assert.are.equal(areaBeta, areas["MapperSpecBeta"])
      assert.are.equal(areaGamma, areas["MapperSpecGamma"])
    end)

    it("getAreaTableSwap maps every area ID to its name", function()
      local areas = getAreaTableSwap()
      assert.is_table(areas)
      assert.are.equal("MapperSpecAlpha", areas[areaAlpha])
      assert.are.equal("MapperSpecGamma", areas[areaGamma])
    end)

    it("getRoomAreaName resolves an area ID to its name", function()
      assert.are.equal("MapperSpecAlpha", getRoomAreaName(areaAlpha))
    end)

    it("getRoomAreaName resolves an area name to its ID", function()
      assert.are.equal(areaBeta, getRoomAreaName("MapperSpecBeta"))
    end)

    it("getRoomAreaName returns -1 and a message for an unknown area ID", function()
      local id, err = getRoomAreaName(missingAreaId)
      assert.are.equal(-1, id)
      assert.is_string(err)
    end)

    it("getRoomAreaName hard-errors on a non-number, non-string argument", function()
      assert.has_error(function() getRoomAreaName(true) end)
    end)

    it("setAreaName renames an area and getAreaTable reflects it", function()
      assert.is_true(setAreaName(areaGamma, "MapperSpecGammaRenamed"))
      assert.are.equal("MapperSpecGammaRenamed", getRoomAreaName(areaGamma))
      -- restore so later assertions and teardown keep working
      assert.is_true(setAreaName(areaGamma, "MapperSpecGamma"))
    end)

    it("setAreaName rejects an empty new name with nil and a message", function()
      local ok, err = setAreaName(areaAlpha, "")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("setAreaName rejects duplicating an existing area name", function()
      local ok, err = setAreaName(areaAlpha, "MapperSpecBeta")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addAreaName rejects an empty (whitespace-only) name with nil and a message", function()
      local ok, err = addAreaName("   ")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addAreaName rejects a duplicate name with nil and a message", function()
      local ok, err = addAreaName("MapperSpecAlpha")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests deleteArea", function()
    it("removes a throwaway area from getAreaTable", function()
      addAreaName("MapperSpecDeleteMe")
      assert.is_not_nil(getAreaTable()["MapperSpecDeleteMe"])
      assert.is_true(deleteArea("MapperSpecDeleteMe"))
      assert.is_nil(getAreaTable()["MapperSpecDeleteMe"])
    end)

    it("returns nil and a message for an unknown areaID", function()
      local ok, err = deleteArea(missingAreaId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("returns nil and a message for an empty area name", function()
      local ok, err = deleteArea("")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("refuses to delete the default area", function()
      local ok, err = deleteArea(getRoomAreaName(-1))
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests area room membership", function()
    it("getAreaRooms1 lists the rooms of an area 1-based", function()
      local rooms = getAreaRooms1(areaBeta)
      assert.is_table(rooms)
      assert.is_not_nil(rooms[1])
      assert.is_not_nil(rooms[2])
      assert.is_nil(rooms[3])
      local set = {}
      for _, id in pairs(rooms) do set[id] = true end
      assert.is_true(set[rB1])
      assert.is_true(set[rB2])
    end)

    it("getAreaRooms lists the rooms of an area 0-based for compatibility", function()
      local rooms = getAreaRooms(areaBeta)
      assert.is_table(rooms)
      assert.is_not_nil(rooms[0])
      assert.is_nil(rooms[2])
    end)

    it("getAreaRooms returns nil for an unknown area", function()
      assert.is_nil(getAreaRooms(missingAreaId))
    end)

    it("getRoomsByPosition1 finds the room at a coordinate 1-based", function()
      local rooms = getRoomsByPosition1(areaAlpha, 0, 0, 0)
      assert.is_table(rooms)
      assert.are.equal(rA1, rooms[1])
    end)

    it("getRoomsByPosition finds the room at a coordinate 0-based", function()
      local rooms = getRoomsByPosition(areaAlpha, 1, 0, 0)
      assert.is_table(rooms)
      assert.are.equal(rA2, rooms[0])
    end)

    it("getRoomsByPosition returns nil for an unknown area", function()
      assert.is_nil(getRoomsByPosition(missingAreaId, 0, 0, 0))
    end)

    it("getAreaExits lists the rooms with exits leaving the area", function()
      local exits = getAreaExits(areaBeta)
      assert.is_table(exits)
      local set = {}
      for _, id in pairs(exits) do set[id] = true end
      -- rB1 exits Beta via "down" to rA3, rB2 exits via the special exit to rG1
      assert.is_true(set[rB1])
      assert.is_true(set[rB2])
    end)

    it("getAreaExits with full data keys by source room and command", function()
      local exits = getAreaExits(areaBeta, true)
      assert.is_table(exits)
      assert.is_table(exits[rB1])
      -- rB1 leaves Beta down to rA3; the inner table maps a command to that room
      local leavesToRA3 = false
      for _, toRoom in pairs(exits[rB1]) do
        if toRoom == rA3 then leavesToRA3 = true end
      end
      assert.is_true(leavesToRA3)
    end)

    it("getAreaExits returns nil and a message for an unknown area", function()
      local exits, err = getAreaExits(missingAreaId)
      assert.is_nil(exits)
      assert.is_string(err)
    end)
  end)

  describe("Tests grid mode", function()
    it("getGridMode reports false for a normal area", function()
      assert.is_false(getGridMode(areaGamma))
    end)

    it("setGridMode toggles the flag which getGridMode reads back", function()
      assert.is_true(setGridMode(areaGamma, true))
      assert.is_true(getGridMode(areaGamma))
      assert.is_true(setGridMode(areaGamma, false))
      assert.is_false(getGridMode(areaGamma))
    end)

    it("setGridMode returns false for an unknown area", function()
      assert.is_false(setGridMode(missingAreaId, true))
    end)

    it("getGridMode returns nil and a message for an unknown area", function()
      local ok, err = getGridMode(missingAreaId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room area assignment", function()
    it("getRoomArea returns the area a room belongs to", function()
      assert.are.equal(areaBeta, getRoomArea(rB1))
    end)

    it("getRoomArea returns nil for an unknown room", function()
      assert.is_nil(getRoomArea(missingRoomId))
    end)

    it("resetRoomArea parks a room in the default area (-1)", function()
      local id = createRoomID()
      addRoom(id)
      setRoomArea(id, areaAlpha)
      assert.are.equal(areaAlpha, getRoomArea(id))
      assert.is_true(resetRoomArea(id))
      assert.are.equal(-1, getRoomArea(id))
      deleteRoom(id)
    end)

    it("resetRoomArea returns nil and a message for an unknown room", function()
      local ok, err = resetRoomArea(missingRoomId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests setRoomArea forms", function()
    it("moves a table of rooms into an area in one call", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      assert.is_true(setRoomArea({a, b}, areaBeta))
      assert.are.equal(areaBeta, getRoomArea(a))
      assert.are.equal(areaBeta, getRoomArea(b))
      deleteRoom(a); deleteRoom(b)
    end)

    it("accepts an area name as well as an ID", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      assert.is_true(setRoomArea(a, "MapperSpecBeta"))
      assert.are.equal(areaBeta, getRoomArea(a))
      deleteRoom(a)
    end)

    it("returns nil and a message for an unknown areaID", function()
      local ok, err = setRoomArea(rSandA, missingAreaId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("rejects an empty area name with nil and a message", function()
      local ok, err = setRoomArea(rSandA, "")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room existence and names", function()
    it("roomExists is true for a fixture room and false for a missing one", function()
      assert.is_true(roomExists(rA1))
      assert.is_false(roomExists(missingRoomId))
    end)

    it("getRooms maps every room ID to its name", function()
      local rooms = getRooms()
      assert.is_table(rooms)
      assert.is_not_nil(rooms[rA1])
      assert.is_not_nil(rooms[rG1])
    end)

    it("setRoomName is read back by getRoomName", function()
      assert.is_true(setRoomName(rSandA, "SandboxRoomName"))
      assert.are.equal("SandboxRoomName", getRoomName(rSandA))
    end)

    it("getRoomName returns nil and a message for an unknown room", function()
      local name, err = getRoomName(missingRoomId)
      assert.is_nil(name)
      assert.is_string(err)
    end)
  end)

  describe("Tests room coordinates", function()
    it("getRoomCoordinates returns the stored x, y and z", function()
      local x, y, z = getRoomCoordinates(rA3)
      assert.are.equal(2, x)
      assert.are.equal(0, y)
      assert.are.equal(0, z)
    end)

    it("getRoomCoordinates returns three nils for an unknown room", function()
      local x, y, z = getRoomCoordinates(missingRoomId)
      assert.is_nil(x)
      assert.is_nil(y)
      assert.is_nil(z)
    end)
  end)

  describe("Tests room environment", function()
    it("setRoomEnv is read back by getRoomEnv", function()
      assert.is_true(setRoomEnv(rSandA, 42))
      assert.are.equal(42, getRoomEnv(rSandA))
    end)

    it("setRoomEnv returns nil and a message for an unknown room", function()
      local ok, err = setRoomEnv(missingRoomId, 1)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room weight", function()
    it("rooms default to a weight of 1", function()
      assert.are.equal(1, getRoomWeight(rB2))
    end)

    it("setRoomWeight is read back by getRoomWeight", function()
      assert.is_true(setRoomWeight(rSandB, 5))
      assert.are.equal(5, getRoomWeight(rSandB))
      setRoomWeight(rSandB, 1)
    end)

    it("setRoomWeight returns nil and a message for an unknown room", function()
      local ok, err = setRoomWeight(missingRoomId, 3)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room symbol character", function()
    it("setRoomChar is read back by getRoomChar", function()
      assert.is_true(setRoomChar(rSandA, "@"))
      assert.are.equal("@", getRoomChar(rSandA))
    end)

    it("an empty string clears the room symbol", function()
      setRoomChar(rSandA, "#")
      assert.is_true(setRoomChar(rSandA, ""))
      assert.are.equal("", getRoomChar(rSandA))
    end)

    it("getRoomChar returns nil and a message for an unknown room", function()
      local ch, err = getRoomChar(missingRoomId)
      assert.is_nil(ch)
      assert.is_string(err)
    end)
  end)

  describe("Tests room symbol colour", function()
    it("setRoomCharColor is read back by getRoomCharColor", function()
      assert.is_true(setRoomCharColor(rSandA, 10, 20, 30))
      local r, g, b = getRoomCharColor(rSandA)
      assert.are.equal(10, r)
      assert.are.equal(20, g)
      assert.are.equal(30, b)
    end)

    it("setRoomCharColor hard-errors on an out-of-range component", function()
      assert.has_error(function() setRoomCharColor(rSandA, 256, 0, 0) end)
    end)

    it("unsetRoomCharColor returns true and clears the stored colour", function()
      setRoomCharColor(rSandA, 100, 100, 100)
      -- The symbol colour is reset to an invalid QColor whose RGB read-back is
      -- undefined, so only the success contract is pinned here.
      assert.is_true(unsetRoomCharColor(rSandA))
    end)

    it("unsetRoomCharColor returns nil and a message for an unknown room", function()
      local ok, err = unsetRoomCharColor(missingRoomId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room hidden state", function()
    it("setRoomHidden is read back by getRoomHidden", function()
      assert.is_true(setRoomHidden(rSandB, true))
      assert.is_true(getRoomHidden(rSandB))
      assert.is_true(setRoomHidden(rSandB, false))
      assert.is_false(getRoomHidden(rSandB))
    end)

    it("getHiddenRooms lists only the hidden rooms", function()
      setRoomHidden(rSandB, true)
      local hidden = getHiddenRooms()
      assert.is_table(hidden)
      local set = {}
      for _, id in pairs(hidden) do set[id] = true end
      assert.is_true(set[rSandB])
      assert.is_nil(set[rA1])
      setRoomHidden(rSandB, false)
    end)

    it("setRoomHidden returns nil and a message for an unknown room", function()
      local ok, err = setRoomHidden(missingRoomId, true)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests room locking", function()
    it("lockRoom toggles a flag that roomLocked reads back", function()
      assert.is_true(lockRoom(rSandB, true))
      assert.is_true(roomLocked(rSandB))
      assert.is_true(lockRoom(rSandB, false))
      assert.is_false(roomLocked(rSandB))
    end)

    it("lockRoom returns false for an unknown room", function()
      assert.is_false(lockRoom(missingRoomId, true))
    end)

    it("roomLocked returns false for an unknown room", function()
      assert.is_false(roomLocked(missingRoomId))
    end)
  end)

  describe("Tests room hashes", function()
    it("setRoomIDbyHash is read back by both hash getters", function()
      setRoomIDbyHash(rSandA, "sandbox-hash")
      assert.are.equal(rSandA, getRoomIDbyHash("sandbox-hash"))
      assert.are.equal("sandbox-hash", getRoomHashByID(rSandA))
    end)

    it("getRoomIDbyHash returns -1 for an unknown hash", function()
      assert.are.equal(-1, getRoomIDbyHash("no-such-hash-anywhere"))
    end)

    it("getRoomHashByID returns nil and a message for a room without a hash", function()
      local hash, err = getRoomHashByID(rB1)
      assert.is_nil(hash)
      assert.is_string(err)
    end)
  end)

  describe("Tests room highlighting", function()
    it("highlightRoom returns true for a valid room", function()
      assert.is_true(highlightRoom(rSandA, 255, 0, 0, 0, 255, 0, 10, 100, 100))
    end)

    it("highlightRoom returns false for an unknown room", function()
      assert.is_false(highlightRoom(missingRoomId, 255, 0, 0, 0, 255, 0, 10, 100, 100))
    end)

    it("unHighlightRoom returns true for a valid room and false for a missing one", function()
      highlightRoom(rSandA, 255, 0, 0, 0, 255, 0, 10, 100, 100)
      assert.is_true(unHighlightRoom(rSandA))
      assert.is_false(unHighlightRoom(missingRoomId))
    end)
  end)

  describe("Tests normal exits", function()
    it("getRoomExits reports every stored exit direction", function()
      local exits = getRoomExits(rA1)
      assert.is_table(exits)
      assert.are.equal(rA2, exits["east"])
      assert.are.equal(rA4, exits["south"])
    end)

    it("getRoomExits returns nothing for an unknown room", function()
      assert.is_nil(getRoomExits(missingRoomId))
    end)

    it("setExit adds a new exit that getRoomExits reflects", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      assert.is_true(setExit(a, b, "north"))
      assert.are.equal(b, getRoomExits(a)["north"])
      deleteRoom(a); deleteRoom(b)
    end)

    it("setExit hard-errors on an unparseable direction", function()
      assert.has_error(function() setExit(rA1, rA2, "sideways") end)
    end)

    it("getAllRoomEntrances lists the rooms that exit into a room", function()
      local entrances = getAllRoomEntrances(rA2)
      assert.is_table(entrances)
      local set = {}
      for _, id in pairs(entrances) do set[id] = true end
      -- rA1 (east) and rA3 (west) both lead into rA2
      assert.is_true(set[rA1])
      assert.is_true(set[rA3])
    end)

    it("getAllRoomEntrances returns nil and a message for an unknown room", function()
      local entrances, err = getAllRoomEntrances(missingRoomId)
      assert.is_nil(entrances)
      assert.is_string(err)
    end)
  end)

  describe("Tests exit stubs", function()
    it("getExitStubs1 lists stub direction codes 1-based", function()
      local stubs = getExitStubs1(rSandB)
      assert.is_table(stubs)
      assert.are.equal(1, stubs[1]) -- DIR_NORTH
    end)

    it("getExitStubs lists stub direction codes 0-based for compatibility", function()
      local stubs = getExitStubs(rSandB)
      assert.is_table(stubs)
      assert.are.equal(1, stubs[0])
    end)

    it("getExitStubsNames maps stub codes to direction names", function()
      local names = getExitStubsNames(rSandB)
      assert.is_table(names)
      assert.are.equal("north", names[1])
    end)

    it("getExitStubs1 returns nil and a message for an unknown room", function()
      local stubs, err = getExitStubs1(missingRoomId)
      assert.is_nil(stubs)
      assert.is_string(err)
    end)

    it("setExitStub hard-errors when the room does not exist", function()
      assert.has_error(function() setExitStub(missingRoomId, "north", true) end)
    end)

    it("setExitStub hard-errors on an unparseable direction", function()
      assert.has_error(function() setExitStub(rSandA, "sideways", true) end)
    end)

    it("connectExitStub turns a stub into a real exit", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      -- Both rooms need a matching stub: the source going up, the target the
      -- reverse (down).
      setExitStub(a, "up", true)
      setExitStub(b, "down", true)
      assert.is_true(connectExitStub(a, b, "up"))
      assert.are.equal(b, getRoomExits(a)["up"])
      deleteRoom(a); deleteRoom(b)
    end)

    it("connectExitStub hard-errors when the second argument is missing", function()
      assert.has_error(function() connectExitStub(rSandA) end)
    end)

    it("connectExitStub with only a target ID reports when there is no matching stub", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      local ok, err = connectExitStub(a, b)
      assert.is_nil(ok)
      assert.is_string(err)
      deleteRoom(a); deleteRoom(b)
    end)

    it("connectExitStub returns nil and a message for an unparseable direction", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      local ok, err = connectExitStub(a, b, "sideways")
      assert.is_nil(ok)
      assert.is_string(err)
      deleteRoom(a); deleteRoom(b)
    end)
  end)

  describe("Tests special exits", function()
    it("getSpecialExits reports the special exit and its lock state", function()
      local exits = getSpecialExits(rB2)
      assert.is_table(exits)
      assert.is_table(exits[rG1])
      assert.are.equal("0", exits[rG1]["enter gate"])
    end)

    it("getSpecialExitsSwap keys special exits by command", function()
      local exits = getSpecialExitsSwap(rB2)
      assert.is_table(exits)
      assert.are.equal(rG1, exits["enter gate"])
    end)

    it("getSpecialExits returns nil and a message for an unknown room", function()
      local exits, err = getSpecialExits(missingRoomId)
      assert.is_nil(exits)
      assert.is_string(err)
    end)

    it("addSpecialExit is read back and removeSpecialExit clears it", function()
      assert.is_true(addSpecialExit(rSandB, rSandA, "crawl"))
      assert.are.equal(rSandA, getSpecialExitsSwap(rSandB)["crawl"])
      assert.is_true(removeSpecialExit(rSandB, "crawl"))
      assert.is_nil(getSpecialExitsSwap(rSandB)["crawl"])
    end)

    it("addSpecialExit rejects an empty command with nil and a message", function()
      local ok, err = addSpecialExit(rSandA, rSandB, "")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addSpecialExit returns nil and a message for an unknown source room", function()
      local ok, err = addSpecialExit(missingRoomId, rSandB, "go")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addSpecialExit returns nil and a message for an unknown entrance room", function()
      local ok, err = addSpecialExit(rSandA, missingRoomId, "go")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("getSpecialExits picks the best unlocked exit, or lists all with showAllExits", function()
      local x = createRoomID(); addRoom(x); setRoomArea(x, areaAlpha)
      local y = createRoomID(); addRoom(y); setRoomArea(y, areaAlpha)
      addSpecialExit(x, y, "path1")
      addSpecialExit(x, y, "path2")
      lockSpecialExit(x, 0, "path1", true)

      -- Default: only the best (unlocked) command to y is returned.
      local best = getSpecialExits(x)[y]
      assert.is_table(best)
      assert.is_nil(best["path1"])
      assert.are.equal("0", best["path2"])

      -- showAllExits=true: every command is returned with its lock state.
      local all = getSpecialExits(x, true)[y]
      assert.are.equal("1", all["path1"])
      assert.are.equal("0", all["path2"])

      deleteRoom(x); deleteRoom(y)
    end)

    it("removeSpecialExit returns nil and a message for a non-existent command", function()
      local ok, err = removeSpecialExit(rB2, "no such command")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("clearSpecialExits removes every special exit of a room", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      addSpecialExit(a, rSandB, "one")
      addSpecialExit(a, rSandA, "two")
      clearSpecialExits(a)
      assert.is_nil(next(getSpecialExitsSwap(a)))
      deleteRoom(a)
    end)
  end)

  describe("Tests exit weights", function()
    it("setExitWeight is read back by getExitWeights", function()
      assert.is_true(setExitWeight(rSandA, "east", 7))
      assert.are.equal(7, getExitWeights(rSandA)["e"])
      setExitWeight(rSandA, "east", 0)
    end)

    it("setExitWeight rejects a negative weight with nil and a message", function()
      local ok, err = setExitWeight(rSandA, "east", -1)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("setExitWeight returns nil and a message for a direction with no exit", function()
      local ok, err = setExitWeight(rSandA, "down", 3)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("getExitWeights returns an empty table for a room with no weights", function()
      assert.is_nil(next(getExitWeights(rB1)))
    end)
  end)

  describe("Tests exit locks", function()
    it("lockExit is read back by hasExitLock", function()
      lockExit(rSandA, "east", true)
      assert.is_true(hasExitLock(rSandA, "east"))
      lockExit(rSandA, "east", false)
      assert.is_false(hasExitLock(rSandA, "east"))
    end)

    it("hasExitLock returns nothing for an unknown room", function()
      assert.is_nil(hasExitLock(missingRoomId, "east"))
    end)

    it("lockSpecialExit is read back by hasSpecialExitLock", function()
      assert.is_true(lockSpecialExit(rB2, 0, "enter gate", true))
      assert.is_true(hasSpecialExitLock(rB2, 0, "enter gate"))
      assert.is_true(lockSpecialExit(rB2, 0, "enter gate", false))
      assert.is_false(hasSpecialExitLock(rB2, 0, "enter gate"))
    end)

    it("lockSpecialExit returns nil and a message for a non-existent command", function()
      local ok, err = lockSpecialExit(rB2, 0, "no such command", true)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("hasSpecialExitLock returns nil and a message for a non-existent command", function()
      local ok, err = hasSpecialExitLock(rB2, 0, "no such command")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests doors", function()
    it("setDoor is read back by getDoors", function()
      assert.is_true(setDoor(rSandA, "e", 2))
      assert.are.equal(2, getDoors(rSandA)["e"])
      setDoor(rSandA, "e", 0)
    end)

    it("setDoor rejects an out-of-range door type with nil and a message", function()
      local ok, err = setDoor(rSandA, "e", 9)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("setDoor returns nil and a message for a direction with no exit", function()
      local ok, err = setDoor(rSandA, "w", 1)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("getDoors returns nil and a message for an unknown room", function()
      local doors, err = getDoors(missingRoomId)
      assert.is_nil(doors)
      assert.is_string(err)
    end)
  end)

  describe("Tests custom exit lines", function()
    it("addCustomLine is read back by getCustomLines1 and removed by removeCustomLine", function()
      assert.is_true(addCustomLine(rSandA, {{2, 2, 0}}, "e", "dash line", {10, 20, 30}, true))
      local lines = getCustomLines1(rSandA)
      assert.is_table(lines["e"])
      assert.are.equal("dash line", lines["e"]["attributes"]["style"])
      assert.is_true(lines["e"]["attributes"]["arrow"])
      assert.is_true(removeCustomLine(rSandA, "e"))
      assert.is_nil(getCustomLines1(rSandA)["e"])
    end)

    it("getCustomLines uses 0-based point indexing for compatibility", function()
      addCustomLine(rSandA, {{2, 2, 0}}, "e", "solid line", {1, 2, 3}, false)
      local lines = getCustomLines(rSandA)
      assert.is_table(lines["e"])
      assert.is_not_nil(lines["e"]["points"][0])
      removeCustomLine(rSandA, "e")
    end)

    it("addCustomLine rejects a direction the room has no exit for", function()
      local ok, err = addCustomLine(rSandA, {{1, 1, 0}}, "w", "solid line", {0, 0, 0}, false)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addCustomLine draws a line to a target given as a room number", function()
      assert.is_true(addCustomLine(rSandA, rSandB, "e", "solid line", {0, 0, 0}, false))
      assert.is_table(getCustomLines1(rSandA)["e"])
      removeCustomLine(rSandA, "e")
    end)

    it("addCustomLine rejects an empty coordinate table (Issue #5272 crash guard)", function()
      local ok, err = addCustomLine(rSandA, {{}}, "e", "solid line", {0, 0, 0}, false)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addCustomLine rejects a target room in a different area", function()
      local ok, err = addCustomLine(rSandA, rB1, "e", "solid line", {0, 0, 0}, false)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addCustomLine rejects an invalid line style", function()
      local ok, err = addCustomLine(rSandA, {{2, 2, 0}}, "e", "wiggly line", {0, 0, 0}, false)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addCustomLine rejects an out-of-range colour component", function()
      local ok, err = addCustomLine(rSandA, {{2, 2, 0}}, "e", "solid line", {256, 0, 0}, false)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("addCustomLine hard-errors when the second argument is neither number nor table", function()
      assert.has_error(function() addCustomLine(rSandA, "notvalid", "e", "solid line", {0, 0, 0}, false) end)
    end)

    it("getCustomLines1 returns nil and a message for an unknown room", function()
      local lines, err = getCustomLines1(missingRoomId)
      assert.is_nil(lines)
      assert.is_string(err)
    end)
  end)

  describe("Tests custom environment colours", function()
    it("setCustomEnvColor is read back by getCustomEnvColorTable", function()
      assert.is_true(setCustomEnvColor(500, 11, 22, 33, 44))
      local colors = getCustomEnvColorTable()
      assert.is_table(colors[500])
      assert.are.equal(11, colors[500][1])
      assert.are.equal(22, colors[500][2])
      assert.are.equal(33, colors[500][3])
      assert.are.equal(44, colors[500][4])
    end)

    it("setCustomEnvColor for IDs 257-272 also updates the profile ANSI colour (documented sync)", function()
      -- Since Mudlet 4.20 setting 257-272 deliberately mutates the profile's
      -- mapper colours; getCustomEnvColorTable reflects the stored value. This
      -- is profile state that outlives even deleteMap, so restore it from a
      -- finally() hook: a failed assertion below must not leave the persistent
      -- self-test profile stuck on the test colour for later runs.
      local before = getCustomEnvColorTable()[257]
      finally(function()
        setCustomEnvColor(257, before[1], before[2], before[3], before[4])
      end)
      assert.is_true(setCustomEnvColor(257, 1, 2, 3, 255))
      local colors = getCustomEnvColorTable()
      assert.are.equal(1, colors[257][1])
      assert.are.equal(2, colors[257][2])
      assert.are.equal(3, colors[257][3])
    end)

    it("setCustomEnvColor returns nil and a message for an out-of-range component", function()
      local ok, err = setCustomEnvColor(501, 256, 0, 0)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests map labels", function()
    local labelId

    it("createMapLabel returns a numeric label ID", function()
      labelId = createMapLabel(areaAlpha, "MapperSpecLabel", 0, 0, 0, 255, 255, 255, 0, 0, 0)
      assert.is_number(labelId)
      assert.is_true(labelId >= 0)
    end)

    it("getMapLabels lists the label by ID and text", function()
      local labels = getMapLabels(areaAlpha)
      assert.is_table(labels)
      assert.are.equal("MapperSpecLabel", labels[labelId])
    end)

    it("getMapLabel returns the properties of a label looked up by ID", function()
      local label = getMapLabel(areaAlpha, labelId)
      assert.is_table(label)
      assert.are.equal("MapperSpecLabel", label.Text)
    end)

    it("getMapLabel returns nil and a message for an unknown area", function()
      local label, err = getMapLabel(missingAreaId, 0)
      assert.is_nil(label)
      assert.is_string(err)
    end)

    it("createMapImageLabel creates a label (ID >= 0) even when the image is missing", function()
      -- A missing image still creates a real (image-less) label; only an invalid
      -- area returns -1, so pin ID >= 0 and its presence in the area.
      local id = createMapImageLabel(areaAlpha, getMudletHomeDir() .. "/nonexistent.png", 0, 0, 0, 10, 10, 30.0, true)
      assert.is_number(id)
      assert.is_true(id >= 0)
      assert.is_not_nil(getMapLabels(areaAlpha)[id])
      deleteMapLabel(areaAlpha, id)
    end)

    it("deleteMapLabel removes the label from getMapLabels", function()
      deleteMapLabel(areaAlpha, labelId)
      assert.is_nil(getMapLabels(areaAlpha)[labelId])
    end)
  end)

  describe("Tests room user data", function()
    it("setRoomUserData is read back by getRoomUserData", function()
      assert.is_true(setRoomUserData(rSandA, "colour", "blue"))
      assert.are.equal("blue", getRoomUserData(rSandA, "colour"))
    end)

    it("getRoomUserData returns an empty string for a missing key in back-compat mode", function()
      assert.are.equal("", getRoomUserData(rSandA, "no-such-key"))
    end)

    it("getRoomUserData returns nil and a message for a missing key with full error reporting", function()
      local value, err = getRoomUserData(rSandA, "no-such-key", true)
      assert.is_nil(value)
      assert.is_string(err)
    end)

    it("getRoomUserDataKeys lists the keys of a room", function()
      setRoomUserData(rSandB, "alpha", "1")
      setRoomUserData(rSandB, "beta", "2")
      local keys = getRoomUserDataKeys(rSandB)
      assert.is_table(keys)
      local set = {}
      for _, k in pairs(keys) do set[k] = true end
      assert.is_true(set["alpha"])
      assert.is_true(set["beta"])
    end)

    it("getAllRoomUserData returns the whole key/value map", function()
      local data = getAllRoomUserData(rSandB)
      assert.is_table(data)
      assert.are.equal("1", data["alpha"])
    end)

    it("clearRoomUserDataItem removes a single key", function()
      setRoomUserData(rSandB, "toremove", "x")
      assert.is_true(clearRoomUserDataItem(rSandB, "toremove"))
      assert.is_false(clearRoomUserDataItem(rSandB, "toremove"))
    end)

    it("clearRoomUserData empties the room and returns false when already empty", function()
      local id = createRoomID(); addRoom(id); setRoomArea(id, areaAlpha)
      setRoomUserData(id, "k", "v")
      assert.is_true(clearRoomUserData(id))
      assert.is_nil(next(getAllRoomUserData(id)))
      assert.is_false(clearRoomUserData(id))
      deleteRoom(id)
    end)

    it("setRoomUserData returns nil and a message for an unknown room", function()
      local ok, err = setRoomUserData(missingRoomId, "k", "v")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("searchRoomUserData with no arguments lists all room-data keys", function()
      setRoomUserData(rSandA, "searchable", "yes")
      local keys = searchRoomUserData()
      assert.is_table(keys)
      local set = {}
      for _, k in pairs(keys) do set[k] = true end
      assert.is_true(set["searchable"])
    end)

    it("searchRoomUserData with a key and value returns the matching room IDs", function()
      setRoomUserData(rSandA, "team", "red")
      local rooms = searchRoomUserData("team", "red")
      assert.is_table(rooms)
      local set = {}
      for _, id in pairs(rooms) do set[id] = true end
      assert.is_true(set[rSandA])
    end)
  end)

  describe("Tests room name offset and visibility", function()
    -- these three wrap the room.ui_nameOffset / room.ui_showName user data
    -- keys the map renderer reads, so the round trip is the observable effect
    after_each(function()
      clearRoomUserDataItem(rSandA, "room.ui_nameOffset")
      clearRoomUserDataItem(rSandA, "room.ui_showName")
    end)

    it("getRoomNameOffset returns zeroes for a room that has never been offset", function()
      assert.are.same({0, 0}, {getRoomNameOffset(rSandA)})
    end)

    it("setRoomNameOffset round-trips an x and y shift", function()
      setRoomNameOffset(rSandA, 3, 4)
      assert.are.same({3, 4}, {getRoomNameOffset(rSandA)})
      assert.are.equal("3 4", getRoomUserData(rSandA, "room.ui_nameOffset"))
    end)

    it("setRoomNameOffset stores only the y shift when x is zero", function()
      setRoomNameOffset(rSandA, 0, 5)
      assert.are.equal("5", getRoomUserData(rSandA, "room.ui_nameOffset"))
      assert.are.same({0, 5}, {getRoomNameOffset(rSandA)})
    end)

    it("getRoomNameOffset reads a legacy single value as the y shift", function()
      setRoomUserData(rSandA, "room.ui_nameOffset", "7")
      assert.are.same({0, 7}, {getRoomNameOffset(rSandA)})
    end)

    it("setRoomNameVisible writes the flag the renderer looks for", function()
      setRoomNameVisible(rSandA, true)
      assert.are.equal("1", getRoomUserData(rSandA, "room.ui_showName"))
      setRoomNameVisible(rSandA, false)
      assert.are.equal("0", getRoomUserData(rSandA, "room.ui_showName"))
    end)

    it("all three reject arguments of the wrong type", function()
      assert.has_error(function() getRoomNameOffset("1") end)
      assert.has_error(function() setRoomNameOffset(rSandA, "1", 1) end)
      assert.has_error(function() setRoomNameOffset(rSandA, 1, "1") end)
      assert.has_error(function() setRoomNameVisible(rSandA, "yes") end)
    end)

    it("getRoomNameOffset keeps the sign of a negative shift", function()
      -- BUG: setRoomNameOffset stores the offset as "x y", but
      -- getRoomNameOffset reads it back with the pattern '[%.%d]+', which
      -- cannot match a minus sign, so every negative shift comes back positive
      pending("getRoomNameOffset drops the sign of a stored offset - see the Wave 3d report")
      setRoomNameOffset(rSandA, -3, -4)
      assert.are.same({-3, -4}, {getRoomNameOffset(rSandA)})
    end)
  end)

  describe("Tests area user data", function()
    it("setAreaUserData is read back by getAreaUserData", function()
      assert.is_true(setAreaUserData(areaAlpha, "climate", "temperate"))
      assert.are.equal("temperate", getAreaUserData(areaAlpha, "climate"))
    end)

    it("getAllAreaUserData returns the whole key/value map", function()
      setAreaUserData(areaAlpha, "climate", "temperate")
      local data = getAllAreaUserData(areaAlpha)
      assert.is_table(data)
      assert.are.equal("temperate", data["climate"])
    end)

    it("getAreaUserData returns nil and a message for a missing key", function()
      local value, err = getAreaUserData(areaAlpha, "no-such-key")
      assert.is_nil(value)
      assert.is_string(err)
    end)

    it("setAreaUserData rejects an empty key with nil and a message", function()
      local ok, err = setAreaUserData(areaAlpha, "", "value")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("clearAreaUserDataItem removes a single key", function()
      setAreaUserData(areaBeta, "toremove", "x")
      assert.is_true(clearAreaUserDataItem(areaBeta, "toremove"))
      assert.is_false(clearAreaUserDataItem(areaBeta, "toremove"))
    end)

    it("clearAreaUserData empties the area and returns false when already empty", function()
      setAreaUserData(areaGamma, "k", "v")
      assert.is_true(clearAreaUserData(areaGamma))
      assert.is_nil(next(getAllAreaUserData(areaGamma)))
      assert.is_false(clearAreaUserData(areaGamma))
    end)

    it("searchAreaUserData with a key and value returns the matching area IDs", function()
      setAreaUserData(areaBeta, "region", "north")
      local areas = searchAreaUserData("region", "north")
      assert.is_table(areas)
      local set = {}
      for _, id in pairs(areas) do set[id] = true end
      assert.is_true(set[areaBeta])
    end)

    it("getAllAreaUserData returns nil and a message for an unknown area", function()
      local data, err = getAllAreaUserData(missingAreaId)
      assert.is_nil(data)
      assert.is_string(err)
    end)
  end)

  describe("Tests map user data", function()
    it("setMapUserData is read back by getMapUserData", function()
      assert.is_true(setMapUserData("mapper.spec.key", "value"))
      assert.are.equal("value", getMapUserData("mapper.spec.key"))
    end)

    it("getMapUserData returns nil and a message for a missing key", function()
      local value, err = getMapUserData("mapper.spec.missing")
      assert.is_nil(value)
      assert.is_string(err)
    end)

    it("setMapUserData rejects an empty key with nil and a message", function()
      local ok, err = setMapUserData("", "value")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("getAllMapUserData includes a set key", function()
      setMapUserData("mapper.spec.all", "here")
      local data = getAllMapUserData()
      assert.is_table(data)
      assert.are.equal("here", data["mapper.spec.all"])
    end)

    it("clearMapUserDataItem removes a single key", function()
      setMapUserData("mapper.spec.item", "x")
      assert.is_true(clearMapUserDataItem("mapper.spec.item"))
      assert.is_false(clearMapUserDataItem("mapper.spec.item"))
    end)

    it("clearMapUserData wipes all map user data and reports it had data", function()
      setMapUserData("mapper.spec.clearall", "x")
      assert.is_true(clearMapUserData())
      assert.is_nil(getAllMapUserData()["mapper.spec.clearall"])
    end)
  end)

  describe("Tests collision detection", function()
    it("getCollisionLocationsInArea reports coordinates shared by rooms", function()
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaGamma); setRoomCoordinates(a, 7, 7, 7)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaGamma); setRoomCoordinates(b, 7, 7, 7)
      local collisions = getCollisionLocationsInArea(areaGamma)
      assert.is_table(collisions)
      local found = false
      for _, coordinate in pairs(collisions) do
        if coordinate[1] == 7 and coordinate[2] == 7 and coordinate[3] == 7 then
          found = true
        end
      end
      assert.is_true(found)
      deleteRoom(a); deleteRoom(b)
    end)

    it("getCollisionLocationsInArea returns nil and a message for an unknown area", function()
      local collisions, err = getCollisionLocationsInArea(missingAreaId)
      assert.is_nil(collisions)
      assert.is_string(err)
    end)
  end)

  describe("Tests pathfinding with getPath", function()
    after_each(function()
      -- Guarantee a clean routing graph even if an assertion above failed.
      setExitWeightFilter(nil)
      setExitWeight(rA1, "east", 0)
      lockRoom(rA2, false)
      lockExit(rA1, "east", false)
    end)

    it("finds the shortest route and fills the speedwalk globals", function()
      local ok, weight = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.equal(2, weight)
      assert.are.same({"e", "e"}, speedWalkDir)
      assert.are.same({tostring(rA2), tostring(rA3)}, speedWalkPath)
    end)

    it("routes across an area boundary", function()
      local ok = getPath(rA1, rB2)
      assert.is_true(ok)
      assert.are.same({"e", "e", "up", "e"}, speedWalkDir)
      assert.are.same({tostring(rA2), tostring(rA3), tostring(rB1), tostring(rB2)}, speedWalkPath)
    end)

    it("routes through a special exit using its command as the direction", function()
      local ok = getPath(rB2, rG1)
      assert.is_true(ok)
      assert.are.same({"enter gate"}, speedWalkDir)
      assert.are.same({tostring(rG1)}, speedWalkPath)
    end)

    it("returns false, -1 and a message when no path exists", function()
      local ok, weight, err = getPath(rA1, rSandA)
      assert.is_false(ok)
      assert.are.equal(-1, weight)
      assert.is_string(err)
    end)

    it("returns nil and a message for an invalid source roomID", function()
      local ok, err = getPath(missingRoomId, rA3)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("reroutes when an exit weight makes the short route expensive", function()
      setExitWeight(rA1, "east", 100)
      local ok = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.same({"s", "e", "n"}, speedWalkDir)
      assert.are.same({tostring(rA4), tostring(rA5), tostring(rA3)}, speedWalkPath)
    end)

    it("reroutes around a locked room", function()
      lockRoom(rA2, true)
      local ok = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.same({tostring(rA4), tostring(rA5), tostring(rA3)}, speedWalkPath)
    end)

    it("reroutes around a locked exit", function()
      lockExit(rA1, "east", true)
      local ok = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.same({tostring(rA4), tostring(rA5), tostring(rA3)}, speedWalkPath)
    end)

    it("honours an exit weight filter that blocks every exit", function()
      setExitWeightFilter(function() return "block" end)
      local ok = getPath(rA1, rA3)
      assert.is_false(ok)
    end)

    it("honours an exit weight filter that overrides a weight to reroute", function()
      setExitWeightFilter(function(roomId) if roomId == rA2 then return 100000 end end)
      local ok = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.same({tostring(rA4), tostring(rA5), tostring(rA3)}, speedWalkPath)
    end)
  end)

  describe("Tests gotoRoom argument contract", function()
    it("returns nil and a message for an invalid target room", function()
      local ok, err = gotoRoom(missingRoomId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("returns nil and a message when no path leads to the target", function()
      -- Set the player room to an isolated sandbox room so the target is
      -- unreachable and no speedwalk command is ever sent.
      centerview(rSandA)
      -- gotoRoom reports the no-path failure with false + message (it uses
      -- warnArgumentValue's useFalseInsteadofNil form), unlike the invalid-room
      -- case above which returns nil + message.
      local ok, err = gotoRoom(rA1)
      assert.is_false(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests player room and centering", function()
    it("centerview sets the player room that getPlayerRoom reads back", function()
      assert.is_true(centerview(rA1))
      assert.are.equal(rA1, getPlayerRoom())
    end)

    it("centerview returns nil and a message for an unknown room", function()
      local ok, err = centerview(missingRoomId)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests map zoom", function()
    it("setMapZoom is read back by getMapZoom for a given area", function()
      assert.is_true(setMapZoom(15, areaAlpha))
      assert.are.equal(15, getMapZoom(areaAlpha))
    end)

    it("getMapZoom returns nil and a message for an unknown area", function()
      local zoom, err = getMapZoom(missingAreaId)
      assert.is_nil(zoom)
      assert.is_string(err)
    end)
  end)

  describe("Tests createMapper argument contract", function()
    it("hard-errors when the required coordinate arguments are missing", function()
      assert.has_error(function() createMapper() end)
    end)
  end)

  describe("Tests secondary map views", function()
    it("createMapView, getMapViewIds, getMapViewInfo and closeMapView round-trip", function()
      local viewId = createMapView(areaBeta)
      assert.is_number(viewId)
      assert.is_true(viewId > 0)

      local ids = getMapViewIds()
      local set = {}
      for _, id in pairs(ids) do set[id] = true end
      assert.is_true(set[viewId])

      local info = getMapViewInfo(viewId)
      assert.is_table(info)
      assert.are.equal(areaBeta, info.areaId)
      assert.is_number(info.zoom)
      assert.is_number(info.zLevel)
      assert.is_number(info.centeredRoomId)

      assert.is_true(closeMapView(viewId))
    end)

    it("closeAllMapViews reports how many views it closed", function()
      createMapView(areaAlpha)
      createMapView(areaBeta)
      local count = closeAllMapViews()
      assert.is_number(count)
      assert.is_true(count >= 2)
    end)

    it("getMapViewInfo returns nil and a message for an unknown view", function()
      local info, err = getMapViewInfo(987654)
      assert.is_nil(info)
      assert.is_string(err)
    end)
  end)

  describe("Tests registered map info", function()
    it("registerMapInfo makes the label appear in getMapInfo and killMapInfo removes it", function()
      assert.is_true(registerMapInfo("MapperSpecInfo", function() return "info", false, false end))
      assert.is_not_nil(getMapInfo()["MapperSpecInfo"])
      assert.is_true(killMapInfo("MapperSpecInfo"))
      assert.is_nil(getMapInfo()["MapperSpecInfo"])
    end)

    it("killMapInfo returns nil and a message for an unknown label", function()
      local ok, err = killMapInfo("NoSuchMapInfoLabel")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests area image export", function()
    it("exportAreaImage returns true for a valid area (the file is written asynchronously)", function()
      assert.is_true(exportAreaImage(areaAlpha, getMudletHomeDir() .. "/mapper_spec_export.png"))
    end)

    it("exportAreaImage returns nil and a message for an unknown area", function()
      local ok, err = exportAreaImage(missingAreaId, getMudletHomeDir() .. "/unused.png")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  -- Selection functions require a mouse-driven selection that Lua cannot make,
  -- and audit/updateMap have no directly observable return; only their argument
  -- contracts and safe no-op paths are pinned here.
  describe("Tests selection, audit and repaint contracts", function()
    it("getMapSelection returns an empty table when nothing is selected", function()
      local selection = getMapSelection()
      assert.is_table(selection)
      assert.is_nil(selection.center)
    end)

    it("clearMapSelection returns false when there is no selection to clear", function()
      assert.is_false(clearMapSelection())
    end)

    it("auditAreas runs without error", function()
      assert.has_no.errors(function() auditAreas() end)
    end)

    it("updateMap runs without error", function()
      assert.has_no.errors(function() updateMap() end)
    end)
  end)

end)

-- deleteMap wipes the whole map, so it lives in its own block that runs after
-- the shared-fixture tests and builds its own throwaway rooms.
describe("Tests deleteMap", function()
  it("removes every room from the map", function()
    local a = createRoomID(); addRoom(a)
    local b = createRoomID(); addRoom(b)
    assert.is_true(roomExists(a))
    assert.is_true(deleteMap())
    assert.is_false(roomExists(a))
    assert.is_false(roomExists(b))
    assert.is_nil(next(getRooms()))
  end)
end)
