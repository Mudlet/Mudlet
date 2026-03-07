--- Speech-to-Text UI Components for Mudlet
-- Provides UI for speech recognition control using toolbar buttons.
-- @module STT.UI

STT = STT or {}
STT.UI = STT.UI or {}

-- UI state
STT.UI._micButton = nil      -- Toolbar.Button
STT.UI._statusLabel = nil    -- Geyser.Label for overlay text
STT.UI._setupDialog = nil    -- Geyser-based setup dialog
STT.UI._isVisible = false
STT.UI._menuItem = nil       -- Menu.Item
STT.UI._hideTimerId = nil    -- Timer ID for auto-hiding status label

-- Default styles for Geyser elements (status label, setup dialog)
STT.UI._styles = {
  statusLabel = [[
    background-color: rgba(40, 40, 40, 180);
    border-radius: 4px;
    padding: 4px;
  ]],
}

--- Create the microphone toolbar button.
-- Uses the Toolbar module for native Qt toolbar integration.
-- @return Toolbar.Button the created button object
function STT.UI.createMicButton()
  if STT.UI._micButton then
    STT.UI._micButton:remove()
    STT.UI._micButton = nil
  end

  STT.UI._micButton = Toolbar.addButton({
    name = "Speech",
    icon = ":/icons/microphone.png",
    tooltip = "Toggle speech recognition\n(Speech-to-Text)",
    onClick = function()
      STT.UI.toggleListening()
    end,
    enabled = STT.isAvailable(),
  })

  -- Set initial state
  STT.UI._updateMicButtonState()

  return STT.UI._micButton
end

--- Create a menu item for STT settings.
-- @return Menu.Item the created menu item
function STT.UI.createMenuItem()
  if STT.UI._menuItem then
    STT.UI._menuItem:remove()
    STT.UI._menuItem = nil
  end

  STT.UI._menuItem = Menu.addItem({
    name = "Speech Recognition Setup...",
    menu = "Tools",
    onClick = function()
      STT.UI.showSetupDialog()
    end,
  })

  return STT.UI._menuItem
end

--- Create a status label to show recognized text.
-- @param cons table of constraints for positioning
-- @param container optional parent container
-- @return Geyser.Label the created label
function STT.UI.createStatusLabel(cons, container)
  cons = cons or {}

  -- Default position: above mic button
  cons.name = cons.name or "STT_StatusLabel"
  cons.x = cons.x or -260
  cons.y = cons.y or -100
  cons.width = cons.width or 240
  cons.height = cons.height or 30

  if STT.UI._statusLabel then
    STT.UI._statusLabel:hide()
    -- Properly destroy the Geyser label by deleting the underlying window
    if STT.UI._statusLabel.name then
      deleteLabel(STT.UI._statusLabel.name)
    end
    STT.UI._statusLabel = nil
  end

  STT.UI._statusLabel = Geyser.Label:new(cons, container)
  STT.UI._statusLabel:setStyleSheet(STT.UI._styles.statusLabel)
  STT.UI._statusLabel:echo("", "white", "c")
  STT.UI._statusLabel:hide()

  return STT.UI._statusLabel
end

--- Toggle speech recognition.
function STT.UI.toggleListening()
  if not STT.isAvailable() then
    STT.UI._showError("Speech recognition not available")
    return
  end

  if not STT.isInitialized() then
    -- Try to initialize with default model
    local success, msg = STT.init()
    if not success then
      STT.UI._showError(msg or "Failed to initialize")
      return
    end
  end

  local success, msg = STT.toggle()
  if not success then
    STT.UI._showError(msg or "Failed to toggle")
  end
end

