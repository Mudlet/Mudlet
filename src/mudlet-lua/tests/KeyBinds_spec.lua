describe("Tests keybind-related functions", function()

  describe("Tests the functionality of getKeyCode", function()

    setup(function()
      -- tempKey creates temporary keybinds that killKey can actually remove
      -- tempKey(modifier, key code, lua code)
      _G.testKeyID = tempKey(mudlet.keymodifier.Control, mudlet.key.F1, [[echo("Test key pressed")]])
    end)

    teardown(function()
      if _G.testKeyID then
        killKey(_G.testKeyID)
      end
      _G.testKeyID = nil
    end)

    it("should return key code and modifiers for a valid keybind by ID", function()
      local keyCode, modifiers = getKeyCode(_G.testKeyID)

      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.is_not_nil(modifiers, "Expected modifiers to be returned")
      assert.equals(mudlet.key.F1, keyCode, "Expected key code to match F1")
      assert.equals(mudlet.keymodifier.Control, modifiers, "Expected modifiers to include Control")
    end)

    it("should return nil and error message for non-existent keybind name", function()
      local nonExistentName = "NonExistentKeyBind_" .. os.time()
      local keyCode, errorMsg = getKeyCode(nonExistentName)

      assert.is_nil(keyCode, "Expected nil for non-existent keybind")
      assert.is_string(errorMsg, "Expected error message string")
    end)

    it("should return nil and error message for non-existent keybind ID", function()
      local nonExistentID = 999999
      local keyCode, errorMsg = getKeyCode(nonExistentID)

      assert.is_nil(keyCode, "Expected nil for non-existent keybind ID")
      assert.is_string(errorMsg, "Expected error message string")
    end)

    it("should return nil and error message for invalid ID (negative number)", function()
      local invalidID = -1
      local keyCode, errorMsg = getKeyCode(invalidID)

      assert.is_nil(keyCode, "Expected nil for invalid negative ID")
      assert.is_string(errorMsg, "Expected error message string")
    end)

    it("should raise an error if called with no arguments", function()
      assert.has_error(function()
        getKeyCode()
      end)
    end)

    it("should raise an error if called with a boolean argument", function()
      assert.has_error(function()
        getKeyCode(true)
      end)
    end)

    it("should raise an error if called with a table argument", function()
      assert.has_error(function()
        getKeyCode({})
      end)
    end)

    it("should raise an error if called with a nil argument", function()
      assert.has_error(function()
        getKeyCode(nil)
      end)
    end)

    it("should work with keybinds that have no modifiers", function()
      local testKeyID2 = tempKey(mudlet.keymodifier.None, mudlet.key.F2, [[echo("Test")]])

      local keyCode, modifiers = getKeyCode(testKeyID2)

      killKey(testKeyID2)

      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.equals(mudlet.key.F2, keyCode, "Expected key code to match F2")
      assert.equals(mudlet.keymodifier.None, modifiers, "Expected no modifiers")
    end)

    it("should work with keybinds that have multiple modifiers", function()
      local multiMod = mudlet.keymodifier.Control + mudlet.keymodifier.Shift
      local testKeyID3 = tempKey(multiMod, mudlet.key.F3, [[echo("Test")]])

      local keyCode, modifiers = getKeyCode(testKeyID3)

      killKey(testKeyID3)

      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.equals(mudlet.key.F3, keyCode, "Expected key code to match F3")
      local expectedMod = mudlet.keymodifier.Control + mudlet.keymodifier.Shift
      assert.equals(expectedMod, modifiers, "Expected Control + Shift modifiers")
    end)

  end)

end)
