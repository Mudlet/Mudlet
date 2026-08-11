-- Specs for the Discord rich-presence Lua API.
--
-- Networking_spec.lua covers the availability contract - every gated function
-- returning the same denial while the discord-rpc library cannot be loaded.
-- These specs need the opposite arrangement: a Discord client to talk to.
-- CI/discord-ipc-fixture.py is one, a fake Discord IPC server that completes
-- the genuine discord-rpc handshake, reports a logged-in user, and appends
-- every frame the library sends it to the capture file named by
-- MUDLET_TEST_DISCORD_CAPTURE_FILE. That capture is what is asserted on here:
-- the SET_ACTIVITY payload that actually reached "Discord", rather than the
-- return value of the setter that produced it. Nothing is mocked - the real
-- libdiscord-rpc does the talking, over a real socket.
--
-- To run them locally, start the fixture first:
--   python3 CI/discord-ipc-fixture.py --runtime-dir "$(mktemp -d /tmp/mdxdg-XXXX)" \
--       --capture-file /tmp/discord-frames.jsonl --ready-file /tmp/discord-ready &
-- then start Mudlet with XDG_RUNTIME_DIR set to that runtime directory,
-- MUDLET_TEST_DISCORD_CAPTURE_FILE to that capture file, and LD_LIBRARY_PATH
-- including 3rdparty/discord/rpc/lib so the bundled library can be found.
--
-- The fixture has to be listening BEFORE Mudlet starts. discord-rpc's
-- reconnect backoff is process-global, survives Discord_Shutdown and gates
-- even the READY read, so a server that only appears after the first failed
-- attempt costs up to a couple of minutes instead of the ~1s a cold start
-- takes.

local capturePath = os.getenv("MUDLET_TEST_DISCORD_CAPTURE_FILE")
-- A developer's local run without the fixture pends the whole family; CI sets
-- MUDLET_TEST_REQUIRE_DISCORD so that a workflow which stops starting the
-- fixture, or an image where the library cannot be loaded, fails instead of
-- quietly skipping everything.
local requireDiscord = os.getenv("MUDLET_TEST_REQUIRE_DISCORD")

-- Mudlet's own Discord application, from Discord::mMudletApplicationId
local mudletApplicationId = "450571881909583884"
-- MidMUD's, one of the registered test applications listed in src/discord.cpp
local otherApplicationId = "460618737712889858"

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

-- Asserts that calling fn raises a Lua error whose message contains needle.
-- Matching the message rather than merely "did it error?" proves the call
-- reached its own argument validation: an unregistered function would raise a
-- different "attempt to call" error.
local function assertArgError(fn, needle)
  local ok, err = pcall(fn)
  assert.is_false(ok)
  assert.is_true(contains(err, needle), tostring(err))
end

