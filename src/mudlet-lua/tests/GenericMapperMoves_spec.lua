-- The generic mapper runs on every move and every command sent, so neither
-- should read the whole map or a whole area unless the answer depends on it.
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

describe("Tests that the generic mapper does not read more of the map than a step needs", function()
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
      savedDownloading = downloadingIndex and select(2, debug.getupvalue(map.checkVersion, downloadingIndex))
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

-- The map can open before the player has logged in - a game's own package may
-- open it while it installs - and a game that asks for a name first takes
-- whatever arrives then as the new character's name
describe("Tests that the generic mapper sends nothing when the map opens", function()
  if type(map) ~= "table" or type(map.eventHandler) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local grabLine = upvalue(map.eventHandler, "grab_line")
  local _, helpShownIndex = upvalue(map.eventHandler, "help_shown")
  local _, linesIndex = upvalue(grabLine, "lines")
  local _, awaitPromptIndex = upvalue(grabLine, "await_prompt")
  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local saveFile = getMudletHomeDir() .. "/map downloads/map_save.dat"
  local character, saved, sent, sendHandler

  local function readFile(path)
    local file = io.open(path, "rb")
    if not file then
      return nil
    end
    local content = file:read("*a")
    file:close()
    return content
  end

  -- the game's own output, each block ended as a prompt with telnet GA
  local function gameSends(text)
    local ok, message = feedTelnet(text .. "<T_IAC><T_GA>")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))
  end

  local function guideTimers()
    local calls = {}
    for _, call in ipairs(tempTimer.calls) do
      if call.vals[1] == 3 then
        calls[#calls + 1] = call
      end
    end
    return calls
  end

  before_each(function()
    character = map.character or ""
    saved = {
      helpShown = select(2, debug.getupvalue(map.eventHandler, helpShownIndex)),
      lines = select(2, debug.getupvalue(grabLine, linesIndex)),
      pattern = map.save.prompt_pattern[character],
      saveFile = readFile(saveFile),
      prompt = table.deepcopy(map.prompt),
      state = {},
    }
    for _, key in ipairs(mapperState) do
      saved.state[key] = map[key]
    end
    debug.setupvalue(map.eventHandler, helpShownIndex, false)
    debug.setupvalue(grabLine, linesIndex, {})
    map.save.prompt_pattern[character] = nil
    map.set("currentName", nil)
    map.set("currentExits", nil)
    sent = {}
    -- anything that reaches the game passes through here, whichever function sent it
    sendHandler = registerAnonymousEventHandler("sysDataSendRequest", function(_, command)
      sent[#sent + 1] = command
      denyCurrentSend()
    end)
    stub(_G, "sendSocket")
    -- the guide comes up on a timer, which would print it into a later spec's output
    stub(_G, "tempTimer")
  end)

  after_each(function()
    killAnonymousEventHandler(sendHandler)
    sendSocket:revert()
    tempTimer:revert()
    debug.setupvalue(map.eventHandler, helpShownIndex, saved.helpShown)
    debug.setupvalue(grabLine, linesIndex, saved.lines)
    if awaitPromptIndex then
      debug.setupvalue(grabLine, awaitPromptIndex, false)
    end
    map.save.prompt_pattern[character] = saved.pattern
    if saved.saveFile then
      local file = io.open(saveFile, "wb")
      file:write(saved.saveFile)
      file:close()
    end
    for key in pairs(map.prompt) do
      map.prompt[key] = nil
    end
    for key, value in pairs(saved.prompt) do
      map.prompt[key] = value
    end
    for _, key in ipairs(mapperState) do
      map.set(key, saved.state[key])
    end
  end)

  it("can reach the mapper's state", function()
    assert.is_function(grabLine)
    assert.is_number(helpShownIndex)
    assert.is_number(linesIndex)
  end)

  it("sends nothing to the game", function()
    map.eventHandler("mapOpenEvent")
    assert.same({}, sent)
    assert.stub(sendSocket).was_not_called()
  end)

  it("offers the quick start guide once, a moment after the map first opens", function()
    map.eventHandler("mapOpenEvent")
    local timers = guideTimers()
    assert.equals(1, #timers)
    local showHelp = stub(map, "show_help")
    finally(function() showHelp:revert() end)
    timers[1].vals[2]()
    assert.stub(showHelp).was_called_with("quick_start")
    assert.is_true(select(2, debug.getupvalue(map.eventHandler, helpShownIndex)))

    map.eventHandler("mapOpenEvent")
    assert.equals(1, #guideTimers())
  end)

  it("offers no guide once the character has a prompt pattern", function()
    map.save.prompt_pattern[character] = "^>"
    map.eventHandler("mapOpenEvent")
    assert.equals(0, #guideTimers())
  end)

  it("learns the prompt from the game's own output, and maps the next room", function()
    map.eventHandler("mapOpenEvent")
    gameSends("Where The Spec Starts\r\nA quiet corner.\r\n[ Exits: north ]\r\n<100hp 50mp> ")
    assert.equals("^%[?%a*%]?<.*>", map.save.prompt_pattern[character])
    gameSends("Where The Spec Goes Next\r\nA busier corner.\r\n[ Exits: south ]\r\n<100hp 50mp> ")
    assert.equals("Where The Spec Goes Next", map.currentName)
  end)

  it("does not take an exits line for the prompt", function()
    map.eventHandler("mapOpenEvent")
    local ok, message = feedTelnet("Where The Spec Starts\r\n[ Exits: north ]\r\n")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))
    assert.is_nil(map.save.prompt_pattern[character])
  end)
end)
