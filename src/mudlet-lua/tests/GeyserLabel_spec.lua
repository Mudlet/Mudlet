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

    it("starts out hidden when the constraints ask for it", function()
      local label = track(Geyser.Label:new({name = "glsHiddenNew", x = 0, y = 0, width = 40, height = 20, hidden = true}))
      assert.is_true(label.hidden)
      assert.is_false(windowVisible("glsHiddenNew"))
      label:show()
      assert.is_false(label.hidden)
      assert.is_true(windowVisible("glsHiddenNew"))
    end)

    it("starts out hidden when new2 constraints ask for it", function()
      local label = track(Geyser.Label:new2({name = "glsHiddenNew2", x = 0, y = 0, width = 40, height = 20, hidden = true}))
      assert.is_true(label.hidden)
      assert.is_false(windowVisible("glsHiddenNew2"))
      label:show()
      assert.is_true(windowVisible("glsHiddenNew2"))
    end)

    it("starts out hidden when the constraints ask for auto_hidden", function()
      local label = track(Geyser.Label:new({name = "glsAutoHiddenNew", x = 0, y = 0, width = 40, height = 20, auto_hidden = true}))
      assert.is_true(label.auto_hidden)
      assert.is_false(windowVisible("glsAutoHiddenNew"))
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

    -- autoAdjustSize is what every echo calls; called by hand it has to be a
    -- no-op unless one of the two flags is set, or a label that never asked to
    -- be auto sized would shrink to its text the first time anything ran it
    it("autoAdjustSize does nothing while neither dimension is auto sized", function()
      assert.is_nil(label.autoWidth)
      assert.is_nil(label.autoHeight)
      label:autoAdjustSize()
      assert.are.same({x = 0, y = 0, width = 400, height = 200}, geometry("glsHint"))
    end)

    it("autoAdjustSize fits the dimensions that are auto sized", function()
      label.autoHeight = true
      local _, height = label:getSizeHint()
      label:autoAdjustSize()
      assert.are.same({x = 0, y = 0, width = 400, height = height}, geometry("glsHint"))
      -- the hint is re-read, because the height it just took can move the width
      -- Qt suggests for the same text
      label.autoWidth = true
      local width, secondHeight = label:getSizeHint()
      label:autoAdjustSize()
      assert.are.same({x = 0, y = 0, width = width, height = secondHeight}, geometry("glsHint"))
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

  describe("Geyser.Label clickthrough and cursor", function()
    it("enableClickthrough and disableClickthrough track the flag on the object", function()
      local label = track(Geyser.Label:new({name = "glsClick", x = 0, y = 0, width = 40, height = 20}))
      label:enableClickthrough()
      assert.is_true(label.clickthrough)
      label:disableClickthrough()
      assert.is_false(label.clickthrough)
    end)

    it("setCursor stores the shape as a name whichever form it was given", function()
      local label = track(Geyser.Label:new({name = "glsCursor", x = 0, y = 0, width = 40, height = 20}))
      label:setCursor("OpenHand")
      assert.are.equal("OpenHand", label.cursorShape)
      label:setCursor(mudlet.cursor.ClosedHand)
      assert.are.equal("ClosedHand", label.cursorShape)
    end)

    it("resetCursor puts the shape back to the default", function()
      local label = track(Geyser.Label:new({name = "glsResetCursor", x = 0, y = 0, width = 40, height = 20}))
      label:setCursor("OpenHand")
      label:setCustomCursor(":/icons/mudlet.png")
      label:resetCursor()
      assert.are.equal(0, label.cursorShape)
      assert.are.equal("", label.customCursor)
    end)

    it("setCustomCursor passes the image and hotspot on and remembers it", function()
      -- there is no getter for a label cursor, so spy on the global to see
      -- the hotspot defaults the wrapper fills in; spy.on keeps the real call
      local customCursor = spy.on(_G, "setLabelCustomCursor")
      finally(function() customCursor:revert() end)
      local label = track(Geyser.Label:new({name = "glsCustomCursor", x = 0, y = 0, width = 40, height = 20}))
      label:setCustomCursor(":/icons/mudlet.png", 1, 2)
      assert.spy(customCursor).was.called_with("glsCustomCursor", ":/icons/mudlet.png", 1, 2)
      assert.are.equal(":/icons/mudlet.png", label.customCursor)
      label:setCustomCursor(":/icons/mudlet.png")
      assert.spy(customCursor).was.called_with("glsCustomCursor", ":/icons/mudlet.png", -1, -1)
    end)
  end)

  describe("Geyser.Label:setBackgroundImage", function()
    it("puts the image on the label without touching its stylesheet", function()
      -- there is no getter for a label's background image, so spy on the
      -- global; the stylesheet assertion is what separates this from the
      -- tiled variant below, which works through the stylesheet instead
      local background = spy.on(_G, "setBackgroundImage")
      finally(function() background:revert() end)
      local label = track(Geyser.Label:new({name = "glsBackground", x = 0, y = 0, width = 40, height = 20}))
      label:setStyleSheet("border: 1px solid red;")
      label:setBackgroundImage(":/icons/mudlet.png")
      assert.spy(background).was.called_with("glsBackground", ":/icons/mudlet.png")
      assert.are.equal("border: 1px solid red;", getLabelStyleSheet("glsBackground"))
    end)

    it("setTiledBackgroundImage goes through the stylesheet instead", function()
      local label = track(Geyser.Label:new({name = "glsTiled", x = 0, y = 0, width = 40, height = 20}))
      label:setTiledBackgroundImage("/tmp/whatever.png")
      assert.are.equal("background-image: url(/tmp/whatever.png);", getLabelStyleSheet("glsTiled"))
    end)
  end)
end)

-- The movie wrappers, the callback registration bookkeeping and the nested
-- label machinery. All three are places where Geyser keeps state of its own
-- alongside the widget's, and the state is what these specs read back: a real
-- mouse is what fires the callbacks and what drives the nest, and Lua cannot
-- make one.
describe("Tests Geyser.Label movies, callbacks and nesting", function()
  local created
  local container
  local gifPath = getMudletHomeDir() .. "/geyser_label_spec.gif"

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

  -- The smallest animated GIF there is: 1x1 pixels, two frames, a two entry
  -- colour table. Written at run time so that no binary has to be committed.
  local function writeAnimatedGif(path)
    local bytes = {
      0x47, 0x49, 0x46, 0x38, 0x39, 0x61,             -- "GIF89a"
      0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00,       -- 1x1, global colour table of 2
      0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,             -- black, white
      0x21, 0xFF, 0x0B,                               -- application extension
      0x4E, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45, -- "NETSCAPE"
      0x32, 0x2E, 0x30,                               -- "2.0"
      0x03, 0x01, 0x00, 0x00, 0x00,                   -- loop forever
      0x21, 0xF9, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, -- frame 1 control block
      0x2C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
      0x02, 0x02, 0x44, 0x01, 0x00,                   -- frame 1: the black pixel
      0x21, 0xF9, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, -- frame 2 control block
      0x2C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
      0x02, 0x02, 0x4C, 0x01, 0x00,                   -- frame 2: the white pixel
      0x3B,                                           -- trailer
    }
    local characters = {}
    for index, byte in ipairs(bytes) do
      characters[index] = string.char(byte)
    end
    local file = assert(io.open(path, "wb"), "could not write the GIF fixture")
    file:write(table.concat(characters))
    file:close()
  end

  setup(function()
    writeAnimatedGif(gifPath)
  end)

  teardown(function()
    os.remove(gifPath)
  end)

  before_each(function()
    created = {}
    container = track(Geyser.Container:new({name = "glnHost", x = 0, y = 0, width = 600, height = 400}))
  end)

  after_each(function()
    -- doNestShow/doNestLeave arm a timer that closes the nest seconds later,
    -- long after the labels it closes have been deleted
    if Geyser.Label.closeAllTimer then
      killTimer(Geyser.Label.closeAllTimer)
      Geyser.Label.closeAllTimer = nil
    end
    for _, object in ipairs(created) do
      -- the scroll tables are keyed by the label object and outlive it
      Geyser.Label.scrollV[object] = nil
      Geyser.Label.scrollH[object] = nil
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.Label movie wrappers", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glnMovie", x = 0, y = 0, width = 60, height = 40}, container))
    end)

    it("setMovie puts the GIF on the label", function()
      assert.is_true(label:setMovie(gifPath))
    end)

    it("setMovie reports a file that is not a movie", function()
      local ok, message = label:setMovie(getMudletHomeDir() .. "/nosuchmovie.gif")
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_truthy(message:find("no valid movie", 1, true))
    end)

    it("startMovie and pauseMovie drive the movie that was set", function()
      label:setMovie(gifPath)
      assert.is_true(label:startMovie())
      assert.is_true(label:pauseMovie())
      assert.is_true(label:startMovie())
    end)

    it("the movie functions all refuse a label with no movie on it", function()
      local bare = track(Geyser.Label:new({name = "glnNoMovie", x = 0, y = 0, width = 60, height = 40}, container))
      for _, call in ipairs({
        function() return bare:startMovie() end,
        function() return bare:pauseMovie() end,
        function() return bare:setMovieSpeed(200) end,
        function() return bare:setMovieFrame(0) end,
        function() return bare:scaleMovie() end,
      }) do
        local ok, message = call()
        assert.is_nil(ok)
        assert.is_truthy(message:find("no movie found at label 'glnNoMovie'", 1, true))
      end
    end)

    it("setMovieSpeed takes a percentage and refuses anything else", function()
      label:setMovie(gifPath)
      assert.is_true(label:setMovieSpeed(200))
      assert.is_true(label:setMovieSpeed(50))
      assert.has_error(function() label:setMovieSpeed("double") end)
    end)

    it("setMovieFrame reports whether the frame could be reached", function()
      label:setMovie(gifPath)
      assert.is_true(label:setMovieFrame(0))
      -- the fixture has two frames, so this one is out of reach
      assert.is_false(label:setMovieFrame(99))
      assert.has_error(function() label:setMovieFrame("first") end)
    end)

    -- whether the movie is actually being kept at the label's size is not
    -- readable from Lua: the connection scaleMovie(true) makes lives on the
    -- widget and there is no getter for the movie's scaled size
    pending("the movie following the label's size needs a getter for the scaled size")

    it("scaleMovie turns scaling on unless it is explicitly told false", function()
      -- the argument is what carries the meaning and the return value is true
      -- either way, so watch what the wrapper passes on
      local scaling = spy.on(_G, "scaleMovie")
      finally(function() scaling:revert() end)
      label:setMovie(gifPath)

      assert.is_true(label:scaleMovie(false))
      assert.spy(scaling).was.called_with("glnMovie", false)

      assert.is_true(label:scaleMovie(true))
      assert.spy(scaling).was.called_with("glnMovie", true)
    end)

    it("scaleMovie treats anything that is not false as a yes", function()
      local scaling = spy.on(_G, "scaleMovie")
      finally(function() scaling:revert() end)
      label:setMovie(gifPath)

      -- no argument at all, and a nil one, both mean scale
      assert.is_true(label:scaleMovie())
      assert.is_true(label:scaleMovie(nil))
      -- so does something that is not a boolean, rather than raising
      assert.is_true(label:scaleMovie("nonsense"))
      assert.spy(scaling).was.called(3)
      assert.spy(scaling).was_not.called_with("glnMovie", false)
    end)
  end)

  describe("Geyser.Label callback registration", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glnCallback", x = 0, y = 0, width = 60, height = 40}, container))
    end)

    -- firing needs a real mouse over a real widget, which the suite has no way
    -- of producing; what is checked here is that the registration reached the
    -- widget and that Geyser remembers it
    pending("the label callbacks firing on a real mouse event needs GUI automation")

    it("remembers the function and the arguments it registered", function()
      local handler = function() end
      label:setClickCallback(handler, "first", 2)
      assert.are.equal(handler, label.clickCallback)
      assert.are.same({"first", 2}, label.clickArgs)
    end)

    it("passes the label name, the function and the arguments straight through", function()
      -- no getter for a registered callback, so spy on the global; spy.on
      -- leaves the real registration in place
      local registration = spy.on(_G, "setLabelClickCallback")
      finally(function() registration:revert() end)
      local handler = function() end
      label:setClickCallback(handler, "first", 2)
      assert.spy(registration).was.called_with("glnCallback", handler, "first", 2)
    end)

    it("registers each kind of callback through its own global", function()
      local handler = function() end
      local globals = {
        setClickCallback = "setLabelClickCallback",
        setDoubleClickCallback = "setLabelDoubleClickCallback",
        setReleaseCallback = "setLabelReleaseCallback",
        setMoveCallback = "setLabelMoveCallback",
        setWheelCallback = "setLabelWheelCallback",
        setOnEnter = "setLabelOnEnter",
        setOnLeave = "setLabelOnLeave",
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

      for method, global in pairs(globals) do
        local registration = spy.on(_G, global)
        spied[#spied + 1] = global
        label[method](label, handler, "arg")
        assert.spy(registration).was.called_with("glnCallback", handler, "arg")
      end
    end)

    it("deregisters when it is handed nil instead of a function", function()
      label:setClickCallback(function() end, "first")
      label:setClickCallback(nil)
      assert.is_nil(label.clickCallback)
      assert.are.same({}, label.clickArgs)
    end)

    it("remembers what the other callbacks registered too", function()
      local handler = function() end
      label:setDoubleClickCallback(handler, "d")
      label:setReleaseCallback(handler, "r")
      label:setMoveCallback(handler, "m")
      label:setWheelCallback(handler, "w")
      label:setOnEnter(handler, "e")
      label:setOnLeave(handler, "l")
      assert.are.same({"d"}, label.doubleClickArgs)
      assert.are.same({"r"}, label.releaseArgs)
      assert.are.same({"m"}, label.moveArgs)
      assert.are.same({"w"}, label.wheelArgs)
      assert.are.same({"e"}, label.onEnterArgs)
      assert.are.same({"l"}, label.onLeaveArgs)
      assert.are.equal(handler, label.doubleClickCallback)
      assert.are.equal(handler, label.releaseCallback)
      assert.are.equal(handler, label.moveCallback)
      assert.are.equal(handler, label.wheelCallback)
      assert.are.equal(handler, label.onEnter)
      assert.are.equal(handler, label.onLeave)
    end)

    it("hard-errors on a callback that is neither a function nor nil", function()
      assert.has_error(function() label:setClickCallback(42) end)
      assert.has_error(function() label:setWheelCallback({}) end)
    end)

    it("registers the callbacks the constructor was given", function()
      local built = track(Geyser.Label:new({
        name = "glnConsCallback", x = 0, y = 0, width = 60, height = 40,
        clickCallback = "echo", clickArgs = {"hello"},
        doubleClickCallback = "echo", doubleClickArgs = "hello",
        onEnter = "echo", onEnterArgs = "hello",
      }, container))
      assert.are.equal("echo", built.clickCallback)
      assert.are.same({"hello"}, built.clickArgs)
      assert.are.equal("echo", built.doubleClickCallback)
      assert.are.same({"hello"}, built.doubleClickArgs)
      assert.are.equal("echo", built.onEnter)
      assert.are.same({"hello"}, built.onEnterArgs)
    end)

    it("hands a double-click callback back to the constructor's spelling", function()
      -- the constructor and Geyser.Label:new's re-registration only look at
      -- doubleClickCallback, so a setter that stored any other key would leave
      -- a label that forgets its handler the moment it is rebuilt
      local handler = function() end
      label:setDoubleClickCallback(handler, "a", 2)
      assert.are.equal(handler, label.doubleClickCallback)
      assert.are.same({"a", 2}, label.doubleClickArgs)
      assert.is_nil(label.doubleclickCallback)
      assert.is_nil(label.doubleclickArgs)
    end)
  end)

  describe("Geyser.Label:addChild", function()
    local parent

    before_each(function()
      parent = track(Geyser.Label:new({name = "glnParent", x = 100, y = 100, width = 50, height = 20}, container))
    end)

    it("hands back a hidden nested label that knows its parent", function()
      local child = track(parent:addChild({name = "glnChild", width = 50, height = 20}, container))
      assert.are.equal("nestedLabel", child.type)
      assert.are.equal(parent, child.nestParent)
      assert.is_true(child.hidden)
      assert.is_false(windowVisible("glnChild"))
      assert.are.same({child}, parent.nestedLabels)
    end)

    it("defaults to flying out to the left, laid out vertically", function()
      local child = track(parent:addChild({name = "glnChildDefault", width = 50, height = 20}, container))
      assert.are.equal("L", child.flyDir)
      assert.are.equal("V", child.layoutDir)
    end)

    it("splits layoutDir into a fly direction and a layout axis", function()
      local child = track(parent:addChild({name = "glnChildRH", width = 50, height = 20, layoutDir = "RH"}, container))
      assert.are.equal("R", child.flyDir)
      assert.are.equal("H", child.layoutDir)
    end)

    it("wires the child up so that hovering it opens its own nest", function()
      local child = track(parent:addChild({name = "glnChildHover", width = 50, height = 20}, container))
      assert.are.equal("doNestEnter", child.onEnter)
      assert.are.equal("doNestLeave", child.onLeave)
    end)

    it("keeps the children in the order they were added", function()
      local first = track(parent:addChild({name = "glnChildOne", width = 50, height = 20}, container))
      local second = track(parent:addChild({name = "glnChildTwo", width = 50, height = 20}, container))
      assert.are.same({first, second}, parent.nestedLabels)
    end)

    it("puts a child with an index where the index says", function()
      local first = track(parent:addChild({name = "glnIndexOne", width = 50, height = 20}, container))
      local jumped = track(parent:addChild({name = "glnIndexTwo", width = 50, height = 20, index = 1}, container))
      assert.are.same({jumped, first}, parent.nestedLabels)
    end)

    it("nests a child under a child", function()
      local child = track(parent:addChild({name = "glnGrandParent", width = 50, height = 20}, container))
      local grandChild = track(child:addChild({name = "glnGrandChild", width = 50, height = 20}, container))
      assert.are.equal(child, grandChild.nestParent)
      assert.are.same({grandChild}, child.nestedLabels)
    end)

    it("gives a nestable label the click callback that opens its nest", function()
      local nestable = track(Geyser.Label:new({name = "glnNestable", x = 0, y = 0, width = 50, height = 20, nestable = true}, container))
      assert.are.equal("doNestShow", nestable.clickCallback)
    end)

    it("gives a nestflyout label the hover callback that opens its nest", function()
      local flyout = track(Geyser.Label:new({name = "glnFlyout", x = 0, y = 0, width = 50, height = 20, nestflyout = true}, container))
      assert.are.equal("doNestShow", flyout.onEnter)
    end)
  end)

  describe("Geyser.Label nest display and closing", function()
    local parent, child

    before_each(function()
      parent = track(Geyser.Label:new({name = "glnNestParent", x = 100, y = 100, width = 50, height = 20}, container))
      child = track(parent:addChild({name = "glnNestChild", width = 50, height = 20, layoutDir = "RV"}, container))
    end)

    it("displayNest shows the children and lays them out beside the parent", function()
      parent:displayNest()
      assert.is_true(windowVisible("glnNestChild"))
      -- flyDir R puts the child past the parent's right edge, at its own top
      assert.are.same({x = 150, y = 100, width = 50, height = 20}, geometry("glnNestChild"))
    end)

    it("displayNest stacks a second child below the first", function()
      local second = track(parent:addChild({name = "glnNestChildTwo", width = 50, height = 20, layoutDir = "RV"}, container))
      parent:displayNest()
      assert.is_true(windowVisible("glnNestChildTwo"))
      assert.are.equal(100, geometry("glnNestChild").y)
      assert.are.equal(120, geometry(second.name).y)
    end)

    it("displayNest lays a horizontal nest out sideways instead", function()
      -- two children, because one H child lands on the same pixel as one V
      -- child would: only the second one shows which axis the nest grew along
      local first = track(parent:addChild({name = "glnNestSideways", width = 50, height = 20, layoutDir = "RH"}, container))
      local second = track(parent:addChild({name = "glnNestSidewaysTwo", width = 50, height = 20, layoutDir = "RH"}, container))
      parent:displayNest()

      assert.is_true(windowVisible(first.name))
      assert.is_true(windowVisible(second.name))
      assert.are.same({x = 150, y = 100}, {x = geometry(first.name).x, y = geometry(first.name).y})
      -- along x, where the vertical nest would have gone along y
      assert.are.same({x = 200, y = 100}, {x = geometry(second.name).x, y = geometry(second.name).y})
    end)

    it("closeNestChildren hides the children again", function()
      parent:displayNest()
      closeNestChildren(parent)
      assert.is_false(windowVisible("glnNestChild"))
    end)

    it("closeNestChildren reaches grandchildren too", function()
      local grandChild = track(child:addChild({name = "glnNestGrandChild", width = 50, height = 20}, container))
      parent:displayNest()
      child:displayNest()
      assert.is_true(windowVisible(grandChild.name))

      closeNestChildren(parent)
      assert.is_false(windowVisible("glnNestChild"))
      assert.is_false(windowVisible(grandChild.name))
    end)

    it("closeNestChildren does nothing for a label with no nest", function()
      local lonely = track(Geyser.Label:new({name = "glnLonely", x = 0, y = 0, width = 50, height = 20}, container))
      assert.has_no.errors(function() closeNestChildren(lonely) end)
      assert.is_true(windowVisible("glnLonely"))
    end)

    it("closeAllLevels hides every nested label in the container", function()
      local other = track(Geyser.Label:new({name = "glnOtherParent", x = 300, y = 100, width = 50, height = 20}, container))
      local otherChild = track(other:addChild({name = "glnOtherChild", width = 50, height = 20}, container))
      parent:displayNest()
      other:displayNest()

      closeAllLevels(parent)

      assert.is_false(windowVisible("glnNestChild"))
      assert.is_false(windowVisible(otherChild.name))
      -- the parents themselves have no nestParent, so they stay put
      assert.is_true(windowVisible("glnNestParent"))
      assert.is_true(windowVisible(other.name))
    end)

    it("closeNeighbourChildren closes the nests either side of a child", function()
      local sibling = track(parent:addChild({name = "glnSibling", width = 50, height = 20}, container))
      local siblingChild = track(sibling:addChild({name = "glnSiblingChild", width = 50, height = 20}, container))
      parent:displayNest()
      sibling:displayNest()
      assert.is_true(windowVisible(siblingChild.name))

      closeNeighbourChildren(child)

      assert.is_false(windowVisible(siblingChild.name))
    end)

    it("doNestShow opens a closed nest and arms the timer that closes it", function()
      assert.is_false(windowVisible("glnNestChild"))
      doNestShow(parent)
      assert.is_true(windowVisible("glnNestChild"))
      assert.is_number(Geyser.Label.closeAllTimer)
      assert.is_number(remainingTime(Geyser.Label.closeAllTimer))
    end)

    it("doNestShow closes a nest that is already open", function()
      -- it is the click handler of a nestable label, so clicking twice has to
      -- put the nest away again: it always closes everything first and only
      -- reopens when the first child was hidden
      doNestShow(parent)
      assert.is_true(windowVisible("glnNestChild"))
      doNestShow(parent)
      assert.is_false(windowVisible("glnNestChild"))
    end)

    it("doNestShow replaces the timer rather than stacking a second one", function()
      doNestShow(parent)
      local firstTimer = Geyser.Label.closeAllTimer
      doNestShow(parent)
      assert.are_not.equal(firstTimer, Geyser.Label.closeAllTimer)
      assert.is_nil(remainingTime(firstTimer))
    end)

    it("doNestEnter opens the nest of a child that flies out", function()
      local grandChild = track(child:addChild({name = "glnEnterGrandChild", width = 50, height = 20}, container))
      child.flyOut = true
      doNestEnter(child)
      assert.is_true(windowVisible(grandChild.name))
    end)

    it("doNestEnter leaves the nest of a child that does not fly out closed", function()
      local grandChild = track(child:addChild({name = "glnNoFlyGrandChild", width = 50, height = 20}, container))
      child.flyOut = nil
      doNestEnter(child)
      assert.is_false(windowVisible(grandChild.name))
    end)

    it("doNestEnter cancels the timer that would have closed everything", function()
      doNestShow(parent)
      local armed = Geyser.Label.closeAllTimer
      doNestEnter(child)
      assert.is_nil(remainingTime(armed))
    end)

    it("doNestEnter ignores being handed nothing", function()
      assert.has_no.errors(function() doNestEnter(nil) end)
    end)

    it("doNestLeave arms the timer that closes everything", function()
      doNestLeave(child)
      assert.is_number(Geyser.Label.closeAllTimer)
      assert.is_number(remainingTime(Geyser.Label.closeAllTimer))
    end)
  end)

  describe("Geyser.Label:addScrollbars and doNestScroll", function()
    local parent, first, second

    before_each(function()
      parent = track(Geyser.Label:new({name = "glnScrollParent", x = 100, y = 100, width = 50, height = 20}, container))
      first = track(parent:addChild({name = "glnScrollOne", width = 50, height = 20, layoutDir = "RV"}, container))
      second = track(parent:addChild({name = "glnScrollTwo", width = 50, height = 20, layoutDir = "RV"}, container))
    end)

    local function makeScrollbars()
      local bars = Geyser.Label:addScrollbars(parent, "RV")
      track(bars[1])
      track(bars[2])
      Geyser.Label.scrollV[parent] = bars
      finally(function() Geyser.Label.scrollV[parent] = nil end)
      return bars[1], bars[2]
    end

    it("makes a backward and a forward label named after the nest", function()
      local backward, forward = makeScrollbars()
      assert.are.equal("backScrollglnScrollOneRV", backward.name)
      assert.are.equal("forScrollglnScrollOneRV", forward.name)
      assert.are.equal(parent, backward.nestParent)
      assert.are.equal(parent, forward.nestParent)
      assert.are.equal("More...", forward.message)
    end)

    it("sizes the forward scrollbar's reach to the nest it scrolls", function()
      local _, forward = makeScrollbars()
      -- two children in the nest, plus the scroll window's own end marker
      assert.are.equal(3, forward.maxScroll)
    end)

    it("wires both scrollbars up to doNestScroll", function()
      local backward, forward = makeScrollbars()
      assert.are.equal("doNestScroll", backward.clickCallback)
      assert.are.equal("doNestScroll", forward.clickCallback)
      assert.are.equal("doNestEnter", forward.onEnter)
      assert.are.equal("doNestLeave", forward.onLeave)
    end)

    it("doNestScroll moves the window forward when the forward bar is clicked", function()
      local backward, forward = makeScrollbars()
      backward.scroll, forward.scroll, forward.maxScroll = 0, 3, 5

      doNestScroll(forward)

      assert.are.equal(1, backward.scroll)
      assert.are.equal(4, forward.scroll)
    end)

    it("doNestScroll moves the window back when the backward bar is clicked", function()
      local backward, forward = makeScrollbars()
      backward.scroll, forward.scroll, forward.maxScroll = 1, 4, 5

      doNestScroll(backward)

      assert.are.equal(0, backward.scroll)
      assert.are.equal(3, forward.scroll)
    end)

    it("doNestScroll will not scroll back past the first entry", function()
      local backward, forward = makeScrollbars()
      backward.scroll, forward.scroll, forward.maxScroll = 0, 3, 5

      doNestScroll(backward)

      assert.are.equal(0, backward.scroll)
      assert.are.equal(3, forward.scroll, "the window has to keep its size when it hits the top")
    end)

    it("doNestScroll will not scroll on past the last entry", function()
      local backward, forward = makeScrollbars()
      backward.scroll, forward.scroll, forward.maxScroll = 2, 5, 5

      doNestScroll(forward)

      assert.are.equal(2, backward.scroll)
      assert.are.equal(5, forward.scroll)
    end)
  end)
end)

