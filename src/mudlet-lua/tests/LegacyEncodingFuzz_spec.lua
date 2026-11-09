-- Differential fuzzer for the multibyte decoders in TBuffer: GBK, GB18030,
-- BIG5, BIG5-HKSCS, SHIFT_JIS, EUC-JP and EUC-KR, plus UTF-8 for comparison.
--
-- A game sends its text in reads cut wherever the network cut them, so a
-- character can arrive as a lead byte at the end of one read and its trail
-- bytes at the start of the next. Each round sets one encoding, then feeds
-- the same random lines - valid characters, lone and truncated leads, invalid
-- trail bytes, GB18030 four byte forms, mixed with ASCII and SGR colours - once
-- as one read and once in reads of one to seven bytes, and requires the same
-- text and getTextFormat() for every character from both.
--
-- Run against a sanitizer build, as with the other fuzzers.
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_ENCFUZZ_SEED.
--   MUDLET_ENCFUZZ_SEED    integer seed                    (default 1)
--   MUDLET_ENCFUZZ_ROUNDS  batches fed                     (default 80)
--   MUDLET_ENCFUZZ_DUMP    path; each batch is written here, %q quoted, before it is fed

-- Gated like the other fuzzers: busted recurses into the whole tests directory,
-- and this leaves odd text in the main console and changes the server encoding.
if not os.getenv("MUDLET_FUZZ") then
  return
end

local floor = math.floor
local concat = table.concat
local char = string.char

local function envnum(name, default)
  local v = tonumber(os.getenv(name) or "")
  return v and floor(v) or default
end

local SEED = envnum("MUDLET_ENCFUZZ_SEED", 1)
local ROUNDS = envnum("MUDLET_ENCFUZZ_ROUNDS", 80)
local DUMP_PATH = os.getenv("MUDLET_ENCFUZZ_DUMP")

local s = SEED % 2147483647
if s <= 0 then s = s + 2147483646 end
local function rng()
  s = (16807 * s) % 2147483647
  return s
