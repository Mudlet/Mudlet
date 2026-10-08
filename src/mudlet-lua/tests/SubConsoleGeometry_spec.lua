describe("Sub-console geometry", function()
  -- Unique names per run, so a window an earlier run left behind is never measured instead.
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  describe("the height a miniconsole is painted at", function()
    local consoleName = uniqueName("sgMiniConsole")

    -- Creation queues a layout pass; nothing is measurable before it has run.
    local function createConsole(height)
      createMiniConsole(consoleName, 20, 20, 200, height)
      pumpEvents(100)
    end

    after_each(function()
      deleteMiniConsole(consoleName)
    end)

    it("leaves room for as many rows as the font fits into it", function()
      createConsole(200)
      local _, characterHeight = calcFontSize(consoleName)
      local rowsThatFit = math.floor(200 / characterHeight)
      local rows = getRowCount(consoleName)
      -- The pane may round a part row up, so one row over is allowed; one row
      -- short never is.
      assert.is_true(rows >= rowsThatFit,
        ("a 200 pixel high console has room for %d rows of %d pixels but reports %d"):format(rowsThatFit, characterHeight, rows))
      assert.is_true(rows <= rowsThatFit + 1,
        ("a 200 pixel high console has room for %d rows of %d pixels but reports %d"):format(rowsThatFit, characterHeight, rows))
    end)

    it("is the same as soon as it is created as it is after a resize", function()
      createConsole(200)
      local atCreation = getRowCount(consoleName)
      -- away and back, so the console is measured again, this time laid out
      resizeWindow(consoleName, 200, 300)
      pumpEvents(100)
      resizeWindow(consoleName, 200, 200)
      pumpEvents(100)
      assert.equals(atCreation, getRowCount(consoleName))
    end)

    it("still has a row to draw in when it is barely over one row tall", function()
      createConsole(200)
      local _, characterHeight = calcFontSize(consoleName)
      deleteMiniConsole(consoleName)

      -- Over one row tall, and under one row plus 30 pixels, whatever the font.
      createConsole(characterHeight + 10)
      assert.is_true(getRowCount(consoleName) >= 1, "a console barely over one row tall shows nothing at all")
    end)

    it("keeps its rows when it is created with a command line", function()
      -- Both in one turn: no layout runs in between, which is the moment under
      -- test.
      createMiniConsole(consoleName, 20, 20, 200, 200)
      enableCommandLine(consoleName)
      pumpEvents(100)
      local atCreation = getRowCount(consoleName)
      resizeWindow(consoleName, 200, 300)
      pumpEvents(100)
      resizeWindow(consoleName, 200, 200)
      pumpEvents(100)
      assert.equals(atCreation, getRowCount(consoleName))
    end)
  end)

  describe("sysUserWindowResizeEvent", function()
    it("reports the size the window really is, as it is created", function()
      local windowName = uniqueName("sgUserWindow")
      local payloads = {}
      local handler = registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, name)
        if name ~= windowName then
          return
        end
        local reportedWidth, reportedHeight = getUserWindowSize(name)
        payloads[#payloads + 1] = {width = width, height = height,
          reportedWidth = reportedWidth, reportedHeight = reportedHeight}
      end)

      finally(function()
        killAnonymousEventHandler(handler)
        hideWindow(windowName)
      end)

      -- loadLayout is off so a saved layout cannot resize the window under us
      openUserWindow(windowName, false)
      pumpEvents(500)

      assert.is_true(#payloads > 0, "opening a user window raised no sysUserWindowResizeEvent")
      for index, payload in ipairs(payloads) do
        assert.is_true(payload.height >= 0,
          ("event %d handed out a height of %d pixels"):format(index, payload.height))
        -- Below 50 pixels of dock width getUserWindowSize answers from a cache
        -- rather than the dock, so the cross-check only means something above it.
        if payload.reportedWidth >= 50 then
          assert.are.same({payload.reportedWidth, payload.reportedHeight}, {payload.width, payload.height},
            ("event %d disagrees with getUserWindowSize"):format(index))
        end
      end
    end)
  end)

  describe("a docked user window opened after another was deleted", function()
    it("gets the height the deleted one had", function()
      local deletedName = uniqueName("sgDeletedUserWindow")
      local laterName = uniqueName("sgLaterUserWindow")
      finally(function()
        deleteMiniConsole(deletedName)
        deleteMiniConsole(laterName)
      end)

      -- On the left: the map widget other specs open stays docked on the right
      -- for the rest of the session, leaving a window docked next to it no height.
      assert.is_true(openUserWindow(deletedName, false, true, "left"), "the first user window did not open")
      pumpEvents(100)
      local _, deletedHeight = getUserWindowSize(deletedName)
      assert.is_true(deletedHeight > 0, "the first user window came up with no height, so there is nothing to compare with")
      -- a console with no dock of its own would have the main window's height
      assert.are_not.equal(select(2, getMainWindowSize()), deletedHeight, "the first user window has no dock of its own")
      deleteMiniConsole(deletedName)
      -- the dock's deferred delete cannot run inside a pump, so it is still alive
      -- when the next window is docked
      pumpEvents(100)

      assert.is_true(openUserWindow(laterName, false, true, "left"), "the second user window did not open")
      pumpEvents(100)
      local _, laterHeight = getUserWindowSize(laterName)
      assert.equals(deletedHeight, laterHeight)
    end)
  end)

  describe("getUserWindowSize", function()
    it("refuses a name no user window has", function()
      local unknown = uniqueName("sgNoSuchUserWindow")
      assert.are.same({nil, ("user window name '%s' not found"):format(unknown)}, {getUserWindowSize(unknown)})
      local miniConsole = uniqueName("sgMiniConsole")
      finally(function() deleteMiniConsole(miniConsole) end)
      assert.is_true(createMiniConsole(miniConsole, 0, 0, 100, 50))
      assert.are.same({nil, ("user window name '%s' not found"):format(miniConsole)}, {getUserWindowSize(miniConsole)})
    end)

    it("answers the main window's size for the main window", function()
      local width, height = getMainWindowSize()
      assert.are.same({width, height}, {getUserWindowSize("main")})
      assert.are.same({width, height}, {getUserWindowSize()})
      -- the creators take "main" in any case
      assert.are.same({width, height}, {getUserWindowSize("Main")})
    end)
  end)
end)
