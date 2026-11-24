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
      awaitPrompt = select(2, debug.getupvalue(grabLine, awaitPromptIndex)),
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
    debug.setupvalue(grabLine, awaitPromptIndex, false)
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
    debug.setupvalue(grabLine, awaitPromptIndex, saved.awaitPrompt)
    map.save.prompt_pattern[character] = saved.pattern
    if saved.saveFile then
      local file = io.open(saveFile, "wb")
      file:write(saved.saveFile)
      file:close()
    else
      -- the mapper reloads this file on any package install, learned prompt and all
      os.remove(saveFile)
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
    assert.is_number(awaitPromptIndex)
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

  it("does not take a login pager or menu line for the prompt", function()
    map.eventHandler("mapOpenEvent")
    gameSends("Welcome to SpecMUD!\r\nMessage of the day...\r\n[Hit Return to continue] ")
    gameSends("1) Play\r\n2) Quit\r\nChoice> ")
    assert.is_nil(map.save.prompt_pattern[character])
    gameSends("Where The Spec Starts\r\nA quiet corner.\r\n[ Exits: north ]\r\n<100hp 50mp> ")
    assert.equals("^%[?%a*%]?<.*>", map.save.prompt_pattern[character])
  end)

  it("stops listening for a prompt once the player sets one", function()
    local echo = stub(map, "echo")
    local savedAlt = map.save.prompt_pattern.SpecAlt
    finally(function()
      echo:revert()
      map.character = character
      map.save.prompt_pattern.SpecAlt = savedAlt
    end)
    map.eventHandler("mapOpenEvent")
    map.make_prompt_pattern("<.*>")
    map.character = "SpecAlt"
    map.save.prompt_pattern.SpecAlt = nil
    gameSends("<100hp 50mp> ")
    assert.is_nil(map.save.prompt_pattern.SpecAlt)
  end)

  it("stops listening for a prompt once find prompt gives up", function()
    local echo = stub(map, "echo")
    finally(function() echo:revert() end)
    map.eventHandler("mapOpenEvent")
    map.find_prompt()
    map.search_timer_check()
    gameSends("<100hp 50mp> ")
    assert.is_nil(map.save.prompt_pattern[character])
  end)
end)

