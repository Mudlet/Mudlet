-- https://wiki.mudlet.org/w/Manual:UI_Functions
describe("Tests UI functions", function()

  describe("Test the operation of the copy2decho function", function()
    setup(function()
      -- create Mudlet miniconsole top-left
      createMiniConsole("testconsole", 0,0,800,100)
      setMiniConsoleFontSize("testconsole", 10)
      setBackgroundColor("testconsole", unpack(color_table.DarkSlateGray))
      setWindowWrap("testconsole", 100)
    end)

    -- clear miniconsole before each test
    before_each(function()
      clearWindow("testconsole")
    end)

    teardown(function()
      hideWindow("testconsole")
    end)

    it("Should copy colored English text", function()
      local testdecho = "<50,50,0:0,255,0>test<r><192,192,192:0,0,0> <r><255,0,0:0,0,0>red <r><0,255,0:0,0,0>green<r><0,0,255:0,0,0>blue<r>"
      decho("testconsole", testdecho)

      assert.are.equal(testdecho, copy2decho("testconsole"))
    end)

    -- TODO: https://github.com/Mudlet/Mudlet/issues/5590
    -- it("Should copy text with background transparency", function()
    --   local testdecho = "<50,50,0:0,255,0,100>semi-transparent"
    --   decho("testconsole", testdecho)

    --   assert.are.equal(testdecho, copy2decho("testconsole", true))
    -- end)

    it("Should copy colored Chinese text", function()
      local testdecho = "<50,50,0:0,255,0>测试<r><192,192,192:0,0,0> <r><255,0,0:0,0,0>红色<r><0,255,0:0,0,0>绿色<r><0,0,255:0,0,0>蓝色<r>"
      decho("testconsole", testdecho)

      assert.are.equal(testdecho, copy2decho("testconsole"))
    end)

    -- TODO: https://github.com/Mudlet/Mudlet/issues/5589
    -- it("Should copy2decho text with italics, bold, and underline", function()
    --   local testdecho = "separate: <i>italic</i>, <b>bold</b>, <u>underline</u>. all together: <i>italic<b>bold<u>underline<r>"
    --   decho("testconsole", testdecho)

    --   assert.are.equal(testdecho, copy2decho("testconsole"))
    -- end)
  end)

  describe("Test the operation of the copy2html function", function()
    setup(function()
      -- create Mudlet miniconsole top-left
      createMiniConsole("testconsole", 0,0,800,100)
      setMiniConsoleFontSize("testconsole", 10)
      setBackgroundColor("testconsole", unpack(color_table.DarkSlateGray))
      setWindowWrap("testconsole", 100)
    end)

    -- clear miniconsole before each test
    before_each(function()
      clearWindow("testconsole")
    end)

    it("Should copy colored English text", function()
      local testdecho = "<50,50,0:0,255,0>test<r><192,192,192:0,0,0> <r><255,0,0:0,0,0>red <r><0,255,0:0,0,0>green<r><0,0,255:0,0,0>blue<r>"
      local outputhtml = [[<span style='color: rgb(50,50,0);background: rgb(0,255,0);'>test</span><span style='color: rgb(192,192,192);background: rgb(0,0,0);'> </span><span style='color: rgb(255,0,0);background: rgb(0,0,0);'>red </span><span style='color: rgb(0,255,0);background: rgb(0,0,0);'>green</span><span style='color: rgb(0,0,255);background: rgb(0,0,0);'>blue</span>]]
      decho("testconsole", testdecho)

      assert.are.equal(outputhtml, copy2html("testconsole"))
    end)

    it("Should copy colored Chinese text", function()
      local testdecho = "<50,50,0:0,255,0>测试<r><192,192,192:0,0,0> <r><255,0,0:0,0,0>红色<r><0,255,0:0,0,0>绿色<r><0,0,255:0,0,0>蓝色<r>"
      local outputhtml = [[<span style='color: rgb(50,50,0);background: rgb(0,255,0);'>测试</span><span style='color: rgb(192,192,192);background: rgb(0,0,0);'> </span><span style='color: rgb(255,0,0);background: rgb(0,0,0);'>红色</span><span style='color: rgb(0,255,0);background: rgb(0,0,0);'>绿色</span><span style='color: rgb(0,0,255);background: rgb(0,0,0);'>蓝色</span>]]

      decho("testconsole", testdecho)

      assert.are.equal(outputhtml, copy2html("testconsole"))
    end)
  end)

  describe("Test the operation of the windowType() function", function()
    it("Should identify an existing label correctly", function()
      createLabel("testlabel", 0,0,0,0, 1)

      assert.are.equal(windowType("testlabel"), "label")
    end)

    it("Should not identify a non-existing label", function()
      assert.are.equal(windowType("fake label"), nil)
    end)

    it("Should identify the main window as 'main'", function()
      assert.are.equal(windowType("main"), "main")
    end)

    it("Should identify an existing userwindow correctly", function()
      openUserWindow("testuserwindow")

      assert.are.equal(windowType("testuserwindow"), "userwindow")
    end)

    it("Should not identify a non-existing userwindow", function()
      assert.are.equal(windowType("fake userwindow"), nil)
    end)

    it("Should identify a buffer correctly", function()
      createBuffer("testbuffer")

      assert.are.equal(windowType("testbuffer"), "buffer")
    end)

    it("Should identify an existing miniconsole", function()
      createMiniConsole("testminiconsole", 0,0,0,0)

      assert.are.equal(windowType("testminiconsole"), "miniconsole")
    end)

    it("Should not identify a non-existing miniconsole", function()
      assert.are.equal(windowType("fake miniconsole"), nil)
    end)

    it("Should identify a commandline", function()
      createCommandLine("testcommandline", 0,0,0,0)

      assert.are.equal(windowType("testcommandline"), "commandline")
    end)

    it("Should not identify a non-existing commandline", function()
      assert.are.equal(windowType("fake commandline"), nil)
    end)

    teardown(function()
      deleteLabel("testlabel")
      hideWindow("testuserwindow")
    end)
  end)
end)