-- The label's font, hyperlink styling and tooltip. Only the tooltip has a
-- getter, so the other two are read back from the object's own bookkeeping and
-- from what the wrapper handed the global - which is where Geyser's own
-- default of an underline unless one is refused lives.
describe("Tests Geyser.Label font, link style and tooltip", function()
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

  describe("Geyser.Label:setFont", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glfFont", x = 0, y = 0, width = 200, height = 50}))
      label:echo("some text")
    end)

    it("puts the family into the markup and remembers it", function()
      -- Ubuntu Mono ships with Mudlet, so it is available on every platform
      label:setFont("Ubuntu Mono")
      assert.are.equal("Ubuntu Mono", label.font)
      assert.is_truthy(getLabelText("glfFont"):find('<font face ="Ubuntu Mono">', 1, true))
    end)

    it("takes the family back out again when it is given an empty string", function()
      label:setFont("Ubuntu Mono")
      label:setFont("")
      assert.are.equal("", label.font)
      assert.is_nil(getLabelText("glfFont"):find("<font face", 1, true))
    end)

    it("says so rather than raising when the font is not installed", function()
      local debugMessage = spy.on(_G, "debugc")
      finally(function() debugMessage:revert() end)
      assert.has_no.errors(function() label:setFont("No Such Font At All") end)
      assert.spy(debugMessage).was.called()
      assert.is_truthy(debugMessage.calls[#debugMessage.calls].vals[1]:find("No Such Font At All", 1, true))
      -- it still records what it was asked for, so the markup shows the ask
      assert.are.equal("No Such Font At All", label.font)
    end)
  end)

  describe("Geyser.Label:setLinkStyle/resetLinkStyle/clearVisitedLinks", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glfLink", x = 0, y = 0, width = 200, height = 50}))
    end)

    -- there is no getter for a label's link styling, so what the wrapper hands
    -- the global is the observable part; spy.on keeps the real call
    it("underlines links unless it is told not to", function()
      local linkStyle = spy.on(_G, "setLinkStyle")
      finally(function() linkStyle:revert() end)

      label:setLinkStyle("cyan", "purple")
      assert.spy(linkStyle).was.called_with("glfLink", "cyan", "purple", true)

      label:setLinkStyle("cyan", "purple", false)
      assert.spy(linkStyle).was.called_with("glfLink", "cyan", "purple", false)
    end)

    it("reaches the label without raising", function()
      assert.has_no.errors(function()
        label:setLinkStyle("#00ffff", "#ff00ff")
        label:clearVisitedLinks()
        label:resetLinkStyle()
      end)
    end)

    it("stays quiet about a label that is no longer there", function()
      -- the globals underneath answer nil and a message for a label they cannot
      -- find, and a wrapper that raised on it would take a script down for a
      -- widget it had already deleted
      local gone = track(Geyser.Label:new({name = "glfLinkGone", x = 0, y = 0, width = 40, height = 20}))
      gone:delete()
      assert.has_no.errors(function()
        gone:setLinkStyle("cyan", "purple")
        gone:resetLinkStyle()
        gone:clearVisitedLinks()
      end)
    end)

    -- getLabelText() answers with the text after the styling pass has rewritten
    -- it, which is the only readable trace the injected colour leaves
    it("colours an anchor whichever whitespace follows its tag name", function()
      label:setLinkStyle("#00ffff", "#ff00ff")

      label:echo([[<a href='https://example.com'>go</a>]])
      assert.is_truthy(getLabelText("glfLink"):find("#00ffff", 1, true))

      label:echo("<a\nhref='https://example.com'>go</a>")
      assert.is_truthy(getLabelText("glfLink"):find("#00ffff", 1, true))
    end)

    pending("Geyser.Label:setLinkStyle reporting whether the styling reached the label - the three wrappers discard the nil and error message their global answers with, and Mudlet has no link style getter")
  end)

  describe("Geyser.Label:setToolTip/resetToolTip", function()
    local label

    before_each(function()
      label = track(Geyser.Label:new({name = "glfToolTip", x = 0, y = 0, width = 200, height = 50}))
    end)

    it("puts the text on the widget and remembers it", function()
      assert.are.equal("", getLabelToolTip("glfToolTip"))
      label:setToolTip("what this does")
      assert.are.equal("what this does", getLabelToolTip("glfToolTip"))
      assert.are.equal("what this does", label.toolTip)
      label:setToolTip("and now this")
      assert.are.equal("and now this", getLabelToolTip("glfToolTip"))
    end)

    it("defaults the duration to ten seconds", function()
      -- the duration is not part of what getLabelToolTip reports, so the
      -- default Geyser fills in is watched on the way to the global
      local toolTip = spy.on(_G, "setLabelToolTip")
      finally(function() toolTip:revert() end)

      label:setToolTip("what this does")
      assert.are.equal(10, label.toolTipDuration)
      assert.spy(toolTip).was.called_with("glfToolTip", "what this does", 10)

      label:setToolTip("and now this", 3)
      assert.are.equal(3, label.toolTipDuration)
      assert.spy(toolTip).was.called_with("glfToolTip", "and now this", 3)
    end)

    it("takes the tooltip off the widget again on reset", function()
      label:setToolTip("temporary", 1)
      assert.are.equal("temporary", getLabelToolTip("glfToolTip"))

      label:resetToolTip()

      assert.are.equal("", getLabelToolTip("glfToolTip"))
      assert.is_nil(label.toolTip)
      assert.is_nil(label.toolTipDuration)
    end)

    it("takes the tooltip the constructor was given", function()
      local built = track(Geyser.Label:new({
        name = "glfToolTipCons", x = 0, y = 0, width = 40, height = 20,
        toolTip = "from the constructor", toolTipDuration = 7,
      }))
      -- the constraint copy alone would leave toolTip looking right, so the
      -- widget is what says the constructor applied it
      assert.are.equal("from the constructor", getLabelToolTip("glfToolTipCons"))
      assert.are.equal("from the constructor", built.toolTip)
      assert.are.equal(7, built.toolTipDuration)
    end)
  end)
end)