-- Every frame the fake Discord client has recorded so far, oldest first, still
-- as JSON text. Only whole lines are taken: the fixture appends one JSON
-- object per line in a single write, so an unterminated tail can only be a
-- record still being written.
local function capturedLines()
  local handle = io.open(capturePath, "rb")
  if not handle then
    return {}
  end
  local body = handle:read("*a")
  handle:close()
  local lines = {}
  for line in body:gmatch("[^\n]+\n") do
    lines[#lines + 1] = line
  end
  return lines
end

local function frameCount()
  return #capturedLines()
end

-- The frames recorded after `mark`, as {op = <opcode>, payload = <frame>}.
-- Decoding from `mark` rather than from the start of the file is what keeps
-- the polling below cheap as the capture grows over a suite run.
local function framesAfter(mark)
  local lines = capturedLines()
  local frames = {}
  for index = mark + 1, #lines do
    local decoded = select(2, pcall(yajl.to_value, lines[index]))
    if type(decoded) == "table" then
      frames[#frames + 1] = decoded
    end
  end
  return frames
end

-- Hands control back to Mudlet's event loop for a moment. The frames arrive on
-- discord-rpc's own IO thread and are written by the fixture process, so the
-- specs can only see them while Mudlet is idle.
local function pumpEventLoop(milliseconds)
  tempTimer(milliseconds / 1000, function() raiseEvent("bustedDiscordTick") end)
  waitForEvent("bustedDiscordTick", milliseconds + 1000)
end

-- The rich presence of the first SET_ACTIVITY recorded after `mark` that
-- `accept` is satisfied with. Every new frame is offered to `accept`, not just
-- the newest one, so an update that lands while waiting cannot make the
-- expectation unsatisfiable. On timeout the most recent frame seen is returned
-- instead, so a spec whose expectation is never met reports what Discord
-- really received rather than a bare timeout.
local function waitForActivity(mark, accept, timeoutMilliseconds)
  timeoutMilliseconds = timeoutMilliseconds or 5000
  local waited = 0
  local latest
  while true do
    for _, frame in ipairs(framesAfter(mark)) do
      if frame.op == 1 and type(frame.payload) == "table" and frame.payload.cmd == "SET_ACTIVITY" then
        latest = frame.payload.args.activity
        if latest and (not accept or accept(latest)) then
          return latest
        end
      end
    end
    if waited >= timeoutMilliseconds then
      return latest
    end
    -- A frame normally lands within a few milliseconds of the setter, so poll
    -- finely: over the whole file this is the difference between adding a
    -- couple of seconds to the suite and adding ten.
    pumpEventLoop(10)
    waited = waited + 10
  end
end

-- Runs `action` and returns the rich presence Discord received because of it.
local function activityFrom(action, accept, timeoutMilliseconds)
  local mark = frameCount()
  action()
  local activity = waitForActivity(mark, accept, timeoutMilliseconds)
  assert.is_table(activity, "no SET_ACTIVITY frame reached the fake Discord client in time")
  -- Fail here, on the whole payload, rather than leaving the spec's own
  -- assertions to index a field that never arrived and report a nil error.
  if accept and not accept(activity) then
    assert.is_true(false, "the presence Discord received is not the one expected: " .. tostring(select(2, pcall(yajl.to_string, activity))))
  end
  return activity
end

-- How many of the frames recorded after `mark` the fake Discord client could
-- not decode. The fixture files a frame whose payload is not valid JSON (or not
-- valid UTF-8, which JSON decoding of the payload requires) as {"raw": <text>}
-- instead of the parsed object, so this counts exactly the presence updates a
-- real Discord client would have had to throw away whole.
local function undecodableFramesAfter(mark)
  local count = 0
  for _, frame in ipairs(framesAfter(mark)) do
    if type(frame.payload) == "table" and frame.payload.raw ~= nil then
      count = count + 1
    end
  end
  return count
end

-- How many presence updates have reached the fake Discord client. Counting
-- SET_ACTIVITY frames rather than all of them keeps an unrelated handshake or
-- subscription from being mistaken for a presence update.
local function activityFrameCount()
  local count = 0
  for _, frame in ipairs(framesAfter(0)) do
    if frame.op == 1 and type(frame.payload) == "table" and frame.payload.cmd == "SET_ACTIVITY" then
      count = count + 1
    end
  end
  return count
end

-- The application IDs of the handshakes recorded after `mark`, waited for
-- until at least one arrives. Changing the application ID makes discord-rpc
-- tear its connection down and hand the new ID over in a fresh handshake,
-- which is the only externally visible proof that the switch took effect.
local function waitForHandshakes(mark, timeoutMilliseconds)
  local waited = 0
  while true do
    local applicationIds = {}
    for _, frame in ipairs(framesAfter(mark)) do
      if frame.op == 0 then
        applicationIds[#applicationIds + 1] = frame.payload.client_id
      end
    end
    if #applicationIds > 0 or waited >= (timeoutMilliseconds or 20000) then
      return applicationIds
    end
    pumpEventLoop(50)
    waited = waited + 50
  end
end

local function discordApiAvailable()
  -- A read-access getter: nil plus a message means the API is gated off, any
  -- string means the library is loaded and Discord is enabled for this profile.
  return getDiscordState() ~= nil
end

-- Established lazily by connectedToFakeDiscord(): discord-rpc opens its
-- connection on the first presence update and only sends once the READY
-- dispatch has arrived, so the first frame of a run takes about a second while
-- every later one lands within ~50ms.
local connectionProbe
-- Latched once a reset stops coming back, so that a fixture which dies partway
-- through fails the rest of the file at once instead of spending every
-- remaining spec's timeout on a connection that is not going to answer.
local connectionLost = false

-- What resetDiscordData() puts on the wire: everything cleared but the Mudlet
-- logo, which is not profile data. Recognising it exactly is what lets the
-- reset below double as a drain - once that frame has been seen, no frame from
-- an earlier spec can still be in flight.
local function emptyPresence(activity)
  return type(activity.assets) == "table" and activity.assets.large_image == "mudlet" and activity.assets.large_text == nil
         and activity.assets.small_image == nil and activity.assets.small_text == nil and activity.details == nil
         and activity.state == nil and activity.party == nil and activity.timestamps == nil
end

-- Clears the presence and waits for the frame that proves it arrived, which is
-- also the whole of the connection probe on the first call.
local function resetPresence(timeoutMilliseconds)
  local mark = frameCount()
  resetDiscordData()
  local activity = waitForActivity(mark, emptyPresence, timeoutMilliseconds)
  -- Re-checked, because waitForActivity() hands back the last frame it saw
  -- when it times out rather than nothing at all.
  return type(activity) == "table" and emptyPresence(activity)
end

local function connectedToFakeDiscord()
  if connectionProbe == nil then
    connectionProbe = resetPresence(20000)
  end
  return connectionProbe
end

-- Every spec below starts here. Returns false when there is no fake Discord
-- client to talk to, and otherwise leaves the presence empty so that the frame
-- the spec's own call produces is unambiguous. The reset is re-checked every
-- time rather than trusting the first probe: a fixture that dies mid-run would
-- otherwise let the remaining specs pass without asserting anything.
local function readyForDiscord()
  local reason
  if not capturePath then
    reason = "MUDLET_TEST_DISCORD_CAPTURE_FILE is not set (fake Discord IPC server not running)"
  elseif not discordApiAvailable() then
    reason = "the Discord API is unavailable (discord-rpc could not be loaded, or Discord is disabled for this profile)"
  elseif not connectedToFakeDiscord() then
    reason = "no presence update reached the fake Discord IPC server"
  elseif connectionLost then
    reason = "the fake Discord IPC server did not see the cleared presence resetDiscordData() should have sent"
  elseif not resetPresence(8000) then
    connectionLost = true
    reason = "the fake Discord IPC server did not see the cleared presence resetDiscordData() should have sent"
  end
  if reason then
    if requireDiscord then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_DISCORD is set but " .. reason)
    end
    pending(reason)
    return false
  end
  return true
end

describe("Discord presence reaches Discord", function()
  it("completes the IPC handshake with Mudlet's own application ID", function()
    if not readyForDiscord() then
      return
    end
    -- The first handshake of the run, which is the one Mudlet's own presence
    -- opened before any spec asked for a different application.
    assert.equals(mudletApplicationId, waitForHandshakes(0)[1])
  end)

  it("reports that the default Mudlet application ID is in use", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(usingMudletsDiscordID())
  end)

  it("sends the Mudlet logo as the large icon when the profile sets none", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordState("no icon of its own") end)
    assert.equals("mudlet", activity.assets.large_image)
  end)

  it("leaves every read-access function callable while Discord is available", function()
    if not readyForDiscord() then
      return
    end
    -- The mirror of Networking_spec.lua's availability contract: those specs
    -- prove one shared denial while the API is gated off, this one proves the
    -- same set is reachable once it is not.
    local readers = {
      "getDiscordDetail", "getDiscordLargeIcon", "getDiscordLargeIconText",
      "getDiscordParty", "getDiscordSmallIcon", "getDiscordSmallIconText",
      "getDiscordState", "getDiscordTimeStamps", "usingMudletsDiscordID",
    }
    for _, name in ipairs(readers) do
      assert.is_not_nil(_G[name](), name .. " should be reachable while Discord is available")
    end
  end)
end)

describe("setDiscordDetail", function()
  it("sends the detail text to Discord", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordDetail("Exploring the fixture") end,
                                  function(seen) return seen.details == "Exploring the fixture" end)
    assert.equals("Exploring the fixture", activity.details)
  end)

  it("reports the detail text it sent", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordDetail("Hunting in the woods"))
    assert.equals("Hunting in the woods", getDiscordDetail())
  end)

  it("substitutes a placeholder for an empty detail text", function()
    if not readyForDiscord() then
      return
    end
    -- The placeholder is tr("via Mudlet"), so a localised Mudlet sends a
    -- different string; what matters is that something took the empty text's
    -- place and that it is what the getter reports.
    local activity = activityFrom(function() setDiscordDetail("") end,
                                  function(seen) return seen.details ~= nil end)
    assert.is_true(#activity.details > 1)
    assert.equals(getDiscordDetail(), activity.details)
  end)

  it("refuses a one character detail text and leaves the presence alone", function()
    if not readyForDiscord() then
      return
    end
    -- Waited for, so that the frame count below can only move if the rejected
    -- call produced one of its own.
    activityFrom(function() setDiscordDetail("still here") end,
                 function(seen) return seen.details == "still here" end)
    local presenceUpdates = activityFrameCount()
    local ok, message = setDiscordDetail("x")
    assert.is_nil(ok)
    assert.is_true(contains(message, "text of length 1 not allowed by Discord"))
    assert.equals("still here", getDiscordDetail())
    -- A rejected setter must not reach Discord at all, so no further presence
    -- update is expected - unlike everywhere else, seeing one here is the
    -- failure.
    pumpEventLoop(500)
    assert.equals(presenceUpdates, activityFrameCount())
  end)

  it("sends a detail text containing a percent sequence unchanged", function()
    if not readyForDiscord() then
      return
    end
    -- Only what reaches Discord is asserted on here; reading the same text back
    -- is the other half, covered by the percent sequence specs below.
    local activity = activityFrom(function() setDiscordDetail("Level %d Mage") end,
                                  function(seen) return seen.details ~= nil end)
    assert.equals("Level %d Mage", activity.details)
  end)

  it("raises a Lua error when the detail text is not a string", function()
    if not readyForDiscord() then
      return
    end
    -- Only reachable with the API available: the availability gate is checked
    -- before any argument is, so Networking_spec.lua cannot get this far.
    assertArgError(function() setDiscordDetail({}) end, "setDiscordDetail: bad argument #1")
  end)

  it("truncates a detail text that overflows Discord's 128 byte field", function()
    if not readyForDiscord() then
      return
    end
    local overlong = string.rep("a", 200)
    local activity = activityFrom(function() setDiscordDetail(overlong) end,
                                  function(seen) return seen.details ~= nil end)
    -- The whole documented 128 bytes, not 127: the buffer holding this used to
    -- be exactly 128 bytes and lost its last byte to the null terminator
    -- (#9634).
    assert.equals(128, #activity.details)
    assert.equals(string.rep("a", 128), activity.details)
    -- Only what Discord is sent is truncated; Mudlet keeps the whole string.
    assert.equals(overlong, getDiscordDetail())
  end)

  it("cuts an overlong non-ASCII detail text between characters", function()
    if not readyForDiscord() then
      return
    end
    -- #9634: the cut used to be made at the byte limit with no regard for
    -- UTF-8, leaving the last character in the field as a lone lead byte. That
    -- does not merely damage one field - the payload stops being decodable, so
    -- the whole SET_ACTIVITY frame is discarded and every well-formed field in
    -- it goes with it.
    local mark = frameCount()
    -- 65 two-byte characters, 130 bytes: two more than the field holds, so the
    -- cut has to fall inside the 65th character.
    local activity = activityFrom(function() setDiscordDetail(string.rep("ä", 65)) end,
                                  function(seen) return seen.details ~= nil end)
    assert.equals(string.rep("ä", 64), activity.details)
    assert.equals(128, #activity.details)
    assert.equals(0, undecodableFramesAfter(mark))
  end)
end)

describe("setDiscordState", function()
  it("sends the state text to Discord", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordState("Level 50 Mage") end,
                                  function(seen) return seen.state == "Level 50 Mage" end)
    assert.equals("Level 50 Mage", activity.state)
  end)

  it("reports the state text it sent", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordState("In combat"))
    assert.equals("In combat", getDiscordState())
  end)

  it("omits the state field entirely when the state text is empty", function()
    if not readyForDiscord() then
      return
    end
    -- Empty fields are sent as JSON absences, not as "", so Discord hides the
    -- line rather than showing a blank one.
    local activity = activityFrom(function() setDiscordState("") end)
    assert.is_nil(activity.state)
  end)

  it("refuses a one character state text", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordState("holding"))
    local ok, message = setDiscordState("x")
    assert.is_nil(ok)
    assert.is_true(contains(message, "text of length 1 not allowed by Discord"))
    assert.equals("holding", getDiscordState())
  end)
end)

describe("setDiscordGame", function()
  it("sets both the detail text and the large icon from the game name", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordGame("WoTMUD") end,
                                  function(seen) return seen.details ~= nil and seen.assets.large_image == "wotmud" end)
    -- The detail text is tr("Playing %1"), so only the interpolated game name
    -- is the same on a localised Mudlet.
    assert.is_true(contains(activity.details, "WoTMUD"))
    assert.equals("wotmud", activity.assets.large_image)
  end)
end)

