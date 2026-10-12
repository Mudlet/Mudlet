-- echo() and echoUserWindow() to a window other than the main console: a mini
-- console, user window or buffer has the text appended to its buffer, and a
-- label has it put in place of what it showed.
describe("Tests echo() to a named window", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))

  local function lastLines(window, count)
    local last = getLastLineNumber(window)
    return getLines(window, last - count + 1, last + 1)
  end

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 100) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height, so this one is only closed and left behind
      {"user window", function(name) return openUserWindow(name, false) end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specSubConsoleEcho " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        before_each(function()
          clearWindow(window)
          echo(window, "SpecSubEcho first\n")
        end)

        it("answers true and appends the text to that console alone", function()
          local mainLast = getLastLineNumber("main")
          assert.are.same({true}, {echo(window, "SpecSubEcho second\n")})
          assert.are.same({"SpecSubEcho first", "SpecSubEcho second", ""}, lastLines(window, 3))
          assert.are.equal(mainLast, getLastLineNumber("main"), "echo() to a sub-console wrote to the main console")
        end)

        it("leaves a line without a line feed open for the next echo", function()
          echo(window, "SpecSubEcho open, ")
          assert.are.same({"SpecSubEcho open, "}, lastLines(window, 1))
          echo(window, "closed\n")
          assert.are.same({"SpecSubEcho open, closed", ""}, lastLines(window, 2))
        end)

        it("keeps carriage returns, which only an echo to the main console drops", function()
          echo(window, "SpecSubEcho a\rb\r\n")
          assert.are.same({"SpecSubEcho a\rb\r", ""}, lastLines(window, 2))
        end)

        it("appends at the end of the buffer wherever the cursor is", function()
          echo(window, "SpecSubEcho second\n")
          assert.is_true(moveCursor(window, 0, 0))
          echo(window, "SpecSubEcho appended\n")
          assert.are.same({"SpecSubEcho first", "SpecSubEcho second", "SpecSubEcho appended", ""}, lastLines(window, 4))
          assert.are.equal(0, getLineNumber(window), "echo() moved the cursor")
        end)

        it("writes in the console's current format", function()
          setFgColor(window, 12, 34, 56)
          setBold(window, true)
          echo(window, "SpecSubEcho formatted\n")
          resetFormat(window)
          -- selectString() searches the line the cursor is on
          assert.is_true(moveCursor(window, 0, getLastLineNumber(window) - 1))
          assert.are.equal(0, selectString(window, "SpecSubEcho formatted", 1))
          finally(function() deselect(window) end)
          assert.are.same({12, 34, 56}, {getFgColor(window)})
          assert.is_true(getTextFormat(window).bold)
        end)

        it("is what echoUserWindow() does too, answering nothing", function()
          assert.are.equal(0, select("#", echoUserWindow(window, "SpecSubEcho by echoUserWindow\n")))
          assert.are.same({"SpecSubEcho first", "SpecSubEcho by echoUserWindow", ""}, lastLines(window, 3))
        end)

        it("appends to that console from inside a trigger on the main console", function()
          local triggerId = tempRegexTrigger("^SpecSubEchoTriggerLine$", function()
            echo(window, "SpecSubEcho from a trigger\n")
          end)
          finally(function() killTrigger(triggerId) end)
          feedTriggers("SpecSubEchoTriggerLine\n")
          assert.are.same({"SpecSubEcho first", "SpecSubEcho from a trigger", ""}, lastLines(window, 3))
        end)
      end)
    end
  end)

  describe("with a label's name", function()
    local label = "specSubConsoleEcho label" .. suffix

    setup(function()
      createLabel(label, 0, 0, 50, 20, 1)
    end)

    teardown(function()
      deleteLabel(label)
    end)

    it("answers true and puts the text in place of the label's", function()
      assert.are.same({true}, {echo(label, "SpecSubEcho <b>one</b>")})
      assert.are.equal("SpecSubEcho <b>one</b>", getLabelText(label))
      assert.are.same({true}, {echo(label, "SpecSubEcho two\r\n")})
      assert.are.equal("SpecSubEcho two\r\n", getLabelText(label))
    end)

    it("is what echoUserWindow() does too, answering nothing", function()
      assert.are.equal(0, select("#", echoUserWindow(label, "SpecSubEcho three")))
      assert.are.equal("SpecSubEcho three", getLabelText(label))
    end)
  end)

  describe("with the name of no window", function()
    local unknown = "specSubConsoleEchoNoSuchWindow" .. suffix

    it("answers nil and why from echo()", function()
      assert.are.same({nil, ("console/label '%s' does not exist"):format(unknown)}, {echo(unknown, "text")})
    end)

    it("answers nothing from echoUserWindow()", function()
      assert.are.equal(0, select("#", echoUserWindow(unknown, "text")))
    end)
  end)
end)
