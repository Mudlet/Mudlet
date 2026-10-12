-- clearWindow() finds its console by name: none, an empty name or "main" is
-- the main console, and any other is a mini console, user window or buffer.
-- It answers nothing whether or not it found one, so that clearing the main
-- console from the command line does not print a result onto it.
describe("Tests that clearWindow() finds its console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specClearByNameNoSuchWindow" .. suffix
  local sibling = "specClearByName sibling" .. suffix

  local function fill(window, count)
    for i = 1, count do
      echo(window, ("clear line %d\n"):format(i))
    end
  end

  local function assertCleared(window)
    assert.are.equal(0, getLastLineNumber(window))
    assert.are.same({""}, getLines(window, 0, 1))
  end

  -- getScroll() is copied out of the buffer as the pane repaints, and the first
  -- scroll of a console waits a turn for its lower pane to appear, so a new
  -- position needs the event loop to run before it can be read back
  local function pumpEventLoop()
    tempTimer(0, function() raiseEvent("specClearByNamePump") end)
    waitForEvent("specClearByNamePump", 2000)
  end

  -- scrollTo() scrolls by the distance from where the pane last painted, so it
  -- can land short until the pane catches up; repeating it settles it
  local function parkAt(window, line)
    for _ = 1, 10 do
      scrollTo(window, line)
      for _ = 1, 20 do
        if getScroll(window) == line then
          return line
        end
        pumpEventLoop()
      end
    end
    return getScroll(window)
  end

  setup(function()
    assert.is_true(createMiniConsole(sibling, 0, 0, 300, 100))
  end)

  teardown(function()
    deleteMiniConsole(sibling)
  end)

  before_each(function()
    fill(sibling, 3)
  end)

  it("checks its argument before it looks for the console", function()
    assert.has_error(function() clearWindow({}) end,
      "clearUserWindow: bad argument #1 type (window name as string is optional, got table!)")
    assert.has_error(function() clearWindow(true) end)
  end)

  it("answers nothing and clears nothing for a name that is no window at all", function()
    local mainLast = getLastLineNumber("main")
    local siblingLast = getLastLineNumber(sibling)
    assert.are.equal(0, select("#", clearWindow(unknown)))
    assert.are.equal(mainLast, getLastLineNumber("main"))
    assert.are.equal(siblingLast, getLastLineNumber(sibling))
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specClearByNameOtherWindow" .. suffix

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
      it("answers nothing and clears nothing for a " .. kind[1], function()
        assert.is_true(kind[2]())
        local mainLast = getLastLineNumber("main")
        local siblingLast = getLastLineNumber(sibling)
        assert.are.equal(0, select("#", clearWindow(otherName)))
        assert.are.equal(mainLast, getLastLineNumber("main"))
        assert.are.equal(siblingLast, getLastLineNumber(sibling))
      end)
    end

    it("leaves a label's text alone", function()
      assert.is_true(createLabel(otherName, 0, 0, 50, 20, 1))
      echo(otherName, "label text")
      clearWindow(otherName)
      assert.are.equal("label text", getLabelText(otherName))
    end)
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 100) end, deleteMiniConsole},
      -- floated at a fixed size, as a docked user window opened after another
      -- was deleted comes up with no height and never scrolls
      {"user window", function(name)
        local opened = openUserWindow(name, false)
        resizeWindow(name, 300, 400)
        return opened
      end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specClearByName " .. kind[1] .. suffix
      local isBuffer = kind[1] == "buffer"

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        it("clears that console down to one empty line and answers nothing", function()
          fill(window, 5)
          local mainLast = getLastLineNumber("main")
          assert.are.equal(0, select("#", clearWindow(window)))
          assertCleared(window)
          assert.are.equal(mainLast, getLastLineNumber("main"))
          assert.are.same({"clear line 3"}, getLines(sibling, getLastLineNumber(sibling) - 1, getLastLineNumber(sibling)))
        end)

        it("puts the user cursor on the line left and keeps the selection, no longer valid", function()
          clearWindow(window)
          fill(window, 3)
          moveCursor(window, 0, 2)
          assert.are.equal(2, selectString(window, "ear", 1))
          clearWindow(window)
          assert.are.equal(0, getLineNumber(window))
          assert.are.same({nil, "the selection is no longer valid"}, {getSelection(window)})
          echo(window, "after")
          assert.are.same({"after"}, getLines(window, 0, 1))
          assert.are.equal("after", getCurrentLine(window))
          assert.are.equal(1, selectString(window, "fter", 1))
        end)

        if isBuffer then
          -- a buffer has no pane to scroll
        elseif not os.getenv("MUDLET_TEST_MODE") then
          pending("follows new text again after being scrolled back - needs MUDLET_TEST_MODE for waitForEvent")
        else
          it("follows new text again after being scrolled back", function()
            assert.is_true(getRowCount(window) > 0, kind[1] .. " came up with no visible rows")
            clearWindow(window)
            fill(window, 200)
            assert.are.equal(150, parkAt(window, 150))
            clearWindow(window)
            fill(window, 200)
            assert.are.equal(getLastLineNumber(window), getScroll(window))
          end)
        end
      end)
    end
  end)

  describe("with the main console", function()
    for _, entry in ipairs({{"no name", function() return clearWindow() end}, {"an empty name", function() return clearWindow("") end}, {"\"main\"", function() return clearWindow("main") end}}) do
      it("clears the main console for " .. entry[1] .. " and answers nothing", function()
        fill("main", 5)
        local siblingLast = getLastLineNumber(sibling)
        assert.are.equal(0, select("#", entry[2]()))
        assertCleared("main")
        assert.are.equal(siblingLast, getLastLineNumber(sibling))
      end)
    end

    if not os.getenv("MUDLET_TEST_MODE") then
      pending("follows new text again after being scrolled back - needs MUDLET_TEST_MODE for waitForEvent")
    else
      it("follows new text again after being scrolled back", function()
        clearWindow()
        fill("main", 200)
        assert.are.equal(150, parkAt("main", 150))
        clearWindow()
        fill("main", 200)
        assert.are.equal(getLastLineNumber("main"), getScroll("main"))
      end)
    end
  end)
end)
