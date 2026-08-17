-- Raw game data driven through the real telnet parser with feedTelnet(), which
-- only injects while the profile is offline - see the tests README.

describe("Tests SGR default colour handling", function()

  local mark

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

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
