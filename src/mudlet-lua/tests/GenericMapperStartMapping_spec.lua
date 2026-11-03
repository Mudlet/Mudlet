describe("Tests starting the generic mapper", function()
  if type(map) ~= "table" or type(map.start_mapping) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- map.set takes nil, but a table of saved values cannot hold one, hence the list
  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local defaultArea = "New area"
  local roomName = "Where the first map begins"
  local saved, savedHash, savedPlayerRoom, savedRooms, savedAreas

  local function makeRoom(areaId, name)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, 0, 0, 0)
    setRoomName(id, name)
    return id
  end

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedHash = map.prompt.hash
    savedPlayerRoom = getPlayerRoom()
    savedRooms = getRooms()
    savedAreas = getAreaTable()
    map.prompt.hash = nil
    map.set("mapping", false)
    map.set("currentRoom", nil)
    map.set("currentArea", nil)
    map.set("currentName", roomName)
    map.set("currentExits", {})
  end)

  after_each(function()
    for id in pairs(getRooms()) do
      if not savedRooms[id] then
        deleteRoom(id)
      end
    end
    for name, id in pairs(getAreaTable()) do
      if savedAreas[name] == nil then
        deleteArea(id)
      end
    end
    for _, key in ipairs(mapperState) do
      map.set(key, saved[key])
    end
    map.prompt.hash = savedHash
    if savedPlayerRoom and roomExists(savedPlayerRoom) then
      centerview(savedPlayerRoom)
    end
  end)

  describe("on an empty map", function()
    for _, areaName in ipairs({"", false}) do
      it(string.format("starts in a default area when given %s", areaName and "an empty name" or "no name"), function()
        assert.is_true(table.is_empty(getRooms()), "an earlier spec left rooms on the map")
        assert.has_no.errors(function()
          map.start_mapping(areaName or nil)
        end)
        assert.is_true(map.mapping)
        assert.are.equal(defaultArea:lower(), getRoomAreaName(map.currentArea):lower())
        assert.are.equal(map.currentArea, getRoomArea(map.currentRoom))
      end)
    end

    it("uses the area name it is given", function()
      local named = "genericMapperStartMappingSpec"
      map.start_mapping(named)
      assert.are.equal(getAreaTable()[named], map.currentArea)
    end)
  end)

  describe("on a map that already has rooms", function()
    local areaName = "genericMapperStartMappingExisting"
    local areaId, room

    before_each(function()
      areaId = addAreaName(areaName)
      room = makeRoom(areaId, roomName)
    end)

    it("keeps mapping in the current area when no name is given", function()
      map.set("currentRoom", room)
      map.set("currentArea", areaId)
      map.start_mapping(nil)
      assert.is_true(map.mapping)
      assert.are.equal(areaId, map.currentArea)
      assert.are.equal(areaId, getRoomArea(room))
      assert.are.equal(savedAreas[defaultArea], getAreaTable()[defaultArea])
    end)

    it("refuses to guess an area when it does not know where the player is", function()
      local ok, err = pcall(map.start_mapping, nil)
      assert.is_false(ok)
      assert.truthy(tostring(err):find("Not sure where you are", 1, true), tostring(err))
      assert.is_falsy(map.mapping)
      assert.are.equal(savedAreas[defaultArea], getAreaTable()[defaultArea])
      assert.are.equal(1, table.size(getAreaRooms(areaId)))
    end)
  end)
end)
