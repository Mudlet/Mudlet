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
      assert.are.equal("spawn: need read function and process name as parameters.", err)
    end)

    it("should reject a first argument that is not a function", function()
      local ok, err = pcall(spawn, "not a function", "echo")
      assert.is_false(ok)
      assert.are.equal("spawn: need read function as first parameter.", err)
    end)

    it("should reject a non-string process name", function()
      local ok, err = pcall(spawn, function() end, {})
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("spawn: bad argument #2 type (process name", 1, true), tostring(err))
      assert.is_truthy(tostring(err):find("string expected, got table", 1, true))
    end)

    -- the process name is already built and held when a later argument is
    -- rejected, and every argument before the bad one is in the list too
    it("should reject a non-string argument after valid ones", function()
      local ok, err = pcall(spawn, function() end, "echo", "first", "second", {})
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("spawn: bad argument #5 type (process argument", 1, true), tostring(err))
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
      assert.is_truthy(tostring(err):find("spawn: Failed to start process '/nonexistent/mudlet-spawn-test'", 1, true), tostring(err))
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

  -- Tests that need a profile of their own write one here, load it and take it
  -- away again. busted's finally() keeps only the last function it is given, so
  -- the cleanups each test needs are collected here and run, in reverse, by one
  -- after_each
  local profilesDirectory = getMudletHomeDir():match("^(.*)[/\\]")
  local cleanups = {}

  local function onCleanup(undo)
    cleanups[#cleanups + 1] = undo
  end

  after_each(function()
    for index = #cleanups, 1, -1 do
      cleanups[index]()
    end
    cleanups = {}
  end)

  local function removeTree(path)
    if lfs.attributes(path, "mode") ~= "directory" then
      os.remove(path)
      return
    end
    for entry in lfs.dir(path) do
      if entry ~= "." and entry ~= ".." then
        removeTree(path .. "/" .. entry)
      end
    end
    lfs.rmdir(path)
  end

  local function loaded(name)
    local entry = getProfiles()[name]
    return entry ~= nil and entry.loaded
  end

  local function skipped()
    if getOS() == "windows" then
      pending("no POSIX shell for the child")
      return true
    end
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waiting for the child needs pumpEvents(), which is refused outside MUDLET_TEST_MODE")
      return true
    end
    return false
  end

  -- Writes a profile named `name` whose script is `script`, giving its Host
  -- element `hostAttributes` if there are any
  local function makeFixtureProfile(name, script, hostAttributes)
    local directory = profilesDirectory .. "/" .. name
    -- written into the profile made here, so a profile of the same name that
    -- this spec did not make is never deleted
    local marker = directory .. "/mudlet-spec-fixture"
    if lfs.attributes(marker) then
      removeTree(directory)
    end
    assert.is_nil(lfs.attributes(directory), "a profile named " .. name .. " already exists and is not this spec's to delete")
    onCleanup(function()
      if loaded(name) then
        closeProfile(name)
      end
      for _ = 1, 100 do
        if not loaded(name) then
          break
        end
        pumpEvents(50)
      end
      if lfs.attributes(marker) then
        removeTree(directory)
      end
    end)
    local function mkdir(path)
      local ok, err = lfs.mkdir(path)
      assert(ok, "cannot make " .. path .. ": " .. tostring(err))
    end
    mkdir(directory)
    assert(io.open(marker, "w")):close()
    mkdir(directory .. "/current")
    local host = hostAttributes and ("  <HostPackage><Host " .. hostAttributes .. "></Host></HostPackage>\n") or ""
    local file = assert(io.open(directory .. "/current/2026-01-01#00-00-00.xml", "w"))
    file:write([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
]] .. host .. [[
  <ScriptPackage>
    <Script isActive="yes" isFolder="no">
      <name>]] .. name .. [[</name>
      <packageName></packageName>
      <script>]] .. script .. [[</script>
      <eventHandlerList />
    </Script>
  </ScriptPackage>
</MudletPackage>
]])
    file:close()
  end

  -- Hands back a table of `count` values: passed on to pumpEvents(), they leave
  -- whatever it calls only LUAI_MAXCSTACK (8000) - 1 - `count` stack slots
  local function stackFiller(count)
    local filler = {}
    for index = 1, count do
      filler[index] = true
    end
    return filler
  end

  -- A child outlives the Lua state that spawned it unless something ends it:
  -- once its profile closed, the next line it wrote was read off the freed
  -- interpreter, and after a reset it called whatever the new state held at the
  -- old callback reference. Each child below writes a line after a second and,
  -- if still alive a second later, touches a file. The suite's own profile
  -- cannot be torn down from inside it, so each test loads a second one.
  describe("a profile torn down while its process runs", function()
    -- Writes a profile whose script is `script` with its %s replaced by the
    -- spawn() call, and returns the file the child touches if it survives
    local function makeProfile(name, script)
      local survivor = getMudletHomeDir() .. "/" .. name .. "-survivor"
      onCleanup(function() os.remove(survivor) end)
      os.remove(survivor)
      local command = string.format("sleep 1; echo spawnAfterTeardown; sleep 1; touch '%s'", survivor)
      local child = string.format([[spawn(function(line) raiseGlobalEvent("mudletSpecSpawnLine", line) end, "/bin/sh", "-c", %q)]], command)
      makeFixtureProfile(name, string.format(script, child))
      return survivor
    end

    local function collectLines()
      local lines = {}
      local handler = registerAnonymousEventHandler("mudletSpecSpawnLine", function(_, line)
        lines[#lines + 1] = line
      end)
      onCleanup(function() killAnonymousEventHandler(handler) end)
      return lines
    end

    it("should end the process when its profile closes", function()
      if skipped() then
        return
      end
      local name = "mudlet-spec-spawn-closed"
      local lines = collectLines()
      local survivor = makeProfile(name, "%s")

      assert.is_true(loadProfile(name, true))
      assert.is_true(closeProfile(name))
      for _ = 1, 30 do
        pumpEvents(100)
      end

      assert.is_false(loaded(name))
      assert.are.same({}, lines)
      assert.is_nil(lfs.attributes(survivor), "the closed profile's process was still running")
    end)

    it("should end the process when its profile is reset", function()
      if skipped() then
        return
      end
      local name = "mudlet-spec-spawn-reset"
      local lines = collectLines()
      local reruns = 0
      local handler = registerAnonymousEventHandler("mudletSpecSpawnRerun", function()
        reruns = reruns + 1
      end)
      onCleanup(function() killAnonymousEventHandler(handler) end)
      -- the reset runs the script again, which must not start a second child
      local survivor = makeProfile(name, [[
local marker = getMudletHomeDir() .. "/mudlet-spec-spawned"
if io.exists(marker) then
  raiseGlobalEvent("mudletSpecSpawnRerun")
else
  io.open(marker, "w"):close()
  %s
  tempTimer(0, function() resetProfile() end)
end]])

      assert.is_true(loadProfile(name, true))
      for _ = 1, 30 do
        pumpEvents(100)
      end

      assert.are.equal(1, reruns)
      assert.are.same({}, lines)
      assert.is_nil(lfs.attributes(survivor), "the reset profile's process was still running")
    end)

    -- finalizers run inside lua_close(), after the profile's processes were ended
    it("should not start a process from a finalizer run as its profile is reset", function()
      if skipped() then
        return
      end
      local name = "mudlet-spec-spawn-finalizer"
      local lines = collectLines()
      local survivor = makeProfile(name, [[
local marker = getMudletHomeDir() .. "/mudlet-spec-spawned"
if not io.exists(marker) then
  io.open(marker, "w"):close()
  local proxy = newproxy(true)
  getmetatable(proxy).__gc = function()
    %s
  end
  mudletSpecSpawnProxy = proxy
  tempTimer(0, function() resetProfile() end)
end]])

      assert.is_true(loadProfile(name, true))
      for _ = 1, 30 do
        pumpEvents(100)
      end

      assert.are.same({}, lines)
      assert.is_nil(lfs.attributes(survivor), "the finalizer started a process the reset left running")
    end)

  end)

  describe("a callback that raises", function()
    -- an error object left behind on the stack by each line would use it up
    it("should leave the stack as it found it", function()
      if skipped() then
        return
      end
      local calls = 0
      -- one write, so every line is read while the stack is nearly full
      local process = spawn(function()
        calls = calls + 1
        error("mudletSpecSpawnStackCheck")
      end, "/bin/sh", "-c", [[printf '1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n']])
      -- room for the callback and four slots more, fewer than the ten errors
      local filler = stackFiller(7993)
      for _ = 1, 100 do
        if not process.isRunning() then
          break
        end
        pumpEvents(50, unpack(filler))
      end

      assert.is_false(process.isRunning(), "the child never finished")
      assert.are.equal(10, calls)
    end)

    -- Nothing in Lua can read back the suite's own error console or turn on
    -- echoing errors to its main console, so these load a profile that has it
    -- on and read that profile's main console back
    local name = "mudlet-spec-spawn-callback-error"

    -- Loads a profile running `script` and returns its main console's text once
    -- that holds every string in `expected`, or as it was when time ran out
    local function consoleOfProfileRunning(script, expected)
      local consoleText
      local handler = registerAnonymousEventHandler("mudletSpecSpawnErrorConsole", function(_, text)
        consoleText = text
      end)
      onCleanup(function() killAnonymousEventHandler(handler) end)
      makeFixtureProfile(name, script .. [[

tempTimer(0.1, function() raiseGlobalEvent("mudletSpecSpawnErrorConsole", table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")) end, true)]], [[mEchoLuaErrors="yes"]])

      local function complete()
        if not consoleText then
          return false
        end
        for _, text in ipairs(expected) do
          if not consoleText:find(text, 1, true) then
            return false
          end
        end
        return true
      end

      assert.is_true(loadProfile(name, true))
      for _ = 1, 200 do
        if complete() then
          break
        end
        pumpEvents(50)
      end
      assert.is_string(consoleText, "the other profile never reported its console")
      return consoleText
    end

    it("should have its error reported like any other script's", function()
      if skipped() then
        return
      end
      -- the messages are assembled as they run, so only the raised errors can contain them whole
      local text = consoleOfProfileRunning([[
spawn(function() error("mudletSpecSpawnCallback" .. "Boom") end, "/bin/echo", "hello")
spawn(function() error({}) end, "/bin/echo", "hello")]], {"mudletSpecSpawnCallbackBoom", "error object is a table value"})

      assert.is_truthy(text:find("mudletSpecSpawnCallbackBoom", 1, true), text)
      assert.is_truthy(text:find("error object is a table value", 1, true), text)
      assert.is_truthy(text:find("spawn() callback", 1, true), text)
    end)

    it("should report output it has no stack space to hand over", function()
      if skipped() then
        return
      end
      local text = consoleOfProfileRunning([[
tempTimer(0, function()
  local process = spawn(function() end, "/bin/echo", "hello")
  -- passed on to pumpEvents(), these leave the callback no room on the stack
  local filler = {}
  for index = 1, 7999 do
    filler[index] = true
  end
  for _ = 1, 100 do
    if not process.isRunning() then
      break
    end
    pumpEvents(50, unpack(filler))
  end
end)]], {"not enough Lua stack space"})

      assert.is_truthy(text:find("not enough Lua stack space", 1, true), text)
      assert.is_truthy(text:find("spawn() callback", 1, true), text)
    end)
  end)

end)