describe("setDiscordLargeIcon and setDiscordSmallIcon", function()
  it("lower-cases the large icon key on the way to Discord", function()
    if not readyForDiscord() then
      return
    end
    -- Discord asset keys are lower case, so Mudlet folds whatever it is given.
    local activity = activityFrom(function() setDiscordLargeIcon("Achaea") end,
                                  function(seen) return seen.assets.large_image ~= "mudlet" end)
    assert.equals("achaea", activity.assets.large_image)
    assert.equals("achaea", getDiscordLargeIcon())
  end)

  it("sends the large icon's tooltip text", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordLargeIconText("Achaea, Dreams of Divine Lands") end,
                                  function(seen) return seen.assets.large_text ~= nil end)
    assert.equals("Achaea, Dreams of Divine Lands", activity.assets.large_text)
    assert.equals("Achaea, Dreams of Divine Lands", getDiscordLargeIconText())
  end)

  it("lower-cases the small icon key on the way to Discord", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordSmallIcon("Shield") end,
                                  function(seen) return seen.assets.small_image ~= nil end)
    assert.equals("shield", activity.assets.small_image)
    assert.equals("shield", getDiscordSmallIcon())
  end)

  it("sends a full length icon key without dropping its last character", function()
    if not readyForDiscord() then
      return
    end
    -- #9634: a Discord asset key may be the full 32 bytes the API documents,
    -- but the buffer was 32 bytes including the terminator, so the last
    -- character was cut off and the icon never resolved.
    local key = string.rep("a", 32)
    local activity = activityFrom(function() setDiscordLargeIcon(key) end,
                                  function(seen) return seen.assets.large_image ~= "mudlet" end)
    assert.equals(32, #activity.assets.large_image)
    assert.equals(key, activity.assets.large_image)
  end)

  it("sends the small icon's tooltip text", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordSmallIconText("Guardian") end,
                                  function(seen) return seen.assets.small_text ~= nil end)
    assert.equals("Guardian", activity.assets.small_text)
    assert.equals("Guardian", getDiscordSmallIconText())
  end)

  it("refuses a one character large icon text", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordLargeIconText("unchanged"))
    local ok, message = setDiscordLargeIconText("x")
    assert.is_nil(ok)
    assert.is_true(contains(message, "text of length 1 not allowed by Discord"))
    assert.equals("unchanged", getDiscordLargeIconText())
  end)

  it("refuses a one character small icon text", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordSmallIconText("unchanged"))
    local ok, message = setDiscordSmallIconText("x")
    assert.is_nil(ok)
    assert.is_true(contains(message, "text of length 1 not allowed by Discord"))
    assert.equals("unchanged", getDiscordSmallIconText())
  end)
