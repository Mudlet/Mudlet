-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console. There is one clipboard to a
-- profile, whichever console copy() and paste() are aimed at.
describe("Tests that the clipboard and buffer size functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specClipboardByNameNoSuchWindow" .. suffix
  local helper = "specClipboardByName helper" .. suffix

  setup(function()
    assert.is_true(createMiniConsole(helper, 0, 0, 300, 200))
    setWindowWrap(helper, 200)
  end)

  teardown(function()
    deleteMiniConsole(helper)
  end)

  -- Lua cannot read the profile's "use the maximum" setting, which a plain
  -- setConsoleBufferSize() clears, but with it set the limit is the machine's
  -- maximum; so if asking for the maximum gives back the old limit, it was set
  local function restoreMainBufferSize(lines, batch)
    setConsoleBufferSize("main", lines, batch, true)
    if getConsoleBufferSize("main") ~= lines then
      setConsoleBufferSize("main", lines, batch)
    end
  end

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order; each is a call that would succeed on a console
  local calls = {
    {"copy", function(n) return copy(n) end},
    {"paste", function(n) return paste(n) end},
    {"appendBuffer", function(n) return appendBuffer(n) end},
    {"getConsoleBufferSize", function(n) return getConsoleBufferSize(n) end},
    {"setConsoleBufferSize", function(n) return setConsoleBufferSize(n, 500, 50) end},
    {"setConsoleBufferSize useMaximum", function(n) return setConsoleBufferSize(n, 500, 50, true) end},
    {"setConsoleBufferSize no maximum", function(n) return setConsoleBufferSize(n, 500, 50, false) end},
  }

  local function assertRefuses(windowName)
    local notFound = ('window "%s" not found'):format(windowName)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      assert.are.same({nil, notFound}, {call(windowName)}, functionName)
      assert.are.equal(2, select("#", call(windowName)), functionName)
    end
  end

  -- what appendBuffer() puts into a cleared helper is what the clipboard holds
  local function clipboard()
    clearWindow(helper)
    appendBuffer(helper)
    return getLines(helper, 0, 1)[1]
  end

  -- puts text on the clipboard by way of the helper
  local function copyToClipboard(text)
    clearWindow(helper)
    echo(helper, text .. "\n")
    moveCursor(helper, 0, 0)
    selectCurrentLine(helper)
    copy(helper)
    deselect(helper)
  end

  -- the main console's last line, which a call aimed elsewhere must not touch
  local function mainTail()
    local last = getLastLineNumber("main")
    return last, getLines("main", last, last + 1)[1]
  end

  it("refuses pasteWindow() of a name that is no window at all", function()
    assert.are.same({nil, ('window "%s" not found'):format(unknown)}, {pasteWindow(unknown)})
  end)

  it("refuses a name that is no window at all", function()
    assertRefuses(unknown)
  end)

  -- A copied function link holds a registry reference of its own, which the
  -- clipboard has to let go of when it is given something else to hold.
  -- Slots are counted by what they hold rather than how many there are, as
  -- a leaked reference can reuse a slot an earlier spec released.
  local function referencesTo(fn)
    local count = 0
    for _, value in pairs(debug.getregistry()) do
      if value == fn then
        count = count + 1
      end
    end
    return count
  end

  it("lets go of a copied function link when copy() replaces it", function()
    local fn = function() end
    clearWindow(helper)
    echoLink(helper, "FNLINK", fn, "hint")
    echo(helper, "\n")
    moveCursor(helper, 0, 0)
    selectCurrentLine(helper)
    copy(helper)
    local held = referencesTo(fn)
    for _ = 1, 20 do
      copy(helper)
    end
    deselect(helper)
    -- the helper's own link and the clipboard's copy of it
    assert.are.equal(2, held)
    assert.are.equal(held, referencesTo(fn))
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specClipboardByNameOtherWindow" .. suffix

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
      it("refuses every clipboard and buffer size function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefuses(otherName)
      end)
    end
  end)

  describe("before it looks for the window", function()
    it("checks every argument", function()
      local wrong = {
        function() copy({}) end,
        function() paste({}) end,
        function() appendBuffer({}) end,
        function() pasteWindow({}) end,
        function() getConsoleBufferSize({}) end,
        function() setConsoleBufferSize({}, 500, 50) end,
        function() setConsoleBufferSize(unknown, "many", 50) end,
        function() setConsoleBufferSize(unknown, 500, "some") end,
        -- with two arguments there is no name, so the first is the limit
        function() setConsoleBufferSize(unknown, 500) end,
      }
      for index, call in ipairs(wrong) do
        assert.has_error(call, nil, ("call #%d"):format(index))
      end
    end)

    it("reports an unknown window ahead of refusing the maximum", function()
      assert.are.same({nil, ('window "%s" not found'):format(unknown)}, {setConsoleBufferSize(unknown, 500, 50, true)})
    end)
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 200) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height, so this one is only closed and left behind
      {"user window", function(name) return openUserWindow(name, false) end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specClipboardByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        local mainLast, mainLine, mainLines, mainBatch

        setup(function()
          assert.is_true(kind[2](window))
          setWindowWrap(window, 200)
        end)

        teardown(function()
          kind[3](window)
        end)

        before_each(function()
          clearWindow(window)
          moveCursor(window, 0, 0)
          deselect(window)
          mainLast, mainLine = mainTail()
          mainLines, mainBatch = getConsoleBufferSize("main")
        end)

        after_each(function()
          local last, line = mainTail()
          assert.are.equal(mainLast, last, "the main console gained a line")
          assert.are.equal(mainLine, line, "the main console's last line changed")
          assert.are.same({mainLines, mainBatch}, {getConsoleBufferSize("main")}, "the main console's buffer size changed")
        end)

        it("copies that console's selection to the one clipboard", function()
          echo(window, "alpha beta\n")
          moveCursor(window, 0, 0)
          assert.are.equal(6, selectString(window, "beta", 1))
          assert.are.equal(0, select("#", copy(window)))
          assert.are.equal("beta", clipboard())
        end)

        it("pastes the clipboard at that console's cursor", function()
          copyToClipboard("gamma")
          -- a line after the cursor's, so this inserts rather than appends
          echo(window, "xxx\nyyy\n")
          moveCursor(window, 0, 0)
          assert.are.equal(0, select("#", paste(window)))
          assert.are.same({"gammaxxx", "yyy"}, getLines(window, 0, 2))
        end)

        it("pastes onto the end of that console with no line after the cursor's", function()
          copyToClipboard("delta")
          assert.are.equal(0, select("#", paste(window)))
          assert.are.equal("delta", getLines(window, 0, 1)[1])
        end)

        it("pastes the clipboard at that console's cursor with pasteWindow", function()
          copyToClipboard("zeta")
          echo(window, "xxx\nyyy\n")
          moveCursor(window, 0, 0)
          assert.are.equal(0, select("#", pasteWindow(window)))
          assert.are.same({"zetaxxx", "yyy"}, getLines(window, 0, 2))
        end)

        it("appends the clipboard to that console", function()
          copyToClipboard("epsilon")
          echo(window, "first\n")
          assert.are.equal(0, select("#", appendBuffer(window)))
          assert.are.same({"first", "epsilon"}, getLines(window, 0, 2))
        end)

        it("sets and reads that console's buffer size", function()
          assert.are.same({true}, {setConsoleBufferSize(window, 500, 50)})
          assert.are.same({500, 50}, {getConsoleBufferSize(window)})
          assert.are.same({true}, {setConsoleBufferSize(window, 600, 60, false)})
          assert.are.same({600, 60}, {getConsoleBufferSize(window)})
          -- the limit is at least 100, and a batch of that limit or more is a tenth of it
          assert.are.same({true}, {setConsoleBufferSize(window, 50, 150)})
          assert.are.same({100, 10}, {getConsoleBufferSize(window)})
          assert.are.same({true}, {setConsoleBufferSize(window, 500, 0)})
          assert.are.same({500, 1}, {getConsoleBufferSize(window)})
        end)

        it("refuses the maximum buffer size, leaving the size as it was", function()
          setConsoleBufferSize(window, 500, 50)
          assert.are.same({nil, "useMaximum parameter is only supported for the main console"}, {setConsoleBufferSize(window, 700, 70, true)})
          assert.are.same({500, 50}, {getConsoleBufferSize(window)})
        end)
      end)
    end
  end)

  describe("with the main console", function()
    local function recentLinesHave(text)
      local last = getLastLineNumber("main")
      for _, line in ipairs(getLines("main", math.max(0, last - 2), last + 1)) do
        if line:find(text, 1, true) then
          return true
        end
      end
      return false
    end

    -- selects text on one of the main console's last few lines
    local function selectOnMain(text)
      local last = getLastLineNumber("main")
      for y = last, math.max(0, last - 2), -1 do
        moveCursor("main", 0, y)
        if selectString("main", text, 1) > -1 then
          return
        end
      end
      error(text .. " is not on the main console")
    end

    after_each(function()
      echo("main", "\n")
      deselect("main")
      moveCursorEnd("main")
    end)

    it("lets go of a copied function link when cut() replaces it", function()
      local fn = function() end
      local marker = "specClipboardByNameCut" .. suffix
      echoLink("main", marker, fn, "hint")
      echo("main", " plain\n")
      selectOnMain(marker)
      copy("main")
      assert.are.equal(2, referencesTo(fn))
      selectOnMain(" plain")
      cut()
      assert.are.equal(1, referencesTo(fn))
    end)

    -- names that are all the main console's, and how to hand each one over
    local names = {
      {"no name", function(fn, ...) return fn(...) end},
      {'""', function(fn, ...) return fn("", ...) end},
      {'"main"', function(fn, ...) return fn("main", ...) end},
    }

    for index, name in ipairs(names) do
      local call = name[2]

      it("copies the main console's selection given " .. name[1], function()
        local marker = "specClipboardByNameCopy" .. index .. suffix
        echo("main", "lead " .. marker .. " tail\n")
        selectOnMain(marker)
        assert.are.equal(0, select("#", call(copy)))
        assert.are.equal(marker, clipboard())
      end)

      it("pastes and appends onto the main console given " .. name[1], function()
        local pasted = "specClipboardByNamePaste" .. index .. suffix
        copyToClipboard(pasted)
        moveCursorEnd("main")
        assert.are.equal(0, select("#", call(paste)))
        assert.is_true(recentLinesHave(pasted))
        echo("main", "\n")
        local appended = "specClipboardByNameAppend" .. index .. suffix
        copyToClipboard(appended)
        assert.are.equal(0, select("#", call(appendBuffer)))
        assert.is_true(recentLinesHave(appended))
      end)

      it("sets and reads the main console's buffer size given " .. name[1], function()
        local lines, batch = getConsoleBufferSize("main")
        finally(function()
          restoreMainBufferSize(lines, batch)
        end)
        assert.are.same({lines, batch}, {call(getConsoleBufferSize)})
        assert.are.same({true}, {call(setConsoleBufferSize, 700 + index, 70)})
        assert.are.same({700 + index, 70}, {getConsoleBufferSize("main")})
        assert.are.same({700 + index, 70}, {call(getConsoleBufferSize)})
      end)
    end

    -- pasteWindow() only ever pastes into a sub-console
    for _, name in ipairs({"", "main"}) do
      it(("leaves the main console alone given pasteWindow(%q)"):format(name), function()
        copyToClipboard("specClipboardByNamePasteWindow" .. suffix)
        moveCursorEnd("main")
        local last, line = mainTail()
        assert.are.same({nil, ('window "%s" not found'):format(name)}, {pasteWindow(name)})
        assert.are.same({last, line}, {mainTail()})
      end)
    end

    -- with no name, a third argument would be taken for the name
    for index, name in ipairs({"", "main"}) do
      it(("takes the maximum buffer size for %q"):format(name), function()
        local lines, batch = getConsoleBufferSize("main")
        finally(function()
          restoreMainBufferSize(lines, batch)
        end)
        assert.are.same({true}, {setConsoleBufferSize(name, 700, 70 + index, true)})
        local maxLines, maxBatch = getConsoleBufferSize("main")
        assert.is_true(maxLines > 700, ("the limit is %d"):format(maxLines))
        assert.are.equal(70 + index, maxBatch)
        -- the maximum does not depend on the limit asked for
        assert.are.same({true}, {setConsoleBufferSize(name, 900, 90, true)})
        assert.are.same({maxLines, 90}, {getConsoleBufferSize("main")})
      end)
    end
  end)
end)
