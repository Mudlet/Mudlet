describe("Tests keybind-related functions", function()

  describe("Tests the functionality of getKeyCode", function()

    -- Note: These tests require actual keybinds to exist in the test environment
    -- The following tests demonstrate the expected behavior

    setup(function()
      -- Create test keybinds for testing
      -- testKeyID will hold the ID of a test keybind
      _G.testKeyName = "TestKeyBind_" .. os.time()
      _G.testKeyID = nil

      -- Attempt to create a test keybind
      -- Using permKey to create a permanent keybind for testing
      -- permKey(name, parent, modifier, key code, lua code)
      if permKey then
        _G.testKeyID = permKey(_G.testKeyName, "", mudlet.keymodifier.Control, mudlet.key.F1, [[echo("Test key pressed")]])
      end
    end)

    teardown(function()
      -- Clean up test keybind
      if _G.testKeyID and killKey then
        killKey(_G.testKeyID)
      end
      _G.testKeyName = nil
      _G.testKeyID = nil
    end)

    it("should return key code and modifiers for a valid keybind by name", function()
      -- Skip if we couldn't create a test keybind
      if not _G.testKeyID then
        pending("permKey function not available in test environment")
        return
      end

      local keyCode, modifiers = getKeyCode(_G.testKeyName)

      -- Verify we got two return values
      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.is_not_nil(modifiers, "Expected modifiers to be returned")

      -- Verify the values match what we set
      assert.equals(mudlet.key.F1, keyCode, "Expected key code to match F1")
      assert.equals(mudlet.keymodifier.Control, modifiers, "Expected modifiers to include Control")
    end)

    it("should return key code and modifiers for a valid keybind by ID", function()
      -- Skip if we couldn't create a test keybind
      if not _G.testKeyID then
        pending("permKey function not available in test environment")
        return
      end

      local keyCode, modifiers = getKeyCode(_G.testKeyID)

      -- Verify we got two return values
      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.is_not_nil(modifiers, "Expected modifiers to be returned")

      -- Verify the values match what we set
      assert.equals(mudlet.key.F1, keyCode, "Expected key code to match F1")
      assert.equals(mudlet.keymodifier.Control, modifiers, "Expected modifiers to include Control")
    end)

    it("should return nil and error message for non-existent keybind name", function()
      local nonExistentName = "NonExistentKeyBind_" .. os.time()
      local keyCode, modifiers = getKeyCode(nonExistentName)

      -- Function should return nil on error
      assert.is_nil(keyCode, "Expected nil for non-existent keybind")
    end)

    it("should return nil and error message for non-existent keybind ID", function()
      local nonExistentID = 999999
      local keyCode, modifiers = getKeyCode(nonExistentID)

      -- Function should return nil on error
      assert.is_nil(keyCode, "Expected nil for non-existent keybind ID")
    end)

    it("should return nil and error message for invalid ID (negative number)", function()
      local invalidID = -1
      local keyCode, modifiers = getKeyCode(invalidID)

      -- Function should return nil on error
      assert.is_nil(keyCode, "Expected nil for invalid negative ID")
    end)

    it("should accept both string and number types for the first argument", function()
      -- Skip if we couldn't create a test keybind
      if not _G.testKeyID then
        pending("permKey function not available in test environment")
        return
      end

      -- Test with string (name)
      local keyCode1, modifiers1 = getKeyCode(_G.testKeyName)
      assert.is_not_nil(keyCode1, "Expected result when passing string name")

      -- Test with number (ID)
      local keyCode2, modifiers2 = getKeyCode(_G.testKeyID)
      assert.is_not_nil(keyCode2, "Expected result when passing numeric ID")

      -- Both should return the same values
      assert.equals(keyCode1, keyCode2, "Key codes should match when querying same keybind")
      assert.equals(modifiers1, modifiers2, "Modifiers should match when querying same keybind")
    end)

    it("should work with keybinds that have no modifiers", function()
      -- Create a keybind with no modifiers (NoModifier)
      local testKeyName2 = "TestKeyBind_NoMod_" .. os.time()
      local testKeyID2 = nil

      if permKey then
        testKeyID2 = permKey(testKeyName2, "", mudlet.keymodifier.None, mudlet.key.F2, [[echo("Test")]])
      else
        pending("permKey function not available in test environment")
        return
      end

      local keyCode, modifiers = getKeyCode(testKeyName2)

      -- Clean up
      if testKeyID2 and killKey then
        killKey(testKeyID2)
      end

      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.equals(mudlet.key.F2, keyCode, "Expected key code to match F2")
      assert.equals(mudlet.keymodifier.None, modifiers, "Expected no modifiers")
    end)

    it("should work with keybinds that have multiple modifiers", function()
      -- Create a keybind with multiple modifiers (Ctrl + Shift)
      local testKeyName3 = "TestKeyBind_MultiMod_" .. os.time()
      local testKeyID3 = nil

      if permKey then
        -- Combine modifiers using bitwise OR
        local multiMod = mudlet.keymodifier.Control + mudlet.keymodifier.Shift
        testKeyID3 = permKey(testKeyName3, "", multiMod, mudlet.key.F3, [[echo("Test")]])
      else
        pending("permKey function not available in test environment")
        return
      end

      local keyCode, modifiers = getKeyCode(testKeyName3)

      -- Clean up
      if testKeyID3 and killKey then
        killKey(testKeyID3)
      end

      assert.is_not_nil(keyCode, "Expected keyCode to be returned")
      assert.equals(mudlet.key.F3, keyCode, "Expected key code to match F3")

      -- Check that the modifiers contain both Control and Shift
      -- In Qt, modifiers are flags that can be combined
      local expectedMod = mudlet.keymodifier.Control + mudlet.keymodifier.Shift
      assert.equals(expectedMod, modifiers, "Expected Control + Shift modifiers")
    end)

  end)

end)
