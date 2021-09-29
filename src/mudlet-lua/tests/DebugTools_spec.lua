describe("Tests DebugTools.lua functions", function()
  describe("printDebug(msg[, showTrace])", function()
    local s
    before_each(function()
      s = spy.on(_G, "debugc")
    end)

    after_each(function()
      debugc:revert()
      s = nil
    end)

    it("should pass a message on to debugc", function()
      printDebug("This is a test")
      assert.spy(s).was_called(1)
      assert.spy(s).was_called_with(match.has_match("%(.+\\busted\\core.lua:line %d+%) This is a test"))
    end)

    it("should include a stacktrace when called with stackTrace true", function()
      printDebug("This is a test", true)
      assert.spy(s).was_called(1)
      assert.spy(s).was_called_with(match.has_match("\nstack traceback:\n"))
    end)

    it("should NOT include a stacktrace when called without stackTrace true", function()
      printDebug("This is a test", false)
      assert.spy(s).was_called(1)
      assert.spy(s).was_not_called_with(match.has_match("\nstack traceback:\n"))
    end)
  end)

  describe("printError(msg[, showTrace, haltExecution])", function()
    local s
    before_each(function()
      s = spy.on(_G, "errorc")
    end)

    after_each(function()
      errorc:revert()
      s = nil
    end)

    it("should pass msg on to error with additional information", function()
      printError("This is a test")
      assert.spy(s).was_called(1)
      assert.spy(s).was_called_with(match.has_match("This is a test"), match.has_match("%(.+\\busted\\core.lua:line %d+%)"))
    end)

    it("should include a stacktrace when called with stackTrace true", function()
      printError("This is a test", true)
      assert.spy(s).was_called(1)
      assert.spy(s).was_called_with(match.has_match("\nstack traceback:\n"), match.has_match("%(.+\\busted\\core.lua:line %d+%)"))
    end)

    it("should NOT include a stacktrace when called without stackTrace true", function()
      printError("This is a test", false)
      assert.spy(s).was_called(1)
      assert.spy(s).was_not_called_with(match.has_match("\nstack traceback:\n"))
    end)

    it("should raise an error when haltExecution is set to true", function()
      local errFunc = function()
        printError("This is a test", false, true)
      end
      assert.error_matches(errFunc, " This is a test")
    end)

    it("should include a stacktrace when stackTrace and haltExecution are set to true", function()
      local errFunc = function()
        printError("This is a test", true, true)
      end
      assert.error_matches(errFunc, "\nstack traceback:\n")
    end)

    it("should NOT include a stacktrace when stackTrace and haltExecution are set to true", function()
      local errFunc = function()
        printError("This is a test", false, true)
      end
      assert.Not.error_matches(errFunc, "\nstack traceback:\n")
    end)

  end)
end)