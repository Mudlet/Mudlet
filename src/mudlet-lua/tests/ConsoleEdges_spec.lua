-- Edge cases of the console and window APIs that other specs do not reach:
-- edits made from inside a trigger, and the refusals for bad or clashing names.
describe("Console edges", function()
  local function name(base)
    return "ce" .. base .. tostring(math.random(100000, 999999))
  end

  describe("editing the line a trigger matched keeps later capture groups aligned", function()
    -- Group 1 is the whole match, so "beta" is group 3. Each edit shifts where
    -- "beta" sits in the line; unless the capture positions shift with it,
    -- selectCaptureGroup(3) selects the wrong characters afterwards.
    local function inTrigger(edit)
      local result = {}
      local id = tempRegexTrigger("^ceTrig (\\w+) (\\w+)$", function()
        result.before = getCurrentLine()
        edit(result)
        selectCaptureGroup(3)
        result.group3 = getSelection()
        result.after = getCurrentLine()
        deselect()
      end, 1)
      finally(function() killTrigger(id) end)
      feedTriggers("\nceTrig alpha beta\n")
      return result
    end

    it("after replace() shortens an earlier group", function()
      local result = inTrigger(function()
        selectCaptureGroup(2)
        replace("x")
      end)
      assert.are.equal("ceTrig alpha beta", result.before)
      assert.are.equal("ceTrig x beta", result.after)
      -- where "beta" was before the edit no longer holds it
      assert.are_not.equal("beta", result.after:sub(14, 17))
      assert.are.equal("beta", result.group3)
    end)

    it("after replace() lengthens an earlier group", function()
      local result = inTrigger(function()
        selectCaptureGroup(2)
        replace("alphabetical")
      end)
      assert.are.equal("ceTrig alphabetical beta", result.after)
      assert.are_not.equal("beta", result.after:sub(14, 17))
      assert.are.equal("beta", result.group3)
    end)

    it("after insertLink() puts a link in front of it", function()
      local result = inTrigger(function()
        moveCursor(0, getLineNumber())
        insertLink("[L]", "", "a hint")
      end)
      assert.are.equal("[L]ceTrig alpha beta", result.after)
      assert.are_not.equal("beta", result.after:sub(14, 17))
      assert.are.equal("beta", result.group3)
    end)
  end)

  describe("insertLink() inside a trigger", function()
    local function linkColour(useCurrentFormat)
      local fg
      local id = tempRegexTrigger("^ceLinkFormat$", function()
        setFgColor(10, 20, 30)
        moveCursor(0, getLineNumber())
        if useCurrentFormat then
          insertLink("LINK", "", "a hint", true)
        else
          insertLink("LINK", "", "a hint")
        end
        selectSection(0, 4)
        fg = {getFgColor()}
        deselect()
        resetFormat()
      end, 1)
      finally(function() killTrigger(id) end)
      feedTriggers("\nceLinkFormat\n")
      return fg
    end

    it("takes the current format only when asked to", function()
      assert.are.same({10, 20, 30}, linkColour(true))
      assert.are_not.same({10, 20, 30}, linkColour(false))
    end)
  end)

  describe("setTextFormat() on the main window", function()
    it("sets the format the main console echoes with", function()
      finally(function()
        resetFormat()
        moveCursorEnd()
      end)
      local function echoedColours()
        local marker = name("Format")
        echo("\n" .. marker .. "\n")
        moveCursor("main", 0, getLastLineNumber("main") - 1)
        assert.are.equal(marker, getCurrentLine())
        selectString(marker, 1)
        local fg, bg = {getFgColor()}, {getBgColor()}
        deselect()
        return fg, bg
      end

      local fgBefore = echoedColours()
      assert.are_not.same({40, 50, 60}, fgBefore)

      assert.is_true(setTextFormat("main", 1, 2, 3, 40, 50, 60, false, false, false))
      local fg, bg = echoedColours()
      assert.are.same({40, 50, 60}, fg)
      assert.are.same({1, 2, 3}, bg)
    end)
  end)

  describe("setLabelCustomCursor()", function()
    local label = name("CursorLabel")

    setup(function()
      createLabel(label, 0, 0, 10, 10, 1)
    end)

    teardown(function()
      deleteLabel(label)
    end)

    it("reports an empty label name", function()
      local ok, err = setLabelCustomCursor("", "/some/cursor.png")
      assert.is_nil(ok)
      assert.are.equal("a label cannot have an empty string as its name", err)
    end)

    it("reports an empty cursor location", function()
      local ok, err = setLabelCustomCursor(label, "")
      assert.is_nil(ok)
      assert.are.equal("custom cursor location cannot be an empty string", err)
    end)

    it("reports a cursor image it cannot load", function()
      local ok, err = setLabelCustomCursor(label, "/no/such/cursor.png")
      assert.is_nil(ok)
      assert.are.equal('couldn\'t find custom cursor, is the location "/no/such/cursor.png" correct?', err)
    end)

    it("reports a label it cannot find", function()
      local unknown = name("NoSuchLabel")
      local ok, err = setLabelCustomCursor(unknown, "/no/such/cursor.png")
      assert.is_nil(ok)
      assert.are.equal(("label name '%s' not found"):format(unknown), err)
    end)
  end)

  describe("setUserWindowTitle()", function()
    it("refuses an empty window name", function()
      local ok, err = setUserWindowTitle("", "a title")
      assert.is_nil(ok)
      assert.are.equal("a user window cannot have an empty string as its name", err)
    end)

    it("refuses a miniconsole, which has no title bar", function()
      local mini = name("TitleMini")
      createMiniConsole(mini, 0, 0, 100, 100)
      finally(function() deleteMiniConsole(mini) end)
      local ok, err = setUserWindowTitle(mini, "a title")
      assert.is_nil(ok)
      assert.are.equal(('"%s" is not a user window'):format(mini), err)
    end)
  end)

  describe("elements other than labels", function()
    local scrollBox = name("ScrollBox")
    local target = name("TargetBox")
    local cmdLine = name("CmdLine")
    local textEdit = name("TextEdit")
    local mini = name("Mini")

    setup(function()
      createScrollBox(scrollBox, 0, 0, 100, 100)
      createScrollBox(target, 0, 0, 200, 200)
      createCommandLine(cmdLine, 0, 0, 100, 20)
      createTextEdit(textEdit, 0, 30, 100, 20)
      createMiniConsole(mini, 0, 60, 100, 100)
    end)

    teardown(function()
      deleteCommandLine(cmdLine)
      deleteTextEdit(textEdit)
      deleteMiniConsole(mini)
      deleteScrollBox(scrollBox)
      deleteScrollBox(target)
    end)

    it("can be raised and lowered", function()
      assert.is_false(raiseWindow(name("NoSuchElement")))
      for _, element in ipairs({scrollBox, cmdLine, textEdit, mini}) do
        assert.is_true(raiseWindow(element), element)
        assert.is_true(lowerWindow(element), element)
      end
    end)

    it("can be moved into a scroll box with setWindow()", function()
      local unknown = name("NoSuchElement")
      local ok, err = setWindow(target, unknown, 5, 5, true)
      assert.is_nil(ok)
      assert.are.equal(("element '%s' not found"):format(unknown), err)
      for _, element in ipairs({scrollBox, cmdLine, textEdit}) do
        assert.is_true(setWindow(target, element, 5, 5, true), element)
      end
    end)
  end)

  describe("refuses names it cannot use", function()
    it("an empty name for a command line or a text edit", function()
      local ok, err = createCommandLine("", 0, 0, 100, 20)
      assert.is_nil(ok)
      assert.are.equal("a commandLine cannot have an empty string as its name", err)

      ok, err = createTextEdit("", 0, 0, 100, 20)
      assert.is_nil(ok)
      assert.are.equal("a text edit cannot have an empty string as its name", err)
    end)

    it("a command line or a text edit that already exists", function()
      local cmdLine = name("DupCmdLine")
      local textEdit = name("DupTextEdit")
      assert.is_true(createCommandLine(cmdLine, 0, 0, 100, 20))
      assert.is_true(createTextEdit(textEdit, 0, 30, 100, 20))
      finally(function()
        deleteCommandLine(cmdLine)
        deleteTextEdit(textEdit)
      end)

      local ok, err = createCommandLine(cmdLine, 0, 0, 100, 20)
      assert.is_nil(ok)
      assert.are.equal("couldn't create commandLine", err)

      ok, err = createTextEdit(textEdit, 0, 0, 100, 20)
      assert.is_nil(ok)
      assert.are.equal("couldn't create text edit", err)
    end)

    it("an empty name to delete a miniconsole or a label", function()
      local ok, err = deleteMiniConsole("")
      assert.is_false(ok)
      assert.are.equal("a miniconsole cannot have an empty string as its name", err)

      ok, err = deleteLabel("")
      assert.is_false(ok)
      assert.are.equal("a label cannot have an empty string as its name", err)
    end)
  end)
end)
