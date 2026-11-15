-- What styling a selection does to each character of the line it is on, read
-- back one character at a time. A line that arrived in one colour is stored as
-- a single format shared by all its characters, so styling all of it, part of
-- it, or a line already in several colours each takes a different route.

describe("Tests how styling a selection changes the characters it covers", function()
  local win = "consoleSelectionStyleSpecWindow"

  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  -- one entry per character of line lineNumber: its foreground, background and bold
  local function styles(lineNumber)
    local out = {}
    local text = getLines(win, lineNumber, lineNumber + 1)[1]
    for x = 0, utf8.len(text) - 1 do
      moveCursor(win, x, lineNumber)
      selectSection(win, x, 1)
      local format = getTextFormat(win)
      out[#out + 1] = table.concat(format.foreground, ",") .. "/" .. table.concat(format.background, ",") .. (format.bold and "/b" or "")
    end
    deselect(win)
    return out
  end

  -- the three lines every test works on, the middle one the one it styles
  local function threeLines()
    clearWindow(win)
    resetFormat(win)
    echo(win, "above\nmiddle\nbelow\n")
    return styles(0), styles(1), styles(2)
  end

  local function repeated(entry, count)
    local out = {}
    for index = 1, count do
      out[index] = entry
    end
    return out
  end

  local function selectMiddle(from, length)
    moveCursor(win, from, 1)
    selectSection(win, from, length)
  end

  it("restyles every character of a line selected whole", function()
    local above, middle, below = threeLines()
    local plain = middle[1]
    selectMiddle(0, 6)
    setFgColor(win, 255, 0, 0)
    setBgColor(win, 0, 0, 80)
    setBold(win, true)
    resetFormat(win)
    assert.are.same(repeated("255,0,0/0,0,80/b", 6), styles(1))
    assert.are.same(above, styles(0))
    assert.are.same(below, styles(2))
    assert.are.same(repeated(plain, 6), middle)
  end)

  it("restyles only the characters selected in part of a line", function()
    local _, middle = threeLines()
    local plain = middle[1]
    selectMiddle(2, 3)
    setFgColor(win, 255, 0, 0)
    resetFormat(win)
    local red = "255,0,0/" .. plain:match("/(.*)$")
    assert.are.same({plain, plain, red, red, red, plain}, styles(1))
  end)

  it("restyles a selection that ends where the line does, but starts after it begins", function()
    local _, middle = threeLines()
    local plain = middle[1]
    selectMiddle(4, 2)
    setBold(win, true)
    resetFormat(win)
    assert.are.same({plain, plain, plain, plain, plain .. "/b", plain .. "/b"}, styles(1))
  end)

  it("keeps the other styles of a line already in several colours", function()
    local _, middle = threeLines()
    local plainBackground = middle[1]:match("/(.*)$")
    selectMiddle(0, 2)
    setBgColor(win, 0, 80, 0)
    selectMiddle(0, 6)
    setFgColor(win, 255, 0, 0)
    resetFormat(win)
    local onGreen, onPlain = "255,0,0/0,80,0", "255,0,0/" .. plainBackground
    assert.are.same({onGreen, onGreen, onPlain, onPlain, onPlain, onPlain}, styles(1))
  end)

  it("restyles part of a line that was restyled whole before", function()
    local _, middle = threeLines()
    local plain = middle[1]
    selectMiddle(0, 6)
    setFgColor(win, 255, 0, 0)
    selectMiddle(1, 2)
    setFgColor(win, 0, 255, 0)
    resetFormat(win)
    local red, green = "255,0,0/" .. plain:match("/(.*)$"), "0,255,0/" .. plain:match("/(.*)$")
    assert.are.same({red, green, green, red, red, red}, styles(1))
  end)
end)