end)

describe("presence text containing percent sequences", function()
  -- The getters used to hand the stored text to lua_pushfstring() as its format
  -- string, so every '%' in it was read as a printf specifier: "Level %d Mage"
  -- came back with a garbage number where the %d was, and a "%s" dereferenced a
  -- pointer that had never been passed. Presence text can arrive from the game
  -- server over GMCP, so a status line with a stray percent sign in it was all
  -- it took. All six getters are covered below rather than a sample of them,
  -- so a seventh added the old way would be caught here too.
  it("reports a detail text containing %d unchanged", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordDetail("Level %d Mage"))
    assert.equals("Level %d Mage", getDiscordDetail())
  end)

  it("reports a state text containing %s unchanged", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordState("Wielding %s in the left hand"))
    assert.equals("Wielding %s in the left hand", getDiscordState())
  end)

  it("reports an icon tooltip containing percent signs unchanged", function()
    if not readyForDiscord() then
      return
    end
    -- A doubled "%%" is the sequence the format-string path did not garble but
    -- silently halved, and a bare "% " one it left alone - a caller could not
    -- have escaped its way around either.
    assert.is_true(setDiscordLargeIconText("100%% health, 50% mana"))
    assert.equals("100%% health, 50% mana", getDiscordLargeIconText())
  end)

  it("reports a detail text ending in a percent sign unchanged", function()
    if not readyForDiscord() then
      return
    end
    -- The worst shape of the old bug rather than another spelling of the first
    -- spec: on a trailing '%' the format-string path stepped one byte past the
    -- terminator and scanned on, which ASan reports as a heap buffer overflow.
    assert.is_true(setDiscordDetail("mana at 50%"))
    assert.equals("mana at 50%", getDiscordDetail())
  end)

  it("reports the icon keys and the small icon tooltip unchanged", function()
    if not readyForDiscord() then
      return
    end
    -- The remaining three of the six getters. Icon keys come back lower-cased
    -- because that is what Discord's asset names are, which is the only change
    -- to them anyone should see.
    assert.is_true(setDiscordLargeIcon("Level %d Mage"))
    assert.equals("level %d mage", getDiscordLargeIcon())
    assert.is_true(setDiscordSmallIcon("Shield %s"))
    assert.equals("shield %s", getDiscordSmallIcon())
    assert.is_true(setDiscordSmallIconText("100%% shielded, 50% rested"))
    assert.equals("100%% shielded, 50% rested", getDiscordSmallIconText())
  end)
