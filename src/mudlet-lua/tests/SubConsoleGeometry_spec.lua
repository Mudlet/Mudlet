-- How tall a sub-console is painted, and what size a user window reports as it
-- opens, both come out of TConsole::resizeEvent. A console is given its size as
-- it is created, before its own layout has ever run, so anything measured from a
-- child widget at that moment reads a Qt default rather than a real value -
-- which is how every miniconsole and user window once came out the height of a
-- phantom top toolbar short of what it asked for.

describe("Sub-console geometry", function()
  -- user windows cannot be deleted from Lua, so keep the names unique per run.
  -- math.random is unseeded in Lua 5.1, so it contributes nothing on its own;
  -- the counter is what separates two runs inside the same second.
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  describe("the height a miniconsole is painted at", function()
    local consoleName = uniqueName("sgMiniConsole")

    -- 100ms is the layout pass the creation queues; nothing here is measurable
    -- before it has run.
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
      -- The slack is one-sided on purpose. How the pane rounds a part row is a
      -- font metric matter and may leave one row over, but the toolbar's worth
      -- of pixels this is about only ever goes missing - and on a font tall
      -- enough it is exactly the one row a two-sided slack would have allowed.
      assert.is_true(rows >= rowsThatFit,
        ("a 200 pixel high console has room for %d rows of %d pixels but reports %d"):format(rowsThatFit, characterHeight, rows))
      assert.is_true(rows <= rowsThatFit + 1,
        ("a 200 pixel high console has room for %d rows of %d pixels but reports %d"):format(rowsThatFit, characterHeight, rows))
    end)

    -- The load-bearing one: at creation against after a resize, which is the
    -- same measurement twice and so cannot be fooled by any font metric.
    it("is the same as soon as it is created as it is after a resize", function()
      createConsole(200)
      local atCreation = getRowCount(consoleName)
      -- away and back: the console is measured again, this time laid out
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

      -- Just over one row tall, and under a row plus the missing strip whatever
      -- the font is: any taller and a large font would still fit a row into
      -- what is left, so the case would stop biting on the machines that have
      -- one.
      createConsole(characterHeight + 10)
      assert.is_true(getRowCount(consoleName) >= 1, "a console barely over one row tall shows nothing at all")
    end)

    -- The other branch of the same function: a console with a visible command
    -- line is resized through its own frame first, so the top bar it measures
    -- has been laid out. Nothing else covers that branch.
    it("keeps its rows when it is created with a command line", function()
      -- Both in one turn, which is how a script writes it, and which leaves the
      -- command line asking for the console's size before any layout has run -
      -- the same moment the branch above is measured at.
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
        -- getUserWindowSize answers from a cache rather than the dock below 50
        -- pixels wide, and a docked user window can be exactly that narrow, so
        -- the cross-check only means something above that width.
        if payload.reportedWidth >= 50 then
          assert.are.same({payload.reportedWidth, payload.reportedHeight}, {payload.width, payload.height},
            ("event %d disagrees with getUserWindowSize"):format(index))
        end
      end
    end)
  end)
end)
