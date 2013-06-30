describe("Tests Other.lua functions", function()
  describe("Tests speedwalking() utility", function()
    setup(function()
      echo      = function() end
      send      = function() end
      tempTimer = function(time, code)
        if type(code) == "string" then
          loadstring(code)()
        elseif type(code) == "function" then
          code()
        else
          error("tempTimer: Code must be a string or a function.")
        end
      end

      -- add in the location of our files
      local oldpath = package.path
      package.path = "../lua/?.lua;"
      require"Other"
      package.path = oldpath

    end)

    it("Tests basic speedwalk() with chained directions", function()
      send = spy.new(send)

      -- Will walk 16 times down, once southeast, once up. All in immediate succession.
      speedwalk('16d1se1u')
      assert.spy(send).was.called(18)
      assert.spy(send).was.called_with("d")
      assert.spy(send).was.called_with("se")
      assert.spy(send).was.called_with("u")
      assert.spy(send).was_not_called_with("e")
    end)

    it("Tests basic speedwalk() with commas as separators", function()
      send = spy.new(send)

      -- Will walk twice northeast, thrice east, twice north, once east. All in immediate succession.")
      speedwalk('2ne,3e,2n,e')
      assert.spy(send).was.called(8)
      assert.spy(send).was.called_with("ne")
      assert.spy(send).was.called_with("e")
      assert.spy(send).was.called_with("n")
    end)

    it("tests reverse speedwalk", function()
      send = spy.new(send)

      speedwalk("5sw - 3s - 2n - w", true)
      -- Will walk backwards: east, twice south, thrice, north, five times northeast. All in immediate succession.
      assert.spy(send).was.called(11)
      assert.spy(send).was.called_with("ne")
      assert.spy(send).was.called_with("n")
      assert.spy(send).was.called_with("s")
      assert.spy(send).was.called_with("e")
      assert.spy(send).was.was_not_called_with("w")
    end)

    it("tests reverse speedwalk with a delay", function()
      send           = spy.new(send)
      speedwalktimer = spy.new(speedwalktimer)
      tempTimer      = spy.new(tempTimer)

      speedwalk("3w, 2ne, w, u", true, 1.25)
      -- Will walk backwards: down, east, twice southwest, thrice east, with 1.25 seconds delay between every move.

      assert.spy(speedwalktimer).was.called()
      assert.spy(send).was.called(7)
      assert.spy(tempTimer).was.called(6)
    end)
  end)
end)
