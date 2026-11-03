-- The game server encodings a player picks with setServerEncoding(), exercised
-- the way game text meets them: raw bytes driven through the real telnet parser
-- with feedTelnet(), then read back off the buffer as UTF-8. feedTelnet() only
-- injects while the profile is offline - see the tests README.
--
-- Five of the encodings are code pages Mudlet carries itself rather than
-- anything Qt supplies, and it carries each of them twice: game text coming in
-- is decoded through TEncodingTable.cpp, while text going back out to the game
-- is encoded through TTextCodec.cpp. A spec is the only thing that sees both
-- halves at once, so it is the only thing that notices when they drift apart.
--
-- TBufferEncoding_spec.lua covers the incoming half on its own, in more depth
-- and for the multibyte encodings too.

local feedCount = 0

local function feed(data)
  local ok, msg = feedTelnet(data)
  assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
end

-- Delivers the bytes as if the game had sent them under that encoding and hands
-- back what reached the buffer after the marker. The marker is unique per call
-- so an earlier test's identical text cannot answer instead.
local function decoded(encoding, bytes)
  assert.is_true(setServerEncoding(encoding), "the profile would not take the " .. encoding .. " encoding")
  feedCount = feedCount + 1
  local marker = "EncSpec" .. feedCount
  local mark = getLastLineNumber("main")
  feed(marker .. bytes .. "\r\n")
  local lines = getLines("main", mark, getLastLineNumber("main") + 1)
  for i = #lines, 1, -1 do
    local at = lines[i]:find(marker, 1, true)
    if at then
      return lines[i]:sub(at + #marker)
    end
  end
  return nil
end

local function contains(haystack, needle)
  return tostring(haystack):find(needle, 1, true) ~= nil
end

local function fileExists(path)
  return lfs.attributes(path, "mode") ~= nil
end

-- Takes a copy of the encoding in use, to be put back once a spec has changed
-- it. setServerEncoding() also writes the profile's "encoding" file, and a
-- profile that never had one must not be left with one.
local function restoreServerEncoding()
  local encodingFile = getMudletHomeDir() .. "/encoding"
  local hadFile = fileExists(encodingFile)
  local original = getServerEncoding()
  return function()
    setServerEncoding(original)
    if not hadFile then
      os.remove(encodingFile)
    end
  end
end

describe("Tests decoding game text in Mudlet's own code pages", function()

  local restore

  before_each(function()
    restore = restoreServerEncoding()
  end)

  after_each(function()
    restore()
  end)

  it("gives CP437 the accented Latin letters it puts above ASCII", function()
    -- 0x80 0x81 0x82 0xA0 are Ç ü é á in IBM code page 437
    assert.are.equal("\195\135\195\188\195\169\195\161", decoded("CP437", "\128\129\130\160"))
  end)

  it("gives CP437 its currency signs and box drawing and Greek", function()
    -- 0x9B ¢, 0x9E ₧, 0xB0 ░, 0xDB █, 0xE0 α
    assert.are.equal("\194\162\226\130\167\226\150\145\226\150\136\206\177", decoded("CP437", "\155\158\176\219\224"))
  end)

  -- CP437 and the Medievia code page agree from 0xB0 up but disagree below it,
  -- where Medievia has map glyphs and CP437 has letters. Reading CP437 through
  -- the Medievia table is the regression this pins: a game sending Ç arrives as
  -- a piece of map furniture instead.
  it("does not read CP437 through the Medievia table", function()
    assert.are.equal("\226\149\174", decoded("MEDIEVIA", "\128"), "Medievia's 0x80 is not the arc it should be")
    assert.are.equal("\195\135", decoded("CP437", "\128"), "CP437's 0x80 is not the C-cedilla it should be")
    assert.are.equal(decoded("MEDIEVIA", "\176"), decoded("CP437", "\176"), "the two tables should still agree on the shaded block")
  end)

  it("gives the Medievia code page its private use glyphs", function()
    -- 0x84 and 0x88 are U+E100 and U+E104, which only the Medievia font draws
    assert.are.equal("\238\132\128\238\132\132", decoded("MEDIEVIA", "\132\136"))
  end)

  it("gives CP667 the Polish letters that set it apart from CP437", function()
    -- 0x86 ą, 0x8F Ą, 0x92 ł, 0xA0 Ź
    assert.are.equal("\196\133\196\132\197\130\197\185", decoded("CP667", "\134\143\146\160"))
  end)

  it("gives CP737 Greek where CP437 has accented Latin", function()
    -- 0x80 Α, 0x97 Ω
    assert.are.equal("\206\145\206\169", decoded("CP737", "\128\151"))
    assert.are_not.equal(decoded("CP437", "\128"), decoded("CP737", "\128"))
  end)

  it("gives CP869 its own Greek and the euro sign", function()
    -- 0x86 is Ά. 0x87 is €, which is Mudlet's own addition: standard IBM
    -- code page 869 leaves that byte undefined.
    assert.are.equal("\206\134\226\130\172", decoded("CP869", "\134\135"))
  end)

  it("gives CP869 the upsilon with diaeresis rather than a second psi", function()
    -- 0x96 is Ϋ and 0xD4 is Ψ; a table carrying Ψ in both places has no Ϋ at all
    assert.are.equal("\206\171", decoded("CP869", "\150"))
    assert.are.equal("\206\168", decoded("CP869", "\212"))
  end)

  it("shows a byte its code page leaves undefined as the replacement character", function()
    -- CP869 defines nothing for 0x80, unlike every other code page Mudlet carries
    assert.are.equal("\239\191\189", decoded("CP869", "\128"))
  end)
end)

describe("Tests decoding game text that arrives split or already in UTF-8", function()

  local restore

  before_each(function()
    restore = restoreServerEncoding()
  end)

  after_each(function()
    restore()
  end)

  it("passes UTF-8 through untouched", function()
    assert.are.equal("\195\169\226\130\172", decoded("UTF-8", "\195\169\226\130\172"))
  end)

  -- The server can split a character across two packets, so the decoder has to
  -- hold the incomplete tail over rather than emit a replacement character for it
  it("carries an incomplete character over to the next packet", function()
    assert.is_true(setServerEncoding("UTF-8"))
    local mark = getLastLineNumber("main")
    feed("EncSpecSplit\195")
    feed("\169\r\n")
    local lines = getLines("main", mark, getLastLineNumber("main") + 1)
    local found
    for i = #lines, 1, -1 do
      local at = lines[i]:find("EncSpecSplit", 1, true)
      if at then
        found = lines[i]:sub(at + #"EncSpecSplit")
        break
      end
    end
    assert.are.equal("\195\169", found, "the character split across two packets did not survive")
  end)
end)

describe("Tests transcoding text out into the game server encoding", function()

  local restore

  before_each(function()
    restore = restoreServerEncoding()
  end)

  after_each(function()
    restore()
  end)

  -- feedTriggers() takes UTF-8 and has to put it into the game's encoding, which
  -- is the same conversion that carries what a player types out to the game. It
  -- is the only side of that conversion a spec can watch: there is no socket to
  -- read the sent bytes back off.
  local function transcoded(encoding, utf8Text)
    assert.is_true(setServerEncoding(encoding), "the profile would not take the " .. encoding .. " encoding")
    feedCount = feedCount + 1
    local marker = "EncSpecOut" .. feedCount
    local mark = getLastLineNumber("main")
    local ok, err = feedTriggers(marker .. utf8Text .. "\n")
    if not ok then
      return nil, err
    end
    local lines = getLines("main", mark, getLastLineNumber("main") + 1)
    for i = #lines, 1, -1 do
      local at = lines[i]:find(marker, 1, true)
      if at then
        return lines[i]:sub(at + #marker)
      end
    end
    return nil, "nothing carrying " .. marker .. " reached the buffer"
  end

  it("carries a character CP437 has out and back unchanged", function()
    -- Ç and u-umlaut are 0x80 and 0x81 in CP437, so this only survives if both
    -- halves of the code page agree on where they are
    assert.are.equal("\195\135\195\188", transcoded("CP437", "\195\135\195\188"))
  end)

  it("carries a character the Medievia code page has out and back unchanged", function()
    -- U+E100 is one of the private use map glyphs, at 0x84
    assert.are.equal("\238\132\128", transcoded("MEDIEVIA", "\238\132\128"))
  end)

  -- Transcoding out and decoding back in read different halves of a code page,
  -- so a byte the two halves disagree on comes back as a different character
  it("carries a character CP869 has out and back unchanged", function()
    assert.are.equal("\206\171", transcoded("CP869", "\206\171"))
    assert.are.equal("\206\168", transcoded("CP869", "\206\168"))
  end)

  it("refuses a character the game server encoding has no room for", function()
    local ok, err = transcoded("CP437", "\228\184\173")
    assert.is_nil(ok, "CP437 has no Chinese characters, so this should have been refused")
    assert.is_true(contains(err, "cannot be conveyed in the current game server encoding"), tostring(err))
  end)
end)

describe("Tests the functionality of getServerEncodingsList", function()

  it("offers ASCII along with Mudlet's own code pages and Qt's encodings", function()
    local list = getServerEncodingsList()
    assert.is_table(list)
    local offered = {}
    for _, name in ipairs(list) do
      offered[name] = true
    end
    for _, name in ipairs({"ASCII", "UTF-8", "ISO 8859-1", "BIG5", "CP437", "CP667", "CP737", "CP869", "MEDIEVIA"}) do
      assert.is_true(offered[name] == true, name .. " is missing from the list of encodings a game can use")
    end
  end)
end)

describe("Tests the functionality of setServerEncoding", function()

  local restore

  before_each(function()
    restore = restoreServerEncoding()
  end)

  after_each(function()
    restore()
  end)

  it("refuses an encoding it does not have", function()
    assert.is_true(setServerEncoding("ISO 8859-1"))
    local ok, err = setServerEncoding("NO-SUCH-ENCODING")
    assert.is_nil(ok, "an encoding Mudlet does not have should be refused")
    assert.is_true(contains(err, "NO-SUCH-ENCODING"), "the refusal does not name what was asked for: " .. tostring(err))
    assert.are.equal("ISO 8859-1", getServerEncoding(), "a refused encoding was applied regardless")
  end)

  -- The refusal is the only place a script author is shown what they could have
  -- asked for, so it has to carry the whole list, and in the names they can use
  it("names the encodings it does have when refusing one", function()
    local _, err = setServerEncoding("NO-SUCH-ENCODING")
    for _, name in ipairs(getServerEncodingsList()) do
      assert.is_true(contains(err, name), name .. " was left out of the refusal: " .. tostring(err))
    end
    assert.is_false(contains(err, "M_CP437"), "the refusal leaked an internal encoding name: " .. tostring(err))
  end)

  it("takes ASCII as the way to turn transcoding off", function()
    assert.is_true(setServerEncoding("UTF-8"))
    assert.is_true(setServerEncoding("ASCII"))
    assert.are.equal("ASCII", getServerEncoding())
  end)
end)
