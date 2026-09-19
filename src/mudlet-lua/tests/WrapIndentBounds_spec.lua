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

-- Wrap indents that take more of the line than they leave. An indent pads out
-- every line it applies to, so once it passes half the wrap width the padding
-- outgrows the text - and at a single usable column a line of text becomes one
-- buffer line per character, each carrying a full indent (#10458).
describe("Wrap indents wider than the wrap width leaves room for", function()
  local win = "wrapIndentCapTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
    setWindowWrap(win, 20)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  it("accepts an indent of exactly half the wrap width", function()
    assert.is_true(setWindowWrapIndent(win, 10))
    assert.is_true(setWindowWrapHangingIndent(win, 10))
  end)

  it("refuses an indent one column past half the wrap width, naming the limit", function()
    local ok, reason = setWindowWrapIndent(win, 11)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("11", 1, true))
    assert.is_truthy(reason:find("10", 1, true))
  end)

  it("refuses a hanging indent one column past half the wrap width", function()
    local ok, reason = setWindowWrapHangingIndent(win, 11)
    assert.is_nil(ok)
    assert.is_string(reason)
    assert.is_truthy(reason:find("11", 1, true))
  end)

  -- the reproducer from #10458: one usable column, so 20000 characters of text
  -- arrive as 19000 lines carrying 361000 characters of padding
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

-- The wrap width can be narrowed after an indent was set, so the pair the
-- wrapping is handed is still one the setters would have turned away. The
-- wrapping cuts such an indent back to what the width can carry rather than
-- applying it in full or dropping it.
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

  it("is cut back to half the narrowed width", function()
    assert.equals(string.rep(" ", 5) .. string.rep("a", 5), firstLineAfterNarrowing(40, 8, 10))
  end)

  -- an indent that reaches the width used to be discarded, rendering the line
  -- with no indent at all
  it("is cut back rather than dropped when it reaches the narrowed width", function()
    assert.equals(string.rep(" ", 5) .. string.rep("a", 5), firstLineAfterNarrowing(40, 20, 10))
  end)
end)
