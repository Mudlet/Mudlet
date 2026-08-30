describe("Tests Other.lua functions", function()

  describe("Tests the functionality of sendAll", function()
    -- sendAll and the speedwalk family below drive the real send() function.
    -- Offline (the self-test profile is not connected) send() is a no-op on the
    -- wire, so we spy on the real function (pass-through) rather than replacing
    -- it with a mock, and assert the actual dispatch it performs.

    it("should send one command if it is only given one parameter", function()
      local send = spy.on(_G, "send")
      sendAll("look")
      assert.spy(send).was.called(1)
      assert.spy(send).was.called_with("look", true)
      send:revert()
    end)

    it("should send multiple commands when given multiple string parameters", function()
      local send = spy.on(_G, "send")
      local commands = {
        "get gold from pouch",
        "buy potion",
        "put gold in pouch"
      }
      sendAll(unpack(commands))
      assert.spy(send).was.called(#commands)
      for _,command in ipairs(commands) do
        assert.spy(send).was.called_with(command, true)
      end
      send:revert()
    end)

    it("should pass along a final boolean argument of false to all sends", function()
      local send = spy.on(_G, "send")
      sendAll("get gold from pouch", "buy potion", "put gold in pouch", false)
      assert.spy(send).was.called(3)
      assert.spy(send).was.called_with("get gold from pouch", false)
      assert.spy(send).was.called_with("buy potion", false)
      assert.spy(send).was.called_with("put gold in pouch", false)
      send:revert()
    end)

    it("should pass along a final boolean argument of true to all sends", function()
      local send = spy.on(_G, "send")
      sendAll("get gold from pouch", "buy potion", "put gold in pouch", true)
      assert.spy(send).was.called(3)
      assert.spy(send).was.called_with("get gold from pouch", true)
      assert.spy(send).was.called_with("buy potion", true)
      assert.spy(send).was.called_with("put gold in pouch", true)
      send:revert()
    end)

    it("schedules a tempTimer per command instead of sending when the first argument is a delay", function()
      local send = spy.on(_G, "send")
      -- Wrap the real tempTimer (pass-through) purely to capture the ids it
      -- returns so we can cancel the scheduled timers; sendAll discards them.
      local scheduledIds = {}
      local realTempTimer = _G.tempTimer
      _G.tempTimer = function(...)
        local id = realTempTimer(...)
        scheduledIds[#scheduledIds + 1] = id
        return id
      end
      finally(function()
        _G.tempTimer = realTempTimer
        for _, id in ipairs(scheduledIds) do
          pcall(killTimer, id)
        end
        send:revert()
      end)

      sendAll(5, "north", "south")
      assert.spy(send).was_not_called()
      assert.equals(2, #scheduledIds)
    end)
  end)

  describe("Tests the functionality of sendCmdLine", function()
    -- sendCmdLine sets the active command line's text (no wire traffic to
    -- observe offline), which is readable back with getCmdLine. We clear the
    -- command line afterwards so no text is left behind.
    after_each(function()
      pcall(clearCmdLine)
    end)

    it("sets the command line text and returns true", function()
      assert.is_true(sendCmdLine("look"))
      assert.equals("look", getCmdLine())
    end)

    it("errors when given no argument", function()
      assert.has_error(function() sendCmdLine() end)
    end)

    it("errors when the argument is not a string", function()
      assert.has_error(function() sendCmdLine({}) end)
    end)
  end)

  describe("Tests the functionality of permGroup", function()
    -- permGroup creates *permanent* items, which have no public removal API and
    -- are written to disk when the profile saves. To verify the real dispatch
    -- and the documented default arguments without polluting the profile we use
    -- a parent that does not exist: each underlying perm* function marshals its
    -- arguments and then fails at the parent lookup before creating anything.
    local nonexistentParent = "permGroupSpecNonexistentParent"

    describe("dispatches to the correct underlying function with the documented defaults", function()
      -- A nonexistent parent lets us drive the real dispatch without polluting the
      -- profile: each perm* marshals its arguments and then raises at the parent
      -- lookup before creating anything. The spy is reverted immediately after the
      -- call (its recorded history survives revert) so cleanup happens even against
      -- the old raising code. Each test also confirms the failure surfaces as a
      -- false return rather than a raise.
      it("uses permTimer(name, parent, 0, '') for timers", function()
        local permTimer = spy.on(_G, "permTimer")
        local ok, created = pcall(permGroup, "permGroupSpecTimer", "timer", nonexistentParent)
        permTimer:revert()
        assert.spy(permTimer).was.called_with("permGroupSpecTimer", nonexistentParent, 0, "")
        assert.is_true(ok)
        assert.is_false(created)
      end)

      it("uses permSubstringTrigger(name, parent, {}, '') for triggers", function()
        local permSubstringTrigger = spy.on(_G, "permSubstringTrigger")
        local ok, created = pcall(permGroup, "permGroupSpecTrigger", "trigger", nonexistentParent)
        permSubstringTrigger:revert()
        assert.spy(permSubstringTrigger).was.called_with("permGroupSpecTrigger", nonexistentParent, {}, "")
        assert.is_true(ok)
        assert.is_false(created)
      end)

      it("uses permAlias(name, parent, '', '') for aliases", function()
        local permAlias = spy.on(_G, "permAlias")
        local ok, created = pcall(permGroup, "permGroupSpecAlias", "alias", nonexistentParent)
        permAlias:revert()
        assert.spy(permAlias).was.called_with("permGroupSpecAlias", nonexistentParent, "", "")
        assert.is_true(ok)
        assert.is_false(created)
      end)

      it("uses permKey(name, parent, -1, '') for keys", function()
        local permKey = spy.on(_G, "permKey")
        local ok, created = pcall(permGroup, "permGroupSpecKey", "key", nonexistentParent)
        permKey:revert()
        assert.spy(permKey).was.called_with("permGroupSpecKey", nonexistentParent, -1, "")
        assert.is_true(ok)
        assert.is_false(created)
      end)

      it("uses permScript(name, parent, '', '') for scripts", function()
        local permScript = spy.on(_G, "permScript")
        local ok, created = pcall(permGroup, "permGroupSpecScript", "script", nonexistentParent)
        permScript:revert()
        assert.spy(permScript).was.called_with("permGroupSpecScript", nonexistentParent, "", "")
        assert.is_true(ok)
        assert.is_false(created)
      end)

      it("defaults a missing parent to the top level", function()
        -- The documented two-argument form permGroup(name, type) turns a missing
        -- parent into "", which makes the underlying perm* succeed and create a
        -- real top-level item; stub it so nothing is written to the profile.
        local permTimer = stub(_G, "permTimer", 42)
        local created = permGroup("permGroupSpecDefaultParent", "timer")
        permTimer:revert()
        assert.stub(permTimer).was.called_with("permGroupSpecDefaultParent", "", 0, "")
        assert.is_true(created)
      end)
    end)

    describe("reports failure instead of raising when creation fails", function()
      -- #9545: group_creation_functions checked `perm*(...) == -1`, but the perm*
      -- bindings raise a Lua error on failure (for example a missing parent)
      -- rather than returning -1, so permGroup could never honour its documented
      -- false-on-failure contract. It now pcalls the creation and returns false
      -- plus the underlying error message.
      it("returns false when the parent group does not exist", function()
        local created = permGroup("permGroupSpecOrphan", "timer", nonexistentParent)
        assert.is_false(created)
      end)

      it("returns the underlying error message alongside false", function()
        local created, err = permGroup("permGroupSpecOrphan", "timer", nonexistentParent)
        assert.is_false(created)
        assert.is_string(err)
        -- pin that the real underlying error (which names the missing parent)
        -- propagated, rather than coupling to any particular phrasing
        assert.is_truthy(err:find(nonexistentParent, 1, true))
      end)
    end)

    describe("reports success", function()
      -- Stub the underlying binding so no real permanent item is created (which
      -- would pollute the profile). A successful perm* returns an item id, and
      -- permGroup must surface that as a boolean true. Revert before asserting so
      -- the stub can never leak into later tests.
      it("returns true when the underlying creation succeeds", function()
        local permTimer = stub(_G, "permTimer", 42)
        local created = permGroup("permGroupSpecSuccess", "timer", "irrelevantParent")
        permTimer:revert()
        assert.stub(permTimer).was.called_with("permGroupSpecSuccess", "irrelevantParent", 0, "")
        assert.is_true(created)
      end)
    end)

    describe("error handling", function()
      it("should raise an error if name is not a string", function()
        assert.has_error(function()
          permGroup(123, "timer")
        end, "permGroup: need a name for the new thing")
      end)

      it("should raise an error if name is nil", function()
        assert.has_error(function()
          permGroup(nil, "timer")
        end, "permGroup: need a name for the new thing")
      end)

      it("should raise an error if itemtype is invalid", function()
        assert.has_error(function()
          permGroup("TestName", "invalid_type")
        end, "permGroup: invalid_type isn't a valid type")
      end)

      it("should raise an error if itemtype is nil", function()
        assert.has_error(function()
          permGroup("TestName", nil)
        end, "permGroup: nil isn't a valid type")
      end)
    end)
  end)

  describe("Tests the functionality of io.exists", function()
    it("should return true if the item exists", function()
      local item = getMudletHomeDir()
      assert.is_true(io.exists(item))
    end)

    it("should return false if the item does not exist", function()
      local item = getMudletHomeDir() .. math.random(10000)
      assert.is_false(io.exists(item))
    end)
  end)

  describe("Tests the functionality of xor", function()
    it("should return true if a is false and b is true", function()
      assert.is_true(xor(true, false))
    end)

    it("should return true if a is true and b is false", function()
      assert.is_true(xor(false,true))
    end)

    it("should return false if a is true and b is true", function()
      assert.is_false(xor(true, true))
    end)

    it("should return false if a is false and b is false", function()
      assert.is_false(xor(false, false))
    end)
  end)

  describe("Tests the functionality of speedwalking", function()
    -- speedwalk with no delay dispatches synchronously through the real send();
    -- the delayed form is covered by an immediate-contract test below and the
    -- speedwalk state machine (stop/pause/resume) has its own describe block.
    -- Real timer firing is intentionally out of scope here (no sleeps).

    after_each(function()
      -- if the delayed-walk test's assertion fails before it cleans up, cancel
      -- the scheduled timer chain so it cannot fire during later tests
      pcall(stopSpeedwalk)
    end)

    it("Tests basic speedwalk() with chained directions", function()
      local send = spy.on(_G, "send")

      -- Will walk 16 times down, once southeast, once up. All in immediate succession.
      speedwalk('16d1se1u')
      assert.spy(send).was.called(18)
      assert.spy(send).was.called_with("d", true)
      assert.spy(send).was.called_with("se", true)
      assert.spy(send).was.called_with("u", true)
      assert.spy(send).was_not_called_with("e", true)
      send:revert()
    end)

    it("Tests basic speedwalk() with commas as separators", function()
      local send = spy.on(_G, "send")

      -- Will walk twice northeast, thrice east, twice north, once east. All in immediate succession.
      speedwalk('2ne,3e,2n,e')
      assert.spy(send).was.called(8)
      assert.spy(send).was.called_with("ne", true)
      assert.spy(send).was.called_with("e", true)
      assert.spy(send).was.called_with("n", true)
      send:revert()
    end)

    it("tests reverse speedwalk", function()
      local send = spy.on(_G, "send")

      speedwalk("5sw - 3s - 2n - w", true)
      -- Will walk backwards: east, twice south, thrice north, five times northeast. All in immediate succession.
      assert.spy(send).was.called(11)
      assert.spy(send).was.called_with("ne", true)
      assert.spy(send).was.called_with("n", true)
      assert.spy(send).was.called_with("s", true)
      assert.spy(send).was.called_with("e", true)
      assert.spy(send).was_not_called_with("w", true)
      send:revert()
    end)

    it("dispatches only the first step synchronously when a delay is given", function()
      local send = spy.on(_G, "send")

      -- With a delay the remaining steps are scheduled on real tempTimers
      -- (Wave 2 territory); only the first step happens synchronously.
      speedwalk("3w, 2ne, w, u", true, 1.25)
      assert.spy(send).was.called(1)

      -- Cancel the scheduled continuation so it does not fire during later tests.
      local stopped = stopSpeedwalk()
      assert.is_true(stopped)
      send:revert()
    end)
  end)

  describe("Tests the speedwalk state machine", function()
    -- stopSpeedwalk/pauseSpeedwalk/resumeSpeedwalk drive a shared upvalue state
    -- machine and raise sys* events (raiseEvent dispatches synchronously in
    -- process). We start a delayed speedwalk to enter the running state, then
    -- assert the control functions' return contracts and events without sleeps.

    after_each(function()
      -- Fully clear any active OR paused speedwalk so no timer chain or leftover
      -- walklist survives into the next test. resumeSpeedwalk re-arms a paused
      -- walk so the subsequent stopSpeedwalk can clear its list.
      pcall(resumeSpeedwalk)
      pcall(stopSpeedwalk)
    end)

    it("stopSpeedwalk returns nil and a message when nothing is walking", function()
      local ok, err = stopSpeedwalk()
      assert.is_nil(ok)
      assert.equals("stopSpeedwalk(): no active speedwalk found", err)
    end)

    it("pauseSpeedwalk returns nil and a message when nothing is walking", function()
      local ok, err = pauseSpeedwalk()
      assert.is_nil(ok)
      assert.equals("pauseSpeedwalk(): no active speedwalk found", err)
    end)

    it("resumeSpeedwalk refuses to resume when there is no walklist", function()
      local ok, err = resumeSpeedwalk()
      assert.is_nil(ok)
      assert.equals("resumeSpeedwalk(): attempted to resume a speedwalk but no active speedwalk found", err)
    end)

    it("raises sysSpeedwalkStarted when a walk begins", function()
      local started = false
      local handler = registerAnonymousEventHandler("sysSpeedwalkStarted", function() started = true end)
      finally(function() killAnonymousEventHandler(handler) end)
      speedwalk("2n", false, 1)
      assert.is_true(started)
    end)

    it("raises sysSpeedwalkStopped when a running walk is stopped", function()
      local stopped = false
      local handler = registerAnonymousEventHandler("sysSpeedwalkStopped", function() stopped = true end)
      finally(function() killAnonymousEventHandler(handler) end)
      speedwalk("2n1e", false, 1)
      assert.is_true(stopSpeedwalk())
      assert.is_true(stopped)
    end)

    it("pause then resume raises the paused and resumed events, resuming dispatches the next step", function()
      local paused, resumed = false, false
      local hPause = registerAnonymousEventHandler("sysSpeedwalkPaused", function() paused = true end)
      local hResume = registerAnonymousEventHandler("sysSpeedwalkResumed", function() resumed = true end)
      finally(function()
        killAnonymousEventHandler(hPause)
        killAnonymousEventHandler(hResume)
      end)

      speedwalk("2n1e", false, 1)
      assert.is_true(pauseSpeedwalk())
      assert.is_true(paused)

      -- resuming sends the next queued step synchronously before re-scheduling
      local send = spy.on(_G, "send")
      finally(function() send:revert() end)
      assert.is_true(resumeSpeedwalk())
      assert.is_true(resumed)
      assert.spy(send).was.called(1)
    end)

    it("pauseSpeedwalk a second time returns nil and a message", function()
      speedwalk("2n1e", false, 1)
      assert.is_true(pauseSpeedwalk())
      local ok, err = pauseSpeedwalk()
      assert.is_nil(ok)
      assert.equals("pauseSpeedwalk(): no active speedwalk found", err)
    end)

    it("resumeSpeedwalk refuses to resume an already running speedwalk", function()
      speedwalk("2n1e", false, 1)
      local ok, err = resumeSpeedwalk()
      assert.is_nil(ok)
      assert.equals("resumeSpeedwalk(): attempted to resume an already running speedwalk", err)
    end)
  end)

  describe("Tests the functionality of mudletOlderThan", function()
    it("tests the comparisons", function()
      local versionTable = getMudletVersion()
      local currentVersion = { versionTable.major, versionTable.minor, versionTable.revision }
      local newerVersion = { versionTable.major + 1 , versionTable.minor + 1 , versionTable.revision + 1 }
      local olderVersion = { versionTable.major - 1 , versionTable.minor - 1 , versionTable.revision - 1 }
      assert.is_true(mudletOlderThan(unpack(newerVersion)))
      assert.is_false(mudletOlderThan(unpack(olderVersion)))
      assert.is_false(mudletOlderThan(unpack(currentVersion)))
    end)
  end)

  describe("Tests the functionality of _comp", function()

    it("compares two numbers the same as ==", function()
      assert.is_true(_comp(5,5))
      assert.is_false(_comp(5,6))
    end)

    it("compares two strings the same as ==", function()
      assert.is_true(_comp("Test", "Test"))
      assert.is_false(_comp("Test1", "Test2"))
    end)

    it("compares booleans the same as ==", function()
      assert.is_true(_comp(false, false))
      assert.is_false(_comp(true,false))
    end)

    it("compares tables holding false like tables holding any other value", function()
      assert.is_true(_comp({ key = false }, { key = false }))
      assert.is_false(_comp({ key = false }, { key = true }))
      assert.is_false(_comp({ key = false }, {}))
      assert.is_true(_comp({ outer = { inner = false } }, { outer = { inner = false } }))
    end)

    it("returns true if table B has the same value for every key which table A contains.", function()
      local tableA = { "One", "Two" }
      local tableB = { "One", "Two" }
      assert.is_true(_comp(tableA, tableB))
    end)

    it("returns false if table B has a different value for a key than table A has", function()
      local tableA = { "One", 2 }
      local tableB = { "One", "Two" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("returns false if table A has a kick which table B does not have", function()
      local tableA = { "One", "Two", "Three" }
      local tableB = { "One", "Two" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("returns false if table B has a key which table A does not have", function()
      local tableA = { "One", "Two" }
      local tableB = { "One", "Two", "Three" }
      assert.is_false(_comp(tableA, tableB))
    end)

    it("compares two tables recursively", function()
      local tableA = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableB = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableC = {
        "One",
        { "First", "2nd" },
        "Three"
      }
      assert.is_true(_comp(tableA, tableB))
      assert.is_false(_comp(tableB, tableC))
    end)

    it("should return the same regardless of the order of the arguments", function()
      local tableA = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableB = {
        "One",
        { "First", "Second" },
        "Three"
      }
      local tableC = {
        "One",
        { "First", "2nd" },
        "Three"
      }
      assert.are.same(_comp(tableA, tableB), _comp(tableB, tableA))
      assert.are.same(_comp(tableB, tableC), _comp(tableC, tableB))
    end)
  end)

  describe("Tests the functionality of deleteMultiline", function()
    it("Should return nil + error if run in non-multiline context", function()
      local ok, err = deleteMultiline()
      assert.is_nil(ok)
      assert.equals("does not appear to be run during a multiline trigger match, please try again.", err)
    end)

    it("Should delete all lines between the first match and the script executing", function()
      local s = spy.on(_G, "deleteLine")
      feedTriggers("This line should not be deleted\n")
      feedTriggers("This is the first line\n")
      feedTriggers("This line has some substring match\n")
      feedTriggers("This is now the third and final line\n")
      _G.multimatches = {
        { "This is the first line" },
        { "some substring" },
        { "third line" }
      }
      local ok,err = deleteMultiline(5)
      assert.is_true(ok)
      assert.is_nil(err)
      assert.spy(s).was.called(3)
      moveCursorUp()
      local lastLine = getCurrentLine()
      assert.equal("This line should not be deleted", lastLine)
      _G.multimatches = {}
      s:revert()
    end)
  end)


  describe("Tests timeframe", function()
    teardown(function()
      -- timeframe schedules a real cleanup tempTimer; cancel any pending ones
      -- and clear the test variable.
      killtimeframe("TIMEFRAME_TEST_VARIABLE")
      TIMEFRAME_TEST_VARIABLE = nil
    end)

    it("Should immediately set variable to true", function()
      timeframe("TIMEFRAME_TEST_VARIABLE", 0)

      assert.is_true(TIMEFRAME_TEST_VARIABLE)
    end)

    it("Should accept nil for nil_time param", function()
      timeframe("TIMEFRAME_TEST_VARIABLE", 0, nil)

      assert.is_true(TIMEFRAME_TEST_VARIABLE)
    end)

    it("Should immediately set variable to nil", function()
      timeframe("TIMEFRAME_TEST_VARIABLE", 0, 0)

      assert.is_nil(TIMEFRAME_TEST_VARIABLE)
    end)
  end)

  describe("Tests the stopwatch family", function()
    -- Stopwatches are non-persistent by default, so they are not written to the
    -- profile, but every stopwatch created here is deleted in teardown anyway.
    -- A stopped stopwatch does not advance, so adjustStopWatch on one gives an
    -- exact, deterministic elapsed time with no sleeping. Only the running case
    -- (stopStopWatch below) needs a tolerance for wall-clock drift.
    local createdIds = {}

    local function track(id)
      table.insert(createdIds, id)
      return id
    end

    local function assertClose(expected, actual, tolerance)
      tolerance = tolerance or 0.5
      assert.is_true(math.abs(expected - actual) <= tolerance,
        string.format("expected roughly %s but got %s", tostring(expected), tostring(actual)))
    end

    teardown(function()
      for _, id in ipairs(createdIds) do
        pcall(deleteStopWatch, id)
      end
      createdIds = {}
    end)

    describe("createStopWatch", function()
      it("returns a numeric id and autostarts by default", function()
        local id = track(createStopWatch())
        assert.is_number(id)
        local watches = getStopWatches()
        assert.is_table(watches[id])
        assert.is_true(watches[id].isRunning)
      end)

      it("does not autostart when given a name (string form)", function()
        local id = track(createStopWatch("stopwatchSpecNamed"))
        assert.is_number(id)
        assert.is_false(getStopWatches()[id].isRunning)
        assert.equals("stopwatchSpecNamed", getStopWatches()[id].name)
      end)

      it("honours an explicit autostart boolean", function()
        local id = track(createStopWatch(false))
        assert.is_false(getStopWatches()[id].isRunning)
      end)

      it("errors on an unsupported first argument type", function()
        assert.has_error(function() createStopWatch({}) end)
      end)

      it("refuses to create a second stopwatch with an existing name", function()
        track(createStopWatch("stopwatchSpecDuplicate"))
        local ok, err = createStopWatch("stopwatchSpecDuplicate")
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)

    describe("getStopWatchTime and adjustStopWatch", function()
      it("adjustStopWatch shifts the elapsed time of a stopped stopwatch deterministically", function()
        local id = track(createStopWatch(false))
        -- adjusting an as-yet-unstarted stopwatch initialises it at that value
        assert.is_true(adjustStopWatch(id, 12.5))
        assert.equals(12.5, getStopWatchTime(id))
        assert.is_true(adjustStopWatch(id, -2.5))
        assert.equals(10.0, getStopWatchTime(id))
      end)

      it("getStopWatchTime resolves a stopwatch by its name", function()
        local id = track(createStopWatch("stopwatchSpecByName"))
        adjustStopWatch(id, 5)
        assert.equals(5, getStopWatchTime("stopwatchSpecByName"))
      end)

      it("returns nil and a message for an unknown numeric id", function()
        local ok, err = getStopWatchTime(999999)
        assert.is_nil(ok)
        assert.is_string(err)
      end)

      it("returns nil and a message for an unknown name", function()
        local ok, err = getStopWatchTime("stopwatchSpecNoSuchName")
        assert.is_nil(ok)
        assert.is_string(err)
      end)

      it("errors when the first argument is neither number nor string", function()
        assert.has_error(function() getStopWatchTime({}) end)
      end)
    end)

    describe("start, stop and reset", function()
      it("stopStopWatch returns the elapsed time and freezes it", function()
        local id = track(createStopWatch(false))
        startStopWatch(id)
        adjustStopWatch(id, 7)
        local elapsed = stopStopWatch(id)
        assertClose(7, elapsed)
        assert.is_false(getStopWatches()[id].isRunning)
      end)

      it("resetStopWatch zeroes a stopped, initialised stopwatch", function()
        local id = track(createStopWatch(false))
        adjustStopWatch(id, 30)
        assert.is_true(resetStopWatch(id))
        assert.equals(0, getStopWatchTime(id))
      end)

      it("startStopWatch on an unknown id returns nil and a message", function()
        local ok, err = startStopWatch(888888)
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)

    describe("setStopWatchName", function()
      it("renames a stopwatch identified by id", function()
        local id = track(createStopWatch(false))
        assert.is_true(setStopWatchName(id, "stopwatchSpecRenamed"))
        assert.equals("stopwatchSpecRenamed", getStopWatches()[id].name)
        -- and it is now resolvable by the new name
        assert.is_number(getStopWatchTime("stopwatchSpecRenamed"))
      end)

      it("returns nil and a message when renaming an unknown id", function()
        local ok, err = setStopWatchName(777777, "stopwatchSpecNope")
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)

    describe("setStopWatchPersistence", function()
      it("marks a stopwatch persistent and this is reflected in getStopWatches", function()
        local id = track(createStopWatch(false))
        assert.is_false(getStopWatches()[id].isPersistent)
        assert.is_true(setStopWatchPersistence(id, true))
        assert.is_true(getStopWatches()[id].isPersistent)
        -- reset persistence so the stopwatch is never written to the profile
        assert.is_true(setStopWatchPersistence(id, false))
      end)

      it("returns nil and a message for an unknown id", function()
        local ok, err = setStopWatchPersistence(666666, true)
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)

    describe("getStopWatchBrokenDownTime", function()
      it("returns a table broken down into days/hours/minutes/seconds", function()
        local id = track(createStopWatch(false))
        -- 1 day, 2 hours, 3 minutes, 4 seconds
        adjustStopWatch(id, 4 + 3*60 + 2*3600 + 1*86400)
        local t = getStopWatchBrokenDownTime(id)
        assert.is_table(t)
        assert.equals(1, t.days)
        assert.equals(2, t.hours)
        assert.equals(3, t.minutes)
        assert.equals(4, t.seconds)
        assert.is_boolean(t.negative)
      end)

      it("flags negative elapsed time with the negative field", function()
        local id = track(createStopWatch(false))
        adjustStopWatch(id, -90) -- one minute thirty seconds in the past
        local t = getStopWatchBrokenDownTime(id)
        assert.is_true(t.negative)
        assert.equals(1, t.minutes)
        assert.equals(30, t.seconds)
      end)
    end)

    describe("deleteStopWatch", function()
      it("removes a stopwatch so it can no longer be read back", function()
        local id = createStopWatch(false)
        adjustStopWatch(id, 1)
        assert.is_table(getStopWatches()[id])
        assert.is_true(deleteStopWatch(id))
        assert.is_nil(getStopWatches()[id])
        local ok = getStopWatchTime(id)
        assert.is_nil(ok)
      end)

      it("returns nil and a message for an unknown id", function()
        local ok, err = deleteStopWatch(555555)
        assert.is_nil(ok)
        assert.is_string(err)
      end)
    end)

    describe("getStopWatches", function()
      it("reports name, running, persistent and elapsed time for each stopwatch", function()
        local id = track(createStopWatch("stopwatchSpecReport"))
        adjustStopWatch(id, 3)
        local entry = getStopWatches()[id]
        assert.equals("stopwatchSpecReport", entry.name)
        assert.is_boolean(entry.isRunning)
        assert.is_boolean(entry.isPersistent)
        assert.is_table(entry.elapsedTime)
        assert.equals(3, entry.elapsedTime.decimalSeconds)
      end)
    end)
  end)

  describe("Tests getConfig and setConfig round-trips", function()
    -- The supported option list is discovered from getConfig() at runtime rather
    -- than hard-coded, so this adapts to whichever build runs it. Every value
    -- changed here is restored (config is written to the profile on close). Keys
    -- with lossy or multi-valued representations (showSentText, the string enums
    -- and the numeric/table keys) are handled in dedicated tests below; map keys
    -- (which need an open mapper to set) are exercised only when settable.
    local originalValues = {}
    -- show3dMapView is skipped because flipping it to true opens the 3D OpenGL
    -- map view. Initialising the software GL stack under headless CI leaks
    -- one-time allocations in a GL driver module that is unloaded before exit,
    -- so leak detection flags it and it cannot be name-suppressed. The 3D view
    -- is not what this config round-trip is meant to exercise.
    local skipInGenericLoop = { showSentText = true, show3dMapView = true }

    local function snapshot(key)
      if originalValues[key] == nil then
        originalValues[key] = getConfig(key)
      end
    end

    local function restore(key)
      if originalValues[key] ~= nil then
        setConfig(key, originalValues[key])
        originalValues[key] = nil
      end
    end

    teardown(function()
      -- safety net for anything a failing assertion left changed
      for key, value in pairs(originalValues) do
        pcall(setConfig, key, value)
      end
      originalValues = {}
    end)

    it("returns a table of the current configuration when called with no arguments", function()
      local cfg = getConfig()
      assert.is_table(cfg)
      assert.is_boolean(cfg.enableGMCP)
      assert.is_boolean(cfg.editorAutoComplete)
    end)

    it("round-trips every boolean configuration option", function()
      local settable = 0
      for key, value in pairs(getConfig()) do
        if type(value) == "boolean" and not skipInGenericLoop[key] then
          snapshot(key)
          local ok = setConfig(key, not value)
          if ok then
            assert.equals(not value, getConfig(key), "round-trip failed for boolean config key: " .. key)
            settable = settable + 1
          else
            -- not settable in this environment (e.g. a map option with the
            -- mapper closed); the getter still returns a boolean
            assert.is_boolean(getConfig(key), "expected boolean for config key: " .. key)
          end
          restore(key)
        end
      end
      assert.is_true(settable > 0, "expected at least one settable boolean config option")
    end)

    it("round-trips the showSentText enum without losing the mode", function()
      -- getConfig(key, true) returns the string form; the boolean form collapses
      -- 'always' and 'script' both onto true, so restore using the string form.
      -- Register the string original in originalValues so the teardown safety net
      -- can restore it if an assertion below fails partway through.
      local original = getConfig("showSentText", true)
      assert.is_string(original)
      originalValues.showSentText = original
      for _, mode in ipairs({"never", "always", "script"}) do
        assert.is_true(setConfig("showSentText", mode))
        assert.equals(mode, getConfig("showSentText", true))
      end
      -- the legacy boolean form reads false only for 'never'
      assert.is_true(setConfig("showSentText", "never"))
      assert.is_false(getConfig("showSentText"))
      assert.is_true(setConfig("showSentText", "always"))
      assert.is_true(getConfig("showSentText"))
      setConfig("showSentText", original)
      assert.equals(original, getConfig("showSentText", true))
      originalValues.showSentText = nil
    end)

    it("round-trips the 2D map room symbol font", function()
      -- Unlike the other map keys these do not need an open mapper, because the
      -- settings live on the map object rather than the mapper widget.
      local original = getConfig("mapSymbolFont")
      assert.is_string(original)
      snapshot("mapSymbolFont")

      -- pick an installed font that is not the one in use, so the assertion
      -- cannot pass by doing nothing. pairs() has no defined order, so sort.
      local names = {}
      for name in pairs(getAvailableFonts()) do
        names[#names + 1] = name
      end
      table.sort(names)
      local otherFont
      for _, name in ipairs(names) do
        if name ~= original then
          otherFont = name
          break
        end
      end
      assert.is_string(otherFont)

      assert.is_true(setConfig("mapSymbolFont", otherFont))
      assert.equals(otherFont, getConfig("mapSymbolFont"))

      -- matched case-insensitively, read back as the font database spells it
      assert.is_true(setConfig("mapSymbolFont", otherFont:upper()))
      assert.equals(otherFont, getConfig("mapSymbolFont"))

      local ok, err = setConfig("mapSymbolFont", "No Such Font At All")
      assert.is_nil(ok)
      assert.is_string(err)
      assert.equals(otherFont, getConfig("mapSymbolFont"))

      restore("mapSymbolFont")
    end)

    it("round-trips the 2D map room symbol scaling factor", function()
      snapshot("mapSymbolFontScaling")
      assert.is_true(setConfig("mapSymbolFontScaling", 1.25))
      assert.equals(1.25, getConfig("mapSymbolFontScaling"))
      -- the ends of the range the preferences spin-box offers
      assert.is_true(setConfig("mapSymbolFontScaling", 0.50))
      assert.equals(0.50, getConfig("mapSymbolFontScaling"))
      assert.is_true(setConfig("mapSymbolFontScaling", 2.00))
      assert.equals(2.00, getConfig("mapSymbolFontScaling"))

      assert.is_true(setConfig("mapSymbolFontScaling", 1.10))
      -- NaN belongs in this list because it is the one value a plain range
      -- check does not stop: it compares false against both bounds. The
      -- infinities are here only to pin that they stay refused - the range
      -- check already handles those, and 0 is the value that would really do
      -- damage, blanking every room symbol.
      for _, value in ipairs({0.49, 2.01, -1, 0, 0/0, 1/0, -1/0}) do
        local ok, err = setConfig("mapSymbolFontScaling", value)
        assert.is_nil(ok, "setConfig accepted out-of-range value: " .. tostring(value))
        assert.is_string(err)
        assert.is_truthy(err:find("out of range", 1, true), err)
      end
      assert.equals(1.10, getConfig("mapSymbolFontScaling"))

      restore("mapSymbolFontScaling")
    end)

    -- The flag's whole effect on the font is the NoFontMerging style strategy
    -- bit, which is not visible from Lua at all - getConfig("mapSymbolFont")
    -- reports the family. So this only pins the round-trip; that setting a
    -- font afterwards does not silently drop the bit is pinned by
    -- test_pickingAFontKeepsTheOnlyUseSelectedStrategy in
    -- test/functional_tests/MapSymbolFontTest.cpp.
    it("round-trips the only-use-selected symbol font flag", function()
      snapshot("mapSymbolFontOnlyUseSelected")

      assert.is_true(setConfig("mapSymbolFontOnlyUseSelected", true))
      assert.is_true(getConfig("mapSymbolFontOnlyUseSelected"))

      assert.is_true(setConfig("mapSymbolFontOnlyUseSelected", false))
      assert.is_false(getConfig("mapSymbolFontOnlyUseSelected"))

      restore("mapSymbolFontOnlyUseSelected")
    end)

    -- The preferences have a whole dialog listing which room symbols the chosen
    -- font can draw; a script has only what setConfig hands back. The font is
    -- still taken, so the warning rides along with a true rather than
    -- replacing it.
    it("warns when the chosen symbol font cannot draw a symbol the map uses", function()
      snapshot("mapSymbolFont")

      -- U+10FFFD, the last codepoint of Private Use Plane 16. Nothing is
      -- assigned there, so no font on any machine this runs on has a glyph for
      -- it and the map is guaranteed to hold a symbol that cannot be drawn.
      local unrenderable = "\244\143\191\189"
      local roomId = createRoomID()
      assert.is_true(addRoom(roomId))
      assert.is_true(setRoomChar(roomId, unrenderable))

      local ok, warning = setConfig("mapSymbolFont", getConfig("mapSymbolFont"))
      assert.is_true(ok, "the font was refused outright rather than taken with a warning")
      assert.is_string(warning, "setConfig said nothing about a symbol that will show as the replacement character")
      assert.is_truthy(warning:find(unrenderable, 1, true), warning)

      -- and it stops saying so once nothing in the map needs that glyph
      assert.is_true(setRoomChar(roomId, "A"))
      local okAgain, warningAgain = setConfig("mapSymbolFont", getConfig("mapSymbolFont"))
      assert.is_true(okAgain)
      assert.is_falsy(warningAgain and warningAgain:find(unrenderable, 1, true), tostring(warningAgain))

      deleteRoom(roomId)
      restore("mapSymbolFont")
    end)

    it("round-trips the string enum options", function()
      local enums = {
        caretShortcut = {"none", "tab", "ctrltab", "f6"},
        blankLinesBehaviour = {"show", "hide", "replacewithspace"},
        controlCharacterHandling = {"asis", "oem", "picture"},
        ambiguousEAsianWidthCharacters = {"narrow", "wide", "auto"},
        mapperButton = {"scripted", "disabled", "default"},
      }
      local exercised = 0
      for key, values in pairs(enums) do
        if getConfig(key) ~= nil then
          snapshot(key)
          for _, value in ipairs(values) do
            assert.is_true(setConfig(key, value), "could not set " .. key .. " to " .. value)
            assert.equals(value, getConfig(key))
          end
          restore(key)
          exercised = exercised + 1
        end
      end
      assert.is_true(exercised > 0, "expected at least one string enum config option")
    end)

    it("errors on a wrongly typed value for a boolean option", function()
      -- setConfig defers to getVerifiedBool, which raises rather than silently
      -- coercing; the flag is never assigned so there is nothing to restore.
      assert.has_error(function() setConfig("enableGMCP", "not a boolean") end)
    end)

    it("rejects an invalid string for an enum option", function()
      snapshot("caretShortcut")
      local ok, err = setConfig("caretShortcut", "definitelyNotAValidShortcut")
      assert.is_nil(ok)
      assert.is_string(err)
      restore("caretShortcut")
    end)

    it("starts mapperButton on default each session and keeps the last good mode on a bad value", function()
      -- mapperButton is deliberately session-only (an uninstalled UI package
      -- must not leave the map button dead for good), so a fresh self-test
      -- profile has to read "default"
      snapshot("mapperButton")
      assert.equals("default", getConfig("mapperButton"))
      assert.is_true(setConfig("mapperButton", "scripted"))
      local ok, err = setConfig("mapperButton", "sideways")
      assert.is_nil(ok)
      assert.is_string(err)
      assert.equals("scripted", getConfig("mapperButton"))
      restore("mapperButton")
    end)

    it("round-trips commandLineHistorySaveSize (numeric option)", function()
      snapshot("commandLineHistorySaveSize")
      assert.is_true(setConfig("commandLineHistorySaveSize", 42))
      assert.equals(42, getConfig("commandLineHistorySaveSize"))
      restore("commandLineHistorySaveSize")
    end)

    it("validates the undoServerWrapWidth range when the option exists", function()
      if getConfig("undoServerWrapWidth") == nil then
        -- option not present in this build; setting it is rejected as unknown
        assert.is_nil((setConfig("undoServerWrapWidth", 42)))
        return
      end
      snapshot("undoServerWrapWidth")
      assert.is_true(setConfig("undoServerWrapWidth", 42))
      assert.equals(42, getConfig("undoServerWrapWidth"))
      assert.is_nil((setConfig("undoServerWrapWidth", 10)))  -- below the minimum of 20
      assert.is_nil((setConfig("undoServerWrapWidth", 600))) -- above the maximum of 500
      restore("undoServerWrapWidth")
    end)

    it("returns nil and a message for an unknown key", function()
      local value, message = getConfig("totallyBogusConfigKey")
      assert.is_nil(value)
      assert.is_string(message)
    end)

    it("setConfig returns nil and a message for an unknown key", function()
      local ok, message = setConfig("totallyBogusConfigKey", true)
      assert.is_nil(ok)
      assert.is_string(message)
    end)

    it("getConfig and setConfig reject an empty key", function()
      assert.is_nil((getConfig("")))
      assert.is_nil((setConfig("", true)))
    end)

    it("setConfig applies a table of options in one call", function()
      snapshot("enableGMCP")
      snapshot("editorAutoComplete")
      local target = not getConfig("enableGMCP")
      local target2 = not getConfig("editorAutoComplete")
      setConfig({ enableGMCP = target, editorAutoComplete = target2 })
      assert.equals(target, getConfig("enableGMCP"))
      assert.equals(target2, getConfig("editorAutoComplete"))
      restore("enableGMCP")
      restore("editorAutoComplete")
    end)

    it("getConfig returns a keyed table when given a list of keys", function()
      local result = getConfig({ "enableGMCP", "editorAutoComplete" })
      assert.is_table(result)
      assert.equals(getConfig("enableGMCP"), result.enableGMCP)
      assert.equals(getConfig("editorAutoComplete"), result.editorAutoComplete)
    end)
  end)

  describe("Tests the functionality of phpTable", function()
    -- Collects the entire iteration rather than a keyed table: a duplicated key
    -- and a key left behind with a nil value are both only visible in the
    -- sequence the pairs iterator yields.
    local function entries(t)
      local collected = {}
      for key, value in t:pairs() do
        collected[#collected + 1] = tostring(key) .. "=" .. tostring(value)
      end
      return collected
    end

    -- specs that assert iteration order seed one key at a time: the constructor
    -- walks its arguments with pairs(), whose order is unspecified and is not
    -- insertion order
    it("should return a table which contains the keys and values of the tables passed in", function()
      local t = phpTable({ one = "1", two = "2" })
      assert.equals("1", t.one)
      assert.equals("2", t.two)
    end)

    it("should merge multiple tables given as arguments, with later tables winning", function()
      local t = phpTable({ a = 1, b = 1 }, { b = 2 })
      assert.equals(1, t.a)
      assert.equals(2, t.b)
    end)

    it("should iterate over its elements in insertion order using its pairs method", function()
      local t = phpTable({ first = 1 })
      t.second = 2
      local collected = {}
      for key, value in t:pairs() do
        collected[#collected + 1] = tostring(key) .. "=" .. tostring(value)
      end
      assert.same({ "first=1", "second=2" }, collected)
    end)

    it("should yield a key holding false only once after it is overwritten", function()
      local t = phpTable({ flag = false })
      assert.is_false(t.flag)
      t.flag = true
      assert.same({ "flag=true" }, entries(t))
    end)

    it("should delete a key holding false when it is assigned nil", function()
      local t = phpTable({ flag = false })
      t.flag = nil
      assert.is_nil(t.flag)
      assert.same({}, entries(t))
    end)

    it("should not register a key that never existed when it is assigned nil", function()
      local t = phpTable({ kept = 1 })
      t.ghost = nil
      assert.is_nil(t.ghost)
      assert.same({ "kept=1" }, entries(t))
    end)

    it("should keep the original position of a key when its value is overwritten", function()
      local t = phpTable({ a = 1 })
      t.b = 2
      t.a = 3
      local collected = {}
      for key, value in t:pairs() do
        collected[#collected + 1] = tostring(key) .. "=" .. tostring(value)
      end
      assert.same({ "a=3", "b=2" }, collected)
    end)

    it("should remove a key from iteration when its value is set to nil", function()
      local t = phpTable({ a = 1 })
      t.b = 2
      t.b = nil
      assert.is_nil(t.b)
      local keys = {}
      for key in t:pairs() do
        keys[#keys + 1] = key
      end
      assert.same({ "a" }, keys)
    end)

    it("should iterate over no elements when called with no arguments", function()
      local t = phpTable()
      local count = 0
      for _ in t:pairs() do
        count = count + 1
      end
      assert.equals(0, count)
    end)

    it("should error if given a non-nil argument that is not a table", function()
      local errfn = function() phpTable("not a table") end
      assert.error_matches(errfn, "table expected")
    end)
  end)

    --[[
    TODO:
      remember()
      loadVars()
      saveVars()
      table.save()
      table.pickle()
      tacle.load()
      table.unpickle()
      getColorWildcard()
      lockExit()
      hasExitLock()
      registerAnonymousEventHandler()
      killAnonymousEventHandler()
      dispatchEventToFunctions()
      killtimeframe()
      translateTable()
  ]]
end)

describe("Tests the timer API", function()
  -- These drive the real timer engine: every effect assertion is made after the
  -- timer has actually fired, observed through the waitForEvent test helper which
  -- pumps the Qt event loop rather than sleeping.

  -- Temporary timers are killed and permanent ones disabled after every spec, so
  -- no timer created here can still fire during a later spec, even if one of the
  -- assertions above the clean-up code fails.
  --
  -- Permanent timers cannot be deleted, only disabled, and the profile is written
  -- out when Mudlet closes: running the suite twice against the same profile
  -- starts the second run with the first run's (disabled) permanent timers still
  -- in place. Specs below therefore count relative to what already exists rather
  -- than assuming the timer they just made is the only one of its name.
  local temporaryTimerIds = {}
  local permanentTimerNames = {}
  local settleCounter = 0

  local function trackTemp(id)
    if type(id) == "number" and id > 0 then
      table.insert(temporaryTimerIds, id)
    end
    return id
  end

  local function trackPerm(name)
    table.insert(permanentTimerNames, name)
    return name
  end

  -- Waits for one of this block's own events, reporting the timeout message
  -- instead of a bare nil when the event never arrives.
  local function waitFor(eventName)
    local name, message = waitForEvent(eventName, 5000)
    assert.equals(eventName, name, "waiting for " .. eventName .. ": " .. tostring(message))
    return name
  end

  -- Returns once `seconds` of real time have passed, by waiting for an event
  -- raised from a real timer. Lua runs to completion between event loop turns, so
  -- the timer cannot have fired before the wait below is in place. Each call gets
  -- its own event name so that a settling timer which outlived a failing spec can
  -- never satisfy a later one.
  local function settle(seconds)
    settleCounter = settleCounter + 1
    local eventName = "w2aTimerSpecSettled" .. settleCounter
    trackTemp(tempTimer(seconds, function() raiseEvent(eventName) end))
    waitFor(eventName)
  end

  before_each(function()
    _G.W2aTimerSpec = {fired = 0, order = {}}
  end)

  after_each(function()
    for _, id in ipairs(temporaryTimerIds) do
      pcall(killTimer, id)
    end
    temporaryTimerIds = {}
    for _, name in ipairs(permanentTimerNames) do
      pcall(disableTimer, name)
    end
    permanentTimerNames = {}
    _G.W2aTimerSpec = nil
    _G.W2aPermTimerFires = nil
  end)

  describe("Tests the functionality of tempTimer", function()
    it("errors when called without a delay", function()
      assert.has_error(function() tempTimer() end)
    end)

    it("errors when the delay is not a number", function()
      assert.has_error(function() tempTimer({}, [[]]) end)
    end)

    it("errors when the body is neither a string nor a function", function()
      assert.has_error(function() tempTimer(0.1, {}) end)
    end)

    it("returns -1 and a message when the code does not compile", function()
      -- the delay is distinctive, and the compiled chunk is named after the
      -- timer's numeric id, so nothing but a leak can put it in the message
      local id, message = tempTimer(0.125, "this is ( not lua")
      assert.equals(-1, id)
      assert.is_string(message, "the failure should come with a message")
      assert.is_truthy(message:find("compile", 1, true),
        "the failure should say the code could not be compiled, got: " .. tostring(message))
      -- and the reason has to be what Lua said about the code
      assert.is_truthy(message:find("near", 1, true),
        "the reason should be the Lua error, got: " .. tostring(message))
      assert.is_falsy(message:find("0.125", 1, true),
        "the delay must not be reported as the Lua error, got: " .. tostring(message))
    end)

    it("errors for a negative delay", function()
      -- an unguarded negative delay wraps around the 24 hour clock into a timer
      -- that fires almost a day later, so it has to be rejected outright
      assert.has_error(function() tempTimer(-1, [[]]) end)
      assert.has_error(function() tempTimer(-1, function() end) end)
    end)

    it("errors for a delay of a day or more, which wraps around to zero", function()
      assert.has_error(function() tempTimer(86400, [[]]) end)
    end)

    it("errors for a delay that only rounds up onto the day", function()
      -- the delay becomes the interval through qRound(time * 1000), so a delay
      -- of under 86400 seconds can still reach 86400000ms and wrap around to no
      -- interval at all: it is the rounded milliseconds that have to be bounded
      local ok, err = pcall(tempTimer, 86399.9999, [[]])
      assert.is_false(ok,
        "a delay rounding up to a whole day wraps to a zero interval and must be rejected")
      assert.is_truthy(tostring(err):find("bad argument #1", 1, true),
        "the delay should be reported as the offending argument, got: " .. tostring(err))
      local id = trackTemp(tempTimer(86399.4, [[]]))
      assert.is_true(id > 0, "a delay under the day once rounded should still be accepted")
      assert.is_true(killTimer(id))
    end)

    it("fires a code-string body in the global environment", function()
      trackTemp(tempTimer(0.05, [[
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
        raiseEvent("w2aTempTimerFired")
      ]]))
      waitFor("w2aTempTimerFired")
      assert.equals(1, _G.W2aTimerSpec.fired)
    end)

    it("fires a function body", function()
      trackTemp(tempTimer(0.05, function()
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
        raiseEvent("w2aTempTimerFunctionFired")
      end))
      waitFor("w2aTempTimerFunctionFired")
      assert.equals(1, _G.W2aTimerSpec.fired)
    end)

    it("fires a zero delay timer", function()
      trackTemp(tempTimer(0, function() raiseEvent("w2aZeroDelayTimerFired") end))
      waitFor("w2aZeroDelayTimerFired")
    end)

    it("does not repeat by default", function()
      -- a repeating 50ms timer would have fired several times over the 150ms below
      trackTemp(tempTimer(0.05, function()
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
      end))
      settle(0.15)
      assert.equals(1, _G.W2aTimerSpec.fired)
    end)

    it("errors when the repeating argument is not a boolean", function()
      assert.has_error(function() tempTimer(0.1, [[]], "w2aNotABoolean") end)
    end)

    it("repeats until killed when the repeating argument is true", function()
      local id = trackTemp(tempTimer(0.02, [[
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
        raiseEvent("w2aRepeatingTimerFired")
      ]], true))
      assert.is_true(id > 0)
      -- three separate waits, so each firing has to happen for itself
      waitFor("w2aRepeatingTimerFired")
      waitFor("w2aRepeatingTimerFired")
      waitFor("w2aRepeatingTimerFired")
      assert.is_true(killTimer(id))
      assert.is_true(_G.W2aTimerSpec.fired >= 3,
        "a repeating timer should fire more than once, fired " .. tostring(_G.W2aTimerSpec.fired) .. " times")
    end)

    it("runs a timer scheduled from inside another timer", function()
      trackTemp(tempTimer(0, function()
        table.insert(_G.W2aTimerSpec.order, "outer")
        trackTemp(tempTimer(0, function()
          table.insert(_G.W2aTimerSpec.order, "inner")
          raiseEvent("w2aNestedTimerFired")
        end))
      end))
      waitFor("w2aNestedTimerFired")
      assert.are.same({"outer", "inner"}, _G.W2aTimerSpec.order)
    end)
  end)

  describe("Tests the functionality of killTimer", function()
    -- killTimer looks its argument up by name; temporary timers are simply named
    -- after the id it returns, which is why passing that id works.
    it("errors when called without an argument", function()
      assert.has_error(function() killTimer() end)
    end)

    it("returns false when nothing of that name exists", function()
      assert.is_false(killTimer("w2aNoSuchTimerName"))
    end)

    it("returns false for a permanent timer, which cannot be killed", function()
      local before = exists("W2aPermTimerUnkillable", "timer")
      assert.is_true(permTimer(trackPerm("W2aPermTimerUnkillable"), "", 30, [[]]) > 0)
      assert.equals(before + 1, exists("W2aPermTimerUnkillable", "timer"))
      assert.is_false(killTimer("W2aPermTimerUnkillable"))
      assert.equals(before + 1, exists("W2aPermTimerUnkillable", "timer"),
        "a permanent timer survives killTimer")
    end)

    it("returns false the second time, as the timer is already dead", function()
      local id = trackTemp(tempTimer(10, [[]]))
      assert.is_true(killTimer(id))
      assert.is_false(killTimer(id),
        "killing an already killed timer achieves nothing and has to say so")
      -- the object itself is only freed by the timer unit's deferred cleanup,
      -- so check the state a user can see straight away instead
      assert.equals(0, isActive(id, "timer"), "a killed timer is no longer active")
      local left, message = remainingTime(id)
      assert.is_nil(left, "a killed timer is no longer counting down")
      -- "inactive" rather than "not a valid timerID" pins that the timer is
      -- still present and merely stopped, which is what the second kill saw
      assert.is_truthy(tostring(message):find("inactive", 1, true),
        "the killed timer should still be present but stopped, got: " .. tostring(message))
    end)

    it("returns false for a one-shot timer that has already fired", function()
      -- a fired one-shot temporary timer is queued for the same deferred cleanup
      -- a killed one is, so it is just as dead - which is what the manual has
      -- always said killTimer reports
      local id = trackTemp(tempTimer(0, function() raiseEvent("w2aOneShotFinished") end))
      waitFor("w2aOneShotFinished")
      assert.is_false(killTimer(id),
        "a one-shot timer that has already fired cannot be killed again")
    end)

    it("stops a pending timer from ever firing and deactivates it", function()
      local id = trackTemp(tempTimer(0.05, function()
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
      end))
      assert.equals(1, isActive(id, "timer"))
      assert.is_true(killTimer(id))
      -- the killed timer object itself is only freed by the timer unit's deferred
      -- cleanup, so check the state a user can see straight away instead
      assert.equals(0, isActive(id, "timer"), "a killed timer is no longer active")
      assert.is_nil((remainingTime(id)), "a killed timer is no longer counting down")
      settle(0.15)
      assert.equals(0, _G.W2aTimerSpec.fired, "a killed timer must never fire")
    end)

    it("stops a repeating timer", function()
      local id = trackTemp(tempTimer(0.02, function()
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
        if _G.W2aTimerSpec.fired == 2 then raiseEvent("w2aRepeatingTimerToKill") end
      end, true))
      waitFor("w2aRepeatingTimerToKill")
      assert.is_true(killTimer(id))
      local firedWhenKilled = _G.W2aTimerSpec.fired
      settle(0.15)
      assert.equals(firedWhenKilled, _G.W2aTimerSpec.fired,
        "a killed repeating timer must not fire again")
    end)

    it("kills a repeating timer from inside its own callback", function()
      -- killing a timer while its own body is running is the deferred-delete
      -- path: the timer unit may only free the object once the callback has
      -- returned, but the kill still has to stop it firing again
      local id
      id = trackTemp(tempTimer(0.02, function()
        _G.W2aTimerSpec.fired = _G.W2aTimerSpec.fired + 1
        _G.W2aTimerSpec.killed = killTimer(id)
        _G.W2aTimerSpec.killedAgain = killTimer(id)
        raiseEvent("w2aSelfKillingTimerFired")
      end, true))
      waitFor("w2aSelfKillingTimerFired")
      assert.is_true(_G.W2aTimerSpec.killed,
        "killTimer should report success from inside the timer's own callback")
      assert.is_false(_G.W2aTimerSpec.killedAgain,
        "killing the same timer twice from inside its own callback must fail the second time")
      local firedWhenKilled = _G.W2aTimerSpec.fired
      settle(0.15)
      assert.equals(firedWhenKilled, _G.W2aTimerSpec.fired,
        "a self-killed timer must not fire again")
    end)
  end)

  describe("Tests the functionality of remainingTime", function()
    it("errors when given something that is neither a number nor a string", function()
      assert.has_error(function() remainingTime({}) end)
    end)

    it("returns nil and a message for a number that is not a timer id", function()
      local left, message = remainingTime(999999)
      assert.is_nil(left)
      assert.is_string(message)
      assert.is_truthy(message:find("999999", 1, true))
    end)

    it("returns nil and a message for a name that is not a timer", function()
      local left, message = remainingTime("w2aNoSuchTimerName")
      assert.is_nil(left)
      assert.is_string(message)
      assert.is_truthy(message:find("w2aNoSuchTimerName", 1, true))
    end)

    it("returns nil and a message for an inactive timer", function()
      -- permanent timers are created inactive, so their QTimer is not running
      assert.is_true(permTimer(trackPerm("W2aPermTimerIdle"), "", 30, [[]]) > 0)
      local left, message = remainingTime("W2aPermTimerIdle")
      assert.is_nil(left)
      assert.is_string(message)
      assert.is_truthy(message:find("inactive", 1, true))
    end)

    it("reports the time left on a pending temporary timer in seconds", function()
      local id = trackTemp(tempTimer(10, [[]]))
      local left = remainingTime(id)
      assert.is_number(left)
      assert.is_true(left > 9 and left <= 10,
        "a 10 second timer should have just under 10 seconds left, got " .. tostring(left))
    end)

    it("resolves a running temporary timer by its name, which is its id", function()
      local id = trackTemp(tempTimer(10, [[]]))
      local left = remainingTime(tostring(id))
      assert.is_number(left)
      assert.is_true(left > 9 and left <= 10,
        "a 10 second timer should have just under 10 seconds left, got " .. tostring(left))
    end)

    it("resolves a running permanent timer by name", function()
      assert.is_true(permTimer(trackPerm("W2aPermTimerCountdown"), "", 30, [[]]) > 0)
      assert.is_true(enableTimer("W2aPermTimerCountdown"))
      local left = remainingTime("W2aPermTimerCountdown")
      assert.is_number(left)
      assert.is_true(left > 29 and left <= 30,
        "a 30 second timer should have just under 30 seconds left, got " .. tostring(left))
    end)
  end)

  describe("Tests the functionality of permTimer with enableTimer and disableTimer", function()
    it("errors when the parent group does not exist", function()
      assert.has_error(function()
        permTimer("W2aPermTimerOrphan", "w2aNoSuchTimerGroup", 1, [[]])
      end)
    end)

    it("errors when the code does not compile", function()
      local ok, err = pcall(permTimer, trackPerm("W2aPermTimerBadCode"), "", 1, "this is ( not lua")
      assert.is_false(ok, "code that does not parse must not create a timer")
      assert.is_truthy(tostring(err):find("near", 1, true),
        "the reason should be the Lua error, not the timer's name, got: " .. tostring(err))
    end)

    it("errors when the interval is missing", function()
      assert.has_error(function() permTimer("W2aPermTimerNoInterval", "") end)
    end)

    it("errors for a negative interval, creating nothing", function()
      -- counted rather than compared with zero: permanent timers survive into a
      -- second run of the suite against the same profile
      local before = exists("W2aPermTimerNegative", "timer")
      local ok, err = pcall(permTimer, trackPerm("W2aPermTimerNegative"), "", -1, [[]])
      assert.is_false(ok,
        "a negative interval must be rejected rather than wrapped around the 24 hour clock")
      assert.equals(before, exists("W2aPermTimerNegative", "timer"),
        "a rejected interval must not leave a timer behind")
      assert.is_truthy(tostring(err):find("bad argument #3", 1, true),
        "the interval should be reported as the offending argument, got: " .. tostring(err))
    end)

    it("errors for an interval that only rounds up onto the day, creating nothing", function()
      -- as with tempTimer, it is the rounded milliseconds that wrap: 86399.9999
      -- seconds is under the day but reaches it once rounded
      local before = exists("W2aPermTimerRounding", "timer")
      local ok, err = pcall(permTimer, trackPerm("W2aPermTimerRounding"), "", 86399.9999, [[]])
      assert.is_false(ok,
        "an interval rounding up to a whole day wraps to a zero interval and must be rejected")
      assert.equals(before, exists("W2aPermTimerRounding", "timer"),
        "a rejected interval must not leave a timer behind")
      assert.is_truthy(tostring(err):find("bad argument #3", 1, true),
        "the interval should be reported as the offending argument, got: " .. tostring(err))
    end)

    it("enableTimer and disableTimer error when called without a name", function()
      assert.has_error(function() enableTimer() end)
      assert.has_error(function() disableTimer() end)
    end)

    it("reports whether a timer of that name was found", function()
      assert.is_true(permTimer(trackPerm("W2aPermTimerToggle"), "", 30, [[]]) > 0)
      assert.is_true(enableTimer("W2aPermTimerToggle"))
      assert.is_true(disableTimer("W2aPermTimerToggle"))
      assert.is_false(enableTimer("w2aNoSuchTimerName"))
      assert.is_false(disableTimer("w2aNoSuchTimerName"))
    end)

    it("does not fire while it is disabled", function()
      -- permanent timers start out inactive and must be enabled before they run
      assert.is_true(permTimer(trackPerm("W2aPermTimerDisabled"), "", 0.05,
        [[_G.W2aPermTimerFires = (_G.W2aPermTimerFires or 0) + 1]]) > 0)
      assert.equals(0, isActive("W2aPermTimerDisabled", "timer"))
      settle(0.15)
      assert.is_nil(_G.W2aPermTimerFires, "a disabled permanent timer must not fire")
    end)

    it("fires once enabled", function()
      assert.is_true(permTimer(trackPerm("W2aPermTimerEnabled"), "", 0.05, [[
        _G.W2aPermTimerFires = (_G.W2aPermTimerFires or 0) + 1
        raiseEvent("w2aPermTimerFired")
      ]]) > 0)
      assert.is_true(enableTimer("W2aPermTimerEnabled"))
      waitFor("w2aPermTimerFired")
      disableTimer("W2aPermTimerEnabled")
      assert.is_true((_G.W2aPermTimerFires or 0) >= 1)
    end)

    it("stops firing again once disabled", function()
      assert.is_true(permTimer(trackPerm("W2aPermTimerStopped"), "", 0.05, [[
        _G.W2aPermTimerFires = (_G.W2aPermTimerFires or 0) + 1
        raiseEvent("w2aPermTimerStoppedFired")
      ]]) > 0)
      assert.is_true(enableTimer("W2aPermTimerStopped"))
      waitFor("w2aPermTimerStoppedFired")
      assert.is_true(disableTimer("W2aPermTimerStopped"))
      local firedWhenDisabled = _G.W2aPermTimerFires
      settle(0.15)
      assert.equals(firedWhenDisabled, _G.W2aPermTimerFires,
        "a disabled permanent timer must not fire again")
    end)
  end)

  describe("Tests exists and isActive for timers", function()
    it("both return nil and a message for an unknown item type", function()
      -- exists lowercases the type before quoting it back, so use a type that is
      -- lowercase to begin with and both messages can be checked the same way
      local existing, existsMessage = exists("W2aWhatever", "w2anotanitemtype")
      assert.is_nil(existing)
      assert.is_string(existsMessage)
      assert.is_truthy(existsMessage:find("w2anotanitemtype", 1, true))
      local active, isActiveMessage = isActive("W2aWhatever", "w2anotanitemtype")
      assert.is_nil(active)
      assert.is_string(isActiveMessage)
      assert.is_truthy(isActiveMessage:find("w2anotanitemtype", 1, true))
    end)

    it("both return nil and a message for a negative id", function()
      local existing, existsMessage = exists(-1, "timer")
      assert.is_nil(existing)
      assert.is_string(existsMessage)
      assert.is_truthy(existsMessage:find("-1", 1, true))
      local active, isActiveMessage = isActive(-1, "timer")
      assert.is_nil(active)
      assert.is_string(isActiveMessage)
      assert.is_truthy(isActiveMessage:find("-1", 1, true))
    end)

    it("exists counts a temporary timer by id and by name", function()
      local id = trackTemp(tempTimer(10, [[]]))
      -- a temporary timer is named after its own id
      assert.equals(1, exists(id, "timer"))
      assert.equals(1, exists(tostring(id), "timer"))
      assert.equals(0, exists(id + 100000, "timer"))
      assert.equals(0, exists("w2aNoSuchTimerName", "timer"))
    end)

    it("exists counts every permanent timer sharing a name", function()
      local before = exists("W2aPermTimerTwins", "timer")
      assert.is_true(permTimer(trackPerm("W2aPermTimerTwins"), "", 30, [[]]) > 0)
      assert.equals(before + 1, exists("W2aPermTimerTwins", "timer"))
      assert.is_true(permTimer(trackPerm("W2aPermTimerTwins"), "", 30, [[]]) > 0)
      assert.equals(before + 2, exists("W2aPermTimerTwins", "timer"))
    end)

    it("isActive follows the enabled state of a permanent timer", function()
      assert.is_true(permTimer(trackPerm("W2aPermTimerActivity"), "", 30, [[]]) > 0)
      -- enabling and disabling by name acts on every timer of that name
      local named = exists("W2aPermTimerActivity", "timer")
      assert.equals(0, isActive("W2aPermTimerActivity", "timer"),
        "a newly created permanent timer is inactive")
      assert.is_true(enableTimer("W2aPermTimerActivity"))
      assert.equals(named, isActive("W2aPermTimerActivity", "timer"))
      assert.is_true(disableTimer("W2aPermTimerActivity"))
      assert.equals(0, isActive("W2aPermTimerActivity", "timer"))
    end)

    it("isActive only reports a timer inside a disabled group as active when ancestors are not checked", function()
      -- a permTimer with no interval and no code is a group/folder
      assert.is_true(permTimer(trackPerm("W2aTimerGroup"), "", 0, "") > 0)
      assert.is_true(permTimer(trackPerm("W2aTimerInGroup"), "W2aTimerGroup", 30,
        [[_G.W2aPermTimerFires = (_G.W2aPermTimerFires or 0) + 1]]) > 0)
      local named = exists("W2aTimerInGroup", "timer")
      assert.is_true(enableTimer("W2aTimerInGroup"))
      assert.equals(named, isActive("W2aTimerInGroup", "timer"))
      assert.equals(0, isActive("W2aTimerInGroup", "timer", true),
        "the enclosing group is still disabled")
      -- and the flag is not the whole story: a timer whose group is disabled is
      -- not counting down, whatever isActive says without checkAncestors
      assert.is_nil((remainingTime("W2aTimerInGroup")),
        "a timer in a disabled group should not be running")
      assert.is_true(enableTimer("W2aTimerGroup"))
      assert.equals(named, isActive("W2aTimerInGroup", "timer", true))
      assert.is_number(remainingTime("W2aTimerInGroup"),
        "enabling the group should start the timers inside it")
    end)
  end)
end)

describe("Tests the script API", function()
  -- Permanent scripts cannot be removed from Lua, only blanked and disabled, and
  -- the profile is written out when Mudlet closes: running the suite twice against
  -- the same profile starts the second run with the first run's (empty, inactive)
  -- scripts still present. Specs below therefore work out the position of the
  -- script they just created instead of assuming it is the first of its name, and
  -- count relative to what was already there. Script bodies create their own table
  -- rather than assuming one exists, so a body that is recompiled later - when a
  -- saved profile is loaded again, say - can never raise.
  local createdScriptNames = {}

  -- Creates a permanent script, returning its id and its position among the
  -- scripts of that name, which is what getScript and setScript index by. New
  -- scripts get the highest id, so they come last.
  local function makeScript(name, parent, code)
    table.insert(createdScriptNames, name)
    local position = exists(name, "script") + 1
    return permScript(name, parent, code), position
  end

  before_each(function()
    _G.W2aScriptSpec = {}
  end)

  teardown(function()
    for _, name in ipairs(createdScriptNames) do
      pcall(disableScript, name)
      -- blank every script of that name, duplicates included, so that nothing
      -- created here can run again if the profile is saved and reloaded
      for position = 1, exists(name, "script") do
        pcall(setScript, name, "", position)
      end
    end
    createdScriptNames = {}
    _G.W2aScriptSpec = nil
  end)

  describe("Tests the functionality of permScript", function()
    it("errors when the name is missing", function()
      assert.has_error(function() permScript() end)
    end)

    it("errors when the parent group does not exist", function()
      assert.has_error(function()
        permScript("W2aScriptOrphan", "w2aNoSuchScriptGroup", [[]])
      end)
    end)

    it("errors when the code does not parse", function()
      assert.has_error(function()
        makeScript("W2aScriptBadCode", "", "this is ( not lua")
      end)
    end)

    it("creates nothing when the body parses but raises when it is run", function()
      -- a script's body runs as it is compiled, so a body that raises fails
      -- creation just like one that does not parse
      local before = exists("W2aScriptRaises", "script")
      -- the failure quotes the code it was given, so the message is built at run
      -- time: finding it whole proves the Lua error was reported, not the code
      -- that was handed in and not the script's name
      local ok, err = pcall(makeScript, "W2aScriptRaises", "", [[error("w2a script" .. " boom")]])
      assert.is_false(ok, "a body that raises must not create a script")
      assert.equals(before, exists("W2aScriptRaises", "script"))
      assert.is_truthy(tostring(err):find("w2a script boom", 1, true),
        "permScript should report the Lua error, got: " .. tostring(err))
    end)

    it("reports the type when the body raises something other than a string", function()
      -- the error object is not a string, so there is no message to quote - the
      -- reason still has to say what came back rather than name the script
      local before = exists("W2aScriptObjectError", "script")
      local ok, err = pcall(makeScript, "W2aScriptObjectError", "", [[error({w2a = true})]])
      assert.is_false(ok, "a body that raises must not create a script")
      assert.equals(before, exists("W2aScriptObjectError", "script"))
      assert.is_truthy(tostring(err):find("error object is a table", 1, true),
        "the reason should describe the error object, got: " .. tostring(err))
    end)

    it("creates a script whose body runs immediately", function()
      local before = exists("W2aScriptCreated", "script")
      local id = makeScript("W2aScriptCreated", "", [[
        _G.W2aScriptSpec = _G.W2aScriptSpec or {}
        _G.W2aScriptSpec.created = true
      ]])
      assert.is_number(id)
      assert.is_true(id > 0)
      assert.is_true(_G.W2aScriptSpec.created, "a script's body runs when it is compiled")
      assert.equals(before + 1, exists("W2aScriptCreated", "script"))
    end)

    it("creates a script inside a group", function()
      -- a permScript with no code is a group/folder
      assert.is_true(makeScript("W2aScriptGroup", "", "") > 0)
      assert.is_true(makeScript("W2aScriptInGroup", "W2aScriptGroup", [[
        _G.W2aScriptSpec = _G.W2aScriptSpec or {}
        _G.W2aScriptSpec.inGroup = true
      ]]) > 0)
      assert.is_true(_G.W2aScriptSpec.inGroup)
      local named = exists("W2aScriptInGroup", "script")
      assert.is_true(enableScript("W2aScriptInGroup"))
      assert.equals(named, isActive("W2aScriptInGroup", "script"))
      assert.equals(0, isActive("W2aScriptInGroup", "script", true),
        "the enclosing group is still disabled")
      assert.is_true(enableScript("W2aScriptGroup"))
      assert.equals(named, isActive("W2aScriptInGroup", "script", true))
    end)
  end)

  describe("Tests the functionality of getScript", function()
    it("errors when called without a name", function()
      assert.has_error(function() getScript() end)
    end)

    it("returns -1 and a message for a script that does not exist", function()
      local code, message = getScript("w2aNoSuchScriptName")
      assert.equals(-1, code)
      assert.is_string(message)
      assert.is_truthy(message:find("w2aNoSuchScriptName", 1, true))
    end)

    it("returns -1 and a message for a position that does not exist", function()
      local _, position = makeScript("W2aScriptOnePosition", "", [[local w2aOnly = 1]])
      local beyondTheLast = position + 1
      local code, message = getScript("W2aScriptOnePosition", beyondTheLast)
      assert.equals(-1, code)
      assert.is_string(message)
      assert.is_truthy(message:find("position " .. beyondTheLast, 1, true))
    end)

    it("returns -1 and a message for position zero, as positions start at one", function()
      makeScript("W2aScriptPositionZero", "", [[local w2aOnly = 1]])
      local code, message = getScript("W2aScriptPositionZero", 0)
      assert.equals(-1, code)
      assert.is_string(message)
      assert.is_truthy(message:find("position 0", 1, true))
    end)

    it("returns the code and the id of the script", function()
      local body = [[local w2aReadBack = "getScript round trip"]]
      local id, position = makeScript("W2aScriptReadBack", "", body)
      local code, readId = getScript("W2aScriptReadBack", position)
      assert.equals(body, code)
      assert.equals(id, readId)
    end)

    it("reads the script at the requested position when several share a name", function()
      local firstId, firstPosition = makeScript("W2aScriptDuplicate", "", [[local w2aFirst = 1]])
      local secondId, secondPosition = makeScript("W2aScriptDuplicate", "", [[local w2aSecond = 2]])
      assert.equals(firstPosition + 1, secondPosition)
      local firstCode, firstReadId = getScript("W2aScriptDuplicate", firstPosition)
      local secondCode, secondReadId = getScript("W2aScriptDuplicate", secondPosition)
      assert.equals([[local w2aFirst = 1]], firstCode)
      assert.equals(firstId, firstReadId)
      assert.equals([[local w2aSecond = 2]], secondCode)
      assert.equals(secondId, secondReadId)
    end)
  end)

  describe("Tests the functionality of setScript", function()
    it("errors for a script name that does not exist", function()
      assert.has_error(function() setScript("w2aNoSuchScriptName", [[]]) end)
    end)

    it("errors for an empty name", function()
      assert.has_error(function() setScript("", [[]]) end)
    end)

    it("errors for position zero, as positions start at one", function()
      makeScript("W2aScriptSetPositionZero", "", [[local w2aOnly = 1]])
      assert.has_error(function()
        setScript("W2aScriptSetPositionZero", [[local w2aChanged = 1]], 0)
      end)
    end)

    it("errors when the code is not a string", function()
      local _, position = makeScript("W2aScriptBadNewCode", "", [[local w2aOriginal = 1]])
      assert.has_error(function() setScript("W2aScriptBadNewCode", {}, position) end)
    end)

    it("replaces the code, returns the id and runs the new body", function()
      local id, position = makeScript("W2aScriptReplaced", "", [[local w2aOriginal = 1]])
      local newBody = [[
        _G.W2aScriptSpec = _G.W2aScriptSpec or {}
        _G.W2aScriptSpec.replaced = true
      ]]
      assert.equals(id, setScript("W2aScriptReplaced", newBody, position))
      assert.equals(newBody, (getScript("W2aScriptReplaced", position)))
      assert.is_true(_G.W2aScriptSpec.replaced, "the replacement body should have run")
    end)

    it("rejects code that does not parse before touching the script", function()
      -- this one never reaches the script: setScript syntax checks its code
      -- argument first
      local body = [[local w2aKept = 1]]
      local _, position = makeScript("W2aScriptKeptCode", "", body)
      assert.has_error(function() setScript("W2aScriptKeptCode", "this is ( not lua", position) end)
      assert.equals(body, (getScript("W2aScriptKeptCode", position)))
    end)

    it("puts the previous code back when the new body raises as it is run", function()
      -- code that parses gets past the argument check and is then run as it is
      -- compiled into the script, so this is the path that has to roll back
      local body = [[local w2aRolledBack = 1]]
      local _, position = makeScript("W2aScriptRollback", "", body)
      -- as in the permScript spec above, the raised message is assembled at run
      -- time so that only the real Lua error can contain it
      local ok, err = pcall(setScript, "W2aScriptRollback", [[error("w2a setScript" .. " boom")]], position)
      assert.is_false(ok, "a body that raises must not be kept")
      assert.equals(body, (getScript("W2aScriptRollback", position)))
      assert.is_truthy(tostring(err):find("w2a setScript boom", 1, true),
        "setScript should report the Lua error, got: " .. tostring(err))
    end)

    it("sets the script at the requested position when several share a name", function()
      local _, firstPosition = makeScript("W2aScriptSetPosition", "", [[local w2aFirst = 1]])
      local secondId, secondPosition = makeScript("W2aScriptSetPosition", "", [[local w2aSecond = 2]])
      assert.equals(secondId, setScript("W2aScriptSetPosition", [[local w2aSecondChanged = 2]], secondPosition))
      assert.equals([[local w2aFirst = 1]], (getScript("W2aScriptSetPosition", firstPosition)))
      assert.equals([[local w2aSecondChanged = 2]], (getScript("W2aScriptSetPosition", secondPosition)))
    end)
  end)

  describe("Tests the functionality of appendScript", function()
    it("errors when the name is not a string", function()
      assert.has_error(function() appendScript(42, [[]]) end,
        "appendScript: bad argument #1 type (script name as string expected, got number!)")
    end)

    it("errors when the code is not a string", function()
      assert.has_error(function() appendScript("W2aScriptAppended", 42) end,
        "appendScript: bad argument #2 type (lua code as string expected, got number!)")
    end)

    it("errors instead of creating anything when the script does not exist", function()
      -- appendScript does not check getScript's -1 sentinel, so what actually
      -- reports the missing script is the setScript underneath it; either way
      -- nothing may be created
      assert.has_error(function() appendScript("w2aNoSuchScriptName", [[local w2aNew = 1]]) end)
      assert.equals(0, exists("w2aNoSuchScriptName", "script"))
    end)

    it("adds the new code on a line of its own after the existing code", function()
      local body = [[local w2aOriginal = 1]]
      local _, position = makeScript("W2aScriptAppended", "", body)
      appendScript("W2aScriptAppended", [[local w2aAppended = 2]], position)
      assert.equals(body .. "\n" .. [[local w2aAppended = 2]],
        (getScript("W2aScriptAppended", position)))
    end)

    it("appends to the first script of that name when no position is given", function()
      makeScript("W2aScriptAppendDefault", "", [[local w2aOriginal = 1]])
      -- whatever is at position 1 is what the default has to append to
      local firstBefore = (getScript("W2aScriptAppendDefault", 1))
      assert.is_string(firstBefore)
      appendScript("W2aScriptAppendDefault", [[local w2aDefaultAppended = 2]])
      assert.equals(firstBefore .. "\n" .. [[local w2aDefaultAppended = 2]],
        (getScript("W2aScriptAppendDefault", 1)))
    end)

    it("runs the appended code", function()
      local _, position = makeScript("W2aScriptAppendRuns", "", [[local w2aOriginal = 1]])
      appendScript("W2aScriptAppendRuns", [[
        _G.W2aScriptSpec = _G.W2aScriptSpec or {}
        _G.W2aScriptSpec.appended = true
      ]], position)
      assert.is_true(_G.W2aScriptSpec.appended)
    end)
  end)

  describe("Tests the functionality of enableScript and disableScript", function()
    it("error when called without a name", function()
      assert.has_error(function() enableScript() end)
      assert.has_error(function() disableScript() end)
    end)

    it("return nil and a message when no script of that name exists", function()
      local enabled, enableMessage = enableScript("w2aNoSuchScriptName")
      assert.is_nil(enabled)
      assert.is_string(enableMessage)
      assert.is_truthy(enableMessage:find("w2aNoSuchScriptName", 1, true))
      local disabled, disableMessage = disableScript("w2aNoSuchScriptName")
      assert.is_nil(disabled)
      assert.is_string(disableMessage)
      assert.is_truthy(disableMessage:find("w2aNoSuchScriptName", 1, true))
    end)

    it("toggle the active state a script reports through isActive", function()
      assert.is_true(makeScript("W2aScriptToggled", "", [[local w2aOnly = 1]]) > 0)
      -- enabling and disabling by name acts on every script of that name
      local named = exists("W2aScriptToggled", "script")
      assert.equals(0, isActive("W2aScriptToggled", "script"),
        "a newly created script is inactive")
      assert.is_true(enableScript("W2aScriptToggled"))
      assert.equals(named, isActive("W2aScriptToggled", "script"))
      assert.is_true(disableScript("W2aScriptToggled"))
      assert.equals(0, isActive("W2aScriptToggled", "script"))
    end)

    it("toggle every script sharing a name, not just the first", function()
      assert.is_true(makeScript("W2aScriptToggledTwice", "", [[local w2aFirst = 1]]) > 0)
      assert.is_true(makeScript("W2aScriptToggledTwice", "", [[local w2aSecond = 2]]) > 0)
      local named = exists("W2aScriptToggledTwice", "script")
      assert.is_true(named >= 2)
      assert.is_true(enableScript("W2aScriptToggledTwice"))
      assert.equals(named, isActive("W2aScriptToggledTwice", "script"))
      assert.is_true(disableScript("W2aScriptToggledTwice"))
      assert.equals(0, isActive("W2aScriptToggledTwice", "script"))
    end)

    it("do not disturb a differently named script", function()
      assert.is_true(makeScript("W2aScriptUntouched", "", [[local w2aOne = 1]]) > 0)
      assert.is_true(makeScript("W2aScriptSwitched", "", [[local w2aTwo = 2]]) > 0)
      local untouched = exists("W2aScriptUntouched", "script")
      assert.is_true(enableScript("W2aScriptUntouched"))
      assert.is_true(enableScript("W2aScriptSwitched"))
      assert.is_true(disableScript("W2aScriptSwitched"))
      assert.equals(untouched, isActive("W2aScriptUntouched", "script"))
      assert.equals(0, isActive("W2aScriptSwitched", "script"))
    end)
  end)

  describe("Tests the functionality of speedwalktimer", function()
    -- resume first so a paused walk is re-armed and stopSpeedwalk can then
    -- clear its walklist; both are shared upvalues of Other.lua
    after_each(function()
      pcall(resumeSpeedwalk)
      pcall(stopSpeedwalk)
    end)

    it("Should send the head of the walklist and shorten it", function()
      local list = {"n", "e"}
      local send = spy.on(_G, "send")
      finally(function() send:revert() end)
      speedwalktimer(list, 100, false)
      assert.spy(send).was.called(1)
      assert.spy(send).was.called_with("n", false)
      assert.are.same({"e"}, list)
    end)

    it("Should arm a timer for the rest of the walklist", function()
      local list = {"n", "e"}
      local send = spy.on(_G, "send")
      finally(function() send:revert() end)
      speedwalktimer(list, 100, false)
      -- pauseSpeedwalk only succeeds while a step timer is armed
      assert.is_true(pauseSpeedwalk())
    end)

    it("Should raise sysSpeedwalkFinished on the last step", function()
      local finished = false
      local handler = registerAnonymousEventHandler("sysSpeedwalkFinished", function() finished = true end)
      finally(function() killAnonymousEventHandler(handler) end)
      local send = spy.on(_G, "send")
      finally(function() send:revert() end)
      -- clear any step timer an earlier test armed so the pause check below
      -- can only be answering for this walklist
      pcall(pauseSpeedwalk)
      local list = {"n"}
      speedwalktimer(list, 100, false)
      assert.spy(send).was.called_with("n", false)
      assert.are.same({}, list)
      assert.is_true(finished)
      -- nothing was queued, so there is no timer left to pause
      assert.is_nil((pauseSpeedwalk()))
    end)
  end)

  describe("Tests the functionality of deleteFull", function()
    after_each(function()
      -- deleteFull leaves a one line trigger behind; flush it so it cannot
      -- gag a line belonging to a later spec
      feedTriggers("deleteFullFlush\n")
    end)

    it("Should delete the line it runs on", function()
      local id = tempTrigger("deleteFullMarker", function() deleteFull() end)
      feedTriggers("deleteFullMarker line\n")
      killTrigger(id)
      moveCursorEnd()
      moveCursorUp()
      assert.are_not.equal("deleteFullMarker line", getCurrentLine())
    end)

    it("Should arm a one line trigger that gags a following prompt", function()
      local lineTrigger = spy.on(_G, "tempLineTrigger")
      finally(function() lineTrigger:revert() end)
      local id = tempTrigger("deleteFullArmMarker", function() deleteFull() end)
      feedTriggers("deleteFullArmMarker line\n")
      killTrigger(id)
      assert.spy(lineTrigger).was.called(1)
      assert.spy(lineTrigger).was.called_with(1, 1, [[if isPrompt() then deleteLine() end]])
    end)
  end)

  describe("Tests the functionality of condenseMapLoad", function()
    before_each(function()
      clearWindow()
      moveCursorEnd()
    end)

    it("Should delete the map loading block and return the time it took", function()
      echo("[ INFO ]  - Reading map. Please wait...\n")
      echo("[ INFO ]  - Map read in 1.5s.\n")
      echo("[ INFO ]  - Map deserialised in 0.25s.\n")
      local loadTime = condenseMapLoad()
      assert.are.equal(1.75, loadTime)
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_falsy(text:find("Reading map", 1, true))
      assert.is_falsy(text:find("deserialised", 1, true))
    end)

    it("Should refuse to condense when the user must see an alert", function()
      echo("[ INFO ]  - Reading map. Please wait...\n")
      echo("[ ALERT ] - something the user has to read\n")
      local loadTime, err = condenseMapLoad()
      assert.is_nil(loadTime)
      assert.are.equal("an alert, warning, or error that the user must see is present", err)
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_truthy(text:find("something the user has to read", 1, true))
    end)

    it("Should report when there is no map load output to condense", function()
      echo("nothing to do with maps at all\n")
      local loadTime, err = condenseMapLoad()
      assert.is_nil(loadTime)
      assert.are.equal("couldn't find the starting line for map load output", err)
    end)
  end)

  describe("Tests the functionality of loadTranslations", function()
    it("Should return the strings of the package it is asked for", function()
      local translations = loadTranslations("AdjustableContainer")
      assert.is_table(translations)
      assert.is_table(translations.attach)
      assert.is_string(translations.attach.message)
      assert.is_truthy(translations.top and translations.bottom and translations.left and translations.right)
    end)

    it("Should strip the package prefix off every key", function()
      local translations = loadTranslations("AdjustableContainer")
      for key in pairs(translations) do
        assert.is_falsy(key:find("AdjustableContainer.", 1, true))
      end
    end)

    it("Should report a package the translation file has no strings for", function()
      local translations, err = loadTranslations("NoSuchPackageInTheTranslationFile")
      assert.is_nil(translations)
      assert.are.equal("couldn't find translations for 'NoSuchPackageInTheTranslationFile'", err)
    end)

    it("Should report a translation file it cannot find", function()
      local translations, err = loadTranslations("AdjustableContainer", "noSuchTranslationFile")
      assert.is_nil(translations)
      assert.is_truthy(err:find("unable to find 'noSuchTranslationFile.json'", 1, true))
    end)

    it("Should reject arguments of the wrong type", function()
      assert.has_error(function() loadTranslations(5) end)
      assert.has_error(function() loadTranslations("AdjustableContainer", 5) end)
      assert.has_error(function() loadTranslations("AdjustableContainer", "mudlet-lua", 5) end)
    end)
  end)

  describe("Tests the functionality of onConnect", function()
    -- defined in LuaGlobal.lua as an empty default users may override
    it("Should exist and do nothing", function()
      assert.are.equal("function", type(onConnect))
      assert.are.same({}, {onConnect()})
    end)
  end)
end)
