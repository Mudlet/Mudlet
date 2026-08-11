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

    it("should swallow an APC sequence split across two packets", function()
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

end)
