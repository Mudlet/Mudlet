-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console.
describe("Tests that the timestamp and wrap functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specWrapByNameNoSuchWindow" .. suffix
  local text = "the quick brown fox jumps over the lazy dog"

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local calls = {
    {"enableTimeStamps", function(n) return enableTimeStamps(n) end},
    {"disableTimeStamps", function(n) return disableTimeStamps(n) end},
    {"timeStampsEnabled", function(n) return timeStampsEnabled(n) end},
    {"getWindowWrap", function(n) return getWindowWrap(n) end},
    {"setWindowWrap", function(n) return setWindowWrap(n, 80) end},
    {"setWindowWrapIndent", function(n) return setWindowWrapIndent(n, 1) end},
    {"setWindowWrapHangingIndent", function(n) return setWindowWrapHangingIndent(n, 1) end},
  }

  local function assertRefusesAll(windowName)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      assert.are.equal(2, select("#", call(windowName)), functionName)
      local ok, err = call(windowName)
      assert.is_nil(ok, functionName)
      assert.are.equal(('window "%s" not found'):format(windowName), err, functionName)
    end
  end

  local function wrappedLines(window)
    clearWindow(window)
    echo(window, text .. "\n")
    return getLines(window, 0, getLineCount(window))
  end

  it("refuses a name that is no window at all", function()
    assertRefusesAll(unknown)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specWrapByNameOtherWindow" .. suffix

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
      it("refuses every timestamp and wrap function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefusesAll(otherName)
      end)
    end
  end)

  it("checks the argument types before it looks for the console", function()
    for _, call in ipairs({
      function() enableTimeStamps({}) end,
      function() disableTimeStamps({}) end,
      function() timeStampsEnabled({}) end,
      function() getWindowWrap({}) end,
      function() setWindowWrap(unknown, "wide") end,
      function() setWindowWrapIndent(unknown, "deep") end,
      function() setWindowWrapHangingIndent(unknown, "deep") end,
    }) do
      assert.has_error(call)
    end
  end)

  it("looks for the console before it checks the range of a width or indent", function()
    local notFound = ('window "%s" not found'):format(unknown)
    assert.are.same({nil, notFound}, {setWindowWrap(unknown, 0)})
    assert.are.same({nil, notFound}, {setWindowWrapIndent(unknown, -1)})
    assert.are.same({nil, notFound}, {setWindowWrapHangingIndent(unknown, -1)})
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
      local window = "specWrapByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        it("turns that console's timestamps on and off", function()
          local mainShows = timeStampsEnabled("main")
          local enableRefusal = ('timestamps were already enabled for the "%s" console'):format(window)
          local disableRefusal = ('timestamps were not enabled for the "%s" console'):format(window)

          assert.is_false(timeStampsEnabled(window))
          assert.is_true(enableTimeStamps(window))
          assert.is_true(timeStampsEnabled(window))
          assert.are.equal(mainShows, timeStampsEnabled("main"))
          assert.are.same({nil, enableRefusal}, {enableTimeStamps(window)})
          assert.is_true(timeStampsEnabled(window))

          assert.is_true(disableTimeStamps(window))
          assert.is_false(timeStampsEnabled(window))
          assert.are.same({nil, disableRefusal}, {disableTimeStamps(window)})
          assert.is_false(timeStampsEnabled(window))
          assert.are.equal(mainShows, timeStampsEnabled("main"))
        end)

        -- A console reports its size only once it differs from the last one it
        -- reported, and a font change alone reports nothing, so a font change
        -- leaves a size for the next timestamp change to report. A buffer is
        -- never shown, so it has no size to report.
        it("reports a size it has not reported yet when its timestamps change", function()
          disableTimeStamps(window)
          local fontSize = getFontSize(window)
          local reports = {}
          local handler = registerAnonymousEventHandler("sysConsoleSizeChanged", function(_, ...)
            reports[#reports + 1] = {...}
          end)

          setFontSize(window, fontSize + 6)
          assert.are.same({}, reports)
          enableTimeStamps(window)
          enableTimeStamps(window)
          local onReport = {window, getColumnCount(window), getRowCount(window)}
          setFontSize(window, fontSize)
          disableTimeStamps(window)
          disableTimeStamps(window)
          local offReport = {window, getColumnCount(window), getRowCount(window), 0}
          killAnonymousEventHandler(handler)

          if kind[1] == "buffer" then
            assert.are.same({}, reports)
            return
          end
          assert.are.equal(2, #reports)
          local gutter = table.remove(reports[1])
          assert.is_true(gutter > 0, tostring(gutter))
          assert.are.same(onReport, reports[1])
          assert.are.same(offReport, reports[2])
          assert.are_not.same(onReport[2], offReport[2])
        end)

        it("wraps that console at the width and indents it is given", function()
          local mainWrap = getWindowWrap("main")
          local mainWidth = getMainConsoleWidth()

          assert.is_true(setWindowWrap(window, 20))
          assert.are.equal(20, getWindowWrap(window))
          assert.is_true(setWindowWrapIndent(window, 2))
          assert.is_true(setWindowWrapHangingIndent(window, 4))
          assert.are.same({"  the quick brown ", "    fox jumps over ", "    the lazy dog"}, wrappedLines(window))

          assert.are.same({nil, "wrapAt must be greater than zero, got 0"}, {setWindowWrap(window, 0)})
          assert.are.equal(20, getWindowWrap(window))
          assert.are.same({nil, "indent -1 is not valid, it must be 0 or more"}, {setWindowWrapIndent(window, -1)})
          assert.are.same({nil, "indent -1 is not valid, it must be 0 or more"}, {setWindowWrapHangingIndent(window, -1)})
          assert.are.same({"  the quick brown ", "    fox jumps over ", "    the lazy dog"}, wrappedLines(window))

          assert.is_true(setWindowWrapIndent(window, 0))
          assert.is_true(setWindowWrapHangingIndent(window, 0))
          assert.are.same({"the quick brown fox ", "jumps over the lazy ", "dog"}, wrappedLines(window))

          assert.are.equal(mainWrap, getWindowWrap("main"))
          assert.are.equal(mainWidth, getMainConsoleWidth())
        end)

        it("rewraps a line with wrapLine() to the width and indents it is given", function()
          setWindowWrap(window, 200)
          setWindowWrapIndent(window, 0)
          setWindowWrapHangingIndent(window, 0)
          clearWindow(window)
          echo(window, text .. "\n")
          assert.are.same({text}, getLines(window, 0, getLineCount(window)))

          setWindowWrap(window, 20)
          setWindowWrapIndent(window, 2)
          setWindowWrapHangingIndent(window, 4)
          wrapLine(window, 0)
          local lines = getLines(window, 0, getLineCount(window))
          assert.is_true(#lines > 1)
          assert.is_truthy(lines[1]:find("^  %S"), lines[1])
          for i = 2, #lines do
            assert.is_truthy(lines[i]:find("^    %S"), lines[i])
          end
          for _, line in ipairs(lines) do
            assert.is_true(#line <= 20, line)
          end
        end)
      end)
    end
  end)

  describe("with the main console", function()
    local mainWrap, mainShows

    setup(function()
      mainWrap = getWindowWrap("main")
      mainShows = timeStampsEnabled("main")
    end)

    teardown(function()
      setWindowWrap("main", mainWrap)
      if mainShows then
        enableTimeStamps("main")
      else
        disableTimeStamps("main")
      end
    end)

    it("turns the main console's timestamps on and off by either name", function()
      local enabledByEmptyName = "timestamps were already enabled for the main console"
      local enabledByMain = 'timestamps were already enabled for the "main" console'
      local byEmptyName = "timestamps were not enabled for the main console"
      local byMain = 'timestamps were not enabled for the "main" console'
      disableTimeStamps("main")

      assert.is_false(timeStampsEnabled(""))
      assert.is_true(enableTimeStamps(""))
      assert.is_true(timeStampsEnabled("main"))
      assert.is_true(timeStampsEnabled())
      assert.are.same({nil, enabledByEmptyName}, {enableTimeStamps()})
      assert.are.same({nil, enabledByMain}, {enableTimeStamps("main")})

      assert.is_true(disableTimeStamps("main"))
      assert.is_false(timeStampsEnabled(""))
      assert.are.same({nil, byEmptyName}, {disableTimeStamps("")})
      assert.are.same({nil, byMain}, {disableTimeStamps("main")})
    end)

    -- the main console's width is also the profile's, which is what
    -- getMainConsoleWidth() is worked out from
    it("sets the main console's wrap width by an empty name, main or none", function()
      assert.is_true(setWindowWrap("", 70))
      assert.are.equal(70, getWindowWrap("main"))
      assert.are.equal(70, getWindowWrap())
      local columnWidth = getMainConsoleWidth() / 71

      assert.is_true(setWindowWrap("main", 72))
      assert.are.equal(72, getWindowWrap(""))
      assert.are.equal(columnWidth * 73, getMainConsoleWidth())

      assert.is_true(setWindowWrap(74))
      assert.are.equal(74, getWindowWrap("main"))
      assert.are.equal(columnWidth * 75, getMainConsoleWidth())

      assert.are.same({nil, "wrapAt must be greater than zero, got 0"}, {setWindowWrap("main", 0)})
      assert.are.equal(74, getWindowWrap(""))
      assert.are.equal(columnWidth * 75, getMainConsoleWidth())
    end)
  end)
end)
