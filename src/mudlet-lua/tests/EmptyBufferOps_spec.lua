-- Console and buffer operations run against a buffer holding zero lines, which
-- is what deleteLine() leaves behind on a console showing a single line.
--
-- Anything that then reads the last line indexes -1: a Qt assert in a debug
-- build, an out-of-bounds read in a release one. A crash aborts the whole
-- busted run, so this file completing is itself the signal; the assertions
-- additionally check each operation still does something sensible.

-- getLineCount() returns the last line's INDEX, so it reads 0 for an empty
-- buffer and for a one-line one alike. Asking for line 0 is what tells them
-- apart, and every case here depends on the buffer really being empty.
local function assertEmpty(window)
  assert.equals("ERROR: invalid line number", getLines(window, 0, 1)[1])
end

local function linesContain(window, needle)
  -- one past getLineCount(), which is an index, to take in the last line
  for _, line in ipairs(getLines(window, 0, getLineCount(window) + 1)) do
    if type(line) == "string" and line:find(needle, 1, true) then
      return true
    end
  end
  return false
end

describe("Console operations on a buffer emptied by deleteLine()", function()
  local win = "emptyBufferOpsTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  -- clearWindow() leaves exactly one empty line, and deleteLine() removes the
  -- line under the user cursor - which clearWindow() does not reset, so the
  -- cursor has to be put back on that line for the buffer to end up empty.
  local function empty()
    clearWindow(win)
    moveCursor(win, 0, 0)
    deleteLine(win)
  end

  before_each(function()
    empty()
    assertEmpty(win)
  end)

  describe("appending", function()
    it("echo appends its text", function()
      echo(win, "hello\n")
      assert.is_true(linesContain(win, "hello"), "echoed text is missing after append-on-empty")
    end)

    it("cecho appends its text", function()
      cecho(win, "<red>world\n")
      assert.is_true(linesContain(win, "world"), "cechoed text is missing after append-on-empty")
    end)

    it("decho appends its text", function()
      decho(win, "<255,0,0>decho\n")
      assert.is_true(linesContain(win, "decho"))
    end)

    it("hecho appends its text", function()
      hecho(win, "#ff0000hecho\n")
      assert.is_true(linesContain(win, "hecho"))
    end)

    it("echoLink appends a clickable line", function()
      echoLink(win, "linked", function() end, "hint", true)
      assert.is_true(linesContain(win, "linked"))
    end)

    it("appendBuffer copies the clipboard line", function()
      -- seeded rather than inherited: an empty clipboard takes appendBuffer()
      -- down its newline branch instead, which is not the path under test
      echo(win, "clipseed\n")
      selectString(win, "clipseed", 1)
      copy(win)
      empty()
      appendBuffer(win)
      assert.is_true(linesContain(win, "clipseed"))
    end)

    it("echo after echo keeps appending", function()
      echo(win, "first\n")
      echo(win, "second\n")
      assert.is_true(linesContain(win, "first"))
      assert.is_true(linesContain(win, "second"))
    end)
  end)

  describe("inserting", function()
    -- with no line to insert into, TConsole::insertText() and insertLink()
    -- both fall through to appending
    it("insertText appends instead", function()
      insertText(win, "spliced")
      assert.is_true(linesContain(win, "spliced"))
    end)

    it("insertLink appends instead", function()
      insertLink(win, "insertedLink", function() end, "hint", true)
      assert.is_true(linesContain(win, "insertedLink"))
    end)

    it("moveCursorEnd cannot move, and insertText still appends", function()
      -- moveCursorEnd() sizes the cursor from line 0, which is the invalid-line
      -- error string here, so the move is rejected and the cursor stays put
      moveCursorEnd(win)
      insertText(win, "atEnd")
      assert.is_true(linesContain(win, "atEnd"))
    end)

    it("moveCursor onto the missing line is rejected", function()
      assert.is_false(moveCursor(win, 0, 0))
    end)

    it("paste pastes the clipboard line", function()
      echo(win, "pasteseed\n")
      selectString(win, "pasteseed", 1)
      copy(win)
      empty()
      paste(win)
      assert.is_true(linesContain(win, "pasteseed"))
    end)
  end)

  describe("selecting and formatting", function()
    it("selectString finds nothing", function()
      assert.is_true(selectString(win, "nothing", 1) < 0)
      assertEmpty(win)
    end)

    it("selectCurrentLine then setFgColor is harmless", function()
      selectCurrentLine(win)
      setFgColor(win, 255, 0, 0)
      resetFormat(win)
      assertEmpty(win)
    end)

    it("selectSection then setBgColor is harmless", function()
      selectSection(win, 0, 5)
      setBgColor(win, 0, 0, 255)
      assertEmpty(win)
    end)

    it("setUnderline over an empty selection is harmless", function()
      selectSection(win, 0, 1)
      setUnderline(win, true)
      assertEmpty(win)
    end)

    it("replace over an empty selection is harmless", function()
      selectCurrentLine(win)
      replace(win, "replacement")
      assertEmpty(win)
    end)

    it("copy of an empty selection is harmless", function()
      selectCurrentLine(win)
      copy(win)
      assertEmpty(win)
    end)

    it("deselect is harmless", function()
      deselect(win)
      assertEmpty(win)
    end)
  end)

  describe("querying", function()
    it("getCurrentLine returns a string", function()
      assert.is_string(getCurrentLine(win))
    end)

    it("getLines returns a table", function()
      assert.is_table(getLines(win, 0, 1))
    end)

    it("getLastLineNumber and getLineNumber return numbers", function()
      assert.is_number(getLastLineNumber(win))
      assert.is_number(getLineNumber(win))
    end)

    it("getTextFormat returns without a line to read", function()
      selectCurrentLine(win)
      -- no format to report on an empty buffer, so a nil return is expected
      assert.has_no.errors(function() getTextFormat(win) end)
    end)
  end)

  describe("wrapping and clearing", function()
    it("setWindowWrap rewraps nothing", function()
      setWindowWrap(win, 20)
      assertEmpty(win)
    end)

    it("setWindowWrapIndent rewraps nothing", function()
      setWindowWrapIndent(win, 4)
      assertEmpty(win)
    end)

    it("wrapLine on the missing line is harmless", function()
      wrapLine(win, 0)
      assertEmpty(win)
    end)

    it("deleteLine again is harmless", function()
      deleteLine(win)
      assertEmpty(win)
    end)

    it("clearWindow restores a usable buffer", function()
      clearWindow(win)
      echo(win, "after clear\n")
      assert.is_true(linesContain(win, "after clear"))
    end)
  end)

  describe("wrapping is still applied after the buffer recovers", function()
    it("a long echoed line wraps", function()
      setWindowWrap(win, 20)
      setWindowWrapIndent(win, 0)
      echo(win, string.rep("word ", 12) .. "\n")
      assert.is_true(getLineCount(win) > 1, "long line was not wrapped after append-on-empty")
    end)
  end)
end)

