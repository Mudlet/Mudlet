-- Raw game data driven through the real telnet parser with feedTelnet(), which
-- only injects while the profile is offline - see the tests README.

-- where the test running now began writing, set by each describe's before_each
local mark

-- the buffer line the marker landed on, looked for from where this test's own
-- output starts so that an earlier test's identical text cannot answer
local function lineNumberOf(marker)
  local last = getLastLineNumber("main")
  local lines = getLines("main", mark, last + 1)
  for i = #lines, 1, -1 do
    if lines[i]:find(marker, 1, true) then
      return mark + i - 1
    end
  end
  return nil
end

-- isAnsiFgColor/isAnsiBgColor and getTextFormat all read the first character
-- of the selection, so selecting the marker puts them on the cell it opens
local function selectMarker(marker)
  local line = lineNumberOf(marker)
  assert.is_truthy(line, "no line carrying '" .. marker .. "' reached the buffer")
  assert.is_true(moveCursor("main", 0, line))
  assert.is_true(selectString(marker, 1) >= 0, "'" .. marker .. "' was not selectable on the line it landed on")
end

local function formatOf(marker)
  selectMarker(marker)
  return getTextFormat("main")
end

describe("Tests SGR default colour handling", function()

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  before_each(function()
    mark = getLastLineNumber("main")
  end)

  after_each(function()
    deselect()
  end)

  -- the reported bug (#9466): red, then default foreground, then bold - the
  -- bold text must be the default colour in a bold font, not bright red
  it("does not resurrect a reset foreground when bold follows it", function()
    feed("\27[0m\27[31mSgrRedA \27[39mSgrPlainA \27[1mSgrBoldA\r\n")
    selectMarker("SgrBoldA")
    assert.is_false(isAnsiFgColor(3), "the bold text came back as bright red")
    assert.is_true(isAnsiFgColor(0), "the bold text is not in the default foreground")
    assert.is_true(getTextFormat("main").bold)
  end)

  -- the working comparison from the bug report: a full SGR 0 reset instead of
  -- SGR 39 behaves the same way
  it("keeps the default foreground when bold follows a full reset", function()
    feed("\27[0m\27[31mSgrRedB \27[0mSgrPlainB \27[1mSgrBoldB\r\n")
    selectMarker("SgrBoldB")
    assert.is_true(isAnsiFgColor(0))
    assert.is_true(getTextFormat("main").bold)
  end)

  it("lets a colour set after a foreground reset still brighten with bold", function()
    feed("\27[0m\27[31ma \27[39mb \27[31mSgrAgainC \27[1mSgrBrightC\r\n")
    selectMarker("SgrAgainC")
    assert.is_true(isAnsiFgColor(4), "the colour set after the reset did not take")
    selectMarker("SgrBrightC")
    assert.is_true(isAnsiFgColor(3), "bold did not brighten the colour set after the reset")
  end)

  it("brightens a colour set in the same sequence as bold", function()
    feed("\27[0m\27[31;1mSgrBrightD\r\n")
    selectMarker("SgrBrightD")
    assert.is_true(isAnsiFgColor(3))
  end)

  it("applies a foreground reset that arrives while bold is active", function()
    feed("\27[0m\27[1m\27[31mSgrBrightE \27[39mSgrAfterE\r\n")
    selectMarker("SgrBrightE")
    assert.is_true(isAnsiFgColor(3))
    selectMarker("SgrAfterE")
    assert.is_true(isAnsiFgColor(0), "the text after the reset is not in the default foreground")
    assert.is_true(getTextFormat("main").bold)
  end)

  it("treats a foreground reset and bold in one sequence like separate ones", function()
    feed("\27[0m\27[31mSgrRedF \27[39;1mSgrBoldF\r\n")
    selectMarker("SgrBoldF")
    assert.is_true(isAnsiFgColor(0))
    assert.is_true(getTextFormat("main").bold)
  end)

  it("clears a 256 colour foreground on a reset", function()
    feed("\27[0m\27[38;5;196mSgrExtG \27[39mSgrPlainG \27[1mSgrBoldG\r\n")
    selectMarker("SgrBoldG")
    assert.is_true(isAnsiFgColor(0))
    assert.is_true(getTextFormat("main").bold)
  end)

  it("sets a truecolour foreground and background from one sequence", function()
    feed("\27[0m\27[38;2;120;134;94;48;2;85;250;33mSgrTrueI\r\n")
    selectMarker("SgrTrueI")
    local format = getTextFormat("main")
    assert.are.same({120, 134, 94}, format.foreground)
    assert.are.same({85, 250, 33}, format.background)
  end)

  it("reads colon separated and semicolon separated colours in one sequence", function()
    feed("\27[0m\27[1;38:2::120:134:94;48:5:240mSgrMixedJ\r\n")
    selectMarker("SgrMixedJ")
    local format = getTextFormat("main")
    assert.are.same({120, 134, 94}, format.foreground)
    assert.are.same({88, 88, 88}, format.background)
    assert.is_true(format.bold)
  end)

  -- thirteen parameters outgrows the inline capacity of the array decodeSGR()
  -- collects them into, so this is the case that spills onto the heap
  it("keeps a colour correct past the twelfth parameter of a sequence", function()
    feed("\27[0m\27[0;0;0;0;0;0;0;0;0;0;38;5;159mSgrSpillK\r\n")
    selectMarker("SgrSpillK")
    assert.are.same({175, 255, 255}, getTextFormat("main").foreground)
  end)

  -- the parameter string itself outgrows SGR_INLINE_CHARS in TBuffer.cpp, the
  -- inline capacity of the buffer the CSI scanner widens those bytes into, so
  -- this is the case that spills that buffer onto the heap
  it("keeps a colour correct past the inline length of a parameter string", function()
    local parameters = string.rep("0;", 31) .. "38;5;159"
    assert.is_true(#parameters > 64, "the sequence stopped reaching the heap path")
    feed("\27[0m\27[" .. parameters .. "mSgrLongL\r\n")
    selectMarker("SgrLongL")
    assert.are.same({175, 255, 255}, getTextFormat("main").foreground)
  end)

  -- a bare ESC[m is a full reset only while the parameter tokeniser keeps empty
  -- parts - skip them and the sequence carries no parameter at all, and does nothing
  it("treats a parameterless SGR sequence as a full reset", function()
    feed("\27[0m\27[1;31mSgrBoldRedL \27[mSgrPlainL\r\n")
    selectMarker("SgrBoldRedL")
    assert.is_true(isAnsiFgColor(3), "the bold red text did not come out bright red")
    selectMarker("SgrPlainL")
    assert.is_true(isAnsiFgColor(0), "the parameterless sequence did not restore the default foreground")
    assert.is_false(getTextFormat("main").bold, "the parameterless sequence did not clear bold")
  end)

  it("restores the default background and keeps it through a later bold", function()
    feed("\27[0m\27[41mSgrRedBgH \27[49mSgrPlainBgH \27[1mSgrBoldBgH\r\n")
    selectMarker("SgrPlainBgH")
    assert.is_true(isAnsiBgColor(0), "the background reset did not take")
    selectMarker("SgrBoldBgH")
    assert.is_true(isAnsiBgColor(0), "bold disturbed the restored default background")
    assert.is_true(isAnsiFgColor(0))
  end)

  -- bytes captured from eden-test.rpgframework.de:4000's "Check client
  -- compatibility" screen, where the bug was reported: a red checkbox dash
  -- reset with SGR 39, followed by a bold and underlined section header on the
  -- next line
  it("renders a real game's compatibility screen header in the default colour", function()
    feed("\27[0m\27[1m      16 colors\27[22m: [\27[31m-\27[39m]                 \r\n"
      .. "\27[4m\27[1mColor Capabilities                  \27[22m\27[24m\r\n")
    selectMarker("Color Capabilities")
    assert.is_false(isAnsiFgColor(3), "the section header came back as bright red")
    assert.is_true(isAnsiFgColor(0))
    local format = getTextFormat("main")
    assert.is_true(format.bold)
    assert.is_true(format.underline)
  end)
end)

describe("Tests the rest of the SGR decoder", function()

  -- every case starts from a full reset so the case before it cannot leak in,
  -- and ends on one so the specs that run after this file start clean
  local function feed(data)
    local ok, msg = feedTelnet("\27[0m" .. data .. "\27[0m\r\n")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- The profile's own defaults coincide with two palette entries: the default
  -- foreground is ANSI white and the default background ANSI black. A colour
  -- case landing on either would be green without the decoder running at all,
  -- so every one of them starts from a colour no palette entry holds.
  local offPalette = "\27[38;2;1;2;3;48;2;1;2;3m"

  before_each(function()
    mark = getLastLineNumber("main")
  end)

  after_each(function()
    deselect()
  end)

  it("turns each attribute on and its partner code back off again", function()
    local cases = {
      {on = "1", off = "22", field = "bold"},
      {on = "3", off = "23", field = "italic"},
      {on = "4", off = "24", field = "underline"},
      {on = "7", off = "27", field = "reverse"},
      {on = "8", off = "28", field = "concealed"},
      {on = "9", off = "29", field = "strikeout"},
      {on = "53", off = "55", field = "overline"},
    }
    for i, case in ipairs(cases) do
      local onMarker = ("SgrOn%02d"):format(i)
      local offMarker = ("SgrOff%02d"):format(i)
      feed("\27[" .. case.on .. "m" .. onMarker .. " \27[" .. case.off .. "m" .. offMarker)
      assert.is_true(formatOf(onMarker)[case.field], "SGR " .. case.on .. " did not set " .. case.field)
      assert.is_false(formatOf(offMarker)[case.field], "SGR " .. case.off .. " did not clear " .. case.field)
    end
  end)

  -- SGR 2 is faint, an intensity Mudlet has no rendering for, so it drops back
  -- to normal rather than staying bold
  it("clears bold on the faint code", function()
    feed("\27[1mSgrFaintA \27[2mSgrFaintB")
    assert.is_true(formatOf("SgrFaintA").bold)
    assert.is_false(formatOf("SgrFaintB").bold)
  end)

  it("keeps slow and fast blink apart and clears both on the blink-off code", function()
    feed("\27[5mSgrBlinkA \27[6mSgrBlinkB \27[25mSgrBlinkC")
    assert.equals("slow", formatOf("SgrBlinkA").blinking)
    assert.equals("fast", formatOf("SgrBlinkB").blinking)
    assert.equals("none", formatOf("SgrBlinkC").blinking)
  end)

  it("selects each of the nine alternate fonts and returns to the default", function()
    for code = 11, 19 do
      local marker = ("SgrFont%d"):format(code)
      local offMarker = ("SgrFontOff%d"):format(code)
      feed("\27[" .. code .. "m" .. marker .. " \27[10m" .. offMarker)
      assert.equals(code - 10, formatOf(marker).alternateFont, "SGR " .. code .. " did not select its alternate font")
      assert.equals(0, formatOf(offMarker).alternateFont, "SGR 10 did not return to the default font")
    end
  end)

  -- The marker ahead of the sequence carries the off-palette colour, so the
  -- precondition rules out the sequence doing nothing at all. "Was" goes inside
  -- the name because neither marker may contain the other, or selectString
  -- would answer with the first of the two.
  local function checkColour(sequence, marker, target, counterpart, isColour, what)
    local was = (marker:gsub("^Sgr", "SgrWas", 1))
    feed(offPalette .. was .. " \27[" .. sequence .. "m" .. marker)
    selectMarker(was)
    assert.is_false(isColour(target), "the precondition failed - the " .. what .. " SGR " .. sequence .. " asks for was already in place")
    selectMarker(marker)
    assert.is_true(isColour(target), "SGR " .. sequence .. " did not give ANSI " .. what .. " " .. target)
    -- the other intensity of the same hue must not answer too, or the assertion
    -- above would pass on any colour that happened to match
    assert.is_false(isColour(counterpart), "SGR " .. sequence .. " gave the other intensity of its colour")
  end

  it("maps SGR 30 to 37 onto the profile's dark foreground colours", function()
    local expected = {[30] = 2, [31] = 4, [32] = 6, [33] = 8, [34] = 10, [35] = 12, [36] = 14, [37] = 16}
    for code = 30, 37 do
      checkColour(code, ("SgrFg%d"):format(code), expected[code], expected[code] - 1, isAnsiFgColor, "foreground")
    end
  end)

  it("maps SGR 90 to 97 onto the profile's light foreground colours", function()
    local expected = {[90] = 1, [91] = 3, [92] = 5, [93] = 7, [94] = 9, [95] = 11, [96] = 13, [97] = 15}
    for code = 90, 97 do
      checkColour(code, ("SgrFg%d"):format(code), expected[code], expected[code] + 1, isAnsiFgColor, "foreground")
    end
  end)

  it("maps SGR 40 to 47 onto the profile's dark background colours", function()
    local expected = {[40] = 2, [41] = 4, [42] = 6, [43] = 8, [44] = 10, [45] = 12, [46] = 14, [47] = 16}
    for code = 40, 47 do
      checkColour(code, ("SgrBg%d"):format(code), expected[code], expected[code] - 1, isAnsiBgColor, "background")
    end
  end)

  it("maps SGR 100 to 107 onto the profile's light background colours", function()
    local expected = {[100] = 1, [101] = 3, [102] = 5, [103] = 7, [104] = 9, [105] = 11, [106] = 13, [107] = 15}
    for code = 100, 107 do
      checkColour(code, ("SgrBg%d"):format(code), expected[code], expected[code] + 1, isAnsiBgColor, "background")
    end
  end)

  local indexedPalette = {[0] = 2, [1] = 4, [2] = 6, [3] = 8, [4] = 10, [5] = 12, [6] = 14, [7] = 16,
                          [8] = 1, [9] = 3, [10] = 5, [11] = 7, [12] = 9, [13] = 11, [14] = 13, [15] = 15}

  -- indices 8 to 15 are the light half of the palette, which the decoder reaches
  -- by switching bold on internally - the cell still gets the light colour and
  -- no bold flag, because bold only brightens text that is in the default colour
  it("maps the first sixteen indexed foreground colours onto the ANSI palette", function()
    for index = 0, 15 do
      local expected = indexedPalette[index]
      local marker = ("SgrIdxFg%02d"):format(index)
      checkColour("38;5;" .. index, marker, expected, index >= 8 and expected + 1 or expected - 1, isAnsiFgColor, "foreground")
      assert.is_false(formatOf(marker).bold, "38;5;" .. index .. " reported bold on an explicitly coloured cell")
    end
  end)

  it("maps the first sixteen indexed background colours onto the ANSI palette", function()
    for index = 0, 15 do
      local expected = indexedPalette[index]
      checkColour("48;5;" .. index, ("SgrIdxBg%02d"):format(index), expected, index >= 8 and expected + 1 or expected - 1, isAnsiBgColor, "background")
    end
  end)

  -- as checkColour above, but for the cases whose answer is an RGB triplet
  -- rather than a palette entry
  local function checkRgb(sequence, marker, rgb, component)
    local was = (marker:gsub("^Sgr", "SgrWas", 1))
    feed(offPalette .. was .. " \27[" .. sequence .. "m" .. marker)
    assert.are_not.same(rgb, formatOf(was)[component], "the precondition failed - the " .. component .. " SGR " .. sequence .. " asks for was already in place")
    assert.are.same(rgb, formatOf(marker)[component], "SGR " .. sequence .. " did not give " .. component .. " " .. table.concat(rgb, ","))
  end

  it("maps the indexed greyscale ramp onto its shades of grey", function()
    for _, index in ipairs({232, 240, 255}) do
      local value = (index - 232) * 10 + 8
      checkRgb("38;5;" .. index, ("SgrGrey%d"):format(index), {value, value, value}, "foreground")
    end
    checkRgb("48;5;255", "SgrGreyBg255", {238, 238, 238}, "background")
  end)

  it("maps the indexed colour cube onto a background colour", function()
    local cases = {
      {index = 16, rgb = {0, 0, 0}},
      {index = 21, rgb = {0, 0, 255}},
      {index = 196, rgb = {255, 0, 0}},
      {index = 231, rgb = {255, 255, 255}},
    }
    for _, case in ipairs(cases) do
      checkRgb("48;5;" .. case.index, ("SgrCube%d"):format(case.index), case.rgb, "background")
    end
  end)

  it("fills the missing components of a truncated truecolour foreground with zero", function()
    checkRgb("38;2;120;134", "SgrTruncA", {120, 134, 0}, "foreground")
    checkRgb("38;2;120", "SgrTruncB", {120, 0, 0}, "foreground")
    checkRgb("38;2", "SgrTruncC", {0, 0, 0}, "foreground")
  end)

  it("fills the missing components of a truncated truecolour background with zero", function()
    checkRgb("48;2;120;134", "SgrTruncD", {120, 134, 0}, "background")
    checkRgb("48;2;120", "SgrTruncE", {120, 0, 0}, "background")
    checkRgb("48;2", "SgrTruncF", {0, 0, 0}, "background")
  end)

  it("treats an indexed colour with no index at all as index zero", function()
    feed("\27[31mSgrNoIdxA \27[38;5mSgrNoIdxB")
    selectMarker("SgrNoIdxA")
    assert.is_true(isAnsiFgColor(4), "the precondition colour did not take")
    selectMarker("SgrNoIdxB")
    assert.is_true(isAnsiFgColor(2), "a missing colour index was not treated as index zero")
  end)

  -- '<' is a parameter byte, so this is still a complete SGR sequence - only its
  -- own first byte would have made the sequence a private one Mudlet skips
  it("leaves the colour alone when an indexed colour's index cannot be read", function()
    feed("\27[31mSgrBadIdxA \27[38;5;<mSgrBadIdxB")
    selectMarker("SgrBadIdxA")
    assert.is_true(isAnsiFgColor(4), "the precondition colour did not take")
    selectMarker("SgrBadIdxB")
    assert.is_true(isAnsiFgColor(4), "an unreadable colour index was treated as a request for index zero")

    feed("\27[41mSgrBadIdxC \27[48;5;<mSgrBadIdxD")
    selectMarker("SgrBadIdxC")
    assert.is_true(isAnsiBgColor(4), "the precondition background did not take")
    selectMarker("SgrBadIdxD")
    assert.is_true(isAnsiBgColor(4), "an unreadable background index was treated as a request for index zero")
  end)

  -- as xterm does, an index past the end of the 256-colour palette changes
  -- nothing, rather than reading as black or overflowing the greyscale ramp
  it("leaves the colour alone when an indexed colour's index is out of range", function()
    for i, index in ipairs({"256", "999", "2147483647"}) do
      for form, separator in ipairs({";", ":"}) do
        local fg = ("38%s5%s%s"):format(separator, separator, index)
        local before, after = ("SgrBigIdxFgA%d%d"):format(form, i), ("SgrBigIdxFgB%d%d"):format(form, i)
        feed("\27[31m" .. before .. " \27[" .. fg .. "m" .. after)
        selectMarker(before)
        assert.is_true(isAnsiFgColor(4), "the precondition colour did not take")
        selectMarker(after)
        assert.is_true(isAnsiFgColor(4), "SGR " .. fg .. " changed the foreground")

        local bg = ("48%s5%s%s"):format(separator, separator, index)
        before, after = ("SgrBigIdxBgA%d%d"):format(form, i), ("SgrBigIdxBgB%d%d"):format(form, i)
        feed("\27[41m" .. before .. " \27[" .. bg .. "m" .. after)
        selectMarker(before)
        assert.is_true(isAnsiBgColor(4), "the precondition background did not take")
        selectMarker(after)
        assert.is_true(isAnsiBgColor(4), "SGR " .. bg .. " changed the background")
      end
    end
  end)

  it("still applies the codes after an out-of-range indexed colour", function()
    feed("\27[31mSgrBigIdxThenA \27[38;5;999;32;48;5;999;42mSgrBigIdxThenB")
    selectMarker("SgrBigIdxThenA")
    assert.is_true(isAnsiFgColor(4), "the precondition colour did not take")
    selectMarker("SgrBigIdxThenB")
    assert.is_true(isAnsiFgColor(6), "the code after an out-of-range foreground index was not applied")
    assert.is_true(isAnsiBgColor(6), "the code after an out-of-range background index was not applied")
  end)

  -- types 3 (direct CMY) and 4 (direct CMYK) are not rendered, but their
  -- arguments still have to be stepped over or the codes behind them are read
  -- as colour requests of their own
  it("steps over the arguments of an unhandled colour type and resumes after them", function()
    feed("\27[38;3;1;2;3;31mSgrSkipA")
    selectMarker("SgrSkipA")
    assert.is_true(isAnsiFgColor(4), "the SGR 31 behind a 38;3 sequence was not reached")

    feed("\27[38;4;1;2;3;4;32mSgrSkipB")
    selectMarker("SgrSkipB")
    assert.is_true(isAnsiFgColor(6), "the SGR 32 behind a 38;4 sequence was not reached")

    feed("\27[48;3;1;2;3;41mSgrSkipC")
    selectMarker("SgrSkipC")
    assert.is_true(isAnsiBgColor(4), "the SGR 41 behind a 48;3 sequence was not reached")

    feed("\27[48;4;1;2;3;4;42mSgrSkipD")
    selectMarker("SgrSkipD")
    assert.is_true(isAnsiBgColor(6), "the SGR 42 behind a 48;4 sequence was not reached")
  end)

  it("leaves the colour alone when its type is missing, zero or empty", function()
    feed("\27[31mSgrStopA \27[38mSgrStopB")
    selectMarker("SgrStopB")
    assert.is_true(isAnsiFgColor(4), "a bare SGR 38 disturbed the foreground")

    feed("\27[31mSgrStopC \27[38;0mSgrStopD")
    selectMarker("SgrStopD")
    assert.is_true(isAnsiFgColor(4), "a zero colour type disturbed the foreground")

    feed("\27[31mSgrStopE \27[38;mSgrStopF")
    selectMarker("SgrStopF")
    assert.is_true(isAnsiFgColor(4), "an empty colour type disturbed the foreground")

    feed("\27[41mSgrStopG \27[48mSgrStopH")
    selectMarker("SgrStopH")
    assert.is_true(isAnsiBgColor(4), "a bare SGR 48 disturbed the background")

    feed("\27[41mSgrStopI \27[48;mSgrStopJ")
    selectMarker("SgrStopJ")
    assert.is_true(isAnsiBgColor(4), "an empty background colour type disturbed the background")
  end)

  it("applies the parameters around a colour whose type is empty, zero, unreadable or not rendered", function()
    for i, colour in ipairs({"38;;", "38;0;", "38;<;", "38;1;", "38;9;", "48;;", "48;0;", "48;<;", "48;1;", "48;9;"}) do
      local marker = ("SgrBadTypeAttr%02d"):format(i)
      feed("\27[3;41;" .. colour .. "4m" .. marker)
      local format = formatOf(marker)
      assert.is_true(format.italic, "the italic before SGR " .. colour .. " was dropped")
      assert.is_true(format.underline, "the underline after SGR " .. colour .. " was dropped")
      assert.is_false(format.bold, "the type of SGR " .. colour .. " was read as bold")
      assert.is_false(format.strikeout, "the type of SGR " .. colour .. " was read as strikeout")
      selectMarker(marker)
      assert.is_true(isAnsiBgColor(4), "the background before SGR " .. colour .. " was dropped")
    end
  end)

  -- a sequence with an intermediate byte is held over a split between two
  -- reads like any other, rather than its tail reaching the screen
  it("carries a sequence with an intermediate byte over to the next read", function()
    for i, case in ipairs({{"\27[1 ", "qAB"}, {"\27[38;5;-", "1mAB"}}) do
      local marker = ("SgrSplitInt%d"):format(i)
      assert.is_true(feedTelnet(marker .. "(" .. case[1]))
      assert.is_true(feedTelnet(case[2] .. ")" .. marker .. "\r\n"))
      local line = lineNumberOf(marker)
      assert.equals(marker .. "(AB)" .. marker, line and getLines("main", line, line + 1)[1])
    end
  end)

  it("ignores a sub-parameter colour type Mudlet does not render", function()
    feed("\27[31mSgrSubTypeA \27[38:1mSgrSubTypeB \27[38:3mSgrSubTypeC")
    selectMarker("SgrSubTypeB")
    assert.is_true(isAnsiFgColor(4), "the transparent colour type disturbed the foreground")
    selectMarker("SgrSubTypeC")
    assert.is_true(isAnsiFgColor(4), "the direct CMY colour type disturbed the foreground")

    feed("\27[41mSgrSubTypeD \27[48:1mSgrSubTypeE \27[48:4mSgrSubTypeF")
    selectMarker("SgrSubTypeE")
    assert.is_true(isAnsiBgColor(4), "the transparent colour type disturbed the background")
    selectMarker("SgrSubTypeF")
    assert.is_true(isAnsiBgColor(4), "the direct CMYK colour type disturbed the background")
  end)

  -- the VTE proposal for telling italics from slanted text apart, which Mudlet
  -- renders the same way
  it("reads the sub-parameter form of the italics code", function()
    feed("\27[3:1mSgrColItA \27[3:0mSgrColItB")
    assert.is_true(formatOf("SgrColItA").italic, "3:1 did not switch italics on")
    assert.is_false(formatOf("SgrColItB").italic, "3:0 did not switch italics off")

    feed("\27[3:2mSgrColItC")
    assert.is_true(formatOf("SgrColItC").italic, "the slant sub-parameter was not shown as italics")

    feed("\27[3:1mSgrColItD \27[3:7mSgrColItE")
    assert.is_true(formatOf("SgrColItD").italic, "the precondition italics did not take")
    assert.is_false(formatOf("SgrColItE").italic, "an out-of-range italics sub-parameter was not treated as off")

    feed("\27[3:1mSgrColItF \27[3:mSgrColItG")
    assert.is_true(formatOf("SgrColItF").italic, "the precondition italics did not take")
    assert.is_false(formatOf("SgrColItG").italic, "a missing italics sub-parameter was not treated as off")
  end)

  it("treats a missing underline sub-parameter as underline off", function()
    feed("\27[4:1mSgrColUnA \27[4:mSgrColUnB")
    assert.is_true(formatOf("SgrColUnA").underline, "the precondition underline did not take")
    assert.is_false(formatOf("SgrColUnB").underline, "a missing underline sub-parameter was not treated as off")
  end)

  -- fast blink first, so a decoder that read this as a bare SGR 5 would leave
  -- slow blink behind and one that reset the sequence would leave none - the
  -- state it has to be found in cannot be reached by accident
  it("ignores a sub-parameter string whose leading code has no sub-parameter form", function()
    feed("\27[6mSgrColBadA \27[5:1mSgrColBadB")
    assert.equals("fast", formatOf("SgrColBadA").blinking, "the precondition failed - SGR 6 did not set fast blink")
    assert.equals("fast", formatOf("SgrColBadB").blinking, "a 5:1 sequence was acted on rather than ignored")
  end)

  it("ignores a single code it has no handler for and keeps the text after it", function()
    feed("\27[31mSgrUnknownA \27[73mSgrUnknownB")
    selectMarker("SgrUnknownB")
    assert.is_true(isAnsiFgColor(4), "an unhandled SGR code disturbed the foreground")
  end)
end)

describe("Tests telnet subnegotiation handling", function()

  it("drops an oversized subnegotiation until IAC SE and resumes after it", function()
    -- MAX_TELNET_SUBNEGOTIATION_LENGTH in ctelnet.cpp is 5MB; send more than
    -- that inside the subnegotiation, with no IAC SE, then a marker that (only
    -- if recovery is broken) would leak into the display, then the real IAC SE
    -- and a line of ordinary text
    local payload = string.rep("A", 5 * 1024 * 1024 + 1024)
    local mark = getLastLineNumber("main")
    -- 0x2d is not an option Mudlet handles, so nothing downstream acts on it
    local ok, msg = feedTelnet("<T_IAC><T_SB>" .. string.char(0x2d) .. payload
      .. "SUBNEG_LEAK_MARKER<T_IAC><T_SE>SUBNEG_RECOVERED\r\n")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))

    -- everything the feed produced, joined: a leaked payload is one unbroken run
    -- of characters that the console wraps into lines, and the break can land
    -- inside the marker
    local displayed = table.concat(getLines("main", mark, getLastLineNumber("main") + 1))

    assert.is_truthy(displayed:find("SUBNEG_RECOVERED", 1, true), "ordinary text after an oversized subnegotiation was not displayed - recovery failed")
    assert.is_nil(displayed:find("SUBNEG_LEAK_MARKER", 1, true), "subnegotiation payload past the size cap leaked into the display instead of being dropped until IAC SE")
  end)
end)

describe("Tests MCCP decompression", function()
  local function bigEndian32(value)
    return string.char(math.floor(value / 16777216) % 256, math.floor(value / 65536) % 256, math.floor(value / 256) % 256, value % 256)
  end

  local function adler32(data)
    local a, b = 1, 0
    for i = 1, #data do
      a = (a + data:byte(i)) % 65521
      b = (b + a) % 65521
    end
    return b * 65536 + a
  end

  -- a zlib stream of one stored (uncompressed) deflate block, so no zlib is
  -- needed here to build one
  local function zlibStored(data)
    local length = #data
    local inverse = 65535 - length
    return "\120\1\1" .. string.char(length % 256, math.floor(length / 256), inverse % 256, math.floor(inverse / 256)) .. data .. bigEndian32(adler32(data))
  end

  -- once the outer stream ends, its decompressed bytes are scanned for telnet
  -- sequences, and inflating a second stream found there back into the same
  -- buffer overlapped source and destination (#10662)
  local function feedNestedStream(option, start)
    local nested = "AAAAAAAA" .. start .. zlibStored(string.rep("B", 1000)) .. "\r\n"
    local stream = "\255\251" .. option .. start .. zlibStored(nested) .. "MCCP_AFTER_NESTED\r\n"
    -- feedTelnet() stops at a NUL and reads <...> as the name of a telnet code
    assert.is_nil(stream:find("[%z<>]"), "the crafted stream carries a byte feedTelnet() would not pass through")
    finally(function() feedTelnet("\255\252" .. option) end)
    local mark = getLastLineNumber("main")

    local ok, msg = feedTelnet(stream)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))

    local displayed = table.concat(getLines("main", mark, getLastLineNumber("main") + 1))
    assert.is_truthy(displayed:find("AAAAAAAA", 1, true), "the decompressed text before the nested start sequence was lost")
    assert.is_truthy(displayed:find("MCCP_AFTER_NESTED", 1, true), "the plain text after the compressed stream was lost")
  end

  it("does not restart MCCP2 compression from inside data it has just decompressed", function()
    feedNestedStream("\86", "\255\250\86\255\240")
  end)

  -- the MCCP1 start sequence has no IAC before its SE, so it has to be skipped
  -- whole or the telnet parser is left inside a subnegotiation
  it("does not restart MCCP1 compression from inside data it has just decompressed", function()
    feedNestedStream("\85", "\255\250\85\251\240")
  end)
end)

