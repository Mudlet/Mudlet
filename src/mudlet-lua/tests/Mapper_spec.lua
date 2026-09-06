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

  -- the dock widget itself outlives closeMapWidget(), but a closed one answers
  -- the map window functions exactly as a never-opened profile does, so these
  -- two reach the same branch whether or not an earlier file opened it
  it("should report that there is no map widget to read a title from", function()
    closeMapWidget()
    local title, err = getMapWindowTitle()
    assert.is_nil(title)
    assert.are.equal("no floating/dockable type map window found", err)
  end)

  it("should report that there is no map widget to read a geometry from", function()
    closeMapWidget()
    local x, err = getMapWidgetGeometry()
    assert.is_nil(x)
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

  it("refuses a roomID of zero or below", function()
    assert.is_false(addRoom(0))
    assert.is_false(roomExists(0))
    assert.is_false(addRoom(-3))
    assert.is_false(roomExists(-3))
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

  -- saveJsonMap/loadJsonMap replace and re-initialise the entire map, which is
  -- incompatible with this shared fixture, so they have a block of their own
  -- below.

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

    it("removes an area name that never got an area of its own", function()
      -- setAreaName on an unused ID registers the name without instantiating
      -- the area, which from Lua only happens once a room is moved into it, so
      -- this is the one way to reach a name with nothing behind it
      local orphanAreaId = 990000003
      assert.is_nil(getAreaTable()["MapperSpecOrphanArea"])
      assert.is_true(setAreaName(orphanAreaId, "MapperSpecOrphanArea"))
      assert.are.equal(orphanAreaId, getAreaTable()["MapperSpecOrphanArea"])

      assert.is_true(deleteArea(orphanAreaId))
      assert.is_nil(getAreaTable()["MapperSpecOrphanArea"])
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

    it("setExit takes the short alias of every direction, whatever its case", function()
      -- dirToNumber() accepts a one or two letter alias as well as the full
      -- name, and nothing reached that half of it: every spec here writes a
      -- full name, and the lockExit/hasExitLock wrappers in Other.lua translate
      -- their direction to a number before the C++ call ever sees it.
      local aliases = {
        {"n", "north"}, {"e", "east"}, {"s", "south"}, {"w", "west"},
        {"u", "up"}, {"d", "down"}, {"ne", "northeast"}, {"nw", "northwest"},
        {"se", "southeast"}, {"sw", "southwest"}, {"i", "in"}, {"o", "out"},
      }
      local a = createRoomID(); addRoom(a); setRoomArea(a, areaAlpha)
      local b = createRoomID(); addRoom(b); setRoomArea(b, areaAlpha)
      for _, pair in ipairs(aliases) do
        local short, long = pair[1], pair[2]
        assert.is_true(setExit(a, b, short), short)
        assert.are.equal(b, getRoomExits(a)[long], short)
      end
      -- the direction is lowercased before it is matched, so the aliases are
      -- as case-insensitive as the full names are
      local c = createRoomID(); addRoom(c); setRoomArea(c, areaAlpha)
      assert.is_true(setExit(a, c, "SW"))
      assert.are.equal(c, getRoomExits(a)["southwest"])
      deleteRoom(a); deleteRoom(b); deleteRoom(c)
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
      setRoomNameOffset(rSandA, -3, -4)
      assert.are.equal("-3 -4", getRoomUserData(rSandA, "room.ui_nameOffset"))
      assert.are.same({-3, -4}, {getRoomNameOffset(rSandA)})
    end)

    it("getRoomNameOffset keeps the sign of a mixed pair", function()
      setRoomNameOffset(rSandA, -3, 4)
      assert.are.same({-3, 4}, {getRoomNameOffset(rSandA)})
      setRoomNameOffset(rSandA, 3, -4)
      assert.are.same({3, -4}, {getRoomNameOffset(rSandA)})
    end)

    it("getRoomNameOffset keeps the sign of a lone negative y shift", function()
      -- x == 0 makes setRoomNameOffset store the y shift on its own, which is
      -- the one-value branch of the reader
      setRoomNameOffset(rSandA, 0, -5)
      assert.are.equal("-5", getRoomUserData(rSandA, "room.ui_nameOffset"))
      assert.are.same({0, -5}, {getRoomNameOffset(rSandA)})
    end)

    it("getRoomNameOffset keeps the sign of a fractional offset", function()
      -- T2DMap reads the same user data with QString::toDouble(), so the Lua
      -- getter has to accept everything the renderer does
      setRoomUserData(rSandA, "room.ui_nameOffset", "-1.5 -2.5")
      assert.are.same({-1.5, -2.5}, {getRoomNameOffset(rSandA)})
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
      lockExit(rA1, "east", false)
      for _, id in ipairs({rA2, rB1, rB2, rG1, rSandA, rSandB}) do
        lockRoom(id, false)
      end
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

    -- Every case above edits the map first, so each one searches a graph that
    -- has just been rebuilt. These four do not, which is what puts them on the
    -- state findPath() carries from one search to the next.

    it("does not inherit the previous search's state", function()
      local firstOk = getPath(rA1, rA3)
      assert.is_true(firstOk)
      assert.are.same({tostring(rA2), tostring(rA3)}, speedWalkPath)

      -- Reversed, so the second search has to better the rooms the first one
      -- had already settled rather than reaching fresh ones.
      local ok, weight = getPath(rA3, rA1)
      assert.is_true(ok)
      assert.are.equal(2, weight)
      assert.are.same({"w", "w"}, speedWalkDir)
      assert.are.same({tostring(rA2), tostring(rA1)}, speedWalkPath)
    end)

    it("does not inherit the state of a search that found nothing", function()
      -- Giving up means settling every room reachable from the start, so this
      -- leaves behind the largest amount of state a search on this map can.
      local failedOk = getPath(rA1, rSandA)
      assert.is_false(failedOk)

      local ok, weight = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.equal(2, weight)
      assert.are.same({tostring(rA2), tostring(rA3)}, speedWalkPath)
    end)

    it("does not reuse the old room numbering after the graph is rebuilt", function()
      local firstOk = getPath(rA1, rB2)
      assert.is_true(firstOk)

      -- Locking rooms drops them from the graph, so the rooms left are
      -- renumbered and the numbers the search above recorded no longer name the
      -- rooms they did. Fewer rooms than before is the case that matters: a
      -- surviving number can then be past the end of the state itself.
      for _, id in ipairs({rA2, rB1, rB2, rG1, rSandA, rSandB}) do
        lockRoom(id, true)
      end

      local ok, weight = getPath(rA1, rA3)
      assert.is_true(ok)
      assert.are.equal(3, weight)
      assert.are.same({tostring(rA4), tostring(rA5), tostring(rA3)}, speedWalkPath)
    end)
  end)

  describe("Tests pathfinding where the weights disagree with the coordinates", function()
    -- A* is steered by straight-line distance to the target but pays in exit
    -- weights, and nothing makes the two agree. rX sits right beside the target
    -- and is reached early over a costly exit; the cheap way to it only turns up
    -- later, through rY, which is the wrong way entirely as the crow flies. The
    -- route through rZ is there to be beaten: it wins unless the better price
    -- for rX is carried forward to the target.
    local areaWeighted
    local rWStart, rX, rY, rZ, rWGoal

    setup(function()
      areaWeighted = addAreaName("MapperSpecWeighted")

      local function makeRoom(x, y)
        local id = createRoomID()
        addRoom(id)
        setRoomArea(id, areaWeighted)
        setRoomCoordinates(id, x, y, 0)
        return id
      end

      rWGoal = makeRoom(0, 0)
      rX = makeRoom(1, 0)
      rZ = makeRoom(0, 10)
      rY = makeRoom(0, 20)
      rWStart = makeRoom(0, 30)

      -- One-way throughout, so the graph is exactly the one the weights
      -- describe and no return exit offers a cheaper way round.
      setExit(rWStart, rX, "southeast"); setExitWeight(rWStart, "southeast", 10)
      setExit(rWStart, rZ, "southwest"); setExitWeight(rWStart, "southwest", 5)
      setExit(rWStart, rY, "south"); setExitWeight(rWStart, "south", 1)
      setExit(rY, rX, "southeast"); setExitWeight(rY, "southeast", 1)
      setExit(rZ, rWGoal, "south"); setExitWeight(rZ, "south", 20)
      setExit(rX, rWGoal, "west"); setExitWeight(rX, "west", 20)
    end)

    teardown(function()
      for _, id in ipairs({rWStart, rX, rY, rZ, rWGoal}) do
        deleteRoom(id)
      end
      deleteArea("MapperSpecWeighted")
    end)

    it("takes the cheapest route even though a nearer room was settled first", function()
      local ok, weight = getPath(rWStart, rWGoal)
      assert.is_true(ok)
      assert.are.equal(22, weight)
      assert.are.same({tostring(rY), tostring(rX), tostring(rWGoal)}, speedWalkPath)
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

  describe("Tests the map background and room exit colours", function()
    local originalBackground, originalRoomExits

    setup(function()
      originalBackground = {getMapBackgroundColor()}
      originalRoomExits = {getMapRoomExitsColor()}
    end)

    teardown(function()
      setMapBackgroundColor(unpack(originalBackground))
      setMapRoomExitsColor(unpack(originalRoomExits))
    end)

    it("setMapBackgroundColor is read back by getMapBackgroundColor", function()
      assert.is_true(setMapBackgroundColor(12, 34, 56, 78))
      assert.are.same({12, 34, 56, 78}, {getMapBackgroundColor()})
    end)

    it("a background set without an alpha is opaque", function()
      assert.is_true(setMapBackgroundColor(12, 34, 56, 78))
      assert.is_true(setMapBackgroundColor(9, 8, 7))
      assert.are.same({9, 8, 7, 255}, {getMapBackgroundColor()})
    end)

    it("setMapRoomExitsColor is read back by getMapRoomExitsColor", function()
      -- three components, not four: the exit colour carries no alpha either way
      assert.is_true(setMapRoomExitsColor(21, 43, 65))
      assert.are.same({21, 43, 65}, {getMapRoomExitsColor()})
    end)

    it("a component outside 0-255 is refused and changes nothing", function()
      assert.is_true(setMapBackgroundColor(10, 20, 30, 40))
      local rejected = {{-1, 20, 30}, {10, 256, 30}, {10, 20, -5}, {10, 20, 30, 300}}
      for _, components in ipairs(rejected) do
        local ok, err = setMapBackgroundColor(unpack(components))
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_truthy(err:find("needs to be between 0-255", 1, true), err)
      end
      assert.are.same({10, 20, 30, 40}, {getMapBackgroundColor()})

      assert.is_true(setMapRoomExitsColor(11, 22, 33))
      local exitsOk, exitsErr = setMapRoomExitsColor(11, 22, 999)
      assert.is_nil(exitsOk)
      assert.is_string(exitsErr)
      assert.are.same({11, 22, 33}, {getMapRoomExitsColor()})
    end)

    it("a component that is not a number hard-errors", function()
      assert.has_error(function() setMapBackgroundColor("red", 0, 0) end)
      assert.has_error(function() setMapRoomExitsColor(0, "green", 0) end)
    end)

    it("a component too large for an int hard-errors rather than wrapping", function()
      -- getVerifiedInt() reads a Lua number as a 64 bit integer and refuses one
      -- that will not fit an int, which is a separate refusal from the type
      -- check above and from the 0-255 range check: without it the value would
      -- be truncated into range and silently accepted.
      local ok, err = pcall(function() setMapBackgroundColor(2 ^ 40, 0, 0) end)
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("integer over/under-flow", 1, true), tostring(err))
    end)
  end)

  describe("Tests setDefaultAreaVisible", function()
    -- Opened here as well as in the outer setup: the success branch needs the
    -- mapper widget, and busted can be asked to shuffle these blocks.
    setup(function()
      assert.is_true(openMapWidget())
    end)

    -- The flag itself has no Lua readback: TMap::mShowDefaultArea is read by
    -- the mapper's area list, the map view's painting and the preferences
    -- checkbox, and by no Lua function other than this setter's own fixup.
    -- What a spec can hold onto is therefore the call's own contract.
    teardown(function()
      setDefaultAreaVisible(true)
    end)

    it("reports success while the mapper widget is up", function()
      assert.is_true(setDefaultAreaVisible(false))
      -- and again the other way, which must also report success. This does not
      -- reach the combo box fixup inside: that needs the 2D map parked on the
      -- default area, and the fixture's rooms are all in named ones.
      assert.is_true(setDefaultAreaVisible(true))
    end)

    it("hard-errors when its argument is not a boolean", function()
      assert.has_error(function() setDefaultAreaVisible("yes") end)
      assert.has_error(function() setDefaultAreaVisible() end)
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

-- closeMapWidget() has to leave the profile in a state that is distinguishable
-- from "the map widget is open", or every map window function keeps answering
-- for a widget the script just put away.
--
-- The map dock has no window name, so windowVisible() cannot reach it and these
-- specs read the state through the map window functions instead. That works
-- because Host::mapWidget() derives its answer from the dock's own hidden
-- state: drop the hide() out of Host::closeMapWidget() and the two specs below
-- that assert the closed answers fail.
describe("Tests the open and closed states of the map widget", function()
  setup(function()
    assert.is_true(openMapWidget())
  end)

  teardown(function()
    -- back to a right-docked, open widget: the position loop below leaves it
    -- docked at the bottom otherwise, which shrinks the main console for
    -- everything that runs after this file
    openMapWidget("r")
    resetMapWindowTitle()
  end)

  before_each(function()
    openMapWidget()
  end)

  -- companion guard rather than a guard for the bug: closeMapWidget() reported
  -- "already closed" before this was fixed too. It is here so that a fix which
  -- stopped distinguishing the two calls would be caught.
  it("reports the widget as closed once, and as already closed after that", function()
    assert.is_true(closeMapWidget())
    local closed, message = closeMapWidget()
    assert.is_nil(closed)
    assert.are.equal("map widget already closed", message)
  end)

  it("stops setMapWindowTitle from retitling a widget that was closed", function()
    assert.is_true(setMapWindowTitle("still open"))
    assert.is_true(closeMapWidget())
    local set, message = setMapWindowTitle("closed already")
    assert.is_nil(set)
    assert.are.equal("no floating/dockable type map window found", message)
  end)

  it("makes the map window getters agree with setMapWindowTitle", function()
    assert.is_true(closeMapWidget())
    local title, titleMessage = getMapWindowTitle()
    assert.is_nil(title)
    local x, geometryMessage = getMapWidgetGeometry()
    assert.is_nil(x)
    -- same wording from all three, so a script can test one and trust the rest
    assert.are.equal("no floating/dockable type map window found", titleMessage)
    assert.are.equal("no floating/dockable type map window found", geometryMessage)
  end)

  it("hands the widget back on reopen", function()
    setMapWindowTitle("before the close")
    assert.is_true(closeMapWidget())
    assert.is_true(openMapWidget())
    -- the same dock comes back rather than a fresh one, so its title survives
    assert.are.equal("before the close", getMapWindowTitle())
    assert.are.equal(4, select("#", getMapWidgetGeometry()))
    assert.is_true(setMapWindowTitle("after the reopen"))
    assert.are.equal("after the reopen", getMapWindowTitle())
  end)

  it("reopens from every docking position", function()
    for _, position in ipairs({"f", "l", "r", "t", "b"}) do
      assert.is_true(closeMapWidget())
      assert.is_true(openMapWidget(position), "could not reopen the map widget at " .. position)
      assert.is_string(getMapWindowTitle())
    end
  end)

  -- moveMapWidget/resizeMapWidget are openMapWidget in disguise, so they reopen
  -- a closed widget rather than failing the way the getters do. Pinned because
  -- it is the one place where the map functions do not agree about the state.
  it("lets moveMapWidget and resizeMapWidget reopen a closed widget", function()
    assert.is_true(closeMapWidget())
    resizeMapWidget(640, 480)
    local _, _, width, height = getMapWidgetGeometry()
    assert.are.same({640, 480}, {width, height})

    assert.is_true(closeMapWidget())
    moveMapWidget(120, 130)
    assert.are.equal(4, select("#", getMapWidgetGeometry()))
  end)

  -- Neither of these can be reached from Lua, so they are recorded rather than
  -- covered: the dock's own title bar close button and mudlet's map toolbar
  -- button both hide the same dock, and Host::mapWidget() reads the dock's
  -- hidden state so that it follows them without either having to know.
  pending("the map dock's title bar close button leaves the map window functions reporting no map window - needs GUI automation")

  pending("the map toolbar button handing the map to a main window dock leaves the map window functions reporting no map window - needs GUI automation")
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

-- saveMap/loadMap replace the whole map, so this block runs last, after
-- deleteMap has already emptied it, and puts back whatever it found: the map is
-- shared with everything that runs after this file.
describe("Tests saveMap and loadMap", function()
  local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
  assert(specDirectory, "Mapper_spec.lua has to be run from a file so that it can find its fixtures")
  local fixtureMap = specDirectory .. "/fixtures/maps/minimal-map.xml"

  -- Scratch names inside the self-test profile that nothing else writes. A run
  -- that died between the setup below and its teardown leaves them behind, and
  -- the backup one would then be a map from a different run, so they are
  -- cleared on the way in rather than trusted.
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/mapper_spec_backup.dat"
  local savePath = mapDirectory .. "/mapper_spec_roundtrip.dat"
  local brokenXmlPath = getMudletHomeDir() .. "/mapper_spec_broken.xml"
  local notAMapXmlPath = getMudletHomeDir() .. "/mapper_spec_notamap.xml"
  local notXmlPath = getMudletHomeDir() .. "/mapper_spec_notxml.xml"

  -- saveMap() with no arguments writes a timestamped file of its own choosing,
  -- so the only way to clear up after it is to spot what appeared
  local function mapFiles()
    local files = {}
    for entry in lfs.dir(mapDirectory) do
      if entry:lower():match("%.dat$") then
        files[entry] = true
      end
    end
    return files
  end

  local function removeNewMapFiles(before)
    for entry in pairs(mapFiles()) do
      if not before[entry] then
        os.remove(mapDirectory .. "/" .. entry)
      end
    end
  end

  -- three rooms in one area, carrying a value for every kind of room data the
  -- binary format stores separately, so a round-trip that dropped one shows up
  local roomA, roomB, roomC
  local function buildMap()
    deleteMap()
    local area = addAreaName("MapperSpecSaveArea")
    roomA, roomB, roomC = createRoomID(), nil, nil
    addRoom(roomA)
    roomB = createRoomID(); addRoom(roomB)
    roomC = createRoomID(); addRoom(roomC)
    for _, id in ipairs({roomA, roomB, roomC}) do
      setRoomArea(id, area)
    end
    setRoomCoordinates(roomA, 0, 0, 0)
    setRoomCoordinates(roomB, 3, -4, 5)
    setRoomCoordinates(roomC, 1, 1, 1)
    setRoomName(roomA, "Saved Room A")
    setRoomName(roomB, "Saved Room B")
    setRoomEnv(roomB, 42)
    setRoomWeight(roomB, 7)
    setExit(roomA, roomB, "east")
    setExit(roomB, roomA, "west")
    addSpecialExit(roomB, roomC, "squeeze through")
    setDoor(roomA, "e", 2)
    setRoomUserData(roomA, "spec key", "spec value")
    setRoomIDbyHash(roomA, "mapperSpecSavedHash")
    -- the room symbol is stored as a number below format version 19 and as a
    -- string from 19 up, and the custom environment colours are their own
    -- section, so both are here for the versioned round-trip below
    setRoomChar(roomB, "X")
    setCustomEnvColor(42, 10, 20, 30, 255)
    return area
  end

  local function assertMapRestored()
    assert.is_true(roomExists(roomA))
    assert.is_true(roomExists(roomB))
    assert.are.equal("Saved Room A", getRoomName(roomA))
    assert.are.equal("Saved Room B", getRoomName(roomB))
    assert.are.same({3, -4, 5}, {getRoomCoordinates(roomB)})
    assert.are.equal(42, getRoomEnv(roomB))
    assert.are.equal(7, getRoomWeight(roomB))
    assert.are.equal(roomB, getRoomExits(roomA)["east"])
    assert.are.equal(roomC, getSpecialExitsSwap(roomB)["squeeze through"])
    assert.are.equal(2, getDoors(roomA)["e"])
    assert.are.equal("spec value", getRoomUserData(roomA, "spec key"))
    assert.are.equal(roomA, getRoomIDbyHash("mapperSpecSavedHash"))
    assert.are.equal("MapperSpecSaveArea", getRoomAreaName(getRoomArea(roomA)))
    assert.are.equal("X", getRoomChar(roomB))
    assert.are.same({10, 20, 30, 255}, getCustomEnvColorTable()[42])
  end

  setup(function()
    os.remove(backupPath)
    os.remove(savePath)
    os.remove(brokenXmlPath)
    os.remove(notAMapXmlPath)
    os.remove(notXmlPath)
    -- snapshot whatever map the rest of the suite left behind, so that the
    -- teardown can hand it back untouched
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this block replaced could not be put back")
    -- loadMap shows the mapper wherever it last was; the block above this one
    -- guarantees an open, right-docked widget to everything that follows, so
    -- put that back rather than leaving it wherever the loads left it
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(savePath)
    os.remove(brokenXmlPath)
    os.remove(notAMapXmlPath)
    os.remove(notXmlPath)
  end)

  describe("Tests the saveMap argument contract", function()
    it("hard-errors on a save location that is not a string", function()
      -- a table rather than a number: Lua coerces a number to a string, and
      -- saveMap takes it, writing a map file named after the number
      assert.has_error(function() saveMap({}) end)
    end)

    it("hard-errors on a format version that is not a number", function()
      assert.has_error(function() saveMap(savePath, "twenty") end)
    end)

    it("reports failure rather than raising when the file cannot be written", function()
      -- false means the save failed: saveMap answers with success, not with an
      -- error flag, which is worth pinning because it reads the other way round
      assert.is_false(saveMap("/nosuchdirectory/mapper_spec.dat"))
    end)

    it("refuses a format version this Mudlet cannot write", function()
      finally(function()
        -- a refused save leaves the map flagged as unsaved, which puts a
        -- warning on the mapper for every spec that runs after this one
        saveMap(savePath)
        os.remove(savePath)
      end)
      assert.is_false(saveMap(savePath, 9999))
    end)

    it("refuses a format version older than this Mudlet can write", function()
      finally(function()
        saveMap(savePath)
        os.remove(savePath)
      end)
      -- 16 is one below the oldest format this Mudlet writes, and a refusal
      -- has to be as flat as the one for a version that is too new
      assert.is_false(saveMap(savePath, 16))
      assert.is_false(saveMap(savePath, -1))
    end)

    it("resolves a relative location against the profile directory", function()
      -- and not against the directory Mudlet happens to have been started in,
      -- which for a spec run is the build or source tree
      local relative = "mapper_spec_relative.dat"
      local function clear()
        os.remove(getMudletHomeDir() .. "/" .. relative)
        os.remove(relative)
      end
      clear()
      finally(clear)

      assert.is_true(saveMap(relative))
      assert.is_true(io.exists(getMudletHomeDir() .. "/" .. relative))
      assert.is_false(io.exists(relative))
      -- and loadMap has to look in the same place, or a map saved under a bare
      -- name cannot be loaded back under it
      assert.is_true(loadMap(relative))
    end)

    it("resolves a number the same way, Lua having made a name out of it", function()
      local numbered = "42"
      local function clear()
        os.remove(getMudletHomeDir() .. "/" .. numbered)
        os.remove(numbered)
      end
      clear()
      finally(clear)

      assert.is_true(saveMap(42))
      assert.is_true(io.exists(getMudletHomeDir() .. "/" .. numbered))
      assert.is_false(io.exists(numbered))
    end)
  end)

  -- Careful with the order of anything added here: a load that fails can still
  -- have emptied the map first, both for a missing binary file
  -- (TMainConsole::loadMap clears before it restores) and for a map document
  -- that will not parse (TMap::readXmlMapFile clears before it parses), so most
  -- of these leave no map behind for the next spec. A file that is not a map
  -- document at all is the exception: it is refused before the clear.
  describe("Tests the loadMap argument contract", function()
    it("hard-errors on a path that is not a string", function()
      assert.has_error(function() loadMap({}) end)
    end)

    it("returns false for a binary map file that is not there", function()
      assert.is_false(loadMap(mapDirectory .. "/nosuchmapfile.dat"))
    end)

    it("returns nil and a message naming the missing XML file", function()
      local ok, message = loadMap(mapDirectory .. "/nosuchmapfile.xml")
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_truthy(message:find("was not found", 1, true))
      assert.is_truthy(message:find("nosuchmapfile.xml", 1, true))
    end)

    it("returns nil and a message for an XML file it cannot parse", function()
      local file = assert(io.open(brokenXmlPath, "w"))
      file:write("<map><areas><area id=\"1\" name=\"unterminated\">")
      file:close()

      local ok, message = loadMap(brokenXmlPath)
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_truthy(message:find("failure to import XML map file", 1, true))
    end)

    it("refuses an XML file that holds no map, keeping the one that is loaded", function()
      -- what a game with no map to offer answers a map download with: a page
      -- saying so, which is well-formed XML full of elements the map reader
      -- does not know. Every one of them parses, so the reader used to count
      -- that as a successful import - of nothing, over the loaded map.
      local file = assert(io.open(notAMapXmlPath, "w"))
      file:write("<html><body>Not found</body></html>")
      file:close()

      deleteMap()
      local keeper = createRoomID()
      addRoom(keeper)
      assert.is_true(roomExists(keeper))

      local ok, message = loadMap(notAMapXmlPath)
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_truthy(message:find("does not contain a map", 1, true))
      assert.is_true(roomExists(keeper), "the loaded map was thrown away for a file that holds no map")
    end)

    -- a damaged map file, as against somebody else's document: both are refused before
    -- the map is cleared, but a player whose own map will not parse needs to be told
    -- that rather than that the file was never a map
    it("refuses a file that is not XML at all, keeping the one that is loaded", function()
      local file = assert(io.open(notXmlPath, "w"))
      file:write("garbage")
      file:close()

      deleteMap()
      local keeper = createRoomID()
      addRoom(keeper)
      assert.is_true(roomExists(keeper))

      local ok, message = loadMap(notXmlPath)
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_truthy(message:find("damaged or unreadable", 1, true))
      assert.is_falsy(message:find("does not contain a map", 1, true))
      assert.is_true(roomExists(keeper), "the loaded map was thrown away for a file that is not XML")
    end)
  end)

  describe("Tests the saveMap and loadMap round-trip", function()
    it("puts every kind of room data back exactly as it was saved", function()
      buildMap()
      assert.is_true(saveMap(savePath))

      -- wipe the lot, so that a loadMap which did nothing at all cannot pass
      deleteMap()
      assert.is_false(roomExists(roomA))

      assert.is_true(loadMap(savePath))
      assertMapRestored()
    end)

    it("replaces what is on the map rather than merging into it", function()
      buildMap()
      saveMap(savePath)

      local strayArea = addAreaName("MapperSpecStrayArea")
      local stray = createRoomID()
      addRoom(stray)
      setRoomArea(stray, strayArea)

      assert.is_true(loadMap(savePath))
      assert.is_false(roomExists(stray))
      assert.is_nil(getAreaTable()["MapperSpecStrayArea"])
      assertMapRestored()
    end)

    it("round-trips through the oldest format version Mudlet still writes", function()
      buildMap()
      assert.is_true(saveMap(savePath, 17))
      deleteMap()

      assert.is_true(loadMap(savePath))
      -- everything, including the room symbol, which version 17 writes as a
      -- number where 19 and up write a string: the older spelling has to come
      -- back as the same character
      assertMapRestored()
    end)

    it("saves into the profile's own map folder when given no path", function()
      local before = mapFiles()
      finally(function() removeNewMapFiles(before) end)

      buildMap()
      assert.is_true(saveMap())

      local added = 0
      for entry in pairs(mapFiles()) do
        if not before[entry] then
          added = added + 1
        end
      end
      -- the name it picks is a timestamp to the second, so a second save
      -- inside the same second would land on the same file rather than a new
      -- one; what matters is that it wrote into the profile at all
      assert.is_true(added >= 1, "saveMap() with no path should write a map file of its own")
    end)

    it("restores the profile's most recent map when given no path", function()
      local before = mapFiles()
      finally(function() removeNewMapFiles(before) end)

      buildMap()
      -- the other map files in this folder also hold a buildMap() map, so mark
      -- this one: loadMap() picks the newest file and has to pick this one
      setRoomName(roomC, "Only In The Newest Save")
      assert.is_true(saveMap())
      deleteMap()

      assert.is_true(loadMap())
      assertMapRestored()
      assert.are.equal("Only In The Newest Save", getRoomName(roomC))
    end)
  end)

  describe("Tests loadMap importing an XML map", function()
    -- the fixture's own IDs, so that a load which quietly did nothing cannot
    -- be mistaken for a successful import
    local importedRoomA, importedRoomB = 4001, 4002

    before_each(function()
      deleteMap()
      assert.is_true(loadMap(fixtureMap))
    end)

    it("creates the rooms the file describes", function()
      assert.is_true(roomExists(importedRoomA))
      assert.is_true(roomExists(importedRoomB))
      assert.are.equal("Import Room One", getRoomName(importedRoomA))
      assert.are.equal("Import Room Two", getRoomName(importedRoomB))
    end)

    it("puts the rooms in the area the file names", function()
      assert.are.equal(4001, getAreaTable()["Mapper Spec Import Area"])
      assert.are.equal(4001, getRoomArea(importedRoomA))
      assert.are.equal(4001, getRoomArea(importedRoomB))
    end)

    it("reads the coordinates and the environment of each room", function()
      assert.are.same({0, 0, 0}, {getRoomCoordinates(importedRoomA)})
      assert.are.same({1, 2, 3}, {getRoomCoordinates(importedRoomB)})
      assert.are.equal(169, getRoomEnv(importedRoomA))
      assert.are.equal(170, getRoomEnv(importedRoomB))
    end)

    it("reads normal exits, doors and IRE-style special exits", function()
      assert.are.equal(importedRoomB, getRoomExits(importedRoomA)["east"])
      assert.are.equal(importedRoomA, getRoomExits(importedRoomB)["west"])
      assert.are.equal(2, getDoors(importedRoomB)["w"])
      -- an exit with no direction but a command is how IRE maps spell a
      -- special exit, and it has to arrive as one
      assert.are.equal(importedRoomB, getSpecialExitsSwap(importedRoomA)["enter gate"])
    end)

    it("turns a hidden exit into a locked door", function()
      -- IRE maps mark an exit the player cannot see with hidden="1" rather than
      -- with a door type, and it arrives as door type 3, a locked door
      assert.are.equal(importedRoomA, getRoomExits(importedRoomB)["north"])
      assert.are.equal(3, getDoors(importedRoomB)["n"])
    end)

    it("turns a room feature into room user data", function()
      assert.are.equal("true", getRoomUserData(importedRoomA, "feature-shop"))
    end)

    -- the file's <environments> block fills TMap::mEnvColors, which maps an
    -- environment id to a stock colour index. getCustomEnvColorTable() reads
    -- mCustomEnvColors, a different map, so there is nothing to read this back
    -- with from Lua
    pending("the environment colours an XML map declares have no Lua getter")

    it("throws away the map that was there before the import", function()
      local stray = createRoomID()
      addRoom(stray)
      assert.is_true(loadMap(fixtureMap))
      assert.is_false(roomExists(stray))
    end)
  end)

  -- A real IRE map at full size - 22,854 rooms over 379 areas. Routing is the
  -- one part of the mapper whose behaviour a small map cannot describe: the
  -- distance heuristic gives up and returns 1 the moment a route leaves the
  -- target's area, a one-way exit only matters when there is a second way
  -- round, and an exit weight only reroutes when there is something to reroute
  -- onto. See fixtures/maps/README.md for the map's provenance and shape.
  describe("Tests pathfinding over a full-sized IRE map", function()
    local archivePath = specDirectory .. "/fixtures/maps/achaea-map.zip"
    local extractedPath = getMudletHomeDir() .. "/achaea-map.xml"
    local mapRooms, mapAreas = 22854, 379

    -- Every id below is load-bearing for the reason its name gives, so none of
    -- them can be swapped for another room that merely also exists.
    local ashtanWorkshop, cyreneBellTower = 107, 1192   -- 96 steps, 7 areas apart
    local riparium, blackrock = 6595, 18690             -- the map's long diagonal
    local mhaldorRoom, eleusisRoom = 4417, 2377         -- no route: different components
    local cliffTop, cliffFoot = 1201, 9326              -- "down" is one-way
    local hillside, nimick = 1432, 20646                -- joined by a hidden exit
    local nimickDetour = 43                             -- steps round it when it is shut

    -- speedWalkDir spells a direction short and getRoomExits spells it long,
    -- so walking one against the other needs the two put side by side
    local exitDirectionFor = {n = "north", ne = "northeast", e = "east", se = "southeast",
                              s = "south", sw = "southwest", w = "west", nw = "northwest",
                              up = "up", down = "down", ["in"] = "in", out = "out"}

    -- A route of the right length is still wrong if its steps do not join up,
    -- which comparing lengths alone would not notice.
    local function routeIsWalkable(from)
      local current = from
      for step, direction in ipairs(speedWalkDir) do
        local expected = tonumber(speedWalkPath[step])
        local exits = getRoomExits(current)
        if exits[exitDirectionFor[direction] or direction] ~= expected then
          return false, ("step %d: %s does not lead from room %d to room %s"):format(step, direction, current, tostring(expected))
        end
        current = expected
      end
      return true
    end

    -- Two different libraries answer to the global `zip`: Mudlet asks for
    -- lua-zip (brimworks) first and falls back to luazip (Kepler), and only
    -- the latter's entry:read() takes io.read's "*a". brimworks wants a byte
    -- count and raises "number expected, got string" for anything else, so
    -- read in fixed chunks, which both understand, until one comes back empty.
    local function readEntry(entry)
      local chunks = {}
      while true do
        local chunk = entry:read(1024 * 1024)
        if not chunk or chunk == "" then
          break
        end
        chunks[#chunks + 1] = chunk
      end
      return table.concat(chunks)
    end

    local function areasVisited()
      local seen, count = {}, 0
      for _, id in ipairs(speedWalkPath) do
        local area = getRoomArea(tonumber(id))
        if not seen[area] then
          seen[area] = true
          count = count + 1
        end
      end
      return count
    end

    setup(function()
      -- zip is only defined when Mudlet preloaded the module, and lua-zip is a
      -- required rock on every platform, so a nil here is a broken environment
      -- rather than a reason to skip the block
      assert.is_table(zip, "the lua-zip module is missing, so the map fixture cannot be unpacked")
      local archive, openError = zip.open(archivePath)
      assert(archive, ("could not open %s: %s"):format(archivePath, tostring(openError)))
      local entry = assert(archive:open("achaea-map.xml"), "achaea-map.zip does not hold achaea-map.xml")
      local document = readEntry(entry)
      entry:close()
      archive:close()
      local out = assert(io.open(extractedPath, "wb"))
      out:write(document)
      out:close()

      deleteMap()
      assert.is_true(loadMap(extractedPath))
      -- an import that quietly did nothing would leave every route below
      -- failing for a reason that has nothing to do with routing
      local rooms = 0
      for _ in pairs(getRooms()) do rooms = rooms + 1 end
      assert.are.equal(mapRooms, rooms)
      local areas = 0
      for _ in pairs(getAreaTable()) do areas = areas + 1 end
      assert.are.equal(mapAreas, areas)
    end)

    teardown(function()
      os.remove(extractedPath)
    end)

    after_each(function()
      -- leave the graph as the setup built it even if an assertion above
      -- stopped a spec before its own clean-up
      setExitWeight(hillside, "east", 0)
      setRoomWeight(nimick, 1)
      lockExit(hillside, "east", false)
    end)

    it("walks a ninety-six step route across seven areas", function()
      local ok, weight = getPath(ashtanWorkshop, cyreneBellTower)
      assert.is_true(ok)
      assert.are.equal(96, weight)
      assert.are.equal(96, #speedWalkDir)
      assert.are.equal(96, #speedWalkPath)
      assert.are.equal(tostring(cyreneBellTower), speedWalkPath[#speedWalkPath])
      assert.is_true(routeIsWalkable(ashtanWorkshop))
      -- how many areas, and which rooms, depend on Qt's per-process hash seed
      -- ordering the graph's vertices (issue #10181), so only the crossing
      -- itself is pinned
      assert.is_true(areasVisited() > 1)
      assert.are_not.equal(getRoomArea(ashtanWorkshop), getRoomArea(cyreneBellTower))
    end)

    it("walks the map's long diagonal", function()
      local ok, weight = getPath(riparium, blackrock)
      assert.is_true(ok)
      assert.are.equal(186, weight)
      assert.are.equal(186, #speedWalkDir)
      assert.is_true(routeIsWalkable(riparium))
    end)

    it("reports no route between two rooms the game only joins by ship", function()
      -- both are ordinary city rooms; MMP has no way to spell the sailing that
      -- connects them, so the imported graph really is in separate pieces
      local ok, weight, message = getPath(mhaldorRoom, eleusisRoom)
      assert.is_false(ok)
      assert.are.equal(-1, weight)
      assert.is_string(message)
    end)

    it("uses a one-way exit in its own direction only", function()
      assert.are.equal(cliffFoot, getRoomExits(cliffTop)["down"])
      assert.is_nil(getRoomExits(cliffFoot)["up"])

      assert.is_true(getPath(cliffTop, cliffFoot))
      assert.are.same({"down"}, speedWalkDir)

      -- the way back exists but has to go the long way round
      assert.is_true(getPath(cliffFoot, cliffTop))
      assert.are.equal(6, #speedWalkDir)
      assert.is_true(routeIsWalkable(cliffFoot))
    end)

    it("routes through a hidden exit, which imports as a locked door", function()
      -- a door is a description of the exit, not a block on it: only lockExit
      -- keeps a route out, which is worth pinning because door type 3 reads
      -- like it would do the same
      assert.are.equal(3, getDoors(hillside)["e"])
      assert.is_true(getPath(hillside, nimick))
      assert.are.same({"e"}, speedWalkDir)
    end)

    it("takes the long way round once that exit is locked", function()
      lockExit(hillside, "east", true)
      local ok, weight = getPath(hillside, nimick)
      assert.is_true(ok)
      assert.are.equal(nimickDetour, weight)
      assert.are.equal(nimickDetour, #speedWalkDir)
      assert.is_true(routeIsWalkable(hillside))
    end)

    it("takes an exit weight over the detour only once it costs more than one", function()
      -- MMP carries no weights, so both routes cost their step count until a
      -- weight is set here, which is the only way to reach the comparison
      setExitWeight(hillside, "east", nimickDetour - 23)
      local ok, weight = getPath(hillside, nimick)
      assert.is_true(ok)
      assert.are.equal(nimickDetour - 23, weight)
      assert.are.same({"e"}, speedWalkDir)

      setExitWeight(hillside, "east", nimickDetour + 57)
      ok, weight = getPath(hillside, nimick)
      assert.is_true(ok)
      assert.are.equal(nimickDetour, weight)
      assert.are.equal(nimickDetour, #speedWalkDir)
      assert.is_true(routeIsWalkable(hillside))
    end)

    it("charges a room weight for arriving in the room", function()
      setRoomWeight(nimick, 100)
      local ok, weight = getPath(hillside, nimick)
      assert.is_true(ok)
      -- one step still, but it costs what the room asks rather than 1
      assert.are.same({"e"}, speedWalkDir)
      assert.are.equal(100, weight)
    end)

    -- distance_heuristic (src/TAstar.h) returns the euclidean distance between
    -- two rooms' coordinates, but a diagonal exit costs 1 while moving a room
    -- north-east moves sqrt(2) in coordinate space. The heuristic therefore
    -- overestimates, which is what A* is not allowed to do, and the search can
    -- settle for a route a step longer than the shortest one: room 747 to room
    -- 42 comes back 93 steps where 92 exist. Flattening the target area's
    -- coordinates - which makes the heuristic 0 wherever it is consulted -
    -- returns 92, which is how the cause was pinned down. Left unpinned here
    -- because it is a defect to fix rather than behaviour to hold still - every
    -- route this block does pin is the true optimum, checked against a
    -- breadth-first search, so a fix leaves them green.
    pending("routes can come back a step longer than the shortest one - issue #10180")
  end)

  -- setMapPerspective/shiftMapPerspective only exist in a build made with 3D
  -- mapper support, which the CI and release builds are not
  pending("setMapPerspective needs a Mudlet built with the 3D mapper")

  pending("shiftMapPerspective needs a Mudlet built with the 3D mapper")
end)

-- The JSON map format has a writer and a reader of its own, entirely separate
-- from the binary one, and an import always ends by auditing what it read. Like
-- the saveMap block above, this one replaces the whole map, so it saves what it
-- found and hands it back.
describe("Tests saveJsonMap and loadJsonMap", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/mapper_spec_json_backup.dat"
  local jsonPath = getMudletHomeDir() .. "/mapper_spec_roundtrip.json"
  local suffixlessPath = getMudletHomeDir() .. "/mapper_spec_suffixless"
  local notJsonPath = getMudletHomeDir() .. "/mapper_spec_notjson.json"
  -- nothing writes this one: it is the path the reader is asked for to be told
  -- it is not there, so a leftover of any kind would answer a different question
  local absentPath = getMudletHomeDir() .. "/mapper_spec_absent.json"

  local missingRoomId = 990000001
  local roomA, roomB

  local function removeScratchFiles()
    os.remove(jsonPath)
    os.remove(suffixlessPath)
    os.remove(suffixlessPath .. ".json")
    os.remove(notJsonPath)
    os.remove(absentPath)
  end

  setup(function()
    -- the saveJsonMap and loadJsonMap bindings both reach through the mapper
    -- widget, so they fail outright unless one has been built
    openMapWidget("r")
    os.remove(backupPath)
    removeScratchFiles()
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this block replaced could not be put back")
    -- an import shows the mapper wherever it last was, and everything after
    -- this file is entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
    removeScratchFiles()
  end)

  describe("Tests the saveJsonMap and loadJsonMap round-trip", function()
    -- two rooms carrying a value for every kind of room and exit data the JSON
    -- format stores separately, so a round-trip that dropped one shows up
    local function buildMap()
      deleteMap()
      local area = addAreaName("MapperSpecJsonArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      roomB = createRoomID(); addRoom(roomB); setRoomArea(roomB, area)
      setRoomCoordinates(roomA, 0, 0, 0)
      setRoomCoordinates(roomB, 3, -4, 5)
      setRoomName(roomA, "Json Room A")
      setRoomEnv(roomA, 42)
      setRoomWeight(roomA, 7)
      setRoomHidden(roomA, true)
      lockRoom(roomA, true)
      setRoomChar(roomA, "J")
      setRoomCharColor(roomA, 11, 22, 33)
      setRoomBorderColor(roomA, 44, 55, 66)
      setRoomBorderThickness(roomA, 3)
      setRoomIDbyHash(roomA, "mapperSpecJsonHash")
      setRoomUserData(roomA, "json key", "json value")
      setExit(roomA, roomB, "east")
      setExitWeight(roomA, "east", 4)
      lockExit(roomA, "east", true)
      setDoor(roomA, "e", 3)
      addCustomLine(roomA, {{2, 3, 0}, {4, 5, 0}}, "e", "dash dot line", {10, 20, 30}, true)
      addSpecialExit(roomA, roomB, "squeeze through")
      setExitWeight(roomA, "squeeze through", 6)
      lockSpecialExit(roomA, roomB, "squeeze through", true)
      setDoor(roomA, "squeeze through", 1)
      setExitStub(roomA, "north", true)
      lockExit(roomA, "north", true)
    end

    local function roundTrip()
      assert.is_true(saveJsonMap(jsonPath))
      -- wipe the lot, so that an import which did nothing at all cannot pass
      deleteMap()
      assert.is_false(roomExists(roomA))
      assert.is_true(loadJsonMap(jsonPath))
    end

    it("puts every kind of room data back exactly as it was saved", function()
      buildMap()
      roundTrip()

      assert.is_true(roomExists(roomA))
      assert.is_true(roomExists(roomB))
      assert.are.equal("Json Room A", getRoomName(roomA))
      assert.are.same({3, -4, 5}, {getRoomCoordinates(roomB)})
      assert.are.equal("MapperSpecJsonArea", getRoomAreaName(getRoomArea(roomA)))
      assert.are.equal(42, getRoomEnv(roomA))
      assert.are.equal(7, getRoomWeight(roomA))
      assert.is_true(getRoomHidden(roomA))
      assert.is_true(roomLocked(roomA))
      assert.are.equal("J", getRoomChar(roomA))
      assert.are.same({11, 22, 33}, {getRoomCharColor(roomA)})
      -- the alpha is the default 255 because an imported map cannot carry any
      -- other, so this pins the three channels only:
      -- https://github.com/Mudlet/Mudlet/issues/10368
      assert.are.same({44, 55, 66, 255}, {getRoomBorderColor(roomA)})
      assert.are.equal(3, getRoomBorderThickness(roomA))
      assert.are.equal(roomA, getRoomIDbyHash("mapperSpecJsonHash"))
      assert.are.equal("json value", getRoomUserData(roomA, "json key"))
    end)

    it("puts every kind of exit data back exactly as it was saved", function()
      buildMap()
      roundTrip()

      assert.are.equal(roomB, getRoomExits(roomA)["east"])
      assert.are.equal(4, getExitWeights(roomA)["e"])
      assert.is_true(hasExitLock(roomA, "east"))
      assert.are.equal(3, getDoors(roomA)["e"])

      local line = getCustomLines1(roomA)["e"]
      assert.is_table(line)
      assert.are.equal("dash dot line", line.attributes.style)
      assert.is_true(line.attributes.arrow)
      assert.are.same({10, 20, 30}, line.attributes.color)
      assert.are.equal(2, #line.points)
      assert.are.equal(2, line.points[1][1])
      assert.are.equal(3, line.points[1][2])

      assert.are.equal(roomB, getSpecialExitsSwap(roomA)["squeeze through"])
      assert.are.equal(6, getExitWeights(roomA)["squeeze through"])
      assert.is_true(hasSpecialExitLock(roomA, roomB, "squeeze through"))
      assert.are.equal(1, getDoors(roomA)["squeeze through"])

      assert.are.same({1}, getExitStubs1(roomA))
      assert.is_true(hasExitLock(roomA, "north"))
    end)

    it("puts an exit back in each of the twelve directions", function()
      local directions = {"north", "northeast", "northwest", "east", "west", "south",
                          "southeast", "southwest", "up", "down", "in", "out"}
      deleteMap()
      local area = addAreaName("MapperSpecJsonDirectionArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      -- a destination of its own per direction: pointing all twelve at one room
      -- would pin the set of directions but not the mapping, so a reader that
      -- swapped north for south would still pass
      local destinations = {}
      for _, direction in ipairs(directions) do
        local destination = createRoomID(); addRoom(destination); setRoomArea(destination, area)
        destinations[direction] = destination
        assert.is_true(setExit(roomA, destination, direction))
      end

      roundTrip()

      local exits = getRoomExits(roomA)
      for _, direction in ipairs(directions) do
        assert.are.equal(destinations[direction], exits[direction], direction)
      end
    end)

    it("puts every custom exit line style back", function()
      local styles = {"solid line", "dot line", "dash line", "dash dot line", "dash dot dot line"}
      -- one direction per style, so all five travel in the same file
      local directions = {"north", "east", "south", "west", "up"}
      local shortDirections = {"n", "e", "s", "w", "up"}

      deleteMap()
      local area = addAreaName("MapperSpecJsonStyleArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      roomB = createRoomID(); addRoom(roomB); setRoomArea(roomB, area)
      for index, style in ipairs(styles) do
        assert.is_true(setExit(roomA, roomB, directions[index]))
        assert.is_true(addCustomLine(roomA, {{index, index, 0}}, shortDirections[index], style, {1, 2, 3}, index % 2 == 0))
      end

      roundTrip()

      local lines = getCustomLines1(roomA)
      for index, style in ipairs(styles) do
        local line = lines[shortDirections[index]]
        assert.is_table(line, style)
        assert.are.equal(style, line.attributes.style)
        assert.are.equal(index % 2 == 0, line.attributes.arrow, style)
      end
    end)

    it("puts every door state back on a normal and on a special exit", function()
      deleteMap()
      local area = addAreaName("MapperSpecJsonDoorArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      roomB = createRoomID(); addRoom(roomB); setRoomArea(roomB, area)
      setExit(roomA, roomB, "north")
      setExit(roomA, roomB, "east")
      setExit(roomA, roomB, "south")
      setDoor(roomA, "n", 1)
      setDoor(roomA, "e", 2)
      setDoor(roomA, "s", 3)
      addSpecialExit(roomA, roomB, "wriggle")
      setDoor(roomA, "wriggle", 2)

      roundTrip()

      local doors = getDoors(roomA)
      assert.are.equal(1, doors["n"])
      assert.are.equal(2, doors["e"])
      assert.are.equal(3, doors["s"])
      assert.are.equal(2, doors["wriggle"])
    end)

    it("keeps a stub exit's door where the long and short direction names match", function()
      deleteMap()
      local area = addAreaName("MapperSpecJsonStubDoorArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      setExitStub(roomA, "up", true)
      setDoor(roomA, "up", 2)

      roundTrip()

      -- the eight compass directions lose this:
      -- https://github.com/Mudlet/Mudlet/issues/10369
      assert.are.equal(2, getDoors(roomA)["up"])
    end)

    it("puts a custom environment colour back", function()
      deleteMap()
      local area = addAreaName("MapperSpecJsonEnvArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      setRoomEnv(roomA, 501)
      -- an ID outside 257-272, which would sync to the profile's ANSI colours
      -- and survive the deleteMap inside roundTrip
      setCustomEnvColor(501, 10, 20, 30, 255)

      roundTrip()

      assert.are.same({10, 20, 30, 255}, getCustomEnvColorTable()[501])
    end)

    it("puts a symbol font scaling below one back, rather than rounding it away", function()
      deleteMap()
      local area = addAreaName("MapperSpecJsonScalingArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      local savedScaling = getConfig("mapSymbolFontScaling")
      finally(function() setConfig("mapSymbolFontScaling", savedScaling) end)

      -- the scaling is not room or area data, so deleteMap leaves it alone and
      -- roundTrip cannot show it moving. Moving it somewhere else between the
      -- two halves is what makes the value that comes back the file's own.
      setConfig("mapSymbolFontScaling", 0.5)
      assert.is_true(saveJsonMap(jsonPath))
      setConfig("mapSymbolFontScaling", 2.0)
      assert.is_true(loadJsonMap(jsonPath))

      assert.are.equal(0.5, getConfig("mapSymbolFontScaling"))
    end)

    -- TMap::readJsonColor returns QColor(red, green, blue) for a colour array of
    -- either three or four values, so the alpha the exporter wrote as
    -- "color32RGBA" never reaches the QColor and comes back as 255. Every colour
    -- read through that function flattens the same way, custom environment
    -- colours included, so the test above can only use an opaque one.
    -- https://github.com/Mudlet/Mudlet/issues/10368
    pending("a translucent room border colour loses its alpha on import")

    -- TRoom::writeJsonExitStubs looks the stub's door up under the long
    -- direction name while TRoom::doors is keyed by the short one, so a door on
    -- a stub survives only for up, down, in and out, whose two spellings match
    -- https://github.com/Mudlet/Mudlet/issues/10369
    pending("a stub exit's door is dropped on export in the eight compass directions")

    -- TMap::readJsonUserData inserts each key it reads into the live map's user
    -- data without emptying it first, and the JSON import never reaches the
    -- clear a binary load gets, so keys the previous map had outlive it.
    pending("map user data from the map being replaced survives a JSON import")
  end)

  describe("Tests the saveJsonMap and loadJsonMap argument contract", function()
    it("appends the .json suffix to a destination that lacks one", function()
      assert.is_true(saveJsonMap(suffixlessPath))
      local written = io.open(suffixlessPath .. ".json", "r")
      assert.is_not_nil(written, "saveJsonMap should have added the suffix itself")
      written:close()
      assert.is_nil(io.open(suffixlessPath, "r"))
    end)

    it("reports failure rather than raising when the file cannot be written", function()
      local ok, err = saveJsonMap("/nosuchdirectory/mapper_spec.json")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("hard-errors on a destination that is not a string", function()
      local ok, err = pcall(saveJsonMap, {})
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("saveJsonMap: bad argument #1 type", 1, true), tostring(err))
    end)

    it("returns nil and a message for an empty import path", function()
      local ok, err = loadJsonMap("")
      assert.is_nil(ok)
      assert.is_truthy(err:find("a non-empty path and file name", 1, true), err)
    end)

    it("returns nil and a message for a file that is not there", function()
      local ok, err = loadJsonMap(absentPath)
      assert.is_nil(ok)
      assert.is_truthy(err:find("could not open file", 1, true), err)
    end)

    it("returns nil and a message for a file that is not JSON at all", function()
      local file = assert(io.open(notJsonPath, "w"))
      file:write("this is not JSON")
      file:close()
      local ok, err = loadJsonMap(notJsonPath)
      assert.is_nil(ok)
      assert.is_truthy(err:find("could not parse file", 1, true), err)
    end)

    it("returns nil and a message for JSON that carries no format version", function()
      local file = assert(io.open(notJsonPath, "w"))
      file:write('{"areas": []}')
      file:close()
      local ok, err = loadJsonMap(notJsonPath)
      assert.is_nil(ok)
      assert.is_truthy(err:find("no format version detected", 1, true), err)
    end)

    it("returns nil and a message for a format version it cannot read", function()
      local file = assert(io.open(notJsonPath, "w"))
      file:write('{"formatVersion": 2, "areas": []}')
      file:close()
      local ok, err = loadJsonMap(notJsonPath)
      assert.is_nil(ok)
      assert.is_truthy(err:find("invalid format version \"2.000\" detected", 1, true), err)
    end)

    it("returns nil and a message for JSON that holds no areas", function()
      local file = assert(io.open(notJsonPath, "w"))
      file:write('{"formatVersion": 1}')
      file:close()
      local ok, err = loadJsonMap(notJsonPath)
      assert.is_nil(ok)
      assert.is_truthy(err:find("no areas detected", 1, true), err)
    end)
  end)

  -- Every import ends in an audit that repairs data no mapper call can produce,
  -- so these plant the fault into an exported file and read it back in. The last
  -- one plants valid data rather than a fault, because the export cannot write a
  -- compass stub's door at all: https://github.com/Mudlet/Mudlet/issues/10369
  describe("Tests the audit of an imported JSON map", function()
    local function buildMap()
      deleteMap()
      local area = addAreaName("MapperSpecJsonAuditArea")
      roomA = createRoomID(); addRoom(roomA); setRoomArea(roomA, area)
      roomB = createRoomID(); addRoom(roomB); setRoomArea(roomB, area)
      setExit(roomA, roomB, "east")
      setExit(roomA, roomB, "west")
      addSpecialExit(roomA, roomB, "squeeze through")
      setExitStub(roomA, "north", true)
    end

    local function findRoom(document, roomId)
      for _, area in ipairs(document.areas) do
        for _, room in ipairs(area.rooms or {}) do
          if room.id == roomId then
            return room
          end
        end
      end
      error("room " .. tostring(roomId) .. " is not in the exported document")
    end

    local function findExit(room, name)
      for _, exit in ipairs(room.exits or {}) do
        if exit.name == name then
          return exit
        end
      end
      error('the exported room has no exit named "' .. tostring(name) .. '"')
    end

    local function reimportWith(plantFault)
      assert.is_true(saveJsonMap(jsonPath))
      local file = assert(io.open(jsonPath, "r"))
      local document = yajl.to_value(file:read("*a"))
      file:close()
      plantFault(document)
      file = assert(io.open(jsonPath, "w"))
      file:write(yajl.to_string(document))
      file:close()

      deleteMap()
      assert.is_false(roomExists(roomA))
      assert.is_true(loadJsonMap(jsonPath))
    end

    it("turns an exit to a room that is not in the file into a stub", function()
      buildMap()
      reimportWith(function(document)
        findExit(findRoom(document, roomA), "east").exitId = missingRoomId
      end)

      assert.is_nil(getRoomExits(roomA)["east"])
      -- the west exit was left alone, so the east one went for its own reason
      assert.are.equal(roomB, getRoomExits(roomA)["west"])
      local stubs = {}
      for _, code in pairs(getExitStubs1(roomA)) do stubs[code] = true end
      assert.is_true(stubs[4]) -- DIR_EAST
      assert.are.equal(tostring(missingRoomId), getRoomUserData(roomA, "audit.made_stub_of_valid_but_missing_exit.4"))
    end)

    it("removes a special exit to a room that is not in the file", function()
      buildMap()
      reimportWith(function(document)
        findExit(findRoom(document, roomA), "squeeze through").exitId = missingRoomId
      end)

      assert.is_nil(getSpecialExitsSwap(roomA)["squeeze through"])
      assert.are.equal(roomB, getRoomExits(roomA)["east"])
      assert.are.equal(tostring(missingRoomId),
                       getRoomUserData(roomA, "audit.removed_valid_but_missing_special_exit.squeeze through"))
    end)

    it("drops a stub that stands in the same direction as a real exit", function()
      buildMap()
      reimportWith(function(document)
        local room = findRoom(document, roomA)
        room.stubExits[#room.stubExits + 1] = {name = "east"}
        room.stubExits[#room.stubExits + 1] = {name = "south"}
      end)

      local stubs = {}
      for _, code in pairs(getExitStubs1(roomA)) do stubs[code] = true end
      assert.are.equal(roomB, getRoomExits(roomA)["east"])
      assert.is_nil(stubs[4]) -- DIR_EAST, dropped because the exit outranks it
      -- the south stub has no exit to give way to, so it is the control
      assert.is_true(stubs[6]) -- DIR_SOUTH
      assert.is_true(stubs[1]) -- DIR_NORTH
    end)

    it("ignores a stub exit named as a special exit command", function()
      buildMap()
      reimportWith(function(document)
        local room = findRoom(document, roomA)
        room.stubExits[#room.stubExits + 1] = {name = "squeeze through"}
      end)

      -- a stub in a direction Mudlet has no code for is skipped rather than
      -- stored as DIR_OTHER
      assert.are.same({1}, getExitStubs1(roomA))
      assert.are.equal(roomB, getSpecialExitsSwap(roomA)["squeeze through"])
    end)

    it("keeps a door and a lock that a stub exit carries", function()
      buildMap()
      reimportWith(function(document)
        local room = findRoom(document, roomA)
        room.stubExits[1].door = "closed"
        room.stubExits[1].locked = true
      end)

      assert.are.same({1}, getExitStubs1(roomA))
      assert.are.equal(2, getDoors(roomA)["n"])
      assert.is_true(hasExitLock(roomA, "north"))
    end)
  end)
end)
