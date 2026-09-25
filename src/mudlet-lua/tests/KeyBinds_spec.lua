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

  describe("permGroup key groups", function()

    -- Lua cannot delete a permanent item, and what these specs pin is the state
    -- a group is *created* in, so reusing one an earlier run left behind would
    -- quietly test that run's item instead of a new one. Hand out the first
    -- names no run has taken yet rather than random ones: math.random is
    -- unseeded in Lua 5.1, so every process deals the same sequence, and two
    -- runs sharing a profile within the same second would agree on their
    -- "unique" name and stack a second group under it.
    --
    -- Every run of a shared profile takes one more set of names, so the search
    -- has to go as far as it needs to: a run that gave up and skipped its
    -- checks would leave these specs green while testing nothing. The bound is
    -- only there so a broken exists() cannot spin forever, and reaching it
    -- raises rather than skips.
    local searchLimit = 100000

    local function freshNames(stem, ...)
      local suffixes = {...}
      for index = 1, searchLimit do
        local names, free = {}, true
        for _, suffix in ipairs(suffixes) do
          local name = ("%s%s%d"):format(stem, suffix, index)
          if exists(name, "keybind") ~= 0 then
            free = false
            break
          end
          names[#names + 1] = name
        end
        if free then
          return unpack(names)
        end
      end
      error(("no free \"%s\" name in this profile after %d tries"):format(stem, searchLimit))
    end

    it("creates the group active", function()
      local groupName = freshNames("SpecPermKeyGroup", "")
      -- switching it off again is the only cleanup Lua has for a permanent item
      finally(function() disableKey(groupName) end)

      assert.is_true(permGroup(groupName, "key"), "could not create the key group")
      assert.are.equal(1, exists(groupName, "keybind"), "exactly one group should have been created")
      assert.are.equal(1, isActive(groupName, "keybind"),
        "a freshly created key group should be active, like a trigger or alias one")
    end)

    -- #10764: permGroup(name, "key") calls permKey(name, parent, -1, ""), and the
    -- -1 that marks the item as a folder also marked it inactive, so every key
    -- put in the group reported itself active but could never fire
    it("creates a group whose keys are not held back by it", function()
      local groupName, childName = freshNames("SpecPermKeyGroupParent", "", "Key")
      finally(function()
        disableKey(childName)
        disableKey(groupName)
      end)

      assert.is_true(permGroup(groupName, "key"), "could not create the key group")
      local childId = permKey(childName, groupName, mudlet.keymodifier.None, mudlet.key.F9, [[echo("x")]])
      assert.is_true(childId > 0, "could not create the key inside the group")
      assert.are.equal(1, isActive(childId, "keybind"), "the key itself should be active")
      assert.is_true(isAncestorsActive(childId, "keybind"),
        "the group above a fresh key must not be the thing stopping it from firing")

      -- the control the assertion above needs: were a key group active whatever
      -- was done to it, both of these specs would pass over a fix that only
      -- looked right, so a group that is switched off has to be seen holding
      -- its keys back
      assert.is_true(disableKey(groupName))
      assert.is_false(isAncestorsActive(childId, "keybind"),
        "a disabled group should hold back the key inside it")
      assert.are.equal(1, isActive(childId, "keybind"),
        "disabling the group should not have touched the key's own state - it is the group that stops it")
    end)

    it("creates a group inside a group active at every level", function()
      local outerName, innerName, childName = freshNames("SpecPermKeyNested", "Outer", "Inner", "Key")
      finally(function()
        disableKey(childName)
        disableKey(innerName)
        disableKey(outerName)
      end)

      assert.is_true(permGroup(outerName, "key"), "could not create the outer key group")
      assert.is_true(permGroup(innerName, "key", outerName), "could not create a key group inside another")
      local childId = permKey(childName, innerName, mudlet.keymodifier.None, mudlet.key.F10, [[echo("x")]])
      assert.is_true(childId > 0, "could not create the key inside the nested group")

      assert.are.equal(1, isActive(innerName, "keybind"), "a group nested in another should be created active too")
      assert.is_true(isAncestorsActive(childId, "keybind"),
        "neither group above a fresh key should be holding it back")

      -- isAncestorsActive() walks every ancestor rather than just the parent, so
      -- the group that is not the key's own parent has to be able to stop it too
      assert.is_true(disableKey(outerName))
      assert.is_false(isAncestorsActive(childId, "keybind"),
        "the outer group should hold back a key two levels below it")
      assert.are.equal(1, isActive(innerName, "keybind"),
        "disabling the outer group should not have touched the group inside it")
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

    it("freeing a temporary key leaves a same-named permanent one reachable", function()
      -- tempKey names its key after its id, so a permanent key called after that
      -- number shares the name - and the name lookup table holds several keys per
      -- name
      local tempId = tempKey(mudlet.key.F11, [[echo("x")]])
      local sharedName = tostring(tempId)
      -- permanent keys cannot be deleted from Lua, so earlier local runs can leave
      -- same-named ones behind: work from a relative baseline
      local before = exists(sharedName, "keybind")
      assert.is_true(permKey(sharedName, "", mudlet.key.F12, [[echo("x")]]) > 0)
      finally(function() disableKey(sharedName) end)
      assert.are.equal(before + 1, exists(sharedName, "keybind"))

      assert.is_true(killKey(tempId), "the temporary key is the one that can be killed")
      -- an incoming line runs every unit's deferred cleanup, which frees it
      feedTriggers("\nspec_key_eviction_flush\n")

      assert.are.equal(before, exists(sharedName, "keybind"), "only the temporary key should leave the lookup table")
      assert.is_true(enableKey(sharedName), "the permanent key must still be reachable by name")
    end)

    it("killKey finds a temporary key behind a same-named permanent one", function()
      -- killKey walks the root node list in creation order, so a permanent key
      -- restored from the profile sits in front of this session's temporaries: it
      -- must be scanned past, not reported as a failure
      local seed = tempKey(mudlet.key.F9, [[echo("x")]])
      killKey(seed)
      -- permKey itself takes seed + 1, so the next temporary takes seed + 2
      local sharedName = tostring(seed + 2)
      assert.is_true(permKey(sharedName, "", mudlet.key.F10, [[echo("x")]]) > 0)
      finally(function() disableKey(sharedName) end)

      local tempId = tempKey(mudlet.key.F11, [[echo("x")]])
      assert.are.equal(seed + 2, tempId, "ids should still be handed out in sequence")
      assert.is_true(killKey(tempId), "killKey must scan past the permanent key")
    end)

  end)

end)

-- A binding on one of Mudlet's own keys never fires. It is warned about in the
-- editor, not on the main console, where a script making its bindings at
-- profile load would repeat the warning at every startup. AddonControlsTest
-- covers the editor warning, which Lua cannot open.
describe("a key binding on a key Mudlet itself uses", function()

  it("is still made, and says nothing on the main console", function()
    local mark = getLastLineNumber("main")

    -- Ctrl+Alt+T is Mudlet's "Toggle Time Stamps" on every platform
    local key = tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo("mine")]])
    local text = table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
    killKey(key)

    assert.is_number(key, "the binding should still be made")
    assert.is_nil(text:find("WARN", 1, true), "the clash was posted to the main console: " .. text)
    -- the console wraps long lines, so the action's name is looked for without spaces
    assert.is_nil(text:gsub("%s", ""):find("ToggleTimeStamps", 1, true), "the clash was posted to the main console: " .. text)
  end)

end)
