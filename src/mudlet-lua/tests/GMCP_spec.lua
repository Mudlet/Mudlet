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

  it("Should replace the sub table with a payload that is not an object", function()
    for _, v in ipairs({"x", 7, true, yajl.null}) do
      local a = {Char = {hp = 1}, __needMerge = v}
      __gmcp_merge_gmcp_sub_tables(a, "Char")
      assert.are.equal(v, a.Char)
      assert.is_nil(a.__needMerge)
    end
  end)

  it("Should leave no staging key behind when a merge key arrives as a scalar", function()
    gmcp.MergeSpec = nil
    finally(function() gmcp.MergeSpec = nil end)
    setMergeTables("MergeSpec.Status")
    feedTelnet('<T_IAC><T_SB><O_GMCP>MergeSpec.Status {"hp":1}<T_IAC><T_SE>')
    feedTelnet('<T_IAC><T_SB><O_GMCP>MergeSpec.Status "dead"<T_IAC><T_SE>')
    assert.is_nil(gmcp.MergeSpec.__needMerge)
    assert.are.equal("dead", gmcp.MergeSpec.Status)
    feedTelnet('<T_IAC><T_SB><O_GMCP>MergeSpec.Status {"hp":2}<T_IAC><T_SE>')
    assert.are.same({hp = 2}, gmcp.MergeSpec.Status)
  end)

  it("Should merge nested tables by replacing them wholesale", function()
    local a = {Char = {Vitals = {hp = 1}}, __needMerge = {Vitals = {mp = 2}}}
    __gmcp_merge_gmcp_sub_tables(a, "Char")
    assert.are.same({Vitals = {mp = 2}}, a.Char)
  end)
end)

