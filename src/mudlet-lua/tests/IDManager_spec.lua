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
end)