end)

describe("setDiscordElapsedStartTime and setDiscordRemainingEndTime", function()
  it("sends an elapsed start time and no end time", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordElapsedStartTime(1700000000) end,
                                  function(seen) return seen.timestamps ~= nil end)
    assert.equals(1700000000, activity.timestamps.start)
    assert.is_nil(activity.timestamps["end"])
  end)

  it("replaces an elapsed start time with a remaining end time", function()
    if not readyForDiscord() then
      return
    end
    -- The two are mutually exclusive: Discord shows either "elapsed" or
    -- "remaining", so setting one has to clear the other.
    assert.is_true(setDiscordElapsedStartTime(1700000000))
    local activity = activityFrom(function() setDiscordRemainingEndTime(1900000000) end,
                                  function(seen) return seen.timestamps and seen.timestamps["end"] ~= nil end)
    assert.equals(1900000000, activity.timestamps["end"])
    assert.is_nil(activity.timestamps.start)
  end)

  it("reports the timestamps it sent", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordElapsedStartTime(1700000000))
    local startTime, endTime = getDiscordTimeStamps()
    assert.equals(1700000000, startTime)
    assert.equals(0, endTime)

    assert.is_true(setDiscordRemainingEndTime(1900000000))
    startTime, endTime = getDiscordTimeStamps()
    assert.equals(0, startTime)
    assert.equals(1900000000, endTime)
  end)

  it("drops the timestamps entirely when given zero", function()
    if not readyForDiscord() then
      return
    end
    -- Waited for, not just called: the spec below asserts on the first frame
    -- that follows, so this one's has to have landed already.
    activityFrom(function() setDiscordElapsedStartTime(1700000000) end,
                 function(seen) return seen.timestamps ~= nil end)
    local activity = activityFrom(function() setDiscordElapsedStartTime(0) end)
    assert.is_nil(activity.timestamps)
  end)

  it("refuses a negative elapsed start time", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordElapsedStartTime(1700000000))
    local ok, message = setDiscordElapsedStartTime(-1)
    assert.is_nil(ok)
    assert.is_true(contains(message, "the timestamp must be zero"))
    assert.equals(1700000000, (getDiscordTimeStamps()))
  end)

  it("refuses a negative remaining end time", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordRemainingEndTime(1900000000))
    local ok, message = setDiscordRemainingEndTime(-1)
    assert.is_nil(ok)
    assert.is_true(contains(message, "the timestamp must be zero"))
    assert.equals(1900000000, select(2, getDiscordTimeStamps()))
  end)
