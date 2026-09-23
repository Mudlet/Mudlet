-- A gauge is a container holding three labels: back (the full size backdrop),
-- front (the part that shrinks with the value) and text (the caption). Only
-- the front label changes geometry, so that is where setValue is measured.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.Gauge", function()
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

  describe("Geyser.Gauge:new/new2", function()
    it("builds a back, front and text label over the gauge's geometry", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsNew", x = 10, y = 20, width = 200, height = 40}))
      assert.are.equal("gauge", gauge.type)
      assert.are.equal(100, gauge.value)
      assert.are.same({"ggsNew_back", "ggsNew_front", "ggsNew_text"}, gauge.windows)
      for _, name in ipairs({"ggsNew_back", "ggsNew_front", "ggsNew_text"}) do
        assert.are.equal("label", windowType(name))
        assert.are.same({x = 10, y = 20, width = 200, height = 40}, geometry(name))
      end
      -- the gauge itself is a container, so it has no widget
      assert.is_nil(getWindowGeometry("ggsNew"))
    end)

    it("defaults to a horizontal, non strict gauge", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsDefaults", x = 0, y = 0, width = 100, height = 20}))
      assert.are.equal("horizontal", gauge.orientation)
      assert.is_false(gauge.strict)
    end)

    it("echoes the message constraint onto the text label", function()
      track(Geyser.Gauge:new({name = "ggsMessage", x = 0, y = 0, width = 100, height = 20, message = "50%"}))
      assert.is_truthy(getLabelText("ggsMessage_text"):find("50%%"))
    end)

    it("new2 marks the gauge as using add2", function()
      local gauge = track(Geyser.Gauge:new2({name = "ggsNew2", x = 0, y = 0, width = 100, height = 20}))
      assert.is_true(gauge.useAdd2)
      assert.are.equal("gauge", gauge.type)
    end)

    it("starts out hidden when the constraints ask for it, and shows again", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsHiddenNew", x = 0, y = 0, width = 100, height = 20, hidden = true}))
      assert.is_true(gauge.hidden)
      for _, name in ipairs({"ggsHiddenNew_back", "ggsHiddenNew_front", "ggsHiddenNew_text"}) do
        assert.is_false(windowVisible(name))
      end
      gauge:show()
      for _, name in ipairs({"ggsHiddenNew_back", "ggsHiddenNew_front", "ggsHiddenNew_text"}) do
        assert.is_true(windowVisible(name))
      end
    end)
  end)

  describe("Geyser.Gauge:setValue", function()
    local gauge

    before_each(function()
      gauge = track(Geyser.Gauge:new({name = "ggsValue", x = 0, y = 0, width = 200, height = 40}))
    end)

    it("sizes the front label to the percentage given", function()
      gauge:setValue(25)
      assert.are.equal(25, gauge.value)
      assert.are.same({x = 0, y = 0, width = 50, height = 40}, geometry("ggsValue_front"))
      gauge:setValue(75)
      assert.are.equal(150, geometry("ggsValue_front").width)
    end)

    it("treats a second argument as the maximum", function()
      gauge:setValue(50, 200)
      assert.are.equal(25, gauge.value)
      assert.are.equal(50, geometry("ggsValue_front").width)
    end)

    it("leaves the back label at full size", function()
      gauge:setValue(10)
      assert.are.same({x = 0, y = 0, width = 200, height = 40}, geometry("ggsValue_back"))
      assert.are.same({x = 0, y = 0, width = 200, height = 40}, geometry("ggsValue_text"))
    end)

    it("clamps a negative value to empty", function()
      gauge:setValue(-20)
      assert.are.equal(0, gauge.value)
      assert.are.equal(0, geometry("ggsValue_front").width)
    end)

    it("lets the front overflow past the back unless the gauge is strict", function()
      gauge:setValue(150)
      assert.are.equal(150, gauge.value)
      assert.are.equal(300, geometry("ggsValue_front").width)
    end)

    it("caps a strict gauge at its own width", function()
      local strict = track(Geyser.Gauge:new({name = "ggsStrict", x = 0, y = 0, width = 200, height = 40, strict = true}))
      strict:setValue(150)
      assert.are.equal(100, strict.value)
      assert.are.equal(200, geometry("ggsStrict_front").width)
    end)

    it("writes the optional third argument onto the text label", function()
      gauge:setValue(40, 100, "40 of 100")
      assert.is_truthy(getLabelText("ggsValue_text"):find("40 of 100", 1, true))
    end)

    it("rejects a value that is not a number", function()
      local ok, message = pcall(function() gauge:setValue("x") end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("currentValue as number expected, got string", 1, true))
    end)

    it("rejects a maximum that is not a number", function()
      local ok, message = pcall(function() gauge:setValue(5, "y") end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("maxValue as number expected, got string", 1, true))
    end)

    it("refuses a maximum that is not positive instead of going infinite", function()
      gauge:setValue(25)
      -- a zero maximum used to leave value at inf, a negative one at a negative
      -- value, and both stuck until the next good call
      for _, bad in ipairs({0, -10, 0 / 0}) do
        local result, message = gauge:setValue(5, bad)
        assert.is_nil(result)
        assert.is_not_nil(message)
        assert.is_truthy(message:find("maxValue must be a positive number", 1, true))
      end
      -- refusing is not fatal, and leaves the gauge on the value it already had
      assert.are.equal(25, gauge.value)
      assert.are.equal(50, geometry("ggsValue_front").width)
      -- a good reading has to be distinguishable from those refusals
      assert.is_true(gauge:setValue(50, 100))
    end)

    -- a gauge only lays its front label out again when something about the fill
    -- moved, so that a UI repainting every gauge on every prompt costs nothing
    -- for the ones sitting still
    it("lays the front label out once per update and not at all for a repeat", function()
      gauge:setValue(50)
      local move = spy.on(_G, "moveWindow")
      local resize = spy.on(_G, "resizeWindow")
      finally(function() move:revert() resize:revert() end)
      gauge:setValue(60)
      assert.spy(move).was.called(1)
      assert.spy(resize).was.called(1)
      gauge:setValue(60)
      gauge:setValue(60)
      assert.spy(move).was.called(1)
      assert.spy(resize).was.called(1)
      -- and the skipped repeats left it where the update put it
      assert.are.equal(120, geometry("ggsValue_front").width)
    end)

    -- the geometry is the gauge's answer, not something a later event-loop turn
    -- makes true, so a script may read it straight back
    it("has the new geometry ready by the time it returns", function()
      for _, value in ipairs({10, 20, 30, 40}) do
        gauge:setValue(value)
        assert.are.equal(value * 2, geometry("ggsValue_front").width)
      end
    end)

    -- the front label's constraints are only rebuilt when they need to be, so a
    -- gauge whose front label was resized out from under it has to notice
    it("takes the front label back after something else resized it", function()
      gauge:setValue(50)
      gauge.front:resize(30, 10)
      assert.are.same({x = 0, y = 0, width = 30, height = 10}, geometry("ggsValue_front"))
      gauge:setValue(50)
      assert.are.same({x = 0, y = 0, width = 100, height = 40}, geometry("ggsValue_front"))
      gauge.front:move(7, 9)
      gauge:setValue(60)
      assert.are.same({x = 0, y = 0, width = 120, height = 40}, geometry("ggsValue_front"))
    end)

    it("keeps a format change made between two identical updates", function()
      gauge:setValue(50, 100, "HP 50")
      gauge:setBold(true)
      gauge:setFontSize(18)
      gauge:setValue(50, 100, "HP 50")
      local shown = getLabelText("ggsValue_text")
      assert.is_truthy(shown:find("<b>HP 50</b>", 1, true))
      assert.is_truthy(shown:find("font%-size: 18pt"))
    end)

    it("puts its own text back after something else wrote to the label", function()
      gauge:setValue(50, 100, "HP 50")
      gauge.text:rawEcho("something else")
      gauge:setValue(50, 100, "HP 50")
      assert.is_truthy(getLabelText("ggsValue_text"):find("HP 50", 1, true))
    end)

    it("stays full over repeated overflow and comes back down", function()
      local strict = track(Geyser.Gauge:new({name = "ggsStrictRepeat", x = 0, y = 0, width = 200, height = 40, strict = true}))
      strict:setValue(150)
      strict:setValue(300)
      assert.are.equal(200, geometry("ggsStrictRepeat_front").width)
      strict:setValue(50)
      assert.are.equal(100, geometry("ggsStrictRepeat_front").width)
    end)

    it("follows a resize that happens between two identical updates", function()
      gauge:setValue(50)
      assert.are.equal(100, geometry("ggsValue_front").width)
      gauge:resize(100, 40)
      gauge:setValue(50)
      assert.are.equal(50, geometry("ggsValue_front").width)
    end)

    it("follows a stylesheet that changes between two identical updates", function()
      gauge:setValue(50)
      gauge:setStyleSheet("margin: 10px;", "margin: 10px;")
      gauge:setValue(50)
      assert.are.same({x = 10, y = 10, width = 90, height = 20}, geometry("ggsValue_front"))
    end)

    it("keeps one gauge's spacing out of another's", function()
      local plain = track(Geyser.Gauge:new({name = "ggsPlain", x = 0, y = 0, width = 200, height = 40}))
      local inset = track(Geyser.Gauge:new({name = "ggsInset", x = 0, y = 0, width = 200, height = 40}))
      inset:setStyleSheet("padding: 5px;", "padding: 5px;")
      for _ = 1, 3 do
        plain:setValue(50)
        inset:setValue(50)
      end
      assert.are.same({x = 0, y = 0, width = 100, height = 40}, geometry("ggsPlain_front"))
      assert.are.same({x = 5, y = 5, width = 95, height = 30}, geometry("ggsInset_front"))
    end)

    it("updates a gauge that is hidden, so showing it again is right", function()
      gauge:hide()
      gauge:setValue(25)
      gauge:show()
      assert.are.same({x = 0, y = 0, width = 50, height = 40}, geometry("ggsValue_front"))
      assert.is_true(windowVisible("ggsValue_front"))
    end)

    -- the front label's constraints outlive the update that installed them, so
    -- everything that lays a gauge out without going through setValue has to
    -- keep working: the container cascade, and the reposition every window
    -- resize runs
    it("follows its container being moved, resized and repositioned", function()
      local box = track(Geyser.Container:new({name = "ggsBoxMove", x = 100, y = 100, width = 400, height = 100}))
      local gauge = track(Geyser.Gauge:new({name = "ggsInMove", x = 0, y = 0, width = "50%", height = "100%"}, box))
      gauge:setValue(50)
      assert.are.same({x = 100, y = 100, width = 100, height = 100}, geometry("ggsInMove_front"))
      box:move(300, 200)
      box:resize(200, 50)
      assert.are.same({x = 300, y = 200, width = 50, height = 50}, geometry("ggsInMove_front"))
      -- the value it already sits on must not undo that
      gauge:setValue(50)
      assert.are.same({x = 300, y = 200, width = 50, height = 50}, geometry("ggsInMove_front"))
      box:reposition()
      assert.are.same({x = 300, y = 200, width = 50, height = 50}, geometry("ggsInMove_front"))
    end)

    it("lays a whole row of gauges out in one go", function()
      local row = {}
      for i = 1, 5 do
        row[i] = track(Geyser.Gauge:new({name = "ggsRow" .. i, x = 0, y = 0, width = 200, height = 40}))
      end
      for i = 1, 5 do
        row[i]:setValue(i * 10, 100, "row " .. i)
      end
      for i = 1, 5 do
        assert.are.equal(i * 20, geometry("ggsRow" .. i .. "_front").width)
        assert.is_truthy(getLabelText("ggsRow" .. i .. "_text"):find("row " .. i, 1, true))
      end
    end)

    -- each orientation wires the offsets into its own branch, so a gauge with
    -- back spacing has to come out right in every one of them and on the way back
    it("changes orientation on the next update, spacing and all", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsTurn", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 5px;", "margin: 5px;")
      gauge:setValue(25)
      assert.are.same({x = 5, y = 5, width = 48, height = 90}, geometry("ggsTurn_front"))
      gauge.orientation = "vertical"
      gauge:setValue(25)
      assert.are.same({x = 5, y = 73, width = 190, height = 23}, geometry("ggsTurn_front"))
      gauge.orientation = "goofy"
      gauge:setValue(25)
      assert.are.same({x = 148, y = 5, width = 48, height = 90}, geometry("ggsTurn_front"))
      gauge.orientation = "batty"
      gauge:setValue(25)
      assert.are.same({x = 5, y = 5, width = 190, height = 23}, geometry("ggsTurn_front"))
      gauge.orientation = "horizontal"
      gauge:setValue(25)
      assert.are.same({x = 5, y = 5, width = 48, height = 90}, geometry("ggsTurn_front"))
    end)

    -- pixel offsets against a container-relative size are where a memoised
    -- spacing would drift, because the size is a fraction and the offsets are not
    it("combines a percentage size with a pixel spacing", function()
      local box = track(Geyser.Container:new({name = "ggsPctBox", x = 0, y = 0, width = 400, height = 100}))
      local gauge = track(Geyser.Gauge:new({name = "ggsPct", x = 0, y = 0, width = "33%", height = "100%"}, box))
      gauge:setStyleSheet("margin: 3px;", "margin: 3px;")
      gauge:setValue(50)
      assert.are.same({x = 3, y = 3, width = 63, height = 94}, geometry("ggsPct_front"))
      box:resize(800, 100)
      assert.are.same({x = 3, y = 3, width = 129, height = 94}, geometry("ggsPct_front"))
    end)
  end)

  describe("Geyser.Gauge orientations", function()
    it("fills a vertical gauge from the bottom", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsVertical", x = 0, y = 0, width = 100, height = 200, orientation = "vertical"}))
      gauge:setValue(25)
      assert.are.same({x = 0, y = 150, width = 100, height = 50}, geometry("ggsVertical_front"))
      assert.are.same({x = 0, y = 0, width = 100, height = 200}, geometry("ggsVertical_back"))
    end)

    it("fills a goofy gauge from the right", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsGoofy", x = 0, y = 0, width = 200, height = 40, orientation = "goofy"}))
      gauge:setValue(25)
      assert.are.same({x = 150, y = 0, width = 50, height = 40}, geometry("ggsGoofy_front"))
    end)

    it("fills a batty gauge from the top", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsBatty", x = 0, y = 0, width = 100, height = 200, orientation = "batty"}))
      gauge:setValue(25)
      assert.are.same({x = 0, y = 0, width = 100, height = 50}, geometry("ggsBatty_front"))
    end)
  end)

  describe("Geyser.Gauge:setStyleSheet", function()
    it("keeps the front label inside the back label's margins", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsMargin", x = 0, y = 0, width = 200, height = 40}))
      gauge:setStyleSheet("margin: 5px; background-color: red;", "margin: 5px; background-color: blue;")
      assert.are.same({x = 0, y = 0, width = 200, height = 40}, geometry("ggsMargin_back"))
      assert.are.same({x = 5, y = 5, width = 190, height = 30}, geometry("ggsMargin_front"))
    end)

    it("reads a two value margin as vertical then horizontal", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsTwoValue", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 10px 30px;", "margin: 10px 30px;")
      assert.are.same({x = 30, y = 10, width = 140, height = 80}, geometry("ggsTwoValue_front"))
    end)

    it("reads a three value margin as top, horizontal, bottom", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsThreeValue", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 10px 20px 30px;", "margin: 10px 20px 30px;")
      assert.are.same({x = 20, y = 10, width = 160, height = 60}, geometry("ggsThreeValue_front"))
    end)

    it("reads a three value padding the same way as a margin", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsThreePadding", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("padding: 4px 6px 8px;", "padding: 4px 6px 8px;")
      assert.are.same({x = 6, y = 4, width = 188, height = 88}, geometry("ggsThreePadding_front"))
    end)

    -- A unitless zero is the ordinary way to write "no margin on this axis".
    -- Counting only the px tokens dropped it, so what is left reads as a
    -- shorter shorthand and every component shifts.
    it("counts a unitless zero as a margin value", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsZero", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 0 10px;", "margin: 0 10px;")
      assert.are.same({x = 10, y = 0, width = 180, height = 100}, geometry("ggsZero_front"))
    end)

    it("counts a unitless zero in a four value margin", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsZeroFour", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 10px 0 10px 0;", "margin: 10px 0 10px 0;")
      assert.are.same({x = 0, y = 10, width = 200, height = 80}, geometry("ggsZeroFour_front"))
    end)

    it("keeps the sign of a negative margin", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsNegative", x = 20, y = 20, width = 200, height = 100}))
      gauge:setStyleSheet("margin: -5px;", "margin: -5px;")
      assert.are.same({x = 15, y = 15, width = 210, height = 110}, geometry("ggsNegative_front"))
    end)

    it("reads a margin longhand", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsLonghand", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin-left: 10px;", "margin-left: 10px;")
      assert.are.same({x = 10, y = 0, width = 190, height = 100}, geometry("ggsLonghand_front"))
    end)

    it("lets a margin longhand override the shorthand it follows", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsOverride", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 5px; margin-left: 20px;", "margin: 5px; margin-left: 20px;")
      assert.are.same({x = 20, y = 5, width = 175, height = 90}, geometry("ggsOverride_front"))
    end)

    it("reads a border-width longhand", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsBorderWidth", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("border-width: 2px;", "border-width: 2px;")
      assert.are.same({x = 2, y = 2, width = 196, height = 96}, geometry("ggsBorderWidth_front"))
    end)

    it("reads an upper case px unit", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsUpperCase", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 10PX;", "margin: 10PX;")
      assert.are.same({x = 10, y = 10, width = 180, height = 80}, geometry("ggsUpperCase_front"))
    end)

    -- em and % cannot be turned into pixels here, so the whole declaration is
    -- left alone rather than half of it being read as zero
    it("leaves a margin it cannot measure in pixels alone", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsEm", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 1em;", "margin: 1em;")
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("ggsEm_front"))
      gauge:setStyleSheet("margin: 5% 10px;", "margin: 5% 10px;")
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("ggsEm_front"))
    end)

    -- qproperty-margin is a Qt property, not a margin, and used to be picked up
    -- by the unanchored property pattern - both when working out the offset and
    -- when stripping margins off the front label, where it left "qproperty-"
    -- behind and Qt threw the whole sheet out
    it("does not read qproperty-margin as a margin", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsQProperty", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("qproperty-margin: 5px; color: red;", "qproperty-margin: 5px;")
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("ggsQProperty_front"))
      assert.is_truthy(getLabelStyleSheet("ggsQProperty_front"):find("qproperty-margin: 5px;", 1, true))
    end)

    -- Qt takes !important on a declaration; it marks priority and is not one of
    -- the box's sides
    it("reads a margin that carries !important", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsImportant", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 10px !important;", "margin: 10px !important;")
      assert.are.same({x = 10, y = 10, width = 180, height = 80}, geometry("ggsImportant_front"))
      gauge:setStyleSheet("margin-left: 10px !important;", "margin-left: 10px !important;")
      assert.are.same({x = 10, y = 0, width = 190, height = 100}, geometry("ggsImportant_front"))
    end)

    -- a declaration written inside a selector block usually carries no
    -- semicolon, and the closing brace is neither part of the value nor
    -- something the front label's margin strip may swallow
    it("reads a margin inside a selector block without wrecking the sheet", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsBlock", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("QLabel { background-color: red; margin: 4px }", "QLabel { background-color: blue; margin: 4px }")
      assert.are.same({x = 4, y = 4, width = 192, height = 92}, geometry("ggsBlock_front"))
      local front = getLabelStyleSheet("ggsBlock_front")
      assert.is_nil(front:find("margin", 1, true))
      assert.is_truthy(front:find("background-color: red;", 1, true))
      assert.is_truthy(front:find("}", 1, true))
    end)

    it("does not read a commented out margin, and leaves the comment whole", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsComment", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("background-color: red; /* margin: 20px */ padding: 3px;", "background-color: blue; /* margin: 20px */ padding: 3px;")
      assert.are.same({x = 3, y = 3, width = 194, height = 94}, geometry("ggsComment_front"))
      local front = getLabelStyleSheet("ggsComment_front")
      assert.is_truthy(front:find("background-color: red;", 1, true))
      assert.is_nil(front:find("20px", 1, true))
      -- whatever is left of the comment, it must still be closed
      assert.are.equal(select(2, front:gsub("/%*", "")), select(2, front:gsub("%*/", "")))
    end)

    it("reads a length written without a leading digit", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsLeadingDot", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: .5px;", "margin: .5px;")
      -- .5px used to match the "5px" inside it and inset the gauge tenfold; half
      -- a pixel each side comes off the size and rounds away on the position
      assert.are.same({x = 0, y = 0, width = 199, height = 99}, geometry("ggsLeadingDot_front"))
    end)

    it("reads border longhands", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsBorderLong", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("border-top: 3px solid red;", "border-top: 3px solid red;")
      assert.are.same({x = 0, y = 3, width = 200, height = 97}, geometry("ggsBorderLong_front"))
      gauge:setStyleSheet("border-left-width: 4px;", "border-left-width: 4px;")
      assert.are.same({x = 4, y = 0, width = 196, height = 100}, geometry("ggsBorderLong_front"))
      gauge:setStyleSheet("border-width: 1px 2px 3px 4px;", "border-width: 1px 2px 3px 4px;")
      assert.are.same({x = 4, y = 1, width = 194, height = 96}, geometry("ggsBorderLong_front"))
    end)

    it("reads a padding longhand", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsPaddingLong", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("padding-bottom: 7px;", "padding-bottom: 7px;")
      assert.are.same({x = 0, y = 0, width = 200, height = 93}, geometry("ggsPaddingLong_front"))
    end)

    -- the offsets reach the front label through a different branch per
    -- orientation, and a negative one has to stay negative in each
    it("keeps a negative margin in every orientation", function()
      local expected = {
        vertical = {x = 15, y = 15, width = 210, height = 110},
        goofy = {x = 15, y = 15, width = 210, height = 110},
        batty = {x = 15, y = 15, width = 210, height = 110},
      }
      for orientation, geometryWanted in pairs(expected) do
        local name = "ggsNegative" .. orientation
        local gauge = track(Geyser.Gauge:new({name = name, x = 20, y = 20, width = 200, height = 100, orientation = orientation}))
        gauge:setStyleSheet("margin: -5px;", "margin: -5px;")
        gauge:setValue(100)
        assert.are.same(geometryWanted, geometry(name .. "_front"), orientation .. " gauge")
      end
    end)

    -- Qt applies a spacing Geyser cannot measure, so the fill bar is laid out
    -- against the wrong box: that has to be said rather than left looking like
    -- a Geyser bug
    it("says so when it cannot measure a spacing in pixels", function()
      local debugMessage = spy.on(_G, "debugc")
      finally(function() debugMessage:revert() end)
      local gauge = track(Geyser.Gauge:new({name = "ggsUnreadable", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 1em;", "margin: 1em;")
      assert.spy(debugMessage).was.called()
      local said = debugMessage.calls[#debugMessage.calls].vals[1]
      assert.is_truthy(said:find("ggsUnreadable", 1, true))
      assert.is_truthy(said:find("margin: 1em", 1, true))
      -- and it is latched, so a gauge updated every prompt does not flood
      local saidOnce = #debugMessage.calls
      gauge:setValue(50)
      gauge:setValue(75)
      assert.are.equal(saidOnce, #debugMessage.calls)
    end)

    it("says nothing about an ordinary borderless stylesheet", function()
      local debugMessage = spy.on(_G, "debugc")
      finally(function() debugMessage:revert() end)
      local gauge = track(Geyser.Gauge:new({name = "ggsQuiet", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("border: none; background-color: red; margin: 0;", "border: none; background-color: blue; margin: 0;")
      assert.spy(debugMessage).was_not.called()
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("ggsQuiet_front"))
    end)

    it("reads a four value margin as top, right, bottom, left", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsFourValue", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("margin: 1px 2px 3px 4px;", "margin: 1px 2px 3px 4px;")
      assert.are.same({x = 4, y = 1, width = 194, height = 96}, geometry("ggsFourValue_front"))
    end)

    it("makes room for a border on the back label", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsBorder", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("border: 2px solid red;", "border: 2px solid red;")
      assert.are.same({x = 2, y = 2, width = 196, height = 96}, geometry("ggsBorder_front"))
    end)

    it("makes room for padding on the back label", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsPadding", x = 0, y = 0, width = 200, height = 100}))
      gauge:setStyleSheet("padding: 3px;", "padding: 3px;")
      assert.are.same({x = 3, y = 3, width = 194, height = 94}, geometry("ggsPadding_front"))
    end)

    it("adds margin, border and padding together", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsCombined", x = 0, y = 0, width = 200, height = 100}))
      local css = "margin: 2px; border: 1px solid red; padding: 3px;"
      gauge:setStyleSheet(css, css)
      assert.are.same({x = 6, y = 6, width = 188, height = 88}, geometry("ggsCombined_front"))
    end)

    it("strips the margin from the front stylesheet but not the back", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsCss", x = 0, y = 0, width = 200, height = 40}))
      gauge:setStyleSheet("margin: 5px; background-color: red;", "margin: 5px; background-color: blue;")
      assert.is_nil(getLabelStyleSheet("ggsCss_front"):find("margin", 1, true))
      assert.is_truthy(getLabelStyleSheet("ggsCss_front"):find("background-color: red;", 1, true))
      assert.is_truthy(getLabelStyleSheet("ggsCss_back"):find("margin: 5px;", 1, true))
      assert.are.equal("margin: 5px; background-color: blue;", gauge.backCSS)
    end)

    -- the last declaration in a stylesheet carries no semicolon, and a margin
    -- left on the front label is applied on top of the offset computed from the
    -- back label, doubling it
    it("strips a margin that carries no trailing semicolon", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsNoSemicolon", x = 0, y = 0, width = 200, height = 40}))
      gauge:setStyleSheet("background-color: red; margin: 5px", "margin: 5px;")
      assert.is_nil(getLabelStyleSheet("ggsNoSemicolon_front"):find("margin", 1, true))
      assert.are.same({x = 5, y = 5, width = 190, height = 30}, geometry("ggsNoSemicolon_front"))
    end)

    it("uses the front stylesheet for the back when only one is given", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsOneCss", x = 0, y = 0, width = 100, height = 20}))
      gauge:setStyleSheet("background-color: green;")
      assert.are.equal("background-color: green;", gauge.backCSS)
      assert.are.equal("background-color: green;", getLabelStyleSheet("ggsOneCss_back"))
    end)

    it("applies a text stylesheet when one is given", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsTextCss", x = 0, y = 0, width = 100, height = 20}))
      gauge:setStyleSheet("background-color: green;", nil, "color: white;")
      assert.are.equal("color: white;", getLabelStyleSheet("ggsTextCss_text"))
    end)

    it("keeps the current value when the stylesheet changes", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsCssValue", x = 0, y = 0, width = 200, height = 40}))
      gauge:setValue(50)
      gauge:setStyleSheet("background-color: red;")
      assert.are.equal(50, gauge.value)
      assert.are.equal(100, geometry("ggsCssValue_front").width)
    end)
  end)

  describe("Geyser.Gauge text", function()
    local gauge

    before_each(function()
      gauge = track(Geyser.Gauge:new({name = "ggsText", x = 0, y = 0, width = 200, height = 40}))
    end)

    it("writes setText onto the text label", function()
      gauge:setText("hello gauge")
      assert.is_truthy(getLabelText("ggsText_text"):find("hello gauge", 1, true))
    end)

    it("echoes with a colour", function()
      gauge:echo("colored", "red")
      local text = getLabelText("ggsText_text")
      assert.is_truthy(text:find("colored", 1, true))
      assert.is_truthy(text:find("color: #ff0000", 1, true))
    end)

    it("mirrors the text label's format state back onto the gauge", function()
      gauge:setBold(true)
      gauge:setItalics(true)
      gauge:setUnderline(true)
      gauge:setStrikethrough(true)
      gauge:setText("styled")
      local text = getLabelText("ggsText_text")
      assert.is_truthy(text:find("<b>", 1, true))
      assert.is_truthy(text:find("<i>", 1, true))
      assert.is_truthy(text:find("<u>", 1, true))
      assert.is_truthy(text:find("<s>", 1, true))
      assert.are.equal("8bius", gauge.format)
      assert.is_true(gauge.formatTable.bold)
      assert.is_true(gauge.formatTable.strikethrough)
    end)

    it("sets the font size of the text label", function()
      gauge:setFontSize(18)
      gauge:setText("bigger")
      assert.is_truthy(getLabelText("ggsText_text"):find("font%-size: 18pt"))
    end)

    it("aligns the text label", function()
      gauge:setAlignment("center")
      gauge:setText("middle")
      assert.is_truthy(getLabelText("ggsText_text"):find('align="center"', 1, true))
    end)

    it("sets the text colour", function()
      gauge:setFgColor("#00ff00")
      gauge:setText("green")
      assert.is_truthy(getLabelText("ggsText_text"):find("color: #00ff00", 1, true))
    end)

    it("setFormat replaces the whole text format at once", function()
      gauge:setBold(true)
      gauge:setFormat("ci18")
      gauge:setText("reformatted")
      local text = getLabelText("ggsText_text")
      assert.is_truthy(text:find('align="center"', 1, true))
      assert.is_truthy(text:find("<i>reformatted</i>", 1, true))
      assert.is_truthy(text:find("font%-size: 18pt"))
      assert.is_nil(text:find("<b>", 1, true))
      -- the gauge mirrors the text label's format state, so both copies move
      assert.are.equal("ci18", gauge.format)
      assert.is_false(gauge.formatTable.bold)
      assert.is_true(gauge.formatTable.italics)
    end)
  end)

  -- setColor paints the fill bar at full alpha and the backdrop in the same
  -- colour at a fixed alpha of 100, which is what makes the unfilled part read
  -- as the same gauge rather than as a hole.
  describe("Geyser.Gauge:setColor", function()
    local function backgroundColor(name)
      local red, green, blue, alpha = getBackgroundColor(name)
      return {red, green, blue, alpha}
    end

    it("paints the front at full alpha and the back at a fraction of it", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsColour", x = 0, y = 0, width = 200, height = 40}))
      gauge:setColor(10, 20, 30)
      assert.are.same({10, 20, 30, 255}, backgroundColor("ggsColour_front"))
      assert.are.same({10, 20, 30, 100}, backgroundColor("ggsColour_back"))
      -- the caption label stays transparent so the fill shows through it
      assert.are.equal(0, select(4, getBackgroundColor("ggsColour_text")))
    end)

    it("takes a colour name and a hex code", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsColourName", x = 0, y = 0, width = 200, height = 40}))
      gauge:setColor("green")
      assert.are.same({0, 255, 0, 255}, backgroundColor("ggsColourName_front"))
      gauge:setColor("#0000ff")
      assert.are.same({0, 0, 255, 255}, backgroundColor("ggsColourName_front"))
    end)

    it("writes the optional fourth argument onto the text label", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsColourText", x = 0, y = 0, width = 200, height = 40}))
      gauge:setColor("red", nil, nil, "HP")
      assert.is_truthy(getLabelText("ggsColourText_text"):find("HP", 1, true))
    end)
  end)

  describe("Geyser.Gauge geometry and visibility", function()
    it("moves and resizes all three labels", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsMove", x = 0, y = 0, width = 200, height = 40}))
      gauge:setValue(50)
      gauge:move(60, 70)
      gauge:resize(100, 20)
      assert.are.same({x = 60, y = 70, width = 100, height = 20}, geometry("ggsMove_back"))
      assert.are.same({x = 60, y = 70, width = 100, height = 20}, geometry("ggsMove_text"))
      -- the front label keeps its share of the new size
      assert.are.same({x = 60, y = 70, width = 50, height = 20}, geometry("ggsMove_front"))
    end)

    it("hides and shows all three labels", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsHide", x = 0, y = 0, width = 100, height = 20}))
      gauge:hide()
      for _, name in ipairs({"ggsHide_back", "ggsHide_front", "ggsHide_text"}) do
        assert.is_false(windowVisible(name))
      end
      gauge:show()
      for _, name in ipairs({"ggsHide_back", "ggsHide_front", "ggsHide_text"}) do
        assert.is_true(windowVisible(name))
      end
    end)

    it("sizes a percentage gauge against its container", function()
      local container = track(Geyser.Container:new({name = "ggsBox", x = 100, y = 100, width = 400, height = 100}))
      local gauge = track(Geyser.Gauge:new({name = "ggsInBox", x = 0, y = 0, width = "50%", height = "100%"}, container))
      gauge:setValue(50)
      assert.are.same({x = 100, y = 100, width = 200, height = 100}, geometry("ggsInBox_back"))
      assert.are.same({x = 100, y = 100, width = 100, height = 100}, geometry("ggsInBox_front"))
    end)
  end)

  describe("Geyser.Gauge:type_delete", function()
    it("deletes the back, front and text labels with the gauge", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsDelete", x = 0, y = 0, width = 100, height = 20}))
      gauge:delete()
      for _, name in ipairs({"ggsDelete_back", "ggsDelete_front", "ggsDelete_text"}) do
        assert.is_nil(getWindowGeometry(name))
      end
      assert.is_nil(Geyser.windowList.ggsDelete)
    end)
  end)

  describe("Geyser.Gauge clickthrough and tooltip", function()
    it("enableClickthrough and disableClickthrough reach all three labels", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsClick", x = 0, y = 0, width = 100, height = 20}))
      -- there is no getter for the clickthrough flag, so the delegation to the
      -- three labels is what can be checked
      -- finally() only holds one function, so both spies are reverted from the
      -- same one: a second call would drop the first, leaving a spy on a Mudlet
      -- global that every later spy.on picks up as the real function
      local enable = spy.on(_G, "enableClickthrough")
      local disable = spy.on(_G, "disableClickthrough")
      finally(function() enable:revert() disable:revert() end)

      gauge:enableClickthrough()
      assert.spy(enable).was.called(3)
      assert.spy(enable).was.called_with("ggsClick_front")
      assert.spy(enable).was.called_with("ggsClick_back")
      assert.spy(enable).was.called_with("ggsClick_text")

      gauge:disableClickthrough()
      assert.spy(disable).was.called(3)
      assert.spy(disable).was.called_with("ggsClick_text")
    end)

    it("setToolTip and resetToolTip go to the text label and are remembered", function()
      local gauge = track(Geyser.Gauge:new({name = "ggsToolTip", x = 0, y = 0, width = 100, height = 20}))
      gauge:setToolTip("how much is left", 5)
      assert.are.equal("how much is left", gauge.text.toolTip)
      assert.are.equal(5, gauge.text.toolTipDuration)
      gauge:resetToolTip()
      assert.is_nil(gauge.text.toolTip)
    end)
  end)
end)
