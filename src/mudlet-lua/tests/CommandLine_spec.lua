-- The C++ TCommandLine widget, reached through the command line Lua API. The
-- Geyser wrapper around a sub command line is covered in
-- GeyserCommandLine_spec.lua.

-- Password mode cannot be turned on from Lua: the game server takes the ECHO
-- option, so the real telnet parser is what has to be fed to reach it. It can be
-- turned off, by the profile's "disable password masking" preference, which the
-- self-test profile leaves alone.
local echoActive = false
local function serverEcho(takesEcho)
  local ok, msg = feedTelnet(takesEcho and "<T_IAC><T_WILL><O_ECHO>" or "<T_IAC><T_WONT><O_ECHO>")
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  echoActive = takesEcho
end

-- These pin what becomes of the surrounding text at a password prompt. The
-- masking itself is painted over the document by TCommandLine::paintEvent, so
-- getCmdLine still reads the password back and no spec can see it happen.
describe("Tests the functionality of the main command line at a server password prompt", function()
  -- cTelnet stops answering ECHO once five negotiations arrive with less than
  -- five seconds between consecutive ones, and it restarts that window on every
  -- one, so spacing prompts out does not help. A prompt costs two negotiations,
  -- so the tests below use four of the five: a third prompt needs a five second
  -- wait in front of it, or the masking it asks for never happens.
  -- https://github.com/Mudlet/Mudlet/issues/10367
  after_each(function()
    -- the suppression is process wide, so a prompt left open by a failed
    -- assertion would follow every later spec file
    if echoActive then
      serverEcho(false)
    end
    clearCmdLine("main")
  end)

  it("puts the command aside while the server asks for a password, and gives it back", function()
    -- what the profile does at a password prompt when auto-clear is off: the
    -- command that was just sent is still there, selected. Only the text is
    -- readable from Lua, so what the selection itself becomes goes unpinned
    printCmdLine("main", "specCommandUnderPassword")
    selectCmdLineText("main")
    assert.are.equal("specCommandUnderPassword", getCmdLine("main"))

    -- a sub command line is exempt from echo suppression, so the same prompt
    -- doubles as its control
    createCommandLine("specPasswordSubCmdLine", 0, 0, 200, 30)
    finally(function() deleteCommandLine("specPasswordSubCmdLine") end)
    printCmdLine("specPasswordSubCmdLine", "specSubCommandLineText")

    serverEcho(true)
    assert.are.equal("", getCmdLine("main"), "the typed command was left on screen for the password prompt")
    assert.are.equal("specSubCommandLineText", getCmdLine("specPasswordSubCmdLine"),
                     "the password prompt emptied a sub command line, which it has no business touching")

    serverEcho(false)
    assert.are.equal("specCommandUnderPassword", getCmdLine("main"), "the typed command was not put back when the password prompt ended")
  end)

  it("does not leave the password behind when the prompt ends", function()
    clearCmdLine("main")
    serverEcho(true)

    printCmdLine("main", "specSecretPassword")
    assert.are.equal("specSecretPassword", getCmdLine("main"))

    serverEcho(false)
    assert.are.equal("", getCmdLine("main"), "the password was still readable in the command line after the prompt ended")
  end)
end)

-- Neither the suggestion list nor the tab completion blacklist can be read back
-- from Lua, and only a Tab keypress consumes them, so what these pin is which
-- argument the command line name is taken from: one argument is the word for
-- the main command line, two make the first one a name.
describe("Tests the functionality of addCmdLineSuggestion, removeCmdLineSuggestion, addCmdLineBlacklist and removeCmdLineBlacklist", function()
  local missing = "mudlet-spec-no-such-command-line"
  local functions = {"addCmdLineSuggestion", "removeCmdLineSuggestion", "addCmdLineBlacklist", "removeCmdLineBlacklist"}

  teardown(function()
    clearCmdLineSuggestions("main")
    clearCmdLineBlacklist("main")
  end)

  for _, name in ipairs(functions) do
    it(name .. " takes a lone argument as the word for the main command line", function()
      assert.are.equal(0, select("#", _G[name](missing)))
    end)

    it(name .. " takes the first of two arguments as the command line name", function()
      local ok, err = _G[name](missing, "specWord")
      assert.is_nil(ok)
      assert.are.equal('command line "' .. missing .. '" not found', err)
    end)

    it(name .. " raises a Lua error when the command line name is not a string", function()
      local ok, err = pcall(_G[name], {}, "specWord")
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find(name .. ": bad argument #1 type (command line name as string expected, got table)!", 1, true), tostring(err))
    end)
  end

  -- clearCmdLineSuggestions and clearCmdLineBlacklist read their arguments the
  -- other way round, so the lone-argument call that silently adds a word above
  -- is a named lookup here
  for _, name in ipairs({"clearCmdLineSuggestions", "clearCmdLineBlacklist"}) do
    it(name .. " takes a lone argument as the command line name, unlike its add and remove siblings", function()
      local ok, err = _G[name](missing)
      assert.is_nil(ok)
      assert.are.equal('command line "' .. missing .. '" not found', err)
    end)
  end
