-- The audit that runs when a binary map file is loaded, and repairs what it
-- finds wrong with it. Nothing the Lua API offers can build a map that is
-- damaged in most of these ways, so those tests save a sound map, alter bytes
-- of the file it wrote in one planted place, and load the result back. The planting
-- helpers count what they replaced, so a change to the file format that moves
-- or renames what they look for fails loudly rather than loading an undamaged
-- map that the assertions would then be checking for nothing.

describe("Tests the audit of a damaged binary map file", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/mapfileaudit_spec_backup.dat"
  local damagedPath = mapDirectory .. "/mapfileaudit_spec_damaged.dat"

  -- well clear of anything createRoomID() or addAreaName() hands out, and with
  -- no zero byte in them, so each one appears in the file only where it is used
  local distantRoomId = 0x5A5B5C01
  local otherDistantRoomId = 0x5A5B5C02
  local distantAreaId = 0x5A5B5C21

  -- QDataStream writes an int as four bytes, most significant first
  local function int32(value)
    if value < 0 then
      value = value + 2 ^ 32
    end
    local bytes = {}
    for i = 4, 1, -1 do
      bytes[i] = string.char(value % 256)
      value = math.floor(value / 256)
    end
    return table.concat(bytes)
  end

  -- and a QString as its length in bytes followed by UTF-16BE code units,
  -- which for the ASCII used here is a zero byte before each character
  local function qstring(text)
    return int32(#text * 2) .. text:gsub(".", "\0%0")
  end

  local minusOne = int32(-1)

  local function readFile(path)
    local file = assert(io.open(path, "rb"))
    local data = file:read("*a")
    file:close()
    return data
  end

  local function writeFile(path, data)
    local file = assert(io.open(path, "wb"))
    file:write(data)
    file:close()
  end

  -- plain find, which unlike the pattern functions copes with zero bytes
  local function replaceBytes(data, from, to, limit)
    local parts, position, count = {}, 1, 0
    while not limit or count < limit do
      local first, last = data:find(from, position, true)
      if not first then
        break
      end
      parts[#parts + 1] = data:sub(position, first - 1)
      parts[#parts + 1] = to
      position = last + 1
      count = count + 1
    end
    parts[#parts + 1] = data:sub(position)
    return table.concat(parts), count
  end

  local function countBytes(data, wanted)
    local _, count = replaceBytes(data, wanted, wanted)
    return count
  end

  -- saves the map as it stands, hands the file's bytes to plantFault, which
  -- returns them altered, and loads the result back over a wiped map; a test
  -- whose fault depends on how one format version lays the data out names
  -- that version, so a change of the default format cannot move it
  local function reloadWith(plantFault, version)
    assert.is_true(saveMap(damagedPath, version or 0))
    writeFile(damagedPath, plantFault(readFile(damagedPath)))
    deleteMap()
    assert.is_true(loadMap(damagedPath))
  end

  local function planted(data, from, to, expected, limit)
    local result, count = replaceBytes(data, from, to, limit)
    assert.are.equal(expected, count, "the fault was not planted where the file format was expected to have it")
    return result
  end

  local function newArea(name)
    local area = addAreaName(name)
    assert.is_true(area > 0)
    return area
  end

  local function newRoom(area, x, id)
    id = id or createRoomID()
    assert.is_true(addRoom(id))
    assert.is_true(setRoomArea(id, area))
    setRoomCoordinates(id, x, 0, 0)
    return id
  end

  local function listHas(list, wanted)
    for _, value in pairs(list or {}) do
      if value == wanted then
        return true
      end
    end
    return false
  end

  setup(function()
    os.remove(backupPath)
    os.remove(damagedPath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  -- saveMap() leaves temporary map labels out, so any the map had when this
  -- file started are gone once it is put back; nothing after it relies on one
  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(damagedPath)
  end)

  before_each(function()
    deleteMap()
  end)

  describe("Tests exits that lead to a room ID below one", function()
    it("turns a normal exit into a stub and keeps what a stub can carry", function()
      local area = newArea("MapFileAuditSpecExits")
      local from = newRoom(area, 0)
      local to = newRoom(area, 1, distantRoomId)
      assert.is_true(setExit(from, to, "east"))
      assert.is_true(setExitWeight(from, "east", 5))
      assert.is_true(setDoor(from, "e", 2))
      assert.is_true(addCustomLine(from, {{0.5, 0.5, 0}}, "e", "dot line", {1, 2, 3}, false))
      -- the control: an exit that stays sound
      assert.is_true(setExit(to, from, "west"))

      reloadWith(function(data)
        -- a room writes its twelve exits in order north, northeast, east,
        -- southeast, ..., so the east exit is the one after two absent ones
        return planted(data, minusOne .. minusOne .. int32(to) .. minusOne,
                       minusOne .. minusOne .. int32(-3) .. minusOne, 1)
      end)

      assert.is_nil(getRoomExits(from)["east"])
      assert.is_true(listHas(getExitStubs1(from), 4)) -- DIR_EAST
      assert.are.equal("-3", getRoomUserData(from, "audit.made_stub_of_invalid_exit.4"))
      -- a stub cannot be weighted, so the weight is only kept as a note
      assert.is_nil(getExitWeights(from)["e"])
      assert.are.equal("5", getRoomUserData(from, "audit.invalid_exit.4.weight"))
      assert.is_nil((getCustomLines1(from) or {})["e"])
      -- but a stub can have a door
      assert.are.equal(2, getDoors(from)["e"])
      assert.are.equal(from, getRoomExits(to)["west"])
    end)

    it("removes a special exit and everything that it carried", function()
      local area = newArea("MapFileAuditSpecSpecials")
      local from = newRoom(area, 0)
      local to = newRoom(area, 1, distantRoomId)
      assert.is_true(addSpecialExit(from, to, "zqc"))
      assert.is_true(setDoor(from, "zqc", 3))
      assert.is_true(setExitWeight(from, "zqc", 4))
      assert.is_true(addSpecialExit(from, to, "zqk"))

      reloadWith(function(data)
        -- each special exit is its destination followed by its command, with
        -- a "0" in front of the command for an unlocked exit
        return planted(data, int32(to) .. qstring("0zqc"), int32(-4) .. qstring("0zqc"), 1)
      end, 20)

      local specials = getSpecialExitsSwap(from)
      assert.is_nil(specials["zqc"])
      assert.are.equal(to, specials["zqk"])
      assert.are.equal("-4", getRoomUserData(from, "audit.removed_invalid_special_exit.zqc"))
      assert.is_nil(getDoors(from)["zqc"])
      assert.is_nil(getExitWeights(from)["zqc"])
    end)
  end)

  describe("Tests special exit data that the file holds without the exit", function()
    it("removes a special exit that has no command", function()
      local area = newArea("MapFileAuditSpecNameless")
      local from = newRoom(area, 0)
      local to = newRoom(area, 1, distantRoomId)
      assert.is_true(addSpecialExit(from, to, "zqd"))
      assert.is_true(addSpecialExit(from, to, "zqk"))

      reloadWith(function(data)
        return planted(data, int32(to) .. qstring("0zqd"), int32(to) .. qstring("0"), 1)
      end, 20)

      local specials = getSpecialExitsSwap(from)
      assert.is_nil(specials[""])
      assert.is_nil(specials["zqd"])
      assert.are.equal(to, specials["zqk"])
    end)

    it("drops the door, weight and custom line of a command the room no longer has", function()
      local area = newArea("MapFileAuditSpecLeftovers")
      local from = newRoom(area, 0)
      local to = newRoom(area, 1)
      assert.is_true(addSpecialExit(from, to, "zqa"))
      assert.is_true(setDoor(from, "zqa", 1))
      assert.is_true(setExitWeight(from, "zqa", 3))
      assert.is_true(addCustomLine(from, {{0.5, 0.5, 0}}, "zqa", "dash line", {4, 5, 6}, true))

      reloadWith(function(data)
        -- only the special exit list prefixes the command with its lock
        -- state, so this renames the exit and leaves the rest keyed by the
        -- old command
        return planted(data, qstring("0zqa"), qstring("0zqb"), 1)
      end, 20)

      assert.are.equal(to, getSpecialExitsSwap(from)["zqb"])
      assert.is_nil(getDoors(from)["zqa"])
      assert.is_nil(getExitWeights(from)["zqa"])
      assert.is_nil((getCustomLines1(from) or {})["zqa"])
    end)
  end)

  describe("Tests room IDs", function()
    it("renumbers a room whose ID is below one and keeps its area, exits and hash", function()
      local area = newArea("MapFileAuditSpecBadRoomId")
      local from = newRoom(area, 0)
      newRoom(area, 1, distantRoomId)
      setRoomName(distantRoomId, "MapFileAuditSpecBadRoom")
      setRoomIDbyHash(distantRoomId, "MapFileAuditSpecBadRoomHash")
      assert.are.equal(distantRoomId, getRoomIDbyHash("MapFileAuditSpecBadRoomHash"))
      assert.is_true(setExit(from, distantRoomId, "east"))
      assert.is_true(setExit(distantRoomId, from, "west"))

      reloadWith(function(data)
        -- the room's own key, its place in its area's list of rooms, the east
        -- exit that leads to it and its entry in the table of room hashes
        return planted(data, int32(distantRoomId), int32(-7), 4)
      end)

      local renumbered
      for id, name in pairs(getRooms()) do
        if name == "MapFileAuditSpecBadRoom" then
          renumbered = id
        end
      end
      assert.is_not_nil(renumbered, "the room with the bad ID was lost")
      assert.is_true(renumbered >= 1)
      assert.are.equal("-7", getRoomUserData(renumbered, "audit.remapped_id"))
      assert.are.equal(area, getRoomArea(renumbered))
      local areaRooms = getAreaRooms1(area)
      table.sort(areaRooms)
      local expected = {from, renumbered}
      table.sort(expected)
      assert.are.same(expected, areaRooms)
      assert.are.equal(renumbered, getRoomExits(from)["east"])
      assert.are.equal(from, getRoomExits(renumbered)["west"])
      assert.are.equal(renumbered, getRoomIDbyHash("MapFileAuditSpecBadRoomHash"))
    end)

    it("renumbers a room whose ID is -1 without giving it every absent exit on the map", function()
      local distantRoomId = 0x5A5B5C01
      local area = addAreaName("MapFileAuditSpecMinusOneRoomId")
      local from = newRoom(area, 0)
      local bystander = newRoom(area, 5)
      assert.is_true(addRoom(distantRoomId))
      assert.is_true(setRoomArea(distantRoomId, area))
      setRoomName(distantRoomId, "MapFileAuditSpecMinusOneRoom")
      assert.is_true(setExit(from, distantRoomId, "east"))
      assert.is_true(setExit(distantRoomId, from, "west"))

      reloadWith(function(data)
        -- the room's own key, its place in its area's list of rooms and the
        -- east exit that leads to it, which as -1 reads as no exit at all
        return planted(data, int32(distantRoomId), int32(-1), 3)
      end)

      local renumbered
      for id, name in pairs(getRooms()) do
        if name == "MapFileAuditSpecMinusOneRoom" then
          renumbered = id
        end
      end
      assert.is_not_nil(renumbered, "the room with the bad ID was lost")
      assert.is_true(renumbered >= 1)
      assert.are.same({west = from}, getRoomExits(renumbered))
      assert.are.same({}, getRoomExits(bystander))
      assert.are.same({}, getRoomExits(from))
      assert.are.same({}, getSpecialExitsSwap(bystander))
    end)

    it("keeps a room whose ID is below one when it loads a JSON map", function()
      local jsonPath = mapDirectory .. "/mapfileaudit_spec.json"
      local distantRoomId = 0x5A5B5C01
      local area = addAreaName("MapFileAuditSpecJsonBadRoomId")
      local from = newRoom(area, 0)
      assert.is_true(addRoom(distantRoomId))
      assert.is_true(setRoomArea(distantRoomId, area))
      setRoomName(distantRoomId, "MapFileAuditSpecJsonBadRoom")
      assert.is_true(setExit(from, distantRoomId, "east"))
      assert.is_true(setExit(distantRoomId, from, "west"))
      assert.is_true(saveJsonMap(jsonPath))
      local text, count = readFile(jsonPath):gsub(tostring(distantRoomId), "-7")
      -- the room's own id and the east exit that leads to it
      assert.are.equal(2, count, "the fault was not planted where the file format was expected to have it")
      writeFile(jsonPath, text)
      deleteMap()
      assert.is_true(loadJsonMap(jsonPath))
      os.remove(jsonPath)

      local renumbered
      for id, name in pairs(getRooms()) do
        if name == "MapFileAuditSpecJsonBadRoom" then
          renumbered = id
        end
      end
      assert.is_not_nil(renumbered, "the room with the bad ID was lost")
      assert.is_true(renumbered >= 1)
      assert.are.equal("-7", getRoomUserData(renumbered, "audit.remapped_id"))
      local areaRooms = getAreaRooms1(area)
      table.sort(areaRooms)
      local expected = {from, renumbered}
      table.sort(expected)
      assert.are.same(expected, areaRooms)
      -- the JSON reader drops any exit whose target is below one, before the
      -- audit could send it to the renumbered room
      assert.is_nil(getRoomExits(from)["east"])
      assert.are.equal(from, getRoomExits(renumbered)["west"])
    end)
  end)

  describe("Tests area IDs and area membership", function()
    it("renumbers an area whose ID is below one and moves its rooms with it", function()
      assert.is_true(setAreaName(distantAreaId, "MapFileAuditSpecBadId"))
      -- moving a room into an area that so far is only a name creates it
      assert.is_nil(getAreaRooms1(distantAreaId))
      local room = newRoom(distantAreaId, 0)
      assert.are.same({room}, getAreaRooms1(distantAreaId))

      reloadWith(function(data)
        -- once as a key of the area names, once as the area itself and once
        -- as the area of its one room
        return planted(data, int32(distantAreaId), int32(-5), 3)
      end)

      local areaId = getAreaTable()["MapFileAuditSpecBadId"]
      assert.is_true(areaId >= 1)
      assert.are.equal(areaId, getRoomArea(room))
      -- the area's own list of its rooms has to come along too, or the mapper
      -- draws the area empty until the next audit puts the room back in it
      assert.are.same({room}, getAreaRooms1(areaId))
      assert.is_nil(getAreaRooms1(-5))
      assert.are.equal("-5", getAreaUserData(areaId, "audit.remapped_id"))
      assert.are.equal("-5", getRoomUserData(room, "audit.remapped_area"))
    end)

    it("gives each room back to the area it names, whatever the areas claimed", function()
      local home = newArea("MapFileAuditSpecHome")
      local away = newArea("MapFileAuditSpecAway")
      local homeRoom = newRoom(home, 0, distantRoomId)
      local awayRoom = newRoom(away, 1, otherDistantRoomId)

      reloadWith(function(data)
        -- the areas come before the rooms in the file, so the first place the
        -- home room's ID appears is in the list of rooms its area holds
        assert.are.equal(2, countBytes(data, int32(homeRoom)))
        return planted(data, int32(homeRoom), int32(awayRoom), 1, 1)
      end)

      assert.are.same({homeRoom}, getAreaRooms1(home))
      assert.are.same({awayRoom}, getAreaRooms1(away))
      assert.are.equal(home, getRoomArea(homeRoom))
      assert.are.equal(away, getRoomArea(awayRoom))
    end)

    it("creates an area that has a name but was never instantiated", function()
      local orphan = distantAreaId
      assert.is_true(setAreaName(orphan, "MapFileAuditSpecOrphan"))
      assert.is_nil(getAreaRooms1(orphan))

      auditAreas()

      assert.are.same({}, getAreaRooms1(orphan))
      assert.are.equal(orphan, getAreaTable()["MapFileAuditSpecOrphan"])
    end)
  end)

  describe("Tests area names", function()
    it("numbers the second of two areas with the same name", function()
      local first = newArea("MapFileAuditSpecDupAAAA")
      local second = newArea("MapFileAuditSpecDupBBBB")
      assert.is_true(first < second)

      reloadWith(function(data)
        return planted(data, qstring("MapFileAuditSpecDupBBBB"), qstring("MapFileAuditSpecDupAAAA"), 1)
      end)

      local names = getAreaTableSwap()
      assert.are.equal("MapFileAuditSpecDupAAAA", names[first])
      assert.are.equal("MapFileAuditSpecDupAAAA_001", names[second])
    end)

    it("replaces a duplicated name's own number rather than adding another", function()
      local first = newArea("MapFileAuditSpecDup_007")
      local second = newArea("MapFileAuditSpecDup_008")
      assert.is_true(first < second)

      reloadWith(function(data)
        return planted(data, qstring("MapFileAuditSpecDup_008"), qstring("MapFileAuditSpecDup_007"), 1)
      end)

      local names = getAreaTableSwap()
      assert.are.equal("MapFileAuditSpecDup_007", names[first])
      assert.are.equal("MapFileAuditSpecDup_001", names[second])
    end)

    -- the name the audit gives is translated, so these check what it has to
    -- be rather than the English text of it
    it("names an area whose name is empty", function()
      local nameless = newArea("MapFileAuditSpecNoName")
      local namesBefore = getAreaTable()

      reloadWith(function(data)
        return planted(data, qstring("MapFileAuditSpecNoName"), qstring(""), 1)
      end)

      local name = getAreaTableSwap()[nameless]
      assert.is_string(name)
      assert.are_not.equal("", name)
      assert.are_not.equal("MapFileAuditSpecNoName", name)
      assert.is_nil(namesBefore[name], "the audit gave the area a name another area already had")
    end)

    it("numbers the second of two areas that both have an empty name", function()
      local first = newArea("MapFileAuditSpecNoName1")
      local second = newArea("MapFileAuditSpecNoName2")
      assert.is_true(first < second)

      reloadWith(function(data)
        data = planted(data, qstring("MapFileAuditSpecNoName1"), qstring(""), 1)
        return planted(data, qstring("MapFileAuditSpecNoName2"), qstring(""), 1)
      end)

      local names = getAreaTableSwap()
      assert.is_string(names[first])
      assert.are_not.equal("", names[first])
      assert.are.equal(names[first] .. "_001", names[second])
    end)
  end)
end)
