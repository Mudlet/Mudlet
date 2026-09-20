-- Game data arrives as bytes and TBuffer turns it into the characters that land
-- in the buffer. feedTelnet() drives the real decoders (it only injects while
-- the profile is offline - see the tests README) and getLines() reads back what
-- they produced, so each case below is a byte sequence and the text it becomes.

local bytes = string.char

-- what TBuffer substitutes for a byte sequence it cannot decode
local replacement = "\239\191\189"

-- Takes a copy of the encoding in use, to be put back once a spec has changed
-- it. setServerEncoding() also writes the profile's "encoding" file, and a
-- profile that never had one must not be left with one.
local function restoreServerEncoding()
  local encodingFile = getMudletHomeDir() .. "/encoding"
  local hadFile = lfs.attributes(encodingFile, "mode") ~= nil
  local original = getServerEncoding()
  return function()
    setServerEncoding(original)
    if not hadFile then
      os.remove(encodingFile)
    end
  end
end

-- The prefix is ASCII, which every encoding here passes through untouched, so
-- it marks this feed's own line without disturbing the bytes under test.
local function decoded(data)
  local mark = getLastLineNumber("main")
  local ok, msg = feedTelnet("enc:" .. data .. "\r\n")
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  local lines = getLines("main", mark, getLastLineNumber("main") + 1)
  for i = #lines, 1, -1 do
    local payload = lines[i]:match("^enc:(.*)$")
    if payload then
      return payload
    end
  end
  error("no line carrying the fed bytes reached the buffer")
end

