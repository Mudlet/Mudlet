-- A console that reaches its line limit drops its oldest lines in one batch, which
-- moves every line left up. A script's cursor and selection name a line by number,
-- so they have to move with it.
describe("Tests that the cursor and the selection follow their line when the buffer trims", function()

  local function countTrims(consoleName)
    local trimmed = {lines = 0}
    trimmed.handler = registerAnonymousEventHandler("sysBufferShrinkEvent", function(_, name, removed)
      if name == consoleName then
        trimmed.lines = trimmed.lines + removed
      end
    end)
    return trimmed
  end

  describe("in the main console", function()
    it("leaves getLineNumber() on the line a feed added when that feed trims the buffer", function()
      local limit = getConsoleBufferSize("main")
      if limit > 100000 then
        pending("the main console keeps more lines than this spec can feed in good time")
        return
      end
      local trimmed = countTrims("main")
      finally(function() killAnonymousEventHandler(trimmed.handler) end)

      local fed = 0
      while trimmed.lines == 0 and fed <= limit do
        fed = fed + 1
        feedTriggers("trim cursor feed " .. fed .. "\n")
      end

      assert.is_true(trimmed.lines > 0, "the main console did not trim")
      assert.are.equal("trim cursor feed " .. fed, getCurrentLine())
    end)
  end)

  describe("in a mini console", function()
    local window = ("specBufferTrimCursor-%d-%d"):format(os.time(), math.random(100000))
    local trimmed

    before_each(function()
      assert.is_true(createMiniConsole(window, 0, 0, 300, 100))
      setConsoleBufferSize(window, 100, 10)
      clearWindow(window)
      for i = 1, 60 do
        echo(window, "kept line " .. i .. "\n")
      end
      trimmed = countTrims(window)
    end)

    after_each(function()
      killAnonymousEventHandler(trimmed.handler)
      deleteMiniConsole(window)
    end)

    local function echoUntilTrimmed()
      local echoed = 0
      while trimmed.lines == 0 and echoed <= 100 do
        echoed = echoed + 1
        echo(window, "more line " .. echoed .. "\n")
      end
      assert.is_true(trimmed.lines > 0, "the mini console did not trim")
    end

    it("keeps a cursor moveCursor() placed on the line it was on", function()
      local line = getLastLineNumber(window) - 30
      assert.is_true(moveCursor(window, 0, line))
      local text = getCurrentLine(window)

      echoUntilTrimmed()

      assert.are.equal(line - trimmed.lines, getLineNumber(window))
      assert.are.equal(text, getCurrentLine(window))
    end)

    it("keeps a selection on the text it selected", function()
      local line = getLastLineNumber(window) - 30
      assert.is_true(moveCursor(window, 0, line))
      local text = getCurrentLine(window)
      assert.is_true(selectString(window, text, 1) >= 0)

      echoUntilTrimmed()

      local selected, start, length = getSelection(window)
      assert.are.same({text, 0, #text}, {selected, start, length})
    end)

    it("moves a cursor and drops a selection whose line was trimmed away", function()
      assert.is_true(moveCursor(window, 3, 2))
      assert.is_true(selectString(window, getCurrentLine(window), 1) >= 0)

      echoUntilTrimmed()

      assert.are.equal(0, getLineNumber(window))
      assert.are.same({"", 0, 0}, {getSelection(window)})
    end)
  end)
end)
