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
