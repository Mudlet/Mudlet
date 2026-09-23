-- Every spawn() error path longjmps out of C++ code that owns heap: the
-- program name, the accumulated argument list and the failure message. These
-- drive each path so LeakSanitizer fails the build if one starts stranding
-- again, and pin the messages while doing it.
--
-- A spawn that actually starts must also be drained before its test ends: the
-- C++ wrapper is freed from TLuaInterpreter's purge timer, which only ticks
-- while the event loop runs, and the suite holds that loop shut.

describe("spawn", function()

  describe("argument checking", function()

    it("should reject a call with no process name", function()
      local ok, err = pcall(spawn, function() end)
      assert.is_false(ok)
      assert.are.equal("Need read function and process name as parameters.", err)
    end)

    it("should reject a first argument that is not a function", function()
      local ok, err = pcall(spawn, "not a function", "echo")
      assert.is_false(ok)
      assert.are.equal("Need read function as first parameter.", err)
    end)

    it("should reject a non-string process name", function()
      local ok, err = pcall(spawn, function() end, {})
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("bad argument #2", 1, true))
      assert.is_truthy(tostring(err):find("string expected, got table", 1, true))
    end)

    -- the process name is already built and held when a later argument is
    -- rejected, and every argument before the bad one is in the list too
    it("should reject a non-string argument after valid ones", function()
      local ok, err = pcall(spawn, function() end, "echo", "first", "second", {})
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("bad argument #5", 1, true))
      assert.is_truthy(tostring(err):find("string expected, got table", 1, true))
    end)

    it("should accept numbers where strings are expected, as Lua does", function()
      -- coercible, so this gets past argument checking and fails on the binary
      local ok, err = pcall(spawn, function() end, "/nonexistent/mudlet-spawn-test", 42)
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("Failed to start process", 1, true))
    end)

  end)

  describe("start failure", function()

    -- the failure message embeds the program name, working directory and PATH,
    -- so it is the largest thing this function ever holds at a raise
    it("should report a binary that does not exist", function()
      local ok, err = pcall(spawn, function() end, "/nonexistent/mudlet-spawn-test")
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("Failed to start process '/nonexistent/mudlet-spawn-test'", 1, true))
      assert.is_truthy(tostring(err):find("Working directory:", 1, true))
      assert.is_truthy(tostring(err):find("PATH:", 1, true))
    end)

    it("should report a binary that does not exist when given arguments too", function()
      local ok, err = pcall(spawn, function() end, "/nonexistent/mudlet-spawn-test", "one", "two", "three")
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("Failed to start process", 1, true))
    end)

  end)

  describe("reading output", function()

    -- A child that writes its lines in one go can produce a single readyRead
    -- with every line already in the buffer, and nothing then arrives to signal
    -- again, so a reader that took one line per signal lost the rest
    it("should hand the callback every line of a burst, not just the first (#322)", function()
      if getOS() == "windows" then
        pending("no POSIX shell to write several lines in one go")
        return
      end
      if not os.getenv("MUDLET_TEST_MODE") then
        pending("waiting for the child's output needs pumpEvents(), which is refused outside MUDLET_TEST_MODE")
        return
      end

      local lines = {}
      spawn(function(line)
        table.insert(lines, line)
      end, "/bin/sh", "-c", "printf 'spawnBurstA\\nspawnBurstB\\nspawnBurstC\\n'")
      finally(function()
        -- one full period of the 2s purge timer that owns the wrapper's deletion
        pumpEvents(2200)
      end)

      for _ = 1, 500 do
        if #lines >= 3 then
          break
        end
        pumpEvents(20)
      end

      assert.are.same({"spawnBurstA\n", "spawnBurstB\n", "spawnBurstC\n"}, lines)
    end)

  end)

end)
