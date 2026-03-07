--- Speech-to-Text Module for Mudlet
-- Provides Lua-based control of speech recognition using Vosk library.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Speech">Mudlet Manual</a>
-- @module STT

STT = STT or {}

-- Internal state
STT._initialized = false
STT._callbacks = {
  onResult = nil,
  onPartial = nil,
  onStateChanged = nil,
  onError = nil,
}
STT._eventHandlerIds = {}
STT._defaultModelPath = nil

--- Check if speech-to-text is available on this system.
-- @return boolean true if STT is available
function STT.isAvailable()
  return stt and stt.isAvailable and stt.isAvailable()
end

--- Check if the speech recognizer is initialized.
-- @return boolean true if initialized
function STT.isInitialized()
  return stt and stt.isInitialized and stt.isInitialized()
end

--- Check if currently listening for speech.
-- @return boolean true if listening
function STT.isListening()
  return stt and stt.isListening and stt.isListening()
end

--- Initialize the speech recognizer with a model.
-- @param modelPath optional path to Vosk model directory
-- @return boolean, string success status and message
function STT.init(modelPath)
  if not STT.isAvailable() then
    return false, "Speech-to-text is not available on this system"
  end

  modelPath = modelPath or STT._defaultModelPath
  if modelPath then
    return stt.init(modelPath)
  else
    -- Let the C++ side use its default model path
    return stt.init()
  end
end

--- Start listening for speech.
-- @return boolean, string success status and message
function STT.start()
  if not STT.isAvailable() then
    return false, "Speech-to-text is not available"
  end
  return stt.start()
end

--- Stop listening for speech.
-- @return boolean, string success status and message
function STT.stop()
  if not STT.isAvailable() then
    return false, "Speech-to-text is not available"
  end
  return stt.stop()
end

--- Toggle speech recognition on/off.
-- @return boolean, string success status and message
function STT.toggle()
  if not STT.isAvailable() then
    return false, "Speech-to-text is not available"
  end
  return stt.toggle()
end

--- Close the speech recognizer and release resources.
-- @return boolean, string success status and message
function STT.close()
  if not STT.isAvailable() then
    return false, "Speech-to-text is not available"
  end
  return stt.close()
end

--- Get information about the speech recognizer.
-- @return table with keys: backend, modelPath, sensitivity, state, wordsEnabled
function STT.getInfo()
  if not STT.isAvailable() then
    return nil
  end
  return stt.getInfo()
end

--- Get the current model path.
-- @return string path to the current model
function STT.getModelPath()
  if not STT.isAvailable() then
    return nil
  end
  return stt.getModelPath()
end

--- List available Vosk models.
-- @return table array of model info tables
function STT.listModels()
  if not STT.isAvailable() then
    return {}
  end
  return stt.listModels()
end

--- Set callback for final speech recognition results.
-- @param callback function(text) to call when speech is recognized
function STT.setOnResult(callback)
  STT._callbacks.onResult = callback
end

--- Set callback for partial speech recognition results.
-- @param callback function(text) to call during ongoing speech
function STT.setOnPartial(callback)
  STT._callbacks.onPartial = callback
end

--- Set callback for state changes.
-- @param callback function(state) to call when state changes ('ready', 'listening', 'processing', 'error')
function STT.setOnStateChanged(callback)
  STT._callbacks.onStateChanged = callback
end

--- Set callback for errors.
-- @param callback function(errorMessage) to call when an error occurs
function STT.setOnError(callback)
  STT._callbacks.onError = callback
end

--- Set the default model path to use when init() is called without arguments.
-- @param path string path to Vosk model directory
function STT.setDefaultModelPath(path)
  STT._defaultModelPath = path
end

-- Internal event handlers
local function handlePartialResult(event, text)
  if STT._callbacks.onPartial then
    STT._callbacks.onPartial(text)
  end
end

local function handleStateChanged(event, state)
  if STT._callbacks.onStateChanged then
    STT._callbacks.onStateChanged(state)
  end
end

local function handleError(event, errorMessage)
  if STT._callbacks.onError then
    STT._callbacks.onError(errorMessage)
  end
end

--- Initialize event handlers (called automatically on load).
-- Users typically don't need to call this directly.
function STT._setupEventHandlers()
  if STT._initialized then
    return
  end

  -- Register event handlers
  if registerAnonymousEventHandler then
    STT._eventHandlerIds.partial = registerAnonymousEventHandler("sysSTTPartialResult", handlePartialResult)
    STT._eventHandlerIds.state = registerAnonymousEventHandler("sysSTTStateChanged", handleStateChanged)
    STT._eventHandlerIds.error = registerAnonymousEventHandler("sysSTTError", handleError)
  end

  STT._initialized = true
end

--- Remove event handlers (for cleanup).
-- Users typically don't need to call this directly.
function STT._removeEventHandlers()
  if not STT._initialized then
    return
  end

  -- Remove event handlers
  if killAnonymousEventHandler then
    for _, handlerId in pairs(STT._eventHandlerIds) do
      if handlerId then
        killAnonymousEventHandler(handlerId)
      end
    end
  end

  STT._eventHandlerIds = {}
  STT._initialized = false
end

-- Auto-initialize on load
STT._setupEventHandlers()
