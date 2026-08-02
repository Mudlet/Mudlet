describe("Tests functionality of Geyser.Label", function()
  describe('Tests that decho/hecho/cecho preserve font size', function()
    local label
    local globalEchoSpy

    before_each(function()
      -- Spy on the global echo function to inspect what HTML is generated
      globalEchoSpy = spy.on(_G, "echo")

      -- Create a label with a specific font size
      label = Geyser.Label:new({
        name = "testLabel",
        x = 0, y = 0,
        width = 100, height = 100,
      })

      -- Set font size to 50
      label:setFontSize(50)
    end)

    after_each(function()
      _G.echo:revert()
      if label then
        label:hide()
      end
    end)

    it('preserves font size when using echo()', function()
      label:echo("test message")

      -- Verify echo was called
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2] -- second argument is the message

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"))
    end)

    it('preserves font size when using decho()', function()
      label:decho("<255,0,0>red text")

      -- Verify echo was called (decho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "decho did not preserve font size")
    end)

    it('preserves font size when using hecho()', function()
      label:hecho("|cff0000red text")

      -- Verify echo was called (hecho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "hecho did not preserve font size")
    end)

    it('preserves font size when using cecho()', function()
      label:cecho("<red>red text")

      -- Verify echo was called (cecho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "cecho did not preserve font size")
    end)

    it('preserves bold formatting when using decho()', function()
      label:setBold(true)
      label:decho("<255,0,0>bold red text")

      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains both font-size and bold tags
      assert.is_truthy(html:find("font%-size: 50pt"))
      assert.is_truthy(html:find("<b>"))
    end)

    it('preserves alignment when using hecho()', function()
      label:setAlignment("center")
      label:hecho("|cff0000centered text")

      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains alignment
      assert.is_truthy(html:find('align="center"'))
      assert.is_truthy(html:find("font%-size: 50pt"))
    end)
  end)
end)

