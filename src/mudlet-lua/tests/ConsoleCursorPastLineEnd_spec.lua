-- The cursor can be moved past the end of its line. Writing there first pads
-- the line out to the cursor with spaces, and those take the console's current
-- format, whatever format the written text itself has.
describe("Tests that writing past the end of a line pads it in the console's current format", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local window = "specPastLineEnd" .. suffix
  local helper = "specPastLineEnd helper" .. suffix
  local current = {fg = {0, 200, 0}, bg = {0, 0, 128}}
  local pasted = {fg = {200, 0, 0}, bg = {40, 40, 40}}

  setup(function()
    assert.is_true(createMiniConsole(window, 0, 0, 300, 200))
    assert.is_true(createMiniConsole(helper, 0, 300, 300, 200))
    setWindowWrap(window, 200)
    setWindowWrap(helper, 200)
  end)

  teardown(function()
    deleteMiniConsole(window)
    deleteMiniConsole(helper)
  end)

  local function coloursAt(name, x, y)
    moveCursor(name, 0, y)
    selectSection(name, x, 1)
    local found = {fg = {getFgColor(name)}, bg = {getBgColor(name)}}
    deselect(name)
    return found
  end

  -- puts text in the pasted colours on the clipboard by way of the helper
  local function copyToClipboard(text)
    clearWindow(helper)
    deselect(helper)
    setFgColor(helper, unpack(pasted.fg))
    setBgColor(helper, unpack(pasted.bg))
    echo(helper, text .. "\n")
    resetFormat(helper)
    moveCursor(helper, 0, 0)
    selectCurrentLine(helper)
    copy(helper)
    deselect(helper)
  end

  -- "ab" with a line after it, so that the writes insert rather than append,
  -- and the cursor four columns past the end of "ab"
  before_each(function()
    clearWindow(window)
    deselect(window)
    resetFormat(window)
    echo(window, "ab\nyyy\n")
    setFgColor(window, unpack(current.fg))
    setBgColor(window, unpack(current.bg))
    assert.is_true(moveCursor(window, 6, 0))
  end)

  after_each(function()
    resetFormat(window)
  end)

  local function assertPaddedInTheCurrentFormat()
    for x = 2, 5 do
      assert.are.same(current, coloursAt(window, x, 0), ("column %d"):format(x))
    end
  end

  it("pads for insertText()", function()
    insertText(window, "X")
    assert.are.equal("ab    X", getLines(window, 0, 1)[1])
    assertPaddedInTheCurrentFormat()
  end)

  it("pads for insertLink() in the current format rather than the link's", function()
    insertLink(window, "L", "", "", false)
    assert.are.equal("ab    L", getLines(window, 0, 1)[1])
    assertPaddedInTheCurrentFormat()
    assert.are_not.same(current, coloursAt(window, 6, 0))
  end)

  it("pads for paste() of text in other colours", function()
    copyToClipboard("gamma")
    paste(window)
    assert.are.same({"ab    gamma", "yyy"}, getLines(window, 0, 2))
    assertPaddedInTheCurrentFormat()
    assert.are.same(pasted, coloursAt(window, 6, 0))
  end)

  -- A write there would pad a line with millions of spaces at a time, until memory runs out
  it("refuses a column farther past the end than one echo may write", function()
    local echoLimit = 1000000
    assert.is_false(moveCursor(window, 2147483647, 0))
    assert.is_false(moveCursor(window, 2 + echoLimit + 1, 0))
    assert.are.equal(6, getColumnNumber(window))
    assert.is_true(moveCursor(window, 2 + echoLimit, 0))
  end)

  it("leaves the rest of the main console's line as it was on cut()", function()
    local marker = "specPastLineEndCut" .. suffix
    deselect("main")
    resetFormat("main")
    echo("main", "lead ")
    setFgColor("main", unpack(pasted.fg))
    setBgColor("main", unpack(pasted.bg))
    echo("main", marker)
    resetFormat("main")
    echo("main", " tail\n")
    local y = getLastLineNumber("main") - 1
    local leadColours = coloursAt("main", 0, y)
    moveCursor("main", 0, y)
    assert.are.equal(5, selectString("main", marker, 1))
    cut()
    deselect("main")
    moveCursorEnd("main")
    assert.are.equal("lead  tail", getLines("main", y, y + 1)[1])
    for x = 0, 9 do
      assert.are.same(leadColours, coloursAt("main", x, y), ("column %d"):format(x))
    end
    moveCursorEnd("main")
    clearWindow(helper)
    appendBuffer(helper)
    assert.are.equal(marker, getLines(helper, 0, 1)[1])
    assert.are.same(pasted, coloursAt(helper, 0, 0))
  end)
end)
