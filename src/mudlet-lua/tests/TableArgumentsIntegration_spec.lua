-- Integration tests for table argument support in C++ Lua functions
-- These tests verify table argument support for functions exposed from C++ to Lua
-- Note: These tests require a full Mudlet environment and may need to be run as integration tests

describe("Tests table argument support for C++ Lua functions with 5+ parameters", function()

  -- Skip these tests if we're not in a full Mudlet environment
  if not mudlet then
    pending("Skipping C++ integration tests - requires full Mudlet environment")
    return
  end

  describe("Tests echoLink() function", function()
    it("Should accept positional arguments with all 5 parameters", function()
      if not echoLink then
        pending("echoLink not available")
        return
      end

      local result = pcall(echoLink, "main", "click me", "send('test')", "tooltip text", true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not echoLink then
        pending("echoLink not available")
        return
      end

      local result = pcall(echoLink, {
        windowName = "main",
        text = "click me",
        command = "send('test')",
        hint = "tooltip text",
        useCurrentFormat = true
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      if not echoLink then
        pending("echoLink not available")
        return
      end

      local result = pcall(echoLink, {
        window = "main",
        linkText = "click me",
        luaCode = "send('test')",
        tooltip = "tooltip text"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests insertLink() function", function()
    it("Should accept positional arguments", function()
      if not insertLink then
        pending("insertLink not available")
        return
      end

      local result = pcall(insertLink, "main", "click me", "send('test')", "tooltip", true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not insertLink then
        pending("insertLink not available")
        return
      end

      local result = pcall(insertLink, {
        windowName = "main",
        text = "click me",
        command = "send('test')",
        hint = "tooltip",
        useCurrentFormat = true
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests echoPopup() function", function()
    it("Should accept positional arguments", function()
      if not echoPopup then
        pending("echoPopup not available")
        return
      end

      local result = pcall(echoPopup, "main", "popup text", {"cmd1", "cmd2"}, {"hint1", "hint2"}, true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not echoPopup then
        pending("echoPopup not available")
        return
      end

      local result = pcall(echoPopup, {
        windowName = "main",
        text = "popup text",
        commands = {"cmd1", "cmd2"},
        hints = {"hint1", "hint2"},
        useCurrentFormat = true
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests insertPopup() function", function()
    it("Should accept positional arguments", function()
      if not insertPopup then
        pending("insertPopup not available")
        return
      end

      local result = pcall(insertPopup, "main", "popup text", {"cmd1", "cmd2"}, {"hint1", "hint2"}, true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not insertPopup then
        pending("insertPopup not available")
        return
      end

      local result = pcall(insertPopup, {
        windowName = "main",
        text = "popup text",
        commands = {"cmd1", "cmd2"},
        hints = {"hint1", "hint2"}
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setCommandBackgroundColor() function", function()
    it("Should accept positional arguments", function()
      if not setCommandBackgroundColor then
        pending("setCommandBackgroundColor not available")
        return
      end

      local result = pcall(setCommandBackgroundColor, "main", 255, 128, 64, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setCommandBackgroundColor then
        pending("setCommandBackgroundColor not available")
        return
      end

      local result = pcall(setCommandBackgroundColor, {
        windowName = "main",
        r = 255,
        g = 128,
        b = 64,
        transparency = 200
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      if not setCommandBackgroundColor then
        pending("setCommandBackgroundColor not available")
        return
      end

      local result = pcall(setCommandBackgroundColor, {
        window = "main",
        red = 255,
        green = 128,
        blue = 64,
        alpha = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setCommandForegroundColor() function", function()
    it("Should accept positional arguments", function()
      if not setCommandForegroundColor then
        pending("setCommandForegroundColor not available")
        return
      end

      local result = pcall(setCommandForegroundColor, "main", 255, 128, 64, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setCommandForegroundColor then
        pending("setCommandForegroundColor not available")
        return
      end

      local result = pcall(setCommandForegroundColor, {
        windowName = "main",
        r = 255,
        g = 128,
        b = 64,
        transparency = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setBackgroundColor() function", function()
    it("Should accept positional arguments", function()
      if not setBackgroundColor then
        pending("setBackgroundColor not available")
        return
      end

      local result = pcall(setBackgroundColor, "main", 255, 128, 64, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setBackgroundColor then
        pending("setBackgroundColor not available")
        return
      end

      local result = pcall(setBackgroundColor, {
        windowName = "main",
        r = 255,
        g = 128,
        b = 64,
        transparency = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setBgColor() function", function()
    it("Should accept positional arguments", function()
      if not setBgColor then
        pending("setBgColor not available")
        return
      end

      local result = pcall(setBgColor, "main", 255, 128, 64, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setBgColor then
        pending("setBgColor not available")
        return
      end

      local result = pcall(setBgColor, {
        windowName = "main",
        r = 255,
        g = 128,
        b = 64,
        transparency = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setCustomEnvColor() function", function()
    it("Should accept positional arguments", function()
      if not setCustomEnvColor then
        pending("setCustomEnvColor not available")
        return
      end

      local result = pcall(setCustomEnvColor, 255, 255, 128, 64, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setCustomEnvColor then
        pending("setCustomEnvColor not available")
        return
      end

      local result = pcall(setCustomEnvColor, {
        environmentID = 255,
        r = 255,
        g = 128,
        b = 64,
        alpha = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests permKey() function", function()
    it("Should accept positional arguments without modifier", function()
      if not permKey then
        pending("permKey not available")
        return
      end

      local result = pcall(permKey, "testKey", "testGroup", string.byte('a'), "send('test')")
      assert.is_true(result)
    end)

    it("Should accept positional arguments with modifier", function()
      if not permKey then
        pending("permKey not available")
        return
      end

      -- Qt::ControlModifier = 0x02000000
      local result = pcall(permKey, "testKey2", "testGroup", 0x02000000, string.byte('a'), "send('test')")
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not permKey then
        pending("permKey not available")
        return
      end

      local result = pcall(permKey, {
        name = "testKey3",
        parent = "testGroup",
        keyCode = string.byte('a'),
        code = "send('test')"
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with modifier", function()
      if not permKey then
        pending("permKey not available")
        return
      end

      local result = pcall(permKey, {
        name = "testKey4",
        parentGroup = "testGroup",
        modifier = 0x02000000,
        key = string.byte('a'),
        luaCode = "send('test')"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createMapper() function", function()
    it("Should accept positional arguments", function()
      if not createMapper then
        pending("createMapper not available")
        return
      end

      local result = pcall(createMapper, 0, 0, 300, 300)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not createMapper then
        pending("createMapper not available")
        return
      end

      local result = pcall(createMapper, {
        x = 0,
        y = 0,
        width = 300,
        height = 300
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests addMapEvent() function", function()
    it("Should accept positional arguments", function()
      if not addMapEvent then
        pending("addMapEvent not available")
        return
      end

      local result = pcall(addMapEvent, "uniqueEvent", "eventName", "parent", "displayName", "arg1,arg2")
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not addMapEvent then
        pending("addMapEvent not available")
        return
      end

      local result = pcall(addMapEvent, {
        uniquename = "uniqueEvent2",
        event = "eventName",
        parent = "parent",
        displayname = "displayName",
        arguments = "arg1,arg2"
      })
      assert.is_true(result)
    end)
  end)

end)
