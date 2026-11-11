-- Geyser.TextEdit wraps Mudlet's text edit primitive. The primitive's own Lua
-- functions, and their error paths, are covered in TextEdit_spec.lua; what is
-- specced here is the Geyser layer on top of them - the constraints, the
-- container cascade, and which widget each wrapper reaches. Mudlet reports
-- nothing about a text edit beyond its text and its window geometry, so the
-- property setters are read back through the global they call.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.TextEdit", function()
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

  describe("Geyser.TextEdit:new/new2", function()
    it("creates a text edit widget at the constrained geometry", function()
      local editor = track(Geyser.TextEdit:new({name = "gteNew", x = 30, y = 40, width = 200, height = 100}))
      -- Geyser's own type string is camel cased, Mudlet's windowType is not
      assert.are.equal("textEdit", editor.type)
      assert.are.equal("textedit", windowType("gteNew"))
      assert.are.same({x = 30, y = 40, width = 200, height = 100}, geometry("gteNew"))
      assert.is_true(windowVisible("gteNew"))
      assert.are.equal(editor, Geyser.windowList.gteNew)
      assert.are.equal("main", editor.windowname)
    end)

    it("uses Geyser's defaults when no constraints are given", function()
      track(Geyser.TextEdit:new({name = "gteDefaults"}))
      assert.are.same({x = 10, y = 10, width = 300, height = 200}, geometry("gteDefaults"))
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gteBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.TextEdit:new({name = "gteInBox", x = "25%", y = "50%", width = "50%", height = "25%"}, container))
      assert.are.same({x = 200, y = 150, width = 200, height = 50}, geometry("gteInBox"))
    end)

    it("new2 marks the text edit as using add2", function()
      local editor = track(Geyser.TextEdit:new2({name = "gteNew2", x = 0, y = 0, width = 100, height = 50}))
      assert.is_true(editor.useAdd2)
      assert.are.equal("textedit", windowType("gteNew2"))
    end)

    it("starts out hidden when the constraints ask for it", function()
      local editor = track(Geyser.TextEdit:new({name = "gteHiddenNew", x = 0, y = 0, width = 100, height = 50, hidden = true}))
      assert.is_true(editor.hidden)
      assert.is_false(windowVisible("gteHiddenNew"))
      editor:show()
      assert.is_false(editor.hidden)
      assert.is_true(windowVisible("gteHiddenNew"))
    end)

    it("keeps a new text edit of a hidden add2 container hidden", function()
      local container = track(Geyser.Container:new2({name = "gteHiddenBox", x = 0, y = 0, width = 200, height = 100}))
      container:hide()
      local editor = track(Geyser.TextEdit:new2({name = "gteHiddenChild", x = 0, y = 0, width = 50, height = 20}, container))
      assert.is_true(editor.auto_hidden)
      assert.is_false(windowVisible("gteHiddenChild"))
      container:show()
      assert.is_true(windowVisible("gteHiddenChild"))
    end)

    it("applies a stylesheet given as a constraint", function()
      local styleSheet = spy.on(_G, "setTextEditStyleSheet")
      finally(function() styleSheet:revert() end)
      local editor = track(Geyser.TextEdit:new({
        name = "gteCssCons", x = 0, y = 0, width = 100, height = 50,
        stylesheet = "QPlainTextEdit { background: #222; }",
      }))
      assert.spy(styleSheet).was.called_with("gteCssCons", "QPlainTextEdit { background: #222; }")
      assert.are.equal("QPlainTextEdit { background: #222; }", editor.stylesheet)
    end)
  end)

  describe("Geyser.TextEdit:setText/getText/clear", function()
    local editor

    before_each(function()
      editor = track(Geyser.TextEdit:new({name = "gteText", x = 0, y = 0, width = 200, height = 100}))
    end)

    it("round-trips the text through the widget", function()
      editor:setText("Geyser text")
      assert.are.equal("Geyser text", editor:getText())
      assert.are.equal("Geyser text", getTextEditText("gteText"))
    end)

    it("replaces what was there on the next setText", function()
      editor:setText("first")
      editor:setText("second")
      assert.are.equal("second", editor:getText())
    end)

    it("keeps the line breaks in multi-line text", function()
      editor:setText("one\ntwo\nthree")
      assert.are.equal("one\ntwo\nthree", editor:getText())
    end)

    it("clears the text edit", function()
      editor:setText("something")
      editor:clear()
      assert.are.equal("", editor:getText())
      assert.are.equal("", getTextEditText("gteText"))
    end)

    it("reads and writes its own widget rather than another one", function()
      local other = track(Geyser.TextEdit:new({name = "gteOther", x = 0, y = 200, width = 200, height = 100}))
      editor:setText("mine")
      other:setText("theirs")
      assert.are.equal("mine", editor:getText())
      assert.are.equal("theirs", other:getText())
      editor:clear()
      assert.are.equal("theirs", other:getText())
    end)
  end)

  describe("Geyser.TextEdit property setters", function()
    local editor

    before_each(function()
      editor = track(Geyser.TextEdit:new({name = "gteProps", x = 0, y = 0, width = 200, height = 100}))
    end)

    -- spy.on leaves the real call in place, so this still exercises Mudlet
    it("passes read-only, placeholder, font, font size and tab focus straight through", function()
      local calls = {
        {method = "setReadOnly", global = "setTextEditReadOnly", value = true},
        {method = "setPlaceholder", global = "setTextEditPlaceholder", value = "Type here..."},
        {method = "setFont", global = "setTextEditFont", value = "Ubuntu Mono"},
        {method = "setFontSize", global = "setTextEditFontSize", value = 14},
        {method = "setTabMovesFocus", global = "setTextEditTabMovesFocus", value = false},
      }
      -- reverting inside the loop would be skipped by a failing assertion, and
      -- a spy left on a Mudlet global is picked up as the "real" function by
      -- the next spy.on in any later spec file
      local spied = {}
      finally(function()
        for _, global in ipairs(spied) do
          _G[global]:revert()
        end
      end)

      for _, call in ipairs(calls) do
        local registration = spy.on(_G, call.global)
        spied[#spied + 1] = call.global
        editor[call.method](editor, call.value)
        assert.spy(registration).was.called_with("gteProps", call.value)
      end
    end)

    it("setStyleSheet remembers the sheet and reuses it when called with none", function()
      local styleSheet = spy.on(_G, "setTextEditStyleSheet")
      finally(function() styleSheet:revert() end)

      editor:setStyleSheet("QPlainTextEdit { color: #eee; }")
      assert.are.equal("QPlainTextEdit { color: #eee; }", editor.stylesheet)
      assert.spy(styleSheet).was.called_with("gteProps", "QPlainTextEdit { color: #eee; }")

      -- called_with is satisfied by any recorded call, so the second one is
      -- read off the spy directly: the remembered sheet has to be what it sent
      editor:setStyleSheet()
      assert.spy(styleSheet).was.called(2)
      local second = styleSheet.calls[2].vals
      assert.are.equal("gteProps", second[1])
      assert.are.equal("QPlainTextEdit { color: #eee; }", second[2])
    end)

    it("none of the setters disturbs the text", function()
      editor:setText("kept")
      editor:setReadOnly(true)
      editor:setPlaceholder("hint")
      editor:setFontSize(16)
      editor:setStyleSheet("QPlainTextEdit { color: #eee; }")
      assert.are.equal("kept", editor:getText())
    end)
  end)

  pending("Geyser.TextEdit read-only, placeholder, font and tab focus taking effect on the widget - Mudlet reports none of them back, so this needs text edit getters")

  describe("Geyser.TextEdit geometry and visibility", function()
    local editor

    before_each(function()
      editor = track(Geyser.TextEdit:new({name = "gteMove", x = 10, y = 20, width = 200, height = 100}))
    end)

    it("moves and resizes the widget", function()
      editor:move(60, 70)
      editor:resize(120, 60)
      assert.are.same({x = 60, y = 70, width = 120, height = 60}, geometry("gteMove"))
    end)

    it("hides and shows the widget", function()
      editor:hide()
      assert.is_true(editor.hidden)
      assert.is_false(windowVisible("gteMove"))
      editor:show()
      assert.is_false(editor.hidden)
      assert.is_true(windowVisible("gteMove"))
    end)

    it("follows its container when the container moves and resizes", function()
      local container = track(Geyser.Container:new({name = "gteDragBox", x = 0, y = 0, width = 200, height = 100}))
      track(Geyser.TextEdit:new({name = "gteDragged", x = 0, y = 0, width = "100%", height = "100%"}, container))
      container:move(150, 30)
      assert.are.same({x = 150, y = 30, width = 200, height = 100}, geometry("gteDragged"))
      container:resize(100, 50)
      assert.are.same({x = 150, y = 30, width = 100, height = 50}, geometry("gteDragged"))
    end)

    it("stacks in a box like any other Geyser widget", function()
      local box = track(Geyser.VBox:new({name = "gteBoxStack", x = 0, y = 0, width = 200, height = 200}))
      track(Geyser.TextEdit:new({name = "gteStackA"}, box))
      track(Geyser.TextEdit:new({name = "gteStackB"}, box))
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("gteStackA"))
      assert.are.same({x = 0, y = 100, width = 200, height = 100}, geometry("gteStackB"))
    end)
  end)

  describe("Geyser.TextEdit:type_delete", function()
    it("deletes the widget with the object", function()
      local editor = track(Geyser.TextEdit:new({name = "gteDelete", x = 0, y = 0, width = 100, height = 50}))
      assert.are.equal("textedit", windowType("gteDelete"))
      editor:delete()
      assert.is_nil(windowType("gteDelete"))
      assert.is_nil(getWindowGeometry("gteDelete"))
      assert.is_nil(Geyser.windowList.gteDelete)
    end)

    it("goes away with the container it was put in", function()
      local container = track(Geyser.Container:new({name = "gteOuter", x = 0, y = 0, width = 300, height = 200}))
      track(Geyser.TextEdit:new({name = "gteNested", x = 0, y = 0, width = "100%", height = "100%"}, container))
      container:delete()
      assert.is_nil(windowType("gteNested"))
    end)
  end)
end)
