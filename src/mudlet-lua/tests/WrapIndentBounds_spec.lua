-- Negative wrap indents, which used to abort Mudlet: wrapLine() discarded an
-- indent wider than the wrap column, but let a negative one reach a
-- vector::insert() that takes an unsigned count, throwing std::length_error.

describe("Wrap indents with a negative value", function()
  local win = "wrapIndentBoundsTest"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  -- both setters return nothing on success, so the reason string is what tells
  -- a refusal apart, not the first return
  it("refuses a negative wrap indent, with a reason", function()
    local _, reason = setWindowWrapIndent(win, -2)
    assert.is_string(reason)
    assert.is_truthy(reason:find("-2", 1, true))
  end)

  it("refuses a negative hanging indent, with a reason", function()
    local _, reason = setWindowWrapHangingIndent(win, -3)
    assert.is_string(reason)
    assert.is_truthy(reason:find("-3", 1, true))
  end)

  it("keeps accepting a valid indent", function()
    local _, reason = setWindowWrapIndent(win, 2)
    assert.is_nil(reason)
    local _, hangingReason = setWindowWrapHangingIndent(win, 0)
    assert.is_nil(hangingReason)
  end)

  it("still wraps a line after a negative indent was refused", function()
    setWindowWrapIndent(win, -2)
    setWindowWrapHangingIndent(win, -3)
    setWindowWrap(win, 4)
    echo(win, "some text that needs to wrap several times over\n")
    assert.is_true(getLineCount(win) > 1)
  end)
end)
