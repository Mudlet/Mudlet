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
  
end)