describe("Tests that GMCP data decodes as json_to_value decodes it", function()
  local stockDecoder = _G.json_to_value
  local stockDecoderIsC = debug.getinfo(stockDecoder, "S").what == "C"
  local unset = {}

  teardown(function()
    gmcp.JsonSpec = nil
  end)

  -- what gmcp.JsonSpec.Doc holds after the game sends payload, or unset when it was rejected
  local function received(payload)
    gmcp.JsonSpec = {Doc = unset}
    -- feedTelnet() reads <..> as codes, and IAC must be doubled inside SB
    local escaped = payload:gsub("[<>\255]", {["<"] = "<<", [">"] = ">>", ["\255"] = "\255\255"})
    feedTelnet("<T_IAC><T_SB><O_GMCP>JsonSpec.Doc " .. escaped .. "<T_IAC><T_SE>")
    return gmcp.JsonSpec.Doc
  end

  -- a wrapper is not the stock decoder, so the data goes through it as a script's own decoder would;
  -- through _G, as busted gives each spec file globals of its own
  local function receivedThroughYajl(payload)
    local calls = 0
    _G.json_to_value = function(text)
      calls = calls + 1
      return stockDecoder(text)
    end
    local ok, value = pcall(received, payload)
    _G.json_to_value = stockDecoder
    assert(ok, value)
    assert.are.equal(1, calls)
    return value
  end

  -- the same values, the same yajl.null, and tables that pairs() walks in the same order
  local function sameValue(a, b, path)
    if type(a) ~= type(b) then
      return false, path .. ": " .. type(a) .. " vs " .. type(b)
    end
    if type(a) == "number" then
      if a ~= b or (a == 0 and 1 / a ~= 1 / b) then
        return false, path .. ": " .. string.format("%.17g vs %.17g", a, b)
      end
      return true
    end
    if type(a) ~= "table" then
      return rawequal(a, b), path .. ": " .. tostring(a) .. " vs " .. tostring(b)
    end
    if getmetatable(a) ~= getmetatable(b) then
      return false, path .. ": metatables differ"
    end
    local keyA, keyB = next(a), next(b)
    while keyA ~= nil or keyB ~= nil do
      if keyA ~= keyB then
        return false, path .. ": walks " .. tostring(keyA) .. " vs " .. tostring(keyB)
      end
      local same, why = sameValue(a[keyA], b[keyB], path .. "." .. tostring(keyA))
      if not same then
        return false, why
      end
      keyA, keyB = next(a, keyA), next(b, keyB)
    end
    return true
  end

  -- levels containers deep, objects and arrays in turn
  local function nested(levels)
    local opening, closing = {}, {}
    for level = 1, levels do
      opening[level] = level % 2 == 1 and '{"a":' or "["
      closing[levels - level + 1] = level % 2 == 1 and "}" or "]"
    end
    return table.concat(opening) .. "1" .. table.concat(closing)
  end

  local manyKeys = {}
  for i = 1, 60 do
    manyKeys[#manyKeys + 1] = string.format('"k%d":%d', (i * 37) % 101, i)
  end

  local tooDeep = {[nested(65)] = true, [nested(300)] = true, [nested(1000)] = true, [nested(6000)] = true}

  local documents = {
    '{"int":1,"neg":-7,"zero":0,"negzero":-0,"negzerofloat":-0.0,"frac":2.5,"tenth":0.1,"exp":1e3,"Exp":1E-3,"plus":123.456e+2}',
    '{"big":12345678901234567890,"past53":9007199254740993,"huge":1e400,"neghuge":-1e400,"tiny":5e-324,"under":1e-400}',
    '[' .. string.rep("1", 80) .. ',0.' .. string.rep("0", 300) .. '1,-' .. string.rep("9", 70) .. 'e-50]',
    '[' .. string.rep("1", 63) .. ',' .. string.rep("2", 64) .. ',1.' .. string.rep("5", 61) .. ',1.' .. string.rep("5", 62) .. ']',
    '{"a":1,"a":2,"b":{"x":1},"b":[1],"c":null,"c":3}',
    '{"nul":"\\u0000","mid":"a\\u0000b","pair":"\\ud83d\\ude00","high":"\\ud800","low":"\\udc00x","e":"\\u00e9","esc":"\\"\\\\\\/\\b\\f\\n\\r\\t"}',
    '{"raw":"caf\195\169 \240\159\152\128","key\195\169":1,"":"empty key"}',
    '{"o":{},"a":[],"oa":{"x":{},"y":[]},"aa":[[],[{}],{}]}',
    '[null,1,null,false,true,"s",null]',
    '{"n":null,"f":false,"t":true}',
    '{"1":"one","2":"two","10":"ten"}',
    '{' .. table.concat(manyKeys, ",") .. '}',
    -- pairs() can walk these in another order when a decoder presizes their tables
    '{"hp":"3200","maxhp":"3500"}', '{"name":"Vadi","level":80}',
    '[' .. string.rep("1,", 199) .. '1]',
    ' \t{ "spaced" : [ 1 , 2 ] , "x" : { } } \t',
    '5', '-0', '"text"', 'true', 'false', 'null', '[]', '{}',
    '{"esc":"\27[31mred"}',
    nested(63), nested(64), nested(65), nested(300), nested(1000), nested(6000),
    '{"a":}', '{"a":1', '[1,]', "{'a':1}", 'tru', '01', '1.', '.5', '-', '"\\x"', '{} x', '{}{}', '/* c */ {}', 'NaN', '[Infinity]', '"tab\there"', '{"text":"a\tb","n":1}',
  }

  it("builds the same value as yajl.to_value, or rejects what it rejects", function()
    for _, document in ipairs(documents) do
      local fast, slow = received(document), receivedThroughYajl(document)
      assert.are.equal(rawequal(slow, unset), rawequal(fast, unset), document:sub(1, 60))
      local same, why = sameValue(fast, slow, document:sub(1, 60))
      assert.is_true(same, why)
    end
  end)

  it("gives null as yajl.null", function()
    assert.is_true(rawequal(yajl.null, received('{"n":null}').n))
    assert.is_true(rawequal(yajl.null, received("null")))
  end)

  it("keeps the previous value when the data is not JSON", function()
    -- yajl takes a raw tab inside a string for an error, so a whole message with one is dropped
    for _, broken in ipairs({"{broken", '{"text":"a\tb"}'}) do
      gmcp.JsonSpec = {Doc = "before"}
      feedTelnet("<T_IAC><T_SB><O_GMCP>JsonSpec.Doc " .. broken .. "<T_IAC><T_SE>")
      assert.are.equal("before", gmcp.JsonSpec.Doc, broken)
    end
  end)

  it("keeps the previous value when a Lua error is raised while decoding", function()
    if not stockDecoderIsC then
      pending("the finaliser below raises only inside a decoder written in C")
      return
    end
    local raised, armed = false, true
    local pause = collectgarbage("setpause", 0)
    local stepmul = collectgarbage("setstepmul", 0)
    finally(function()
      armed = false
      collectgarbage("setpause", pause)
      collectgarbage("setstepmul", stepmul)
    end)
    -- a finaliser the collector runs from inside the decoder's own allocations, mid-parse
    local function chain()
      getmetatable(newproxy(true)).__gc = function()
        if not armed or raised then
          return
        end
        local running, caller = debug.getinfo(2, "S"), debug.getinfo(3, "f")
        if running.what == "C" and caller and caller.func == feedTelnet then
          raised = true
          error("raised by a finaliser while decoding")
        end
        chain()
      end
    end
    gmcp.JsonSpec = {Doc = "before"}
    chain()
    -- after setpause, as a full collection sets the next threshold from it
    collectgarbage()
    feedTelnet("<T_IAC><T_SB><O_GMCP>JsonSpec.Doc [" .. string.rep('{"k":"v","n":[1,2]},', 50) .. "{}]<T_IAC><T_SE>")
    assert.is_true(raised)
    assert.are.equal("before", gmcp.JsonSpec.Doc)
  end)

  it("hands the data to a script's own json_to_value", function()
    finally(function() _G.json_to_value = stockDecoder end)
    local seen
    _G.json_to_value = function(text)
      seen = text
      return "replaced"
    end
    assert.are.equal("replaced", received('{"a":1}'))
    assert.are.equal('{"a":1}', seen)
    _G.json_to_value = string.reverse
    assert.are.equal('}1:"a"{', received('{"a":1}'))
  end)

  it("calls yajl.to_value only for what it rejects or what nests past 64 levels", function()
    if not stockDecoderIsC then
      pending("parseJSON() only stands in for lua-yajl's yajl.to_value, a C function")
      return
    end
    local hook, mask, count = debug.gethook()
    finally(function() debug.sethook(hook, mask, count) end)
    local calls
    debug.sethook(function()
      if debug.getinfo(2, "f").func == stockDecoder then
        calls = calls + 1
      end
    end, "c")
    for _, document in ipairs(documents) do
      calls = 0
      local value = received(document)
      local expected = (rawequal(value, unset) or tooDeep[document]) and 1 or 0
      assert.are.equal(expected, calls, document:sub(1, 60))
    end
  end)
end)
