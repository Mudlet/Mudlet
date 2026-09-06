-- MXP tags reach the console through the same processor a game's output goes
-- through, and feedTriggers is the connection-free way to hand them over. The
-- profile's forced-processor setting is what puts the parser in secure mode,
-- where every tag is allowed - the ESC[#z mode switches a game would send are
-- inert here, since TBuffer only acts on those for data flagged as coming from
-- a server.
--
-- Each tag is read back the way a user would meet it: the characters the
-- console kept and the format it drew them in, the mxp table a <SEND> fills,
-- and - for the tags that answer the game - the data Mudlet sends back.

describe("Tests the tags MXP handles", function()
  local plainFormat

  local function feed(data)
    feedTriggers(data .. "\n")
  end

  local function linesFor(data)
    local mark = getLastLineNumber("main")
    feed(data)
    return getLines("main", mark, getLastLineNumber("main") + 1)
  end

  -- the format of the first character of the word, read off the main window.
  -- selectSection() works on the line the cursor is on and counts columns from
  -- zero, where string.find() counts from one
  local function formatOf(word)
    local lastLine = getLastLineNumber("main")
    for lineNumber = lastLine, math.max(0, lastLine - 20), -1 do
      local line = getLines("main", lineNumber, lineNumber + 1)[1]
      local at = line and line:find(word, 1, true)
      if at then
        moveCursor("main", 0, lineNumber)
        selectSection("main", at - 1, 1)
        local format = getTextFormat("main")
        -- a selection left behind is what a later replace() would act on
        deselect("main")
        return format
      end
    end
    return nil
  end

  local function formatFor(data, word)
    feed(data)
    local format = formatOf(word)
    assert.is_not_nil(format, ("no line holds %q"):format(word))
    return format
  end

  -- everything Mudlet sends back while the tags are fed
  local function replyTo(data)
    local sent = {}
    local handler = registerAnonymousEventHandler("sysDataSendRequest", function(_, payload)
      sent[#sent + 1] = payload
    end)
    feed(data)
    killAnonymousEventHandler(handler)
    return sent
  end

  local function sendEvent(data)
    if type(mxp) == "table" then
      mxp.send = nil
    end
    feed(data)
    return mxp and mxp.send
  end

  setup(function()
    setConfig("specialForceMXPProcessorOn", true)
    plainFormat = formatFor("mxpTagsSpecPlain", "mxpTagsSpecPlain")
  end)

  teardown(function()
    setConfig("specialForceMXPProcessorOn", false)
  end)

  after_each(function()
    -- a tag left open in secure mode stays open, so close one of everything: an
    -- end tag with nothing to close is ignored, and a spec that failed partway
    -- through would otherwise style every line after it
    feed("</B></I></U></S></COLOR></FONT>")
  end)

  describe("Tests the styling MXP puts on text", function()
    it("colours the text a COLOR tag opens", function()
      assert.are.same({255, 0, 0}, formatFor("<COLOR fore=red>mxpColourRed</COLOR>", "mxpColourRed").foreground)
    end)

    it("takes the colour back off at the closing tag", function()
      feed("<COLOR fore=red>mxpColourInside</COLOR>mxpColourAfter")
      assert.are.same(plainFormat.foreground, formatOf("mxpColourAfter").foreground)
    end)

    it("colours the background a COLOR tag names", function()
      local format = formatFor("<COLOR fore=red back=blue>mxpColourBoth</COLOR>", "mxpColourBoth")
      assert.are.same({255, 0, 0}, format.foreground)
      assert.are.same({0, 0, 255}, format.background)
    end)

    it("reads the two colours by position as well", function()
      local format = formatFor("<C red blue>mxpColourPositional</C>", "mxpColourPositional")
      assert.are.same({255, 0, 0}, format.foreground)
      assert.are.same({0, 0, 255}, format.background)
    end)

    -- Qt's "green" is the darker HTML one; Mudlet's own colour table calls that
    -- one "lime", so a script and a game asking for the same name get different
    -- colours
    it("reads a colour name the way Qt does, not the way a script would", function()
      assert.are.same({0, 128, 0}, formatFor("<COLOR fore=green>mxpColourGreen</COLOR>", "mxpColourGreen").foreground)
      assert.are_not.same(color_table.green, {0, 128, 0})
    end)

    it("gives back the colour underneath when a nested tag closes", function()
      feed("<COLOR fore=red><COLOR fore=blue>mxpColourInner</COLOR>mxpColourOuter</COLOR>")
      assert.are.same({0, 0, 255}, formatOf("mxpColourInner").foreground)
      assert.are.same({255, 0, 0}, formatOf("mxpColourOuter").foreground)
    end)

    it("colours the text a FONT tag names", function()
      local format = formatFor("<FONT COLOR=yellow BACK=blue>mxpFontColour</FONT>", "mxpFontColour")
      assert.are.same({255, 255, 0}, format.foreground)
      assert.are.same({0, 0, 255}, format.background)
    end)

    -- the face and size are read and thrown away, but they still take up the
    -- first two positions the colours are counted from
    it("counts past the face and size a FONT tag gives by position", function()
      local format = formatFor([[<FONT "Courier" 14 red blue>mxpFontPositional</FONT>]], "mxpFontPositional")
      assert.are.same({255, 0, 0}, format.foreground)
      assert.are.same({0, 0, 255}, format.background)
    end)

    it("marks the text a B tag wraps as bold", function()
      assert.is_true(formatFor("<B>mxpStyleBold</B>", "mxpStyleBold").bold)
    end)

    it("marks the text an I tag wraps as italic", function()
      assert.is_true(formatFor("<I>mxpStyleItalic</I>", "mxpStyleItalic").italic)
    end)

    it("marks the text a U tag wraps as underlined", function()
      assert.is_true(formatFor("<U>mxpStyleUnderline</U>", "mxpStyleUnderline").underline)
    end)

    it("marks the text an S tag wraps as struck out", function()
      assert.is_true(formatFor("<S>mxpStyleStrikeout</S>", "mxpStyleStrikeout").strikeout)
    end)

    it("takes the spelled-out names for the same four styles", function()
      feed("<BOLD>mxpStyleLongBold</BOLD><EM>mxpStyleLongItalic</EM><UNDERLINE>mxpStyleLongUnderline</UNDERLINE><STRIKEOUT>mxpStyleLongStrikeout</STRIKEOUT>")
      assert.is_true(formatOf("mxpStyleLongBold").bold)
      assert.is_true(formatOf("mxpStyleLongItalic").italic)
      assert.is_true(formatOf("mxpStyleLongUnderline").underline)
      assert.is_true(formatOf("mxpStyleLongStrikeout").strikeout)
    end)

    -- a secure line's tags are the game's to close: only the tags of an open
    -- line are closed for it when the line ends
    it("keeps a style the game left open running into the next line", function()
      feed("<B>mxpStyleUnclosed")
      feed("mxpStyleNextLine")
      assert.is_true(formatOf("mxpStyleNextLine").bold)
      feed("</B>")
      feed("mxpStyleLineAfterClose")
      assert.is_false(formatOf("mxpStyleLineAfterClose").bold)
    end)

    -- recognised so it is taken out of the text, but Mudlet draws no heading
    it("takes a heading tag out of the line without styling it", function()
      local format = formatFor("<H1>mxpStyleHeading</H1>", "mxpStyleHeading")
      assert.is_false(format.bold)
      assert.are.same(plainFormat.foreground, format.foreground)
    end)

    -- IMAGE has no support behind it, and swallowing it is what keeps the tag
    -- itself from being printed
    it("swallows an IMAGE tag and joins the text around it", function()
      local lines = linesFor([[mxpImageBefore<IMAGE fname="nothing.png">mxpImageAfter]])
      assert.is_true(table.concat(lines, "\n"):find("mxpImageBeforemxpImageAfter", 1, true) ~= nil,
        table.concat(lines, "\n"))
    end)

    it("breaks the line where a BR tag is", function()
      local lines = linesFor("mxpBreakFirst<BR>mxpBreakSecond")
      local joined = table.concat(lines, "\n")
      assert.is_true(joined:find("mxpBreakFirst\nmxpBreakSecond", 1, true) ~= nil, joined)
    end)
  end)

  -- A <SEND> hands Lua the command it would run, the hint it would show and the
  -- text it wrapped, through the mxp table and an mxp.send event.
  describe("Tests the commands a SEND tag carries", function()
    it("turns the href into a command that sends it", function()
      local event = sendEvent([[<SEND HREF="look">a sign</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[look]])"}, event.actions)
      assert.are.equal("a sign", event.text)
      assert.are.equal("look", event.href)
    end)

    it("uses the text it wrapped as the command when given no attributes", function()
      local event = sendEvent("<SEND>buy bread</SEND>")
      assert.is_table(event)
      assert.are.same({"send([[buy bread]])"}, event.actions)
      assert.are.equal("buy bread", event.text)
    end)

    it("writes the command to the command line instead when PROMPT is given", function()
      local event = sendEvent([[<SEND "tell Zugg " PROMPT>Zugg</SEND>]])
      assert.is_table(event)
      assert.are.same({"printCmdLine([[tell Zugg ]])"}, event.actions)
      assert.are.equal("Zugg", event.text)
    end)

    it("makes a command of each href a | separates", function()
      local event = sendEvent([[<SEND HREF="probe it|buy it" HINT="menu|probe|buy">30901</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[probe it]])", "send([[buy it]])"}, event.actions)
      assert.are.equal("menu|probe|buy", event.hint)
    end)

    it("fills &text; in the href with the text the tag wrapped", function()
      local event = sendEvent([[<SEND href="say &text; now" hint="tip">hello</SEND>]])
      assert.is_table(event)
      assert.are.same({"send([[say hello now]])"}, event.actions)
      assert.are.equal("say hello now", event.href)
      assert.are.equal("tip", event.hint)
    end)
  end)

  -- The two tags a game uses to ask what it is talking to. Both answer over the
  -- connection rather than on screen, which is what sysDataSendRequest reports.
  describe("Tests what MXP tells the game about the client", function()
    it("answers a VERSION tag with the client name and its version", function()
      local sent = replyTo("<VERSION>")
      assert.are.equal(1, #sent, table.concat(sent, " | "))
      -- nothing but the version may follow: once a style has been named (the
      -- last spec of this file) every later answer carries it, and this must
      -- notice if that ever runs first
      local version = sent[1]:match("^\27%[1z<VERSION MXP=1%.0 CLIENT=Mudlet VERSION=([^%s>]+)>$")
      assert.is_not_nil(version, sent[1])
    end)

    it("lists what it supports for a SUPPORT tag that names nothing", function()
      local sent = replyTo("<SUPPORT>")
      assert.are.equal(1, #sent)
      assert.is_true(sent[1]:find("^\27%[1z<SUPPORTS ") ~= nil, sent[1])
      for _, element in ipairs({"+send", "+send.href", "+frame", "+color.fore"}) do
        assert.is_true(sent[1]:find(element .. " ", 1, true) ~= nil or sent[1]:find(element .. ">", 1, true) ~= nil,
          ("%s missing from %s"):format(element, sent[1]))
      end
    end)

    it("answers about the one element a SUPPORT tag names", function()
      local sent = replyTo("<SUPPORT color>")
      assert.are.equal(1, #sent)
      assert.are.equal("\27[1z<SUPPORTS +color +color.fore +color.back>", sent[1])
    end)

    it("answers with a minus for an element it does not have", function()
      local sent = replyTo("<SUPPORT nosuchtag>")
      assert.are.equal(1, #sent)
      assert.are.equal("\27[1z<SUPPORTS -nosuchtag>", sent[1])
    end)

    it("answers about a single attribute when one is named", function()
      local sent = replyTo("<SUPPORT frame.name>")
      assert.are.equal(1, #sent)
      assert.are.equal("\27[1z<SUPPORTS +frame.name>", sent[1])
    end)

    it("answers with a minus for an attribute the element does not have", function()
      local sent = replyTo("<SUPPORT frame.nosuchattribute>")
      assert.are.equal(1, #sent)
      assert.are.equal("\27[1z<SUPPORTS -frame.nosuchattribute>", sent[1])
    end)

    it("answers with the whole element when the attribute is a star", function()
      local sent = replyTo("<SUPPORT color.*>")
      assert.are.equal(1, #sent)
      assert.are.equal("\27[1z<SUPPORTS +color +color.fore +color.back>", sent[1])
    end)

    -- Last of the file: an MXP style is set for good, and a game that asked for
    -- one gets it back on every later answer. Nothing takes it back off - the
    -- parser drops an empty attribute, so <VERSION ""> is just <VERSION> - and
    -- it is only ever read back into a VERSION answer, which only this file asks
    -- for.
    it("answers nothing to a VERSION tag that names a style, and carries it afterwards", function()
      assert.are.equal(0, #replyTo("<VERSION mxpTagsSpecStyle>"))

      local sent = replyTo("<VERSION>")
      assert.are.equal(1, #sent)
      assert.is_true(sent[1]:find(" STYLE=mxpTagsSpecStyle>", 1, true) ~= nil, sent[1])
    end)
  end)
end)
