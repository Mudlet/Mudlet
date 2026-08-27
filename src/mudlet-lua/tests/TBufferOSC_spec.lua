-- How TBuffer::translateToPlainText() handles the out-of-band sequences that
-- arrive mixed into the game's text: OSC, the DCS/SOS/PM/APC string sequences
-- and escape sequences that Mudlet does not act on.

describe("Tests TBuffer OSC sequence handling", function()

  -- feedTriggers writes to the main console; fish the line carrying our
  -- unique marker back out of the buffer to see what actually rendered
  local function findRecentLine(needle)
    local lastLine = getLastLineNumber("main")
    local lines = getLines("main", math.max(0, lastLine - 15), lastLine + 1)
    for i = #lines, 1, -1 do
      if lines[i]:find(needle, 1, true) then
        return lines[i]
      end
    end
    return nil
  end

  describe("Tests the protection against buffer underflow in OSC sequences", function()
    
    it("should handle OSC sequences at buffer start without crashing", function()
      -- This test attempts to trigger the OSC sequence handling code path
      -- that was vulnerable to buffer underflow when the sequence appeared
      -- at the very beginning of the buffer (position 0)
      
      -- Create a test buffer for receiving data
      createBuffer("oscTestBuffer")
      clearWindow("oscTestBuffer")
      
      -- OSC sequence: ESC ] 0 ; Title ESC \
      -- In decimal: 27 93 48 59 84 105 116 108 101 27 92
      -- This should not cause a crash even if it appears at the start of data
      local oscSequence = string.char(27) .. "]0;Test Title" .. string.char(27, 92)
      
      -- Test 1: OSC sequence at the very beginning
      -- This should trigger the fixed code path without crashing
      local testSuccess = pcall(function()
        -- We'll try to echo the raw sequence to see if it's processed safely
        -- The exact mechanism might vary but this should exercise the TBuffer code
        echo("oscTestBuffer", oscSequence)
      end)
      
      -- The important thing is that we don't crash
      assert.is_true(testSuccess, "OSC sequence at buffer start should not crash")
      
      -- Test 2: OSC sequence after some text
      clearWindow("oscTestBuffer")
      local testSuccess2 = pcall(function()
        echo("oscTestBuffer", "Some text" .. oscSequence)
      end)
      
      assert.is_true(testSuccess2, "OSC sequence after text should not crash")
      
      -- Test 3: Incomplete OSC sequence (no terminator)
      clearWindow("oscTestBuffer")
      local incompleteOsc = string.char(27) .. "]0;Test Title"
      local testSuccess3 = pcall(function()
        echo("oscTestBuffer", incompleteOsc)
      end)
      
      assert.is_true(testSuccess3, "Incomplete OSC sequence should not crash")
    end)
    
    it("should handle multiple OSC sequences safely", function()
      createBuffer("oscTestBuffer2")
      clearWindow("oscTestBuffer2")
      
      -- Multiple OSC sequences, including one at the start
      local multipleOsc = string.char(27) .. "]0;Title1" .. string.char(27, 92) ..
                         "Some text" ..
                         string.char(27) .. "]0;Title2" .. string.char(27, 92)
      
      local testSuccess = pcall(function()
        echo("oscTestBuffer2", multipleOsc)
      end)
      
      assert.is_true(testSuccess, "Multiple OSC sequences should be handled safely")
    end)
    
    it("should handle edge case escape sequences", function()
      createBuffer("oscTestBuffer3")
      clearWindow("oscTestBuffer3")
      
      -- Edge cases that might trigger the buffer underflow issue
      local edgeCases = {
        -- Just ESC ]
        string.char(27, 93),
        -- ESC ] with one character
        string.char(27, 93, 48),
        -- ESC ] 0 ; (incomplete)
        string.char(27, 93, 48, 59),
        -- Empty OSC
        string.char(27, 93, 27, 92)
      }
      
      for i, edgeCase in ipairs(edgeCases) do
        local testSuccess = pcall(function()
          clearWindow("oscTestBuffer3")
          echo("oscTestBuffer3", edgeCase)
        end)

        assert.is_true(testSuccess, "Edge case " .. i .. " should not crash")
      end
    end)

  end)

  describe("Tests ANSI string sequence handling (DCS, SOS, PM, APC)", function()

    it("should swallow an APC sequence terminated by ST", function()
      assert.is_true(feedTriggers("APCST1(\027_secret apc payload\027\\)APCST1\n"))
      assert.equals("APCST1()APCST1", findRecentLine("APCST1"))
    end)

    it("should swallow a DCS sequence terminated by ST", function()
      assert.is_true(feedTriggers("DCSST1(\027P+q544e\027\\)DCSST1\n"))
      assert.equals("DCSST1()DCSST1", findRecentLine("DCSST1"))
    end)

    it("should swallow PM and SOS sequences terminated by ST", function()
      assert.is_true(feedTriggers("PMSOS1(\027^privacy message\027\\|\027Xstart of string\027\\)PMSOS1\n"))
      assert.equals("PMSOS1(|)PMSOS1", findRecentLine("PMSOS1"))
    end)

    it("should swallow an APC sequence terminated by BEL", function()
      assert.is_true(feedTriggers("APCBEL1(\027_bel terminated\7)APCBEL1\n"))
      assert.equals("APCBEL1()APCBEL1", findRecentLine("APCBEL1"))
    end)

    -- As with the private-CSI case below, two feedTriggers() calls cannot express
    -- a packet split; what carries between local feeds is the mGotString latch,
    -- not the pending bytes.
    it("should swallow an APC sequence whose bytes arrive across two local feeds", function()
      assert.is_true(feedTriggers("APCSPLIT1(\027_first half "))
      assert.is_true(feedTriggers("second half\027\\)APCSPLIT1\n"))
      assert.equals("APCSPLIT1()APCSPLIT1", findRecentLine("APCSPLIT1"))
    end)

    it("should still render OSC 8 hyperlink text", function()
      assert.is_true(feedTriggers("OSCLINK1(\027]8;;https://example.com\027\\click me\027]8;;\027\\)OSCLINK1\n"))
      assert.equals("OSCLINK1(click me)OSCLINK1", findRecentLine("OSCLINK1"))
    end)

    it("should still consume OSC color sequences", function()
      assert.is_true(feedTriggers("OSCCOLOR1(\027]P1ff0000\027\\)OSCCOLOR1\n"))
      assert.equals("OSCCOLOR1()OSCCOLOR1", findRecentLine("OSCCOLOR1"))
    end)

    it("should not treat text after an unhandled two-byte escape as a sequence", function()
      assert.is_true(feedTriggers("STICKY1(\027" .. "7[not-a-csi)STICKY1\n"))
      assert.equals("STICKY1([not-a-csi)STICKY1", findRecentLine("STICKY1"))
    end)

  end)

  describe("Tests escape sequences that Mudlet does not handle", function()

    local previousEncoding

    -- these tests are not encoding agnostic: feedTriggers transcodes its UTF-8
    -- argument into the server encoding, so under anything else the "\195\169"
    -- pairs below reach the parser as a single byte and stop exercising the
    -- multibyte lead byte that it must not swallow
    setup(function()
      previousEncoding = getServerEncoding()
      setServerEncoding("UTF-8")
    end)

    teardown(function()
      setServerEncoding(previousEncoding)
    end)

    it("should consume the two-byte escapes it recognises", function()
      assert.is_true(feedTriggers("TWOBYTE1(\027" .. "7|\027" .. "8|\027c)TWOBYTE1\n"))
      assert.equals("TWOBYTE1(||)TWOBYTE1", findRecentLine("TWOBYTE1"))
    end)

    it("should keep the byte of a two-byte escape it does not recognise", function()
      assert.is_true(feedTriggers("UNKNOWN1(\027M\027D\027>\027=)UNKNOWN1\n"))
      assert.equals("UNKNOWN1(MD>=)UNKNOWN1", findRecentLine("UNKNOWN1"))
    end)

    it("should consume a character set designation", function()
      assert.is_true(feedTriggers("CHARSET1(\027(B)CHARSET1\n"))
      assert.equals("CHARSET1()CHARSET1", findRecentLine("CHARSET1"))
    end)

    it("should not let a CSI introducer name a character set and start a CSI", function()
      assert.is_true(feedTriggers("GUARD1(\027([31mred\027[0m)GUARD1\n"))
      assert.equals("GUARD1(31mred)GUARD1", findRecentLine("GUARD1"))
    end)

    it("should not let an APC introducer name a character set and start a string sequence", function()
      assert.is_true(feedTriggers("GUARD2(\027(_payload)GUARD2\n"))
      assert.equals("GUARD2(payload)GUARD2", findRecentLine("GUARD2"))
    end)

    it("should restart the sequence when an escape follows a designation", function()
      assert.is_true(feedTriggers("RELATCH1(\027(\027[31mred\027[0m)RELATCH1\n"))
      assert.equals("RELATCH1(red)RELATCH1", findRecentLine("RELATCH1"))
    end)

    it("should keep the letter after a stray escape", function()
      assert.is_true(feedTriggers("STRAY1(\027ABC)STRAY1\n"))
      assert.equals("STRAY1(ABC)STRAY1", findRecentLine("STRAY1"))
    end)

    it("should keep the digit after a stray escape", function()
      assert.is_true(feedTriggers("STRAY2(\027" .. "1234)STRAY2\n"))
      assert.equals("STRAY2(1234)STRAY2", findRecentLine("STRAY2"))
    end)

    it("should keep the text after several stray escapes", function()
      assert.is_true(feedTriggers("STRAY3(\027A\027BCD)STRAY3\n"))
      assert.equals("STRAY3(ABCD)STRAY3", findRecentLine("STRAY3"))
    end)

    it("should keep a run of punctuation after a stray escape", function()
      assert.is_true(feedTriggers("PUNCT1(\027--- Hello)PUNCT1\n"))
      assert.equals("PUNCT1(--- Hello)PUNCT1", findRecentLine("PUNCT1"))
    end)

    it("should keep a space after a stray escape", function()
      assert.is_true(feedTriggers("PUNCT2(\027 spaced)PUNCT2\n"))
      assert.equals("PUNCT2( spaced)PUNCT2", findRecentLine("PUNCT2"))
    end)

    it("should keep a multibyte character that follows a stray escape", function()
      assert.is_true(feedTriggers("UTF8ESC1(caf\027\195\169)UTF8ESC1\n"))
      assert.equals("UTF8ESC1(caf\195\169)UTF8ESC1", findRecentLine("UTF8ESC1"))
    end)

    it("should keep a multibyte character that cannot name a character set", function()
      assert.is_true(feedTriggers("UTF8ESC2(\027(\195\169)UTF8ESC2\n"))
      assert.equals("UTF8ESC2(\195\169)UTF8ESC2", findRecentLine("UTF8ESC2"))
    end)

    it("should keep a line break that follows a stray escape", function()
      assert.is_true(feedTriggers("NLESC1(\027\nNLESC2)\n"))
      assert.equals("NLESC1(", findRecentLine("NLESC1"))
      assert.equals("NLESC2)", findRecentLine("NLESC2"))
    end)

    it("should keep a line break that cannot name a character set", function()
      assert.is_true(feedTriggers("NLESC3(\027(\nNLESC4)\n"))
      assert.equals("NLESC3(", findRecentLine("NLESC3"))
      assert.equals("NLESC4)", findRecentLine("NLESC4"))
    end)

    it("should apply a trailing escape to the next packet", function()
      assert.is_true(feedTriggers("SPLITESC1(\027"))
      assert.is_true(feedTriggers("7 then ABC)SPLITESC1\n"))
      assert.equals("SPLITESC1( then ABC)SPLITESC1", findRecentLine("SPLITESC1"))
    end)

    it("should apply a trailing designation to the next packet", function()
      assert.is_true(feedTriggers("SPLITINT1(\027("))
      assert.is_true(feedTriggers("B)SPLITINT1\n"))
      assert.equals("SPLITINT1()SPLITINT1", findRecentLine("SPLITINT1"))
    end)

    it("should not eat a multibyte character starting the next packet", function()
      assert.is_true(feedTriggers("SPLITESC2(\027"))
      assert.is_true(feedTriggers("\195\169)SPLITESC2\n"))
      assert.equals("SPLITESC2(\195\169)SPLITESC2", findRecentLine("SPLITESC2"))
    end)

    it("should keep an 8-bit character that follows a stray escape", function()
      setServerEncoding("ISO 8859-1")
      assert.is_true(feedTriggers("LATIN1(\027\195\169)LATIN1\n"))
      assert.equals("LATIN1(\195\169)LATIN1", findRecentLine("LATIN1"))
      setServerEncoding("UTF-8")
    end)

  end)

  -- A CSI parameter string may only carry one of '<', '=', '>' or '?' in its
  -- FIRST byte, where it marks a private/reserved sequence that Mudlet does not
  -- interpret; after that only "0-9:;" are allowed. Getting those two sets the
  -- wrong way round leaves the tail of such a sequence on screen as game text.
  describe("Tests private/reserved CSI sequences", function()

    it("should consume a private DEC sequence that hides the cursor", function()
      assert.is_true(feedTriggers("CSIPRIV1(\027[?25l)CSIPRIV1\n"))
      assert.equals("CSIPRIV1()CSIPRIV1", findRecentLine("CSIPRIV1"))
    end)

    it("should consume a private DEC sequence with a multi-digit parameter", function()
      assert.is_true(feedTriggers("CSIPRIV2(\027[?1049h)CSIPRIV2\n"))
      assert.equals("CSIPRIV2()CSIPRIV2", findRecentLine("CSIPRIV2"))
    end)

    it("should consume a reserved sequence introduced by '<'", function()
      assert.is_true(feedTriggers("CSIPRIV3(\027[<0;10;10M)CSIPRIV3\n"))
      assert.equals("CSIPRIV3()CSIPRIV3", findRecentLine("CSIPRIV3"))
    end)

    -- Two feedTriggers() calls cannot express a packet split: a local feed keeps
    -- its own mGotCSI latch between calls but drops the incomplete bytes, since
    -- the carry is gated on isFromServer. So the latch swallows the "l" rather
    -- than "?25" surviving. This still fails without the fix - the "25l" leaks -
    -- but it does not guard the private branch's ordering against the
    -- incomplete-packet check, which needs a real split from the socket.
    it("should not leak a private sequence whose bytes arrive across two local feeds", function()
      assert.is_true(feedTriggers("CSISPLIT1(\027[?25"))
      assert.is_true(feedTriggers("l)CSISPLIT1\n"))
      assert.equals("CSISPLIT1()CSISPLIT1", findRecentLine("CSISPLIT1"))
    end)

    -- Guards the other direction: an ordinary digit-initial parameter string and
    -- an empty one must still reach the SGR handler and leave no text behind.
    it("should still consume a digit-initial SGR sequence", function()
      assert.is_true(feedTriggers("CSISGR1(\027[0;32mgreen\027[0m)CSISGR1\n"))
      assert.equals("CSISGR1(green)CSISGR1", findRecentLine("CSISGR1"))
    end)

    it("should still consume an SGR sequence with no parameters", function()
      assert.is_true(feedTriggers("CSISGR2(\027[mplain)CSISGR2\n"))
      assert.equals("CSISGR2(plain)CSISGR2", findRecentLine("CSISGR2"))
    end)

  end)

  -- A line written through TBuffer::appendLine() that holds the documentation
  -- phrase is dropped whole, and a banner of worked OSC 8 examples goes into
  -- the main console's buffer instead - whichever console the line was written
  -- to. Paths that do not reach appendLine() print the phrase as ordinary text:
  -- anything the game sends, and an echo from inside a trigger. The injection
  -- is debounced to a second of wall clock and the timestamp it keeps lives on
  -- the main buffer, so each spec here waits that window out before it starts.
  -- The phrase is built rather than written out below, and kept out of the
  -- describe and it names, so busted's own report cannot set it off.
  describe("Tests the OSC 8 documentation helper phrase", function()
    local phrase = "!osc8-" .. "docs"
    local otherWindow = "osc8DocsSpecWindow"
    -- pumpEvents() is inert outside test mode, so the debounce window below
    -- never elapses and the second injection would be suppressed
    local testMode = os.getenv("MUDLET_TEST_MODE")

    setup(function()
      createMiniConsole(otherWindow, 0, 0, 200, 100)
    end)

    teardown(function()
      deleteMiniConsole(otherWindow)
    end)

    -- everything main has been told since the marker line, found by scanning
    -- back for the marker rather than by holding on to an index: sixty lines of
    -- banner can push the main buffer over its limit, and the trim that follows
    -- moves every absolute index
    local function mainSinceMarker(marker)
      local lastLine = getLastLineNumber("main")
      local reversed = {}
      for lineNumber = lastLine, math.max(0, lastLine - 300), -1 do
        local line = getLines("main", lineNumber, lineNumber + 1)[1] or ""
        if line:find(marker, 1, true) then
          local ordered = {}
          for index = #reversed, 1, -1 do
            ordered[#ordered + 1] = reversed[index]
          end
          return ordered
        end
        reversed[#reversed + 1] = line
      end
      return nil
    end

    local function someLineHolds(lines, needle)
      for _, line in ipairs(lines) do
        if line:find(needle, 1, true) then
          return true
        end
      end
      return false
    end

    it("writes the worked examples into the main console", function()
      if not testMode then
        pending("waiting out the injection debounce needs MUDLET_TEST_MODE")
        return
      end
      pumpEvents(1100)
      echo("OSCDOCSMARKA\n")
      echo("OSCDOCS1 " .. phrase .. " OSCDOCS2")
      local injected = mainSinceMarker("OSCDOCSMARKA")
      assert.is_truthy(injected, "the marker line the examples follow is gone")
      assert.is_true(#injected > 10, "only " .. #injected .. " lines were injected")
      assert.is_true(someLineHolds(injected, "OSC 8 Hyperlink Examples"), table.concat(injected, "\n"))
      assert.is_true(someLineHolds(injected, "wiki.mudlet.org"), table.concat(injected, "\n"))
      -- what was written goes whole, not just the phrase out of the middle of it
      assert.is_false(someLineHolds(injected, "OSCDOCS1"))
      assert.is_false(someLineHolds(injected, "OSCDOCS2"))
    end)

    it("writes them into main whichever window they were asked for in", function()
      if not testMode then
        pending("waiting out the injection debounce needs MUDLET_TEST_MODE")
        return
      end
      pumpEvents(1100)
      clearWindow(otherWindow)
      echo("OSCDOCSMARKB\n")
      echo(otherWindow, phrase)
      local injected = mainSinceMarker("OSCDOCSMARKB")
      assert.is_truthy(injected, "the marker line the examples follow is gone")
      assert.is_true(someLineHolds(injected, "OSC 8 Hyperlink Examples"), table.concat(injected, "\n"))
      -- and nothing at all in the window the phrase was written to
      assert.are.equal("", getLines(otherWindow, 0, 1)[1] or "")
      -- the debounce timestamp lives on the main buffer, so the window the
      -- phrase arrived in does not get a window of its own
      echo(phrase)
      assert.are.equal(#injected, #mainSinceMarker("OSCDOCSMARKB"))
    end)
  end)

  -- Of the three visibility actions a link can carry, a delayed reveal is the
  -- only one that completes without a click: concealment waits to be clicked
  -- before its timer even starts, and reveal-then-conceal only gets through its
  -- reveal half unattended. The link is written concealed and the visibility
  -- manager puts its text back on the first tick of its 100ms poll past the
  -- delay, so the wait below is not the delay itself. The manager also restores
  -- the link indices at the same time, which no Lua call can read back - that
  -- half of the reveal is left to a functional test.
  describe("Tests OSC 8 hyperlink visibility expiry", function()
    local function lineHolding(needle)
      local lastLine = getLastLineNumber("main")
      for lineNumber = lastLine, math.max(0, lastLine - 20), -1 do
        local line = getLines("main", lineNumber, lineNumber + 1)[1]
        if line and line:find(needle, 1, true) then
          return lineNumber, line
        end
      end
      return nil
    end

    -- scans the whole buffer, unlike lineHolding()'s 20-line tail
    local function findLine(needle)
      for lineNumber = 0, getLastLineNumber("main") do
        local line = getLines("main", lineNumber, lineNumber + 1)[1]
        if line and line:find(needle, 1, true) then
          return lineNumber, line
        end
      end
      return nil
    end

    -- the buffer size is global state, so restore it even when the body throws or
    -- a failure here fails an unrelated spec later in the run
    local function withSmallBuffer(body)
      local wasLines, wasBatch = getConsoleBufferSize("main")
      setConsoleBufferSize("main", 100, 20)
      local ok, err = pcall(body)
      setConsoleBufferSize("main", wasLines, wasBatch)
      if not ok then error(err, 0) end
    end

    it("reveals a link that was written concealed once its delay is up", function()
      if not os.getenv("MUDLET_TEST_MODE") then
        -- pumpEvents() is inert outside test mode, so the reveal never fires
        pending("waiting for the reveal timer needs MUDLET_TEST_MODE")
        return
      end
      local link = "\027]8;;send:osc8reveal?config={\"visibility\":{\"action\":\"reveal\",\"delay\":250}}\027\\HIDDENWORD\027]8;;\027\\"
      assert.is_true(feedTriggers("OSCREVEAL1(" .. link .. ")OSCREVEAL1\n"))
      local lineNumber, concealed = lineHolding("OSCREVEAL1")
      assert.is_truthy(lineNumber, "the line carrying the link never reached the buffer")
      -- concealment keeps the character count identical so buffer indices stay
      -- valid, which is why the text is replaced space for space
      assert.equals("OSCREVEAL1(          )OSCREVEAL1", concealed)
      pumpEvents(700)
      assert.equals("OSCREVEAL1(HIDDENWORD)OSCREVEAL1", getLines("main", lineNumber, lineNumber + 1)[1])
    end)

    it("reveals a link on its own line after the buffer has trimmed", function()
      if not os.getenv("MUDLET_TEST_MODE") then
        pending("waiting for the reveal timer needs MUDLET_TEST_MODE")
        return
      end
      withSmallBuffer(function()
        local link = "\027]8;;send:osc8trim?config={\"visibility\":{\"action\":\"reveal\",\"delay\":250}}\027\\HIDDENWORD\027]8;;\027\\"
        assert.is_true(feedTriggers("OSCTRIM1(" .. link .. ")OSCTRIM1\n"))
        local before = findLine("OSCTRIM1")
        assert.is_truthy(before, "the line carrying the link never reached the buffer")

        for i = 1, 70 do echo("osctrimfiller " .. i .. "\n") end
        local moved = findLine("OSCTRIM1")
        assert.is_truthy(moved, "the link's line scrolled out entirely, so this test proves nothing")
        assert.is_not.equal(before, moved, "the buffer never trimmed, so this test proves nothing")

        pumpEvents(700)
        assert.equals("OSCTRIM1(HIDDENWORD)OSCTRIM1", getLines("main", moved, moved + 1)[1])
      end)
    end)

    it("does not rewrite an unrelated line when the link's own line was trimmed away", function()
      if not os.getenv("MUDLET_TEST_MODE") then
        pending("waiting for the reveal timer needs MUDLET_TEST_MODE")
        return
      end
      withSmallBuffer(function()
        -- the link has to sit at a LOW buffer index: its stale index is what a
        -- broken reveal writes to, and a high one falls outside the trimmed buffer
        -- where performReveal() no-ops instead of corrupting
        clearWindow()
        for i = 1, 3 do echo("oscgoneseed " .. i .. "\n") end
        local link = "\027]8;;send:osc8gone?config={\"visibility\":{\"action\":\"reveal\",\"delay\":3000}}\027\\HIDDENWORD\027]8;;\027\\"
        assert.is_true(feedTriggers("OSCGONE1(" .. link .. ")OSCGONE1\n"))
        local registeredAt = findLine("OSCGONE1")
        assert.is_truthy(registeredAt and registeredAt < 20, "the link did not land at a low buffer index")

        -- fillers must be longer than the link's startColumn + length, or
        -- performReveal() bounds-checks out and the case passes without the fix
        for i = 1, 260 do echo("oscgonefiller padded out well past the link column " .. i .. "\n") end
        assert.is_nil(findLine("OSCGONE1"), "the link's line survived, so this test proves nothing")

        local lastLine = getLastLineNumber("main")
        local snapshot = {}
        for lineNumber = 0, lastLine do
          snapshot[lineNumber] = getLines("main", lineNumber, lineNumber + 1)[1]
        end

        pumpEvents(3500)
        for lineNumber = 0, lastLine do
          assert.equals(snapshot[lineNumber], getLines("main", lineNumber, lineNumber + 1)[1],
            "revealing a link whose line was trimmed away rewrote line " .. lineNumber)
        end
      end)
    end)
  end)

end)
