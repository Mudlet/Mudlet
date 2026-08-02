-- Contract-only specs for the networking, MMCP, media and Discord APIs.
--
-- These functions all depend on live infrastructure for their real effect
-- (a connected game server, connected MMCP peers, an audio device, a running
-- Discord client). Those effects are covered by separate, stub-backed work.
-- What is verified here is the part that is fully deterministic offline:
-- argument validation, and the nil+message / hard-error shapes each function
-- returns when its precondition (a connection, a peer, an enabled protocol,
-- an available API) is not met. Nothing here mocks a real API function, opens
-- a socket, issues an HTTP request or asserts playback.
--
-- This is a new per-domain spec file. The standing convention is to extend an
-- existing domain spec file, but there is no existing home for the networking,
-- MMCP, media or Discord Lua API; this file provides one for their contract
-- layer. Whether to keep it as a single combined file is a convention call for
-- review.

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

-- Asserts that calling fn raises a Lua error whose message contains needle.
-- Matching a message substring (rather than merely "did it error?") ensures the
-- function is actually registered and reached its own argument validation: an
-- unregistered/nil function would raise a different "attempt to call" error.
local function assertArgError(fn, needle)
  local ok, err = pcall(fn)
  assert.is_false(ok)
  assert.is_true(contains(err, needle))
end

describe("Networking send functions honour their disconnected/offline contracts", function()
  -- Force a non-connected telnet state so the connection guards fire
  -- deterministically regardless of what the self-test profile's socket is
  -- doing. disconnect() only closes the socket; it issues no traffic.
  before_each(function()
    disconnect()
  end)

  describe("sendMSDP", function()
    it("raises a Lua error when called with no arguments", function()
      assert.has_error(function() sendMSDP() end)
    end)

    it("raises a Lua error when a value argument is not a string", function()
      assert.has_error(function() sendMSDP("HEALTH", {}) end)
    end)

    it("returns nil and a message while disconnected", function()
      local ok, err = sendMSDP("HEALTH")
      assert.is_nil(ok)
      assert.is_true(contains(err, "not connected to game server"))
    end)
  end)

  describe("sendATCP", function()
    it("names the offending value's real type when the message is not a string", function()
      -- Regression #9543: the type-name placeholder must be expanded, not printed
      -- as a literal "%1". lua_pushfstring only understands C-style "%s".
      local ok, err = pcall(function() sendATCP({}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "sendATCP: bad argument #1 type (message as string expected, got table!)"), tostring(err))
      assert.is_false(contains(err, "%1"), tostring(err))
    end)

    it("names the real type when the optional second argument is not a string", function()
      local ok, err = pcall(function() sendATCP("Char.Login", {}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "sendATCP: bad argument #2 type (what as string is optional, got table!)"), tostring(err))
      assert.is_false(contains(err, "%1"), tostring(err))
    end)

    it("returns nil and a message while disconnected", function()
      local ok, err = sendATCP("Char.Login")
      assert.is_nil(ok)
      assert.is_true(contains(err, "not connected to game server"))
    end)
  end)

  describe("sendTelnetChannel102", function()
    it("raises a Lua error when the payload is not a string", function()
      assert.has_error(function() sendTelnetChannel102({}) end)
    end)

    it("returns nil when the payload is not exactly two bytes", function()
      local ok, err = sendTelnetChannel102("x")
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid message of length 1"))
    end)

    it("returns nil when subchannel 102 has not been enabled by the server", function()
      local ok, err = sendTelnetChannel102("ab")
      assert.is_nil(ok)
      assert.is_true(contains(err, "102 subchannel support has not been enabled"))
    end)
  end)

  describe("sendSocket", function()
    it("raises a Lua error when the data is not a string", function()
      assert.has_error(function() sendSocket({}) end)
    end)

    it("returns nil and a message when the socket cannot accept the data", function()
      local ok, err = sendSocket("noop")
      assert.is_nil(ok)
      assert.is_true(contains(err, "unable to send"))
    end)
  end)
end)

describe("connectToServer validates its arguments without connecting", function()
  it("raises a Lua error when the url is missing", function()
    assert.has_error(function() connectToServer() end)
  end)

  it("rejects an out-of-range port and returns nil plus a message", function()
    local ok, err = connectToServer("example.invalid", 70000)
    assert.is_nil(ok)
    assert.is_true(contains(err, "invalid port number"))
  end)

  it("rejects a port below 1", function()
    local ok, err = connectToServer("example.invalid", 0)
    assert.is_nil(ok)
    assert.is_true(contains(err, "invalid port number"))
  end)
end)

