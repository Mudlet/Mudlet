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
-- Format: { name, identifier, language, url, size (in MB) }
STT.UI._availableModels = {
  -- English models
  { name = "English (US) - Small", identifier = "vosk-model-small-en-us-0.15", language = "en",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip", size = 40 },
  { name = "English (US) - Large", identifier = "vosk-model-en-us-0.22", language = "en",
    url = "https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip", size = 1800 },
  -- German models
  { name = "German - Small", identifier = "vosk-model-small-de-0.15", language = "de",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-de-0.15.zip", size = 45 },
  { name = "German - Large", identifier = "vosk-model-de-0.21", language = "de",
    url = "https://alphacephei.com/vosk/models/vosk-model-de-0.21.zip", size = 1900 },
  -- Spanish models
  { name = "Spanish - Small", identifier = "vosk-model-small-es-0.42", language = "es",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-es-0.42.zip", size = 39 },
  -- French models
  { name = "French - Small", identifier = "vosk-model-small-fr-0.22", language = "fr",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip", size = 41 },
  { name = "French - Large", identifier = "vosk-model-fr-0.22", language = "fr",
    url = "https://alphacephei.com/vosk/models/vosk-model-fr-0.22.zip", size = 1400 },
  -- Italian models
  { name = "Italian - Small", identifier = "vosk-model-small-it-0.22", language = "it",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-it-0.22.zip", size = 48 },
  -- Portuguese models
  { name = "Portuguese - Small", identifier = "vosk-model-small-pt-0.3", language = "pt",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-pt-0.3.zip", size = 31 },
  -- Russian models
  { name = "Russian - Small", identifier = "vosk-model-small-ru-0.22", language = "ru",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-ru-0.22.zip", size = 45 },
  -- Chinese models
  { name = "Chinese - Small", identifier = "vosk-model-small-cn-0.22", language = "zh",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip", size = 42 },
  -- Japanese models
  { name = "Japanese - Small", identifier = "vosk-model-small-ja-0.22", language = "ja",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip", size = 48 },
  -- Polish models
  { name = "Polish - Small", identifier = "vosk-model-small-pl-0.22", language = "pl",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-pl-0.22.zip", size = 47 },
  -- Dutch models
  { name = "Dutch - Small", identifier = "vosk-model-small-nl-0.22", language = "nl",
    url = "https://alphacephei.com/vosk/models/vosk-model-small-nl-0.22.zip", size = 39 },
}

-- Pinned Vosk library builds. Hashes verified against the published artifacts.
-- macOS builds stopped at v0.3.42; Linux and Windows are on v0.3.45.
STT.UI._libraryBuilds = {
  ["macos"] = {
    url = "https://github.com/alphacep/vosk-api/releases/download/v0.3.42/vosk-osx-0.3.42.zip",
    sha256 = "65395f196c9d0583d79949142b25560acaf9c295f36284e18433097f3adb0ea1",
    libraryName = "libvosk.dylib",
    sizeBytes = 4702506,
  },
  ["windows-x64"] = {
    url = "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-win64-0.3.45.zip",
    sha256 = "f1dcc9cca460630f81ea8f71794f69c80bed6556d2a4e6237b5785e1d2dff34b",
    libraryName = "libvosk.dll",
    sizeBytes = 14882445,
  },
  ["linux-x86_64"] = {
    url = "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-x86_64-0.3.45.zip",
    sha256 = "bbdc8ed85c43979f6443142889770ea95cbfbc56cffb5c5dcd73afa875c5fbb2",
    libraryName = "libvosk.so",
    sizeBytes = 7185065,
  },
  ["linux-aarch64"] = {
    url = "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-aarch64-0.3.45.zip",
    sha256 = "45e95d37755deb07568e79497d7feba8c03aee5a9e071df29961aa023fd94541",
    libraryName = "libvosk.so",
    sizeBytes = 2367582,
  },
}

--- Select the Vosk library build for the current platform.
-- @return table build info, or nil when this platform has no published build
function STT.UI._libraryBuildForPlatform()
  local key = stt and stt.getPlatformKey and stt.getPlatformKey()
  if not key then
    return nil
  end
  return STT.UI._libraryBuilds[key]
end

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
-- @return Adjustable.Container the dialog container
function STT.UI.showSetupDialog()
  -- Always recreate the dialog to ensure the model list is fresh
  if STT.UI._setupDialog then
    STT.UI._cleanupSetupDialog()
  end

  -- Create main container using Adjustable.Container (provides title bar with close button)
  STT.UI._setupDialog = Adjustable.Container:new({
    name = "STT_SetupDialog",
    x = "10%", y = "2%",
    width = "80%", height = "96%",
    titleText = "Speech Recognition Setup",
    autoLoad = false,
    autoSave = false,
    adjLabelstyle = [[
      background-color: #1e1e1e;
      border: 2px solid #555;
    ]],
  })

  -- Content goes inside the container's Inside element
  local inside = STT.UI._setupDialog.Inside
  local yOffset = 5

  -- Status message area (for showing progress/errors)
  STT.UI._dialogStatus = Geyser.Label:new({
    name = "STT_SetupDialog_Status",
    x = 10, y = yOffset,
    width = "95%", height = 22,
    fgColor = "white",
    fontSize = 10,
  }, inside)
  STT.UI._dialogStatus:setStyleSheet([[background-color: #323232;]])
  STT.UI._dialogStatus:echo("")
  -- A status set just before a refresh is re-applied here, so the outcome of an
  -- action survives the rebuild that shows its result.
  if STT.UI._pendingStatus then
    STT.UI._dialogStatus:echo("<center>" .. STT.UI._pendingStatus.message .. "</center>",
                              STT.UI._pendingStatus.color or "white")
    STT.UI._pendingStatus = nil
  end
  yOffset = yOffset + 25

  -- Progress bar (hidden by default)
  STT.UI._progressBar = Geyser.Gauge:new({
    name = "STT_SetupDialog_Progress",
    x = 10, y = yOffset,
    width = "95%", height = 18,
  }, inside)
  STT.UI._progressBar:setStyleSheet([[
    background-color: #323232;
    border: 1px solid #666;
    border-radius: 3px;
  ]], [[
    background-color: #3c8c3c;
    border-radius: 3px;
  ]], [[]])
  STT.UI._progressBar:setValue(0)
  STT.UI._progressBar:hide()

  -- Status panel: current library / model / state
  for _, line in ipairs(STT.UI._statusLines()) do
    local row = Geyser.Label:new({
      name = "STT_SetupDialog_Status_" .. line.label,
      x = 10, y = yOffset,
      width = "95%", height = 20,
      fgColor = "#cccccc",
      fontSize = 9,
    }, inside)
    row:setStyleSheet([[background-color: #2a2a2a;]])
    row:echo(string.format("  <b>%s:</b> %s", line.label, line.value))
    yOffset = yOffset + 22
  end
  yOffset = yOffset + 8

  -- Section: Speech Recognition Library (only when the library is missing)
  if not STT.isAvailable() then
    local libTitle = Geyser.Label:new({
      name = "STT_SetupDialog_LibraryTitle",
      x = 10, y = yOffset,
      width = "95%", height = 22,
      fgColor = "#cccccc",
      fontSize = 10,
    }, inside)
    libTitle:setStyleSheet([[background-color: #3c3c3c;]])
    libTitle:echo("<b>Speech Recognition Library</b> (required)")
    yOffset = yOffset + 25

    local build = STT.UI._libraryBuildForPlatform()
    if build then
      local libBtn = Geyser.Label:new({
        name = "STT_SetupDialog_LibraryDownload",
        x = 10, y = yOffset,
        width = "95%", height = 26,
        fgColor = "white",
        fontSize = 10,
      }, inside)
      libBtn:setStyleSheet([[
        background-color: #32325a;
        border: 1px solid #55a;
        border-radius: 4px;
      ]])
      libBtn:echo(string.format("  Download library (~%d MB)", math.floor(build.sizeBytes / 1048576)))
      libBtn:setClickCallback(function()
        STT.UI._downloadLibrary()
      end)
      yOffset = yOffset + 30
    end

    local manualBtn = Geyser.Label:new({
      name = "STT_SetupDialog_LibraryManual",
      x = 10, y = yOffset,
      width = "95%", height = 26,
      fgColor = "white",
      fontSize = 10,
    }, inside)
    manualBtn:setStyleSheet([[
      background-color: #323232;
      border: 1px solid #666;
      border-radius: 4px;
    ]])
    manualBtn:echo("  Manual Install...")
    manualBtn:setClickCallback(function()
      openWebPage("https://alphacephei.com/vosk/install")
    end)
    yOffset = yOffset + 40
  end

  -- Section: Installed Models
  local installedTitle = Geyser.Label:new({
    name = "STT_SetupDialog_InstalledTitle",
    x = 10, y = yOffset,
    width = "95%", height = 22,
    fgColor = "#cccccc",
    fontSize = 10,
  }, inside)
  installedTitle:setStyleSheet([[background-color: #3c3c3c;]])
  installedTitle:echo("<b>Installed Models</b> (click to use)")
  yOffset = yOffset + 25

  -- List installed models
  local installedModels = STT.listModels()

  if #installedModels == 0 then
    local noModels = Geyser.Label:new({
      name = "STT_SetupDialog_NoInstalled",
      x = 10, y = yOffset,
      width = "95%", height = 26,
      fgColor = "#999999",
      fontSize = 10,
    }, inside)
    noModels:setStyleSheet([[background-color: #323232;]])
    noModels:echo("<center><i>No models installed yet. Download one below.</i></center>")
    yOffset = yOffset + 30
  else
    for i, model in ipairs(installedModels) do
      local modelBtn = Geyser.Label:new({
        name = "STT_SetupDialog_Installed_" .. i,
        x = 10, y = yOffset,
        width = "80%", height = 26,
        fgColor = "white",
        fontSize = 10,
      }, inside)
      modelBtn:setStyleSheet([[
        background-color: #285028;
        border: 1px solid #4a4;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s", model.name or model.path))
      modelBtn:setClickCallback(function()
        -- Loading a model is synchronous and can take a while for the large
        -- ones, so say what is happening before starting.
        STT.UI._setDialogStatus("Loading " .. (model.name or model.path) .. "...", "cyan")
        local success, errorMsg = STT.init(model.path)
        if success then
          -- Refresh rather than close: the status panel then shows the newly
          -- loaded model and the recognizer's state.
          STT.UI._refreshWithStatus("Model loaded: " .. (model.name or model.path), "#00ff00")
        else
          STT.UI._setDialogStatus("Failed: " .. (errorMsg or "Unknown error"), "red")
        end
      end)

      local removeBtn = Geyser.Label:new({
        name = "STT_SetupDialog_RemoveModel_" .. i,
        x = "82%", y = yOffset,
        width = "13%", height = 26,
        fgColor = "white",
        fontSize = 10,
      }, inside)
      removeBtn:setStyleSheet([[
        background-color: #5a2828;
        border: 1px solid #a44;
        border-radius: 4px;
      ]])
      local removeBtnStyle = [[
        background-color: #5a2828;
        border: 1px solid #a44;
        border-radius: 4px;
      ]]
      removeBtn:echo("<center>remove</center>")
      removeBtn:setClickCallback(function()
        STT.UI._armConfirm("model:" .. (model.path or tostring(i)), function()
          local ok, err = STT.UI._removeModel(model)
          if ok then
            STT.UI._refreshWithStatus("Model removed", "#00ff00")
          else
            STT.UI._refreshWithStatus("Could not remove model: " .. (err or "unknown error"), "red")
          end
        end, {
          label = removeBtn,
          text = "<center>remove</center>",
          style = removeBtnStyle,
          armedText = "<center><b>confirm?</b></center>",
        })
      end)
      yOffset = yOffset + 30
    end
  end

  yOffset = yOffset + 10

  -- Only our own copy of the library can be removed; one found on the system
  -- search path is not ours to delete and the attempt would only fail.
  if STT.isAvailable() and STT.UI._libraryIsOurs() then
    local removeLib = Geyser.Label:new({
      name = "STT_SetupDialog_RemoveLibrary",
      x = 10, y = yOffset,
      width = "95%", height = 26,
      fgColor = "white",
      fontSize = 10,
    }, inside)
    local removeLibStyle = [[
      background-color: #5a2828;
      border: 1px solid #a44;
      border-radius: 4px;
    ]]
    removeLib:setStyleSheet(removeLibStyle)
    removeLib:echo("  Remove speech recognition library")
    removeLib:setClickCallback(function()
      STT.UI._armConfirm("library", function()
        local ok, err = STT.UI._removeLibrary()
        if ok then
          STT.UI._refreshWithStatus("Library removed", "#00ff00")
        else
          STT.UI._refreshWithStatus("Could not remove library: " .. (err or "unknown error"), "red")
        end
      end, {
        label = removeLib,
        text = "  Remove speech recognition library",
        style = removeLibStyle,
      })
    end)
    yOffset = yOffset + 30
  end

  local resetRow = Geyser.Label:new({
    name = "STT_SetupDialog_ResetAll",
    x = 10, y = yOffset,
    width = "95%", height = 26,
    fgColor = "white",
    fontSize = 10,
  }, inside)
  local resetRowStyle = [[
    background-color: #4a2a2a;
    border: 1px solid #844;
    border-radius: 4px;
  ]]
  resetRow:setStyleSheet(resetRowStyle)
  resetRow:echo("  Reset everything (remove library and all models)")
  resetRow:setClickCallback(function()
    STT.UI._armConfirm("reset", function()
      local ok, summary = STT.UI._resetEverything()
      STT.UI._refreshWithStatus(ok and "Reset complete" or (summary or "Reset failed"), ok and "#00ff00" or "red")
    end, {
      label = resetRow,
      text = "  Reset everything (remove library and all models)",
      style = resetRowStyle,
    })
  end)
  yOffset = yOffset + 40

  -- Section: Download Models
  local downloadTitle = Geyser.Label:new({
    name = "STT_SetupDialog_DownloadTitle",
    x = 10, y = yOffset,
    width = "95%", height = 22,
    fgColor = "#cccccc",
    fontSize = 10,
  }, inside)
  downloadTitle:setStyleSheet([[background-color: #3c3c3c;]])
  downloadTitle:echo("<b>Download New Model</b> (click to download)")
  yOffset = yOffset + 25

  -- Add downloadable models directly
  for i, model in ipairs(STT.UI._availableModels) do
    -- Check if already installed
    local isInstalled = STT.UI._isModelInstalled(model.identifier)
    local sizeStr = model.size < 100 and string.format("~%d MB", model.size) or string.format("~%.1f GB", model.size / 1024)

    if isInstalled then
      local modelBtn = Geyser.Label:new({
        name = "STT_SetupDialog_Download_" .. i,
        x = 10, y = yOffset,
        width = "95%", height = 28,
        fgColor = "#777777",
        fontSize = 10,
      }, inside)
      modelBtn:setStyleSheet([[
        background-color: #3c3c3c;
        border: 1px solid #555;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s (%s) <i>[installed]</i>", model.name, sizeStr))
    else
      local modelBtn = Geyser.Label:new({
        name = "STT_SetupDialog_Download_" .. i,
        x = 10, y = yOffset,
        width = "95%", height = 28,
        fgColor = "white",
        fontSize = 10,
      }, inside)
      modelBtn:setStyleSheet([[
        background-color: #323250;
        border: 1px solid #668;
        border-radius: 4px;
      ]])
      modelBtn:echo(string.format("  %s (%s)", model.name, sizeStr))
      modelBtn:setClickCallback(function()
        STT.UI._downloadModel(model)
      end)
    end
    yOffset = yOffset + 32
  end

  -- Ensure dialog is visible and raised to front
  STT.UI._setupDialog:show()
  raiseWindow("STT_SetupDialog")

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
  if STT.UI._unzipDoneHandlerId then
    killAnonymousEventHandler(STT.UI._unzipDoneHandlerId)
    STT.UI._unzipDoneHandlerId = nil
  end
  if STT.UI._unzipErrorHandlerId then
    killAnonymousEventHandler(STT.UI._unzipErrorHandlerId)
    STT.UI._unzipErrorHandlerId = nil
  end
  if STT.UI._errorHandlerId then
    killAnonymousEventHandler(STT.UI._errorHandlerId)
    STT.UI._errorHandlerId = nil
  end
  if STT.UI._libDownloadHandlerId then
    killAnonymousEventHandler(STT.UI._libDownloadHandlerId)
    STT.UI._libDownloadHandlerId = nil
  end
  if STT.UI._libUnzipDoneId then
    killAnonymousEventHandler(STT.UI._libUnzipDoneId)
    STT.UI._libUnzipDoneId = nil
  end
  if STT.UI._libUnzipErrorId then
    killAnonymousEventHandler(STT.UI._libUnzipErrorId)
    STT.UI._libUnzipErrorId = nil
  end
  if STT.UI._libErrorHandlerId then
    killAnonymousEventHandler(STT.UI._libErrorHandlerId)
    STT.UI._libErrorHandlerId = nil
  end
  STT.UI._downloadingLibrary = false
  if STT.UI._setupDialog then
    -- Hide rather than delete. Adjustable.Container keeps its own registries
    -- (Adjustable.Container.all and .Attached) and does not override the
    -- inherited Geyser.Container:delete(), so deleting leaves those entries
    -- pointing at a destroyed object and the next container of the same name
    -- comes up broken.
    STT.UI._setupDialog:hide()
  end
  STT.UI._setupDialog = nil
  STT.UI._dialogStatus = nil
  STT.UI._progressBar = nil
  STT.UI._downloading = false
  STT.UI._pendingConfirm = nil
  -- The armed row's widget is destroyed with the dialog, so drop the reference
  -- rather than reverting it.
  STT.UI._pendingConfirmRow = nil
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

--- Remove a file, or a directory and everything under it.
-- Entries are collected before deletion because mutating a directory while
-- iterating it with lfs.dir is undefined.
-- @param path string the path to remove
-- @return boolean ok, string errorMessage
function STT.UI._removePath(path)
  -- symlinkattributes, not attributes: a symlink reports as whatever it points
  -- at, and descending into one would delete files outside the tree being
  -- removed (and could loop forever on a cycle). Unlinking the link is enough.
  local mode = lfs.symlinkattributes(path, "mode")
  if not mode then
    return false, "does not exist: " .. tostring(path)
  end

  if mode ~= "directory" then
    local ok, err = os.remove(path)
    if not ok then
      return false, err or ("could not remove " .. path)
    end
    return true
  end

  -- lfs.dir raises on a directory it cannot open rather than returning nil, and
  -- this runs inside a click callback where a raised error is not recoverable.
  local entries = {}
  local listed, listError = pcall(function()
    for entry in lfs.dir(path) do
      if entry ~= "." and entry ~= ".." then
        entries[#entries + 1] = entry
      end
    end
  end)
  if not listed then
    return false, "could not read " .. path .. ": " .. tostring(listError)
  end

  local failures = {}
  for _, entry in ipairs(entries) do
    local ok, err = STT.UI._removePath(path .. "/" .. entry)
    if not ok then
      failures[#failures + 1] = err
    end
  end

  if #failures > 0 then
    return false, table.concat(failures, "; ")
  end

  local ok, err = lfs.rmdir(path)
  if not ok then
    return false, err or ("could not remove directory " .. path)
  end
  return true
end

--- Whether the loaded library is the copy Mudlet installed into its own
--- directory, rather than one picked up from the system search path. Only our
--- own copy can be described by, or removed from, this dialog.
-- The first library search path is the file the recognizer looks for inside
-- stt.getLibraryPath(), so its presence is what makes the library ours.
-- @param info table optional, an already-fetched stt.getInfo() result
-- @return boolean ours, string the library file path when it is ours
function STT.UI._libraryIsOurs(info)
  info = info or (stt and stt.getInfo and stt.getInfo()) or {}
  local expected = info.searchPaths and info.searchPaths[1]
  if not expected or expected == "" then
    return false
  end
  if not lfs.attributes(expected, "mode") then
    return false
  end
  return true, expected
end

--- Build the dialog's status lines describing what is currently in use.
-- @return table array of {label, value} pairs
function STT.UI._statusLines()
  local info = (stt and stt.getInfo and stt.getInfo()) or {}
  local lines = {}

  if info.available then
    local ours, libraryFile = STT.UI._libraryIsOurs(info)
    local where = "installed elsewhere on this system"
    if ours then
      where = (stt and stt.getLibraryPath and stt.getLibraryPath()) or libraryFile
    end
    -- The version is only reported once a recognizer object exists, and is a
    -- placeholder even then, so omit it rather than claiming it is unknown.
    local version = info.version
    if version and version ~= "" then
      lines[#lines + 1] = {
        label = "Library",
        value = string.format("Installed (%s) - %s", version, where),
      }
    else
      lines[#lines + 1] = {label = "Library", value = string.format("Installed - %s", where)}
    end
  else
    lines[#lines + 1] = {label = "Library", value = "Not installed"}
  end

  -- info.modelPath is the model actually loaded; stt.getModelPath() is the
  -- directory models are installed into and would name every model the same.
  local modelPath = info.modelPath
  if info.initialized and modelPath and modelPath ~= "" then
    lines[#lines + 1] = {label = "Model", value = modelPath:match("([^/\\]+)$") or modelPath}
  else
    lines[#lines + 1] = {label = "Model", value = "None loaded"}
  end

  local state = "Idle"
  if info.listening then
    state = "Listening"
  elseif info.state == "error" then
    state = "Error"
  elseif info.initialized then
    state = "Ready"
  end
  lines[#lines + 1] = {label = "State", value = state}

  return lines
end

--- Set the dialog status message.
-- @param message string the message to display
-- @param color string optional color name
function STT.UI._setDialogStatus(message, color)
  if not STT.UI._dialogStatus then
    return
  end
  local html = "<center>" .. message .. "</center>"
  -- Geyser raises if it cannot parse the colour, and this is called on paths
  -- that go on to rebuild the dialog, so a bad colour must not abort the
  -- caller. Fall back to the label's own colour.
  if not pcall(function() STT.UI._dialogStatus:echo(html, color or "white") end) then
    pcall(function() STT.UI._dialogStatus:echo(html) end)
  end
end

--- Require two clicks before performing a destructive action.
-- @param key string identifies the row being armed
-- @param onConfirm function run on the second click
-- @return boolean whether the action was performed
--- Rebuild the dialog immediately, carrying a status message across the rebuild.
-- Used after a destructive action so the affected row disappears and the
-- sections below it reflect the new state at once, while the outcome stays
-- readable.
-- @param message string status to show after the rebuild
-- @param color string optional colour for the status
function STT.UI._refreshWithStatus(message, color)
  -- Shown at once so the outcome is visible immediately. Harmless when no
  -- dialog is open, where _setDialogStatus is a no-op.
  STT.UI._setDialogStatus(message, color)
  if not STT.UI._setupDialog then
    return
  end
  -- Stored so the rebuild that follows re-applies it rather than blanking it.
  STT.UI._pendingStatus = {message = message, color = color}
  -- Deferred by one event-loop tick rather than rebuilt inline: this runs from
  -- a row's click callback, and rebuilding tears down the very widget whose
  -- callback is still executing. A zero-delay timer is still visually
  -- immediate.
  tempTimer(0, function()
    if STT.UI._setupDialog then
      STT.UI.showSetupDialog()
    else
      STT.UI._pendingStatus = nil
    end
  end)
end

--- Restore an armed row to its original text and styling.
-- @param row table as passed to _armConfirm
function STT.UI._revertConfirmRow(row)
  if not row or not row.label then
    return
  end
  row.label:setStyleSheet(row.style)
  row.label:echo(row.text)
end

function STT.UI._armConfirm(key, onConfirm, row)
  if STT.UI._pendingConfirm == key then
    STT.UI._revertConfirmRow(STT.UI._pendingConfirmRow)
    STT.UI._pendingConfirm = nil
    STT.UI._pendingConfirmRow = nil
    onConfirm()
    return true
  end

  -- Arming a different row disarms the previous one, so only ever one row
  -- shows as armed.
  STT.UI._revertConfirmRow(STT.UI._pendingConfirmRow)

  STT.UI._pendingConfirm = key
  STT.UI._pendingConfirmRow = row
  if row and row.label then
    row.label:setStyleSheet([[
      background-color: #a02020;
      border: 2px solid #ff6666;
      border-radius: 4px;
    ]])
    row.label:echo(row.armedText or "<center><b>Click again to confirm</b></center>")
  end

  STT.UI._setDialogStatus("Click again to confirm", "red")
  tempTimer(5, function()
    if STT.UI._pendingConfirm == key then
      STT.UI._revertConfirmRow(STT.UI._pendingConfirmRow)
      STT.UI._pendingConfirm = nil
      STT.UI._pendingConfirmRow = nil
      STT.UI._setDialogStatus("")
    end
  end)
  return false
end

--- Normalise a filesystem path for defensive comparison: strip a trailing
--- separator and lowercase the result. Not a full canonicalisation - no
--- symlink resolution.
-- Lowercasing unconditionally can call two distinct paths equal on a
-- case-sensitive filesystem; that is deliberate, since the only cost is
-- refusing to delete a model, and the alternative cost is deleting one that is
-- in use.
-- @param path string
-- @return string normalisedPath
function STT.UI._normalizePathForCompare(path)
  return (path or ""):gsub("[/\\]+$", ""):lower()
end

--- Remove an installed model from disk.
-- @param model table with name and path
-- @return boolean ok, string errorMessage
function STT.UI._removeModel(model)
  local info = (stt and stt.getInfo and stt.getInfo()) or {}
  if info.listening then
    return false, "stop speech recognition before removing a model"
  end

  local activePath = info.modelPath
  if info.initialized and activePath and activePath ~= "" and
     STT.UI._normalizePathForCompare(activePath) == STT.UI._normalizePathForCompare(model.path) then
    return false, "that model is currently loaded - close speech recognition first"
  end

  local ok, err = STT.UI._removePath(model.path)
  if not ok then
    return false, err
  end
  return true
end

--- Remove the installed Vosk library from disk, unloading it first.
-- @return boolean ok, string errorMessage
function STT.UI._removeLibrary()
  local info = (stt and stt.getInfo and stt.getInfo()) or {}
  if info.listening then
    return false, "stop speech recognition before removing the library"
  end

  local libDir = stt and stt.getLibraryPath and stt.getLibraryPath()
  if not libDir then
    return false, "could not determine the library location"
  end

  if stt and stt.close then
    stt.close()
  end

  -- stt.close() releases the model and recognizer but leaves the shared library
  -- mapped, and Windows will not delete a mapped module, so it has to come out
  -- of the process before its file is touched.
  if stt and stt.unloadLibrary then
    local unloaded, unloadError = stt.unloadLibrary()
    if not unloaded then
      return false, unloadError or "could not unload the speech recognition library"
    end
  end

  local ok, err = STT.UI._removePath(libDir)
  if not ok then
    return false, err
  end

  -- A message here means the reload was refused, which leaves the library
  -- reporting as loaded even though its files are gone - the dialog would show
  -- it as installed and hide the install section, so say so instead.
  if stt and stt.reloadLibrary then
    local _, reloadError = stt.reloadLibrary()
    if reloadError then
      return false, "the library files were removed, but: " .. reloadError
    end
  end
  return true
end

--- Remove every installed model and the library, returning to a clean state.
-- @return boolean ok, string summary
function STT.UI._resetEverything()
  local info = (stt and stt.getInfo and stt.getInfo()) or {}
  if info.listening then
    return false, "stop speech recognition before resetting"
  end

  -- Close first: the recognizer holds native handles into the loaded model's
  -- directory, and removing a model out from under them would be a use after
  -- free. It also releases the model whose removal would otherwise be refused.
  if stt and stt.close then
    stt.close()
  end

  local failures = {}
  for _, model in ipairs(STT.listModels()) do
    local ok, err = STT.UI._removeModel(model)
    if not ok then
      failures[#failures + 1] = string.format("%s (%s)", model.name or model.path, err or "unknown error")
    end
  end

  local ok, err = STT.UI._removeLibrary()
  if not ok then
    failures[#failures + 1] = string.format("library (%s)", err or "unknown error")
  end

  if #failures > 0 then
    return false, "could not remove: " .. table.concat(failures, ", ")
  end
  return true, "Everything removed"
end

--- Download and install a Vosk model.
-- @param model table the model info from _availableModels
function STT.UI._downloadModel(model)
  if STT.UI._downloading then
    STT.UI._setDialogStatus("Download already in progress...", "yellow")
    return
  end

  -- Reject non-HTTPS URLs to prevent man-in-the-middle tampering of model archives,
  -- which are consumed by a native library.
  if not model.url or not model.url:lower():match("^https://") then
    STT.UI._setDialogStatus("Refusing to download model: URL must use HTTPS", "red")
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

  -- Create directory if needed (check if it exists first since mkdir fails on existing dirs)
  local dirAttr = lfs.attributes(modelDir, "mode")
  if not dirAttr then
    local ok, errMsg = lfs.mkdir(modelDir)
    if not ok then
      STT.UI._setDialogStatus("Failed to create model directory: " .. (errMsg or "unknown error"), "red")
      STT.UI._downloading = false
      return
    end
  elseif dirAttr ~= "directory" then
    STT.UI._setDialogStatus("Model path exists but is not a directory: " .. modelDir, "red")
    STT.UI._downloading = false
    return
  end

  local zipPath = modelDir .. "/" .. model.identifier .. ".zip"
  local extractPath = modelDir

  -- Register download completion handler
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
    if STT.UI._errorHandlerId then
      killAnonymousEventHandler(STT.UI._errorHandlerId)
      STT.UI._errorHandlerId = nil
    end

    STT.UI._setDialogStatus("Extracting model...", "cyan")

    -- Basic integrity check: verify downloaded zip size is within a sane range of the
    -- expected size (model.size is in MB). This catches truncated/corrupted downloads
    -- and obvious substitutions but is not a substitute for a real checksum.
    if model.size and model.size > 0 then
      local attr = lfs.attributes(zipPath)
      local actualBytes = attr and attr.size or 0
      local expectedBytes = model.size * 1024 * 1024
      local minBytes = math.floor(expectedBytes * 0.25)
      local maxBytes = math.floor(expectedBytes * 3.0)
      if actualBytes < minBytes or actualBytes > maxBytes then
        STT.UI._setDialogStatus(string.format(
          "Downloaded model size (%d bytes) is outside the expected range for %s (~%d MB); aborting.",
          actualBytes, model.name, model.size), "red")
        STT.UI._downloading = false
        os.remove(zipPath)
        return
      end
    end

    -- Register unzip completion handler
    STT.UI._unzipDoneHandlerId = registerAnonymousEventHandler("sysUnzipDone", function(evt, zip, dest)
      if zip ~= zipPath then return end

      if STT.UI._unzipDoneHandlerId then
        killAnonymousEventHandler(STT.UI._unzipDoneHandlerId)
        STT.UI._unzipDoneHandlerId = nil
      end
      if STT.UI._unzipErrorHandlerId then
        killAnonymousEventHandler(STT.UI._unzipErrorHandlerId)
        STT.UI._unzipErrorHandlerId = nil
      end

      -- Clean up zip file
      os.remove(zipPath)

      STT.UI._downloading = false

      if STT.UI._progressBar then
        STT.UI._progressBar:setValue(100)
      end

      STT.UI._refreshWithStatus("Model installed successfully!", "#00ff00")
    end)

    STT.UI._unzipErrorHandlerId = registerAnonymousEventHandler("sysUnzipError", function(evt, zip, dest)
      if zip ~= zipPath then return end

      if STT.UI._unzipDoneHandlerId then
        killAnonymousEventHandler(STT.UI._unzipDoneHandlerId)
        STT.UI._unzipDoneHandlerId = nil
      end
      if STT.UI._unzipErrorHandlerId then
        killAnonymousEventHandler(STT.UI._unzipErrorHandlerId)
        STT.UI._unzipErrorHandlerId = nil
      end

      STT.UI._setDialogStatus("Failed to extract model archive", "red")
      STT.UI._downloading = false
      os.remove(zipPath)
    end)

    -- unzipAsync can fail synchronously without ever raising an event, which
    -- would leave the dialog waiting on "Extracting model..." with _downloading
    -- still set, refusing every retry - so check its return value, as the
    -- library install does.
    local started, unzipErr = unzipAsync(zipPath, extractPath)
    if not started then
      if STT.UI._unzipDoneHandlerId then
        killAnonymousEventHandler(STT.UI._unzipDoneHandlerId)
        STT.UI._unzipDoneHandlerId = nil
      end
      if STT.UI._unzipErrorHandlerId then
        killAnonymousEventHandler(STT.UI._unzipErrorHandlerId)
        STT.UI._unzipErrorHandlerId = nil
      end
      STT.UI._downloading = false
      os.remove(zipPath)
      STT.UI._setDialogStatus("Could not start extraction: " .. tostring(unzipErr), "red")
    end
  end)

  -- Register download error handler
  STT.UI._errorHandlerId = registerAnonymousEventHandler("sysDownloadError", function(event, errorMsg, path)
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
    if STT.UI._errorHandlerId then
      killAnonymousEventHandler(STT.UI._errorHandlerId)
      STT.UI._errorHandlerId = nil
    end
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

--- Verify a downloaded library archive against its pinned digest.
-- @param path string archive path
-- @param build table build info carrying the expected sha256
-- @return boolean ok, string errorMessage
function STT.UI._verifyDownloadedLibrary(path, build)
  local digest, err = hashFile(path, "sha256")
  if not digest then
    return false, err or "could not hash the downloaded archive"
  end
  if digest:lower() ~= build.sha256:lower() then
    return false, string.format("checksum mismatch: expected %s, got %s", build.sha256, digest)
  end
  return true
end

--- Move the extracted library (and on Windows its runtime DLLs) up into libDir.
-- The archives nest their payload in a versioned directory, one level deeper
-- than the library search path looks.
-- @param libDir string the vosk-lib directory
-- @param build table build info carrying libraryName
-- @return boolean ok, string errorMessage
function STT.UI._installExtractedLibrary(libDir, build)
  -- Collect the listings before moving anything: renaming into a directory that
  -- lfs.dir() is still iterating is a readdir-during-mutation.
  -- Wrapped as in _removePath: lfs.dir raises on a directory it cannot open, and
  -- this runs from the sysUnzipDone handler, where a raise would stop the dialog
  -- ever being told how the install ended.
  local nestedDirs = {}
  local listed, listError = pcall(function()
    for entry in lfs.dir(libDir) do
      if entry ~= "." and entry ~= ".." then
        local nested = libDir .. "/" .. entry
        if lfs.attributes(nested, "mode") == "directory" then
          local contents = {}
          for inner in lfs.dir(nested) do
            if inner ~= "." and inner ~= ".." then
              contents[#contents + 1] = inner
            end
          end
          nestedDirs[#nestedDirs + 1] = {path = nested, contents = contents}
        end
      end
    end
  end)
  if not listed then
    -- listError names the directory that could not be read, which may be a
    -- versioned subdirectory rather than libDir itself
    return false, "could not read the extracted library: " .. tostring(listError)
  end

  local moved = false
  for _, nested in ipairs(nestedDirs) do
    for _, inner in ipairs(nested.contents) do
      local isLibrary = inner == build.libraryName
      local isRuntimeDll = inner:match("%.dll$") ~= nil
      if isLibrary or isRuntimeDll then
        local ok, err = os.rename(nested.path .. "/" .. inner, libDir .. "/" .. inner)
        if not ok then
          return false, string.format("could not move %s: %s", inner, tostring(err))
        end
        if isLibrary then
          moved = true
        end
      end
    end
  end

  if not moved then
    return false, "the archive did not contain " .. build.libraryName
  end

  -- Discard what the archive shipped alongside the library (headers, examples)
  -- and the now-empty versioned directories, so the install leaves no residue.
  for _, nested in ipairs(nestedDirs) do
    for _, inner in ipairs(nested.contents) do
      -- _removePath, not os.remove: the archives ship directories alongside the
      -- library, and os.remove fails on a non-empty one, which would leave the
      -- versioned directory non-empty so the rmdir below fails too.
      STT.UI._removePath(nested.path .. "/" .. inner)
    end
    lfs.rmdir(nested.path)
  end

  return true
end

--- Download, verify and install the Vosk library for this platform.
function STT.UI._downloadLibrary()
  if STT.UI._downloadingLibrary then
    STT.UI._setDialogStatus("Library download already in progress...", "yellow")
    return
  end

  local build = STT.UI._libraryBuildForPlatform()
  if not build then
    STT.UI._setDialogStatus("No prebuilt library for this platform - use Manual Install", "yellow")
    return
  end

  if not build.url:find("^https://") then
    STT.UI._setDialogStatus("Refusing to download over an insecure connection", "red")
    return
  end

  -- The library has to land in the one directory the recognizer searches, which
  -- only the C++ side knows.
  if not (stt and stt.getLibraryPath) then
    STT.UI._setDialogStatus("This Mudlet version cannot install the library - use Manual Install", "yellow")
    return
  end

  local libDir = stt.getLibraryPath()
  if not lfs.attributes(libDir, "mode") then
    local made, mkErr = lfs.mkdir(libDir)
    if not made then
      STT.UI._setDialogStatus("Could not create " .. libDir .. ": " .. (mkErr or "unknown error"), "red")
      return
    end
  end

  local zipPath = libDir .. "/vosk-library.zip"
  STT.UI._downloadingLibrary = true
  -- Identifies this attempt, so that a timer left over from an earlier one
  -- cannot report on, or clobber the status of, a newer attempt.
  STT.UI._libAttempt = (STT.UI._libAttempt or 0) + 1
  local attempt = STT.UI._libAttempt
  STT.UI._setDialogStatus("Downloading speech recognition library...", "cyan")

  STT.UI._libDownloadHandlerId = registerAnonymousEventHandler("sysDownloadDone", function(_, path)
    if path ~= zipPath then return end
    if STT.UI._libDownloadHandlerId then
      killAnonymousEventHandler(STT.UI._libDownloadHandlerId)
      STT.UI._libDownloadHandlerId = nil
    end
    -- The download is over, so its error handler is done regardless of how the
    -- rest of this function turns out.
    if STT.UI._libErrorHandlerId then
      killAnonymousEventHandler(STT.UI._libErrorHandlerId)
      STT.UI._libErrorHandlerId = nil
    end

    STT.UI._setDialogStatus("Verifying library...", "cyan")
    local ok, err = STT.UI._verifyDownloadedLibrary(zipPath, build)
    if not ok then
      os.remove(zipPath)
      STT.UI._downloadingLibrary = false
      STT.UI._setDialogStatus("Library verification failed: " .. err, "red")
      return
    end

    STT.UI._setDialogStatus("Extracting library...", "cyan")
    STT.UI._libUnzipDoneId = registerAnonymousEventHandler("sysUnzipDone", function(_, zip)
      if zip ~= zipPath then return end
      if STT.UI._libUnzipDoneId then
        killAnonymousEventHandler(STT.UI._libUnzipDoneId)
        STT.UI._libUnzipDoneId = nil
      end
      if STT.UI._libUnzipErrorId then
        killAnonymousEventHandler(STT.UI._libUnzipErrorId)
        STT.UI._libUnzipErrorId = nil
      end
      os.remove(zipPath)
      STT.UI._downloadingLibrary = false

      local installed, installErr = STT.UI._installExtractedLibrary(libDir, build)
      if not installed then
        STT.UI._refreshWithStatus("Could not install the library: " .. installErr, "red")
        return
      end

      local available, reloadErr = stt.reloadLibrary()
      if available then
        STT.UI._refreshWithStatus("Speech recognition library installed!", "#00ff00")
      elseif reloadErr then
        STT.UI._refreshWithStatus("Library installed, but: " .. reloadErr, "yellow")
      else
        STT.UI._refreshWithStatus("Library installed but could not be loaded - see Manual Install", "yellow")
      end
    end)

    STT.UI._libUnzipErrorId = registerAnonymousEventHandler("sysUnzipError", function(_, zip)
      if zip ~= zipPath then return end
      if STT.UI._libUnzipDoneId then
        killAnonymousEventHandler(STT.UI._libUnzipDoneId)
        STT.UI._libUnzipDoneId = nil
      end
      if STT.UI._libUnzipErrorId then
        killAnonymousEventHandler(STT.UI._libUnzipErrorId)
        STT.UI._libUnzipErrorId = nil
      end
      os.remove(zipPath)
      STT.UI._downloadingLibrary = false
      STT.UI._setDialogStatus("Could not extract the library archive", "red")
    end)

    -- unzipAsync can fail synchronously without ever raising an event, which
    -- would leave this dialog waiting forever, so check its return value.
    local started, unzipErr = unzipAsync(zipPath, libDir)
    if not started then
      if STT.UI._libUnzipDoneId then
        killAnonymousEventHandler(STT.UI._libUnzipDoneId)
        STT.UI._libUnzipDoneId = nil
      end
      if STT.UI._libUnzipErrorId then
        killAnonymousEventHandler(STT.UI._libUnzipErrorId)
        STT.UI._libUnzipErrorId = nil
      end
      STT.UI._downloadingLibrary = false
      STT.UI._setDialogStatus("Could not start extraction: " .. tostring(unzipErr), "red")
      return
    end

    tempTimer(120, function()
      -- A timer outliving its own attempt must not speak for a newer one
      if attempt ~= STT.UI._libAttempt then return end
      if STT.UI._libUnzipDoneId or STT.UI._libUnzipErrorId then
        if STT.UI._libUnzipDoneId then
          killAnonymousEventHandler(STT.UI._libUnzipDoneId)
          STT.UI._libUnzipDoneId = nil
        end
        if STT.UI._libUnzipErrorId then
          killAnonymousEventHandler(STT.UI._libUnzipErrorId)
          STT.UI._libUnzipErrorId = nil
        end
        STT.UI._downloadingLibrary = false
        STT.UI._setDialogStatus("Extraction timed out - use Manual Install", "red")
      end
    end)
  end)

  STT.UI._libErrorHandlerId = registerAnonymousEventHandler("sysDownloadError", function(_, errorMsg, path)
    if path ~= zipPath then return end
    if STT.UI._libDownloadHandlerId then
      killAnonymousEventHandler(STT.UI._libDownloadHandlerId)
      STT.UI._libDownloadHandlerId = nil
    end
    if STT.UI._libErrorHandlerId then
      killAnonymousEventHandler(STT.UI._libErrorHandlerId)
      STT.UI._libErrorHandlerId = nil
    end
    STT.UI._downloadingLibrary = false
    STT.UI._setDialogStatus("Library download failed: " .. (errorMsg or "unknown error"), "red")
  end)

  downloadFile(zipPath, build.url)
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
