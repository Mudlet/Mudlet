-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console.
describe("Tests that the selection and format functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specConsoleByNameNoSuchWindow" .. suffix

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local calls = {
    {"deselect", function(n) return deselect(n) end},
    {"selectCurrentLine", function(n) return selectCurrentLine(n) end},
    {"selectSection", function(n) return selectSection(n, 0, 1) end},
    {"selectString", function(n) return selectString(n, "a", 1) end},
    {"getSelection", function(n) return getSelection(n) end},
    {"setBold", function(n) return setBold(n, true) end},
    {"setItalics", function(n) return setItalics(n, true) end},
    {"setOverline", function(n) return setOverline(n, true) end},
    {"setReverse", function(n) return setReverse(n, true) end},
    {"setStrikeOut", function(n) return setStrikeOut(n, true) end},
    {"setUnderline", function(n) return setUnderline(n, true) end},
    {"setFgColor", function(n) return setFgColor(n, 1, 2, 3) end},
    {"setBgColor", function(n) return setBgColor(n, 1, 2, 3) end},
    {"resetFormat", function(n) return resetFormat(n) end},
  }

  local function assertRefusesAll(windowName)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      assert.are.equal(2, select("#", call(windowName)), functionName)
      local ok, err = call(windowName)
      assert.is_nil(ok, functionName)
      assert.are.equal(('window "%s" not found'):format(windowName), err, functionName)
    end
    -- getTextFormat() has always worded its refusal differently
    local ok, err = getTextFormat(windowName)
    assert.is_nil(ok)
    assert.are.equal(("window '%s' not found"):format(windowName), err)
  end

  -- one line, with the user cursor on it
  local function writeLine(window, text)
    clearWindow(window)
    -- clearWindow() leaves one empty line behind and does not move the user
    -- cursor, so the line has to be deleted for line 0 to be the echoed one
    moveCursor(window, 0, 0)
    deleteLine(window)
    echo(window, text .. "\n")
    moveCursor(window, 0, 0)
  end

  it("refuses a name that is no window at all", function()
    assertRefusesAll(unknown)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specConsoleByNameOtherWindow" .. suffix

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
      it("refuses every selection and format function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefusesAll(otherName)
      end)
    end
  end)

  it("checks the arguments before it looks for the console", function()
    for _, call in ipairs({
      function() selectSection(unknown, "from", 1) end,
      function() selectString(unknown, {}, 1) end,
      function() selectString(unknown, "a", "first") end,
      function() setBold(unknown, "yes") end,
      function() setItalics(unknown, "yes") end,
      function() setOverline(unknown, "yes") end,
      function() setReverse(unknown, "yes") end,
      function() setStrikeOut(unknown, "yes") end,
      function() setUnderline(unknown, "yes") end,
      function() setFgColor(unknown, 1, "green", 3) end,
      function() setBgColor(unknown, 1, "green", 3) end,
      function() getTextFormat({}) end,
    }) do
      assert.has_error(call)
    end

    assert.are.same({nil, "red value 256 needs to be between 0-255"}, {setFgColor(unknown, 256, 0, 0)})
    assert.are.same({nil, "alpha value 256 needs to be between 0-255"}, {setBgColor(unknown, 0, 0, 0, 256)})
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 100) end, deleteMiniConsole},
      {"user window", function(name) return openUserWindow(name, false) end, deleteMiniConsole},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specConsoleByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
          setWindowWrap(window, 80)
        end)

        teardown(function()
          kind[3](window)
        end)

        before_each(function()
          deselect(window)
          resetFormat(window)
          writeLine(window, "alpha bravo charlie")
        end)

        it("selects on that console", function()
          assert.are.equal(6, selectString(window, "bravo", 1))
          assert.are.same({"bravo", 6, 5}, {getSelection(window)})

          assert.is_true(selectSection(window, 12, 7))
          assert.are.same({"charlie", 12, 7}, {getSelection(window)})

          selectCurrentLine(window)
          assert.are.same({"alpha bravo charlie", 0, 19}, {getSelection(window)})

          assert.is_true(deselect(window))
          assert.are.same({"", 0, 0}, {getSelection(window)})
        end)

        it("formats that console's selection", function()
          assert.are.equal(6, selectString(window, "bravo", 1))
          local before = getTextFormat(window)
          assert.is_false(before.bold)

          assert.is_true(setBold(window, true))
          assert.is_true(setItalics(window, true))
          assert.is_true(setOverline(window, true))
          assert.is_true(setReverse(window, true))
          assert.is_true(setStrikeOut(window, true))
          assert.is_true(setUnderline(window, true))
          assert.are.equal(0, select("#", setFgColor(window, 10, 20, 30)))
          assert.is_true(setBgColor(window, 40, 50, 60))

          local after = getTextFormat(window)
          assert.is_true(after.bold)
          assert.is_true(after.italic)
          assert.is_true(after.overline)
          assert.is_true(after.reverse)
          assert.is_true(after.strikeout)
          assert.is_true(after.underline)
          assert.are.same({10, 20, 30}, after.foreground)
          assert.are.same({40, 50, 60}, after.background)

          -- the characters outside the selection keep their format
          deselect(window)
          moveCursor(window, 0, 0)
          assert.are.same(before, getTextFormat(window))
        end)

        it("resets the format that console echoes with", function()
          -- with nothing selected, setBold() sets the format the next echo takes
          assert.is_true(setBold(window, true))
          echo(window, "bold\n")
          assert.is_true(resetFormat(window))
          echo(window, "plain\n")

          moveCursor(window, 0, 1)
          assert.is_true(getTextFormat(window).bold)
          moveCursor(window, 0, 2)
          assert.is_false(getTextFormat(window).bold)
        end)
      end)
    end
  end)

  describe("with the main console", function()
    local marker = "specConsoleByNameMainLine" .. suffix

    -- the main console carries on after this spec, so its line is found
    -- rather than cleared into place
    local function markerLine()
      echo("\n" .. marker .. " alpha bravo\n")
      local lastLine = getLastLineNumber("main")
      local first = math.max(0, lastLine - 5)
      local lines = getLines("main", first, lastLine + 1)
      for i = #lines, 1, -1 do
        if lines[i] == marker .. " alpha bravo" then
          return first + i - 1
        end
      end
      error("the marker line did not reach the main console")
    end

    after_each(function()
      deselect("main")
      resetFormat("main")
      moveCursorEnd("main")
    end)

    for _, name in ipairs({"", "main"}) do
      local other = name == "" and "main" or ""

      it(("takes %q for the main console"):format(name), function()
        local line = markerLine()
        assert.is_true(moveCursor("main", 0, line))

        local at = #marker + 7
        assert.are.equal(at, selectString(name, "bravo", 1))
        assert.are.same({"bravo", at, 5}, {getSelection(other)})
        assert.are.same({"bravo", at, 5}, {getSelection(name)})

        assert.is_true(selectSection(name, 0, #marker))
        assert.are.same({marker, 0, #marker}, {getSelection(other)})

        assert.is_true(setBold(name, true))
        assert.are.equal(0, select("#", setFgColor(name, 10, 20, 30)))
        assert.is_true(setBgColor(name, 40, 50, 60))
        local format = getTextFormat(other)
        assert.is_true(format.bold)
        assert.are.same({10, 20, 30}, format.foreground)
        assert.are.same({40, 50, 60}, format.background)
        assert.are.same(format, getTextFormat(name))

        selectCurrentLine(name)
        assert.are.same({marker .. " alpha bravo", 0, #marker + 12}, {getSelection(other)})

        assert.is_true(deselect(name))
        assert.are.same({"", 0, 0}, {getSelection(other)})
        assert.is_true(resetFormat(name))
      end)
    end
  end)
end)