end
local function rnd(n) return rng() % n end
local function rrange(lo, hi) return lo + rng() % (hi - lo + 1) end
local function pick(list) return list[1 + rng() % #list] end

-- lead and trail byte ranges, wide enough to also give invalid pairs
local encodings = {
  {name = "GBK", lead = {0x81, 0xFE}, trail = {0x40, 0xFE}},
  {name = "GB18030", lead = {0x81, 0xFE}, trail = {0x40, 0xFE}, four = true},
  {name = "BIG5", lead = {0x81, 0xFE}, trail = {0x40, 0xFE}},
  {name = "BIG5-HKSCS", lead = {0x81, 0xFE}, trail = {0x40, 0xFE}},
  {name = "SHIFT_JIS", lead = {0x81, 0xFC}, trail = {0x40, 0xFC}, single = {0xA1, 0xDF}},
  {name = "EUC-JP", lead = {0xA1, 0xFE}, trail = {0xA1, 0xFE}, single = {0x8E, 0x8E}},
  {name = "EUC-KR", lead = {0xA1, 0xFE}, trail = {0xA1, 0xFE}},
  {name = "UTF-8", lead = {0xC2, 0xF4}, trail = {0x80, 0xBF}},
}

-- the encoding the batch being made is in
local current

-- never NUL, CR, LF, '<' or '>' (feedTelnet() reads those as tag delimiters) or IAC. ICU's SHIFT_JIS
-- swaps 0x1A, 0x1C and 0x7F, as IBM's Shift-JIS does, while a run of plain bytes copied straight into
-- the line keeps them, so where a read was cut decides which a game gets; no game sends them, so they
-- are left out for a round to turn up something else.
local function safe(byte)
  if byte == 0 or byte == 10 or byte == 13 or byte == 0x3C or byte == 0x3E or byte == 0xFF then
    return 0x20
  end
  if current == "SHIFT_JIS" and (byte == 0x1A or byte == 0x1C or byte == 0x7F) then
    return 0x20
  end
  return byte
end

local function inRange(range)
  return char(safe(rrange(range[1], range[2])))
end

local function segment(encoding)
  local k = rnd(16)
  if k < 5 then
    -- a lead and a trail byte, valid or not
    return inRange(encoding.lead) .. inRange(encoding.trail)
  elseif k == 5 then
    -- a lead byte with nothing after it, or followed by ASCII
    return inRange(encoding.lead) .. pick({"", "a", " ", "\27[0m"})
  elseif k == 6 then
    return inRange(encoding.single or encoding.lead) .. inRange(encoding.trail)
  elseif k == 7 then
    if encoding.four then
      -- a four byte GB18030 form, or one cut short
      local bytes = inRange({0x81, 0xFE}) .. char(rrange(0x30, 0x39)) .. inRange({0x81, 0xFE}) .. char(rrange(0x30, 0x39))
      return bytes:sub(1, rrange(2, 4))
    end
    return inRange(encoding.lead) .. inRange(encoding.trail) .. inRange(encoding.trail)
  elseif k == 8 then
    return char(safe(rrange(0x80, 0xFE)))
  elseif k == 9 then
    return "\27[" .. pick({"0", "1", "31", "1;33;44", "38;5;" .. rrange(0, 255), "48;2;" .. rrange(0, 255) .. ";9;9", "4", "7", "22"}) .. "m"
  elseif k == 10 then
    return pick({"\t", "  ", "sword", "HP:100", "[", "\\"})
  elseif k == 11 then
    return char(safe(rrange(1, 31)))
  end
  return string.rep(pick({"a", "z", "-"}), rrange(1, 12))
end

local function line(encoding)
  local parts = {}
  for _ = 1, rrange(0, 16) do
    parts[#parts + 1] = segment(encoding)
  end
  return concat(parts)
end

local function dump(round, name, lines)
  if not DUMP_PATH then return end
  local f = io.open(DUMP_PATH, "w")
  if f then
    f:write(string.format("seed=%d round=%d encoding=%s\n", SEED, round, name))
    for _, text in ipairs(lines) do
      f:write(string.format("%q\n", text))
    end
    f:close()
  end
end

local function feed(data)
  local ok, msg = feedTelnet(data)
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
end

-- selectSection() counts UTF-16 units, so a character outside the BMP - four bytes in UTF-8 - takes two
local function cells(text)
  local count = 0
  for character in text:gmatch("[%z\1-\127\194-\244][\128-\191]*") do
    count = count + (#character == 4 and 2 or 1)
  end
  return count
end

-- each line's text, then the format of each of its characters
local function snapshot(from, to)
  local result = {}
  for lineNumber = from, to do
    assert.is_true(moveCursor("main", 0, lineNumber))
    local text = getCurrentLine("main")
    local found = {text}
    for offset = 0, cells(text) - 1 do
      selectSection("main", offset, 1)
      local fields = {}
      for key, value in pairs(getTextFormat("main")) do
        fields[#fields + 1] = key .. "=" .. (type(value) == "table" and concat(value, ",") or tostring(value))
      end
      table.sort(fields)
      found[#found + 1] = concat(fields, " ")
    end
    result[#result + 1] = concat(found, "|")
  end
  deselect("main")
  return result
end

local passes = 0

-- feeds the lines as one read, or in reads of one to seven bytes, and returns what they left in the buffer
local function pass(lines, pieces)
  passes = passes + 1
  local opening, closing = "EncFuzzOpens" .. passes, "EncFuzzCloses" .. passes
  feed("\27[0m" .. opening .. "\r\n")
  local joined = concat(lines, "\r\n") .. "\r\n"
  if pieces then
    local at = 1
    for _, size in ipairs(pieces) do
      feed(joined:sub(at, at + size - 1))
      at = at + size
    end
    feed(joined:sub(at))
  else
    feed(joined)
  end
  feed("\27[0m" .. closing .. "\r\n")
  -- found from the end after the feeds: a full buffer trims its oldest lines and renumbers the rest
  local from, to
  for i = getLastLineNumber("main"), 0, -1 do
    local text = getLines("main", i, i + 1)[1]
    if not to then
      to = text == closing and i - 1 or nil
    elseif text == opening then
      from = i + 1
      break
    end
  end
  assert.is_truthy(from, "the pass markers did not both reach the buffer")
  return snapshot(from, to)
end

describe("fuzz: multibyte decoding across read boundaries", function()
  local originalEncoding

  setup(function()
    print(string.format("[LegacyEncodingFuzz] seed=%d rounds=%d", SEED, ROUNDS))
    originalEncoding = getServerEncoding()
    -- without a GA the telnet layer holds an unfinished line back, so a cut read is never seen as one
    feed("<T_IAC><T_GA>")
  end)

  teardown(function()
    deselect("main")
    setServerEncoding(originalEncoding)
    feed("\27[0m\r\n")
  end)

  for round = 1, ROUNDS do
    local encoding = pick(encodings)
    current = encoding.name
    local lines = {}
    for i = 1, rrange(1, 12) do
      lines[i] = line(encoding)
    end
    local total = #concat(lines, "\r\n") + 2
    local pieces, used = {}, 0
    while used < total do
      local size = rrange(1, 7)
      pieces[#pieces + 1] = size
      used = used + size
    end

    it(string.format("round %d (seed %d, %s) decodes the same from small reads as from one", round, SEED, encoding.name), function()
      dump(round, encoding.name, lines)
      assert.is_true(setServerEncoding(encoding.name))
      local whole = pass(lines)
      local cut = pass(lines, pieces)
      assert.are.equal(#whole, #cut, "a different number of lines came out of small reads")
      for i = 1, #whole do
        assert.are.equal(whole[i], cut[i], "line " .. i .. " came out differently from small reads")
      end
    end)
  end
end)
