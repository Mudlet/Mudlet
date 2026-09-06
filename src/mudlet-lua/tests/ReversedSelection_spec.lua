-- Selections running backwards, which used to corrupt the heap: selectSection()
-- accepted a negative length, and replaceInLine() checked both columns were in
-- range but not that the start preceded the end, so erase() was handed a
-- reversed range. The abort lands at some later allocation, so a run getting
-- through this file and the ones after it is part of the signal.

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

  it("leaves the buffer intact when the selection was refused", function()
    showLine("a reasonably long line here")
    selectSection(win, 10, -6)
    replace(win, "XXXX")
    assert.equals("a reasonably long line here", firstLine())
  end)

  it("still replaces a forward selection", function()
    showLine("a reasonably long line here")
    assert.is_true(selectSection(win, 2, 10))
    replace(win, "XXXX")
    assert.equals("a XXXX long line here", firstLine())
  end)
end)
