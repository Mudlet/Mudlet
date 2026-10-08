-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console. The link and popup functions can
-- be handed Lua functions as their commands, which Mudlet anchors in the Lua
-- registry as it reads them, so a call that refuses must not leave any behind.
describe("Tests that the link and text functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specLinksByNameNoSuchWindow" .. suffix

  -- replace() is wrapped in Lua to add a keepcolor argument, and the wrapper
  -- drops whatever the function it wraps answers, so what that function
  -- answers is only seen by calling it directly
  local rawReplace
  for index = 1, math.huge do
    local name, value = debug.getupvalue(replace, index)
    if not name then
      error("the function replace() wraps was not found")
    end
    if name == "oldreplace" then
      rawReplace = value
      break
    end
  end

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order; each is a call that would succeed on a console
  local calls = {
    {"echoLink", function(n) return echoLink(n, "text", "cmd", "hint") end},
    {"echoPopup", function(n) return echoPopup(n, "text", {"cmd"}, {"hint"}) end},
    {"insertLink", function(n) return insertLink(n, "text", "cmd", "hint") end},
    {"insertPopup", function(n) return insertPopup(n, "text", {"cmd"}, {"hint"}) end},
    {"setLink", function(n) return setLink(n, "cmd", "hint") end},
    {"setPopup", function(n) return setPopup(n, {"cmd"}, {"hint"}) end},
    {"insertText", function(n) return insertText(n, "text") end},
    {"replace", function(n) return rawReplace(n, "text") end},
  }

  -- the calls that take commands, each handed a Lua function among them
  local function anyFunction(fn)
    return fn or function() end
  end
  local functionCalls = {
    {"echoLink", function(n, fn) return echoLink(n, "text", anyFunction(fn), "hint") end},
    {"echoPopup", function(n, fn) return echoPopup(n, "text", {anyFunction(fn), "cmd"}, {"one", "two"}) end},
    {"insertLink", function(n, fn) return insertLink(n, "text", anyFunction(fn), "hint") end},
    {"insertPopup", function(n, fn) return insertPopup(n, "text", {anyFunction(fn), "cmd"}, {"one", "two"}) end},
    {"setLink", function(n, fn) return setLink(n, anyFunction(fn), "hint") end},
    {"setPopup", function(n, fn) return setPopup(n, {anyFunction(fn), "cmd"}, {"one", "two"}) end},
  }

  local function assertRefuses(windowName, entries)
    local notFound = ('window "%s" not found'):format(windowName)
    for _, entry in ipairs(entries) do
      local functionName, call = entry[1], entry[2]
      assert.are.equal(2, select("#", call(windowName)), functionName)
      local ok, err = call(windowName)
      assert.is_nil(ok, functionName)
      assert.are.equal(notFound, err, functionName)
    end
  end

  local function registryEntryCount()
    local count = 0
    for _ in pairs(debug.getregistry()) do
      count = count + 1
    end
    return count
  end

  -- Counts the registry across 20 calls, after one warm-up call: releasing a
  -- reference puts it on the registry's free list, and the first release is
  -- what creates that list, so a count taken across it grows even when nothing
  -- leaks.
  local function registryGrowthOver(callOnce)
    callOnce()
    local before = registryEntryCount()
    for _ = 1, 20 do
      callOnce()
    end
    return registryEntryCount() - before
  end

  -- The registry keeps its released references in a list threaded through
  -- the slots themselves, headed at index 0. Releasing one reference twice
  -- links its slot back into that list, so the next two functions anchored
  -- would share a slot.
  local function assertFreeListWhole()
    local registry = debug.getregistry()
    local seen = {}
    local ref = registry[0]
    while type(ref) == "number" and ref ~= 0 do
      assert.is_nil(seen[ref], ("registry slot %d is on the free list twice"):format(ref))
      seen[ref] = true
      ref = registry[ref]
    end
  end

  -- whether fn, once handed to call and dropped, is still held on to anywhere
  local function keptAfter(call)
    local weak = setmetatable({}, {__mode = "k"})
    local function handOver()
      local fn = function() end
      weak[fn] = true
      call(fn)
    end
    handOver()
    collectgarbage("collect")
    collectgarbage("collect")
    return next(weak) ~= nil
  end

  -- the main console's last line, which a call aimed elsewhere must not touch
  local function mainTail()
    local last = getLastLineNumber("main")
    return last, getLines("main", last, last + 1)[1]
  end

  it("refuses a name that is no window at all", function()
    assertRefuses(unknown, calls)
    assertRefuses(unknown, functionCalls)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specLinksByNameOtherWindow" .. suffix

    after_each(function()
      deleteLabel(otherName)
      deleteScrollBox(otherName)
      deleteTextEdit(otherName)
      deleteCommandLine(otherName)
    end)

    local kinds = {
      {"label", function() return createLabel(otherName, 0, 0, 50, 20, 1) end},
      {"scroll box", function() return createScrollBox(otherName, 0, 0, 50, 20) end},
      {"text edit", function() return createTextEdit("main", otherName, 0, 0, 50, 20) end},
      {"command line", function() return createCommandLine(otherName, 0, 0, 50, 20) end},
    }

    for _, kind in ipairs(kinds) do
      it("refuses every link and text function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefuses(otherName, calls)
        assertRefuses(otherName, functionCalls)
      end)
    end
  end)

  describe("before it reads the commands", function()
    it("looks for the window, so refusing an unknown one keeps no function", function()
      for _, entry in ipairs(functionCalls) do
        local grewBy = registryGrowthOver(function()
          entry[2](unknown)
        end)
        assert.are.equal(0, grewBy, entry[1])
        assert.is_false(keptAfter(function(fn)
          entry[2](unknown, fn)
        end), entry[1])
      end
      assertFreeListWhole()
    end)

    it("reports an unknown window ahead of commands and hints that do not pair up", function()
      local notFound = {nil, ('window "%s" not found'):format(unknown)}
      assert.are.same(notFound, {echoPopup(unknown, "text", {"one", "two"}, {"one"})})
      assert.are.same(notFound, {insertPopup(unknown, "text", {"one", "two"}, {"one"})})
      assert.are.same(notFound, {setPopup(unknown, {"one", "two"}, {"one"})})
    end)

    it("checks every argument, taking no function when one is wrong", function()
      local fn = function() end
      local wrong = {
        function() echoLink(unknown, {}, "cmd", "hint") end,
        function() echoLink(unknown, "text", {}, "hint") end,
        function() echoLink(unknown, "text", fn, {}) end,
        function() echoLink(unknown, "text", fn, "hint", "yes") end,
        function() echoPopup(unknown, {}, {fn}, {"hint"}) end,
        function() echoPopup(unknown, "text", "cmd", {"hint"}) end,
        function() echoPopup(unknown, "text", {fn, {}}, {"one", "two"}) end,
        function() echoPopup(unknown, "text", {fn}, "hint") end,
        function() echoPopup(unknown, "text", {fn}, {"hint"}, "yes") end,
        function() insertLink(unknown, {}, "cmd", "hint") end,
        function() insertLink(unknown, "text", {}, "hint") end,
        function() insertLink(unknown, "text", fn, {}) end,
        function() insertLink(unknown, "text", fn, "hint", "yes") end,
        function() insertPopup(unknown, {}, {fn}, {"hint"}) end,
        function() insertPopup(unknown, "text", "cmd", {"hint"}) end,
        function() insertPopup(unknown, "text", {fn, {}}, {"one", "two"}) end,
        function() insertPopup(unknown, "text", {fn}, "hint") end,
        function() insertPopup(unknown, "text", {fn}, {"hint"}, "yes") end,
        function() setLink({}, "cmd", "hint") end,
        function() setLink(unknown, {}, "hint") end,
        function() setLink(unknown, fn, {}) end,
        function() setPopup({}, {fn}, {"hint"}) end,
        function() setPopup(unknown, "cmd", {"hint"}) end,
        function() setPopup(unknown, {fn, {}}, {"one", "two"}) end,
        function() setPopup(unknown, {fn}, "hint") end,
        function() insertText({}, "text") end,
        function() insertText(unknown, {}) end,
        function() rawReplace({}, "text") end,
        function() rawReplace(unknown, {}) end,
      }
      for index, call in ipairs(wrong) do
        assert.has_error(call, nil, ("call #%d"):format(index))
      end
      local grewBy = registryGrowthOver(function()
        for _, call in ipairs(wrong) do
          pcall(call)
        end
      end)
      assert.are.equal(0, grewBy)
      assertFreeListWhole()
    end)
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 200) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height, so this one is only closed and left behind
      {"user window", function(name) return openUserWindow(name, false) end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specLinksByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        local mainLast, mainLine

        setup(function()
          assert.is_true(kind[2](window))
          setWindowWrap(window, 200)
        end)

        teardown(function()
          kind[3](window)
        end)

        before_each(function()
          clearWindow(window)
          moveCursor(window, 0, 0)
          deselect(window)
          mainLast, mainLine = mainTail()
        end)

        after_each(function()
          local last, line = mainTail()
          assert.are.equal(mainLast, last, "the main console gained a line")
          assert.are.equal(mainLine, line, "the main console's last line changed")
        end)

        local function lineOf(y)
          moveCursor(window, 0, y)
          return getCurrentLine(window)
        end

        it("echoes a link and a popup onto that console", function()
          assert.are.same({true}, {echoLink(window, "linked", "cmd", "hint")})
          assert.are.same({true}, {echoPopup(window, " popped", {"one", "two"}, {"one", "two"})})
          assert.are.equal("linked popped", lineOf(0))
        end)

        it("inserts a link, a popup and text at that console's cursor", function()
          echo(window, "HelloWorld\n")
          moveCursor(window, 5, 0)
          assert.are.same({true}, {insertText(window, "!")})
          moveCursor(window, 5, 0)
          assert.are.same({true}, {insertLink(window, "-link-", "cmd", "hint")})
          moveCursor(window, 5, 0)
          assert.are.same({true}, {insertPopup(window, "-popup-", {"cmd"}, {"hint"})})
          assert.are.equal("Hello-popup--link-!World", lineOf(0))
        end)

        it("replaces that console's selection and answers nothing", function()
          echo(window, "replace me\n")
          moveCursor(window, 0, 0)
          assert.are.equal(8, selectString(window, "me", 1))
          assert.are.equal(0, select("#", rawReplace(window, "you")))
          assert.are.equal("replace you", lineOf(0))
        end)

        it("keeps the functions of a link and a popup set on that console's selection", function()
          echo(window, "link me\n")
          moveCursor(window, 0, 0)
          assert.are.equal(0, selectString(window, "link", 1))
          assert.is_true(keptAfter(function(fn)
            assert.are.same({true}, {setLink(window, fn, "hint")})
          end))
          assert.is_true(keptAfter(function(fn)
            assert.are.same({true}, {setPopup(window, {fn}, {"hint"})})
          end))
          assert.is_true(keptAfter(function(fn)
            assert.are.same({true}, {echoLink(window, "more", fn, "hint")})
          end))
          assert.is_true(keptAfter(function(fn)
            assert.are.same({true}, {insertPopup(window, "more", {fn}, {"hint"})})
          end))
          assertFreeListWhole()
        end)

        it("refuses commands and hints that do not pair up, keeping no function", function()
          local refusal = "command table and hint table sizes do not match up (2 and 1, either they must be the same or there should be one extra hint) - cannot create popup"
          for _, call in ipairs({
            function(fn) return echoPopup(window, "text", {fn, "cmd"}, {"one"}) end,
            function(fn) return insertPopup(window, "text", {fn, "cmd"}, {"one"}) end,
            function(fn) return setPopup(window, {fn, "cmd"}, {"one"}) end,
          }) do
            assert.are.same({nil, refusal}, {call(function() end)})
            assert.is_false(keptAfter(call))
            assert.are.equal(0, registryGrowthOver(function()
              call(function() end)
            end))
          end
          assertFreeListWhole()
        end)

        it("refuses to link no selection or no text, keeping no function", function()
          echo(window, "nothing selected\n")
          moveCursor(window, 1, 0)
          for _, entry in ipairs({
            {"no text is selected - cannot create link", function(fn) return setLink(window, fn, "hint") end},
            {"no text is selected - cannot create popup", function(fn) return setPopup(window, {fn}, {"hint"}) end},
            {"text is empty - cannot create link", function(fn) return insertLink(window, "", fn, "hint") end},
            {"text is empty - cannot create popup", function(fn) return insertPopup(window, "", {fn}, {"hint"}) end},
          }) do
            local refusal, call = entry[1], entry[2]
            assert.are.same({nil, refusal}, {call(function() end)})
            assert.is_false(keptAfter(call))
            assert.are.equal(0, registryGrowthOver(function()
              call(function() end)
            end))
          end
          assertFreeListWhole()
        end)
      end)
    end
  end)

  describe("with the main console", function()
    local function recentLinesHave(text)
      local last = getLastLineNumber("main")
      for _, line in ipairs(getLines("main", math.max(0, last - 2), last + 1)) do
        if line:find(text, 1, true) then
          return true
        end
      end
      return false
    end

    after_each(function()
      echo("main", "\n")
      deselect("main")
      moveCursorEnd("main")
    end)

    for _, name in ipairs({"", "main"}) do
      it(("takes %q for the main console"):format(name), function()
        local marker = ("specLinksByName%q"):format(name) .. suffix
        assert.are.same({true}, {echoLink(name, marker .. "-link", "cmd", "hint")})
        assert.are.same({true}, {echoPopup(name, marker .. "-popup", {"cmd"}, {"hint"})})
        assert.is_true(recentLinesHave(marker .. "-link" .. marker .. "-popup"))
        assert.is_true(keptAfter(function(fn)
          assert.are.same({true}, {echoLink(name, marker, fn, "hint")})
        end))
      end)
    end

    it("takes no name for the main console", function()
      local marker = "specLinksByNameNoName" .. suffix
      assert.are.same({true}, {echoLink(marker .. "-link", "cmd", "hint")})
      assert.are.same({true}, {echoPopup(marker .. "-popup", {"cmd"}, {"hint"})})
      assert.is_true(recentLinesHave(marker .. "-link" .. marker .. "-popup"))
      moveCursorEnd("main")
      assert.is_true(selectString(marker .. "-link", 1) > -1)
      assert.are.same({true}, {setLink("cmd", "hint")})
      assert.are.same({true}, {setPopup({"cmd"}, {"hint"})})
    end)

    -- Text put into the line a trigger is running over moves every capture
    -- after it, so the trigger's captures have to move with it to stay
    -- selectable. Only the main console's line is the trigger's.
    describe("while a trigger runs", function()
      local seen

      local function captureAfter(tag, edit)
        seen = nil
        local line = "SpecLinksByName" .. tag .. " Marker payload"
        local trigger = tempRegexTrigger("^SpecLinksByName" .. tag .. " Marker (\\w+)$", function()
          moveCursor(0, getLineNumber())
          edit()
          selectCaptureGroup(2)
          seen = {selection = getSelection(), line = getCurrentLine()}
          deselect()
        end)
        feedTriggers("\n" .. line .. "\n")
        killTrigger(trigger)
        assert.is_not_nil(seen, "the trigger did not fire")
        return seen.selection, seen.line
      end

      local inserts = {
        {"insertText with no name", function() insertText("ZZZZ") end},
        {"insertText by \"\"", function() insertText("", "ZZZZ") end},
        {"insertText by \"main\"", function() insertText("main", "ZZZZ") end},
        {"insertLink by \"main\"", function() insertLink("main", "ZZZZ", "cmd", "hint") end},
        {"insertPopup by \"main\"", function() insertPopup("main", "ZZZZ", {"cmd"}, {"hint"}) end},
      }

      for index, insert in ipairs(inserts) do
        it("moves its captures past " .. insert[1], function()
          local selection, line = captureAfter("Insert" .. index, insert[2])
          assert.are.equal("payload", selection)
          assert.are.equal("ZZZZSpecLinksByNameInsert" .. index .. " Marker payload", line)
        end)
      end

      for _, name in ipairs({"", "main"}) do
        for _, replacement in ipairs({"LongerLead", "L"}) do
          it(("moves its captures when replace by %q makes the line %s"):format(name, #replacement > 4 and "longer" or "shorter"), function()
            local tag = "Replace" .. #name .. #replacement
            local selection, line = captureAfter(tag, function()
              assert.are.equal(0, selectString("Spec", 1))
              replace(name, replacement)
            end)
            assert.are.equal("payload", selection)
            assert.are.equal(replacement .. "LinksByName" .. tag .. " Marker payload", line)
          end)
        end
      end

      it("leaves its captures alone for text put into a mini console", function()
        local window = "specLinksByName trigger mini console" .. suffix
        assert.is_true(createMiniConsole(window, 0, 0, 300, 200))
        finally(function()
          deleteMiniConsole(window)
        end)
        local selection, line = captureAfter("Mini", function()
          echo(window, "HelloWorld\n")
          moveCursor(window, 5, 0)
          insertText(window, "ZZZZ")
          moveCursor(window, 0, 0)
          insertLink(window, "YY", "cmd", "hint")
        end)
        assert.are.equal("payload", selection)
        assert.are.equal("SpecLinksByNameMini Marker payload", line)
        moveCursor(window, 0, 0)
        assert.are.equal("YYHelloZZZZWorld", getCurrentLine(window))
      end)
    end)
  end)
end)