describe("Tests MSDP subnegotiation handling", function()
  -- MSDP forbids only NUL, IAC and its own six markers inside a value, so every
  -- other control code is legal payload the game means to send. Mudlet turns the
  -- payload into JSON for the Lua decoder, which rejects the whole document
  -- rather than one value if a control character arrives unescaped.
  local VAR, VAL = "<01>", "<02>"
  local TABLE_OPEN, TABLE_CLOSE = "<03>", "<04>"
  local ARRAY_OPEN, ARRAY_CLOSE = "<05>", "<06>"

  local function feedMsdp(payload)
    local ok, msg = feedTelnet("<T_IAC><T_SB><O_MSDP>" .. payload .. "<T_IAC><T_SE>")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  it("keeps a value carrying a control code, as the byte the game sent", function()
    local cases = {
      {"MSDPCTLBEL", "a<BELL>b", 7},
      {"MSDPCTLTAB", "a<HTAB>b", 9},
      {"MSDPCTLLF", "a<LF>b", 10},
      {"MSDPCTLVT", "a<VTAB>b", 11},
      {"MSDPCTLCR", "a<CR>b", 13},
      {"MSDPCTLESC", "a<ESC>b", 27},
    }
    for _, case in ipairs(cases) do
      local name, payload, byte = case[1], case[2], case[3]
      feedMsdp(VAR .. name .. VAL .. payload)
      assert.is_table(msdp, "no MSDP variable was registered at all")
      assert.is_not_nil(msdp[name], name .. " went missing, so its control code reached the decoder unescaped")
      -- the byte itself, not text describing it: BEL and ESC used to arrive as a
      -- literal "\007" and "\027", which is not what the game sent
      assert.equals("a" .. string.char(byte) .. "b", msdp[name], name .. " did not arrive as the byte the game sent")
    end
  end)

  it("does not read a backslash in a value as an escape sequence", function()
    -- a lone backslash reached the decoder as the start of an escape, so "a\\b"
    -- arrived as an "a" followed by a backspace
    feedMsdp(VAR .. "MSDPSLASH" .. VAL .. "a\\b")
    assert.equals("a\\b", msdp.MSDPSLASH)
  end)

  it("keeps the rest of a table when one value carries a control code", function()
    -- the case that bites in play: a room description with a newline in it used
    -- to take the room's name and exits down with it
    feedMsdp(VAR .. "MSDPROOM" .. VAL .. TABLE_OPEN
             .. VAR .. "NAME" .. VAL .. "The Hall"
             .. VAR .. "DESC" .. VAL .. "line one<LF>line two"
             .. VAR .. "EXITS" .. VAL .. "north"
             .. TABLE_CLOSE)

    assert.is_table(msdp.MSDPROOM, "the whole table went missing, not only the value holding the newline")
    assert.equals("The Hall", msdp.MSDPROOM.NAME)
    assert.equals("north", msdp.MSDPROOM.EXITS)
    assert.equals("line one\nline two", msdp.MSDPROOM.DESC)
  end)

    -- The specification restricts a name to [A-Za-z_][A-Za-z0-9_]*, so none of the
    -- names below is one a conforming server can send. They are here because the
    -- prefix strip sized itself from the raw name while the JSON carried the escaped
    -- one, so a name that grew when escaped left part of its own prefix behind and
    -- took the variable down with it.
    it("keeps a variable whose name carries a control code", function()
      for _, case in ipairs({{"MSDPNTAB", 9}, {"MSDPNCR", 13}, {"MSDPNBEL", 7}}) do
        local name = case[1] .. string.char(case[2]) .. "X"
        feedMsdp(VAR .. name .. VAL .. "value")
        assert.equals("value", msdp[name], (name:gsub("%c", "?")) .. " went missing, so the strip and the escaped name disagree")
      end
    end)

    it("keeps a variable whose name carries a backslash", function()
      -- two bytes longer once escaped, and the key is the byte the game sent rather
      -- than the escape the JSON needed
      feedMsdp(VAR .. "MSDPNSLASHX" .. "\\Y" .. VAL .. "value")
      assert.equals("value", msdp["MSDPNSLASHX\\Y"])
    end)

    it("keeps a non-ASCII value as the bytes the game sent", function()
      -- char is signed on some targets, so every byte >= 0x80 reads as below 0x20 at
      -- the escape test and arrives as \u00xx mojibake without the cast there
      -- decimal escapes: Lua 5.1 has no \x form, and "\xC3" there is a literal
      -- x, C, 3 - which would make this spec pass on ASCII and prove nothing
      feedMsdp(VAR .. "MSDPUTF8" .. VAL .. "caf\195\169")
      assert.equals("caf\195\169", msdp.MSDPUTF8)
    end)

    it("keeps every variable of a batched update, as a string", function()
      -- a server answering REPORT sends many at once. Flushing one variable threw
      -- away the opening quote of the next name, so a numeric value came back a
      -- number and a non-numeric one went missing entirely.
      feedMsdp(VAR .. "MSDPB1" .. VAL .. "1200"
               .. VAR .. "MSDPB2" .. VAL .. "1500"
               .. VAR .. "MSDPB3" .. VAL .. "Market Square"
               .. VAR .. "MSDPB4" .. VAL .. "a rat")
      assert.equals("1200", msdp.MSDPB1)
      assert.equals("1500", msdp.MSDPB2, "a numeric value mid-batch came back as a " .. type(msdp.MSDPB2))
      assert.equals("Market Square", msdp.MSDPB3, "a non-numeric value mid-batch went missing")
      assert.equals("a rat", msdp.MSDPB4)
    end)

    it("keeps a table's shape when it holds an array of two or more elements", function()
      -- two adjacent values inside an explicit array look, from inside, like the
      -- top-level unmarked-list pattern, and used to make the whole variable gain
      -- an array level it never had
      feedMsdp(VAR .. "MSDPSHAPE" .. VAL .. TABLE_OPEN
               .. VAR .. "L" .. VAL .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. VAR .. "Z" .. VAL .. "plain"
               .. TABLE_CLOSE)
      assert.is_table(msdp.MSDPSHAPE, "the variable went missing entirely")
      assert.equals("plain", msdp.MSDPSHAPE.Z, "Z is not reachable, so the table gained a spurious array level")
      assert.same({"a", "b"}, msdp.MSDPSHAPE.L)
      assert.is_nil(msdp.MSDPSHAPE[1], "the whole table was wrapped in an array it never had")
    end)

    -- An array's elements are siblings, and an element may itself be a table or
    -- an array. Games differ over whether each element carries a value marker of
    -- its own, so both forms appear below.
    it("keeps an array of tables, the form games send for a group roster", function()
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPGROUP", function() fired = true end)
      feedMsdp(VAR .. "MSDPGROUP" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. VAR .. "HEALTH" .. VAL .. "90" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Barney" .. VAR .. "HEALTH" .. VAL .. "50" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      killAnonymousEventHandler(id)
      assert.same({{NAME = "Fred", HEALTH = "90"}, {NAME = "Barney", HEALTH = "50"}}, msdp.MSDPGROUP,
                  "the whole variable went missing, so the array of tables never reached Lua")
      assert.is_true(fired, "the roster arrived without raising its event, so no script would hear about it")
    end)

    it("keeps an array of tables whose elements each carry a value marker", function()
      -- the form the specification spells out, which needed no separator of its
      -- own and is here so the fix keeps working for it
      feedMsdp(VAR .. "MSDPGROUPV" .. VAL .. ARRAY_OPEN
               .. VAL .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. VAL .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Barney" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fred"}, {NAME = "Barney"}}, msdp.MSDPGROUPV, "the whole variable went missing")
    end)

    it("keeps an array holding a single table", function()
      -- one of the nested shapes that always worked: nothing precedes the table,
      -- so no separator was ever needed - it guards against the fix overcorrecting
      feedMsdp(VAR .. "MSDPSOLOT" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fred"}}, msdp.MSDPSOLOT)
    end)

    it("keeps an array of arrays", function()
      feedMsdp(VAR .. "MSDPMATRIX" .. VAL .. ARRAY_OPEN
               .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. ARRAY_OPEN .. VAL .. "c" .. ARRAY_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{"a", "b"}, {"c"}}, msdp.MSDPMATRIX,
                  "the whole variable went missing, so the nested arrays never reached Lua")
    end)

    it("keeps an array that mixes a plain value with a table", function()
      feedMsdp(VAR .. "MSDPMIXED" .. VAL .. ARRAY_OPEN
               .. VAL .. "solo"
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. VAL .. "tail"
               .. ARRAY_CLOSE)
      assert.same({"solo", {NAME = "Fred"}, "tail"}, msdp.MSDPMIXED,
                  "the values around the nested table lost their place in the array")
    end)

    -- An array may hold elements of different kinds, and each pairing is its own
    -- adjacency for the separator: a value before an array, a table before an
    -- array, an array before a table.
    it("keeps an array whose element after a plain value is an array", function()
      feedMsdp(VAR .. "MSDPVALARR" .. VAL .. ARRAY_OPEN
               .. VAL .. "solo"
               .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. ARRAY_CLOSE)
      assert.same({"solo", {"a", "b"}}, msdp.MSDPVALARR,
                  "the value ahead of the nested array lost its place in the array")
    end)

    it("keeps an array whose element after a table is an array", function()
      feedMsdp(VAR .. "MSDPTABARR" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fred"}, {"a", "b"}}, msdp.MSDPTABARR,
                  "the array following a table in the same array never reached Lua")
    end)

    it("keeps an array whose element after an array is a table", function()
      feedMsdp(VAR .. "MSDPARRTAB" .. VAL .. ARRAY_OPEN
               .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{"a", "b"}, {NAME = "Fred"}}, msdp.MSDPARRTAB,
                  "the table following an array in the same array never reached Lua")
    end)

    it("keeps a table whose values are arrays of tables", function()
      feedMsdp(VAR .. "MSDPDEEP" .. VAL .. TABLE_OPEN
               .. VAR .. "MEMBERS" .. VAL .. ARRAY_OPEN
                 .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
                 .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Barney" .. TABLE_CLOSE
               .. ARRAY_CLOSE
               .. VAR .. "LEADER" .. VAL .. "Fred"
               .. TABLE_CLOSE)
      assert.same({MEMBERS = {{NAME = "Fred"}, {NAME = "Barney"}}, LEADER = "Fred"}, msdp.MSDPDEEP,
                  "the array of tables, or the key after it, did not survive the nesting")
    end)

    it("keeps a table holding two tables side by side", function()
      feedMsdp(VAR .. "MSDPROOMS" .. VAL .. TABLE_OPEN
               .. VAR .. "HERE" .. VAL .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "The Hall" .. TABLE_CLOSE
               .. VAR .. "NORTH" .. VAL .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "The Study" .. TABLE_CLOSE
               .. TABLE_CLOSE)
      assert.same({HERE = {NAME = "The Hall"}, NORTH = {NAME = "The Study"}}, msdp.MSDPROOMS,
                  "the second table in the row did not reach Lua")
    end)

    it("keeps the characters JSON escapes inside a nested value", function()
      -- names in a roster carry colour codes and punctuation, so the escaping
      -- branches and the new separator meet on the very shape games send
      feedMsdp(VAR .. "MSDPESCGROUP" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fr\"ed" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Bar\\ney<ESC>x" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fr\"ed"}, {NAME = "Bar\\ney" .. string.char(27) .. "x"}}, msdp.MSDPESCGROUP,
                  "a quote, a backslash or an escape character in a nested value took the variable down with it")
    end)

    it("replaces a roster that is sent again with fewer members", function()
      -- a game re-sends the whole list on every change, and the interesting
      -- direction is shrinking: a member who left must not linger
      feedMsdp(VAR .. "MSDPPARTY" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Barney" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Wilma" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fred"}, {NAME = "Barney"}, {NAME = "Wilma"}}, msdp.MSDPPARTY,
                  "the first roster did not arrive with all three of its members")

      feedMsdp(VAR .. "MSDPPARTY" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Wilma" .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{NAME = "Fred"}, {NAME = "Wilma"}}, msdp.MSDPPARTY,
                  "the member who left the roster is still in the table")
    end)

    it("keeps an empty table", function()
      feedMsdp(VAR .. "MSDPEMPTYT" .. VAL .. TABLE_OPEN .. TABLE_CLOSE)
      assert.same({}, msdp.MSDPEMPTYT, "an empty table did not arrive as an empty table")
    end)

    it("keeps an empty array", function()
      feedMsdp(VAR .. "MSDPEMPTYA" .. VAL .. ARRAY_OPEN .. ARRAY_CLOSE)
      assert.same({}, msdp.MSDPEMPTYA, "an empty array did not arrive as an empty table")
    end)

    it("keeps an array of empty tables", function()
      feedMsdp(VAR .. "MSDPEMPTIES" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. TABLE_CLOSE .. TABLE_OPEN .. TABLE_CLOSE
               .. ARRAY_CLOSE)
      assert.same({{}, {}}, msdp.MSDPEMPTIES, "the array did not arrive with both of its empty tables")
    end)

    -- A server answering REPORT sends the lot at once, so a structure is rarely
    -- the last thing in a message. One case per shape rather than three feeds in
    -- one, so a failure of any of them is its own report.
    it("keeps a table and the variable that follows it in the same message", function()
      feedMsdp(VAR .. "MSDPPAIRT" .. VAL .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "The Hall" .. TABLE_CLOSE
               .. VAR .. "MSDPAFTERT" .. VAL .. "after")
      assert.same({NAME = "The Hall"}, msdp.MSDPPAIRT, "the table went missing because another variable followed it")
      assert.equals("after", msdp.MSDPAFTERT, "the variable after the table went missing")
    end)

    it("keeps an array and the variable that follows it in the same message", function()
      feedMsdp(VAR .. "MSDPPAIRA" .. VAL .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. VAR .. "MSDPAFTERA" .. VAL .. "after")
      assert.same({"a", "b"}, msdp.MSDPPAIRA, "the array went missing because another variable followed it")
      assert.equals("after", msdp.MSDPAFTERA, "the variable after the array went missing")
    end)

    it("keeps an array of tables and the variable that follows it in the same message", function()
      feedMsdp(VAR .. "MSDPPAIRG" .. VAL .. ARRAY_OPEN
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Fred" .. TABLE_CLOSE
               .. TABLE_OPEN .. VAR .. "NAME" .. VAL .. "Barney" .. TABLE_CLOSE
               .. ARRAY_CLOSE
               .. VAR .. "MSDPAFTERG" .. VAL .. "after")
      assert.same({{NAME = "Fred"}, {NAME = "Barney"}}, msdp.MSDPPAIRG,
                  "the array of tables went missing because another variable followed it")
      assert.equals("after", msdp.MSDPAFTERG, "the variable after the array of tables went missing")
    end)

    it("keeps an unmarked list that holds an array, and the variable after it", function()
      -- the list the specification allows for command-like variables, with an
      -- array as one of its entries: the wrap has to close around the structure,
      -- not around a value left hanging open
      feedMsdp(VAR .. "MSDPCMDLIST" .. VAL .. "look"
               .. VAL .. ARRAY_OPEN .. VAL .. "a" .. VAL .. "b" .. ARRAY_CLOSE
               .. VAR .. "MSDPAFTERLIST" .. VAL .. "after")
      assert.same({"look", {"a", "b"}}, msdp.MSDPCMDLIST,
                  "the unmarked list holding an array never reached Lua")
      assert.equals("after", msdp.MSDPAFTERLIST, "the variable after the unmarked list went missing")
    end)

    it("still turns adjacent top-level values into a list", function()
      -- the specification allows "string values together for command-like
      -- variables" with no array markers, and that is the one case the wrap is for
      feedMsdp(VAR .. "MSDPCMD" .. VAL .. "alpha" .. VAL .. "beta")
      assert.same({"alpha", "beta"}, msdp.MSDPCMD)
    end)

    it("wraps only the variable that was an unmarked list", function()
      -- one flag used to serve the whole subnegotiation, so a list flushed
      -- mid-message went out unwrapped and the wrap landed on whichever variable
      -- came last instead
      feedMsdp(VAR .. "MSDPLIST" .. VAL .. "a" .. VAL .. "b" .. VAR .. "MSDPSOLO" .. VAL .. "solo")
      assert.same({"a", "b"}, msdp.MSDPLIST)
      assert.equals("solo", msdp.MSDPSOLO, "the unmarked-list wrap leaked onto the wrong variable")
    end)

    it("drops a variable whose table the game never closed, without an event", function()
      -- IAC SE already arrived, so there is no more data coming for this message:
      -- an open structure at its end is malformed, and what yajl makes of the
      -- truncated JSON varies by version - from a silent nothing to a stored null
      -- sentinel a script can trip over
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPCUT", function() fired = true end)
      feedMsdp(VAR .. "MSDPWHOLE" .. VAL .. "fine" .. VAR .. "MSDPCUT" .. VAL .. TABLE_OPEN)
      killAnonymousEventHandler(id)
      assert.equals("fine", msdp.MSDPWHOLE, "the complete variable ahead of the truncated one has to survive")
      assert.is_nil(msdp.MSDPCUT)
      assert.is_false(fired, "a variable the game never finished sending raised its arrival event")
    end)

    it("yields no variable and no event when the game closes a table it never opened", function()
      -- the drop and a failed decode are indistinguishable from Lua: both leave the
      -- variable unset and silent. What the drop adds is a named diagnostic and the
      -- same outcome on every yajl version, and MsdpMalformedDiagnosticTest covers that
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPOVER", function() fired = true end)
      feedMsdp(VAR .. "MSDPOVER" .. VAL .. TABLE_CLOSE .. VAR .. "MSDPNEXT" .. VAL .. "ok")
      killAnonymousEventHandler(id)
      assert.is_nil(msdp.MSDPOVER)
      assert.is_false(fired, "a variable with an unbalanced close raised its arrival event")
      assert.equals("ok", msdp.MSDPNEXT, "the malformed variable took the rest of the message with it")
    end)

    it("yields no variable and no event when an unbalanced close ends the message", function()
      -- the end flush rather than the mid-loop one; as above, this asserts the
      -- outcome both mechanisms share, not the drop itself
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPSOLE", function() fired = true end)
      feedMsdp(VAR .. "MSDPSOLE" .. VAL .. TABLE_CLOSE)
      killAnonymousEventHandler(id)
      assert.is_nil(msdp.MSDPSOLE)
      assert.is_false(fired, "a variable with an unbalanced close raised its arrival event")
    end)

    -- the close arrives on a variable that never got a value, so there is nothing to
    -- flush and nothing to clear the malformed flag - it used to survive to the next
    -- variable, dropping a well-formed one and naming it in the error. One case per
    -- marker rather than a loop over both, so a failure of either is its own report
    local function strayCloseKeepsTheNextVariable(marker, name)
      local fired = false
      local id = registerAnonymousEventHandler("msdp." .. name, function() fired = true end)
      feedMsdp(VAR .. "MSDPKEPT" .. VAL .. "1" .. VAR .. "MSDPVOID" .. marker .. VAR .. name .. VAL .. "3")
      killAnonymousEventHandler(id)
      assert.equals("3", msdp[name], name .. " was dropped, so the stray close was still blamed on it")
      assert.is_true(fired, name .. " arrived without raising its event")
    end

    it("keeps the variable after a stray table close, which used to take the blame", function()
      strayCloseKeepsTheNextVariable(TABLE_CLOSE, "MSDPSTRAYT")
    end)

    it("keeps the variable after a stray array close, which used to take the blame", function()
      strayCloseKeepsTheNextVariable(ARRAY_CLOSE, "MSDPSTRAYA")
    end)

    -- A marker that opens a table or an array is not legal inside a name or a
    -- value, so the two shapes below are malformed MSDP. Both used to arrive as
    -- something the game never sent - a variable named after an inner key, and a
    -- value with the markers swallowed into its text - and both now fail to
    -- decode, which is what the end of a message has always done with them.
    it("yields no variable and no event when a name runs straight into a table", function()
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPINNER", function() fired = true end)
      feedMsdp(VAR .. "MSDPKEPTN" .. VAL .. "x"
               .. VAR .. "MSDPRUNNAME" .. TABLE_OPEN .. VAR .. "MSDPINNER" .. VAL .. "v" .. TABLE_CLOSE)
      killAnonymousEventHandler(id)
      assert.equals("x", msdp.MSDPKEPTN, "the well-formed variable ahead of the malformed one has to survive")
      assert.is_nil(msdp.MSDPINNER, "a key from inside the malformed table was stored as a variable of its own")
      assert.is_nil(msdp.MSDPRUNNAME)
      assert.is_false(fired, "a variable the game never sent raised an arrival event")
    end)

    it("yields no variable and no event when a value runs straight into a table", function()
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPRUNVAL", function() fired = true end)
      feedMsdp(VAR .. "MSDPRUNVAL" .. VAL .. "a" .. TABLE_OPEN .. TABLE_CLOSE
               .. VAR .. "MSDPAFTERRUN" .. VAL .. "b")
      killAnonymousEventHandler(id)
      assert.is_nil(msdp.MSDPRUNVAL, "the table markers were swallowed into the value as if they were text")
      assert.is_false(fired, "a value that ran into a table marker raised an arrival event")
      assert.equals("b", msdp.MSDPAFTERRUN, "the malformed variable took the rest of the message with it")
    end)

    it("keeps the old value and stays silent when a decode fails", function()
      -- balanced markers can still make undecodable JSON (two roots), which slips
      -- past the structural checks and fails at the decoder - the arrival event
      -- used to fire anyway, handing every handler the stale value
      feedMsdp(VAR .. "MSDPSTALE" .. VAL .. "old")
      local fired = false
      local id = registerAnonymousEventHandler("msdp.MSDPSTALE", function() fired = true end)
      feedMsdp(VAR .. "MSDPSTALE" .. VAL .. TABLE_OPEN .. TABLE_CLOSE .. TABLE_OPEN .. TABLE_CLOSE)
      killAnonymousEventHandler(id)
      assert.equals("old", msdp.MSDPSTALE, "the failed decode overwrote the value it could not replace")
      assert.is_false(fired, "a decode failure raised the arrival event anyway")
    end)
