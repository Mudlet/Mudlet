describe("Tests Other.lua functions", function()

  describe("sendAll", function()
    setup(function()
      _G.echo      = function() end
      _G.send      = function() end
      _G.tempTimer = function(time, code)
        if type(code) == "string" then
          loadstring(code)()
        elseif type(code) == "function" then
          code()
        else
          error("tempTimer: Code must be a string or a function.")
        end
      end
    end)

    it("should send one command if it is only given one parameter", function()
      local send = spy.on(_G, "send")
      sendAll("look")
      assert.spy(send).was.called(1)
      assert.spy(send).was.called_with("look", true)
    end)

    it("should send multiple commands when given multiple string parameters", function()
      local send = spy.on(_G, "send")
      local commands = {
        "get gold from pouch",
        "buy potion",
        "put gold in pouch"
      }
      sendAll(unpack(commands))
      assert.spy(send).was.called(#commands)
      for _,command in ipairs(commands) do
        assert.spy(send).was.called_with(command, true)
      end
    end)

    it("should pass along the final boolean argument to all sends if provided", function()
      local send = spy.on(_G, "send")
      sendAll("get gold from pouch", "buy potion", "put gold in pouch", false)
      assert.spy(send).was.called(3)
      assert.spy(send).was.called_with("get gold from pouch", false)
      assert.spy(send).was.called_with("buy potion", false)
      assert.spy(send).was.called_with("put gold in pouch", false)
    end)

    it("should pass along the final boolean argument to all sends if provided", function()
      local send = spy.on(_G, "send")
      sendAll("get gold from pouch", "buy potion", "put gold in pouch", true)
      assert.spy(send).was.called(3)
      assert.spy(send).was.called_with("get gold from pouch", true)
      assert.spy(send).was.called_with("buy potion", true)
      assert.spy(send).was.called_with("put gold in pouch", true)
    end)
  end)

  describe("permGroup", function()

    describe("success", function()
      setup(function()
        _G.oldPermTimer = _G.permTimer
        _G.oldPermSubstringTrigger = _G.permSubstringTrigger
        _G.oldPermAlias = _G.permAlias
        _G.permTimer = function() return 1 end
        _G.permSubstringTrigger = function() return 1 end
        _G.permAlias = function() return 1 end
      end)

      teardown(function()
        _G.permTimer = _G.oldPermTimer
        _G.permSubstringTrigger = _G.oldPermSubstringTrigger
        _G.permAlias = _G.oldPermAlias
        _G.oldPermTimer = nil
        _G.oldPermSubstringTrigger = nil
        _G.oldPermAlias = nil
      end)

      it("should return true if the timer group was created", function()
        local permTimer = spy.on(_G, "permTimer")
        local name = "TestTimer"
        local parent = "Parent"
        local successful = permGroup(name, "timer", parent)
        assert.spy(permTimer).was.called_with(name, parent, 0, "")
        assert.is_true(successful)
      end)

      it("should return true if the alias group was created", function()
        local permAlias = spy.on(_G, "permAlias")
        local name = "TestAlias"
        local parent = "Parent"
        local successful = permGroup(name, "alias", parent)
        assert.spy(permAlias).was.called_with(name, parent, "", "")
        assert.is_true(successful)
      end)

      it("should return true if the trigger group was created", function()
        local permTrigger = spy.on(_G, "permSubstringTrigger")
        local name = "TestTrigger"
        local parent = "Parent"
        local successful = permGroup(name, "trigger", parent)
        assert.spy(permTrigger).was.called_with(name, parent, {}, "")
        assert.is_true(successful)
      end)
    end)

    describe("failure", function()
      setup(function()
        _G.oldPermTimer = _G.permTimer
        _G.oldPermSubstringTrigger = _G.permSubstringTrigger
        _G.oldPermAlias = _G.permAlias
        _G.permTimer = function() return -1 end
        _G.permSubstringTrigger = function() return -1 end
        _G.permAlias = function() return -1 end
      end)

      teardown(function()
        _G.permTimer = _G.oldPermTimer
        _G.permSubstringTrigger = _G.oldPermSubstringTrigger
        _G.permAlias = _G.oldPermAlias
        _G.oldPermTimer = nil
        _G.oldPermSubstringTrigger = nil
        _G.oldPermAlias = nil
      end)

      it("should return false if the timer group was not created", function()
        local permTimer = spy.on(_G, "permTimer")
        local name = "TestTimer"
        local parent = "Parent"
        local successful = permGroup(name, "timer", parent)
        assert.spy(permTimer).was.called_with(name, parent, 0, "")
        assert.is_false(successful)
      end)

      it("should return false if the alias group was not created", function()
        local permAlias = spy.on(_G, "permAlias")
        local name = "TestAlias"
        local parent = "Parent"
        local successful = permGroup(name, "alias", parent)
        assert.spy(permAlias).was.called_with(name, parent, "", "")
        assert.is_false(successful)
      end)

      it("should return false if the trigger group was not created", function()
        local permTrigger = spy.on(_G, "permSubstringTrigger")
        local name = "TestTrigger"
        local parent = "Parent"
        local successful = permGroup(name, "trigger", parent)
        assert.spy(permTrigger).was.called_with(name, parent, {}, "")
        assert.is_false(successful)
      end)
    end)
  end)

  describe("io.exists", function()
    it("should return true if the item exists", function()
      local item = getMudletHomeDir()
      assert.is_true(io.exists(item))
    end)

    it("should return false if the item does not exist", function()
      local item = getMudletHomeDir() .. math.random(10000)
      assert.is_false(io.exists(item))
    end)
  end)

  describe("xor(a,b)", function()
    it("should return true if a is false and b is true", function()
      assert.is_true(xor(true, false))
    end)

    it("should return true if a is true and b is false", function()
      assert.is_true(xor(false,true))
    end)
    
    it("should return false if a is true and b is true", function()
      assert.is_false(xor(true, true))
    end)

    it("should return false if a is false and b is false", function()
      assert.is_false(xor(false, false))
    end)
  end)

  describe("Tests speedwalking() utility", function()
    -- Note that busted insulates changes in each test file, so
    -- these changes won't escape outside this file.
    setup(function()
      _G.echo      = function() end
      _G.send      = function() end
      _G.tempTimer = function(time, code)
        if type(code) == "string" then
          loadstring(code)()
        elseif type(code) == "function" then
          code()
        else
          error("tempTimer: Code must be a string or a function.")
        end
      end
    end)

    it("Tests basic speedwalk() with chained directions", function()
      local send = spy.on(_G, "send")

      -- Will walk 16 times down, once southeast, once up. All in immediate succession.
      speedwalk('16d1se1u')
      assert.spy(send).was.called(18)
      assert.spy(send).was.called_with("d", true)
      assert.spy(send).was.called_with("se", true)
      assert.spy(send).was.called_with("u", true)
      assert.spy(send).was_not_called_with("e", true)
    end)

    it("Tests basic speedwalk() with commas as separators", function()
      local send = spy.on(_G, "send")

      -- Will walk twice northeast, thrice east, twice north, once east. All in immediate succession.")
      speedwalk('2ne,3e,2n,e')
      assert.spy(send).was.called(8)
      assert.spy(send).was.called_with("ne", true)
      assert.spy(send).was.called_with("e", true)
      assert.spy(send).was.called_with("n", true)
    end)

    it("tests reverse speedwalk", function()
      local send = spy.on(_G, "send")

      speedwalk("5sw - 3s - 2n - w", true)
      -- Will walk backwards: east, twice south, thrice, north, five times northeast. All in immediate succession.
      assert.spy(send).was.called(11)
      assert.spy(send).was.called_with("ne", true)
      assert.spy(send).was.called_with("n", true)
      assert.spy(send).was.called_with("s", true)
      assert.spy(send).was.called_with("e", true)
      assert.spy(send).was.was_not_called_with("w", true)
    end)

    it("tests reverse speedwalk with a delay", function()
      local send           = spy.on(_G, "send")
      local speedwalktimer = spy.on(_G, "speedwalktimer")
      local tempTimer      = spy.on(_G, "tempTimer")

      speedwalk("3w, 2ne, w, u", true, 1.25)
      -- Will walk backwards: down, east, twice southwest, thrice east, with 1.25 seconds delay between every move.

      assert.spy(speedwalktimer).was.called()
      assert.spy(send).was.called(7)
      assert.spy(tempTimer).was.called(6)
    end)
  end)

  describe("Tests mudletOlderThan()", function()
    it("tests the comparisons", function()
      local versionTable = getMudletVersion()
      local currentVersion = { versionTable.major, versionTable.minor, versionTable.revision }
      local newerVersion = { versionTable.major + 1 , versionTable.minor + 1 , versionTable.revision + 1 }
      local olderVersion = { versionTable.major - 1 , versionTable.minor - 1 , versionTable.revision - 1 }
      assert.is_true(mudletOlderThan(unpack(newerVersion)))
      assert.is_false(mudletOlderThan(unpack(olderVersion)))
      assert.is_false(mudletOlderThan(unpack(currentVersion)))
    end)
  end)

  describe("shms(seconds, echoResults)", function()
    it("should seconds as hh, mm, ss when called without a second parameter", function()
      local expected = { '00', '01', '00' }
      assert.are.same(expected, {shms(60)})
      local expected = { '01', '32', '15' }
      assert.are.same(expected, {shms(5535)})
    end)

    it("should cecho the information out if the second parameter is truthy", function()
      local seconds = 5535
      local expected = { '01', '32', '15' }
      local expectedString = string.format("<green>%s <grey>seconds converts to: <green>%s<white>h,<green> %s<white>m <grey>and<green> %s<white>s.", seconds, expected[1], expected[2], expected[3])
      -- quick and dirty stub
      _G.oldcecho = _G.cecho
      _G.cecho = function() end
      local cecho = spy.on(_G, "cecho")
      shms(seconds, true)
      assert.spy(cecho).was.called(1)
      assert.spy(cecho).was.called_with(expectedString)
      -- undo my sins
      _G.cecho = _G.oldcecho
      _G.oldcecho = nil
    end)
  end)

  describe("_comp(A,B)", function()

    it("compares two numbers the same as ==", function()
      assert.is_true(_comp(5,5))
      assert.is_false(_comp(5,6))
    end)

    it("compares two strings the same as ==", function()
      assert.is_true(_comp("Test", "Test"))
      assert.is_false(_comp("Test1", "Test2"))
    end)

    it("compares booleans the same as ==", function()
      assert.is_true(_comp(false, false))
      assert.is_false(_comp(true,false))
    end)

    it("returns true if table B has the same value for every key which table A contains.", function()
      local tableA = { "One", "Two" }
      local tableB = { "One", "Two" }
      assert.is_true(_comp(tableA, tableB))
    end)

    it("returns false if table B has a different value for a key than table A has", function()
      local tableA = { "One", 2 }
      local tableB = { "One", "Two" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("returns false if table A has a kick which table B does not have", function()
      local tableA = { "One", "Two", "Three" }
      local tableB = { "One", "Two" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("returns false if table B has a key which table A does not have", function()
      local tableA = { "One", "Two" }
      local tableB = { "One", "Two", "Three" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("compares two tables recursively", function()
      local tableA = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableB = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableC = {
        "One",
        { "First", "2nd" },
        "Three"
      }
      assert.is_true(_comp(tableA, tableB))
      assert.is_false(_comp(tableB, tableC))
    end)

    it("should return the same regardless of the order of the arguments", function()
      local tableA = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableB = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableC = {
        "One",
        { "First", "2nd" },
        "Three"
      }
      assert.are.same(_comp(tableA, tableB), _comp(tableB, tableA))
      assert.are.same(_comp(tableB, tableC), _comp(tableC, tableB))
    end)
  end)

    --[[ 
    TODO:
      remember()
      loadVars()
      saveVars()
      table.save()
      table.pickle()
      tacle.load()
      table.unpickle()
      phpTable()
      getColorWildcard()
      lockExit()
      hasExitLock()
      registerAnonymousEventHandler()
      killAnonymousEventHandler()
      dispatchEventToFunctions()
      timeframe()
      killtimeframe()
      translateTable()
  ]]
end)