--- Update the mic button visual state.
function STT.UI._updateMicButtonState()
  if not STT.UI._micButton then
    return
  end

  if not STT.isAvailable() then
    STT.UI._micButton:setEnabled(false)
    STT.UI._micButton:setTooltip("Speech recognition not available")
    STT.UI._micButton:stopPulse()
  elseif STT.isListening() then
    STT.UI._micButton:setEnabled(true)
    STT.UI._micButton:setTooltip("Click to stop listening\n(Speech-to-Text)")
    STT.UI._micButton:setState("listening")
    -- Start pulse with red colors for "recording" indication
    STT.UI._micButton:startPulse("#ff4444", "#cc0000", 500)
  else
    STT.UI._micButton:setEnabled(true)
    STT.UI._micButton:setTooltip("Click to start speech recognition\n(Speech-to-Text)")
    STT.UI._micButton:setState("idle")
    STT.UI._micButton:stopPulse()
  end
end

--- Show an error message briefly.
-- @param message string error message to show
function STT.UI._showError(message)
  if STT.UI._statusLabel then
    -- Cancel any existing hide timer to prevent race condition
    if STT.UI._hideTimerId then
      killTimer(STT.UI._hideTimerId)
      STT.UI._hideTimerId = nil
    end
    STT.UI._statusLabel:show()
    STT.UI._statusLabel:echo(message, "red", "c")
    STT.UI._hideTimerId = tempTimer(3, function()
      STT.UI._hideTimerId = nil
      if STT.UI._statusLabel then
        STT.UI._statusLabel:hide()
      end
    end)
  else
    cecho("\n<red>[STT] " .. message .. "\n")
  end
end

--- Show partial/recognized text.
-- @param text string text to show
function STT.UI._showText(text)
  if STT.UI._statusLabel and text and text ~= "" then
    -- Cancel any existing hide timer to prevent race condition
    if STT.UI._hideTimerId then
      killTimer(STT.UI._hideTimerId)
      STT.UI._hideTimerId = nil
    end
    STT.UI._statusLabel:show()
    STT.UI._statusLabel:echo(text, "white", "c")
    -- Hide after a delay
    STT.UI._hideTimerId = tempTimer(5, function()
      STT.UI._hideTimerId = nil
      if STT.UI._statusLabel then
        STT.UI._statusLabel:hide()
      end
    end)
  end
end

--- Show or hide the mic button.
-- @param show boolean true to show, false to hide
function STT.UI.showMicButton(show)
  if show then
    -- Create the button if it doesn't exist
    if not STT.UI._micButton then
      STT.UI.createMicButton()
    end
    STT.UI._isVisible = true
  else
    -- Remove the button if it exists
    if STT.UI._micButton then
      STT.UI._micButton:remove()
      STT.UI._micButton = nil
    end
    STT.UI._isVisible = false
  end
end

