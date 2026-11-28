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
      assert.is_number(block.message)
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
    local first = getLastLineNumber("main")
    feed(ON .. text .. "\n" .. OFF)
    local last = lastLine()
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
    local id = tempRegexTrigger("^", function() fired = fired + 1 end)
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

  -- A trigger that edits the line it fired on, as scripts do
  local function onLine(text, action)
    local id = tempTrigger(text, action)
    finally(function() killTrigger(id) end)
  end

  it("drops a line a trigger deletes, and its tag with it", function()
    onLine("SpecDeleteHeading", function() deleteLine() end)
    feed(ON .. "# SpecDeleteHeading\nSpecAfterDelete\n" .. OFF)
    assert.are.equal("SpecAfterDelete", lineText(lastLine()))
    assert.are.equal("paragraph", getMarkdownBlock("main", lastLine()).kind)
    assert.are_not.equal("SpecDeleteHeading", lineText(lastLine(2)))
  end)

  it("keeps the tag, and moves its links, when a trigger edits an item's text", function()
    onLine("SpecEditItem", function()
      selectString("old", 1)
      replace("newer")
    end)
    feed(ON .. "- SpecEditItem old [a link](send:x)\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal("item", block.kind)
    assert.are.equal("SpecEditItem newer a link", block.text)
    local link = block.links[1]
    local offset = link.start - block.contentStart
    assert.are.equal("a link", block.text:sub(offset + 1, offset + link.length))
  end)

  it("drops the tag when a trigger edits an item's bullet", function()
    onLine("SpecEditBullet", function()
      selectSection(0, 1)
      replace("*")
    end)
    feed(ON .. "- SpecEditBullet\n" .. OFF)
    assert.are.equal("* SpecEditBullet", lineText(lastLine()))
    assert.is_nil(getMarkdownBlock("main", lastLine()))
  end)

  it("drops only the edited row's tag when a trigger edits a table row", function()
    onLine("SpecEditRow", function()
      selectString("SpecEditRow", 1)
      replace("SpecEditedRow")
    end)
    feed(ON .. "| SpecEditHead |\n|---|\n| SpecEditRow |\n| SpecKeptRow |\n" .. OFF)
    assert.is_nil(getMarkdownBlock("main", lastLine(2)), "the edited row kept its tag")
    assert.are.equal("tableRow", getMarkdownBlock("main", lastLine()).kind)
    assert.are.equal("tableHeader", getMarkdownBlock("main", lastLine(4)).kind)
  end)

  it("keeps the tag when a trigger only restyles the line", function()
    onLine("SpecRestyle", function()
      selectString("SpecRestyle", 1)
      fg("red")
      setBold(true)
      setUnderline(true)
    end)
    feed(ON .. "## SpecRestyle\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal("heading", block.kind)
    assert.are.equal(2, block.level)
  end)

  -- The background parser reads a flood ahead of the main thread only once the flood is under way, and only
  -- while nothing in it needs the main thread: Markdown lines in such a flood must still all be read as Markdown
  it("reads every Markdown line of a flood the background parser reads ahead of", function()
    local plain, items, more = {}, {}, {}
    for index = 1, 20 do
      plain[index] = "SpecAheadPlain " .. index
    end
    for index = 1, 40 do
      items[index] = "- SpecAheadItem " .. index .. " with enough words to make the burst long"
    end
    for index = 1, 20 do
      more[index] = "- SpecAheadMore " .. index
    end
    local parsedAhead = getProfileStats().linesParsedAhead
    feed(table.concat(plain, "\n") .. "\n" .. ON .. table.concat(items, "\n") .. "\n")
    feed(table.concat(more, "\n") .. "\n" .. OFF)
    if not parsedAhead or getProfileStats().linesParsedAhead == parsedAhead then
      pending("the ingest worker did not read ahead, so this compared the main thread with itself")
    end
    local last = lastLine()
    for back = 0, 59 do
      local block = getMarkdownBlock("main", last - back)
      assert.are.equal("item", block and block.kind, "line " .. (last - back) .. " of the flood was not read as Markdown")
    end
  end)

  -- Once the game has sent a GA, Mudlet joins a line's pieces however slowly they come; before, it shows a piece it
  -- has waited on as a line of its own, which the standard lets a client treat as a prompt, so it is never Markdown
  it("reads a line whose rest comes after Mudlet has shown its start as Markdown only where it is whole", function()
    feed(ON .. "SpecHalf **hel")
    feed("lo** there\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal("paragraph", block.kind)
    if lineText(lastLine()) == "SpecHalf hello there" then
      assert.are.equal("SpecHalf hello there", block.text)
    else
      assert.are.equal("SpecHalf **hel", lineText(lastLine(2)))
      assert.is_nil(getMarkdownBlock("main", lastLine(2)), "a piece shown on its own was read as Markdown")
      assert.are.equal("lo** there", block.text)
    end
  end)

  local function osc8(uri, text)
    return ESC .. "]8;;" .. uri .. ESC .. "\\" .. text .. ESC .. "]8;;" .. ESC .. "\\"
  end

  it("shows the markup of a Markdown link an OSC 8 link covers as typed", function()
    feed(ON .. "[**SpecCovered** " .. osc8("send:x", "y") .. "](send:z)\n" .. OFF)
    assert.are.equal("[**SpecCovered** y](send:z)", getMarkdownBlock("main", lastLine()).text)
  end)

  it("hides an OSC 8 spoiler at the end of a Markdown line without losing its place", function()
    feed(ON .. "SpecKiller is " .. osc8('local:?config={"spoiler":true}', "Bob") .. "\n" .. OFF)
    assert.are.equal("SpecKiller is    ", lineText(lastLine()))
  end)

  it("hides an OSC 8 spoiler that fills a table cell without losing the cell", function()
    feed(ON .. "| SpecSpoilerHead |\n|---|\n| " .. osc8('local:?config={"spoiler":true}', "Bob") .. " |\n" .. OFF)
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("tableRow", row.kind)
    assert.are.equal("   ", row.cells[1].text)
  end)

  it("hides an OSC 8 spoiler that runs on to the next Markdown line on both lines", function()
    feed(ON .. "SpecSpoilFirst " .. ESC .. ']8;;local:?config={"spoiler":true}' .. ESC .. "\\Bob\nAlice" .. ESC .. "]8;;" .. ESC .. "\\ SpecSpoilLast\n" .. OFF)
    assert.are.equal("SpecSpoilFirst    ", lineText(lastLine(2)))
    assert.are.equal("      SpecSpoilLast", lineText(lastLine()))
  end)

  it("hides an OSC 8 spoiler in every table cell it runs across", function()
    feed(ON .. "| SpecSpoilCells | B |\n|---|---|\n| " .. ESC .. ']8;;local:?config={"spoiler":true}' .. ESC .. "\\Bob | Al" .. ESC .. "]8;;" .. ESC .. "\\ |\n" .. OFF)
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("   ", row.cells[1].text)
    assert.are.equal("  ", row.cells[2].text)
  end)

  -- clearWindow() sweeps the links no line holds, and a held row's are on no line yet
  it("keeps the links and spoilers of a held table's rows across a clearWindow()", function()
    feed(ON .. "| SpecHeldLinkHead | B |\n|---|---|\n| " .. osc8("send:held", "go") .. " | " .. osc8('local:?config={"spoiler":true}', "Bob") .. " |\n")
    clearWindow()
    settle(function() return getLastLineNumber("main") >= 4 end)
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("tableRow", row and row.kind, "the held rows were not shown after the clear")
    assert.are.equal("send:held", row.links[1].destination)
    assert.are.equal("   ", row.cells[2].text)
  end)

  it("pads a table by the columns its characters take on screen", function()
    feed(ON .. "| 名前 | SpecWide |\n|---|---|\n| abcd | x |\n" .. OFF)
    assert.are.equal("名前 │ SpecWide", lineText(lastLine(3)))
    assert.are.equal("abcd │ x       ", lineText(lastLine()))
  end)

  it("keeps a held table's mark when the next message starts before the table is shown", function()
    feed(ESC .. "]67;1;mark=warn" .. ESC .. "\\| SpecHeldMark |\n|---|\n| r |\n" .. ON .. "SpecNextMessage\n" .. OFF)
    assert.are.equal("warn", getMarkdownBlock("main", lastLine(2)).mark)
    assert.is_nil(getMarkdownBlock("main", lastLine()).mark)
  end)

  it("counts a paragraph's offsets from its text, not from the indent wrapping adds", function()
    local savedWrap = getWindowWrap("main")
    setWindowWrap("main", 20)
    setWindowWrapIndent("main", 2)
    finally(function()
      setWindowWrapIndent("main", 0)
      setWindowWrap("main", savedWrap)
    end)
    feed(ON .. "- SpecIndent one two three [a link](send:x) four five\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal("SpecIndent one two three a link four five", block.text)
    local link = block.links[1]
    local offset = link.start - block.contentStart
    assert.are.equal("a link", block.text:sub(offset + 1, offset + link.length))
  end)

  it("drops a table row's tag when a trigger echoes onto it", function()
    onLine("SpecEchoRow", function() echo(" extra") end)
    feed(ON .. "| SpecEchoHead |\n|---|\n| SpecEchoRow |\n" .. OFF)
    assert.is_truthy(lineText(lastLine()):find(" extra", 1, true))
    assert.is_nil(getMarkdownBlock("main", lastLine()))
  end)

  it("keeps a row under a list item that is not a table in the item", function()
    feed(ON .. "- SpecPipeItem\n  | not a table\nSpecPipeAfter\n" .. OFF)
    local row = getMarkdownBlock("main", lastLine(2))
    assert.are.equal("continuation", row.kind)
    assert.are.equal("| not a table", row.text)
  end)

  it("still makes a table of a header whose delimiter row comes after the game went quiet", function()
    feed(ON .. "| SpecIdleHead |\n")
    assert.is_true(pumpEvents(500), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
    feed("|---|\n| r |\n" .. OFF)
    assert.are.equal("tableHeader", getMarkdownBlock("main", lastLine(3)).kind)
  end)

  it("shows a row that never gets a delimiter row once the game has been quiet a while", function()
    local before = getLastLineNumber("main")
    feed(ON .. "| SpecLoneRow |\n")
    settle(function() return getLastLineNumber("main") > before end)
    assert.are.equal("| SpecLoneRow |", lineText(lastLine()))
    assert.are.equal("paragraph", getMarkdownBlock("main", lastLine()).kind)
  end)

  it("keeps an escaped pipe at the end of a row in its cell", function()
    feed(ON .. "| SpecPipe \\|\n|---|\n" .. OFF)
    assert.are.equal("SpecPipe |", getMarkdownBlock("main", lastLine(2)).cells[1].text)
  end)

  it("reads a backtick in a link's title as part of the title", function()
    feed(ON .. "[a](send:x \"`t\") `b`\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal("a b", block.text)
    assert.are.equal("`t", block.links[1].title)
  end)

  it("does not pad a table too wide to show, so its rows stay as long as their text", function()
    local wide = {}
    for index = 1, 10 do
      wide[index] = string.rep("x", 300)
    end
    feed(ON .. "| " .. table.concat(wide, " | ") .. " |\n|" .. string.rep("---|", 10) .. "\n| y | y | y | y | y | y | y | y | y | y |\n" .. OFF)
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("tableRow", row.kind)
    assert.is_true(utf8.len(lineText(lastLine())) < 100, "the row was padded to the header's 3000 columns")
  end)

  it("ends the message after the line an off sequence part way through it belongs to", function()
    local before = getLastLineNumber("main")
    feed(ON .. "| SpecMidOffHead |\n|---|\n| r |" .. OFF .. "\n")
    assert.are.equal(before + 3, getLastLineNumber("main"), "the table waited for a later line")
  end)

  -- The grid never folds a section, so its summary line carries a mark that does not look like something to click
  it("marks a section's summary in the grid with a section sign", function()
    feed(ON .. "<details>\n<summary>SpecSummaryMark</summary>\ninside\n</details>\n" .. OFF)
    assert.are.equal("§ SpecSummaryMark", lineText(lastLine(2)))
    assert.are.equal("summary", getMarkdownBlock("main", lastLine(2)).kind)
  end)

  it("keeps an unmatched </details> under a list item in the item", function()
    feed(ON .. "- SpecDetailsItem\n  </details>\n  SpecDetailsMore\n" .. OFF)
    assert.are.equal("continuation", getMarkdownBlock("main", lastLine(2)).kind)
    assert.are.equal("</details>", getMarkdownBlock("main", lastLine(2)).text)
    assert.are.equal("continuation", getMarkdownBlock("main", lastLine()).kind)
  end)

  -- An emoji is a symbol, and CommonMark counts symbols as punctuation when it decides where emphasis may open
  it("reads a symbol outside the basic plane as punctuation beside an emphasis marker", function()
    feed(ON .. "a**🚀**b SpecEmoji\n" .. OFF)
    assert.are.equal("a**🚀**b SpecEmoji", getMarkdownBlock("main", lastLine()).text)
  end)

  -- A game controls these lines, so a line of links that never close must not cost more than its length
  it("reads a table row full of links that never close in reasonable time", function()
    local cells, header, delimiter = {}, {}, {}
    for index = 1, 20 do
      cells[index] = string.rep("[a](b", 1999)
      header[index] = "h"
      delimiter[index] = "---"
    end
    local started = os.clock()
    feed(ON .. "| " .. table.concat(header, " | ") .. " |\n| " .. table.concat(delimiter, " | ") .. " |\n| " .. table.concat(cells, " | ") .. " |\n" .. OFF)
    local spent = os.clock() - started
    assert.are.equal("tableRow", getMarkdownBlock("main", lastLine()).kind)
    assert.is_true(spent < 1.5, string.format("the row took %.2fs", spent))
  end)


  it("still reads a link after a run of links that never close", function()
    feed(ON .. string.rep("[a](b", 1900) .. " [SpecLateLink](send:north)\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal(1, #(block.links or {}))
    assert.are.equal("send:north", block.links[1].destination)
  end)

  it("still reads a link after code spans that never close", function()
    local runs = {}
    for length = 1, 130 do
      runs[length] = string.rep("`", length) .. "aaaaaaaaaa"
    end
    feed(ON .. table.concat(runs) .. " [SpecLateCode](send:north)\n" .. OFF)
    local block = getMarkdownBlock("main", lastLine())
    assert.are.equal(1, #(block.links or {}))
    assert.are.equal("send:north", block.links[1].destination)
  end)

  it("shows a link with no text as typed, rather than losing it", function()
    feed(ON .. "[](send:north) SpecEmptyLink\n" .. OFF)
    assert.are.equal("[](send:north) SpecEmptyLink", getMarkdownBlock("main", lastLine()).text)
  end)

  it("reads nothing inside a link with no text", function()
    feed(ON .. "[](send:*north*) SpecEmptyMarks\n" .. OFF)
    assert.are.equal("[](send:*north*) SpecEmptyMarks", getMarkdownBlock("main", lastLine()).text)
    feed(ON .. "[](https://example.com) SpecEmptyAddress\n" .. OFF)
    assert.are.equal(0, #(getMarkdownBlock("main", lastLine()).links or {}))
  end)

  it("keeps a held table's mark when a trigger gags its first row and the next message has begun", function()
    onLine("SpecGagHead", function() deleteLine() end)
    feed(ESC .. "]67;1;mark=warn" .. ESC .. "\\| SpecGagHead |\n|---|\n| r |\nSpecGagEnd" .. ON .. "\n" .. OFF)
    local line = lastLine()
    while line > 0 and lineText(line) ~= "SpecGagEnd" do
      line = line - 1
    end
    assert.are.equal("warn", getMarkdownBlock("main", line).mark)
    local row = line
    while row > 0 do
      local block = getMarkdownBlock("main", row)
      if block and block.kind == "tableRow" and block.cells[1].text == "r" then
        break
      end
      row = row - 1
    end
    assert.is_true(row > 0, "the table's row did not survive the gag")
    assert.are.equal("warn", getMarkdownBlock("main", row).mark)
  end)

  -- MXP is off on a Markdown line, so <details> and <https://…> are Markdown's, and back on for the game's next line
  it("keeps MXP off its lines and on for the lines after", function()
    local savedForceMxp = getConfig("specialForceMXPProcessorOn")
    setConfig("specialForceMXPProcessorOn", true)
    finally(function() setConfig("specialForceMXPProcessorOn", savedForceMxp) end)
    feed(ON .. "<details>\n<summary>SpecMxpSum</summary>\ninside\n</details>\nsee <https://example.com>\n" .. OFF)
    local link = getMarkdownBlock("main", lastLine())
    assert.are.equal(1, #(link.links or {}))
    assert.are.equal("https://example.com", link.links[1].destination)
    assert.are.equal("§ SpecMxpSum", lineText(lastLine(3)))
    feed("<B>SpecMxpAfter</B>\n")
    assert.are.equal("SpecMxpAfter", lineText(lastLine()))
    assert.is_true(formatOf(lastLine(), 0).bold, "MXP stayed off after Markdown")
  end)

  it("never holds a Markdown line to join it to the next, as undoing the game's wrapping would", function()
    local savedUndo, savedWidth = getConfig("undoServerWrap"), getConfig("undoServerWrapWidth")
    setConfig("undoServerWrap", true)
    setConfig("undoServerWrapWidth", 20)
    finally(function()
      setConfig("undoServerWrap", savedUndo)
      setConfig("undoServerWrapWidth", savedWidth)
    end)
    feed(ON .. "- SpecServerWrapItem\n- SpecNextItem\n" .. OFF)
    assert.are.equal("SpecServerWrapItem", getMarkdownBlock("main", lastLine(2)).text)
    assert.are.equal("SpecNextItem", getMarkdownBlock("main", lastLine()).text)
  end)

  it("ignores an on or off sequence in text a script feeds", function()
    feed(ON .. "- SpecFedItem\n")
    feedTriggers(OFF)
    feed("  SpecFedMore\n" .. OFF)
    assert.are.equal("continuation", getMarkdownBlock("main", lastLine()).kind)
    feedTriggers(ON .. "# SpecFedOn\n")
    feed("# SpecGameAfter\n")
    assert.is_nil(getMarkdownBlock("main", lastLine()), "an on sequence a script fed turned Markdown on")
  end)

  -- Game text a trigger feeds opens a message of its own, which the line the trigger ran on must not end
  it("keeps a message that a trigger's fed game text opened going", function()
    onLine("SpecNestOuter", function()
      feedTelnet(ESC .. "]67;1;mark=busy" .. ESC .. "\\| SpecNestHead |\n|---|\n| r1 |\n")
    end)
    feed(ON .. "SpecNestOuter\n")
    feed("| r2 |\n" .. OFF)
    local row = getMarkdownBlock("main", lastLine())
    assert.are.equal("tableRow", row and row.kind, "the fed message's table was ended under it")
    assert.are.equal("r2", row.cells[1].text)
  end)

  it("drops a paragraph's tag when a script edits the indent wrapping put in front of a row", function()
    local savedWrap = getWindowWrap("main")
    setWindowWrap("main", 20)
    setWindowWrapHangingIndent("main", 2)
    finally(function()
      setWindowWrapHangingIndent("main", 0)
      setWindowWrap("main", savedWrap)
    end)
    local first = getLastLineNumber("main")
    feed(ON .. "SpecPadEdit one two three four five [a link](send:x) six\n" .. OFF)
    assert.is_not_nil(getMarkdownBlock("main", first), "the paragraph was not read as Markdown")
    assert.are.equal("  ", lineText(first + 1):sub(1, 2), "the second row has no indent, so this proves nothing")
    moveCursor("main", 0, first + 1)
    selectSection("main", 0, 3)
    replace("main", "")
    deselect("main")
    assert.is_nil(getMarkdownBlock("main", first), "the tag kept offsets that no longer fit the row")
  end)

  it("drops a line's tag when a trigger's line feed and wrapLine() make two lines of it", function()
    onLine("SpecSplitHead", function()
      echo(" and on\nSpecSplitTail")
      wrapLine("main", getLineNumber())
    end)
    feed(ON .. "# SpecSplitHead [a link](send:x)\n" .. OFF)
    assert.are.equal("SpecSplitTail", lineText(lastLine()))
    assert.is_nil(getMarkdownBlock("main", lastLine(2)), "the line kept offsets made for the text before it was split")
  end)

  it("shows a line quoted deeper than the eight levels it places as a paragraph, markers and all", function()
    feed(ON .. "> > > > > > > > SpecEightQuotes\n> > > > > > > > > SpecNineQuotes\n" .. OFF)
    local eight = getMarkdownBlock("main", lastLine(2))
    assert.are.equal(8, eight.quote)
    assert.are.equal("SpecEightQuotes", eight.text)
    local nine = getMarkdownBlock("main", lastLine())
    assert.are.equal("paragraph", nine.kind)
    assert.are.equal(0, nine.quote)
    assert.are.equal("> > > > > > > > > SpecNineQuotes", nine.text)
  end)

  it("shows a list item deeper than the eight levels it places as a paragraph, marker and all", function()
    local items = {}
    for level = 1, 9 do
      items[level] = string.rep("  ", level - 1) .. "- SpecDeep" .. level
    end
    feed(ON .. table.concat(items, "\n") .. "\n" .. OFF)
    local eighth = getMarkdownBlock("main", lastLine(2))
    assert.are.equal("item", eighth.kind)
    assert.are.equal(8, eighth.list)
    local ninth = getMarkdownBlock("main", lastLine())
    assert.are.equal("paragraph", ninth.kind)
    assert.are.equal("- SpecDeep9", ninth.text)
  end)

  it("moves a paragraph's later links by the spaces an insert past a row's end puts in", function()
    local savedWrap = getWindowWrap("main")
    setWindowWrap("main", 20)
    finally(function() setWindowWrap("main", savedWrap) end)
    local first = getLastLineNumber("main")
    feed(ON .. "SpecExpand one two three [a link](send:x) four\n" .. OFF)
    assert.is_true(lastLine() > first, "the paragraph did not wrap, so this proves nothing")
    moveCursor("main", utf8.len(lineText(first)) + 3, first)
    insertText("main", "X")
    local block = getMarkdownBlock("main", first)
    local link = block.links[1]
    local offset = link.start - block.contentStart
    assert.are.equal("a link", block.text:sub(offset + 1, offset + link.length))
  end)

  -- Last: the GA it sends changes how Mudlet waits for the rest of a line for every test after it
  it("hides an OSC 8 spoiler on a prompt that begins while Markdown is on", function()
    feed(ON .. "SpecBefore\nSpecPrompt " .. osc8('local:?config={"spoiler":true}', "SECRET") .. "<T_IAC><T_GA>")
    feed("\n" .. OFF)
    local prompt = lastLine(2)
    while prompt > 0 and lineText(prompt) ~= "SpecPrompt       " and not lineText(prompt):find("SpecPrompt", 1, true) do
      prompt = prompt - 1
    end
    assert.are.equal("SpecPrompt       ", lineText(prompt))
  end)
end)
