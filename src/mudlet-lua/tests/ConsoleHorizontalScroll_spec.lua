if not os.getenv("MUDLET_TEST_MODE") then

describe("A console's horizontal scroll range", function()
  it("needs test mode", function()
    pending("these specs wait for a console to paint, which needs pumpEvents()")
  end)
end)

else

-- The horizontal scroll bar only shows while some line is wider than the
-- console, and when it shows it takes height from the text pane, which is
-- the one way a script can see whether a console offers to scroll sideways.
describe("A console's horizontal scroll range", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local columns = 200
  local consoles

  local function make(role, y)
    local name = "specHScroll" .. role .. suffix
    createMiniConsole(name, 0, y, 100, 150)
    consoles[#consoles + 1] = name
    return name
  end

  before_each(function()
    consoles = {}
  end)

  after_each(function()
    for _, name in ipairs(consoles) do
      deleteMiniConsole(name)
    end
  end)

  -- Several sizes, as whether a whole-pixel cell is wider or narrower than the
  -- font's fractional average width, and so which way a mismatch errs, depends on it
  it("covers exactly the columns a line overruns the console by", function()
    local checked = 0
    for size = 8, 16 do
      consoles = {}
      local empty = make("Empty" .. size, 0)
      local fits = make("Fits" .. size, 160)
      local over = make("Over" .. size, 320)
      local long = make("Long" .. size, 480)
      local lines = {[fits] = columns, [over] = columns + 1, [long] = columns * 2}
      for _, name in ipairs(consoles) do
        setFontSize(name, size)
        local cellWidth = calcFontSize(name, true)
        resizeWindow(name, cellWidth * columns, 150)
        setWindowWrap(name, columns * 3)
        enableHorizontalScrollBar(name)
        if lines[name] then
          echo(name, string.rep("x", lines[name]) .. "\n")
        end
      end
      pumpEvents(100)
      pumpEvents(100)

      local emptyRows, longRows = getRowCount(empty), getRowCount(long)
      -- the scroll bar's height does not always round off to a row
      if longRows < emptyRows then
        checked = checked + 1
        assert.are.equal(emptyRows, getRowCount(fits), ("a line of exactly %d columns at %dpt"):format(columns, size))
        assert.are.equal(longRows, getRowCount(over), ("a line of %d columns at %dpt"):format(columns + 1, size))
      end
      for _, name in ipairs(consoles) do
        deleteMiniConsole(name)
      end
      consoles = {}
    end
    -- a frontend that draws no scroll bar leaves nothing to observe
    if checked == 0 then
      pending("no font size showed the scroll bar's height in the row count")
    end
  end)
end)

end
