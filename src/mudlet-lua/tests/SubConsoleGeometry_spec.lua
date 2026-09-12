-- How tall a sub-console is painted, and what size a user window reports as it
-- opens, both come out of the same branch of TConsole::resizeEvent. A console
-- is given its size as it is created, before its own layout has ever run, so
-- anything measured from a child widget at that moment reads a Qt default
-- rather than a real value - which is how every miniconsole and user window
-- once came out the height of a phantom top toolbar short of what it asked for.

describe("Sub-console geometry", function()
  -- user windows cannot be deleted from Lua, so keep the names unique per run
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))

  describe("the height a miniconsole is painted at", function()
    local consoleName = "sgMiniConsole" .. suffix

    after_each(function()
      deleteMiniConsole(consoleName)
    end)

    it("leaves room for as many rows as the font fits into it", function()
      createMiniConsole(consoleName, 20, 20, 200, 200)
      pumpEvents(100)
      local _, characterHeight = calcFontSize(consoleName)
      local rowsThatFit = math.floor(200 / characterHeight + 0.5)
      local rows = getRowCount(consoleName)
      -- a row of slack, because how the pane rounds a part row is a font metric
      -- matter - losing a toolbar's worth of pixels off the bottom is not
      assert.is_true(math.abs(rows - rowsThatFit) <= 1,
        ("a 200 pixel high console has room for %d rows but reports %d"):format(rowsThatFit, rows))
    end)

    it("is the same as soon as it is created as it is after a resize", function()
      createMiniConsole(consoleName, 20, 20, 200, 200)
      pumpEvents(100)
      local atCreation = getRowCount(consoleName)
      -- away and back: the console is measured again, this time laid out
      resizeWindow(consoleName, 200, 300)
      pumpEvents(100)
      resizeWindow(consoleName, 200, 200)
      pumpEvents(100)
      assert.equals(getRowCount(consoleName), atCreation)
    end)

    it("still has a row to draw in when it is only a couple of rows tall", function()
      createMiniConsole(consoleName, 20, 20, 200, 200)
      pumpEvents(100)
      local _, characterHeight = calcFontSize(consoleName)
      deleteMiniConsole(consoleName)

      createMiniConsole(consoleName, 20, 20, 200, 2 * characterHeight)
      pumpEvents(100)
      assert.is_true(getRowCount(consoleName) >= 1, "a console two rows tall shows nothing at all")
    end)
  end)

  describe("sysUserWindowResizeEvent", function()
    it("reports the size the window really is, as it is created", function()
      local windowName = "sgUserWindow" .. suffix
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
        assert.are.same({payload.reportedWidth, payload.reportedHeight}, {payload.width, payload.height},
          ("event %d disagrees with getUserWindowSize"):format(index))
      end
    end)
  end)
end)
