describe("Tests the functionality of IDMgr", function()
  describe("Test the event functionality", function()
    local RESpy
    local KESpy
    local eventName = "testEvent"
    local handlerName = "tester"
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
      deleteAllNamedEventHandlers()
    end)

    it("Should register an event handler", function()
      local ok = registerNamedEventHandler(handlerName, eventName, func)
      assert.is_true(ok)
      assert.spy(RESpy).was_called(1)
      assert.spy(RESpy).was_called_with(eventName, func, false)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(1)
    end)

    it("Should kill the old handler and reregister a new one if you register to the same name more than once", function()
      local ok = registerNamedEventHandler(handlerName, eventName, func)
      assert.is_true(ok)
      ok = registerNamedEventHandler(handlerName, eventName, func)
      assert.is_true(ok)
      assert.spy(RESpy).was_called(2)
      assert.spy(RESpy).was_called_with(eventName, func, false)
      assert.spy(KESpy).was_called(1)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(1)
    end)

    it("Should allow for you to stop a handler", function()
      registerNamedEventHandler(handlerName, eventName, func)
      local ok = stopNamedEventHandler(handlerName)
      assert.is_true(ok)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_not_called()
    end)

    it("Should allow you to resume a stopped handler", function()
      registerNamedEventHandler(handlerName, eventName, func)
      stopNamedEventHandler(handlerName)
      raiseEvent(eventName)
      local ok = resumeNamedEventHandler(handlerName)
      assert.is_true(ok)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(1)
    end)

    it("Should provide a list of registered named handlers", function()
      registerNamedEventHandler(handlerName, eventName, func)
      local handlers = getNamedEventHandlers()
      assert.are.same({handlerName}, handlers)
    end)

    it("Should allow for deleting a handler entirely", function()
      registerNamedEventHandler(handlerName, eventName, func)
      deleteNamedEventHandler(handlerName)
      local handlers = getNamedEventHandlers()
      assert.are.same(handlers, {})
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_not_called()
    end)

    it("Should stop all handlers when asked", function()
      local handlerName2 = handlerName .. "2"
      registerNamedEventHandler(handlerName, eventName, func)
      registerNamedEventHandler(handlerName2, eventName, func)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(2)
      stopAllNamedEventHandlers()
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(2) -- remains at 2
      resumeNamedEventHandler(handlerName)
      resumeNamedEventHandler(handlerName2)
      raiseEvent(eventName)
      assert.spy(handlerSpy).was_called(4)
    end)

    it("Should delete all handlers when asked", function()
      local handlerName2 = handlerName .. "2"
      registerNamedEventHandler(handlerName, eventName, func)
      registerNamedEventHandler(handlerName2, eventName, func)
      local handlers_before = getNamedEventHandlers()
      assert.are.same({handlerName, handlerName2}, handlers_before)
      deleteAllNamedEventHandlers()
      local handlers_after = getNamedEventHandlers()
      assert.are.same({}, handlers_after)
    end)

    it("Should consume and pass along the modified error message on error", function()
      local exec = function()
        registerNamedEventHandler(name, eventName)
      end
      local exec2 = function()
        registerNamedEventHandler(name)
      end
      -- since we pass along all the parameters to registerAnonymousEventHandler
      -- we have to catch the error, bump the arg# up by one, and rebrand it so
      -- the error origin isn't confusing to the end user.
      assert.error_matches(exec, "registerNamedEventHandler: bad argument #3 type")
      assert.error_matches(exec2, "registerNamedEventHandler: bad argument #2 type")
    end)
  end)

  -- timer functionality tests awaiting me figuring out async tests in busted
  -- functional testing shows it works, and the code paths are the same for timers as
  -- for events in the underlying IDMgr, just using different core Mudlet API functions
  -- I have personally functionally tested this though. -- Demonnic
  -- TODO: write timer tests https://github.com/Mudlet/Mudlet/issues/5520
  describe("Tests the timer functionality", function()
    pending("Should register a named timer")
    pending("Should reset a named timer if it is registered a second+ time")
    pending("Should allow you to stop a named timer")
    pending("Should allow you to resume a named timer")
    pending("Should allow you to stop all named timers")
    pending("Should allow you to delete a named timer")
    pending("Should allow you to delete all named timers")
    pending("Should consume and raise modified tempTimer error message on error")
  end)
end)