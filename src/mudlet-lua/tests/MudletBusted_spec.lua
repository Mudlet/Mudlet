describe("Mudlet Busted sanity check", function()
    it("runs a simple assert", function()
        assert.truthy("Not nil or false.")
      end)
    it("checks that it has access to mudlet environment functions", function()
        assert.truthy(string.find(getMudletHomeDir(), "[mM]udlet"))
      end)
  end)

describe("waitForEvent test helper", function()
    -- select('#', ...) counts embedded and trailing nils that a plain table
    -- constructor would lose, so it lets a spec verify the argument count too.
    local function grab(...)
        return select('#', ...), ...
      end

    it("observes a tempTimer firing through a raised event", function()
        tempTimer(0.05, function() raiseEvent("mudletTestTimerFired", 42) end)
        local name, value = waitForEvent("mudletTestTimerFired", 2000)
        assert.equals("mudletTestTimerFired", name)
        assert.equals(42, value)
      end)

    it("round-trips raiseEvent arguments of several types", function()
        tempTimer(0, function() raiseEvent("mudletTestRoundTrip", "hello", 7, true) end)
        local name, str, num, boolean = waitForEvent("mudletTestRoundTrip", 2000)
        assert.equals("mudletTestRoundTrip", name)
        assert.equals("hello", str)
        assert.equals(7, num)
        assert.is_true(boolean)
      end)

    it("preserves nil and false arguments in their positions", function()
        tempTimer(0, function() raiseEvent("mudletTestNilArg", nil, false, "after") end)
        local count, name, first, second, third = grab(waitForEvent("mudletTestNilArg", 2000))
        assert.equals(4, count)
        assert.equals("mudletTestNilArg", name)
        assert.is_nil(first)
        assert.is_false(second)
        assert.equals("after", third)
      end)

    it("round-trips a table argument that outlives the event", function()
        tempTimer(0, function() raiseEvent("mudletTestTableArg", {a = 1, b = "two"}) end)
        local name, payload = waitForEvent("mudletTestTableArg", 2000)
        assert.equals("mudletTestTableArg", name)
        assert.is_table(payload)
        assert.equals(1, payload.a)
        assert.equals("two", payload.b)
      end)

    it("returns just the event name when there is no payload", function()
        tempTimer(0, function() raiseEvent("mudletTestNameOnly") end)
        local count, name = grab(waitForEvent("mudletTestNameOnly", 2000))
        assert.equals(1, count)
        assert.equals("mudletTestNameOnly", name)
      end)

    it("uses a default timeout when none is supplied", function()
        tempTimer(0, function() raiseEvent("mudletTestDefaultTimeout", "ok") end)
        local name, value = waitForEvent("mudletTestDefaultTimeout")
        assert.equals("mudletTestDefaultTimeout", name)
        assert.equals("ok", value)
      end)

    it("returns nil and a message naming the event when it never arrives", function()
        local result, message = waitForEvent("mudletTestNeverRaised", 100)
        assert.is_nil(result)
        assert.is_string(message)
        assert.truthy(message:find("timed out"))
        assert.truthy(message:find("mudletTestNeverRaised"))
      end)

    it("does not wake for a different event", function()
        tempTimer(0, function() raiseEvent("mudletTestOtherEvent") end)
        local result = waitForEvent("mudletTestWantedEvent", 200)
        assert.is_nil(result)
      end)

    it("does not observe an event raised before the wait began", function()
        raiseEvent("mudletTestPreRaised")
        local result = waitForEvent("mudletTestPreRaised", 100)
        assert.is_nil(result)
      end)

    it("clamps a negative timeout to zero", function()
        local result, message = waitForEvent("mudletTestNeverRaised", -50)
        assert.is_nil(result)
        assert.truthy(message:find("0ms"))
      end)

    it("returns nil and a message for an empty event name", function()
        local result, message = waitForEvent("")
        assert.is_nil(result)
        assert.is_string(message)
        assert.truthy(message:find("empty"))
      end)

    it("errors when called without an event name", function()
        assert.has_error(function() waitForEvent() end)
      end)

    it("observes an event raised from inside another timer's callback", function()
        -- The #9670 shape: the wait itself is armed from inside a timer callback.
        tempTimer(0, function()
            tempTimer(0.05, function() raiseEvent("mudletTestNestedTimer", "deep") end)
            _G.mudletTestNestedTimerResult = {waitForEvent("mudletTestNestedTimer", 2000)}
          end)
        local waited = 0
        while not _G.mudletTestNestedTimerResult and waited < 5000 do
          pumpEvents(50)
          waited = waited + 50
        end
        local result = _G.mudletTestNestedTimerResult
        _G.mudletTestNestedTimerResult = nil
        assert.is_table(result, "the wait inside the timer callback never returned")
        assert.equals("mudletTestNestedTimer", result[1])
        assert.equals("deep", result[2])
      end)

    it("supports a nested waitForEvent while one is already blocked", function()
        local innerName
        tempTimer(0, function()
            -- Raise the shared event only once both waits are blocked, so both
            -- the outer wait and this inner one should observe it.
            tempTimer(0.05, function() raiseEvent("mudletTestNested", "payload") end)
            innerName = waitForEvent("mudletTestNested", 2000)
          end)
        local outerName, outerValue = waitForEvent("mudletTestNested", 2000)
        assert.equals("mudletTestNested", outerName)
        assert.equals("payload", outerValue)
        assert.equals("mudletTestNested", innerName)
      end)
  end)

describe("pumpEvents test helper", function()
    it("returns true once the time is up", function()
        assert.is_true(pumpEvents(20))
      end)

    it("accepts no argument and clamps a negative duration", function()
        assert.is_true(pumpEvents())
        assert.is_true(pumpEvents(-50))
      end)

    it("runs a timer that falls due while it is pumping", function()
        local fired = false
        tempTimer(0.05, function() fired = true end)
        pumpEvents(300)
        assert.is_true(fired, "a timer that came due during the pump did not fire")
      end)

    it("keeps running timers when pumping from inside a timer's callback", function()
        -- The #9670 shape: on macOS this position stops Qt timers entirely, so
        -- a regression hangs the spec rather than failing it.
        local result = {}
        tempTimer(0, function()
            tempTimer(0.05, function() result.innerFired = true end)
            pumpEvents(300)
            result.firedDuringPump = result.innerFired == true
            result.done = true
          end)
        local waited = 0
        while not result.done and waited < 5000 do
          pumpEvents(50)
          waited = waited + 50
        end
        assert.is_true(result.done, "the pump inside the timer callback never returned")
        assert.is_true(result.firedDuringPump, "a timer did not fire while pumping from inside a timer callback")
      end)
  end)
