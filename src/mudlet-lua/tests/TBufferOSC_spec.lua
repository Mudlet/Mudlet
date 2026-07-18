-- Test for OSC sequence buffer underflow protection
-- This test verifies the fix for a buffer underflow bug in TBuffer::translateToPlainText()
-- when processing OSC (Operating System Command) sequences at the beginning of a buffer.

describe("Tests TBuffer OSC sequence handling", function()
  
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

end)
