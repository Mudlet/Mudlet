-- MSP is the protocol a game uses to ask for sound. Through MXP those requests
-- arrive as <SOUND> and <MUSIC> tags, which is the only way into the protocol
-- that does not need a connected game, so that is what these specs feed.
--
-- What a game starts is deliberately kept out of getPlayingSounds() and
-- getPlayingMusic() - those report what a script started - so the observable
-- here is the sysMediaStarted and sysMediaFinished events instead. Each spec
-- plays a file of its own name, so a sound another spec left behind can never
-- stand in for the one it is watching for.

describe("Tests the sound and music MSP asks for", function()
  local mediaDirectory = getMudletHomeDir() .. "/media"
  -- the target of a name that tries to climb out of the media directory
  local outsideFile = getMudletHomeDir() .. "/busted-msp-outside.wav"
  local writtenFiles = {}
  local playbackObserved

  local function littleEndian(value, byteCount)
    local bytes = {}
    for _ = 1, byteCount do
      bytes[#bytes + 1] = string.char(value % 256)
      value = math.floor(value / 256)
    end
    return table.concat(bytes)
  end

  -- 8 bit, 8kHz mono silence: every decoder accepts it, and generating it keeps
  -- a binary fixture out of the repository
  local function silentWav(milliseconds)
    local sampleRate = 8000
    local samples = string.rep(string.char(128), math.floor(sampleRate * milliseconds / 1000))
    local format = "fmt " .. littleEndian(16, 4) .. littleEndian(1, 2) .. littleEndian(1, 2)
      .. littleEndian(sampleRate, 4) .. littleEndian(sampleRate, 4) .. littleEndian(1, 2) .. littleEndian(8, 2)
    local data = "data" .. littleEndian(#samples, 4) .. samples
    local body = "WAVE" .. format .. data
    return "RIFF" .. littleEndian(#body, 4) .. body
  end

  local function writeWav(path, milliseconds)
    local handle = io.open(path, "wb")
    assert.is_not_nil(handle, "could not write the media fixture " .. path)
    handle:write(silentWav(milliseconds))
    handle:close()
  end

  local function fixture(stem, milliseconds)
    lfs.mkdir(mediaDirectory)
    local name = ("busted-msp-%s.wav"):format(stem)
    writeWav(mediaDirectory .. "/" .. name, milliseconds)
    writtenFiles[name] = true
    return name
  end

  -- ten seconds, so it is still playing when the spec looks at it
  local function hold(stem)
    return fixture(stem, 10000)
  end

  -- a fifth of a second, for the spec that waits for a loop to play out
  local function brief(stem)
    return fixture(stem, 150)
  end

  local function removeFixtures()
    for name in pairs(writtenFiles) do
      os.remove(mediaDirectory .. "/" .. name)
    end
    writtenFiles = {}
    os.remove(outsideFile)
  end

  local function feed(tag)
    feedTriggers(tag .. "\n")
  end

  local function pump(rounds)
    for _ = 1, (rounds or 3) do
      waitForEvent("sysMediaStarted", 400)
    end
  end

  -- Naming the file "Off" is how MSP asks for a stop, and it is the only way to
  -- stop what MSP started: stopSounds() and stopMusic() act on the players the
  -- Lua API keeps and leave a game's alone.
  local function stopEverything()
    feed('<SOUND FName="Off">')
    feed('<MUSIC FName="Off">')
    pump(2)
  end

  -- busted keeps one finally() per spec, so cleanups that have to stack go here
  -- and are drained in reverse afterwards.
  local cleanups = {}

  local function onCleanup(undo)
    cleanups[#cleanups + 1] = undo
  end

  local function collect(eventName, into)
    local handler = registerAnonymousEventHandler(eventName, function(_, file, path, mediaType, key, tag)
      into[#into + 1] = {file = file, path = path, mediaType = mediaType, key = key, tag = tag}
    end)
    onCleanup(function() killAnonymousEventHandler(handler) end)
  end

  -- A media event can be raised inside the call that caused it, so waiting for
  -- one has to start with a look at what is already there.
  local function waitForCount(eventName, collected, count)
    for _ = 1, 6 do
      if #collected >= count then
        return
      end
      waitForEvent(eventName, 1000)
    end
  end

  local function watchStarts()
    local started = {}
    collect("sysMediaStarted", started)
    return started
  end

  local function names(collected)
    local list = {}
    for index, entry in ipairs(collected) do
      list[index] = entry.file
    end
    return table.concat(list, ", ")
  end

  -- CI sets this so a missing playback fails there rather than passing as a
  -- green skip; a developer's machine without a media backend still passes.
  local requireMedia = os.getenv("MUDLET_TEST_REQUIRE_MEDIA")

  local function mediaPlaybackUnavailable()
    if playbackObserved == nil then
      local canary = brief("canary")
      feed(('<SOUND FName="%s">'):format(canary))
      playbackObserved = waitForEvent("sysMediaStarted", 5000) ~= nil
      stopEverything()
      if not playbackObserved then
        removeFixtures()
      end
    end
    if playbackObserved then
      return false
    end
    if requireMedia then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_MEDIA is set but a <SOUND> tag raised no sysMediaStarted event")
    end
    pending("Qt Multimedia did not start playback in this environment")
    return true
  end

  local mspWasEnabled

  setup(function()
    setConfig("specialForceMXPProcessorOn", true)
    mspWasEnabled = getConfig("enableMSP")
    setConfig("enableMSP", true)
  end)

  teardown(function()
    stopEverything()
    removeFixtures()
    setConfig("enableMSP", mspWasEnabled)
    setConfig("specialForceMXPProcessorOn", false)
  end)

  after_each(function()
    -- handlers go before the stops, or one still listening would count the
    -- sysMediaFinished the cleanup itself causes
    for index = #cleanups, 1, -1 do
      cleanups[index]()
    end
    cleanups = {}
    stopEverything()
  end)

  it("plays the file a SOUND tag names and reports it as a sound", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("sound")
    local started = watchStarts()

    feed(('<SOUND FName="%s">'):format(file))

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
    assert.equals("sound", started[1].mediaType)
  end)

  it("plays the file a MUSIC tag names as music", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("music")
    local started = watchStarts()

    feed(('<MUSIC FName="%s">'):format(file))

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
    assert.equals("music", started[1].mediaType)
  end)

  it("reports the type attribute as the media tag, lowercased", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("tagged")
    local started = watchStarts()

    feed(('<SOUND FName="%s" T="Weather">'):format(file))

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
    assert.equals("weather", started[1].tag)
  end)

  -- FName, V, L, P, T and U in that order, which is how a game that writes
  -- !!SOUND(door.wav 100 1 50 misc) spells the same request
  it("reads the attributes given by position rather than by name", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("positional")
    local started = watchStarts()

    feed(('<SOUND %s 100 1 50 Misc>'):format(file))

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
    assert.equals("misc", started[1].tag)
  end)

  it("matches a file name with a wildcard in it against the media directory", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("wildcard")
    local started = watchStarts()

    feed('<SOUND FName="busted-msp-wildc*.wav">')

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
  end)

  it("matches a single character wildcard as well", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("question")
    local started = watchStarts()

    feed('<SOUND FName="busted-msp-questio?.wav">')

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
  end)

  it("adds .wav to a sound whose name has no extension", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("noextension")
    local started = watchStarts()

    feed('<SOUND FName="busted-msp-noextension">')

    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))
    assert.equals(file, started[1].file)
  end)

  -- music without an extension is looked for as .mid, so the .wav of that name
  -- sitting in the media directory is not what it finds
  it("looks for .mid instead when the request is for music", function()
    if mediaPlaybackUnavailable() then
      return
    end
    hold("midonly")
    local started = watchStarts()

    feed('<MUSIC FName="busted-msp-midonly">')

    pump()
    assert.equals(0, #started, names(started))
  end)

  it("refuses a file name that climbs out of the media directory", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeWav(outsideFile, 10000)
    local started = watchStarts()

    feed('<SOUND FName="../busted-msp-outside.wav">')

    pump()
    assert.equals(0, #started, names(started))
  end)

  it("refuses a file name given as an absolute path", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeWav(outsideFile, 10000)
    local started = watchStarts()

    feed(('<SOUND FName="%s">'):format(outsideFile))

    pump()
    assert.equals(0, #started, names(started))
  end)

  it("does nothing for a tag that names no file at all", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local started = watchStarts()

    feed('<SOUND>')

    pump()
    assert.equals(0, #started, names(started))
  end)

  it("plays every pass of a finite loop count", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = brief("loops")
    local started = watchStarts()

    feed(('<SOUND FName="%s" L=2>'):format(file))

    waitForCount("sysMediaStarted", started, 2)
    assert.equals(2, #started, names(started))
    assert.equals(file, started[2].file)
  end)

  it("stops the sound it is playing when the file name is Off", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("offsound")
    local started = watchStarts()
    local finished = {}
    collect("sysMediaFinished", finished)

    feed(('<SOUND FName="%s" P=100>'):format(file))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    feed('<SOUND FName="Off">')

    waitForCount("sysMediaFinished", finished, 1)
    -- the file runs for ten seconds, so a finish this soon is the stop
    assert.equals(1, #finished, names(finished))
    assert.equals(file, finished[1].file)
  end)

  it("stops the music it is playing when the file name is Off", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("offmusic")
    local started = watchStarts()
    local finished = {}
    collect("sysMediaFinished", finished)

    feed(('<MUSIC FName="%s">'):format(file))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    feed('<MUSIC FName="Off">')

    waitForCount("sysMediaFinished", finished, 1)
    assert.equals(1, #finished, names(finished))
    assert.equals(file, finished[1].file)
  end)

  it("stops only what an Off names when it names a type", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("offtagged")
    local started = watchStarts()
    local finished = {}
    collect("sysMediaFinished", finished)

    feed(('<SOUND FName="%s" P=100 T="keep">'):format(file))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    feed('<SOUND FName="Off" T="other">')
    pump()
    assert.equals(0, #finished, names(finished))

    feed('<SOUND FName="Off" T="keep">')

    waitForCount("sysMediaFinished", finished, 1)
    assert.equals(1, #finished, names(finished))
    assert.equals(file, finished[1].file)
  end)

  -- MSP gives every sound request a priority, fifty when it names none, and a
  -- request has to beat what is already playing to be heard at all
  it("refuses a request that does not outrank what is already playing", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local playing = hold("ranking")
    local refused = brief("rankingrefused")
    local started = watchStarts()

    feed(('<SOUND FName="%s" P=75>'):format(playing))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    feed(('<SOUND FName="%s" P=75>'):format(refused))

    pump()
    assert.equals(1, #started, names(started))
  end)

  it("lets a request with a higher priority take over", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local playing = hold("takenover")
    local takingOver = brief("takingover")
    local started = watchStarts()
    local finished = {}
    collect("sysMediaFinished", finished)

    feed(('<SOUND FName="%s" P=50>'):format(playing))
    waitForCount("sysMediaStarted", started, 1)

    feed(('<SOUND FName="%s" P=51>'):format(takingOver))

    waitForCount("sysMediaStarted", started, 2)
    assert.equals(2, #started, names(started))
    assert.equals(takingOver, started[2].file)
    assert.equals(playing, finished[1] and finished[1].file, names(finished))
  end)

  -- brought down to the maximum rather than left to outrank a sound already
  -- playing at it
  it("brings a priority above the maximum down to it", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local playing = hold("clamping")
    local refused = brief("clampingrefused")
    local started = watchStarts()

    feed(('<SOUND FName="%s" P=100>'):format(playing))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    feed(('<SOUND FName="%s" P=200>'):format(refused))

    pump()
    assert.equals(1, #started, names(started))
  end)

  it("plays nothing while the profile has MSP turned off", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("disabled")
    local started = watchStarts()
    setConfig("enableMSP", false)
    onCleanup(function() setConfig("enableMSP", true) end)

    feed(('<SOUND FName="%s">'):format(file))

    pump()
    assert.equals(0, #started, names(started))
  end)

  -- Pinning today's behaviour rather than asserting it is right: the Lua
  -- queries filter on the API's own protocol, so a script cannot see what the
  -- game asked for at all.
  it("keeps what the game started out of the Lua queries", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local file = hold("queried")
    local started = watchStarts()

    feed(('<SOUND FName="%s">'):format(file))
    waitForCount("sysMediaStarted", started, 1)
    assert.equals(1, #started, names(started))

    assert.equals(0, #getPlayingSounds())
    assert.equals(0, #getPlayingMusic())
  end)
end)
