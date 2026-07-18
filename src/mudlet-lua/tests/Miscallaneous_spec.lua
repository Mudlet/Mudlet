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
  end)
