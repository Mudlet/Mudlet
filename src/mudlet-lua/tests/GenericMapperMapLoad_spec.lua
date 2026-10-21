describe("Tests the generic mapper when a map loads", function()
  if type(map) ~= "table" or type(map.load_map) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- map.set takes nil, but a table of saved values cannot hold one, hence the list
  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits", "prevRoom", "prevName", "prevExits"}
  local homeDirectory = getMudletHomeDir():gsub("\\", "/")
  local specDirectory = homeDirectory .. "/generic_mapper_map_load_spec"
  local backupPath = specDirectory .. "/backup.dat"
  local otherMapPath = specDirectory .. "/map.dat"
  -- where "map load" with no address reads from, and the download lands
  local downloadDirectory = homeDirectory .. "/map downloads"
  local localCopyPath = downloadDirectory .. "/map.dat"
  local localCopyBackupPath = specDirectory .. "/local-copy-backup.dat"
  local loadedRoomName = "A room of the map that was loaded"
  local saved, savedHash, otherMapRoom

  setup(function()
    lfs.mkdir(specDirectory)
    os.remove(backupPath)
    os.remove(localCopyBackupPath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(otherMapPath)
    lfs.rmdir(specDirectory)
  end)

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedHash = map.prompt.hash
    map.prompt.hash = nil

    deleteMap()
    -- kept short: the docked mapper widens to its longest area name and stays
    -- that wide after the map is put back, squeezing UI_spec's main window
    local area = addAreaName("gmlLoaded")
    otherMapRoom = createRoomID()
    addRoom(otherMapRoom)
    setRoomArea(otherMapRoom, area)
    setRoomCoordinates(otherMapRoom, 0, 0, 0)
    setRoomName(otherMapRoom, loadedRoomName)
    assert.is_true(saveMap(otherMapPath))

    deleteMap()
    map.set("mapping", false)
    map.set("currentRoom", nil)
    map.set("currentArea", nil)
    map.set("currentName", "Where the mapping began")
    map.set("currentExits", {})
    map.start_mapping("gmlMapped")
    assert.is_true(map.mapping)
    assert.are.equal(otherMapRoom, map.currentRoom, "the loaded map has to reuse the mapped room's id for this test to mean anything")
  end)

  after_each(function()
    for _, key in ipairs(mapperState) do
      map.set(key, saved[key])
    end
    map.prompt.hash = savedHash
  end)

  local function copyFile(from, to)
    local source = io.open(from, "rb")
    if not source then
      return false
    end
    local data = source:read("*a")
    source:close()
    local target = io.open(to, "wb")
    if not target then
      return false
    end
    target:write(data)
    target:close()
    return true
  end

  local function assertForgotThePosition()
    assert.are.equal(loadedRoomName, getRoomName(otherMapRoom), "the other map was not loaded")
    assert.is_falsy(map.mapping)
    assert.is_nil(map.currentRoom)
    assert.is_nil(map.currentArea)
    assert.is_nil(map.prevRoom)
  end

  it("stops mapping and forgets the room when a downloaded map loads", function()
    -- nothing listens on the discard port, so the real download fails later and is ignored
    map.load_map("http://127.0.0.1:9/map.dat")
    raiseEvent("sysDownloadDone", otherMapPath)
    assertForgotThePosition()
  end)

  it("stops mapping and forgets the room when the local copy is reloaded", function()
    lfs.mkdir(downloadDirectory)
    local hadLocalCopy = os.rename(localCopyPath, localCopyBackupPath)
    local copied = copyFile(otherMapPath, localCopyPath)
    local ok, err = pcall(function()
      assert.is_true(copied, "the other map could not be put where map load reads it")
      map.load_map()
      assertForgotThePosition()
    end)
    os.remove(localCopyPath)
    if hadLocalCopy then
      os.rename(localCopyBackupPath, localCopyPath)
    else
      lfs.rmdir(downloadDirectory)
    end
    assert(ok, err)
  end)

  it("finds the player on the loaded map after a random move", function()
    raiseEvent("onRandomMove")
    map.load_map("http://127.0.0.1:9/map.dat")
    raiseEvent("sysDownloadDone", otherMapPath)
    assertForgotThePosition()
    map.set("currentName", loadedRoomName)
    map.set("currentExits", {})
    local ok, err = pcall(map.find_me, nil, nil, nil, true)
    assert(ok, err)
    assert.are.equal(otherMapRoom, map.currentRoom)
  end)
end)
