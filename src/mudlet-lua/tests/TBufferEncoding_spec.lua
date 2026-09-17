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
local function brokenUp(data)
  local broken = {}
  for i = 1, #data do
    broken[#broken + 1] = "\27[m" .. data:sub(i, i)
  end
  return table.concat(broken)
end

local function decodedByteByByte(data)
  return decoded(brokenUp(data))
end

-- every line a feed left behind, for data that does not stay on one line
local function decodedLines(data)
  local mark = getLastLineNumber("main")
  local ok, msg = feedTelnet(data .. "\r\n")
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  return getLines("main", mark, getLastLineNumber("main") + 1), mark
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

describe("Tests the bulk copy of plain text runs", function()
  -- The end of a run is looked for eight bytes at a time, so whatever ends one
  -- is tried at every place within such a group it can fall on. The text that
  -- follows it is long enough for the search to get going a second time.
  local filler = "The quick brown fox jumps over the lazy dog"

  local function atEveryOffset(check)
    for lead = 0, 17 do
      check(filler:sub(1, lead), filler, lead .. " characters into the run")
    end
  end

  it("copies every printable character through unchanged", function()
    -- in two halves, as a line holding all of them would be wrapped
    for _, range in ipairs({{0x20, 0x4F}, {0x50, 0x7E}}) do
      local printable = {}
      for byte = range[1], range[2] do
        printable[#printable + 1] = bytes(byte)
      end
      local text = "|" .. table.concat(printable) .. "|"

      assert.equals(text, decoded(text))
    end
  end)

  it("ends a run at a byte the code page has a character for", function()
    using("CP437")

    -- both ends of the part of the upper half that is text, 0xFF marking a prompt
    local characters = {[0x80] = "Ç", [0x86] = "å", [0xFE] = "■"}
    for byte, character in pairs(characters) do
      atEveryOffset(function(before, after, where)
        assert.equals(before .. character .. after, decoded(before .. bytes(byte) .. after), string.format("byte 0x%02X, %s", byte, where))
      end)
    end
  end)

  it("ends a run at the first byte of a UTF-8 sequence", function()
    using("UTF-8")

    for _, character in ipairs({"é", "€"}) do
      atEveryOffset(function(before, after, where)
        assert.equals(before .. character .. after, decoded(before .. character .. after), character .. ", " .. where)
      end)
    end
  end)

  it("ends a run at the one 7-bit byte EUC-KR rejects", function()
    using("EUC-KR")

    atEveryOffset(function(before, after, where)
      assert.equals(before .. replacement .. after, decoded(before .. bytes(0x7F) .. after), where)
    end)
  end)

  it("ends a run at a line feed", function()
    atEveryOffset(function(before, after, where)
      local lines = decodedLines("run:" .. before .. "\nrun:" .. after)

      assert.equals("run:" .. before, lines[1], where)
      assert.equals("run:" .. after, lines[2], where)
    end)
  end)

  it("ends a run at the other bytes that end a line just as the byte by byte decoder does", function()
    local endings = {
      {name = "End of Transmission", fed = bytes(0x04)},
      -- telnet's IAC as well, so it has to be doubled to arrive as data - and
      -- a carry out of it is what a test of eight bytes at once would get wrong
      {name = "the byte a prompt is marked with", fed = bytes(0xFF, 0xFF)},
    }
    for _, ending in ipairs(endings) do
      atEveryOffset(function(before, after, where)
        local expected = decodedLines(brokenUp("run:" .. before) .. ending.fed .. brokenUp("run:" .. after))

        assert.same(expected, decodedLines("run:" .. before .. ending.fed .. "run:" .. after), ending.name .. ", " .. where)
      end)
    end
  end)

  it("carries a control character that is plain text along with the run", function()
    atEveryOffset(function(before, after, where)
      assert.equals(before .. "\t" .. after, decoded(before .. "\t" .. after), where)
    end)
  end)

  it("starts a format change on the character right after its sequence", function()
    finally(function()
      deselect("main")
    end)

    local function boldAt(line, column)
      assert.is_true(moveCursor("main", 0, line))
      assert.is_true(selectSection("main", column, 1))
      return getTextFormat("main").bold
    end

    atEveryOffset(function(before, after, where)
      local lines, line = decodedLines("\27[0mfmt:" .. before .. "\27[1m" .. after .. "\27[0m")

      assert.equals("fmt:" .. before .. after, lines[1], where)
      local changeAt = #"fmt:" + #before
      assert.is_false(boldAt(line, changeAt - 1), "bold began a character early, " .. where)
      assert.is_true(boldAt(line, changeAt), "bold began a character late, " .. where)
      assert.is_true(boldAt(line, changeAt + #after - 1), "bold did not reach the end of the run, " .. where)
    end)
  end)
end)
