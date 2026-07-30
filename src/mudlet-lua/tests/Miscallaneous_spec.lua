describe("Tests C++ functions in the Miscallaneous category", function()
    describe("Tests the functionality of sendMSDP", function()
      it("should return nil and an error message when MSDP cannot be sent", function()
        local ok, err = sendMSDP("CLIENT_NAME", "Mudlet")
        if ok ~= nil then
          -- connected to a server which negotiated MSDP, so the send succeeded
          assert.is_true(ok)
          return
        end
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_true(err:find("MSDP") ~= nil)
      end)
    end)

    describe("Tests the functionality of getOS", function()
      it("should return the correct number of values for the current OS", function()
        local results = {getOS()}
        -- Linux returns 4 values, all others return 3
        if results[1] == "linux" then
          assert.equals(4, #results)
        else
          assert.equals(3, #results)
        end
      end)

      it("should return string values", function()
        local osName, osVersion, osType = getOS()
        assert.is_string(osName)
        assert.is_string(osVersion)
        if osType then
          assert.is_string(osType)
        end
      end)

      it("should return a valid OS name as first value", function()
        local validOSNames = {
          "windows", "mac", "linux", "cygwin", "hurd",
          "freebsd", "kfreebsd", "openbsd", "netbsd",
          "bsd4", "unix", "unknown"
        }
        local osName = getOS()
        assert.is_true(table.contains(validOSNames, osName))
      end)

      it("should not return empty strings", function()
        local osName, osVersion, osType = getOS()
        assert.is_true(osName ~= "")
        assert.is_true(osVersion ~= "")
        if osType then
          assert.is_true(osType ~= "")
        end
      end)
    end)

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

    describe("Tests the functionality of getTimestamp", function()
      it("should return a string for a valid line number", function()
        echo("getTimestamp test line\n")
        assert.is_string(getTimestamp(1))
      end)

      it("should return nil+msg for an out-of-range line number", function()
        local timestamp, err = getTimestamp(getLineCount() + 1000)
        assert.is_nil(timestamp)
        assert.is_string(err)
        assert.is_true(err:find("beyond the last line", 1, true) ~= nil)
      end)

      it("should return nil+msg for an out-of-range line number in a miniconsole", function()
        createMiniConsole("getTimestampTestConsole", 0, 0, 100, 100)
        local timestamp, err = getTimestamp("getTimestampTestConsole", 1000000)
        assert.is_nil(timestamp)
        assert.is_string(err)
        assert.is_true(err:find("beyond the last line", 1, true) ~= nil)
        deleteMiniConsole("getTimestampTestConsole")
      end)
    end)

    describe("Tests the functionality of getModulePath", function()
      it("should return nil+msg for a module that does not exist", function()
        local path, err = getModulePath("busted-nonexistent-module")
        assert.is_nil(path)
        assert.is_string(err)
        assert.is_true(err:find("module doesn't exist", 1, true) ~= nil)
      end)
    end)

    describe("Tests the functionality of getModulePriority", function()
      it("should return nil+msg for a module that does not exist", function()
        local priority, err = getModulePriority("busted-nonexistent-module")
        assert.is_nil(priority)
        assert.is_string(err)
        assert.is_true(err:find("module doesn't exist", 1, true) ~= nil)
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

    describe("Tests HTTP requests against the local fixture server", function()
      -- Exercises getHTTP/downloadFile against the fixture server started by the
      -- "Start fixture HTTP server" workflow step, whose port is handed over in
      -- MUDLET_TEST_HTTP_PORT. Skips cleanly when that is unset (a local run with
      -- no fixture server) so the suite still passes. The wire response is
      -- asynchronous (sysDownloadDone); asserting on it needs the waitForEvent
      -- helper and is left to the post-enabler HTTP specs.
      local port = os.getenv("MUDLET_TEST_HTTP_PORT")

      -- Runs everywhere (no server needed): an invalid url is rejected up front.
      it("getHTTP rejects an invalid url with nil and a message", function()
        local ok, err = getHTTP("")
        assert.is_nil(ok)
        assert.is_string(err)
      end)

      it("getHTTP accepts a request to the local fixture server", function()
        if not port then
          pending("MUDLET_TEST_HTTP_PORT not set (fixture HTTP server not running)")
          return
        end
        local ok, actualUrl = getHTTP("http://127.0.0.1:" .. port .. "/fixture.txt")
        assert.is_true(ok)
        assert.is_string(actualUrl)
        assert.is_true(actualUrl:find("127.0.0.1:" .. port, 1, true) ~= nil)
      end)

      it("downloadFile accepts a request to the local fixture server", function()
        if not port then
          pending("MUDLET_TEST_HTTP_PORT not set (fixture HTTP server not running)")
          return
        end
        local target = getMudletHomeDir() .. "/busted-http-fixture.txt"
        os.remove(target)
        local ok, actualUrl = downloadFile(target, "http://127.0.0.1:" .. port .. "/fixture.txt")
        assert.is_true(ok)
        assert.is_string(actualUrl)
        assert.is_true(actualUrl:find("127.0.0.1:" .. port, 1, true) ~= nil)
      end)
    end)
  end)
