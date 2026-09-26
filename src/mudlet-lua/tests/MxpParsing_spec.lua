-- How MXP reads a tag: its attributes, the elements and entities a game
-- defines with <!ELEMENT> and <!ENTITY>, and the ESC[#z line modes that decide
-- which tags are read at all. MXPTags_spec.lua covers what each built-in tag
-- does once it has been read.

describe("Tests how MXP reads the tags a game sends", function()
  local function feed(data)
    feedTriggers(data .. "\n")
  end

  local function shown(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return table.concat(getLines("main", mark, getLastLineNumber("main")), "|")
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
      assert.are.same({"send([[look]])"}, event.actions)
      assert.are.equal("look", event.href)
    end)

    it("keeps the spaces inside a single-quoted value", function()
      local event = sendEvent([[<SEND href='say hello there'>mxpParseSingle</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[say hello there]])"}, event.actions)
    end)

    it("does not end the tag at a > inside a quoted value", function()
      local event = sendEvent([[<SEND href="say a>b">mxpParseAngle</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[say a>b]])"}, event.actions)
      assert.are.equal("mxpParseAngle", event.text)
    end)

    it("takes the text as the command when the only word is PROMPT", function()
      local event = sendEvent([[<SEND PROMPT>mxpParsePrompt</SEND>]])
      assert.is_table(event)
      assert.are.same({"printCmdLine([[mxpParsePrompt]])"}, event.actions)
    end)

    it("fills a game's entity into an attribute value", function()
      feed([[<!ENTITY mxpParseTarget "orc">]])
      local event = sendEvent([[<SEND href="kill &mxpParseTarget;">mxpParseEntityAttr</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[kill orc]])"}, event.actions)
    end)
  end)

  describe("Tests the elements a game defines", function()
    it("forgets an element that is defined again with DELETE", function()
      feed([[<!ELEMENT mxpParseGone '<B>'>]])
      assert.are.equal("mxpParseGoneBefore", shown("<mxpParseGone>mxpParseGoneBefore</mxpParseGone>"))

      feed([[<!ELEMENT mxpParseGone DELETE>]])
      assert.are.equal("<mxpParseGone>mxpParseGoneAfter</mxpParseGone>", shown("<mxpParseGone>mxpParseGoneAfter</mxpParseGone>"))
    end)

    it("shows a definition that names only the element as text", function()
      assert.are.equal("<!ELEMENT mxpParseBare>mxpParseBareAfter", shown("<!ELEMENT mxpParseBare>mxpParseBareAfter"))
    end)

    it("fills a declared attribute from its default when the tag leaves it out", function()
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
      assert.are.equal("mxpParseEmpty[]", shown("<!ENTITY mxpParseEmpty PUBLISH>mxpParseEmpty[&mxpParseEmpty;]"))
    end)

    it("forgets an entity that is defined again with DELETE", function()
      assert.are.equal("mxpParseDel[kept]", shown('<!ENTITY mxpParseDel "kept">mxpParseDel[&mxpParseDel;]'))
      assert.are.equal("mxpParseDel[&mxpParseDel;]", shown("<!ENTITY mxpParseDel DELETE>mxpParseDel[&mxpParseDel;]"))
    end)
  end)

  describe("Tests SUPPORT for an attribute of an element Mudlet lacks", function()
    it("answers with a minus for the element and attribute together", function()
      local sent = table.concat(replyTo("<SUPPORT mxpnosuch.color>"))
      assert.is_truthy(sent:find("-mxpnosuch.color", 1, true), sent)
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
    assert.equals("MXPPARSENONUMBER", displayed("\27[zMXPPARSENONUMBER\r\n"))
    assert.equals("<SEND href=\"x\">MXPPARSESTILLOPEN</SEND>", displayed("\27[1;2z<SEND href=\"x\">MXPPARSESTILLOPEN</SEND>\r\n"))
  end)
end)