-- A run of bytes below 0x7F is copied into the line in one go by a fast path
-- that never reaches the decoder, so a byte in that range only goes through the
-- decoder when something breaks the run ahead of it. An SGR reset does that and
-- contributes no text of its own.
local function decodedByteByByte(data)
  local broken = {}
  for i = 1, #data do
    broken[#broken + 1] = "\27[m" .. data:sub(i, i)
  end
  return decoded(table.concat(broken))
end

-- Text comes back as UTF-8; code points spell out the expectations that have no
-- printable literal - private use characters and C1 controls.
local function codePoints(text)
  local out = {}
  local i = 1
  while i <= #text do
    local lead = text:byte(i)
    local value, length
    if lead < 0x80 then
      value, length = lead, 1
    elseif lead < 0xE0 then
      value, length = lead - 0xC0, 2
    elseif lead < 0xF0 then
      value, length = lead - 0xE0, 3
    else
      value, length = lead - 0xF0, 4
    end
    for continuation = 1, length - 1 do
      value = value * 64 + (text:byte(i + continuation) or 0) - 0x80
    end
    out[#out + 1] = value
    i = i + length
  end
  return out
end

local function using(encoding)
  finally(restoreServerEncoding())
  assert.is_true(setServerEncoding(encoding), "setServerEncoding refused " .. encoding)
end

describe("Tests the single byte encoding tables", function()

  it("gives byte 0x86 the character its own code page assigns to it", function()
    local expected = {
      ["CP437"] = "å",
      ["CP667"] = "ą",
      ["CP737"] = "Η",
      ["CP850"] = "å",
      ["CP869"] = "Ά",
      ["KOI8-R"] = "├",
    }
    finally(restoreServerEncoding())
    for encoding, character in pairs(expected) do
      assert.is_true(setServerEncoding(encoding), "setServerEncoding refused " .. encoding)
      assert.equals(character, decoded(bytes(0x86)), encoding .. " decoded byte 0x86 wrongly")
    end
  end)

  it("uses Mudlet's own table for the code page that has one", function()
    using("MEDIEVIA")

    -- Medievia maps the upper half onto a private use area its own font draws
    assert.same({0xE102}, codePoints(decoded(bytes(0x86))))
  end)

  it("leaves the ASCII half of a code page alone", function()
    using("CP437")

    assert.equals("Az~", decodedByteByByte(bytes(0x41, 0x7A, 0x7E)))
    assert.equals("Az~", decoded(bytes(0x41, 0x7A, 0x7E)), "the bulk copy of a plain run answered differently")
  end)
end)

describe("Tests decoding when no encoding is in use", function()

  it("replaces every byte that has its top bit set", function()
    using("CP437")
    assert.equals("å", decoded(bytes(0x86)), "the precondition failed - 0x86 is meant to be decodable to start with")

    assert.is_true(setServerEncoding("ASCII"))
    assert.equals("ASCII", getServerEncoding())
    assert.equals("A" .. replacement .. "B", decoded(bytes(0x41, 0x86, 0x42)))
  end)
end)

describe("Tests ISO 8859-1 decoding", function()

  it("maps each byte onto the Latin-1 character of the same value", function()
    using("ISO 8859-1")

    assert.equals("åþ", decoded(bytes(0xE5, 0xFE)))
  end)
end)

describe("Tests GBK decoding", function()

  it("decodes each of the areas the encoding is divided into", function()
    using("GBK")

    -- one pair from every range processGBSequence tells apart
    assert.same({0x4E02}, codePoints(decoded(bytes(0x81, 0x40))), "area 3")
    assert.same({0x3000}, codePoints(decoded(bytes(0xA1, 0xA1))), "area 1")
    assert.equals("啊", decoded(bytes(0xB0, 0xA1)), "area 2")
    assert.equals("ˊ", decoded(bytes(0xA8, 0x40)), "area 5")
    assert.equals("狜", decoded(bytes(0xAA, 0x40)), "area 4")
    assert.same({0xE000}, codePoints(decoded(bytes(0xAA, 0xA1))), "user defined area 1")
    assert.same({0xE234}, codePoints(decoded(bytes(0xF8, 0xA1))), "user defined area 2")
  end)

  it("decodes a pair sitting between ASCII bytes without disturbing them", function()
    using("GBK")

    assert.equals("A你B", decoded(bytes(0x41, 0xC4, 0xE3, 0x42)))
  end)

  it("rejects a lead byte the encoding has no meaning for", function()
    using("GBK")

    assert.equals(replacement, decoded(bytes(0x80)))
  end)

  it("rejects the second byte the areas carve out of their own range", function()
    using("GBK")
    assert.equals("丂", decoded(bytes(0x81, 0x40)), "the precondition failed - 0x81 is meant to be a usable lead byte")

    assert.equals(replacement, decoded(bytes(0x81, 0x7F)))
  end)

  it("consumes both bytes of a pair whose second byte is out of range", function()
    using("GBK")

    -- the space is part of the rejected pair, so only the Z survives it
    assert.equals(replacement .. "Z", decoded(bytes(0xC4, 0x20, 0x5A)))
  end)

  it("refuses the four byte sequences that only GB18030 defines", function()
    using("GBK")

    assert.equals(replacement, decoded(bytes(0x90, 0x30)), "the lead pair of a non-BMP sequence")
    assert.equals(replacement, decoded(bytes(0xFD, 0x30)), "the lead pair of a private use sequence")
  end)
end)

describe("Tests GB18030 decoding", function()

  it("decodes a four byte sequence into a character outside the BMP", function()
    -- decodes on Linux and Windows but reaches the buffer as nothing at all on
    -- macOS, with no replacement mark to show a character went missing (#10408)
    pending("a GB18030 sequence above the BMP vanishes on macOS")
  end)

  it("decodes a four byte sequence that lands inside the BMP", function()
    using("GB18030")

    assert.same({0x0080}, codePoints(decoded(bytes(0x81, 0x30, 0x81, 0x30))))
  end)

  it("rejects a four byte sequence whose lead byte is out of range", function()
    using("GB18030")

    assert.equals(replacement, decoded(bytes(0x85, 0x31, 0x81, 0x30)))
  end)

  it("still decodes the two byte sequences GBK shares with it", function()
    using("GB18030")

    assert.equals("你", decoded(bytes(0xC4, 0xE3)))
  end)

  it("rejects a lead byte the encoding has no meaning for", function()
    using("GB18030")

    assert.equals(replacement, decoded(bytes(0x80)))
  end)
end)

describe("Tests Big5 decoding", function()

  it("decodes second bytes from both of the ranges Big5 uses", function()
    using("BIG5")

    assert.equals("你好", decoded(bytes(0xA7, 0x41, 0xA6, 0x6E)), "second bytes from the lower range")
    assert.equals("中", decoded(bytes(0xA4, 0xA4)), "a second byte from the upper range")
  end)

  it("rejects a second byte from the gap between those two ranges", function()
    using("BIG5")

    assert.equals(replacement, decoded(bytes(0xA7, 0x80)))
  end)

  it("rejects a second byte below the lower range", function()
    using("BIG5")

    assert.equals(replacement, decoded(bytes(0xA7, 0x20)))
  end)

  it("rejects a lead byte the encoding has no meaning for", function()
    using("BIG5")

    assert.equals(replacement, decoded(bytes(0x80)))
  end)

  it("decodes the same bytes when the HKSCS superset is selected", function()
    using("BIG5-HKSCS")

    assert.equals("你", decoded(bytes(0xA7, 0x41)))
  end)
end)

describe("Tests EUC-KR decoding", function()

  it("decodes two byte Hangul", function()
    using("EUC-KR")

    assert.equals("한글", decoded(bytes(0xC7, 0xD1, 0xB1, 0xDB)))
  end)

  it("takes bytes below 0x7F as ASCII and rejects 0x7F itself", function()
    using("EUC-KR")

    assert.equals("~", decodedByteByByte(bytes(0x7E)))
    assert.equals(replacement, decoded(bytes(0x7F)))
  end)

  it("rejects a lead byte below the two byte range", function()
    using("EUC-KR")

    assert.equals(replacement, decoded(bytes(0xA0)))
  end)

  it("rejects a pair whose second byte is out of range", function()
    using("EUC-KR")

    assert.equals(replacement, decoded(bytes(0xC7, 0x20)))
  end)
end)

describe("Tests a character whose bytes are split by the posting timeout", function()

  -- cTelnet holds on to a line the game has not finished sending, and flushes
  -- what it has by appending a carriage return of its own once the game has been
  -- quiet for the network packet timeout (cTelnet::mTimeOut, 300ms by default).
  -- That marker used to be decoded as though it were the rest of the multi-byte
  -- character the same pause had split, which cost the character a pair of
  -- replacement marks and, in Big5 and the GB encodings, the byte after it as
  -- well - see issue #10766.

  local telnetDirectory = os.getenv("MUDLET_TEST_TELNET_DIR")
  local fixtureRequired = os.getenv("MUDLET_TEST_REQUIRE_TELNET_FIXTURE")

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- Comfortably past the timeout, in slices, so the posting timer gets to run:
  local function beQuiet()
    local waited = 0
    while waited < 500 do
      assert.is_true(pumpEvents(50), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
      waited = waited + 50
    end
  end

  local function connected()
    local _, _, isConnected = getConnectionInfo()
    return isConnected
  end

  local function waitFor(predicate)
    for _ = 1, 100 do
      if predicate() then
        return true
      end
      pumpEvents(50)
    end
    return predicate()
  end

  -- Feeds part of a line and answers whether the flush committed it, which is
  -- the whole precondition of the cases below.
  local function postingTimerRuns()
    local mark = getLastLineNumber("main")
    feed("timercheck")
    beQuiet()
    local flushed = false
    for _, line in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do
      if line:find("timercheck", 1, true) then
        flushed = true
      end
    end
    -- Held rather than flushed, the probe would otherwise open the next line:
    feed("\n")
    beQuiet()
    return flushed
  end

  -- The posting timer stops for the rest of the session once a game has ended a
  -- line with IAC GA (cTelnet::mGA_Driver): such a game marks its own line ends,
  -- so the timeout has nothing left to do. Only connecting or disconnecting
  -- clears that, and other spec files sharing this profile do feed an IAC GA, so
  -- put the session back by taking a connection to the test fixture and dropping
  -- it again. Without it no flush marker is produced at all, which the line
  -- count each case asserts catches - so a reconnect that stopped working would
  -- turn these red rather than quietly green, and that assertion is what makes
  -- it so.
  local function restorePostingTimer()
    if postingTimerRuns() then
      return nil
    end
    local handle = telnetDirectory and io.open(telnetDirectory .. "/port", "r")
    if not handle then
      return "an earlier spec stopped cTelnet's posting timer and the telnet fixture is not running to "
             .. "restart it (run CI/telnet-fixture-server.py with MUDLET_TEST_TELNET_DIR set)"
    end
    local port = tonumber((handle:read("*a") or ""):match("%d+") or "")
    handle:close()
    if not port then
      return "the telnet fixture wrote no port to " .. telnetDirectory .. "/port"
    end
    connectToServer("127.0.0.1", port)
    if not waitFor(connected) then
      return "never connected to the telnet fixture on port " .. port
    end
    disconnect()
    if not waitFor(function() return not connected() end) then
      return "the telnet fixture connection outlived the reconnect"
    end
    if not postingTimerRuns() then
      return "cTelnet's posting timer did not start again after a reconnect"
    end
    return nil
  end

  -- nil once the timer is running again, otherwise why it could not be
  local unavailable

  setup(function()
    if not os.getenv("MUDLET_TEST_MODE") then
      -- beQuiet() leans on pumpEvents() to let the posting timer run, and that
      -- returns nothing at all outside test mode - so the flush these cases are
      -- about never happens, and the profile opened by hand through runTests
      -- would meet an assertion rather than a skip
      unavailable = "waiting out the posting timeout needs MUDLET_TEST_MODE (pumpEvents() does nothing without it)"
      return
    end
    unavailable = restorePostingTimer()
  end)

  local function timerUnavailable()
    if not unavailable then
      return false
    end
    if fixtureRequired then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_TELNET_FIXTURE is set but " .. unavailable)
    end
    pending(unavailable)
    return true
  end

  -- The foreground colour of the first "RED" in the buffer from lineNumber on,
  -- or nil when there is none - which is itself a result worth asserting
  local function redForegroundFrom(lineNumber)
    for line = lineNumber, getLastLineNumber("main") do
      moveCursor("main", 0, line)
      if selectString("RED", 1) >= 0 then
        return {getFgColor("main")}
      end
    end
    return nil
  end

  -- Hands the pieces over with a quiet gap longer than the timeout between each,
  -- so the flush marker lands between them, and answers what reached the buffer:
  -- the text after the mark, and how many buffer lines it took. The marker ends
  -- a line, so a feed the timer really did interrupt spans more than one - every
  -- case checks that too, or a gap too short to reach the timeout would pass
  -- them without ever exercising the flush.
  local function splitAcrossTimeout(...)
    local pieces = {...}
    local mark = getLastLineNumber("main")
    feed("split:" .. pieces[1])
    for i = 2, #pieces do
      beQuiet()
      feed(i == #pieces and (pieces[i] .. ":end\n") or pieces[i])
    end
    beQuiet()

    local payload = {}
    for _, line in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do
      if #payload > 0 then
        if line ~= "" then
          payload[#payload + 1] = line
        end
      else
        local first = line:match("^split:(.*)$")
        if first then
          payload[1] = first
        end
      end
    end
    assert.is_true(#payload > 0, "no line carrying the fed bytes reached the buffer")
    return table.concat(payload), #payload, payload
  end

  -- Feeds "CSI 31 m RED" parted at the point its arguments name and checks the
  -- colour reached the screen, rather than only that the sequence stopped
  -- printing as text. Which colour ANSI 31 is depends on the profile's palette,
  -- so the same sequence arriving whole says what to expect - and a line with no
  -- sequence at all says what a lost colour looks like, without which a
  -- regression that stopped SGR 31 applying anywhere would leave both
  -- deliveries default-coloured and both comparisons content. A mark is the
  -- number of the empty line the next feed fills, so each one is taken just
  -- before the feed it belongs to.
  local function expectRedSurvivesSplit(...)
    local plainMark = getLastLineNumber("main")
    feed("plain:RED\n")
    beQuiet()

    local splitMark = getLastLineNumber("main")
    local text, lines, perLine = splitAcrossTimeout(...)
    assert.equals("RED:end", text)
    assert.equals(2, lines)
    assert.same({"", "RED:end"}, perLine)

    local splitColour = redForegroundFrom(splitMark)
    assert.is_not_nil(splitColour, "no coloured text to read a colour from")
    local wholeMark = getLastLineNumber("main")
    feed("whole:\27[31mRED\27[m\n")
    beQuiet()
    assert.same(redForegroundFrom(wholeMark), splitColour)
    assert.are_not.same(redForegroundFrom(plainMark), splitColour)
  end

  it("breaks an ASCII line at the flush marker", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- the shape every case below is measured against: the marker ends the line
    -- the first piece arrived on, and the rest opens the next one
    local text, lines = splitAcrossTimeout("AB", "CD")
    assert.equals("ABCD:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a two byte UTF-8 character the marker lands inside", function()
    if timerUnavailable() then return end
    using("UTF-8")

    local text, lines = splitAcrossTimeout(bytes(0xC3), bytes(0xA9))
    assert.equals("é:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a three byte UTF-8 character the marker lands inside", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- U+65E5, the character the defect was reported with
    local text, lines, perLine = splitAcrossTimeout(bytes(0xE6, 0x97), bytes(0xA5))
    assert.equals("日:end", text)
    assert.equals(2, lines)
    -- Which line each piece lands on, not just what they add up to: the marker
    -- ends the line the first piece opened, and the character belongs wholly to
    -- the line the rest of its bytes opened - one byte held too many or too few
    -- would move it and still add up the same
    assert.same({"", "日:end"}, perLine)
  end)

  it("keeps a four byte UTF-8 character two markers land inside", function()
    if timerUnavailable() then return end
    using("UTF-8")

    local text, lines = splitAcrossTimeout(bytes(0xF0, 0x9F), bytes(0x98), bytes(0x80))
    assert.equals("😀:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a Big5 character, and the byte after it, when the marker lands inside", function()
    if timerUnavailable() then return end
    using("BIG5")

    -- U+4E2D; the rejected pair used to take the ":" following it as well
    local text, lines = splitAcrossTimeout(bytes(0xA4), bytes(0xA4))
    assert.equals("中:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a GB18030 four byte character the marker lands inside", function()
    if timerUnavailable() then return end
    using("GB18030")

    -- U+00A5; the rejected sequence used to print the "6" of its last pair.
    -- A four byte sequence that lands inside the BMP, because one above it
    -- reaches the buffer as nothing at all on macOS whether it was split or
    -- not (#10408) - the split is what this case is about, and the hold is the
    -- same code either way
    local text, lines = splitAcrossTimeout(bytes(0x81, 0x30), bytes(0x84, 0x36))
    assert.equals("¥:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a GBK character, and the byte after it, when the marker lands inside", function()
    if timerUnavailable() then return end
    using("GBK")

    -- GBK holds back a lone lead byte from a different place in the decoder than
    -- GB18030 does, so it needs a case of its own
    local text, lines = splitAcrossTimeout(bytes(0xA4), bytes(0xA4))
    assert.equals("い:end", text)
    assert.equals(2, lines)
  end)

  it("keeps a GB18030 character the marker lands after its first byte", function()
    if timerUnavailable() then return end
    using("GB18030")

    -- The other GB18030 case parts the sequence once its length is already
    -- known; this one parts it before the second byte says how long it is
    local text, lines = splitAcrossTimeout(bytes(0x81), bytes(0x30, 0x84, 0x36))
    assert.equals("¥:end", text)
    assert.equals(2, lines)
  end)

  it("keeps an EUC-KR character the marker lands inside", function()
    if timerUnavailable() then return end
    using("EUC-KR")

    -- the rejected pair used to take the ":" following it as well
    local text, lines = splitAcrossTimeout(bytes(0xC7), bytes(0xD1))
    assert.equals("한:end", text)
    assert.equals(2, lines)
  end)

  it("keeps an ANSI colour sequence the marker lands inside", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- The marker is no more part of an escape sequence than it is of a
    -- character: held with the half of "CSI 31 m" that had arrived, it would
    -- never match a parameter byte when the rest turned up, so the colour would
    -- be dropped and the "1m" that completes it printed as text
    expectRedSurvivesSplit("\27[3", "1mRED\27[m")
  end)

  it("keeps an ANSI colour sequence the marker lands right after its escape", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- The narrowest place the marker can part a sequence: the escape has
    -- arrived and nothing else has. Read as the byte after the escape it names
    -- no sequence, so the escape used to be taken for a stray one and thrown
    -- away, leaving the whole of "[31m" to print as text once it turned up
    -- - see issue #10874
    expectRedSurvivesSplit("\27", "[31mRED\27[m")
  end)

  it("keeps a character set designation the marker lands inside", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- ESC ( B names a character set and shows nothing, but its final byte waits
    -- on a second latch (mGotEscCharset) rather than the one the case above
    -- exercises: with the marker taken for that byte the designation was
    -- abandoned and the "B" printed as text - see issue #10874
    local text, lines, perLine = splitAcrossTimeout("\27(", "B")
    assert.equals(":end", text)
    assert.equals(2, lines)
    assert.same({"", ":end"}, perLine)
  end)

  it("keeps an operating system command the marker lands right after its escape", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- The same split where the escape turns out to open an OSC rather than a
    -- colour sequence - the branch Mudlet reads hyperlinks on: with the escape
    -- gone the payload used to be put on the line as text
    local text, lines, perLine = splitAcrossTimeout("\27", "]0;title\27\\")
    assert.equals(":end", text)
    assert.equals(2, lines)
    assert.same({"", ":end"}, perLine)
  end)

  it("spends a held character set designation on the byte after the pause", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- What holding the latch costs, which is the same as what an ordinary
    -- packet boundary costs: a designation the game never completes takes the
    -- next byte it sends, however long the quiet spell was. A held escape on
    -- its own does not - "H" names no sequence, so it stays text. The two
    -- latches part company here, which makes this the easiest place for a
    -- later tidy-up to go wrong
    local eaten = splitAcrossTimeout("\27(", "Hello")
    assert.equals("ello:end", eaten)
    local kept = splitAcrossTimeout("\27", "Hello")
    assert.equals("Hello:end", kept)
  end)

  it("keeps the held escape when locally fed text arrives during the pause", function()
    if timerUnavailable() then return end
    using("UTF-8")

    -- Holding the latch opens a window nothing could land in before: the game
    -- is part way through a sequence and quiet, so anything the profile itself
    -- prints meanwhile meets that parser. Local text runs through it on a copy
    -- of the sequence state of its own (TBuffer::swapParserSequenceState()) and
    -- has to leave the game's alone
    local mark = getLastLineNumber("main")
    feed("split:\27")
    beQuiet()
    feedTriggers("interleaved\n")
    feed("[31mRED\27[m:end\n")
    beQuiet()

    local seen = {}
    for _, line in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do
      if line ~= "" then
        seen[#seen + 1] = line
      end
    end
    assert.same({"split:", "interleaved", "RED:end"}, seen)
  end)

  it("takes a carriage return in locally fed text as data, not as a marker", function()
    using("UTF-8")

    -- Only cTelnet makes the marker, so a carriage return handed to
    -- feedTriggers() is the caller's own byte and the decoder has to see it: the
    -- truncated lead byte ahead of it is rejected together with it, in the one
    -- replacement mark, and does not end the line.
    local mark = getLastLineNumber("main")
    feedTriggers("local:" .. bytes(0xC3, 0x0D))
    feedTriggers("tail\n")

    local seen
    for _, line in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do
      if line:find("^local:") then
        seen = line
      end
    end
    assert.equals("local:" .. replacement .. "tail", seen)
  end)
end)

describe("Tests changing the encoding from a trigger", function()
  -- A trigger fires while its packet is still being decoded. One that changes
  -- the encoding and feeds more text re-enters the decoder, which takes the new
  -- encoding up there and then, and the rest of the outer packet has to follow
  -- it too.
  it("decodes the rest of the packet with the encoding a re-entering trigger set", function()
    -- busted keeps one finally() per test, so the trigger and the encoding
    -- have to be put back from the same one
    local restoreEncoding = restoreServerEncoding()
    assert.is_true(setServerEncoding("UTF-8"))
    -- The buffer takes the encoding up when a packet arrives, not when it is
    -- set, so settle it on UTF-8 with a packet of its own first. Without this
    -- the packet below starts on whichever decoder the spec before it left
    -- behind, and every multi-byte decoder reads the encoding of the moment -
    -- the very thing the trigger changes - so the case would pass however
    -- coarsely the decoder was resolved.
    assert.equals("settled", decoded("settled"))
    local trigger = tempTrigger("enc:switch", function()
      setServerEncoding("GBK")
      feedTriggers("switched\n")
    end)
    finally(function()
      killTrigger(trigger)
      restoreEncoding()
    end)

    assert.equals("中", decoded("switch\r\nenc:" .. bytes(0xD6, 0xD0)))
  end)
end)
