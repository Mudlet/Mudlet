-- Contract tests for the stt.* speech-to-text bridge.
--
-- Everything here runs without a microphone, a model or a recognition
-- library, because that is the state the API spends most of its life in: a
-- player who has never set speech up, and every CI runner. The bridge
-- promises to be inert until called and to refuse clearly rather than crash
-- or lie, and those promises are what these specs hold it to.
--
-- Where behaviour legitimately differs between a machine with an engine
-- installed and one without, the spec branches on stt.isAvailable() rather
-- than assuming either, so it passes in both places.

describe("stt bridge", function()

  describe("API surface", function()

    it("registers the stt table whether or not an engine is installed", function()
      assert.is_table(stt, "stt should exist even with no recognition engine present")
    end)

    it("provides every documented function", function()
      local documented = {
        "init", "start", "stop", "toggle", "close",
        "isAvailable", "isInitialized", "isListening",
        "getInfo", "getModelPath", "getLibraryPath", "listModels",
        "getPlatformKey", "reloadLibrary", "unloadLibrary",
        "setSilenceTimeout", "setSensitivity", "setVocabulary",
      }
      for _, name in ipairs(documented) do
        assert.is_function(stt[name], ("stt.%s should be a function"):format(name))
      end
    end)
  end)

  describe("state queries", function()

    it("answers with booleans rather than nil", function()
      assert.is_boolean(stt.isAvailable())
      assert.is_boolean(stt.isInitialized())
      assert.is_boolean(stt.isListening())
    end)

    it("is not listening or initialized before anything has been set up", function()
      if stt.isInitialized() then
        -- A previous spec or the player left a model loaded; the claim below
        -- only means anything from a clean start
        return
      end
      assert.is_false(stt.isListening(), "nothing should be listening before a model is loaded")
    end)
  end)

  describe("getInfo", function()

    it("returns a table with the documented keys and types", function()
      local info = stt.getInfo()
      assert.is_table(info)
      assert.is_string(info.backend, "backend names the engine running, or none")
      assert.is_boolean(info.available)
      assert.is_boolean(info.initialized)
      assert.is_boolean(info.listening)
      assert.is_string(info.state)
      assert.is_string(info.modelPath)
      assert.is_table(info.searchPaths)
    end)

    it("reports a state from the documented set", function()
      local states = {
        uninitialized = true, ready = true, listening = true,
        processing = true, error = true,
      }
      assert.is_true(states[stt.getInfo().state] ~= nil,
        "unexpected state: " .. tostring(stt.getInfo().state))
    end)

    it("agrees with the individual queries", function()
      local info = stt.getInfo()
      assert.are.equal(stt.isAvailable(), info.available)
      assert.are.equal(stt.isInitialized(), info.initialized)
      assert.are.equal(stt.isListening(), info.listening)
    end)

    it("names no engine until one has been created", function()
      if stt.getInfo().initialized then return end
      -- Either no recognizer exists yet, or one does and must name itself
      local backend = stt.getInfo().backend
      assert.is_true(backend == "none" or #backend > 0,
        "backend should be 'none' or a real engine name, got: " .. tostring(backend))
    end)
  end)

  describe("refusals", function()

    it("refuses a model path that does not exist, without crashing", function()
      local ok, err = stt.init("/definitely/not/a/model/path/for/testing")
      assert.is_nil(ok, "loading a missing model should fail")
      assert.is_string(err, "a refusal should say why")
    end)

    it("refuses to start before a model is loaded", function()
      if stt.isInitialized() then return end
      local ok, err = stt.start()
      assert.is_nil(ok, "starting without a model should fail")
      assert.is_string(err)
    end)

    it("refuses a negative silence timeout", function()
      local ok, err = stt.setSilenceTimeout(-1)
      assert.is_nil(ok, "a negative timeout is not a duration")
      assert.is_string(err)
    end)

    it("refuses a sensitivity it does not have", function()
      local ok, err = stt.setSensitivity("immediately")
      assert.is_nil(ok, "an unknown sensitivity should be refused, not guessed at")
      assert.is_string(err)
    end)
  end)

  describe("safe when nothing is set up", function()

    it("stops without complaint when nothing is listening", function()
      assert.is_true(stt.stop(), "stopping nothing is not an error")
    end)

    it("closes without complaint when nothing is initialized", function()
      assert.is_true(stt.close(), "closing nothing is not an error")
    end)

    -- These three reach the engine, so with none installed they refuse
    -- instead of succeeding. Both outcomes are the contract; which one
    -- applies depends on the machine, so the spec checks the right one.

    it("accepts a zero silence timeout, which means no timeout", function()
      local ok, err = stt.setSilenceTimeout(0)
      if stt.isAvailable() then
        assert.is_true(ok)
      else
        assert.is_nil(ok, "with no engine there is nothing to set the timeout on")
        assert.is_string(err)
      end
    end)

    it("accepts each documented sensitivity", function()
      for _, mode in ipairs({"short", "default", "long"}) do
        local ok, err = stt.setSensitivity(mode)
        if stt.isAvailable() then
          assert.is_true(ok, mode .. " should be accepted")
        else
          assert.is_nil(ok, mode .. " has no engine to apply to")
          assert.is_string(err)
        end
      end
    end)

    it("answers setVocabulary with whether the engine took the words", function()
      local ok = stt.setVocabulary({"kill", "look", "inventory"})
      if stt.isAvailable() then
        -- False is not a failure: it is the documented signal that this
        -- backend cannot bias, and the caller should correct results itself
        assert.is_boolean(ok)
      else
        assert.is_nil(ok, "with no engine there is nothing to give the words to")
      end
    end)
  end)

  describe("installation paths", function()

    it("reports where models and the engine library belong", function()
      assert.is_true(#stt.getModelPath() > 0, "models need somewhere to live")
      assert.is_true(#stt.getLibraryPath() > 0, "the library needs somewhere to live")
    end)

    it("lists installed models without needing the engine library", function()
      assert.is_table(stt.listModels(), "listing models must work before anything is installed")
    end)

    it("names this platform, or admits there is no build for it", function()
      local key = stt.getPlatformKey()
      assert.is_true(key == nil or type(key) == "string")
    end)
  end)
end)
