-- echo() to the main console: what it answers, and where its text lands in the
-- buffer, outside a trigger and from inside one.
describe("Tests echo() to the main console", function()
  local function assertArgError(fn, needle)
    local ok, err = pcall(fn)
    assert.is_false(ok)
    assert.is_truthy(tostring(err):find(needle, 1, true), tostring(err))
  end

  local function lastLines(count)
    local last = getLastLineNumber("main")
    return getLines("main", last - count + 1, last + 1)
  end

  -- where a line of the main console is, looking only at the last few lines
  local function findRecentLine(needle)
    local last = getLastLineNumber("main")
    local first = math.max(0, last - 10)
    for i, line in ipairs(getLines("main", first, last + 1)) do
      if line:find(needle, 1, true) then
        return first + i - 1, line
      end
    end
    return nil
  end

  -- starts every spec on an empty last line, whatever the one before left open
  before_each(function()
    echo("\n")
  end)

  after_each(function()
    moveCursorEnd("main")
  end)

  it("answers true, and nothing else, by every name of the main console", function()
    assert.are.same({true}, {echo("SpecMainEchoNoName\n")})
    assert.are.same({true}, {echo("main", "SpecMainEchoMain\n")})
    assert.are.same({true}, {echo("", "SpecMainEchoEmpty\n")})
    assert.are.same({"SpecMainEchoNoName", "SpecMainEchoMain", "SpecMainEchoEmpty", ""}, lastLines(4))
  end)

  it("raises a Lua error for text that is not a string", function()
    assertArgError(function() echo() end, "echo: bad argument #1 type (text to display as string expected")
    assertArgError(function() echo("main", {}) end, "echo: bad argument #2 type (text to display as string expected")
    assertArgError(function() echo({}, "text") end, "echo: bad argument #1 type (console name as string is optional")
  end)

  it("reports a console that does not exist", function()
    assert.are.same({nil, "console/label 'SpecMainEchoNoSuchConsole' does not exist"}, {echo("SpecMainEchoNoSuchConsole", "text")})
  end)

  it("drops carriage returns, so \\r\\n ends a line and a lone \\r vanishes", function()
    echo("SpecCr one\r\nSpecCr t\rwo\n")
    assert.are.same({"SpecCr one", "SpecCr two", ""}, lastLines(3))
  end)

  it("leaves a line without a line feed open for the next echo", function()
    echo("SpecOpen first, ")
    assert.are.same({"SpecOpen first, "}, lastLines(1))
    echo("second\n")
    assert.are.same({"SpecOpen first, second", ""}, lastLines(2))
  end)

  it("appends at the end of the buffer wherever the cursor is", function()
    echo("SpecCursor above\n")
    local above = getLastLineNumber("main") - 1
    moveCursor(0, above)
    echo("SpecCursor appended\n")
    assert.are.same({"SpecCursor above", "SpecCursor appended", ""}, lastLines(3))
    assert.are.equal(above, getLineNumber(), "echo() moved the cursor")
  end)

  it("puts text after a prompt the server ended with IAC GA on a line of its own", function()
    local ok, msg = feedTelnet("SpecEchoPrompt> <T_IAC><T_GA>")
    -- ends the prompt for the telnet parser too, whatever the assertions do
    finally(function() feedTelnet("\r\n") end)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    echo("SpecAfterPrompt\n")
    assert.are.same({"SpecEchoPrompt> ", "SpecAfterPrompt", ""}, lastLines(3))
    moveCursor(0, getLastLineNumber("main") - 2)
    assert.is_true(isPrompt(), "the prompt's line lost its prompt mark")
    moveCursor(0, getLastLineNumber("main") - 1)
    assert.is_false(isPrompt(), "the echoed line took the prompt mark")
  end)

  describe("from inside a trigger", function()
    local triggerId

    after_each(function()
      if triggerId then
        killTrigger(triggerId)
        triggerId = nil
      end
    end)

    it("adds to the line the trigger runs on, rather than after it", function()
      triggerId = tempRegexTrigger("^SpecTriggerEchoLine$", function()
        echo(" ta\ril")
      end)
      feedTriggers("SpecTriggerEchoLine\nSpecTriggerEchoNext\n")
      local at, line = findRecentLine("SpecTriggerEchoLine")
      assert.are.equal("SpecTriggerEchoLine tail", line)
      assert.are.equal("SpecTriggerEchoNext", getLines("main", at + 1, at + 2)[1])
    end)

    it("still answers true", function()
      triggerId = tempRegexTrigger("^SpecTriggerEchoAnswer$", function()
        _G.SpecTriggerEchoAnswer = {echo(" [answered]")}
      end)
      feedTriggers("SpecTriggerEchoAnswer\n")
      local answer = _G.SpecTriggerEchoAnswer
      _G.SpecTriggerEchoAnswer = nil
      assert.are.same({true}, answer)
    end)
  end)
end)
