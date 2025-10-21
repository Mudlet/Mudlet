-- Tests for complex functions with many parameters (10+)
-- These functions benefit most from table argument syntax

describe("Tests table argument support for complex functions with 10+ parameters", function()

  -- Skip these tests if we're not in a full Mudlet environment
  if not mudlet then
    pending("Skipping complex function tests - requires full Mudlet environment")
    return
  end

  describe("Tests createMapLabel() function with 11 required + 7 optional parameters", function()
    it("Should accept minimum positional arguments (11 required params)", function()
      if not createMapLabel then
        pending("createMapLabel not available")
        return
      end

      -- area, text, x, y, z, fgR, fgG, fgB, bgR, bgG, bgB
      local result = pcall(createMapLabel, 1, "Test Label", 0, 0, 0, 255, 255, 255, 0, 0, 0)
      assert.is_true(result)
    end)

    it("Should accept all 18 positional arguments", function()
      if not createMapLabel then
        pending("createMapLabel not available")
        return
      end

      -- All params: area, text, x, y, z, fgR, fgG, fgB, bgR, bgG, bgB, zoom, fontSize, showOnTop, noScaling, fontName, fgAlpha, bgAlpha
      local result = pcall(createMapLabel, 1, "Full Label", 0, 0, 0, 255, 255, 255, 0, 0, 0, 30.0, 50, true, true, "Ubuntu", 255, 50)
      assert.is_true(result)
    end)

    it("Should accept table arguments with required parameters", function()
      if not createMapLabel then
        pending("createMapLabel not available")
        return
      end

      local result = pcall(createMapLabel, {
        area = 1,
        text = "Table Label",
        posX = 0,
        posY = 0,
        posZ = 0,
        fgRed = 255,
        fgGreen = 255,
        fgBlue = 255,
        bgRed = 0,
        bgGreen = 0,
        bgBlue = 0
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with all optional parameters", function()
      if not createMapLabel then
        pending("createMapLabel not available")
        return
      end

      local result = pcall(createMapLabel, {
        areaID = 1,
        labelText = "Full Table Label",
        x = 0,
        y = 0,
        z = 0,
        fgR = 255,
        fgG = 255,
        fgB = 255,
        bgR = 0,
        bgG = 0,
        bgB = 0,
        zoom = 30.0,
        fontSize = 50,
        showOnTop = true,
        noScaling = true,
        fontName = "Ubuntu",
        foregroundTransparency = 255,
        backgroundTransparency = 50,
        temporary = false
      })
      assert.is_true(result)
    end)

    it("Should accept mixed case table key names", function()
      if not createMapLabel then
        pending("createMapLabel not available")
        return
      end

      local result = pcall(createMapLabel, {
        AREA = 1,
        TEXT = "Case Test",
        POSX = 0,
        posy = 0,
        PosZ = 0,
        FGRED = 255,
        FgGreen = 255,
        fgblue = 255,
        BGRED = 0,
        bggreen = 0,
        BGBlue = 0
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests setTextFormat() function with 13 parameters", function()
    it("Should accept positional arguments", function()
      if not setTextFormat then
        pending("setTextFormat not available")
        return
      end

      -- windowName, r1, g1, b1, r2, g2, b2, bold, underline, italics, strikeout, overline, reverse
      local result = pcall(setTextFormat, "main", 255, 255, 255, 0, 0, 0, true, false, false, false, false, false)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not setTextFormat then
        pending("setTextFormat not available")
        return
      end

      local result = pcall(setTextFormat, {
        windowName = "main",
        fgRed = 255,
        fgGreen = 255,
        fgBlue = 255,
        bgRed = 0,
        bgGreen = 0,
        bgBlue = 0,
        bold = true,
        underline = false,
        italics = false,
        strikeout = false,
        overline = false,
        reverse = false
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with alternative key names", function()
      if not setTextFormat then
        pending("setTextFormat not available")
        return
      end

      local result = pcall(setTextFormat, {
        window = "main",
        r1 = 255,
        g1 = 255,
        b1 = 255,
        r2 = 0,
        g2 = 0,
        b2 = 0,
        b = true,  -- bold
        u = false, -- underline
        i = false, -- italics
        s = false, -- strikeout
        o = false, -- overline
        rev = false -- reverse
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests tempComplexRegexTrigger() function with 14 parameters", function()
    it("Should accept positional arguments", function()
      if not tempComplexRegexTrigger then
        pending("tempComplexRegexTrigger not available")
        return
      end

      -- name, pattern, code, multiline, fg, bg, filter, matchall, hlFg, hlBg, soundFile, fireLen, lineDelta, expiry
      local result = pcall(tempComplexRegexTrigger,
        "test", ".*test.*", "send('found')",
        false, 0, 0, false, false,
        0, 0, "", 0, 0, -1
      )
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not tempComplexRegexTrigger then
        pending("tempComplexRegexTrigger not available")
        return
      end

      local result = pcall(tempComplexRegexTrigger, {
        name = "test2",
        pattern = ".*test.*",
        code = "send('found')",
        multiLine = false,
        colourTriggerFgColor = 0,
        colourTriggerBgColor = 0,
        filterTrigger = false,
        matchAll = false,
        highlightFgColor = 0,
        highlightBgColor = 0,
        soundTrigger = "",
        fireLength = 0,
        lineDelta = 0,
        expireAfter = -1
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with function code", function()
      if not tempComplexRegexTrigger then
        pending("tempComplexRegexTrigger not available")
        return
      end

      local codeFunc = function() send('found') end
      local result = pcall(tempComplexRegexTrigger, {
        triggerName = "test3",
        regex = ".*test.*",
        luaFunction = codeFunc,
        multiline = false,
        fgColor = 0,
        bgColor = 0,
        filter = false,
        matchAllLines = false,
        hlFg = 0,
        hlBg = 0,
        sound = "",
        fireLen = 0,
        delta = 0,
        expiry = -1
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests highlightRoom() function with 10 parameters", function()
    it("Should accept positional arguments", function()
      if not highlightRoom then
        pending("highlightRoom not available")
        return
      end

      -- roomID, r1, g1, b1, r2, g2, b2, radius, alpha1, alpha2
      local result = pcall(highlightRoom, 1, 255, 0, 0, 128, 0, 0, 1.0, 255, 128)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not highlightRoom then
        pending("highlightRoom not available")
        return
      end

      local result = pcall(highlightRoom, {
        roomID = 1,
        color1Red = 255,
        color1Green = 0,
        color1Blue = 0,
        color2Red = 128,
        color2Green = 0,
        color2Blue = 0,
        highlightRadius = 1.0,
        color1Alpha = 255,
        color2Alpha = 128
      })
      assert.is_true(result)
    end)

    it("Should accept table arguments with abbreviated key names", function()
      if not highlightRoom then
        pending("highlightRoom not available")
        return
      end

      local result = pcall(highlightRoom, {
        id = 1,
        r1 = 255,
        g1 = 0,
        b1 = 0,
        r2 = 128,
        g2 = 0,
        b2 = 0,
        radius = 1.0,
        a1 = 255,
        a2 = 128
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createMapImageLabel() function with 9 parameters", function()
    it("Should accept positional arguments", function()
      if not createMapImageLabel then
        pending("createMapImageLabel not available")
        return
      end

      -- area, filePath, x, y, z, width, height, zoom, showOnTop
      local result = pcall(createMapImageLabel, 1, "/tmp/test.png", 0, 0, 0, 100, 100, 30.0, true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not createMapImageLabel then
        pending("createMapImageLabel not available")
        return
      end

      local result = pcall(createMapImageLabel, {
        areaID = 1,
        imagePath = "/tmp/test.png",
        posX = 0,
        posY = 0,
        posZ = 0,
        width = 100,
        height = 100,
        zoom = 30.0,
        showOnTop = true,
        temporary = false
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createLabel() function with 8 parameters", function()
    it("Should accept positional arguments", function()
      if not createLabel then
        pending("createLabel not available")
        return
      end

      -- userwindow, name, x, y, width, height, fillBg, clickthrough
      local result = pcall(createLabel, "main", "testLabel", 0, 0, 100, 50, true, false)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not createLabel then
        pending("createLabel not available")
        return
      end

      local result = pcall(createLabel, {
        name = "testLabel2",
        x = 0,
        y = 0,
        width = 100,
        height = 50,
        fillBackground = true,
        enableClickthrough = false
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests addCustomLine() function with 6 parameters", function()
    it("Should accept positional arguments", function()
      if not addCustomLine then
        pending("addCustomLine not available")
        return
      end

      -- roomID, targetID, direction, style, color, arrow
      local result = pcall(addCustomLine, 1, 2, "north", "solid line", {255, 0, 0}, true)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not addCustomLine then
        pending("addCustomLine not available")
        return
      end

      local result = pcall(addCustomLine, {
        roomID = 1,
        targetID = 2,
        direction = "north",
        style = "solid line",
        color = {255, 0, 0},
        arrow = true
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createMiniConsole() function with 6 parameters", function()
    it("Should accept positional arguments", function()
      if not createMiniConsole then
        pending("createMiniConsole not available")
        return
      end

      -- userwindow, name, x, y, width, height
      local result = pcall(createMiniConsole, "main", "testMini", 0, 0, 300, 200)
      assert.is_true(result)
    end)

    it("Should accept table arguments", function()
      if not createMiniConsole then
        pending("createMiniConsole not available")
        return
      end

      local result = pcall(createMiniConsole, {
        name = "testMini2",
        x = 0,
        y = 0,
        width = 300,
        height = 200
      })
      assert.is_true(result)
    end)
  end)

  describe("Tests createCommandLine() and createScrollBox() functions with 6 parameters", function()
    it("createCommandLine should accept table arguments", function()
      if not createCommandLine then
        pending("createCommandLine not available")
        return
      end

      local result = pcall(createCommandLine, {
        name = "testCmd",
        x = 0,
        y = 0,
        width = 300,
        height = 30
      })
      assert.is_true(result)
    end)

    it("createScrollBox should accept table arguments", function()
      if not createScrollBox then
        pending("createScrollBox not available")
        return
      end

      local result = pcall(createScrollBox, {
        name = "testScroll",
        x = 0,
        y = 0,
        width = 300,
        height = 200
      })
      assert.is_true(result)
    end)
  end)

end)
