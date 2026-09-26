-- The C++ TCommandLine widget, reached through the command line Lua API. The
-- Geyser wrapper around a sub command line is covered in
-- GeyserCommandLine_spec.lua.

-- Hidden input cannot be asked for from Lua: the game server takes the ECHO
-- option, so the real telnet parser is what has to be fed to reach it. It can be
-- turned off, by the profile's "Show passwords as you type them" preference,
-- which the self-test profile leaves alone.
local echoActive = false
local function serverEcho(takesEcho)
  local ok, msg = feedTelnet(takesEcho and "<T_IAC><T_WILL><O_ECHO>" or "<T_IAC><T_WONT><O_ECHO>")
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  echoActive = takesEcho
end

-- getCmdLine reads the command line and never the hidden-input box over it
describe("Tests the functionality of the main command line while the server asks for hidden input", function()
  -- cTelnet stops answering ECHO once five negotiations arrive with less than
  -- five seconds between consecutive ones, and it restarts that window on every
  -- one, so spacing prompts out does not help. A prompt costs two negotiations,
  -- so the tests below use four of the five: a third prompt needs a five second
  -- wait in front of it, or the hidden input it asks for never happens.
  -- https://github.com/Mudlet/Mudlet/issues/10367
  after_each(function()
    -- the ECHO state is process wide, so a prompt left open by a failed
    -- assertion would follow every later spec file
    if echoActive then
      serverEcho(false)
    end
    clearCmdLine("main")
  end)

  it("keeps a left-over command in the command line while the game asks for hidden input", function()
    -- what the profile does at a password prompt when auto-clear is off: the
    -- command that was just sent is still there, selected
    printCmdLine("main", "specCommandUnderPassword")
    selectCmdLineText("main")
    assert.are.equal("specCommandUnderPassword", getCmdLine("main"))

    -- a sub command line is not the main one, so the same prompt doubles as
    -- its control
    createCommandLine("specPasswordSubCmdLine", 0, 0, 200, 30)
    finally(function() deleteCommandLine("specPasswordSubCmdLine") end)
    printCmdLine("specPasswordSubCmdLine", "specSubCommandLineText")

    serverEcho(true)
    assert.are.equal("specCommandUnderPassword", getCmdLine("main"), "the prompt for hidden input touched the command that was left in the command line")
    assert.are.equal("specSubCommandLineText", getCmdLine("specPasswordSubCmdLine"),
                     "the prompt for hidden input emptied a sub command line, which it has no business touching")

    serverEcho(false)
    assert.are.equal("specCommandUnderPassword", getCmdLine("main"), "the command left in the command line did not survive the prompt ending")
  end)

  it("does not leave the password behind when the prompt ends", function()
    clearCmdLine("main")
    serverEcho(true)

    -- goes into the hidden-input box
    printCmdLine("main", "specSecretPassword")
    assert.are.equal("", getCmdLine("main"), "text written to the main command line during a prompt for hidden input was readable from the command line")

    serverEcho(false)
    assert.are.equal("", getCmdLine("main"), "the password was readable in the command line after the prompt ended")
  end)
end)

describe("Tests the functionality of the main command line writers with no prompt open", function()
  after_each(function()
    clearCmdLine("main")
  end)

  it("printCmdLine, appendCmdLine, selectCmdLineText and clearCmdLine reach the main command line", function()
    printCmdLine("main", "specWritten")
    assert.are.equal("specWritten", getCmdLine("main"))
    appendCmdLine("main", "Twice")
    assert.are.equal("specWrittenTwice", getCmdLine("main"))
    assert.is_true(selectCmdLineText("main"))
    clearCmdLine("main")
    assert.are.equal("", getCmdLine("main"))
  end)

  it("treats a lone argument as the text for the main command line", function()
    printCmdLine("specLoneArgument")
    assert.are.equal("specLoneArgument", getCmdLine())
    appendCmdLine("!")
    assert.are.equal("specLoneArgument!", getCmdLine())
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
