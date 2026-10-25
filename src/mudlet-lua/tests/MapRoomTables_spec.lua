-- getRooms() and getAreaRooms() size their tables up front and write room names
-- through a reused buffer, which is only safe while what a script reads out of
-- them stays the same: every key and name, and what `#` makes of the result.
-- Every test starts from a wiped map, so the map is saved first and put back after.

describe("Tests the tables getRooms and getAreaRooms hand back", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/maproomtables_spec_backup.dat"
  local plantedPath = mapDirectory .. "/maproomtables_spec_planted.dat"

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

  -- a binary map holds a QString as UTF-16BE, a zero byte before each ASCII character
  local function utf16(text)
    return (text:gsub(".", "\0%0"))
  end

  local function replaceOnce(data, from, to)
    local first, last = data:find(from, 1, true)
    assert.is_truthy(first, "the name to plant into was not found in the saved map")
    assert.is_falsy(data:find(from, last + 1, true), "the name to plant into is in the saved map twice")
    return data:sub(1, first - 1) .. to .. data:sub(last + 1)
  end

  local function newArea(name)
    local area = addAreaName(name)
    assert.is_true(area > 0)
    return area
  end

  local function newRoom(area, id, name)
    assert.is_true(addRoom(id))
    assert.is_true(setRoomArea(id, area))
    if name then
      setRoomName(id, name)
    end
  end

  local function countKeys(t)
    local n = 0
    for _ in pairs(t) do
      n = n + 1
    end
    return n
  end

  setup(function()
    os.remove(backupPath)
    os.remove(plantedPath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(plantedPath)
  end)

  before_each(function()
    deleteMap()
  end)

  describe("getRooms", function()
    it("returns an empty table for a map without rooms", function()
      assert.are.same({}, getRooms())
    end)

    it("names every room by its ID, in the same text getRoomName gives", function()
      local area = newArea("MapRoomTablesSpecNames")
      local names = {
        [1] = "Plain",
        [2] = "Café crème",
        [3] = "日本語の部屋",
        [4] = "🐉 lair",
        [5] = "",
        [6] = string.rep("A long corridor ", 300),
        [70] = "Short",
        [123456] = "Far away",
      }
      for id, name in pairs(names) do
        newRoom(area, id, name)
      end

      local rooms = getRooms()

      assert.are.same(names, rooms)
      for id in pairs(names) do
        assert.are.equal(getRoomName(id), rooms[id])
      end
    end)

    it("ends a name at a zero byte and drops an unpaired surrogate, as getRoomName does", function()
      local area = newArea("MapRoomTablesSpecPlanted")
      newRoom(area, 1, "MapRoomTablesSpecZeroQafter")
      newRoom(area, 2, "MapRoomTablesSpecLoneQafter")
      newRoom(area, 3, "MapRoomTablesSpecTailQ")
      assert.is_true(saveMap(plantedPath))
      local data = readFile(plantedPath)
      data = replaceOnce(data, utf16("ZeroQafter"), utf16("Zero") .. "\0\0" .. utf16("after"))
      data = replaceOnce(data, utf16("LoneQafter"), utf16("Lone") .. "\216\0" .. utf16("after"))
      data = replaceOnce(data, utf16("TailQ"), utf16("Tail") .. "\220\0")
      writeFile(plantedPath, data)
      deleteMap()
      assert.is_true(loadMap(plantedPath))

      local rooms = getRooms()

      assert.are.equal("MapRoomTablesSpecZero", rooms[1])
      for id = 1, 3 do
        assert.are_not.equal(nil, rooms[id])
        assert.are.equal(getRoomName(id), rooms[id])
        assert.is_falsy(rooms[id]:find("\239\191\189", 1, true), "a replacement character turned up in room " .. id)
      end
    end)

    -- `#` on a table with holes depends on how Lua split it between its array
    -- and hash parts, and scripts in the wild test a map for rooms with it
    it("gives the length of a run of rooms whether or not room 1 exists", function()
      local area = newArea("MapRoomTablesSpecLength")
      for id = 2, 40 do
        newRoom(area, id)
      end
      assert.are.equal(40, #getRooms())

      newRoom(area, 1)
      assert.are.equal(40, #getRooms())
    end)
  end)

  describe("getAreaRooms", function()
    it("lists an area's rooms from index zero", function()
      local area = newArea("MapRoomTablesSpecArea")
      local wanted = {}
      for id = 11, 15 do
        newRoom(area, id)
        wanted[id] = true
      end

      local rooms = getAreaRooms(area)

      assert.are.equal(5, countKeys(rooms))
      assert.are.equal(4, #rooms)
      local seen = {}
      for index = 0, 4 do
        local id = rooms[index]
        assert.is_true(wanted[id], "index " .. index .. " holds " .. tostring(id))
        assert.is_nil(seen[id])
        seen[id] = true
      end
    end)

    it("returns an empty table for an area without rooms and nil for no area", function()
      local area = newArea("MapRoomTablesSpecEmpty")
      assert.are.same({}, getAreaRooms(area))
      assert.is_nil(getAreaRooms(area + 1000))
    end)

    it("holds one room at index zero", function()
      local area = newArea("MapRoomTablesSpecSingle")
      newRoom(area, 7)
      assert.are.same({[0] = 7}, getAreaRooms(area))
      assert.are.equal(0, #getAreaRooms(area))
    end)
  end)
end)