end)

describe("Tests GMCP decode failures", function()
  it("raises no events for a GMCP message whose JSON cannot be decoded", function()
    -- the decode-failure path is shared with MSDP: no arrival events for a
    -- message that changed nothing
    local fired = false
    local id = registerAnonymousEventHandler("gmcp.Spec.BadJson", function() fired = true end)
    local ok, msg = feedTelnet("<T_IAC><T_SB><O_GMCP>Spec.BadJson {broken<T_IAC><T_SE>")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    killAnonymousEventHandler(id)
    assert.is_nil(gmcp.Spec and gmcp.Spec.BadJson)
    assert.is_false(fired, "a GMCP decode failure raised the arrival event anyway")
  end)
end)

describe("Tests how a GMCP message is split into name and data", function()

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  teardown(function()
    gmcp.Spec = nil
  end)

  it("gives a message that carries no data an empty table of its own", function()
    -- Core.Ping is the one every game sends, and a script reading gmcp.Core.Ping
    -- would break on a nil
    assert.is_nil(gmcp.Spec and gmcp.Spec.Ping)
    feed("<T_IAC><T_SB><O_GMCP>Spec.Ping<T_IAC><T_SE>")
    assert.same({}, gmcp.Spec.Ping)

    -- a name with nothing but the separator after it is the same case
    feed("<T_IAC><T_SB><O_GMCP>Spec.PingSpace <T_IAC><T_SE>")
    assert.same({}, gmcp.Spec.PingSpace)
  end)

  it("takes a newline as the separator when no space comes before it", function()
    assert.is_nil(gmcp.Spec and gmcp.Spec.NewlineSplit)
    feed("<T_IAC><T_SB><O_GMCP>Spec.NewlineSplit\n{\"a\": 1}<T_IAC><T_SE>")
    assert.same({a = 1}, gmcp.Spec.NewlineSplit)
  end)

  it("reads a payload the game spread over several lines", function()
    assert.is_nil(gmcp.Spec and gmcp.Spec.Pretty)
    feed("<T_IAC><T_SB><O_GMCP>Spec.Pretty {\r\n  \"b\": 2\r\n}<T_IAC><T_SE>")
    assert.same({b = 2}, gmcp.Spec.Pretty)
  end)

  it("keeps an escape character a game left raw inside a string", function()
    -- a raw ESC is not valid inside JSON, so without the escaping the decoder
    -- would reject the whole message
    assert.is_nil(gmcp.Spec and gmcp.Spec.Ansi)
    feed("<T_IAC><T_SB><O_GMCP>Spec.Ansi {\"t\": \"\27[31mred\"}<T_IAC><T_SE>")
    assert.same({t = "\27[31mred"}, gmcp.Spec.Ansi)
  end)

  it("reads a payload that is a bare number", function()
    pending("json_to_value returns a function rather than a number for a bare JSON number, so gmcp.X ends up holding a function (#10362)")
  end)
end)

