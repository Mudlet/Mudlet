-- The starter UI and the generic mapper only need to know whether the map holds
-- any room at all. getRooms() answers that by copying out every room name, so
-- they ask roomExists(1) first and fall back to it for maps without a room 1.
-- Every test starts from a wiped map, so the map is saved first and put back after.

describe("Tests how the bundled packages tell an empty map from one with rooms", function()
  local backupPath = getMudletHomeDir() .. "/map/mapemptinesschecks_spec_backup.dat"

  local function newRoom(id)
    assert.is_true(addRoom(id))
  end

  setup(function()
    os.remove(backupPath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
  end)

  before_each(function()
    deleteMap()
  end)

  describe("BaseUI.mapHasData", function()
    it("is false for a map without rooms", function()
      assert.is_false(BaseUI.mapHasData())
    end)

    it("is true without reading every room when room 1 exists", function()
      newRoom(1)
      newRoom(2)
      local rooms = spy.on(_G, "getRooms")
      finally(function() getRooms:revert() end)
      assert.is_true(BaseUI.mapHasData())
      assert.spy(rooms).was_not_called()
    end)

    it("is true for a map whose rooms start past room 1", function()
      newRoom(5000)
      assert.is_true(BaseUI.mapHasData())
    end)
  end)

  describe("map.checkVersion", function()
    local savedSilent, savedPath, savedWaiting, savedTimer

    before_each(function()
      savedSilent, savedPath = map.silentUpdate, map.configs.download_path
      savedWaiting, savedTimer = map.update_waiting, map.update_timer
      -- no download path, so the check downloads nothing
      map.configs.download_path = ""
      map.update_waiting, map.update_timer = nil, nil
      map.silentUpdate = true
    end)

    after_each(function()
      map.silentUpdate, map.configs.download_path = savedSilent, savedPath
      map.update_waiting, map.update_timer = savedWaiting, savedTimer
    end)

    it("keeps updates silent for a map without rooms", function()
      map.checkVersion()
      assert.is_true(map.silentUpdate)
    end)

    it("announces updates once room 1 exists", function()
      newRoom(1)
      map.checkVersion()
      assert.is_false(map.silentUpdate)
    end)

    -- it has always asked `#getRooms() > 0`, which is 0 for rooms that do not run on from room 1
    it("keeps updates silent for a map whose rooms start past room 1", function()
      newRoom(5000)
      newRoom(5001)
      map.checkVersion()
      assert.is_true(map.silentUpdate)
    end)

    it("announces updates for a run of rooms from room 2", function()
      for id = 2, 40 do
        newRoom(id)
      end
      map.checkVersion()
      assert.is_false(map.silentUpdate)
    end)
  end)
end)
