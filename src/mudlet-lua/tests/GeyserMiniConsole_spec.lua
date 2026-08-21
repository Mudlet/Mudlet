local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.MiniConsole", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  local function alive(object)
    if not object or not object.container or not object.container.windowList then
      return false
    end
    return object.container.windowList[object.name] == object
  end

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.MiniConsole:new/new2", function()
    it("creates a miniconsole widget at the constrained geometry", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcNew", x = 30, y = 40, width = 300, height = 150}))
      -- Geyser's own type string is camel cased, Mudlet's windowType is not
      assert.are.equal("miniConsole", console.type)
      assert.are.equal("miniconsole", windowType("gmcNew"))
      assert.are.same({x = 30, y = 40, width = 300, height = 150}, geometry("gmcNew"))
      assert.is_true(windowVisible("gmcNew"))
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gmcBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.MiniConsole:new({name = "gmcInBox", x = "25%", y = "50%", width = "50%", height = "50%"}, container))
      assert.are.same({x = 200, y = 150, width = 200, height = 100}, geometry("gmcInBox"))
    end)

    it("takes the font size from its container when it is not given one", function()
      local container = track(Geyser.Container:new({name = "gmcFontBox", x = 0, y = 0, width = 200, height = 100, fontSize = 12}))
      track(Geyser.MiniConsole:new({name = "gmcInheritsFont"}, container))
      assert.are.equal(12, getFontSize("gmcInheritsFont"))
    end)

    it("new2 marks the console as using add2", function()
      local console = track(Geyser.MiniConsole:new2({name = "gmcNew2", x = 0, y = 0, width = 100, height = 50}))
      assert.is_true(console.useAdd2)
      assert.are.equal("miniconsole", windowType("gmcNew2"))
    end)

    it("starts out hidden when the constraints ask for it", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcHiddenNew", x = 0, y = 0, width = 100, height = 50, hidden = true}))
      assert.is_true(console.hidden)
      assert.is_false(windowVisible("gmcHiddenNew"))
      console:show()
      assert.is_false(console.hidden)
      assert.is_true(windowVisible("gmcHiddenNew"))
    end)
  end)

  describe("Geyser.MiniConsole geometry and visibility", function()
    local console

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcMove", x = 10, y = 20, width = 200, height = 100}))
    end)

    it("moves and resizes the widget", function()
      console:move(60, 70)
      console:resize(120, 60)
      assert.are.same({x = 60, y = 70, width = 120, height = 60}, geometry("gmcMove"))
    end)

    it("hides and shows the widget", function()
      console:hide()
      assert.is_false(windowVisible("gmcMove"))
      console:show()
      assert.is_true(windowVisible("gmcMove"))
    end)

    it("follows its container when the container moves", function()
      local container = track(Geyser.Container:new({name = "gmcDragBox", x = 0, y = 0, width = 200, height = 100}))
      track(Geyser.MiniConsole:new({name = "gmcDragged", x = 0, y = 0, width = "100%", height = "100%"}, container))
      container:move(150, 30)
      assert.are.same({x = 150, y = 30, width = 200, height = 100}, geometry("gmcDragged"))
    end)
  end)

  describe("Geyser.MiniConsole:setWrap/enableAutoWrap/disableAutoWrap/resetAutoWrap", function()
    it("sets the wrap column", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcWrap", x = 0, y = 0, width = 300, height = 100}))
      console:setWrap(42)
      assert.are.equal(42, console.wrapAt)
      assert.are.equal(42, getWindowWrap("gmcWrap"))
    end)

    it("refuses to set the wrap while auto wrap is on", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcWrapLocked", x = 0, y = 0, width = 300, height = 100}))
      console:enableAutoWrap()
      local derivedWrap = getWindowWrap("gmcWrapLocked")
      local result, message = console:setWrap(11)
      assert.is_nil(result)
      assert.is_truthy(message:find("autoWrap is enabled", 1, true))
      assert.are.equal(derivedWrap, getWindowWrap("gmcWrapLocked"))
    end)

    it("derives the wrap from the width when auto wrap is on", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcAutoWrap", x = 0, y = 0, width = 300, height = 100, wrapAt = "auto"}))
      local charWidth = calcFontSize("gmcAutoWrap")
      assert.is_true(console.autoWrap)
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcAutoWrap"))
    end)

    it("re-derives the wrap when the console is resized", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcRewrap", x = 0, y = 0, width = 300, height = 100, autoWrap = true}))
      local charWidth = calcFontSize("gmcRewrap")
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcRewrap"))
      console:resize(150, 100)
      assert.are.equal(math.floor(150 / charWidth), getWindowWrap("gmcRewrap"))
    end)

    it("stops re-deriving the wrap once auto wrap is disabled", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcNoAutoWrap", x = 0, y = 0, width = 300, height = 100, autoWrap = true}))
      console:disableAutoWrap()
      console:setWrap(17)
      console:resize(150, 100)
      assert.is_false(console.autoWrap)
      assert.are.equal(17, getWindowWrap("gmcNoAutoWrap"))
    end)

    -- a console too narrow for even one character works out as zero columns,
    -- which Mudlet refuses (and which used to hang it, issue #9622)
    it("never derives a wrap of less than one column", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcTinyAutoWrap", x = 0, y = 0, width = 4, height = 100, autoWrap = true}))
      assert.are.equal(1, console.wrapAt)
      assert.are.equal(1, getWindowWrap("gmcTinyAutoWrap"))
    end)

    it("passes on a refused wrap width instead of recording it", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcZeroWrap", x = 0, y = 0, width = 300, height = 100}))
      console:setWrap(30)
      local result, message = console:setWrap(0)
      assert.is_nil(result)
      assert.is_truthy(message:find("greater than zero", 1, true))
      -- the refused width must not be remembered, or every later setWrap()
      -- would re-send it and be refused as well
      assert.are.equal(30, console.wrapAt)
      assert.are.equal(30, getWindowWrap("gmcZeroWrap"))
    end)

    it("reports that resetAutoWrap has nothing to do when auto wrap is off", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcResetWrap", x = 0, y = 0, width = 300, height = 100}))
      local result, message = console:resetAutoWrap()
      assert.is_nil(result)
      assert.is_truthy(message:find("Autowrap is not enabled", 1, true))
    end)
  end)

  describe("Geyser.MiniConsole:setFontSize/getFont", function()
    it("changes the font size of the console", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFont", x = 0, y = 0, width = 300, height = 100, fontSize = 8}))
      assert.are.equal(8, getFontSize("gmcFont"))
      console:setFontSize(14)
      assert.are.equal(14, getFontSize("gmcFont"))
      assert.are.equal(14, console.fontSize)
    end)

    it("re-derives an auto wrap from the new font size", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFontWrap", x = 0, y = 0, width = 300, height = 100, fontSize = 8, autoWrap = true}))
      local smallWrap = getWindowWrap("gmcFontWrap")
      console:setFontSize(20)
      local bigWrap = getWindowWrap("gmcFontWrap")
      assert.are.equal(math.floor(300 / calcFontSize("gmcFontWrap")), bigWrap)
      assert.is_true(bigWrap < smallWrap)
    end)

    it("reads the font family back out of Mudlet", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFontFamily", x = 0, y = 0, width = 300, height = 100}))
      local family = getFont("gmcFontFamily")
      assert.are.equal("string", type(family))
      assert.is_true(#family > 0)
      -- getFont refreshes the cached family rather than reporting the cache
      console.font = "not the real font"
      assert.are.equal(family, console:getFont())
      assert.are.equal(family, console.font)
    end)
  end)

  describe("Geyser.MiniConsole:clear", function()
    it("empties the console", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcClear", x = 0, y = 0, width = 300, height = 100}))
      console:echo("one\ntwo\n")
      assert.is_true(getLineCount("gmcClear") > 1)
      console:clear()
      assert.are.equal(0, getLineCount("gmcClear"))
    end)
  end)

  describe("Geyser.MiniConsole:type_delete", function()
    it("deletes the widget with the object", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcDelete", x = 0, y = 0, width = 100, height = 50}))
      assert.is_not_nil(getWindowGeometry("gmcDelete"))
      console:delete()
      assert.is_nil(getWindowGeometry("gmcDelete"))
      assert.is_nil(Geyser.windowList.gmcDelete)
    end)
  end)

  describe("Geyser.MiniConsole command line", function()
    local console

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcCmd", x = 0, y = 0, width = 300, height = 100}))
      console:enableCommandLine()
    end)

    it("enableCommandLine creates a command line the console can read back", function()
      assert.are.equal("", console:getCmdLine())
    end)

    it("printCmd replaces the command line contents", function()
      console:printCmd("first")
      assert.are.equal("first", console:getCmdLine())
      console:printCmd("second")
      assert.are.equal("second", console:getCmdLine())
    end)

    it("appendCmd adds to what is already there", function()
      console:printCmd("hello")
      console:appendCmd(" world")
      assert.are.equal("hello world", console:getCmdLine())
    end)

    it("clearCmd empties the command line", function()
      console:printCmd("something")
      console:clearCmd()
      assert.are.equal("", console:getCmdLine())
    end)

    it("selectCmdLinetext reports success after selecting the typed text", function()
      console:printCmd("select me")
      assert.is_true(console:selectCmdLinetext())
    end)

    it("selectCmdLinetext accepts the console's own command line", function()
      console:printCmd("select me")
      assert.has_no.errors(function() console:selectCmdLinetext() end)
      -- selecting must not disturb what is typed
      assert.are.equal("select me", console:getCmdLine())
    end)

    it("setCmdLineStyleSheet applies the sheet and remembers it", function()
      -- the command line has no stylesheet getter, so spy on the global to
      -- see what actually reached it; spy.on keeps the real function
      local styleSheet = spy.on(_G, "setCmdLineStyleSheet")
      finally(function() styleSheet:revert() end)
      console:setCmdLineStyleSheet("color: red;")
      assert.spy(styleSheet).was.called_with("gmcCmd", "color: red;")
      assert.are.equal("color: red;", console.cmdLineStylesheet)
      -- called with no argument it re-applies the remembered sheet
      console:setCmdLineStyleSheet()
      assert.spy(styleSheet).was.called(2)
      assert.spy(styleSheet).was.called_with("gmcCmd", "color: red;")
    end)

    it("disableCommandLine hides the command line without discarding what is typed", function()
      local disable = spy.on(_G, "disableCommandLine")
      finally(function() disable:revert() end)
      console:printCmd("still here")
      console:disableCommandLine()
      assert.spy(disable).was.called_with("gmcCmd")
      -- disabling only hides the widget, so the text is still readable and
      -- comes back when the command line is enabled again
      assert.are.equal("still here", console:getCmdLine())
      console:enableCommandLine()
      assert.are.equal("still here", console:getCmdLine())
    end)

    -- An action only runs when the user presses return in the command line,
    -- which nothing in Lua can make happen, so what is pinned here is the
    -- registration: which function and arguments reached the widget, and what
    -- the console remembers about them
    pending("a command line action running on a real return keypress needs GUI automation")

    it("setCmdAction registers the function with the console's command line", function()
      local action = spy.on(_G, "setCmdLineAction")
      finally(function() action:revert() end)
      local handler = function() end

      console:setCmdAction(handler, "first", 2)

      assert.spy(action).was.called_with("gmcCmd", handler, "first", 2)
      assert.are.equal(handler, console.actionFunc)
      assert.are.same({"first", 2}, console.actionArgs)
    end)

    it("setCmdAction replaces the action rather than adding a second one", function()
      local action = spy.on(_G, "setCmdLineAction")
      finally(function() action:revert() end)
      local first = function() end
      local second = function() end

      console:setCmdAction(first, "one")
      console:setCmdAction(second)

      -- the widget holds one action, so the second registration has to reach it
      -- and the console has to forget the first one's arguments
      assert.spy(action).was.called(2)
      assert.spy(action).was.called_with("gmcCmd", second)
      assert.are.equal(second, console.actionFunc)
      assert.are.same({}, console.actionArgs)
    end)

    it("setCmdAction takes the name of a function as a string too", function()
      -- setCmdLineAction is wrapped in Lua, and that wrapper compiles a string
      -- into a call of the function it names
      assert.has_no.errors(function() console:setCmdAction("echo") end)
      assert.are.equal("echo", console.actionFunc)
    end)

    it("setCmdAction hard-errors on anything it cannot call", function()
      assert.has_error(function() console:setCmdAction({}) end)
      -- unlike the label callbacks, a command line action cannot be cleared by
      -- registering nil: resetCmdAction is the way to put it back
      assert.has_error(function() console:setCmdAction(nil) end)
    end)

    it("resetCmdAction puts the command line back to sending to the game", function()
      local reset = spy.on(_G, "resetCmdLineAction")
      finally(function() reset:revert() end)
      console:setCmdAction(function() end, "first")

      console:resetCmdAction()

      assert.spy(reset).was.called_with("gmcCmd")
      assert.is_nil(console.actionFunc)
      assert.is_nil(console.actionArgs)
    end)

    it("resetCmdAction is safe on a command line that never had an action", function()
      assert.has_no.errors(function() console:resetCmdAction() end)
      assert.is_nil(console.actionFunc)
    end)
  end)

  describe("Geyser.MiniConsole:setBufferSize", function()
    it("caps how many lines the console keeps", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcBuffer", x = 0, y = 0, width = 300, height = 100}))
      console:setBufferSize(100, 20)
      for i = 1, 600 do
        console:echo("buffered line " .. i .. "\n")
      end
      -- trimming happens in batches once the limit is passed, so the line
      -- count settles between the limit and limit + batch rather than at 600
      local kept = getLineCount("gmcBuffer")
      assert.is_true(kept < 600, "a capped console must not keep every line, kept " .. kept)
      assert.is_true(kept <= 121, "a capped console should settle near its limit, kept " .. kept)
    end)
  end)

  describe("Geyser.MiniConsole replace family", function()
    local console

    local function firstLine()
      console:moveCursor(0, 0)
      console:selectCurrentLine()
      return console:getCurrentLine()
    end

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcReplace", x = 0, y = 0, width = 400, height = 200}))
      console:setWrap(60)
      console:echo("hello world\n")
      console:moveCursor(0, 0)
    end)

    it("replace swaps the current selection", function()
      console:selectString("world", 1)
      console:replace("earth")
      assert.are.equal("hello earth", firstLine())
    end)

    it("replaceLine swaps the whole line", function()
      console:replaceLine("a new line")
      assert.are.equal("a new line", firstLine())
    end)

    it("dreplaceLine and hreplaceLine swap the line with colour", function()
      console:dreplaceLine("<0,255,0>green line")
      assert.are.equal("green line", firstLine())
      console:hreplaceLine("#0000ffblue line")
      assert.are.equal("blue line", firstLine())
    end)

    it("creplaceLine swaps the line with a named colour", function()
      console:creplaceLine("<red>named line")
      assert.are.equal("named line", firstLine())
      assert.are.same(color_table["red"], getTextFormat("gmcReplace").foreground)
    end)

    it("fg and bg colour what is echoed next", function()
      console:clear()
      console:fg("red")
      console:bg("blue")
      console:echo("coloured\n")
      console:selectString("coloured", 1)
      assert.are.same(color_table["red"], {getFgColor("gmcReplace")})
      assert.are.same(color_table["blue"], {getBgColor("gmcReplace")})
    end)

    it("display renders a table into the console", function()
      console:clear()
      console:display({alpha = 1})
      local text = table.concat(getLines("gmcReplace", 0, getLineCount("gmcReplace")), "\n")
      assert.is_truthy(text:find("alpha", 1, true))
    end)

    it("appendBuffer copies the main console selection in", function()
      clearWindow()
      echo("copy this line\n")
      moveCursorEnd()
      moveCursorUp()
      selectCurrentLine()
      copy()
      console:clear()
      console:appendBuffer()
      assert.is_truthy(table.concat(getLines("gmcReplace", 0, getLineCount("gmcReplace") + 1), "\n"):find("copy this line", 1, true))
    end)
  end)

  describe("Geyser.MiniConsole cursor movement", function()
    local console

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcCursor", x = 0, y = 0, width = 400, height = 200}))
      console:echo("one\ntwo\nthree\nfour\n")
      console:moveCursor(0, 0)
    end)

    it("moveCursorDown walks down the buffer and stops at the end", function()
      console:moveCursorDown(2)
      assert.are.equal(2, getLineNumber("gmcCursor"))
      console:moveCursorDown(500)
      assert.are.equal(getLastLineNumber("gmcCursor"), getLineNumber("gmcCursor"))
    end)

    it("moveCursorUp walks back up and stops at the top", function()
      console:moveCursorEnd()
      console:moveCursorUp(1)
      local afterOne = getLineNumber("gmcCursor")
      console:moveCursorUp(500)
      assert.are.equal(0, getLineNumber("gmcCursor"))
      assert.is_true(afterOne > 0)
    end)
  end)

  describe("Geyser.MiniConsole link and popup echoes", function()
    local console

    local function currentLine()
      console:selectCurrentLine()
      return console:getCurrentLine()
    end

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcLinks", x = 0, y = 0, width = 400, height = 200}))
      console:setWrap(60)
    end)

    -- there is no getter for a link's command or hint, so the text each
    -- variant lays down is the observable part
    it("echoes plain, colour, decimal and hex links", function()
      console:echoLink("plain link", "send('x')", "hint", true)
      assert.are.equal("plain link", currentLine())

      console:clear()
      console:cechoLink("<red>colour link", "send('x')", "hint", true)
      assert.are.equal("colour link", currentLine())

      console:clear()
      console:dechoLink("<0,255,0>decimal link", "send('x')", "hint", true)
      assert.are.equal("decimal link", currentLine())

      console:clear()
      console:hechoLink("#0000ffhex link", "send('x')", "hint", true)
      assert.are.equal("hex link", currentLine())
    end)

    it("inserts a plain link and a plain popup at the cursor", function()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:insertLink("C", "send('x')", "hint", true)
      console:moveCursor(0, 0)
      assert.are.equal("ACB", currentLine())

      console:clear()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:insertPopup("D", {"send('one')"}, {"first"}, true)
      console:moveCursor(0, 0)
      assert.are.equal("ADB", currentLine())
    end)

    it("inserts decimal and hex popups at the cursor", function()
      local commands = {"send('one')", "send('two')"}
      local hints = {"first", "second"}

      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:dinsertPopup("<0,255,0>D", commands, hints, true)
      console:moveCursor(0, 0)
      assert.are.equal("ADB", currentLine())

      console:clear()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:hinsertPopup("#0000ffE", commands, hints, true)
      console:moveCursor(0, 0)
      assert.are.equal("AEB", currentLine())
    end)

    it("inserts colour, decimal and hex links at the cursor", function()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:cinsertLink("<red>C", "send('x')", "hint", true)
      console:moveCursor(0, 0)
      assert.are.equal("ACB", currentLine())

      console:clear()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:dinsertLink("<0,255,0>D", "send('x')", "hint", true)
      console:moveCursor(0, 0)
      assert.are.equal("ADB", currentLine())

      console:clear()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:hinsertLink("#0000ffE", "send('x')", "hint", true)
      console:moveCursor(0, 0)
      assert.are.equal("AEB", currentLine())
    end)

    it("echoes and inserts popups in every colour syntax", function()
      local commands = {"send('one')", "send('two')"}
      local hints = {"first", "second"}

      console:echoPopup("plain popup", commands, hints, true)
      assert.are.equal("plain popup", currentLine())

      console:clear()
      console:cechoPopup("<red>colour popup", commands, hints, true)
      assert.are.equal("colour popup", currentLine())

      console:clear()
      console:dechoPopup("<0,255,0>decimal popup", commands, hints, true)
      assert.are.equal("decimal popup", currentLine())

      console:clear()
      console:hechoPopup("#0000ffhex popup", commands, hints, true)
      assert.are.equal("hex popup", currentLine())

      console:clear()
      console:echo("AB\n")
      console:moveCursor(1, 0)
      console:cinsertPopup("<red>C", commands, hints, true)
      console:moveCursor(0, 0)
      assert.are.equal("ACB", currentLine())
    end)

    it("setLink turns the current selection into a link", function()
      -- a link's command and hint have no getter, so the observable part is
      -- that the call is routed at this console and leaves the text alone
      local setLinkSpy = spy.on(_G, "setLink")
      finally(function() setLinkSpy:revert() end)
      console:echo("clickable\n")
      console:moveCursor(0, 0)
      console:selectString("clickable", 1)
      console:setLink("send('x')", "hint")
      assert.spy(setLinkSpy).was.called_with("gmcLinks", "send('x')", "hint")
      console:moveCursor(0, 0)
      assert.are.equal("clickable", currentLine())
    end)
  end)

  -- Every one of these is a one line wrapper around the Mudlet global of the
  -- same name, so what each spec pins is that the wrapper reaches this console
  -- rather than the main window: a wrapper that forgot self.name would answer
  -- from the main console and look plausible.
  describe("Geyser.MiniConsole buffer getters", function()
    local console

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcGetters", x = 0, y = 0, width = 400, height = 200}))
      console:setWrap(60)
      console:echo("alpha\nbeta\ngamma\n")
    end)

    it("counts the lines it holds and names the last one", function()
      -- getLineCount answers the index of the console's last line, and the
      -- trailing newline leaves an empty line after "gamma"
      assert.are.equal(3, console:getLineCount())
      assert.are.equal(3, console:getLastLineNumber())
      assert.are.equal(getLineCount("gmcGetters"), console:getLineCount())
    end)

    it("reports where the cursor is", function()
      console:moveCursor(0, 1)
      assert.are.equal(1, console:getLineNumber())
      assert.are.equal(0, console:getColumnNumber())
      console:moveCursor(2, 1)
      assert.are.equal(2, console:getColumnNumber())
    end)

    it("hands back a range of lines and the one under the cursor", function()
      -- the range is walked forwards from the first line for as many lines as
      -- the two are apart, so the last line asked for is not included
      assert.are.same({"alpha", "beta"}, console:getLines(0, 2))
      assert.are.same({"alpha", "beta", "gamma"}, console:getLines(0, 3))
      console:moveCursor(0, 1)
      console:selectCurrentLine()
      assert.are.equal("beta", console:getCurrentLine())
    end)

    it("selectSection selects part of a line and getSelection reads it back", function()
      console:moveCursor(0, 0)
      assert.is_true(console:selectSection(1, 3))
      assert.are.equal("lph", console:getSelection())
    end)

    it("getTextFormat describes the selected text", function()
      console:clear()
      console:setFgColor(255, 0, 0)
      console:setBold(true)
      console:echo("formatted\n")
      console:moveCursor(0, 0)
      console:selectCurrentLine()
      local format = console:getTextFormat()
      assert.are.same({255, 0, 0}, format.foreground)
      assert.is_true(format.bold)
    end)

    it("reports the wrap it was set to and the font size it is drawn at", function()
      assert.are.equal(60, console:getWindowWrap())
      console:setFontSize(14)
      assert.are.equal(14, console:getFontSize())
    end)

    it("reports how many characters fit across it and how many rows down", function()
      local charWidth, charHeight = console:calcFontSize()
      assert.is_true(charWidth > 0 and charHeight > 0)
      assert.are.same({calcFontSize("gmcGetters")}, {console:calcFontSize()})
      -- the counts follow the console's own size, so a narrower console has to
      -- report fewer columns rather than the main window's
      local columns, rows = console:getColumnCount(), console:getRowCount()
      assert.is_true(columns > 0 and rows > 0)
      console:resize(100, 200)
      assert.is_true(console:getColumnCount() < columns)
    end)
  end)

  -- The first scroll of a console opens its split view, and that one is
  -- applied on the next turn of the event loop rather than on the call, so
  -- every reading here is taken after pumping. Opening the split also shortens
  -- the upper pane, and that first scroll compensates for it by the height of
  -- the pane that appeared, so only the first scroll of a console lands above
  -- the line it asked for - hence openSplit() below. Later scrolls go through
  -- the pane that is already there, synchronously and to the exact line.
  describe("Geyser.MiniConsole scrolling", function()
    local console

    local function openSplit()
      console:scrollTo(50)
      pumpEvents(50)
    end

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcScroll", x = 0, y = 0, width = 400, height = 200}))
      for index = 1, 60 do
        console:echo("line " .. index .. "\n")
      end
    end)

    it("scrollingActive follows enableScrolling and disableScrolling", function()
      assert.is_true(console:scrollingActive())
      console:disableScrolling()
      assert.is_false(console.scrolling)
      assert.is_false(console:scrollingActive())
      console:enableScrolling()
      assert.is_true(console.scrolling)
      assert.is_true(console:scrollingActive())
    end)

    it("starts at the end of its buffer", function()
      assert.are.equal(console:getLastLineNumber(), console:getScroll())
    end)

    -- how far above the asked for line the first scroll lands is the height of
    -- the split pane that just appeared, which no Lua getter reports, so what
    -- is pinned is that the console scrolled back and did not overshoot past
    -- the line it was given
    it("scrolls back on the first scroll, which is what opens the split", function()
      local atTheEnd = console:getScroll()
      console:scrollTo(50)
      pumpEvents(50)
      local scrolled = console:getScroll()
      assert.is_true(scrolled < atTheEnd, "the console did not scroll back at all")
      assert.is_true(scrolled > 0 and scrolled <= 50,
                     "the first scroll landed at " .. scrolled .. ", which is not at or above line 50")
    end)

    it("scrolls to the line it is given once the split is open", function()
      openSplit()
      console:scrollTo(20)
      pumpEvents(50)
      assert.are.equal(20, console:getScroll())
      console:scrollTo(35)
      pumpEvents(50)
      assert.are.equal(35, console:getScroll())
    end)

    it("scrollUp and scrollDown walk from where it already is", function()
      openSplit()
      console:scrollTo(30)
      pumpEvents(50)
      console:scrollUp(5)
      pumpEvents(50)
      assert.are.equal(25, console:getScroll())
      console:scrollDown(10)
      pumpEvents(50)
      assert.are.equal(35, console:getScroll())
    end)

    it("scrollUp and scrollDown move one line when given no count", function()
      openSplit()
      console:scrollTo(30)
      pumpEvents(50)
      console:scrollUp()
      pumpEvents(50)
      assert.are.equal(29, console:getScroll())
      console:scrollDown()
      pumpEvents(50)
      assert.are.equal(30, console:getScroll())
    end)

    it("scrollUp will not walk back past the top of the buffer", function()
      openSplit()
      console:scrollTo(3)
      pumpEvents(50)
      console:scrollUp(500)
      pumpEvents(50)
      assert.are.equal(0, console:getScroll())
    end)

    it("scrollTo with no line goes back to the end", function()
      openSplit()
      console:scrollTo(20)
      pumpEvents(50)
      assert.are.equal(20, console:getScroll())
      console:scrollTo()
      pumpEvents(50)
      assert.are.equal(console:getLastLineNumber(), console:getScroll())
    end)

    it("will not scroll a console that has scrolling turned off", function()
      console:disableScrolling()
      local atTheEnd = console:getScroll()
      console:scrollTo(10)
      pumpEvents(50)
      assert.are.equal(atTheEnd, console:getScroll())
    end)
  end)

  describe("Geyser.MiniConsole scroll bars", function()
    it("enableScrollBar and disableScrollBar put the bar on the widget", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcScrollBar", x = 0, y = 0, width = 300, height = 100}))
      -- a console the constructor was given no scrollBar constraint has one
      -- turned off, which is what makes the enable below observable
      assert.is_false(getScrollBarVisible("gmcScrollBar"))
      console:enableScrollBar()
      assert.is_true(console.scrollBar)
      assert.is_true(getScrollBarVisible("gmcScrollBar"))
      console:disableScrollBar()
      assert.is_false(console.scrollBar)
      assert.is_false(getScrollBarVisible("gmcScrollBar"))
    end)

    -- turning the bar on also has to re-derive an auto wrap, because
    -- resetAutoWrap keeps 15 pixels clear for it and the text would otherwise
    -- run under it
    it("enableScrollBar takes room off the wrap and disableScrollBar gives it back", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcScrollBarWrap", x = 0, y = 0, width = 300, height = 100, autoWrap = true}))
      local charWidth = calcFontSize("gmcScrollBarWrap")

      console:enableScrollBar()
      assert.are.equal(math.floor((300 - 15) / charWidth), getWindowWrap("gmcScrollBarWrap"))

      console:disableScrollBar()
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcScrollBarWrap"))
    end)

    it("the horizontal scroll bar is remembered on the object and reaches Mudlet", function()
      -- getScrollBarVisible reports the vertical bar only, and a horizontal one
      -- costs no width either, so it leaves nothing readable behind: the
      -- delegation is what can be checked
      local enable = spy.on(_G, "enableHorizontalScrollBar")
      local disable = spy.on(_G, "disableHorizontalScrollBar")
      finally(function() enable:revert() disable:revert() end)
      local console = track(Geyser.MiniConsole:new({name = "gmcHScrollBar", x = 0, y = 0, width = 300, height = 100}))

      console:enableHorizontalScrollBar()
      assert.is_true(console.horizontalScrollBar)
      assert.spy(enable).was.called_with("gmcHScrollBar")

      console:disableHorizontalScrollBar()
      assert.is_false(console.horizontalScrollBar)
      assert.spy(disable).was.called_with("gmcHScrollBar")
    end)

    it("takes the scroll bar constraint it was built with", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcScrollBarCons", x = 0, y = 0, width = 300, height = 100, scrollBar = true, horizontalScrollBar = true}))
      assert.is_true(console.scrollBar)
      assert.is_true(console.horizontalScrollBar)
      assert.is_true(getScrollBarVisible("gmcScrollBarCons"))
    end)
  end)

  describe("Geyser.MiniConsole:reposition", function()
    it("re-derives the auto wrap after the console has been placed again", function()
      local container = track(Geyser.Container:new({name = "gmcRepositionBox", x = 0, y = 0, width = 300, height = 100}))
      local console = track(Geyser.MiniConsole:new({name = "gmcReposition", x = 0, y = 0, width = "100%", height = "100%", autoWrap = true}, container))
      local charWidth = calcFontSize("gmcReposition")
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcReposition"))

      container:resize(150, 100)
      container:reposition()

      assert.are.equal(math.floor(150 / charWidth), getWindowWrap("gmcReposition"))
      assert.are.equal(math.floor(150 / charWidth), console.wrapAt)
    end)

    it("leaves a manually wrapped console's wrap alone", function()
      local container = track(Geyser.Container:new({name = "gmcNoRewrapBox", x = 0, y = 0, width = 300, height = 100}))
      local console = track(Geyser.MiniConsole:new({name = "gmcNoRewrap", x = 0, y = 0, width = "100%", height = "100%"}, container))
      console:setWrap(23)
      container:resize(150, 100)
      container:reposition()
      assert.are.equal(23, getWindowWrap("gmcNoRewrap"))
    end)
  end)

  describe("Geyser.MiniConsole:setFont/resetFormat", function()
    it("setFont changes the family and remembers it", function()
      -- Ubuntu Mono ships with Mudlet, so it is there on every platform, and it
      -- is not the console default, so a family that never reached the widget
      -- still fails this
      local console = track(Geyser.MiniConsole:new({name = "gmcSetFont", x = 0, y = 0, width = 300, height = 100}))
      assert.are_not.equal("Ubuntu Mono", getFont("gmcSetFont"))
      console:setFont("Ubuntu Mono")
      assert.are.equal("Ubuntu Mono", getFont("gmcSetFont"))
      assert.are.equal("Ubuntu Mono", console.font)
    end)

    it("resetFormat puts the colours and attributes back to the defaults", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcResetFormat", x = 0, y = 0, width = 300, height = 100}))
      console:setTextFormat(10, 20, 30, 200, 100, 50, true, true, true)
      console:echo("styled\n")

      console:resetFormat()
      console:echo("plain\n")

      console:moveCursor(0, 1)
      console:selectCurrentLine()
      local format = console:getTextFormat()
      assert.is_false(format.bold)
      assert.is_false(format.underline)
      assert.is_false(format.italic)
      -- and the styled line above is untouched, so this really was a reset of
      -- what comes next rather than a rewrite of the buffer
      console:moveCursor(0, 0)
      console:selectCurrentLine()
      assert.is_true(console:getTextFormat().bold)
    end)
  end)

  describe("Geyser.MiniConsole background image", function()
    -- a Qt resource that ships with every Mudlet, so no fixture file is needed
    local imagePath = ":/icons/mudlet.png"

    it("remembers the image it was given and forgets it on reset", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcBackground", x = 0, y = 0, width = 200, height = 100}))
      assert.is_true(console:setBackgroundImage(imagePath, 2))
      assert.are.equal(imagePath, console.imgPath)
      assert.is_true(console:resetBackgroundImage())
      assert.is_nil(console.imgPath)
    end)
  end)
end)