end)

-- Every one of these finds its command line by name: an empty name or "main"
-- is the main command line, and any other is one made by createCommandLine()
-- or a mini console's own. Every kind of window shares the one name space, so
-- a name that belongs to some other kind must not be taken for a command line.
describe("Tests that the command line functions find their command line by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local cmdLine = "specByNameCmdLine" .. suffix
  local unknown = "specByNameNoSuchLine" .. suffix
  local menuLabel = "specByNameMenuLabel" .. suffix

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local calls = {
    {"getCmdLine", function(n) return getCmdLine(n) end},
    {"printCmdLine", function(n) return printCmdLine(n, "text") end},
    {"appendCmdLine", function(n) return appendCmdLine(n, "text") end},
    {"clearCmdLine", function(n) return clearCmdLine(n) end},
    {"selectCmdLineText", function(n) return selectCmdLineText(n) end},
    {"addCmdLineSuggestion", function(n) return addCmdLineSuggestion(n, "word") end},
    {"removeCmdLineSuggestion", function(n) return removeCmdLineSuggestion(n, "word") end},
    {"clearCmdLineSuggestions", function(n) return clearCmdLineSuggestions(n) end},
    {"addCmdLineBlacklist", function(n) return addCmdLineBlacklist(n, "word") end},
    {"removeCmdLineBlacklist", function(n) return removeCmdLineBlacklist(n, "word") end},
    {"clearCmdLineBlacklist", function(n) return clearCmdLineBlacklist(n) end},
    {"addCommandLineMenuEvent", function(n) return addCommandLineMenuEvent(n, menuLabel, "event") end},
    {"removeCommandLineMenuEvent", function(n) return removeCommandLineMenuEvent(n, menuLabel) end},
    {"getSaveCommandHistory", function(n) return getSaveCommandHistory(n) end},
    {"setSaveCommandHistory", function(n) return setSaveCommandHistory(n, true) end},
    {"enableCommandLine", function(n) return enableCommandLine(n) end},
    {"disableCommandLine", function(n) return disableCommandLine(n) end},
  }

  -- these two take a console's name for the command line at its foot, so a
  -- mini console is not "some other kind of window" to them
  local takesAConsole = {enableCommandLine = true, disableCommandLine = true}

  local function assertRefusesAll(windowName, skip)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      if not (skip and skip[functionName]) then
        assert.are.equal(2, select("#", call(windowName)), functionName)
        local ok, err = call(windowName)
        assert.is_nil(ok, functionName)
        assert.are.equal(('command line "%s" not found'):format(windowName), err, functionName)
      end
    end
  end

  setup(function()
    createCommandLine(cmdLine, 10, 10, 150, 30)
  end)

  teardown(function()
    deleteCommandLine(cmdLine)
  end)

  after_each(function()
    clearCmdLine("main")
    removeCommandLineMenuEvent("main", menuLabel)
  end)

  it("refuses a name that is no window at all", function()
    assertRefusesAll(unknown)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specByNameOtherWindow" .. suffix

    after_each(function()
      deleteLabel(otherName)
      deleteScrollBox(otherName)
      deleteTextEdit(otherName)
      deleteMiniConsole(otherName)
    end)

    local kinds = {
      {"label", function() return createLabel(otherName, 0, 0, 50, 20, 1) end},
      {"scroll box", function() return createScrollBox(otherName, 0, 0, 50, 20) end},
      {"text edit", function() return createTextEdit("main", otherName, 0, 0, 50, 20) end},
    }

    for _, kind in ipairs(kinds) do
      it("refuses every command line function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefusesAll(otherName)
      end)
    end

    it("refuses a mini console that has no command line of its own", function()
      createMiniConsole(otherName, 0, 0, 50, 20)
      assertRefusesAll(otherName, takesAConsole)
    end)

    it("reaches a mini console's own command line by the mini console's name", function()
      createMiniConsole(otherName, 0, 0, 200, 50)
      assert.is_true(enableCommandLine(otherName))
      printCmdLine(otherName, "in the mini console")
      appendCmdLine(otherName, "!")
      assert.are.equal("in the mini console!", getCmdLine(otherName))
      assert.are.equal("", getCmdLine("main"))
      assert.is_true(selectCmdLineText(otherName))
      clearCmdLine(otherName)
      assert.are.equal("", getCmdLine(otherName))
      assert.is_true(disableCommandLine(otherName))
    end)
  end)

  it("takes an empty name, like \"main\", for the main command line", function()
    printCmdLine("", "via the empty name")
    assert.are.equal("via the empty name", getCmdLine("main"))
    assert.are.equal("via the empty name", getCmdLine(""))
    appendCmdLine("", "!")
    assert.are.equal("via the empty name!", getCmdLine("main"))
    assert.is_true(selectCmdLineText(""))
    clearCmdLine("")
    assert.are.equal("", getCmdLine("main"))

    for _, name in ipairs({"addCmdLineSuggestion", "removeCmdLineSuggestion", "addCmdLineBlacklist", "removeCmdLineBlacklist"}) do
      assert.are.equal(0, select("#", _G[name]("", "specByNameWord")), name)
    end
    assert.are.equal(0, select("#", clearCmdLineSuggestions("")))
    assert.are.equal(0, select("#", clearCmdLineBlacklist("")))

    assert.is_true(addCommandLineMenuEvent("", menuLabel, "event"))
    assert.is_true(removeCommandLineMenuEvent("main", menuLabel))
    assert.is_true(addCommandLineMenuEvent("main", menuLabel, "event"))
    assert.is_true(removeCommandLineMenuEvent("", menuLabel))

    local original = getSaveCommandHistory()
    finally(function() setSaveCommandHistory("main", original) end)
    assert.is_true(setSaveCommandHistory("", false))
    assert.is_false((getSaveCommandHistory("main")))
    assert.is_true(setSaveCommandHistory("main", true))
    assert.is_true((getSaveCommandHistory("")))
  end)

  it("leaves the main command line alone when given another one's name", function()
    printCmdLine("main", "main text")
    printCmdLine(cmdLine, "named")
    assert.are.equal("named", getCmdLine(cmdLine))
    appendCmdLine(cmdLine, " text")
    assert.are.equal("named text", getCmdLine(cmdLine))
    clearCmdLine(cmdLine)
    assert.are.equal("", getCmdLine(cmdLine))
    assert.are.equal("main text", getCmdLine("main"))

    assert.is_true(addCommandLineMenuEvent(cmdLine, menuLabel, "event"))
    local ok, err = removeCommandLineMenuEvent("main", menuLabel)
    assert.is_false(ok)
    assert.are.equal(("removeCommandLineMenuEvent: cannot remove '%s', menu item does not exist"):format(menuLabel), err)
    assert.is_true(removeCommandLineMenuEvent(cmdLine, menuLabel))
    ok, err = removeCommandLineMenuEvent(cmdLine, menuLabel)
    assert.is_false(ok)
    assert.are.equal(("removeCommandLineMenuEvent: cannot remove '%s', menu item does not exist"):format(menuLabel), err)
  end)

  -- the arguments are all checked before the command line is looked for, so a
  -- bad one is raised even for a command line that is not there
  local badArguments = {
    {"printCmdLine", function() return printCmdLine(unknown, {}) end},
    {"appendCmdLine", function() return appendCmdLine(unknown, {}) end},
    {"addCmdLineSuggestion", function() return addCmdLineSuggestion(unknown, {}) end},
    {"removeCmdLineSuggestion", function() return removeCmdLineSuggestion(unknown, {}) end},
    {"addCmdLineBlacklist", function() return addCmdLineBlacklist(unknown, {}) end},
    {"removeCmdLineBlacklist", function() return removeCmdLineBlacklist(unknown, {}) end},
    {"addCommandLineMenuEvent", function() return addCommandLineMenuEvent(unknown, {}, "event") end},
    {"addCommandLineMenuEvent", function() return addCommandLineMenuEvent(unknown, "label", {}) end, 3},
    {"removeCommandLineMenuEvent", function() return removeCommandLineMenuEvent(unknown, {}) end},
    {"setSaveCommandHistory", function() return setSaveCommandHistory(unknown, "yes") end},
  }
  for _, entry in ipairs(badArguments) do
    local functionName, call, position = entry[1], entry[2], entry[3] or 2
    it(("%s raises a bad argument #%d ahead of an unknown command line"):format(functionName, position), function()
      local ok, err = pcall(call)
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find(("%s: bad argument #%d type"):format(functionName, position), 1, true), tostring(err))
    end)
  end

  it("reports history saving turned off for the profile ahead of an unknown command line", function()
    local savedLines = getConfig("commandLineHistorySaveSize")
    finally(function() setConfig("commandLineHistorySaveSize", savedLines) end)
    setConfig("commandLineHistorySaveSize", 0)

    local saving, message = getSaveCommandHistory(unknown)
    assert.is_false(saving)
    assert.are.equal("disabled by profile global preference", message)
    local ok, err = setSaveCommandHistory(unknown, true)
    assert.is_nil(ok)
    assert.are.equal("disabled by profile global preference", err)
  end)
end)