describe("The main console after deleteLine() empties its buffer", function()
  -- Game text arriving at an emptied main console reaches a different last-line
  -- read to the one echo() takes: TBuffer::commitLineData().
  local function emptyMain()
    clearWindow()
    moveCursor("main", 0, 0)
    deleteLine()
    assertEmpty("main")
  end

  -- the buffer is left usable, but the user cursor is still on the line that
  -- was deleted, and nothing else resets it for the specs that run after this
  teardown(function()
    moveCursorEnd("main")
  end)

  -- kills the trigger even if the feed or an assertion raises, so a stray
  -- trigger cannot go on clearing the main console for the rest of the run
  local function withTrigger(pattern, body, feed)
    local id = tempRegexTrigger(pattern, body)
    local ok, err = pcall(feedTriggers, feed)
    killTrigger(id)
    assert.is_true(ok, tostring(err))
  end

  it("accepts fed game text", function()
    emptyMain()
    feedTriggers("fed after empty\n")
    assert.is_true(linesContain("main", "fed after empty"))
  end)

  it("accepts echo", function()
    emptyMain()
    echo("echoed after empty\n")
    assert.is_true(linesContain("main", "echoed after empty"))
  end)

  it("survives a trigger that empties the buffer as the line commits", function()
    withTrigger("emptyingTrigger", emptyMain, "emptyingTrigger\n")
    assert.is_number(getLineCount("main"))
  end)

  it("keeps the rest of a packet a gagging trigger emptied the buffer for", function()
    -- the production shape of this bug: the second line commits into the
    -- buffer the first line's trigger just emptied
    withTrigger("emptyingGag", emptyMain, "emptyingGag\nsecond line\n")
    assert.is_true(linesContain("main", "second line"))
  end)

  it("survives an echo from inside the trigger that emptied the buffer", function()
    -- TConsole::echo() takes its own branch while a trigger is running, not
    -- the one a top-level echo() reaches
    withTrigger("emptyingEcho", function()
      emptyMain()
      echo("from inside\n")
    end, "emptyingEcho\n")
    assert.is_true(linesContain("main", "from inside"))
  end)

  it("survives a command echoed into the buffer the same trigger emptied", function()
    -- echoing the command lands in TConsole::printCommand(), which reads the
    -- last line to decide whether it needs a newline of its own first
    withTrigger("emptyingSender", function()
      emptyMain()
      send("look", true)
    end, "emptyingSender\n")
    assert.is_number(getLineCount("main"))
  end)

  it("survives a command sent outside a trigger", function()
    -- the other half of printCommand(), which looks two lines back for a prompt
    emptyMain()
    send("look", true)
    assert.is_number(getLineCount("main"))
  end)
end)
