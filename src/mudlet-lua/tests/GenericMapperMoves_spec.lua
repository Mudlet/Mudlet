-- The generic mapper runs on every move and every command sent, so neither
-- should read the whole map or a whole area unless the answer depends on it.
describe("Tests that the generic mapper does not read more of the map than a step needs", function()
  local function upvalue(fn, name)
    if type(fn) ~= "function" then
      return nil
    end
    local index = 1
    while true do
      local upvalueName, value = debug.getupvalue(fn, index)
      if not upvalueName then
        return nil
      end
      if upvalueName == name then
        return value, index
      end
      index = index + 1
    end
  end

  if type(map) ~= "table" or type(map.eventHandler) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  describe("finding the room a new move leads to", function()
    local findLink = upvalue(upvalue(upvalue(upvalue(map.eventHandler, "handle_exits"), "capture_room_info"), "move_map"), "find_link")
    -- map.set takes nil, but a table of saved values cannot hold one, hence the list
    local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
    local areaId, fromRoom, toRoom
    local saved, savedHash, savedPlayerRoom

    local function makeRoom(name, y)
      local id = createRoomID()
      addRoom(id)
      setRoomArea(id, areaId)
      setRoomCoordinates(id, 0, y, 0)
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
      areaId = addAreaName("genericMapperMovesSpec")
      fromRoom = makeRoom("Where the walk starts", 0)
      toRoom = makeRoom("One step north", 1)
      map.set("mapping", true)
      map.set("currentRoom", fromRoom)
      map.set("currentArea", areaId)
      map.prompt.hash = nil
    end)

    after_each(function()
      for _, key in ipairs(mapperState) do
        map.set(key, saved[key])
      end
      map.prompt.hash = savedHash
      if savedPlayerRoom and roomExists(savedPlayerRoom) then
        centerview(savedPlayerRoom)
      end
      deleteArea(areaId)
    end)

    it("can reach the mapper's search", function()
      assert.is_function(findLink)
    end)

    it("links to the adjacent room without reading the whole area when the search distance is bounded", function()
      local areaRooms = spy.on(_G, "getAreaRooms")
      finally(function() getAreaRooms:revert() end)
      -- a max_search_distance of 1, the default, searches one room ahead
      findLink("One step north", {}, "north", 1)
      assert.spy(areaRooms).was_not_called()
      assert.equals(toRoom, map.currentRoom)
      assert.equals(toRoom, getRoomExits(fromRoom).north)
    end)

    it("still bounds an unlimited search by the area it is in", function()
      local areaRooms = spy.on(_G, "getAreaRooms")
      finally(function() getAreaRooms:revert() end)
      -- a max_search_distance of 0 searches to the edge of the area
      findLink("One step north", {}, "north", 0)
      assert.spy(areaRooms).was_called(1)
      assert.equals(toRoom, map.currentRoom)
    end)
  end)

  describe("sending a command", function()
    local savedPath, savedWaiting, savedTimer, savedDownloading
    local downloadingIndex = select(2, upvalue(map.checkVersion, "downloading"))

    before_each(function()
      savedPath, savedWaiting, savedTimer = map.configs.download_path, map.update_waiting, map.update_timer
      savedDownloading = downloadingIndex and debug.getupvalue(map.checkVersion, downloadingIndex)
      map.update_waiting, map.update_timer = nil, nil
    end)

    after_each(function()
      map.configs.download_path, map.update_waiting, map.update_timer = savedPath, savedWaiting, savedTimer
      if downloadingIndex then
        debug.setupvalue(map.checkVersion, downloadingIndex, savedDownloading)
      end
    end)

    it("does not read the whole map when updates have no download path", function()
      map.configs.download_path = ""
      local rooms = spy.on(_G, "getRooms")
      finally(function() getRooms:revert() end)
      map.eventHandler("sysDataSendRequest", "say hello")
      assert.spy(rooms).was_not_called()
    end)

    it("still starts the update check when there is a download path", function()
      assert.is_number(downloadingIndex)
      map.configs.download_path = "https://example.invalid/generic_mapper"
      local download = stub(_G, "downloadFile")
      finally(function() downloadFile:revert() end)
      map.eventHandler("sysDataSendRequest", "say hello")
      assert.stub(download).was_called(1)
      assert.matches("/versions%.lua$", download.calls[1].vals[2])
      assert.is_true(map.update_waiting)
    end)
  end)
end)
