-- How MXP reads a tag: its attributes, the elements and entities a game
-- defines with <!ELEMENT> and <!ENTITY>, and the ESC[#z line modes that decide
-- which tags are read at all. MXPTags_spec.lua covers what each built-in tag
-- does once it has been read, and Telnet_spec.lua's "Tests MXP line modes" the
-- open, secure, temp secure and locked-secure switches.

describe("Tests how MXP reads the tags a game sends", function()
  local function feed(data)
    feedTriggers(data .. "\n")
  end

  local function shown(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "|")
  end

  -- the format of the first character of the word, read off the main window
  local function formatOf(word)
    local lastLine = getLastLineNumber("main")
    for lineNumber = lastLine, math.max(0, lastLine - 5), -1 do
      local at = getLines("main", lineNumber, lineNumber + 1)[1]:find(word, 1, true)
      if at then
        moveCursor("main", 0, lineNumber)
        selectSection("main", at - 1, 1)
        local format = getTextFormat("main")
        deselect("main")
        return format
      end
    end
    error(("no recent line holds %q"):format(word))
  end

  local function sendEvent(data)
    if type(mxp) == "table" then
      mxp.send = nil
    end
    feed(data)
    return mxp and mxp.send
  end

  local function replyTo(data)
    local sent = {}
    local handler = registerAnonymousEventHandler("sysDataSendRequest", function(_, payload)
      sent[#sent + 1] = payload
    end)
    feed(data)
    killAnonymousEventHandler(handler)
    return sent
  end

  setup(function()
    -- feedTriggers data is not from a server, so the ESC[#z switches are inert
    -- and the forced processor's secure mode is what lets every tag through
    setConfig("specialForceMXPProcessorOn", true)
  end)

  teardown(function()
    setConfig("specialForceMXPProcessorOn", false)
  end)

  describe("Tests reading a tag's attributes", function()
    it("reads tag and attribute names in any case", function()
      local event = sendEvent([[<send HrEf="look">mxpParseCase</SeNd>]])
      assert.is_table(event)
      assert.are.same({"send([[\nlook]])"}, event.actions)
      assert.are.equal("look", event.href)
    end)

    it("keeps the spaces inside a single-quoted value", function()
      local event = sendEvent([[<SEND href='say hello there'>mxpParseSingle</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[\nsay hello there]])"}, event.actions)
    end)

    it("does not end the tag at a > inside a quoted value", function()
      local event = sendEvent([[<SEND href="say a>b">mxpParseAngle</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[\nsay a>b]])"}, event.actions)
      assert.are.equal("mxpParseAngle", event.text)
    end)

    it("takes the text as the command when the only word is PROMPT", function()
      local event = sendEvent([[<SEND PROMPT>mxpParsePrompt</SEND>]])
      assert.is_table(event)
      assert.are.same({"printCmdLine([[\nmxpParsePrompt]])"}, event.actions)
    end)

    it("fills a game's entity into an attribute value", function()
      finally(function() feed("<!ENTITY mxpParseTarget DELETE>") end)
      feed([[<!ENTITY mxpParseTarget "orc">]])
      local event = sendEvent([[<SEND href="kill &mxpParseTarget;">mxpParseEntityAttr</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[\nkill orc]])"}, event.actions)
    end)
  end)

  describe("Tests the elements a game defines", function()
    it("forgets an element that is defined again with DELETE", function()
      finally(function() feed("<!ELEMENT mxpParseGone DELETE>") end)
      feed([[<!ELEMENT mxpParseGone "<B>">]])
      assert.are.equal("mxpParseGoneBefore", shown("<mxpParseGone>mxpParseGoneBefore</mxpParseGone>"))

      feed([[<!ELEMENT mxpParseGone DELETE>]])
      assert.are.equal("<mxpParseGone>mxpParseGoneAfter</mxpParseGone>", shown("<mxpParseGone>mxpParseGoneAfter</mxpParseGone>"))
    end)

    it("ignores a closing tag inside an element's definition", function()
      finally(function() feed("<!ELEMENT mxpParseClosing DELETE>") end)
      feed([[<!ELEMENT mxpParseClosing '<B>mxpParseClosingDefinition</B>'>]])
      feed("<mxpParseClosing>mxpParseClosingIn</mxpParseClosing>mxpParseClosingOut")
      assert.is_true(formatOf("mxpParseClosingIn").bold)
      assert.is_false(formatOf("mxpParseClosingOut").bold)
    end)

    it("ignores a closing tag inside the definition of an element being closed", function()
      finally(function() feed("<!ELEMENT mxpParseOnlyClosing DELETE>") end)
      feed([[<!ELEMENT mxpParseOnlyClosing '</B>'>]])
      assert.are.equal("mxpParseOnlyClosingText", shown("mxpParseOnlyClosingText</mxpParseOnlyClosing>"))
      feed("<B>mxpParseOnlyClosingBefore<mxpParseOnlyClosing>mxpParseOnlyClosingIn</mxpParseOnlyClosing>mxpParseOnlyClosingAfter</B>")
      assert.is_true(formatOf("mxpParseOnlyClosingAfter").bold)
    end)

    it("does not expand an element defined as itself", function()
      finally(function() feed("<!ELEMENT mxpParseSelf DELETE>") end)
      feed([[<!ELEMENT mxpParseSelf '<B><mxpParseSelf>'>]])
      feed("<mxpParseSelf>mxpParseSelfIn</mxpParseSelf>mxpParseSelfOut")
      assert.is_true(formatOf("mxpParseSelfIn").bold)
      assert.is_false(formatOf("mxpParseSelfOut").bold)
    end)

    it("does not expand elements defined as each other", function()
      finally(function()
        feed("<!ELEMENT mxpParseLoopA DELETE>")
        feed("<!ELEMENT mxpParseLoopB DELETE>")
      end)
      feed([[<!ELEMENT mxpParseLoopA '<B><mxpParseLoopB>'>]])
      feed([[<!ELEMENT mxpParseLoopB '<mxpParseLoopA>'>]])
      feed("<mxpParseLoopA>mxpParseLoopIn</mxpParseLoopA>mxpParseLoopOut")
      assert.is_true(formatOf("mxpParseLoopIn").bold)
      assert.is_false(formatOf("mxpParseLoopOut").bold)
    end)

    local function defineChain(name, length)
      local definitions = {}
      for i = 1, length do
        definitions[i] = ("<!ELEMENT %s%d '<%s%d>'>"):format(name, i, name, i + 1)
      end
      definitions[length + 1] = ("<!ELEMENT %s%d '<B>'>"):format(name, length + 1)
      feed(table.concat(definitions))
    end

    local function deleteChain(name, length)
      local deletions = {}
      for i = 1, length + 1 do
        deletions[i] = ("<!ELEMENT %s%d DELETE>"):format(name, i)
      end
      feed(table.concat(deletions))
    end

    it("expands an element through a short chain of others", function()
      finally(function() deleteChain("mxpParseShortChain", 3) end)
      defineChain("mxpParseShortChain", 3)
      feed("<mxpParseShortChain1>mxpParseShortChainIn</mxpParseShortChain1>mxpParseShortChainOut")
      assert.is_true(formatOf("mxpParseShortChainIn").bold)
      assert.is_false(formatOf("mxpParseShortChainOut").bold)
    end)

    -- long enough to overflow an AddressSanitizer build's 8MB stack were every link expanded
    it("stops expanding a chain of elements nested deeper than any game would", function()
      local length = 10000
      finally(function() deleteChain("mxpParseLongChain", length) end)
      defineChain("mxpParseLongChain", length)
      feed("<mxpParseLongChain1>mxpParseLongChainText</mxpParseLongChain1>")
      assert.is_false(formatOf("mxpParseLongChainText").bold)
    end)

    it("shows a definition that names only the element as text", function()
      assert.are.equal("<!ELEMENT mxpParseBare>mxpParseBareAfter", shown("<!ELEMENT mxpParseBare>mxpParseBareAfter"))
    end)

    it("fills a declared attribute from its default when the tag leaves it out", function()
      finally(function()
        feed("<!ELEMENT mxpParseDefault DELETE>")
        if type(mxp) == "table" then
          mxp.mxpparsedefault = nil
        end
      end)
      feed([[<!ELEMENT mxpParseDefault FLAG="mxpParseDefault" ATT="where=north what">]])
      if type(mxp) == "table" then
        mxp.mxpparsedefault = nil
      end
      feed([[<mxpParseDefault>mxpParseDefaultText</mxpParseDefault>]])
      assert.is_table(mxp.mxpparsedefault)
      assert.are.equal("north", mxp.mxpparsedefault.where)
      assert.is_nil(mxp.mxpparsedefault.what)
    end)
  end)

  describe("Tests the entities a game defines", function()
    it("shows an entity definition that names nothing as text", function()
      assert.are.equal("<!ENTITY>mxpParseNoEntity", shown("<!ENTITY>mxpParseNoEntity"))
    end)

    it("resolves an entity published without a value to nothing", function()
      finally(function() feed("<!ENTITY mxpParseEmpty DELETE>") end)
      assert.are.equal("mxpParseEmpty[]", shown("<!ENTITY mxpParseEmpty PUBLISH>mxpParseEmpty[&mxpParseEmpty;]"))
    end)

    it("forgets an entity that is defined again with DELETE", function()
      finally(function() feed("<!ENTITY mxpParseDel DELETE>") end)
      assert.are.equal("mxpParseDel[kept]", shown('<!ENTITY mxpParseDel "kept">mxpParseDel[&mxpParseDel;]'))
      assert.are.equal("mxpParseDel[&mxpParseDel;]", shown("<!ENTITY mxpParseDel DELETE>mxpParseDel[&mxpParseDel;]"))
    end)
  end)

  describe("Tests SUPPORT for an attribute of an element Mudlet lacks", function()
    it("answers with a minus for the element and attribute together", function()
      assert.are.equal("\27[1z<SUPPORTS -mxpnosuch.color>", table.concat(replyTo("<SUPPORT mxpnosuch.color>")))
    end)
  end)
end)

describe("Tests the MXP line modes a game switches between", function()
  -- the ESC[#z switches only act on data from a server, so these negotiate MXP
  -- and feed through the telnet parser

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  local function displayed(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "|")
  end

  local function boldAt(word)
    local lineNumber = getLastLineNumber("main")
    for n = lineNumber, math.max(0, lineNumber - 10), -1 do
      local line = getLines("main", n, n + 1)[1]
      local at = line and line:find(word, 1, true)
      if at then
        moveCursor("main", 0, n)
        selectSection("main", at - 1, 1)
        local format = getTextFormat("main")
        deselect("main")
        return format.bold
      end
    end
    error("no line holds " .. word)
  end

  setup(function()
    feed("<T_IAC><T_DO><O_MXP>")
    -- forcing the processor on, as the block above does, locks the default
    -- mode to secure, and turning it off again leaves that default behind
    feed("\27[5z\r\n")
  end)

  teardown(function()
    feed("\27[5z\r\n")
    feed("<T_IAC><T_DONT><O_MXP>")
  end)

  it("shows every tag as text on a locked line and parses the next one", function()
    assert.equals("<B>MXPPARSELOCKED</B>", displayed("\27[2z<B>MXPPARSELOCKED</B>\r\n"))
    assert.equals("MXPPARSEAFTERLOCKED", displayed("<B>MXPPARSEAFTERLOCKED</B>\r\n"))
  end)

  it("keeps a locked mode the game locked in until it unlocks it", function()
    finally(function() feed("\27[5z\r\n") end)
    feed("\27[7z\r\n")
    assert.equals("<B>MXPPARSELOCKIN1</B>", displayed("<B>MXPPARSELOCKIN1</B>\r\n"))
    assert.equals("<B>MXPPARSELOCKIN2</B>", displayed("<B>MXPPARSELOCKIN2</B>\r\n"))

    feed("\27[5z\r\n")
    assert.equals("MXPPARSEUNLOCKED", displayed("<B>MXPPARSEUNLOCKED</B>\r\n"))
  end)

  it("closes the tags left open when the game resets", function()
    finally(function() feed("\27[5z</B>\r\n") end)
    feed("\27[6z<B>MXPPARSERESETBOLD\r\n")
    assert.is_true(boldAt("MXPPARSERESETBOLD"))

    feed("\27[3zMXPPARSERESETPLAIN\r\n")
    assert.is_false(boldAt("MXPPARSERESETPLAIN"))
  end)

  it("ignores a mode switch that carries no number", function()
    -- the line stays open: B is taken out and SEND is not, which neither a
    -- secure nor a locked reading of the switch would do
    assert.equals("MXPPARSENONUMBER<SEND href=\"x\">N</SEND>", displayed("\27[z<B>MXPPARSENONUMBER</B><SEND href=\"x\">N</SEND>\r\n"))
    assert.equals("MXPPARSESTILLOPEN<SEND href=\"x\">N</SEND>", displayed("\27[1;2z<B>MXPPARSESTILLOPEN</B><SEND href=\"x\">N</SEND>\r\n"))
  end)
end)
