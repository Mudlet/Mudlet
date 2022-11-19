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

  describe("Tests the functionality of replace", function()
    it("Should return nil+msg if nothing is selected to replace", function()
      deselect()
      local ok,err = replace("]")
      assert.is_nil(ok)
      assert.equals("replace: nothing is selected to be replaced. Did selectString return -1?", err)
    end)
  end)
end)

--[[
  TODO:
    replaceLine and variants
--]]