describe("getConnectionInfo returns a host/port/connected triple", function()
  it("returns a string, a number and a boolean", function()
    local host, port, connected = getConnectionInfo()
    assert.is_string(host)
    assert.is_number(port)
    assert.is_boolean(connected)
  end)
end)

describe("HTTP and download functions validate arguments before issuing a request", function()
  -- Every case below returns (hard error, or nil+message) strictly before the
  -- network call: either the url is invalid (so the request is refused locally)
  -- or a valid-looking url is never contacted because a header/argument error is
  -- raised first.
  describe("downloadFile", function()
    it("raises a Lua error when the local filename is missing", function()
      assertArgError(function() downloadFile() end, "downloadFile: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() downloadFile("/tmp/mudlet-contract-test") end, "downloadFile: bad argument")
    end)

    it("returns nil for an invalid url without downloading", function()
      local ok, err = downloadFile("/tmp/mudlet-contract-test", "")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)
  end)

  describe("getHTTP", function()
    it("raises a Lua error when the url is missing", function()
      assertArgError(function() getHTTP() end, "getHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = getHTTP("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() getHTTP("http://localhost/", 5) end, "getHTTP: bad argument")
    end)

    it("raises a Lua error when a header value is not a string", function()
      assertArgError(function() getHTTP("http://localhost/", {["X-Test"] = 5}) end, "getHTTP: bad argument")
    end)
  end)

  describe("deleteHTTP", function()
    it("raises a Lua error when the url is missing", function()
      assertArgError(function() deleteHTTP() end, "deleteHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = deleteHTTP("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() deleteHTTP("http://localhost/", 5) end, "deleteHTTP: bad argument")
    end)

    it("raises a Lua error when a header value is not a string", function()
      assertArgError(function() deleteHTTP("http://localhost/", {["X-Test"] = 5}) end, "deleteHTTP: bad argument")
    end)
  end)

  describe("postHTTP", function()
    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() postHTTP() end, "postHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() postHTTP("payload") end, "postHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = postHTTP("payload", "")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() postHTTP("payload", "http://localhost/", 5) end, "postHTTP: bad argument")
    end)
  end)

  describe("putHTTP", function()
    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() putHTTP() end, "putHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() putHTTP("payload") end, "putHTTP: bad argument")
    end)
  end)

  describe("customHTTP", function()
    it("raises a Lua error when the method is missing", function()
      assertArgError(function() customHTTP() end, "customHTTP: bad argument")
    end)

    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() customHTTP("REPORT") end, "customHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() customHTTP("REPORT", "payload") end, "customHTTP: bad argument")
    end)

    it("reports the real type of a non-table headers argument", function()
      -- Regression #9544: performHttpRequest must read the type of the offending
      -- slot (pos + 3), not a hardcoded slot 3, so the headers error names the
      -- number that was actually passed rather than the url's type.
      local ok, err = pcall(function() customHTTP("REPORT", "payload", "http://localhost/", 5) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "customHTTP: bad argument #4 type (headers as a table expected, got number!)"), tostring(err))
    end)

    it("reports the real type of a non-string file argument", function()
      -- Regression #9544: the file error must read pos + 4, not a hardcoded slot 4,
      -- so it names the boolean that was passed and not the headers table's type.
      -- A boolean is used rather than a number because lua_isstring also accepts
      -- numbers, so only a genuinely non-string value reaches the type error.
      local ok, err = pcall(function() customHTTP("REPORT", "payload", "http://localhost/", {}, true) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "customHTTP: bad argument #5 type (file to send as string location expected, got boolean!)"), tostring(err))
    end)
  end)
end)

describe("openUrl validates its argument without launching anything", function()
  it("raises a Lua error when the url is missing", function()
    assertArgError(function() openUrl() end, "openUrl: bad argument")
  end)

  it("raises a Lua error when the url is not a string", function()
    assertArgError(function() openUrl({}) end, "openUrl: bad argument")
  end)
end)

