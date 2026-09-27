-- The audit that runs when a binary map file is loaded, and repairs what it
-- finds wrong with it. Nothing the Lua API offers can build a map that is
-- damaged in these ways, so each test saves a sound map, alters bytes of the
-- file it wrote in one planted place, and loads the result back. The planting
-- helpers count what they replaced, so a change to the file format that moves
-- or renames what they look for fails loudly rather than loading an undamaged
-- map that the assertions would then be checking for nothing.

describe("Tests the audit of a damaged binary map file", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/mapfileaudit_spec_backup.dat"
  local damagedPath = mapDirectory .. "/mapfileaudit_spec_damaged.dat"

  -- well clear of anything addAreaName() hands out, and with no zero byte in
  -- it, so it appears in the file only where it is used
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

  -- saves the map as it stands, hands the file's bytes to plantFault, which
  -- returns them altered, and loads the result back over a wiped map
  local function reloadWith(plantFault)
    assert.is_true(saveMap(damagedPath))
    writeFile(damagedPath, plantFault(readFile(damagedPath)))
    deleteMap()
    assert.is_true(loadMap(damagedPath))
  end

  local function planted(data, from, to, expected, limit)
    local result, count = replaceBytes(data, from, to, limit)
    assert.are.equal(expected, count, "the fault was not planted where the file format was expected to have it")
    return result
  end

  local function newRoom(area, x)
    local id = createRoomID()
    assert.is_true(addRoom(id))
    assert.is_true(setRoomArea(id, area))
    setRoomCoordinates(id, x, 0, 0)
    return id
  end

  setup(function()
    os.remove(backupPath)
    os.remove(damagedPath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

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

  describe("Tests room IDs", function()
    it("renumbers a room whose ID is below one and keeps its area, exits and hash", function()
      local distantRoomId = 0x5A5B5C01
      local area = addAreaName("MapFileAuditSpecBadRoomId")
      local from = newRoom(area, 0)
      assert.is_true(addRoom(distantRoomId))
      assert.is_true(setRoomArea(distantRoomId, area))
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

    it("renumbers a room whose ID is -1 and keeps the special exit that leads to it", function()
      local distantRoomId = 0x5A5B5C01
      local area = addAreaName("MapFileAuditSpecMinusOneSpecialExit")
      local from = newRoom(area, 0)
      assert.is_true(addRoom(distantRoomId))
      assert.is_true(setRoomArea(distantRoomId, area))
      setRoomName(distantRoomId, "MapFileAuditSpecMinusOneTarget")
      assert.is_true(addSpecialExit(from, distantRoomId, "climb the rope"))

      reloadWith(function(data)
        -- the room's own key, its place in its area's list of rooms and the
        -- special exit that leads to it
        return planted(data, int32(distantRoomId), int32(-1), 3)
      end)

      local renumbered
      for id, name in pairs(getRooms()) do
        if name == "MapFileAuditSpecMinusOneTarget" then
          renumbered = id
        end
      end
      assert.is_not_nil(renumbered, "the room with the bad ID was lost")
      assert.is_true(renumbered >= 1)
      assert.are.same({["climb the rope"] = renumbered}, getSpecialExitsSwap(from))
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
  end)
end)
