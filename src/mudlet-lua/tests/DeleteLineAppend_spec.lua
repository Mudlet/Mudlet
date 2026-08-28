-- Regression tests: appending to a console whose buffer deleteLine() has
-- emptied must not crash.
--
-- deleteLine() on a console holding a single line drops its TBuffer to zero
-- lines, which two paths then read off the end of:
--
--   * TBuffer::append() computes the last line as buffer.size() - 1, i.e. index
--     -1. appendFormatted() and appendLine() guard for the empty buffer and
--     re-seed an empty line; append() did not, so echo/appendBuffer/insertText
--     after such a deleteLine indexed lineBuffer.at(-1).
--   * TBuffer::commitLineData() takes lineBuffer.back() for the line arriving
--     from the game, so the next line after such a deleteLine asserted on an
--     empty QList. Only the main console reaches this one.
--
-- Both are a Qt assert and abort in a debug build, an out-of-bounds QList read
-- in a release build.
--
-- Found by the buffer-manipulation fuzzer (BufferManipFuzz_spec.lua). Each case
-- below aborts the whole busted run without the fix, so the file completing is
-- itself the signal; the assertions additionally confirm the buffer recovers.
describe("Appending after deleteLine() empties a console buffer", function()
  local win = "deleteLineAppendTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  before_each(function()
    -- clearWindow() leaves exactly one empty line; deleteLine() then removes it,
    -- leaving the buffer with zero lines - the state that used to crash.
    clearWindow(win)
    deleteLine(win)
  end)

  local function linesContain(needle)
    local count = getLineCount(win)
    for _, line in ipairs(getLines(win, 0, count)) do
      if type(line) == "string" and line:find(needle, 1, true) then
        return true
      end
    end
    return false
  end

  it("echo does not crash and appends its text", function()
    echo(win, "hello\n")
    assert.is_true(getLineCount(win) >= 1)
    assert.is_true(linesContain("hello"), "echoed text is missing after append-on-empty")
  end)

  it("cecho does not crash and appends its text", function()
    cecho(win, "<red>world\n")
    assert.is_true(getLineCount(win) >= 1)
    assert.is_true(linesContain("world"), "cechoed text is missing after append-on-empty")
  end)

  it("insertText does not crash and leaves the buffer queryable", function()
    -- insertText places text at the cursor, which deleteLine() has left pointing
    -- at the removed line, so on an emptied buffer it is a no-op rather than an
    -- append - the buffer can legitimately stay at zero lines. Reaching this
    -- assertion at all proves it did not abort, which is what this guards against.
    insertText(win, "spliced")
    assert.is_number(getLineCount(win))
  end)

  it("appendBuffer does not crash", function()
    -- appendBuffer() copies the main console's clipboard; with the buffer empty
    -- it reaches TBuffer::append() via the empty-clipboard newline path, which is
    -- exactly where the fuzzer first tripped the crash.
    appendBuffer(win)
    assert.is_true(getLineCount(win) >= 1)
  end)
end)

describe("Feeding the main console after deleteLine() empties its buffer", function()
  -- The miniconsole cases above cover TBuffer::append(); a line arriving from
  -- the game instead goes through TBuffer::commitLineData(), which only the main
  -- console has. clearWindow() leaves exactly one empty line and deleteLine()
  -- removes it, so the feed below lands on a buffer with no lines at all.
  it("does not crash and keeps accepting lines", function()
    clearWindow()
    deleteLine()
    feedTriggers("recovered\n")
    assert.is_number(getLineCount())
  end)
end)
