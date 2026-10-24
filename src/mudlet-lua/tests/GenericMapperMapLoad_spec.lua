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
  -- where "map save" writes, and so where "map load" with no address reads first
  local savedMapPath = homeDirectory .. "/map.dat"
  local savedMapBackupPath = specDirectory .. "/saved-map-backup.dat"
  -- where a download lands, and where "map load" with no address reads when "map save" has made no map
  local downloadDirectory = homeDirectory .. "/map downloads"
  local localCopyPath = downloadDirectory .. "/map.dat"
  local localCopyBackupPath = specDirectory .. "/local-copy-backup.dat"
  local loadedRoomName = "A room of the map that was loaded"
  local saved, savedHash, otherMapRoom

  setup(function()
    lfs.mkdir(specDirectory)
    os.remove(backupPath)
    -- a run cut short leaves the profile's own copies set aside here
    for live, aside in pairs({[savedMapPath] = savedMapBackupPath, [localCopyPath] = localCopyBackupPath}) do
      if not io.exists(live) then
        os.rename(aside, live)
      end
      os.remove(aside)
    end
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

  -- puts both files "map load" can read aside, fills the ones named in copies, and puts them back after body
  local function withLocalCopies(copies, body)
    local hadDownloadDirectory = lfs.attributes(downloadDirectory, "mode") == "directory"
    lfs.mkdir(downloadDirectory)
    local places = {
      {path = savedMapPath, aside = savedMapBackupPath, from = copies.saved},
      {path = localCopyPath, aside = localCopyBackupPath, from = copies.downloaded},
    }
    for _, place in ipairs(places) do
      place.hadOne = os.rename(place.path, place.aside)
    end
    local ok, err = pcall(function()
      for _, place in ipairs(places) do
        if place.from then
          assert.is_true(copyFile(place.from, place.path), "a map could not be put where map load reads it")
        end
      end
      body()
    end)
    for _, place in ipairs(places) do
      os.remove(place.path)
      if place.hadOne then
        os.rename(place.aside, place.path)
      end
    end
    if not hadDownloadDirectory then
      lfs.rmdir(downloadDirectory)
    end
    assert(ok, err)
  end

  local function assertForgotThePosition()
    assert.are.equal(loadedRoomName, getRoomName(otherMapRoom), "the other map was not loaded")
    assert.is_falsy(map.mapping)
    assert.is_nil(map.currentRoom)
    assert.is_nil(map.currentArea)
    assert.is_nil(map.prevRoom)
  end

  -- runs body with map.echo recorded, and returns what it echoed
  local function recordEchoes(body)
    local realEcho = map.echo
    local echoes = {}
    map.echo = function(what, debug, err)
      echoes[#echoes + 1] = {what = what, err = err}
    end
    local ok, err = pcall(body)
    map.echo = realEcho
    assert(ok, err)
    return echoes
  end

  local function assertReportedSuccess(echoes, message)
    local said = false
    for _, echo in ipairs(echoes) do
      assert.is_falsy(echo.err, "a map that loaded was reported as an error: " .. tostring(echo.what))
      said = said or echo.what == message
    end
    assert.is_true(said, "the load did not say: " .. message)
  end

  it("stops mapping and forgets the room when a downloaded map loads", function()
    -- nothing listens on the discard port, so the real download fails later and is ignored
    map.load_map("http://127.0.0.1:9/map.dat")
    local echoes = recordEchoes(function()
      raiseEvent("sysDownloadDone", otherMapPath)
    end)
    assertForgotThePosition()
    assertReportedSuccess(echoes, "Map File Loaded.")
  end)

  it("stops mapping and forgets the room when the local copy is reloaded", function()
    withLocalCopies({saved = otherMapPath}, function()
      local echoes = recordEchoes(map.load_map)
      assertForgotThePosition()
      assertReportedSuccess(echoes, "Map reloaded from local copy.")
    end)
  end)

  it("reloads the map that map save wrote rather than a downloaded one", function()
    withLocalCopies({downloaded = otherMapPath}, function()
      local room = map.currentRoom
      assert.are.equal("Where the mapping began", getRoomName(room))
      expandAlias("map save")
      setRoomName(room, "Renamed after the map was saved")
      expandAlias("map load")
      assert.are.equal("Where the mapping began", getRoomName(room))
      assert.is_falsy(map.mapping)
      assert.is_nil(map.currentRoom)
    end)
  end)

  it("reloads a downloaded map when map save has made none", function()
    withLocalCopies({downloaded = otherMapPath}, function()
      map.load_map()
      assertForgotThePosition()
    end)
  end)

  -- runs body and checks it echoed one error and left the mapping running
  local function assertReportsAFailedLoad(body)
    local room = map.currentRoom
    local echoes = recordEchoes(body)
    assert.is_true(map.mapping, "a map that did not load should leave the mapping running")
    assert.are.equal(room, map.currentRoom)
    assert.are.equal(1, #echoes)
    assert.is_true(echoes[1].err, "the failure was not shown as an error: " .. tostring(echoes[1].what))
    return echoes[1].what
  end

  it("says the map did not load when there is none to reload", function()
    local message = assertReportsAFailedLoad(function()
      withLocalCopies({}, map.load_map)
    end)
    assert.is_truthy(message:find(savedMapPath, 1, true), "the error should name the file map save writes: " .. message)
  end)

  it("says a downloaded map did not load when the download is not a map", function()
    local notAMapDirectory = specDirectory .. "/not-a-map"
    local notAMapPath = notAMapDirectory .. "/map.dat"
    lfs.mkdir(notAMapDirectory)
    local file = io.open(notAMapPath, "wb")
    file:write("<html>Not Found</html>")
    file:close()
    map.load_map("http://127.0.0.1:9/map.dat")
    local ok, err = pcall(assertReportsAFailedLoad, function()
      raiseEvent("sysDownloadDone", notAMapPath)
    end)
    os.remove(notAMapPath)
    lfs.rmdir(notAMapDirectory)
    assert(ok, err)
    -- a failed download must not leave the mapper waiting to load the next map.dat anyone downloads
    raiseEvent("sysDownloadDone", otherMapPath)
    assert.are_not.equal(loadedRoomName, getRoomName(otherMapRoom))
    assert.is_true(map.mapping)
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
