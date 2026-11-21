-- How an echo() that spans line feeds lands in a console's buffer: every line
-- it starts gets its own text and timestamp, every character the format the
-- echo was made in, and a line the wrap left waiting drops the spaces the echo
-- starts with, as the break it carries on from already ate those.

local win = "echoLineFeedsSpecWindow"

local function lines()
  return getLines(win, 0, getLineCount(win))
end

local function sameColor(a, b)
  return a[1] == b[1] and a[2] == b[2] and a[3] == b[3]
end

local styledFg, styledBg = {200, 10, 20}, {30, 40, 210}

-- One letter per character of the line: "S" for the styled format the specs
-- echo in, "p" for the plain one either side of it, "?" for anything else.
local function formats(lineNumber, text)
  moveCursor(win, 0, lineNumber)
  local out = {}
  for x = 0, #text - 1 do
    selectSection(win, x, 1)
    local fg, bg = {getFgColor(win)}, {getBgColor(win)}
    local bold = getTextFormat(win).bold
    if sameColor(fg, styledFg) and sameColor(bg, styledBg) and bold then
      out[#out + 1] = "S"
    elseif not sameColor(fg, styledFg) and not sameColor(bg, styledBg) and not bold then
      out[#out + 1] = "p"
    else
      out[#out + 1] = "?"
    end
  end
  deselect(win)
  return table.concat(out)
end

local function styledEcho(text)
  setFgColor(win, unpack(styledFg))
  setBgColor(win, unpack(styledBg))
  setBold(win, true)
  echo(win, text)
  resetFormat(win)
end

describe("Tests how an echo spanning line feeds lands in a console", function()
  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  before_each(function()
    clearWindow(win)
    resetFormat(win)
    setWindowWrap(win, 100)
  end)

  it("gives each line feed's line its own text", function()
    echo(win, "\nfirst\n\n  second\nthird\n")
    assert.are.same({"", "first", "", "  second", "third"}, lines())
  end)

  it("splits a long echo with many line feeds exactly where they fall", function()
    local expected = {}
    for i = 1, 300 do
      expected[i] = string.rep(string.char(97 + i % 26), i % 37)
    end
    echo(win, table.concat(expected, "\n") .. "\n")
    assert.are.same(expected, lines())
  end)

  it("gives every character of every line it spans the echo's format", function()
    echo(win, "pp")
    styledEcho("SS\nSSS\n\nS")
    echo(win, "pp\n")
    local out = lines()
    assert.are.same({"ppSS", "SSS", "", "Spp"}, out)
    -- each line's text spells out the format its characters should have
    for i, text in ipairs(out) do
      assert.are.equal(text, formats(i - 1, text))
    end
  end)

  it("stamps each line it starts with the time", function()
    echo(win, "one\ntwo\n\nthree\n")
    for line = 0, 3 do
      assert.matches("^%d%d:%d%d:%d%d%.%d%d%d $", getTimestamp(win, line))
    end
  end)

  describe("onto a line the wrap left waiting", function()
    -- the word that fills the second line pushes the spaces after it onto a
    -- third, which drops them and is left empty, waiting
    before_each(function()
      setWindowWrap(win, 8)
      echo(win, "aaaa bbbbbbbb   ")
      assert.are.same({"aaaa ", "bbbbbbbb", ""}, getLines(win, 0, 3))
    end)

    it("drops the spaces the echo starts with", function()
      echo(win, "  cccc\n")
      assert.are.same({"aaaa ", "bbbbbbbb", "cccc"}, lines())
    end)

    it("drops an echo of nothing but spaces and keeps waiting", function()
      echo(win, "   ")
      echo(win, " cccc\n")
      assert.are.same({"aaaa ", "bbbbbbbb", "cccc"}, lines())
    end)

    it("keeps the spaces that start a line a line feed in the same echo began", function()
      echo(win, "  cccc\n   dddd\n")
      assert.are.same({"aaaa ", "bbbbbbbb", "cccc", "   dddd"}, lines())
    end)

    it("keeps the waiting line's blank stamp and stamps the lines after it", function()
      echo(win, "  cccc\n   dddd\n")
      assert.matches("^%-+%s*$", getTimestamp(win, 2))
      assert.matches("^%d%d:%d%d:%d%d%.%d%d%d $", getTimestamp(win, 3))
    end)

    it("gives the characters it keeps the echo's format", function()
      styledEcho("  SS\n SS")
      echo(win, "p\n")
      assert.are.same({"aaaa ", "bbbbbbbb", "SS", " SSp"}, lines())
      assert.are.equal("SS", formats(2, "SS"))
      -- the space the second line starts with is kept, and styled
      assert.are.equal("SSSp", formats(3, " SSp"))
    end)
  end)
end)
