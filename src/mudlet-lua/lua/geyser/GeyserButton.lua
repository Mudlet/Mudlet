--- Represents a clickable button. Can be a single clickable action or a two state button
-- which alternates between the two states when clicked.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Geyser.Button">Mudlet Manual</a>
-- @author demonnic
-- @module Geyser.Button

--- A clickable button.
-- @field name The name of the button.
-- @field height The height of the button in pixels.
-- @field width The height of the button in pixels.
-- @field color The color to make the button. If a two state button, will be used for the 'up' state. Superceded by 'style' which is valid and optional stylesheet to use for the button. If a two state button, will be used for the 'up' state.
-- @field downColor The color to make the button while it is in the 'down' state. Superceded by 'downStyle' which is valid and optional stylesheet to use for the button while it is in the 'down' state.
-- @field msg The text to put on the button. If a two state button, will be used for the 'up' state.
-- @field downMsg The text to put on the button in the 'down' state.
-- @field tooltip The text to show when the button is hovered over. For two state buttons will be used for the 'up' tooltip
-- @field downTooltip The text to show when the button is hovered over while in the 'down' state.
-- @field downCommand The command to send when the two state button is clicked while in the 'down' state. 'downFunction' is also valid and optional function to run when the two state button is clicked while in the 'down' state.
-- @field clickCommand The command to send when the button is clicked. For two state buttons, used when clicking in the 'up' state. Will be skipped if 'clickFunction' is defined.  'clickFunction' is also valid and optional function to run when button clicked. For two state buttons, used when clicking in the 'up' state. If defined takes precedence over clickCommand.
-- @field twoState If true, the button will be treated as a two state button, with 'up' and 'down' states.
-- @field state 'up' or 'down' depending on button state for two state buttons. Will always be 'up' if a single state button.
-- @field toolTipDuration The amount of time for the tooltip to show. Used for both 'up' and 'down' states.
Geyser.Button = {
  name            = "GeyserButtonClass",
  height          = 50,
  width           = 50,
  color           = "blue",
  downColor       = "blue",
  msg             = "<center>Look</center>",
  downMsg         = "<center>Look</center>",
  tooltip         = "Click to look",
  downTooltip     = "Click to look",
  downCommand     = "look",
  clickCommand    = "look",
  twoState        = false,
  state           = "up",
  toolTipDuration = 5,
}
Geyser.Button.parent = Geyser.Label
setmetatable(Geyser.Button, Geyser.Label)
Geyser.Button.__index = Geyser.Button

--- Creates a new button and returns it
-- @param cons table of constraints which define the button
-- @param container the container to place the button into
function Geyser.Button:new(cons, container)
  cons = cons or {}
  local consType = type(cons)
  if consType ~= "table" then
    printError(f"bad argument #1 type (cons as table expects, got {consType})", true, true)
  end
  cons.name = cons.name or Geyser.nameGen("button")
  local me = self.parent:new(cons, container)
  setmetatable(me, self)
  me:setClickCallback(function() me:press() end)
  me:setState(me.state)
  me:resize() -- to pick up the Geyser.Button default size rather than Geyser.Label's
  return me
end

--- Sets the state of the button to "up" or "down" and performs all accompanying visual transformations.
-- Does not simulate the actual click, see Geyser.Button:press for that
-- @param state the state to set the button to. One of "up" or "down"
-- @see press
function Geyser.Button:setState(state)
  local stateType = type(state)
  if stateType ~= "string" then
    return nil, f"bad argument #1 type (state as string expected, got {stateType})"
  end
  state = state:lower()
  if state ~= "up" and state ~= "down" then
    return nil, f"bad argument #1 value (state must be one of 'up' or 'down', got {state})"
  end
  self.state = state
  if state == "up" then
    self:echo(self.msg)
    if self.style then
      if type(self.style) == "table" then
        self:setStyleSheet(self.style:getCSS())
      else
        self:setStyleSheet(self.style)
      end
    else
      self.parent.setColor(self, self.color)
    end
    self:setToolTip(self.tooltip, self.toolTipDuration)
    return true
  end
  if not self.twoState then
    return nil, "cannot set a single state button's state to 'down', only 'up'"
  end
  self:echo(self.downMsg)
  if self.downStyle then
    if type(self.downStyle) == "table" then
      self:setStyleSheet(self.downStyle:getCSS())
    else
      self:setStyleSheet(self.downStyle)
    end
  else
    self.parent.setColor(self, self.downColor)
  end
  self:setToolTip(self.downTooltip, self.toolTipDuration)
