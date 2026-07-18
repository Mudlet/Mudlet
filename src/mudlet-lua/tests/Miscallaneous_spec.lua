describe("Tests C++ functions in the Miscallaneous category", function()
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
  end)
