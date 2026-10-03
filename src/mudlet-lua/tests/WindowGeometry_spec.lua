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

    it("follows a floating user window as it is resized", function()
      local name = uniqueName("wgFloatingUserWindow")
      finally(function() deleteMiniConsole(name) end)
      assert.is_true(openUserWindow(name, false, false, "f"), "the user window did not open")
      hideWindow(name)
      resizeWindow(name, 321, 234)
      assert.are.same({321, 234}, {select(3, getWindowGeometry(name))})
    end)

    it("follows a label a style sheet gives a minimum size", function()
      local name = uniqueName("wgStyledLabel")
      finally(function() deleteLabel(name) end)
      createLabel("main", name, 10, 20, 30, 40, 1)
      hideWindow(name)
      setLabelStyleSheet(name, "min-width: 200px; min-height: 100px;")
      assert.are.same({200, 100}, {select(3, getWindowGeometry(name))})
    end)
  end)

  describe("of a window in a hidden scroll box", function()
    local scrollBox

    before_each(function()
      scrollBox = uniqueName("wgHiddenScrollBox")
      createScrollBox("main", scrollBox, 10, 20, 300, 300)
      hideWindow(scrollBox)
    end)

    after_each(function()
      deleteScrollBox(scrollBox)
    end)

    it("is where each kind of window was created", function()
      local label, console, commandLine, textEdit =
        uniqueName("wgLabel"), uniqueName("wgMiniConsole"), uniqueName("wgCommandLine"), uniqueName("wgTextEdit")
      createLabel(scrollBox, label, 5, 6, 40, 20, 1)
      createMiniConsole(scrollBox, console, 7, 8, 200, 100)
      createCommandLine(scrollBox, commandLine, 9, 10, 140, 35)
      createTextEdit(scrollBox, textEdit, 11, 12, 160, 110)
      assert.are.same({5, 6, 40, 20}, geometry(label))
      assert.are.same({7, 8, 200, 100}, geometry(console))
      assert.are.same({9, 10, 140, 35}, geometry(commandLine))
      assert.are.same({11, 12, 160, 110}, geometry(textEdit))
    end)

    -- setParent() hides the widget, and showing a child of a hidden parent flushes nothing
    it("is where setWindow put it", function()
      local label, console = uniqueName("wgLabel"), uniqueName("wgMiniConsole")
      finally(function()
        deleteLabel(label)
        deleteMiniConsole(console)
      end)
      createLabel("main", label, 11, 22, 100, 50, 1)
      createMiniConsole("main", console, 10, 20, 200, 100)
      assert.is_true(setWindow(scrollBox, label, 3, 4, true))
      assert.is_true(setWindow(scrollBox, console, 5, 6, false))
      assert.are.same({3, 4, 100, 50}, geometry(label))
      assert.are.same({5, 6, 200, 100}, geometry(console))
    end)
  end)

  -- Nothing a script does sizes a docked user window: the main window's layout does
  it("follows a docked user window as the main window lays its dock area out again", function()
    local first, second = uniqueName("wgFirstUserWindow"), uniqueName("wgSecondUserWindow")
    finally(function()
      deleteMiniConsole(first)
      deleteMiniConsole(second)
    end)
    assert.is_true(openUserWindow(first, false, true, "left"), "the first user window did not open")
    pumpEvents(100)
    local _, _, _, heightAlone = getWindowGeometry(first)
    assert.is_true(openUserWindow(second, false, true, "left"), "the second user window did not open")
    pumpEvents(100)
    local _, _, _, heightShared = getWindowGeometry(first)
    assert.is_true(heightShared < heightAlone,
      ("sharing the dock area left the first window %d pixels tall, as it was alone"):format(heightShared))
  end)

  -- QMainWindow::restoreState() places a hidden dock without sending it any event
  describe("of a hidden user window after loadWindowLayout", function()
    local configurationDirectory = getMudletHomeDir():match("^(.*)/profiles/[^/]*$")
    local layoutFiles = {
      configurationDirectory .. "/windowLayout.dat",
      configurationDirectory .. "/windowLayoutGeometry.dat",
    }
    local contentsBefore = {}

    setup(function()
      for _, path in ipairs(layoutFiles) do
        local handle = io.open(path, "rb")
        if handle then
          contentsBefore[path] = handle:read("*a")
          handle:close()
        end
      end
    end)

    -- the shared files the next Mudlet start reads its layout from
    teardown(function()
      for _, path in ipairs(layoutFiles) do
        if contentsBefore[path] then
          local handle = assert(io.open(path, "wb"))
          handle:write(contentsBefore[path])
          handle:close()
        else
          os.remove(path)
        end
      end
    end)

    it("is the size the layout saved", function()
      local name = uniqueName("wgLayoutUserWindow")
      finally(function() deleteMiniConsole(name) end)
      assert.is_true(openUserWindow(name, false, false, "f"), "the user window did not open")
      resizeWindow(name, 300, 200)
      hideWindow(name)
      assert.is_true(saveWindowLayout())
      resizeWindow(name, 420, 330)
      assert.is_true(loadWindowLayout())
      assert.are.same({300, 200}, {select(3, getWindowGeometry(name))})
    end)
  end)

  it("forgets a deleted label", function()
    local name = uniqueName("wgDeletedLabel")
    createLabel("main", name, 10, 20, 30, 40, 1)
    deleteLabel(name)
    assert.is_nil(getWindowGeometry(name))
  end)
end)