end

--- Handles clicking the button. If the button is twoState, also handles switching the button's state
function Geyser.Button:press()
  local command, func, newState
  local state = self.state
  if state == "up" then
    command = self.clickCommand
    func = self.clickFunction
    newState = self.twoState and "down" or "up"
  else
    command = self.downCommand
    func = self.downFunction
    newState = "up"
  end
  if func then
    local ok,err = pcall(func)
    if not ok then
      printError(err, true, true)
    end
  else
    expandAlias(command)
  end
  self:setState(newState)
end

--- Stores the command to send when clicking the button in its 'down' state
-- @param command the command to send. Will be run as though it is an alias.
function Geyser.Button:setDownCommand(command)
  local commandType = type(command)
  if commandType ~= "string" then
    printError(f"bad argument #1 type (command as string expected, got {commandType})", true, true)
  end
  self.downCommand = command
end

--- Stores the function to run when clicking the button in its 'down' state. Supercedes downCommand
-- @param downFunction The function to run when the button is clicked in its 'down' state. Should be a lua function or valid lua code as a string. 
function Geyser.Button:setDownFunction(downFunction)
  local funcType = type(downFunction)
  if funcType ~= "function" then
    if funcType == "string" then
      local ok, err = loadstring(downFunction)
      if ok then
        downFunction = ok
      else
        printError(f"Error while compiling Lua code from string for downFunction: {err}", true, true)
      end
    else
      printError(f"bad argument #1 type (downFunction as a string or a function expected, got {funcType})")
    end
  end
  self.downFunction = downFunction
end

--- Sets the command to use when the button is clicked. If a two state button, will be used for the 'up' state.
-- @param command the command to send when the button is clicked. Will be treated as an alias.
function Geyser.Button:setClickCommand(command)
  local commandType = type(command)
  if commandType ~= "string" then
    printError(f"bad argument #1 type (command as string expected, got {commandType})", true, true)
  end
  self.clickCommand = command
end

--- Sets the function to be run when the button is clicked. If a two state button, will be used for the 'up' state.
-- @param clickFunction the function to run when the button is clicked.
function Geyser.Button:setClickFunction(clickFunction)
  local funcType = type(clickFunction)
  if funcType ~= "function" then
    if funcType == "string" then
      local ok, err = loadstring(clickFunction)
      if ok then
        clickFunction = ok
      else
        printError(f"Error while compiling codestring for clickFunction: {err}", true, true)
      end
    else
      printError(f"bad argument #1 type (clickFunction as function or valid Lua string expected, got {funcType})")
    end
  end
  self.clickFunction = clickFunction
end

--- Sets the color to use when the button is in its 'down' state
-- @param color the color to use.
function Geyser.Button:setDownColor(color)
  self.downColor = color
  self:setState(self.state)
end

--- Sets the color to use for the button. If a two state button, will be used for the 'up' state
-- @param color the color to use
function Geyser.Button:setColor(color)
  self.color = color
  self:setState(self.state)
end

--- Sets the style to use when the button is in its 'down' state
-- @param style the stylesheet to use
function Geyser.Button:setDownStyle(style)
  self.downStyle = style
  self:setState(self.state)
end

--- Sets the style to use for the button. If a two state button, will be used for the 'up' state
-- @param style the stylesheet to use
function Geyser.Button:setStyle(style)
  self.style = style
  self:setState(self.state)
end

--- Enables two state functionality for the button
function Geyser.Button:enableTwoState()
  self.twoState = true
  self:setState(self.state)
end

--- Disables two state functionality and resets the button state to 'up' if necessary
function Geyser.Button:disableTwoState()
  self.twoState = false
  self:setState("up")
end

--- Set the msg displayed on the button. If a two state button, will be used for the 'up' state
-- @param msg the msg to display.
function Geyser.Button:setMsg(msg)
  local msgType = type(msg)
  if msgType ~= "string" then
    return nil, f"bad argument #1 type (msg as string expected, got {msgType})"
  end
  self.msg = msg
  self:setState(self.state)
end

--- Set the msg displayed on the button when in the 'down' state
-- @param msg the msg to display.
function Geyser.Button:setDownMsg(msg)
  local msgType = type(msg)
  if msgType ~= "string" then
    return nil, f"bad argument #1 type (msg as string expected, got {msgType})"
  end
  self.downMsg = msg
  self:setState(self.state)
end
