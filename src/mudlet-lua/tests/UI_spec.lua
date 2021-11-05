-- https://wiki.mudlet.org/w/Manual:UI_Functions
describe("Tests UI functions", function()

  describe("Test the operation of the copy2decho function", function()
    setup(function()
      -- create Mudlet miniconsole top-left
      createMiniConsole("testing", 0,0,650,300)
      setMiniConsoleFontSize("testing", 10)
      setWindowWrap("testing", 40)
    end)

    -- clear miniconsole before each test
    before_each(function()
      clearWindow("testing")
    end)

    it("Should copy2decho colored English text", function()
      local testdecho = "<50,50,0:0,255,0>test <255,0,0>red <0,255,0>green <0,0,255>blue"
      decho("testing", testdecho)

      assert.are.equal(testdecho, copy2decho("testing"))
    end)
  end)
end)
