describe("getWindowGeometry", function()
  local runCounter = 0
  local function uniqueName(stem)
    runCounter = runCounter + 1
    return ("%s-%d-%d"):format(stem, os.time(), runCounter)
  end

  local function geometry(name)
    return {getWindowGeometry(name)}
  end

  -- A hidden widget gets no Move or Resize events until it is shown again
  describe("of a hidden window", function()
    it("follows a label as it is moved and resized", function()
      local name = uniqueName("wgLabel")
      finally(function() deleteLabel(name) end)
      createLabel("main", name, 10, 20, 30, 40, 1)
      hideWindow(name)
      moveWindow(name, 50, 60)
      resizeWindow(name, 70, 80)
      assert.are.same({50, 60, 70, 80}, geometry(name))
    end)

    it("follows a miniconsole as it is moved and resized", function()
      local name = uniqueName("wgMiniConsole")
      finally(function() deleteMiniConsole(name) end)
      createMiniConsole("main", name, 10, 20, 200, 100)
      hideWindow(name)
      moveWindow(name, 50, 60)
      resizeWindow(name, 170, 180)
      assert.are.same({50, 60, 170, 180}, geometry(name))
    end)

    it("follows a scroll box as it is moved and resized", function()
      local name = uniqueName("wgScrollBox")
      finally(function() deleteScrollBox(name) end)
      createScrollBox("main", name, 10, 20, 100, 100)
      hideWindow(name)
      moveWindow(name, 50, 60)
      resizeWindow(name, 170, 180)
      assert.are.same({50, 60, 170, 180}, geometry(name))
    end)
  end)

  it("follows a docked user window to the size the main window laid it out at", function()
    local name = uniqueName("wgUserWindow")
    finally(function() deleteMiniConsole(name) end)
    assert.is_true(openUserWindow(name, false, true, "left"), "the user window did not open")
    pumpEvents(100)
    local _, _, width, height = getWindowGeometry(name)
    assert.are.same({getUserWindowSize(name)}, {width, height})
  end)

  it("forgets a deleted label", function()
    local name = uniqueName("wgDeletedLabel")
    createLabel("main", name, 10, 20, 30, 40, 1)
    deleteLabel(name)
    assert.is_nil(getWindowGeometry(name))
  end)
end)
