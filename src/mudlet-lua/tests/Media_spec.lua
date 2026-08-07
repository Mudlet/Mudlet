-- Specs for the media and text-to-speech Lua APIs.
--
-- Both families were previously homed in other domain spec files: the media
-- contracts in Networking_spec.lua and the text-to-speech ones in
-- Miscallaneous_spec.lua. They live here now so the audio side of the API has
-- one home.
--
-- The contract specs check what is deterministic without any backend at all:
-- argument validation and the nil+message / hard-error shapes. The effect specs
-- need a backend, so they play a WAV these specs generate into the profile's
-- media directory and drive Qt's mock speech engine, and they skip cleanly
-- where neither is available. Nothing here mocks a real API function.

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

  describe("pauseSounds", function()
    it("raises a Lua error when the single argument is not a table", function()
      assertArgError(function() pauseSounds(5) end, "pauseSounds: needs to be a table")
    end)
  end)

  describe("pauseMusic", function()
    it("raises a Lua error when the single argument is not a table", function()
      assertArgError(function() pauseMusic("all") end, "pauseMusic: needs to be a table")
    end)
  end)

  describe("stopSounds", function()
    it("returns true when stopping everything with no arguments", function()
      assert.is_true(stopSounds())
      assert.equals(0, #getPlayingSounds())
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

describe("Media playback effects with a generated sound file", function()
  -- The API media functions play files out of the profile's own media
  -- directory, so instead of shipping a binary fixture these specs write a
  -- short WAV there: 150ms of 8 bit, 8kHz mono silence, which every decoder
  -- accepts and which keeps a play-to-finish round trip under a fifth of a
  -- second.
  --
  -- Playback needs Qt Multimedia to actually run a player. It does so without
  -- any audio device - verified locally with PulseAudio and ALSA made
  -- unreachable, where the ffmpeg backend still drives the player from start
  -- to finish - which is the situation on CI. Should an environment turn up
  -- where it cannot, the canary in mediaPlaybackUnavailable() pends these
  -- specs instead of failing them.
  -- Two files: a short one for the spec that waits for playback to end on its
  -- own, and a long one for the specs that have to still be playing when they
  -- stop or pause it, so a slow runner cannot turn a natural finish into a
  -- spurious failure.
  -- A second long file so a spec can tell one playback from another by name.
  local soundFile = "busted-media-tone.wav"
  local longSoundFile = "busted-media-hold.wav"
  local otherLongSoundFile = "busted-media-hold-other.wav"
  local mediaDirectory = getMudletHomeDir() .. "/media"
  local playbackObserved

  local function littleEndian(value, byteCount)
    local bytes = {}
    for _ = 1, byteCount do
      bytes[#bytes + 1] = string.char(value % 256)
      value = math.floor(value / 256)
    end
    return table.concat(bytes)
  end

  local function silentWav(milliseconds)
    local sampleRate = 8000
    -- 128 is silence for unsigned 8 bit samples
    local samples = string.rep(string.char(128), math.floor(sampleRate * milliseconds / 1000))
    local format = "fmt " .. littleEndian(16, 4) .. littleEndian(1, 2) .. littleEndian(1, 2)
      .. littleEndian(sampleRate, 4) .. littleEndian(sampleRate, 4) .. littleEndian(1, 2) .. littleEndian(8, 2)
    local data = "data" .. littleEndian(#samples, 4) .. samples
    local body = "WAVE" .. format .. data
    return "RIFF" .. littleEndian(#body, 4) .. body
  end

  local function writeMediaFile(name, milliseconds)
    lfs.mkdir(mediaDirectory)
    local handle = io.open(mediaDirectory .. "/" .. name, "wb")
    assert.is_not_nil(handle, "could not write the media fixture " .. name)
    handle:write(silentWav(milliseconds))
    handle:close()
  end

  local function writeSoundFiles()
    writeMediaFile(soundFile, 150)
    writeMediaFile(longSoundFile, 10000)
    writeMediaFile(otherLongSoundFile, 10000)
  end

  -- purgeMediaCache() empties the whole media directory, not just the fixtures
  -- these specs wrote, and the self-test profile persists between runs on a
  -- developer's machine. Anything else already in there is moved aside for the
  -- duration of the spec and put back afterwards.
  local function preserveMediaDirectory()
    local stash = getMudletHomeDir() .. "/busted-media-stash"
    local preserved = {}
    for entry in lfs.dir(mediaDirectory) do
      if entry ~= "." and entry ~= ".." and entry ~= soundFile and entry ~= longSoundFile and entry ~= otherLongSoundFile then
        preserved[#preserved + 1] = entry
      end
    end
    if #preserved == 0 then
      return
    end
    lfs.mkdir(stash)
    for _, entry in ipairs(preserved) do
      os.rename(mediaDirectory .. "/" .. entry, stash .. "/" .. entry)
    end
    finally(function()
      lfs.mkdir(mediaDirectory)
      for _, entry in ipairs(preserved) do
        os.rename(stash .. "/" .. entry, mediaDirectory .. "/" .. entry)
      end
      lfs.rmdir(stash)
    end)
  end

  -- Collects every occurrence of a media event for the duration of one spec.
  -- stopSounds() and pauseSounds() change the player's state inside the call
  -- itself, so the matching event is raised before a waitForEvent() could be
  -- armed; a handler sees those as well as the asynchronous ones.
  local function collect(eventName, into)
    local handler = registerAnonymousEventHandler(eventName, function(_, file, path, mediaType, key, tag)
      into[#into + 1] = {file = file, path = path, mediaType = mediaType, key = key, tag = tag}
    end)
    finally(function() killAnonymousEventHandler(handler) end)
  end

  -- Waits until collected holds count entries. A media event can be raised
  -- inside the call that caused it, so waiting has to start with a look.
  local function waitForCount(eventName, collected, count)
    for _ = 1, 5 do
      if #collected >= count then
        return
      end
      waitForEvent(eventName, 1000)
    end
  end

  -- The media events carry QUrl::path(), which puts a slash in front of a
  -- drive-lettered Windows path ("/C:/..."). Take that back off so one
  -- expected value works on every platform.
  local function eventPath(path)
    return (tostring(path):gsub("^/(%a:/)", "%1"))
  end

  -- CI sets this so a missing playback turns into a failure there rather than
  -- into a green skip; a developer's machine without a media backend still
  -- passes.
  local requireMedia = os.getenv("MUDLET_TEST_REQUIRE_MEDIA")

  -- Returns true when the caller must stop because this environment has no
  -- working media backend at all. Runs one throwaway playback to find out, and
  -- takes its fixtures back out of the profile when there is no point keeping
  -- them.
  local function mediaPlaybackUnavailable()
    if playbackObserved == nil then
      writeSoundFiles()
      playSoundFile({name = soundFile, key = "busted-media-canary"})
      playbackObserved = waitForEvent("sysMediaStarted", 5000) ~= nil
      stopSounds()
      if not playbackObserved then
        os.remove(mediaDirectory .. "/" .. soundFile)
        os.remove(mediaDirectory .. "/" .. longSoundFile)
        os.remove(mediaDirectory .. "/" .. otherLongSoundFile)
      end
    end
    if playbackObserved then
      return false
    end
    if requireMedia then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_MEDIA is set but playing a sound raised no sysMediaStarted event")
    end
    pending("Qt Multimedia did not start playback in this environment")
    return true
  end

  after_each(function()
    stopSounds()
    stopMusic()
  end)

  it("playSoundFile plays the file and reports it from start to finish", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    assert.is_true(playSoundFile({name = soundFile, key = "busted-key", tag = "busted-tag"}))

    local event, file, path, mediaType, key, tag = waitForEvent("sysMediaStarted", 5000)
    assert.equals("sysMediaStarted", event)
    assert.equals(soundFile, file)
    assert.equals(mediaDirectory .. "/" .. soundFile, eventPath(path))
    assert.equals("sound", mediaType)
    assert.equals("busted-key", key)
    assert.equals("busted-tag", tag)

    local finishedEvent, finishedFile, _, finishedType, finishedKey = waitForEvent("sysMediaFinished", 5000)
    assert.equals("sysMediaFinished", finishedEvent)
    assert.equals(soundFile, finishedFile)
    assert.equals("sound", finishedType)
    assert.equals("busted-key", finishedKey)
    assert.equals(0, #getPlayingSounds())
  end)

  it("playSoundFile plays a file given in the ordered argument form", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    -- name[,volume]: the ordered form has its own parser, and it is the form
    -- most scripts use
    assert.is_true(playSoundFile(longSoundFile, 80))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    local playing = getPlayingSounds()
    assert.equals(1, #playing)
    assert.equals(longSoundFile, playing[1].name)
    assert.equals(80, playing[1].volume)
  end)

  it("getPlayingSounds lists the sound that is playing", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-listed", tag = "busted-listed-tag"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    local playing = getPlayingSounds()
    assert.equals(1, #playing)
    assert.equals(longSoundFile, playing[1].name)
    assert.equals("busted-listed", playing[1].key)
    assert.equals("busted-listed-tag", playing[1].tag)
    assert.is_number(playing[1].volume)
  end)

  it("stopSounds stops the sound and reports it as finished", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local finished = {}
    collect("sysMediaFinished", finished)

    writeSoundFiles()
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-stopped"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))
    assert.equals(1, #getPlayingSounds())

    assert.is_true(stopSounds())
    if #finished == 0 then
      waitForEvent("sysMediaFinished", 5000)
    end
    -- the file runs for ten seconds, so a finish reported this soon after the
    -- start is the stop taking effect rather than the file running out
    assert.equals(1, #finished)
    assert.equals("busted-stopped", finished[1].key)
    assert.equals(0, #getPlayingSounds())
  end)

  it("pauseSounds parks the sound and playing it again resumes it", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local paused = {}
    collect("sysMediaPaused", paused)

    writeSoundFiles()
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-paused"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    assert.is_true(pauseSounds())
    if #paused == 0 then
      -- the backend here reports the pause inside the call; wait in case
      -- another one reports it a turn later
      waitForEvent("sysMediaPaused", 5000)
    end
    assert.equals(1, #paused)
    assert.equals("busted-paused", paused[1].key)
    assert.equals(0, #getPlayingSounds())
    local pausedSounds = getPausedSounds()
    assert.equals(1, #pausedSounds)
    assert.equals(longSoundFile, pausedSounds[1].name)

    -- playing the same file again resumes the paused player rather than
    -- starting a second one
    playSoundFile({name = longSoundFile, key = "busted-paused"})
    assert.equals(1, #getPlayingSounds())
    assert.equals(0, #getPausedSounds())
  end)

  it("playing a different sound while one is paused ends the paused one and starts the new one", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local finished, started = {}, {}
    collect("sysMediaFinished", finished)
    collect("sysMediaStarted", started)

    writeSoundFiles()
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-parked", tag = "busted-parked-tag"}))
    waitForCount("sysMediaStarted", started, 1)
    assert.is_true(pauseSounds())
    assert.equals(1, #getPausedSounds())

    assert.is_true(playSoundFile({name = otherLongSoundFile, key = "busted-replacement"}))

    assert.equals(1, #finished)
    assert.equals(longSoundFile, finished[1].file)
    assert.equals("busted-parked", finished[1].key)
    assert.equals("busted-parked-tag", finished[1].tag)

    waitForCount("sysMediaStarted", started, 2)
    assert.equals(2, #started)
    assert.equals(otherLongSoundFile, started[2].file)
    local playing = getPlayingSounds()
    assert.equals(1, #playing)
    assert.equals(otherLongSoundFile, playing[1].name)
    assert.equals(0, #getPausedSounds())
  end)

  it("playing different music while some is paused ends the paused track", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local finished, started = {}, {}
    collect("sysMediaFinished", finished)
    collect("sysMediaStarted", started)

    writeSoundFiles()
    assert.is_true(playMusicFile({name = longSoundFile, key = "busted-music-parked", tag = "busted-music-parked-tag"}))
    waitForCount("sysMediaStarted", started, 1)
    assert.is_true(pauseMusic())
    assert.equals(1, #getPausedMusic())

    assert.is_true(playMusicFile({name = otherLongSoundFile, key = "busted-music-new"}))

    assert.equals(1, #finished)
    assert.equals(longSoundFile, finished[1].file)
    assert.equals("busted-music-parked", finished[1].key)
    assert.equals("busted-music-parked-tag", finished[1].tag)

    waitForCount("sysMediaStarted", started, 2)
    local music = getPlayingMusic()
    assert.equals(1, #music)
    assert.equals(otherLongSoundFile, music[1].name)
    assert.equals(0, #getPausedMusic())
  end)

  it("a request refused on priority leaves the paused sound it would have taken over", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local finished, started = {}, {}
    collect("sysMediaFinished", finished)
    collect("sysMediaStarted", started)

    writeSoundFiles()
    -- first, because a priority of its own would stop every sound that has none
    assert.is_true(playSoundFile({name = otherLongSoundFile, key = "busted-priority-loud", priority = 90}))
    waitForCount("sysMediaStarted", started, 1)
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-priority-parked"}))
    waitForCount("sysMediaStarted", started, 2)
    assert.is_true(pauseSounds({key = "busted-priority-parked"}))
    assert.equals(1, #getPausedSounds())
    assert.equals(1, #getPlayingSounds())

    -- refused, since the sound already playing is louder - and a paused player
    -- is what a request is handed before anything else in the pool
    assert.is_true(playSoundFile({name = soundFile, key = "busted-priority-refused", priority = 10}))

    assert.equals(0, #getPlayingSounds({key = "busted-priority-refused"}))
    assert.equals(0, #finished)
    local paused = getPausedSounds()
    assert.equals(1, #paused)
    assert.equals(longSoundFile, paused[1].name)
    assert.equals("busted-priority-parked", paused[1].key)
  end)

  it("a finite loop count plays every pass and reports each one", function()
    if mediaPlaybackUnavailable() then
      return
    end
    local finished, started = {}, {}
    collect("sysMediaFinished", finished)
    collect("sysMediaStarted", started)

    writeSoundFiles()
    assert.is_true(playSoundFile({name = soundFile, key = "busted-looped", tag = "busted-looped-tag", loops = 2}))
    waitForCount("sysMediaFinished", finished, 2)

    assert.equals(2, #started)
    assert.equals(2, #finished)
    for _, pass in ipairs(finished) do
      assert.equals(soundFile, pass.file)
      assert.equals("busted-looped", pass.key)
      assert.equals("busted-looped-tag", pass.tag)
    end
    assert.equals(0, #getPlayingSounds())
  end)

  it("a sysMediaFinished handler that starts a sound leaves the caller's own request playing", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()

    -- A player joins the pool only once the play() that made it has returned,
    -- so warming several up - each started while the one before it is playing -
    -- is what puts the re-entrant call in reach of the one being set up below.
    local warmed = {}
    collect("sysMediaStarted", warmed)
    for index, key in ipairs({"busted-warm-one", "busted-warm-two", "busted-warm-three"}) do
      assert.is_true(playSoundFile({name = longSoundFile, key = key}))
      waitForCount("sysMediaStarted", warmed, index)
    end
    assert.equals(3, #getPlayingSounds())
    assert.is_true(stopSounds())

    local reentered = 0
    local handler = registerAnonymousEventHandler("sysMediaFinished", function()
      reentered = reentered + 1
      playSoundFile({name = otherLongSoundFile, key = "busted-handler-sound"})
    end)
    finally(function() killAnonymousEventHandler(handler) end)

    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-quiet", priority = 10}))
    -- stops the sound above while it is still loading, which raises
    -- sysMediaFinished into the handler from inside this very call
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-loud", priority = 90, loops = 3}))

    assert.is_true(reentered > 0, "the priority stop raised no sysMediaFinished, so nothing re-entered")
    local loud = getPlayingSounds({key = "busted-loud"})
    assert.equals(1, #loud)
    assert.equals(longSoundFile, loud[1].name)
    assert.is_true(#getPlayingSounds({key = "busted-handler-sound"}) > 0)
  end)

  it("the key filter picks out which sound is listed and stopped", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-filter"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    assert.equals(1, #getPlayingSounds({key = "busted-filter"}))
    assert.equals(0, #getPlayingSounds({key = "busted-other-key"}))

    -- a stop aimed at another key leaves this sound alone
    assert.is_true(stopSounds({key = "busted-other-key"}))
    assert.equals(1, #getPlayingSounds())

    assert.is_true(stopSounds({key = "busted-filter"}))
    assert.equals(0, #getPlayingSounds())
  end)

  it("playMusicFile reports the music type and stopMusic ends it", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    assert.is_true(playMusicFile({name = longSoundFile, key = "busted-music"}))

    local event, file, _, mediaType, key = waitForEvent("sysMediaStarted", 5000)
    assert.equals("sysMediaStarted", event)
    assert.equals(longSoundFile, file)
    assert.equals("music", mediaType)
    assert.equals("busted-music", key)

    local music = getPlayingMusic()
    assert.equals(1, #music)
    assert.equals(longSoundFile, music[1].name)
    assert.equals("busted-music", music[1].key)
    -- sounds and music are tracked separately
    assert.equals(0, #getPlayingSounds())

    assert.is_true(stopMusic())
    assert.equals(0, #getPlayingMusic())
  end)

  it("pauseMusic parks the music and playing it again resumes it", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    assert.is_true(playMusicFile({name = longSoundFile, key = "busted-music-paused"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    assert.is_true(pauseMusic())
    assert.equals(0, #getPlayingMusic())
    assert.equals(1, #getPausedMusic())
    assert.equals(longSoundFile, getPausedMusic()[1].name)

    playMusicFile({name = longSoundFile, key = "busted-music-paused"})
    assert.equals(1, #getPlayingMusic())
    assert.equals(0, #getPausedMusic())
  end)

  it("playSoundFile starts nothing for a file the media directory does not have", function()
    if mediaPlaybackUnavailable() then
      return
    end
    -- the return value only says the request was understood; with no file and
    -- no download url configured there is nothing to play
    assert.is_true(playSoundFile("busted-media-absent.wav"))
    assert.equals(0, #getPlayingSounds())
  end)

  it("purgeMediaCache empties the profile's media directory", function()
    if mediaPlaybackUnavailable() then
      return
    end
    writeSoundFiles()
    preserveMediaDirectory()
    local soundPath = mediaDirectory .. "/" .. soundFile
    assert.is_not_nil(lfs.attributes(soundPath, "mode"))
    assert.is_true(playSoundFile({name = longSoundFile, key = "busted-purged"}))
    assert.equals("sysMediaStarted", (waitForEvent("sysMediaStarted", 5000)))

    assert.is_true(purgeMediaCache())
    -- it stops every player before removing the directory
    assert.equals(0, #getPlayingSounds())
    assert.is_nil(lfs.attributes(soundPath, "mode"))
  end)

  it("purgeMediaCache returns nil and a message when it cannot empty the directory", function()
    if getOS() == "windows" then
      pending("staging an undeletable file needs chmod")
      return
    end
    writeSoundFiles()
    preserveMediaDirectory()

    local lockedDirectory = mediaDirectory .. "/busted-media-locked"
    lfs.mkdir(lockedDirectory)
    local pinnedFile = lockedDirectory .. "/busted-media-pinned.wav"
    local handle = io.open(pinnedFile, "wb")
    assert.is_not_nil(handle, "could not write the pinned media fixture")
    handle:write("pinned")
    handle:close()
    os.execute("chmod 500 '" .. lockedDirectory .. "'")
    finally(function()
      os.execute("chmod 700 '" .. lockedDirectory .. "'")
      os.remove(pinnedFile)
      lfs.rmdir(lockedDirectory)
    end)

    if os.remove(pinnedFile) then
      pending("this user can delete files out of a directory it cannot write")
      return
    end

    local ok, err = purgeMediaCache()
    assert.is_nil(ok)
    assert.is_true(contains(err, mediaDirectory), tostring(err))
    -- a purge that half happened, not one that did not happen
    assert.is_nil(lfs.attributes(mediaDirectory .. "/" .. soundFile, "mode"))
  end)

  it("a media url that is not http(s) reports a download error", function()
    local errors = {}
    local handler = registerAnonymousEventHandler("sysDownloadError", function(_, message, path)
      errors[#errors + 1] = {message = message, path = path}
    end)
    finally(function() killAnonymousEventHandler(handler) end)

    -- a file the media directory does not have, so the url is the only way to get it
    assert.is_true(playSoundFile({name = "busted-media-absent-scheme.wav", url = "ftp://example.invalid/sounds"}))
    waitForCount("sysDownloadError", errors, 1)
    assert.equals(1, #errors)
    assert.is_true(contains(errors[1].message, "http"), tostring(errors[1].message))
    assert.is_true(contains(errors[1].path, "busted-media-absent-scheme.wav"), tostring(errors[1].path))
  end)
end)

describe("receiveMSP reports MSP is not enabled while offline", function()
  it("returns nil and a message when MSP has not been negotiated", function()
    local ok, err = receiveMSP("!!SOUND(x.wav)")
    assert.is_nil(ok)
    assert.is_true(contains(err, "MSP is not currently enabled"))
  end)
end)

describe("Tests the text-to-speech Lua API", function()
    describe("Tests the text-to-speech family", function()
      -- Mudlet can be compiled without TTS at all, in which case Other.lua
      -- installs no-op shims that return nil instead of the real functions.
      -- ttsGetQueue() returning a table is the cheapest proof that the real
      -- ones are in place.
      local function ttsSupported()
        return type(ttsGetQueue()) == "table"
      end

      -- Returns true when the caller should stop because this build has no TTS
      -- functions to test.
      local function ttsUnsupported()
        if ttsSupported() then
          return false
        end
        pending("Mudlet was compiled without TTS support")
        return true
      end

      -- ttsBuild() selects Qt's deterministic mock engine under
      -- MUDLET_TEST_MODE, so the effect specs below never drive a developer's
      -- real speech engine and nothing is ever spoken out loud. Where the mock
      -- plugin is absent Qt leaves the engine with no voices, and those specs
      -- skip so a local run still passes; CI sets MUDLET_TEST_REQUIRE_TTS_MOCK
      -- to turn that skip into a failure, so a broken mock selection cannot
      -- hide behind a green skip.
      local testMode = os.getenv("MUDLET_TEST_MODE")
      local requireMock = os.getenv("MUDLET_TEST_REQUIRE_TTS_MOCK")

      local function mockEngineReady()
        return testMode and ttsSupported() and #ttsGetVoices() > 0
      end

      local function noMockEngine()
        if mockEngineReady() then
          return false
        end
        if requireMock then
          assert.is_true(false, "MUDLET_TEST_REQUIRE_TTS_MOCK is set but the mock TTS engine has no voices - it was not selected")
        end
        pending("mock TTS engine unavailable (run with MUDLET_TEST_MODE and Qt's mock plugin)")
        return true
      end

      -- Switching voice needs a second voice to switch to. Gated like
      -- noMockEngine() so a mock engine that stopped offering two voices cannot
      -- quietly turn the voice-switching specs green by pending them.
      local function tooFewVoices()
        if #ttsGetVoices() >= 2 then
          return false
        end
        if requireMock then
          assert.is_true(false, "MUDLET_TEST_REQUIRE_TTS_MOCK is set but the mock TTS engine offers fewer than two voices")
        end
        pending("the mock engine offers only one voice in this environment")
        return true
      end

      -- Collects every occurrence of an event for the duration of one spec.
      -- The mock engine changes state inside the ttsSpeak()/ttsSkip() call
      -- itself, so the matching event is raised before a waitForEvent() could
      -- be armed; a handler sees those as well as the asynchronous ones.
      local function collect(eventName, into)
        local handler = registerAnonymousEventHandler(eventName, function(_, first)
          into[#into + 1] = first == nil and true or first
        end)
        finally(function() killAnonymousEventHandler(handler) end)
      end

      -- The mock engine speaks in real time at roughly a tenth of a second per
      -- word, so every utterance in these specs is deliberately short.
      after_each(function()
        if ttsSupported() then
          -- clear first: skipping while the queue still holds a line starts
          -- speaking that line, which would run on into the next spec
          ttsClearQueue()
          ttsSkip()
        end
      end)

      it("ttsSpeak rejects whitespace-only text", function()
        if ttsUnsupported() then
          return
        end
        local ok, err = ttsSpeak("   ")
        assert.is_nil(ok)
        assert.is_true(err:find("skipped empty text to speak (TTS)", 1, true) ~= nil)
      end)

      it("ttsQueue rejects whitespace-only text", function()
        if ttsUnsupported() then
          return
        end
        local ok, err = ttsQueue("\t \n")
        assert.is_nil(ok)
        assert.is_true(err:find("skipped empty text to speak (TTS)", 1, true) ~= nil)
      end)

      it("ttsSpeak and ttsQueue raise a Lua error for a non-string argument", function()
        if ttsUnsupported() then
          return
        end
        assert.has_error(function() ttsSpeak({}) end)
        assert.has_error(function() ttsQueue({}) end)
      end)

      it("the rate, pitch and volume setters raise a Lua error for a non-number", function()
        if ttsUnsupported() then
          return
        end
        assert.has_error(function() ttsSetRate("fast") end)
        assert.has_error(function() ttsSetPitch({}) end)
        assert.has_error(function() ttsSetVolume(false) end)
      end)

      it("ttsGetQueue returns a table and false for out-of-range indexes", function()
        if ttsUnsupported() then
          return
        end
        ttsClearQueue()
        assert.is_table(ttsGetQueue())
        -- Regression #9471: on an empty queue index 1 is exactly one past the
        -- end (index == size), which used to pass the bounds check and read
        -- out of range.
        assert.is_false(ttsGetQueue(1))
        assert.is_false(ttsGetQueue(0))
      end)

      it("ttsClearQueue reports an out-of-range index instead of removing anything", function()
        if ttsUnsupported() then
          return
        end
        ttsClearQueue()
        local ok, err = ttsClearQueue(3)
        assert.is_nil(ok)
        assert.equals("index 3 out of bounds for queue size 0", err)
      end)

      it("ttsGetState reports one of the documented states", function()
        if ttsUnsupported() then
          return
        end
        -- ttsUnknownState is deliberately not accepted: it is the fallback the
        -- state switch prints for a state it does not know about, so allowing
        -- it here would make this assertion impossible to fail.
        local states = {
          ttsSpeechReady = true, ttsSpeechPaused = true,
          ttsSpeechStarted = true, ttsSpeechError = true,
        }
        assert.is_true(states[ttsGetState()] == true, ttsGetState())
      end)

      it("ttsGetRate, ttsGetPitch and ttsGetVolume return numbers", function()
        if ttsUnsupported() then
          return
        end
        assert.is_number(ttsGetRate())
        assert.is_number(ttsGetPitch())
        assert.is_number(ttsGetVolume())
      end)

      it("the voice setters return false for a voice that does not exist", function()
        if ttsUnsupported() then
          return
        end
        assert.is_false(ttsSetVoiceByIndex(0))
        assert.is_false(ttsSetVoiceByIndex(9999))
        assert.is_false(ttsSetVoiceByName("no such voice is installed"))
      end)

      it("the voice setters raise a Lua error for a wrongly typed argument", function()
        if ttsUnsupported() then
          return
        end
        assert.has_error(function() ttsSetVoiceByIndex("first") end)
        assert.has_error(function() ttsSetVoiceByName({}) end)
      end)

      it("ttsGetVoices lists the mock engine's voices and ttsGetCurrentVoice names one of them", function()
        if noMockEngine() then
          return
        end
        local voices = ttsGetVoices()
        assert.is_true(#voices > 0)
        local current = ttsGetCurrentVoice()
        assert.is_string(current)
        assert.is_true(table.contains(voices, current), current)
        for _, name in ipairs(voices) do
          assert.is_string(name)
        end
      end)

      it("ttsSpeak speaks the text and reports it until the engine goes ready again", function()
        if noMockEngine() then
          return
        end
        local started, ready = {}, {}
        collect("ttsSpeechStarted", started)
        collect("ttsSpeechReady", ready)

        ttsSpeak("Mudlet spec one")
        assert.equals("ttsSpeechStarted", ttsGetState())
        assert.equals("Mudlet spec one", ttsGetCurrentLine())
        -- the first utterance of a session used to report an empty text here,
        -- see the ttsSpeechStarted spec below
        assert.same({"Mudlet spec one"}, started)

        assert.equals("ttsSpeechReady", (waitForEvent("ttsSpeechReady", 5000)))
        assert.equals("ttsSpeechReady", ttsGetState())
        assert.equals(1, #ready)
        -- with nothing being spoken the line is no longer reported
        local line, err = ttsGetCurrentLine()
        assert.is_nil(line)
        assert.is_true(err:find("not speaking any text", 1, true) ~= nil)
      end)

      it("ttsSpeak drops angle brackets from the text it speaks", function()
        if noMockEngine() then
          return
        end
        -- discussion: https://github.com/Mudlet/Mudlet/issues/4689
        ttsSpeak("<b>bold</b>")
        assert.equals("bbold/b", ttsGetCurrentLine())
      end)

      it("ttsPause holds the utterance and ttsResume runs it to the end", function()
        if noMockEngine() then
          return
        end
        local paused = {}
        collect("ttsSpeechPaused", paused)

        ttsSpeak("pause this line")
        assert.equals("ttsSpeechStarted", ttsGetState())
        ttsPause()
        -- the engine reports the pause asynchronously
        assert.equals("ttsSpeechPaused", (waitForEvent("ttsSpeechPaused", 5000)))
        assert.equals("ttsSpeechPaused", ttsGetState())
        assert.equals(1, #paused)
        -- the paused utterance is still the current one
        assert.equals("pause this line", ttsGetCurrentLine())

        ttsResume()
        assert.equals("ttsSpeechStarted", ttsGetState())
        assert.equals("ttsSpeechReady", (waitForEvent("ttsSpeechReady", 5000)))
        assert.equals("ttsSpeechReady", ttsGetState())
      end)

      it("ttsSkip ends the current utterance immediately", function()
        if noMockEngine() then
          return
        end
        local ready = {}
        collect("ttsSpeechReady", ready)

        ttsSpeak("a long enough sentence that it cannot possibly finish on its own by now")
        assert.equals("ttsSpeechStarted", ttsGetState())
        ttsSkip()
        -- the utterance would take over a second to speak, so a ready state
        -- straight after the call can only be the skip taking effect
        assert.equals("ttsSpeechReady", ttsGetState())
        assert.equals(1, #ready)
      end)

      it("ttsQueue holds lines while the engine is busy and ttsGetQueue reads them back", function()
        if noMockEngine() then
          return
        end
        local queued = {}
        collect("ttsSpeechQueued", queued)

        ttsClearQueue()
        ttsSpeak("occupying the engine with a line that takes a while to speak")
        ttsQueue("queued one")
        ttsQueue("queued two")
        assert.equals(2, #ttsGetQueue())
        assert.equals("queued one", ttsGetQueue(1))
        assert.equals("queued two", ttsGetQueue(2))
        assert.equals(2, #queued)
        assert.equals("queued one", queued[1])
        assert.equals("queued two", queued[2])

        -- an explicit index inserts rather than appends
        ttsQueue("queued zero", 1)
        assert.same({"queued zero", "queued one", "queued two"}, ttsGetQueue())

        ttsClearQueue(1)
        assert.same({"queued one", "queued two"}, ttsGetQueue())
        ttsClearQueue()
        assert.equals(0, #ttsGetQueue())
      end)

      it("ttsQueue speaks straight away when the engine is idle", function()
        if noMockEngine() then
          return
        end
        ttsClearQueue()
        assert.equals("ttsSpeechReady", ttsGetState())

        ttsQueue("queued while idle")
        -- nothing is waiting, so the line is taken back off the queue and
        -- spoken instead of being held
        assert.equals(0, #ttsGetQueue())
        assert.equals("ttsSpeechStarted", ttsGetState())
        assert.equals("queued while idle", ttsGetCurrentLine())
      end)

      it("ttsQueue drops angle brackets like ttsSpeak does", function()
        if noMockEngine() then
          return
        end
        ttsClearQueue()
        ttsSpeak("occupying the engine with a line that takes a while to speak")
        ttsQueue("<i>queued</i>")
        assert.same({"iqueued/i"}, ttsGetQueue())
      end)

      it("ttsQueue clamps an index outside the queue instead of failing", function()
        if noMockEngine() then
          return
        end
        ttsClearQueue()
        ttsSpeak("occupying the engine with a line that takes a while to speak")
        ttsQueue("middle")
        ttsQueue("beyond the end", 99)
        ttsQueue("before the start", -5)
        assert.same({"before the start", "middle", "beyond the end"}, ttsGetQueue())
      end)

      it("ttsSkip moves on to the next queued line", function()
        if noMockEngine() then
          return
        end
        ttsClearQueue()
        ttsSpeak("occupying the engine with a line that takes a while to speak")
        ttsQueue("the line after the skip")
        assert.equals(1, #ttsGetQueue())

        ttsSkip()
        assert.equals(0, #ttsGetQueue())
        assert.equals("the line after the skip", ttsGetCurrentLine())
      end)

      it("a queued line starts speaking when the current one ends", function()
        if noMockEngine() then
          return
        end
        ttsClearQueue()
        ttsSpeak("first line")
        ttsQueue("second line")
        assert.equals(1, #ttsGetQueue())

        assert.equals("ttsSpeechReady", (waitForEvent("ttsSpeechReady", 5000)))
        -- the queue is drained by the state change that ended the first line
        assert.equals(0, #ttsGetQueue())
        assert.equals("second line", ttsGetCurrentLine())
      end)

      it("ttsSetRate, ttsSetPitch and ttsSetVolume are read back and clamped", function()
        if noMockEngine() then
          return
        end
        local rates, pitches, volumes = {}, {}, {}
        collect("ttsRateChanged", rates)
        collect("ttsPitchChanged", pitches)
        collect("ttsVolumeChanged", volumes)
        local rate, pitch, volume = ttsGetRate(), ttsGetPitch(), ttsGetVolume()
        finally(function()
          ttsSetRate(rate)
          ttsSetPitch(pitch)
          ttsSetVolume(volume)
        end)

        ttsSetRate(0.5)
        assert.equals(0.5, ttsGetRate())
        ttsSetRate(5)
        assert.equals(1, ttsGetRate())
        ttsSetRate(-5)
        assert.equals(-1, ttsGetRate())
        assert.same({0.5, 1, -1}, rates)

        ttsSetPitch(0.25)
        assert.equals(0.25, ttsGetPitch())
        ttsSetPitch(9)
        assert.equals(1, ttsGetPitch())
        ttsSetPitch(-9)
        assert.equals(-1, ttsGetPitch())
        assert.same({0.25, 1, -1}, pitches)

        ttsSetVolume(0.3)
        assert.equals(0.3, ttsGetVolume())
        ttsSetVolume(9)
        assert.equals(1, ttsGetVolume())
        -- volume clamps to zero rather than to -1
        ttsSetVolume(-9)
        assert.equals(0, ttsGetVolume())
        assert.same({0.3, 1, 0}, volumes)
      end)

      it("the voice setters switch voice and report it back", function()
        if noMockEngine() or tooFewVoices() then
          return
        end
        local voices = ttsGetVoices()
        local changes = {}
        local originalVoice = ttsGetCurrentVoice()
        collect("ttsVoiceChanged", changes)
        finally(function() ttsSetVoiceByName(originalVoice) end)

        assert.is_true(ttsSetVoiceByName(voices[2]))
        assert.equals(voices[2], ttsGetCurrentVoice())
        assert.is_true(ttsSetVoiceByIndex(1))
        assert.equals(voices[1], ttsGetCurrentVoice())
        assert.same({voices[2], voices[1]}, changes)
      end)

      it("ttsSpeechStarted carries the text that just started being spoken", function()
        if noMockEngine() then
          return
        end
        -- Regression #9591: the text used to be recorded after say() returned,
        -- and the engine changes state inside say(), so the event carried the
        -- previous utterance. The handler has to be armed up front because the
        -- event is raised before ttsSpeak() returns.
        local started = {}
        collect("ttsSpeechStarted", started)

        ttsClearQueue()
        ttsSpeak("first spoken line")
        assert.same({"first spoken line"}, started)

        ttsSkip()
        ttsSpeak("second spoken line")
        assert.same({"first spoken line", "second spoken line"}, started)
      end)

      it("announces an utterance spoken over one that is still running", function()
        if noMockEngine() then
          return
        end
        -- #9659: the events are raised off the engine's state edges, and an
        -- engine that is already speaking has no edge to report when it is
        -- handed something else - so a script tracking what is being spoken was
        -- never told the text had changed, while ttsGetCurrentLine() moved on
        -- underneath it. No ttsSkip() here: the interruption is the point.
        local started = {}
        collect("ttsSpeechStarted", started)

        ttsClearQueue()
        ttsSpeak("the utterance being spoken over")
        ttsSpeak("the utterance spoken over it")
        assert.same({"the utterance being spoken over", "the utterance spoken over it"}, started)
        assert.equals("the utterance spoken over it", ttsGetCurrentLine())
      end)

      it("ttsSpeechStarted carries the queued line the drain started speaking", function()
        if noMockEngine() then
          return
        end
        -- Regression #9591 again: the queue drain in ttsStateChanged() had the
        -- same say()-before-record ordering as ttsSpeak(), so the event named
        -- the utterance the skip had just ended rather than the queued one that
        -- replaced it. ttsGetCurrentLine() reads correctly either way, so only
        -- the event's own argument can catch this.
        local started = {}
        collect("ttsSpeechStarted", started)

        ttsClearQueue()
        ttsSpeak("the line that gets skipped")
        ttsQueue("line taken off the queue")
        ttsSkip()
        assert.same({"the line that gets skipped", "line taken off the queue"}, started)
      end)

      it("speaking over a busy engine leaves the direct utterance current", function()
        if noMockEngine() then
          return
        end
        -- The utterance a script asks for outright has to survive the queue:
        -- an engine reporting Ready for the utterance say() interrupted used to
        -- be read as an idle engine, which drained the queued line straight
        -- over the requested one (#9659). The mock engine reports no such Ready,
        -- so what this spec can hold onto is the state ttsSpeak() leaves behind
        -- - the guard itself is exercised by TtsInterruptingSpeakTest, which
        -- delivers that Ready the way a real engine does.
        local started = {}
        collect("ttsSpeechStarted", started)

        ttsClearQueue()
        ttsSpeak("the busy utterance")
        ttsQueue("still queued")
        ttsSpeak("the direct utterance")
        assert.equals(1, #ttsGetQueue())
        assert.equals("the direct utterance", ttsGetCurrentLine())
        -- ...and the queued line was not what got announced:
        assert.same({"the busy utterance", "the direct utterance"}, started)
      end)

      it("ttsSetVoiceByName reports success for a voice it switched to", function()
        if noMockEngine() then
          return
        end
        local voices = ttsGetVoices()
        if #voices < 2 then
          pending("the mock engine offers only one voice in this environment")
          return
        end
        -- Regression #9590: dispatching ttsVoiceChanged used to wipe this
        -- lua_State's whole stack, taking the already-pushed result with it and
        -- handing the caller stack garbage. A handler must be listening for the
        -- event to reach Lua at all, which is what collect() arranges here.
        local changes = {}
        collect("ttsVoiceChanged", changes)
        local originalVoice = ttsGetCurrentVoice()
        finally(function() ttsSetVoiceByName(originalVoice) end)

        assert.is_true(ttsSetVoiceByName(voices[2]))
        assert.equals(voices[2], ttsGetCurrentVoice())
        assert.same({voices[2]}, changes)
      end)
    end)
  end)