-- Geometry, visibility and text readback for Geyser.Label, asserted against
-- the widget itself through getWindowGeometry/windowVisible/getLabelText
-- rather than by spying on the echo call.
describe("Tests functionality of Geyser.Label widget state", function()
  local created

  local function geometry(name)
    local x, y, width, height = getWindowGeometry(name)
    return {x = x, y = y, width = width, height = height}
  end

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

  describe("Geyser.Label:new/new2", function()
    it("creates a visible label widget at the constrained geometry", function()
      local label = track(Geyser.Label:new({name = "glsNew", x = 15, y = 25, width = 120, height = 40}))
      assert.are.equal("label", label.type)
      assert.are.equal("label", windowType("glsNew"))
      assert.are.same({x = 15, y = 25, width = 120, height = 40}, geometry("glsNew"))
      assert.is_true(windowVisible("glsNew"))
      assert.are.equal("main", label.windowname)
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "glsBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.Label:new({name = "glsInBox", x = "25%", y = "50%", width = "50%", height = "25%"}, container))
      assert.are.same({x = 200, y = 150, width = 200, height = 50}, geometry("glsInBox"))
    end)

    it("new2 marks the label as using add2", function()
      local label = track(Geyser.Label:new2({name = "glsNew2", x = 0, y = 0, width = 40, height = 20}))
      assert.is_true(label.useAdd2)
      assert.are.equal("label", windowType("glsNew2"))
    end)
  end)

  describe("Geyser.Label geometry and visibility", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glsMove", x = 10, y = 20, width = 100, height = 50}))
    end)

    it("moves and resizes the widget", function()
      label:move(70, 80)
      label:resize(90, 30)
      assert.are.same({x = 70, y = 80, width = 90, height = 30}, geometry("glsMove"))
    end)

    it("hides and shows the widget", function()
      label:hide()
      assert.is_false(windowVisible("glsMove"))
      assert.is_true(label.hidden)
      label:show()
      assert.is_true(windowVisible("glsMove"))
      assert.is_false(label.hidden)
    end)
  end)

  describe("Geyser.Label:echo/rawEcho/decho/hecho/cecho and Geyser.Label:clear", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glsText", x = 0, y = 0, width = 200, height = 50}))
    end)

    it("wraps the message in a styled div", function()
      label:echo("hello label")
      local text = getLabelText("glsText")
      assert.is_truthy(text:find("hello label", 1, true))
      assert.is_truthy(text:find("font%-size: 8pt"))
      assert.are.equal("hello label", label.message)
    end)

    it("reuses the last message when echoed with no arguments", function()
      label:echo("sticky")
      label:echo()
      assert.is_truthy(getLabelText("glsText"):find("sticky", 1, true))
    end)

    it("colours the text with the colour it is given", function()
      label:echo("red text", "red")
      assert.is_truthy(getLabelText("glsText"):find("color: #ff0000", 1, true))
      assert.are.equal("red", label.fgColor)
    end)

    it("leaves the colour to the stylesheet when told nocolor", function()
      label:echo("plain", "nocolor")
      assert.is_nil(getLabelText("glsText"):find("color: #", 1, true))
    end)

    it("applies a format string given to echo", function()
      label:echo("formatted", nil, "cb14")
      local text = getLabelText("glsText")
      assert.is_truthy(text:find('align="center"', 1, true))
      assert.is_truthy(text:find("<b>formatted</b>", 1, true))
      assert.is_truthy(text:find("font%-size: 14pt"))
    end)

    it("rawEcho writes the markup through untouched", function()
      label:rawEcho("<b>raw</b>")
      assert.are.equal("<b>raw</b>", getLabelText("glsText"))
    end)

    it("clear empties the label", function()
      label:echo("something")
      label:clear()
      assert.are.equal("", getLabelText("glsText"))
      assert.are.equal("", label.message)
    end)

    it("decho, hecho and cecho put their colours into the markup", function()
      label:decho("<0,0,255>blue")
      local blue = getLabelText("glsText")
      assert.is_truthy(blue:find("blue", 1, true))
      assert.is_truthy(blue:find("color: rgb(0, 0, 255)", 1, true))
      label:hecho("|cff0000red")
      local red = getLabelText("glsText")
      assert.is_truthy(red:find("red", 1, true))
      assert.is_truthy(red:find("color: rgb(255, 0, 0)", 1, true))
      label:cecho("<green>green")
      local green = getLabelText("glsText")
      assert.is_truthy(green:find("green", 1, true))
      assert.is_truthy(green:find("color: rgb(0, 255, 0)", 1, true))
    end)
  end)

  describe("Geyser.Label format setters", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glsFormat", x = 0, y = 0, width = 200, height = 50}))
      label:echo("styled")
    end)

    it("turns bold, italics, underline and strikethrough into markup", function()
      label:setBold(true)
      assert.is_truthy(getLabelText("glsFormat"):find("<b>styled</b>", 1, true))
      label:setItalics(true)
      label:setUnderline(true)
      label:setStrikethrough(true)
      local text = getLabelText("glsFormat")
      assert.is_truthy(text:find("<i>", 1, true))
      assert.is_truthy(text:find("<u>", 1, true))
      assert.is_truthy(text:find("<s>", 1, true))
      -- the font size was put into the format string first, the flags append
      assert.are.equal("8bius", label.format)
    end)

    it("takes the markup away again", function()
      label:setBold(true)
      label:setBold(false)
      assert.is_nil(getLabelText("glsFormat"):find("<b>", 1, true))
      assert.is_nil(label.format:find("b"))
    end)

    it("sets the font size in the markup", function()
      label:setFontSize(20)
      assert.is_truthy(getLabelText("glsFormat"):find("font%-size: 20pt"))
      assert.are.equal(20, label.fontSize)
    end)

    it("sets the alignment in the markup", function()
      label:setAlignment("center")
      assert.is_truthy(getLabelText("glsFormat"):find('align="center"', 1, true))
      label:setAlignment("right")
      assert.is_truthy(getLabelText("glsFormat"):find('align="right"', 1, true))
      label:setAlignment("")
      assert.is_nil(getLabelText("glsFormat"):find("align=", 1, true))
    end)

    it("sets the text colour", function()
      label:setFgColor("#00ff00")
      assert.is_truthy(getLabelText("glsFormat"):find("color: #00ff00", 1, true))
    end)

    it("setFormat replaces the whole format at once", function()
      label:setFormat("ci18")
      local text = getLabelText("glsFormat")
      assert.is_truthy(text:find('align="center"', 1, true))
      assert.is_truthy(text:find("<i>styled</i>", 1, true))
      assert.is_truthy(text:find("font%-size: 18pt"))
      assert.is_nil(text:find("<b>", 1, true))
    end)

    it("processFormatString fills in the format table", function()
      label:processFormatString("bu12")
      assert.are.equal(true, label.formatTable.bold)
      assert.are.equal(true, label.formatTable.underline)
      assert.are.equal(false, label.formatTable.italics)
      assert.are.equal("12", label.formatTable.fontSize)
      assert.are.equal("", label.formatTable.alignment)
    end)

    it("keeps the label's own font size when the format string has no number", function()
      label:setFontSize(11)
      label:processFormatString("b")
      assert.are.equal(11, label.formatTable.fontSize)
      assert.are.equal("b11", label.format)
    end)

    it("rejects an alignment it does not know", function()
      local ok, message = pcall(function() label:setAlignment("nonsense") end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("invalid alignment sent", 1, true))
    end)

    it("rejects a font size that is not a number", function()
      local ok, message = pcall(function() label:setFontSize("big") end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("fontSize as number expected, got string", 1, true))
    end)

    it("rejects a format that is not a string", function()
      local ok, message = pcall(function() label:processFormatString(42) end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("format as string expected, got number", 1, true))
    end)
  end)

  describe("Geyser.Label:getSizeHint and Geyser.Label auto-size adjustSize/adjustHeight/adjustWidth/autoAdjustSize/enableAutoAdjustSize/disableAutoAdjustSize", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glsHint", x = 0, y = 0, width = 400, height = 200}))
      label:echo("some text in a label")
    end)

    it("reports a size hint big enough for the content", function()
      local width, height = label:getSizeHint()
      assert.is_true(width > 0)
      assert.is_true(height > 0)
      -- the hint comes from the font metrics, so it is only bounded loosely
      assert.is_true(width < 400, "size hint width was " .. tostring(width))
    end)

    it("adjustSize resizes the widget to the hint", function()
      local width, height = label:getSizeHint()
      label:adjustSize()
      assert.are.same({x = 0, y = 0, width = width, height = height}, geometry("glsHint"))
    end)

    it("adjustWidth only touches the width", function()
      local width = label:getSizeHint()
      label:adjustWidth()
      assert.are.same({x = 0, y = 0, width = width, height = 200}, geometry("glsHint"))
    end)

    it("adjustHeight only touches the height", function()
      local _, height = label:getSizeHint()
      label:adjustHeight()
      assert.are.same({x = 0, y = 0, width = 400, height = height}, geometry("glsHint"))
    end)

    it("enableAutoAdjustSize makes every echo fit the content", function()
      assert.is_true(label:enableAutoAdjustSize())
      label:echo("tiny")
      local width, height = label:getSizeHint()
      assert.are.same({x = 0, y = 0, width = width, height = height}, geometry("glsHint"))
    end)

    it("enableAutoAdjustSize can be limited to one dimension", function()
      label:enableAutoAdjustSize(false)
      assert.is_false(label.autoWidth)
      assert.is_true(label.autoHeight)
      label:echo("tiny")
      local _, height = label:getSizeHint()
      assert.are.same({x = 0, y = 0, width = 400, height = height}, geometry("glsHint"))
    end)

    it("disableAutoAdjustSize leaves the size alone again", function()
      label:enableAutoAdjustSize()
      label:echo("tiny")
      assert.is_true(label:disableAutoAdjustSize())
      label:resize(400, 200)
      label:echo("a much longer piece of text than before")
      assert.are.same({x = 0, y = 0, width = 400, height = 200}, geometry("glsHint"))
    end)
  end)

  describe("Geyser.Label:setStyleSheet and Geyser.Label:getFormat", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glsStyle", x = 0, y = 0, width = 100, height = 50}))
    end)

    it("round-trips a stylesheet through Mudlet", function()
      label:setStyleSheet("background-color: red; border: 1px solid white;")
      assert.are.equal("background-color: red; border: 1px solid white;", getLabelStyleSheet("glsStyle"))
      assert.are.equal("background-color: red; border: 1px solid white;", label.stylesheet)
    end)

    it("reuses the stored stylesheet when called with no argument", function()
      label.stylesheet = "background-color: blue;"
      label:setStyleSheet()
      assert.are.equal("background-color: blue;", getLabelStyleSheet("glsStyle"))
    end)

    it("setTiledBackgroundImage puts the image into the stylesheet", function()
      label:setTiledBackgroundImage("/tmp/nosuchimage.png")
      assert.are.equal("background-image: url(/tmp/nosuchimage.png);", getLabelStyleSheet("glsStyle"))
    end)

    it("getFormat reports the label's format defaults", function()
      local format = label:getFormat()
      assert.are.equal("table", type(format))
      assert.is_false(format.bold)
      assert.is_false(format.italic)
      assert.are.equal("table", type(format.foreground))
    end)
  end)

  describe("Geyser.Label:type_delete", function()
    it("deletes the widget with the object", function()
      local label = track(Geyser.Label:new({name = "glsDelete", x = 0, y = 0, width = 40, height = 20}))
      assert.is_not_nil(getWindowGeometry("glsDelete"))
      label:delete()
      assert.is_nil(getWindowGeometry("glsDelete"))
      assert.is_nil(Geyser.windowList.glsDelete)
    end)

    it("clears the nested label bookkeeping", function()
      local label = track(Geyser.Label:new({name = "glsNested", x = 0, y = 0, width = 40, height = 20}))
      label.nestedLabels = {"something"}
      label:delete()
      assert.are.same({}, label.nestedLabels)
    end)
  end)
end)
