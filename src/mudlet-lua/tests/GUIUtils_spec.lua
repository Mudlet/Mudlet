describe("Tests the GUI utilities as far as possible without mudlet", function()

  describe("Test the operation of the ansi2decho function", function()

    it("Should have loaded the function successfully", function()
      assert.truthy(ansi2decho)
    end)

    it("Should convert simple single ANSI sequences correctly", function()
      local sequences = {
        {"\27[0m", "<r>"},
        {"\27[00m", "<r>"},
        {"\27[30m", "<0,0,0>"},
        {"\27[31m", "<128,0,0>"},
        {"\27[32m", "<0,128,0>"},
        {"\27[33m", "<128,128,0>"},
        {"\27[34m", "<0,0,128>"},
        {"\27[35m", "<128,0,128>"},
        {"\27[36m", "<0,128,128>"},
        {"\27[37m", "<192,192,192>"},
        {"\27[40m", "<:0,0,0>"},
        {"\27[41m", "<:128,0,0>"},
        {"\27[42m", "<:0,128,0>"},
        {"\27[43m", "<:128,128,0>"},
        {"\27[44m", "<:0,0,128>"},
        {"\27[45m", "<:128,0,128>"},
        {"\27[46m", "<:0,128,128>"},
        {"\27[47m", "<:192,192,192>"},
        {"\27[90m", "<128,128,128>"},
        {"\27[91m", "<255,0,0>"},
        {"\27[92m", "<0,255,0>"},
        {"\27[93m", "<255,255,0>"},
        {"\27[94m", "<0,0,255>"},
        {"\27[95m", "<255,0,255>"},
        {"\27[96m", "<0,255,255>"},
        {"\27[97m", "<255,255,255>"},
        {"\27[100m", "<:128,128,128>"},
        {"\27[101m", "<:255,0,0>"},
        {"\27[102m", "<:0,255,0>"},
        {"\27[103m", "<:255,255,0>"},
        {"\27[104m", "<:0,0,255>"},
        {"\27[105m", "<:255,0,255>"},
        {"\27[106m", "<:0,255,255>"},
        {"\27[107m", "<:255,255,255>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should combine tags correctly", function()
      local sequences = {
        {"\27[0;30m", "<r><0,0,0>"},
        {"\27[1;30m", "<128,128,128>"},
        {"\27[1;40m", "<:0,0,0>"},
        {"\27[31;42m", "<128,0,0:0,128,0>"},
        {"\27[30;0m", "<r>"},
        {"\27[0;1;30;40m", "<r><128,128,128:0,0,0>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should leave normal text and other escape sequences alone", function()
      local sequences = {
        {"Hello World", "Hello World"},
        {"[Something in braces]", "[Something in braces]"},
        {"\27[4z<PROMPT>4876h, 3539m, 22200e, 21648w cexkdb-\27[4z</PROMPT>", "\27[4z<PROMPT>4876h, 3539m, 22200e, 21648w cexkdb-\27[4z</PROMPT>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should convert xterm256 codes correctly", function()
      local sequences = {
        { "\27[38;2;120;134;94m", "<120,134,94>"},
        { "\27[48;2;85;250;33m", "<:85,250,33>"},
        { "\27[38;2;120;134;94;48;2;85;250;33m", "<120,134,94:85,250,33>"},
        { "\27[38;2m", "<0,0,0>"},
        { "\27[38;2;120m", "<120,0,0>"},
        { "\27[38;2;120;134m", "<120,134,0>"},
        { "\27[38;5;4m", "<0,0,128>"},
        { "\27[48;5;3m", "<:128,128,0>"},
        { "\27[38;5;4;48;5;3m", "<0,0,128:128,128,0>"},
        { "\27[38;5;10m", "<0,255,0>"},
        { "\27[48;5;9m", "<:255,0,0>"},
        { "\27[38;5;10;48;5;9m", "<0,255,0:255,0,0>"},
        { "\27[38;5;159m", "<175,255,255>"},
        { "\27[48;5;106m", "<:135,175,0>"},
        { "\27[38;5;159;48;5;106m", "<175,255,255:135,175,0>"},
        { "\27[38;5;240m", "<88,88,88>"},
        { "\27[48;5;245m", "<:138,138,138>"},
        { "\27[38;5;240;48;5;245m", "<88,88,88:138,138,138>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should convert some real life examples correctly", function()
      local sequences = {
        {"\27[4z<PROMPT>\27[0;32;40m4876h, \27[0;1;33;40m3539m, \27[0;1;31;40m22200e, \27[0;1;32;40m21648w \27[0;37;40mcexkdb-\27[4z</PROMPT>", "\27[4z<PROMPT><r><0,128,0:0,0,0>4876h, <r><255,255,0:0,0,0>3539m, <r><255,0,0:0,0,0>22200e, <r><0,255,0:0,0,0>21648w <r><192,192,192:0,0,0>cexkdb-\27[4z</PROMPT>"},
        {'\27[0;1;36;40mYou say in a baritone voice, "Test."\27[0;37;40m', '<r><0,255,255:0,0,0>You say in a baritone voice, "Test."<r><192,192,192:0,0,0>'},
        {'\27[38;5;179;48;5;230mYou say in a baritone voice, "Test."\27[0;37;40m', '<215,175,95:255,255,215>You say in a baritone voice, "Test."<r><192,192,192:0,0,0>'}
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

  end)

  describe("Tests the functionality of decho2ansi()", function()
    local simple_original = "<128,0,0>This is in red<r> And then reset."
    local simple_expected = "\27[38:2::128:0:0mThis is in red\27[0m And then reset."

    it("should convert a simple decho string to an equivalent ansi string", function()
      local actual = decho2ansi(simple_original)
      assert.equals(simple_expected, actual)
    end)

    it("should create ansi which can be converted back to the same decho string", function()
      local actual = ansi2decho(decho2ansi(simple_original))
      assert.equals(simple_original, actual)
    end)

    it("should handle bold", function()
      local expected = "\27[1mbold\27[22m"
      local actual = decho2ansi("<b>bold</b>")
      assert.equals(expected, actual)
    end)

    it("should handle underline", function()
      local expected = "\27[4munderline\27[24m"
      local actual = decho2ansi("<u>underline</u>")
      assert.equals(expected, actual)
    end)

    it("should handle italics", function()
      local expected = "\27[3mitalics\27[23m"
      local actual = decho2ansi("<i>italics</i>")
      assert.equals(expected, actual)
    end)

    it("should handle strikeout", function()
      local expected = "\27[9mstrikeout\27[29m"
      local actual = decho2ansi("<s>strikeout</s>")
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of ansi2string()", function()
    it("should return the string fed into it with ansi codes removed", function()
      local original = '\27[38;5;179;48;5;230mYou say in a baritone voice, "Test."\27[0;37;40m'
      local expected = 'You say in a baritone voice, "Test."'
      local actual = ansi2string(original)
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of setHexFgColor()", function()

    it("Should convert hex string correctly", function()
      local hexStrings = {
        {"000000", { r = 0, g = 0, b = 0 }},
        {"FFFFFF", { r = 255, g = 255, b = 255 }},
        {"B22222", { r = 178, g = 34, b = 34 }},
      }
      local origSetFgColor = _G.setFgColor
      local outputTable
      _G.setFgColor = function(r, g, b)
        outputTable = { r = r, g = g, b = b }
      end
      for _, pair in ipairs(hexStrings) do
        setHexFgColor(pair[1])
        assert.are.same(pair[2], outputTable)
      end
      _G.setFgColor = origSetFgColor
    end)

  end)

  describe("Tests the functionality of setHexBgColor()", function()

    it("Should convert hex string correctly", function()
      local hexStrings = {
        {"000000", { r = 0, g = 0, b = 0 }},
        {"FFFFFF", { r = 255, g = 255, b = 255 }},
        {"B22222", { r = 178, g = 34, b = 34 }},
      }
      local origSetBgColor = _G.setBgColor
      local outputTable
      _G.setBgColor = function(r, g, b)
        outputTable = { r = r, g = g, b = b }
      end
      for _, pair in ipairs(hexStrings) do
        setHexBgColor(pair[1])
        assert.are.same(pair[2], outputTable)
      end
      _G.setBgColor = origSetBgColor
    end)

  end)

  describe("Tests the functionality of closestColor", function()
    it("Should handle a table of {R,G,B} components: closestColor({R,G,B})", function()
      local expected = "ansi_001"
      local actual = closestColor({127,0,0})
      assert.equals(expected, actual)
    end)

    it("Should handle separate R,G,B parameters: closestColor(R,G,B)", function()
      local expected = "ansi_001"
      local actual = closestColor(127,0,0)
      assert.equals(expected, actual)
    end)

    it("Should handle a decho color string: closestColor('<R,G,B>')", function()
      local expected = "ansi_001"
      local actual = closestColor({127,0,0})
      assert.equals(expected, actual)
    end)

    it("Should handle an hecho # color string: closestColor('#RRGGBB')", function()
      local expected = "ansi_001"
      local actual = closestColor("#7f0000")
      assert.equals(expected, actual)
    end)

    it("Should handle an hecho |c color string: closestColor('|cRRGGBB')", function()
      local expected = "ansi_001"
      local actual = closestColor("|c7f0000")
      assert.equals(expected, actual)
    end)

    it("Should handle return the parameter if it's an entry in color_table: closestColor('purple')", function()
      local expected = "purple"
      local actual = closestColor("purple")
      assert.equals(expected, actual)
    end)

    it("Should return nil + error if handed garbage: closestColor('asdf')", function()
      local expectedErr = "Could not parse asdf into a set of RGB coordinates to look for.\n"
      local actual, actualErr = closestColor("asdf")
      assert.is_nil(actual)
      assert.equals(expectedErr, actualErr)
    end)

    it("Should return nil + error if handed garbage: closestColor({'tea', 1, 1})", function()
      local expectedErr = "Could not parse tea,1,1 into RGB coordinates to look for.\n"
      local actual, actualErr = closestColor({'tea', 1, 1})
      assert.is_nil(actual)
      assert.equals(expectedErr, actualErr)
    end)

    it("Should return nil + error if handed garbage: closestColor({1, 1})", function()
      local expectedErr = "Could not parse 1,1 into RGB coordinates to look for.\n"
      local actual, actualErr = closestColor({1, 1})
      assert.is_nil(actual)
      assert.equals(expectedErr, actualErr)
    end)

    it("Should return nil + error if handed garbage: closestColor({500, 0, 1})", function()
      local expectedErr = "Could not parse 500,0,1 into RGB coordinates to look for.\n"
      local actual, actualErr = closestColor({500, 0, 1})
      assert.is_nil(actual)
      assert.equals(expectedErr, actualErr)
    end)

    it("Should return nil + error if handed garbage: closestColor(true)", function()
      local expectedErr = "Could not parse your parameters into RGB coordinates.\n"
      local actual, actualErr = closestColor(true)
      assert.is_nil(actual)
      assert.equals(expectedErr, actualErr)
    end)
  end)

  describe("Tests the functionality of copy2decho", function()
    it ("Should return an empty string if line == ''", function()
      local oldgcl = getCurrentLine
      _G.getCurrentLine = spy.new(function()
        return ""
      end)
      local expected = ""
      local actual = copy2decho()
      assert.equals(expected, actual)
      assert.spy(_G.getCurrentLine).was.called()
      _G.getCurrentLine = oldgcl
    end)
  end)

  describe("Tests the functionality of _Echoes.Process()", function()
    it("Should parse hex patterns correctly", function()
      assert.are.same(
        _Echos.Process('#ff0000Red', 'Hex'),
        { "", { fg = { 255, 0, 0 } }, "Red" }
      )

      assert.are.same(
        _Echos.Process('#rReset', 'Hex'),
        { "", "\27reset", "Reset" }
      )

      assert.are.same(
        _Echos.Process('#bBold#/b', 'Hex'),
        { "", "\27bold", "Bold", "\27boldoff", "" }
      )

      assert.are.same(
        _Echos.Process('#iItalics#/i', 'Hex'),
        { "", "\27italics", "Italics", "\27italicsoff", "" }
      )

      assert.are.same(
        _Echos.Process('#uUnderline#/u', 'Hex'),
        { "", "\27underline", "Underline", "\27underlineoff", "" }
      )
    end)

    it("Should parse decimal patterns correctly", function()
      assert.are.same(
        _Echos.Process('<255,0,0>Red', 'Decimal'),
        { "", { fg = { "255", "0", "0" } }, "Red" }
      )

      assert.are.same(
        _Echos.Process('<r>Reset', 'Decimal'),
        { "", "\27reset", "Reset" }
      )

      assert.are.same(
        _Echos.Process('<b>Bold</b>', 'Decimal'),
        { "", "\27bold", "Bold", "\27boldoff", "" }
      )

      assert.are.same(
        _Echos.Process('<i>Italics</i>', 'Decimal'),
        { "", "\27italics", "Italics", "\27italicsoff", "" }
      )

      assert.are.same(
        _Echos.Process('<u>Underline</u>', 'Decimal'),
        { "", "\27underline", "Underline", "\27underlineoff", "" }
      )
    end)

    it("Should parse color patterns correctly", function()
      assert.are.same(
        _Echos.Process('<red>Red', 'Color'),
        { "", { fg = { 255, 0, 0 } }, "Red" }
      )

      assert.are.same(
        _Echos.Process('<r>Reset', 'Color'),
        { "", "\27reset", "Reset" }
      )

      assert.are.same(
        _Echos.Process('<b>Bold</b>', 'Color'),
        { "", "\27bold", "Bold", "\27boldoff", "" }
      )

      assert.are.same(
        _Echos.Process('<i>Italics</i>', 'Color'),
        { "", "\27italics", "Italics", "\27italicsoff", "" }
      )

      assert.are.same(
        _Echos.Process('<u>Underline</u>', 'Color'),
        { "", "\27underline", "Underline", "\27underlineoff", "" }
      )
    end)
  end)
end)

--[[
  TODO:
    replaceLine and variants
--]]
