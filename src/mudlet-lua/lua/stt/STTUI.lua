--- Speech-to-Text UI Components for Mudlet
-- Provides Geyser-based UI for speech recognition control.
-- @module STT.UI

STT = STT or {}
STT.UI = STT.UI or {}

-- UI state
STT.UI._micButton = nil
STT.UI._statusLabel = nil
STT.UI._setupDialog = nil
STT.UI._isVisible = false

-- Default styles
STT.UI._styles = {
  micButton = {
    idle = [[
      background-color: rgba(60, 60, 60, 200);
      border-radius: 20px;
      border: 2px solid #666;
    ]],
    listening = [[
      background-color: rgba(200, 60, 60, 200);
      border-radius: 20px;
      border: 2px solid #ff4444;
    ]],
    listeningPulse = [[
      background-color: rgba(255, 80, 80, 200);
      border-radius: 20px;
      border: 2px solid #ff6666;
    ]],
    unavailable = [[
      background-color: rgba(40, 40, 40, 150);
      border-radius: 20px;
      border: 2px solid #333;
    ]],
  },
  statusLabel = [[
    background-color: rgba(40, 40, 40, 180);
    border-radius: 4px;
    padding: 4px;
  ]],
}

-- Pulse timer for listening animation
STT.UI._pulseTimer = nil
STT.UI._pulseState = false

--- Create the microphone button.
-- @param cons table of constraints for positioning
-- @param container optional parent container
-- @return Geyser.Label the created button
function STT.UI.createMicButton(cons, container)
  cons = cons or {}

  -- Default position: bottom right corner
  cons.name = cons.name or "STT_MicButton"
  cons.x = cons.x or -60
  cons.y = cons.y or -60
  cons.width = cons.width or 40
  cons.height = cons.height or 40

  if STT.UI._micButton then
    STT.UI._micButton:hide()
    STT.UI._micButton = nil
  end

  STT.UI._micButton = Geyser.Label:new(cons, container)

  -- Set initial state
  STT.UI._updateMicButtonState()

  -- Set up click handler
  STT.UI._micButton:setClickCallback(function()
    STT.UI.toggleListening()
  end)

  -- Set tooltip
  STT.UI._micButton:setToolTip("Click to toggle speech recognition", 3)

  return STT.UI._micButton
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

  local micIcon = "🎤" -- Unicode microphone

  if not STT.isAvailable() then
    STT.UI._micButton:setStyleSheet(STT.UI._styles.micButton.unavailable)
    STT.UI._micButton:echo("<center>" .. micIcon .. "</center>", "#666", "c")
    STT.UI._stopPulse()
  elseif STT.isListening() then
    STT.UI._micButton:echo("<center>" .. micIcon .. "</center>", "white", "c")
    STT.UI._startPulse()
  else
    STT.UI._micButton:setStyleSheet(STT.UI._styles.micButton.idle)
    STT.UI._micButton:echo("<center>" .. micIcon .. "</center>", "white", "c")
    STT.UI._stopPulse()
  end
end

--- Start the pulsing animation for the listening state.
function STT.UI._startPulse()
  if STT.UI._pulseTimer then
    return -- Already pulsing
  end

  STT.UI._pulseState = true
  STT.UI._pulseTimer = tempTimer(0.5, function()
    if not STT.UI._micButton then
      STT.UI._stopPulse()
      return
    end

    STT.UI._pulseState = not STT.UI._pulseState
    if STT.UI._pulseState then
      STT.UI._micButton:setStyleSheet(STT.UI._styles.micButton.listening)
    else
      STT.UI._micButton:setStyleSheet(STT.UI._styles.micButton.listeningPulse)
    end
  end, true) -- true for repeating timer
end

--- Stop the pulsing animation.
function STT.UI._stopPulse()
  if STT.UI._pulseTimer then
    killTimer(STT.UI._pulseTimer)
    STT.UI._pulseTimer = nil
  end
  STT.UI._pulseState = false
end

--- Show an error message briefly.
-- @param message string error message to show
function STT.UI._showError(message)
  if STT.UI._statusLabel then
    STT.UI._statusLabel:show()
    STT.UI._statusLabel:echo(message, "red", "c")
    tempTimer(3, function()
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
    STT.UI._statusLabel:show()
    STT.UI._statusLabel:echo(text, "white", "c")
    -- Hide after a delay
    tempTimer(5, function()
      if STT.UI._statusLabel then
        STT.UI._statusLabel:hide()
      end
    end)
  end
end

--- Show or hide the mic button.
-- @param show boolean true to show, false to hide
function STT.UI.showMicButton(show)
  if STT.UI._micButton then
    if show then
      STT.UI._micButton:show()
    else
      STT.UI._micButton:hide()
    end
    STT.UI._isVisible = show
  end
end

--- Create and show a simple setup dialog for selecting Vosk models.
-- @return Geyser.Container the dialog container
function STT.UI.showSetupDialog()
  if STT.UI._setupDialog then
    STT.UI._setupDialog:show()
    return STT.UI._setupDialog
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
        STT.init(model.path)
        STT.UI._setupDialog:hide()
        cecho("\n<green>[STT] Model loaded: " .. (model.name or model.path) .. "\n")
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
end

-- Auto-setup callbacks when this module loads
STT.UI._setupDefaultCallbacks()
