-- Selections running backwards, which used to corrupt the heap: selectSection()
-- accepted a negative length, and replaceInLine() checked both columns were in
-- range but not that the start preceded the end, so erase() was handed a
-- reversed range. The abort lands at some later allocation, so a run getting
-- through this file and the ones after it is part of the signal.
--
-- Each refusal here is made in front of a selection that is already valid, so
-- what it leaves behind is observable: replace() then acts on that earlier
-- selection, which is what puts TBuffer::replaceInLine() on the path. The
-- ordering guard inside replaceInLine() itself is not reachable from Lua once
-- selectSection() refuses first, and is covered by MainConsoleSelectionTest.

describe("Selections running backwards", function()
  local win = "reversedSelectionTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  local function firstLine()
    return getLines(win, 0, 1)[1]
  end

  -- clearWindow() leaves one empty line behind, which the echo then fills, so
  -- the text under test is always line 0
  local function showLine(text)
    clearWindow(win)
    moveCursor(win, 0, 0)
    echo(win, text .. "\n")
    moveCursor(win, 0, 0)
  end

  it("refuses a negative length", function()
    showLine("a reasonably long line here")
    assert.is_false(selectSection(win, 10, -6))
  end)

  it("leaves the selection already in force alone when it refuses one", function()
    showLine("a reasonably long line here")
    assert.is_true(selectSection(win, 2, 10))
    assert.equals("reasonably", (getSelection(win)))

    assert.is_false(selectSection(win, 10, -6))
    assert.equals("reasonably", (getSelection(win)))

    -- so the replacement lands on the earlier selection, rather than on the
    -- default empty one that a wiped selection would leave replace() to reject
    -- before it reaches the buffer at all
    replace(win, "XXXX")
    assert.equals("a XXXX long line here", firstLine())
  end)

  -- from + to overflows for a large enough `to`, and wraps negative: written as
  -- `from + to > s` the length check passed, and the selection it handed back
  -- ended at INT_MIN, before its own start
  it("refuses a length that overflows past the end of the line", function()
    showLine("a reasonably long line here")
    assert.is_true(selectSection(win, 2, 10))

    assert.is_false(selectSection(win, 1, 2147483647))
    assert.equals("reasonably", (getSelection(win)))

    replace(win, "XXXX")
    assert.equals("a XXXX long line here", firstLine())
  end)

  it("still replaces a forward selection", function()
    showLine("a reasonably long line here")
    assert.is_true(selectSection(win, 2, 10))
    replace(win, "XXXX")
    assert.equals("a XXXX long line here", firstLine())
  end)
end)
