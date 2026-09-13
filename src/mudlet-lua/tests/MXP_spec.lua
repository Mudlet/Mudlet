describe("Tests MXP handling", function()

  setup(function()
    -- these specs carry MXP tags in through feedTriggers, so the processor is
    -- forced on: that is what locks secure mode, which negotiating MXP with a
    -- server would not do
    setConfig("specialForceMXPProcessorOn", true)
  end)

  teardown(function()
    -- turn the MXP processor back off so later specs are not affected
    setConfig("specialForceMXPProcessorOn", false)
  end)

  describe("Tests custom element events populate the mxp table", function()
    it("should expose positional values under declared (ATT) attribute names", function()
      feedTriggers([[<!ELEMENT RMob FLAG="RoomMob" ATT="Name">]] .. "\n")
      feedTriggers([[<RMob "Urthguk">Urthguk, a hulking half-ogre</RMob>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.rmob)
      -- declared attribute resolves the positional token, value case intact
      assert.are.equal("Urthguk", mxp.rmob.name)
      -- the legacy lowercased token key remains for older scripts
      assert.is_not_nil(mxp.rmob.urthguk)
      assert.is_nil(mxp.rmob.Urthguk)
    end)

    it("should lowercase named attribute keys", function()
      feedTriggers([[<!ELEMENT RExit FLAG="RoomExit">]] .. "\n")
      feedTriggers([[<RExit Dir="North">north</RExit>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.rexit)
      assert.are.equal("North", mxp.rexit.dir)
      assert.is_nil(mxp.rexit.Dir)
    end)

    it("should keep positional token keys lowercased without a declared attribute", function()
      feedTriggers([[<!ELEMENT RItem FLAG="RoomItem">]] .. "\n")
      feedTriggers([[<RItem "Sword">a gleaming sword</RItem>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.ritem)
      assert.is_not_nil(mxp.ritem.sword)
      assert.is_nil(mxp.ritem.Sword)
    end)
  end)

  describe("Tests the text an MXP line is displayed as", function()
    local encodingFile = getMudletHomeDir() .. "/encoding"
    local hadEncodingFile, originalEncoding

    setup(function()
      -- the payloads below carry their non-ASCII characters as the multi-byte
      -- sequences a game sends; under another encoding the decoder reassembles
      -- them into something else and the assertions stop meaning anything.
      -- setServerEncoding() writes the profile's "encoding" file, and a profile
      -- that never had one must not be left with one.
      hadEncodingFile = lfs.attributes(encodingFile, "mode") ~= nil
      originalEncoding = getServerEncoding()
      setServerEncoding("UTF-8")
    end)

    teardown(function()
      setServerEncoding(originalEncoding)
      if not hadEncodingFile then
        os.remove(encodingFile)
      end
    end)

    -- The payloads are what a game sends, ESC[#z mode switches and all, but
    -- those bytes are inert here: TBuffer only acts on them for data flagged as
    -- coming from a server (TBuffer.cpp's isFromServer gate), which feedTriggers
    -- is not. Secure mode comes from the forced processor this file turns on.
    -- feedTelnet would flag the data and run the mode switches, but its ESC[1z
    -- also makes cTelnet auto-enable MXP, and only a new connection clears that
    -- flag - the teardown's setConfig cannot, so every later spec file would
    -- inherit an enabled MXP processor. The tag and entity handling under test
    -- is the same either way.
    local function displayedLines(data)
      local mark = getLastLineNumber("main")
      feedTriggers(data .. "\n")
      return getLines("main", mark, getLastLineNumber("main") + 1)
    end

    -- a whole line, not a substring: an entity that resolved to the wrong thing
    -- would still be a substring of the line it landed on
    local function assertLineShown(data, expected)
      local lines = displayedLines(data)
      local shown = false
      for _, line in ipairs(lines) do
        shown = shown or line == expected
      end
      assert.is_true(shown, ("no line reads %q, the console shows:\n%s"):format(expected, table.concat(lines, "\n")))
    end

    it("takes the tags out of an MXP line", function()
      assertLineShown("\27[1z<B>Greetings < hunters & sorcerers</B>\27[7z", "Greetings < hunters & sorcerers")
    end)

    -- an unescaped & running into a non-ASCII character is not an entity, so
    -- the raw bytes have to be passed through for the charset decoder to
    -- reassemble them (follow-up to #9439)
    it("keeps the non-ASCII bytes around a malformed entity", function()
      assertLineShown("\27[1zKäse&Brötchen and &Ф too", "Käse&Brötchen and &Ф too")
    end)

    -- a custom entity whose value is not Latin1 (#9439)
    it("resolves a custom entity to its non-ASCII value", function()
      assertLineShown("\27[1z<!ENTITY storm \"Гроза\">The &storm; rages", "The Гроза rages")
    end)

    -- <HR> is not written out, it is fed back through the parser as a rule of
    -- its own, so its width is the window's wrap column with a forty column floor
    it("draws a horizontal rule as wide as the window wraps", function()
      local width = math.max(getWindowWrap("main"), 40)
      assertLineShown("\27[1zMXPRULE1<HR>MXPRULE2", ("-"):rep(width))
    end)
  end)

  -- <DEST> hands the game a print sink other than the main window: everything
  -- between it and </DEST> goes into the named frame's own console, which is a
  -- miniconsole registered under the frame's name so Lua can read it back like
  -- any other window. A frame name that does not resolve is not an error - the
  -- tag still counts as handled and the text goes to main with only a qWarning
  -- - so a red spec here is worth checking the frame name over first.
  describe("Tests output redirected into an MXP frame", function()
    local frame = "mxpSpecDestFrame"

    local function frameLines()
      local lineCount = getLineCount(frame)
      if not lineCount or lineCount < 1 then
        return {}
      end
      return getLines(frame, 0, lineCount + 1)
    end

    -- the frame line the needle is on, so a redirect that ran two segments
    -- together on one line can be told from one that kept the break
    local function frameLineWith(needle)
      for lineNumber = 0, getLineCount(frame) do
        local line = getLines(frame, lineNumber, lineNumber + 1)[1]
        if line and line:find(needle, 1, true) then
          return lineNumber, line
        end
      end
      return nil
    end

    local function holds(lines, needle)
      for _, line in ipairs(lines) do
        if line:find(needle, 1, true) then
          return true
        end
      end
      return false
    end

    -- a redirect that leaked would put its text among the last few main lines,
    -- and looking back from the end rather than from a saved index keeps that
    -- true whether or not the main buffer trimmed in between
    local function mainRecentlyHolds(needle)
      local lastLine = getLastLineNumber("main")
      for lineNumber = lastLine, math.max(0, lastLine - 20), -1 do
        local line = getLines("main", lineNumber, lineNumber + 1)[1]
        if line and line:find(needle, 1, true) then
          return true
        end
      end
      return false
    end

    -- the colour of the first character of the run of text holding the needle,
    -- read off the main window. selectSection() works on the line the cursor is
    -- on and counts columns from zero, where string.find() counts from one
    local function mainColourOf(needle)
      local lastLine = getLastLineNumber("main")
      for lineNumber = lastLine, math.max(0, lastLine - 20), -1 do
        local line = getLines("main", lineNumber, lineNumber + 1)[1]
        local at = line and line:find(needle, 1, true)
        if at then
          moveCursor("main", 0, lineNumber)
          selectSection("main", at - 1, 1)
          local colour = getTextFormat("main").foreground
          -- a selection left behind is what a later replace() would act on
          deselect("main")
          return colour
        end
      end
      return nil
    end

    setup(function()
      feedTriggers(('<FRAME Name="%s" Align="right" Width="20%%" Height="30%%">'):format(frame) .. "\n")
    end)

    teardown(function()
      -- a frame left open would sit in the main window's layout for every later
      -- spec file, and its console would stay in the window registry
      feedTriggers(('<FRAME %s ACTION="close">'):format(frame) .. "\n")
      -- closeFrame() answers true for a name it never had, so the only proof
      -- the main window got its width back is the console being gone
      assert.is_nil(windowType(frame))
    end)

    it("gives the frame a console of its own", function()
      assert.are.equal("miniconsole", windowType(frame))
    end)

    it("puts the redirected text in the frame and not in the main window", function()
      assert.is_true(feedTriggers(('<DEST %s>MXPDEST1 routed away</DEST>'):format(frame) .. "\n"))
      local _, routed = frameLineWith("MXPDEST1")
      assert.are.equal("MXPDEST1 routed away", routed, table.concat(frameLines(), "|"))
      assert.is_false(mainRecentlyHolds("MXPDEST1"))
    end)

    it("keeps a line break inside the redirect in the frame too", function()
      assert.is_true(feedTriggers(('<DEST %s>MXPDEST2 first'):format(frame) .. "\nMXPDEST2 second</DEST>\n"))
      local shown = table.concat(frameLines(), "|")
      local firstIndex, firstLine = frameLineWith("MXPDEST2 first")
      local secondIndex, secondLine = frameLineWith("MXPDEST2 second")
      assert.is_truthy(firstIndex, shown)
      assert.is_truthy(secondIndex, shown)
      -- whole lines and adjacent indexes: a redirect that dropped the break
      -- would still hold both strings, just run together on one line
      assert.are.equal("MXPDEST2 first", firstLine, shown)
      assert.are.equal("MXPDEST2 second", secondLine, shown)
      assert.are.equal(firstIndex + 1, secondIndex, shown)
      assert.is_false(mainRecentlyHolds("MXPDEST2"))
    end)

    -- </DEST> resets the text format to the profile's own colours rather than
    -- restoring whatever was in force, so colour set before the redirect does
    -- not survive it either
    it("does not let colour set inside the redirect follow the text back to main", function()
      -- the colour below is never closed by hand, so if the reset under test is
      -- the thing severed the main console would stay red for every later spec
      finally(function() feedTriggers("\027[0m\n") end)
      assert.is_true(feedTriggers("MXPDEST3 plain\n"))
      local plainColour = mainColourOf("MXPDEST3 plain")
      assert.is_truthy(plainColour)
      assert.is_true(feedTriggers(('<DEST %s>'):format(frame) .. "\027[31mMXPDEST4 red in the frame</DEST>MXPDEST4 back in main\n"))
      assert.is_true(holds(frameLines(), "MXPDEST4 red in the frame"))
      assert.are.same(plainColour, mainColourOf("MXPDEST4 back in main"))
    end)

    -- EOF on the opening tag empties the frame before the redirected text
    -- lands, so the frame ends up holding only what this redirect wrote
    it("empties the frame when the redirect carries EOF", function()
      assert.is_true(feedTriggers(('<DEST %s>MXPDEST5 stale</DEST>'):format(frame) .. "\n"))
      -- the state EOF is meant to undo, so a frame that was empty anyway
      -- cannot pass this by accident
      assert.is_true(holds(frameLines(), "MXPDEST5 stale"))
      assert.is_true(feedTriggers("MXPDEST12 anchored in main\n"))

      assert.is_true(feedTriggers(('<DEST %s EOF>MXPDEST6 fresh</DEST>'):format(frame) .. "\n"))
      local shown = table.concat(frameLines(), "|")
      assert.is_false(holds(frameLines(), "MXPDEST5 stale"), shown)
      assert.is_true(holds(frameLines(), "MXPDEST6 fresh"), shown)
      -- an EOF clear that overreached past the frame would take main with it
      assert.is_true(mainRecentlyHolds("MXPDEST12 anchored in main"))
    end)

    -- EOL discards the part-written line the frame was left sitting on rather
    -- than continuing it. A redirect closes its own last line, so the open
    -- line has to come from elsewhere - an echo into the frame leaves one the
    -- way a game writing a partial line into the frame would.
    it("drops the frame's unfinished line when the redirect carries EOL", function()
      echo(frame, "MXPDEST7 unfinished")
      assert.is_true(holds(frameLines(), "MXPDEST7 unfinished"))
      assert.is_true(feedTriggers("MXPDEST13 anchored in main\n"))

      assert.is_true(feedTriggers(('<DEST %s EOL>MXPDEST8 after</DEST>'):format(frame) .. "\n"))
      local shown = table.concat(frameLines(), "|")
      -- without the clear the redirect continues that line instead, so the
      -- needle survives as the head of a joined-up line
      assert.is_false(holds(frameLines(), "MXPDEST7 unfinished"), shown)
      assert.is_true(holds(frameLines(), "MXPDEST8 after"), shown)
      assert.is_true(mainRecentlyHolds("MXPDEST13 anchored in main"))
    end)

    -- EOL is the narrower of the two: it must leave the finished lines above
    -- the open one where they are, which is what tells it apart from EOF
    it("keeps the frame's finished lines when the redirect carries EOL", function()
      assert.is_true(feedTriggers(('<DEST %s>MXPDEST9 kept</DEST>'):format(frame) .. "\n"))
      echo(frame, "MXPDEST10 unfinished")
      assert.is_true(holds(frameLines(), "MXPDEST10 unfinished"))
      assert.is_true(holds(frameLines(), "MXPDEST9 kept"))

      assert.is_true(feedTriggers(('<DEST %s EOL>MXPDEST11 after</DEST>'):format(frame) .. "\n"))
      local shown = table.concat(frameLines(), "|")
      assert.is_true(holds(frameLines(), "MXPDEST9 kept"), shown)
      assert.is_false(holds(frameLines(), "MXPDEST10 unfinished"), shown)
    end)
  end)

  -- An internal frame takes its space out of the main console, and the main
  -- window's character grid is the only way to see that: getBorderLeft() and
  -- its friends report the borders the user set, not the ones MXP adds.
  describe("Tests the frames MXP asks for", function()
    local function openFrame(name, attributes)
      feedTriggers(('<FRAME Name="%s" %s>'):format(name, attributes) .. "\n")
    end

    local function closeFrame(name)
      feedTriggers(('<FRAME %s ACTION="close">'):format(name) .. "\n")
    end

    -- everything the main window gained since it was at line `mark`: a tag that
    -- was refused is still in the stream and shows up here as text
    local function mainSince(mark)
      return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "|")
    end

    it("takes the columns an internal frame needs from the main window", function()
      finally(function() closeFrame("mxpSpecLeftFrame") end)
      local columns, rows = getColumnCount("main"), getRowCount("main")

      openFrame("mxpSpecLeftFrame", 'Align="left" Width="25%" Height="50%"')

      assert.are.equal("miniconsole", windowType("mxpSpecLeftFrame"))
      assert.is_true(getColumnCount("main") < columns, ("the main window still has %d columns"):format(getColumnCount("main")))
      assert.are.equal(rows, getRowCount("main"), "a frame down one side took rows as well as columns")

      closeFrame("mxpSpecLeftFrame")

      assert.is_nil(windowType("mxpSpecLeftFrame"))
      assert.are.equal(columns, getColumnCount("main"), "the main window did not get its columns back")
    end)

    it("takes rows instead when the frame is along the bottom", function()
      finally(function() closeFrame("mxpSpecBottomFrame") end)
      local columns, rows = getColumnCount("main"), getRowCount("main")

      openFrame("mxpSpecBottomFrame", 'Align="bottom" Width="50%" Height="20%"')

      assert.is_true(getRowCount("main") < rows, ("the main window still has %d rows"):format(getRowCount("main")))
      assert.are.equal(columns, getColumnCount("main"), "a frame along the bottom took columns as well as rows")
    end)

    -- a height of "5c" is five lines of the profile's font; read as a percentage
    -- it would be a twentieth of the window, which is nothing like the same
    it("reads a size given in characters as characters", function()
      finally(function()
        closeFrame("mxpSpecCharsFrame")
        closeFrame("mxpSpecPercentFrame")
      end)
      local rows = getRowCount("main")

      openFrame("mxpSpecCharsFrame", 'Align="bottom" Width="50%" Height="5c"')
      local rowsTakenByCharacters = rows - getRowCount("main")
      closeFrame("mxpSpecCharsFrame")

      openFrame("mxpSpecPercentFrame", 'Align="bottom" Width="50%" Height="5%"')
      local rowsTakenByPercent = rows - getRowCount("main")

      assert.is_true(rowsTakenByCharacters > rowsTakenByPercent,
        ("five characters took %d rows and five percent took %d"):format(rowsTakenByCharacters, rowsTakenByPercent))
    end)

    -- CMUD leaves a frame the player has moved or resized alone when the game
    -- opens it again, and Mudlet follows it
    it("leaves a frame that is already open at the size it has", function()
      finally(function() closeFrame("mxpSpecReopenedFrame") end)
      openFrame("mxpSpecReopenedFrame", 'Align="left" Width="25%" Height="50%"')
      local frameColumns = getColumnCount("mxpSpecReopenedFrame")
      local mainColumns = getColumnCount("main")

      openFrame("mxpSpecReopenedFrame", 'Align="left" Width="60%" Height="90%"')

      assert.are.equal(frameColumns, getColumnCount("mxpSpecReopenedFrame"), "the second tag resized the frame")
      assert.are.equal(mainColumns, getColumnCount("main"), "the second tag took more of the main window")
    end)

    it("gives an external frame a console without taking main window space", function()
      finally(function() closeFrame("mxpSpecExternalFrame") end)
      local columns, rows = getColumnCount("main"), getRowCount("main")

      openFrame("mxpSpecExternalFrame", 'EXTERNAL Width="30%" Height="30%"')

      assert.are.equal("miniconsole", windowType("mxpSpecExternalFrame"))
      assert.are.equal(columns, getColumnCount("main"))
      assert.are.equal(rows, getRowCount("main"))
    end)

    -- DOCK names the frame to dock into and ALIGN=CLIENT makes it a tab of that
    -- frame rather than a window of its own, so it costs the main window nothing
    it("docks a frame into another as a tab and closes it with its parent", function()
      finally(function()
        closeFrame("mxpSpecTabChild")
        closeFrame("mxpSpecTabParent")
      end)
      openFrame("mxpSpecTabParent", 'Align="right" Width="30%" Height="50%" TITLE="Parent"')
      local columns = getColumnCount("main")

      openFrame("mxpSpecTabChild", 'DOCK="mxpSpecTabParent" Align="client" TITLE="Tab"')

      assert.are.equal("miniconsole", windowType("mxpSpecTabChild"))
      assert.are.equal(columns, getColumnCount("main"), "the tab took space of its own instead of sharing its parent's")

      closeFrame("mxpSpecTabParent")

      assert.is_nil(windowType("mxpSpecTabChild"), "the tab outlived the frame it was docked into")
    end)

    it("shows the tag as text when the frame it names is not there", function()
      finally(function() closeFrame("mxpSpecFocusFrame") end)
      openFrame("mxpSpecFocusFrame", 'Align="left" Width="25%" Height="50%"')

      local mark = getLastLineNumber("main")
      feedTriggers('<FRAME mxpSpecFocusFrame ACTION="focus">MXPFRAME1 focused' .. "\n")
      local handled = mainSince(mark)
      assert.is_truthy(handled:find("MXPFRAME1 focused", 1, true), handled)
      assert.is_falsy(handled:find("<FRAME", 1, true), handled)

      mark = getLastLineNumber("main")
      feedTriggers('<FRAME mxpSpecNoSuchFrame ACTION="focus">MXPFRAME2 not focused' .. "\n")
      local refused = mainSince(mark)
      assert.is_truthy(refused:find("<FRAME mxpSpecNoSuchFrame", 1, true), refused)
    end)

    -- closing one that is not there is the exception: it answers as though it
    -- had been, so a game that closes a frame twice does not print its own tag
    it("says nothing when asked to close a frame that is not there", function()
      local mark = getLastLineNumber("main")
      feedTriggers('<FRAME mxpSpecNeverOpened ACTION="close">MXPFRAME3 closed anyway' .. "\n")

      local shown = mainSince(mark)
      assert.is_truthy(shown:find("MXPFRAME3 closed anyway", 1, true), shown)
      assert.is_falsy(shown:find("<FRAME", 1, true), shown)
    end)

    it("refuses a name that is not a plain word", function()
      finally(function()
        closeFrame("mxpSpecDotted.name")
        closeFrame("mxpSpec spaced")
      end)
      local columns = getColumnCount("main")

      local mark = getLastLineNumber("main")
      feedTriggers('<FRAME Name="mxpSpec spaced" Align="left" Width="25%" Height="50%">MXPFRAME4 spaced' .. "\n")

      assert.is_nil(windowType("mxpSpec spaced"))
      assert.are.equal(columns, getColumnCount("main"))
      -- the refusal leaves the tag in the stream rather than eating it silently
      assert.is_truthy(mainSince(mark):find("<FRAME", 1, true), mainSince(mark))

      feedTriggers('<FRAME Name="mxpSpecDotted.name" Align="left" Width="25%" Height="50%">' .. "\n")

      assert.is_nil(windowType("mxpSpecDotted.name"))
    end)

    it("stops at twenty frames", function()
      local names = {}
      for index = 1, 21 do
        names[index] = ("mxpSpecLimit%d"):format(index)
      end
      finally(function()
        for _, name in ipairs(names) do
          closeFrame(name)
        end
      end)

      for _, name in ipairs(names) do
        openFrame(name, 'Align="left" Width="2c" Height="2c"')
      end

      assert.are.equal("miniconsole", windowType(names[20]), "the twentieth frame was refused")
      assert.is_nil(windowType(names[21]), "a twenty-first frame was allowed")
    end)

    -- a frame opened while output is redirected belongs to the frame it was
    -- redirected into, and comes out of that frame's space rather than the main
    -- window's
    it("nests a frame opened inside a redirect in the frame it was redirected to", function()
      finally(function()
        closeFrame("mxpSpecNestedInner")
        closeFrame("mxpSpecNestedOuter")
      end)
      openFrame("mxpSpecNestedOuter", 'Align="left" Width="40%" Height="60%"')
      local columns = getColumnCount("main")

      feedTriggers('<DEST mxpSpecNestedOuter><FRAME Name="mxpSpecNestedInner" Align="left" Width="50%" Height="50%"></DEST>' .. "\n")

      assert.are.equal("miniconsole", windowType("mxpSpecNestedInner"))
      assert.are.equal(columns, getColumnCount("main"), "the nested frame took main window space of its own")
      assert.is_true(getColumnCount("mxpSpecNestedInner") < getColumnCount("mxpSpecNestedOuter"),
        "the nested frame is not inside the one it was opened in")
    end)
  end)
end)
