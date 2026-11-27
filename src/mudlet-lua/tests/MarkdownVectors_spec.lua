-- Plays each of the Markdown Text for MUDs standard's test vectors (fixtures/markdown, described in the
-- standard's VECTORS.md) through the telnet path and compares the main console with what the vector expects.
-- Needs --offline for feedTelnet(), as the tests README says.

local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")

-- The parts of the standard implemented so far
local GROUPS = {switching = true, lines = true, headings = true, code = true, lists = true, quotes = true, rules = true, emphasis = true, literal = true}

local ESC = "\27"
local ON = ESC .. "]67;1" .. ESC .. "\\"
local OFF = ESC .. "]67;0" .. ESC .. "\\"

local function loadVectors()
  local file = assert(io.open(specDirectory .. "/fixtures/markdown/markdown-test-vectors.json", "rb"))
  local text = file:read("*a")
  file:close()
  return yajl.to_value(text)
end

local function feed(data)
  local ok, message = feedTelnet(data)
  assert(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))
end

-- feedTelnet() reads <..> as telnet codes, and << and >> as < and >
local function escapeForTelnet(text)
  return (text:gsub("[<>]", function(c) return c .. c end))
end

local function utf16Units(text)
  local units = 0
  for _, code in utf8.codes(text) do
    units = units + (code > 0xFFFF and 2 or 1)
  end
  return units
end

local function rgb(color)
  return string.format("%d,%d,%d", color[1], color[2], color[3])
end

local function formatAt(lineNumber, column)
  moveCursor("main", 0, lineNumber)
  selectSection("main", column, 1)
  local format = getTextFormat("main")
  deselect("main")
  return format
end

-- The colour each SGR value the vector names gives a character, read back from a line fed with it
local function colorsFor(vector)
  local wanted = {}
  local function collect(spans)
    for _, span in ipairs(spans or {}) do
      if span.fg then wanted["fg:" .. span.fg] = {"fg", span.fg} end
      if span.bg then wanted["bg:" .. span.bg] = {"bg", span.bg} end
    end
  end
  for _, line in ipairs(vector.lines) do
    collect(line.spans)
    for _, cell in ipairs(line.cells or {}) do
      collect(cell.spans)
    end
  end
  local byColor = {fg = {}, bg = {}}
  for _, entry in pairs(wanted) do
    local which, parameters = entry[1], entry[2]
    feed(ESC .. "[" .. parameters .. "mX" .. ESC .. "[0m\n")
    local format = formatAt(getLastLineNumber("main") - 1, 0)
    byColor[which][rgb(which == "fg" and format.foreground or format.background)] = parameters
  end
  return byColor
end

local function countMessages(vector)
  local count = 0
  for _, piece in ipairs(vector.input) do
    if type(piece) == "string" then
      for terminator in piece:gmatch(ESC .. "%]67;1([;" .. ESC .. "\7])") do
        count = count + 1
      end
    end
  end
  return count
end

local function sameAttributes(a, b)
  for key, value in pairs(a) do
    if key ~= "start" and key ~= "length" and not (type(value) == "table" and type(b[key]) == "table" and value.destination == b[key].destination and value.title == b[key].title) and b[key] ~= value then
      return false
    end
  end
  for key in pairs(b) do
    if key ~= "start" and key ~= "length" and a[key] == nil then
      return false
    end
  end
  return true
end