end)

describe("setDiscordParty", function()
  it("sends the party size and maximum", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordParty(2, 5) end,
                                  function(seen) return seen.party ~= nil end)
    assert.same({2, 5}, activity.party.size)
  end)

  it("raises the maximum to the size when only a size is given", function()
    if not readyForDiscord() then
      return
    end
    local activity = activityFrom(function() setDiscordParty(3) end,
                                  function(seen) return seen.party ~= nil end)
    assert.same({3, 3}, activity.party.size)
  end)

  it("keeps an established maximum when only a smaller size is given", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordParty(2, 5))
    local activity = activityFrom(function() setDiscordParty(1) end,
                                  function(seen) return seen.party and seen.party.size[1] == 1 end)
    assert.same({1, 5}, activity.party.size)
  end)

  it("removes the party from the presence when the maximum is zero", function()
    if not readyForDiscord() then
      return
    end
    activityFrom(function() setDiscordParty(2, 5) end, function(seen) return seen.party ~= nil end)
    local activity = activityFrom(function() setDiscordParty(0, 0) end)
    assert.is_nil(activity.party)
  end)

  it("reports the party it sent", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordParty(4, 8))
    local size, maximum = getDiscordParty()
    assert.equals(4, size)
    assert.equals(8, maximum)
  end)

  it("refuses a negative party size", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordParty(2, 5))
    local ok, message = setDiscordParty(-1)
    assert.is_nil(ok)
    assert.is_true(contains(message, "the current party size must be zero or more"))
    assert.equals(2, (getDiscordParty()))
  end)

  it("refuses a negative party maximum", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordParty(2, 5))
    local ok, message = setDiscordParty(3, -1)
    assert.is_nil(ok)
    assert.is_true(contains(message, "the optional party maximum size"))
    -- Both, so that a rejected call falling through to the size-only overload
    -- would be caught rather than looking unchanged.
    local size, maximum = getDiscordParty()
    assert.equals(2, size)
    assert.equals(5, maximum)
  end)

  it("raises a Lua error when the party size is not a number", function()
    if not readyForDiscord() then
      return
    end
    assertArgError(function() setDiscordParty("a few") end, "setDiscordParty: bad argument #1")
  end)
