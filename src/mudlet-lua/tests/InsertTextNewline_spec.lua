-- Regression tests for insertText newline handling
-- https://github.com/Mudlet/Mudlet/issues/8945
-- https://github.com/Mudlet/Mudlet/issues/8824
describe("Tests insertText and creplaceLine newline regressions", function()

  -- https://github.com/Mudlet/Mudlet/issues/8945
  -- insertText with newlines should create new lines, not insert literal \n
  describe("Tests the functionality of insertText", function()
    local consoleName = "insertTextTest"

    setup(function()
      createMiniConsole(consoleName, 0, 0, 800, 200)
      setBackgroundColor(consoleName, 0, 0, 0, 255)
      setMiniConsoleFontSize(consoleName, 12)
      setWindowWrap(consoleName, 60)
    end)

    before_each(function()
      clearWindow(consoleName)
    end)

    teardown(function()
      hideWindow(consoleName)
    end)

    it("should create new lines when text contains newlines", function()
      -- echo two lines of initial content
      echo(consoleName, "line1\n")
      echo(consoleName, "line2\n")

      local lineCountBefore = getLineCount(consoleName)

      -- move cursor to beginning and insert text with a newline
      moveCursor(consoleName, 0, 0)
      insertText(consoleName, "inserted line\n")

      local lineCountAfter = getLineCount(consoleName)

      -- inserting text with \n should increase the line count
      assert.is_true(lineCountAfter > lineCountBefore,
        "insertText with \\n should create a new line, but line count went from "
        .. lineCountBefore .. " to " .. lineCountAfter)
    end)

    it("should split content correctly when inserting newline in the middle of a line", function()
      echo(consoleName, "HelloWorld\n")

      -- move cursor to position 5 (between "Hello" and "World")
      moveCursor(consoleName, 5, 0)
      insertText(consoleName, "\n")

      -- line 0 should now be "Hello" and line 1 should start with "World"
      moveCursor(consoleName, 0, 0)
      selectCurrentLine(consoleName)
      local firstLine = getCurrentLine(consoleName)
      deselect(consoleName)

      moveCursor(consoleName, 0, 1)
      selectCurrentLine(consoleName)
      local secondLine = getCurrentLine(consoleName)
      deselect(consoleName)

      assert.are.equal("Hello", firstLine,
        "First line should be 'Hello' after inserting newline, got '" .. tostring(firstLine) .. "'")
      assert.are.equal("World", secondLine,
        "Second line should be 'World' after inserting newline, got '" .. tostring(secondLine) .. "'")
    end)

    it("should handle the sample code from issue 8945", function()
      -- Simplified reproduction from the issue report
      echo(consoleName, "test1---line1\n")
      echo(consoleName, "test1---line2\n")
      echo(consoleName, "test1---line3\n")

      local lineCountBefore = getLineCount(consoleName)

      -- insert a line at pos 0,0 with a trailing newline (from the issue)
      moveCursor(consoleName, 0, 0)
      insertText(consoleName, "------- line inserted at: 0/0 -----\n")

      local lineCountAfter = getLineCount(consoleName)

      assert.is_true(lineCountAfter > lineCountBefore,
        "insertText with \\n from issue sample should create a new line, but line count went from "
        .. lineCountBefore .. " to " .. lineCountAfter)

      -- the inserted text should be on its own line, not concatenated with line1
      moveCursor(consoleName, 0, 0)
      selectCurrentLine(consoleName)
      local firstLine = getCurrentLine(consoleName)
      deselect(consoleName)

      assert.are.equal("------- line inserted at: 0/0 -----", firstLine,
        "First line should be the inserted text, got '" .. tostring(firstLine) .. "'")
    end)

    -- https://github.com/Mudlet/Mudlet/issues/8945
    -- cinsertText was also reported as broken with newlines
    it("should create new lines with cinsertText", function()
      echo(consoleName, "line1\n")
      echo(consoleName, "line2\n")

      local lineCountBefore = getLineCount(consoleName)

      moveCursor(consoleName, 0, 0)
      cinsertText(consoleName, "<red>inserted line\n")

      local lineCountAfter = getLineCount(consoleName)

      assert.is_true(lineCountAfter > lineCountBefore,
        "cinsertText with \\n should create a new line, but line count went from "
        .. lineCountBefore .. " to " .. lineCountAfter)
    end)
  end)

  -- https://github.com/Mudlet/Mudlet/pull/9022#issuecomment-4163011131
  -- Word wrapping should not accumulate line width across newline boundaries
  describe("Tests that echo newlines reset wrap width tracking", function()

    it("should not word-wrap trigger echoes when each line is under wrap width", function()
      local triggerId = tempTrigger("TEST_WRAP_XPOS", function()
        -- Each line is ~47 chars, well under the default 80 wrap width.
        -- Combined they exceed 80. If xPos doesn't reset at the newline,
        -- the second line gets incorrectly word-wrapped.
        cecho("\n ============= /\\  /\\  /\\  /\\ =============")
        cecho("\n ============= \\/  \\/  \\/  \\/ =============")
      end)

      feedTriggers("TEST_WRAP_XPOS\n")
      killTrigger(triggerId)

      local lineCount = getLineCount()
      for i = lineCount - 1, math.max(0, lineCount - 10), -1 do
        moveCursor(0, i)
        selectCurrentLine()
        local line = getCurrentLine()
        deselect()
        if string.find(line, "============= \\/", 1, true) then
          assert.truthy(string.find(line, "\\/ =============", 1, true),
            "Line should not be word-wrapped, got '" .. tostring(line) .. "'")
          break
        end
      end
    end)

    it("should not accumulate xPos across multiple trigger echo newlines", function()
      local triggerId = tempTrigger("TEST_WRAP_ACCUM", function()
        -- 5 lines of 30 chars. If xPos accumulates: 150 > 80, causing wraps.
        -- Correct behavior: each line is 30 chars, no wrapping needed.
        for i = 1, 5 do
          echo("\n" .. string.rep(tostring(i), 30))
        end
      end)

      feedTriggers("TEST_WRAP_ACCUM\n")
      killTrigger(triggerId)

      local lineCount = getLineCount()
      local splitFound = false
      for i = lineCount - 1, math.max(0, lineCount - 15), -1 do
        moveCursor(0, i)
        selectCurrentLine()
        local line = getCurrentLine()
        deselect()
        -- A wrapped fragment would be shorter than 30 chars but contain our pattern
        if line:match("^%d+$") and #line < 28 and #line > 0 then
          splitFound = true
          break
        end
      end
      assert.is_false(splitFound,
        "Found a split line fragment - xPos is accumulating across newlines")
    end)
  end)

  -- https://github.com/Mudlet/Mudlet/issues/8824
  -- cecho behavior change with newlines and creplaceLine
  -- These tests use feedTriggers + tempTrigger to get proper trigger context,
  -- since echo/cecho only operate at cursor position within triggers.
  describe("Tests cecho with creplaceLine in trigger context", function()

    -- Reproduction from the issue: cecho("\n") then creplaceLine then cecho
    -- should place subsequent cecho text on the replaced line, not a new one
    it("should place cecho text on same line after creplaceLine", function()
      local triggerId = tempTrigger("TEST_8824_REPLACE", function()
        cecho("\n")
        selectCurrentLine()
        creplaceLine("<red>REPLACED")
        cecho("(cecho)")
      end)

      feedTriggers("TEST_8824_REPLACE\n")
      killTrigger(triggerId)

      -- search recent main console lines for the result
      local lineCount = getLineCount()
      local found = false
      for i = lineCount - 1, math.max(0, lineCount - 10), -1 do
        moveCursor(0, i)
        selectCurrentLine()
        local line = getCurrentLine()
        deselect()
        if string.find(line, "REPLACED", 1, true) then
          assert.truthy(string.find(line, "(cecho)", 1, true),
            "Line with 'REPLACED' should also contain '(cecho)', got '" .. tostring(line) .. "'")
          found = true
          break
        end
      end
      assert.is_true(found, "No line contained 'REPLACED' in main console")
    end)

    -- Harrison-Teeg's reproduction: original echoed text should not bleed
    -- through after creplaceLine
    it("should not show original echo text after creplaceLine", function()
      local triggerId = tempTrigger("TEST_8824_BLEED", function()
        echo("\n cecho before lineselection / replace...")
        selectCurrentLine()
        creplaceLine("<red>REPLACED")
        cecho("\n cecho after")
      end)

      feedTriggers("TEST_8824_BLEED\n")
      killTrigger(triggerId)

      -- find the line with "REPLACED" and verify original text is gone
      local lineCount = getLineCount()
      for i = lineCount - 1, math.max(0, lineCount - 10), -1 do
        moveCursor(0, i)
        selectCurrentLine()
        local line = getCurrentLine()
        deselect()
        if string.find(line, "REPLACED", 1, true) then
          assert.falsy(string.find(line, "cecho before", 1, true),
            "Original echo text should not bleed through after creplaceLine, got '" .. tostring(line) .. "'")
          break
        end
      end
    end)
  end)
end)
