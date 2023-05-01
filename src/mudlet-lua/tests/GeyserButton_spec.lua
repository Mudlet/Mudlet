describe("Tests functionality of Geyser.Button", function()
  describe('Tests the functionality of Geyser.Button:new', function()
    it('creates a button with certain defaults if called with no constraints', function()
      local gb = Geyser.Button:new()
      assert.equals("table", type(gb))
      assert.equals(50, gb.height)
      assert.equals(gb.height, gb.width)
      assert.is_truthy(gb.name:find("button"))
      assert.equals("look", gb.clickCommand)
      assert.equals("quicklook", gb.downCommand)
      assert.equals("dark_green", gb.downColor)
      assert.equals("blue",gb.color)
      assert.equals("<center>Look</center>", gb.msg)
      assert.equals("<center>Quicklook</center>", gb.downMsg)
      assert.equals("up", gb.state)
      assert.equals(5, gb.toolTipDuration)
      assert.equals("Click to look", gb.tooltip)
      assert.equals("Click to quicklook", gb.downTooltip)
      assert.is_false(gb.twoState)
      gb:hide()
    end)
  end)

  describe('Tests the functionality of Geyser.Button:press', function()
    local eaSpy
    local pcSpy
    local gb
    before_each(function()
      eaSpy = spy.on(_G, "expandAlias")
      pcSpy = spy.on(_G, "pcall")
      gb = Geyser.Button:new({
        name = "testButton",
      })
    end)
    after_each(function()
      expandAlias:revert()
      pcall:revert()
    end)

    it('uses expandAlias to send clickCommand in "up" state', function()
      gb:press()
      assert.spy(eaSpy).was.called(1)
      assert.spy(eaSpy).was.called_with("look")
    end)

    it('does not change the state if twoState is false', function()
      gb:press()
      assert.is_false(gb.twoState)
      assert.equals("up", gb.state)
    end)

    it('does change the state if twoState is true', function()
      gb:enableTwoState()
      gb:press()
      assert.equals("down", gb.state)
      gb:press()
      assert.equals("up", gb.state)
    end)

    it('calls expandAlias with the downCommand if twoState is enabled and state is "down"', function()
      gb:enableTwoState()
      gb:setState("down")
      gb:press()
      assert.spy(eaSpy).was.called(1)
      assert.spy(eaSpy).was.called_with(gb.downCommand)
    end)

    it('uses pcall with the clickFunction if set', function()
      local cf = function() end
      gb:setClickFunction(cf)
      gb:press()
      assert.spy(pcSpy).was.called(1)
      assert.spy(pcSpy).was.called_with(cf)
    end)

    it('uses pcall with the downFunction if set and state is "down"', function()
      local df = function() end
      gb:setDownFunction(df)
      gb:enableTwoState()
      gb:setState("down")
      gb:press()
      assert.spy(pcSpy).was.called(1)
      assert.spy(pcSpy).was.called_with(df)
    end)
  end)

  describe('Tests the functionality of Geyser.Button:setState', function()
    local gb
    local echoSpy, styleSheetSpy, colorSpy, toolTipSpy
    before_each(function()
      echoSpy = spy.on(Geyser.Label, "echo")
      styleSheetSpy = spy.on(Geyser.Label, "setStyleSheet")
      colorSpy = spy.on(Geyser.Label, "setColor")
      toolTipSpy = spy.on(Geyser.Label, "setToolTip")
      gb = Geyser.Button:new({
        name = "testButton",
        twoState = true
      })
    end)
    after_each(function()
      Geyser.Label.echo:revert()
      Geyser.Label.setStyleSheet:revert()
      Geyser.Label.setColor:revert()
      Geyser.Label.setToolTip:revert()
    end)

    it('should call echo with the message for the state set', function()
      gb:setState("up")
      assert.spy(echoSpy).was.called(1)
      assert.spy(echoSpy).was.called_with(gb.msg)
      gb:setState("down")
      assert.spy(echoSpy).was.called(2)
      assert.spy(echoSpy).was.called_with(gb.downMsg)
    end)

    it('should call setColor with the color for the state being set', function()
      gb:setState("up")
      assert.spy(colorSpy).was.called(1)
      assert.spy(colorSpy).was.called_with(gb.color)
      gb:setState("down")
      assert.spy(colorSpy).was.called(2)
      assert.spy(colorSpy).was.called_with(gb.downColor)
    end)

    it('should call setStyleSheet instead of setColor if the stylesheet is set', function()
      gb.downStyle = [[background-color: blue;]]
      gb.upStyle = [[background-color: black;]]
      gb:setState("up")
      assert.spy(colorSpy).was_not_called()
      assert.spy(styleSheetSpy).was.called(1)
      assert.spy(styleSheetSpy).was.called_with(gb.upStyle)
      gb:setState("down")
      assert.spy(colorSpy).was_not_called()
      assert.spy(styleSheetSpy).was.called(2)
      assert.spy(styleSheetSpy).was.called_with(gb.downStyle)
    end)

    it('should call setToolTip with the appropriate tool tip for the state being set', function()
      gb:setState("down")
      assert.spy(toolTipSpy).was.called(1)
      assert.spy(toolTipSpy).was.called_with(gb.tooltip, gb.toolTipDuration)
      gb:setState("up")
      assert.spy(toolTipSpy).was.called(2)
      assert.spy(toolTipSpy).was.called_with(gb.downTooltip, gb.toolTipDuration)
    end)
  end)
end)