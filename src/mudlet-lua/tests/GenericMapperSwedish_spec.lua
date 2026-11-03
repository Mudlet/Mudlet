-- SvenskMUD rooms have no title line, list their exits in Swedish and glue the
-- prompt to the start of the next line of output.
describe("Tests that the generic mapper follows a game in Swedish", function()
  local function upvalue(fn, name)
    local index = 1
    while true do
      local upvalueName, value = debug.getupvalue(fn, index)
      if not upvalueName then
        return nil
      end
      if upvalueName == name then
        return value
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

  local captureMoveCmd = upvalue(map.eventHandler, "capture_move_cmd")
  local function moveQueue()
    return upvalue(captureMoveCmd, "move_queue")
  end

  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local configKeys = {"lang_dirs", "translate", "language", "use_translation", "clear_lines_on_send"}
  local saved, savedPrompt, savedConfigs, savedQueue, dockWasShowing, savedCreateMapper

  -- the base UI reads the prompts fed here as vitals and docks itself on the right
  -- border, leaving the main console narrower for every later spec
  -- prompts with hp/mp labels make the base UI embed a mapper once rooms exist, and an
  -- embedded mapper cannot be closed, so every later openMapWidget() would fail
  setup(function()
    dockWasShowing = BaseUI ~= nil and BaseUI.container ~= nil and not BaseUI.container.hidden
    if BaseUI then
      savedCreateMapper = BaseUI.createMapper
      BaseUI.createMapper = function() end
    end
  end)

  teardown(function()
    if BaseUI then
      BaseUI.createMapper = savedCreateMapper
    end
    if BaseUI and not dockWasShowing and BaseUI.container and not BaseUI.container.hidden then
      BaseUI.container:hide()
      BaseUI.container:adjustBorder()
    end
  end)

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedPrompt = map.save.prompt_pattern[map.character]
    savedConfigs = {}
    for _, key in ipairs(configKeys) do
      savedConfigs[key] = map.configs[key]
    end
    savedQueue = {unpack(moveQueue())}
    -- the mapper saves its configs to the profile, which would outlive the spec
    stub(table, "save")
    map.setLanguage("english")
    map.configs.language = nil
    map.configs.use_translation = true
    map.set("mapping", false)
    map.set("currentRoom", nil)
    map.save.prompt_pattern[map.character] = "^%d+/%d+ %d+/%d+"
    local queue = moveQueue()
    for index = #queue, 1, -1 do
      queue[index] = nil
    end
  end)

  after_each(function()
    for _, key in ipairs(mapperState) do
      map.set(key, saved[key])
    end
    map.save.prompt_pattern[map.character] = savedPrompt
    for _, key in ipairs(configKeys) do
      map.configs[key] = savedConfigs[key]
    end
    table.save:revert()
    local queue = moveQueue()
    for index = #queue, 1, -1 do
      queue[index] = nil
    end
    for index, move in ipairs(savedQueue) do
      queue[index] = move
    end
  end)

  local room = "\n52/52 52/52Du står vid sjön. Norrut ligger byn.\n   Det finns två synliga vägar härifrån: norr och söder\n"

  local function sortedExits()
    local exits = {unpack(map.currentExits)}
    table.sort(exits)
    return exits
  end

  it("names a room by the first sentence of its description and reads its Swedish exits", function()
    feedTriggers("\n52/52 52/52Du står vid stranden till en liten sjö som ligger vid Muddevalla. Norrut ser du\n"
      .. "ena långväggen av kyrkan. Söderut går en liten bro till en holme med ett litet\n"
      .. "hus på. Vägen genom Muddevalla går här.\n"
      .. "   Det finns fyra synliga vägar härifrån: väster, öster, sydöst och ner\n")
    assert.are.equal("Du står vid stranden till en liten sjö som ligger vid Muddevalla.", map.currentName)
    assert.are.same({"down", "east", "southeast", "west"}, sortedExits())
    assert.are.equal("norr", map.configs.lang_dirs.north)
  end)

  it("reads the description beside the ASCII minimap", function()
    feedTriggers("\n52/52 52/52     +###         Du står utanför kyrkan, uppe på toppen av kyrkbacken. Vägen\n"
      .. "   ##|#?#         söderut verkar vara lite ombyggd.\n"
      .. "     +-@K#\n"
      .. "    ###+\n"
      .. "\n"
      .. "                  Det finns fyra synliga vägar härifrån: norr, söder, väster och in\n")
    assert.are.equal("Du står utanför kyrkan, uppe på toppen av kyrkbacken.", map.currentName)
    assert.are.same({"in", "north", "south", "west"}, sortedExits())
  end)

  it("queues Swedish movement commands as the directions they lead", function()
    map.setLanguage("swedish")
    for _, command in ipairs({"nö", "väster", "ner", "ut", "t"}) do
      captureMoveCmd(command)
    end
    assert.are.same({"northeast", "west", "down", "out", "look"}, moveQueue())
  end)

  it("drops the queued move when the game does not understand it or a door is in the way", function()
    map.setLanguage("swedish")
    captureMoveCmd("nv")
    captureMoveCmd("v")
    captureMoveCmd("s")
    feedTriggers("\n52/52 52/52Ursäkta?\n")
    assert.are.same({"west", "south"}, moveQueue())
    feedTriggers("\n52/52 52/52Dörren hindrar dig.\n")
    assert.are.same({"south"}, moveQueue())
    feedTriggers("\nKalle säger: Vadå?\n")
    assert.are.same({"south"}, moveQueue())
  end)

  it("keeps English directions once they were chosen", function()
    map.setLanguage("english")
    feedTriggers(room)
    assert.are.equal("north", map.configs.lang_dirs.north)
  end)

  it("leaves translations the player changed alone", function()
    map.configs.lang_dirs.look = "kolla"
    feedTriggers(room)
    assert.are.equal("kolla", map.configs.lang_dirs.look)
  end)

  it("keeps the queued move when another game answers Eh?", function()
    captureMoveCmd("n")
    feedTriggers("\nEh?\n")
    assert.are.same({"north"}, moveQueue())
  end)

  it("keeps a prompt with no letters out of the name before a prompt pattern is set", function()
    map.save.prompt_pattern[map.character] = nil
    feedTriggers(room)
    assert.are.equal("Du står vid sjön.", map.currentName)
  end)

  it("drops the move when the reply follows a prompt with letters in it", function()
    map.setLanguage("swedish")
    map.save.prompt_pattern[map.character] = "^HP %d+/%d+ SP %d+/%d+"
    captureMoveCmd("nv")
    feedTriggers("\nHP 52/52 SP 52/52Ursäkta?\n")
    assert.are.same({}, moveQueue())
    map.save.prompt_pattern[map.character] = "^%a+:%d+ >"
    captureMoveCmd("nv")
    feedTriggers("\nHälsa:52 >Ursäkta?\n")
    assert.are.same({}, moveQueue())
  end)

  it("links a walk there and back to the room it started in", function()
    local areaId = addAreaName("genericMapperSwedishSpec")
    finally(function() deleteArea(areaId) end)
    local start = createRoomID()
    addRoom(start)
    setRoomArea(start, areaId)
    setRoomCoordinates(start, 0, 0, 0)
    setRoomName(start, "Du står vid sjön.")
    map.setLanguage("swedish")
    map.set("currentRoom", start)
    map.set("currentArea", areaId)
    map.set("currentName", "Du står vid sjön.")
    map.set("currentExits", {"north"})
    map.set("mapping", true)
    map.prompt.hash = nil

    captureMoveCmd("n")
    feedTriggers("\n52/52 52/52Du står i byn. Söderut ligger sjön.\n   Det finns en synlig väg härifrån: söder\n")
    local village = map.currentRoom
    assert.are_not.equal(start, village)
    assert.are.equal("Du står i byn.", getRoomName(village))
    assert.are.equal(village, getRoomExits(start).north)

    captureMoveCmd("s")
    feedTriggers("\n52/52 52/52Du står vid sjön. Norrut ligger byn.\n   Det finns en synlig väg härifrån: norr\n")
    assert.are.equal(start, map.currentRoom)
    assert.are.equal(start, getRoomExits(village).south)
    assert.are.equal(2, table.size(getAreaRooms(areaId)))
  end)

  -- the prompt bookkeeping lives in upvalues that outlast a single spec
  local function resetPromptState()
    local function set(fn, name, value)
      local index = 1
      while true do
        local upvalueName = debug.getupvalue(fn, index)
        assert(upvalueName, "the mapper keeps no " .. name)
        if upvalueName == name then
          debug.setupvalue(fn, index, value)
          return
        end
        index = index + 1
      end
    end
    local function reset()
      set(map.learnPromptTemplate, "awaiting_prompt_reply", false)
      set(map.captureUntitledRoom, "untitled_room_seen", false)
      upvalue(map.captureUntitledRoom, "asked_for_prompt")[map.character or ""] = nil
    end
    reset()
    finally(reset)
  end

  local function countPromptSends()
    local realSend = _G.send
    local count = {prompt = 0}
    _G.send = function(command, ...)
      if command == "prompt" then
        count.prompt = count.prompt + 1
      end
      return realSend(command, ...)
    end
    finally(function() _G.send = realSend end)
    return count
  end

  it("learns the prompt from the template the game reports", function()
    resetPromptState()
    send("prompt", false)
    feedTriggers('\n52/52 52/52Du sätter din prompt till "<HP>/<MAXHP>hp <MP>/<MAXMP>mp".\n')
    local learned = map.save.prompt_pattern[map.character]
    assert.truthy(string.match("96/96hp 96/96mp", learned))
    assert.truthy(string.match("-5/96hp 96/96mp", learned))
    assert.falsy(string.match("96/96 96/96", learned))

    send("prompt", false)
    feedTriggers('\n96/96hp 96/96mpDin nuvarande prompt är "[<HP>] (<NAMN>) 5%.".\n')
    learned = map.save.prompt_pattern[map.character]
    assert.truthy(string.match("[52] (Kalle) 5%.", learned))
    assert.falsy(string.match("52 Kalle 5", learned))
  end)

  it("ignores the prompt reply when nobody asked for it", function()
    resetPromptState()
    feedTriggers('\nKalle säger: Du sätter din prompt till "<HP>".\n')
    assert.are.equal("^%d+/%d+ %d+/%d+", map.save.prompt_pattern[map.character])
  end)

  it("forgets the prompt when the game reports an empty one", function()
    resetPromptState()
    send("prompt", false)
    feedTriggers('\nDu sätter din prompt till "".\n')
    assert.is_nil(map.save.prompt_pattern[map.character])
  end)

  it("keeps the saved prompt when the template ends in a word it cannot tell from the room", function()
    resetPromptState()
    send("prompt", false)
    feedTriggers('\nDu sätter din prompt till "<HP> <NAMN>".\n')
    assert.are.equal("^%d+/%d+ %d+/%d+", map.save.prompt_pattern[map.character])
  end)

  it("names a room when a learned prompt with letters is glued to it", function()
    map.save.prompt_pattern[map.character] = "^%-?%d+/%-?%d+hp %-?%d+/%-?%d+mp"
    feedTriggers("\n96/96hp 96/96mpDu står vid ängen. Söderut ligger sjön.\n   Det finns en synlig väg härifrån: söder\n")
    assert.are.equal("Du står vid ängen.", map.currentName)
  end)

  it("announces a new room at the changed prompt that follows it", function()
    resetPromptState()
    local areaId = addAreaName("genericMapperSwedishPromptSpec")
    finally(function() deleteArea(areaId) end)
    local start = createRoomID()
    addRoom(start)
    setRoomArea(start, areaId)
    setRoomCoordinates(start, 0, 0, 0)
    setRoomName(start, "Du står vid sjön.")
    map.setLanguage("swedish")
    map.set("currentRoom", start)
    map.set("currentArea", areaId)
    map.set("currentName", "Du står vid sjön.")
    map.set("currentExits", {"north"})
    map.set("mapping", true)
    map.prompt.hash = nil

    local function announced()
      local last = getLastLineNumber("main")
      for _, text in ipairs(getLines("main", math.max(0, last - 10), last + 1)) do
        if text:find("New Room: Du står vid ängen.", 1, true) then
          return true
        end
      end
      return false
    end

    send("prompt", false)
    feedTriggers('\n52/52 52/52Du sätter din prompt till "<HP>/<MAXHP>hp <MP>/<MAXMP>mp".\n')
    captureMoveCmd("n")
    feedTriggers("\n96/96hp 96/96mp\nDu står vid ängen. Söderut ligger sjön.\n   Det finns en synlig väg härifrån: söder\n")
    assert.is_false(announced())
    feedTriggers("96/96hp 96/96mp\n")
    assert.is_true(announced())
  end)

  it("asks the game for its prompt once when it has none, after naming the room", function()
    resetPromptState()
    map.save.prompt_pattern[map.character] = nil
    map.configs.clear_lines_on_send = true
    local sent = countPromptSends()
    feedTriggers(room)
    assert.are.equal("Du står vid sjön.", map.currentName)
    feedTriggers(room)
    assert.are.equal(1, sent.prompt)
  end)

  it("asks the game for its prompt again after reconnecting", function()
    resetPromptState()
    map.save.prompt_pattern[map.character] = nil
    local sent = countPromptSends()
    feedTriggers(room)
    map.eventHandler("sysConnectionEvent")
    feedTriggers(room)
    assert.are.equal(2, sent.prompt)
  end)

  it("asks the game for its prompt again once the saved one stops showing", function()
    resetPromptState()
    local sent = countPromptSends()
    local function walk(prompt)
      feedTriggers("\nDu står vid sjön. Norrut ligger byn.\n   Det finns två synliga vägar härifrån: norr och söder\n" .. prompt .. "\n")
    end
    walk("52/52 52/52")
    walk("52/52 52/52")
    assert.are.equal(0, sent.prompt)
    walk("96/96hp 96/96mp")
    walk("96/96hp 96/96mp")
    assert.are.equal(1, sent.prompt)
  end)
end)
