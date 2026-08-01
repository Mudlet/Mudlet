describe("tests the functionality of the STT module", function()
  describe("Tests the functionality of STT.listModels", function()
    local realIsAvailable, realListModels

    before_each(function()
      realIsAvailable = stt.isAvailable
      realListModels = stt.listModels
    end)

    after_each(function()
      stt.isAvailable = realIsAvailable
      stt.listModels = realListModels
    end)

    it("should list models on disk even when the Vosk library is unavailable", function()
      stt.isAvailable = function() return false end
      stt.listModels = function()
        return {{name = "vosk-model-small-en-us-0.15", path = "/models/vosk-model-small-en-us-0.15"}}
      end

      local models = STT.listModels()
      assert.equals(1, #models)
      assert.equals("vosk-model-small-en-us-0.15", models[1].name)
    end)

    it("should list models on disk when the Vosk library is available", function()
      stt.isAvailable = function() return true end
      stt.listModels = function()
        return {{name = "vosk-model-en-us-0.22", path = "/models/vosk-model-en-us-0.22"}}
      end

      local models = STT.listModels()
      assert.equals(1, #models)
      assert.equals("vosk-model-en-us-0.22", models[1].name)
    end)

    it("should return an empty table when the stt.listModels binding is missing", function()
      stt.isAvailable = function() return false end
      stt.listModels = nil

      assert.equals(0, #STT.listModels())
    end)
  end)

  describe("Tests the functionality of STT.UI._isModelInstalled", function()
    local realIsAvailable, realListModels

    before_each(function()
      realIsAvailable = stt.isAvailable
      realListModels = stt.listModels
    end)

    after_each(function()
      stt.isAvailable = realIsAvailable
      stt.listModels = realListModels
    end)

    it("should detect an installed model when the Vosk library is unavailable", function()
      stt.isAvailable = function() return false end
      stt.listModels = function()
        return {{name = "vosk-model-small-en-us-0.15", path = "/models/vosk-model-small-en-us-0.15"}}
      end

      assert.is_true(STT.UI._isModelInstalled("vosk-model-small-en-us-0.15"))
    end)

    it("should not report a model that is not on disk", function()
      stt.isAvailable = function() return false end
      stt.listModels = function() return {} end

      assert.is_false(STT.UI._isModelInstalled("vosk-model-small-en-us-0.15"))
    end)
  end)

  describe("Tests the functionality of hashFile", function()
    local path = getMudletHomeDir() .. "/hashfile-test.txt"

    before_each(function()
      local f = io.open(path, "w")
      f:write("abc")
      f:close()
    end)

    after_each(function()
      os.remove(path)
    end)

    it("should return the known sha256 digest of 'abc'", function()
      assert.equals("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                    hashFile(path, "sha256"))
    end)

    it("should accept the algorithm name case-insensitively", function()
      assert.equals("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                    hashFile(path, "SHA256"))
    end)

    it("should return nil and a message for an unsupported algorithm", function()
      local digest, err = hashFile(path, "sha3")
      assert.is_nil(digest)
      assert.is_string(err)
    end)

    it("should return nil and a message when the file does not exist", function()
      local digest, err = hashFile(getMudletHomeDir() .. "/no-such-file", "sha256")
      assert.is_nil(digest)
      assert.is_string(err)
    end)
  end)

  describe("Tests the functionality of stt.getPlatformKey", function()
    it("should return a supported platform key on this machine", function()
      local key = stt.getPlatformKey()
      local supported = {
        ["macos"] = true, ["windows-x64"] = true, ["windows-x86"] = true,
        ["linux-x86_64"] = true, ["linux-aarch64"] = true,
      }
      assert.is_true(supported[key] == true)
    end)
  end)

  describe("Tests the Vosk library search paths", function()
    it("should include a writable vosk-lib path under the Mudlet data directory", function()
      local paths = stt.getInfo().searchPaths or {}
      local found = false
      for _, p in ipairs(paths) do
        if p:find("vosk%-lib") then found = true end
      end
      assert.is_true(found)
    end)
  end)

  describe("Tests the functionality of stt.getLibraryPath", function()
    it("should be the directory the library search paths look in", function()
      local libDir = stt.getLibraryPath()
      assert.is_string(libDir)
      assert.is_true(libDir:find("vosk%-lib") ~= nil)

      local found = false
      for _, p in ipairs(stt.getInfo().searchPaths or {}) do
        if p:sub(1, #libDir + 1) == libDir .. "/" then found = true end
      end
      assert.is_true(found)
    end)
  end)

  describe("Tests the functionality of stt.reloadLibrary", function()
    it("should return a boolean reflecting library availability", function()
      local available = stt.reloadLibrary()
      assert.equals("boolean", type(available))
      assert.equals(stt.isAvailable(), available)
    end)
  end)

  describe("Tests the functionality of STT.UI._libraryBuildForPlatform", function()
    local realGetPlatformKey

    before_each(function() realGetPlatformKey = stt.getPlatformKey end)
    after_each(function() stt.getPlatformKey = realGetPlatformKey end)

    it("should select the macOS build", function()
      stt.getPlatformKey = function() return "macos" end
      local build = STT.UI._libraryBuildForPlatform()
      assert.equals("libvosk.dylib", build.libraryName)
      assert.is_true(build.url:find("^https://") ~= nil)
      assert.equals(64, #build.sha256)
    end)

    it("should select the linux aarch64 build", function()
      stt.getPlatformKey = function() return "linux-aarch64" end
      assert.equals("libvosk.so", STT.UI._libraryBuildForPlatform().libraryName)
    end)

    it("should select the linux x86_64 build", function()
      stt.getPlatformKey = function() return "linux-x86_64" end
      local build = STT.UI._libraryBuildForPlatform()
      assert.equals("libvosk.so", build.libraryName)
      assert.is_true(build.url:find("x86_64") ~= nil)
    end)

    it("should select the windows x64 build", function()
      stt.getPlatformKey = function() return "windows-x64" end
      local build = STT.UI._libraryBuildForPlatform()
      assert.equals("libvosk.dll", build.libraryName)
      assert.equals(64, #build.sha256)
    end)

    it("should return nil for a platform key with no published build", function()
      -- 32-bit Windows is a key stt.getPlatformKey() can return, but no build
      -- is pinned for it
      stt.getPlatformKey = function() return "windows-x86" end
      assert.is_nil(STT.UI._libraryBuildForPlatform())
    end)

    it("should return nil on an unsupported platform", function()
      stt.getPlatformKey = function() return nil end
      assert.is_nil(STT.UI._libraryBuildForPlatform())
    end)
  end)

  describe("Tests the functionality of STT.UI._verifyDownloadedLibrary", function()
    local path = getMudletHomeDir() .. "/vosk-verify-test.zip"

    before_each(function()
      local f = io.open(path, "w")
      f:write("abc")
      f:close()
    end)

    after_each(function() os.remove(path) end)

    it("should accept an archive whose digest matches the pin", function()
      local ok = STT.UI._verifyDownloadedLibrary(path,
        {sha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"})
      assert.is_true(ok)
    end)

    it("should reject an archive whose digest does not match", function()
      local ok, err = STT.UI._verifyDownloadedLibrary(path, {sha256 = string.rep("0", 64)})
      assert.is_false(ok)
      assert.is_string(err)
    end)

    it("should reject a missing archive", function()
      local ok, err = STT.UI._verifyDownloadedLibrary(getMudletHomeDir() .. "/nope.zip",
        {sha256 = string.rep("0", 64)})
      assert.is_false(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests the functionality of STT.UI._installExtractedLibrary", function()
    local libDir = getMudletHomeDir() .. "/vosk-lib-test"
    local nested = libDir .. "/vosk-linux-aarch64-0.3.45"

    before_each(function()
      lfs.mkdir(libDir)
      lfs.mkdir(nested)
      local f = io.open(nested .. "/libvosk.so", "w")
      f:write("payload")
      f:close()
    end)

    after_each(function()
      os.remove(libDir .. "/libvosk.so")
      os.remove(nested .. "/libvosk.so")
      lfs.rmdir(nested)
      lfs.rmdir(libDir)
    end)

    it("should move the library out of the versioned subdirectory", function()
      local ok = STT.UI._installExtractedLibrary(libDir, {libraryName = "libvosk.so"})
      assert.is_true(ok)
      assert.is_not_nil(lfs.attributes(libDir .. "/libvosk.so"))
    end)

    it("should report an error when the archive held no library", function()
      os.remove(nested .. "/libvosk.so")
      local ok, err = STT.UI._installExtractedLibrary(libDir, {libraryName = "libvosk.so"})
      assert.is_false(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests the functionality of STT.UI._downloadLibrary", function()
    local realGetPlatformKey, realSetDialogStatus, statuses

    before_each(function()
      realGetPlatformKey = stt.getPlatformKey
      realSetDialogStatus = STT.UI._setDialogStatus
      statuses = {}
      STT.UI._setDialogStatus = function(msg) table.insert(statuses, msg) end
    end)

    after_each(function()
      stt.getPlatformKey = realGetPlatformKey
      STT.UI._setDialogStatus = realSetDialogStatus
    end)

    it("should report manual install guidance on an unsupported platform", function()
      stt.getPlatformKey = function() return nil end
      STT.UI._downloadLibrary()
      assert.is_true(#statuses > 0)
      assert.is_true(statuses[#statuses]:lower():find("manual") ~= nil)
    end)

    it("should report manual install guidance without the getLibraryPath binding", function()
      local realGetLibraryPath = stt.getLibraryPath
      stt.getLibraryPath = nil
      STT.UI._downloadLibrary()
      stt.getLibraryPath = realGetLibraryPath
      assert.is_true(#statuses > 0)
      assert.is_true(statuses[#statuses]:lower():find("manual") ~= nil)
    end)
  end)

  -- Drives the library install through the real event handlers by raising the
  -- events the download and unzip would have raised. Nothing is fetched or
  -- extracted for real.
  describe("Tests the Vosk library install event flow", function()
    local libDir = getMudletHomeDir() .. "/vosk-lib-events"
    local nested = libDir .. "/vosk-linux-aarch64-0.3.45"
    local zipPath = libDir .. "/vosk-library.zip"
    local saved, statuses

    before_each(function()
      saved = {
        getPlatformKey = stt.getPlatformKey,
        getLibraryPath = stt.getLibraryPath,
        reloadLibrary = stt.reloadLibrary,
        setDialogStatus = STT.UI._setDialogStatus,
        verifyDownloadedLibrary = STT.UI._verifyDownloadedLibrary,
        downloadFile = _G.downloadFile,
        unzipAsync = _G.unzipAsync,
      }
      statuses = {}

      lfs.mkdir(libDir)
      lfs.mkdir(nested)
      local f = io.open(nested .. "/libvosk.so", "w")
      f:write("payload")
      f:close()

      stt.getPlatformKey = function() return "linux-aarch64" end
      stt.getLibraryPath = function() return libDir end
      stt.reloadLibrary = function() return true end
      STT.UI._setDialogStatus = function(msg) table.insert(statuses, msg) end
      STT.UI._verifyDownloadedLibrary = function() return true end
      -- busted sandboxes plain global writes per test context, so these have
      -- to go through _G to be seen by the code under test
      _G.downloadFile = function() end
      _G.unzipAsync = function() return true end
    end)

    after_each(function()
      stt.getPlatformKey = saved.getPlatformKey
      stt.getLibraryPath = saved.getLibraryPath
      stt.reloadLibrary = saved.reloadLibrary
      STT.UI._setDialogStatus = saved.setDialogStatus
      STT.UI._verifyDownloadedLibrary = saved.verifyDownloadedLibrary
      _G.downloadFile = saved.downloadFile
      _G.unzipAsync = saved.unzipAsync

      for _, key in ipairs({"_libDownloadHandlerId", "_libErrorHandlerId", "_libUnzipDoneId", "_libUnzipErrorId"}) do
        if STT.UI[key] then
          killAnonymousEventHandler(STT.UI[key])
          STT.UI[key] = nil
        end
      end
      STT.UI._downloadingLibrary = false

      os.remove(zipPath)
      os.remove(libDir .. "/libvosk.so")
      os.remove(nested .. "/libvosk.so")
      lfs.rmdir(nested)
      lfs.rmdir(libDir)
    end)

    it("should drop the download error handler once the download completes", function()
      STT.UI._downloadLibrary()
      assert.is_not_nil(STT.UI._libErrorHandlerId)

      raiseEvent("sysDownloadDone", zipPath)

      assert.is_nil(STT.UI._libErrorHandlerId)
      assert.is_nil(STT.UI._libDownloadHandlerId)
      assert.is_not_nil(STT.UI._libUnzipDoneId)
      assert.is_not_nil(STT.UI._libUnzipErrorId)
    end)

    it("should unregister the unzip error handler when extraction succeeds", function()
      STT.UI._downloadLibrary()
      raiseEvent("sysDownloadDone", zipPath)
      raiseEvent("sysUnzipDone", zipPath)

      assert.is_nil(STT.UI._libUnzipErrorId)
      assert.is_nil(STT.UI._libUnzipDoneId)
      assert.is_false(STT.UI._downloadingLibrary)
      assert.is_true(statuses[#statuses]:find("installed") ~= nil)
      assert.is_not_nil(lfs.attributes(libDir .. "/libvosk.so"))
    end)

    it("should unregister the unzip done handler when extraction fails", function()
      STT.UI._downloadLibrary()
      raiseEvent("sysDownloadDone", zipPath)
      raiseEvent("sysUnzipError", zipPath)

      assert.is_nil(STT.UI._libUnzipDoneId)
      assert.is_nil(STT.UI._libUnzipErrorId)
      assert.is_false(STT.UI._downloadingLibrary)
      assert.is_true(statuses[#statuses]:lower():find("extract") ~= nil)
    end)

    it("should surface why reloading the library was refused", function()
      stt.reloadLibrary = function() return false, "close speech recognition first" end
      STT.UI._downloadLibrary()
      raiseEvent("sysDownloadDone", zipPath)
      raiseEvent("sysUnzipDone", zipPath)

      assert.is_true(statuses[#statuses]:find("close speech recognition first") ~= nil)
    end)

    it("should clean up every handler when extraction cannot be started", function()
      _G.unzipAsync = function() return false, "no unzip for you" end
      STT.UI._downloadLibrary()
      raiseEvent("sysDownloadDone", zipPath)

      assert.is_nil(STT.UI._libUnzipDoneId)
      assert.is_nil(STT.UI._libUnzipErrorId)
      assert.is_nil(STT.UI._libErrorHandlerId)
      assert.is_false(STT.UI._downloadingLibrary)
    end)
  end)

  describe("Tests the functionality of stt.unloadLibrary", function()
    it("should be registered", function()
      assert.equals("function", type(stt.unloadLibrary))
    end)

    it("should leave the library detectable again after a reload", function()
      stt.close()
      assert.is_true(stt.unloadLibrary())
      local available = stt.reloadLibrary()
      assert.equals("boolean", type(available))
    end)
  end)

  describe("Tests the functionality of stt.getInfo", function()
    it("should report the active model path separately from the models directory", function()
      local info = stt.getInfo()
      assert.equals("string", type(info.modelPath))
      -- with nothing loaded the active model path is empty, never the models
      -- base directory that stt.getModelPath() reports
      assert.is_true(info.modelPath == "" or info.modelPath ~= stt.getModelPath())
    end)

    it("should report a state string", function()
      local info = stt.getInfo()
      assert.equals("string", type(info.state))
    end)
  end)

  describe("Tests the functionality of stt.close releasing resources", function()
    it("should leave the library reloadable after closing", function()
      assert.is_true(stt.close())
      local available = stt.reloadLibrary()
      assert.equals("boolean", type(available))
    end)
  end)

  describe("Tests the functionality of STT.UI._removePath", function()
    local root = getMudletHomeDir() .. "/removepath-test"

    before_each(function()
      lfs.mkdir(root)
      lfs.mkdir(root .. "/nested")
      local f = io.open(root .. "/top.txt", "w") f:write("a") f:close()
      local g = io.open(root .. "/nested/inner.txt", "w") g:write("b") g:close()
    end)

    after_each(function()
      os.remove(root .. "/nested/inner.txt")
      os.remove(root .. "/top.txt")
      lfs.rmdir(root .. "/nested")
      lfs.rmdir(root)
    end)

    it("should remove a nested directory tree", function()
      local ok = STT.UI._removePath(root)
      assert.is_true(ok)
      assert.is_nil(lfs.attributes(root, "mode"))
    end)

    it("should remove a single file", function()
      local ok = STT.UI._removePath(root .. "/top.txt")
      assert.is_true(ok)
      assert.is_nil(lfs.attributes(root .. "/top.txt", "mode"))
    end)

    it("should report failure for a path that does not exist", function()
      local ok, err = STT.UI._removePath(root .. "/no-such-entry")
      assert.is_false(ok)
      assert.is_string(err)
    end)

    it("should report failure when a child cannot be removed", function()
      local realRmdir = lfs.rmdir
      lfs.rmdir = function(path)
        if path == root .. "/nested" then
          return nil, "permission denied"
        end
        return realRmdir(path)
      end
      local ok, err = STT.UI._removePath(root)
      lfs.rmdir = realRmdir
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_not_nil(lfs.attributes(root, "mode"))
    end)

    it("should report failure rather than raise when a directory cannot be listed", function()
      local realDir = lfs.dir
      lfs.dir = function(path)
        if path == root then
          error("cannot open " .. path)
        end
        return realDir(path)
      end
      local ok, err = STT.UI._removePath(root)
      lfs.dir = realDir
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_not_nil(lfs.attributes(root, "mode"))
    end)
  end)

  describe("Tests the functionality of STT.UI._statusLines", function()
    local realGetInfo, realGetModelPath, realGetLibraryPath

    before_each(function()
      realGetInfo = stt.getInfo
      realGetModelPath = stt.getModelPath
      realGetLibraryPath = stt.getLibraryPath
    end)

    after_each(function()
      stt.getInfo = realGetInfo
      stt.getModelPath = realGetModelPath
      stt.getLibraryPath = realGetLibraryPath
    end)

    it("should report the library as not installed when unavailable", function()
      stt.getInfo = function() return {available = false} end
      local lines = STT.UI._statusLines()
      assert.equals("Library", lines[1].label)
      assert.is_true(lines[1].value:find("Not installed") ~= nil)
    end)

    it("should report the library location when it is the copy we installed", function()
      local dir = getMudletHomeDir() .. "/statuslines-lib-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/libvosk.testlib", "w") f:write("x") f:close()
      stt.getInfo = function() return {available = true, version = "0.3.x", searchPaths = {dir .. "/libvosk.testlib"}} end
      stt.getLibraryPath = function() return dir end
      local lines = STT.UI._statusLines()
      STT.UI._removePath(dir)
      assert.is_true(lines[1].value:find("0.3.x", 1, true) ~= nil)
      assert.is_true(lines[1].value:find(dir, 1, true) ~= nil)
    end)

    it("should omit the version when none is reported", function()
      local dir = getMudletHomeDir() .. "/statuslines-nover-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/libvosk.testlib", "w") f:write("x") f:close()
      stt.getInfo = function() return {available = true, searchPaths = {dir .. "/libvosk.testlib"}} end
      stt.getLibraryPath = function() return dir end
      local lines = STT.UI._statusLines()
      STT.UI._removePath(dir)
      assert.is_nil(lines[1].value:find("unknown", 1, true))
      assert.is_nil(lines[1].value:find("()", 1, true))
      assert.is_true(lines[1].value:find("Installed", 1, true) ~= nil)
      assert.is_true(lines[1].value:find(dir, 1, true) ~= nil)
    end)

    it("should omit the version when it is reported as an empty string", function()
      local dir = getMudletHomeDir() .. "/statuslines-emptyver-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/libvosk.testlib", "w") f:write("x") f:close()
      stt.getInfo = function() return {available = true, version = "", searchPaths = {dir .. "/libvosk.testlib"}} end
      stt.getLibraryPath = function() return dir end
      local lines = STT.UI._statusLines()
      STT.UI._removePath(dir)
      assert.is_nil(lines[1].value:find("()", 1, true))
      assert.is_true(lines[1].value:find("Installed", 1, true) ~= nil)
    end)

    it("should not claim our own location for a library installed elsewhere", function()
      local dir = getMudletHomeDir() .. "/statuslines-nolib-test"
      stt.getInfo = function() return {available = true, version = "0.3.x", searchPaths = {dir .. "/libvosk.testlib"}} end
      stt.getLibraryPath = function() return dir end
      local lines = STT.UI._statusLines()
      assert.is_true(lines[1].value:find("elsewhere", 1, true) ~= nil)
      assert.is_nil(lines[1].value:find(dir, 1, true))
    end)

    it("should report no model loaded when uninitialized", function()
      stt.getInfo = function() return {available = true, initialized = false, modelPath = ""} end
      assert.equals("None loaded", STT.UI._statusLines()[2].value)
    end)

    it("should report the active model by folder name", function()
      stt.getInfo = function() return {available = true, initialized = true, modelPath = "/models/vosk-model-small-en-us-0.15"} end
      assert.equals("vosk-model-small-en-us-0.15", STT.UI._statusLines()[2].value)
    end)

    it("should not name a model from the models base directory", function()
      -- stt.getModelPath() is the directory models live in, never the loaded
      -- model, so the panel must not fall back to it
      stt.getModelPath = function() return "/models/vosk-models" end
      stt.getInfo = function() return {available = true, initialized = true, modelPath = ""} end
      assert.equals("None loaded", STT.UI._statusLines()[2].value)
    end)

    it("should report Listening state", function()
      stt.getInfo = function() return {available = true, initialized = true, modelPath = "/models/m", listening = true, state = "listening"} end
      assert.equals("Listening", STT.UI._statusLines()[3].value)
    end)

    it("should report Error state rather than Idle", function()
      stt.getInfo = function() return {available = true, initialized = false, listening = false, state = "error", modelPath = ""} end
      assert.equals("Error", STT.UI._statusLines()[3].value)
    end)

    it("should report Idle when uninitialized", function()
      stt.getInfo = function() return {available = true, initialized = false, listening = false, state = "uninitialized", modelPath = ""} end
      assert.equals("Idle", STT.UI._statusLines()[3].value)
    end)
  end)

  describe("Tests the functionality of STT.UI._libraryIsOurs", function()
    it("should report false when the expected library file is absent", function()
      local ours = STT.UI._libraryIsOurs({searchPaths = {getMudletHomeDir() .. "/no-such-vosk-lib/libvosk.testlib"}})
      assert.is_false(ours)
    end)

    it("should report false when there are no search paths", function()
      assert.is_false(STT.UI._libraryIsOurs({}))
    end)

    it("should report true and the file when the expected library file exists", function()
      local dir = getMudletHomeDir() .. "/libraryisours-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/libvosk.testlib", "w") f:write("x") f:close()
      local ours, file = STT.UI._libraryIsOurs({searchPaths = {dir .. "/libvosk.testlib"}})
      STT.UI._removePath(dir)
      assert.is_true(ours)
      assert.equals(dir .. "/libvosk.testlib", file)
    end)
  end)

  describe("Tests the functionality of STT.UI._setDialogStatus", function()
    local realDialogStatus

    before_each(function() realDialogStatus = STT.UI._dialogStatus end)
    after_each(function() STT.UI._dialogStatus = realDialogStatus end)

    it("should not raise when Geyser cannot parse the colour", function()
      STT.UI._dialogStatus = Geyser.Label:new({
        name = "STT_StatusColourTest", x = 0, y = 0, width = 10, height = 10,
      })
      -- "lime" is not a Geyser colour name; parse returns nothing and echo
      -- raises. This must not propagate to callers that go on to rebuild.
      assert.has_no.errors(function() STT.UI._setDialogStatus("hello", "lime") end)
      deleteLabel("STT_StatusColourTest")
    end)

    it("should use every colour the dialog actually passes without raising", function()
      STT.UI._dialogStatus = Geyser.Label:new({
        name = "STT_StatusColourTest2", x = 0, y = 0, width = 10, height = 10,
      })
      for _, colour in ipairs({"#00ff00", "red", "cyan", "yellow", "white"}) do
        assert.is_true(Geyser.Color.parse(colour) ~= nil, colour .. " is not parseable")
        assert.has_no.errors(function() STT.UI._setDialogStatus("hello", colour) end)
      end
      deleteLabel("STT_StatusColourTest2")
    end)
  end)

  describe("Tests the functionality of STT.UI._refreshWithStatus", function()
    local realShow, realSetupDialog

    before_each(function()
      realShow = STT.UI.showSetupDialog
      realSetupDialog = STT.UI._setupDialog
      STT.UI._pendingStatus = nil
    end)

    after_each(function()
      STT.UI.showSetupDialog = realShow
      STT.UI._setupDialog = realSetupDialog
      STT.UI._pendingStatus = nil
    end)

    it("should not rebuild inline, since it runs from the row's own callback", function()
      local rebuilt = false
      STT.UI._setupDialog = {}
      STT.UI.showSetupDialog = function() rebuilt = true end
      STT.UI._refreshWithStatus("Model removed", "lime")
      -- Rebuilding here would destroy the widget whose callback is running.
      assert.is_false(rebuilt)
    end)

    it("should leave the status pending for the rebuild to apply", function()
      STT.UI._setupDialog = {}
      STT.UI.showSetupDialog = function() end
      STT.UI._refreshWithStatus("Model removed", "lime")
      assert.equals("Model removed", STT.UI._pendingStatus.message)
      assert.equals("lime", STT.UI._pendingStatus.color)
    end)

    it("should show the status immediately, not only after the rebuild", function()
      local shown
      local realStatus = STT.UI._setDialogStatus
      STT.UI._setDialogStatus = function(message) shown = message end
      STT.UI._setupDialog = {}
      STT.UI.showSetupDialog = function() end
      STT.UI._refreshWithStatus("Model removed", "lime")
      STT.UI._setDialogStatus = realStatus
      assert.equals("Model removed", shown)
    end)

    it("should not leave a pending status when no dialog is open", function()
      STT.UI._setupDialog = nil
      STT.UI.showSetupDialog = function() end
      STT.UI._refreshWithStatus("Model removed", "lime")
      assert.is_nil(STT.UI._pendingStatus)
    end)
  end)

  describe("Tests the functionality of STT.UI._armConfirm", function()
    local realSetDialogStatus

    before_each(function()
      realSetDialogStatus = STT.UI._setDialogStatus
      STT.UI._setDialogStatus = function() end
      STT.UI._pendingConfirm = nil
    end)

    after_each(function()
      STT.UI._setDialogStatus = realSetDialogStatus
      STT.UI._pendingConfirm = nil
    end)

    it("should not perform the action on the first click", function()
      local ran = false
      local performed = STT.UI._armConfirm("k", function() ran = true end)
      assert.is_false(performed)
      assert.is_false(ran)
    end)

    it("should perform the action on the second click", function()
      local ran = false
      STT.UI._armConfirm("k", function() ran = true end)
      STT.UI._armConfirm("k", function() ran = true end)
      assert.is_true(ran)
    end)

    it("should disarm a previously armed key", function()
      local ranFirst = false
      STT.UI._armConfirm("first", function() ranFirst = true end)
      STT.UI._armConfirm("second", function() end)
      STT.UI._armConfirm("first", function() ranFirst = true end)
      assert.is_false(ranFirst)
    end)

    it("should restyle the row and change its text when arming", function()
      local echoed, styled
      local row = {
        label = {
          echo = function(_, text) echoed = text end,
          setStyleSheet = function(_, style) styled = style end,
        },
        text = "<center>remove</center>",
        style = "original",
        armedText = "<center><b>confirm?</b></center>",
      }
      STT.UI._armConfirm("k", function() end, row)
      assert.equals("<center><b>confirm?</b></center>", echoed)
      assert.is_true(styled ~= "original")
    end)

    it("should restore the row's text and style on the confirming click", function()
      local echoed, styled
      local row = {
        label = {
          echo = function(_, text) echoed = text end,
          setStyleSheet = function(_, style) styled = style end,
        },
        text = "<center>remove</center>",
        style = "original",
      }
      STT.UI._armConfirm("k", function() end, row)
      STT.UI._armConfirm("k", function() end, row)
      assert.equals("<center>remove</center>", echoed)
      assert.equals("original", styled)
    end)

    it("should restore a previously armed row when a different row is armed", function()
      local firstEchoed
      local firstRow = {
        label = {
          echo = function(_, text) firstEchoed = text end,
          setStyleSheet = function() end,
        },
        text = "<center>remove</center>",
        style = "original",
      }
      local secondRow = {
        label = {echo = function() end, setStyleSheet = function() end},
        text = "other",
        style = "other",
      }
      STT.UI._armConfirm("first", function() end, firstRow)
      STT.UI._armConfirm("second", function() end, secondRow)
      assert.equals("<center>remove</center>", firstEchoed)
    end)
  end)

  describe("Tests the functionality of STT.UI._removeModel", function()
    local realGetInfo, realGetModelPath, realSetDialogStatus

    before_each(function()
      realGetInfo = stt.getInfo
      realGetModelPath = stt.getModelPath
      realSetDialogStatus = STT.UI._setDialogStatus
      STT.UI._setDialogStatus = function() end
    end)

    after_each(function()
      stt.getInfo = realGetInfo
      stt.getModelPath = realGetModelPath
      STT.UI._setDialogStatus = realSetDialogStatus
    end)

    it("should refuse while listening", function()
      stt.getInfo = function() return {listening = true} end
      local ok, err = STT.UI._removeModel({name = "m", path = "/models/m"})
      assert.is_false(ok)
      assert.is_string(err)
    end)

    it("should refuse to remove the model currently loaded", function()
      local dir = getMudletHomeDir() .. "/removemodel-loaded-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      -- The guard has to read the loaded model from getInfo().modelPath;
      -- stt.getModelPath() is the models base directory and can never match
      stt.getInfo = function() return {listening = false, initialized = true, modelPath = dir} end
      stt.getModelPath = function() return getMudletHomeDir() .. "/vosk-models" end
      local ok, err = STT.UI._removeModel({name = "m", path = dir})
      local survived = lfs.attributes(dir, "mode") ~= nil
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_true(survived)
    end)

    it("should refuse when the active path differs only by a trailing separator", function()
      local dir = getMudletHomeDir() .. "/removemodel-trailingsep-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getInfo = function() return {listening = false, initialized = true, modelPath = dir .. "/"} end
      stt.getModelPath = function() return getMudletHomeDir() .. "/vosk-models" end
      local ok, err = STT.UI._removeModel({name = "m", path = dir})
      local survived = lfs.attributes(dir, "mode") ~= nil
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_true(survived)
    end)

    it("should remove a model that is not in use", function()
      local dir = getMudletHomeDir() .. "/removemodel-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      -- stt.getModelPath() reporting this very path must not block removal:
      -- only the loaded model, reported by getInfo().modelPath, is protected
      stt.getInfo = function() return {listening = false, initialized = false, modelPath = ""} end
      stt.getModelPath = function() return dir end
      local ok = STT.UI._removeModel({name = "removemodel-test", path = dir})
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_true(ok)
      assert.is_nil(lfs.attributes(dir, "mode"))
    end)
  end)

  describe("Tests the functionality of STT.UI._removeLibrary", function()
    local realGetInfo, realGetLibraryPath, realClose, realReloadLibrary, realUnloadLibrary, realSetDialogStatus

    before_each(function()
      realGetInfo = stt.getInfo
      realGetLibraryPath = stt.getLibraryPath
      realClose = stt.close
      realReloadLibrary = stt.reloadLibrary
      realUnloadLibrary = stt.unloadLibrary
      realSetDialogStatus = STT.UI._setDialogStatus
      STT.UI._setDialogStatus = function() end
    end)

    after_each(function()
      stt.getInfo = realGetInfo
      stt.getLibraryPath = realGetLibraryPath
      stt.close = realClose
      stt.reloadLibrary = realReloadLibrary
      stt.unloadLibrary = realUnloadLibrary
      STT.UI._setDialogStatus = realSetDialogStatus
    end)

    it("should refuse while listening", function()
      stt.getInfo = function() return {listening = true} end
      local dir = getMudletHomeDir() .. "/removelibrary-listening-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getLibraryPath = function() return dir end
      local ok, err = STT.UI._removeLibrary()
      local stillThere = lfs.attributes(dir, "mode") ~= nil
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_true(stillThere)
    end)

    it("should delete the library directory", function()
      stt.getInfo = function() return {listening = false} end
      stt.close = function() end
      stt.unloadLibrary = function() return true end
      stt.reloadLibrary = function() end
      local dir = getMudletHomeDir() .. "/removelibrary-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getLibraryPath = function() return dir end
      local ok = STT.UI._removeLibrary()
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_true(ok)
      assert.is_nil(lfs.attributes(dir, "mode"))
    end)

    it("should unload the library after closing and before deleting its files", function()
      local order = {}
      stt.getInfo = function() return {listening = false} end
      stt.close = function() order[#order + 1] = "close" end
      stt.unloadLibrary = function() order[#order + 1] = "unload" return true end
      stt.reloadLibrary = function() order[#order + 1] = "reload" end
      local dir = getMudletHomeDir() .. "/removelibrary-order-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getLibraryPath = function()
        -- recorded lazily so that the deletion shows up between unload and reload
        return dir
      end
      local realRemovePath = STT.UI._removePath
      STT.UI._removePath = function(path)
        -- only the top-level call, since _removePath recurses through itself
        if path == dir then
          order[#order + 1] = "remove"
        end
        return realRemovePath(path)
      end
      local ok = STT.UI._removeLibrary()
      STT.UI._removePath = realRemovePath
      if lfs.attributes(dir, "mode") then
        realRemovePath(dir)
      end
      assert.is_true(ok)
      assert.equals("close,unload,remove,reload", table.concat(order, ","))
    end)

    it("should not delete the library files when the unload is refused", function()
      stt.getInfo = function() return {listening = false} end
      stt.close = function() end
      stt.unloadLibrary = function() return false, "close speech recognition first" end
      stt.reloadLibrary = function() end
      local dir = getMudletHomeDir() .. "/removelibrary-unloadrefused-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getLibraryPath = function() return dir end
      local ok, err = STT.UI._removeLibrary()
      local survived = lfs.attributes(dir, "mode") ~= nil
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_true(survived)
    end)

    it("should surface a refusal from the reload after the files are gone", function()
      stt.getInfo = function() return {listening = false} end
      stt.close = function() end
      stt.unloadLibrary = function() return true end
      stt.reloadLibrary = function() return false, "close speech recognition first" end
      local dir = getMudletHomeDir() .. "/removelibrary-reloadrefused-test"
      lfs.mkdir(dir)
      local f = io.open(dir .. "/f.txt", "w") f:write("x") f:close()
      stt.getLibraryPath = function() return dir end
      local ok, err = STT.UI._removeLibrary()
      if lfs.attributes(dir, "mode") then
        STT.UI._removePath(dir)
      end
      assert.is_false(ok)
      assert.is_string(err)
      assert.is_true(err:find("close speech recognition first", 1, true) ~= nil)
    end)

    it("should refuse when the library path cannot be determined", function()
      stt.getInfo = function() return {listening = false} end
      stt.close = function() end
      stt.unloadLibrary = function() return true end
      stt.getLibraryPath = function() return nil end
      local ok, err = STT.UI._removeLibrary()
      assert.is_false(ok)
      assert.is_string(err)
    end)
  end)

  describe("Tests the functionality of STT.UI._resetEverything", function()
    local realGetInfo, realListModels, realClose, realRemoveModel, realRemoveLibrary, realSetDialogStatus

    before_each(function()
      realGetInfo = stt.getInfo
      realListModels = stt.listModels
      realClose = stt.close
      realRemoveModel = STT.UI._removeModel
      realRemoveLibrary = STT.UI._removeLibrary
      realSetDialogStatus = STT.UI._setDialogStatus
      STT.UI._setDialogStatus = function() end
      stt.close = function() end
    end)

    after_each(function()
      stt.getInfo = realGetInfo
      stt.listModels = realListModels
      stt.close = realClose
      STT.UI._removeModel = realRemoveModel
      STT.UI._removeLibrary = realRemoveLibrary
      STT.UI._setDialogStatus = realSetDialogStatus
    end)

    it("should refuse while listening", function()
      stt.getInfo = function() return {listening = true} end
      local removeModelCalled = false
      local removeLibraryCalled = false
      local closed = false
      stt.close = function() closed = true end
      STT.UI._removeModel = function() removeModelCalled = true return true end
      STT.UI._removeLibrary = function() removeLibraryCalled = true return true end

      local ok = STT.UI._resetEverything()
      assert.is_false(ok)
      assert.is_false(closed)
      assert.is_false(removeModelCalled)
      assert.is_false(removeLibraryCalled)
    end)

    it("should close the recognizer before removing any model", function()
      stt.getInfo = function() return {listening = false} end
      stt.listModels = function() return {{name = "a", path = "/m/a"}} end
      local order = {}
      stt.close = function() order[#order + 1] = "close" end
      STT.UI._removeModel = function() order[#order + 1] = "model" return true end
      STT.UI._removeLibrary = function() order[#order + 1] = "library" return true end

      local ok = STT.UI._resetEverything()
      assert.is_true(ok)
      assert.equals("close,model,library", table.concat(order, ","))
    end)

    it("should remove every model and the library", function()
      stt.getInfo = function() return {listening = false} end
      stt.listModels = function() return {{name = "a", path = "/m/a"}, {name = "b", path = "/m/b"}} end
      local removed = {}
      STT.UI._removeModel = function(m) removed[#removed + 1] = m.name return true end
      local libraryRemoved = false
      STT.UI._removeLibrary = function() libraryRemoved = true return true end

      local ok = STT.UI._resetEverything()
      assert.is_true(ok)
      assert.equals(2, #removed)
      assert.is_true(libraryRemoved)
    end)

    it("should report failure when a model cannot be removed", function()
      stt.getInfo = function() return {listening = false} end
      stt.listModels = function() return {{name = "a", path = "/m/a"}} end
      STT.UI._removeModel = function() return false, "locked" end
      STT.UI._removeLibrary = function() return true end

      local ok, summary = STT.UI._resetEverything()
      assert.is_false(ok)
      assert.is_string(summary)
    end)

    it("should attempt every model and still attempt the library after one model fails", function()
      stt.getInfo = function() return {listening = false} end
      stt.listModels = function() return {{name = "a", path = "/m/a"}, {name = "b", path = "/m/b"}} end
      local attempted = {}
      STT.UI._removeModel = function(m)
        attempted[#attempted + 1] = m.name
        if m.name == "a" then return false, "locked" end
        return true
      end
      local libraryAttempted = false
      STT.UI._removeLibrary = function() libraryAttempted = true return true end

      local ok, summary = STT.UI._resetEverything()
      assert.is_false(ok)
      assert.equals(2, #attempted)
      assert.is_true(libraryAttempted)
      assert.is_string(summary)
    end)
  end)
end)