describe("Tests the Client.GUI package offer", function()

  -- A game may send Client.GUI as JSON, or as the raw telnet form of a version
  -- and a URL on two lines. Most offers here only have to reach the
  -- downloader, so offerUrl() is one nothing answers: the harness's fixture
  -- server 404s it when it started one, a closed port refuses it when it did not.
  local httpPort = os.getenv("MUDLET_TEST_HTTP_PORT")

  local function offerUrl(packageName)
    local port = (httpPort and httpPort ~= "" and httpPort) or "1"
    return "http://127.0.0.1:" .. port .. "/not-served/" .. packageName .. ".mpackage"
  end

  local function displayedSince(mark)
    return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "\n")
  end

  -- the console wraps a line wherever it runs out of width, and these URLs are
  -- long enough to be split, so the whitespace comes out of both sides first
  local function shownSince(mark, text)
    return displayedSince(mark):gsub("%s+", ""):find((text:gsub("%s+", "")), 1, true)
  end

  local function downloadStarted(mark, packageName)
    return shownSince(mark, "Downloading and installing package '" .. packageName .. "'")
  end

  -- sends a Client.GUI message carrying this payload, and waits out any
  -- download it started for the named package
  local function offerGui(payload, packageName)
    local mark = getLastLineNumber("main")
    local ok, msg = feedTelnet("<T_IAC><T_SB><O_GMCP>Client.GUI " .. payload .. "<T_IAC><T_SE>")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    if downloadStarted(mark, packageName) then
      -- the download the offer started is this test's to finish: the next offer
      -- aborts a reply still in flight, and the failure message would otherwise
      -- land in a later test's window
      local drained = false
      for _ = 1, 150 do
        if shownSince(mark, "Package download failed from '" .. offerUrl(packageName) .. "'") then
          drained = true
          break
        end
        local pumped, why = pumpEvents(20)
        assert.is_true(pumped, "pumpEvents could not run the event loop: " .. tostring(why))
      end
      assert.is_true(drained, "the download of '" .. packageName .. "' has not finished, so it will land in a later test: " .. displayedSince(mark))
    end
    return mark
  end

  local function offerRawTelnetGui(packageName, version)
    return offerGui(version .. "\n" .. offerUrl(packageName), packageName)
  end

  it("acts on a Client.GUI offer sent as raw telnet rather than JSON (#7704)", function()
    local mark = offerRawTelnetGui("RegressRawGui", "7704")
    assert.is_truthy(downloadStarted(mark, "RegressRawGui"), "the raw telnet offer never reached the downloader: " .. displayedSince(mark))
  end)

  it("keeps a raw telnet Client.GUI out of the gmcp table (#7034)", function()
    -- an earlier spec may have left a gmcp.Client of its own behind, which would
    -- answer for this one
    local previousClient = gmcp.Client
    finally(function() gmcp.Client = previousClient end)
    gmcp.Client = nil
    local mark = offerRawTelnetGui("RegressRawGuiTable", "7034")
    -- the gmcp.Client check below would be just as happy with an offer that was
    -- ignored outright, so prove first that this one was acted on
    assert.is_truthy(downloadStarted(mark, "RegressRawGuiTable"), "the offer never reached the downloader")
    -- gmcp.Client.GUI stays nil either way, because parseJSON creates the parent
    -- table before it fails on the payload - only the parent tells the two apart
    assert.is_nil(gmcp.Client, "the raw telnet offer was pushed into the gmcp table")
  end)

  it("acts on a JSON Client.GUI offer whose version is a number", function()
    local previousClient = gmcp.Client
    finally(function() gmcp.Client = previousClient end)
    local mark = offerGui('{"version": 39, "url": "' .. offerUrl("RegressNumericGui") .. '"}', "RegressNumericGui")
    assert.is_truthy(downloadStarted(mark, "RegressNumericGui"), "an offer numbering its version was not acted on: " .. displayedSince(mark))
  end)

  it("ignores a Client.GUI offer that is missing its version or its URL", function()
    local previousClient = gmcp.Client
    finally(function() gmcp.Client = previousClient end)
    local mark = offerGui('{"url": "' .. offerUrl("RegressNoVersionGui") .. '"}', "RegressNoVersionGui")
    offerGui('{"version": "5"}', "RegressNoUrlGui")
    -- and the raw telnet form, with a version but no URL line, or the other way round
    offerGui("5", "RegressRawNoUrlGui")
    offerGui("\n" .. offerUrl("RegressRawNoVersionGui"), "RegressRawNoVersionGui")
    assert.is_falsy(shownSince(mark, "Downloading and installing package"), "an incomplete offer was acted on: " .. displayedSince(mark))

    -- the same offer made complete, so the silence above is about what was missing
    mark = offerGui('{"version": "5", "url": "' .. offerUrl("RegressCompleteGui") .. '"}', "RegressCompleteGui")
    assert.is_truthy(downloadStarted(mark, "RegressCompleteGui"), "a complete offer was not acted on either: " .. displayedSince(mark))
  end)

  local minimalGuiPackage = '<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE MudletPackage><MudletPackage version="1.001"></MudletPackage>'
  local servedGuiCleanups = {}

  -- busted keeps one finally() per test, so every offerServedGui() of a test is
  -- undone by this one, last first
  local function cleanUpServedGuis()
    for index = #servedGuiCleanups, 1, -1 do
      servedGuiCleanups[index]()
      servedGuiCleanups[index] = nil
    end
  end

  -- serves content as name.xml from a file: URL, offers it as the game's
  -- interface at this version and waits for the download to be settled one way
  -- or the other. afterOffer, if given, runs straight after the offer. The test
  -- passes cleanUpServedGuis to finally() first
  local function offerServedGui(name, content, version, afterOffer)
    local source = getMudletHomeDir() .. "/mudlet-spec-servedgui-source"
    local served = source .. "/" .. name .. ".xml"
    local downloaded = getMudletHomeDir() .. "/" .. name .. ".xml"
    local previousClient = gmcp.Client
    -- the starter UI stands aside on sysServerGuiInstalled and saves that in its
    -- settings, which are not this spec's to change
    local standAside = BaseUI and BaseUI.standAside
    local result = {guis = {}, listed = {}, installs = {}, downloaded = downloaded}
    local handlers = {
      registerAnonymousEventHandler("sysServerGuiInstalled", function(_, packageName)
        result.guis[#result.guis + 1] = packageName
        result.listed[#result.listed + 1] = table.contains(getPackages(), packageName)
      end),
      registerAnonymousEventHandler("sysInstallPackage", function(_, packageName)
        result.installs[#result.installs + 1] = packageName
      end),
    }
    servedGuiCleanups[#servedGuiCleanups + 1] = function()
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      gmcp.Client = previousClient
      if standAside then
        BaseUI.standAside = standAside
      end
      for _ = 1, 100 do
        if not table.contains(getPackages(), name) or uninstallPackage(name) then
          break
        end
        pumpEvents(50)
      end
      os.remove(downloaded)
      os.remove(served)
      lfs.rmdir(source)
    end
    if standAside then
      BaseUI.standAside = function() end
    end
    if lfs.attributes(source, "mode") ~= "directory" then
      assert(lfs.mkdir(source))
    end
    local file = io.open(served, "wb")
    assert.is_not_nil(file, "could not write " .. served)
    file:write(content)
    file:close()
    local path = served:gsub("\\", "/"):gsub("[%%#%?%s]", function(character) return string.format("%%%02X", character:byte()) end)
    -- A Windows path starts with its drive letter, which Qt would read as the URL's host
    if path:sub(1, 1) ~= "/" then
      path = "/" .. path
    end
    local url = "file://" .. path

    -- while a profile save runs, installPackage() postpones the install and
    -- answers true, and an upgrade cannot uninstall the old interface; it gives
    -- a save away by postponing even the empty path it would otherwise refuse
    for _ = 1, 100 do
      if installPackage("") == nil then
        break
      end
      local pumped, why = pumpEvents(50)
      assert.is_true(pumped, "pumpEvents could not run the event loop: " .. tostring(why))
    end
    assert.is_nil(installPackage(""), "a profile save was still running after 5s, so this offer's install would be postponed")

    local mark = getLastLineNumber("main")
    result.mark = mark
    local ok, msg = feedTelnet('<T_IAC><T_SB><O_GMCP>Client.GUI {"version": "' .. (version or "1") .. '", "url": "' .. url .. '"}<T_IAC><T_SE>')
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    assert.is_truthy(downloadStarted(mark, name), "the offer never reached the downloader: " .. displayedSince(mark))
    if afterOffer then
      afterOffer()
    end
    for _ = 1, 300 do
      -- sysInstallPackage is raised a pass after sysServerGuiInstalled
      if (#result.guis > 0 and #result.installs > 0) or shownSince(mark, "was empty") or shownSince(mark, "Package download failed") or shownSince(mark, "Failed to load package") then
        break
      end
      local pumped, why = pumpEvents(50)
      assert.is_true(pumped, "pumpEvents could not run the event loop: " .. tostring(why))
    end
    return result
  end

  it("installs nothing from a Client.GUI download that came back empty", function()
    local name = "mudlet-spec-emptygui"
    finally(cleanUpServedGuis)
    local result = offerServedGui(name, "")

    -- the URL is shown decoded, so only its end is matched
    assert.is_truthy(shownSince(result.mark, "/" .. name .. ".xml' was empty"), displayedSince(result.mark))
    assert.same({}, result.guis, "an empty download was announced as the game's interface")
    assert.same({}, result.installs, "an empty download was announced as an installed package")
    assert.is_false(table.contains(getPackages(), name), "an empty download was registered as a package")
    assert.is_nil(lfs.attributes(result.downloaded), "an empty download was written into the profile")
  end)

  -- the progress dialog closing on completion must not cancel the download it
  -- tracked: aborting a finished file: reply empties it
  it("installs a Client.GUI package downloaded from a file URL", function()
    local name = "mudlet-spec-servedgui"
    finally(cleanUpServedGuis)
    local result = offerServedGui(name, minimalGuiPackage)

    assert.same({name}, result.guis, "the downloaded interface was not announced: " .. displayedSince(result.mark))
    assert.same({name}, result.installs, displayedSince(result.mark))
    assert.is_true(table.contains(getPackages(), name), "the downloaded interface was not registered")
    assert.is_falsy(shownSince(result.mark, "was empty"), displayedSince(result.mark))
    assert.is_true((lfs.attributes(result.downloaded, "size") or 0) > 0, "the package was saved into the profile without its contents")
  end)

  -- the recorded version is the installed copy's, so offering that version again
  -- has nothing to do, where anything else would reinstall it at every login
  it("leaves an installed Client.GUI package alone when it is offered at the same version", function()
    local name = "mudlet-spec-currentgui"
    finally(cleanUpServedGuis)
    assert.same({name}, offerServedGui(name, minimalGuiPackage, "10660").guis)

    local mark = offerGui('{"version": "10660", "url": "' .. offerUrl(name) .. '"}', name)
    assert.is_falsy(shownSince(mark, "Upgrading the GUI"), displayedSince(mark))
    assert.is_falsy(downloadStarted(mark, name), displayedSince(mark))
  end)

  -- the version recorded for an interface that has since been uninstalled
  -- belongs to no installed copy, so it must not stand for one installed later
  it("forgets the recorded Client.GUI version once that package is uninstalled", function()
    local name = "mudlet-spec-forgottengui"
    finally(cleanUpServedGuis)
    -- a save postpones an install and refuses an uninstall, so each step waits one out first
    local function waitForSaveToPass()
      for _ = 1, 100 do
        if installPackage("") == nil then
          return
        end
        pumpEvents(50)
      end
    end
    local result = offerServedGui(name, minimalGuiPackage, "10662")
    assert.same({name}, result.guis)
    waitForSaveToPass()
    assert.is_true(uninstallPackage(name), "the served interface could not be uninstalled")
    waitForSaveToPass()
    assert.is_true(installPackage(result.downloaded), "the interface could not be put back by hand")
    waitForSaveToPass()
    assert.is_true(table.contains(getPackages(), name), "the interface put back by hand was not registered")

    local mark = offerGui('{"version": "10662", "url": "' .. offerUrl(name) .. '"}', name)
    assert.is_truthy(shownSince(mark, "Upgrading the GUI to new version '10662'"), displayedSince(mark))
  end)

  -- an upgrade has to replace the installed copy of the package offered, even
  -- when another interface was recorded since: otherwise every offer fails again
  it("upgrades a Client.GUI package after another one was installed", function()
    local switched, other = "mudlet-spec-switchedgui", "mudlet-spec-othergui"
    finally(cleanUpServedGuis)
    assert.same({switched}, offerServedGui(switched, minimalGuiPackage, "1").guis)
    assert.same({other}, offerServedGui(other, minimalGuiPackage, "2").guis)

    local result = offerServedGui(switched, minimalGuiPackage, "3")
    assert.is_truthy(shownSince(result.mark, "Upgrading the GUI to new version '3'"), displayedSince(result.mark))
    assert.same({switched}, result.guis, displayedSince(result.mark))
    assert.is_true(table.contains(getPackages(), other), "upgrading one interface uninstalled another")
  end)

  -- while a profile save runs installPackage() only queues the install, so
  -- announcing the package then would announce one that is not installed yet
  it("announces a Client.GUI package only once it is installed when a save was running", function()
    local name = "mudlet-spec-savedovergui"
    finally(cleanUpServedGuis)
    local result = offerServedGui(name, minimalGuiPackage, "1", function()
      assert.is_true(saveProfile())
    end)
    assert.same({name}, result.guis, displayedSince(result.mark))
    assert.same({true}, result.listed, "the interface was announced before it was installed")
  end)

  -- installing runs the package's scripts, and an offer handled meanwhile must
  -- not be taken for the package that has just installed
  it("records the Client.GUI package that installed, not an offer made while it installed", function()
    local name, offeredMeanwhile = "mudlet-spec-scriptedgui", "mudlet-spec-meanwhilegui"
    local mark = getLastLineNumber("main")
    finally(function()
      _G.mudletSpecOfferMeanwhile = nil
      cleanUpServedGuis()
    end)
    _G.mudletSpecOfferMeanwhile = function()
      _G.mudletSpecOfferMeanwhile = nil
      feedTelnet('<T_IAC><T_SB><O_GMCP>Client.GUI {"version": "2", "url": "' .. offerUrl(offeredMeanwhile) .. '"}<T_IAC><T_SE>')
    end
    local scripted = '<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE MudletPackage><MudletPackage version="1.001"><ScriptPackage>'
      .. '<Script isActive="yes" isFolder="no"><name>mudlet-spec-scriptedgui</name><packageName></packageName>'
      .. '<script>if mudletSpecOfferMeanwhile then mudletSpecOfferMeanwhile() end</script><eventHandlerList /></Script>'
      .. '</ScriptPackage></MudletPackage>'
    local result = offerServedGui(name, scripted, "1")
    assert.is_truthy(downloadStarted(mark, offeredMeanwhile), "the package's script made no offer: " .. displayedSince(mark))
    assert.same({name}, result.guis, displayedSince(result.mark))
    -- the offer made meanwhile is this test's to finish, as in offerGui()
    for _ = 1, 150 do
      if shownSince(mark, "Package download failed from '" .. offerUrl(offeredMeanwhile) .. "'") then
        break
      end
      pumpEvents(20)
    end
    assert.is_truthy(shownSince(mark, "Package download failed from '" .. offerUrl(offeredMeanwhile) .. "'"), displayedSince(mark))
  end)

  -- the recorded version is what decides whether an installed interface is
  -- offered an upgrade, so one that never arrived must not take its place
  it("records no version for a Client.GUI download that installed nothing", function()
    local installed = "mudlet-spec-keptgui"
    finally(cleanUpServedGuis)
    assert.same({installed}, offerServedGui(installed, minimalGuiPackage, "10660").guis)
    offerServedGui("mudlet-spec-emptiedgui", "", "10661")

    local result = offerServedGui(installed, minimalGuiPackage, "10661")
    assert.is_truthy(shownSince(result.mark, "Upgrading the GUI to new version '10661'"), displayedSince(result.mark))
    assert.same({installed}, result.guis, displayedSince(result.mark))
  end)

  -- a game with an interface of its own can decline the built-in starter UI,
  -- which Mudlet passes on as the event an installed interface raises
  it("raises sysServerGuiInstalled when the game declines the starter UI", function()
    local previousClient = gmcp.Client
    -- the starter UI stands aside on that event and saves it in its settings,
    -- which are not this spec's to change
    local standAside = BaseUI and BaseUI.standAside
    local seen = {}
    local handler
    finally(function()
      if handler then
        killAnonymousEventHandler(handler)
      end
      gmcp.Client = previousClient
      if standAside then
        BaseUI.standAside = standAside
      end
    end)
    if standAside then
      BaseUI.standAside = function() end
    end
    handler = registerAnonymousEventHandler("sysServerGuiInstalled", function(_, packageName)
      -- the gmcp table is already up to date by the time the event is raised
      seen[#seen + 1] = {packageName, gmcp.Client and gmcp.Client.GUI and gmcp.Client.GUI.baseui}
    end)

    for _, keep in ipairs({'{"baseui": true}', '{"baseui": "no"}', '{"other": false}'}) do
      offerGui(keep, "nothing")
    end
    assert.same({}, seen, "a game keeping the starter UI was taken to be declining it")

    offerGui('{"baseui": false}', "nothing")
    offerGui('{"baseui": " False "}', "nothing")
    assert.same({{nil, false}, {nil, " False "}}, seen)
  end)
end)

describe("Tests Discord GMCP routing", function()

  -- getDiscordState() is denied when the bundled discord-rpc library is not on
  -- the library search path, which the harness arranges, or when the profile has
  -- Discord switched off. The state text read back is Mudlet's own, so nothing
  -- has to be listening on Discord.
  local requireDiscord = os.getenv("MUDLET_TEST_REQUIRE_DISCORD")

  local function discordUnavailable()
    local state, denial = getDiscordState()
    if state ~= nil then
      return false
    end
    if requireDiscord then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_DISCORD is set but the Discord API is unavailable: " .. tostring(denial))
    end
    pending("the Discord API is unavailable: " .. tostring(denial))
    return true
  end

  it("hands a Discord GMCP message to Discord under the message name alone (#2284)", function()
    if discordUnavailable() then return end
    local previousState = getDiscordState()
    finally(function() setDiscordState(previousState) end)
    local ok, msg = feedTelnet("<T_IAC><T_SB><O_GMCP>External.Discord.Status {\"state\": \"RegressDiscordState\"}<T_IAC><T_SE>")
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    assert.equals("RegressDiscordState", getDiscordState(), "is this profile still set to show game details in its Discord settings?")
  end)
end)

describe("Tests addSupportedTelnetOption", function()
  -- Only the argument contract is reachable. What the call changes is the
  -- reply cTelnet writes back when the server offers that option - IAC DO
  -- rather than IAC DONT - and the option bit that goes with it, which nothing
  -- outside further negotiation and the editor's statistics report ever reads.
  -- Bytes sent to the server are not observable from Lua, and the report is
  -- reachable only from a button in the editor, so the acceptance itself
  -- cannot be asserted here.

  it("returns nothing for an option number it accepts", function()
    -- 137 is an option Mudlet has no handler for, so registering it only adds
    -- a map entry that no negotiation in this offline run will consult. Do not
    -- reach for a round number here: 200 is ATCP and 201 is GMCP.
    assert.is_nil(addSupportedTelnetOption(137))
  end)

  it("raises a Lua error when the option is missing or not a number", function()
    assert.has_error(function() addSupportedTelnetOption() end)
    assert.has_error(function() addSupportedTelnetOption("mssp") end)
  end)

  it("raises a Lua error for a number too large to be an option", function()
    -- there is no range check on the option itself, so the refusal that does
    -- exist is getVerifiedInt's, and it is the only one worth holding
    local ok, err = pcall(function() addSupportedTelnetOption(2 ^ 40) end)
    assert.is_false(ok)
    assert.is_truthy(tostring(err):find("integer over/under-flow", 1, true), tostring(err))
  end)
end)

describe("Tests the payload sysTelnetEvent hands over", function()
  -- Options Mudlet has no handler of its own for can carry binary data, so the
  -- payload has to reach Lua byte for byte, NULs and bytes from 0x80 up
  -- included. 140 is never registered, so the WILL below is refused rather
  -- than leaving an option negotiated for the specs that follow.
  local option = 140

  local function received(data)
    local seen = {}
    local handler = registerAnonymousEventHandler("sysTelnetEvent", function(_, kind, opt, payload)
      if opt == option then
        seen[kind] = payload
      end
    end)
    local ok, msg = feedTelnet(data)
    killAnonymousEventHandler(handler)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
    return seen
  end

  it("hands the subnegotiation payload over byte for byte", function()
    -- the IAC is doubled in the stream, as telnet escapes it
    local seen = received("<T_IAC><T_SB>" .. string.char(option) .. "AB<00>CD\195\169<T_IAC><T_IAC>EF<T_IAC><T_SE>")
    assert.equals("AB\0CD\195\169\255EF", seen[250])
  end)

  it("hands an empty subnegotiation over as an empty payload", function()
    -- not as its framing, which an escaped payload of those same five bytes would also give
    assert.equals("", received("<T_IAC><T_SB>" .. string.char(option) .. "<T_IAC><T_SE>")[250])
    assert.equals("\255\250\140\255\240", received("<T_IAC><T_SB>" .. string.char(option) .. "<T_IAC><T_IAC>\250\140<T_IAC><T_IAC>\240<T_IAC><T_SE>")[250])
  end)

  it("hands a command that is not a subnegotiation over whole", function()
    assert.equals("\255\251\140", received("<T_IAC><T_WILL>" .. string.char(option))[251])
  end)

  it("hands the payload over byte for byte to waitForEvent", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waitForEvent needs MUDLET_TEST_MODE")
      return
    end
    tempTimer(0, function()
      feedTelnet("<T_IAC><T_SB>" .. string.char(option) .. "AB<00>\195\169<T_IAC><T_IAC><T_IAC><T_SE>")
    end)
    local name, kind, opt, data = waitForEvent("sysTelnetEvent", 2000)
    assert.equals("sysTelnetEvent", name)
    assert.equals(250, kind)
    assert.equals(option, opt)
    assert.equals("AB\0\195\169\255", data)
  end)

  it("hands a replayed subnegotiation over as the live one was", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("letting the replay timer run needs MUDLET_TEST_MODE")
      return
    end
    local function bigEndian32(value)
      return string.char(math.floor(value / 16777216) % 256, math.floor(value / 65536) % 256, math.floor(value / 256) % 256, value % 256)
    end
    local wire = "\255\250" .. string.char(option) .. "AB\0CD\255\255EF\255\240"
    local replay = getMudletHomeDir() .. "/telnet-spec-subnegotiation-replay.dat"
    local file = assert(io.open(replay, "wb"))
    file:write(bigEndian32(0) .. bigEndian32(#wire) .. wire)
    file:close()
    local replayed
    local handler = registerAnonymousEventHandler("sysTelnetEvent", function(_, kind, opt, payload)
      if kind == 250 and opt == option then
        replayed = payload
      end
    end)
    finally(function()
      killAnonymousEventHandler(handler)
      os.remove(replay)
    end)

    assert.is_true(loadReplay(replay))
    for _ = 1, 40 do
      if replayed then
        break
      end
      pumpEvents(50)
    end
    -- whether a replay is running is application-wide, so let this one run out
    pumpEvents(200)
    assert.equals("AB\0CD\255EF", replayed)
  end)
end)

describe("Tests telnet option negotiation", function()

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- Mode codes 5, 6 and 7 become the processor's default for the rest of the
  -- connection and a DONT does not undo that, so anything that reaches one has to
  -- put the default back. The escape only counts while the option is up, and it
  -- has to be a read of its own: an ESC[#z arriving with MXP already off is what
  -- cTelnet reads as a game wanting MXP without negotiating, and that forces the
  -- processor on with its default locked to secure for good.
  local function restoreMxpDefaultMode()
    feed("<T_IAC><T_DO><O_MXP>")
    feed("\27[5z\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
  end

  -- Mudlet raises these from inside processSocketData, so they are all in by the
  -- time feedTelnet returns and nothing has to be waited for.
  local function protocolEventsFrom(data)
    local seen = {}
    local handlers = {}
    for _, event in ipairs({"sysProtocolEnabled", "sysProtocolDisabled", "sysProtocolRejected"}) do
      handlers[#handlers + 1] = registerAnonymousEventHandler(event, function(name, protocol)
        seen[#seen + 1] = name .. ":" .. protocol
      end)
    end
    feed(data)
    for _, handler in ipairs(handlers) do
      killAnonymousEventHandler(handler)
    end
    return seen
  end

  -- 102 is Aardwolf's channel, which the token table spells <O_AARDWULF>
  local options = {
    {"<O_GMCP>", "GMCP"},
    {"<O_MSSP>", "MSSP"},
    {"<O_MSDP>", "MSDP"},
    {"<O_MSP>", "MSP"},
    {"<O_MXP>", "MXP"},
    {"<O_NENV>", "NEW_ENVIRON"},
    {"<O_CHARS>", "CHARSET"},
    {"<O_AARDWULF>", "channel102"},
  }

  -- An assertion that stops a test between turning a protocol on and turning it
  -- off again would otherwise leave it on for the thousands of tests that follow.
  after_each(function()
    for _, option in ipairs(options) do
      feed("<T_IAC><T_DONT>" .. option[1])
    end
  end)

  teardown(function()
    if channel102 then
      channel102[5] = nil
    end
    if mssp then
      mssp.TELNETSPLITVAR = nil
    end
  end)

  it("takes up each protocol the server offers and drops it again on DONT", function()
    for _, option in ipairs(options) do
      local token, protocol = option[1], option[2]
      assert.same({"sysProtocolEnabled:" .. protocol}, protocolEventsFrom("<T_IAC><T_DO>" .. token))
      assert.same({"sysProtocolDisabled:" .. protocol}, protocolEventsFrom("<T_IAC><T_DONT>" .. token))
    end
  end)

  -- The events alone cannot tell a negotiated protocol from one that was already
  -- on, so these two go through a Lua call that refuses while the protocol is off
  it("only lets a script talk on channel 102 while the server has it enabled", function()
    local before, refusal = sendTelnetChannel102("ab")
    assert.is_nil(before)
    assert.is_truthy(tostring(refusal):find("102 subchannel support has not been enabled", 1, true), tostring(refusal))

    feed("<T_IAC><T_DO><O_AARDWULF>")
    assert.is_true(sendTelnetChannel102("ab"), "the channel stayed shut after the server enabled it")

    feed("<T_IAC><T_DONT><O_AARDWULF>")
    local after, refusedAgain = sendTelnetChannel102("ab")
    assert.is_nil(after, "the channel stayed open after the server withdrew it")
    assert.is_truthy(tostring(refusedAgain):find("102 subchannel support has not been enabled", 1, true), tostring(refusedAgain))
  end)

  it("only accepts MSP messages while the server has MSP enabled", function()
    local before, refusal = receiveMSP("!!SOUND(Off)")
    assert.is_nil(before)
    assert.is_truthy(tostring(refusal):find("MSP is not currently enabled", 1, true), tostring(refusal))

    feed("<T_IAC><T_DO><O_MSP>")
    assert.is_true(receiveMSP("!!SOUND(Off)"), "MSP messages were still refused after the server enabled MSP")

    feed("<T_IAC><T_DONT><O_MSP>")
    local after = receiveMSP("!!SOUND(Off)")
    assert.is_nil(after, "MSP messages were still accepted after the server withdrew MSP")
  end)

  it("turns an offer down when the profile has that protocol switched off", function()
    -- both halves in one test: the refusal on its own would pass just as well
    -- against a build that never enables MSSP at all
    local original = getConfig("enableMSSP")
    finally(function() setConfig("enableMSSP", original) end)
    setConfig("enableMSSP", false)
    assert.same({}, protocolEventsFrom("<T_IAC><T_DO><O_MSSP>"))

    setConfig("enableMSSP", true)
    assert.same({"sysProtocolEnabled:MSSP"}, protocolEventsFrom("<T_IAC><T_DO><O_MSSP>"))
    feed("<T_IAC><T_DONT><O_MSSP>")
  end)

  it("answers a NAWS offer either way round, following the profile setting", function()
    local original = getConfig("enableNAWS")
    finally(function()
      setConfig("enableNAWS", original)
      feed("<T_IAC><T_DONT><O_NAWS>")
    end)

    setConfig("enableNAWS", false)
    assert.same({"sysProtocolDisabled:NAWS"}, protocolEventsFrom("<T_IAC><T_DO><O_NAWS>"))

    setConfig("enableNAWS", true)
    assert.same({"sysProtocolEnabled:NAWS"}, protocolEventsFrom("<T_IAC><T_DO><O_NAWS>"))
  end)

  it("rejects the options that would take Mudlet out of line mode", function()
    -- Mudlet only ever sends whole lines, so both of these are refused from
    -- either direction rather than negotiated
    assert.same({"sysProtocolRejected:LINEMODE"}, protocolEventsFrom("<T_IAC><T_DO>" .. string.char(34)))
    assert.same({"sysProtocolRejected:LINEMODE"}, protocolEventsFrom("<T_IAC><T_WILL>" .. string.char(34)))
    assert.same({"sysProtocolRejected:SUPPRESS_GO_AHEAD"}, protocolEventsFrom("<T_IAC><T_WILL><O_SGA>"))
  end)

  it("acts on a negotiation whose bytes are split across two packets", function()
    -- a real socket read can end anywhere, including between the IAC and the
    -- command that follows it, so the parser has to carry the partial sequence
    local mark = getLastLineNumber("main")
    assert.same({}, protocolEventsFrom("SPLITNEGBEFORE\r\n<T_IAC>"), "a lone IAC was acted on before the rest of the command arrived")
    assert.same({"sysProtocolEnabled:MSSP"}, protocolEventsFrom("<T_DO><O_MSSP>SPLITNEGAFTER\r\n"))
    feed("<T_IAC><T_DONT><O_MSSP>")

    local displayed = table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "\n")
    assert.is_truthy(displayed:find("SPLITNEGBEFORE", 1, true), displayed)
    assert.is_truthy(displayed:find("SPLITNEGAFTER", 1, true), displayed)
  end)

  it("carries a subnegotiation split across two packets over to the next one", function()
    feed("<T_IAC><T_SB><O_MSSP><01>TELNETSPLITVAR<02>par")
    assert.is_nil(mssp and mssp.TELNETSPLITVAR, "the subnegotiation was acted on before its IAC SE arrived")
    feed("t2<T_IAC><T_SE>")
    assert.equals("part2", mssp.TELNETSPLITVAR)
  end)

  -- a variable with no value used to abandon the whole subnegotiation, so every
  -- variable a game sent behind a malformed one went missing
  it("keeps reading MSSP variables past one that arrives without a value (#4233)", function()
    finally(function()
      mssp.TELNETMSSPNOVALUE = nil
      mssp.TELNETMSSPAFTERBAD = nil
    end)

    feed("<T_IAC><T_SB><O_MSSP><01>TELNETMSSPNOVALUE<01>TELNETMSSPAFTERBAD<02>arrived<T_IAC><T_SE>")
    assert.is_nil(mssp.TELNETMSSPNOVALUE, "a variable with no value should not be recorded")
    assert.equals("arrived", mssp.TELNETMSSPAFTERBAD)
  end)

  it("displays nothing for the commands it answers on the wire", function()
    -- AYT and NOP produce no text of their own; a parser that lost track of
    -- them would leak 0xff and the command byte into the line instead
    local mark = getLastLineNumber("main")
    feed("AYTBEFORE\r\n<T_IAC><T_AYT><T_IAC><T_NOP>AYTAFTER\r\n")
    assert.same({"AYTBEFORE", "AYTAFTER"}, getLines("main", mark, getLastLineNumber("main")))
  end)

  it("hands the payload of a channel 102 subnegotiation to Lua as two numbers", function()
    feed("<T_IAC><T_DO><O_AARDWULF>")
    local seen
    local handler = registerAnonymousEventHandler("channel102Message", function(_, variable, value)
      seen = {variable, value}
    end)
    feed("<T_IAC><T_SB><O_AARDWULF>" .. string.char(5) .. string.char(3) .. "<T_IAC><T_SE>")
    killAnonymousEventHandler(handler)
    feed("<T_IAC><T_DONT><O_AARDWULF>")

    assert.same({5, 3}, seen)
    assert.equals(3, channel102[5])
  end)

  it("switches the MXP processor on for the rest of the connection", function()
    -- the MXP mode escapes only act on data flagged as coming from a server, so
    -- this is a path feedTriggers cannot reach at all
    assert.same({"sysProtocolEnabled:MXP"}, protocolEventsFrom("<T_IAC><T_DO><O_MXP>"))

    local mark = getLastLineNumber("main")
    feed("\27[1z<B>MXPTELNETBOLD</B>\r\n")
    assert.same({"MXPTELNETBOLD"}, getLines("main", mark, getLastLineNumber("main")))

    -- leaving the processor on would change how every later spec file's data is
    -- parsed, and only a DONT (or a fresh connection) turns it back off
    assert.same({"sysProtocolDisabled:MXP"}, protocolEventsFrom("<T_IAC><T_DONT><O_MXP>"))
  end)

  it("switches the MXP processor on from a subnegotiation, in open mode", function()
    -- some games negotiate nothing and just send IAC SB MXP IAC SE. That starts
    -- the processor in open mode, the initial default in the MXP spec, so the
    -- open tags are read without a mode switch and the rest wait for one
    finally(restoreMxpDefaultMode)
    assert.same({"sysProtocolEnabled:MXP"}, protocolEventsFrom("<T_IAC><T_SB><O_MXP><T_IAC><T_SE>"))

    local mark = getLastLineNumber("main")
    feed("<B>MXPSUBNEGDEFAULT</B><SEND href=\"x\">N</SEND>\r\n")
    assert.same({"MXPSUBNEGDEFAULT<SEND href=\"x\">N</SEND>"}, getLines("main", mark, getLastLineNumber("main")))

    mark = getLastLineNumber("main")
    feed("\27[1z<SEND href=\"x\">MXPSUBNEGSECURE</SEND>\r\n")
    assert.same({"MXPSUBNEGSECURE"}, getLines("main", mark, getLastLineNumber("main")))

    mark = getLastLineNumber("main")
    feed("<B>MXPSUBNEGAFTER</B><SEND href=\"x\">N</SEND>\r\n")
    assert.same({"MXPSUBNEGAFTER<SEND href=\"x\">N</SEND>"}, getLines("main", mark, getLastLineNumber("main")))

    assert.same({"sysProtocolDisabled:MXP"}, protocolEventsFrom("<T_IAC><T_DONT><O_MXP>"))
  end)

  it("drops each protocol again when the server withdraws it with WONT", function()
    for _, option in ipairs(options) do
      local token, protocol = option[1], option[2]
      assert.same({"sysProtocolEnabled:" .. protocol}, protocolEventsFrom("<T_IAC><T_WILL>" .. token))
      assert.same({"sysProtocolDisabled:" .. protocol}, protocolEventsFrom("<T_IAC><T_WONT>" .. token))
    end
  end)

  it("stops acting on MSP and channel 102 once the server withdraws them with WONT", function()
    feed("<T_IAC><T_WILL><O_MSP>")
    assert.is_true(receiveMSP("!!SOUND(Off)"), "MSP messages were refused while the server had MSP enabled")
    feed("<T_IAC><T_WONT><O_MSP>")
    assert.is_nil(receiveMSP("!!SOUND(Off)"), "MSP messages were still accepted after the server's WONT")

    feed("<T_IAC><T_WILL><O_AARDWULF>")
    assert.is_true(sendTelnetChannel102("ab"), "the channel stayed shut while the server had it enabled")
    feed("<T_IAC><T_WONT><O_AARDWULF>")
    assert.is_nil(sendTelnetChannel102("ab"), "the channel stayed open after the server's WONT")
  end)

  -- A player can switch a protocol off while the game has it up. The next
  -- offer, either way round, has to take it down rather than leave it running.
  local switchable = {
    {"<O_GMCP>", "GMCP", "enableGMCP"},
    {"<O_MSSP>", "MSSP", "enableMSSP"},
    {"<O_MSDP>", "MSDP", "enableMSDP"},
    {"<O_MSP>", "MSP", "enableMSP"},
    {"<O_MXP>", "MXP", "enableMXP"},
    {"<O_NENV>", "NEW_ENVIRON", "enableNEWENVIRON"},
    {"<O_CHARS>", "CHARSET", "enableCHARSET"},
  }

  for _, command in ipairs({{"WILL", "<T_WILL>"}, {"DO", "<T_DO>"}}) do
    local name, token = command[1], command[2]
    it("takes a protocol down on " .. name .. " once the profile has it switched off", function()
      local originals = {}
      for _, option in ipairs(switchable) do
        originals[option[3]] = getConfig(option[3])
      end
      finally(function()
        for key, value in pairs(originals) do
          setConfig(key, value)
        end
      end)

      for _, option in ipairs(switchable) do
        local offer, protocol, key = "<T_IAC>" .. token .. option[1], option[2], option[3]
        setConfig(key, true)
        assert.same({"sysProtocolEnabled:" .. protocol}, protocolEventsFrom(offer))
        setConfig(key, false)
        assert.same({"sysProtocolDisabled:" .. protocol}, protocolEventsFrom(offer), protocol .. " was left up")
        -- and only the once, as it is no longer up to be taken down
        assert.same({}, protocolEventsFrom(offer), protocol .. " was taken down twice")
      end
    end)
  end

  -- ATCP is the protocol GMCP replaced, so it is only taken up while the
  -- profile has GMCP switched off
  it("takes up ATCP only while the profile has GMCP switched off", function()
    local original = getConfig("enableGMCP")
    finally(function()
      feed("<T_IAC><T_DONT><O_ATCP>")
      setConfig("enableGMCP", original)
    end)

    setConfig("enableGMCP", true)
    assert.same({}, protocolEventsFrom("<T_IAC><T_DO><O_ATCP>"))
    assert.same({}, protocolEventsFrom("<T_IAC><T_WILL><O_ATCP>"))

    setConfig("enableGMCP", false)
    assert.same({"sysProtocolEnabled:ATCP"}, protocolEventsFrom("<T_IAC><T_DO><O_ATCP>"))
    assert.same({"sysProtocolDisabled:ATCP"}, protocolEventsFrom("<T_IAC><T_WONT><O_ATCP>"))
    assert.same({"sysProtocolEnabled:ATCP"}, protocolEventsFrom("<T_IAC><T_WILL><O_ATCP>"))
    assert.same({"sysProtocolDisabled:ATCP"}, protocolEventsFrom("<T_IAC><T_DONT><O_ATCP>"))

    -- switching GMCP back on takes an ATCP that is still up down on the next offer
    for _, offer in ipairs({"<T_IAC><T_DO><O_ATCP>", "<T_IAC><T_WILL><O_ATCP>"}) do
      setConfig("enableGMCP", false)
      assert.same({"sysProtocolEnabled:ATCP"}, protocolEventsFrom(offer))
      setConfig("enableGMCP", true)
      assert.same({"sysProtocolDisabled:ATCP"}, protocolEventsFrom(offer))
    end
  end)

  -- a subnegotiation that runs straight into the next command has lost its
  -- IAC SE, and both halves are acted on rather than one swallowing the other
  -- (#4385)
  it("recovers a subnegotiation that runs into the next command without its IAC SE", function()
    finally(function() gmcp.SpecNoSE = nil end)
    assert.is_nil(gmcp.SpecNoSE)
    local mark = getLastLineNumber("main")
    local events = protocolEventsFrom("<T_IAC><T_SB><O_GMCP>SpecNoSE.Vitals {\"hp\": 7}<T_IAC><T_DO><O_MSSP>NOSEAFTER\r\n")
    assert.same({hp = 7}, gmcp.SpecNoSE and gmcp.SpecNoSE.Vitals)
    assert.same({"sysProtocolEnabled:MSSP"}, events, "the command that cut the subnegotiation short was lost")
    assert.same({"NOSEAFTER"}, getLines("main", mark, getLastLineNumber("main")))
  end)
end)

describe("Tests MCCP compressed streams", function()

  -- feedTelnet() turns "<..>" tokens into bytes, so a compressed stream has to
  -- reach it with its angle brackets doubled; its NULs go through as they are
  local function escaped(bytes)
    return (bytes:gsub("[<>]", {["<"] = "<<", [">"] = ">>"}))
  end

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local function linesSince(mark)
    return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "\n")
  end

  -- zlib.compress("MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK\r\n"),
  -- as the bytes a server would put on the wire after the start sequence
  local COMPRESSED = "\120\218\243\117\118\14\112\113\117\246\247\13\8\114\13\14\118\117\241\247\86\240\37\70\136\151\11\0\228\236\16\9"
  -- zlib.compress("MCCPSECONDSTREAMOK MCCPSECONDSTREAMOK\r\n")
  local COMPRESSED_SECOND = "\120\156\243\117\118\14\8\118\117\246\247\115\9\14\9\114\117\244\245\247\86\240\197\16\226\229\2\0\223\154\10\194"
  -- zlib.compress("MCCPVERSIONONEOK MCCPVERSIONONEOK\r\n")
  local COMPRESSED_V1 = "\120\156\243\117\118\14\8\115\13\10\246\244\247\243\247\115\245\247\86\240\69\19\224\229\2\0\183\122\9\194"

  -- the end of a stream does not clear the WILL, so without this every later
  -- spec's IAC SB is still a candidate MCCP start sequence
  after_each(function()
    feed("<T_IAC><T_WONT><O_MCCP2>")
    feed("<T_IAC><T_WONT><O_MCCP>")
  end)

  it("shows the text a server sends once it switches to MCCP v2", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(COMPRESSED) .. "MCCPPLAINAFTEREND\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK", 1, true), shown)
    -- the stream ended inside that read, so what follows it is plain text again
    assert.is_truthy(shown:find("MCCPPLAINAFTEREND", 1, true), shown)
  end)

  it("warns and falls back to plain text when the compressed stream is broken", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>MCCPNOTCOMPRESSED\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCP decompression error", 1, true), shown)
    -- the text was never compressed, so none of it may go missing into the
    -- stream header zlib tried to read out of it
    assert.is_truthy(shown:find("MCCPNOTCOMPRESSED", 1, true), shown)
    feed("MCCPPLAINAFTERBROKEN\r\n")
    shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCPPLAINAFTERBROKEN", 1, true), shown)
  end)

  -- the end of a stream arms a fresh inflate state for the next one, and a
  -- second start sequence used to allocate over it, leaking it (#10410) - only
  -- the leak detection half of the Linux CI job can see that
  it("decompresses a second stream once the first has ended", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(COMPRESSED))

    -- this time the stream arrives in a read of its own after the start sequence
    feed("MCCPSECONDSTART\r\n<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>")
    feed(escaped(COMPRESSED_SECOND))
    local shown = linesSince(mark)
    local first = shown:find("MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK", 1, true)
    local between = shown:find("MCCPSECONDSTART", 1, true)
    local second = shown:find("MCCPSECONDSTREAMOK MCCPSECONDSTREAMOK", 1, true)
    assert.is_truthy(first and between and second and first < between and between < second, shown)
  end)

  it("warns when a stream that started in an earlier read turns out to be broken", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>")
    feed("MCCPLATERBROKEN\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCP decompression error", 1, true), shown)
    assert.is_truthy(shown:find("MCCPLATERBROKEN", 1, true), shown)
  end)

  it("shows all of a text that arrives instead of a stream split across reads", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    -- zlib takes the first byte on its own as half of a stream header
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>M")
    feed("CCPSPLIT\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCP decompression error", 1, true), shown)
    -- once, and whole: the byte zlib held back must not be lost or shown twice
    local _, occurrences = shown:gsub("%f[%w]MCCPSPLIT", "")
    assert.equals(1, occurrences, shown)
  end)

  -- zlib reports a header asking for a preset dictionary without counting the
  -- six bytes it read for it, so those have to be kept track of another way
  it("shows all of a text that begins like a stream wanting a dictionary", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>8nMCCPDICT\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCP decompression error", 1, true), shown)
    assert.is_truthy(shown:find("8nMCCPDICT", 1, true), shown)
  end)

  -- a broken stream is refused on the wire, so a start sequence the game sends
  -- after that is not the start of a stream until it offers MCCP again
  it("stops decompressing after a broken stream until the game offers MCCP again", function()
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>MCCPNOTCOMPRESSED\r\n")

    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(COMPRESSED) .. "\r\n")
    local shown = linesSince(mark)
    assert.is_falsy(shown:find("MCCPDECOMPRESSEDOK", 1, true), "a refused stream was decompressed: " .. shown)

    mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(COMPRESSED))
    shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK MCCPDECOMPRESSEDOK", 1, true), "offering MCCP again did not bring it back: " .. shown)
  end)

  it("shows the text a server sends once it switches to MCCP v1", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP>")
    -- v1's start sequence is IAC SB COMPRESS WILL SE, not terminated by an IAC
    feed("<T_IAC><T_SB><O_MCCP><T_WILL><T_SE>" .. escaped(COMPRESSED_V1) .. "MCCPV1PLAINAFTER\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCPVERSIONONEOK MCCPVERSIONONEOK", 1, true), shown)
    assert.is_truthy(shown:find("MCCPV1PLAINAFTER", 1, true), shown)
  end)

  -- One read can inflate to more than the drain works through in one go: the
  -- rest has to be shown too, and what the game sends after it
  it("shows all of a stream that inflates past the depth of one drain", function()
    -- zlib.compress(string.rep("\0", 1000000) .. "MCCPTAILOFBOMB\r\n", 9): NULs
    -- are not displayed, so the depth is reached without drawing ~1 MB of text.
    -- It has to inflate to more than scmMaxDecompressionRecursion * BUFFER_SIZE
    -- (8 * 100000 bytes in ctelnet), or the drain never goes that deep.
    local bomb = "\120\218\237\193\209\9\0\16\20\0\64\223\202\80\40\165\188\248\176\255\44\6\113\119\41\1"
      .. string.rep("\0", 968)
      .. "\191\138\222\207\173\115\237\209\118\180\146\31\105\128\4\26"
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(bomb))
    -- the stream ended with the bomb, so this is plain text
    feed("MCCPPLAINAFTERBOMB\r\n")
    local shown = linesSince(mark)
    assert.is_truthy(shown:find("MCCPTAILOFBOMB", 1, true), shown)
    assert.is_falsy(shown:find("Too much compressed data to process at once", 1, true), shown)
    assert.is_falsy(shown:find("MCCP decompression error", 1, true), shown)
    assert.is_truthy(shown:find("MCCPPLAINAFTERBOMB", 1, true), shown)
  end)
end)

describe("Tests CHARSET negotiation", function()

  local encodingFile = getMudletHomeDir() .. "/encoding"
  local hadEncodingFile, originalEncoding

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- an RFC 2066 CHARSET REQUEST: a separator byte followed by the names on offer
  local function request(...)
    feed("<T_IAC><T_SB><O_CHARS><01>;" .. table.concat({...}, ";") .. "<T_IAC><T_SE>")
  end

  setup(function()
    -- setServerEncoding() writes the profile's "encoding" file, and a profile
    -- that never had one must not be left with one
    hadEncodingFile = lfs.attributes(encodingFile, "mode") ~= nil
    originalEncoding = getServerEncoding()
    feed("<T_IAC><T_DO><O_CHARS>")
  end)

  teardown(function()
    feed("<T_IAC><T_DONT><O_CHARS>")
    setServerEncoding(originalEncoding)
    if not hadEncodingFile then
      os.remove(encodingFile)
    end
  end)

  before_each(function()
    setServerEncoding("UTF-8")
  end)

  it("changes the game encoding to the character set the server offers", function()
    assert.equals("UTF-8", getServerEncoding())
    request("CP437")
    assert.equals("CP437", getServerEncoding())
  end)

  it("keeps the encoding already in use when the server offers it too", function()
    -- taking the first name on offer would let a game listing ASCII ahead of
    -- UTF-8 quietly downgrade a UTF-8 profile
    request("ASCII", "UTF-8")
    assert.equals("UTF-8", getServerEncoding())
  end)

  it("keeps the encoding when nothing on offer is one Mudlet knows", function()
    setServerEncoding("CP437")
    request("NOSUCHCHARSET", "ALSOUNKNOWN")
    assert.equals("CP437", getServerEncoding())

    -- the same request with one name Mudlet does know, so the assertion above is
    -- about the names rather than about the request never having been read
    request("NOSUCHCHARSET", "UTF-8")
    assert.equals("UTF-8", getServerEncoding())
  end)

  it("recognises a character set spelled the way a server writes it", function()
    -- Mudlet keys its table by "ISO 8859-2"; a server writes it with a hyphen
    request("ISO-8859-2")
    assert.equals("ISO 8859-2", getServerEncoding())
    setServerEncoding("UTF-8")
    request("ISO8859-2")
    assert.equals("ISO 8859-2", getServerEncoding())
  end)

  it("takes a variant spelling of ASCII as ASCII", function()
    request("US-ASCII")
    assert.equals("ASCII", getServerEncoding())
  end)

  it("ignores a request with nothing on offer after the separator", function()
    setServerEncoding("CP437")

    feed("<T_IAC><T_SB><O_CHARS><01>;<T_IAC><T_SE>")
    assert.equals("CP437", getServerEncoding())

    -- and the shorter form, a separator with nothing at all behind it: taking a
    -- separator character out of that payload would read past the end of it
    feed("<T_IAC><T_SB><O_CHARS><01><T_IAC><T_SE>")
    assert.equals("CP437", getServerEncoding())

    -- neither may leave the parser unable to act on the request that follows
    request("UTF-8")
    assert.equals("UTF-8", getServerEncoding())
  end)

  it("ignores a translate table it cannot use", function()
    setServerEncoding("CP437")
    -- Mudlet has no translate table support and answers TTABLE_REJECTED. The
    -- payload is a character set list Mudlet would act on were this a request,
    -- so a subcommand byte that went unchecked would show up as CP437 changing
    feed("<T_IAC><T_SB><O_CHARS><04>;UTF-8<T_IAC><T_SE>")
    assert.equals("CP437", getServerEncoding())
  end)

  it("ignores a request once the server has withdrawn CHARSET", function()
    feed("<T_IAC><T_DONT><O_CHARS>")
    request("CP437")
    assert.equals("UTF-8", getServerEncoding(), "a request was acted on after CHARSET was turned off")
    feed("<T_IAC><T_DO><O_CHARS>")
  end)

  it("ignores a request once the server has said it WONT do CHARSET", function()
    feed("<T_IAC><T_WONT><O_CHARS>")
    request("CP437")
    assert.equals("UTF-8", getServerEncoding(), "a request was acted on after the server's WONT")

    feed("<T_IAC><T_DO><O_CHARS>")
    request("CP437")
    assert.equals("CP437", getServerEncoding(), "offering CHARSET again did not bring it back")
  end)
end)

describe("Tests the encodings Mudlet carries its own tables for", function()
  -- CP437, CP667, CP737, CP869 and MEDIEVIA have no converter in Qt, so Mudlet
  -- transcodes them itself. Only the out-of-band and outgoing paths use those
  -- tables; the main display has its own copy.

  local encodingFile = getMudletHomeDir() .. "/encoding"
  local hadEncodingFile, originalEncoding

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- what an MSSP variable carrying these raw bytes arrives as, once cTelnet has
  -- transcoded the message out of the game's encoding. Cleared first: the table
  -- keeps whatever the last message put there, so a feed that is accepted but
  -- delivers no MSSP update would otherwise be answered with the previous
  -- encoding's reading - and two of the encodings below expect the same
  -- character, so that would pass.
  local function msspValueOf(bytes)
    if mssp then
      mssp.TELNETENCPROBE = nil
    end
    feed("<T_IAC><T_SB><O_MSSP><01>TELNETENCPROBE<02>" .. bytes .. "<T_IAC><T_SE>")
    return mssp and mssp.TELNETENCPROBE
  end

  -- the line feedTriggers put on the screen, or nil with the refusal if the
  -- encoding cannot carry the text at all
  local function displayed(text)
    local mark = getLastLineNumber("main")
    local ok, refusal = feedTriggers(text .. "\n", true)
    if not ok then
      return nil, refusal
    end
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "\n")
  end

  setup(function()
    hadEncodingFile = lfs.attributes(encodingFile, "mode") ~= nil
    originalEncoding = getServerEncoding()
  end)

  teardown(function()
    setServerEncoding(originalEncoding)
    if not hadEncodingFile then
      os.remove(encodingFile)
    end
    if mssp then
      mssp.TELNETENCPROBE = nil
    end
  end)

  it("reads an out-of-band message in the encoding the game is using", function()
    -- one byte, five encodings: a decoder that fell back to Latin-1 would
    -- answer "ã" every time
    local cases = {
      {"CP437", "π"},
      {"CP667", "π"},
      {"CP737", "ή"},
      {"CP869", "ι"},
      {"MEDIEVIA", "☠"},
    }
    for _, case in ipairs(cases) do
      local encoding, expected = case[1], case[2]
      assert.is_true(setServerEncoding(encoding))
      assert.equals(expected, msspValueOf(string.char(0xE3)), "byte 0xE3 came back wrong under " .. encoding)
    end
  end)

  -- ASCII is held as no encoding at all, and an out-of-band message is then
  -- read as UTF-8, so a game that sends UTF-8 anyway still gets its text through
  it("reads an out-of-band message as UTF-8 while the game encoding is ASCII", function()
    assert.is_true(setServerEncoding("ASCII"))
    assert.equals("ASCII", getServerEncoding())
    assert.equals("π", msspValueOf("\207\128"))
  end)

  it("sends a character the encoding has and refuses one it does not", function()
    -- the refusal is the half that bites: without it the character goes out as
    -- a question mark and the game never sees what was typed
    local cases = {
      {"CP437", "π", "ą"},
      {"CP667", "ą", "☠"},
      {"CP737", "ή", "ą"},
      {"CP869", "ι", "ß"},
      {"MEDIEVIA", "☠", "ą"},
    }
    for _, case in ipairs(cases) do
      local encoding, carried, refused = case[1], case[2], case[3]
      assert.is_true(setServerEncoding(encoding))

      local shown = displayed("TELNETENC" .. encoding .. " " .. carried)
      assert.is_truthy(shown, encoding .. " refused " .. carried .. ", which it can carry")
      assert.is_truthy(shown:find(carried, 1, true), encoding .. " did not round-trip " .. carried .. ": " .. shown)

      local sent, refusal = displayed("TELNETENC" .. encoding .. " " .. refused)
      assert.is_nil(sent, encoding .. " accepted " .. refused .. ", which it cannot carry")
      assert.is_truthy(tostring(refusal):find("current game server encoding of '" .. encoding .. "'", 1, true), tostring(refusal))
    end
  end)

  it("uses its own table for an encoding Qt spells differently", function()
    -- Mudlet keys this one "ISO 8859-2"; Qt's converters only answer to
    -- "ISO-8859-2", so the lookup table is what has to serve
    assert.is_true(setServerEncoding("ISO 8859-2"))
    assert.equals("ł", msspValueOf(string.char(0xB3)))

    local shown = displayed("TELNETENCISO2 ł")
    assert.is_truthy(shown and shown:find("ł", 1, true), tostring(shown))

    local sent, refusal = displayed("TELNETENCISO2 ☠")
    assert.is_nil(sent)
    assert.is_truthy(tostring(refusal):find("current game server encoding of 'ISO 8859-2'", 1, true), tostring(refusal))
  end)

  it("carries the accented letters of CP437", function()
    pending("TTextCodec_437's table holds Medievia's map glyphs from 0x80 to 0xAF, so CP437 can neither send nor read Ç, ü, é or Ü (#10395)")
  end)

  it("carries the lower case Greek letters of CP737", function()
    pending("both CP737 tables hold CP437's characters from 0x98 to 0xAF, so bytes for α to ψ come out as ÿ, Ö, á and friends (#10395)")
  end)
end)