describe("Tests that the generic mapper reports its move queue in debug mode", function()
  if type(map) ~= "table" or type(map.eventHandler) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local captureMoveCmd = upvalue(map.eventHandler, "capture_move_cmd")
  local moveMap = upvalue(upvalue(upvalue(map.eventHandler, "handle_exits"), "capture_room_info"), "move_map")
  local _, queueIndex = upvalue(moveMap, "move_queue")
  local _, randomMoveIndex = upvalue(moveMap, "random_move")
  local _, waitEchoIndex = upvalue(map.echo, "wait_echo")
  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local areaId, fromRoom, toRoom, saved, echoed

  local function queue()
    return select(2, debug.getupvalue(moveMap, queueIndex))
  end

  local function echoedMentioning(text)
    local found = {}
    for _, line in ipairs(echoed) do
      if line:find(text, 1, true) then
        found[#found + 1] = line
      end
    end
    return found
  end

  local function makeRoom(name, y)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, 0, y, 0)
    setRoomName(id, name)
    return id
  end

  before_each(function()
    saved = {
      debug = map.configs.debug,
      queue = queue(),
      randomMove = select(2, debug.getupvalue(moveMap, randomMoveIndex)),
      waitEcho = select(2, debug.getupvalue(map.echo, waitEchoIndex)),
      descriptionMatching = map.configs.use_description_matching,
      hash = map.prompt.hash,
      playerRoom = getPlayerRoom(),
      state = {},
    }
    for _, key in ipairs(mapperState) do
      saved.state[key] = map[key]
    end
    debug.setupvalue(moveMap, queueIndex, {})
    debug.setupvalue(moveMap, randomMoveIndex, false)
    debug.setupvalue(map.echo, waitEchoIndex, {})
    map.configs.use_description_matching = false
    areaId = addAreaName("genericMapperMoveQueueSpec")
    fromRoom = makeRoom("Where the queue starts", 0)
    toRoom = makeRoom("Where the queue leads", 1)
    setExit(fromRoom, toRoom, "n")
    setExit(toRoom, fromRoom, "s")
    map.set("mapping", false)
    map.set("currentRoom", fromRoom)
    map.set("currentArea", areaId)
    map.prompt.hash = nil
    echoed = {}
    stub(_G, "cecho", function(text) echoed[#echoed + 1] = text end)
    stub(_G, "decho")
    stub(_G, "echo")
  end)

  after_each(function()
    cecho:revert()
    decho:revert()
    echo:revert()
    map.configs.debug = saved.debug
    debug.setupvalue(moveMap, queueIndex, saved.queue)
    debug.setupvalue(moveMap, randomMoveIndex, saved.randomMove)
    debug.setupvalue(map.echo, waitEchoIndex, saved.waitEcho)
    map.configs.use_description_matching = saved.descriptionMatching
    map.prompt.hash = saved.hash
    for _, key in ipairs(mapperState) do
      map.set(key, saved.state[key])
    end
    if saved.playerRoom and roomExists(saved.playerRoom) then
      centerview(saved.playerRoom)
    end
    deleteArea(areaId)
  end)

  it("can reach the mapper's move queue", function()
    assert.is_function(captureMoveCmd)
    assert.is_function(moveMap)
    assert.is_number(queueIndex)
    assert.is_number(randomMoveIndex)
    assert.is_number(waitEchoIndex)
  end)

  it("reports a move as it is queued and as the map follows it", function()
    map.configs.debug = true
    captureMoveCmd("north")
    assert.same({"north"}, queue())
    -- held back until the mapper next prints, so it does not split the line being sent
    assert.same({}, echoedMentioning("move queue"))

    map.set("currentName", "Where the queue leads")
    map.set("currentExits", {"south"})
    moveMap()
    assert.same({}, queue())
    assert.equals(toRoom, map.currentRoom)
    local queueLines = echoedMentioning("move queue")
    assert.equals(2, #queueLines)
    assert.truthy(queueLines[1]:find("Queued move north - move queue: north", 1, true))
    assert.truthy(queueLines[2]:find("Following move north - move queue: empty", 1, true))
  end)

  it("reports what a failed or random move drops from the queue", function()
    map.configs.debug = true
    captureMoveCmd("north")
    captureMoveCmd("east")
    echoed = {}
    map.eventHandler("onMoveFail")
    assert.same({"east"}, queue())
    assert.equals(1, #echoedMentioning("dropped move north"))

    map.eventHandler("onRandomMove")
    assert.same({}, queue())
    assert.equals(1, #echoedMentioning("Random move, dropping the queue - move queue: east"))
  end)

  it("reports the moves dropped while it looks for the player", function()
    map.configs.debug = true
    captureMoveCmd("north")
    echoed = {}
    map.find_me("Where the queue leads", {"south"})
    assert.same({}, queue())
    assert.equals(toRoom, map.currentRoom)
    assert.equals(1, #echoedMentioning("Locating you, dropping the queue - move queue: north"))
  end)

  it("echoes nothing about the queue with debug off", function()
    map.configs.debug = false
    captureMoveCmd("north")
    map.set("currentName", "Where the queue leads")
    map.set("currentExits", {"south"})
    moveMap()
    map.eventHandler("onMoveFail")
    map.find_me("Where the queue leads", {"south"})
    assert.equals(toRoom, map.currentRoom)
    assert.same({}, echoedMentioning("move queue"))
  end)
end)

-- A trigger on "You must stand up first" or a stun holds the walk; it then walks on from wherever the character is
describe("Tests the generic mapper's map.customwalkdelay", function()
  if type(map) ~= "table" or type(map.customwalkdelay) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local continueWalk = upvalue(map.speedwalk, "continue_walk")
  local mapperState = {"mapping", "currentRoom", "currentArea"}
  local walkConfigs = {"speedwalk_delay", "speedwalk_wait", "speedwalk_random", "use_translation"}
  local areaId, rooms, saved, sent, timers

  local function makeRoom(name, y)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, 0, y, 0)
    setRoomName(id, name)
    return id
  end

  -- the timers the walk has started and not seen killed, latest last
  local function liveTimers()
    local live = {}
    for _, timer in ipairs(timers) do
      if not timer.killed then
        live[#live + 1] = timer
      end
    end
    return live
  end

  before_each(function()
    saved = {configs = {}, state = {}, playerRoom = getPlayerRoom()}
    for _, key in ipairs(walkConfigs) do
      saved.configs[key] = map.configs[key]
    end
    for _, key in ipairs(mapperState) do
      saved.state[key] = map[key]
    end
    areaId = addAreaName("genericMapperWalkDelaySpec")
    rooms = {makeRoom("Walk start", 0), makeRoom("Walk middle", 1), makeRoom("Walk end", 2)}
    setExit(rooms[1], rooms[2], "n")
    setExit(rooms[2], rooms[3], "n")
    map.set("mapping", false)
    map.set("currentRoom", rooms[1])
    map.set("currentArea", areaId)
    map.configs.speedwalk_delay = 1
    map.configs.speedwalk_wait = false
    map.configs.speedwalk_random = false
    map.configs.use_translation = false
    sent, timers = {}, {}
    stub(_G, "send", function(command) sent[#sent + 1] = command end)
    -- ids far above Mudlet's own, so killing one of these never kills a real timer, and the rest still reach Mudlet
    local firstId = 1000000000
    local realKillTimer = killTimer
    stub(_G, "tempTimer", function(seconds, fn)
      timers[#timers + 1] = {seconds = seconds, fn = fn}
      return firstId + #timers
    end)
    stub(_G, "killTimer", function(id)
      local timer = type(id) == "number" and timers[id - firstId]
      if not timer then
        return realKillTimer(id)
      end
      timer.killed = true
      return true
    end)
    stub(map, "echo")
  end)

  after_each(function()
    -- speedWalkDir is nil until the profile's first path
    if speedWalkDir and #speedWalkDir > 0 then
      map.stopSpeedwalk()
    end
    send:revert()
    tempTimer:revert()
    killTimer:revert()
    map.echo:revert()
    for _, key in ipairs(walkConfigs) do
      map.configs[key] = saved.configs[key]
    end
    for _, key in ipairs(mapperState) do
      map.set(key, saved.state[key])
    end
    if saved.playerRoom and roomExists(saved.playerRoom) then
      centerview(saved.playerRoom)
    end
    deleteArea(areaId)
  end)

  it("can reach the mapper's walk", function()
    assert.is_function(continueWalk)
  end)

  it("holds the walk for the seconds given, then walks on from where the character is", function()
    map.speedwalk(rooms[3])
    assert.equals(1, #sent)

    assert.equals(2.5, map.customwalkdelay(2.5))
    local live = liveTimers()
    assert.equals(1, #live, "the walk's own next step should have been cancelled")
    assert.equals(2.5, live[1].seconds)

    -- the game said no to the first step, so the character is still at the start
    live[1].fn()
    assert.equals(2, #sent)
    assert.equals(sent[1], sent[2])
  end)

  it("walks on to where it was going after another route was looked up during the hold", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    getPath(rooms[1], rooms[2])
    liveTimers()[1].fn()
    -- getPath fills speedWalkPath with room ids as strings
    assert.equals(tostring(rooms[3]), tostring(speedWalkPath[#speedWalkPath]))
  end)

  it("walks to a destination chosen during the hold once the hold ends", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    map.speedwalk(rooms[2])
    assert.equals(1, #sent)
    local paths = spy.on(_G, "getPath")
    finally(function() getPath:revert() end)
    liveTimers()[1].fn()
    assert.equals(2, #sent)
    -- a one-step walk is over, and speedWalkPath emptied, once its step is sent
    assert.spy(paths).was_called_with(rooms[1], rooms[2])
  end)

  it("keeps a destination chosen during the hold when the hold is renewed", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    map.speedwalk(rooms[2])
    map.customwalkdelay(1)
    local paths = spy.on(_G, "getPath")
    finally(function() getPath:revert() end)
    local live = liveTimers()
    live[#live].fn()
    assert.spy(paths).was_called_with(rooms[1], rooms[2])
  end)

  it("resumes towards a destination chosen during the hold", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    map.speedwalk(rooms[2])
    stub(map, "find_me")
    finally(function() map.find_me:revert() end)
    map.pauseSpeedwalk()
    map.resumeSpeedwalk()
    local paths = spy.on(_G, "getPath")
    finally(function() getPath:revert() end)
    local live = liveTimers()
    live[#live].fn()
    assert.spy(paths).was_called_with(rooms[1], rooms[2])
  end)

  it("ends the walk when the hold ends with no way on from where the character got to", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    setExit(rooms[1], -1, "n")
    local stopped = 0
    local handler = registerAnonymousEventHandler("sysSpeedwalkStopped", function() stopped = stopped + 1 end)
    finally(function() killAnonymousEventHandler(handler) end)
    liveTimers()[1].fn()
    assert.equals(1, stopped)
    -- the old directions are not walked on
    continueWalk()
    assert.equals(1, #sent)
  end)

  it("can be stopped while held after another route was not found", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    local hold = liveTimers()[1]
    getPath(rooms[1], makeRoom("Walk island", 5))
    assert.equals(0, #speedWalkDir, "the failed lookup should have emptied speedWalkDir")
    map.stopSpeedwalk()
    assert.is_true(hold.killed)
    hold.fn()
    assert.equals(1, #sent)
  end)

  it("forgets a paused walk's destination once a resume finds the character already there", function()
    stub(map, "find_me")
    finally(function() map.find_me:revert() end)
    map.speedwalk(rooms[3])
    map.pauseSpeedwalk()
    -- walked the rest of the way by hand
    map.set("currentRoom", rooms[3])
    map.resumeSpeedwalk()
    local live = liveTimers()
    live[#live].fn()
    assert.stub(map.echo).was_called_with("You are already in that room.")

    map.set("currentRoom", rooms[1])
    local before = #timers
    map.resumeSpeedwalk()
    assert.stub(map.echo).was_called_with("Not currently speedwalking.")
    assert.equals(before, #timers, "a resume with no walk left should not set off towards the old destination")
    assert.equals(1, #sent)
  end)

  it("stops a paused walk whose directions a route not found has emptied", function()
    stub(map, "find_me")
    finally(function() map.find_me:revert() end)
    map.speedwalk(rooms[3])
    map.pauseSpeedwalk()
    getPath(rooms[1], makeRoom("Walk island", 5))
    assert.equals(0, #speedWalkDir, "the failed lookup should have emptied speedWalkDir")
    map.stopSpeedwalk()
    assert.stub(map.echo).was_called_with("Speedwalking stopped.")

    local before = #timers
    map.resumeSpeedwalk()
    assert.equals(before, #timers, "a stopped walk should not resume")
    assert.equals(1, #sent)
  end)

  it("picks a time from the range when given a second number", function()
    map.speedwalk(rooms[3])
    for _ = 1, 20 do
      local seconds = map.customwalkdelay(0.5, 1.5)
      assert.is_true(seconds >= 0.5 and seconds <= 1.5, tostring(seconds))
      local live = liveTimers()
      assert.equals(seconds, live[#live].seconds)
    end
  end)

  it("sends nothing on arriving in a room while the walk is held", function()
    map.configs.speedwalk_wait = true
    map.configs.speedwalk_delay = 0
    map.speedwalk(rooms[3])
    assert.equals(1, #sent)
    map.customwalkdelay(1)
    continueWalk(true)
    assert.equals(1, #sent)
  end)

  it("lets a stopped walk stay stopped", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    local hold = liveTimers()[1]
    map.stopSpeedwalk()
    assert.is_true(hold.killed)
    hold.fn()
    assert.equals(1, #sent)
  end)

  it("returns nil and a message when no walk is going step by step", function()
    local seconds, message = map.customwalkdelay(1)
    assert.is_nil(seconds)
    assert.is_string(message)
    assert.equals(0, #timers)
  end)

  it("walks a last step the game refused again", function()
    map.speedwalk(rooms[2])
    assert.equals(1, #sent)

    assert.equals(1, map.customwalkdelay(1))
    local live = liveTimers()
    live[#live].fn()
    assert.equals(2, #sent)
    assert.equals(sent[1], sent[2])
  end)

  it("walks on to where it was going when a walk with no room given is chosen during the hold", function()
    map.speedwalk(rooms[3])
    map.customwalkdelay(1)
    getPath(rooms[1], makeRoom("Walk island", 5))
    map.speedwalk()
    local paths = spy.on(_G, "getPath")
    finally(function() getPath:revert() end)
    liveTimers()[1].fn()
    assert.spy(paths).was_called_with(rooms[1], rooms[3])
    assert.equals(2, #sent)
  end)

  it("leaves nothing to resume after mapStop", function()
    stub(map, "find_me")
    finally(function() map.find_me:revert() end)
    map.speedwalk(rooms[3])
    getPath(rooms[1], makeRoom("Walk island", 5))
    raiseEvent("mapStop")
    assert.equals(0, #liveTimers(), "the walk's next step is still to come")
    local before = #timers
    map.resumeSpeedwalk()
    assert.stub(map.echo).was_called_with("Not currently speedwalking.")
    assert.equals(before, #timers)
  end)

  it("resumes at once when resumed during a hold", function()
    stub(map, "find_me")
    finally(function() map.find_me:revert() end)
    map.speedwalk(rooms[3])
    map.customwalkdelay(5)
    local hold = liveTimers()[1]
    map.resumeSpeedwalk()
    assert.is_true(hold.killed)
    local live = liveTimers()
    live[#live].fn()
    assert.equals(2, #sent)
  end)

  it("refuses seconds that are not a number of 0 or more", function()
    assert.has_error(function() map.customwalkdelay("2") end)
    assert.has_error(function() map.customwalkdelay(-1) end)
    assert.has_error(function() map.customwalkdelay(0 / 0) end)
    assert.has_error(function() map.customwalkdelay(2, 1) end)
    assert.has_error(function() map.customwalkdelay(1, "3") end)
  end)
end)
