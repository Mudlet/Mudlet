-- A buffer window's wrap width and indent live in its own buffer and nowhere
-- else: the profile holds the main console's, there is no getter to read a
-- window's back, and nothing re-applies them. A recolour used to copy the
-- profile's settings over a buffer window's, so whatever a script had asked for
-- was silently replaced with the main console's (#10865). changeColors() is
-- reached from an application theme change, a profile load and several
-- preferences paths as well as from the reset below, so this was easy to trip.

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
end)
