-- Tests for table argument support in Lua functions with 5+ parameters
-- These tests verify that functions accept both positional and table-based arguments

describe("Tests table argument support for functions with 5+ parameters", function()

  describe("Tests prefix() function", function()
    setup(function()
      -- Mock necessary functions
      _G.moveCursor = function() end
      _G.insertText = function() end
      _G.selectString = function() end
      _G.setFgColor = function() end
      _G.setBgColor = function() end
      _G.resetFormat = function() end
    end)

    it("Should accept positional arguments", function()
      -- Test with positional arguments
      local result = pcall(prefix, "test", "echo", "red", "blue", "main")
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      -- Test with table arguments
      local result = pcall(prefix, {
        text = "test",
        func = "echo",
        fgColor = "red",
        bgColor = "blue",
        window = "main"
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local result = pcall(prefix, {
        what = "test",
        writingFunction = "echo",
        foregroundColor = "red",
        backgroundColor = "blue",
        windowName = "main"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests suffix() function", function()
    setup(function()
      _G.moveCursor = function() end
      _G.insertText = function() end
      _G.selectString = function() end
      _G.setFgColor = function() end
      _G.setBgColor = function() end
      _G.resetFormat = function() end
    end)

    it("Should accept positional arguments", function()
      local result = pcall(suffix, "test", "echo", "red", "blue", "main")
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      local result = pcall(suffix, {
        text = "test",
        func = "echo",
        fgColor = "red",
        bgColor = "blue",
        window = "main"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createGauge() function", function()
    setup(function()
      -- Mock gauge-related functions
      _G.gaugesTable = {}
      _G.createLabel = function(container, name, x, y, w, h)
        return true
      end
      _G.setBackgroundColor = function() end
      _G.moveWindow = function() end
      _G.setLabelClickCallback = function() end
    end)

    it("Should accept positional arguments", function()
      local result = pcall(createGauge, "main", "testGauge", 100, 20, 10, 10, "HP", 255, 0, 0, "horizontal")
      assert.is_true(result)
    end)

    it("Should accept table arguments with all parameters", function()
      local result = pcall(createGauge, {
        name = "testGauge2",
        width = 100,
        height = 20,
        Xpos = 10,
        Ypos = 10,
        gaugeText = "HP",
        r = 255,
        g = 0,
        b = 0,
        orientation = "horizontal"
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local result = pcall(createGauge, {
        gaugeName = "testGauge3",
        w = 100,
        h = 20,
        x = 10,
        y = 10,
        text = "HP",
        red = 255,
        green = 0,
        blue = 0,
        orient = "vertical"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createConsole() function", function()
    setup(function()
      -- Mock console-related functions
      _G.createMiniConsole = function() return true end
      _G.setMiniConsoleFontSize = function() end
      _G.setConsoleBufferSize = function() end
    end)

    it("Should accept positional arguments (6 params)", function()
      local result = pcall(createConsole, "testConsole", 10, 80, 20, 100, 100)
      assert.is_true(result)
    end)

    it("Should accept positional arguments (7 params with window)", function()
      local result = pcall(createConsole, "main", "testConsole2", 10, 80, 20, 100, 100)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      local result = pcall(createConsole, {
        name = "testConsole3",
        fontSize = 10,
        charsPerLine = 80,
        numberOfLines = 20,
        Xpos = 100,
        Ypos = 100
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local result = pcall(createConsole, {
        consoleName = "testConsole4",
        size = 10,
        width = 80,
        lines = 20,
        x = 100,
        y = 100,
        windowName = "main"
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests registerNamedEventHandler() function", function()
    setup(function()
      _G.registerAnonymousEventHandler = function() return 1 end
    end)

    it("Should accept positional arguments", function()
      local handler = function() end
      local result = pcall(registerNamedEventHandler, "testUser", "testHandler", "testEvent", handler, false)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      local handler = function() end
      local result = pcall(registerNamedEventHandler, {
        user = "testUser",
        name = "testHandler2",
        event = "testEvent",
        handler = handler,
        oneShot = false
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local handler = function() end
      local result = pcall(registerNamedEventHandler, {
        userName = "testUser",
        handlerName = "testHandler3",
        eventName = "testEvent",
        functionReference = handler
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests registerNamedTimer() function", function()
    setup(function()
      _G.tempTimer = function() return 1 end
      _G.killTimer = function() end
    end)

    it("Should accept positional arguments", function()
      local timerFunc = function() end
      local result = pcall(registerNamedTimer, "testUser", "testTimer", 1.0, timerFunc, true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      local timerFunc = function() end
      local result = pcall(registerNamedTimer, {
        user = "testUser",
        name = "testTimer2",
        time = 1.0,
        handler = timerFunc,
        repeating = true
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local timerFunc = function() end
      local result = pcall(registerNamedTimer, {
        userName = "testUser",
        timerName = "testTimer3",
        interval = 1.0,
        functionReference = timerFunc
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests timeframe() function", function()
    setup(function()
      _G.tempTimer = function() return 1 end
      _G.killTimer = function() end
    end)

    it("Should accept positional arguments", function()
      local result = pcall(timeframe, "testVar", 1.0, 2.0)
      assert.is_true(result)
    end)

    it("Should accept positional arguments with additional timers", function()
      local result = pcall(timeframe, "testVar2", 1.0, 2.0, {3.0, "value3"}, {4.0, "value4"})
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      local result = pcall(timeframe, {
        vname = "testVar3",
        true_time = 1.0,
        nil_time = 2.0,
        timerlist = {{3.0, "value3"}, {4.0, "value4"}}
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      local result = pcall(timeframe, {
        name = "testVar4",
        trueTime = 1.0,
        nilTime = 2.0,
        timers = {{3.0, "value3"}}
      })
      assert.is_true(result)
    end)

    it("Should accept function as vname parameter", function()
      local callback = function(val) end
      local result = pcall(timeframe, callback, 1.0, 2.0)
      assert.is_true(result)
    end)
  end)

end)
