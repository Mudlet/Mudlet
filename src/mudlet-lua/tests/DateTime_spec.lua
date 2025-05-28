describe("Tests DateTime.lua functions", function()

  describe("Tests the functionality of shms", function()
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

end)
