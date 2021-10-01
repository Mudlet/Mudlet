describe("tests the functionality of the gmod module", function()
  describe("gmod.registerUser(user)", function()
    it("Should return 'user registered' upon successfully registering a user", function()
      local expected = "user registered"
      local actual = gmod.registerUser("testUser" .. math.random(10000))
      assert.equals(expected, actual)
    end)

    it("Should return 'user exists' if you try to register the same user more than once", function()
      local user = "testUser" .. math.random(10000)
      local expected = "user exists"
      gmod.registerUser(user)
      local actual = gmod.registerUser(user)
      assert.equals(expected, actual)
    end)
  end)

  describe("gmod.enableModule(user, module)", function()
    local module = "OogaBoogaFakeModule"
    local user = "testUser"
    after_each(function()
      gmod.disableModule(user, module)
    end)

    it("should register the user", function()
      local ru = spy.on(gmod, "registerUser")
      gmod.enableModule(user, module)
      assert.spy(ru).was.called(1)
      assert.spy(ru).was.called_with(user)
      gmod.registerUser:revert()
    end)

    it("should add the module to the registered modules", function()
      assert.is_false(gmod.isRegisteredModule(module))
      gmod.enableModule(user, module)
      assert.is_truthy(gmod.isRegisteredModule(module))
    end)

    it("should send Core.Supports.Add over gmcp for a new module", function()
      local sg = spy.on(_G, "sendGMCP")
      gmod.enableModule(user, module)
      assert.spy(sg).was_called_with(match.has_match("Core.Supports.Add .*" .. module))
      sendGMCP:revert()
    end)

    it("should send Core.Supports.Add over gmcp only once if a module is registered twice", function()
      local sg = spy.on(_G, "sendGMCP")
      gmod.enableModule(user, module)
      gmod.enableModule(user, module)
      assert.spy(sg).was_called(1)
      sendGMCP:revert()
    end)
  end)

  describe("gmod.disableModule(user, module)", function()
    local module = "OogaBoogaFakeModule"
    local user = "testUser"
    local sg
    before_each(function()
      gmod.enableModule(user, module)
      sg = spy.on(_G, "sendGMCP")
    end)

    after_each(function()
      gmod.disableModule(user, module)
      sendGMCP:revert()
    end)

    it([[should send "Core.Supports.Remove" if it's the last user to disable the module]], function()
      gmod.disableModule(user, module)
      assert.spy(sg).was_called_with(match.has_match("Core.Supports.Remove .*" .. module))
    end)

    it([[should not send "Core.Supports.Remove" if more than any other user is using it]], function()
      local user2 = user .. "1"
      gmod.enableModule(user2, module)
      gmod.disableModule(user, module)
      assert.spy(sg).was_not_called()
      gmod.disableModule(user2, module) -- clean up after ourselves
    end)

    it([[should only send "Core.Supports.Remove" once if the module is disabled multiple times]], function()
      gmod.disableModule(user, module)
      gmod.disableModule(user, module)
      assert.spy(sg).was_called(1)
    end)
  end)

  describe("gmod.isRegisteredModule(module)", function()
    it("should return false if a module is not registered", function()
      local actual = gmod.isRegisteredModule("ThisModuleIsDefinitelyNotRegistered1281728923489348912")
      assert.is_false(actual)
    end)

    it("should return a table with enabled users as the keys", function()
      local user = "testUser"
      local mod = "OogaBoogaFakeModule"
      gmod.enableModule(user, mod)
      local expected = {testUser = true}
      local actual = gmod.isRegisteredModule(mod)
      assert.is_equal("table", type(actual))
      assert.is.same(expected, actual)
      assert.is_true(actual.testUser)
      -- clean up after the test
      gmod.disableModule(user, mod)
    end)
  end)

  describe("gmod.printModules", function()
  end)
end)