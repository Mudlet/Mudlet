--- Toolbar Module for Mudlet
-- Provides Lua-based control of toolbar buttons with object-oriented API.
-- @module Toolbar

Toolbar = Toolbar or {}

-- Internal state tracking
Toolbar._buttons = {}
Toolbar._eventHandler = nil

--- Button class for toolbar buttons
-- @type Toolbar.Button
Toolbar.Button = {}
Toolbar.Button.__index = Toolbar.Button

--- Create a new toolbar button.
-- @param cons table of button configuration:
--   - name: string - Button text label
--   - icon: string - Icon path or resource (e.g., ":/icons/microphone.png")
--   - tooltip: string - Hover tooltip text
--   - onClick: function - Optional callback when clicked
--   - enabled: boolean - Initial enabled state (default true)
-- @return Toolbar.Button the created button object
function Toolbar.addButton(cons)
  cons = cons or {}

  if not cons.name then
    error("Toolbar.addButton requires 'name' parameter", 2)
  end

  local name = cons.name
  local icon = cons.icon or ""
  local tooltip = cons.tooltip or name

  -- Call C++ to create the button
  local buttonId = addToolbarButton(name, icon, tooltip)
  if not buttonId or buttonId < 0 then
    return nil, "Failed to create toolbar button"
  end

  -- Create button object
  local button = setmetatable({}, Toolbar.Button)
  button._id = buttonId
  button._name = name
  button._onClick = cons.onClick
  button._enabled = cons.enabled ~= false
  button._pulsing = false

  -- Track the button
  Toolbar._buttons[buttonId] = button

  -- Set initial enabled state
  if not button._enabled then
    setToolbarButtonEnabled(buttonId, false)
  end

  return button
end

--- Remove a toolbar button.
-- @param button Toolbar.Button the button to remove
-- @return boolean success
function Toolbar.removeButton(button)
  if not button or not button._id then
    return false
  end

  local success = removeToolbarButton(button._id)
  if success then
    Toolbar._buttons[button._id] = nil
  end
  return success
end

--- Get the button ID.
-- @return number the button ID
function Toolbar.Button:getId()
  return self._id
end

--- Get the button name.
-- @return string the button name
function Toolbar.Button:getName()
  return self._name
end

--- Set the button's click callback.
-- @param callback function to call when clicked
function Toolbar.Button:setOnClick(callback)
  self._onClick = callback
end

--- Set the button icon.
-- @param icon string path or resource for the icon
-- @return boolean success
function Toolbar.Button:setIcon(icon)
  return setToolbarButtonIcon(self._id, icon)
end

--- Set the button tooltip.
-- @param tooltip string tooltip text
-- @return boolean success
function Toolbar.Button:setTooltip(tooltip)
  return setToolbarButtonTooltip(self._id, tooltip)
end

--- Enable or disable the button.
-- @param enabled boolean enabled state
-- @return boolean success
function Toolbar.Button:setEnabled(enabled)
  local result = setToolbarButtonEnabled(self._id, enabled)
  if result then
    self._enabled = enabled
  end
  return result
end

--- Check if the button is enabled.
-- @return boolean enabled state
function Toolbar.Button:isEnabled()
  return self._enabled
end

--- Set the button's custom state.
-- @param state string custom state value
-- @return boolean success
function Toolbar.Button:setState(state)
  local result = setToolbarButtonState(self._id, state)
  if result then
    self._state = state
  end
  return result
end

--- Get the button's custom state.
-- @return string the current state
function Toolbar.Button:getState()
  return self._state
end

--- Start pulsing animation on the button.
-- @param color1 string first pulse color (default "#ff4444")
-- @param color2 string second pulse color (default "#cc0000")
-- @param interval number pulse interval in ms (default 500)
-- @return boolean success
function Toolbar.Button:startPulse(color1, color2, interval)
  color1 = color1 or "#ff4444"
  color2 = color2 or "#cc0000"
  interval = interval or 500
  local success = setToolbarButtonPulse(self._id, true, color1, color2, interval)
  if success then
    self._pulsing = true
  end
  return success
end

--- Stop pulsing animation on the button.
-- @return boolean success
function Toolbar.Button:stopPulse()
  local success = setToolbarButtonPulse(self._id, false)
  if success then
    self._pulsing = false
  end
  return success
end

--- Check if the button is pulsing.
-- @return boolean pulsing state
function Toolbar.Button:isPulsing()
  return self._pulsing
end

--- Remove this button from the toolbar.
-- @return boolean success
function Toolbar.Button:remove()
  return Toolbar.removeButton(self)
end

-- Event handler for button clicks
local function handleToolbarButtonClick(event, buttonId)
  buttonId = tonumber(buttonId)
  if buttonId and Toolbar._buttons[buttonId] then
    local button = Toolbar._buttons[buttonId]
    if button._onClick then
      button._onClick(button)
    end
  end
end

--- Initialize the toolbar event handler (called automatically).
function Toolbar._setupEventHandler()
  if Toolbar._eventHandler then
    return
  end

  if registerAnonymousEventHandler then
    Toolbar._eventHandler = registerAnonymousEventHandler(
      "sysToolbarButtonClicked",
      handleToolbarButtonClick
    )
  end
end

-- Auto-initialize on load
Toolbar._setupEventHandler()
