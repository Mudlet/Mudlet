-- Differential fuzzer for the SGR decoder and the ingest worker.
--
-- A read of 16 lines or more is parsed ahead on the ingest worker (see "Runtime
-- tuning: the ingest worker" in docs/platform-builds.md), a read per line only
-- ever on the main thread. Both must leave the same buffer, so each round feeds
-- one batch of random game lines both ways - and once more cut at a random
-- byte - then compares every character's text and getTextFormat(). The lines
-- are dense in SGR sequences, with mixed ';' and ':' separators, empty,
-- oversized and out of range values and truncated 38/48/58 forms, plus UTF-8,
-- invalid bytes, other CSI sequences and OSC 8 links. Each round also checks
-- that ESC[0m restores the default format whatever came before it, bold included.
--
-- Run against a sanitizer build, as with the other fuzzers, and with
-- MUDLET_INGEST_THREAD=verify to also have the main thread warn
-- "TBuffer ingest worker MISMATCH" for any line the worker made differently.
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_SGRFUZZ_SEED.
--   MUDLET_SGRFUZZ_SEED    integer seed                    (default 1)
--   MUDLET_SGRFUZZ_ROUNDS  batches fed                     (default 40)
--   MUDLET_SGRFUZZ_DUMP    path; each batch is written here, %q quoted, before it is fed

-- Gated like the other fuzzers: busted recurses into the whole tests directory,
-- and the lines this leaves in the main console would upset the specs after it.
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

local SEED = envnum("MUDLET_SGRFUZZ_SEED", 1)
local ROUNDS = envnum("MUDLET_SGRFUZZ_ROUNDS", 40)
local DUMP_PATH = os.getenv("MUDLET_SGRFUZZ_DUMP")

local s = SEED % 2147483647
if s <= 0 then s = s + 2147483646 end
local function rng()
  s = (16807 * s) % 2147483647
  return s
end
local function rnd(n) return rng() % n end
local function rrange(lo, hi) return lo + rng() % (hi - lo + 1) end
local function pick(list) return list[1 + rng() % #list] end
local function chance(pct) return rng() % 100 < pct end

local function number()
  local k = rnd(10)
  if k < 5 then return tostring(rrange(0, 255)) end
  if k < 7 then return tostring(rrange(0, 9)) end
  if k == 7 then return "" end
  if k == 8 then return pick({"256", "999", "65535", "2147483647", "2147483648", "99999999999", "00000000001"}) end
  return tostring(rrange(256, 100000))
end

-- how many SGR parameters and segments in a thousand are malformed, set per round: the worker
-- hands a line with anything it cannot decode quietly back to the main thread, so a round
-- full of those would compare the main thread with itself
local junk = 0

local function wellFormedParameter()
  local k = rnd(8)
  if k < 3 then
    return pick({"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "22", "23", "24", "25",
                 "27", "28", "29", "39", "49", "53", "55", ""})
  elseif k < 5 then
    return tostring(pick({30, 40, 90, 100}) + rrange(0, 7))
  elseif k == 5 then
    return pick({"38", "48"}) .. pick({";5;", ":5:"}) .. rrange(0, 255)
  elseif k == 6 then
    local layer = pick({"38", "48"})
    if chance(50) then
      return layer .. ";2;" .. rrange(0, 255) .. ";" .. rrange(0, 255) .. ";" .. rrange(0, 255)
    end
    return layer .. ":2::" .. rrange(0, 255) .. ":" .. rrange(0, 255) .. ":" .. rrange(0, 255)
  end
  return "4:" .. rrange(0, 5)
end

-- an SGR parameter that is out of range, cut short, oddly separated or unknown
local function malformedParameter()
  local k = rnd(7)
  if k == 0 then
    return pick({"38", "48", "58"}) .. ":5:" .. number()
  elseif k == 1 then
    -- with and without the colour space slot, and cut short
    local parts = {pick({"38", "48", "58"}), "2"}
    for _ = 1, rrange(0, 5) do
      parts[#parts + 1] = number()
    end
    return concat(parts, ":")
  elseif k == 2 then
    local parts = {pick({"38", "48", "58"}), pick({"2", "5", "3", "4", "1", "0", "", "9"})}
    for _ = 1, rrange(0, 5) do
      parts[#parts + 1] = number()
    end
    return concat(parts, ";")
  elseif k == 3 then
    return pick({"4:6", "4:", "4:x", "38", "48", "58", "38:", "48:", ":", "::", "38::", "51", "59", "20", "21", "00"})
  elseif k == 4 then
    return number()
  end
  local parts = {}
  for _ = 1, rrange(1, 4) do
    parts[#parts + 1] = number()
  end
  return concat(parts, ":")
end

local function sgr()
  local parts = {}
  for _ = 1, rrange(0, 8) do
    parts[#parts + 1] = rnd(1000) < junk and malformedParameter() or wellFormedParameter()
  end
  return "\27[" .. concat(parts, rnd(1000) < junk and ":" or ";") .. "m"
end

-- no '<' or '>' (feedTelnet() reads those as tag delimiters), NUL or IAC
local words = {"sword", "the orc", "HP:100", "  ", "\t", "café", "┌─┐", "✓", "😀", "日本語", "e\204\129", "[]", "\\", "|", "%"}

local function malformedSegment()
  local k = rnd(4)
  if k == 0 then
    -- a sequence the decoder does not treat as SGR, or one cut short
    return pick({"\27[5C", "\27[2J", "\27[K", "\27[?25l", "\27[1;31", "\27[38;2;1", "\27[", "\27", "\27(B", "\27[5n", "\27[6n", "\27[z"})
  elseif k == 1 then
    -- a byte that is not valid UTF-8, or the first half of a character
    return char(rrange(0x80, 0xfe))
  elseif k == 2 then
    return pick({"\27]8;;send:look\27\\", "\27]8;;\27\\", "\27]0;title\7", "\27]8;id=1;https://example.com\27\\link\27]8;;\27\\"})
  end
  return (char(rrange(1, 31)):gsub("[\r\n]", ""))
end

local function segment()
  if rnd(1000) < junk then
    return malformedSegment()
  end
  local k = rnd(10)
  if k < 4 then
    return sgr()
  elseif k < 8 then
    return pick(words)
  end
  return string.rep(pick({"a", "b", "-"}), rrange(1, 40))
end

local function line()
  local parts = {}
  for _ = 1, rrange(0, 12) do
    parts[#parts + 1] = segment()
  end
  return concat(parts)
end

local function dump(round, lines)
  if not DUMP_PATH then return end
  local f = io.open(DUMP_PATH, "w")
  if f then
    f:write(string.format("seed=%d round=%d\n", SEED, round))
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
local function cellCount(text)
  local count = 0
  for character in text:gmatch("[%z\1-\127\194-\244][\128-\191]*") do
    count = count + (#character == 4 and 2 or 1)
  end
  return count
end

-- each line's text and prompt flag, then the format of each of its characters
local function snapshot(from, to)
  local result = {}
  for lineNumber = from, to do
    assert.is_true(moveCursor("main", 0, lineNumber))
    local text = getCurrentLine("main")
    local cells = {text, "prompt=" .. tostring(isPrompt())}
    for offset = 0, cellCount(text) - 1 do
      selectSection("main", offset, 1)
      local fields = {}
      for key, value in pairs(getTextFormat("main")) do
        fields[#fields + 1] = key .. "=" .. (type(value) == "table" and concat(value, ",") or tostring(value))
      end
      table.sort(fields)
      cells[#cells + 1] = concat(fields, " ")
    end
    result[#result + 1] = concat(cells, "|")
  end
  deselect("main")
  return result
end

local passes = 0

-- feeds the lines one read each, or all in one read (cut in two after byte cut),
-- and returns what they left in the buffer, then the format of bold text after an ESC[0m
local function pass(lines, oneRead, cut)
  passes = passes + 1
  local opening, closing = "SgrFuzzOpens" .. passes, "SgrFuzzCloses" .. passes
  local mark = getLastLineNumber("main")
  feed("\27[0m\27]8;;\27\\" .. opening .. "\r\n")
  if oneRead then
    local joined = concat(lines, "\r\n") .. "\r\n"
    if cut then
      feed(joined:sub(1, cut))
      feed(joined:sub(cut + 1))
    else
      feed(joined)
    end
  else
    for _, text in ipairs(lines) do
      feed(text .. "\r\n")
    end
  end
  -- ends a sequence or link a line left unfinished; the marker is bold on a 16 colour
  -- background, which draws it in the bright foreground, so ESC[0m has to reset that too
  feed("\27\\\27]8;;\27\\\r\n\27[0m\27[1;48;5;4m" .. closing .. "\r\n")
  local found = getLines("main", mark, getLastLineNumber("main") + 1)
  local from, to, closingLine
  for i, text in ipairs(found) do
    if text == opening then
      from = mark + i
    elseif text == closing then
      to = mark + i - 2
      closingLine = mark + i - 1
    end
  end
  assert.is_truthy(from and to, "the pass markers did not both reach the buffer")
  local result = snapshot(from, to)
  -- the marker's first character, as the markers differ in length
  local reset = snapshot(closingLine, closingLine)[1]:match("^[^|]*|[^|]*|([^|]*)")
  return result, reset
end

describe("fuzz: SGR decoding on the ingest worker and the main thread", function()
  local baseline, parsedAheadBefore

  setup(function()
    print(string.format("[IngestSgrFuzz] seed=%d rounds=%d", SEED, ROUNDS))
    parsedAheadBefore = getProfileStats().linesParsedAhead
    -- without a GA the telnet layer holds an unfinished line back, so a cut read is never seen as one
    feed("<T_IAC><T_GA>")
    feed("\27[0m\27]8;;\27\\\r\n")
    local _, reset = pass({"SgrFuzzBaseline"}, false)
    baseline = reset
  end)

  teardown(function()
    -- nil outside MUDLET_TEST_MODE; 0 means the worker never ran and both sides were the main thread
    local after = getProfileStats().linesParsedAhead
    print(string.format("[IngestSgrFuzz] lines parsed ahead: %s", after and parsedAheadBefore and after - parsedAheadBefore or "unknown"))
    deselect("main")
    feed("\27\\\27]8;;\27\\\27[0m\r\n")
  end)

  for round = 1, ROUNDS do
    junk = pick({0, 0, 0, 1, 3, 10, 30, 100, 400})
    local roundJunk = junk
    local lines = {}
    for i = 1, rrange(16, 70) do
      lines[i] = line()
    end
    local joinedLength = #concat(lines, "\r\n") + 2
    local cut = rrange(1, joinedLength - 1)

    it(string.format("round %d (seed %d, %d per mille malformed) leaves the same buffer from one read as from a read per line", round, SEED, roundJunk), function()
      dump(round, lines)
      local apart, apartReset = pass(lines, false)
      local together, togetherReset = pass(lines, true)
      local split, splitReset = pass(lines, true, cut)
      for name, other in pairs({["one read"] = together, ["a read cut at byte " .. cut] = split}) do
        assert.are.equal(#apart, #other, "a different number of lines came out of " .. name)
        for i = 1, #apart do
          assert.are.equal(apart[i], other[i], "line " .. i .. " came out differently from " .. name)
        end
      end
      for name, reset in pairs({["a read per line"] = apartReset, ["one read"] = togetherReset, ["a cut read"] = splitReset}) do
        assert.are.equal(baseline, reset, "ESC[0m after " .. name .. " did not restore the default format")
      end
    end)
  end
end)