end)

describe("resetDiscordData", function()
  it("clears every presence field it had set", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordDetail("Exploring the forest"))
    assert.is_true(setDiscordState("Level 50 Mage"))
    assert.is_true(setDiscordLargeIcon("achaea"))
    assert.is_true(setDiscordLargeIconText("Achaea"))
    assert.is_true(setDiscordSmallIcon("shield"))
    assert.is_true(setDiscordSmallIconText("Guardian"))
    assert.is_true(setDiscordParty(2, 5))
    assert.is_true(setDiscordElapsedStartTime(1700000000))

    local activity = activityFrom(function() resetDiscordData() end,
                                  function(seen) return seen.details == nil end)
    assert.is_nil(activity.details)
    assert.is_nil(activity.state)
    assert.is_nil(activity.party)
    assert.is_nil(activity.timestamps)
    assert.is_nil(activity.assets.large_text)
    assert.is_nil(activity.assets.small_image)
    assert.is_nil(activity.assets.small_text)
    -- The Mudlet logo is not profile data, so it comes back with the reset
    -- presence rather than being cleared by it.
    assert.equals("mudlet", activity.assets.large_image)
  end)

  it("clears what the getters report", function()
    if not readyForDiscord() then
      return
    end
    assert.is_true(setDiscordDetail("Exploring the forest"))
    assert.is_true(setDiscordSmallIconText("Guardian"))
    assert.is_true(setDiscordParty(2, 5))
    assert.is_true(setDiscordElapsedStartTime(1700000000))

    assert.is_true(resetDiscordData())

    assert.equals("", getDiscordDetail())
    assert.equals("", getDiscordState())
    assert.equals("", getDiscordLargeIcon())
    assert.equals("", getDiscordSmallIconText())
    assert.equals(0, (getDiscordParty()))
    assert.equals(0, (getDiscordTimeStamps()))
  end)
