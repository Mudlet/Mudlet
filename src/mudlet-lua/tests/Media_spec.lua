-- Specs for the media and text-to-speech Lua APIs.
--
-- Both families were previously homed in other domain spec files: the media
-- contracts in Networking_spec.lua and the text-to-speech ones in
-- Miscallaneous_spec.lua. They live here now so the audio side of the API has
-- one home. The specs themselves are unchanged by that move.

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

describe("Tests the text-to-speech Lua API", function()
    describe("Tests the functionality of ttsGetQueue", function()
      -- Mudlet compiled without TTS support installs dummy tts functions
      -- which return nil, whereas the real ttsGetQueue() returns a table
      local function ttsAvailable()
        return type(ttsGetQueue()) == "table"
      end

      it("should return a table when called without an index", function()
        if not ttsAvailable() then
          pending("TTS is not available in this build")
          return
        end
        assert.is_table(ttsGetQueue())
      end)

      it("should return false for an index just past the end of the queue", function()
        if not ttsAvailable() then
          pending("TTS is not available in this build")
          return
        end
        ttsClearQueue()
        -- on an empty queue, index 1 is exactly one past the end (index == size)
        assert.is_false(ttsGetQueue(1))
      end)

      it("should return false for an index below the start of the queue", function()
        if not ttsAvailable() then
          pending("TTS is not available in this build")
          return
        end
        assert.is_false(ttsGetQueue(0))
      end)
    end)

    describe("Tests the text-to-speech mock engine", function()
      -- ttsBuild() selects Qt's deterministic mock engine under MUDLET_TEST_MODE,
      -- so these specs only run in test mode and never drive a developer's real
      -- speech engine. Where the mock plugin is absent they skip so local runs
      -- still pass; CI sets MUDLET_TEST_REQUIRE_TTS_MOCK to turn that skip into a
      -- failure, so a broken mock selection cannot hide behind a green skip. Async
      -- speech events (ttsSpeechStarted...) need the waitForEvent helper and are
      -- left to the post-enabler TTS specs.
      local testMode = os.getenv("MUDLET_TEST_MODE")
      local requireMock = os.getenv("MUDLET_TEST_REQUIRE_TTS_MOCK")

      local function ttsEngineAvailable()
        return testMode and type(ttsGetVoices) == "function" and type(ttsGetVoices()) == "table" and #ttsGetVoices() > 0
      end

      -- Returns true when the caller should stop because no engine is available
      -- and skipping is permitted; fails hard where the mock is mandatory.
      local function ttsEngineUnavailable()
        if ttsEngineAvailable() then
          return false
        end
        if requireMock then
          assert.is_true(false, "MUDLET_TEST_REQUIRE_TTS_MOCK is set but the mock TTS engine has no voices - it was not selected")
        end
        pending("mock TTS engine unavailable (run with MUDLET_TEST_MODE and Qt's mock plugin)")
        return true
      end

      it("ttsGetVoices returns a non-empty list of voice-name strings", function()
        if ttsEngineUnavailable() then
          return
        end
        local voices = ttsGetVoices()
        assert.is_table(voices)
        assert.is_true(#voices > 0)
        for _, name in ipairs(voices) do
          assert.is_string(name)
        end
        -- ttsGetState maps the freshly built engine's ready state to its string
        assert.equals("ttsSpeechReady", ttsGetState())
      end)

      it("ttsSpeak accepts valid text and rejects whitespace-only text", function()
        if ttsEngineUnavailable() then
          return
        end
        -- valid text is accepted without leaving the engine in the error state
        ttsSpeak("Mudlet self test speaking")
        assert.is_true(ttsGetState() ~= "ttsSpeechError")
        ttsSkip()
        -- contract: whitespace-only text is rejected with nil + message
        local ok, err = ttsSpeak("   ")
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)
  end)
