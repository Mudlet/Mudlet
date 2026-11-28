describe("getMainWindowInfo", function()
  local classes = { compact = true, medium = true, expanded = true }

  local function widthClass(width)
    return width < 600 and "compact" or width < 840 and "medium" or "expanded"
  end

  local function heightClass(height)
    return height < 480 and "compact" or height < 900 and "medium" or "expanded"
  end

  local function kind(info)
    return ("%s/%s/%s"):format(info.widthClass, info.heightClass, info.orientation)
  end

  it("describes the main window", function()
    local info = getMainWindowInfo()
    assert.is_table(info)
    assert.is_true(type(info.width) == "number" and info.width >= 0)
    assert.is_true(type(info.height) == "number" and info.height >= 0)
    assert.is_true(classes[info.widthClass], tostring(info.widthClass))
    assert.is_true(classes[info.heightClass], tostring(info.heightClass))
    assert.equals(widthClass(info.width), info.widthClass)
    assert.equals(heightClass(info.height), info.heightClass)
    assert.equals(info.height >= info.width and "portrait" or "landscape", info.orientation)
    assert.is_true(info.pointer == "fine" or info.pointer == "coarse", tostring(info.pointer))
    assert.is_boolean(info.hover)
    assert.is_true(type(info.devicePixelRatio) == "number" and info.devicePixelRatio > 0)
    assert.is_true(type(info.pixelScale) == "number" and info.pixelScale > 0)
  end)

  it("gives a new table each call", function()
    local first = getMainWindowInfo()
    first.width = -1
    assert.is_not.equals(-1, getMainWindowInfo().width)
  end)

  it("is the window getMainWindowSize() measures, in logical pixels", function()
    local info = getMainWindowInfo()
    local width, height = getMainWindowSize()
    assert.are.same({ width / info.pixelScale, height / info.pixelScale }, { info.width, info.height })
  end)

  describe("sysMainWindowClassChangeEvent", function()
    local restoreWidth, restoreHeight
    local heard, handlers

    setup(function()
      local width, height = getMainWindowSize()
      assert.is_true(setMainWindowSize(1000, 1400))
      pumpEvents(100)
      local innerWidth, innerHeight = getMainWindowSize()
      -- setMainWindowSize() sizes the whole window and getMainWindowSize() the console in it
      restoreWidth, restoreHeight = width + 1000 - innerWidth, height + 1400 - innerHeight
    end)

    teardown(function()
      setMainWindowSize(restoreWidth, restoreHeight)
      pumpEvents(100)
    end)

    before_each(function()
      heard = {}
      handlers = {
        registerAnonymousEventHandler("sysWindowResizeEvent", function(_, _, _, name)
          if name == "main" then
            heard[#heard + 1] = "resize"
          end
        end),
        registerAnonymousEventHandler("sysMainWindowClassChangeEvent", function(...)
          heard[#heard + 1] = ("class(%d)"):format(select("#", ...) - 1)
        end),
      }
    end)

    after_each(function()
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
    end)

    it("is raised once, after the resize, when a class changes", function()
      setMainWindowSize(1000, 1400)
      pumpEvents(100)
      local before = kind(getMainWindowInfo())
      heard = {}

      setMainWindowSize(1000, 700)
      pumpEvents(100)

      local after = kind(getMainWindowInfo())
      assert.is_not.equals(before, after, "the resize left the main window in the same classes")
      assert.equals("class(0)", heard[#heard], table.concat(heard, " "))
      assert.equals("resize", heard[#heard - 1], table.concat(heard, " "))
      local raised = 0
      for _, name in ipairs(heard) do
        raised = raised + (name == "class(0)" and 1 or 0)
      end
      assert.equals(1, raised, table.concat(heard, " "))
    end)

    it("is not raised for a resize within the same classes", function()
      setMainWindowSize(1000, 700)
      pumpEvents(100)
      local before = kind(getMainWindowInfo())
      heard = {}

      setMainWindowSize(1040, 730)
      pumpEvents(100)

      assert.equals(before, kind(getMainWindowInfo()))
      assert.is_true(#heard > 0, "the main window was not resized")
      for _, name in ipairs(heard) do
        assert.equals("resize", name, table.concat(heard, " "))
      end
    end)
  end)
end)

describe("setAdaptiveLayout", function()
  after_each(function()
    setAdaptiveLayout("MainWindowInfo_spec", false)
  end)

  it("takes an owner and whether it lays out for any shape", function()
    assert.is_true(setAdaptiveLayout("MainWindowInfo_spec", true))
    assert.is_true(setAdaptiveLayout("MainWindowInfo_spec", true))
    assert.is_true(setAdaptiveLayout("MainWindowInfo_spec", false))
    assert.is_true(setAdaptiveLayout("MainWindowInfo_spec", false))
  end)

  it("refuses an empty owner", function()
    local ok, message = setAdaptiveLayout("", true)
    assert.is_nil(ok)
    assert.equals("owner must not be an empty string", message)
  end)

  it("raises for arguments of the wrong type", function()
    assert.has_error(function() setAdaptiveLayout() end)
    assert.has_error(function() setAdaptiveLayout("MainWindowInfo_spec") end)
    assert.has_error(function() setAdaptiveLayout("MainWindowInfo_spec", "yes") end)
    assert.has_error(function() setAdaptiveLayout(true, true) end)
  end)

  -- Laying out for the real shape changes nothing where the main window always has it
  it("leaves the desktop's main window as it was", function()
    if getMainWindowInfo().pixelScale ~= 1 then
      return
    end
    local width, height = getMainWindowSize()
    setAdaptiveLayout("MainWindowInfo_spec", true)
    pumpEvents(100)
    assert.are.same({ width, height }, { getMainWindowSize() })
  end)
end)
