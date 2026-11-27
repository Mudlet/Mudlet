-- How Mudlet keeps Markdown from the game in its main console, beyond what the standard's test vectors cover
-- (those run in MarkdownVectors_spec.lua). Needs --offline for feedTelnet(), as the tests README says.

local ESC = "\27"
local ON = ESC .. "]67;1" .. ESC .. "\\"
local OFF = ESC .. "]67;0" .. ESC .. "\\"

local function feed(data)
  local ok, message = feedTelnet(data)
  assert(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))
  -- Until the game sends a GA, what follows the last newline waits for the game to go quiet (300ms)
  if data:sub(-1) ~= "\n" then
    assert(pumpEvents(600), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
  end
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
    -- Straight on, as two packets of one read would be: a wait would let Mudlet show the first piece on its own
    assert(feedTelnet(ON .. "SpecSplit hel"))
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

  -- Held rows are committed by a timer once the game goes quiet, so pump until done says they are in
  local function settle(done)
    local waited = 0
    while waited < 3000 and not done() do
      assert.is_true(pumpEvents(100), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
      waited = waited + 100
    end
  end

  it("shows each row of a table to triggers once, in the form it is shown in", function()
    local seen = {}
    local id = tempTrigger("SpecTriggerRow", function() seen[#seen + 1] = line end)
    finally(function() killTrigger(id) end)
    feed(ON .. "| SpecTriggerRowHead | x |\n|---|---|\n| SpecTriggerRowA | 1 |\n| SpecTriggerRowB | 22 |\n" .. OFF)
    assert.are.equal(3, #seen)
    assert.are.same({lineText(lastLine(4)), lineText(lastLine(2)), lineText(lastLine())}, seen)
  end)

  it("shows a table's rows once the game goes quiet, and pads later rows to them", function()
    local header = getLastLineNumber("main")
    feed(ON .. "| SpecQuiet | b |\n|---|---|\n| a1 | b1 |\n")
    assert.are.equal(header, getLastLineNumber("main"), "the rows were shown before the table could have ended")
    settle(function() return getLastLineNumber("main") >= header + 3 end)
    assert.are.equal(header + 3, getLastLineNumber("main"))
    feed("| a2 | b2 |\n")
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("tableRow", row.kind)
    assert.are.equal("b2", row.cells[2].text)
    local head = getMarkdownBlock("main", header)
    assert.are.equal(head.table, row.table)
    assert.are.equal(head.cells[2].start, row.cells[2].start)
  end)

  it("holds no more than 500 lines of a table", function()
    local rows = {}
    for index = 1, 501 do
      rows[index] = "| SpecHeldRow" .. index .. " |"
    end
    local header = getLastLineNumber("main")
    feed(ON .. "| SpecHeld |\n|---|\n" .. table.concat(rows, "\n") .. "\n")
    assert.are.equal(header + 503, getLastLineNumber("main"))
    assert.are.equal("SpecHeldRow501", getMarkdownBlock("main", lastLine()).cells[1].text)
  end)

  it("leaves text a script feeds as it is, and the game's open list as it was", function()
    feed(ON .. "- SpecServerItem\n")
    feedTriggers("# SpecLocal\n")
    local fed = lastLine()
    feed("  SpecServerMore\n" .. OFF)
    assert.are.equal("# SpecLocal", lineText(fed))
    assert.is_nil(getMarkdownBlock("main", fed), "a line a script fed was read as Markdown")
    local more = getMarkdownBlock("main", lastLine())
    assert.are.equal("continuation", more.kind)
  end)

  local function linksOf(markdown)
    feed(ON .. markdown .. "\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    return block.text, block.links or {}
  end

  it("makes no link of a label that holds a link already", function()
    local text, links = linksOf("[<https://example.com>](send:y)")
    assert.are.equal("[https://example.com](send:y)", text)
    assert.are.equal(1, #links)
    assert.are.equal("https://example.com", links[1].destination)
  end)

  it("reads an escaped character as part of a bare address", function()
    local text, links = linksOf("see https://example.com/a\\_b now")
    assert.are.equal("see https://example.com/a_b now", text)
    assert.are.equal(1, #links)
    assert.are.equal("https://example.com/a_b", links[1].destination)
  end)

  it("makes no link of an address an OSC 8 link covers", function()
    local _, links = linksOf(ESC .. "]8;;send:x" .. ESC .. "\\https://example.com" .. ESC .. "]8;;" .. ESC .. "\\")
    assert.are.equal(1, #links)
    assert.are.equal("send:x", links[1].destination)
  end)

  it("makes one link of a destination between angle brackets that looks like an address", function()
    local text, links = linksOf("[a](<https://example.com>)")
    assert.are.equal("a", text)
    assert.are.equal(1, #links)
    assert.are.equal("https://example.com", links[1].destination)
  end)

  it("changes only the named message's mark, and runs no trigger", function()
    feed(ON:gsub("1", "1;id=specMarkA:mark=busy", 1) .. "SpecMarkA\n" .. OFF)
    local a = lastLine()
    feed(ON:gsub("1", "1;id=specMarkB:mark=info", 1) .. "SpecMarkB\n" .. OFF)
    local b = lastLine()
    local fired = 0
    local id = tempTrigger("SpecMark", function() fired = fired + 1 end)
    finally(function() killTrigger(id) end)
    feed(ESC .. "]67;2;id=specMarkA:mark=ok" .. ESC .. "\\")
    assert.are.equal("ok", getMarkdownBlock("main", a).mark)
    assert.are.equal("info", getMarkdownBlock("main", b).mark)
    assert.are.equal(0, fired)
  end)

  it("keeps the 256 most recent ids", function()
    local messages = {}
    for index = 0, 256 do
      messages[#messages + 1] = ESC .. "]67;1;id=specKept" .. index .. ":mark=busy" .. ESC .. "\\SpecKept" .. index .. "\n"
    end
    local first = getLastLineNumber("main")
    feed(table.concat(messages) .. OFF)
    feed(ESC .. "]67;2;id=specKept0:mark=ok" .. ESC .. "\\" .. ESC .. "]67;2;id=specKept1:mark=ok" .. ESC .. "\\")
    assert.are.equal("busy", getMarkdownBlock("main", first).mark, "the oldest id still named its message")
    assert.are.equal("ok", getMarkdownBlock("main", first + 1).mark)
  end)
end)

