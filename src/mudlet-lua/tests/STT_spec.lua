-- Contract tests for the stt.* speech-to-text bridge.
--
-- Everything here runs without a microphone, a model or a recognition
-- library, because that is the state the API spends most of its life in: a
-- player who has never set speech up, and every CI runner. The bridge
-- promises to be inert until called and to refuse clearly rather than crash
-- or lie, and those promises are what these specs hold it to.
--
-- Where behaviour legitimately differs between a machine with an engine
-- installed and one without, the spec branches on stt.available() rather
-- than assuming either, so it passes in both places.

describe("stt bridge", function()

  describe("API surface", function()

    it("registers the stt table whether or not an engine is installed", function()
      assert.is_table(stt, "stt should exist even with no recognition engine present")
    end)

    it("provides every documented function", function()
      local documented = {
        "init", "start", "stop", "toggle", "close",
        "available", "initialized", "listening",
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
      assert.is_boolean(stt.available())
      assert.is_boolean(stt.initialized())
      assert.is_boolean(stt.listening())
    end)

    it("is not listening or initialized before anything has been set up", function()
      if stt.initialized() then
        -- A previous spec or the player left a model loaded; the claim below
        -- only means anything from a clean start
        return
      end
      assert.is_false(stt.listening(), "nothing should be listening before a model is loaded")
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

    it("answers every documented key, engine installed or not", function()
      -- The keys a package probes before deciding what it can do must be
      -- readable before anything is installed, which is exactly when it
      -- probes. Only version and language are documented as appearing later.
      local info = stt.getInfo()
      assert.is_table(info.capabilities, "capabilities must be readable with no engine present")
      assert.is_boolean(info.capabilities.biasing)
      assert.is_boolean(info.capabilities.grammar)
      assert.is_boolean(info.capabilities.words)
      assert.is_boolean(info.capabilities.onDevice)
      assert.is_number(info.silenceTimeout, "0 while disabled, not absent")
      assert.is_number(info.audioLevel)
      assert.is_string(info.sensitivity)
    end)

    it("reports a sensitivity from the documented set", function()
      local modes = {short = true, default = true, long = true}
      assert.is_true(modes[stt.getInfo().sensitivity] ~= nil,
        "unexpected sensitivity: " .. tostring(stt.getInfo().sensitivity))
    end)

    it("reports a state from the documented set", function()
      local states = {
        uninitialized = true, ready = true, starting = true,
        listening = true, processing = true, error = true,
      }
      assert.is_true(states[stt.getInfo().state] ~= nil,
        "unexpected state: " .. tostring(stt.getInfo().state))
    end)

    it("agrees with the individual queries", function()
      local info = stt.getInfo()
      assert.are.equal(stt.available(), info.available)
      assert.are.equal(stt.initialized(), info.initialized)
      assert.are.equal(stt.listening(), info.listening)
    end)

    it("names the engine it would use", function()
      -- Asserting "non-empty or the string none" was a test that could not
      -- fail, since every string is one or the other. The contract worth
      -- holding is that the name is one this build actually has.
      assert.are.equal("Vosk", stt.getInfo().backend)
    end)
  end)

  describe("refusals", function()

    -- A raise through the binding must not strand anything it built first:
    -- getVerifiedString ends in lua_error(), which longjmps past C++
    -- destructors, and no spec reached that path until this one
    it("raises on an argument of the wrong type without leaking what it built", function()
      assert.has_error(function() stt.init({}) end)
      assert.has_error(function() stt.init(true) end)
    end)

    -- QDir("") is Qt's spelling for the working directory, so an empty path
    -- passes an existence check and the engine is handed wherever Mudlet was
    -- started from
    it("refuses an empty model path rather than reading the working directory", function()
      local ok, err = stt.init("")
      assert.is_nil(ok)
      assert.is_string(err)
      assert.is_truthy(err:find("empty"), "the refusal should name the empty path, got: " .. tostring(err))
    end)

    it("refuses a model path that does not exist, without crashing", function()
      local ok, err = stt.init("/definitely/not/a/model/path/for/testing")
      assert.is_nil(ok, "loading a missing model should fail")
      assert.is_string(err, "a refusal should say why")
    end)

    -- "Refusals speak" has to hold with no engine installed too, which is
    -- where there is no recognizer to emit through and so was the one place it
    -- did not: a package driving the bridge from events alone saw nothing
    -- happen and could not tell a missing engine from a quiet microphone.
    it("tells a package listening for sysSTTError why it refused", function()
      local seen
      local handler = registerAnonymousEventHandler("sysSTTError", function(_, message) seen = message end)
      finally(function() killAnonymousEventHandler(handler) end)

      local _, err = stt.init("/definitely/not/a/model/path/for/testing")
      assert.is_string(seen, "the refusal was returned to the caller but never announced")
      assert.are.equal(err, seen, "the event and the return value should carry the same reason")
    end)

    -- A refusal that is the script's own mistake is not news for every package
    -- on the profile - only what the engine could not do is
    it("does not announce an argument mistake as an engine error", function()
      local raised = false
      local handler = registerAnonymousEventHandler("sysSTTError", function() raised = true end)
      finally(function() killAnonymousEventHandler(handler) end)

      stt.setSilenceTimeout(-1)
      assert.is_false(raised, "a bad argument is a script error, not something the engine reports")
    end)

    -- modelPath is what a package reads to decide whether setup already
    -- happened, so a path that failed to load standing in it skips the init
    -- that was needed
    it("names no model after a failed load", function()
      if stt.initialized() then return end
      stt.init("/definitely/not/a/model/path/for/testing")
      assert.are.equal("", stt.getInfo().modelPath, "a model that never loaded was reported as loaded")
    end)

    -- The message has to name the thing that is actually missing. When the
    -- engine library is absent no backend is available, so no model can be
    -- chosen however many are installed - and being told to install a model
    -- you already have sends you looking in the wrong place.
    it("names the missing engine library rather than blaming the models", function()
      if stt.available() then return end
      local ok, err = stt.init()
      assert.is_nil(ok, "init with no engine library should fail")
      assert.is_string(err)
      assert.is_truthy(err:find("librar"), "the refusal should name the engine library, got: " .. tostring(err))
    end)

    -- With the library present and no model, the refusal has to name the
    -- directory a model belongs in. It used to report a made-up default path
    -- as missing, which named a directory the reader never created and left
    -- the "install a model" message unreachable.
    it("names where a model belongs when none is installed", function()
      if not stt.available() or #stt.listModels() > 0 then return end
      local ok, err = stt.init()
      assert.is_nil(ok, "init with no model installed should fail")
      assert.is_string(err)
      assert.is_truthy(err:find(stt.getModelPath(), 1, true), "the refusal should name the models directory, got: " .. tostring(err))
    end)

    it("refuses to start before a model is loaded", function()
      if stt.initialized() then return end
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
      -- Only from a state that is not error: "stopped" and "was never running
      -- because it failed" are different answers, and the second one is
      -- reported rather than dressed up as the first
      if stt.getInfo().state == "error" then
        local ok, err = stt.stop()
        assert.is_nil(ok, "stopping in an error state should not claim a clean stop")
        assert.is_string(err)
        return
      end
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
      if stt.available() then
        assert.is_true(ok)
      else
        assert.is_nil(ok, "with no engine there is nothing to set the timeout on")
        assert.is_string(err)
      end
    end)

    -- With an engine present the answer is either true, or a refusal because
    -- this build of it cannot tune end-of-speech detection at all - an older
    -- libvosk without the endpointer symbol is a supported configuration, not
    -- a fault, and asserting true here would go red on a correct build
    it("accepts each documented sensitivity, or says the engine cannot", function()
      for _, mode in ipairs({"short", "default", "long"}) do
        local ok, err = stt.setSensitivity(mode)
        if stt.available() then
          if ok == nil then
            assert.is_string(err, mode .. " was refused without saying why")
          else
            assert.is_true(ok, mode .. " should be accepted")
          end
        else
          assert.is_nil(ok, mode .. " has no engine to apply to")
          assert.is_string(err)
        end
      end
    end)

    it("answers setVocabulary with whether the engine took the words", function()
      local ok = stt.setVocabulary({"kill", "look", "inventory"})
      if stt.available() then
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

    -- "nil or a string" is every value there is, so it could not fail. The key
    -- is what an installer picks a download by, so a wrong one is worse than
    -- none, and the platform running the spec is known.
    it("names this platform with the key an installer would download by", function()
      local key = stt.getPlatformKey()
      -- The architecture is not visible from Lua, so each platform's keys are
      -- named rather than one of them: Windows on ARM64 has no build and
      -- correctly answers nil, which is the only nil this may be.
      local allowed = {
        mac = {["macos"] = true},
        linux = {["linux-x86_64"] = true, ["linux-aarch64"] = true},
        windows = {["windows-x64"] = true, ["windows-x86"] = true},
      }
      local keys = allowed[getOS()]
      assert.is_table(keys, "this spec does not know the keys for " .. tostring(getOS()))
      if getOS() == "windows" and key == nil then
        return -- ARM64 Windows, which ships no engine build
      end
      assert.is_string(key, "a platform with a build should name its key")
      assert.is_true(keys[key] == true, "unexpected platform key: " .. tostring(key))
    end)
  end)
end)
