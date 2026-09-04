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

describe("Tests MSDP subnegotiation handling", function()
  -- MSDP forbids only NUL, IAC and its own six markers inside a value, so every
  -- other control code is legal payload the game means to send. Mudlet turns the
  -- payload into JSON for the Lua decoder, which rejects the whole document
  -- rather than one value if a control character arrives unescaped.
  local VAR, VAL = "<01>", "<02>"
  local TABLE_OPEN, TABLE_CLOSE = "<03>", "<04>"
  local ARRAY_CLOSE = "<06>"

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
               .. VAR .. "L" .. VAL .. "<05>" .. VAL .. "a" .. VAL .. "b" .. "<06>"
               .. VAR .. "Z" .. VAL .. "plain"
               .. TABLE_CLOSE)
      assert.is_table(msdp.MSDPSHAPE, "the variable went missing entirely")
      assert.equals("plain", msdp.MSDPSHAPE.Z, "Z is not reachable, so the table gained a spurious array level")
      assert.same({"a", "b"}, msdp.MSDPSHAPE.L)
      assert.is_nil(msdp.MSDPSHAPE[1], "the whole table was wrapped in an array it never had")
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
    pending("json_to_value returns a function rather than a number for a bare JSON number, so gmcp.X ends up holding a function")
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

  it("switches the MXP processor on from a subnegotiation, in locked mode", function()
    -- some games negotiate nothing and just send IAC SB MXP IAC SE. That starts
    -- the processor in locked mode, where nothing is a tag until the game sends
    -- a mode switch of its own
    finally(restoreMxpDefaultMode)
    assert.same({"sysProtocolEnabled:MXP"}, protocolEventsFrom("<T_IAC><T_SB><O_MXP><T_IAC><T_SE>"))

    local mark = getLastLineNumber("main")
    feed("<B>MXPSUBNEGLOCKED</B>\r\n")
    assert.same({"<B>MXPSUBNEGLOCKED</B>"}, getLines("main", mark, getLastLineNumber("main")))

    mark = getLastLineNumber("main")
    feed("\27[1z<B>MXPSUBNEGOPEN</B>\r\n")
    assert.same({"MXPSUBNEGOPEN"}, getLines("main", mark, getLastLineNumber("main")))

    assert.same({"sysProtocolDisabled:MXP"}, protocolEventsFrom("<T_IAC><T_DONT><O_MXP>"))
  end)
end)

describe("Tests MCCP compressed streams", function()

  -- feedTelnet() reads its argument as a C string and turns "<..>" tokens into
  -- bytes, so a compressed stream has to reach it with its NULs written as the
  -- token and its angle brackets doubled
  local function escaped(bytes)
    return (bytes:gsub("[%z<>]", {["\0"] = "<00>", ["<"] = "<<", [">"] = ">>"}))
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

  -- neither the end of a stream nor a broken one clears the WILL, so without this
  -- every later spec's IAC SB is still a candidate MCCP start sequence
  after_each(function()
    feed("<T_IAC><T_WONT><O_MCCP2>")
  end)

  it("shows the text a server sends once it switches to MCCP v2", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    -- the start sequence is IAC SB COMPRESS2 IAC SE with the deflate stream
    -- running straight on from it, in the same read
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped(COMPRESSED))

    local displayed = linesSince(mark)
    assert.is_truthy(displayed:find("MCCPDECOMPRESSEDOK", 1, true), "nothing from the compressed stream reached the display: " .. displayed)
    -- the compressed bytes are not the text, so a parser that skipped inflate
    -- would show mojibake rather than three copies of the marker
    local _, copies = displayed:gsub("MCCPDECOMPRESSEDOK", "")
    assert.equals(3, copies, displayed)
  end)

  it("warns and falls back to plain text when the compressed stream is broken", function()
    local mark = getLastLineNumber("main")
    feed("<T_IAC><T_WILL><O_MCCP2>")
    feed("<T_IAC><T_SB><O_MCCP2><T_IAC><T_SE>" .. escaped("\120\156NOTREALLYCOMPRESSEDATALL") .. "\r\nMCCPPLAINAFTERBREAK\r\n")

    local displayed = linesSince(mark)
    assert.is_truthy(displayed:find("MCCP decompression error", 1, true), "a broken stream was swallowed without a warning: " .. displayed)
    assert.is_truthy(displayed:find("MCCPPLAINAFTERBREAK", 1, true), "text after the broken stream was lost instead of being shown: " .. displayed)
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
  -- transcoded the message out of the game's encoding
  local function msspValueOf(bytes)
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
    pending("TTextCodec_437's table holds Medievia's map glyphs from 0x80 to 0xAF, so CP437 can neither send nor read Ç, ü, é or Ü")
  end)

  it("carries the lower case Greek letters of CP737", function()
    pending("both CP737 tables hold CP437's characters from 0x98 to 0xAF, so bytes for α to ψ come out as ÿ, Ö, á and friends")
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
