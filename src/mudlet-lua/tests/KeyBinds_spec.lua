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

  describe("tempKey creation and cleanup", function()

    it("creates a key from a modifier, key code and string body", function()
      local id = tempKey(mudlet.keymodifier.Alt, mudlet.key.F5, [[echo("hi")]])
      assert.is_number(id)
      assert.are.equal(1, exists(id, "keybind"), "the temp key should exist after creation")
      local keyCode, modifiers = getKeyCode(id)
      assert.are.equal(mudlet.key.F5, keyCode)
      assert.are.equal(mudlet.keymodifier.Alt, modifiers)
      killKey(id)
    end)

    it("creates a key from the two-argument (key code, body) form", function()
      local id = tempKey(mudlet.key.F6, [[echo("hi")]])
      assert.is_number(id)
      local keyCode, modifiers = getKeyCode(id)
      assert.are.equal(mudlet.key.F6, keyCode)
      assert.are.equal(mudlet.keymodifier.None, modifiers, "the two-arg form should have no modifier")
      killKey(id)
    end)

    it("creates a key from a function body", function()
      -- a key cannot be pressed headlessly, so this only asserts creation
      local id = tempKey(mudlet.key.F7, function() end)
      assert.is_number(id)
      assert.are.equal(1, exists(id, "keybind"))
      killKey(id)
    end)

    it("rejects a non-string, non-function body", function()
      -- a table is not string-coercible (a number would be accepted as code)
      assert.has_error(function() tempKey(mudlet.key.F8, {}) end)
    end)

  end)

  describe("permKey creation and validation", function()

    -- permanent keys cannot be removed with killKey, so use unique names and
    -- disable them after each test so they do not leak into other specs
    after_each(function()
      disableKey("SpecPermKeyOne")
      disableKey("SpecPermKeyTwo")
    end)

    it("creates a permanent key without a modifier (four-argument form)", function()
      local id = permKey("SpecPermKeyOne", "", mudlet.key.F9, [[echo("perm")]])
      assert.is_number(id)
      assert.is_true(id > 0)
      local keyCode, modifiers = getKeyCode(id)
      assert.are.equal(mudlet.key.F9, keyCode)
      assert.are.equal(mudlet.keymodifier.None, modifiers)
    end)

    it("creates a permanent key with a modifier (five-argument form)", function()
      local id = permKey("SpecPermKeyTwo", "", mudlet.keymodifier.Control, mudlet.key.F10, [[echo("perm")]])
      assert.is_number(id)
      assert.is_true(id > 0)
      local keyCode, modifiers = getKeyCode(id)
      assert.are.equal(mudlet.key.F10, keyCode)
      assert.are.equal(mudlet.keymodifier.Control, modifiers)
    end)

    it("errors when the lua code argument is not a string", function()
      assert.has_error(function()
        permKey("SpecPermKeyOne", "", mudlet.key.F9, 42)
      end)
    end)

  end)

  describe("enable, disable, kill, isActive and exists for keys", function()

    after_each(function()
      disableKey("SpecPermKeyKill")
      disableKey("SpecPermKeyToggle")
      disableKey("SpecDupKeys")
    end)

    it("killKey returns true for a temp key and false for a missing name", function()
      local id = tempKey(mudlet.key.F11, [[echo("x")]])
      assert.are.equal(1, exists(id, "keybind"))
      assert.is_true(killKey(id), "killing an existing temp key should return true")
      assert.is_false(killKey("no_such_key_name"), "killing a missing key should return false")
    end)

    it("killKey returns false the second time, as the key is already dead", function()
      local id = tempKey(mudlet.key.F11, [[echo("x")]])
      assert.is_true(killKey(id), "killing a live temporary key should report success")
      -- the key is still present here: only the deferred cleanup frees it, so the
      -- second kill really is being told about a corpse it can find
      assert.are.equal(1, exists(id, "keybind"), "the killed key is still present until cleanup runs")
      assert.are.equal(0, isActive(id, "keybind"), "a killed key is no longer active")
      assert.is_false(killKey(id),
        "killing an already killed key achieves nothing and has to say so")
      -- an incoming line runs every unit's deferred cleanup, which is what finally
      -- frees the key; the answer has to be the same after it. A key press cannot be
      -- synthesised headlessly, so there is no in-callback double kill to pin here -
      -- KeyUnit's depth and cleanup machinery matches AliasUnit's, whose spec has one
      feedTriggers("\nspec_key_kill_flush\n")
      assert.are.equal(0, exists(id, "keybind"), "the key should be gone after kill and cleanup")
      assert.is_false(killKey(id), "a freed key cannot be killed either")
    end)

    it("killKey returns false for a permanent key (they cannot be killed)", function()
      local id = permKey("SpecPermKeyKill", "", mudlet.key.F12, [[echo("x")]])
      assert.is_true(id > 0)
      assert.is_false(killKey("SpecPermKeyKill"), "permanent keys cannot be removed with killKey")
    end)

    it("enableKey and disableKey report whether a matching key was found", function()
      local id = permKey("SpecPermKeyToggle", "", mudlet.key.F12, [[echo("x")]])
      assert.is_true(id > 0)
      assert.is_true(disableKey("SpecPermKeyToggle"), "disableKey should report it found the key")
      assert.is_true(enableKey("SpecPermKeyToggle"), "enableKey should report it found the key")
      assert.is_false(disableKey("no_such_key_name"), "disableKey should return false when nothing matched")
      assert.is_false(enableKey("no_such_key_name"), "enableKey should return false when nothing matched")
    end)

    it("isActive reflects a key's enabled state", function()
      local id = tempKey(mudlet.key.F11, [[echo("x")]])
      assert.are.equal(1, isActive(id, "keybind"), "a fresh key should be active")
      disableKey(id)
      assert.are.equal(0, isActive(id, "keybind"), "a disabled key should not be active")
      enableKey(id)
      assert.are.equal(1, isActive(id, "keybind"), "a re-enabled key should be active")
      killKey(id)
    end)

    it("exists reports keys by ID and reports zero for an unknown name", function()
      local id = tempKey(mudlet.key.F11, [[echo("x")]])
      assert.are.equal(1, exists(id, "keybind"))
      assert.are.equal(0, exists("no_such_key_name", "keybind"))
      killKey(id)
    end)

    -- KeyUnit carries its own copy of the equal_range duplicate-name fix
    -- (PR #9366), so enable/disable by name must toggle every same-named key.
    -- Key firing cannot be synthesised headlessly, so isActive's count is the proxy.
    it("enableKey/disableKey by name toggle every duplicate-named key", function()
      local id1 = permKey("SpecDupKeys", "", mudlet.key.F11, [[echo("x")]])
      local id2 = permKey("SpecDupKeys", "", mudlet.key.F12, [[echo("x")]])
      assert.is_true(id1 > 0 and id2 > 0)
      -- invariants rather than exact counts: permanent keys from earlier local
      -- runs accumulate under this name in the saved profile
      assert.is_true(exists("SpecDupKeys", "keybind") >= 2, "at least the two duplicate-named keys exist")
      disableKey("SpecDupKeys")
      assert.are.equal(0, isActive("SpecDupKeys", "keybind"), "disabling by name must leave zero of the duplicates active")
      enableKey("SpecDupKeys")
      assert.are.equal(exists("SpecDupKeys", "keybind"), isActive("SpecDupKeys", "keybind"), "enabling by name must reactivate every duplicate")
    end)

  end)

end)
