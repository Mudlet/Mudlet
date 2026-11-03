-- Lines are only put through the full Unicode line-breaking analysis when
-- they could reach the wrap column; these check that a line which has to
-- break never skips it.
describe("Tests wrapping of lines that hold wide characters", function()
  local win = "wrapWidthSpecConsole"
  local wide = "日" -- two columns

  setup(function()
    createMiniConsole(win, 0, 0, 800, 200)
    setWindowWrap(win, 40)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  before_each(function()
    clearWindow(win)
  end)

  local function contentLines()
    local lines = {}
    for _, line in ipairs(getLines(win, 0, getLineCount(win))) do
      if line ~= "" then
        lines[#lines + 1] = line
      end
    end
    return lines
  end

  it("keeps a line of wide characters that fits the width whole", function()
    echo(win, string.rep(wide, 15) .. "\n") -- 30 columns of a 40 column window
    assert.are.same({string.rep(wide, 15)}, contentLines())
  end)

  it("breaks a line of wide characters at the wrap column", function()
    -- 25 characters is well under 40, but they are 50 columns wide
    echo(win, string.rep(wide, 25) .. "\n")
    assert.are.same({string.rep(wide, 20), string.rep(wide, 5)}, contentLines())
  end)

  it("splits a short line of wide characters at a line feed inserted into it", function()
    echo(win, string.rep(wide, 10) .. "\n")
    moveCursor(win, 5, 0)
    insertText(win, "\n")
    assert.are.same({string.rep(wide, 5), string.rep(wide, 5)}, contentLines())
  end)
end)
