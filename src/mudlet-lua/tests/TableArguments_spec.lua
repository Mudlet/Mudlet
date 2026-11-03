-- Every function that takes its arguments as a table as well calls through to
-- the same work as its positional form, so the two must leave the same result
-- behind: each spec makes one object positionally and its twin from a table,
-- then reads both back.
describe("Tests that the table form of a function does what its positional form does", function()
  local tag = ("%d%d"):format(os.time(), math.random(100000))

  local function pack(...)
    return {n = select("#", ...), ...}
  end

  describe("for windows", function()
    local a, b = "tableArgsA" .. tag, "tableArgsB" .. tag

    after_each(function()
      deleteMiniConsole(a)
      deleteMiniConsole(b)
      deleteLabel(a)
      deleteLabel(b)
      deleteScrollBox(a)
      deleteScrollBox(b)
      deleteCommandLine(a)
      deleteCommandLine(b)
    end)

    local creators = {
      {"createMiniConsole", "miniconsole",
        function() return createMiniConsole("main", a, 11, 12, 210, 110) end,
        function() return createMiniConsole({windowName = "main", name = b, x = 11, y = 12, width = 210, height = 110}) end},
      {"createLabel", "label",
        function() return createLabel("main", a, 11, 12, 210, 110, 1) end,
        function() return createLabel({windowName = "main", name = b, x = 11, y = 12, width = 210, height = 110, fillBackground = true}) end},
      {"createLabel with 0/1 flags", "label",
        function() return createLabel(a, 11, 12, 210, 110, 0, 1) end,
        function() return createLabel({name = b, x = 11, y = 12, width = 210, height = 110, fillBackground = 0, clickthrough = 1}) end},
      {"createScrollBox", "scrollbox",
        function() return createScrollBox("main", a, 11, 12, 210, 110) end,
        function() return createScrollBox({windowName = "main", name = b, x = 11, y = 12, width = 210, height = 110}) end},
      {"createCommandLine", "commandline",
        function() return createCommandLine("main", a, 11, 12, 210, 110) end,
        function() return createCommandLine({windowName = "main", name = b, x = 11, y = 12, width = 210, height = 110}) end},
      {"createConsole", "miniconsole",
        function() return createConsole("main", a, 10, 20, 5, 11, 12) end,
        function() return createConsole({windowName = "main", consoleName = b, fontSize = 10, charsPerLine = 20, numberOfLines = 5, x = 11, y = 12}) end},
    }

    for _, creator in ipairs(creators) do
      local name, kind, positional, named = unpack(creator)
      it(name .. " makes the same window", function()
        local positionalResult, namedResult = pack(positional()), pack(named())
        assert.are.same(positionalResult, namedResult)
        assert.is_true(positionalResult[1])
        assert.are.equal(kind, windowType(a))
        assert.are.equal(kind, windowType(b))
        local geometry = pack(getWindowGeometry(a))
        assert.is_number(geometry[1])
        assert.are.same(geometry, pack(getWindowGeometry(b)))
      end)
    end

    it("createConsole sets the same font size", function()
      createConsole("main", a, 13, 20, 5, 0, 0)
      createConsole({consoleName = b, fontSize = 13, charsPerLine = 20, numberOfLines = 5, x = 0, y = 0})
      assert.are.equal(13, getFontSize(a))
      assert.are.equal(getFontSize(a), getFontSize(b))
    end)

    it("names the table field that has the wrong type", function()
      local ok, err = pcall(createLabel, {name = b, x = 0, y = 0, width = 10, height = 10, fillBackground = "yes"})
      assert.is_false(ok)
      assert.is_truthy(err:find("fillBackground as boolean/number (0/1) expected, got string!", 1, true), err)
      assert.is_nil(windowType(b))
    end)

    it("createMapper refuses a parent window that does not exist with the same answer", function()
      local unknown = "tableArgsNoSuchWindow" .. tag
      local positionalResult = pack(createMapper(unknown, 0, 0, 100, 100))
      assert.is_falsy(positionalResult[1])
      assert.are.same(positionalResult, pack(createMapper({windowName = unknown, x = 0, y = 0, width = 100, height = 100})))
    end)
  end)

  describe("for gauges", function()
    local a, b = "tableArgsGaugeA" .. tag, "tableArgsGaugeB" .. tag

    after_each(function()
      for _, name in ipairs({a, b}) do
        for _, part in ipairs({"_back", "_front", "_text"}) do
          deleteLabel(name .. part)
        end
        gaugesTable[name] = nil
      end
    end)

    local function assertSameGauge()
      assert.is_table(gaugesTable[a])
      assert.are.same(gaugesTable[a], gaugesTable[b])
      for _, part in ipairs({"_back", "_front", "_text"}) do
        assert.are.same(pack(getWindowGeometry(a .. part)), pack(getWindowGeometry(b .. part)))
      end
    end

    it("createGauge with colour numbers makes the same gauge", function()
      createGauge(a, 150, 20, 30, 40, "text", 10, 20, 30, "vertical")
      createGauge({name = b, width = 150, height = 20, x = 30, y = 40, text = "text", r = 10, g = 20, b = 30, orientation = "vertical"})
      assertSameGauge()
    end)

    it("createGauge with a colour name makes the same gauge", function()
      createGauge("main", a, 150, 20, 30, 40, "text", "purple", "goofy")
      createGauge({windowName = "main", name = b, width = 150, height = 20, x = 30, y = 40, text = "text", r = "purple", orientation = "goofy"})
      assertSameGauge()
    end)
  end)

  describe("for console text and colours", function()
    local a, b = "tableArgsTextA" .. tag, "tableArgsTextB" .. tag

    before_each(function()
      createMiniConsole(a, 0, 0, 300, 100)
      createMiniConsole(b, 0, 0, 300, 100)
      for _, name in ipairs({a, b}) do
        setFgColor(name, 200, 100, 50)
        setBgColor(name, 5, 6, 7)
        echo(name, "start ")
      end
    end)

    after_each(function()
      deleteMiniConsole(a)
      deleteMiniConsole(b)
    end)

    local function formatOf(name, text)
      selectString(name, text, 1)
      local format = {fg = pack(getFgColor(name)), bg = pack(getBgColor(name)), line = getCurrentLine(name)}
      deselect(name)
      return format
    end

    local writers = {
      {"echoLink using the current format",
        function() return echoLink(a, "linked", "send('x')", "hint", true) end,
        function() return echoLink({windowName = b, text = "linked", command = "send('x')", hint = "hint", useCurrentFormat = true}) end},
      {"echoLink using the link format",
        function() return echoLink(a, "linked", "send('x')", "hint", false) end,
        function() return echoLink({windowName = b, text = "linked", command = "send('x')", hint = "hint", useCurrentFormat = false}) end},
      {"echoLink with a function",
        function() return echoLink(a, "linked", function() end, "hint") end,
        function() return echoLink({windowName = b, text = "linked", command = function() end, hint = "hint"}) end},
      {"insertLink using the current format",
        function() return insertLink(a, "linked", "send('x')", "hint", true) end,
        function() return insertLink({windowName = b, text = "linked", command = "send('x')", hint = "hint", useCurrentFormat = true}) end},
      {"insertLink using the link format",
        function() return insertLink(a, "linked", "send('x')", "hint", false) end,
        function() return insertLink({windowName = b, text = "linked", command = "send('x')", hint = "hint", useCurrentFormat = false}) end},
      {"insertPopup using the current format",
        function() return insertPopup(a, "linked", {"send('x')", "send('y')"}, {"one", "two"}, true) end,
        function() return insertPopup({windowName = b, text = "linked", commands = {"send('x')", "send('y')"}, hints = {"one", "two"}, useCurrentFormat = true}) end},
      {"insertPopup using the link format",
        function() return insertPopup(a, "linked", {"send('x')"}, {"one"}) end,
        function() return insertPopup({windowName = b, text = "linked", commands = {"send('x')"}, hints = {"one"}}) end},
      {"prefix",
        function() return prefix("linked", nil, "green", "blue", a) end,
        function() return prefix({text = "linked", fgColor = "green", bgColor = "blue", window = b}) end},
      {"suffix",
        function() return suffix("<red>linked", cecho, nil, nil, a) end,
        function() return suffix({text = "<red>linked", func = cecho, window = b}) end},
    }

    for _, writer in ipairs(writers) do
      local name, positional, named = unpack(writer)
      it(name .. " writes the same text in the same format", function()
        assert.are.same(pack(positional()), pack(named()))
        local format = formatOf(a, "linked")
        assert.is_truthy(format.line:find("linked", 1, true))
        assert.are.same(format, formatOf(b, "linked"))
      end)
    end

    it("setTextFormat sets the same format", function()
      assert.are.same(
        pack(setTextFormat(a, 10, 20, 30, 40, 50, 60, true, false, true, false, true, false)),
        pack(setTextFormat({windowName = b, bgR = 10, bgG = 20, bgB = 30, fgR = 40, fgG = 50, fgB = 60,
          bold = true, underline = false, italics = true, strikeout = false, overline = true, reverse = false})))
      local function formatted(name)
        echo(name, "formatted")
        selectString(name, "formatted", 1)
        return getTextFormat(name)
      end
      local format = formatted(a)
      assert.is_true(format.bold)
      assert.are.same(format, formatted(b))
    end)

    it("setBackgroundColor sets the same colour", function()
      assert.are.same(pack(setBackgroundColor(a, 11, 22, 33, 200)), pack(setBackgroundColor({windowName = b, r = 11, g = 22, b = 33, alpha = 200})))
      assert.are.same({11, 22, 33, 200}, {getBackgroundColor(a)})
      assert.are.same(pack(getBackgroundColor(a)), pack(getBackgroundColor(b)))
    end)

    it("setBgColor sets the same colour on a selection", function()
      echo(a, "painted")
      echo(b, "painted")
      selectString(a, "painted", 1)
      selectString(b, "painted", 1)
      assert.are.same(pack(setBgColor(a, 11, 22, 33, 200)), pack(setBgColor({windowName = b, r = 11, g = 22, b = 33, alpha = 200})))
      local format = formatOf(a, "painted")
      assert.are.same({11, 22, 33}, {unpack(format.bg, 1, 3)})
      assert.are.same(format, formatOf(b, "painted"))
    end)

    for _, name in ipairs({"setCommandBackgroundColor", "setCommandForegroundColor"}) do
      it(name .. " answers the same", function()
        local setter = _G[name]
        enableCommandLine(a)
        enableCommandLine(b)
        assert.are.same(pack(setter(a, 11, 22, 33, 200)), pack(setter({windowName = b, r = 11, g = 22, b = 33, alpha = 200})))
        local unknown = "tableArgsNoSuchWindow" .. tag
        local refusal = pack(setter(unknown, 11, 22, 33))
        assert.is_nil(refusal[1])
        assert.are.same(refusal, pack(setter({windowName = unknown, r = 11, g = 22, b = 33})))
      end)
    end
  end)

  describe("for the map", function()
    local area, roomA, roomB, roomTo

    before_each(function()
      area = assert(addAreaName("tableArgsArea" .. tag))
      local rooms = {}
      for _, coordinates in ipairs({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}) do
        local id = createRoomID()
        assert.is_true(addRoom(id))
        setRoomArea(id, area)
        setRoomCoordinates(id, unpack(coordinates))
        rooms[#rooms + 1] = id
      end
      roomA, roomB, roomTo = unpack(rooms)
      setExit(roomA, roomTo, "n")
      setExit(roomB, roomTo, "n")
    end)

    after_each(function()
      deleteArea(area)
    end)

    local function labelsMatch(first, second)
      local one, two = getMapLabel(area, first), getMapLabel(area, second)
      assert.is_table(one)
      assert.is_table(two)
      assert.are.same(one, two)
    end

    it("createMapLabel makes the same label", function()
      local font = getFont()
      local first = createMapLabel(area, "label", 1, 2, 0, 10, 20, 30, 40, 50, 60, 2, 14, true, true, font, 100, 150, false, 70, 80, 90)
      local second = createMapLabel({area = area, text = "label", x = 1, y = 2, z = 0, fgRed = 10, fgGreen = 20, fgBlue = 30,
        bgRed = 40, bgGreen = 50, bgBlue = 60, zoom = 2, fontSize = 14, showOnTop = true, noScaling = true, fontName = font,
        foregroundTransparency = 100, backgroundTransparency = 150, temporary = false, outlineRed = 70, outlineGreen = 80, outlineBlue = 90})
      assert.is_number(first)
      assert.is_number(second)
      labelsMatch(first, second)
    end)

    it("createMapImageLabel makes the same label", function()
      local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
      local image = specDirectory .. "/fixtures/images/solid-magenta-4x4.png"
      local first = createMapImageLabel(area, image, 1, 2, 0, 3, 4, 5, true, false)
      local second = createMapImageLabel({areaID = area, imagePath = image, x = 1, y = 2, z = 0, width = 3, height = 4, zoom = 5, showOnTop = true, temporary = false})
      assert.is_number(first)
      assert.is_number(second)
      labelsMatch(first, second)
    end)

    it("addCustomLine to a room draws the same line", function()
      assert.are.same(pack(addCustomLine(roomA, roomTo, "n", "dash line", {10, 20, 30}, true)),
        pack(addCustomLine({roomID = roomB, toRoomID = roomTo, direction = "n", style = "dash line", color = {10, 20, 30}, arrow = true})))
      local line = getCustomLines(roomA)
      assert.is_table(line.n)
      assert.are.same(line, getCustomLines(roomB))
    end)

    it("addCustomLine through points draws the same line", function()
      local points = {{0, 1, 0}, {1, 1, 0}}
      assert.are.same(pack(addCustomLine(roomA, points, "n", "dot line", {10, 20, 30}, false)),
        pack(addCustomLine({roomID = roomB, coordinates = points, direction = "n", style = "dot line", color = {10, 20, 30}, arrow = false})))
      local line = getCustomLines(roomA)
      assert.is_table(line.n)
      assert.are.same(line, getCustomLines(roomB))
    end)

    it("highlightRoom answers the same", function()
      assert.are.same(pack(highlightRoom(roomA, 10, 20, 30, 40, 50, 60, 2, 100, 200)),
        pack(highlightRoom({roomID = roomB, fgr = 10, fgg = 20, fgb = 30, bgr = 40, bgg = 50, bgb = 60, radius = 2, alpha1 = 100, alpha2 = 200})))
      unHighlightRoom(roomA)
      unHighlightRoom(roomB)
    end)

    it("setCustomEnvColor sets the same colour", function()
      local envA, envB = 4242001, 4242002
      assert.are.same(pack(setCustomEnvColor(envA, 10, 20, 30, 40)), pack(setCustomEnvColor({environmentID = envB, r = 10, g = 20, b = 30, a = 40})))
      local colours = getCustomEnvColorTable()
      assert.is_table(colours[envA])
      assert.are.same(colours[envA], colours[envB])
    end)

    it("addMapEvent adds the same menu entry", function()
      local a, b = "tableArgsEventA" .. tag, "tableArgsEventB" .. tag
      finally(function()
        removeMapEvent(a)
        removeMapEvent(b)
      end)
      assert.are.same(pack(addMapEvent(a, "tableArgsEvent", "", "Display", 1, "two")),
        pack(addMapEvent({uniquename = b, eventname = "tableArgsEvent", parent = "", displayname = "Display", arguments = {1, "two"}})))
      local events = getMapEvents()
      assert.is_table(events[a])
      assert.are.same(events[a], events[b])
    end)
  end)

  describe("for triggers and keys", function()
    it("permKey binds the same key", function()
      local a, b = "tableArgsKeyA" .. tag, "tableArgsKeyB" .. tag
      local f35, shift = 0x01000052, 0x02000000
      local first = permKey(a, "", shift, f35, "")
      local second = permKey({name = b, parent = "", modifier = shift, keyCode = f35, code = ""})
      disableKey(a)
      disableKey(b)
      assert.is_number(first)
      assert.is_number(second)
      assert.are.same({f35, shift}, {getKeyCode(first)})
      assert.are.same(pack(getKeyCode(first)), pack(getKeyCode(second)))
    end)

    describe("tempComplexRegexTrigger", function()
      local a, b = "tableArgsTriggerA" .. tag, "tableArgsTriggerB" .. tag
      local pattern = [[^tableArgsTrigger (\w+)$]]

      after_each(function()
        killTrigger(a)
        killTrigger(b)
        _G.tableArgsSpecFired = nil
      end)

      it("runs the same code", function()
        -- trigger code runs in _G, not in the environment busted gives this file
        _G.tableArgsSpecFired = {}
        tempComplexRegexTrigger(a, pattern, [[tableArgsSpecFired[#tableArgsSpecFired + 1] = "a " .. matches[2] ]], 0, 0, 0, 0, 0, 0, 0, "", 0, 0)
        tempComplexRegexTrigger({name = b, regex = pattern, code = [[tableArgsSpecFired[#tableArgsSpecFired + 1] = "b " .. matches[2] ]],
          multiline = false, fgColor = 0, bgColor = 0, filter = false, matchAll = false, highlightFgColor = 0, highlightBgColor = 0,
          soundFile = "", fireLength = 0, lineDelta = 0})
        feedTriggers("tableArgsTrigger one\n")
        assert.are.same({"a one", "b one"}, _G.tableArgsSpecFired)
      end)

      it("runs the same function", function()
        local fired = {}
        tempComplexRegexTrigger(a, pattern, function() fired[#fired + 1] = "a " .. matches[2] end, 0, 0, 0, 0, 0, 0, 0, "", 0, 0)
        tempComplexRegexTrigger({name = b, regex = pattern, code = function() fired[#fired + 1] = "b " .. matches[2] end,
          multiline = false, fgColor = 0, bgColor = 0, filter = false, matchAll = false, highlightFgColor = 0, highlightBgColor = 0,
          soundFile = "", fireLength = 0, lineDelta = 0})
        feedTriggers("tableArgsTrigger two\n")
        assert.are.same({"a two", "b two"}, fired)
      end)

      it("matches all the same with matchAll given as a number", function()
        local count = {a = 0, b = 0}
        local words = [[^tableArgsTrigger|\bword\b]]
        tempComplexRegexTrigger(a, words, function() count.a = count.a + #matches end, 0, 0, 0, 0, 1, 0, 0, "", 0, 0)
        tempComplexRegexTrigger({name = b, regex = words, code = function() count.b = count.b + #matches end,
          multiline = 0, fgColor = 0, bgColor = 0, filter = 0, matchAll = 1, highlightFgColor = 0, highlightBgColor = 0,
          soundFile = "", fireLength = 0, lineDelta = 0})
        feedTriggers("tableArgsTrigger word word word\n")
        assert.are.equal(4, count.a)
        assert.are.same(count.a, count.b)
      end)

      it("expires after the same number of matches", function()
        local count = {a = 0, b = 0}
        tempComplexRegexTrigger(a, pattern, function() count.a = count.a + 1 end, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 2)
        tempComplexRegexTrigger({name = b, regex = pattern, code = function() count.b = count.b + 1 end,
          multiline = false, fgColor = 0, bgColor = 0, filter = false, matchAll = false, highlightFgColor = 0, highlightBgColor = 0,
          soundFile = "", fireLength = 0, lineDelta = 0, expireAfter = 2})
        for _ = 1, 3 do
          feedTriggers("tableArgsTrigger again\n")
        end
        assert.are.same({a = 2, b = 2}, count)
      end)
    end)
  end)

  describe("for named handlers and timers", function()
    local user = "tableArgsUser" .. tag

    after_each(function()
      deleteAllNamedEventHandlers(user)
      deleteAllNamedTimers(user)
    end)

    it("registerNamedEventHandler handles the event the same way", function()
      local count = {a = 0, b = 0}
      local event = "tableArgsEvent" .. tag
      registerNamedEventHandler(user, "a", event, function() count.a = count.a + 1 end, true)
      registerNamedEventHandler({user = user, name = "b", eventName = event, handler = function() count.b = count.b + 1 end, oneShot = true})
      raiseEvent(event)
      raiseEvent(event)
      assert.are.same({a = 1, b = 1}, count)
    end)

    -- the timer engine is stubbed so that what registerNamedTimer() hands
    -- tempTimer() can be seen without waiting for a timer to fire
    local function tempTimerCalls(register)
      local calls = {}
      local realTempTimer = _G.tempTimer
      _G.tempTimer = function(time, _, repeating)
        calls[#calls + 1] = {time, repeating}
        return realTempTimer(600, function() end)
      end
      local ok, err = pcall(register)
      _G.tempTimer = realTempTimer
      assert(ok, err)
      return calls
    end

    for _, repeating in ipairs({true, false}) do
      it(("registerNamedTimer makes the same %s timer"):format(repeating and "repeating" or "one-shot"), function()
        local fn = function() end
        local positional = tempTimerCalls(function() registerNamedTimer(user, "a", 5, fn, repeating) end)
        assert.are.same({{5, repeating}}, positional)
        assert.are.same(positional, tempTimerCalls(function()
          registerNamedTimer({user = user, name = "b", time = 5, handler = fn, repeating = repeating})
        end))
        assert.are.same(positional, tempTimerCalls(function()
          registerNamedTimer({user = user, name = "c", time = 5, handler = fn, oneShot = not repeating})
        end))
      end)
    end

    it("timeframe sets the variable to the same values", function()
      local function valuesOver(start)
        local timers = {}
        local realTempTimer, realKillTimer = _G.tempTimer, _G.killTimer
        _G.tempTimer = function(time, fn)
          timers[#timers + 1] = {time, fn}
          return #timers
        end
        _G.killTimer = function() return true end
        local values = {}
        local ok, err = pcall(function()
          start()
          values[1] = tostring(_G.tableArgsSpecTimeframe)
          table.sort(timers, function(one, two) return one[1] < two[1] end)
          for _, timer in ipairs(timers) do
            timer[2]()
            values[#values + 1] = timer[1] .. "=" .. tostring(_G.tableArgsSpecTimeframe)
          end
        end)
        _G.tempTimer, _G.killTimer = realTempTimer, realKillTimer
        _G.tableArgsSpecTimeframe = nil
        assert(ok, err)
        return values
      end

      local positional = valuesOver(function() timeframe("tableArgsSpecTimeframe", 1, 4, {2, "two"}, {3, 3}) end)
      assert.are.same({"nil", "1=true", "2=two", "3=3", "4=nil", "4.1=nil"}, positional)
      assert.are.same(positional, valuesOver(function()
        timeframe({vname = "tableArgsSpecTimeframe", true_time = 1, nil_time = 4, timerlist = {{2, "two"}, {3, 3}}})
      end))
    end)
  end)
end)
