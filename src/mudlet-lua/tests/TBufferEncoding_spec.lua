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
