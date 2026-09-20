describe("tests the functionality of the gmod module", function()
  describe("Tests the functionality of gmod.registerUser", function()
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

  describe("Tests the functionality of gmod.enableModule", function()
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

  describe("Tests the functionality of gmod.disableModule", function()
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

  describe("Tests the functionality of gmod.isRegisteredModule", function()
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

  describe("Tests the functionality of gmod.printModules", function()
    local user = "testUser"
    local module = "OogaBoogaFakeModule"
    local gp
    local ce
    before_each(function()
      gp = spy.on(gmod, "print")
      ce = stub(_G, "cecho")
    end)
    after_each(function()
      gmod.print:revert()
      cecho:revert()
    end)

    it("Should include an enabled module in its output", function()
      gmod.enableModule(user, module)
      gmod.printModules()
      assert.spy(gp).was_called()
      assert.stub(ce).was_called()
      assert.stub(ce).was_called_with(match.has_match(module))
      gmod.disableModule(user, module) -- cleanup
    end)

    it("Should not include a module in its output if it is disabled", function()
      gmod.enableModule(user, module)
      gmod.disableModule(user, module)
      gmod.printModules()
      assert.stub(ce).was_not_called_with(match.has_match(module))
    end)

    it("Should only print a specific user's enabled modules when called with a user", function()
      local user2 = user .. "2"
      local module2 = module .. "2"
      gmod.enableModule(user, module)
      gmod.enableModule(user2, module2)
      gmod.printModules(user)
      assert.stub(ce).was_not_called_with(match.has_match(module2))
      assert.stub(ce).was_not_called_with(match.has_match(user2))
      assert.stub(ce).was_called_with(match.has_match(user))
      assert.stub(ce).was_called_with(match.has_match(module))
      gmod.disableModule(user, module)
      gmod.disableModule(user2, module2)
    end)
  end)
end)

describe("Tests the argument and disconnected contract of sendGMCP", function()
  -- Contract-only checks: sendGMCP is never mocked here and never reaches a
  -- live game server. The self-test profile is forced into a disconnected
  -- state so the connection guard is exercised deterministically; verifying
  -- the actual bytes on the wire is a separate, stub-based effort.
  local function contains(haystack, needle)
    return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
  end

  before_each(function()
    disconnect()
  end)

  it("names the offending value's real type when the message is not a string", function()
    -- Regression #9543: the type-name placeholder must be expanded, not printed
    -- as a literal "%1". lua_pushfstring only understands C-style "%s".
    local ok, err = pcall(function() sendGMCP({}) end)
    assert.is_false(ok)
    assert.is_true(contains(err, "sendGMCP: bad argument #1 type (message as string expected, got table!)"), tostring(err))
    assert.is_false(contains(err, "%1"), tostring(err))

    local okBool, errBool = pcall(function() sendGMCP(true) end)
    assert.is_false(okBool)
    assert.is_true(contains(errBool, "sendGMCP: bad argument #1 type (message as string expected, got boolean!)"), tostring(errBool))
  end)

  it("names the real type when the optional second argument is not a string", function()
    local ok, err = pcall(function() sendGMCP("Core.Ping", {}) end)
    assert.is_false(ok)
    assert.is_true(contains(err, "sendGMCP: bad argument #2 type (what as string is optional, got table!)"), tostring(err))
    assert.is_false(contains(err, "%1"), tostring(err))
  end)

  it("returns nil and an explanatory message while disconnected", function()
    local ok, err = sendGMCP("External.Discord.Hello")
    assert.is_nil(ok)
    assert.is_true(contains(err, "not connected to game server"))
  end)
end)

describe("Tests the functionality of gmod.print", function()
  it("Should write the tracker prefixed message to the main console", function()
    clearWindow()
    gmod.print("a tracker message")
    local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
    assert.is_truthy(text:find("[GMCP Tracker]", 1, true))
    assert.is_truthy(text:find("a tracker message", 1, true))
  end)

  it("Should colour the prefix yellow and the message white", function()
    clearWindow()
    gmod.print("coloured message")
    -- the message is wrapped in newlines, so park the cursor on its line
    -- before selecting: selectString only searches the current line
    local lines = getLines("main", 0, getLastLineNumber("main") + 1)
    local index
    for i, line in ipairs(lines) do
      if line:find("[GMCP Tracker]", 1, true) then
        index = i - 1
      end
    end
    assert.is_not_nil(index, "gmod.print should have written a tracker line")
    moveCursor(0, index)
    selectString("[GMCP Tracker]", 1)
    assert.are.same(color_table["yellow"], getTextFormat().foreground)
    selectString("coloured message", 1)
    assert.are.same(color_table["white"], getTextFormat().foreground)
  end)
end)

describe("Tests the functionality of gmod.reenableModules", function()
  local user = "reenableUser"
  local module = "OogaBoogaReenableModule"

  after_each(function()
    gmod.disableModule(user, module)
    gmcp.BustedReenableProbe = nil
  end)

  it("Should send nothing while the gmcp table is still empty", function()
    -- reenableModules is driven by sysProtocolEnabled, which can fire before
    -- the server has sent any GMCP at all
    if next(gmcp) then
      -- the profile or an earlier spec left GMCP data behind, so the guard
      -- this test is about cannot be reached
      pending("the gmcp table is not empty in this profile")
    end
    gmod.enableModule(user, module)
    local sg = spy.on(_G, "sendGMCP")
    finally(function() sendGMCP:revert() end)
    gmod.reenableModules()
    assert.spy(sg).was_not_called()
  end)

  it("Should re-announce every registered module once GMCP data has arrived", function()
    gmod.enableModule(user, module)
    gmcp.BustedReenableProbe = {}
    local sg = spy.on(_G, "sendGMCP")
    finally(function() sendGMCP:revert() end)
    gmod.reenableModules()
    assert.spy(sg).was_called_with(match.has_match("Core.Supports.Add .*" .. module .. " 1"))
  end)

  it("Should send nothing when no module is registered", function()
    gmcp.BustedReenableProbe = {}
    local sg = spy.on(_G, "sendGMCP")
    finally(function() sendGMCP:revert() end)
    -- a module registered by an earlier spec would be re-announced too, so
    -- measure the difference this test's own module makes
    gmod.reenableModules()
    local withoutOurs = #sg.calls
    gmod.enableModule(user, module)
    sendGMCP:clear()
    gmod.reenableModules()
    assert.is_true(#sg.calls > 0, "a registered module should be re-announced")
    assert.are.equal(0, withoutOurs, "nothing should be announced while no module is registered")
  end)
end)

describe("Tests the functionality of __gmcp_merge_gmcp_sub_tables", function()
  it("Should fold the staged table into the named sub table", function()
    local a = {Char = {name = "old", level = 1}, __needMerge = {name = "new", hp = 50}}
    __gmcp_merge_gmcp_sub_tables(a, "Char")
    assert.are.same({name = "new", level = 1, hp = 50}, a.Char)
  end)

  it("Should clear the staging table afterwards", function()
    local a = {Room = {}, __needMerge = {num = 7}}
    __gmcp_merge_gmcp_sub_tables(a, "Room")
    assert.is_nil(a.__needMerge)
  end)

  it("Should leave the sub table alone when nothing is staged", function()
    local a = {Room = {num = 7}, __needMerge = {}}
    __gmcp_merge_gmcp_sub_tables(a, "Room")
    assert.are.same({num = 7}, a.Room)
    assert.is_nil(a.__needMerge)
  end)

  it("Should raise rather than silently drop data when the sub table is missing", function()
    -- the C++ side stages into __needMerge and calls this immediately, so a
    -- module arriving before its sub table exists is a real ordering case
    assert.has_error(function() __gmcp_merge_gmcp_sub_tables({__needMerge = {a = 1}}, "Char") end)
    assert.has_error(function() __gmcp_merge_gmcp_sub_tables({Char = {}}, "Char") end)
  end)

  it("Should merge nested tables by replacing them wholesale", function()
    local a = {Char = {Vitals = {hp = 1}}, __needMerge = {Vitals = {mp = 2}}}
    __gmcp_merge_gmcp_sub_tables(a, "Char")
    assert.are.same({Vitals = {mp = 2}}, a.Char)
  end)
end)
