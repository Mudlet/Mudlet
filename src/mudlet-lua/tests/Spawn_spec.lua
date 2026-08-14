-- Every spawn() error path longjmps out of C++ code that owns heap: the
-- program name, the accumulated argument list and the failure message. These
-- drive each path so LeakSanitizer fails the build if one starts stranding
-- again, and pin the messages while doing it.
--
-- Only the failing paths are exercised - a successful spawn would leave a real
-- child process behind for the rest of the suite.

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

end)
