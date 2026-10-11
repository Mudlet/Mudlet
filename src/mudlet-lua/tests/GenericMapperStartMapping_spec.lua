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
  local saved, savedHash, savedPlayerRoom, areaExisted

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
    areaExisted = getAreaTable()[defaultArea] ~= nil
    map.prompt.hash = nil
    map.set("mapping", false)
    map.set("currentRoom", nil)
    map.set("currentArea", nil)
    map.set("currentName", roomName)
    map.set("currentExits", {})
  end)

  after_each(function()
    local areaId = getAreaTable()[defaultArea]
    if areaId and not areaExisted then
      deleteArea(areaId)
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
        local areaId = getAreaTable()[defaultArea]
        assert.is_not_nil(areaId)
        assert.are.equal(areaId, map.currentArea)
        assert.are.equal(areaId, getRoomArea(map.currentRoom))
      end)
    end

    it("uses the area name it is given", function()
      local named = "genericMapperStartMappingSpec"
      finally(function()
        local areaId = getAreaTable()[named]
        if areaId then
          deleteArea(areaId)
        end
      end)
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

    after_each(function()
      deleteArea(areaId)
    end)

    it("keeps mapping in the current area when no name is given", function()
      map.set("currentRoom", room)
      map.set("currentArea", areaId)
      map.start_mapping(nil)
      assert.is_true(map.mapping)
      assert.are.equal(areaId, map.currentArea)
      assert.are.equal(areaId, getRoomArea(room))
      assert.is_nil(getAreaTable()[defaultArea])
    end)

    it("refuses to guess an area when it does not know where the player is", function()
      assert.has_error(function()
        map.start_mapping(nil)
      end)
      assert.is_falsy(map.mapping)
      assert.is_nil(getAreaTable()[defaultArea])
      assert.are.equal(1, table.size(getAreaRooms(areaId)))
    end)
  end)
end)
