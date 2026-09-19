-- Negative wrap indents. These setters used to accept one and pass it to
-- TBuffer::wrapLine(), which threw std::length_error once a line wrapped; that
-- clamp is covered by WrapLineRewrapTest, as they now refuse before reaching it.

describe("Wrap indents with a negative value", function()
  local win = "wrapIndentBoundsTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  it("refuses a negative wrap indent, with a reason", function()
    local ok, reason = setWindowWrapIndent(win, -2)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("-2", 1, true))
  end)

  it("refuses a negative hanging indent, with a reason", function()
    local ok, reason = setWindowWrapHangingIndent(win, -3)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("-3", 1, true))
  end)

  it("reports success, so a caller can tell the two apart", function()
    assert.is_true(setWindowWrapIndent(win, 2))
    assert.is_true(setWindowWrapHangingIndent(win, 0))
  end)

  -- the indent already in force surviving is what tells a refusal apart from
  -- quietly applying 0
  it("leaves the previous indent in force when a negative is refused", function()
    setWindowWrap(win, 20)
    assert.is_true(setWindowWrapIndent(win, 5))
    local _, reason = setWindowWrapIndent(win, -2)
    assert.is_string(reason)

    clearWindow(win)
    moveCursor(win, 0, 0)
    echo(win, string.rep("a", 30) .. "\n")
    assert.equals(string.rep(" ", 5) .. string.rep("a", 15), getLines(win, 0, 1)[1])
  end)
end)

-- Wrap indents wider than the wrap width leaves room for. An indent pads out
-- every line it applies to, so as it approaches the width what it pads out
-- outgrows the text - and at a single usable column a line of text becomes one
-- buffer line per character, each carrying a full indent (#10458). A quarter of
-- the width, rounded up, is kept for text.
describe("Wrap indents wider than the wrap width leaves room for", function()
  local win = "wrapIndentCapTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
    setWindowWrap(win, 20)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  it("accepts the widest indent the wrap width allows", function()
    assert.is_true(setWindowWrapIndent(win, 15))
    assert.is_true(setWindowWrapHangingIndent(win, 15))
    assert.equals(15, getWindowWrapIndent(win))
    assert.equals(15, getWindowWrapHangingIndent(win))
  end)

  it("refuses an indent one column past it, naming the value and the limit", function()
    local ok, reason = setWindowWrapIndent(win, 16)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("16", 1, true))
    assert.is_truthy(reason:find("15", 1, true))
  end)

  it("refuses a hanging indent one column past it", function()
    local ok, reason = setWindowWrapHangingIndent(win, 16)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("16", 1, true))
  end)

  -- what a refusal has to do that quietly applying 0 would not
  it("leaves the previous indent in force when a too-wide one is refused", function()
    assert.is_true(setWindowWrapIndent(win, 15))
    assert.is_nil(setWindowWrapIndent(win, 16))
    assert.equals(15, getWindowWrapIndent(win))

    clearWindow(win)
    moveCursor(win, 0, 0)
    echo(win, string.rep("a", 30) .. "\n")
    assert.equals(string.rep(" ", 15) .. string.rep("a", 5), getLines(win, 0, 1)[1])
  end)

  -- the shape measured in #10458: one usable column, so a line of text arrives
  -- as one buffer line per character and the padding dwarfs the text
  it("refuses an indent leaving a single usable column", function()
    assert.is_nil(setWindowWrapIndent(win, 19))
  end)

  -- an indent this wide used to be accepted and then dropped by the wrapping,
  -- so the window rendered with no indent at all and nothing said why
  it("refuses an indent at or beyond the wrap width", function()
    assert.is_nil(setWindowWrapIndent(win, 20))
    assert.is_nil(setWindowWrapIndent(win, 40))
  end)
end)

-- Narrowing the wrap width can leave an indent that was legal when it was set
-- wider than the new width allows. Geyser does exactly this on every resize of
-- an auto-wrapping window, so it is ordinary rather than exotic.
describe("A wrap indent left too wide by a later, narrower wrap width", function()
  local win = "wrapIndentNarrowedTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  local function firstLineAfterNarrowing(width, indent, narrowedTo)
    setWindowWrap(win, width)
    assert.is_true(setWindowWrapIndent(win, indent))
    assert.is_true(setWindowWrapHangingIndent(win, indent))
    setWindowWrap(win, narrowedTo)
    clearWindow(win)
    moveCursor(win, 0, 0)
    echo(win, string.rep("a", 40) .. "\n")
    return getLines(win, 0, 1)[1]
  end

  -- 20 keeps 5 columns for text, so 15 is as wide as the indent can be
  it("is cut back to what the narrowed width allows", function()
    assert.equals(string.rep(" ", 15) .. string.rep("a", 5), firstLineAfterNarrowing(100, 18, 20))
  end)

  -- an indent that reaches the width used to be discarded, rendering the line
  -- with no indent at all
  it("is cut back rather than dropped when it reaches the narrowed width", function()
    assert.equals(string.rep(" ", 15) .. string.rep("a", 5), firstLineAfterNarrowing(100, 40, 20))
  end)

  -- the window reports what it will use, so a script can tell this happened -
  -- narrowing the width is not itself refused, and nothing else would say
  it("reports the cut-back indent rather than the one that was asked for", function()
    setWindowWrap(win, 100)
    assert.is_true(setWindowWrapIndent(win, 40))
    assert.is_true(setWindowWrapHangingIndent(win, 40))
    setWindowWrap(win, 20)
    assert.equals(15, getWindowWrapIndent(win))
    assert.equals(15, getWindowWrapHangingIndent(win))
  end)

  -- the two are separate settings, so a regression applying one bound to the
  -- wrong member would otherwise pass every case above
  it("cuts the two indents back independently", function()
    setWindowWrap(win, 100)
    assert.is_true(setWindowWrapIndent(win, 0))
    assert.is_true(setWindowWrapHangingIndent(win, 40))
    setWindowWrap(win, 20)
    assert.equals(0, getWindowWrapIndent(win))
    assert.equals(15, getWindowWrapHangingIndent(win))

    clearWindow(win)
    moveCursor(win, 0, 0)
    echo(win, string.rep("a", 40) .. "\n")
    local lines = getLines(win, 0, 2)
    assert.equals(string.rep("a", 20), lines[1])
    assert.equals(string.rep(" ", 15) .. string.rep("a", 5), lines[2])
  end)
end)