end)

describe("setDiscordApplicationID", function()
  it("reconnects to Discord under the new application ID and back again", function()
    if not readyForDiscord() then
      return
    end
    -- Should an assertion below leave the other application in place, the
    -- following spec would open with a reconnect it does not expect.
    finally(function() setDiscordApplicationID() end)

    -- Both directions in one spec on purpose: each switch makes discord-rpc
    -- drop its socket and hand the new ID over in a fresh handshake, which
    -- costs about a second and a half of real reconnect time.
    local mark = frameCount()
    assert.is_true(setDiscordApplicationID(otherApplicationId))
    assert.is_false(usingMudletsDiscordID())
    -- The first handshake after the switch, rather than the whole list: a
    -- connection attempt that had to be retried would add another.
    assert.equals(otherApplicationId, waitForHandshakes(mark)[1])

    mark = frameCount()
    assert.is_true(setDiscordApplicationID())
    assert.is_true(usingMudletsDiscordID())
    assert.equals(mudletApplicationId, waitForHandshakes(mark)[1])
  end)

  it("treats an empty application ID as the request to go back to Mudlet's", function()
    if not readyForDiscord() then
      return
    end
    finally(function() setDiscordApplicationID() end)

    local mark = frameCount()
    assert.is_true(setDiscordApplicationID(otherApplicationId))
    assert.equals(otherApplicationId, waitForHandshakes(mark)[1])

    mark = frameCount()
    assert.is_true(setDiscordApplicationID(""))
    assert.is_true(usingMudletsDiscordID())
    assert.equals(mudletApplicationId, waitForHandshakes(mark)[1])
  end)

  it("refuses an application ID that is not a number", function()
    if not readyForDiscord() then
      return
    end
    local ok, message = setDiscordApplicationID("not-an-id")
    assert.is_nil(ok)
    assert.is_true(contains(message, "can not be converted to the expected numeric Discord application ID"))
    assert.is_true(usingMudletsDiscordID())
  end)
end)

describe("an icon key that has to be truncated", function()
  it("cuts a non-ASCII key between characters and keeps the frame decodable", function()
    if not readyForDiscord() then
      return
    end
    -- The 32 byte fields cut in the same place as the 128 byte ones, so they
    -- broke the frame in the same way (#9634). 16 two-byte characters are 32
    -- bytes, which now fits exactly; a 17th has to go, whole.
    local mark = frameCount()
    local activity = activityFrom(function() setDiscordLargeIcon(string.rep("é", 17)) end,
                                  function(seen) return seen.assets.large_image ~= "mudlet" end)
    assert.equals(string.rep("é", 16), activity.assets.large_image)
    assert.equals(32, #activity.assets.large_image)
    assert.equals(0, undecodableFramesAfter(mark))
  end)
end)
