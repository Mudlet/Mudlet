-- moveCursor() accepts a column on an existing line up to one echo past its
-- end. The cursor move itself leaves the line alone; insertText() at such a
-- column pads the gap with spaces so the text lands at that column, while
-- echo() ignores the user cursor and appends.
describe("moveCursor() past the end of a line", function()
  describe("on a mini console", function()
    local win = "moveCursorPastEndTest"

    setup(function()
      createMiniConsole(win, 0, 0, 400, 300)
    end)

    teardown(function()
      deleteMiniConsole(win)
    end)

    local function lineAt(y)
      return getLines(win, y, y + 1)[1]
    end

    -- two lines, so the edited one is not the last
    before_each(function()
      clearWindow(win)
      echo(win, "abc\ndef")
      assert.are.equal("abc", lineAt(0))
      assert.are.equal("def", lineAt(1))
    end)

    it("is accepted and leaves the line unchanged", function()
      assert.is_true(moveCursor(win, 10, 0))
      assert.are.equal(10, getColumnNumber(win))
      assert.are.equal(0, getLineNumber(win))
      assert.are.equal("abc", lineAt(0))
    end)

    it("inside the line leaves it unchanged", function()
      assert.is_true(moveCursor(win, 1, 0))
      assert.are.equal("abc", lineAt(0))
    end)

    it("then insertText() pads the line so the text lands at that column", function()
      moveCursor(win, 6, 0)
      insertText(win, "X")
      assert.are.equal("abc   X", lineAt(0))
      assert.are.equal("def", lineAt(1))
    end)

    it("one past the last character, then insertText() appends without padding", function()
      moveCursor(win, 3, 0)
      insertText(win, "X")
      assert.are.equal("abcX", lineAt(0))
    end)

    it("on the last line, then insertText() pads it too", function()
      moveCursor(win, 5, 1)
      insertText(win, "X")
      assert.are.equal("def  X", lineAt(1))
    end)

    it("then echo() ignores the cursor and appends to the last line", function()
      moveCursor(win, 6, 0)
      echo(win, "X")
      assert.are.equal("abc", lineAt(0))
      assert.are.equal("defX", lineAt(1))
    end)
  end)

  describe("on the main console", function()
    teardown(function()
      moveCursorEnd("main")
    end)

    it("then insertText() pads the line so the text lands at that column", function()
      finally(function() moveCursorEnd("main") end)
      echo("\nmcPastEndMain\n")
      local y = getLastLineNumber("main") - 1
      assert.is_true(moveCursor("main", 16, y))
      assert.are.equal("mcPastEndMain", getCurrentLine())
      insertText("main", "X")
      assert.are.equal("mcPastEndMain   X", getCurrentLine())
    end)

    describe("from inside a trigger", function()
      local function inTrigger(body)
        local result = {}
        local id = tempRegexTrigger("^mcPastEnd$", function()
          body(result)
          result.after = getCurrentLine()
        end, 1)
        finally(function() killTrigger(id) end)
        feedTriggers("\nmcPastEnd\n")
        return result
      end

      it("is accepted and leaves the line unchanged", function()
        local result = inTrigger(function(r)
          r.moved = moveCursor(12, getLineNumber())
          r.column = getColumnNumber()
        end)
        assert.is_true(result.moved)
        assert.are.equal(12, result.column)
        assert.are.equal("mcPastEnd", result.after)
      end)

      it("then insertText() pads the line so the text lands at that column", function()
        local result = inTrigger(function()
          moveCursor(12, getLineNumber())
          insertText("X")
        end)
        assert.are.equal("mcPastEnd   X", result.after)
      end)

      it("then echo() ignores the cursor and appends to the trigger line", function()
        local result = inTrigger(function()
          moveCursor(12, getLineNumber())
          echo("X")
        end)
        assert.are.equal("mcPastEndX", result.after)
      end)
    end)
  end)
end)