-- Styled ranges of the length units of text starting at column, in UTF-16 units from 0
local function readSpans(lineNumber, column, length, block, plainFormat, colors)
  local spans = {}
  for offset = 0, length - 1 do
    local format = formatAt(lineNumber, column + offset)
    local attributes = {}
    local link
    for _, candidate in ipairs(block.links or {}) do
      if column + offset >= candidate.start and column + offset < candidate.start + candidate.length then
        link = {destination = candidate.destination, title = candidate.title}
      end
    end
    -- A heading and a summary are bold as a whole in the grid
    if format.bold and block.kind ~= "heading" and block.kind ~= "summary" then attributes.bold = true end
    if format.italic then attributes.italic = true end
    if format.strikeout then attributes.strike = true end
    if format.code then attributes.code = true end
    if format.reverse then attributes.reverse = true end
    if format.blinking and format.blinking ~= "none" then attributes.blink = true end
    if link then
      -- A link's own look is the client's, so its underline and colour are not the game's
      attributes.link = link
    else
      if format.underline then attributes.underline = true end
      local fg = rgb(format.foreground)
      if fg ~= rgb(plainFormat.foreground) then attributes.fg = colors.fg[fg] or ("rgb " .. fg) end
    end
    local bg = rgb(format.background)
    if bg ~= rgb(plainFormat.background) then attributes.bg = colors.bg[bg] or ("rgb " .. bg) end
    if next(attributes) then
      local previous = spans[#spans]
      if previous and previous.start + previous.length == offset and sameAttributes(previous, attributes) then
        previous.length = previous.length + 1
      else
        attributes.start = offset
        attributes.length = 1
        spans[#spans + 1] = attributes
      end
    end
  end
  return #spans > 0 and spans or nil
end

local function vectorLine(lineNumber, text, base, plainFormat, colors)
  local block = getMarkdownBlock("main", lineNumber)
  if not block then
    return {kind = "plain", text = text}
  end
  local line = {kind = block.kind}
  if block.kind == "tableHeader" or block.kind == "tableRow" then
    line.cells = {}
    for _, cell in ipairs(block.cells or {}) do
      line.cells[#line.cells + 1] = {text = cell.text, spans = readSpans(lineNumber, cell.start, cell.length, block, plainFormat, colors)}
    end
    if block.kind == "tableHeader" then
      line.align = block.align
    end
  else
    line.text = block.text
    line.spans = readSpans(lineNumber, block.contentStart, utf16Units(block.text), block, plainFormat, colors)
  end
  for _, key in ipairs({"level", "quote", "list", "number", "section"}) do
    if (block[key] or 0) ~= 0 then
      line[key] = block[key]
    end
  end
  line.marker = block.marker
  line.task = block.task
  line.language = block.language
  if block.open then
    line.open = true
  end
  line.message = block.message - base - 1
  return line, block
end

local function play(vector)
  local colors = colorsFor(vector)
  -- Off where no line has begun turns Markdown off and makes no line
  feed(OFF)
  feed(ON .. "probe\n" .. OFF)
  local probeLine = getLastLineNumber("main") - 1
  local probe = assert(getMarkdownBlock("main", probeLine), "the probe line was not tagged as Markdown")
  local base = probe.message
  local plainFormat = formatAt(probeLine, 0)
  local first = getLastLineNumber("main")

  for _, piece in ipairs(vector.input) do
    if type(piece) == "string" then
      feed(escapeForTelnet(piece))
    else
      feed("<T_IAC><T_" .. piece.telnet .. ">")
    end
  end
  if getLines("main", getLastLineNumber("main"), getLastLineNumber("main") + 1)[1] ~= "" then
    feed("\n")
  end

  local last = getLastLineNumber("main") - 1
  local lines, messages = {}, {}
  for index = 1, countMessages(vector) do
    messages[index] = {}
  end
  local texts = getLines("main", first, last + 1)
  for offset, text in ipairs(texts) do
    local line, block = vectorLine(first + offset - 1, text, base, plainFormat, colors)
    if line.kind ~= "tableDelimiter" then
      lines[#lines + 1] = line
    end
    if block then
      local message = messages[line.message + 1]
      if message and not message.seen then
        message.seen = true
        message.mark = block.mark
        message.id = block.id
      end
    end
  end
  for _, message in ipairs(messages) do
    message.seen = nil
  end
  return lines, messages
end

describe("The Markdown Text for MUDs test vectors", function()
  local file = loadVectors()
  local savedWrap

  setup(function()
    savedWrap = getWindowWrap("main")
    setWindowWrap("main", 300)
  end)

  teardown(function()
    feed(OFF)
    setWindowWrap("main", savedWrap)
  end)

  it("loads all of them", function()
    assert.are.equal(67, file.osc)
    assert.are.equal(215, #file.vectors)
  end)

  for _, vector in ipairs(file.vectors) do
    if GROUPS[vector.group] then
      it(vector.id .. ": " .. vector.rule, function()
        local lines, messages = play(vector)
        assert.are.same(vector.lines, lines)
        assert.are.same(vector.messages, messages)
      end)
    end
  end
end)
