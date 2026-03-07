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
STT.UI._downloading = false  -- Download in progress flag
STT.UI._downloadHandlerId = nil  -- Event handler for download completion
STT.UI._progressHandlerId = nil  -- Event handler for download progress

-- Default styles for Geyser elements (status label, setup dialog)
STT.UI._styles = {
  statusLabel = [[
    background-color: rgba(40, 40, 40, 180);
    border-radius: 4px;
    padding: 4px;
  ]],
}

-- Available Vosk models for download
-- Format: { name, identifier, language, url, sizeBytes, isSmall }
STT.UI._availableModels = {
  -- English models
  { name = "English (US) - Small", identifier = "vosk-model-small-en-us-0.15", language = "en",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip", size = 40, isSmall = true },
  { name = "English (US) - Large", identifier = "vosk-model-en-us-0.22", language = "en",
    url = "https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip", size = 1800, isSmall = false },
  -- German models
  { name = "German - Small", identifier = "vosk-model-small-de-0.15", language = "de",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-de-0.15.zip", size = 45, isSmall = true },
  { name = "German - Large", identifier = "vosk-model-de-0.21", language = "de",
    url = "https://alphacephei.com/vosk/models/vosk-model-de-0.21.zip", size = 1900, isSmall = false },
  -- Spanish models
  { name = "Spanish - Small", identifier = "vosk-model-small-es-0.42", language = "es",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-es-0.42.zip", size = 39, isSmall = true },
  -- French models
  { name = "French - Small", identifier = "vosk-model-small-fr-0.22", language = "fr",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip", size = 41, isSmall = true },
  { name = "French - Large", identifier = "vosk-model-fr-0.22", language = "fr",
    url = "https://alphacephei.com/vosk/models/vosk-model-fr-0.22.zip", size = 1400, isSmall = false },
  -- Italian models
  { name = "Italian - Small", identifier = "vosk-model-small-it-0.22", language = "it",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-it-0.22.zip", size = 48, isSmall = true },
  -- Portuguese models
  { name = "Portuguese - Small", identifier = "vosk-model-small-pt-0.3", language = "pt",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-pt-0.3.zip", size = 31, isSmall = true },
  -- Russian models
  { name = "Russian - Small", identifier = "vosk-model-small-ru-0.22", language = "ru",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-ru-0.22.zip", size = 45, isSmall = true },
  -- Chinese models
  { name = "Chinese - Small", identifier = "vosk-model-small-cn-0.22", language = "zh",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip", size = 42, isSmall = true },
  -- Japanese models
  { name = "Japanese - Small", identifier = "vosk-model-small-ja-0.22", language = "ja",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip", size = 48, isSmall = true },
  -- Polish models
  { name = "Polish - Small", identifier = "vosk-model-small-pl-0.22", language = "pl",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-pl-0.22.zip", size = 47, isSmall = true },
  -- Dutch models
  { name = "Dutch - Small", identifier = "vosk-model-small-nl-0.22", language = "nl",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-nl-0.22.zip", size = 39, isSmall = true },
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

--- Create and show the setup dialog for selecting and downloading Vosk models.
-- @return Geyser.Container the dialog container
function STT.UI.showSetupDialog()
  -- Always recreate the dialog to ensure the model list is fresh
  if STT.UI._setupDialog then
    STT.UI._cleanupSetupDialog()
  end

  -- Create dialog container - make it taller to fit both sections
  STT.UI._setupDialog = Geyser.Container:new({
    name = "STT_SetupDialog",
    x = "15%", y = "10%",
    width = "70%", height = "80%",
  })

  -- Background
  local bg = Geyser.Label:new({
    name = "STT_SetupDialog_BG",
    x = 0, y = 0,
    width = "100%", height = "100%",
  }, STT.UI._setupDialog)
  bg:setStyleSheet([[
    background-color: rgba(30, 30, 30, 245);
    border: 2px solid #555;
    border-radius: 8px;
  ]])

  -- Title
  local title = Geyser.Label:new({
    name = "STT_SetupDialog_Title",
    x = 0, y = 10,
    width = "100%", height = 30,
  }, STT.UI._setupDialog)
  title:echo("<center><b>Speech Recognition Setup</b></center>", "white", "c14")

  -- Status message area (for showing progress/errors)
  STT.UI._dialogStatus = Geyser.Label:new({
    name = "STT_SetupDialog_Status",
    x = 10, y = 45,
    width = "95%", height = 25,
  }, STT.UI._setupDialog)
  STT.UI._dialogStatus:setStyleSheet([[
    background-color: transparent;
  ]])
  STT.UI._dialogStatus:echo("", "white", "c")

  -- Progress bar (hidden by default)
  STT.UI._progressBar = Geyser.Gauge:new({
    name = "STT_SetupDialog_Progress",
    x = 10, y = 70,
    width = "95%", height = 20,
  }, STT.UI._setupDialog)
  STT.UI._progressBar:setStyleSheet([[
    background-color: rgba(50, 50, 50, 200);
    border: 1px solid #666;
    border-radius: 3px;
  ]], [[
    background-color: rgba(60, 140, 60, 255);
    border-radius: 3px;
  ]], [[]])
  STT.UI._progressBar:setValue(0)
  STT.UI._progressBar:hide()

  local yOffset = 75

  -- Section: Installed Models
  local installedTitle = Geyser.Label:new({
    name = "STT_SetupDialog_InstalledTitle",
    x = 10, y = yOffset,
    width = "95%", height = 25,
  }, STT.UI._setupDialog)
  installedTitle:echo("<b>Installed Models</b> (click to use)", "#aaa", "l")
  yOffset = yOffset + 30

  -- Scroll area for installed models
  local installedModels = STT.listModels()
  local installedAreaHeight = math.min(#installedModels * 35 + 10, 120)

  if #installedModels == 0 then
    local noModels = Geyser.Label:new({
      name = "STT_SetupDialog_NoInstalled",
      x = 10, y = yOffset,
      width = "95%", height = 35,
    }, STT.UI._setupDialog)
    noModels:echo("<i>No models installed yet. Download one below.</i>", "#888", "c")
    yOffset = yOffset + 40
  else
    for i, model in ipairs(installedModels) do
      local modelBtn = Geyser.Label:new({
        name = "STT_SetupDialog_Installed_" .. i,
        x = 10, y = yOffset,
        width = "95%", height = 30,
      }, STT.UI._setupDialog)
      modelBtn:setStyleSheet([[
        background-color: rgba(40, 80, 40, 200);
        border: 1px solid #4a4;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s", model.name or model.path), "white", "l")
      modelBtn:setClickCallback(function()
        local success, errorMsg = STT.init(model.path)
        if success then
          STT.UI._setDialogStatus("Model loaded: " .. (model.name or model.path), "lime")
          tempTimer(1.5, function()
            STT.UI.hideSetupDialog()
          end)
        else
          STT.UI._setDialogStatus("Failed: " .. (errorMsg or "Unknown error"), "red")
        end
      end)
      yOffset = yOffset + 35
    end
  end

  yOffset = yOffset + 15

  -- Section: Download Models
  local downloadTitle = Geyser.Label:new({
    name = "STT_SetupDialog_DownloadTitle",
    x = 10, y = yOffset,
    width = "95%", height = 25,
  }, STT.UI._setupDialog)
  downloadTitle:echo("<b>Download New Model</b> (click to download)", "#aaa", "l")
  yOffset = yOffset + 30

  -- Create scrollable area for downloadable models
  local scrollContainer = Geyser.Container:new({
    name = "STT_SetupDialog_ScrollArea",
    x = 10, y = yOffset,
    width = "95%", height = -100,  -- Leave room for close button
  }, STT.UI._setupDialog)

  -- Add downloadable models
  local scrollY = 0
  for i, model in ipairs(STT.UI._availableModels) do
    -- Check if already installed
    local isInstalled = STT.UI._isModelInstalled(model.identifier)
    local sizeStr = model.size < 100 and string.format("~%d MB", model.size) or string.format("~%.1f GB", model.size / 1024)

    local modelBtn = Geyser.Label:new({
      name = "STT_SetupDialog_Download_" .. i,
      x = 0, y = scrollY,
      width = "100%", height = 35,
    }, scrollContainer)

    if isInstalled then
      modelBtn:setStyleSheet([[
        background-color: rgba(60, 60, 60, 150);
        border: 1px solid #555;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s (%s) <i>[installed]</i>", model.name, sizeStr), "#777", "l")
    else
      modelBtn:setStyleSheet([[
        background-color: rgba(50, 50, 80, 200);
        border: 1px solid #668;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s (%s)", model.name, sizeStr), "white", "l")
      modelBtn:setClickCallback(function()
        STT.UI._downloadModel(model)
      end)
    end
    scrollY = scrollY + 40
  end

  -- Close button
  local closeBtn = Geyser.Label:new({
    name = "STT_SetupDialog_Close",
    x = "40%", y = -50,
    width = "20%", height = 35,
  }, STT.UI._setupDialog)
  closeBtn:setStyleSheet([[
    background-color: rgba(80, 80, 80, 200);
    border: 1px solid #888;
    border-radius: 4px;
  ]])
  closeBtn:echo("<center>Close</center>", "white", "c")
  closeBtn:setClickCallback(function()
    STT.UI.hideSetupDialog()
  end)

  return STT.UI._setupDialog
end

--- Clean up the setup dialog and event handlers.
function STT.UI._cleanupSetupDialog()
  if STT.UI._downloadHandlerId then
    killAnonymousEventHandler(STT.UI._downloadHandlerId)
    STT.UI._downloadHandlerId = nil
  end
  if STT.UI._progressHandlerId then
    killAnonymousEventHandler(STT.UI._progressHandlerId)
    STT.UI._progressHandlerId = nil
  end
  if STT.UI._setupDialog and STT.UI._setupDialog.name then
    deleteLabel(STT.UI._setupDialog.name)
  end
  STT.UI._setupDialog = nil
  STT.UI._dialogStatus = nil
  STT.UI._progressBar = nil
  STT.UI._downloading = false
end

--- Check if a model is already installed.
-- @param identifier string the model identifier (folder name)
-- @return boolean true if installed
function STT.UI._isModelInstalled(identifier)
  local models = STT.listModels()
  for _, model in ipairs(models) do
    if model.name == identifier or (model.path and model.path:find(identifier, 1, true)) then
      return true
    end
  end
  return false
end

--- Set the dialog status message.
-- @param message string the message to display
-- @param color string optional color name
function STT.UI._setDialogStatus(message, color)
  if STT.UI._dialogStatus then
    STT.UI._dialogStatus:echo(message, color or "white", "c")
  end
end

--- Download and install a Vosk model.
-- @param model table the model info from _availableModels
function STT.UI._downloadModel(model)
  if STT.UI._downloading then
    STT.UI._setDialogStatus("Download already in progress...", "yellow")
    return
  end

  STT.UI._downloading = true
  STT.UI._setDialogStatus("Downloading " .. model.name .. "...", "cyan")

  if STT.UI._progressBar then
    STT.UI._progressBar:show()
    STT.UI._progressBar:setValue(0)
  end

  -- Get model directory path
  local modelDir = stt.getModelPath()
  if not modelDir then
    modelDir = getMudletHomeDir() .. "/speech-models"
  end

  -- Create directory if needed
  lfs.mkdir(modelDir)

  local zipPath = modelDir .. "/" .. model.identifier .. ".zip"
  local extractPath = modelDir

  -- Register download completion handler
  local unzipDoneHandlerId = nil
  local unzipErrorHandlerId = nil

  STT.UI._downloadHandlerId = registerAnonymousEventHandler("sysDownloadDone", function(event, path, bytesWritten)
    if path ~= zipPath then return end

    -- Unregister download handlers
    if STT.UI._downloadHandlerId then
      killAnonymousEventHandler(STT.UI._downloadHandlerId)
      STT.UI._downloadHandlerId = nil
    end
    if STT.UI._progressHandlerId then
      killAnonymousEventHandler(STT.UI._progressHandlerId)
      STT.UI._progressHandlerId = nil
    end

    STT.UI._setDialogStatus("Extracting model...", "cyan")

    -- Register unzip completion handler
    unzipDoneHandlerId = registerAnonymousEventHandler("sysUnzipDone", function(evt, zip, dest)
      if zip ~= zipPath then return end

      if unzipDoneHandlerId then
        killAnonymousEventHandler(unzipDoneHandlerId)
        unzipDoneHandlerId = nil
      end
      if unzipErrorHandlerId then
        killAnonymousEventHandler(unzipErrorHandlerId)
        unzipErrorHandlerId = nil
      end

      -- Clean up zip file
      os.remove(zipPath)

      STT.UI._setDialogStatus("Model installed successfully!", "lime")
      STT.UI._downloading = false

      if STT.UI._progressBar then
        STT.UI._progressBar:setValue(100)
      end

      -- Refresh the dialog to show the new model
      tempTimer(1.5, function()
        if STT.UI._setupDialog then
          STT.UI.showSetupDialog()
        end
      end)
    end)

    unzipErrorHandlerId = registerAnonymousEventHandler("sysUnzipError", function(evt, zip, dest)
      if zip ~= zipPath then return end

      if unzipDoneHandlerId then
        killAnonymousEventHandler(unzipDoneHandlerId)
        unzipDoneHandlerId = nil
      end
      if unzipErrorHandlerId then
        killAnonymousEventHandler(unzipErrorHandlerId)
        unzipErrorHandlerId = nil
      end

      STT.UI._setDialogStatus("Failed to extract model archive", "red")
      STT.UI._downloading = false
      os.remove(zipPath)
    end)

    -- Start async unzip
    unzipAsync(zipPath, extractPath)
  end)

  -- Register download error handler
  local errorHandlerId = registerAnonymousEventHandler("sysDownloadError", function(event, errorMsg, path)
    if path ~= zipPath then return end

    if STT.UI._downloadHandlerId then
      killAnonymousEventHandler(STT.UI._downloadHandlerId)
      STT.UI._downloadHandlerId = nil
    end
    if STT.UI._progressHandlerId then
      killAnonymousEventHandler(STT.UI._progressHandlerId)
      STT.UI._progressHandlerId = nil
    end

    STT.UI._setDialogStatus("Download failed: " .. (errorMsg or "Unknown error"), "red")
    STT.UI._downloading = false

    if STT.UI._progressBar then
      STT.UI._progressBar:hide()
    end

    -- Unregister this error handler
    killAnonymousEventHandler(errorHandlerId)
  end)

  -- Track download progress (uses URL, not path)
  STT.UI._progressHandlerId = registerAnonymousEventHandler("sysDownloadFileProgress", function(event, url, downloaded, total)
    if url ~= model.url then return end
    if total and total > 0 and STT.UI._progressBar then
      local percent = math.floor((downloaded / total) * 100)
      STT.UI._progressBar:setValue(percent)
      STT.UI._setDialogStatus(string.format("Downloading %s... %d%%", model.name, percent), "cyan")
    end
  end)

  -- Start the download
  downloadFile(zipPath, model.url)
end

--- Hide the setup dialog.
function STT.UI.hideSetupDialog()
  STT.UI._cleanupSetupDialog()
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