describe("Tests MXP line modes", function()
  -- MXP_spec.lua carries its tags in through feedTriggers, which TBuffer does not
  -- flag as coming from a server, so the ESC[#z mode switches are inert there and
  -- every tag is parsed in the forced processor's secure mode. Only a negotiated
  -- processor fed server data runs the modes, which is what these pin.

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local function displayed(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "|")
  end

  setup(function()
    feed("<T_IAC><T_DO><O_MXP>")
  end)

  teardown(function()
    -- back to the open default the processor starts on, then off entirely: a
    -- locked-in secure mode would outlive this file otherwise
    feed("\27[5z\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
  end)

  it("acts only on the formatting tags while the line is open", function()
    assert.equals("MXPOPENBOLD", displayed("\27[0z<B>MXPOPENBOLD</B>\r\n"))
    assert.equals("MXPOPENFONT", displayed("\27[0z<FONT color=\"red\">MXPOPENFONT</FONT>\r\n"))
    assert.equals("<SEND href=\"x\">MXPOPENSEND</SEND>", displayed("\27[0z<SEND href=\"x\">MXPOPENSEND</SEND>\r\n"))
    assert.equals("<VERSION>", displayed("\27[0z<VERSION>\r\n"))
  end)

  it("acts on every tag while the line is secure", function()
    assert.equals("<SEND href=\"x\">MXPPLAINSEND</SEND>", displayed("\27[0z<SEND href=\"x\">MXPPLAINSEND</SEND>\r\n"))
    assert.equals("MXPSECURESEND", displayed("\27[1z<SEND href=\"x\">MXPSECURESEND</SEND>\r\n"))
  end)

  it("falls back to the default mode at the end of the line", function()
    assert.equals("MXPONELINE", displayed("\27[1z<SEND href=\"x\">MXPONELINE</SEND>\r\n"))
    assert.equals("<SEND href=\"x\">MXPNEXTLINE</SEND>", displayed("<SEND href=\"x\">MXPNEXTLINE</SEND>\r\n"))
  end)

  it("keeps a secure mode the server locked in until it unlocks it again", function()
    feed("\27[6z\r\n")
    assert.equals("MXPLOCKEDSECURE", displayed("<SEND href=\"x\">MXPLOCKEDSECURE</SEND>\r\n"))

    feed("\27[5z\r\n")
    assert.equals("<SEND href=\"x\">MXPUNLOCKED</SEND>", displayed("<SEND href=\"x\">MXPUNLOCKED</SEND>\r\n"))
  end)

  it("takes a temporary secure mode for one tag and no further", function()
    -- VERSION is a tag of its own with nothing to close, so the second one
    -- reaching the screen as text is the whole of what temp secure did not cover
    assert.equals("MXPTEMPSECURE<VERSION>", displayed("\27[4z<VERSION>MXPTEMPSECURE<VERSION>\r\n"))
  end)

  it("ignores a mode code it has no meaning for and carries on", function()
    assert.equals("MXPUNKNOWNMODE", displayed("\27[9zMXPUNKNOWNMODE\r\n"))
    assert.equals("MXPSTILLPARSING", displayed("\27[1z<B>MXPSTILLPARSING</B>\r\n"))
  end)
end)

describe("MXP auto-detection from the mode switch escape", function()

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local promptedBefore, forcedBefore

  before_each(function()
    promptedBefore = getConfig("promptForMXPProcessorOn")
    forcedBefore = getConfig("specialForceMXPProcessorOn")
    setConfig("promptForMXPProcessorOn", false)
    setConfig("specialForceMXPProcessorOn", false)
  end)

  -- detection turns MXP on for the rest of the connection as though the game
  -- had negotiated it, and only a WONT from the game turns that off again
  after_each(function()
    setConfig("specialForceMXPProcessorOn", forcedBefore)
    setConfig("promptForMXPProcessorOn", promptedBefore)
    feed("<T_IAC><T_WONT><O_MXP>")
  end)

  it("turns MXP on when the game sends a mode switch", function()
    feed("\27[1z<send>look</send>\r\n")
    assert.is_true(getConfig("promptForMXPProcessorOn"))
    assert.is_true(getConfig("specialForceMXPProcessorOn"))
  end)

  it("finds the switch in the middle of a read", function()
    feed("Welcome, adventurer.\r\nThe gates are open. \27[7zLocked open.\r\n")
    assert.is_true(getConfig("promptForMXPProcessorOn"))
  end)

  it("finds the switch when it ends the read", function()
    feed("prompt> \27[0z")
    assert.is_true(getConfig("promptForMXPProcessorOn"))
    feed("\r\n")
  end)

  -- each feedTelnet() is a read of its own, as two packets from the game are
  for cut = 1, 3 do
    it(("finds the switch when it is split between two reads after byte %d"):format(cut), function()
      local mark = getLastLineNumber("main")
      local switch = "\27[1z"
      feed(switch:sub(1, cut))
      feed(switch:sub(cut + 1) .. "<send>MXPSPLITLOOK</send>\r\n")
      assert.is_true(getConfig("promptForMXPProcessorOn"))
      -- and in time for the line it opens to be read as MXP
      local lines = getLines("main", mark, getLastLineNumber("main"))
      assert.equals("MXPSPLITLOOK", lines[#lines])
    end)
  end

  it("finds the switch when each of its bytes is a read of its own", function()
    for byte in ("\27[1z"):gmatch(".") do
      feed(byte)
    end
    feed("<send>MXPSPLITBYTES</send>\r\n")
    assert.is_true(getConfig("promptForMXPProcessorOn"))
  end)

  -- a read that is not scanned, here because MXP was negotiated, comes between
  -- the two, so they are not the two halves of one switch
  it("does not join a read to one from before MXP was negotiated and dropped", function()
    -- a profile with MXP off refuses the DO, and the middle read is then
    -- scanned like the other two, so nothing unscanned comes between them
    local enabledBefore = getConfig("enableMXP")
    setConfig("enableMXP", true)
    finally(function() setConfig("enableMXP", enabledBefore) end)
    local negotiated = false
    local handler = registerAnonymousEventHandler("sysProtocolEnabled", function(_, protocol)
      negotiated = negotiated or protocol == "MXP"
    end)
    feed("\27[1")
    feed("<T_IAC><T_DO><O_MXP>")
    killAnonymousEventHandler(handler)
    assert.is_true(negotiated, "the game's DO MXP was not taken up")
    feed("mMXPNEGOTIATEDREAD\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
    feed("z after it\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
    assert.is_false(getConfig("specialForceMXPProcessorOn"))
  end)

  -- the game pausing mid-switch for longer than the network packet timeout
  -- makes cTelnet flush what it holds, which must not lose the switch's start
  it("finds the switch when the posting timer flushes between its two reads", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waiting out the posting timeout needs MUDLET_TEST_MODE (pumpEvents() does nothing without it)")
      return
    end
    local quietFor = function()
      for _ = 1, 10 do
        assert.is_true(pumpEvents(50), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
      end
    end
    -- held until the timer flushes it, or the case would not reach the timer
    local mark = getLastLineNumber("main")
    feed("MXPTIMERCHECK")
    local heldBack = not table.contains(getLines("main", mark, getLastLineNumber("main") + 1), "MXPTIMERCHECK")
    quietFor()
    local flushed = table.contains(getLines("main", mark, getLastLineNumber("main") + 1), "MXPTIMERCHECK")
    feed("\r\n")
    if not (heldBack and flushed) then
      -- an IAC GA from an earlier spec stops the posting timer for the session
      pending("cTelnet's posting timer is not running in this session")
      return
    end
    feed("\27[1")
    quietFor()
    feed("z<send>MXPTIMERSPLIT</send>\r\n")
    assert.is_true(getConfig("promptForMXPProcessorOn"))
  end)

  -- the console joins an escape a read leaves unfinished to the next read, not
  -- to what a trigger the read fired feeds in the meantime. cTelnet posts a read
  -- in two places, depending on whether the game has sent an IAC GA yet, and
  -- once it has, that holds for the rest of the connection, so that run is last
  for _, gaDriven in ipairs({false, true}) do
    local after = gaDriven and " once the game has sent an IAC GA" or ""

    local function feedingTrigger(fedData)
      local fired = 0
      local id = tempExactMatchTrigger("MXPFEEDINGLINE", function()
        fired = fired + 1
        feedTelnet(fedData)
      end)
      if gaDriven then
        feed("<T_IAC><T_GA>")
      end
      return id, function() return fired end
    end

    it("finds the switch split around a read whose trigger feeds more data" .. after, function()
      local id, firings = feedingTrigger("fed by a trigger\r\n")
      finally(function() killTrigger(id) end)
      feed("MXPFEEDINGLINE\r\n\27[1")
      feed("z<send>MXPAFTERFEED</send>\r\n")
      assert.equals(1, firings(), "the trigger that feeds more data did not fire")
      assert.is_true(getConfig("promptForMXPProcessorOn"))
    end)

    it("does not join what a trigger feeds to the end of the read that fired it" .. after, function()
      local id, firings = feedingTrigger("z fed by a trigger\r\n")
      finally(function() killTrigger(id) end)
      feed("MXPFEEDINGLINE\r\n\27[1")
      feed("m after it\27[0m\r\n")
      assert.equals(1, firings(), "the trigger that feeds more data did not fire")
      assert.is_false(getConfig("promptForMXPProcessorOn"))
      assert.is_false(getConfig("specialForceMXPProcessorOn"))
    end)
  end

  -- a GA prompt that came while MXP was negotiated is not scanned, so it too
  -- comes between the reads either side of it
  it("does not join a read to one from before a prompt sent while MXP was negotiated", function()
    local enabledBefore = getConfig("enableMXP")
    setConfig("enableMXP", true)
    finally(function() setConfig("enableMXP", enabledBefore) end)
    local negotiated = false
    local handler = registerAnonymousEventHandler("sysProtocolEnabled", function(_, protocol)
      negotiated = negotiated or protocol == "MXP"
    end)
    feed("x\27[1")
    feed("<T_IAC><T_DO><O_MXP>")
    killAnonymousEventHandler(handler)
    assert.is_true(negotiated, "the game's DO MXP was not taken up")
    feed("HP> <T_IAC><T_GA>")
    feed("<T_IAC><T_DONT><O_MXP>")
    feed("z after it\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
    assert.is_false(getConfig("specialForceMXPProcessorOn"))
  end)

  it("ignores a mode number MXP does not define", function()
    feed("\27[8z<send>look</send>\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
    assert.is_false(getConfig("specialForceMXPProcessorOn"))
  end)

  -- the detection is ESC [ # z, not ESC anything # z: another escape family
  -- carrying the same two bytes after it must not turn MXP on
  it("ignores an escape that is not a control sequence introducer", function()
    feed("\27X1z<send>look</send>\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
    assert.is_false(getConfig("specialForceMXPProcessorOn"))
  end)

  it("ignores SGR sequences and a bracket that is not part of an escape", function()
    feed("\27[1mBold\27[0m and [1z in plain text\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
  end)

  -- connecting and disconnecting both reset cTelnet, so the reads either side
  -- of a connection are not the two halves of one switch
  it("does not join a read to one from before a connection", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waiting for the connection to come and go needs MUDLET_TEST_MODE (pumpEvents() does nothing without it)")
      return
    end
    local telnetDirectory = os.getenv("MUDLET_TEST_TELNET_DIR")
    local handle = telnetDirectory and io.open(telnetDirectory .. "/port", "r")
    local port = handle and tonumber((handle:read("*a") or ""):match("%d+") or "")
    if handle then
      handle:close()
    end
    if not port then
      local reason = "needs the telnet fixture (run CI/telnet-fixture-server.py with MUDLET_TEST_TELNET_DIR set)"
      assert.is_nil(os.getenv("MUDLET_TEST_REQUIRE_TELNET_FIXTURE"), "MUDLET_TEST_REQUIRE_TELNET_FIXTURE is set but this " .. reason)
      pending(reason)
      return
    end
    local function connected()
      local _, _, isConnected = getConnectionInfo()
      return isConnected
    end
    local function waitFor(predicate)
      for _ = 1, 100 do
        if predicate() then
          return true
        end
        pumpEvents(50)
      end
      return predicate()
    end

    feed("\27[1")
    connectToServer("127.0.0.1", port)
    assert.is_true(waitFor(connected), "never connected to the telnet fixture on port " .. port)
    disconnect()
    assert.is_true(waitFor(function() return not connected() end), "the telnet fixture connection outlived the disconnect")
    feed("z after it\r\n")
    assert.is_false(getConfig("promptForMXPProcessorOn"))
    assert.is_false(getConfig("specialForceMXPProcessorOn"))
  end)
end)

-- A forced processor goes on scanning what the game sends, so that a game
-- turning MXP back on re-locks it secure
describe("MXP re-initialisation of a forced processor", function()

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local function displayed(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "|")
  end

  local stillOpen = "<SEND href=\"x\">MXPSTILLOPEN</SEND>"
  local promptedBefore, forcedBefore

  -- forcing a processor the game already had on and locked open leaves it open,
  -- so only a re-initialisation makes a secure tag act
  before_each(function()
    promptedBefore = getConfig("promptForMXPProcessorOn")
    forcedBefore = getConfig("specialForceMXPProcessorOn")
    setConfig("specialForceMXPProcessorOn", false)
    setConfig("promptForMXPProcessorOn", false)
    feed("<T_IAC><T_DO><O_MXP>")
    feed("\27[5z\r\n")
    setConfig("specialForceMXPProcessorOn", true)
    setConfig("promptForMXPProcessorOn", true)
    feed("<T_IAC><T_DONT><O_MXP>")
    assert.equals(stillOpen, displayed(stillOpen .. "\r\n"))
  end)

  -- a forced processor refuses to unlock a secure default, so unlock it unforced
  after_each(function()
    setConfig("specialForceMXPProcessorOn", false)
    feed("<T_IAC><T_DO><O_MXP>")
    feed("\27[5z\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
    setConfig("specialForceMXPProcessorOn", forcedBefore)
    setConfig("promptForMXPProcessorOn", promptedBefore)
  end)

  it("re-locks it secure from a switch split between two reads", function()
    feed("\27[1")
    feed("z<B>MXPREINIT</B>\r\n")
    assert.equals("MXPRELOCKED", displayed("<SEND href=\"x\">MXPRELOCKED</SEND>\r\n"))
  end)

  it("re-locks it secure from a switch in a prompt", function()
    feed("\27[1z<B>MXPREINIT</B> <T_IAC><T_GA>")
    feed("\r\n")
    assert.equals("MXPRELOCKED", displayed("<SEND href=\"x\">MXPRELOCKED</SEND>\r\n"))
  end)

  it("re-locks it secure from a switch split between a read and a prompt", function()
    feed("\27[1")
    feed("z<B>MXPREINIT</B> <T_IAC><T_GA>")
    feed("\r\n")
    assert.equals("MXPRELOCKED", displayed("<SEND href=\"x\">MXPRELOCKED</SEND>\r\n"))
  end)

  it("does not re-lock it from two reads a prompt came between", function()
    feed("x\27[1")
    feed("HP> <T_IAC><T_GA>")
    feed("z after it\r\n")
    assert.equals(stillOpen, displayed(stillOpen .. "\r\n"))
  end)
end)

describe("Tests game text the ingest worker parses ahead", function()
  -- These feed the same lines one read per line, which the worker never sees, and as one read, and
  -- require the same buffer from both.

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local function parsedAhead()
    return getProfileStats().linesParsedAhead
  end

  -- a test whose reads the worker did not parse compared the main thread with itself
  local function requireWorker(before)
    if not before then
      pending("telling whether the worker parsed the read needs MUDLET_TEST_MODE")
    elseif parsedAhead() == before then
      pending("the ingest worker is off, so this compared the main thread with itself")
    end
  end

  -- each line's text, then the format getTextFormat() reports for each of its characters, or only the first
  local function snapshot(from, to, firstOnly)
    local result = {}
    for line = from, to do
      assert.is_true(moveCursor("main", 0, line))
      local text = getCurrentLine("main")
      local cells = {text, "prompt=" .. tostring(isPrompt())}
      local count = firstOnly and math.min(1, utf8.len(text)) or utf8.len(text)
      for offset = 0, count - 1 do
        selectSection("main", offset, 1)
        local fields = {}
        for key, value in pairs(getTextFormat("main")) do
          fields[#fields + 1] = key .. "=" .. (type(value) == "table" and table.concat(value, ",") or tostring(value))
        end
        table.sort(fields)
        cells[#cells + 1] = table.concat(fields, " ")
      end
      result[#result + 1] = table.concat(cells, "|")
    end
    deselect("main")
    return result
  end

  local passes = 0

  -- feeds the lines one read each, or all in one read (or two, cut after byte cut), and returns what they left in the buffer
  local function pass(lines, oneRead, firstOnly, cut)
    passes = passes + 1
    local opening, closing = "IngestPassOpens" .. passes, "IngestPassCloses" .. passes
    local mark = getLastLineNumber("main")
    feed("\27[0m" .. opening .. "\r\n")
    if oneRead then
      local joined = table.concat(lines, "\r\n") .. "\r\n"
      if cut then
        feed(joined:sub(1, cut))
        feed(joined:sub(cut + 1))
      else
        feed(joined)
      end
    else
      for _, line in ipairs(lines) do
        feed(line .. "\r\n")
      end
    end
    feed("\27[0m" .. closing .. "\r\n")
    local found = getLines("main", mark, getLastLineNumber("main") + 1)
    local from, to
    for i, text in ipairs(found) do
      if text == opening then
        from = mark + i
      elseif text == closing then
        to = mark + i - 2
      end
    end
    assert.is_truthy(from and to, "the pass markers did not both reach the buffer")
    return snapshot(from, to, firstOnly)
  end

  local function sameLines(apart, together)
    assert.are.equal(#apart, #together, "a different number of lines came out of one read")
    for i = 1, #apart do
      assert.are.equal(apart[i], together[i], "line " .. i .. " came out differently from one read")
    end
  end

  local function sameEitherWay(lines, firstOnly)
    local apart = pass(lines, false, firstOnly)
    local together = pass(lines, true, firstOnly)
    sameLines(apart, together)
    return together
  end

  local function corpus(count)
    local lines = {}
    for i = 1, count or 48 do
      local kind = i % 8
      if kind == 0 then
        lines[i] = "IngestPlain line " .. i .. " with nothing but text in it"
      elseif kind == 1 then
        lines[i] = "\27[31mIngestRed " .. i .. " \27[1;32mbright green\27[39m default \27[0mreset"
      elseif kind == 2 then
        -- left open, so the next line starts in it
        lines[i] = "IngestCarry " .. i .. " \27[33;44m yellow on blue to the end"
      elseif kind == 3 then
        lines[i] = "\27[38;5;208mIngest256 " .. i .. "\27[48;5;17m on navy \27[38;2;10;200;30mtruecolour\27[0m"
      elseif kind == 4 then
        lines[i] = "\27[3mIngestItalic\27[23m \27[4munder\27[24m \27[9mstruck\27[29m \27[7mrev\27[27m \27[53mover\27[55m \27[5mblink\27[25m " .. i
      elseif kind == 5 then
        lines[i] = "IngestUnicode " .. i .. " ┌─┐ café ✓ 😀 │ naïve"
      elseif kind == 6 then
        lines[i] = ""
      else
        lines[i] = "\27[12mIngestFont " .. i .. "\27[10m back\tand a tab"
      end
    end
    return lines
  end

  local triggers

  before_each(function()
    triggers = {}
  end)

  after_each(function()
    for _, id in ipairs(triggers) do
      killTrigger(id)
    end
    deselect("main")
    feed("\27[0m\r\n")
  end)

  it("commits the same text and formats from one read as from a read per line", function()
    local before = parsedAhead()
    sameEitherWay(corpus())
    requireWorker(before)
  end)

  it("commits the same from a read that a line was cut across", function()
    -- without a GA from the game, the telnet layer holds an unfinished line back and the buffer never sees a cut
    feed("<T_IAC><T_GA>")
    local lines = corpus()
    local joined = table.concat(lines, "\r\n") .. "\r\n"
    local apart = pass(lines, false)
    local cuts = {
      ["text"] = joined:find("bright", 1, true) + 2,
      ["an SGR sequence"] = joined:find("\27[1;32m", 1, true) + 4,
      ["a UTF-8 character"] = joined:find("\226\148\140", 1, true),
      ["a line ending"] = joined:find("\r\n", 1, true),
      ["text later on"] = joined:find("IngestPlain line 24", 1, true) + 8,
      ["an SGR sequence later on"] = joined:find("\27[38;5;208mIngest256 27", 1, true) + 5,
      ["a UTF-8 character later on"] = joined:find("IngestUnicode 29 \226", 1, true) + #"IngestUnicode 29 ",
    }
    local before = parsedAhead()
    for name, cut in pairs(cuts) do
      local together = pass(lines, true, false, cut)
      assert.are.equal(#apart, #together, "a different number of lines came out of a read cut in " .. name)
      for i = 1, #apart do
        assert.are.equal(apart[i], together[i], "line " .. i .. " came out differently from a read cut in " .. name)
      end
    end
    requireWorker(before)
  end)

  it("leaves a line too long for the worker to the main thread", function()
    local long = "\27[31m" .. ("r"):rep(5000) .. "\27[32m" .. ("g"):rep(4000) .. "\27[0m IngestLong"
    local before = parsedAhead()
    for _, at in ipairs({1, 20}) do
      local lines = corpus(60)
      lines[at] = long
      sameEitherWay(lines, true)
    end
    requireWorker(before)
  end)

  it("puts a hyperlink still open at the end of a line on the next line", function()
    local lines = {}
    for i = 1, 60 do
      lines[i] = "IngestLink " .. i .. " and enough text after it to leave the worker a long rest of the read"
    end
    -- nothing after it, so the link stays open past the end of the line
    lines[20] = "IngestLinkOpens 20 \27]8;;send:x?config={\"style\":{\"color\":\"#00ff00\"}}\27\\"
    lines[30] = "IngestLinkCloses 30\27]8;;\27\\ after it"
    local before = parsedAhead()
    local together = sameEitherWay(lines, true)
    assert.truthy(together[21]:find("foreground=0,255,0", 1, true), "the link's colour did not reach the next line")
    requireWorker(before)
  end)

  it("decodes a single byte code page through its table from one read", function()
    local original = getServerEncoding()
    finally(function()
      setServerEncoding(original)
    end)
    setServerEncoding("CP437")
    local lines = {}
    for i = 1, 40 do
      lines[i] = "IngestCp437 " .. i .. " \196\179\218\191"
    end
    local before = parsedAhead()
    local together = sameEitherWay(lines, true)
    assert.truthy(together[20]:find("^IngestCp437 20 ─│┌┐|"), "the read was not decoded as CP437")
    requireWorker(before)
  end)

  it("picks up the rest of a read where the worker stopped short of it", function()
    local lines = corpus(700)
    -- an unhandled sequence the worker leaves to the main thread, part way through
    lines[40] = "\27[5nIngestUnhandled 40"
    local before = parsedAhead()
    assert.are.equal(700, #sameEitherWay(lines, true))
    requireWorker(before)
    -- the first job stops after 39 lines and a job holds at most 512, so more than 39 + 512 takes at least two resumes
    assert.is_true(parsedAhead() - before > 39 + 512, "the worker was not handed the rest of the read")
  end)

  it("carries an attribute a trigger feeds part way through a read on to the lines after it", function()
    local control = pass(corpus(), true, true)
    local first = parsedAhead()
    local replayed = {}
    for _, sgr in ipairs({"\27[35m", "\27[45m", "\27[1m", "\27[3m", "\27[4:3m", "\27[6m", "\27[7m", "\27[8m", "\27[9m", "\27[53m", "\27[11m"}) do
      local id = tempTrigger("IngestFont 7 back", function()
        feedTriggers(sgr)
      end)
      local before = parsedAhead()
      local together = sameEitherWay(corpus(), true)
      killTrigger(id)
      assert.are_not.equal(control[8], together[8], "the trigger's " .. sgr:sub(2) .. " did not reach the next line")
      replayed[sgr:sub(2)] = before and parsedAhead() - before
    end
    requireWorker(first)
    for sgr, count in pairs(replayed) do
      -- lines 1-7, then 10-48 once the reset at the end of line 9 brings both parses back in step
      assert.is_true(count >= 40, "after " .. sgr .. " only " .. count .. " lines were parsed ahead")
    end
  end)

  it("colours every palette entry the same from one read", function()
    local lines = {}
    for n = 0, 7 do
      lines[#lines + 1] = "\27[3" .. n .. "mIngestPalette fg " .. n .. "\27[0m"
      lines[#lines + 1] = "\27[9" .. n .. "mIngestPalette bright fg " .. n .. "\27[0m"
      lines[#lines + 1] = "\27[4" .. n .. "mIngestPalette bg " .. n .. "\27[0m"
      lines[#lines + 1] = "\27[10" .. n .. "mIngestPalette bright bg " .. n .. "\27[0m"
      lines[#lines + 1] = "\27[1;3" .. n .. "mIngestPalette bold fg " .. n .. "\27[0m"
    end
    for n = 0, 15 do
      lines[#lines + 1] = "\27[38;5;" .. n .. "mIngestPalette 256 " .. n .. "\27[0m"
    end
    local before = parsedAhead()
    sameEitherWay(lines, true)
    requireWorker(before)
  end)

  it("uses the profile background a trigger sets part way through a read", function()
    local r, g, b = getBackgroundColor("main")
    finally(function()
      setBackgroundColor("main", r, g, b)
    end)
    triggers[#triggers + 1] = tempTrigger("IngestHostColour 20 ", function()
      setBackgroundColor("main", 10, 20, 30)
    end)
    -- Each line ends in the background the trigger sets, so only the profile colour tells the parses apart
    local lines = {}
    for i = 1, 700 do
      lines[i] = "\27[0mIngestHostColour " .. i .. " \27[48;2;10;20;30m"
    end
    local apart = pass(lines, false, true)
    setBackgroundColor("main", r, g, b)
    local before = parsedAhead()
    local together = pass(lines, true, true)
    sameLines(apart, together)
    assert.truthy(together[600]:find("background=10,20,30", 1, true), "the new background did not reach the rest of the read")
    requireWorker(before)
    -- 20 lines before the change; a job of at most 512 resumed under the new background parses the rest
    assert.is_true(parsedAhead() - before > 20 + 100, "the worker was not handed the rest of the read")
  end)

  it("marks a prompt in a read as a prompt", function()
    -- A prompt ends the read it is in, so the worker only sees one after enough lines to take the read
    local lines = {}
    for i = 1, 60 do
      lines[i] = i % 20 == 0 and ("IngestPrompt " .. i .. "> <T_IAC><T_GA>") or ("IngestPromptText " .. i)
    end
    local before = parsedAhead()
    local together = sameEitherWay(lines, true)
    local marked = {}
    for _, line in ipairs(together) do
      local text, prompt = line:match("^([^|]*)|prompt=(%a+)")
      marked[text] = prompt
    end
    assert.are.equal("true", marked["IngestPrompt 20> "], "the prompt was not marked")
    assert.are.equal("false", marked["IngestPromptText 19"], "the line before the prompt was marked")
    requireWorker(before)
  end)

  it("takes up the read again after a burst a trigger feeds part way through it", function()
    local burst = {}
    for i = 1, 20 do
      burst[i] = "IngestNested " .. i
    end
    triggers[#triggers + 1] = tempTrigger("IngestFont 7 back", function()
      feedTelnet(table.concat(burst, "\r\n") .. "\r\n\27[36m")
    end)
    local before = parsedAhead()
    sameEitherWay(corpus())
    requireWorker(before)
    -- lines 1-7, then 10-48 once the reset at the end of line 9 brings both parses back in step
    assert.is_true(parsedAhead() - before >= 40, "only " .. parsedAhead() - before .. " lines were parsed ahead")
  end)

  it("joins the lines a game wrapped itself the same from one read", function()
    local wrap, width = getConfig("undoServerWrap"), getConfig("undoServerWrapWidth")
    finally(function()
      setConfig("undoServerWrap", wrap)
      setConfig("undoServerWrapWidth", width)
    end)
    setConfig("undoServerWrap", true)
    setConfig("undoServerWrapWidth", 40)
    local vocabulary = {"the", "quick", "brown", "fox", "jumps", "over", "lazy", "dogs", "while", "ingest", "runs"}
    local lines, line = {}, ""
    for i = 1, 400 do
      local word = vocabulary[i % #vocabulary + 1]
      if #line + 1 + #word > 40 then
        lines[#lines + 1] = line
        line = word
      else
        line = line == "" and word or line .. " " .. word
      end
    end
    lines[#lines + 1] = line .. "."
    local before = parsedAhead()
    local together = sameEitherWay(lines, true)
    assert.is_true(#together < #lines, "no wrapped lines were joined")
    requireWorker(before)
  end)

  it("follows a trigger that edits the line it fired on", function()
    triggers[#triggers + 1] = tempTrigger("IngestRed 9 ", function()
      selectString("bright green", 1)
      fg("magenta")
      deselect()
      echo(" IngestEchoed")
    end)
    local before = parsedAhead()
    local together = sameEitherWay(corpus())
    assert.truthy(together[9]:find("IngestEchoed", 1, true))
    requireWorker(before)
  end)

  -- every fifth line: a short one, then each rarer kind of long line in turn
  local function oddLongLine(i)
    local kind = math.floor(i / 5) % 6
    if kind == 1 then
      -- a family emoji joined by ZWJs, two columns, and an e with a combining acute accent
      return "IngestEmoji " .. i .. " " .. ("\240\159\145\168\226\128\141\240\159\145\169\226\128\141\240\159\145\167 e\204\129 "):rep(25)
    elseif kind == 2 or kind == 3 then
      local prefix = (kind == 2 and "IngestExact " or "IngestOver ") .. i .. " ─"
      return prefix .. ("x"):rep((kind == 2 and 100 or 101) - utf8.len(prefix))
    elseif kind == 4 then
      return "IngestSpaces " .. i .. " " .. ("spaced   "):rep(15)
    elseif kind == 5 then
      return "IngestTab " .. i .. "\t" .. ("tabbed\tword "):rep(12)
    end
    return "IngestShort " .. i
  end

  -- ASCII and CJK lines past the main window's width of 100, ambiguous-width ones past it only when those
  -- are wide, lines of exactly 100 and 101 columns at narrow ambiguous width, and short ones
  local function longLines(count)
    local lines = {}
    for i = 1, count or 60 do
      local kind = i % 5
      if kind == 0 then
        lines[i] = "IngestLong " .. i .. " " .. ("word "):rep(40)
      elseif kind == 1 then
        lines[i] = "\27[32mIngestLongGreen " .. i .. " \27[1m" .. ("several words in bold "):rep(8) .. "\27[0m"
      elseif kind == 2 then
        lines[i] = "IngestWide " .. i .. " " .. ("你好世界"):rep(16)
      elseif kind == 3 then
        -- East Asian ambiguous width: one column each, unless the profile makes them two
        lines[i] = "IngestAmbiguous " .. i .. " " .. ("─±°"):rep(25)
      else
        lines[i] = oddLongLine(i)
      end
    end
    return lines
  end

  local function firstFragment(lines, prefix)
    for _, line in ipairs(lines) do
      local text = line:match("^([^|]*)")
      if text:sub(1, #prefix) == prefix then
        return text
      end
    end
  end

  local function wrapHintsUsed()
    return getProfileStats().wrapHintsUsed
  end

  it("wraps long lines the same from one read", function()
    local before, hints = parsedAhead(), wrapHintsUsed()
    local together = sameEitherWay(longLines(), true)
    assert.is_true(#together > 60, "no line was wrapped")
    requireWorker(before)
    assert.is_true(wrapHintsUsed() > hints, "no line was wrapped at the breaks the worker found")
  end)

  it("wraps long lines with a wrap and a hanging indent the same from one read", function()
    finally(function()
      setWindowWrapIndent("main", 0)
      setWindowWrapHangingIndent("main", 0)
    end)
    setWindowWrapIndent("main", 4)
    setWindowWrapHangingIndent("main", 2)
    local before, hints = parsedAhead(), wrapHintsUsed()
    local together = sameEitherWay(longLines(), true)
    assert.truthy(firstFragment(together, "    IngestLong 10 "), "the wrap indent was not applied")
    requireWorker(before)
    assert.is_true(wrapHintsUsed() > hints, "no line was wrapped at the breaks the worker found")
  end)

  local wrapSettings = {
    {"wrap width", function()
      setWindowWrap("main", 60)
    end},
    {"wrap indent", function()
      setWindowWrapIndent("main", 6)
    end},
    {"wrap indent past half the width", function()
      setWindowWrapIndent("main", 70)
    end},
    {"hanging indent", function()
      setWindowWrapHangingIndent("main", 5)
    end},
    {"East Asian ambiguous width", function()
      setConfig("ambiguousEAsianWidthCharacters", "wide")
    end},
  }
  for _, setting in ipairs(wrapSettings) do
    local name, change = setting[1], setting[2]
    it("wraps the rest of a read at the " .. name .. " a trigger sets part way through it", function()
      local wrapAt, ambiguous = getWindowWrap("main"), getConfig("ambiguousEAsianWidthCharacters")
      local function restore()
        setWindowWrap("main", wrapAt)
        setWindowWrapIndent("main", 0)
        setWindowWrapHangingIndent("main", 0)
        setConfig("ambiguousEAsianWidthCharacters", ambiguous)
      end
      finally(restore)
      setConfig("ambiguousEAsianWidthCharacters", "narrow")
      local lines = longLines(700)
      local control = pass(lines, true, true)
      triggers[#triggers + 1] = tempRegexTrigger("^IngestShort 4$", change)
      local apart = pass(lines, false, true)
      restore()
      setConfig("ambiguousEAsianWidthCharacters", "narrow")
      local before, hints = parsedAhead(), wrapHintsUsed()
      local together = pass(lines, true, true)
      sameLines(apart, together)
      assert.are_not.same(control, together, "the " .. name .. " made no difference")
      requireWorker(before)
      -- the job queued again after the change wraps at it
      assert.is_true(wrapHintsUsed() - hints > 100, "the worker's breaks were not used after the change")
    end)
  end

  it("wraps a long line a trigger edits as it now stands", function()
    triggers[#triggers + 1] = tempRegexTrigger("^IngestLong 10 ", function()
      selectString("word word", 1)
      replace(("a much longer replacement "):rep(6))
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestLong 20 ", function()
      selectString(line, 1)
      replace("IngestShortened 20")
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestWide 22 ", function()
      moveCursor(4, getLineNumber())
      insertText(("inserted "):rep(5))
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestLong 30 ", function()
      deleteLine()
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestAmbiguous 33 ", function()
      echo(" IngestEchoed " .. ("echo "):rep(10))
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestWide 12 ", function()
      echo("\nIngestSplit " .. ("split "):rep(30))
    end)
    triggers[#triggers + 1] = tempRegexTrigger("^IngestLong 40 ", function()
      feedTriggers("IngestNestedLong " .. ("nested "):rep(30) .. "\n")
    end)
    local before = parsedAhead()
    local together = sameEitherWay(longLines(), true)
    assert.truthy(firstFragment(together, "IngestShortened 20"), "the line was not shortened")
    assert.falsy(firstFragment(together, "IngestLong 30 "), "the line was not deleted")
    assert.truthy(firstFragment(together, "IngestSplit "), "the echo did not start a line")
    assert.truthy(firstFragment(together, "IngestNestedLong "), "the fed line did not arrive")
    requireWorker(before)
  end)

  it("wraps a long line a trigger recolours at the breaks the worker found", function()
    triggers[#triggers + 1] = tempRegexTrigger("^IngestLong 10 ", function()
      selectString("word", 3)
      fg("red")
      bg("blue")
      setUnderline(true)
      deselect()
    end)
    local before, hints = parsedAhead(), wrapHintsUsed()
    local together = sameEitherWay(longLines(20), false)
    requireWorker(before)
    assert.is_true(wrapHintsUsed() > hints, "no line was wrapped at the breaks the worker found")
    local recoloured
    for _, line in ipairs(together) do
      if line:sub(1, #"IngestLong 10 ") == "IngestLong 10 " and line:find("foreground=255,0,0", 1, true) then
        recoloured = true
      end
    end
    assert.is_true(recoloured, "the trigger's colour did not reach the line")
  end)

  for _, behaviour in ipairs({"hide", "replacewithspace"}) do
    it("commits blank lines after a background colour the same from one read when they are set to " .. behaviour, function()
      local original = getConfig("blankLinesBehaviour")
      finally(function()
        setConfig("blankLinesBehaviour", original)
      end)
      setConfig("blankLinesBehaviour", behaviour)
      local lines = {}
      for i = 1, 60 do
        local kind = i % 3
        if kind == 1 then
          lines[i] = "IngestBackground " .. i .. " \27[4" .. (i % 7 + 1) .. "mon colour"
        elseif kind == 2 then
          lines[i] = ""
        else
          lines[i] = "\27[0mIngestAfterBlank " .. i
        end
      end
      local before = parsedAhead()
      assert.are.equal(behaviour == "hide" and 40 or 60, #sameEitherWay(lines))
      requireWorker(before)
    end)
  end

  it("decodes a whole read in the encoding it arrived in, even when a trigger switches it part way", function()
    local original = getServerEncoding()
    finally(function()
      setServerEncoding(original)
    end)
    triggers[#triggers + 1] = tempTrigger("IngestEncodingSwitch", function()
      setServerEncoding("ISO 8859-1")
    end)
    setServerEncoding("UTF-8")
    local lines = {}
    for i = 1, 40 do
      lines[i] = "IngestEncoding " .. i .. " caf\233"
    end
    lines[5] = "IngestEncodingSwitch"
    local before = parsedAhead()
    local together = pass(lines, true, true)
    -- in UTF-8 a lone \233 is no character
    assert.truthy(together[20]:find("^IngestEncoding 20 caf\239\191\189|"), "the switch reached the rest of the read it was made in")
    local after = pass({"IngestEncoding after caf\233"}, true, true)
    assert.truthy(after[1]:find("^IngestEncoding after café|"), "the switch did not reach the next read")
    requireWorker(before)
  end)

  it("decodes the rest of a read in the encoding a trigger's own feed switched it to", function()
    local original = getServerEncoding()
    finally(function()
      setServerEncoding(original)
    end)
    triggers[#triggers + 1] = tempTrigger("IngestGbkSwitch", function()
      setServerEncoding("GBK")
      -- a feed of its own applies the encoding at once, to the rest of this read as well
      feedTriggers("IngestGbkFed\n")
    end)
    local lines = {}
    for i = 1, 700 do
      lines[i] = "IngestGbk " .. i .. " \196\227\186\195"
    end
    lines[5] = "IngestGbkSwitch"
    setServerEncoding("UTF-8")
    local apart = pass(lines, false, true)
    setServerEncoding("UTF-8")
    local before = parsedAhead()
    local together = pass(lines, true, true)
    sameLines(apart, together)
    assert.truthy(together[#together]:find("^IngestGbk 700 你好|"), "the rest of the read was not decoded as GBK")
    requireWorker(before)
  end)

  it("styles a read with the MXP colour left open when MXP was turned off", function()
    -- separate reads: a read's telnet commands are handled before its text, and an MXP mode sequence turns MXP on
    finally(function()
      feed("<T_IAC><T_DO><O_MXP>\27[5z\27[3z\r\n")
      feed("<T_IAC><T_DONT><O_MXP>")
    end)
    feed("<T_IAC><T_DO><O_MXP>\27[6z<COLOR red>IngestMxpLeftOpen\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
    local lines = {}
    for i = 1, 40 do
      lines[i] = "<B>IngestMxpColour " .. i .. "</B>"
    end
    local together = sameEitherWay(lines, true)
    assert.truthy(together[20]:find("^<B>IngestMxpColour 20</B>|"), "MXP was still on")
    assert.truthy(together[20]:find("foreground=255,0,0", 1, true), "the MXP colour did not reach the read")
  end)

  it("styles a read with the MXP bold, italic, underline and strikeout left open when MXP was turned off", function()
    finally(function()
      feed("<T_IAC><T_DO><O_MXP>\27[5z\27[3z\r\n")
      feed("<T_IAC><T_DONT><O_MXP>")
    end)
    feed("<T_IAC><T_DO><O_MXP>\27[6z<B><I><U><S>IngestMxpLeftOpen\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
    local lines = {}
    for i = 1, 40 do
      lines[i] = "IngestMxpStyle " .. i
    end
    local before = parsedAhead()
    local together = sameEitherWay(lines, true)
    for _, field in ipairs({"bold=true", "italic=true", "strikeout=true", "underline=true"}) do
      assert.truthy(together[20]:find(field, 1, true), "the MXP style did not reach the read: " .. field)
    end
    requireWorker(before)
  end)

  it("hands the rest of a read to MXP once a trigger turns it on", function()
    finally(function()
      feed("<T_IAC><T_DONT><O_MXP>")
    end)
    local lines = {}
    for i = 1, 40 do
      lines[i] = "<B>IngestMxp" .. i .. "</B>"
    end
    lines[5] = "IngestMxpSwitch"
    triggers[#triggers + 1] = tempTrigger("IngestMxpSwitch", function()
      feedTelnet("<T_IAC><T_DO><O_MXP>")
    end)
    local apart = pass(lines, false)
    feed("<T_IAC><T_DONT><O_MXP>")
    local before = parsedAhead()
    local together = pass(lines, true)
    assert.are.same(apart, together)
    assert.truthy(together[3]:find("^<B>IngestMxp3</B>|"), "a tag before the switch was acted on")
    assert.truthy(together[20]:find("^IngestMxp20|"), "a tag after the switch was not acted on")
    requireWorker(before)
  end)

  it("runs a read's triggers after the GMCP that arrived with it", function()
    finally(function()
      gmcp.IngestSpec = nil
    end)
    local seen = {}
    triggers[#triggers + 1] = tempRegexTrigger("^IngestGmcpLine (\\d+)$", function()
      seen[#seen + 1] = tostring(gmcp.IngestSpec and gmcp.IngestSpec.Order and gmcp.IngestSpec.Order.n)
    end)
    local parts = {}
    for i = 1, 60 do
      parts[#parts + 1] = "IngestGmcpLine " .. i .. "\r\n"
      if i % 20 == 0 then
        parts[#parts + 1] = "<T_IAC><T_SB><O_GMCP>IngestSpec.Order {\"n\": " .. i .. "}<T_IAC><T_SE>"
      end
    end
    local before = parsedAhead()
    feed(table.concat(parts))
    assert.are.equal(60, #seen)
    for i = 1, 60 do
      assert.are.equal("60", seen[i], "line " .. i .. " ran its triggers before the read's GMCP was handled")
    end
    requireWorker(before)
  end)

  it("goes on with the rest of a read after a trigger disconnects", function()
    local count = 0
    triggers[#triggers + 1] = tempRegexTrigger("^IngestDisconnect \\d+$", function()
      count = count + 1
      if count == 10 then
        disconnect()
      end
    end)
    local lines = {}
    for i = 1, 40 do
      lines[i] = "IngestDisconnect " .. i
    end
    local before = parsedAhead()
    feed(table.concat(lines, "\r\n") .. "\r\n")
    assert.are.equal(40, count)
    requireWorker(before)
  end)
end)
