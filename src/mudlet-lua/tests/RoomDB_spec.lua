-- The map's data layer rather than its file formats: the room and area tables,
-- the exit members of a room and the per-area indexes that follow them around.
-- Every block builds and drops its own areas so the map handed to the rest of
-- the suite comes out as it went in.

describe("Tests the room and area database behind the map", function()

  local missingRoomId = 991000001

  local areaHome, areaAway
  local rHome1, rHome2, rHome3, rAway1

  local function makeRoom(area, x, y, z)
    local id = createRoomID()
    assert.is_true(addRoom(id))
    assert.is_true(setRoomArea(id, area))
    setRoomCoordinates(id, x, y, z)
    return id
  end

  local function listHas(list, wanted)
    for _, value in pairs(list) do
      if value == wanted then
        return true
      end
    end
    return false
  end

  local function countKeys(t)
    local n = 0
    for _ in pairs(t) do
      n = n + 1
    end
    return n
  end

  setup(function()
    areaHome = addAreaName("RoomDBSpecHome")
    areaAway = addAreaName("RoomDBSpecAway")

    rHome1 = makeRoom(areaHome, 0, 0, 0)
    rHome2 = makeRoom(areaHome, 1, 0, 0)
    rHome3 = makeRoom(areaHome, 2, 0, 0)
    rAway1 = makeRoom(areaAway, 0, 0, 7)

    setExit(rHome1, rHome2, "east"); setExit(rHome2, rHome1, "west")
    setExit(rHome2, rHome3, "east"); setExit(rHome3, rHome2, "west")
  end)

  teardown(function()
    for _, id in ipairs({rHome1, rHome2, rHome3, rAway1}) do
      deleteRoom(id)
    end
    deleteArea("RoomDBSpecHome")
    deleteArea("RoomDBSpecAway")
  end)

  describe("Tests deleting an area that still holds rooms", function()
    it("deleteArea takes the area's rooms and every exit leading into them", function()
      -- more than one room in the area, which is what puts the room database
      -- on its bulk-deletion path rather than the single-room one deleteRoom uses
      local area = addAreaName("RoomDBSpecDoomed")
      local doomedA = makeRoom(area, 0, 0, 8)
      local doomedB = makeRoom(area, 1, 0, 8)
      local outside = makeRoom(areaHome, 5, 5, 0)
      finally(function()
        deleteRoom(outside); deleteRoom(doomedA); deleteRoom(doomedB)
        deleteArea("RoomDBSpecDoomed")
      end)

      setExit(outside, doomedA, "north")
      addSpecialExit(outside, doomedB, "slip inside")
      setExit(doomedA, doomedB, "east")
      assert.are.equal(doomedA, getRoomExits(outside)["north"])
      assert.are.equal(doomedB, getSpecialExitsSwap(outside)["slip inside"])

      assert.is_true(deleteArea(area))

      assert.is_false(roomExists(doomedA))
      assert.is_false(roomExists(doomedB))
      assert.is_nil(getAreaTable()["RoomDBSpecDoomed"])
      assert.is_nil(getRoomExits(outside)["north"])
      assert.is_nil(getSpecialExitsSwap(outside)["slip inside"])
    end)

    it("an area outlives the last of its rooms", function()
      local area = addAreaName("RoomDBSpecEmptied")
      local only = makeRoom(area, 0, 0, 9)
      finally(function() deleteRoom(only); deleteArea("RoomDBSpecEmptied") end)

      assert.are.same({only}, getAreaRooms1(area))
      assert.is_true(deleteRoom(only))
      assert.are.equal(area, getAreaTable()["RoomDBSpecEmptied"])
      assert.are.same({}, getAreaRooms1(area))
    end)

    it("deleting an area hands its ID back to the next area created", function()
      -- an area above the freed one, or handing out one past the highest ID in
      -- use would answer this just as well as reusing the hole
      local recycled = addAreaName("RoomDBSpecRecycled")
      local above = addAreaName("RoomDBSpecAbove")
      finally(function()
        deleteArea("RoomDBSpecRecycled")
        deleteArea("RoomDBSpecAbove")
        deleteArea("RoomDBSpecReused")
      end)
      assert.is_true(above > recycled)

      assert.is_true(deleteArea(recycled))
      assert.are.equal(recycled, addAreaName("RoomDBSpecReused"))
    end)
  end)

  describe("Tests room ID allocation", function()
    it("addRoom refuses an ID that is taken and leaves the room holding it alone", function()
      local id = createRoomID()
      assert.is_true(addRoom(id))
      finally(function() deleteRoom(id) end)

      setRoomName(id, "RoomDBSpec original")
      assert.is_false(addRoom(id))
      assert.are.equal("RoomDBSpec original", getRoomName(id))
    end)

    it("createRoomID hands back the number of a room that has been deleted", function()
      -- a room above the freed one, or handing out one past the highest number
      -- in use would answer this just as well as reusing the hole
      local recycled = createRoomID()
      assert.is_true(addRoom(recycled))
      local above = createRoomID()
      assert.is_true(addRoom(above))
      finally(function() deleteRoom(recycled); deleteRoom(above) end)
      assert.is_true(above > recycled)

      assert.are_not.equal(recycled, createRoomID())

      assert.is_true(deleteRoom(recycled))
      assert.are.equal(recycled, createRoomID())
    end)

    it("createRoomID answers a minimum with the first free number at or above it", function()
      local low = createRoomID()
      assert.is_true(addRoom(low))
      local middle = createRoomID()
      assert.is_true(addRoom(middle))
      local high = createRoomID()
      assert.is_true(addRoom(high))
      finally(function() deleteRoom(low); deleteRoom(middle); deleteRoom(high) end)
      assert.is_true(low < middle and middle < high)

      -- the hole, not one past the highest number in use
      assert.is_true(deleteRoom(middle))
      assert.are.equal(middle, createRoomID(low))

      -- and the hole at or above the minimum, not the one below it
      assert.is_true(deleteRoom(low))
      assert.are.equal(middle, createRoomID(middle))

      assert.are.equal(missingRoomId, createRoomID(missingRoomId))
    end)

    it("createRoomID rejects a minimum below one", function()
      local ok, err = createRoomID(0)
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests moving a room between areas", function()
    it("setRoomArea takes the room out of the room list of the area it left", function()
      local mover = makeRoom(areaHome, 7, 7, 0)
      finally(function() deleteRoom(mover) end)

      assert.is_true(listHas(getAreaRooms1(areaHome), mover))
      assert.is_false(listHas(getAreaRooms1(areaAway), mover))

      assert.is_true(setRoomArea(mover, areaAway))

      assert.is_false(listHas(getAreaRooms1(areaHome), mover))
      assert.is_true(listHas(getAreaRooms1(areaAway), mover))
    end)

    it("resetRoomArea hands the room over to the default area", function()
      local mover = makeRoom(areaHome, 8, 8, 0)
      finally(function() deleteRoom(mover) end)

      assert.is_true(listHas(getAreaRooms1(areaHome), mover))
      assert.is_false(listHas(getAreaRooms1(-1), mover))

      assert.is_true(resetRoomArea(mover))

      assert.is_false(listHas(getAreaRooms1(areaHome), mover))
      assert.is_true(listHas(getAreaRooms1(-1), mover))
    end)

    it("setRoomArea to the area a room is already in leaves it where it was", function()
      -- the move drops the room from its old area before adding it to the new
      -- one, and here those are the same area's indexes
      local stayer = makeRoom(areaHome, 6, 6, 0)
      finally(function() deleteRoom(stayer) end)

      assert.is_true(setRoomArea(stayer, areaHome))

      assert.is_true(listHas(getAreaRooms1(areaHome), stayer))
      assert.are.same({stayer}, getRoomsByPosition1(areaHome, 6, 6, 0))
    end)

    it("a room leaving an area turns its exits home into exits out of that area", function()
      local mover = makeRoom(areaHome, 3, 0, 0)
      finally(function()
        setExit(rHome3, -1, "east")
        deleteRoom(mover)
      end)

      setExit(rHome3, mover, "east"); setExit(mover, rHome3, "west")
      assert.is_false(listHas(getAreaExits(areaHome), rHome3))

      assert.is_true(setRoomArea(mover, areaAway))

      assert.is_true(listHas(getAreaExits(areaHome), rHome3))
      assert.is_true(listHas(getAreaExits(areaAway), mover))
    end)

    it("deleting a room drops it from its area's exit list", function()
      local leaver = makeRoom(areaAway, 5, 5, 7)
      finally(function() deleteRoom(leaver) end)

      setExit(leaver, rHome1, "up")
      assert.is_true(listHas(getAreaExits(areaAway), leaver))

      assert.is_true(deleteRoom(leaver))
      assert.is_false(listHas(getAreaExits(areaAway), leaver))
    end)
  end)

  describe("Tests room positions within an area", function()
    it("getRoomsByPosition1 lists every room stacked on one coordinate in ID order", function()
      -- more than a pair, because an area's room set iterates a pair in ID
      -- order of its own accord and so cannot show the sorting happening
      local area = addAreaName("RoomDBSpecStack")
      local stack = {}
      finally(function()
        for _, id in ipairs(stack) do
          deleteRoom(id)
        end
        deleteArea("RoomDBSpecStack")
      end)
      for _ = 1, 4 do
        stack[#stack + 1] = makeRoom(area, 4, 4, 0)
      end

      local ascending = {}
      for index, id in ipairs(stack) do
        ascending[index] = id
      end
      table.sort(ascending)
      assert.are.same(ascending, getRoomsByPosition1(area, 4, 4, 0))
    end)

    it("a room moved by setRoomCoordinates leaves the position it was at", function()
      local area = addAreaName("RoomDBSpecMoved")
      local mover = makeRoom(area, 1, 1, 0)
      finally(function() deleteRoom(mover); deleteArea("RoomDBSpecMoved") end)

      assert.are.same({mover}, getRoomsByPosition1(area, 1, 1, 0))
      assert.is_true(setRoomCoordinates(mover, 2, 2, 3))

      assert.are.same({}, getRoomsByPosition1(area, 1, 1, 0))
      assert.are.same({mover}, getRoomsByPosition1(area, 2, 2, 3))
    end)

    it("a room moved to another area is found by position only in the new one", function()
      local mover = makeRoom(areaHome, 9, 9, 0)
      finally(function() deleteRoom(mover) end)

      assert.are.same({mover}, getRoomsByPosition1(areaHome, 9, 9, 0))
      assert.is_true(setRoomArea(mover, areaAway))

      assert.are.same({}, getRoomsByPosition1(areaHome, 9, 9, 0))
      assert.are.same({mover}, getRoomsByPosition1(areaAway, 9, 9, 0))
    end)
  end)

  describe("Tests exits that are taken away again", function()
    it("setExit with a roomID below one removes the exit and the entrance it made", function()
      local a = makeRoom(areaHome, 10, 0, 0)
      local b = makeRoom(areaHome, 11, 0, 0)
      finally(function() deleteRoom(a); deleteRoom(b) end)

      assert.is_true(setExit(a, b, "east"))
      assert.are.equal(b, getRoomExits(a)["east"])
      assert.is_true(listHas(getAllRoomEntrances(b), a))

      assert.is_true(setExit(a, -1, "east"))

      assert.is_nil(getRoomExits(a)["east"])
      assert.is_false(listHas(getAllRoomEntrances(b), a))
    end)

    it("setting a real exit clears the stub standing in its direction", function()
      local a = makeRoom(areaHome, 12, 0, 0)
      local b = makeRoom(areaHome, 13, 0, 0)
      finally(function() deleteRoom(a); deleteRoom(b) end)

      setExitStub(a, "east", true)
      assert.are.same({4}, getExitStubs1(a))

      assert.is_true(setExit(a, b, "east"))
      assert.are.same({}, getExitStubs1(a))
    end)

    it("setExitStub refuses a direction that already has an exit", function()
      local a = makeRoom(areaHome, 14, 0, 0)
      local b = makeRoom(areaHome, 15, 0, 0)
      finally(function() deleteRoom(a); deleteRoom(b) end)

      assert.is_true(setExit(a, b, "east"))
      setExitStub(a, "east", true)
      setExitStub(a, "west", true)
      -- the free direction shows a stub arrived, so the east one being missing
      -- is the exit turning it away rather than nothing having been asked for
      assert.are.same({5}, getExitStubs1(a))
    end)

    it("setExit refuses a destination room that is not there", function()
      local a = makeRoom(areaHome, 16, 0, 0)
      finally(function() deleteRoom(a) end)

      assert.is_false(setExit(a, missingRoomId, "east"))
      assert.is_nil(getRoomExits(a)["east"])
    end)
  end)

  describe("Tests special exit bookkeeping", function()
    it("addSpecialExit re-points a command that is already in use", function()
      local from = makeRoom(areaHome, 17, 0, 0)
      local was = makeRoom(areaHome, 18, 0, 0)
      local now = makeRoom(areaHome, 19, 0, 0)
      finally(function() deleteRoom(from); deleteRoom(was); deleteRoom(now) end)

      assert.is_true(addSpecialExit(from, was, "portal"))
      assert.are.equal(was, getSpecialExitsSwap(from)["portal"])

      assert.is_true(addSpecialExit(from, now, "portal"))

      local exits = getSpecialExitsSwap(from)
      assert.are.equal(now, exits["portal"])
      assert.are.equal(1, countKeys(exits))
      assert.is_false(listHas(getAllRoomEntrances(was), from))
      assert.is_true(listHas(getAllRoomEntrances(now), from))
    end)

    it("removeSpecialExit takes the door, the lock and the weight the command carried", function()
      local from = makeRoom(areaHome, 20, 0, 0)
      local to = makeRoom(areaHome, 21, 0, 0)
      finally(function() deleteRoom(from); deleteRoom(to) end)

      assert.is_true(addSpecialExit(from, to, "wriggle"))
      assert.is_true(setDoor(from, "wriggle", 2))
      assert.is_true(lockSpecialExit(from, to, "wriggle", true))
      assert.is_true(setExitWeight(from, "wriggle", 9))

      assert.is_true(removeSpecialExit(from, "wriggle"))

      assert.is_nil(getDoors(from)["wriggle"])
      assert.is_nil(getExitWeights(from)["wriggle"])
      -- the lock went with the command, so putting it back leaves it open
      assert.is_true(addSpecialExit(from, to, "wriggle"))
      assert.is_false(hasSpecialExitLock(from, to, "wriggle"))
    end)

    it("clearSpecialExits takes every command's door and lock with it", function()
      local from = makeRoom(areaHome, 22, 0, 0)
      local to = makeRoom(areaHome, 23, 0, 0)
      finally(function() deleteRoom(from); deleteRoom(to) end)

      assert.is_true(addSpecialExit(from, to, "one"))
      assert.is_true(addSpecialExit(from, to, "two"))
      assert.is_true(setDoor(from, "one", 3))
      assert.is_true(lockSpecialExit(from, to, "two", true))

      clearSpecialExits(from)

      assert.are.same({}, getSpecialExitsSwap(from))
      assert.is_nil(getDoors(from)["one"])
      assert.is_true(addSpecialExit(from, to, "two"))
      assert.is_false(hasSpecialExitLock(from, to, "two"))
    end)
  end)

  describe("Tests room and exit weights", function()
    it("a room weight below one is clamped to one", function()
      local id = makeRoom(areaHome, 24, 0, 0)
      finally(function() deleteRoom(id) end)

      assert.is_true(setRoomWeight(id, 5))
      assert.are.equal(5, getRoomWeight(id))

      assert.is_true(setRoomWeight(id, 0))
      assert.are.equal(1, getRoomWeight(id))

      assert.is_true(setRoomWeight(id, -8))
      assert.are.equal(1, getRoomWeight(id))
    end)

    it("setExitWeight recognises an exit in each of the twelve directions", function()
      -- the room stores every direction in a member of its own and the lookup
      -- that decides whether a direction has an exit spells all twelve out, so
      -- one weight per direction is what catches a direction wired to the wrong
      -- member
      local hub = makeRoom(areaHome, 40, 0, 0)
      local far = makeRoom(areaHome, 41, 0, 0)
      finally(function() deleteRoom(hub); deleteRoom(far) end)

      local directions = {
        {"north", "n"}, {"northeast", "ne"}, {"northwest", "nw"}, {"east", "e"},
        {"west", "w"}, {"south", "s"}, {"southeast", "se"}, {"southwest", "sw"},
        {"up", "up"}, {"down", "down"}, {"in", "in"}, {"out", "out"},
      }
      for index, pair in ipairs(directions) do
        assert.is_true(setExit(hub, far, pair[1]), pair[1])
        assert.is_true(setExitWeight(hub, pair[1], index), pair[1])
      end

      local weights = getExitWeights(hub)
      for index, pair in ipairs(directions) do
        assert.are.equal(index, weights[pair[2]], pair[2])
      end
    end)

    it("an exit weight stands in for the weight of the room it leads to", function()
      -- one route only, so the cost getPath reports is the cost of these two
      -- steps rather than a choice between routes
      local start = makeRoom(areaHome, 30, 0, 0)
      local heavy = makeRoom(areaHome, 31, 0, 0)
      local goal = makeRoom(areaHome, 32, 0, 0)
      finally(function() deleteRoom(start); deleteRoom(heavy); deleteRoom(goal) end)

      setExit(start, heavy, "east")
      setExit(heavy, goal, "east")
      assert.is_true(setRoomWeight(heavy, 50))

      local ok, cost = getPath(start, goal)
      assert.is_true(ok)
      assert.are.equal(51, cost)

      assert.is_true(setExitWeight(start, "east", 3))
      ok, cost = getPath(start, goal)
      assert.is_true(ok)
      assert.are.equal(4, cost)

      -- zero drops the exit weight rather than storing it, so the room's own
      -- weight is what the step costs again
      assert.is_true(setExitWeight(start, "east", 0))
      assert.is_nil(getExitWeights(start)["e"])
      ok, cost = getPath(start, goal)
      assert.is_true(ok)
      assert.are.equal(51, cost)
    end)
  end)
end)