describe("MMCP chat commands report the absence of a session", function()
  -- With no connected chat peers, every registered command reports its
  -- no-session state. initMMCPServer() runs lazily inside these calls; it
  -- constructs the server object but never calls listen(), so no socket is
  -- opened (only mmcpStartServer would, and it is not registered into the Lua
  -- mmcp table).
  local NO_CLIENTS = "no connected clients"
  local NO_SUCH = "no client by that name or id"

  it("chatAll returns nil with no peers", function()
    local ok, err = mmcp.chatAll("hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("emoteAll returns nil with no peers", function()
    local ok, err = mmcp.emoteAll("waves")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("chatGroup returns nil with no peers", function()
    local ok, err = mmcp.chatGroup("friends", "hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("getClientFlags returns nil with no peers", function()
    local ok, err = mmcp.getClientFlags("someone")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("sendSideChannel returns nil with no peers", function()
    local ok, err = mmcp.sendSideChannel("Chan", "msg")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("chatTo returns nil for an unknown target", function()
    local ok, err = mmcp.chatTo("nobody", "hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("ping returns nil for an unknown target", function()
    local ok, err = mmcp.ping("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("setPrivate returns nil for an unknown target", function()
    local ok, err = mmcp.setPrivate("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("serve returns nil for an unknown target", function()
    local ok, err = mmcp.serve("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("snoop returns nil for an unknown target", function()
    local ok, err = mmcp.snoop("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("allowSnoop returns nil for an unknown target", function()
    local ok, err = mmcp.allowSnoop("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("setGroup returns nil for an unknown target", function()
    local ok, err = mmcp.setGroup("nobody", "team")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("disconnect returns nil for an unknown target", function()
    local ok, err = mmcp.disconnect("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("ignore returns nil for an unknown target", function()
    local ok, err = mmcp.ignore("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("getClientList returns nil when there are no peers", function()
    assert.is_nil(mmcp.getClientList())
  end)

  it("chatTo requires a target argument", function()
    assert.has_error(function() mmcp.chatTo() end)
  end)

  it("chatAll requires a message argument", function()
    assert.has_error(function() mmcp.chatAll() end)
  end)

  describe("mmcp.call", function()
    it("raises a Lua error when the host is missing", function()
      assert.has_error(function() mmcp.call() end)
    end)

    it("rejects an out-of-range port without connecting", function()
      local ok, err = mmcp.call("127.0.0.1", 70000)
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid port number"))
    end)
  end)

  describe("mmcp.chatName", function()
    it("returns the current chat name as a string", function()
      assert.is_string(mmcp.chatName())
    end)

    it("rejects names containing a tilde or comma", function()
      local ok, err = mmcp.chatName("bad~name")
      assert.is_nil(ok)
      assert.is_true(contains(err, "tilde"))
    end)
  end)
end)

describe("Media playback functions validate their parameters", function()
  -- None of these reach playMedia()/stopMedia() on a real file, so no playback
  -- is started: each returns before the media engine is touched.
  describe("playSoundFile", function()
    it("raises a Lua error when called with no arguments", function()
      assertArgError(function() playSoundFile() end, "playSoundFile: need at least one argument")
    end)

    it("raises a Lua error when the table form has no name", function()
      assert.has_error(function() playSoundFile({}) end)
    end)

    it("raises a Lua error for a negative fadein in the table form", function()
      assert.has_error(function() playSoundFile({name = "x.wav", fadein = -1}) end)
    end)

    it("returns nil when the ordered form supplies no filename", function()
      local ok, err = playSoundFile(nil)
      assert.is_nil(ok)
      assert.is_true(contains(err, "missing argument 1"))
    end)
  end)

  describe("playMusicFile", function()
    it("raises a Lua error when called with no arguments", function()
      assertArgError(function() playMusicFile() end, "playMusicFile: need at least one argument")
    end)

    it("raises a Lua error when the table form has no name", function()
      assert.has_error(function() playMusicFile({}) end)
    end)

    it("raises a Lua error for a negative fadeout in the table form", function()
      assert.has_error(function() playMusicFile({name = "x.mp3", fadeout = -5}) end)
    end)

    it("raises a clean, non-doubled error when continue is not a boolean", function()
      -- Regression #9547 (same defect class): the field publicName must not carry
      -- "must be boolean", which errorArgumentType would then double.
      local ok, err = pcall(function() playMusicFile({name = "x.mp3", continue = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for continue as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)
  end)

  describe("playVideoFile", function()
    -- playVideoFileAsTableArgument shared the identical doubled-message defect on
    -- its continue/stream/close boolean fields (#9547 defect class).
    it("raises a clean, non-doubled error when continue is not a boolean", function()
      local ok, err = pcall(function() playVideoFile({name = "x.mp4", continue = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for continue as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)

    it("raises a clean, non-doubled error when stream is not a boolean", function()
      local ok, err = pcall(function() playVideoFile({name = "x.mp4", stream = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for stream as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)

    it("raises a clean, non-doubled error when close is not a boolean", function()
      local ok, err = pcall(function() playVideoFile({name = "x.mp4", close = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for close as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)
  end)

  describe("getPlayingSounds", function()
    it("raises a clean, non-doubled error when priority is not an integer", function()
      -- Regression #9547 (same defect class): "value for priority must be integer"
      -- doubled into "must be integer as number expected".
      local ok, err = pcall(function() getPlayingSounds({priority = "high"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for priority as number expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be integer"), tostring(err))
    end)
  end)

  describe("stopSounds", function()
    it("returns true when stopping everything with no arguments", function()
      assert.is_true(stopSounds())
    end)

    it("raises a Lua error for a negative fadeout in the table form", function()
      assert.has_error(function() stopSounds({fadeout = -1}) end)
    end)

    it("raises a clean, non-doubled error when priority is not an integer", function()
      -- Regression #9547 (same defect class, adjacent field in this very parser):
      -- "value for priority must be integer" doubled the type constraint.
      local ok, err = pcall(function() stopSounds({priority = "high"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for priority as number expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be integer"), tostring(err))
    end)

    it("raises a clean, non-doubled error when fadeaway is not a boolean", function()
      -- Regression #9547: the message must not double "boolean" (the field's
      -- publicName previously carried "must be boolean" while the type validator
      -- also appended "as boolean expected"). It is reported like the sibling
      -- table-field validations in this parser (fadeout, name, key).
      local ok, err = pcall(function() stopSounds({fadeaway = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for fadeaway as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)
  end)

  -- stopMusic and stopVideos parse the same table shape and shared the identical
  -- doubled-"boolean" fadeaway defect fixed for stopSounds (#9547).
  describe("stopMusic", function()
    it("raises a clean, non-doubled error when fadeaway is not a boolean", function()
      local ok, err = pcall(function() stopMusic({fadeaway = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for fadeaway as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)
  end)

  describe("stopVideos", function()
    it("raises a clean, non-doubled error when fadeaway is not a boolean", function()
      local ok, err = pcall(function() stopVideos({fadeaway = "yes"}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "value for fadeaway as boolean expected, got string!"), tostring(err))
      assert.is_false(contains(err, "must be boolean"), tostring(err))
    end)
  end)
end)

describe("receiveMSP reports MSP is not enabled while offline", function()
  it("returns nil and a message when MSP has not been negotiated", function()
    local ok, err = receiveMSP("!!SOUND(x.wav)")
    assert.is_nil(ok)
    assert.is_true(contains(err, "MSP is not currently enabled"))
  end)
end)

describe("Discord Lua API availability contract", function()
  -- Every rich-presence function is gated on the Discord API being available
  -- (the discord-rpc library loaded and Discord enabled for this profile). In
  -- CI the library is not on the load path, so each gated function is denied
  -- with the same stable reason. On a machine where Discord is live these
  -- pend instead of mutating real presence data.
  local gatedFunctions = {
    "usingMudletsDiscordID", "getDiscordDetail", "getDiscordLargeIcon",
    "getDiscordLargeIconText", "getDiscordParty", "getDiscordSmallIcon",
    "getDiscordSmallIconText", "getDiscordState", "getDiscordTimeStamps",
    "resetDiscordData", "setDiscordApplicationID", "setDiscordDetail",
    "setDiscordElapsedStartTime", "setDiscordGame", "setDiscordLargeIcon",
    "setDiscordLargeIconText", "setDiscordParty", "setDiscordRemainingEndTime",
    "setDiscordSmallIcon", "setDiscordSmallIconText", "setDiscordState",
  }

  -- Probe with a read-access getter. When it returns nil+message the API is
  -- denied and that message is the shared denial reason; otherwise the API is
  -- usable in this environment and the contract tests pend.
  local function discordDenial()
    local ok, msg = getDiscordState()
    if ok == nil and type(msg) == "string" then
      return msg
    end
    return nil
  end

  it("the denial reason refers to Discord", function()
    local denial = discordDenial()
    if not denial then
      pending("Discord API is enabled in this environment")
      return
    end
    assert.is_true(contains(denial, "Discord"))
  end)

  for _, fnName in ipairs(gatedFunctions) do
    it(fnName .. " returns the shared denial while the API is unavailable", function()
      local denial = discordDenial()
      if not denial then
        pending("Discord API is enabled in this environment")
        return
      end
      -- Called with no arguments: the availability gate is checked before any
      -- argument, so nothing is read or mutated on the denied path.
      local ok, msg = _G[fnName]()
      assert.is_nil(ok)
      assert.equals(denial, msg)
    end)
  end

  describe("setDiscordGameUrl (intentionally ungated)", function()
    -- setDiscordGameUrl changes the profile's invite button, not rich
    -- presence, so it has no availability gate. Only its argument type is a
    -- deterministic offline contract; the success path is left to effect tests
    -- as it mutates profile state.
    it("raises a Lua error when the url argument is not a string", function()
      assertArgError(function() setDiscordGameUrl({}) end, "setDiscordGameUrl: bad argument")
    end)
  end)
end)
