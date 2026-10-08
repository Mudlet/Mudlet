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
        {"\27[1;31;22;32m", "<0,128,0>"},
        {"\27[1m\27[22m\27[32m", "<0,128,0>"},
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
        { "\27[38;2;m", "<0,0,0>"},
        { "\27[38;2;10;20;m", "<10,20,0>"},
        { "\27[38:2::10::30m", "<10,0,30>"},
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

    it("Should skip an xterm256 colour cut short before its index", function()
      local sequences = {
        {"\27[38;5mfoo", "foo"},
        {"\27[38;5;mfoo", "foo"},
        {"\27[48;5mfoo", "foo"},
        {"\27[31;38;5mX", "<128,0,0>X"},
        {"\27[41;48;5mX", "<:128,0,0>X"},
        {"\27[38;5;;31mX", "<128,0,0>X"},
        {"\27[31m\27[38;5m\27[1mX", "<128,0,0><255,0,0>X"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should keep an xterm256 or rgb foreground when bold follows it", function()
      local sequences = {
        {"\27[38;5;196;1mX", "<255,0,0>X"},
        {"\27[38;2;10;20;30;1mX", "<10,20,30>X"},
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

    describe("on a long line of many colours and multibyte characters", function()
      local windowName = "guiUtilsCopyDechoLongLine"
      local segments = {}

      setup(function()
        createBuffer(windowName)
        setWindowWrap(windowName, 100000)
        for i = 1, 200 do
          segments[i] = { fg = string.format("%d,%d,%d", i, 100, 200), bg = string.format("0,0,%d", i), text = string.format("é×中%d ", i) }
        end
      end)

      teardown(function()
        deleteMiniConsole(windowName)
      end)

      before_each(function()
        clearWindow(windowName)
        local parts = {}
        for i, segment in ipairs(segments) do
          parts[i] = string.format("<%s:%s>%s", segment.fg, segment.bg, segment.text)
        end
        decho(windowName, table.concat(parts) .. "\n")
        moveCursor(windowName, 0, 0)
      end)

      it("Should copy every character with its own colours", function()
        local expected = {}
        for i, segment in ipairs(segments) do
          expected[i] = string.format("%s<%s:%s>%s", i > 1 and "<r>" or "", segment.fg, segment.bg, segment.text)
        end
        assert.equals(table.concat(expected) .. "<r>", copy2decho(windowName))
      end)

      it("Should copy every character as HTML, closing each colour's span and escaping as it goes", function()
        local expected = {}
        for i, segment in ipairs(segments) do
          expected[i] = string.format("%s<span style='color: rgb(%s);background: rgb(%s);'>%s", i > 1 and "</span>" or "", segment.fg, segment.bg, (segment.text:gsub("×", "&times;")))
        end
        assert.equals(table.concat(expected) .. "</span>", copy2html(windowName))
      end)

      it("Should copy a substring from the far end of the line", function()
        assert.equals("<198,100,200:0,0,198>中198 <r><199,100,200:0,0,199>é×<r>", copy2decho(windowName, "中198 é×"))
        assert.equals("<span style='color: rgb(200,100,200);background: rgb(0,0,200);'>中200</span>", copy2html(windowName, "中200"))
      end)
    end)
  end)

  describe("Tests the functionality of copy2html", function()
    local windowName = "guiUtilsCopyHtmlBuffer"

    setup(function()
      createBuffer(windowName)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      clearWindow(windowName)
      setFgColor(windowName, 255, 0, 0)
      setBgColor(windowName, 0, 0, 0)
      echo(windowName, "a<b&c\n")
      moveCursor(windowName, 0, 0)
    end)

    it("Should wrap the line in a span carrying its foreground and background", function()
      local html = copy2html(windowName)
      assert.is_truthy(html:find("color: rgb(255,0,0)", 1, true), html)
      assert.is_truthy(html:find("background: rgb(0,0,0)", 1, true), html)
      assert.is_truthy(html:find("</span>", 1, true), html)
    end)

    it("Should escape the characters that would otherwise be markup", function()
      -- a < in game text must not open a tag, which is why copy2html exists
      -- rather than callers running copy2decho and swapping the tags
      local html = copy2html(windowName)
      assert.is_truthy(html:find("a&lt;b&amp;c", 1, true), html)
      assert.is_falsy(html:find("a<b&c", 1, true), html)
      -- copy2decho reads the same line and leaves those characters alone
      assert.is_truthy(copy2decho(windowName):find("a<b&c", 1, true))
    end)

    it("Should copy only the substring it was asked for", function()
      assert.is_truthy(copy2html(windowName, "b&c"):find(">b&amp;c<", 1, true))
    end)

    it("Should return an empty string for text that is not on the current line", function()
      assert.equals("", copy2html(windowName, "not on this line"))
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

    it("Should read stylesheets with odd spacing, case, comments and quoting the way it always has", function()
      -- each entry: stylesheet, then what differs from the default format
      local cases = {
        { "color: red", {} }, -- no semicolon, so nothing is read
        { "color: red;", { foreground = "red" } },
        { "  color :  rgb(1, 2, 3)  ;  ", { foreground = "rgb(1, 2, 3)" } },
        { "COLOR: red; Font-Weight: bold;", {} },
        { "color: blue !important; font-weight: bold !important;", { foreground = "blue !important", bold = true } },
        { "/* comment */ color: green; font-style: italic;", { italic = true } },
        { "color: green; /* trailing; comment */", { foreground = "green" } },
        { "font-family: \"Foo: Bar\", 'Baz;Qux'; color: #00ff00;", { foreground = "#00ff00" } },
        { "background-color: red; color: white;", { foreground = "white" } },
        { "background-color: red;", {} },
        { "color: red; color: blue;", { foreground = "blue" } },
        { "QLabel { color: red; }", {} },
        { "QLabel{ font: bold italic 12pt; color: #ff0000; text-decoration: underline line-through }", { foreground = "#ff0000", underline = true, strikeout = true } },
        { "font-weight: 700;", {} },
        { "font: normal; font-weight: bold;", { bold = true } },
        { "font-weight: bold; font: normal;", { bold = true } },
        { "font: italic bold 10pt \"Sans\"; text-decoration: overline;", { bold = true, italic = true, overline = true } },
        { "color:\r\n  rgba(10, 20, 30, 40);\nfont-style:\titalic;", { foreground = "rgba(10, 20, 30, 40)", italic = true } },
        { "color:;font-weight:;", {} },
        { ";;;color: red;;;", { foreground = "red" } },
        { "color: red: blue;", { foreground = "red: blue" } },
        { "border-image: url(a:b.png); color: yellow;", { foreground = "yellow" } },
        { "text-decoration: UNDERLINE; font-weight: BOLD;", {} },
      }
      for _, case in ipairs(cases) do
        local want = table.deepcopy(expected)
        for k, v in pairs(case[2]) do
          want[k] = v
        end
        setLabelStyleSheet(labelName, case[1])
        assert.are.same(want, getLabelFormat(labelName), case[1])
      end
    end)

    it("Should return a fresh table each time, so changing one leaves the next call alone", function()
      local first = getLabelFormat(labelName)
      first.foreground[1] = 0
      first.bold = true
      assert.are.same(expected, getLabelFormat(labelName))

      setLabelStyleSheet(labelName, "color: red; font-weight: bold;")
      expected.foreground = "red"
      expected.bold = true
      local styled = getLabelFormat(labelName)
      styled.foreground = { 1, 2, 3 }
      styled.bold = false
      styled.underline = nil
      assert.are.same(expected, getLabelFormat(labelName))
    end)

    it("Should follow the stylesheet as it changes, and tell labels apart", function()
      local otherLabel = "gldfTestLabel2"
      createLabel(otherLabel, 0, 0, 0, 0, 0)
      finally(function() deleteLabel(otherLabel) end)
      hideWindow(otherLabel)
      setLabelStyleSheet(labelName, "color: red;")
      setLabelStyleSheet(otherLabel, "font-weight: bold;")
      assert.are.equal("red", getLabelFormat(labelName).foreground)
      assert.is_false(getLabelFormat(labelName).bold)
      assert.are.same({ 192, 192, 192 }, getLabelFormat(otherLabel).foreground)
      assert.is_true(getLabelFormat(otherLabel).bold)

      setLabelStyleSheet(labelName, "color: blue;")
      assert.are.equal("blue", getLabelFormat(labelName).foreground)
      setLabelStyleSheet(labelName, "")
      assert.are.same(expected, getLabelFormat(labelName))
      setLabelStyleSheet(labelName, "color: red;")
      assert.are.equal("red", getLabelFormat(labelName).foreground)
    end)

    it("Should error for a label that does not exist", function()
      assert.has_error(function() getLabelFormat("gldfNoSuchLabel") end)
    end)

    it("Should start an echo to the label from its stylesheet, and pick up a new one", function()
      local function span(fg)
        return string.format('<span style="color: %s;background-color: rgba(0, 0, 0, 0); font-weight: bold; font-style: normal; text-decoration: underline;">', fg)
      end
      setLabelStyleSheet(labelName, "color: rgb(1, 2, 3); font-weight: bold; text-decoration: underline;")
      cecho(labelName, "<red>HP\n<reset>ok")
      assert.are.equal(span("rgb(1, 2, 3)") .. span("rgb(255, 0, 0)") .. "HP<br>" .. span("rgb(1, 2, 3)") .. "ok", getLabelText(labelName))

      setLabelStyleSheet(labelName, "color: blue; font-weight: bold; text-decoration: underline;")
      decho(labelName, "<0,255,0>HP<r>ok")
      assert.are.equal(span("blue") .. span("rgb(0, 255, 0)") .. "HP" .. span("blue") .. "ok", getLabelText(labelName))
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

    describe("in a named window", function()
      local windowName = "guiUtilsReplaceConsole"

      setup(function()
        createMiniConsole(windowName, 0, 0, 400, 200)
        setWindowWrap(windowName, 60)
      end)

      teardown(function()
        deleteMiniConsole(windowName)
      end)

      before_each(function()
        clearWindow(windowName)
        resetFormat(windowName)
        echo(windowName, "hello world\n")
        moveCursor(windowName, 0, 0)
      end)

      local function firstLine()
        moveCursor(windowName, 0, 0)
        selectCurrentLine(windowName)
        local text = getSelection(windowName)
        deselect(windowName)
        return text
      end

      it("Should replace the selected text", function()
        selectString(windowName, "world", 1)
        assert.equals(0, select("#", replace(windowName, "there")))
        assert.equals("hello there", firstLine())
      end)

      it("Should delete the selected text when given an empty replacement", function()
        selectString(windowName, "world", 1)
        replace(windowName, "")
        assert.equals("hello ", firstLine())
      end)

      it("Should keep the colour of the selected text when asked to", function()
        selectString(windowName, "world", 1)
        setFgColor(windowName, 255, 0, 0)
        selectString(windowName, "world", 1)
        replace(windowName, "there", true)
        selectSection(windowName, 6, 5)
        assert.are.same({255, 0, 0}, getTextFormat(windowName).foreground)
      end)

      it("Should refuse when nothing is selected, and leave the text alone", function()
        deselect(windowName)
        local ok, err = replace(windowName, "there")
        assert.is_nil(ok)
        assert.equals("replace: nothing is selected to be replaced. Did selectString return -1?", err)
        assert.equals("hello world", firstLine())
      end)

      it("Should only refuse an empty selection, not one whose text is gone", function()
        selectString(windowName, "hello", 1)
        clearWindow(windowName)
        assert.are.same({"", 0, 5}, {getSelection(windowName)})
        assert.equals(0, select("#", replace(windowName, "x")))
      end)

      it("Should not take a selection that is no longer valid for an empty one", function()
        selectString(windowName, "world", 1)
        clearWindow(windowName)
        assert.are.same({nil, "the selection is no longer valid"}, {getSelection(windowName)})
        assert.equals(0, select("#", replace(windowName, "x")))
      end)

      it("Should not complain about the selection of a window that does not exist", function()
        assert.equals(0, select("#", replace("guiUtilsNoSuchWindow", "x")))
      end)
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

    describe("Tests the html conversions with a resetFormat", function()
      local function span(fg, bg, weight, style, decoration)
        return string.format('<span style="color: %s;background-color: %s; font-weight: %s; font-style: %s; text-decoration: %s;">', fg, bg, weight, style, decoration)
      end

      it("starts from the resetFormat and goes back to it on a reset, in all three styles", function()
        local function labelFormat()
          return { foreground = "#ffffff", background = "rgba(0, 0, 0, 0)", bold = true, italic = false, overline = false, reverse = false, strikeout = false, underline = true }
        end
        local function L(fg)
          return span(fg, "rgba(0, 0, 0, 0)", "bold", "normal", "underline")
        end
        local expected = L("#ffffff") .. L("rgb(255, 0, 0)") .. "HP " .. L("rgb(0, 255, 0)") .. "1234" .. L("#ffffff") .. "/200"
        assert.are.equal(expected, cecho2html("<red>HP <green>1234<reset>/200", labelFormat()))
        assert.are.equal(expected, decho2html("<255,0,0>HP <0,255,0>1234<r>/200", labelFormat()))
        assert.are.equal(expected, hecho2html("#ff0000HP #00ff001234#r/200", labelFormat()))
      end)

      it("restores every attribute on each of several resets in a row", function()
        local reset = { foreground = { 10, 20, 30 }, background = { 40, 50, 60 }, bold = false, italic = true, overline = true, reverse = false, strikeout = true, underline = false }
        local function T(weight, decoration)
          return span("rgb(10, 20, 30)", "rgba(40, 50, 60, 255)", weight, "italic", decoration)
        end
        local base = T("normal", "overline line-through")
        local expected = base .. T("bold", "overline line-through") .. T("bold", "overline underline line-through") .. "x" .. base .. "y" .. base .. base
        assert.are.equal(expected, cecho2html("<b><u>x<reset>y</u></b>", reset))

        local blue = span("rgb(0, 0, 255)", "rgba(40, 50, 60, 255)", "normal", "italic", "overline line-through")
        expected = base .. blue .. "a" .. base .. "b" .. base .. "c" .. base .. base .. "d"
        assert.are.equal(expected, cecho2html("<blue>a<reset>b<r>c<reset><reset>d", reset))
      end)

      it("leaves the resetFormat it was given untouched", function()
        -- reversing puts the background table in the foreground slot, where an alpha gets filled in
        local reset = { foreground = { 10, 20, 30 }, background = { 40, 50, 60 }, bold = false, italic = false, overline = false, reverse = true, strikeout = false, underline = false }
        local before = table.deepcopy(reset)
        local R = span("rgb(40, 50, 60)", "rgba(10, 20, 30, 255)", "normal", "normal", "none")
        local swapped = span("rgb(0, 0, 255)", "rgba(255, 0, 0, 255)", "normal", "normal", "none")
        local expected = R .. swapped .. "a" .. R .. "b" .. R .. "c" .. R .. R .. "d"
        assert.are.equal(expected, cecho2html("<red:blue>a<reset>b<r>c<reset><reset>d", reset))
        assert.are.same(before, reset)
      end)

      it("keeps every piece of a long string, in order", function()
        local reset = { foreground = "#ffffff", background = "rgba(0, 0, 0, 0)", bold = false, italic = false, overline = false, reverse = false, strikeout = false, underline = false }
        local red = span("rgb(255, 0, 0)", "rgba(0, 0, 0, 0)", "normal", "normal", "none")
        local green = span("rgb(0, 255, 0)", "rgba(0, 0, 0, 0)", "normal", "normal", "none")
        local input, expected = {}, { span("#ffffff", "rgba(0, 0, 0, 0)", "normal", "normal", "none") }
        for i = 1, 300 do
          input[#input + 1] = (i % 2 == 0 and "<red>" or "<green>") .. "w" .. i .. " "
          expected[#expected + 1] = (i % 2 == 0 and red or green) .. "w" .. i .. " "
        end
        assert.are.equal(table.concat(expected), cecho2html(table.concat(input), reset))
      end)
    end)
  end)

  describe("Tests the exact output of the colour string converters", function()
    -- every converter, fed odd input as well as good: unknown names, malformed and
    -- escaped tags, alpha components, out of range values and multibyte text
    local raises = {}
    local cechoCases = {
      { "", string = "", ansi = "", decho = "", hecho = "" },
      { "plain text", string = "plain text", ansi = "plain text", decho = "plain text", hecho = "plain text" },
      { "héllo wörld ✓ 日本語", string = "héllo wörld ✓ 日本語", ansi = "héllo wörld ✓ 日本語", decho = "héllo wörld ✓ 日本語", hecho = "héllo wörld ✓ 日本語" },
      { "<red>red<blue>blue<reset>plain", string = "redblueplain", ansi = "\27[38:5:1mred\27[38:5:4mblue\27[0mplain", decho = "<255,0,0>red<0,0,255>blue<r>plain", hecho = "#ff0000red#0000ffblue#rplain" },
      { "<notacolour>text<r>", string = "<notacolour>text", ansi = "text\27[0m", decho = "<notacolour>text<r>", hecho = "<notacolour>text#r" },
      { "<red", string = "<red", ansi = "<red", decho = "<red", hecho = "<red" },
      { "red>", string = "red>", ansi = "red>", decho = "red>", hecho = "red>" },
      { "<>", string = "<>", ansi = "<>", decho = "<>", hecho = "<>" },
      { "<<red>>", string = "<>", ansi = "<\27[38:5:1m>", decho = "<<255,0,0>>", hecho = "<#ff0000>" },
      { "</red>x", string = "</red>x", ansi = "x", decho = "</red>x", hecho = "</red>x" },
      { "<red:blue>a<:blue>b<red:>c<red,blue>d", string = "abcd", ansi = "\27[38:5:1m\27[48:5:4ma\27[48:5:4mb\27[38:5:1mcd", decho = "<255,0,0:0,0,255>a<:0,0,255>b<255,0,0>c<255,0,0:0,0,255>d", hecho = "#ff0000,0000ffa#,0000ffb#ff0000c#ff0000,0000ffd" },
      { "<reset><r><reset>x<r>", string = "x", ansi = "\27[0m\27[0m\27[0mx\27[0m", decho = "<r><r><r>x<r>", hecho = "#r#r#rx#r" },
      { "<b>b</b><i>i</i><u>u</u><s>s</s><o>o</o>", string = "biuso", ansi = "\27[1mb\27[22m\27[3mi\27[23m\27[4mu\27[24m\27[9ms\27[29m\27[53mo\27[55m", decho = "<b>b</b><i>i</i><u>u</u><s>s</s><o>o</o>", hecho = "#bb#/b#ii#/i#uu#/u#ss#/s#oo#/o" },
      { "<ansi_light_red:ansi_010>x<DodgerBlue>y<ansi_124>z<124>w<ansi_255:12>v", string = "xyz<124>wv", ansi = "\27[38:2::255:0:0m\27[48:2::0:255:0mx\27[38:2::30:144:255my\27[38:5:124mz\27[38:5:124mw\27[38:5:255m\27[48:5:12mv", decho = "<255,0,0:0,255,0>x<30,144,255>y<175,0,0>z<124>w<238,238,238>v", hecho = "#ff0000,00ff00x#1e90ffy#af0000z<124>w#eeeeeev" },
      { "<light_red>a<lightRed>b<light_white:black>c<ansiBlack>d", string = "<light_red>a<lightRed>bcd", ansi = "\27[38:5:9ma\27[38:5:9mb\27[38:5:15m\27[48:5:0mc\27[38:2::0:0:0md", decho = "<light_red>a<lightRed>b<:0,0,0>c<0,0,0>d", hecho = "<light_red>a<lightRed>b#,000000c#000000d" },
      { "<RED>upper<Red>mixed", string = "<RED>upper<Red>mixed", ansi = "uppermixed", decho = "<RED>upper<Red>mixed", hecho = "<RED>upper<Red>mixed" },
      { "<red:notacolour>x<notacolour:red>y", string = "xy", ansi = "\27[38:5:1mx\27[48:5:1my", decho = "<255,0,0>x<:255,0,0>y", hecho = "#ff0000x#,ff0000y" },
      { "<b><red>bold red</b> <i>it", string = "bold red it", ansi = "\27[1m\27[38:5:1mbold red\27[22m \27[3mit", decho = "<b><255,0,0>bold red</b> <i>it", hecho = "#b#ff0000bold red#/b #iit" },
      { "\\<red>escaped", string = "\\escaped", ansi = "\\\27[38:5:1mescaped", decho = "\\<255,0,0>escaped", hecho = "\\#ff0000escaped" },
      { "<red>日本<blue>語", string = "日本語", ansi = "\27[38:5:1m日本\27[38:5:4m語", decho = "<255,0,0>日本<0,0,255>語", hecho = "#ff0000日本#0000ff語" },
      { "<0>zero<15>fifteen<256>over", string = "<0>zero<15>fifteen<256>over", ansi = "\27[38:5:0mzero\27[38:5:15mfifteenover", decho = "<0>zero<15>fifteen<256>over", hecho = "<0>zero<15>fifteen<256>over" },
    }
    local dechoCases = {
      { "", string = "", ansi = "", cecho = "", hecho = "" },
      { "plain", string = "plain", ansi = "plain", cecho = "plain", hecho = "plain" },
      { "<255,0,0>red<0,255,0>green<r>plain", string = "redgreenplain", ansi = "\27[38:2::255:0:0mred\27[38:2::0:255:0mgreen\27[0mplain", cecho = "<ansi_light_red>red<ansi_010>green<reset>plain", hecho = "#ff0000red#00ff00green#rplain" },
      { "<255,0,0:0,0,255>a<:0,0,255>b<0,0,255:>c", string = "abc", ansi = "\27[38:2::255:0:0m\27[48:2::0:0:255ma\27[48:2::0:0:255mb\27[38:2::0:0:255m\27[48:2:::0:0mc", cecho = "<ansi_light_red:ansi_012>a<:ansi_012>b<ansi_012>c", hecho = "#ff0000,0000ffa#,0000ffb#0000ffc" },
      { "<1,2,3>near<127,0,0>dark<254,1,1>almost", string = "neardarkalmost", ansi = "\27[38:2::1:2:3mnear\27[38:2::127:0:0mdark\27[38:2::254:1:1malmost", cecho = "<ansi_black>near<ansi_001>dark<ansi_light_red>almost", hecho = "#010203near#7f0000dark#fe0101almost" },
      { "<0,0,0:10,20,30,40>alpha", string = "alpha", ansi = "\27[38:2::0:0:0m\27[48:2::10:20:30malpha", cecho = "<ansi_black:ansi_233>alpha", hecho = "#000000,0a141ealpha" },
      { "<10,20,30,40>fgalpha", string = "<10,20,30,40>fgalpha", ansi = "\27[38:2::10:20:30mfgalpha", cecho = "<10,20,30,40>fgalpha", hecho = "<10,20,30,40>fgalpha" },
      { "<1,2>bad<1,2,3,4,5>bad2<>empty", string = "<1,2>bad<1,2,3,4,5>bad2<>empty", ansi = "\27[38:2::1:2:0mbad\27[38:2::1:2:3mbad2<>empty", cecho = "<1,2>bad<1,2,3,4,5>bad2<>empty", hecho = "<1,2>bad<1,2,3,4,5>bad2<>empty" },
      { "<r><b>b</b><i>i</i><u>u</u><s>s</s><o>o</o><r>", string = "biuso", ansi = "\27[0m\27[1mb\27[22m\27[3mi\27[23m\27[4mu\27[24m\27[9ms\27[29m\27[53mo\27[55m\27[0m", cecho = "<reset><b>b</b><i>i</i><u>u</u><s>s</s><o>o</o><reset>", hecho = "#r#bb#/b#ii#/i#uu#/u#ss#/s#oo#/o#r" },
      { "<255,0,0>a<255,0,0>b<255,0,0>c", string = "abc", ansi = "\27[38:2::255:0:0ma\27[38:2::255:0:0mb\27[38:2::255:0:0mc", cecho = "<ansi_light_red>a<ansi_light_red>b<ansi_light_red>c", hecho = "#ff0000a#ff0000b#ff0000c" },
      { "<0,0,0>black<255,255,255>white<128,128,128>grey", string = "blackwhitegrey", ansi = "\27[38:2::0:0:0mblack\27[38:2::255:255:255mwhite\27[38:2::128:128:128mgrey", cecho = "<ansi_black>black<ansi_231>white<ansi_008>grey", hecho = "#000000black#ffffffwhite#808080grey" },
      { "x<12,34,56>é✓<78,90,123>日本", string = "xé✓日本", ansi = "x\27[38:2::12:34:56mé✓\27[38:2::78:90:123m日本", cecho = "x<ansi_235>é✓<ansi_060>日本", hecho = "x#0c2238é✓#4e5a7b日本" },
      { "<007,010,099>leading", string = "leading", ansi = "\27[38:2::007:010:099mleading", cecho = "<ansi_017>leading", hecho = "#070a63leading" },
      { "<255,0,0:>trail", string = "trail", ansi = "\27[38:2::255:0:0m\27[48:2:::0:0mtrail", cecho = "<ansi_light_red>trail", hecho = "#ff0000trail" },
      { "</b>x</i>y", string = "xy", ansi = "\27[22mx\27[23my", cecho = "</b>x</i>y", hecho = "#/bx#/iy" },
      { "</r>x", string = "x", ansi = raises, cecho = "x", hecho = "x" },
      { "<300,0,0>over", string = "over", ansi = "\27[38:2::300:0:0mover", cecho = raises, hecho = "#12c0000over" },
      { "<0,0,0:300,0,0>bgover", string = "bgover", ansi = "\27[38:2::0:0:0m\27[48:2::300:0:0mbgover", cecho = raises, hecho = "#000000,12c0000bgover" },
      { "a<0,0,0:1,2,3,4>b<9,9,9,999>c", string = "ab<9,9,9,999>c", ansi = "a\27[38:2::0:0:0m\27[48:2::1:2:3mb\27[38:2::9:9:9mc", cecho = "a<ansi_black:ansi_black>b<9,9,9,999>c", hecho = "a#000000,010203b<9,9,9,999>c" },
    }
    local hechoCases = {
      { "", string = "", ansi = "", cecho = "", decho = "" },
      { "plain", string = "plain", ansi = "plain", cecho = "plain", decho = "plain" },
      { "#ff0000red#00ff00green#rplain", string = "redgreenplain", ansi = "\27[38:2::255:0:0mred\27[38:2::0:255:0mgreen\27[0mplain", cecho = "<ansi_light_red>red<ansi_010>green<reset>plain", decho = "<255,0,0>red<0,255,0>green<r>plain" },
      { "|cff0000pipe|r", string = "pipe", ansi = "\27[38:2::255:0:0mpipe\27[0m", cecho = "<ansi_light_red>pipe<reset>", decho = "<255,0,0>pipe<r>" },
      { "#ff0000,0000ffa#,0000ffb", string = "", ansi = "\27[38:2::255:0:0m\27[48:2::0:0:255m\27[48:2::0:0:255m", cecho = "<ansi_light_red:ansi_012><:ansi_012>", decho = "<255,0,0:0,0,255><:0,0,255>" },
      { "#ff0000,800000ffalpha", string = "alpha", ansi = "\27[38:2::255:0:0m\27[48:2::128:0:0malpha", cecho = "<ansi_light_red:ansi_012>alpha", decho = "<255,0,0:0,0,255>alpha" },
      { "#FF00AAupper", string = "upper", ansi = "\27[38:2::255:0:170mupper", cecho = "<ansi_199>upper", decho = "<255,0,170>upper" },
      { "#zzzzzzbad#12345five", string = "#zzzzzzbadive", ansi = "#zzzzzzbad\27[38:2::18:52:95mive", cecho = "#zzzzzzbad<midnight_blue>ive", decho = "#zzzzzzbad<18,52,95>ive" },
      { "\\#ff0000escaped", string = "#ff0000escaped", cecho = "#ff0000escaped", decho = "#ff0000escaped" },
      { "#bb#/b#ii#/i#uu#/u#ss#/s#oo#/o", string = "biuso", ansi = "\27[1mb\27[22m\27[3mi\27[23m\27[4mu\27[24m\27[9ms\27[29m\27[53mo\27[55m", cecho = "<b>b</b><i>i</i><u>u</u><s>s</s><o>o</o>", decho = "<b>b</b><i>i</i><u>u</u><s>s</s><o>o</o>" },
      { "|bb|/b|ii|/i", string = "bi", ansi = "\27[1mb\27[22m\27[3mi\27[23m", cecho = "<b>b</b><i>i</i>", decho = "<b>b</b><i>i</i>" },
      { "#010203near#7f0000dark", string = "neardark", ansi = "\27[38:2::1:2:3mnear\27[38:2::127:0:0mdark", cecho = "<ansi_black>near<ansi_001>dark", decho = "<1,2,3>near<127,0,0>dark" },
      { "x#123456é✓#abcdef日本", string = "xé✓日本", ansi = "x\27[38:2::18:52:86mé✓\27[38:2::171:205:239m日本", cecho = "x<midnight_blue>é✓<LightBlue>日本", decho = "x<18,52,86>é✓<171,205,239>日本" },
      { "#ff0000#ff0000#ff0000x", string = "x", ansi = "\27[38:2::255:0:0m\27[38:2::255:0:0m\27[38:2::255:0:0mx", cecho = "<ansi_light_red><ansi_light_red><ansi_light_red>x", decho = "<255,0,0><255,0,0><255,0,0>x" },
      { "#,ff00ff80bgalpha", string = "bgalpha", ansi = "\27[48:2::255:0:255mbgalpha", cecho = "<:spring_green>bgalpha", decho = "<:0,255,128>bgalpha" },
      { "|c,0000ffbg", string = "g", ansi = "\27[48:2::0:0:255mg", cecho = "<:ansi_012>g", decho = "<:0,0,255>g" },
      { "#/rx", string = "x", ansi = raises, cecho = "x", decho = "x" },
    }
    local ansiCases = {
      { "", string = "", decho = "" },
      { "plain", string = "plain", decho = "plain" },
      { "\27[31mred\27[0m", string = "red", decho = "<128,0,0>red<r>" },
      { "\27[1;31mbright\27[22mnormal", string = "brightnormal", decho = "<255,0,0>bright<128,0,0>normal", lastColour = 1 },
      { "\27[38;5;179;48;5;230mx\27[0;37;40m", string = "x", decho = "<215,175,95:255,255,215>x<r><192,192,192:0,0,0>", lastColour = 7 },
      { "\27[38;2;10;20;30mtc\27[48;2;1;2;3mbg", string = "tcbg", decho = "<10,20,30>tc<:1,2,3>bg" },
      { "\27[38:2::10:20:30mcolon\27[48:5:12mbg", string = "colonbg", decho = "<10,20,30>colon<:0,0,255>bg" },
      { "\27[91mlight\27[101mlightbg\27[m", string = "lightlightbg", decho = "<255,0,0>light<:255,0,0>lightbg<r>" },
      { "\27[3mi\27[23m\27[4mu\27[24m\27[9ms\27[29m\27[53mo\27[55m", string = "iuso", decho = "<i>i</i><u>u</u><s>s</s><o>o</o>" },
      { "\27[1mbold-only\27[32mgreen", string = "bold-onlygreen", decho = "bold-only<0,255,0>green", lastColour = 2 },
      { "\27[00mzero\27[;31msemi", string = "zerosemi", decho = "<r>zero<r><128,0,0>semi", lastColour = 1 },
      { "日本\27[34m語", string = "日本語", decho = "日本<0,0,128>語", lastColour = 4 },
      { "\27[38;5mtrunc", string = "trunc", decho = "trunc" },
      { "\27[38;2;1mtrunc2", string = "trunc2", decho = "<1,0,0>trunc2" },
      { "\27[1;30mdark\27[0;30mblack", string = "darkblack", decho = "<128,128,128>dark<r><0,0,0>black", lastColour = 0 },
    }

    -- a case leaves out what it does not pin, such as output that comes from a known bug
    local function check(fname, input, expected)
      if expected == nil then
        return
      end
      local results = { pcall(_G[fname], input) }
      local label = string.format("%s(%q)", fname, input)
      if expected == raises then
        assert.is_false(results[1], label .. " should raise an error")
      else
        assert.is_true(results[1], label .. " raised " .. tostring(results[2]))
        assert.are.equal(expected, results[2], label)
      end
      return results[3]
    end

    it("converts cecho strings", function()
      for _, case in ipairs(cechoCases) do
        check("cecho2string", case[1], case.string)
        check("cecho2ansi", case[1], case.ansi)
        check("cecho2decho", case[1], case.decho)
        check("cecho2hecho", case[1], case.hecho)
      end
    end)

    it("converts decho strings", function()
      for _, case in ipairs(dechoCases) do
        check("decho2string", case[1], case.string)
        check("decho2ansi", case[1], case.ansi)
        check("decho2cecho", case[1], case.cecho)
        check("decho2hecho", case[1], case.hecho)
      end
    end)

    it("converts hecho strings", function()
      for _, case in ipairs(hechoCases) do
        check("hecho2string", case[1], case.string)
        check("hecho2ansi", case[1], case.ansi)
        check("hecho2cecho", case[1], case.cecho)
        check("hecho2decho", case[1], case.decho)
      end
    end)

    it("converts ANSI strings", function()
      for _, case in ipairs(ansiCases) do
        check("ansi2string", case[1], case.string)
        local lastColour = check("ansi2decho", case[1], case.decho)
        if case.lastColour ~= nil then
          assert.are.equal(case.lastColour, lastColour, string.format("last colour of ansi2decho(%q)", case[1]))
        end
      end
    end)

    it("raises the same error for a reset tag ANSI has no code for", function()
      local ok, err = pcall(decho2ansi, "</r>x")
      assert.is_false(ok)
      assert.truthy(err:find("attempt to concatenate", 1, true), err)
      ok, err = pcall(hecho2ansi, "#/rx")
      assert.is_false(ok)
      assert.truthy(err:find("attempt to concatenate", 1, true), err)
    end)
  end)

  describe("Tests that closestColor follows changes to color_table", function()
    -- what closestColor has always done: the first colour pairs() reaches among
    -- the nearest ones
    local function scanColorTable(r, g, b)
      local least, found = math.huge, ""
      for name, color in pairs(color_table) do
        local distance = math.sqrt((color[1] - r) ^ 2 + (color[2] - g) ^ 2 + (color[3] - b) ^ 2)
        if distance < least then
          least, found = distance, name
        end
      end
      return found
    end

    local levels = { 0, 1, 37, 95, 127, 128, 175, 215, 254, 255 }
    local function assertMatchesScan(when)
      local line, expected = {}, {}
      for _, r in ipairs(levels) do
        for _, g in ipairs(levels) do
          for _, b in ipairs(levels) do
            local answer = scanColorTable(r, g, b)
            assert.are.equal(answer, closestColor(r, g, b), string.format("closestColor(%d, %d, %d) %s", r, g, b, when))
            line[#line + 1] = string.format("<%d,%d,%d>x", r, g, b)
            expected[#expected + 1] = "<" .. answer .. ">x"
          end
        end
      end
      assert.are.equal(table.concat(expected), decho2cecho(table.concat(line)), "decho2cecho " .. when)
    end

    -- each test changes a copy, so the real color_table keeps its pairs() order. The
    -- copy goes through _G, as busted keeps a spec's own globals to itself
    local originalColorTable
    before_each(function()
      originalColorTable = color_table
      local copy = {}
      for name, color in pairs(color_table) do
        copy[name] = { color[1], color[2], color[3] }
      end
      _G.color_table = copy
    end)

    after_each(function()
      _G.color_table = originalColorTable
    end)

    it("answers as a scan of color_table does, before and after it changes", function()
      assertMatchesScan("at first")
      assertMatchesScan("asked again")
      color_table.spec_converter_red = { 255, 0, 0 }
      color_table.spec_converter_grey = { 127, 127, 127 }
      assertMatchesScan("after adding colours equal to existing ones")
      color_table.red[2] = 1
      color_table.ansi_001[1] = 120
      assertMatchesScan("after changing colours in place")
      color_table.blue = { 0, 0, 254 }
      assertMatchesScan("after replacing a colour")
      color_table.spec_converter_red = nil
      color_table.white = nil
      assertMatchesScan("after removing colours")
      local another = {}
      for name, color in pairs(color_table) do
        another[name] = { color[1], color[2], color[3] }
      end
      _G.color_table = another
      assertMatchesScan("after replacing color_table")
    end)

    it("finds a colour added, changed or removed between calls", function()
      local nearest = scanColorTable(11, 22, 33)
      assert.are.equal(nearest, closestColor(11, 22, 33))
      assert.are.equal("<" .. nearest .. ">x", decho2cecho("<11,22,33>x"))
      color_table.spec_converter_teal = { 11, 22, 33 }
      assert.are.equal("spec_converter_teal", closestColor(11, 22, 33))
      assert.are.equal("spec_converter_teal", closestColor({ 11, 22, 33 }))
      assert.are.equal("<spec_converter_teal>x", decho2cecho("<11,22,33>x"))
      assert.are.equal("<spec_converter_teal>x", hecho2cecho("#0b1621x"))
      color_table.spec_converter_teal[3] = 250
      assert.are.equal(nearest, closestColor(11, 22, 33))
      assert.are.equal("<" .. nearest .. ">x", decho2cecho("<11,22,33>x"))
      color_table.spec_converter_teal[3] = 33
      assert.are.equal("<spec_converter_teal>x", decho2cecho("<11,22,33>x"))
      color_table.spec_converter_teal = nil
      assert.are.equal(nearest, closestColor(11, 22, 33))
      assert.are.equal("<" .. nearest .. ">x", decho2cecho("<11,22,33>x"))
    end)

    it("still answers for fractional and string components", function()
      -- 12.5 * 256 * 256 is also where 12, 128, 0 would be filed
      assert.are.equal(scanColorTable(12, 128, 0), closestColor(12, 128, 0))
      assert.are.equal(scanColorTable(12.5, 0, 0), closestColor(12.5, 0, 0))
      assert.are.equal(scanColorTable(12.5, 0, 0), closestColor({ 12.5, 0, 0 }))
      assert.are_not.equal(closestColor(12, 128, 0), closestColor(12.5, 0, 0))
      assert.are.equal(scanColorTable(11, 22, 33), closestColor("11", "22", "33"))
      assert.are.equal(scanColorTable(11, 22, 33), closestColor({ "11", "22", "33" }))
    end)

    it("raises an error for a table with a hole in it", function()
      assert.is_false(pcall(closestColor, { 1, nil, 3 }))
    end)

    it("uses a closestColor that a script has replaced when converting to cecho", function()
      local original = closestColor
      _G.closestColor = function() return "spec_converter_picked" end
      local ok, result = pcall(decho2cecho, "<1,2,3>x<4,5,6:7,8,9>y")
      _G.closestColor = original
      assert.is_true(ok, result)
      assert.are.equal("<spec_converter_picked>x<spec_converter_picked:spec_converter_picked>y", result)
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

    it("Should answer nil and a message for a window that does not exist", function()
      local called = false
      local ok, result, message = pcall(selectAll, "selectAllNoSuchWindow", "cat", function() called = true end)
      assert.is_true(ok, result)
      assert.is_nil(result)
      assert.is_string(message)
      assert.is_false(called)
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

  describe("Tests the format and selection state that cecho, decho and hecho work from", function()
    local windowName = "guiUtilsXEchoStateConsole"
    local bufferName = "guiUtilsXEchoStateBuffer"

    setup(function()
      createMiniConsole(windowName, 0, 0, 400, 200)
      setWindowWrap(windowName, 60)
      createBuffer(bufferName)
    end)

    teardown(function()
      deleteMiniConsole(windowName)
    end)

    before_each(function()
      clearWindow(windowName)
      clearWindow(bufferName)
      resetFormat(windowName)
      moveCursor(windowName, 0, 0)
    end)

    local function formatAt(window, from, length)
      selectSection(window, from, length)
      local format = getTextFormat(window)
      deselect(window)
      return format
    end

    local function defaultFormat()
      resetFormat(windowName)
      echo(windowName, "reference\n")
      moveCursor(windowName, 0, 0)
      local format = formatAt(windowName, 0, 1)
      clearWindow(windowName)
      moveCursor(windowName, 0, 0)
      return format
    end

    local function currentLine(window)
      selectCurrentLine(window)
      local text = getSelection(window)
      deselect(window)
      return text
    end

    it("Should start from the default format whatever was set before it", function()
      local default = defaultFormat()
      setBold(windowName, true)
      setItalics(windowName, true)
      setFgColor(windowName, 1, 2, 3)
      setBgColor(windowName, 4, 5, 6)
      cecho(windowName, "plain")
      assert.are.same(default, formatAt(windowName, 0, 5))
    end)

    it("Should leave the default format behind for whatever is echoed next", function()
      local default = defaultFormat()
      cecho(windowName, "<red:blue><b><i><u><s><o>styled")
      echo(windowName, "after")
      assert.equals("styledafter", currentLine(windowName))

      local styled = formatAt(windowName, 0, 6)
      assert.are.same(color_table.red, styled.foreground)
      assert.are.same(color_table.blue, styled.background)
      assert.is_true(styled.bold)
      assert.is_true(styled.italic)
      assert.is_true(styled.underline)
      assert.is_true(styled.strikeout)
      assert.is_true(styled.overline)

      assert.are.same(default, formatAt(windowName, 6, 5))
    end)

    it("Should not recolour text that was selected before it was called", function()
      local default = defaultFormat()
      echo(windowName, "keep\n")
      moveCursor(windowName, 0, 0)
      selectString(windowName, "keep", 1)
      moveCursor(windowName, 0, 1)
      cecho(windowName, "<red>new")
      assert.are.same(color_table.red, formatAt(windowName, 0, 3).foreground)
      moveCursor(windowName, 0, 0)
      assert.are.same(default, formatAt(windowName, 0, 4))
    end)

    it("Should clear the selection of the window it echoed to", function()
      echo(windowName, "keep\n")
      moveCursor(windowName, 0, 0)
      selectString(windowName, "keep", 1)
      decho(windowName, "<255,0,0>new")
      assert.are.same({"", 0, 0}, {getSelection(windowName)})
    end)

    it("Should apply and switch off the formatting tags in each of the three syntaxes", function()
      cecho(windowName, "<b>B</b>n<i>I</i>n<u>U</u>n<s>S</s>n<o>O</o>n\n")
      decho(windowName, "<b>B</b>n<i>I</i>n<u>U</u>n<s>S</s>n<o>O</o>n\n")
      hecho(windowName, "#bB#/bn#iI#/in#uU#/un#sS#/sn#oO#/on\n")
      local flags = {"bold", "italic", "underline", "strikeout", "overline"}
      for line = 0, 2 do
        moveCursor(windowName, 0, line)
        assert.equals("BnInUnSnOn", currentLine(windowName))
        for index, flag in ipairs(flags) do
          assert.is_true(formatAt(windowName, (index - 1) * 2, 1)[flag], flag .. " on line " .. line)
          assert.is_false(formatAt(windowName, (index - 1) * 2 + 1, 1)[flag], flag .. " off on line " .. line)
        end
      end
    end)

    it("Should go back to the default format at a reset in each of the three syntaxes", function()
      local default = defaultFormat()
      cecho(windowName, "<red:blue><b>X<reset>Y\n")
      decho(windowName, "<255,0,0:0,0,255><b>X<r>Y\n")
      hecho(windowName, "#ff0000,0000ff#bX#rY\n")
      for line = 0, 2 do
        moveCursor(windowName, 0, line)
        assert.equals("XY", currentLine(windowName))
        local before = formatAt(windowName, 0, 1)
        assert.are.same({255, 0, 0}, before.foreground)
        assert.are.same({0, 0, 255}, before.background)
        assert.is_true(before.bold)
        assert.are.same(default, formatAt(windowName, 1, 1))
      end
    end)

    it("Should write coloured text into a buffer", function()
      cecho(bufferName, "<red>in<blue>buffer")
      moveCursor(bufferName, 0, 0)
      assert.equals("inbuffer", currentLine(bufferName))
      assert.are.same(color_table.red, formatAt(bufferName, 0, 2).foreground)
      assert.are.same(color_table.blue, formatAt(bufferName, 2, 6).foreground)
    end)

    it("Should insert each coloured piece after the one before it", function()
      echo(windowName, "AB\n")
      moveCursor(windowName, 1, 0)
      cinsertText(windowName, "<red>X<blue>Y")
      moveCursor(windowName, 0, 0)
      assert.equals("AXYB", currentLine(windowName))
      assert.are.same(color_table.red, formatAt(windowName, 1, 1).foreground)
      assert.are.same(color_table.blue, formatAt(windowName, 2, 1).foreground)
    end)

    it("Should insert each piece after one that is not ASCII", function()
      -- é is two bytes, 漢字 six, and 😀 four bytes and two columns
      for _, case in ipairs({{"é", "éXAB"}, {"漢字", "漢字XAB"}, {"😀", "😀XAB"}}) do
        clearWindow(windowName)
        echo(windowName, "AB\n")
        moveCursor(windowName, 0, 0)
        cinsertText(windowName, case[1] .. "<red>X")
        moveCursor(windowName, 0, 0)
        assert.equals(case[2], currentLine(windowName))
      end
    end)

    it("Should insert each piece after bytes that are not valid UTF-8", function()
      local replacement = "\239\191\189"
      -- a stray continuation byte, a truncated emoji and an encoded surrogate
      for _, case in ipairs({{"\128", 1}, {"\240\159\152", 3}, {"\237\160\128", 3}}) do
        clearWindow(windowName)
        echo(windowName, "AB\n")
        moveCursor(windowName, 0, 0)
        cinsertText(windowName, case[1] .. "<red>X")
        moveCursor(windowName, 0, 0)
        assert.equals(string.rep(replacement, case[2]) .. "XAB", currentLine(windowName))
      end
    end)

    it("Should answer nil and a message for an insert into a window that does not exist", function()
      for _, insert in ipairs({{cinsertText, "<red>x"}, {dinsertText, "<255,0,0>x"}, {hinsertText, "#ff0000x"}}) do
        local ok, result, message = pcall(insert[1], "guiUtilsNoSuchWindow", insert[2])
        assert.is_true(ok, result)
        assert.is_nil(result)
        assert.equals("window does not exist", message)
      end
    end)

    it("Should return nothing", function()
      assert.equals(0, select("#", cecho(windowName, "<red>x")))
      assert.equals(0, select("#", xEcho("Color", "echo", windowName, "<red>x")))
      assert.equals(0, select("#", xEcho("Hex", "insertText", windowName, "#ff0000x")))
    end)

    it("Should report a text that is not a string from one line inside xEcho", function()
      -- one line for every wrapper means the position is xEcho's own, as
      -- assert() reports it, rather than that of whoever called xEcho
      local lines = {}
      local function lineOf(err, message)
        local line, text = err:match("^[^\n]*GUIUtils%.lua:(%d+): (.*)$")
        assert.equals(message, text)
        lines[#lines + 1] = line
      end

      local ok, err = pcall(cecho, 5)
      assert.is_false(ok)
      lineOf(err, "cecho: bad argument #1, string expected, got number!)")

      ok, err = pcall(dinsertText)
      assert.is_false(ok)
      lineOf(err, "dinsertText: bad argument #1, string expected, got nil!)")

      ok, err = pcall(hechoLink, {}, "send('x')", "a hint")
      assert.is_false(ok)
      lineOf(err, "hechoLink: bad argument #1, string expected, got table!)")

      assert.is_truthy(lines[1])
      assert.equals(lines[1], lines[2])
      assert.equals(lines[1], lines[3])
    end)

    it("Should take the window name as the text, on the main console, when the text is nil or false", function()
      clearWindow()
      moveCursor(0, 0)
      cecho("<red>mainText", nil)
      cecho(windowName, false)
      selectCurrentLine()
      assert.equals("mainText" .. windowName, getSelection())
      deselect()
      assert.equals("", currentLine(windowName))
    end)

    it("Should echo a number given as the text", function()
      cecho(windowName, 42)
      assert.equals("42", currentLine(windowName))
    end)

    it("Should quietly do nothing for a window that does not exist", function()
      assert.has_no.errors(function() cecho("guiUtilsNoSuchWindow", "<red>x") end)
      assert.has_no.errors(function() decho("guiUtilsNoSuchWindow", "<255,0,0>x") end)
    end)

    it("Should count a trailing nil out of a link's arguments", function()
      clearWindow()
      moveCursor(0, 0)
      cechoLink("<red>trailing", "send('x')", "a hint", nil)
      selectCurrentLine()
      assert.equals("trailing", getSelection())
      deselect()

      cechoPopup(windowName, "<red>menu", {"send('x')"}, {"a hint"}, nil)
      assert.equals("menu", currentLine(windowName))
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

    it("Should answer nil and a message for a window that does not exist", function()
      for _, colourReplace in ipairs({creplace, dreplace, hreplace}) do
        local ok, result, message = pcall(colourReplace, "guiUtilsNoSuchWindow", "x")
        assert.is_true(ok, result)
        assert.is_nil(result)
        assert.is_string(message)
      end
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
      -- only accepts a number, so a name that is not in mudlet.cursor is refused
      -- here, the same way the C++ half refuses a number it does not know
      assert.is_true(setLabelCursor(labelName, "OpenHand"))
      assert.is_true(setLabelCursor(labelName, mudlet.cursor.OpenHand))
      assert.is_nil(mudlet.cursor.definitelyNotACursor)
      local ok, err = setLabelCursor(labelName, "definitelyNotACursor")
      assert.is_nil(ok)
      assert.are.equal("cursor shape 'definitelyNotACursor' not found. see https://doc.qt.io/qt-5/qt.html#CursorShape-enum", err)
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
