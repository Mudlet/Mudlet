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

    it("round-trips the string enum options", function()
      local enums = {
        caretShortcut = {"none", "tab", "ctrltab", "f6"},
        blankLinesBehaviour = {"show", "hide", "replacewithspace"},
        controlCharacterHandling = {"asis", "oem", "picture"},
        ambiguousEAsianWidthCharacters = {"narrow", "wide", "auto"},
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

    --[[
    TODO:
      remember()
      loadVars()
      saveVars()
      table.save()
      table.pickle()
      tacle.load()
      table.unpickle()
      phpTable()
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
