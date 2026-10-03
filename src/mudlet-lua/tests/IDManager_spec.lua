describe("Tests the functionality of IDMgr", function()
  describe("Test the event mgr functionality", function()
    local RESpy
    local KESpy
    local handlerSpy
    local eventName = "testEvent"
    local handlerName = "tester"
    local user = "test user"
    local func
    before_each(function()
      RESpy = spy.on(_G, "registerAnonymousEventHandler")
      KESpy = spy.on(_G, "killAnonymousEventHandler")
      handlerSpy = spy.new(function() end)
      func = function() handlerSpy() end
    end)
    after_each(function()
      registerAnonymousEventHandler:revert()
      killAnonymousEventHandler:revert()
      handlerSpy = nil
      func = nil
      deleteAllNamedEventHandlers(user)
    end)
    describe("Tests the functionality of registerNamedEventHandler", function()
      it("Should register an event handler", function()
        local ok = registerNamedEventHandler(user, handlerName, eventName, func)
        assert.is_true(ok)
        assert.spy(RESpy).was_called(1)
        assert.spy(RESpy).was_called_with(eventName, func, false)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(1)
      end)

      it("Should kill the old handler and reregister a new one if you register to the same name more than once", function()
        local ok = registerNamedEventHandler(user, handlerName, eventName, func)
        assert.is_true(ok)
        ok = registerNamedEventHandler(user, handlerName, eventName, func)
        assert.is_true(ok)
        assert.spy(RESpy).was_called(2)
        assert.spy(RESpy).was_called_with(eventName, func, false)
        assert.spy(KESpy).was_called(1)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(1)
      end)

      it("Should enforce separation between users/packages", function()
        local user2 = user.."2"
        local handlerName2 = handlerName .. "2"
        registerNamedEventHandler(user, handlerName, eventName, func)
        registerNamedEventHandler(user2, handlerName2, eventName, func)
        local handlerList = getNamedEventHandlers(user)
        local handlerList2 = getNamedEventHandlers(user2)
        assert.is_equal(1, #handlerList)
        assert.is_equal(1, #handlerList2)
        assert.is_not.same(handlerList, handlerList2)
        assert.is_equal(handlerName, handlerList[1])
        assert.is_equal(handlerName2, handlerList2[1])
        deleteAllNamedEventHandlers(user2)
      end)
    end)

    describe("Tests the functionality of stopNamedEventHandler", function()
      it("Should allow for you to stop a handler", function()
        registerNamedEventHandler(user, handlerName, eventName, func)
        local ok = stopNamedEventHandler(user, handlerName)
        assert.is_true(ok)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_not_called()
      end)
    end)

    describe("Tests the functionality of stopNamedEventHandler", function()
      it("Should allow you to resume a stopped handler", function()
        registerNamedEventHandler(user, handlerName, eventName, func)
        stopNamedEventHandler(user, handlerName)
        raiseEvent(eventName)
        local ok = resumeNamedEventHandler(user, handlerName)
        assert.is_true(ok)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(1)
      end)
    end)

    describe("Tests the functionality of getNamedEventHandlers", function()
      it("Should provide a list of registered named handlers", function()
        registerNamedEventHandler(user, handlerName, eventName, func)
        local handlers = getNamedEventHandlers(user)
        assert.are.same({handlerName}, handlers)
      end)
    end)

    describe("Tests the functionality of deleteNamedEventHandler", function()
      it("Should allow for deleting a handler entirely", function()
        registerNamedEventHandler(user, handlerName, eventName, func)
        deleteNamedEventHandler(user, handlerName)
        local handlers = getNamedEventHandlers(user)
        assert.are.same(handlers, {})
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_not_called()
      end)
    end)

    describe("Tests the functionality of stopAllNamedEventHandlers", function()
      it("Should stop all handlers when asked", function()
        local handlerName2 = handlerName .. "2"
        registerNamedEventHandler(user, handlerName, eventName, func)
        registerNamedEventHandler(user, handlerName2, eventName, func)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(2)
        stopAllNamedEventHandlers(user)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(2) -- remains at 2
        resumeNamedEventHandler(user, handlerName)
        resumeNamedEventHandler(user, handlerName2)
        raiseEvent(eventName)
        assert.spy(handlerSpy).was_called(4)
      end)
    end)

    describe("Tests the functionality of deleteAllNamedEventHandlers()", function()
      it("Should delete all handlers when asked", function()
        local handlerName2 = handlerName .. "2"
        registerNamedEventHandler(user, handlerName, eventName, func)
        registerNamedEventHandler(user, handlerName2, eventName, func)
        local handlers_before = getNamedEventHandlers(user)
        assert.are.same({handlerName, handlerName2}, handlers_before)
        deleteAllNamedEventHandlers(user)
        local handlers_after = getNamedEventHandlers(user)
        assert.are.same({}, handlers_after)
      end)
    end)

    it("Should raise an error if the handlerName is missing or wrong type", function()
      local reg = function()
        registerNamedEventHandler(user)
      end
      local stop = function()
        stopNamedEventHandler(user)
      end
      local resume = function()
        resumeNamedEventHandler(user)
      end
      local delete = function()
        deleteNamedEventHandler(user)
      end
      assert.error_matches(reg, "bad argument #2 type")
      assert.error_matches(stop, "bad argument #2 type")
      assert.error_matches(resume, "bad argument #2 type")
      assert.error_matches(delete, "bad argument #2 type")
    end)

    it("Should raise an error if the userName is missing or wrong type", function()
      local reg = function()
        registerNamedEventHandler()
      end
      local stop = function()
        stopNamedEventHandler()
      end
      local resume = function()
        resumeNamedEventHandler()
      end
      local delete = function()
        deleteNamedEventHandler()
      end
      assert.error_matches(reg, "bad argument #1 type")
      assert.error_matches(stop, "bad argument #1 type")
      assert.error_matches(resume, "bad argument #1 type")
      assert.error_matches(delete, "bad argument #1 type")
    end)

    it("Should consume and pass along the modified error message on error", function()
      local exec = function()
        registerNamedEventHandler(user, handlerName, eventName)
      end
      local exec2 = function()
        registerNamedEventHandler(user, handlerName)
      end

      assert.error_matches(exec, "registerNamedEventHandler: bad argument #4 type")
      assert.error_matches(exec2, "registerNamedEventHandler: bad argument #3 type")
    end)
  end)

  -- The synchronous named-timer lifecycle (register/reset/stop/resume/delete and
  -- their -All variants) is covered below. Timer *firing* still needs an async
  -- busted harness and is not exercised here. https://github.com/Mudlet/Mudlet/issues/5520
  describe("Tests the timer functionality", function()
    local user = "test user"
    local timerName = "test timer"
    -- long interval so the underlying tempTimer never fires during the test:
    -- these specs cover the synchronous lifecycle only, not firing
    local time = 100

    -- clean up the shared per-user manager after each lifecycle test so a stray
    -- named timer can't leak into later specs (getNewIDManager tests clean their
    -- own private manager inline).
    after_each(function()
      deleteAllNamedTimers(user)
    end)

    describe("Tests registering, stopping and deleting named timers", function()
      it("Should register a named timer and list it as active", function()
        local ok = registerNamedTimer(user, timerName, time, function() end)
        assert.is_true(ok)
        assert.are.same({timerName}, getNamedTimers(user))
        local remaining = remainingNamedTimer(user, timerName)
        assert.is_number(remaining)
        assert.is_true(remaining > 0)
      end)

      it("Should reset a named timer when it is registered a second time", function()
        -- behaviour-based: the second (shorter) registration must replace the
        -- first, observable through the remaining time dropping to the new value
        registerNamedTimer(user, timerName, 5000, function() end)
        local firstRemaining = remainingNamedTimer(user, timerName)
        assert.is_number(firstRemaining)
        assert.is_true(firstRemaining > 100, "the first registration should leave a large remaining time")

        registerNamedTimer(user, timerName, 50, function() end)
        -- one entry, not two, under the reused name
        assert.are.same({timerName}, getNamedTimers(user))
        local secondRemaining = remainingNamedTimer(user, timerName)
        assert.is_number(secondRemaining)
        assert.is_true(secondRemaining <= 50, "re-registering must replace the timer with the new, shorter one")
      end)

      it("Should stop a named timer while leaving it registered", function()
        registerNamedTimer(user, timerName, time, function() end)
        local ok = stopNamedTimer(user, timerName)
        assert.is_true(ok)
        -- stopped, so no time remains...
        local remaining, err = remainingNamedTimer(user, timerName)
        assert.is_nil(remaining)
        assert.is_equal("timer is inactive", err)
        -- ...but a stopped timer is still registered (stop is not delete)
        assert.are.same({timerName}, getNamedTimers(user))
      end)

      it("Should return false when stopping a timer that is not registered", function()
        assert.is_false(stopNamedTimer(user, "no such timer"))
      end)

      it("Should return false when resuming or deleting an unregistered timer", function()
        assert.is_false(resumeNamedTimer(user, "no such timer"))
        assert.is_false(deleteNamedTimer(user, "no such timer"))
      end)

      it("Should resume a stopped named timer", function()
        registerNamedTimer(user, timerName, time, function() end)
        stopNamedTimer(user, timerName)
        local ok = resumeNamedTimer(user, timerName)
        assert.is_true(ok)
        local remaining = remainingNamedTimer(user, timerName)
        assert.is_number(remaining)
        assert.is_true(remaining > 0)
      end)

      it("Should stop all named timers for a user", function()
        local timerName2 = timerName .. "2"
        registerNamedTimer(user, timerName, time, function() end)
        registerNamedTimer(user, timerName2, time, function() end)

        assert.is_true(stopAllNamedTimers(user))

        local _, err1 = remainingNamedTimer(user, timerName)
        local _, err2 = remainingNamedTimer(user, timerName2)
        assert.is_equal("timer is inactive", err1)
        assert.is_equal("timer is inactive", err2)
        -- stopped, not deleted: both remain registered
        assert.is_equal(2, #getNamedTimers(user))
      end)

      it("Should delete a single named timer", function()
        local timerName2 = timerName .. "2"
        registerNamedTimer(user, timerName, time, function() end)
        registerNamedTimer(user, timerName2, time, function() end)

        assert.is_true(deleteNamedTimer(user, timerName))

        assert.are.same({timerName2}, getNamedTimers(user))
        local remaining, err = remainingNamedTimer(user, timerName)
        assert.is_nil(remaining)
        assert.is_equal("timer not found", err)
      end)

      it("Should delete all named timers for a user", function()
        registerNamedTimer(user, timerName, time, function() end)
        registerNamedTimer(user, timerName .. "2", time, function() end)
        assert.is_equal(2, #getNamedTimers(user))

        assert.is_true(deleteAllNamedTimers(user))

        assert.are.same({}, getNamedTimers(user))
      end)
    end)

    describe("Tests the functionality of remainingNamedTimer", function()
      after_each(function()
        deleteAllNamedTimers(user)
      end)

      it("Should return a number for an active named timer", function()
        registerNamedTimer(user, timerName, time, function() end)
        local remaining = remainingNamedTimer(user, timerName)
        assert.is_number(remaining)
        assert.is_true(remaining > 0)
        assert.is_true(remaining <= time)
      end)

      it("Should return nil and a message for an unknown timer name", function()
        local result, err = remainingNamedTimer(user, "nonexistent timer")
        assert.is_nil(result)
        assert.is_equal("timer not found", err)
      end)

      it("Should return nil and a message for a stopped named timer", function()
        registerNamedTimer(user, timerName, time, function() end)
        stopNamedTimer(user, timerName)
        local result, err = remainingNamedTimer(user, timerName)
        assert.is_nil(result)
        assert.is_equal("timer is inactive", err)
      end)

      it("Should return nil and a message when the underlying timer is gone", function()
        local mgr = getNewIDManager()
        mgr:registerTimer(timerName, time, function() end)
        -- kill the tempTimer behind the manager's back, as happens when a
        -- one-shot timer fires naturally
        killTimer(mgr.timers[timerName].handlerID)
        local result, err = mgr:remainingTime(timerName)
        assert.is_nil(result)
        assert.is_equal("timer is inactive", err)
      end)

      it("Should actually kill the underlying tempTimer when stopped", function()
        -- The manager's own bookkeeping (handlerID = -1) would report "inactive"
        -- even if stop failed to kill the tempTimer, so check the raw handle: a
        -- regression that dropped the killTimer call would leave it firing.
        local mgr = getNewIDManager()
        mgr:registerTimer(timerName, time, function() end)
        local handlerID = mgr.timers[timerName].handlerID
        assert.is_number(remainingTime(handlerID))
        mgr:stopTimer(timerName)
        assert.is_nil(remainingTime(handlerID), "stopping must kill the underlying tempTimer, not just update bookkeeping")
        mgr:deleteAllTimers()
      end)
    end)

    it("Should raise an error if the handlerName is missing or wrong type", function()
      local reg = function()
        registerNamedTimer(user)
      end
      local stop = function()
        stopNamedTimer(user)
      end
      local resume = function()
        resumeNamedTimer(user)
      end
      local delete = function()
        deleteNamedTimer(user)
      end
      local remaining = function()
        remainingNamedTimer(user)
      end
      assert.error_matches(reg, "bad argument #2 type")
      assert.error_matches(stop, "bad argument #2 type")
      assert.error_matches(resume, "bad argument #2 type")
      assert.error_matches(delete, "bad argument #2 type")
      assert.error_matches(remaining, "bad argument #2 type")
    end)

    it("Should raise an error if the handlerName is missing or wrong type", function()
      local reg = function()
        registerNamedTimer()
      end
      local stop = function()
        stopNamedTimer()
      end
      local resume = function()
        resumeNamedTimer()
      end
      local delete = function()
        deleteNamedTimer()
      end
      local remaining = function()
        remainingNamedTimer()
      end
      assert.error_matches(reg, "bad argument #1 type")
      assert.error_matches(stop, "bad argument #1 type")
      assert.error_matches(resume, "bad argument #1 type")
      assert.error_matches(delete, "bad argument #1 type")
      assert.error_matches(remaining, "bad argument #1 type")
    end)

    it("Should consume and pass along the modified error message on error", function()
      local exec = function()
        registerNamedTimer(user, timerName, time)
      end
      local exec2 = function()
        registerNamedTimer(user, timerName)
      end

      assert.error_matches(exec, "registerNamedTimer: bad argument #4 type")
      assert.error_matches(exec2, "registerNamedTimer: bad argument #3 type")
    end)
  end)

  -- Unlike named timers, named triggers fire synchronously via feedTriggers, so
  -- the full lifecycle including the firing effect is exercised here.
  describe("Tests the named trigger functionality", function()
    local user = "trig user"
    local tName = "test trigger"

    after_each(function()
      deleteAllNamedTriggers(user)
      _G.NamedTrigFire = nil
    end)

    it("Should register a named trigger that fires on its substring", function()
      _G.NamedTrigFire = 0
      local ok = registerNamedTrigger(user, tName, "named_trig_basic", function() _G.NamedTrigFire = _G.NamedTrigFire + 1 end)
      assert.is_true(ok)
      assert.are.same({tName}, getNamedTriggers(user))
      feedTriggers("\nnamed_trig_basic\n")
      assert.is_true(_G.NamedTrigFire >= 1, "an active named trigger should fire on its substring")
    end)

    it("Should stop a named trigger from firing", function()
      _G.NamedTrigFire = 0
      registerNamedTrigger(user, tName, "named_trig_stop", function() _G.NamedTrigFire = _G.NamedTrigFire + 1 end)
      feedTriggers("\nnamed_trig_stop\n")
      assert.is_true(_G.NamedTrigFire >= 1, "the trigger should fire before being stopped")

      stopNamedTrigger(user, tName)
      -- the underlying killTrigger defers deletion to the end of the next feed,
      -- so flush once to let cleanup run...
      feedTriggers("\nnamed_trig_stop\n")
      local afterFlush = _G.NamedTrigFire
      -- ...then confirm the stopped trigger no longer fires
      feedTriggers("\nnamed_trig_stop\n")
      assert.is_equal(afterFlush, _G.NamedTrigFire, "a stopped named trigger must not keep firing")
    end)

    it("Should resume a stopped named trigger", function()
      _G.NamedTrigFire = 0
      registerNamedTrigger(user, tName, "named_trig_resume", function() _G.NamedTrigFire = _G.NamedTrigFire + 1 end)
      stopNamedTrigger(user, tName)
      feedTriggers("\nnamed_trig_resume\n") -- flush the deferred cleanup
      resumeNamedTrigger(user, tName)
      local before = _G.NamedTrigFire
      feedTriggers("\nnamed_trig_resume\n")
      assert.is_true(_G.NamedTrigFire > before, "a resumed named trigger should fire again")
    end)

    it("Should register a regex named trigger and list it", function()
      registerNamedRegexTrigger(user, "regex_trig", "^whatever_re$", function() end)
      assert.are.same({"regex_trig"}, getNamedTriggers(user))
    end)

    it("Should list registered named triggers, mixing substring and regex types", function()
      -- https://github.com/Mudlet/Mudlet/issues/9542: substring and regex names
      -- live in separate 1..n arrays, so merging them by index collides entries
      -- and drops one. Registering one of each type guards that regression.
      registerNamedTrigger(user, "sub_one", "whatever_sub_one", function() end)
      registerNamedRegexTrigger(user, "regex_one", "^whatever_re$", function() end)
      local names = getNamedTriggers(user)
      assert.is_equal(2, #names)
      local present = {}
      for _, n in ipairs(names) do present[n] = true end
      assert.is_true(present["sub_one"] and present["regex_one"], "both substring and regex named triggers should be listed")
    end)

    it("Should list a name held by both a substring and a regex trigger only once", function()
      -- the two stores can hold the same name at once; the listing is a set of
      -- names, so the #9542 union must dedupe rather than report the name twice
      registerNamedTrigger(user, "shared", "whatever_shared_sub", function() end)
      registerNamedRegexTrigger(user, "shared", "^whatever_shared_re$", function() end)
      assert.are.same({"shared"}, getNamedTriggers(user))
    end)

    it("Should delete a named trigger", function()
      registerNamedTrigger(user, tName, "named_trig_del", function() end)
      assert.is_true(deleteNamedTrigger(user, tName))
      assert.are.same({}, getNamedTriggers(user))
    end)

    it("Should delete all named triggers across both substring and regex stores", function()
      registerNamedTrigger(user, "t1", "named_trig_all_a", function() end)
      registerNamedRegexTrigger(user, "t2", "^named_trig_all_re$", function() end)
      assert.is_equal(2, #getNamedTriggers(user))
      assert.is_true(deleteAllNamedTriggers(user))
      assert.are.same({}, getNamedTriggers(user))
    end)
  end)

  describe("Tests the functionality of stopAllNamedTriggers", function()
    local user = "stop all trig user"

    after_each(function()
      deleteAllNamedTriggers(user)
      _G.StopAllTrigFire = nil
    end)

    it("Should stop a substring named trigger while leaving it registered", function()
      _G.StopAllTrigFire = 0
      registerNamedTrigger(user, "sub", "stop_all_sub", function() _G.StopAllTrigFire = _G.StopAllTrigFire + 1 end)
      feedTriggers("\nstop_all_sub\n")
      assert.is_true(_G.StopAllTrigFire >= 1, "the trigger should fire before being stopped")

      assert.is_true(stopAllNamedTriggers(user))
      -- killTrigger defers the deletion to the end of the next feed
      feedTriggers("\nstop_all_sub\n")
      local afterFlush = _G.StopAllTrigFire
      feedTriggers("\nstop_all_sub\n")
      assert.is_equal(afterFlush, _G.StopAllTrigFire, "a stopped named trigger must not keep firing")
      -- stopped, not deleted
      assert.are.same({"sub"}, getNamedTriggers(user))
    end)

    it("Should stop regex named triggers too", function()
      _G.StopAllTrigFire = 0
      registerNamedRegexTrigger(user, "re", "^stop_all_re$", function() _G.StopAllTrigFire = _G.StopAllTrigFire + 1 end)
      feedTriggers("\nstop_all_re\n")
      assert.is_true(_G.StopAllTrigFire >= 1)

      stopAllNamedTriggers(user)
      -- killTrigger deactivates synchronously, so not one more fire is allowed
      local atStop = _G.StopAllTrigFire
      feedTriggers("\nstop_all_re\n")
      feedTriggers("\nstop_all_re\n")
      assert.is_equal(atStop, _G.StopAllTrigFire, "a stopped regex named trigger must not keep firing")
      -- stopped, not deleted
      assert.are.same({"re"}, getNamedTriggers(user))
    end)

    it("Should let a stopped regex named trigger be resumed", function()
      _G.StopAllTrigFire = 0
      registerNamedRegexTrigger(user, "re", "^stop_all_resume_re$", function() _G.StopAllTrigFire = _G.StopAllTrigFire + 1 end)
      stopAllNamedTriggers(user)
      local atStop = _G.StopAllTrigFire
      feedTriggers("\nstop_all_resume_re\n")
      assert.is_equal(atStop, _G.StopAllTrigFire, "the regex named trigger should be stopped")

      assert.is_true(resumeNamedTrigger(user, "re"), "a stopped regex named trigger must be resumable")
      feedTriggers("\nstop_all_resume_re\n")
      assert.is_true(_G.StopAllTrigFire > atStop, "the resumed regex named trigger should fire again")
    end)

    it("Should raise an error if the userName is missing or wrong type", function()
      assert.has_error(function() stopAllNamedTriggers() end)
      assert.has_error(function() stopAllNamedTriggers(5) end)
    end)
  end)

  describe("Tests the functionality of a private manager from getNewIDManager", function()
    local mgr

    before_each(function()
      mgr = getNewIDManager()
      _G.PrivateMgrFire = nil
    end)

    after_each(function()
      mgr:deleteAllTimers()
      mgr:deleteAllTriggers()
      mgr:deleteAllEvents()
      _G.PrivateMgrFire = nil
    end)

    it("Should hand out managers with independent stores", function()
      local other = getNewIDManager()
      finally(function() other:deleteAllTimers() end)
      mgr:registerTimer("shared name", 100, function() end)
      assert.are.same({"shared name"}, mgr:getTimers())
      assert.are.same({}, other:getTimers())
    end)

    -- IDMgr is a local in IDManager.lua, so the class itself is only reachable
    -- as the metatable of an instance it has already handed out
    describe("Tests the functionality of IDMgr:new", function()
      local IDMgr = getmetatable(getNewIDManager())
      local made

      before_each(function()
        made = {}
      end)

      after_each(function()
        for _, manager in ipairs(made) do
          manager:deleteAllTimers()
          manager:deleteAllTriggers()
          manager:deleteAllEvents()
        end
      end)

      local function newManager()
        local manager = IDMgr:new()
        made[#made + 1] = manager
        return manager
      end

      it("Should give a manager its own four empty stores", function()
        local fresh = newManager()
        assert.are.same({}, fresh.events)
        assert.are.same({}, fresh.timers)
        assert.are.same({}, fresh.triggers)
        assert.are.same({}, fresh.regexTriggers)
      end)

      it("Should not let two managers share a store", function()
        local first, second = newManager(), newManager()
        first:registerTimer("mine", 100, function() end)
        assert.are.same({"mine"}, first:getTimers())
        assert.are.same({}, second:getTimers())
      end)

      it("Should reach the IDMgr methods through the metatable it sets", function()
        local fresh = newManager()
        assert.are.equal(IDMgr, getmetatable(fresh))
        assert.is_function(fresh.registerTimer)
        assert.is_function(fresh.emergencyStop)
      end)

      it("Should give every manager the same metatable", function()
        assert.are.equal(IDMgr, getmetatable(getNewIDManager()))
        assert.are.equal(IDMgr, getmetatable(newManager()))
      end)
    end)

    describe("Tests the functionality of IDMgr:registerTrigger and IDMgr:registerRegexTrigger", function()
      it("Should register a substring trigger that fires", function()
        _G.PrivateMgrFire = 0
        assert.is_true(mgr:registerTrigger("sub", "private_mgr_sub", function() _G.PrivateMgrFire = _G.PrivateMgrFire + 1 end))
        feedTriggers("\nprivate_mgr_sub\n")
        assert.is_true(_G.PrivateMgrFire >= 1)
      end)

      it("Should register a regex trigger that fires", function()
        _G.PrivateMgrFire = 0
        assert.is_true(mgr:registerRegexTrigger("re", "^private_mgr_re$", function() _G.PrivateMgrFire = _G.PrivateMgrFire + 1 end))
        feedTriggers("\nprivate_mgr_re\n")
        assert.is_true(_G.PrivateMgrFire >= 1)
      end)

      it("Should keep substring and regex triggers in separate stores", function()
        mgr:registerTrigger("same", "private_mgr_both_sub", function() end)
        mgr:registerRegexTrigger("same", "^private_mgr_both_re$", function() end)
        assert.is_not_nil(mgr.triggers["same"])
        assert.is_not_nil(mgr.regexTriggers["same"])
      end)

      it("Should report the upstream failure instead of raising", function()
        local ok, err = mgr:registerTrigger("bad", {}, function() end)
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_nil(mgr.triggers["bad"])
      end)
    end)

    describe("Tests the functionality of IDMgr:getTriggers", function()
      it("Should list substring and regex names once each, sorted", function()
        mgr:registerTrigger("bravo", "private_mgr_list_a", function() end)
        mgr:registerRegexTrigger("alpha", "^private_mgr_list_b$", function() end)
        mgr:registerRegexTrigger("bravo", "^private_mgr_list_c$", function() end)
        assert.are.same({"alpha", "bravo"}, mgr:getTriggers())
      end)

      it("Should return an empty list for a fresh manager", function()
        assert.are.same({}, mgr:getTriggers())
      end)
    end)

    describe("Tests the functionality of IDMgr:stopTrigger and IDMgr:resumeTrigger", function()
      it("Should stop a substring trigger and resume it again", function()
        _G.PrivateMgrFire = 0
        mgr:registerTrigger("sub", "private_mgr_stop", function() _G.PrivateMgrFire = _G.PrivateMgrFire + 1 end)
        assert.is_true(mgr:stopTrigger("sub"))
        assert.is_equal(-1, mgr.triggers["sub"].handlerID)
        feedTriggers("\nprivate_mgr_stop\n") -- flush the deferred cleanup
        local afterFlush = _G.PrivateMgrFire
        feedTriggers("\nprivate_mgr_stop\n")
        assert.is_equal(afterFlush, _G.PrivateMgrFire)

        assert.is_true(mgr:resumeTrigger("sub"))
        local before = _G.PrivateMgrFire
        feedTriggers("\nprivate_mgr_stop\n")
        assert.is_true(_G.PrivateMgrFire > before, "a resumed trigger should fire again")
      end)

      it("Should reach the regex store as well", function()
        mgr:registerRegexTrigger("re", "^private_mgr_stop_re$", function() end)
        assert.is_true(mgr:stopTrigger("re"))
        assert.is_equal(-1, mgr.regexTriggers["re"].handlerID)
        assert.is_true(mgr:resumeTrigger("re"))
        assert.is_true(mgr.regexTriggers["re"].handlerID > 0)
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:stopTrigger("nope"))
        assert.is_false(mgr:resumeTrigger("nope"))
      end)
    end)

    describe("Tests the functionality of IDMgr:deleteTrigger and IDMgr:deleteAllTriggers", function()
      it("Should delete a substring trigger and forget its name", function()
        mgr:registerTrigger("sub", "private_mgr_del", function() end)
        assert.is_true(mgr:deleteTrigger("sub"))
        assert.are.same({}, mgr:getTriggers())
        assert.is_nil(mgr.triggers["sub"])
      end)

      it("Should delete a regex trigger too", function()
        mgr:registerRegexTrigger("re", "^private_mgr_del_re$", function() end)
        assert.is_true(mgr:deleteTrigger("re"))
        assert.are.same({}, mgr:getTriggers())
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:deleteTrigger("nope"))
      end)

      it("Should clear both stores at once", function()
        mgr:registerTrigger("sub", "private_mgr_all_a", function() end)
        mgr:registerRegexTrigger("re", "^private_mgr_all_b$", function() end)
        assert.is_equal(2, #mgr:getTriggers())
        assert.is_true(mgr:deleteAllTriggers())
        assert.are.same({}, mgr:getTriggers())
      end)
    end)

    describe("Tests the functionality of IDMgr:registerTimer", function()
      -- Mudlet is single threaded, so a spec body holds the event loop until it
      -- returns and no timer can fire inside one: hand control back for a moment
      local function pumpEventLoop(milliseconds)
        tempTimer(milliseconds / 1000, function() raiseEvent("idManagerTimerPump") end)
        waitForEvent("idManagerTimerPump", milliseconds + 1000)
      end

      local function pumpUntil(condition)
        for _ = 1, 20 do
          if condition() then
            return
          end
          pumpEventLoop(50)
        end
      end

      it("Should give the timer a live tempTimer and list it under its name", function()
        assert.is_true(mgr:registerTimer("timer", 100, function() end))
        assert.is_number(remainingTime(mgr.timers["timer"].handlerID))
        assert.are.same({"timer"}, mgr:getTimers())
      end)

      -- waitForEvent is test mode only, so the two specs that wait for a timer
      -- to fire cannot run when the suite is started by hand with runTests
      local canPump = os.getenv("MUDLET_TEST_MODE") ~= nil

      if not canPump then
        pending("Should run the function it was given exactly once - needs MUDLET_TEST_MODE for waitForEvent")
        pending("Should hand the fourth argument to tempTimer as its repeating flag - needs MUDLET_TEST_MODE for waitForEvent")
      else
        it("Should run the function it was given exactly once", function()
          local fired = 0
          mgr:registerTimer("timer", 0.05, function() fired = fired + 1 end)
          local handlerID = mgr.timers["timer"].handlerID

          pumpUntil(function() return fired > 0 end)
          pumpEventLoop(200)

          assert.are.equal(1, fired)
          -- a timer that is not repeating is gone once it has fired
          assert.is_nil(remainingTime(handlerID))
        end)

        -- IDMgr keeps this flag in a field called oneShot for every store, but
        -- for timers it reaches tempTimer as its repeating argument, which is
        -- the opposite meaning to the event case below - and it matches what
        -- registerNamedTimer(..., [repeating]) is documented to take
        it("Should hand the fourth argument to tempTimer as its repeating flag", function()
          local fired = 0
          mgr:registerTimer("timer", 0.05, function() fired = fired + 1 end, true)
          local handlerID = mgr.timers["timer"].handlerID

          pumpUntil(function() return fired > 1 end)

          assert.is_true(fired > 1, "a repeating timer should fire more than once")
          assert.is_number(remainingTime(handlerID), "a repeating timer stays alive after firing")
        end)
      end

      it("Should replace an earlier timer of the same name instead of adding one", function()
        mgr:registerTimer("timer", 5000, function() end)
        local firstID = mgr.timers["timer"].handlerID

        assert.is_true(mgr:registerTimer("timer", 5000, function() end))

        assert.are_not.equal(firstID, mgr.timers["timer"].handlerID)
        assert.is_nil(remainingTime(firstID), "the timer it replaces has to be killed")
        assert.are.same({"timer"}, mgr:getTimers())
      end)

      it("Should report the upstream failure instead of raising", function()
        local ok, err = mgr:registerTimer("bad", "not a number", function() end)
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_nil(mgr.timers["bad"])
        assert.are.same({}, mgr:getTimers())
      end)

      -- BUG: IDMgr:register stops whatever is registered under the name before
      -- it tries the new registration, and puts nothing back when that fails,
      -- so a re-registration with a bad argument kills a working timer while
      -- leaving the name in getTimers() looking registered.
      pending("Should leave the running timer alone when a re-registration fails")
    end)

    describe("Tests the functionality of IDMgr:stopTimer", function()
      it("Should kill the tempTimer while leaving the name registered", function()
        mgr:registerTimer("timer", 100, function() end)
        local handlerID = mgr.timers["timer"].handlerID

        assert.is_true(mgr:stopTimer("timer"))

        assert.is_nil(remainingTime(handlerID), "stopping has to kill the tempTimer, not just the bookkeeping")
        assert.are.equal(-1, mgr.timers["timer"].handlerID)
        assert.are.same({"timer"}, mgr:getTimers())
      end)

      it("Should report the stopped timer as inactive", function()
        mgr:registerTimer("timer", 100, function() end)
        mgr:stopTimer("timer")
        local remaining, err = mgr:remainingTime("timer")
        assert.is_nil(remaining)
        assert.are.equal("timer is inactive", err)
      end)

      it("Should leave the other timers running", function()
        mgr:registerTimer("stopped", 100, function() end)
        mgr:registerTimer("running", 100, function() end)
        mgr:stopTimer("stopped")
        assert.is_number(mgr:remainingTime("running"))
      end)

      it("Should stay stopped when asked a second time", function()
        mgr:registerTimer("timer", 100, function() end)
        assert.is_true(mgr:stopTimer("timer"))
        assert.is_true(mgr:stopTimer("timer"))
        assert.are.equal(-1, mgr.timers["timer"].handlerID)
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:stopTimer("no such timer"))
      end)
    end)

    describe("Tests the functionality of IDMgr:getTimers", function()
      it("Should return an empty list for a fresh manager", function()
        assert.are.same({}, mgr:getTimers())
      end)

      it("Should list every registered timer name, sorted", function()
        mgr:registerTimer("charlie", 100, function() end)
        mgr:registerTimer("alpha", 100, function() end)
        mgr:registerTimer("bravo", 100, function() end)
        assert.are.same({"alpha", "bravo", "charlie"}, mgr:getTimers())
      end)

      it("Should keep listing a timer that was stopped but not deleted", function()
        mgr:registerTimer("stopped", 100, function() end)
        mgr:stopTimer("stopped")
        assert.are.same({"stopped"}, mgr:getTimers())
      end)

      it("Should forget a deleted timer", function()
        mgr:registerTimer("gone", 100, function() end)
        mgr:deleteTimer("gone")
        assert.are.same({}, mgr:getTimers())
      end)

      it("Should not report events or triggers as timers", function()
        mgr:registerEvent("event", "privateMgrTimerListEvent", function() end)
        mgr:registerTrigger("trigger", "private_mgr_timers_not_triggers", function() end)
        assert.are.same({}, mgr:getTimers())
      end)
    end)

    describe("Tests the functionality of IDMgr:stopAllTimers and IDMgr:deleteAllTimers", function()
      it("Should stop every timer while leaving them registered", function()
        mgr:registerTimer("one", 100, function() end)
        mgr:registerTimer("two", 100, function() end)
        assert.is_true(mgr:stopAllTimers())
        assert.is_equal(-1, mgr.timers["one"].handlerID)
        assert.is_equal(-1, mgr.timers["two"].handlerID)
        assert.are.same({"one", "two"}, mgr:getTimers())
      end)

      it("Should delete every timer", function()
        mgr:registerTimer("one", 100, function() end)
        mgr:registerTimer("two", 100, function() end)
        assert.is_true(mgr:deleteAllTimers())
        assert.are.same({}, mgr:getTimers())
      end)
    end)

    describe("Tests the functionality of IDMgr:stopAllTriggers", function()
      it("Should stop every substring trigger while leaving it registered", function()
        mgr:registerTrigger("one", "private_mgr_stopall_a", function() end)
        mgr:registerTrigger("two", "private_mgr_stopall_b", function() end)
        assert.is_true(mgr:stopAllTriggers())
        assert.is_equal(-1, mgr.triggers["one"].handlerID)
        assert.is_equal(-1, mgr.triggers["two"].handlerID)
        assert.are.same({"one", "two"}, mgr:getTriggers())
      end)

      it("Should stop regex triggers as well", function()
        mgr:registerTrigger("sub", "private_mgr_stopall_sub", function() end)
        mgr:registerRegexTrigger("re", "^private_mgr_stopall_re$", function() end)
        assert.is_true(mgr:stopAllTriggers())
        assert.is_equal(-1, mgr.regexTriggers["re"].handlerID)
        -- the substring store must not regress while the regex one is added
        assert.is_equal(-1, mgr.triggers["sub"].handlerID)
        assert.are.same({"re", "sub"}, mgr:getTriggers())
      end)
    end)

    describe("Tests the functionality of IDMgr:emergencyStop", function()
      it("Should stop timers, events and substring triggers in one call", function()
        mgr:registerTimer("timer", 100, function() end)
        mgr:registerEvent("event", "someEventNameNobodyRaises", function() end)
        mgr:registerTrigger("trigger", "private_mgr_emergency", function() end)

        assert.is_true(mgr:emergencyStop())

        assert.is_equal(-1, mgr.timers["timer"].handlerID)
        assert.is_equal(-1, mgr.events["event"].handlerID)
        assert.is_equal(-1, mgr.triggers["trigger"].handlerID)
        -- everything stays registered so it can be resumed
        assert.are.same({"timer"}, mgr:getTimers())
        assert.are.same({"trigger"}, mgr:getTriggers())
      end)

      it("Should stop regex triggers too", function()
        mgr:registerRegexTrigger("re", "^private_mgr_emergency_re$", function() end)
        assert.is_true(mgr:emergencyStop())
        assert.is_equal(-1, mgr.regexTriggers["re"].handlerID)
        -- stopped, not deleted, so it can still be resumed
        assert.are.same({"re"}, mgr:getTriggers())
      end)
    end)

    describe("Tests the functionality of IDMgr:registerEvent", function()
      local eventName = "privateMgrRegisterEvent"

      it("Should register a handler that fires on the event", function()
        local fired = 0
        assert.is_true(mgr:registerEvent("handler", eventName, function() fired = fired + 1 end))
        raiseEvent(eventName)
        assert.are.equal(1, fired)
        assert.are.same({"handler"}, mgr:getEvents())
      end)

      it("Should pass the arguments the event was raised with", function()
        local seen
        mgr:registerEvent("handler", eventName, function(_, first, second) seen = {first, second} end)
        raiseEvent(eventName, "alpha", 2)
        assert.are.same({"alpha", 2}, seen)
      end)

      it("Should stop after one event when the fourth argument asks for a one shot", function()
        local fired = 0
        mgr:registerEvent("handler", eventName, function() fired = fired + 1 end, true)
        raiseEvent(eventName)
        raiseEvent(eventName)
        assert.are.equal(1, fired)
      end)

      it("Should keep firing without that fourth argument", function()
        local fired = 0
        mgr:registerEvent("handler", eventName, function() fired = fired + 1 end)
        raiseEvent(eventName)
        raiseEvent(eventName)
        assert.are.equal(2, fired)
      end)

      it("Should replace a handler registered under the same name", function()
        local first, second = 0, 0
        mgr:registerEvent("handler", eventName, function() first = first + 1 end)
        mgr:registerEvent("handler", eventName, function() second = second + 1 end)
        raiseEvent(eventName)
        assert.are.equal(0, first, "the first registration has to be killed, not left behind")
        assert.are.equal(1, second)
        assert.are.same({"handler"}, mgr:getEvents())
      end)

      it("Should report the upstream failure instead of raising", function()
        local ok, err = mgr:registerEvent("bad", eventName, 5)
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_nil(mgr.events["bad"])
      end)
    end)

    -- The event store is reached through the shared per-user managers elsewhere
    -- in this file. Here it is driven directly, so that the answers a private
    -- manager gives are pinned even for a package that keeps its own.
    describe("Tests the functionality of IDMgr:getEvents", function()
      local eventName = "privateMgrEventListEvent"

      it("Should return an empty list for a fresh manager", function()
        assert.are.same({}, mgr:getEvents())
      end)

      it("Should list every registered handler name, sorted", function()
        mgr:registerEvent("charlie", eventName, function() end)
        mgr:registerEvent("alpha", eventName, function() end)
        mgr:registerEvent("bravo", eventName, function() end)
        assert.are.same({"alpha", "bravo", "charlie"}, mgr:getEvents())
      end)

      it("Should keep listing a handler that was stopped but not deleted", function()
        mgr:registerEvent("stopped", eventName, function() end)
        mgr:stopEvent("stopped")
        assert.are.same({"stopped"}, mgr:getEvents())
      end)

      it("Should not report timers or triggers as event handlers", function()
        mgr:registerTimer("timer", 100, function() end)
        mgr:registerTrigger("trigger", "private_mgr_events_not_triggers", function() end)
        assert.are.same({}, mgr:getEvents())
      end)
    end)

    describe("Tests the functionality of IDMgr:stopEvent and IDMgr:resumeEvent", function()
      local eventName = "privateMgrStopResumeEvent"
      local fired

      before_each(function()
        fired = 0
        mgr:registerEvent("handler", eventName, function() fired = fired + 1 end)
      end)

      it("Should stop the handler firing while leaving it registered", function()
        raiseEvent(eventName)
        assert.are.equal(1, fired)

        assert.is_true(mgr:stopEvent("handler"))
        raiseEvent(eventName)
        assert.are.equal(1, fired)
        assert.are.equal(-1, mgr.events["handler"].handlerID)
        assert.are.same({"handler"}, mgr:getEvents())
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:stopEvent("no such handler"))
      end)

      it("Should let a stopped handler fire again once resumed", function()
        mgr:stopEvent("handler")
        assert.is_true(mgr:resumeEvent("handler"))
        assert.are_not.equal(-1, mgr.events["handler"].handlerID)
        raiseEvent(eventName)
        assert.are.equal(1, fired)
      end)

      it("Should leave a resumed handler registered exactly once", function()
        -- resume goes through register, which stops the old registration first;
        -- if it did not, the handler would run twice for one event
        mgr:resumeEvent("handler")
        raiseEvent(eventName)
        assert.are.equal(1, fired)
        assert.are.same({"handler"}, mgr:getEvents())
      end)

      it("Should return false when resuming a name it does not know", function()
        assert.is_false(mgr:resumeEvent("no such handler"))
      end)
    end)

    describe("Tests the functionality of IDMgr:deleteEvent", function()
      local eventName = "privateMgrDeleteEvent"

      it("Should stop the handler firing and forget its name", function()
        local fired = 0
        mgr:registerEvent("handler", eventName, function() fired = fired + 1 end)

        assert.is_true(mgr:deleteEvent("handler"))

        raiseEvent(eventName)
        assert.are.equal(0, fired)
        assert.are.same({}, mgr:getEvents())
        assert.is_nil(mgr.events["handler"])
      end)

      it("Should leave a deleted handler beyond resuming", function()
        mgr:registerEvent("handler", eventName, function() end)
        mgr:deleteEvent("handler")
        assert.is_false(mgr:resumeEvent("handler"))
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:deleteEvent("no such handler"))
      end)
    end)

    describe("Tests the functionality of IDMgr:stopAllEvents", function()
      local eventName = "privateMgrStopAllEvent"

      it("Should stop every handler while leaving them all registered", function()
        local fired = 0
        mgr:registerEvent("one", eventName, function() fired = fired + 1 end)
        mgr:registerEvent("two", eventName, function() fired = fired + 1 end)
        raiseEvent(eventName)
        assert.are.equal(2, fired)

        assert.is_true(mgr:stopAllEvents())

        raiseEvent(eventName)
        assert.are.equal(2, fired)
        assert.are.equal(-1, mgr.events["one"].handlerID)
        assert.are.equal(-1, mgr.events["two"].handlerID)
        assert.are.same({"one", "two"}, mgr:getEvents())
      end)

      it("Should leave the stopped handlers resumable one at a time", function()
        local fired = 0
        mgr:registerEvent("one", eventName, function() fired = fired + 1 end)
        mgr:registerEvent("two", eventName, function() fired = fired + 1 end)
        mgr:stopAllEvents()

        mgr:resumeEvent("one")
        raiseEvent(eventName)
        assert.are.equal(1, fired)
      end)

      it("Should not raise for a manager with no handlers", function()
        assert.is_true(mgr:stopAllEvents())
      end)

      it("Should leave timers alone", function()
        mgr:registerTimer("timer", 100, function() end)
        mgr:stopAllEvents()
        assert.is_number(mgr:remainingTime("timer"))
      end)
    end)

    describe("Tests the functionality of IDMgr:deleteAllEvents", function()
      local eventName = "privateMgrDeleteAllEvent"

      it("Should stop every handler firing and empty the store", function()
        local fired = 0
        mgr:registerEvent("one", eventName, function() fired = fired + 1 end)
        mgr:registerEvent("two", eventName, function() fired = fired + 1 end)

        assert.is_true(mgr:deleteAllEvents())

        raiseEvent(eventName)
        assert.are.equal(0, fired)
        assert.are.same({}, mgr:getEvents())
      end)

      it("Should leave the deleted handlers beyond resuming", function()
        mgr:registerEvent("one", eventName, function() end)
        mgr:deleteAllEvents()
        assert.is_false(mgr:resumeEvent("one"))
      end)

      it("Should leave timers and triggers alone", function()
        mgr:registerEvent("event", eventName, function() end)
        mgr:registerTimer("timer", 100, function() end)
        mgr:registerTrigger("trigger", "private_mgr_delete_all_events", function() end)

        mgr:deleteAllEvents()

        assert.is_number(mgr:remainingTime("timer"))
        assert.are.same({"timer"}, mgr:getTimers())
        assert.are.same({"trigger"}, mgr:getTriggers())
      end)

      it("Should return true for a manager with no handlers", function()
        assert.is_true(mgr:deleteAllEvents())
      end)
    end)

    -- stopAll and deleteAll are the store-agnostic bodies behind the seven
    -- stopAll*/deleteAll* wrappers, so they are driven by store name here
    describe("Tests the functionality of IDMgr:stopAll and IDMgr:deleteAll", function()
      local eventName = "privateMgrStoreLoopEvent"

      it("Should stop everything in the store it is named", function()
        mgr:registerEvent("event", eventName, function() end)
        mgr:registerTimer("timer", 100, function() end)

        assert.is_true(mgr:stopAll("events"))

        assert.are.equal(-1, mgr.events["event"].handlerID)
        -- the timer store was not named, so it is untouched
        assert.is_number(mgr:remainingTime("timer"))
      end)

      it("Should stop the timer store when that is the one named", function()
        local handlerID
        mgr:registerTimer("timer", 100, function() end)
        handlerID = mgr.timers["timer"].handlerID

        assert.is_true(mgr:stopAll("timers"))

        assert.are.equal(-1, mgr.timers["timer"].handlerID)
        -- the bookkeeping alone would say inactive, so check the real tempTimer
        assert.is_nil(remainingTime(handlerID))
      end)

      it("Should not raise on an empty store", function()
        assert.is_true(mgr:stopAll("events"))
        assert.is_true(mgr:stopAll("regexTriggers"))
      end)

      it("Should empty the store it is named and stop what was in it", function()
        local fired = 0
        mgr:registerEvent("event", eventName, function() fired = fired + 1 end)
        mgr:registerTimer("timer", 100, function() end)

        assert.is_true(mgr:deleteAll("events"))

        raiseEvent(eventName)
        assert.are.equal(0, fired)
        assert.are.same({}, mgr:getEvents())
        assert.are.same({"timer"}, mgr:getTimers())
      end)

      it("Should not raise on an empty store for deleteAll either", function()
        assert.is_true(mgr:deleteAll("events"))
        assert.is_true(mgr:deleteAll("timers"))
      end)
    end)

    describe("Tests the functionality of IDMgr:resumeTimer and IDMgr:deleteTimer", function()
      it("Should give a stopped timer a running tempTimer again", function()
        mgr:registerTimer("timer", 100, function() end)
        mgr:stopTimer("timer")
        assert.is_nil((mgr:remainingTime("timer")))

        assert.is_true(mgr:resumeTimer("timer"))

        assert.are_not.equal(-1, mgr.timers["timer"].handlerID)
        assert.is_number(mgr:remainingTime("timer"))
      end)

      it("Should restart a timer that was still running rather than add a second one", function()
        mgr:registerTimer("timer", 5000, function() end)
        local firstID = mgr.timers["timer"].handlerID

        assert.is_true(mgr:resumeTimer("timer"))

        assert.are_not.equal(firstID, mgr.timers["timer"].handlerID)
        assert.is_nil(remainingTime(firstID), "resuming has to kill the tempTimer it replaces")
        assert.are.same({"timer"}, mgr:getTimers())
      end)

      it("Should return false when resuming a name it does not know", function()
        assert.is_false(mgr:resumeTimer("no such timer"))
      end)

      it("Should kill the underlying tempTimer and forget the name", function()
        mgr:registerTimer("timer", 100, function() end)
        local handlerID = mgr.timers["timer"].handlerID

        assert.is_true(mgr:deleteTimer("timer"))

        assert.is_nil(remainingTime(handlerID))
        assert.are.same({}, mgr:getTimers())
        local remaining, err = mgr:remainingTime("timer")
        assert.is_nil(remaining)
        assert.are.equal("timer not found", err)
      end)

      it("Should leave the other timers alone", function()
        mgr:registerTimer("keep", 100, function() end)
        mgr:registerTimer("drop", 100, function() end)
        mgr:deleteTimer("drop")
        assert.are.same({"keep"}, mgr:getTimers())
        assert.is_number(mgr:remainingTime("keep"))
      end)

      it("Should return false for a name it does not know", function()
        assert.is_false(mgr:deleteTimer("no such timer"))
      end)
    end)
  end)
end)
