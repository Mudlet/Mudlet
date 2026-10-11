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

  local mapperState = {"mapping", "currentRoom", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local saved, savedPrompt, savedConfigs, savedQueue

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedPrompt = map.save.prompt_pattern[map.character]
    savedConfigs = {lang_dirs = map.configs.lang_dirs, translate = map.configs.translate,
      language = map.configs.language, use_translation = map.configs.use_translation}
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
    for key, value in pairs(savedConfigs) do
      map.configs[key] = value
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
  end)
end)
