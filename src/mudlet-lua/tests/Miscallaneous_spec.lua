describe("Tests C++ functions in the Miscallaneous category", function()
    describe("Tests the functionality of sendMSDP", function()
      it("should return nil and an error message when MSDP cannot be sent", function()
        local ok, err = sendMSDP("CLIENT_NAME", "Mudlet")
        if ok == true then
          -- connected to a server which negotiated MSDP, so the send succeeded
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
  end)
