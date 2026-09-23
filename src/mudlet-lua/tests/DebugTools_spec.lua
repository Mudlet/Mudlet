describe("Tests DebugTools.lua functions", function()
  describe("Tests the functionality of prettywrite", function()
    it("Should still format the way inspect does", function()
      -- prettywrite is the old name for inspect, kept for old scripts; nothing
      -- inside Mudlet calls it, so nothing else would notice it drifting
      assert.equals(inspect({1, 2}), prettywrite({1, 2}))
      assert.equals(inspect({a = {b = 1}}), prettywrite({a = {b = 1}}))
    end)
  end)

  describe("Tests the functionality of printDebug", function()
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
      assert.spy(s).was_called_with(match.has_match("%(.+[\\/]busted[\\/]core.lua:line %d+%) This is a test"))
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

  describe("Tests the functionality of printError", function()
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
      assert.spy(s).was_called_with(match.has_match("This is a test"), match.has_match("%(.+[\\/]busted[\\/]core.lua:line %d+%)"))
    end)

    it("should include a stacktrace when called with stackTrace true", function()
      printError("This is a test", true)
      assert.spy(s).was_called(1)
      assert.spy(s).was_called_with(match.has_match("\nstack traceback:\n"), match.has_match("%(.+[\\/]busted[\\/]core.lua:line %d+%)"))
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

  describe("Tests the functionality of display", function()
    local function mainConsoleText()
      return table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
    end

    before_each(function()
      clearWindow()
    end)

    it("Should write an inspected table to the main console", function()
      display({alpha = 1, beta = "two"})
      local text = mainConsoleText()
      assert.is_truthy(text:find("alpha", 1, true))
      assert.is_truthy(text:find("beta", 1, true))
      assert.is_truthy(text:find("two", 1, true))
    end)

    it("Should write scalars the way inspect renders them", function()
      display("hello")
      assert.is_truthy(mainConsoleText():find('"hello"', 1, true))
    end)

    it("Should render nil rather than printing nothing", function()
      display(nil)
      assert.is_truthy(mainConsoleText():find("nil", 1, true))
    end)

    it("Should display each argument in the order it was given", function()
      display("first", "second")
      local text = mainConsoleText()
      local first, second = text:find('"first"', 1, true), text:find('"second"', 1, true)
      assert.is_truthy(first)
      assert.is_truthy(second)
      assert.is_true(first < second, "the arguments should be rendered in order")
    end)

    it("Should keep the position of a nil in the middle of its arguments", function()
      display("before", nil, "after")
      local text = mainConsoleText()
      local before = text:find('"before"', 1, true)
      local nilAt = text:find("nil", 1, true)
      local after = text:find('"after"', 1, true)
      assert.is_truthy(before)
      assert.is_truthy(nilAt)
      assert.is_truthy(after)
      assert.is_true(before < nilAt and nilAt < after, "the nil should keep its place between the two strings")
    end)

    it("Should return nothing, so its result cannot be echoed by accident", function()
      assert.equals(0, select("#", display("hello")))
      assert.equals(0, select("#", display("one", "two")))
    end)
  end)

  describe("Tests the functionality of showMultimatches", function()
    local savedMultimatches

    before_each(function()
      clearWindow()
      savedMultimatches = _G.multimatches
    end)

    after_each(function()
      _G.multimatches = savedMultimatches
    end)

    it("Should list every regex and its captures", function()
      _G.multimatches = {
        {"first whole match", "first capture"},
        {"second whole match"},
      }
      showMultimatches()
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_truthy(text:find("multimatches[n][m]", 1, true))
      assert.is_truthy(text:find("regex 1 captured", 1, true))
      assert.is_truthy(text:find("regex 2 captured", 1, true))
      assert.is_truthy(text:find("key=1 value=first whole match", 1, true))
      assert.is_truthy(text:find("key=2 value=first capture", 1, true))
      assert.is_truthy(text:find("key=1 value=second whole match", 1, true))
    end)

    it("Should still print its banner when there is nothing to show", function()
      _G.multimatches = {}
      showMultimatches()
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_truthy(text:find("multimatches[n][m]", 1, true))
      assert.is_falsy(text:find("captured", 1, true))
    end)
  end)

  describe("Tests the functionality of showCaptureGroups", function()
    it("Should recolour every capture group of the match", function()
      local selectSpy, captured, defaultFormat, groupFormat
      local id = tempRegexTrigger("^You wave (goodbye) to (everyone)\\.$", function()
        captured = table.size(matches)
        selectString("You wave", 1)
        defaultFormat = getTextFormat().foreground
        selectSpy = spy.on(_G, "selectCaptureGroup")
        -- Mudlet swallows errors raised inside a trigger, so revert through
        -- pcall rather than leaving the spy installed for the whole process
        pcall(showCaptureGroups)
        selectCaptureGroup:revert()
        selectString("goodbye", 1)
        groupFormat = getTextFormat().foreground
      end)
      feedTriggers("You wave goodbye to everyone.\n")
      killTrigger(id)

      assert.is_equal(3, captured, "the whole match plus two capture groups should be present")
      assert.spy(selectSpy).was.called(3)
      -- the colours it picks are random, so the assertion is that the capture
      -- group no longer wears the colour the rest of the line does
      assert.are_not.same(defaultFormat, groupFormat)
    end)
  end)
end)
