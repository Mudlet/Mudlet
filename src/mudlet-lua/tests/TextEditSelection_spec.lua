-- What the selection API does when the selection does not fit the line it is
-- pointed at: an empty line, a negative start, a selection left behind on a
-- line shorter than the one it was made on, and an attribute applied to part of
-- a line rather than all of it. Plus the overflow event that a console which
-- cannot scroll raises once its text no longer fits the pane.

-- Distinct lengths and words on every line, so an off-by-one or a swapped
-- axis produces a different answer rather than the right one by luck.
local fixture = {
  "alpha bravo charlie delta",
  "",
  "echo",
  "foxtrot golf hotel india juliet",
}

local function buildFixture(window)
  clearWindow(window)
  -- clearWindow() leaves one empty line behind and does not move the user
  -- cursor, so the line has to be deleted for line 0 to be the first echo
  moveCursor(window, 0, 0)
  deleteLine(window)
  for _, line in ipairs(fixture) do
    echo(window, line .. "\n")
  end
end

describe("Tests selection against lines it does not fit", function()
  local window = "textEditSelectionTest"

  setup(function()
    createMiniConsole(window, 0, 0, 400, 300)
    -- wider than the longest fixture line, so nothing wraps onto a second
    -- line and shifts every line number after it
    setWindowWrap(window, 80)
  end)

  teardown(function()
    deleteMiniConsole(window)
  end)

  before_each(function()
    -- setBold() and friends also change the format the window echoes with, so
    -- clear that before the fixture is written or a previous test's attribute
    -- ends up on every line of it
    deselect(window)
    resetFormat(window)
    buildFixture(window)
    assert.are.same(fixture, getLines(window, 0, #fixture))
  end)

  describe("Tests selectString on a line it cannot match", function()
    it("returns -1 and clears the selection when the line is empty", function()
      moveCursor(window, 0, 0)
      assert.equals(6, selectString(window, "bravo", 1))
      assert.equals("bravo", (getSelection(window)))

      moveCursor(window, 0, 1)
      assert.equals(-1, selectString(window, "bravo", 1))

      -- a bare -1 that left the old selection in place would report the
      -- stale start of 6, which no longer fits the empty line
      local text, start, length = getSelection(window)
      assert.equals("", text)
      assert.equals(0, start)
      assert.equals(0, length)
    end)

    it("returns -1 and clears the selection when the text is absent", function()
      moveCursor(window, 0, 3)
      assert.equals(8, selectString(window, "golf", 1))
      assert.equals("golf", (getSelection(window)))

      assert.equals(-1, selectString(window, "kilo", 1))
      local text, start, length = getSelection(window)
      assert.equals("", text)
      assert.equals(0, start)
      assert.equals(0, length)
    end)

    it("returns -1 when there are fewer matches than asked for", function()
      -- line 0 holds exactly five "a"s, the last of them the final character
      moveCursor(window, 0, 0)
      assert.equals(24, selectString(window, "a", 5))
      assert.equals(-1, selectString(window, "a", 6))
    end)
  end)

  describe("Tests selectSection rejecting out of range arguments", function()
    it("rejects a negative start without disturbing the selection", function()
      moveCursor(window, 0, 0)
      assert.is_true(selectSection(window, 6, 5))
      assert.equals("bravo", (getSelection(window)))

      assert.is_false(selectSection(window, -1, 5))
      assert.equals("bravo", (getSelection(window)))
    end)

    it("rejects a start past the end of the line without disturbing the selection", function()
      moveCursor(window, 0, 2)
      assert.is_true(selectSection(window, 0, 4))
      assert.equals("echo", (getSelection(window)))

      assert.is_false(selectSection(window, 5, 1))
      assert.equals("echo", (getSelection(window)))
    end)

    it("rejects a length that runs off the end of the line without disturbing the selection", function()
      moveCursor(window, 0, 2)
      assert.is_true(selectSection(window, 0, 4))
      assert.equals("echo", (getSelection(window)))

      assert.is_false(selectSection(window, 2, 3))
      assert.equals("echo", (getSelection(window)))
    end)
  end)

  describe("Tests getSelection after the cursor moves", function()
    it("reports the selection stale when the new line is too short for it", function()
      moveCursor(window, 0, 3)
      assert.is_true(selectSection(window, 19, 5))
      assert.equals("india", (getSelection(window)))

      -- the selection is still 19 characters in, which line 2 does not reach
      moveCursor(window, 0, 2)
      local text, message = getSelection(window)
      assert.is_nil(text)
      assert.is_string(message)
    end)

    it("reads the same columns off a line that is long enough", function()
      moveCursor(window, 0, 3)
      assert.is_true(selectSection(window, 6, 5))
      assert.equals("t gol", (getSelection(window)))

      moveCursor(window, 0, 0)
      local text, start, length = getSelection(window)
      assert.equals("bravo", text)
      assert.equals(6, start)
      assert.equals(5, length)
    end)
  end)

  describe("Tests attributes applied to part of a line", function()
    -- reading an attribute back needs the cursor on the character, and
    -- getTextFormat() prefers the selection over the cursor when one is live
    local function boldAt(column, line)
      deselect(window)
      moveCursor(window, column, line)
      return getTextFormat(window).bold
    end

    it("bolds only the selected columns", function()
      moveCursor(window, 0, 0)
      assert.is_true(selectSection(window, 6, 5))
      assert.is_false(getTextFormat(window).bold)

      setBold(window, true)
      assert.is_true(getTextFormat(window).bold)

      -- the character after the selection is what an attribute run that ran on
      -- to the end of the line would also have picked up
      assert.is_true(boldAt(6, 0))
      assert.is_true(boldAt(10, 0))
      assert.is_false(boldAt(11, 0))
      assert.is_false(boldAt(5, 0))
    end)

    it("bolds the last character when the selection reaches the end of the line", function()
      moveCursor(window, 0, 2)
      assert.is_true(selectSection(window, 0, 4))
      assert.is_false(getTextFormat(window).bold)

      setBold(window, true)
      assert.is_true(boldAt(0, 2))
      assert.is_true(boldAt(3, 2))
      assert.is_false(boldAt(0, 3))
    end)
  end)
end)

describe("Tests sysWindowOverflowEvent", function()
  local window = "textEditOverflowTest"
  local events = {}
  local handler

  setup(function()
    createMiniConsole(window, 0, 0, 400, 100)
    setWindowWrap(window, 80)
    handler = registerAnonymousEventHandler("sysWindowOverflowEvent", function(_, name, amount)
      events[#events + 1] = {window = name, amount = amount}
    end)
  end)

  teardown(function()
    if handler then
      killAnonymousEventHandler(handler)
    end
    deleteMiniConsole(window)
  end)

  before_each(function()
    events = {}
    clearWindow(window)
    moveCursor(window, 0, 0)
    deleteLine(window)
  end)

  -- echo one line at a time so the line count creeps up to the pane's capacity
  -- rather than jumping past it
  local function fillToCapacity(rows)
    while getLineCount(window) + 1 < rows do
      echo(window, "fill\n")
    end
    return getLineCount(window) + 1
  end

  it("raises the event only once the lines no longer fit", function()
    local rows = getRowCount(window)
    assert.is_true(rows >= 3, "the pane is too short to tell a full buffer from an overflowing one")
    disableScrolling(window)
    assert.is_false(scrollingActive(window))

    assert.equals(rows, fillToCapacity(rows))
    assert.equals(0, #events, "the event fired while the lines still fitted")

    echo(window, "one line too many\n")
    assert.equals(1, #events)
    assert.equals(window, events[1].window)
    assert.equals(1, events[1].amount)

    echo(window, "two lines too many\n")
    assert.equals(2, #events)
    assert.equals(2, events[2].amount)
  end)

  it("stays quiet while the window can scroll", function()
    local rows = getRowCount(window)
    enableScrolling(window)
    assert.is_true(scrollingActive(window))

    fillToCapacity(rows)
    for _ = 1, 3 do
      echo(window, "past the bottom\n")
    end
    assert.equals(0, #events)
  end)
end)
