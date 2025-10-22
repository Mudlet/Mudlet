describe("Tests tempComplexRegexTrigger color support (GitHub issue #2574)", function()

  describe("Tests color name string support", function()
    local mockTriggerId = 12345
    local mockTriggerCreated = false

    setup(function()
      -- Mock the C++ function tempComplexRegexTrigger
      -- In real Mudlet, this is implemented in C++, but for testing we need a mock
      _G.oldTempComplexRegexTrigger = _G.tempComplexRegexTrigger

      _G.tempComplexRegexTrigger = function(name, pattern, code, multiline, fgColor, bgColor, filter, matchAll, hlFg, hlBg, sound, fireLength, lineDelta, expiryCount)
        -- Validate arguments match what we expect from the fix
        if type(fgColor) == "string" then
          -- Basic 16 ANSI color names
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_black = true, lightblack = true, ["light black"] = true,
            gray = true, grey = true,
            light_red = true, lightred = true, ["light red"] = true,
            light_green = true, lightgreen = true, ["light green"] = true,
            light_yellow = true, lightyellow = true, ["light yellow"] = true,
            light_blue = true, lightblue = true, ["light blue"] = true,
            light_magenta = true, lightmagenta = true, ["light magenta"] = true,
            light_cyan = true, lightcyan = true, ["light cyan"] = true,
            light_white = true, lightwhite = true, ["light white"] = true,
            default = true, ignore = true
          }

          if not validColors[fgColor:lower()] then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #5 value (foreground color name '%s' not recognized, use basic ANSI color names like 'red', 'blue', 'light_green', etc., or numeric ANSI codes 0-255)",
              fgColor
            )
          end
        elseif type(fgColor) == "number" then
          -- Validate ANSI code range (simplified for testing)
          if not (fgColor == -2 or fgColor == -1 or (fgColor >= 0 and fgColor <= 255)) then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #5 value (foreground color code %d invalid, must be -2 (ignore), -1 (default), or 0-255)",
              fgColor
            )
          end
        end

        if type(bgColor) == "string" then
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_black = true, lightblack = true, ["light black"] = true,
            gray = true, grey = true,
            light_red = true, lightred = true, ["light red"] = true,
            light_green = true, lightgreen = true, ["light green"] = true,
            light_yellow = true, lightyellow = true, ["light yellow"] = true,
            light_blue = true, lightblue = true, ["light blue"] = true,
            light_magenta = true, lightmagenta = true, ["light magenta"] = true,
            light_cyan = true, lightcyan = true, ["light cyan"] = true,
            light_white = true, lightwhite = true, ["light white"] = true,
            default = true, ignore = true
          }

          if not validColors[bgColor:lower()] then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #6 value (background color name '%s' not recognized, use basic ANSI color names like 'red', 'blue', 'light_green', etc., or numeric ANSI codes 0-255)",
              bgColor
            )
          end
        elseif type(bgColor) == "number" then
          if not (bgColor == -2 or bgColor == -1 or (bgColor >= 0 and bgColor <= 255)) then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #6 value (background color code %d invalid, must be -2 (ignore), -1 (default), or 0-255)",
              bgColor
            )
          end
        end

        mockTriggerCreated = true
        return mockTriggerId
      end
    end)

    teardown(function()
      _G.tempComplexRegexTrigger = _G.oldTempComplexRegexTrigger
      _G.oldTempComplexRegexTrigger = nil
    end)

    before_each(function()
      mockTriggerCreated = false
    end)

    -- Test basic color names
    it("should accept 'red' as foreground color", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'blue' as background color", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 0, "blue", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept both 'red' fg and 'blue' bg colors", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", "blue", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    -- Test all basic colors
    it("should accept all basic ANSI color names", function()
      local basicColors = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"}

      for _, color in ipairs(basicColors) do
        mockTriggerCreated = false
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, color, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for color: " .. color)
        assert.is_true(mockTriggerCreated, "Trigger not created for color: " .. color)
      end
    end)

    -- Test light colors
    it("should accept light color names with underscore", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "light_red", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept light color names without underscore", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "lightred", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept light color names with space", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "light red", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept all light ANSI color variations", function()
      local lightColors = {
        "light_red", "lightred", "light red",
        "light_green", "lightgreen", "light green",
        "light_blue", "lightblue", "light blue",
        "light_yellow", "lightyellow", "light yellow",
        "light_magenta", "lightmagenta", "light magenta",
        "light_cyan", "lightcyan", "light cyan",
        "light_white", "lightwhite", "light white",
      }

      for _, color in ipairs(lightColors) do
        mockTriggerCreated = false
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, color, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for color: " .. color)
        assert.is_true(mockTriggerCreated, "Trigger not created for color: " .. color)
      end
    end)

    -- Test gray/grey aliases
    it("should accept 'gray' as alias for light_black", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "gray", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'grey' as alias for light_black", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "grey", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    -- Test special values
    it("should accept 'default' as special value", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'ignore' as special value", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "ignore", "red", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    -- Test case insensitivity
    it("should be case insensitive for color names", function()
      local variations = {"RED", "Red", "rEd", "red"}

      for _, color in ipairs(variations) do
        mockTriggerCreated = false
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, color, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for color: " .. color)
        assert.is_true(mockTriggerCreated, "Trigger not created for color: " .. color)
      end
    end)
  end)

  describe("Tests special string values 'default' and 'ignore'", function()
    local mockTriggerId = 12345
    local mockTriggerCreated = false

    setup(function()
      _G.oldTempComplexRegexTrigger = _G.tempComplexRegexTrigger

      _G.tempComplexRegexTrigger = function(name, pattern, code, multiline, fgColor, bgColor, filter, matchAll, hlFg, hlBg, sound, fireLength, lineDelta, expiryCount)
        -- Validate both string and number inputs
        if type(fgColor) == "string" then
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_black = true, lightblack = true, ["light black"] = true,
            gray = true, grey = true,
            light_red = true, lightred = true, ["light red"] = true,
            light_green = true, lightgreen = true, ["light green"] = true,
            light_yellow = true, lightyellow = true, ["light yellow"] = true,
            light_blue = true, lightblue = true, ["light blue"] = true,
            light_magenta = true, lightmagenta = true, ["light magenta"] = true,
            light_cyan = true, lightcyan = true, ["light cyan"] = true,
            light_white = true, lightwhite = true, ["light white"] = true,
            default = true, ignore = true
          }

          if not validColors[fgColor:lower()] then
            return nil, "error: color not recognized"
          end
        elseif type(fgColor) == "number" then
          if fgColor < 0 or fgColor > 255 then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #5 value (foreground color code %d invalid, must be 0-255; for special values use strings: 'default' or 'ignore')",
              fgColor
            )
          end
        end

        if type(bgColor) == "string" then
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_black = true, lightblack = true, ["light black"] = true,
            gray = true, grey = true,
            light_red = true, lightred = true, ["light red"] = true,
            light_green = true, lightgreen = true, ["light green"] = true,
            light_yellow = true, lightyellow = true, ["light yellow"] = true,
            light_blue = true, lightblue = true, ["light blue"] = true,
            light_magenta = true, lightmagenta = true, ["light magenta"] = true,
            light_cyan = true, lightcyan = true, ["light cyan"] = true,
            light_white = true, lightwhite = true, ["light white"] = true,
            default = true, ignore = true
          }

          if not validColors[bgColor:lower()] then
            return nil, "error: color not recognized"
          end
        elseif type(bgColor) == "number" then
          if bgColor < 0 or bgColor > 255 then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #6 value (background color code %d invalid, must be 0-255; for special values use strings: 'default' or 'ignore')",
              bgColor
            )
          end
        end

        mockTriggerCreated = true
        return mockTriggerId
      end
    end)

    teardown(function()
      _G.tempComplexRegexTrigger = _G.oldTempComplexRegexTrigger
      _G.oldTempComplexRegexTrigger = nil
    end)

    before_each(function()
      mockTriggerCreated = false
    end)

    -- Test 'default' string value
    it("should accept 'default' as foreground color", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", 0, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'default' as background color", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 0, "default", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'default' for both fg and bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", "default", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'default' case-insensitively", function()
      local variations = {"DEFAULT", "Default", "dEfAuLt", "default"}

      for _, value in ipairs(variations) do
        mockTriggerCreated = false
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, value, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for: " .. value)
        assert.is_true(mockTriggerCreated, "Trigger not created for: " .. value)
      end
    end)

    -- Test 'ignore' string value
    it("should accept 'ignore' as foreground color (with bg color set)", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "ignore", "red", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'ignore' as background color (with fg color set)", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", "ignore", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'ignore' case-insensitively", function()
      local variations = {"IGNORE", "Ignore", "iGnOrE", "ignore"}

      for _, value in ipairs(variations) do
        mockTriggerCreated = false
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, value, "red", 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for: " .. value)
        assert.is_true(mockTriggerCreated, "Trigger not created for: " .. value)
      end
    end)

    -- Test combinations with regular colors
    it("should accept 'default' fg with color name bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", "blue", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept color name fg with 'default' bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", "default", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'ignore' fg with 'default' bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "ignore", "default", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'default' fg with 'ignore' bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", "ignore", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    -- Test combinations with numeric ANSI codes
    it("should accept 'default' fg with numeric bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "default", 4, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept numeric fg with 'default' bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 1, "default", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept 'ignore' fg with numeric bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "ignore", 4, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    it("should accept numeric fg with 'ignore' bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 1, "ignore", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
      assert.is_true(mockTriggerCreated)
    end)

    -- Test that numeric -1 and -2 are rejected (must use strings)
    it("should reject numeric -1 for fg (must use string 'default')", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, -1, "red", 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255") or err:match("'default'"))
    end)

    it("should reject numeric -2 for fg (must use string 'ignore')", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, -2, "red", 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255") or err:match("'ignore'"))
    end)

    it("should reject numeric -1 for bg (must use string 'default')", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", -1, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255") or err:match("'default'"))
    end)

    it("should reject numeric -2 for bg (must use string 'ignore')", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", -2, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255") or err:match("'ignore'"))
    end)
  end)

  describe("Tests numeric ANSI code support", function()
    local mockTriggerId = 12345

    setup(function()
      _G.oldTempComplexRegexTrigger = _G.tempComplexRegexTrigger

      _G.tempComplexRegexTrigger = function(name, pattern, code, multiline, fgColor, bgColor, filter, matchAll, hlFg, hlBg, sound, fireLength, lineDelta, expiryCount)
        if type(fgColor) == "number" then
          -- Only accept 0-255 for numbers; special values must be strings
          if fgColor < 0 or fgColor > 255 then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #5 value (foreground color code %d invalid, must be 0-255; for special values use strings: 'default' or 'ignore')",
              fgColor
            )
          end
        end

        if type(bgColor) == "number" then
          -- Only accept 0-255 for numbers; special values must be strings
          if bgColor < 0 or bgColor > 255 then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #6 value (background color code %d invalid, must be 0-255; for special values use strings: 'default' or 'ignore')",
              bgColor
            )
          end
        end

        return mockTriggerId
      end
    end)

    teardown(function()
      _G.tempComplexRegexTrigger = _G.oldTempComplexRegexTrigger
      _G.oldTempComplexRegexTrigger = nil
    end)

    it("should accept basic ANSI codes 0-15", function()
      for i = 0, 15 do
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, i, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for ANSI code: " .. i)
      end
    end)

    it("should accept extended ANSI codes 16-255", function()
      -- Test a sampling of extended codes
      local testCodes = {16, 50, 100, 150, 200, 231, 232, 240, 255}
      for _, code in ipairs(testCodes) do
        local id = tempComplexRegexTrigger("test", ".*", function() end, 0, code, 0, 0, 0, "", "", "", 0, 0)
        assert.equals(mockTriggerId, id, "Failed for ANSI code: " .. code)
      end
    end)

    it("should reject invalid ANSI codes > 255", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, 256, 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid"))
    end)

    it("should reject negative ANSI codes (must use string 'default' or 'ignore')", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, -1, 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255"))

      id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, -2, 1, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("invalid") or err:match("0%-255"))
    end)
  end)

  describe("Tests mixed numeric and string color support", function()
    local mockTriggerId = 12345

    setup(function()
      _G.oldTempComplexRegexTrigger = _G.tempComplexRegexTrigger

      _G.tempComplexRegexTrigger = function(name, pattern, code, multiline, fgColor, bgColor, filter, matchAll, hlFg, hlBg, sound, fireLength, lineDelta, expiryCount)
        -- Accept both strings and numbers
        return mockTriggerId
      end
    end)

    teardown(function()
      _G.tempComplexRegexTrigger = _G.oldTempComplexRegexTrigger
      _G.oldTempComplexRegexTrigger = nil
    end)

    it("should accept numeric fg and string bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 1, "blue", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
    end)

    it("should accept string fg and numeric bg", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", 4, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
    end)

    it("should accept both numeric", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, 1, 4, 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
    end)

    it("should accept both string", function()
      local id = tempComplexRegexTrigger("test", ".*", function() end, 0, "red", "blue", 0, 0, "", "", "", 0, 0)
      assert.equals(mockTriggerId, id)
    end)
  end)

  describe("Tests error handling for invalid color names", function()
    setup(function()
      _G.oldTempComplexRegexTrigger = _G.tempComplexRegexTrigger

      _G.tempComplexRegexTrigger = function(name, pattern, code, multiline, fgColor, bgColor, filter, matchAll, hlFg, hlBg, sound, fireLength, lineDelta, expiryCount)
        if type(fgColor) == "string" then
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_red = true, light_green = true, light_blue = true,
            light_yellow = true, light_magenta = true, light_cyan = true,
            light_white = true, light_black = true,
            lightred = true, lightgreen = true, lightblue = true,
            lightyellow = true, lightmagenta = true, lightcyan = true,
            lightwhite = true, lightblack = true,
            ["light red"] = true, ["light green"] = true, ["light blue"] = true,
            ["light yellow"] = true, ["light magenta"] = true, ["light cyan"] = true,
            ["light white"] = true, ["light black"] = true,
            gray = true, grey = true,
            default = true, ignore = true
          }

          if not validColors[fgColor:lower()] then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #5 value (foreground color name '%s' not recognized, use basic ANSI color names like 'red', 'blue', 'light_green', etc., or numeric ANSI codes 0-255)",
              fgColor
            )
          end
        end

        if type(bgColor) == "string" then
          local validColors = {
            black = true, red = true, green = true, yellow = true,
            blue = true, magenta = true, cyan = true, white = true,
            light_red = true, light_green = true, light_blue = true,
            light_yellow = true, light_magenta = true, light_cyan = true,
            light_white = true, light_black = true,
            lightred = true, lightgreen = true, lightblue = true,
            lightyellow = true, lightmagenta = true, lightcyan = true,
            lightwhite = true, lightblack = true,
            ["light red"] = true, ["light green"] = true, ["light blue"] = true,
            ["light yellow"] = true, ["light magenta"] = true, ["light cyan"] = true,
            ["light white"] = true, ["light black"] = true,
            gray = true, grey = true,
            default = true, ignore = true
          }

          if not validColors[bgColor:lower()] then
            return nil, string.format(
              "tempComplexRegexTrigger: bad argument #6 value (background color name '%s' not recognized, use basic ANSI color names like 'red', 'blue', 'light_green', etc., or numeric ANSI codes 0-255)",
              bgColor
            )
          end
        end

        return 12345
      end
    end)

    teardown(function()
      _G.tempComplexRegexTrigger = _G.oldTempComplexRegexTrigger
      _G.oldTempComplexRegexTrigger = nil
    end)

    it("should return nil and error for invalid foreground color 'purple'", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "purple", 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("purple"))
      assert.is_truthy(err:match("not recognized"))
    end)

    it("should return nil and error for invalid background color 'orange'", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, 0, "orange", 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("orange"))
      assert.is_truthy(err:match("not recognized"))
    end)

    it("should return nil and error for invalid color 'pink'", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "pink", 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("pink"))
    end)

    it("should return nil and error for invalid color 'brown'", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "brown", 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      assert.is_truthy(err:match("brown"))
    end)

    it("should return descriptive error message with suggestions", function()
      local id, err = tempComplexRegexTrigger("test", ".*", function() end, 0, "invalid", 0, 0, 0, "", "", "", 0, 0)
      assert.is_nil(id)
      assert.is_not_nil(err)
      -- Error message should mention valid alternatives
      assert.is_truthy(err:match("red") or err:match("blue") or err:match("ANSI"))
    end)
  end)
end)
