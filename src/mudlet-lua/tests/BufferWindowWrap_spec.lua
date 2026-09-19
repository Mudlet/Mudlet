-- A buffer window's wrap width and indent live in its own buffer and nowhere
-- else: the profile holds the main console's, and nothing re-applies a window's.
-- A recolour used to copy the profile's settings over a buffer window's, so
-- whatever a script had asked for was silently replaced with the main console's
-- (#10865). Of changeColors()'s callers the ones that reach a buffer window are
-- an application theme change, which walks every sub-console, and the background
-- image reset used below; a profile load and the preferences paths only recolour
-- the main console.

local buffered = "bufferWrapSpecWindow"
-- the control: a miniconsole was never re-synced from the profile, so it says
-- whether a difference is this bug or the wrapping changing under both
local mini = "bufferWrapSpecControl"

local function wrappedFirstLine(win)
  clearWindow(win)
  moveCursor(win, 0, 0)
  echo(win, string.rep("a", 80) .. "\n")
  return getLines(win, 0, 1)[1]
end

local function applySettings(win)
  setWindowWrap(win, 40)
  assert.is_true(setWindowWrapIndent(win, 6))
  assert.is_true(setWindowWrapHangingIndent(win, 6))
end

describe("A window's own wrap settings across a recolour", function()
  -- 6 columns of indent then 34 of text, which is the wrap width of 40 less the
  -- indent - the profile's own settings would give one unindented line of 80
  local expected = string.rep(" ", 6) .. string.rep("a", 34)

  setup(function()
    createBuffer(buffered)
    createMiniConsole(mini, 0, 0, 400, 300)
    -- if the profile happened to wrap at 40 with a 6 column indent, the bug
    -- would replace these settings with identical ones and nothing would show
    assert.not_equals(40, getWindowWrap("main"))
    applySettings(buffered)
    applySettings(mini)
  end)

  teardown(function()
    deleteMiniConsole(buffered)
    deleteMiniConsole(mini)
  end)

  it("applies them to a buffer window to begin with", function()
    assert.equals(expected, wrappedFirstLine(buffered))
  end)

  it("keeps them on a buffer window after resetBackgroundImage()", function()
    resetBackgroundImage(buffered)
    assert.equals(expected, wrappedFirstLine(buffered))
  end)

  it("keeps them on a miniconsole after resetBackgroundImage()", function()
    resetBackgroundImage(mini)
    assert.equals(expected, wrappedFirstLine(mini))
  end)

  -- the width has a getter and the indents did not, which is what made this
  -- silent: a script could neither notice the revert nor put it back
  it("reports them unchanged as well", function()
    assert.equals(40, getWindowWrap(buffered))
    assert.equals(6, getWindowWrapIndent(buffered))
    assert.equals(6, getWindowWrapHangingIndent(buffered))
  end)
end)

-- A buffer window is a data store with no widget to measure, so unlike a
-- miniconsole it takes the profile's wrap width as its starting point and wraps
-- from the moment it is made. Nothing else supplies one, so losing this leaves
-- it at TBuffer's "do not wrap" default and a package reading it back gets one
-- very long line where it used to get wrapped ones.
describe("A buffer window that was never given a wrap width", function()
  local fresh = "bufferWrapSpecDefault"

  teardown(function()
    deleteMiniConsole(fresh)
  end)

  it("starts out wrapping at the profile's width", function()
    createBuffer(fresh)
    assert.equals(getWindowWrap("main"), getWindowWrap(fresh))
  end)

  it("wraps a long line rather than keeping it on one", function()
    clearWindow(fresh)
    moveCursor(fresh, 0, 0)
    echo(fresh, string.rep("a", getWindowWrap("main") * 3) .. "\n")
    assert.is_true(getLineCount(fresh) > 1, "a buffer window with the profile's wrap width did not wrap a line three times as long")
  end)
end)
