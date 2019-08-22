describe("Tests Other.lua functions", function()
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
    setup(function()
      -- fake getMudletversion here
      ---[[
      local function getMudletVersion()
        return 2,0,1,"-dev"
      end
      -- ]]
    end)

    it("tests the comparisons", function()
      assert.is_true(mudletOlderThan(2, 5, 10))
      assert.is_true(mudletOlderThan(3, 5, 10))
      assert.is_false(mudletOlderThan(1, 0, 0))

    end)
  end)
end)
