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
