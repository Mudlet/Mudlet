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

  -- timer functionality tests awaiting me figuring out async tests in busted
  -- functional testing shows it works, and the code paths are the same for timers as
  -- for events in the underlying IDMgr, just using different core Mudlet API functions
  -- I have personally functionally tested this though. -- Demonnic
  -- TODO: write timer tests https://github.com/Mudlet/Mudlet/issues/5520
  describe("Tests the timer functionality", function()
    local user = "test user"
    local timerName = "test timer"
    local time = 100
    pending("Should register a named timer")
    pending("Should reset a named timer if it is registered a second+ time")
    pending("Should allow you to stop a named timer")
    pending("Should allow you to resume a named timer")
    pending("Should allow you to stop all named timers")
    pending("Should allow you to delete a named timer")
    pending("Should allow you to delete all named timers")

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
      assert.error_matches(reg, "bad argument #2 type")
      assert.error_matches(stop, "bad argument #2 type")
      assert.error_matches(resume, "bad argument #2 type")
      assert.error_matches(delete, "bad argument #2 type")
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
      assert.error_matches(reg, "bad argument #1 type")
      assert.error_matches(stop, "bad argument #1 type")
      assert.error_matches(resume, "bad argument #1 type")
      assert.error_matches(delete, "bad argument #1 type")
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
end)