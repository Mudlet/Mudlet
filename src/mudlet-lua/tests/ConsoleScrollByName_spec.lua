-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console.
describe("Tests that the scroll bar and scrolling functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specScrollByNameNoSuchWindow" .. suffix
  local mainOnly = "scrolling cannot be enabled/disabled for the 'main' window"

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local calls = {
    {"enableScrollBar", function(n) return enableScrollBar(n) end},
    {"disableScrollBar", function(n) return disableScrollBar(n) end},
    {"enableHorizontalScrollBar", function(n) return enableHorizontalScrollBar(n) end},
    {"disableHorizontalScrollBar", function(n) return disableHorizontalScrollBar(n) end},
    {"getScrollBarVisible", function(n) return getScrollBarVisible(n) end},
    {"enableScrolling", function(n) return enableScrolling(n) end},
    {"disableScrolling", function(n) return disableScrolling(n) end},
    {"scrollingActive", function(n) return scrollingActive(n) end},
    {"getScroll", function(n) return getScroll(n) end},
    {"scrollTo a line", function(n) return scrollTo(n, 1) end},
    {"scrollTo the end", function(n) return scrollTo(n) end},
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

  it("refuses a name that is no window at all", function()
    assertRefusesAll(unknown)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specScrollByNameOtherWindow" .. suffix

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
      it("refuses every scroll bar and scrolling function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefusesAll(otherName)
      end)
    end
  end)

  it("checks the arguments before it looks for the console", function()
    for _, call in ipairs({
      function() enableScrollBar({}) end,
      function() disableScrollBar({}) end,
      function() enableHorizontalScrollBar({}) end,
      function() disableHorizontalScrollBar({}) end,
      function() getScrollBarVisible({}) end,
      function() enableScrolling({}) end,
      function() disableScrolling({}) end,
      function() scrollingActive({}) end,
      function() getScroll({}) end,
      function() scrollTo({}) end,
      function() scrollTo({}, 1) end,
      function() scrollTo(unknown, "last") end,
    }) do
      assert.has_error(call)
    end
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 100) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height and never scrolls, so this one is floated at a
      -- fixed size, and only closed so it cannot do the same to later specs
      {"user window", function(name)
        local opened = openUserWindow(name, false, false)
        resizeWindow(name, 300, 400)
        return opened
      end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specScrollByName " .. kind[1] .. suffix
      local isBuffer = kind[1] == "buffer"

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        it("shows and hides that console's scroll bars", function()
          local mainShows = getScrollBarVisible("main")

          assert.are.equal(0, select("#", enableScrollBar(window)))
          assert.is_true(getScrollBarVisible(window))
          assert.are.equal(0, select("#", disableScrollBar(window)))
          assert.is_false(getScrollBarVisible(window))
          assert.are.equal(mainShows, getScrollBarVisible("main"))
          assert.are.equal(0, select("#", enableScrollBar(window)))
          assert.is_true(getScrollBarVisible(window))
          assert.are.equal(mainShows, getScrollBarVisible("main"))

          assert.are.equal(0, select("#", enableHorizontalScrollBar(window)))
          assert.are.equal(0, select("#", disableHorizontalScrollBar(window)))
        end)

        if isBuffer then
          -- a buffer is never drawn, so it is always left scrolling
          it("will not turn that console's scrolling off or on", function()
            local bufferOnly = ('scrolling cannot be enabled/disabled for buffer "%s"'):format(window)
            assert.are.same({nil, bufferOnly}, {disableScrolling(window)})
            assert.is_true(scrollingActive(window))
            assert.are.same({nil, bufferOnly}, {enableScrolling(window)})
            assert.is_true(scrollingActive(window))
          end)
        else
          it("turns that console's scrolling off and on", function()
            assert.is_true(disableScrolling(window))
            assert.is_false(scrollingActive(window))
            assert.is_true(scrollingActive("main"))
            assert.is_true(enableScrolling(window))
            assert.is_true(scrollingActive(window))
          end)
        end

        if isBuffer then
          it("reports and moves that console's scroll position", function()
            assert.is_number(getScroll(window))
            assert.are.equal(0, select("#", scrollTo(window, 1)))
            assert.are.equal(0, select("#", scrollTo(window)))
          end)

          it("stays at the end of that console when told to scroll back", function()
            for i = 1, 20 do
              echo(window, "scroll line " .. i .. "\n")
            end
            scrollTo(window, 1)
            assert.are.equal(getLastLineNumber(window), getScroll(window))
            if os.getenv("MUDLET_TEST_MODE") then
              pumpEvents(50)
              assert.are.equal(getLastLineNumber(window), getScroll(window), "scrolled back once the event loop ran")
            end
            echo(window, "one more\n")
            assert.are.equal(getLastLineNumber(window), getScroll(window))
          end)
        else
          it("reports and moves that console's scroll position", function()
            assert.is_true(getRowCount(window) > 0, kind[1] .. " came up with no visible rows")
            clearWindow(window)
            for i = 1, 200 do
              echo(window, "scroll line " .. i .. "\n")
            end
            assert.are.equal(getLastLineNumber(window), getScroll(window))

            assert.are.equal(0, select("#", scrollTo(window, 150)))
            assert.are.equal(150, getScroll(window))
            assert.are.equal(0, select("#", scrollTo(window)))
            assert.are.equal(getLastLineNumber(window), getScroll(window))
          end)

          it("lands scrollTo on the line it is given, from the end and back to back", function()
            assert.is_true(getRowCount(window) > 0, kind[1] .. " came up with no visible rows")
            clearWindow(window)
            for i = 1, 300 do
              echo(window, "scroll line " .. i .. "\n")
            end
            finally(function() scrollTo(window) end)

            scrollTo(window, 150)
            assert.are.equal(150, getScroll(window))
            scrollTo(window, 100)
            scrollTo(window, 50)
            assert.are.equal(50, getScroll(window))

            if os.getenv("MUDLET_TEST_MODE") then
              pumpEvents(50)
              assert.are.equal(50, getScroll(window), "the split screen moved the line once it appeared")
            end
          end)

          it("keeps scrolling from the line it shows after the buffer trims lines away above it", function()
            assert.is_true(getRowCount(window) > 0, kind[1] .. " came up with no visible rows")
            local lines, batch = getConsoleBufferSize(window)
            setConsoleBufferSize(window, 200, 50)
            finally(function()
              scrollTo(window)
              setConsoleBufferSize(window, lines, batch)
            end)
            clearWindow(window)
            for i = 1, 190 do
              echo(window, "scroll line " .. i .. "\n")
            end
            scrollTo(window, 100)
            assert.are.equal(100, getScroll(window))
            local lastBefore = getLastLineNumber(window)
            for i = 1, 30 do
              echo(window, "more line " .. i .. "\n")
            end
            local trimmed = lastBefore + 30 - getLastLineNumber(window)
            assert.is_true(trimmed > 0, "the buffer did not trim")

            assert.are.equal(100 - trimmed, getScroll(window))
            scrollUp(window, 1)
            assert.are.equal(99 - trimmed, getScroll(window))
          end)
        end
      end)
    end
  end)

  describe("with the main console", function()
    local mainShows

    setup(function()
      mainShows = getScrollBarVisible("main")
    end)

    teardown(function()
      if mainShows then
        enableScrollBar("main")
      else
        disableScrollBar("main")
      end
    end)

    for _, name in ipairs({"", "main"}) do
      local other = name == "" and "main" or ""

      it(("takes %q for the main console"):format(name), function()
        assert.are.equal(0, select("#", disableScrollBar(name)))
        assert.is_false(getScrollBarVisible(other))
        assert.is_false(getScrollBarVisible(name))
        assert.are.equal(0, select("#", enableScrollBar(name)))
        assert.is_true(getScrollBarVisible(other))
        assert.is_true(getScrollBarVisible(name))

        assert.are.equal(getScroll(other), getScroll(name))
        assert.are.equal(0, select("#", scrollTo(name)))
        assert.are.equal(getScroll(other), getScroll(name))
      end)
    end

    it("lands scrollTo on the line it is given, from the end and back to back", function()
      for i = 1, 300 do
        echo("main", "scroll line " .. i .. "\n")
      end
      finally(function() scrollTo("main") end)
      scrollTo("main")

      scrollTo("main", 150)
      assert.are.equal(150, getScroll("main"))
      scrollTo("main", 100)
      scrollTo("main", 50)
      assert.are.equal(50, getScroll("main"))

      if os.getenv("MUDLET_TEST_MODE") then
        pumpEvents(50)
        assert.are.equal(50, getScroll("main"), "the split screen moved the line once it appeared")
      end
    end)

    it("takes no name for the main console", function()
      disableScrollBar("main")
      assert.is_false(getScrollBarVisible())
      enableScrollBar("main")
      assert.is_true(getScrollBarVisible())
      assert.are.equal(getScroll("main"), getScroll())
      assert.are.equal(0, select("#", scrollTo()))
    end)

    it("will not turn scrolling off or on by the name main", function()
      assert.are.same({nil, mainOnly}, {enableScrolling("main")})
      assert.are.same({nil, mainOnly}, {disableScrolling("main")})
      assert.is_true(scrollingActive("main"))
    end)

    -- the main console is always scrolling, so every name for it is turned away
    it("will not turn scrolling off or on with an empty name or none", function()
      assert.are.same({nil, mainOnly}, {disableScrolling("")})
      assert.are.same({nil, mainOnly}, {enableScrolling("")})
      assert.are.same({nil, mainOnly}, {disableScrolling()})
      assert.are.same({nil, mainOnly}, {enableScrolling()})
      assert.are.same({nil, mainOnly}, {disableScrolling(nil)})
      assert.are.same({nil, mainOnly}, {enableScrolling(nil)})
      assert.is_true(scrollingActive(""))
      assert.is_true(scrollingActive())
    end)
  end)
end)
