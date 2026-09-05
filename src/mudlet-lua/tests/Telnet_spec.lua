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