-- The right click menu. Its items are nested labels registered at the top of
-- Geyser rather than under the label that owns the menu, so the block sweeps
-- everything it added rather than relying on the host label's delete.
describe("Tests Geyser.Label right click menus", function()
  local label
  local topLevelBefore

  local function newTopLevelObjects()
    local new = {}
    for name in pairs(Geyser.windowList) do
      if not topLevelBefore[name] then
        new[#new + 1] = name
      end
    end
    return new
  end

  local function menuItem(name)
    local item = label:findMenuElement(name)
    assert.is_not_nil(item, "there is no menu item called " .. name)
    return item
  end

  local function menuOrder()
    local names = {}
    for index, item in ipairs(label.rightClickMenu.nestedLabels) do
      names[index] = item.menuName
    end
    return names
  end

  before_each(function()
    topLevelBefore = {}
    for name in pairs(Geyser.windowList) do
      topLevelBefore[name] = true
    end
    label = Geyser.Label:new({name = "glmHost", x = 10, y = 10, width = 100, height = 30})
    label:createRightClickMenu({MenuItems = {"First", "Second", "Third"}})
  end)

  after_each(function()
    -- doNestShow arms a timer that closes the nest seconds later, long after
    -- the labels it closes have been deleted
    if Geyser.Label.closeAllTimer then
      killTimer(Geyser.Label.closeAllTimer)
      Geyser.Label.closeAllTimer = nil
    end
    if label and Geyser.windowList.glmHost == label then
      label:delete()
    end
    label = nil
    for _, name in ipairs(newTopLevelObjects()) do
      local object = Geyser.windowList[name]
      if object then
        Geyser.Label.scrollV[object] = nil
        Geyser.Label.scrollH[object] = nil
        object:delete()
      end
    end
  end)

  describe("Geyser.Label:createRightClickMenu/createMenuItems", function()
    it("makes a nestable menu label and hangs it off a right click", function()
      local menu = label.rightClickMenu
      assert.are.equal("glmHostrightClickMenu", menu.name)
      assert.are.equal(label, menu.container)
      assert.are.equal(Geyser.Label.onRightClick, label.clickCallback)
      -- the menu itself is a nestable label of no size, so only its items show
      assert.are.equal(0, menu:get_width())
      assert.are.equal(0, menu:get_height())
    end)

    it("makes one hidden item per entry, in order", function()
      assert.are.same({"First", "Second", "Third"}, menuOrder())
      for _, name in ipairs({"First", "Second", "Third"}) do
        local item = menuItem(name)
        assert.are.equal("glmHostrightClickMenu" .. name, item.name)
        assert.is_true(item.hidden)
        assert.is_truthy(getLabelText(item.name):find(name, 1, true))
      end
    end)

    it("takes the default width, height and format from the menu", function()
      local item = menuItem("First")
      assert.are.equal(140, item:get_width())
      assert.are.equal(25, item:get_height())
      assert.is_truthy(getLabelText(item.name):find('align="center"', 1, true))
      assert.is_truthy(getLabelText(item.name):find("font%-size: 10pt"))
    end)

    it("takes the width, height and format it is given instead", function()
      local other = Geyser.Label:new({name = "glmSized", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"Only"}, MenuWidth = 80, MenuHeight = 40, MenuFormat = "l16"})
      local item = other:findMenuElement("Only")
      assert.are.equal(80, item:get_width())
      assert.are.equal(40, item:get_height())
      assert.is_truthy(getLabelText(item.name):find("font%-size: 16pt"))
    end)

    it("nests a submenu under the item before it", function()
      local other = Geyser.Label:new({name = "glmNested", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"Parent", {"Child"}, "Sibling"}})
      local parent = other:findMenuElement("Parent")
      local child = other:findMenuElement("Parent.Child")
      assert.is_not_nil(child)
      assert.is_true(parent.isParent)
      assert.are.equal(parent, child.nestParent)
      -- the child is not one of the top level menu's own entries
      assert.is_nil(other:findMenuElement("Child"))
    end)
  end)

  describe("Geyser.Label:findMenuElement", function()
    it("says what it could not find", function()
      local item, message = label:findMenuElement("Nowhere")
      assert.is_nil(item)
      assert.is_truthy(message:find("Couldn't find menu element Nowhere", 1, true))
    end)

    it("hands back nothing at all when it is given no name", function()
      assert.is_nil(label:findMenuElement())
    end)

    it("hands back a nested item's own submenu when it is asked for a parent", function()
      local other = Geyser.Label:new({name = "glmFindParent", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"A", {"B", {"C"}}}})

      local element, submenu = other:findMenuElement("A.B", other.rightClickMenu, true)

      -- the submenu hanging off B, not the submenu B is itself listed in
      assert.are.same({"C"}, submenu)
      assert.are.equal(other:findMenuElement("A.B"), element)
    end)

    it("names an item for the parent it sits in, not for its whole path", function()
      -- this is what makes an item added under "A.B" answer to "B.D", and it is
      -- why a name with more than one dot in it never resolves
      local other = Geyser.Label:new({name = "glmDeepName", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"A", {"B", {"C"}}}})

      assert.is_not_nil(other:findMenuElement("B.C"))
      assert.is_nil(other:findMenuElement("A.B.C"))
    end)
  end)

  describe("Geyser.Label:setMenuAction", function()
    it("puts a click callback on the item it names", function()
      local handler = function() end
      label:setMenuAction("First", handler, "an argument")
      local item = menuItem("First")
      assert.are.equal(handler, item.clickCallback)
      assert.are.same({"an argument"}, item.clickArgs)
      -- and leaves the other items on the callback the nest gave them
      assert.are.equal("doNestShow", menuItem("Second").clickCallback)
    end)

    it("raises for an item that is not in the menu", function()
      local ok, message = pcall(function() label:setMenuAction("Nowhere", function() end) end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("setMenuAction: Couldn't find menu element Nowhere", 1, true))
    end)
  end)

  describe("Geyser.Label:hideMenuLabel/showMenuLabel", function()
    it("takes an item out of the menu and puts it back where it was", function()
      label:hideMenuLabel("Second")
      assert.is_true(menuItem("Second").ignore)
      assert.are.same({"First", "Third"}, menuOrder())

      label:showMenuLabel("Second")
      assert.is_false(menuItem("Second").ignore)
      assert.are.same({"First", "Second", "Third"}, menuOrder())
    end)

    it("does nothing when the item is already in the state asked for", function()
      label:hideMenuLabel("Second")
      assert.has_no.errors(function() label:hideMenuLabel("Second") end)
      assert.are.same({"First", "Third"}, menuOrder())
      label:showMenuLabel("Second")
      assert.has_no.errors(function() label:showMenuLabel("Second") end)
      assert.are.same({"First", "Second", "Third"}, menuOrder())
    end)

    it("keeps a hidden item off the screen when the menu is opened", function()
      label:hideMenuLabel("Second")
      label.rightClickMenu:displayNest()
      assert.is_true(windowVisible(menuItem("First").name))
      assert.is_false(windowVisible(menuItem("Second").name))
    end)

    it("raises for an item that is not in the menu", function()
      assert.has_error(function() label:hideMenuLabel("Nowhere") end)
      assert.has_error(function() label:showMenuLabel("Nowhere") end)
    end)
  end)

  describe("Geyser.Label:addMenuLabel", function()
    it("adds an item at the end of the menu", function()
      assert.is_true(label:addMenuLabel("Fourth"))
      assert.are.same({"First", "Second", "Third", "Fourth"}, menuOrder())
      assert.is_not_nil(label:findMenuElement("Fourth"))
    end)

    it("adds an item at the index it is given", function()
      assert.is_true(label:addMenuLabel("Fourth", nil, 2))
      assert.are.same({"First", "Fourth", "Second", "Third"}, menuOrder())
    end)

    it("adds an item under a parent that was declared with a submenu", function()
      -- a parent is declared by following its name with a table of its
      -- children, and an empty one is how a submenu that is filled in later is
      -- reserved: this is how Adjustable.Container's lockstyle menu is built
      local other = Geyser.Label:new({name = "glmParented", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"Parent", {}}})

      other:addMenuLabel("Child", "Parent")

      local child = other:findMenuElement("Parent.Child")
      assert.is_not_nil(child)
      assert.are.equal(other:findMenuElement("Parent"), child.nestParent)
    end)

    it("brings a hidden item back rather than adding a second one", function()
      label:hideMenuLabel("Second")
      label:addMenuLabel("Second")
      assert.are.same({"First", "Second", "Third"}, menuOrder())
      assert.is_false(menuItem("Second").ignore)
    end)

    it("adds an item under a nested parent named by its full path", function()
      local other = Geyser.Label:new({name = "glmDeep", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"A", {"B", {"C"}}}})

      local added = other:addMenuLabel("D", "A.B")

      -- an item is named for the parent it sits in, so D under B is "B.D"
      local child = other:findMenuElement("B.D")
      assert.is_not_nil(child, "D was not put inside B")
      assert.are.equal(other:findMenuElement("A.B"), child.nestParent)
      -- and it is not a second entry beside B, in A's own submenu
      assert.is_nil(other:findMenuElement("A.D"), "D was put beside B instead of inside it")
      assert.is_true(added)
    end)

    it("reports a name it cannot make an item out of", function()
      -- without this the menu takes the value as an entry and answers true, so
      -- the caller is told an item was added that createMenuItems then skips
      local added, message = label:addMenuLabel(5)

      assert.is_false(added)
      assert.is_truthy(message:find("addMenuLabel: needs the name of the item to add", 1, true))
      assert.are.same({"First", "Second", "Third"}, menuOrder())
    end)

    it("reports a parent it cannot find rather than raising", function()
      local added, message = label:addMenuLabel("Child", "Nowhere")

      assert.is_false(added)
      assert.is_truthy(message:find("addMenuLabel: Couldn't find menu parent Nowhere", 1, true))
      assert.are.same({"First", "Second", "Third"}, menuOrder())
    end)

    it("reports a parent that was declared without a submenu", function()
      -- addMenuLabel appends to a submenu, it does not make one, so "Second" -
      -- a plain entry rather than one followed by a table of its children - is
      -- refused
      local added, message = label:addMenuLabel("Child", "Second")

      assert.is_false(added)
      assert.is_truthy(message:find("addMenuLabel: Couldn't find menu parent Second", 1, true))
      assert.are.same({"First", "Second", "Third"}, menuOrder())
      assert.is_nil(label:findMenuElement("Second.Child"))
    end)

    it("reports a nested parent that was declared without a submenu", function()
      local other = Geyser.Label:new({name = "glmDeepLeaf", x = 0, y = 0, width = 100, height = 30})
      other:createRightClickMenu({MenuItems = {"A", {"B"}}})

      local added, message = other:addMenuLabel("D", "A.B")

      assert.is_false(added)
      assert.is_truthy(message:find("addMenuLabel: Couldn't find menu parent A.B", 1, true))
      assert.is_nil(other:findMenuElement("B.D"))
      assert.is_nil(other:findMenuElement("A.D"))
    end)
  end)

  describe("Geyser.Label:changeMenuIndex", function()
    it("moves an item to the index it is given", function()
      label:changeMenuIndex("Third", 1)
      assert.are.same({"Third", "First", "Second"}, menuOrder())
      assert.are.equal(1, menuItem("Third").index)
    end)

    it("moves an item down the menu as well as up it", function()
      label:changeMenuIndex("First", 3)
      assert.are.same({"Second", "Third", "First"}, menuOrder())
    end)

    it("raises for an item that is not in the menu", function()
      assert.has_error(function() label:changeMenuIndex("Nowhere", 1) end)
    end)
  end)

  describe("Geyser.Label:styleMenuItems", function()
    it("restyles the items into the mode it is given", function()
      local light = getLabelStyleSheet(menuItem("First").name)
      label:styleMenuItems("dark")
      local dark = getLabelStyleSheet(menuItem("First").name)
      assert.are_not.equal(light, dark)
      assert.are.equal("dark", label.rightClickMenu.Style)
      -- every item follows, not only the first
      assert.are.equal(dark, getLabelStyleSheet(menuItem("Third").name))
      label:styleMenuItems("light")
      assert.are.equal(light, getLabelStyleSheet(menuItem("First").name))
    end)

    it("takes a stylesheet of its own, which wins over the mode", function()
      label:styleMenuItems("dark", "QLabel{ background-color: red; }")
      assert.are.equal("QLabel{ background-color: red; }", getLabelStyleSheet(menuItem("First").name))
    end)
  end)

  describe("Geyser.Label:onRightClick", function()
    -- a real right click is what normally calls this, with the event table
    -- Mudlet builds for a label callback, so the specs hand it that table
    local function mouseEvent(button, x, y)
      x, y = x or 5, y or 5
      return {button = button, buttons = {button}, x = x, y = y, globalX = x, globalY = y}
    end

    it("opens the menu where the click landed", function()
      label:onRightClick(mouseEvent("RightButton", 7, 9))
      assert.is_true(windowVisible(menuItem("First").name))
      -- the click coordinates are local to the label, and the menu is a child
      -- of it, so the menu lands that far into the label
      assert.are.equal(label:get_x() + 7, label.rightClickMenu:get_x())
      assert.are.equal(label:get_y() + 9, label.rightClickMenu:get_y())
    end)

    it("leaves the menu closed for any other button", function()
      label:onRightClick(mouseEvent("LeftButton"))
      assert.is_false(windowVisible(menuItem("First").name))
    end)

    it("flies the menu out to the left when there is no room on the right", function()
      local mainWidth = getMainWindowSize()
      label:move(mainWidth - 20, 10)
      label:onRightClick(mouseEvent("RightButton", 1, 1))
      assert.are.equal("L", menuItem("First").flyDir)
      -- and back out to the right once there is room again
      label:move(10, 10)
      label:onRightClick(mouseEvent("RightButton", 1, 1))
      assert.are.equal("R", menuItem("First").flyDir)
    end)

    -- unlike clicking a nestable label, which puts its nest away again, a
    -- second right click moves the menu to where the second click was: it
    -- closes every level before opening, so the nest it then opens was shut
    it("reopens the menu at the second click rather than putting it away", function()
      label:onRightClick(mouseEvent("RightButton", 3, 4))
      assert.is_true(windowVisible(menuItem("First").name))
      label:onRightClick(mouseEvent("RightButton", 8, 12))
      assert.is_true(windowVisible(menuItem("First").name))
      assert.are.equal(label:get_x() + 8, label.rightClickMenu:get_x())
      assert.are.equal(label:get_y() + 12, label.rightClickMenu:get_y())
    end)
  end)
end)
