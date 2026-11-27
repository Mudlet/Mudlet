-- How Mudlet keeps Markdown from the game in its main console, beyond what the standard's test vectors cover
-- (those run in MarkdownVectors_spec.lua). Needs --offline for feedTelnet(), as the tests README says.

local ESC = "\27"
local ON = ESC .. "]67;1" .. ESC .. "\\"
local OFF = ESC .. "]67;0" .. ESC .. "\\"

local function feed(data)
  local ok, message = feedTelnet(data)
  assert(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))
end

local function lastLine(back)
  return getLastLineNumber("main") - (back or 1)
end

local function lineText(number)
  return getLines("main", number, number + 1)[1]
end

describe("Markdown from the game", function()
  before_each(function()
    feed(OFF)
  end)

  after_each(function()
    feed(OFF)
  end)

  it("is turned on by OSC 67 alone", function()
    for _, number in ipairs({"66", "68", "670", "6", "067"}) do
      feed(ESC .. "]" .. number .. ";1" .. ESC .. "\\# SpecOtherOsc" .. number .. "\n")
      assert.are.equal("# SpecOtherOsc" .. number, lineText(lastLine()))
      assert.is_nil(getMarkdownBlock("main", lastLine()), "OSC " .. number .. " turned Markdown on")
    end
  end)

  it("reads a line that arrives in two pieces as one line", function()
    feed(ON .. "SpecSplit hel")
    feed("lo there\n" .. OFF)
    assert.are.equal("SpecSplit hello there", lineText(lastLine()))
    local block = getMarkdownBlock("main", lastLine())
    assert.is_not_nil(block, "the joined line was not Markdown")
    assert.are.equal("paragraph", block.kind)
    assert.are.equal("SpecSplit hello there", block.text)
  end)

  it("tags every line of a flood inside one message", function()
    local lines = {}
    for index = 1, 400 do
      lines[index] = "SpecFlood line " .. index
    end
    feed(ON .. table.concat(lines, "\n") .. "\n" .. OFF)
    local first = lastLine(400)
    local message
    for index = 0, 399 do
      local block = getMarkdownBlock("main", first + index)
      assert.is_not_nil(block, "flood line " .. (index + 1) .. " was not Markdown")
      assert.are.equal("SpecFlood line " .. (index + 1), block.text)
      message = message or block.message
      assert.are.equal(message, block.message)
    end
  end)

  it("gives a line in a window other than main no block", function()
    createBuffer("SpecMarkdownOther")
    finally(function() deleteMiniConsole("SpecMarkdownOther") end)
    echo("SpecMarkdownOther", "SpecOtherLine\n")
    feed(ON .. "SpecMainOnly\n" .. OFF)
    assert.is_not_nil(getMarkdownBlock(lastLine()))
    local block, problem = getMarkdownBlock("SpecMarkdownOther", 0)
    assert.is_nil(block)
    assert.is_nil(problem, "a window that exists was reported as a problem")
  end)

  it("reports a window that does not exist", function()
    local block, problem = getMarkdownBlock("SpecNoSuchWindow", 0)
    assert.is_nil(block)
    assert.is_truthy(problem and problem:find("not found", 1, true), "no message said the window was missing")
  end)

  it("gives a paragraph's whole text from any of the rows wrapping split it into", function()
    local savedWrap = getWindowWrap("main")
    setWindowWrap("main", 20)
    finally(function() setWindowWrap("main", savedWrap) end)
    local text = "SpecWrapped one two three four five six seven eight"
    feed(ON .. text .. "\n" .. OFF)
    local last = lastLine()
    local first = last
    while getMarkdownBlock("main", first - 1) and getMarkdownBlock("main", first - 1).text == text do
      first = first - 1
    end
    assert.is_true(last > first, "the paragraph was not wrapped, so this proves nothing")
    for row = first, last do
      assert.are.equal(text, getMarkdownBlock("main", row).text)
    end
  end)

  local function formatOf(lineNumber, column)
    moveCursor("main", 0, lineNumber)
    selectSection("main", column, 1)
    local format = getTextFormat("main")
    deselect("main")
    return format
  end

  it("marks a code span's characters as code, and only those", function()
    feed(ON .. "SpecCode `look` now\n" .. OFF)
    local line = lastLine()
    assert.are.equal("SpecCode look now", lineText(line))
    assert.is_true(formatOf(line, 9).code, "a character of the code span was not marked as code")
    assert.is_false(formatOf(line, 14).code, "a character after the code span was marked as code")
  end)

  it("makes bold text bold without brightening its color", function()
    feed(ESC .. "[31mSpecRedPlain" .. ESC .. "[0m\n")
    local red = formatOf(lastLine(), 0).foreground
    feed(ON .. ESC .. "[31m**SpecRedBold**" .. ESC .. "[0m\n" .. OFF)
    local format = formatOf(lastLine(), 0)
    assert.is_true(format.bold)
    assert.are.same(red, format.foreground)
  end)
end)
