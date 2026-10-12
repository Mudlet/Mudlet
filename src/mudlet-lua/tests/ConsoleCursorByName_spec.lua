-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console.
describe("Tests that the cursor and line functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specCursorByNameNoSuchWindow" .. suffix
  local lines = {"first line", "second line", "third line"}

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local calls = {
    {"moveCursor", function(n) return moveCursor(n, 0, 0) end},
    {"moveCursorEnd", function(n) return moveCursorEnd(n) end},
    {"getLineNumber", function(n) return getLineNumber(n) end},
    {"getLineCount", function(n) return getLineCount(n) end},
    {"getColumnNumber", function(n) return getColumnNumber(n) end},
    {"getColumnCount", function(n) return getColumnCount(n) end},
    {"getRowCount", function(n) return getRowCount(n) end},
    {"deleteLine", function(n) return deleteLine(n) end},
  }

  local function assertRefusesAll(windowName)
    local notFound = ('window "%s" not found'):format(windowName)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      assert.are.equal(2, select("#", call(windowName)), functionName)
      local ok, err = call(windowName)
      assert.is_nil(ok, functionName)
      assert.are.equal(notFound, err, functionName)
    end
    -- these two answer something other than nil for a console they cannot find
    assert.are.same({-1}, {getLastLineNumber(windowName)})
    assert.are.same({"ERROR: mini console does not exist", notFound}, {getCurrentLine(windowName)})
  end

  local function mainCursor()
    return {getLineNumber("main"), getColumnNumber("main")}
  end

  -- where a line fed to the main console landed, looking only at the last few
  -- lines, or nil when it is not there
  local function findRecentMainLine(wanted)
    local last = getLastLineNumber("main")
    local first = math.max(0, last - 5)
    for i, line in ipairs(getLines("main", first, last + 1)) do
      if line == wanted then
        return first + i - 1
      end
    end
    return nil
  end

  it("refuses a name that is no window at all", function()
    assertRefusesAll(unknown)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specCursorByNameOtherWindow" .. suffix

    after_each(function()
      deleteLabel(otherName)
      deleteScrollBox(otherName)
      deleteTextEdit(otherName)
      deleteCommandLine(otherName)
    end)

    local kinds = {
      {"label", function() return createLabel(otherName, 0, 0, 50, 20, 1) end},
      {"scroll box", function() return createScrollBox(otherName, 0, 0, 50, 20) end},
      {"text edit", function() return createTextEdit("main", otherName, 0, 0, 50, 20) end},
      {"command line", function() return createCommandLine(otherName, 0, 0, 50, 20) end},
    }

    for _, kind in ipairs(kinds) do
      it("refuses every cursor and line function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefusesAll(otherName)
      end)
    end
  end)

  it("checks the arguments before it looks for the console", function()
    for _, call in ipairs({
      function() moveCursor({}, 0, 0) end,
      function() moveCursor(unknown, "left", 0) end,
      function() moveCursor(unknown, 0, "top") end,
      function() moveCursor("left", 0) end,
      function() moveCursor(0, "top") end,
      function() moveCursorEnd({}) end,
      function() getLineNumber({}) end,
      function() getLineCount({}) end,
      function() getLastLineNumber({}) end,
      function() getColumnNumber({}) end,
      function() getCurrentLine({}) end,
      function() getColumnCount({}) end,
      function() getRowCount({}) end,
      function() deleteLine({}) end,
    }) do
      assert.has_error(call)
    end
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 100) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height, so this one is only closed and left behind
      {"user window", function(name) return openUserWindow(name, false) end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specCursorByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        before_each(function()
          clearWindow(window)
          echo(window, table.concat(lines, "\n"))
        end)

        it("moves and reads that console's cursor", function()
          local before = mainCursor()

          assert.are.equal(2, getLineCount(window))
          assert.are.equal(2, getLastLineNumber(window))
          assert.is_true(moveCursor(window, 4, 1))
          assert.are.equal(1, getLineNumber(window))
          assert.are.equal(4, getColumnNumber(window))
          assert.are.equal("second line", getCurrentLine(window))

          assert.is_false(moveCursor(window, 0, 3))
          assert.is_false(moveCursor(window, 0, -1))
          assert.are.equal(1, getLineNumber(window))
          assert.are.equal(4, getColumnNumber(window))

          assert.are.equal(0, select("#", moveCursorEnd(window)))
          assert.are.equal(2, getLineNumber(window))
          assert.are.equal(#"third line" - 1, getColumnNumber(window))
          assert.are.equal("third line", getCurrentLine(window))

          assert.are.same(before, mainCursor())
        end)

        it("deletes the line under that console's cursor", function()
          local mainLines = getLineCount("main")

          assert.is_true(moveCursor(window, 0, 1))
          assert.are.equal(0, select("#", deleteLine(window)))
          assert.are.equal(1, getLineCount(window))
          assert.are.same({"first line", "third line"}, getLines(window, 0, 2))
          assert.are.equal(mainLines, getLineCount("main"))
        end)

        it("measures that console", function()
          assert.is_number(getColumnCount(window))
          assert.is_number(getRowCount(window))
        end)
      end)
    end

    describe("of a mini console being resized", function()
      local window = "specCursorByName resized" .. suffix

      setup(function()
        assert.is_true(createMiniConsole(window, 0, 0, 200, 100))
      end)

      teardown(function()
        deleteMiniConsole(window)
      end)

      it("measures that console and not the main one", function()
        -- the main console is only measured once any layout still pending
        -- from earlier specs has run
        resizeWindow(window, 200, 100)
        pumpEvents(100)
        local mainColumns, mainRows = getColumnCount("main"), getRowCount("main")
        local columns, rows = getColumnCount(window), getRowCount(window)
        resizeWindow(window, 400, 200)
        pumpEvents(100)

        assert.is_true(getColumnCount(window) > columns)
        assert.is_true(getRowCount(window) > rows)
        assert.are.equal(mainColumns, getColumnCount("main"))
        assert.are.equal(mainRows, getRowCount("main"))
      end)
    end)

    describe("of a mini console with its split screen open", function()
      local window = "specCursorByName split" .. suffix

      setup(function()
        assert.is_true(createMiniConsole(window, 0, 0, 300, 400))
        enableScrolling(window)
        for i = 1, 300 do
          echo(window, "split line " .. i .. "\n")
        end
      end)

      teardown(function()
        deleteMiniConsole(window)
      end)

      -- the lower pane takes a quarter of the height, so counting it instead
      -- of the upper one would answer well under half the rows
      it("counts the rows above the split", function()
        pumpEvents(100)
        local fullRows = getRowCount(window)
        scrollTo(window, 10)
        pumpEvents(100)
        local splitRows = getRowCount(window)
        scrollTo(window)
        pumpEvents(100)

        assert.is_true(splitRows < fullRows, ("%d rows split, %d without"):format(splitRows, fullRows))
        assert.is_true(splitRows > fullRows / 2, ("%d rows split, %d without"):format(splitRows, fullRows))
        assert.are.equal(fullRows, getRowCount(window))
      end)
    end)
  end)

  describe("with the main console", function()
    setup(function()
      feedTriggers("specCursorByName main one\nspecCursorByName main two\n")
    end)

    for _, name in ipairs({"", "main"}) do
      local other = name == "" and "main" or ""

      it(("takes %q for the main console"):format(name), function()
        local last = getLastLineNumber(name)
        assert.is_true(last >= 1)
        assert.are.equal(last, getLastLineNumber(other))
        assert.are.equal(last, getLineCount(name))
        assert.are.equal(last, getLineCount(other))

        assert.is_true(moveCursor(name, 2, last - 1))
        assert.are.equal(last - 1, getLineNumber(other))
        assert.are.equal(last - 1, getLineNumber(name))
        assert.are.equal(2, getColumnNumber(other))
        assert.are.equal(2, getColumnNumber(name))
        assert.are.equal(getLines("main", last - 1, last)[1], getCurrentLine(name))
        assert.are.equal(getCurrentLine(other), getCurrentLine(name))

        assert.are.equal(0, select("#", moveCursorEnd(name)))
        assert.are.equal(last, getLineNumber(other))

        assert.are.equal(getColumnCount(other), getColumnCount(name))
        assert.are.equal(getRowCount(other), getRowCount(name))
      end)

      it(("deletes a line of the main console by %q"):format(name), function()
        local doomed = ("specCursorByName deleted by %q%s"):format(name, suffix)
        feedTriggers(doomed .. "\n")
        local line = findRecentMainLine(doomed)
        assert.is_number(line)
        local last = getLastLineNumber("main")

        assert.is_true(moveCursor("main", 0, line))
        assert.are.equal(0, select("#", deleteLine(name)))
        assert.are.equal(last - 1, getLastLineNumber("main"))
        assert.is_nil(findRecentMainLine(doomed))
      end)
    end

    it("takes no name for the main console", function()
      local last = getLastLineNumber()
      assert.are.equal(last, getLastLineNumber("main"))
      assert.are.equal(last, getLineCount())
      assert.is_true(moveCursor(1, last - 1))
      assert.are.equal(last - 1, getLineNumber())
      assert.are.equal(1, getColumnNumber())
      assert.are.equal(getCurrentLine("main"), getCurrentLine())
      assert.are.equal(0, select("#", moveCursorEnd()))
      assert.are.equal(last, getLineNumber("main"))
      assert.are.equal(getColumnCount("main"), getColumnCount())
      assert.are.equal(getRowCount("main"), getRowCount())
    end)

    -- a trigger's script starts with the cursor at the start of the line that
    -- fired it, and where it moves the cursor to outlasts the line
    it("reads and moves the cursor on the line a trigger fired on", function()
      local marker = "specCursorByName trigger line" .. suffix
      local window = "specCursorByName trigger console" .. suffix
      assert.is_true(createMiniConsole(window, 0, 0, 300, 100))
      echo(window, table.concat(lines, "\n"))
      assert.is_true(moveCursor(window, 0, 1))

      local seen
      local id = tempTrigger(marker, function()
        seen = {
          line = getCurrentLine(),
          byName = getCurrentLine("main"),
          number = getLineNumber(),
          column = getColumnNumber("main"),
          last = getLastLineNumber(),
          count = getLineCount(),
          subLine = getCurrentLine(window),
          subNumber = getLineNumber(window),
        }
        seen.moved = moveCursor(3, seen.number)
        seen.movedNumber = getLineNumber("main")
        seen.movedColumn = getColumnNumber()
      end)
      feedTriggers(marker .. "\n")
      killTrigger(id)
      deleteMiniConsole(window)

      assert.is_table(seen)
      assert.are.equal(marker, seen.line)
      assert.are.equal(marker, seen.byName)
      assert.are.equal(0, seen.column)
      assert.are.equal(marker, getLines("main", seen.number, seen.number + 1)[1])
      assert.is_true(seen.number <= seen.last)
      assert.are.equal(seen.last, seen.count)
      assert.are.equal("second line", seen.subLine)
      assert.are.equal(1, seen.subNumber)
      assert.is_true(seen.moved)
      assert.are.equal(seen.number, seen.movedNumber)
      assert.are.equal(3, seen.movedColumn)

      assert.are.equal(seen.number, getLineNumber())
      assert.are.equal(3, getColumnNumber("main"))
      assert.are.equal(marker, getCurrentLine())
    end)

    it("deletes the line a trigger fired on", function()
      local gagged = "specCursorByName gagged line" .. suffix
      local id = tempTrigger(gagged, function()
        deleteLine()
      end)
      feedTriggers(gagged .. "\n")
      killTrigger(id)

      assert.is_nil(findRecentMainLine(gagged))
    end)
  end)
end)
