describe("window sizes", function()
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  describe("getMainWindowSize", function()
    -- The console keeps its own size as the command line grows into it
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

      -- The dock's title bar and frame take a fixed share of what resizeWindow() asks for
      assert.are.same({width + 100, height + 60}, {getUserWindowSize(name)})
    end)

    it("is the main window's size for a name with no user window", function()
      assert.are.same({getMainWindowSize()}, {getUserWindowSize(uniqueName("wsNoSuchUserWindow"))})
    end)
  end)
end)
