describe("window sizes", function()
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  describe("getMainWindowSize", function()
    -- Only the command line is resized as it grows, not the console around it
    it("follows the command line as it grows with its text", function()
      finally(function()
        clearCmdLine("main")
        pumpEvents(100)
      end)
      clearCmdLine("main")
      pumpEvents(100)
      local width, height = getMainWindowSize()

      printCmdLine("main", "one\ntwo\nthree\nfour")
      pumpEvents(100)

      local grownWidth, grownHeight = getMainWindowSize()
      assert.equals(width, grownWidth)
      assert.is_true(grownHeight < height,
        ("the main window stayed %d high under a four-line command line"):format(grownHeight))

      clearCmdLine("main")
      pumpEvents(100)
      assert.are.same({width, height}, {getMainWindowSize()})
    end)

    -- As with the command line, only the toolbar is resized
    it("follows a button bar shown and hidden along its top", function()
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

      showToolBar(toolbar)
      pumpEvents(100)

      local shownWidth, shownHeight = getMainWindowSize()
      assert.equals(width, shownWidth)
      assert.is_true(shownHeight < height,
        ("the main window stayed %d high with a button bar along its top"):format(shownHeight))

      hideToolBar(toolbar)
      pumpEvents(100)
      assert.are.same({width, height}, {getMainWindowSize()})
    end)
  end)

  describe("getUserWindowSize", function()
    it("follows a floating user window as it is resized", function()
      local name = uniqueName("wsFloatingUserWindow")
      finally(function() deleteMiniConsole(name) end)
      assert.is_true(openUserWindow(name, false, false, "f"), "the user window did not open")
      resizeWindow(name, 300, 200)
      pumpEvents(100)
      local width, height = getUserWindowSize(name)

      resizeWindow(name, 400, 260)
      pumpEvents(100)

      -- The dock's title bar and frame take a fixed number of pixels of what resizeWindow() asks for
      assert.are.same({width + 100, height + 60}, {getUserWindowSize(name)})
    end)

    it("is the main window's size for a name with no user window", function()
      assert.are.same({getMainWindowSize()}, {getUserWindowSize(uniqueName("wsNoSuchUserWindow"))})
    end)
  end)
end)