--- Create and show a simple setup dialog for selecting Vosk models.
-- @return Geyser.Container the dialog container
function STT.UI.showSetupDialog()
  -- Always recreate the dialog to ensure the model list is fresh
  if STT.UI._setupDialog then
    -- Destroy the existing dialog
    if STT.UI._setupDialog.name then
      deleteLabel(STT.UI._setupDialog.name)
    end
    STT.UI._setupDialog = nil
  end

  -- Create dialog container
  STT.UI._setupDialog = Geyser.Container:new({
    name = "STT_SetupDialog",
    x = "20%", y = "20%",
    width = "60%", height = "60%",
  })

  -- Background
  local bg = Geyser.Label:new({
    name = "STT_SetupDialog_BG",
    x = 0, y = 0,
    width = "100%", height = "100%",
  }, STT.UI._setupDialog)
  bg:setStyleSheet([[
    background-color: rgba(30, 30, 30, 240);
    border: 2px solid #555;
    border-radius: 8px;
  ]])

  -- Title
  local title = Geyser.Label:new({
    name = "STT_SetupDialog_Title",
    x = 0, y = 10,
    width = "100%", height = 30,
  }, STT.UI._setupDialog)
  title:echo("<center>Speech Recognition Setup</center>", "white", "cb14")

  -- Model list
  local models = STT.listModels()
  local yPos = 60

  if #models == 0 then
    local noModels = Geyser.Label:new({
      name = "STT_SetupDialog_NoModels",
      x = 10, y = yPos,
      width = "95%", height = 60,
    }, STT.UI._setupDialog)
    noModels:echo([[<center>No Vosk models found.<br/>
      Download a model from <a href="https://alphacephei.com/vosk/models">alphacephei.com/vosk/models</a><br/>
      and place it in your Mudlet models directory.</center>]], "#aaa", "c")
  else
    for i, model in ipairs(models) do
      local modelBtn = Geyser.Label:new({
        name = "STT_SetupDialog_Model_" .. i,
        x = 10, y = yPos,
        width = "95%", height = 40,
      }, STT.UI._setupDialog)
      modelBtn:setStyleSheet([[
        background-color: rgba(50, 50, 50, 200);
        border: 1px solid #666;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("<center>%s</center>", model.name or model.path), "white", "c")
      modelBtn:setClickCallback(function()
        local success, errorMsg = STT.init(model.path)
        if success then
          STT.UI._setupDialog:hide()
          cecho("\n<green>[STT] Model loaded: " .. (model.name or model.path) .. "\n")
        else
          -- Keep dialog open and show error
          cecho("\n<red>[STT] Failed to load model: " .. (errorMsg or "Unknown error") .. "\n")
        end
      end)
      yPos = yPos + 50
    end
  end

  -- Close button
  local closeBtn = Geyser.Label:new({
    name = "STT_SetupDialog_Close",
    x = "40%", y = -50,
    width = "20%", height = 30,
  }, STT.UI._setupDialog)
  closeBtn:setStyleSheet([[
    background-color: rgba(80, 80, 80, 200);
    border: 1px solid #888;
    border-radius: 4px;
  ]])
  closeBtn:echo("<center>Close</center>", "white", "c")
  closeBtn:setClickCallback(function()
    STT.UI._setupDialog:hide()
  end)

  return STT.UI._setupDialog
end

--- Hide the setup dialog.
function STT.UI.hideSetupDialog()
  if STT.UI._setupDialog then
    STT.UI._setupDialog:hide()
  end
end

--- Set up default callbacks to update UI on events.
function STT.UI._setupDefaultCallbacks()
  -- Guard against STT core module not being loaded yet
  if not STT.setOnStateChanged or not STT.setOnPartial or not STT.setOnError then
    return false
  end

  -- Update button state when state changes
  STT.setOnStateChanged(function(state)
    STT.UI._updateMicButtonState()
  end)

  -- Show partial results
  STT.setOnPartial(function(text)
    STT.UI._showText(text)
  end)

  -- Show errors
  STT.setOnError(function(message)
    STT.UI._showError(message)
    STT.UI._updateMicButtonState()
  end)

  return true
end

--- Handle sysSTTSetupNeeded event to show setup dialog.
-- @param event string event name
-- @param reason string why setup is needed ("backend_unavailable" or "model_not_loaded")
function STT.UI._handleSetupNeeded(event, reason)
  -- Show the setup dialog when setup is needed
  STT.UI.showSetupDialog()
end

-- Register event handler for setup needed
STT.UI._setupNeededHandlerId = nil

function STT.UI._registerSetupNeededHandler()
  -- Clean up existing handler if present
  if STT.UI._setupNeededHandlerId and killAnonymousEventHandler then
    killAnonymousEventHandler(STT.UI._setupNeededHandlerId)
    STT.UI._setupNeededHandlerId = nil
  end

  if registerAnonymousEventHandler then
    STT.UI._setupNeededHandlerId = registerAnonymousEventHandler(
      "sysSTTSetupNeeded",
      STT.UI._handleSetupNeeded
    )
  end
end

-- Auto-setup callbacks when this module loads (defer to ensure STT core is loaded)
if not STT.UI._setupDefaultCallbacks() then
  tempTimer(0, function()
    STT.UI._setupDefaultCallbacks()
  end)
end

-- Register setup needed handler
STT.UI._registerSetupNeededHandler()
