describe("window sizes", function()
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  describe("getMainWindowSize", function()
    local restoreWidth, restoreHeight

    -- getRowCount() measures the console's text area itself, so a failure says whether the
    -- layout moved and only the reported size missed it, or the layout never moved at all
    local function measure(label)
      local width, height = getMainWindowSize()
      return ("%s %dx%d, %d rows"):format(label, width, height, getRowCount())
    end

    local function watchResizes()
      local seen = {}
      local handler = registerAnonymousEventHandler("sysWindowResizeEvent", function(_, width, height)
        seen[#seen + 1] = ("%dx%d"):format(width, height)
      end)
      return seen, handler
    end

    -- Earlier specs can leave the main window a few pixels high, which gives a
    -- command line or button bar nothing to take its room from
    setup(function()
      local width, height = getMainWindowSize()
      setMainWindowSize(1000, 1400)
      pumpEvents(100)
      local innerWidth, innerHeight = getMainWindowSize()
      -- setMainWindowSize() sizes the whole window and getMainWindowSize() the
      -- console in it, so put back the chrome measured around it here
      restoreWidth, restoreHeight = width + 1000 - innerWidth, height + 1400 - innerHeight
      assert.is_true(innerHeight > 200, "could not make the main window tall enough")
    end)

    teardown(function()
      setMainWindowSize(restoreWidth, restoreHeight)
      pumpEvents(100)
    end)

    -- Only the command line is resized as it grows, not the console around it
    it("follows the command line as it grows with its text", function()
      finally(function()
        clearCmdLine("main")
        pumpEvents(100)
      end)
      clearCmdLine("main")
      pumpEvents(100)
      local width, height = getMainWindowSize()
      local before = measure("before")
      local resizes, handler = watchResizes()

      printCmdLine("main", "one\ntwo\nthree\nfour")
      pumpEvents(100)

      local grownWidth, grownHeight = getMainWindowSize()
      if grownHeight >= height then
        local after = measure("after")
        pumpEvents(1000)
        killAnonymousEventHandler(handler)
        assert.is_true(false, ("the main window stayed %d high under a four-line command line (%s; %s; %s; resize events: %s)")
          :format(grownHeight, before, after, measure("a second later"), table.concat(resizes, " ")))
      end
      killAnonymousEventHandler(handler)
      assert.equals(width, grownWidth)

      clearCmdLine("main")
      pumpEvents(100)
      assert.are.same({width, height}, {getMainWindowSize()})
    end)

    -- As with the command line, only the toolbar is resized
    it("follows a button bar shown along its top", function()
      local toolbar = "wsTopToolbar"
      finally(function()
        hideToolBar(toolbar)
        pumpEvents(100)
      end)
      if exists(toolbar, "button") == 0 then
        assert.is_true(tempButtonToolbar(toolbar, 0, 0) > 0)
      end
      local buttonId = findItems("wsTopButton", "button")[1] or tempButton(toolbar, "wsTopButton", 0)
      assert.is_true(type(buttonId) == "number" and buttonId > 0, "could not put a button on " .. toolbar)
      hideToolBar(toolbar)
      pumpEvents(100)
      local width, height = getMainWindowSize()
      local before = measure("hidden")
      local resizes, handler = watchResizes()

      showToolBar(toolbar)
      pumpEvents(100)

      local shownWidth, shownHeight = getMainWindowSize()
      if shownHeight >= height then
        local after = measure("shown")
        pumpEvents(1000)
        killAnonymousEventHandler(handler)
        assert.is_true(false, ("the main window stayed %d high with a button bar along its top (%s; %s; %s; resize events: %s)")
          :format(shownHeight, before, after, measure("a second later"), table.concat(resizes, " ")))
      end
      killAnonymousEventHandler(handler)
      assert.equals(width, shownWidth)
      -- No check that hiding it gives the room back: toolbars other specs leave
      -- along the top can keep that area at the height it grew to
    end)
  end)

  describe("getUserWindowSize", function()
    -- Geyser reads the size back from inside the resize event
    it("follows a floating user window as it is resized", function()
      local name = uniqueName("wsFloatingUserWindow")
      local seen
      local handler = registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, windowName)
        if windowName == name then
          seen = {{width, height}, {getUserWindowSize(name)}}
        end
      end)
      finally(function()
        killAnonymousEventHandler(handler)
        deleteMiniConsole(name)
      end)
      assert.is_true(openUserWindow(name, false, false, "f"), "the user window did not open")
      resizeWindow(name, 300, 200)
      pumpEvents(100)
      local width, height = getUserWindowSize(name)

      seen = nil
      resizeWindow(name, 400, 260)
      pumpEvents(100)

      -- The dock's title bar and frame take a fixed number of pixels of what resizeWindow() asks for
      assert.are.same({width + 100, height + 60}, {getUserWindowSize(name)})
      assert.is_truthy(seen, "resizing the window raised no sysUserWindowResizeEvent")
      assert.are.same(seen[1], seen[2])
    end)

    it("is the main window's size for a name with no user window", function()
      assert.are.same({getMainWindowSize()}, {getUserWindowSize(uniqueName("wsNoSuchUserWindow"))})
    end)
  end)
end)
