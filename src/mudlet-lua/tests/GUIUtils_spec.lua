describe("Tests the GUI utilities as far as possible without mudlet", function()

  describe("Tests the functionality of ansi2decho", function()

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

    it("Should match the user's custom colours if they've changed them", function()
      color_table.ansi_000 = { 0, 0, 1 }
      local expected = "<0,0,1>"
      local actual = ansi2decho("\27[30m", "<0,0,0>")
      color_table.ansi_000= { 0, 0, 0 }
      assert.are.same(expected, actual)
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

    it("Should handle italics", function()
      local sample = "\27[3mitalics\27[23m"
      local expected = "<i>italics</i>"
      local actual = ansi2decho(sample)
      assert.equals(expected, actual)
    end)

    it("Should handle underline", function()
      local sample = "\27[4munderline\27[24m"
      local expected = "<u>underline</u>"
      local actual = ansi2decho(sample)
      assert.equals(expected, actual)
    end)

    it("Should handle strikethrough", function()
      local sample = "\27[9mstrikethrough\27[29m"
      local expected = "<s>strikethrough</s>"
      local actual = ansi2decho(sample)
      assert.equals(expected, actual)
    end)

    it("Should handle overline", function()
      local sample = "\27[53moverline\27[55m"
      local expected = "<o>overline</o>"
      local actual = ansi2decho(sample)
      assert.equals(expected, actual)
    end)

    it("Should handle bold, before or after colours", function()
      local sequences = {
        {"\27[31m\27[1m", "<128,0,0><255,0,0>"},
        {"\27[1m\27[31m", "<255,0,0>"},
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
        {'\27[38;5;179;48;5;230mYou say in a baritone voice, "Test."\27[0;37;40m', '<215,175,95:255,255,215>You say in a baritone voice, "Test."<r><192,192,192:0,0,0>'},
        {'* \27[35m(a338f71)\27[m - \27[33m[Update release.yml]\27[m  \27[1;34m<TheLastDarkthorne>\27[m', '* <128,0,128>(a338f71)<r> - <128,128,0>[Update release.yml]<r>  <0,0,255><TheLastDarkthorne><r>'}
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

  end)

  describe("Tests the functionality of decho2ansi", function()
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

    it("should handle overline", function()
      local expected = "\27[53moverline\27[55m"
      local actual = decho2ansi("<o>overline</o>")
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of hecho2ansi", function()
    local simple_original = "#800000This is in red#r And then reset."
    local simple_expected = "\27[38:2::128:0:0mThis is in red\27[0m And then reset."

    it("should convert a simple hecho string to an equivalent ansi string", function()
      local actual = hecho2ansi(simple_original)
      assert.equals(simple_expected, actual)
    end)

    it("should handle bold", function()
      local expected = "\27[1mbold\27[22m"
      local actual = hecho2ansi("#bbold#/b")
      assert.equals(expected, actual)
    end)

    it("should handle underline", function()
      local expected = "\27[4munderline\27[24m"
      local actual = hecho2ansi("#uunderline#/u")
      assert.equals(expected, actual)
    end)

    it("should handle italics", function()
      local expected = "\27[3mitalics\27[23m"
      local actual = hecho2ansi("#iitalics#/i")
      assert.equals(expected, actual)
    end)

    it("should handle strikeout", function()
      local expected = "\27[9mstrikeout\27[29m"
      local actual = hecho2ansi("#sstrikeout#/s")
      assert.equals(expected, actual)
    end)

    it("should handle overline", function()
      local expected = "\27[53moverline\27[55m"
      local actual = hecho2ansi("#ooverline#/o")
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of cecho2ansi", function()
    local simple_original = "<red>This is in red<r> And then reset."
    local simple_expected = "\27[38:5:1mThis is in red\27[0m And then reset."

    it("should convert a simple cecho string to an equivalent ansi string", function()
      local actual = cecho2ansi(simple_original)
      assert.equals(simple_expected, actual)
    end)

    it("should convert a color name which doesn't have a direct ansi named equivalent", function()
      local actual = cecho2ansi("<DodgerBlue>")
      assert.equals("\27[38:2::30:144:255m", actual)
    end)

    it("should handle bold", function()
      local expected = "\27[1mbold\27[22m"
      local actual = cecho2ansi("<b>bold</b>")
      assert.equals(expected, actual)
    end)

    it("should handle underline", function()
      local expected = "\27[4munderline\27[24m"
      local actual = cecho2ansi("<u>underline</u>")
      assert.equals(expected, actual)
    end)

    it("should handle italics", function()
      local expected = "\27[3mitalics\27[23m"
      local actual = cecho2ansi("<i>italics</i>")
      assert.equals(expected, actual)
    end)

    it("should handle strikeout", function()
      local expected = "\27[9mstrikeout\27[29m"
      local actual = cecho2ansi("<s>strikeout</s>")
      assert.equals(expected, actual)
    end)

    it("should handle overline", function()
      local expected = "\27[53moverline\27[55m"
      local actual = cecho2ansi("<o>overline</o>")
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of ansi2string", function()
    it("should return the string fed into it with ansi codes removed", function()
      local original = '\27[38;5;179;48;5;230mYou say in a baritone voice, "Test."\27[0;37;40m'
      local expected = 'You say in a baritone voice, "Test."'
      local actual = ansi2string(original)
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of setHexFgColor", function()

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

  describe("Tests the functionality of setHexBgColor", function()

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

  describe("Tests the functionality of _Echoes.Process", function()
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

      assert.are.same(
        _Echos.Process('#sStrikethrough#/s', 'Hex'),
        { "", "\27strikethrough", "Strikethrough", "\27strikethroughoff", "" }
      )

      assert.are.same(
        _Echos.Process('#oOverline#/o', 'Hex'),
        { "", "\27overline", "Overline", "\27overlineoff", "" }
      )

      assert.are.same(
        _Echos.Process('\\#ff0000Escaped', 'Hex'),
        { "#ff0000", "Escaped" }
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

      assert.are.same(
        _Echos.Process('<s>Strikethrough</s>', 'Decimal'),
        { "", "\27strikethrough", "Strikethrough", "\27strikethroughoff", "" }
      )

      assert.are.same(
        _Echos.Process('<o>Overline</o>', 'Decimal'),
        { "", "\27overline", "Overline", "\27overlineoff", "" }
      )

      assert.are.same(
        _Echos.Process('<:0,0,255>OnBlue', 'Decimal'),
        { "", { bg = { "0", "0", "255", 255 } }, "OnBlue" }
      )

      assert.are.same(
        _Echos.Process('<1234,0,0>NotAColour', 'Decimal'),
        { "", "<1234,0,0>", "NotAColour" }
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

      assert.are.same(
        _Echos.Process('<s>Strikethrough</s>', 'Color'),
        { "", "\27strikethrough", "Strikethrough", "\27strikethroughoff", "" }
      )

      assert.are.same(
        _Echos.Process('<o>Overline</o>', 'Color'),
        { "", "\27overline", "Overline", "\27overlineoff", "" }
      )

      assert.are.same(
        _Echos.Process('<red:blue>RedOnBlue', 'Color'),
        { "", { fg = { 255, 0, 0 }, bg = { 0, 0, 255 } }, "RedOnBlue" }
      )

      assert.are.same(
        _Echos.Process('<:blue>OnBlue', 'Color'),
        { "", { bg = { 0, 0, 255 } }, "OnBlue" }
      )
    end)
  end)

  describe("Tests the functionality of cecho2string", function()
    it("Should be able to handle stripping colors", function()
      local testCases = {
        {"<red>This is<blue> a simple test", "This is a simple test"},
        {"<purple>This<reset> is a <more> complicated test", "This is a <more> complicated test"},
        {"This <ansiBlack>should also be easy", "This should also be easy"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = cecho2string(case[1])
        assert.equals(expected, actual)
      end
    end)

    it("Should be able to strip formatting codes as well", function()
      local testCases = {
        {"<b>Bold</b>", "Bold"},
        {"<u>Underline</u>", "Underline"},
        {"<i>Italics</i>", "Italics"},
        {"<s>Strikethrough</s>", "Strikethrough"},
        {"<o>Overline</o>", "Overline"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = cecho2string(case[1])
        assert.equals(expected, actual)
      end
    end)
  end)

  describe("Tests the functionality of decho2string", function()
    it("Should be able to handle stripping colors", function()
      local testCases = {
        {"<255,0,0>This is<0,255,0> a simple test", "This is a simple test"},
        {"<128,128,0>This<r> is a <more> complicated test", "This is a <more> complicated test"},
        {"This <0,0,0>should also be easy", "This should also be easy"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = decho2string(case[1])
        assert.equals(expected, actual)
      end
    end)

    it("Should be able to strip formatting codes as well", function()
      local testCases = {
        {"<b>Bold</b>", "Bold"},
        {"<u>Underline</u>", "Underline"},
        {"<i>Italics</i>", "Italics"},
        {"<s>Strikethrough</s>", "Strikethrough"},
        {"<o>Overline</o>", "Overline"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = decho2string(case[1])
        assert.equals(expected, actual)
      end
    end)
  end)

  describe("Tests the functionality of hecho2string", function()
    it("Should be able to handle stripping colors", function()
      local testCases = {
        {"#ff0000This is#00ff00 a simple test", "This is a simple test"},
        {"#777700This#r is a #more complicated test", "This is a #more complicated test"},
        {"This |c000000should also be easy", "This should also be easy"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = hecho2string(case[1])
        assert.equals(expected, actual)
      end
    end)

    it("Should be able to strip formatting codes as well", function()
      local testCases = {
        {"#bBold#/b", "Bold"},
        {"#uUnderline#/u", "Underline"},
        {"#iItalics#/i", "Italics"},
        {"#sStrikethrough#/s", "Strikethrough"},
        {"#oOverline#/o", "Overline"}
      }
      for _, case in ipairs(testCases) do
        local expected = case[2]
        local actual = hecho2string(case[1])
        assert.equals(expected, actual)
      end
    end)
  end)

  describe("Test functionality of buffers", function()

    before_each(function()
      createBuffer("mybuffer")
      -- clear the buffer in case it already exists
      clearWindow("mybuffer")
      -- clearWindow does not reset the user cursor, so tests that move it
      -- would otherwise leak position into the next test
      moveCursor("mybuffer", 0, 0)
    end)

    it("should append text to the buffer", function()
      echo("mybuffer", "Hello, world!")
      selectCurrentLine("mybuffer")
      assert.are.equal("Hello, world!", getSelection("mybuffer"))
    end)
  
    -- https://github.com/Mudlet/Mudlet/issues/6575
    it("selects the last line after moveCursorEnd in a buffer", function()
      echo("mybuffer", "Line 1\nLine 2\nLine 3")
      moveCursorEnd("mybuffer")
      selectCurrentLine("mybuffer")
      assert.are.equal("Line 3", getSelection("mybuffer"))
    end)
  
    it("should append new text to existing text in the buffer", function()
      echo("mybuffer", "Hello")
      echo("mybuffer", ", world!")
      selectCurrentLine("mybuffer")
      assert.are.equal("Hello, world!", getSelection("mybuffer"))
    end)
  
  end)

  describe("Tests the functionality of getHTMLformat", function()
    local fmt
    before_each(function()
      fmt = {
        background = "rgba(0, 0, 0, 0)",
        bold = false,
        foreground = { 0, 160, 0 },
        italic = false,
        overline = false,
        reverse = false,
        strikeout = false,
        underline = false
      }
    end)

    it("Should return a style with no text modifiers but bg/fg colors if none are in the table", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: normal; font-style: normal; text-decoration: none;">'
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with 'font-weight: bold;' if bold is true", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: bold; font-style: normal; text-decoration: none;">'
      fmt.bold = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with 'font-style: italic' if italic is true", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: normal; font-style: italic; text-decoration: none;">'
      fmt.italic = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with 'text-decoration: underline' if underline is true", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: normal; font-style: normal; text-decoration: underline;">'
      fmt.underline = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with 'text-decoration: overline' if overline is true", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: normal; font-style: normal; text-decoration: overline;">'
      fmt.overline = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with 'text-decoration: line-through' if strikeout is true", function()
      local expected = '<span style="color: rgb(0, 160, 0);background-color: rgba(0, 0, 0, 0); font-weight: normal; font-style: normal; text-decoration: line-through;">'
      fmt.strikeout = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should return a style with no text modifiers and bg/fg colors inverted if reverse is true", function()
      local expected = '<span style="color: rgb(0, 0, 0);background-color: rgba(0, 160, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;">'
      fmt.reverse = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should be able to handle all options at once", function()
      local expected = '<span style="color: rgb(0, 0, 0);background-color: rgba(0, 160, 0, 255); font-weight: bold; font-style: italic; text-decoration: overline underline line-through;">'
      fmt = {
        background = { 0, 0, 0 },
        bold = true,
        foreground = { 0, 160, 0 },
        italic = true,
        overline = true,
        reverse = true,
        strikeout = true,
        underline = true
      }
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should use the foreground for the background and invert that if the background is a gradient", function()
      local expected = '<span style="color: rgb(255, 95, 255);background-color: rgba(0, 160, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;">'
      fmt.background = "QLinearGradient(doesn't matter will be ignored)"
      fmt.reverse = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)

    it("Should extract r,g,b from rgba() backgrounds if reverse is true (rgba doesn't work in color)", function()
      local expected = '<span style="color: rgb(128, 0, 128);background-color: rgba(0, 160, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;">'
      fmt.background = "rgba(128, 0, 128, 255)"
      fmt.reverse = true
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
      fmt.background = "rgba(128, 0, 128, 128)"
      local actual = getHTMLformat(fmt)
      assert.equals(expected, actual)
    end)
  end)

  describe("Tests the functionality of getLabelFormat", function()
    local expected
    local labelName = "gldfTestLabel"
    before_each(function()
      expected = {
        background = "rgba(0, 0, 0, 0)",
        bold = false,
        foreground = { 192, 192, 192 },
        italic = false,
        overline = false,
        reverse = false,
        strikeout = false,
        underline = false
      }
      createLabel(labelName, 0, 0, 0, 0, 0)
      hideWindow(labelName)
    end)

    after_each(function()
      deleteLabel(labelName)
    end)

    it("Should return a default table if no background color or stylesheet is set", function()
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should return the transparent background color for default so the background of the label is seen", function()
      setBackgroundColor(labelName, 128, 0, 128)
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect foreground color from a color directive", function()
      setLabelStyleSheet(labelName, "color: rgb(128, 0, 128);")
      expected.foreground = "rgb(128, 0, 128)"
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect underline from text-decorations directive", function()
      setLabelStyleSheet(labelName, "text-decoration: underline;")
      expected.underline = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect overline from text-decorations directive", function()
      setLabelStyleSheet(labelName, "text-decoration: overline;")
      expected.overline = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect strikeout/line-through/strikethrough from text-decorations directive", function()
      setLabelStyleSheet(labelName, "text-decoration: line-through;")
      expected.strikeout = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect underline, overline, and strikeout if all are present", function()
      setLabelStyleSheet(labelName, "text-decoration: underline overline line-through;")
      expected.underline = true
      expected.overline = true
      expected.strikeout = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect italic from font or font-style tag", function()
      setLabelStyleSheet(labelName, "font-style: italic;")
      expected.italic = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
      setLabelStyleSheet(labelName, "font: italic;")
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect bold from font or font-weight tag", function()
      setLabelStyleSheet(labelName, "font: bold;")
      expected.bold = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
      setLabelStyleSheet(labelName, "font-weight: bold;")
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)

    it("Should detect bold and italic from the font tag at the same time", function()
      setLabelStyleSheet(labelName, "font: bold italic;")
      expected.bold = true
      expected.italic = true
      local actual = getLabelFormat(labelName)
      assert.are.same(expected, actual)
    end)
  end)

  describe("Tests the error handling of setLabelStyleSheet", function()
    local testLabel = "setLabelStyleSheetTestLabel"

    after_each(function()
      pcall(deleteLabel, testLabel)
    end)

    it("Should return nil + error when label doesn't exist", function()
      local nonExistentLabel = "thisLabelDoesNotExist"
      local ok, err = setLabelStyleSheet(nonExistentLabel, "color: red;")
      assert.is_nil(ok)
      assert.matches("label name '.*' not found", err)
    end)

    it("Should return nil + error when label name is empty", function()
      local ok, err = setLabelStyleSheet("", "color: red;")
      assert.is_nil(ok)
      assert.matches("cannot have an empty string", err)
    end)

    it("Should return true when successfully setting stylesheet on existing label", function()
      createLabel(testLabel, 10, 10, 100, 50, 1)
      local ok = setLabelStyleSheet(testLabel, "color: blue;")
      assert.is_true(ok)
    end)
  end)

  describe("Tests the functionality of replace", function()
    it("Should return nil+msg if nothing is selected to replace", function()
      deselect()
      local ok,err = replace("]")
      assert.is_nil(ok)
      assert.equals("replace: nothing is selected to be replaced. Did selectString return -1?", err)
    end)
  end)
  describe("Tests the functionality of the color echo transformation functions", function()
    local cechoString = "<reset><ansi_light_red:ansi_010>This <b>is</b> <i>a</i> <:ansi_012><u>test</u> <ansi_010><s>of</s> <o>the</o><reset> echo transformations."
    local dechoString = "<r><255,0,0:0,255,0>This <b>is</b> <i>a</i> <:0,0,255><u>test</u> <0,255,0><s>of</s> <o>the</o><r> echo transformations."
    local hechoString = "#r#ff0000,00ff00This #bis#/b #ia#/i #,0000ff#utest#/u #00ff00#sof#/s #othe#/o#r echo transformations."
    local htmlString = [[<span style="color: rgb(255, 255, 255);background-color: rgba(0, 0, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;"><span style="color: rgb(255, 255, 255);background-color: rgba(0, 0, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;"><span style="color: rgb(255, 0, 0);background-color: rgba(0, 255, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;">This <span style="color: rgb(255, 0, 0);background-color: rgba(0, 255, 0, 255); font-weight: bold; font-style: normal; text-decoration: none;">is<span style="color: rgb(255, 0, 0);background-color: rgba(0, 255, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;"> <span style="color: rgb(255, 0, 0);background-color: rgba(0, 255, 0, 255); font-weight: normal; font-style: italic; text-decoration: none;">a<span style="color: rgb(255, 0, 0);background-color: rgba(0, 255, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;"> <span style="color: rgb(255, 0, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: none;"><span style="color: rgb(255, 0, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: underline;">test<span style="color: rgb(255, 0, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: none;"> <span style="color: rgb(0, 255, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: none;"><span style="color: rgb(0, 255, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: line-through;">of<span style="color: rgb(0, 255, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: none;"> <span style="color: rgb(0, 255, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: overline;">the<span style="color: rgb(0, 255, 0);background-color: rgba(0, 0, 255, 255); font-weight: normal; font-style: normal; text-decoration: none;"><span style="color: rgb(255, 255, 255);background-color: rgba(0, 0, 0, 255); font-weight: normal; font-style: normal; text-decoration: none;"> echo transformations.]]
    describe("Tests the functionality of cecho2decho", function()
      it('can successfully convert a cecho string to a decho one', function()
        local expected = dechoString
        local actual = cecho2decho(cechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of cecho2hecho", function()
      it('can successfully convert a cecho string to an hecho one', function()
        local expected = hechoString
        local actual = cecho2hecho(cechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of cecho2html", function()
      it('can successfully convert a cecho string to an html one', function()
        local expected = htmlString
        local actual = cecho2html(cechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of decho2cecho", function()
      it('can successfully convert a decho string to a cecho one', function()
        local expected = cechoString
        local actual = decho2cecho(dechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of decho2hecho", function()
      it('can successfully convert a decho string to an hecho one', function()
        local expected = hechoString
        local actual = decho2hecho(dechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of decho2html", function()
      it('can successfully convert a decho string to an html one', function()
        local expected = htmlString
        local actual = decho2html(dechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of hecho2cecho", function()
      it('can successfully convert an hecho string to a cecho one', function()
        local expected = cechoString
        local actual = hecho2cecho(hechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of hecho2decho", function()
      it('can successfully convert an hecho string to a decho one', function()
        local expected = dechoString
        local actual = hecho2decho(hechoString)
        assert.equal(expected, actual)
      end)
    end)

    describe("Tests the functionality of hecho2html", function()
      it('can successfully convert an hecho string to an html one', function()
        local expected = htmlString
        local actual = hecho2html(hechoString)
        assert.equal(expected, actual)
      end)
    end)
  end)

  describe("Tests the functionality of selectAll", function()

    before_each(function()
      clearWindow()
    end)

    it("Should error when first argument is not a string", function()
      assert.has_error(function()
        selectAll(123, function() end)
      end)
    end)

    it("Should error when second argument is not a function", function()
      assert.has_error(function()
        selectAll("test", "not a function")
      end)
    end)

    it("Should not call the function if there are no matches", function()
      echo("no match for this")
      moveCursorEnd()
      selectCurrentLine()
      local callCount = 0
      selectAll("zzzzz", function() callCount = callCount + 1 end)
      assert.equals(0, callCount)
    end)

    it("Should call the function once for a single match on the current line", function()
      echo("hello world")
      moveCursorEnd()
      selectCurrentLine()
      local funcCalls = 0
      selectAll("hello", function() funcCalls = funcCalls + 1 end)
      assert.equals(1, funcCalls)
    end)

    it("Should call the function for each match on the current line", function()
      echo("cat dog cat dog cat")
      moveCursorEnd()
      selectCurrentLine()
      local funcCalls = 0
      selectAll("cat", function() funcCalls = funcCalls + 1 end)
      assert.equals(3, funcCalls)
    end)
  end)

  describe("Tests the functionality of selectAll with window name", function()

    it("Should call the function for each match in the window", function()
      local windowName = "selectAllTestBuffer"
      createBuffer(windowName)
      clearWindow(windowName)
      echo(windowName, "cat dog cat dog cat")
      selectCurrentLine(windowName)
      local funcCalls = 0
      selectAll(windowName, "cat", function() funcCalls = funcCalls + 1 end)
      assert.equals(3, funcCalls)
    end)
  end)

  describe("Tests the functionality of PadHexNum", function()
    it("Should zero-pad a single hex digit below ten", function()
      assert.equals("00", PadHexNum("0"))
      assert.equals("05", PadHexNum("5"))
      assert.equals("09", PadHexNum("9"))
    end)

    it("Should leave an already two digit number alone", function()
      assert.equals("FF", PadHexNum("FF"))
      assert.equals("0A", PadHexNum("0A"))
      assert.equals("10", PadHexNum("10"))
      -- "00" is worth its own assertion: its value is below sixteen, so a pad
      -- driven by value rather than by width grows it to three digits
      assert.equals("00", PadHexNum("00"))
    end)

    it("Should error when not given a string", function()
      assert.has_error(function() PadHexNum(15) end)
    end)

    it("Should error when the string is not a hex number", function()
      -- the message matters: the old code reached the same outcome by accident,
      -- comparing a nil tonumber() result against a number
      assert.has_error(function() PadHexNum("zz") end,
        'PadHexNum: bad argument #1 value (hex number as string expected, got "zz"!)')
      assert.has_error(function() PadHexNum("") end,
        'PadHexNum: bad argument #1 value (hex number as string expected, got ""!)')
    end)

    it("Should zero-pad single hex digits above nine as well", function()
      assert.equals("0A", PadHexNum("A"))
      assert.equals("0B", PadHexNum("B"))
      assert.equals("0F", PadHexNum("F"))
    end)

    it("Should pad every single digit to the same width", function()
      for value = 0, 15 do
        local padded = PadHexNum(string.format("%X", value))
        assert.equals(2, #padded)
        assert.equals(value, tonumber(padded, 16))
      end
    end)
  end)

  describe("Tests the functionality of RGB2Hex", function()
    it("Should convert an r, g, b triple to a six digit hex string", function()
      assert.equals("FFFFFF", RGB2Hex(255, 255, 255))
      assert.equals("000000", RGB2Hex(0, 0, 0))
      assert.equals("80C020", RGB2Hex(128, 192, 32))
    end)

    it("Should accept a colour name in place of the triple", function()
      assert.equals("FFFFFF", RGB2Hex("white"))
      assert.equals("000000", RGB2Hex("black"))
      assert.equals(RGB2Hex(getRGB("blue")), RGB2Hex("blue"))
    end)

    it("Should error when given no arguments at all", function()
      assert.has_error(function() RGB2Hex() end)
    end)

    it("Should produce six hex digits for every component below sixteen", function()
      assert.equals("0A0B0C", RGB2Hex(10, 11, 12))
      assert.equals("0A0A0A", RGB2Hex(10, 10, 10))
    end)

    it("Should encode a small component as its own value, not a shifted one", function()
      -- the damaging case: a well formed six digit string that names the wrong
      -- colour, so nothing downstream can notice. 11 must not become 0xB0 (176)
      assert.equals("C80B0C", RGB2Hex(200, 11, 12))
      assert.equals("FF0000", RGB2Hex(255, 0, 0))
    end)

    -- in 0-255 only: RGB2Hex range-checks nothing, so an out of range component
    -- still produces a longer string. That is a separate defect from the padding
    it("Should return six hex digits for every component value in 0-255", function()
      for _, component in ipairs({0, 1, 9, 10, 15, 16, 17, 128, 255}) do
        local hex = RGB2Hex(component, component, component)
        assert.equals(6, #hex)
        for position = 1, 5, 2 do
          assert.equals(component, tonumber(hex:sub(position, position + 1), 16))
        end
      end
    end)
  end)

  describe("Tests the functionality of getRGB", function()
    it("Should return the three components of a named colour", function()
      local r, g, b = getRGB("red")
      assert.are.same({255, 0, 0}, {r, g, b})
      assert.are.same(color_table["green"], {getRGB("green")})
    end)

    it("Should honour a colour the user has redefined", function()
      local original = color_table["ansi_000"]
      color_table["ansi_000"] = {1, 2, 3}
      local r, g, b = getRGB("ansi_000")
      color_table["ansi_000"] = original
      assert.are.same({1, 2, 3}, {r, g, b})
    end)

    it("Should error when not given a string", function()
      assert.has_error(function() getRGB(42) end)
    end)

    it("Should error for a colour name that does not exist", function()
      assert.has_error(function() getRGB("definitelyNotAColour") end)
    end)
  end)

  describe("Tests the functionality of unpack_w_nil", function()
    it("Should return every value up to n, including embedded nils", function()
      local packed = {1, nil, 3, n = 3}
      local a, b, c = unpack_w_nil(packed)
      assert.are.same({1, nil, 3}, {a, b, c})
      assert.is_nil(b)
    end)

    it("Should start at the counter it is given", function()
      local packed = {"a", "b", "c", n = 3}
      assert.are.same({"b", "c"}, {unpack_w_nil(packed, 2)})
    end)

    it("Should return a trailing nil rather than stopping short of n", function()
      local packed = {"only", nil, n = 2}
      -- a plain assignment cannot tell "returned nil" from "returned nothing",
      -- so count the results
      assert.equals(2, select("#", unpack_w_nil(packed)))
      local first, second = unpack_w_nil(packed)
      assert.equals("only", first)
      assert.is_nil(second)
    end)
  end)

  describe("Tests the functionality of the custom gauge family", function()
    local gaugeName = "guiUtilsTestGauge"

    local function geometry(name)
      local x, y, width, height = getWindowGeometry(name)
      return {x = x, y = y, width = width, height = height}
    end

    before_each(function()
      createGauge("main", gaugeName, 300, 20, 30, 300, "start", 0, 255, 0, "horizontal")
    end)

    after_each(function()
      for _, suffixName in ipairs({"_back", "_front", "_text"}) do
        pcall(deleteLabel, gaugeName .. suffixName)
      end
      gaugesTable[gaugeName] = nil
    end)

    describe("Tests the functionality of createGauge", function()
      it("Should create the back, front and text labels at the requested geometry", function()
        for _, suffixName in ipairs({"_back", "_front", "_text"}) do
          assert.equals("label", windowType(gaugeName .. suffixName))
        end
        assert.are.same({x = 30, y = 300, width = 300, height = 20}, geometry(gaugeName .. "_back"))
        assert.are.same({x = 30, y = 300, width = 300, height = 20}, geometry(gaugeName .. "_text"))
        -- a fresh gauge is full, so the front label covers the whole back one
        assert.are.same({x = 30, y = 300, width = 300, height = 20}, geometry(gaugeName .. "_front"))
      end)

      it("Should record the gauge in gaugesTable and show it", function()
        local info = gaugesTable[gaugeName]
        assert.equals(300, info.width)
        assert.equals(20, info.height)
        assert.equals(30, info.x)
        assert.equals(300, info.y)
        assert.equals("horizontal", info.orientation)
        assert.equals(1, info.value)
        assert.is_true(windowVisible(gaugeName .. "_back"))
        assert.is_true(windowVisible(gaugeName .. "_front"))
      end)

      it("Should accept a colour name in place of the r, g, b triple", function()
        finally(function()
          for _, suffixName in ipairs({"_back", "_front", "_text"}) do
            pcall(deleteLabel, "colourNameGauge" .. suffixName)
          end
          gaugesTable.colourNameGauge = nil
        end)
        createGauge("colourNameGauge", 100, 10, 0, 0, nil, "green")
        assert.are.same({0, 255, 0}, {gaugesTable.colourNameGauge.r, gaugesTable.colourNameGauge.g, gaugesTable.colourNameGauge.b})
        assert.equals("horizontal", gaugesTable.colourNameGauge.orientation)
      end)

      it("Should reject an unknown orientation", function()
        assert.has_error(function()
          createGauge("main", "badOrientationGauge", 10, 10, 0, 0, "", 0, 0, 0, "sideways")
        end)
      end)
    end)

    describe("Tests the functionality of setGauge", function()
      it("Should shrink the front label to the fraction given, horizontally", function()
        setGauge(gaugeName, 50, 100)
        assert.equals(0.5, gaugesTable[gaugeName].value)
        assert.are.same({x = 30, y = 300, width = 150, height = 20}, geometry(gaugeName .. "_front"))
        -- the backdrop keeps its full size
        assert.are.same({x = 30, y = 300, width = 300, height = 20}, geometry(gaugeName .. "_back"))
      end)

      it("Should grow a vertical gauge upwards from its bottom edge", function()
        gaugesTable[gaugeName].orientation = "vertical"
        setGauge(gaugeName, 1, 4)
        assert.are.same({x = 30, y = 315, width = 300, height = 5}, geometry(gaugeName .. "_front"))
      end)

      it("Should shrink a goofy gauge towards its right edge", function()
        gaugesTable[gaugeName].orientation = "goofy"
        setGauge(gaugeName, 1, 4)
        assert.are.same({x = 255, y = 300, width = 75, height = 20}, geometry(gaugeName .. "_front"))
      end)

      it("Should shrink a batty gauge downwards from its top edge", function()
        gaugesTable[gaugeName].orientation = "batty"
        setGauge(gaugeName, 1, 2)
        assert.are.same({x = 30, y = 300, width = 300, height = 10}, geometry(gaugeName .. "_front"))
      end)

      it("Should update the caption when one is passed", function()
        setGauge(gaugeName, 1, 2, "half")
        assert.is_truthy(getLabelText(gaugeName .. "_text"):find("half", 1, true))
      end)

      it("Should let the fill run past the backdrop when the value exceeds the maximum", function()
        setGauge(gaugeName, 3, 2)
        assert.equals(1.5, gaugesTable[gaugeName].value)
        assert.equals(450, select(3, getWindowGeometry(gaugeName .. "_front")))
      end)

      it("Should error for an unknown gauge or a non numeric value", function()
        assert.has_error(function() setGauge("noSuchGauge", 1, 1) end)
        assert.has_error(function() setGauge(gaugeName, "lots", 1) end)
        assert.has_error(function() setGauge(gaugeName, 1, "lots") end)
      end)
    end)

    describe("Tests the functionality of moveGauge", function()
      it("Should move every label of the gauge and remember the new position", function()
        moveGauge(gaugeName, 11, 22)
        assert.are.same({x = 11, y = 22, width = 300, height = 20}, geometry(gaugeName .. "_back"))
        assert.are.same({x = 11, y = 22, width = 300, height = 20}, geometry(gaugeName .. "_text"))
        assert.are.same({x = 11, y = 22, width = 300, height = 20}, geometry(gaugeName .. "_front"))
        assert.equals(11, gaugesTable[gaugeName].x)
        assert.equals(22, gaugesTable[gaugeName].y)
      end)

      it("Should keep the current fill when it moves", function()
        setGauge(gaugeName, 1, 4)
        moveGauge(gaugeName, 5, 6)
        assert.are.same({x = 5, y = 6, width = 75, height = 20}, geometry(gaugeName .. "_front"))
      end)

      it("Should error for an unknown gauge or non numeric coordinates", function()
        assert.has_error(function() moveGauge("noSuchGauge", 1, 1) end)
        assert.has_error(function() moveGauge(gaugeName, "1", 1) end)
        assert.has_error(function() moveGauge(gaugeName, 1, "1") end)
      end)
    end)

    describe("Tests the functionality of resizeGauge", function()
      it("Should resize every label of the gauge and remember the new size", function()
        resizeGauge(gaugeName, 120, 40)
        assert.are.same({x = 30, y = 300, width = 120, height = 40}, geometry(gaugeName .. "_back"))
        assert.are.same({x = 30, y = 300, width = 120, height = 40}, geometry(gaugeName .. "_text"))
        assert.are.same({x = 30, y = 300, width = 120, height = 40}, geometry(gaugeName .. "_front"))
        assert.equals(120, gaugesTable[gaugeName].width)
        assert.equals(40, gaugesTable[gaugeName].height)
      end)

      it("Should rescale the fill to the new width", function()
        setGauge(gaugeName, 1, 2)
        resizeGauge(gaugeName, 200, 20)
        assert.equals(100, select(3, getWindowGeometry(gaugeName .. "_front")))
      end)

      it("Should error for an unknown gauge or non numeric sizes", function()
        assert.has_error(function() resizeGauge("noSuchGauge", 1, 1) end)
        assert.has_error(function() resizeGauge(gaugeName, "1", 1) end)
        assert.has_error(function() resizeGauge(gaugeName, 1, "1") end)
      end)
    end)

    describe("Tests the functionality of hideGauge and showGauge", function()
      it("Should hide and show all three labels", function()
        hideGauge(gaugeName)
        assert.is_false(windowVisible(gaugeName .. "_back"))
        assert.is_false(windowVisible(gaugeName .. "_front"))
        assert.is_false(windowVisible(gaugeName .. "_text"))
        showGauge(gaugeName)
        assert.is_true(windowVisible(gaugeName .. "_back"))
        assert.is_true(windowVisible(gaugeName .. "_front"))
        assert.is_true(windowVisible(gaugeName .. "_text"))
      end)

      it("Should error for an unknown gauge", function()
        assert.has_error(function() hideGauge("noSuchGauge") end)
        assert.has_error(function() showGauge("noSuchGauge") end)
      end)
    end)

    describe("Tests the functionality of setGaugeText", function()
      it("Should wrap the text in a font tag coloured black by default", function()
        setGaugeText(gaugeName, "HP: 100%")
        assert.equals([[<font color="#000000">HP: 100%</font>]], gaugesTable[gaugeName].text)
        assert.is_truthy(getLabelText(gaugeName .. "_text"):find("HP: 100%", 1, true))
      end)

      it("Should accept a colour name", function()
        setGaugeText(gaugeName, "hurt", "red")
        assert.equals([[<font color="#FF0000">hurt</font>]], gaugesTable[gaugeName].text)
      end)

      it("Should accept an r, g, b triple", function()
        setGaugeText(gaugeName, "hurt", 0, 128, 255)
        assert.equals([[<font color="#0080FF">hurt</font>]], gaugesTable[gaugeName].text)
      end)

      it("Should emit a six digit colour for components below sixteen", function()
        setGaugeText(gaugeName, "dim", 10, 11, 12)
        assert.equals([[<font color="#0A0B0C">dim</font>]], gaugesTable[gaugeName].text)
      end)

      it("Should clear the caption when no text is given", function()
        setGaugeText(gaugeName, "something")
        setGaugeText(gaugeName)
        assert.equals([[<font color="#000000"></font>]], gaugesTable[gaugeName].text)
      end)

      it("Should error for an unknown gauge", function()
        assert.has_error(function() setGaugeText("noSuchGauge", "x") end)
      end)
    end)

    describe("Tests the functionality of setGaugeStyleSheet", function()
      it("Should apply the stylesheet to the front label and default the others", function()
        setGaugeStyleSheet(gaugeName, "background-color: blue;")
        assert.equals("background-color: blue;", getLabelStyleSheet(gaugeName .. "_front"))
        assert.equals("background-color: blue;", getLabelStyleSheet(gaugeName .. "_back"))
        assert.equals("", getLabelStyleSheet(gaugeName .. "_text"))
      end)

      it("Should use the separate back and text stylesheets when given", function()
        setGaugeStyleSheet(gaugeName, "border: 1px;", "background-color: grey;", "color: white;")
        assert.equals("border: 1px;", getLabelStyleSheet(gaugeName .. "_front"))
        assert.equals("background-color: grey;", getLabelStyleSheet(gaugeName .. "_back"))
        assert.equals("color: white;", getLabelStyleSheet(gaugeName .. "_text"))
      end)

      it("Should error for an unknown gauge or a non string stylesheet", function()
        assert.has_error(function() setGaugeStyleSheet("noSuchGauge", "a") end)
        assert.has_error(function() setGaugeStyleSheet(gaugeName, 5) end)
      end)
    end)

    describe("Tests the functionality of the gauge tooltip and clickthrough helpers", function()
      -- neither a label tooltip nor the clickthrough flag has a getter, so the
      -- observable part is which of the gauge's three labels each helper
      -- reaches; spy.on keeps the real function underneath
      it("Should put the tooltip on the text label and clear it again", function()
        local toolTip = spy.on(_G, "setLabelToolTip")
        finally(function() toolTip:revert() end)
        setGaugeToolTip(gaugeName, "some hint", 3)
        assert.spy(toolTip).was.called_with(gaugeName .. "_text", "some hint", 3)
        resetGaugeToolTip(gaugeName)
        assert.spy(toolTip).was.called_with(gaugeName .. "_text", "")
      end)

      it("Should enable and disable clickthrough on all three labels", function()
        local enable = spy.on(_G, "enableClickthrough")
        finally(function() enable:revert() end)
        enableGaugeClickthrough(gaugeName)
        assert.spy(enable).was.called(3)
        for _, suffixName in ipairs({"_back", "_front", "_text"}) do
          assert.spy(enable).was.called_with(gaugeName .. suffixName)
        end

        local disable = spy.on(_G, "disableClickthrough")
        finally(function() disable:revert() end)
        disableGaugeClickthrough(gaugeName)
        assert.spy(disable).was.called(3)
        for _, suffixName in ipairs({"_back", "_front", "_text"}) do
          assert.spy(disable).was.called_with(gaugeName .. suffixName)
        end
      end)

      it("Should error for an unknown gauge", function()
        assert.has_error(function() setGaugeToolTip("noSuchGauge", "hint") end)
        assert.has_error(function() resetGaugeToolTip("noSuchGauge") end)
        assert.has_error(function() enableGaugeClickthrough("noSuchGauge") end)
        assert.has_error(function() disableGaugeClickthrough("noSuchGauge") end)
      end)
    end)

    describe("Tests the functionality of setGaugeWindow", function()
      local userWindow = "guiUtilsGaugeUserWindow"

      setup(function()
        openUserWindow(userWindow)
      end)

      teardown(function()
        closeUserWindow(userWindow)
      end)

      it("Should reparent every label of the gauge and record the new position", function()
        -- getWindowGeometry is parent relative, so it cannot tell a reparent
        -- from a plain move; setWindow is where the reparenting happens
        local setWindowSpy = spy.on(_G, "setWindow")
        finally(function() setWindowSpy:revert() end)
        setGaugeWindow(userWindow, gaugeName, 7, 8)
        assert.spy(setWindowSpy).was.called(3)
        for _, suffixName in ipairs({"_back", "_front", "_text"}) do
          assert.spy(setWindowSpy).was.called_with(userWindow, gaugeName .. suffixName, 7, 8, true)
        end
        assert.equals(7, gaugesTable[gaugeName].x)
        assert.equals(8, gaugesTable[gaugeName].y)
        assert.are.same({x = 7, y = 8, width = 300, height = 20}, geometry(gaugeName .. "_back"))
        assert.are.same({x = 7, y = 8, width = 300, height = 20}, geometry(gaugeName .. "_front"))
      end)

      it("Should error for an unknown gauge", function()
        assert.has_error(function() setGaugeWindow(userWindow, "noSuchGauge") end)
      end)

      it("Should keep the gauge hidden when show is passed as false", function()
        setGaugeWindow(userWindow, gaugeName, 0, 0, false)
        assert.is_false(windowVisible(gaugeName .. "_back"))
        assert.is_false(windowVisible(gaugeName .. "_front"))
        assert.is_false(windowVisible(gaugeName .. "_text"))
      end)

      it("Should still show the gauge when show is left out", function()
        hideGauge(gaugeName)
        setGaugeWindow(userWindow, gaugeName, 0, 0)
        assert.is_true(windowVisible(gaugeName .. "_back"))
      end)
    end)
  end)

  describe("Tests the functionality of createConsole", function()
    local consoleName = "guiUtilsTestConsole"

    after_each(function()
      deleteMiniConsole(consoleName)
    end)

    it("Should create a miniconsole wrapped to the requested number of characters", function()
      createConsole("main", consoleName, 8, 40, 10, 200, 400)
      assert.equals("miniconsole", windowType(consoleName))
      assert.equals(40, getWindowWrap(consoleName))
      assert.equals(8, getFontSize(consoleName))
    end)

    it("Should size the console from the font metrics and place it where asked", function()
      createConsole("main", consoleName, 8, 40, 10, 200, 400)
      local charWidth, charHeight = calcFontSize(8)
      local x, y, width, height = getWindowGeometry(consoleName)
      assert.are.same({200, 400}, {x, y})
      assert.are.same({charWidth * 40, charHeight * 10}, {width, height})
    end)

    it("Should start out with a white foreground on a transparent background", function()
      createConsole("main", consoleName, 8, 40, 10, 0, 0)
      echo(consoleName, "default colours\n")
      selectString(consoleName, "default colours", 1)
      assert.are.same({255, 255, 255}, {getFgColor(consoleName)})
    end)

    it("Should default the window name to main when it is left out", function()
      createConsole(consoleName, 8, 40, 10, 5, 6)
      assert.equals("miniconsole", windowType(consoleName))
      local x, y = getWindowGeometry(consoleName)
      assert.are.same({5, 6}, {x, y})
    end)

    it("Should error when a size argument is not a number", function()
      assert.has_error(function() createConsole("main", consoleName, "8", 40, 10, 0, 0) end)
      assert.has_error(function() createConsole("main", consoleName, 8, 40, 10, 0, "0") end)
    end)
  end)

  describe("Tests the functionality of bg and fg", function()
    local windowName = "guiUtilsColourBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      resetFormat(windowName)
    end)

    -- getBgColor/getFgColor report the colour of the character the selection
    -- starts on, so the colour has to be laid down on real text to read it back
    it("Should set the background colour of a named window from a colour name", function()
      bg(windowName, "blue")
      echo(windowName, "coloured\n")
      selectString(windowName, "coloured", 1)
      assert.are.same(color_table["blue"], {getBgColor(windowName)})
    end)

    it("Should set the foreground colour of a named window from a colour name", function()
      fg(windowName, "red")
      echo(windowName, "coloured\n")
      selectString(windowName, "coloured", 1)
      assert.are.same(color_table["red"], {getFgColor(windowName)})
    end)

    it("Should colour the main console when given only a colour name", function()
      finally(function() resetFormat() end)
      clearWindow()
      bg("green")
      fg("yellow")
      echo("mainColouredSample\n")
      selectString("mainColouredSample", 1)
      assert.are.same(color_table["green"], {getBgColor("main")})
      assert.are.same(color_table["yellow"], {getFgColor("main")})
    end)

    it("Should error for a colour that does not exist", function()
      assert.error_matches(function() bg("notAColour") end, "doesn't exist")
      assert.error_matches(function() fg("notAColour") end, "doesn't exist")
    end)

    it("Should error when given nothing at all", function()
      assert.has_error(function() bg() end)
      assert.has_error(function() fg() end)
    end)
  end)

  describe("Tests the functionality of gagLine", function()
    it("Should delete the line the cursor is on", function()
      -- gagLine is deprecated and forwards to deleteLine with no arguments, so
      -- it always acts on the main console: prove the forwarding, then that a
      -- gagged line really leaves the buffer
      local deleteLineSpy = spy.on(_G, "deleteLine")
      finally(function() deleteLineSpy:revert() end)
      clearWindow()
      echo("keep me\ngag me\n")
      moveCursor(0, 1)
      gagLine()
      assert.spy(deleteLineSpy).was.called(1)
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_falsy(text:find("gag me", 1, true))
      assert.is_truthy(text:find("keep me", 1, true))
    end)
  end)

  describe("Tests the functionality of replaceLine", function()
    local windowName = "guiUtilsReplaceLineBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
    end)

    it("Should replace the whole current line of a named window", function()
      echo(windowName, "the original line\n")
      moveCursor(windowName, 0, 0)
      replaceLine(windowName, "a brand new line")
      moveCursor(windowName, 0, 0)
      selectCurrentLine(windowName)
      assert.equals("a brand new line", getSelection(windowName))
    end)

    it("Should replace the current line of the main console when given only text", function()
      clearWindow()
      echo("the original main line\n")
      moveCursor(0, 0)
      replaceLine("a brand new main line")
      moveCursor(0, 0)
      selectCurrentLine()
      assert.equals("a brand new main line", getSelection())
    end)

    it("Should error when the window name is not a string", function()
      assert.has_error(function() replaceLine(5, "x") end)
    end)
  end)

  describe("Tests the functionality of handleWindowResizeEvent", function()
    it("Should exist as a do nothing default users can override", function()
      assert.equals("function", type(handleWindowResizeEvent))
      assert.are.same({}, {handleWindowResizeEvent()})
    end)
  end)

  describe("Tests the functionality of replaceWildcard", function()
    local fired

    before_each(function()
      fired = nil
    end)

    it("Should replace the text a capture group matched", function()
      local id = tempRegexTrigger("^You wave (goodbye)\\.$", function()
        replaceWildcard(2, "hello")
        selectCurrentLine()
        fired = getSelection()
      end)
      feedTriggers("You wave goodbye.\n")
      killTrigger(id)
      assert.equals("You wave hello.", fired)
    end)

    it("Should do nothing when either argument is missing", function()
      local id = tempRegexTrigger("^You nod (once)\\.$", function()
        replaceWildcard(2)
        replaceWildcard(nil, "hello")
        selectCurrentLine()
        fired = getSelection()
      end)
      feedTriggers("You nod once.\n")
      killTrigger(id)
      assert.equals("You nod once.", fired)
    end)
  end)

  describe("Tests the functionality of showColors", function()
    -- showColors writes a clickable swatch per colour to the main console; the
    -- text of those swatches is what can be read back
    local function mainConsoleText()
      return getLines("main", 0, getLastLineNumber("main") + 1)
    end

    before_each(function()
      clearWindow()
    end)

    it("Should list only the colours matching the search string", function()
      showColors(1, "cornflower")
      local text = table.concat(mainConsoleText(), "\n")
      assert.is_truthy(text:find("cornflower_blue", 1, true))
      assert.is_truthy(text:find("CornflowerBlue", 1, true))
      assert.is_falsy(text:find("firebrick", 1, true))
    end)

    it("Should never list the ansi_### colours", function()
      showColors(1, "ansi_128")
      local text = table.concat(mainConsoleText(), "\n")
      assert.is_falsy(text:find("ansi_128", 1, true))
    end)

    it("Should honour the requested number of columns", function()
      local function lineHolding(needle)
        for index, line in ipairs(mainConsoleText()) do
          if line:find(needle, 1, true) then
            return index
          end
        end
      end

      showColors(2, "cornflower")
      local shared = lineHolding("cornflower_blue")
      assert.is_truthy(shared, "showColors should have listed the matching colours")
      assert.are.equal(shared, lineHolding("CornflowerBlue"), "two colours should share a line when asked for 2 columns")

      clearWindow()
      showColors(1, "cornflower")
      local first = lineHolding("cornflower_blue")
      assert.is_truthy(first)
      assert.are_not.equal(first, lineHolding("CornflowerBlue"), "one column per line means one colour per line")
    end)
  end)

  describe("Tests the functionality of showAnsiColors", function()
    it("Should list the ansi_### colours and nothing else", function()
      clearWindow()
      showAnsiColors(1)
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_truthy(text:find("ansi_000", 1, true))
      assert.is_truthy(text:find("ansi_255", 1, true))
      assert.is_falsy(text:find("cornflower_blue", 1, true))
    end)
  end)

  -- the c/d/h echo, insert, link and popup functions are all one-line calls
  -- into xEcho and share its argument handling, so that is specced once here
  -- rather than once per wrapper
  describe("Tests the functionality of xEcho", function()
    local windowName = "guiUtilsXEchoConsole"
    local labelName = "guiUtilsXEchoLabel"

    setup(function()
      createMiniConsole(windowName, 0, 0, 400, 200)
      setWindowWrap(windowName, 60)
      createLabel(labelName, 0, 0, 100, 30, 1)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
      deleteLabel(labelName)
    end)

    before_each(function()
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
    end)

    local function currentLine()
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    it("Should name the wrapper that called it when the text is not a string", function()
      -- the message is built from the style and the function name, so a caller
      -- of cecho is told about cecho rather than about xEcho
      local ok, err = pcall(xEcho, "Color", "echo", 5)
      assert.is_false(ok)
      assert.is_truthy(err:find("cecho: bad argument #1, string expected, got number", 1, true))

      local hexOk, hexErr = pcall(xEcho, "Hex", "insertText", {})
      assert.is_false(hexOk)
      assert.is_truthy(hexErr:find("hinsertText: bad argument #1, string expected, got table", 1, true))
    end)

    it("Should write to the window named in its first argument", function()
      xEcho("Color", "echo", windowName, "<red>xEchoWindow")
      assert.are.equal("xEchoWindow", currentLine())
    end)

    it("Should echo to the main console when main is the window it is given", function()
      clearWindow()
      moveCursor(0, 0)
      xEcho("Color", "echo", "main", "<red>xEchoMainNamed")
      selectCurrentLine()
      assert.are.equal("xEchoMainNamed", getSelection())
    end)

    it("Should fall back to the main console when given only the text", function()
      clearWindow()
      moveCursor(0, 0)
      xEcho("Color", "echo", "<red>xEchoMainOnly")
      selectCurrentLine()
      assert.are.equal("xEchoMainOnly", getSelection())
    end)

    it("Should apply every colour change in the string, not just the first", function()
      xEcho("Decimal", "echo", windowName, "<255,0,0>AA<0,0,255>BB")
      assert.are.equal("AABB", currentLine())
      selectSection(windowName, 0, 2)
      assert.are.same({255, 0, 0}, getTextFormat(windowName).foreground)
      selectSection(windowName, 2, 2)
      assert.are.same({0, 0, 255}, getTextFormat(windowName).foreground)
    end)

    it("Should refuse anything but a plain echo on a label", function()
      local ok, err = xEcho("Color", "echoLink", labelName, "<red>x", "send('x')", "a hint")
      assert.is_nil(ok)
      assert.are.equal("you cannot use echoLink, echoPopup, or insertText with Labels", err)
    end)

    it("Should echo to a label, turning a newline into a line break", function()
      -- a label holds HTML, so the newline must arrive as <br> or the second
      -- line is run together with the first
      xEcho("Color", "echo", labelName, "<red>first\nsecond")
      local html = getLabelText(labelName)
      assert.is_truthy(html:find("first<br>second", 1, true))
      assert.is_nil(html:find("\n", 1, true))
    end)

    it("Should take a link or a popup with no window at all", function()
      -- the documented cechoLink(text, command, hint) form: three arguments and
      -- the first one is the text, not a window name
      clearWindow()
      moveCursor(0, 0)
      xEcho("Color", "echoLink", "<red>xEchoBareLink", "send('x')", "a hint")
      selectCurrentLine()
      assert.are.equal("xEchoBareLink", getSelection())

      clearWindow()
      moveCursor(0, 0)
      xEcho("Color", "echoPopup", "<red>xEchoBarePopup", {"send('x')"}, {"a hint"})
      selectCurrentLine()
      assert.are.equal("xEchoBarePopup", getSelection())
    end)

    it("Should take the fourth argument as a format flag when it is a boolean", function()
      clearWindow()
      moveCursor(0, 0)
      xEcho("Color", "echoLink", "<red>xEchoFormattedLink", "send('x')", "a hint", true)
      selectCurrentLine()
      assert.are.equal("xEchoFormattedLink", getSelection())
    end)

    it("Should insist on a command and a hint for the link variants", function()
      local ok, err = pcall(xEcho, "Color", "echoLink", "text", "send('x')")
      assert.is_false(ok)
      assert.is_truthy(err:find("Insufficient arguments, usage: ([window, ] string, command, hint)", 1, true))

      local improperOk, improperErr = pcall(xEcho, "Color", "echoLink", "text", "send('x')", "a hint", 5)
      assert.is_false(improperOk)
      assert.is_truthy(improperErr:find("Improper arguments, usage: ([window, ] string, command, hint)", 1, true))
    end)

    it("Should insist on commands and hints for the popup variants", function()
      local ok, err = pcall(xEcho, "Color", "echoPopup", "text", {"send('x')"})
      assert.is_false(ok)
      assert.is_truthy(err:find("Insufficient arguments, usage: ([window, ] string, {commands}, {hints})", 1, true))

      local improperOk, improperErr = pcall(xEcho, "Color", "echoPopup", "text", {"send('x')"}, {"a hint"}, 5)
      assert.is_false(improperOk)
      assert.is_truthy(improperErr:find("Improper arguments, usage: ([window, ] string, {commands}, {hints})", 1, true))
    end)
  end)

  describe("Tests the functionality of hinsertText and dinsertText", function()
    local windowName = "guiUtilsInsertConsole"

    setup(function()
      createMiniConsole(windowName, 0, 0, 400, 200)
      setWindowWrap(windowName, 60)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      clearWindow(windowName)
    end)

    local function firstLine()
      moveCursor(windowName, 0, 0)
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    it("Should insert hecho formatted text at the cursor", function()
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      hinsertText(windowName, "#ff0000X")
      assert.equals("AXB", firstLine())
    end)

    it("Should insert decho formatted text at the cursor", function()
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      dinsertText(windowName, "<255,0,0>X")
      assert.equals("AXB", firstLine())
    end)

    it("Should apply the colour it was given to the inserted text", function()
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      dinsertText(windowName, "<0,255,0>X")
      selectSection(windowName, 1, 1)
      assert.are.same({0, 255, 0}, getTextFormat(windowName).foreground)
    end)
  end)

  describe("Tests the functionality of the coloured link and popup echoes", function()
    local windowName = "guiUtilsLinkConsole"

    setup(function()
      createMiniConsole(windowName, 0, 0, 400, 200)
      setWindowWrap(windowName, 60)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
    end)

    local function currentLine()
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    local function firstLine()
      moveCursor(windowName, 0, 0)
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    -- there is no getter for a link's command or hint, so these cover the text
    -- and colour each variant lays down plus the fact that the call succeeds
    it("Should echo links with each of the three colour syntaxes", function()
      cechoLink(windowName, "<red>click me", "send('x')", "a hint", true)
      assert.equals("click me", currentLine())
      selectSection(windowName, 0, 5)
      assert.are.same(color_table["red"], getTextFormat(windowName).foreground)

      clearWindow(windowName)
      dechoLink(windowName, "<0,255,0>green link", "send('x')", "a hint", true)
      assert.equals("green link", currentLine())

      clearWindow(windowName)
      hechoLink(windowName, "#0000ffblue link", "send('x')", "a hint", true)
      assert.equals("blue link", currentLine())
    end)

    it("Should insert links with each of the three colour syntaxes", function()
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      cinsertLink(windowName, "<red>C", "send('x')", "a hint", true)
      assert.equals("ACB", firstLine())

      clearWindow(windowName)
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      dinsertLink(windowName, "<0,255,0>D", "send('x')", "a hint", true)
      assert.equals("ADB", firstLine())

      clearWindow(windowName)
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      hinsertLink(windowName, "#0000ffE", "send('x')", "a hint", true)
      assert.equals("AEB", firstLine())
    end)

    it("Should echo popups with each of the three colour syntaxes", function()
      local commands = {"send('one')", "send('two')"}
      local hints = {"first", "second"}
      cechoPopup(windowName, "<red>menu", commands, hints, true)
      assert.equals("menu", currentLine())

      clearWindow(windowName)
      dechoPopup(windowName, "<0,255,0>dmenu", commands, hints, true)
      assert.equals("dmenu", currentLine())

      clearWindow(windowName)
      hechoPopup(windowName, "#0000ffhmenu", commands, hints, true)
      assert.equals("hmenu", currentLine())
    end)

    it("Should insert popups with each of the three colour syntaxes", function()
      local commands = {"send('one')", "send('two')"}
      local hints = {"first", "second"}
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      cinsertPopup(windowName, "<red>C", commands, hints, true)
      assert.equals("ACB", firstLine())

      clearWindow(windowName)
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      dinsertPopup(windowName, "<0,255,0>D", commands, hints, true)
      assert.equals("ADB", firstLine())

      clearWindow(windowName)
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      hinsertPopup(windowName, "#0000ffE", commands, hints, true)
      assert.equals("AEB", firstLine())
    end)
  end)

  describe("Tests the functionality of cfeedTriggers, dfeedTriggers and hfeedTriggers", function()
    local seen

    before_each(function()
      seen = {}
    end)

    -- each variant has to strip its own colour syntax before feeding, so the
    -- line the trigger sees must be the bare marker and must carry the colour
    local function feedAndInspect(feeder, text, marker)
      local result = {}
      local id = tempTrigger(marker, function()
        selectCurrentLine()
        result.line = getSelection()
        selectString(marker, 1)
        result.foreground = getTextFormat().foreground
        seen[#seen + 1] = marker
      end)
      feeder(text)
      killTrigger(id)
      return result
    end

    it("Should feed cecho coloured text through the trigger engine", function()
      local result = feedAndInspect(cfeedTriggers, "<red>cfeedMarker", "cfeedMarker")
      assert.equals(1, #seen)
      assert.equals("cfeedMarker", result.line)
      -- the text goes out as ANSI, so a cecho colour name arrives as its ANSI
      -- equivalent: "red" is ANSI 1, not the brighter color_table["red"]
      assert.are.same(color_table["ansi_001"], result.foreground)
    end)

    it("Should feed decho coloured text through the trigger engine", function()
      local result = feedAndInspect(dfeedTriggers, "<0,255,0>dfeedMarker", "dfeedMarker")
      assert.equals(1, #seen)
      assert.equals("dfeedMarker", result.line)
      assert.are.same({0, 255, 0}, result.foreground)
    end)

    it("Should feed hecho coloured text through the trigger engine", function()
      local result = feedAndInspect(hfeedTriggers, "#0000ffhfeedMarker", "hfeedMarker")
      assert.equals(1, #seen)
      assert.equals("hfeedMarker", result.line)
      assert.are.same({0, 0, 255}, result.foreground)
    end)

    it("Should error when not given a string", function()
      assert.has_error(function() cfeedTriggers(5) end)
      assert.has_error(function() dfeedTriggers(5) end)
      assert.has_error(function() hfeedTriggers(5) end)
    end)
  end)

  describe("Tests the functionality of prefix and suffix", function()
    local windowName = "guiUtilsAffixBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      echo(windowName, "middle")
      moveCursor(windowName, 0, 0)
    end)

    local function currentLine()
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    it("Should put text at the start of the line", function()
      prefix("[", nil, nil, nil, windowName)
      assert.equals("[middle", currentLine())
    end)

    it("Should put text at the end of the line", function()
      suffix("]", nil, nil, nil, windowName)
      assert.equals("middle]", currentLine())
    end)

    it("Should colour what it adds", function()
      prefix("[", nil, "red", nil, windowName)
      assert.equals("[middle", currentLine())
      selectSection(windowName, 0, 1)
      assert.are.same(color_table["red"], getTextFormat(windowName).foreground)
    end)

    it("Should accept a colour aware echo function to add the text with", function()
      prefix("<red>[", cecho, nil, nil, windowName)
      assert.equals("[middle", currentLine())
      selectSection(windowName, 0, 1)
      assert.are.same(color_table["red"], getTextFormat(windowName).foreground)
    end)

    it("Should error when the text is not a string", function()
      assert.has_error(function() prefix(5) end)
      assert.has_error(function() suffix(5) end)
    end)

    -- A line that has been finished off with a newline is the ordinary trigger
    -- case, and the only one where landing a column short is visible: on the
    -- unfinished line the block above uses, an insert past the last character
    -- is appended either way.
    local function completedLine()
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      echo(windowName, "PROBE has a TARGET word\n")
      moveCursor(windowName, 0, 0)
    end

    it("Should put text after the last character of a completed line", function()
      completedLine()
      suffix(" SUF", nil, nil, nil, windowName)
      assert.equals("PROBE has a TARGET word SUF", currentLine())
    end)

    it("Should put text after the last character of a completed line when colouring it", function()
      completedLine()
      suffix(" SUF", nil, "red", nil, windowName)
      assert.equals("PROBE has a TARGET word SUF", currentLine())
    end)

    it("Should not recolour the current selection when prefixing", function()
      selectSection(windowName, 0, 6)
      prefix("[", nil, "red", nil, windowName)
      -- "middle" now starts one column along, and must have kept its colour
      selectSection(windowName, 1, 6)
      assert.are_not.same(color_table["red"], getTextFormat(windowName).foreground)
    end)

    it("Should not recolour the current selection when suffixing", function()
      selectSection(windowName, 0, 6)
      suffix("]", nil, "red", nil, windowName)
      selectSection(windowName, 0, 6)
      assert.are_not.same(color_table["red"], getTextFormat(windowName).foreground)
    end)

    it("Should not repaint the background of the current selection either", function()
      selectSection(windowName, 0, 6)
      prefix("[", nil, nil, "blue", windowName)
      selectSection(windowName, 1, 6)
      assert.are_not.same(color_table["blue"], getTextFormat(windowName).background)
    end)

    it("Should suffix onto an empty line", function()
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      echo(windowName, "\n")
      moveCursor(windowName, 0, 0)
      suffix("added", nil, nil, nil, windowName)
      assert.equals("added", currentLine())
    end)

    it("Should still colour what it adds when something is selected", function()
      selectSection(windowName, 0, 6)
      prefix("[", nil, "red", nil, windowName)
      selectSection(windowName, 0, 1)
      assert.are.same(color_table["red"], getTextFormat(windowName).foreground)
    end)
  end)

  describe("Tests the functionality of moveCursorDown", function()
    local windowName = "guiUtilsCursorBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      echo(windowName, "one\ntwo\nthree\nfour\n")
      moveCursor(windowName, 0, 0)
    end)

    it("Should move the cursor down one line by default", function()
      moveCursorDown(windowName)
      assert.equals(1, getLineNumber(windowName))
    end)

    it("Should move the cursor down the number of lines given", function()
      moveCursorDown(windowName, 2)
      assert.equals(2, getLineNumber(windowName))
    end)

    it("Should stop at the last line of the buffer", function()
      moveCursorDown(windowName, 500)
      assert.equals(getLastLineNumber(windowName), getLineNumber(windowName))
    end)

    it("Should reset the column unless asked to keep it", function()
      moveCursor(windowName, 2, 0)
      moveCursorDown(windowName, 1)
      assert.equals(0, getColumnNumber(windowName))
      moveCursor(windowName, 2, 0)
      moveCursorDown(windowName, 1, true)
      assert.equals(2, getColumnNumber(windowName))
    end)

    it("Should report an unknown window rather than raising", function()
      local ok, err = moveCursorDown("guiUtilsNoSuchWindow", 1)
      assert.is_nil(ok)
      assert.equals("window does not exist", err)
    end)

    it("Should treat a non-boolean keep_horizontal as false", function()
      moveCursor(windowName, 2, 0)
      moveCursorDown(windowName, 1, "yes")
      assert.equals(0, getColumnNumber(windowName))
      moveCursor(windowName, 2, 1)
      moveCursorUp(windowName, 1, "yes")
      assert.equals(0, getColumnNumber(windowName))
    end)

    -- pairs with the assertion above: without this, "coerced to false" and
    -- "keep_horizontal ignored entirely" would look the same for moveCursorUp
    it("Should let moveCursorUp keep the column when asked with a boolean", function()
      moveCursor(windowName, 2, 1)
      moveCursorUp(windowName, 1, true)
      assert.equals(2, getColumnNumber(windowName))
    end)
  end)

  describe("Tests the functionality of xReplace", function()
    local windowName = "guiUtilsXReplaceBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      echo(windowName, "hello world")
      moveCursor(windowName, 0, 0)
    end)

    local function currentLine()
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    it("Should insert the replacement plainly when it is given no type", function()
      -- creplace, dreplace and hreplace always name a type, so the plain
      -- insertText fallback is only reachable by calling xReplace itself
      selectString(windowName, "world", 1)
      xReplace(windowName, "<red>earth")
      assert.are.equal("hello <red>earth", currentLine())
    end)

    it("Should insert plainly for a type it does not know either", function()
      selectString(windowName, "world", 1)
      xReplace(windowName, "<red>earth", "z")
      assert.are.equal("hello <red>earth", currentLine())
    end)

    it("Should put the replacement where the selection was", function()
      selectString(windowName, "hello", 1)
      xReplace(windowName, "goodbye", "c")
      assert.are.equal("goodbye world", currentLine())
    end)

    it("Should fall back to the main console when the window is left out", function()
      clearWindow()
      moveCursor(0, 0)
      echo("xReplaceMain marker")
      selectString("marker", 1)
      xReplace("<red>replaced", nil, "c")
      selectCurrentLine()
      assert.are.equal("xReplaceMain replaced", getSelection())
    end)
  end)

  describe("Tests the functionality of creplace, dreplace and hreplace", function()
    local windowName = "guiUtilsColourReplaceBuffer"

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      createBuffer(windowName)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      echo(windowName, "hello world")
      moveCursor(windowName, 0, 0)
    end)

    local function currentLine()
      selectCurrentLine(windowName)
      return getSelection(windowName)
    end

    it("Should replace the selection with cecho formatted text", function()
      selectString(windowName, "world", 1)
      creplace(windowName, "<red>earth")
      assert.equals("hello earth", currentLine())
    end)

    it("Should replace the selection with decho formatted text", function()
      selectString(windowName, "world", 1)
      dreplace(windowName, "<0,255,0>earth")
      assert.equals("hello earth", currentLine())
    end)

    it("Should replace the selection with hecho formatted text", function()
      selectString(windowName, "world", 1)
      hreplace(windowName, "#0000ffearth")
      assert.equals("hello earth", currentLine())
    end)

    it("Should colour what it puts down", function()
      selectString(windowName, "world", 1)
      dreplace(windowName, "<0,255,0>earth")
      selectString(windowName, "earth", 1)
      assert.are.same({0, 255, 0}, getTextFormat(windowName).foreground)
    end)

    it("Should replace a whole line with dreplaceLine and hreplaceLine", function()
      dreplaceLine(windowName, "<0,255,0>brand new")
      assert.equals("brand new", currentLine())
      hreplaceLine(windowName, "#0000ffnewer still")
      assert.equals("newer still", currentLine())
    end)

    it("Should error when the window name is not a string", function()
      assert.has_error(function() creplace(5, "x") end)
      assert.has_error(function() dreplace(5, "x") end)
      assert.has_error(function() hreplace(5, "x") end)
      assert.has_error(function() dreplaceLine(5, "x") end)
      assert.has_error(function() hreplaceLine(5, "x") end)
    end)
  end)

  describe("Tests the functionality of scrollUp and scrollDown", function()
    local windowName = "guiUtilsScrollConsole"

    -- The scroll position getScroll reports is copied out of the buffer while
    -- the pane repaints, and the very first scroll of a console is deferred to
    -- the next event loop turn so its split screen lower pane can appear. Both
    -- need one turn of the event loop before the new position can be read.
    local function pumpEventLoop()
      tempTimer(0, function() raiseEvent("guiUtilsScrollPump") end)
      waitForEvent("guiUtilsScrollPump", 2000)
    end

    -- the repaint that publishes the new position is only posted, so poll for
    -- it rather than trusting a single turn of the event loop
    local function scrollSettlesAt(expected)
      for _ = 1, 20 do
        if getScroll(windowName) == expected then
          return getScroll(windowName)
        end
        pumpEventLoop()
      end
      return getScroll(windowName)
    end

    -- BUG: scrollTo does not move to the line it is given, it subtracts a
    -- delta from the cursor the pane last copied out of the buffer while
    -- painting, and the first scroll out of tail mode is deferred a turn and
    -- padded by the lower pane's row count. So the first scrollTo of a console
    -- lands short by a font-metric-dependent amount, and a second one issued
    -- before the pane has repainted lands short again. Re-issue it until it
    -- sticks: once the pane's copy has caught up the delta is exact.
    local function parkAt(line)
      for _ = 1, 10 do
        scrollTo(windowName, line)
        if scrollSettlesAt(line) == line then
          return line
        end
      end
      return getScroll(windowName)
    end

    setup(function()
      createMiniConsole(windowName, 0, 0, 200, 100)
      enableScrolling(windowName)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      clearWindow(windowName)
      for i = 1, 200 do
        echo(windowName, "scroll line " .. i .. "\n")
      end
      assert.equals(150, parkAt(150), "the console should be parked mid buffer before each scroll test")
    end)

    it("Should move the view up by the number of lines given", function()
      scrollUp(windowName, 5)
      assert.equals(145, scrollSettlesAt(145))
    end)

    it("Should move the view back down again", function()
      scrollDown(windowName, 4)
      assert.equals(154, scrollSettlesAt(154))
    end)

    it("Should never scroll above the first line", function()
      scrollUp(windowName, 10000)
      assert.equals(0, scrollSettlesAt(0))
    end)

    it("Should never scroll past the last line", function()
      scrollDown(windowName, 10000)
      local lastLine = getLastLineNumber(windowName)
      assert.equals(lastLine, scrollSettlesAt(lastLine))
    end)

    it("Should default to a single line when no count is given", function()
      scrollUp(windowName)
      assert.equals(149, scrollSettlesAt(149))
    end)

    it("Should report an unknown window rather than raising", function()
      local ok, err = scrollUp("guiUtilsNoSuchWindow", 1)
      assert.is_nil(ok)
      assert.equals("window does not exist", err)
      ok, err = scrollDown("guiUtilsNoSuchWindow", 1)
      assert.is_nil(ok)
      assert.equals("window does not exist", err)
    end)
  end)

  describe("Tests the functionality of setLabelCursor and resetLabelCursor", function()
    local labelName = "guiUtilsCursorLabel"

    before_each(function()
      createLabel(labelName, 0, 0, 50, 50, 1)
      hideWindow(labelName)
    end)

    after_each(function()
      pcall(deleteLabel, labelName)
    end)

    it("Should map a cursor name to the id the C++ layer wants", function()
      -- the name to id mapping is the Lua half of this function; the C++ half
      -- only accepts a number, so a name that is not in mudlet.cursor has to
      -- reach it as nil and be refused
      assert.is_true(setLabelCursor(labelName, "OpenHand"))
      assert.is_true(setLabelCursor(labelName, mudlet.cursor.OpenHand))
      assert.is_nil(mudlet.cursor.definitelyNotACursor)
      assert.has_error(function() setLabelCursor(labelName, "definitelyNotACursor") end)
    end)

    it("Should reset the cursor by asking for shape -1", function()
      setLabelCursor(labelName, "OpenHand")
      assert.is_true(resetLabelCursor(labelName))
    end)

    it("Should report an unknown label", function()
      local ok, err = setLabelCursor("guiUtilsNoSuchLabel", "OpenHand")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should error when resetLabelCursor is not given a string", function()
      assert.has_error(function() resetLabelCursor(5) end)
    end)
  end)

  -- GUIUtils.lua replaces the eight C++ callback setters with one shared Lua
  -- wrapper, so what it does with the callback it is handed is common to all of
  -- them. Firing a callback needs a real mouse or key event and so is out of
  -- reach here; this is the registration half.
  describe("Tests the functionality of the shared callback setter wrapper", function()
    local labelName = "guiUtilsCallbackLabel"
    local cmdLineName = "guiUtilsActionCmdLine"
    local labelSetters = {
      "setLabelClickCallback",
      "setLabelDoubleClickCallback",
      "setLabelReleaseCallback",
      "setLabelMoveCallback",
      "setLabelWheelCallback",
      "setLabelOnEnter",
      "setLabelOnLeave",
    }

    setup(function()
      createLabel(labelName, 10, 10, 60, 30, 1)
      createCommandLine(cmdLineName, 10, 10, 140, 30)
    end)

    teardown(function()
      deleteLabel(labelName)
      deleteCommandLine(cmdLineName)
    end)

    describe("Tests the functionality of setLabelClickCallback, setLabelDoubleClickCallback, setLabelReleaseCallback, setLabelMoveCallback, setLabelWheelCallback, setLabelOnEnter and setLabelOnLeave", function()
      for _, setter in ipairs(labelSetters) do
        it(setter .. " accepts a function, a function name and nil", function()
          assert.is_true(_G[setter](labelName, function() end))
          -- a string is compiled into a function calling that name, which does
          -- not have to exist until the callback runs
          assert.is_true(_G[setter](labelName, "guiUtilsNoSuchGlobalFunction"))
          -- nil is accepted rather than refused, which is how these seven clear
          -- a callback and where they part company with setCmdLineAction below
          assert.is_true(_G[setter](labelName, nil))
        end)

        it(setter .. " refuses a value that is none of those", function()
          local ok, err = pcall(_G[setter], labelName, 42)
          assert.is_false(ok)
          assert.is_truthy(err:find(setter .. ": bad argument #2 type (function expected, got number!)", 1, true))
        end)

        it(setter .. " reports a label it cannot find and rejects an empty name", function()
          local ok, err = _G[setter]("guiUtilsNoSuchLabel", function() end)
          assert.is_nil(ok)
          assert.are.equal("label name 'guiUtilsNoSuchLabel' not found", err)

          local emptyOk, emptyErr = _G[setter]("", function() end)
          assert.is_nil(emptyOk)
          assert.are.equal("label name cannot be an empty string", emptyErr)
        end)
      end
    end)

    describe("Tests the functionality of setCmdLineAction", function()
      it("Should accept a function", function()
        assert.is_true(setCmdLineAction(cmdLineName, function() end))
      end)

      it("Should accept a function name as a string", function()
        assert.is_true(setCmdLineAction(cmdLineName, "guiUtilsNoSuchGlobalFunction"))
      end)

      it("Should accept extra arguments alongside the action", function()
        -- what the wrapper then does with them is only observable once the
        -- action fires, which needs a real keypress
        assert.is_true(setCmdLineAction(cmdLineName, function() end, "one", 2))
      end)

      it("Should refuse nil where the label callbacks take it as a request to clear", function()
        -- the one function the wrapper singles out, because resetCmdLineAction
        -- is how a command line action is cleared. The label setter is asserted
        -- here too: the refusal on its own does not show the wrapper made the
        -- distinction, since the C++ setter rejects a nil action as well.
        local ok, err = pcall(setCmdLineAction, cmdLineName, nil)
        assert.is_false(ok)
        assert.is_truthy(err:find("setCmdLineAction: bad argument #2 type (function expected, got nil!)", 1, true))
        assert.is_true(setLabelClickCallback(labelName, nil))
      end)

      it("Should refuse a value that is neither function, string nor nil", function()
        local ok, err = pcall(setCmdLineAction, cmdLineName, 42)
        assert.is_false(ok)
        assert.is_truthy(err:find("setCmdLineAction: bad argument #2 type (function expected, got number!)", 1, true))
      end)

      it("Should report a command line it cannot find", function()
        local ok, err = setCmdLineAction("guiUtilsNoSuchCommandLine", function() end)
        assert.is_nil(ok)
        assert.are.equal("command line name 'guiUtilsNoSuchCommandLine' not found", err)
      end)

      it("Should reject an empty command line name", function()
        local ok, err = setCmdLineAction("", function() end)
        assert.is_nil(ok)
        assert.are.equal("command line name cannot be an empty string", err)
      end)

      it("Should have resetCmdLineAction as the way to take the action back", function()
        -- resetCmdLineAction is not wrapped, so it answers for the command line
        -- rather than for whether an action was ever set on it
        setCmdLineAction(cmdLineName, function() end)
        assert.is_true(resetCmdLineAction(cmdLineName))
        local ok, err = resetCmdLineAction("guiUtilsNoSuchCommandLine")
        assert.is_nil(ok)
        assert.are.equal("command line name 'guiUtilsNoSuchCommandLine' not found", err)
      end)
    end)
  end)

  describe("Tests the functionality of setBackgroundImage", function()
    local consoleName = "guiUtilsBackgroundConsole"
    -- a Qt resource that ships with every Mudlet, so no fixture file is needed
    local imagePath = ":/icons/mudlet.png"

    before_each(function()
      createMiniConsole(consoleName, 0, 0, 100, 100)
    end)

    after_each(function()
      deleteMiniConsole(consoleName)
    end)

    it("Should accept each mode name a console supports", function()
      for _, name in ipairs({"border", "center", "tile", "style"}) do
        assert.is_true(setBackgroundImage(consoleName, imagePath, name), "mode " .. name .. " should be accepted")
      end
    end)

    it("Should accept the numeric mode the names map onto", function()
      assert.is_true(setBackgroundImage(consoleName, imagePath, mudlet.BgImageMode.center))
      assert.equals(2, mudlet.BgImageMode.center)
    end)

    it("Should map the cover mode name, which only the full window accepts", function()
      assert.equals(5, mudlet.BgImageMode.cover)
      assert.is_true(setBackgroundImage("main", imagePath, "cover", true))
      -- the same name on a console reaches the C++ check for mode 5
      local ok, err = setBackgroundImage(consoleName, imagePath, "cover")
      assert.is_nil(ok)
      assert.is_truthy(err:find("cover", 1, true))
      resetBackgroundImage("main")
    end)

    it("Should pass an unknown mode name through so the C++ side rejects it", function()
      assert.has_error(function() setBackgroundImage(consoleName, imagePath, "notAMode") end)
    end)
  end)
end)
