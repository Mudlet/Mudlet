describe("Tests functionality of Geyser.Button", function()
  describe('Tests the functionality of Geyser.Button:new', function()
    it('creates a button with certain defaults if called with no constraints', function()
      local gb = Geyser.Button:new()
      assert.equals("table", type(gb))
      assert.equals("50px", gb.height)
      assert.equals(gb.height, gb.width)
      assert.is_truthy(gb.name:find("button"))
      assert.equals("look", gb.clickCommand)
      assert.equals("look", gb.downCommand)
      assert.equals("blue", gb.downColor)
      assert.equals("blue",gb.color)
      assert.equals("<center>Look</center>", gb.msg)
      assert.equals("<center>Look</center>", gb.downMsg)
      assert.equals("up", gb.state)
      assert.equals(5, gb.toolTipDuration)
      assert.equals("Click to look", gb.tooltip)
      assert.equals("Click to look", gb.downTooltip)
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
      assert.spy(echoSpy).was.called()
      assert.spy(echoSpy).was.called_with(match.is_ref(gb), gb.msg)
      gb:setState("down")
      assert.spy(echoSpy).was.called()
      assert.spy(echoSpy).was.called_with(match.is_ref(gb), gb.downMsg)
    end)

    it('should call setColor with the color for the state being set', function()
      gb:setState("up")
      assert.spy(colorSpy).was.called()
      assert.spy(colorSpy).was.called_with(match.is_ref(gb), gb.color)
      gb:setState("down")
      assert.spy(colorSpy).was.called()
      assert.spy(colorSpy).was.called_with(match.is_ref(gb), gb.downColor)
    end)

    it('should call setStyleSheet instead of setColor if the stylesheet is set', function()
      gb.downStyle = [[background-color: blue;]]
      gb.style = [[background-color: black;]]
      gb:setState("up")
      assert.spy(styleSheetSpy).was.called()
      assert.spy(styleSheetSpy).was.called_with(match.is_ref(gb), gb.style)
      gb:setState("down")
      assert.spy(styleSheetSpy).was.called()
      assert.spy(styleSheetSpy).was.called_with(match.is_ref(gb), gb.downStyle)
    end)

    it('should call setToolTip with the appropriate tool tip for the state being set', function()
      gb:setState("down")
      assert.spy(toolTipSpy).was.called()
      assert.spy(toolTipSpy).was.called_with(match.is_ref(gb), gb.tooltip, gb.toolTipDuration)
      gb:setState("up")
      assert.spy(toolTipSpy).was.called()
      assert.spy(toolTipSpy).was.called_with(match.is_ref(gb), gb.downTooltip, gb.toolTipDuration)
    end)
  end)

  -- The blocks above watch the calls a button makes; these assert what the
  -- widget ends up looking like, through getWindowGeometry and getLabelText.
  describe('Geyser.Button widget state', function()
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

    it('creates a label widget at the constrained geometry', function()
      track(Geyser.Button:new({name = "gbsGeometry", x = 5, y = 6, width = 70, height = 30}))
      assert.are.equal("label", windowType("gbsGeometry"))
      assert.are.same({x = 5, y = 6, width = 70, height = 30}, geometry("gbsGeometry"))
      assert.is_true(windowVisible("gbsGeometry"))
    end)

    it('falls back to the button default size, not the label one', function()
      track(Geyser.Button:new({name = "gbsDefaultSize", x = 0, y = 0}))
      local actual = geometry("gbsDefaultSize")
      assert.are.equal(50, actual.width)
      assert.are.equal(50, actual.height)
    end)

    it('shows the up message on the label to start with', function()
      track(Geyser.Button:new({name = "gbsUpMessage", x = 0, y = 0, width = 60, height = 20, msg = "press me"}))
      assert.is_truthy(getLabelText("gbsUpMessage"):find("press me", 1, true))
    end)

    it('swaps the label text with the state of a two state button', function()
      local button = track(Geyser.Button:new({
        name = "gbsTwoState",
        x = 0, y = 0, width = 60, height = 20,
        msg = "up text",
        downMsg = "down text",
        twoState = true,
      }))
      assert.is_truthy(getLabelText("gbsTwoState"):find("up text", 1, true))
      button:setState("down")
      assert.is_truthy(getLabelText("gbsTwoState"):find("down text", 1, true))
      button:setState("up")
      assert.is_truthy(getLabelText("gbsTwoState"):find("up text", 1, true))
    end)

    it('refuses to push a single state button down', function()
      local button = track(Geyser.Button:new({
        name = "gbsSingleState",
        x = 0, y = 0, width = 60, height = 20,
        msg = "up text",
        downMsg = "down text",
      }))
      local result, message = button:setState("down")
      assert.is_nil(result)
      assert.are.equal("cannot set a single state button's state to 'down', only 'up'", message)
      -- the refusal happens before anything is written, so neither the stored
      -- state nor the drawn message move
      assert.are.equal("up", button.state)
      assert.is_truthy(getLabelText("gbsSingleState"):find("up text", 1, true))
    end)

    it('keeps clicking a refused single state button on its up command', function()
      local clicks, downs = 0, 0
      local button = track(Geyser.Button:new({
        name = "gbsRefusedPress",
        x = 0, y = 0, width = 60, height = 20,
        clickFunction = function() clicks = clicks + 1 end,
        downFunction = function() downs = downs + 1 end,
      }))
      button:setState("down")
      button:press()
      assert.are.equal(1, clicks)
      assert.are.equal(0, downs)
      assert.are.equal("up", button.state)
    end)

    it('reports success for both states of a two state button', function()
      local button = track(Geyser.Button:new({
        name = "gbsStateResult",
        x = 0, y = 0, width = 60, height = 20,
        twoState = true,
      }))
      -- a legitimate 'down' has to be distinguishable from a refusal
      assert.is_true(button:setState("down"))
      assert.is_true(button:setState("up"))
    end)

    it('will not start a single state button in the down state', function()
      local button = track(Geyser.Button:new({
        name = "gbsDownConstraint",
        x = 0, y = 0, width = 60, height = 20,
        msg = "up text",
        state = "down",
      }))
      assert.are.equal("up", button.state)
      assert.is_truthy(getLabelText("gbsDownConstraint"):find("up text", 1, true))
    end)

    it('rejects a state that is not a string or not a known state', function()
      local button = track(Geyser.Button:new({name = "gbsBadState", x = 0, y = 0, width = 60, height = 20}))
      local result, message = button:setState(7)
      assert.is_nil(result)
      assert.is_truthy(message:find("state as string expected, got number", 1, true))
      local badResult, badMessage = button:setState("sideways")
      assert.is_nil(badResult)
      assert.is_truthy(badMessage:find("state must be one of 'up' or 'down'", 1, true))
      assert.are.equal("up", button.state)
    end)

    it('applies the stylesheet of the state it is put into', function()
      local button = track(Geyser.Button:new({
        name = "gbsStyles",
        x = 0, y = 0, width = 60, height = 20,
        twoState = true,
        style = "background-color: black;",
        downStyle = "background-color: blue;",
      }))
      button:setState("up")
      assert.are.equal("background-color: black;", getLabelStyleSheet("gbsStyles"))
      button:setState("down")
      assert.are.equal("background-color: blue;", getLabelStyleSheet("gbsStyles"))
    end)

    it('setMsg and setDownMsg redraw the button in its current state', function()
      local button = track(Geyser.Button:new({
        name = "gbsMessages",
        x = 0, y = 0, width = 60, height = 20,
        twoState = true,
      }))
      button:setMsg("new up")
      assert.is_truthy(getLabelText("gbsMessages"):find("new up", 1, true))
      button:setDownMsg("new down")
      button:setState("down")
      assert.is_truthy(getLabelText("gbsMessages"):find("new down", 1, true))
      local result, message = button:setMsg(42)
      assert.is_nil(result)
      assert.is_truthy(message:find("msg as string expected, got number", 1, true))
    end)

    it('press walks a two state button through both messages', function()
      local pressed = 0
      local button = track(Geyser.Button:new({
        name = "gbsPress",
        x = 0, y = 0, width = 60, height = 20,
        msg = "up text",
        downMsg = "down text",
        twoState = true,
        clickFunction = function() pressed = pressed + 1 end,
        downFunction = function() pressed = pressed + 1 end,
      }))
      button:press()
      assert.are.equal("down", button.state)
      assert.is_truthy(getLabelText("gbsPress"):find("down text", 1, true))
      button:press()
      assert.are.equal("up", button.state)
      assert.is_truthy(getLabelText("gbsPress"):find("up text", 1, true))
      assert.are.equal(2, pressed)
    end)

    it('disableTwoState puts the button back up', function()
      local button = track(Geyser.Button:new({
        name = "gbsDisableTwoState",
        x = 0, y = 0, width = 60, height = 20,
        msg = "up text",
        downMsg = "down text",
        twoState = true,
      }))
      button:setState("down")
      button:disableTwoState()
      assert.is_false(button.twoState)
      assert.are.equal("up", button.state)
      assert.is_truthy(getLabelText("gbsDisableTwoState"):find("up text", 1, true))
    end)

    it('hides and shows the button widget', function()
      local button = track(Geyser.Button:new({name = "gbsVisible", x = 0, y = 0, width = 60, height = 20}))
      button:hide()
      assert.is_false(windowVisible("gbsVisible"))
      button:show()
      assert.is_true(windowVisible("gbsVisible"))
    end)

    it('deletes its widget', function()
      local button = track(Geyser.Button:new({name = "gbsDelete", x = 0, y = 0, width = 60, height = 20}))
      button:delete()
      assert.is_nil(getWindowGeometry("gbsDelete"))
      assert.is_nil(Geyser.windowList.gbsDelete)
    end)
  end)
end)
