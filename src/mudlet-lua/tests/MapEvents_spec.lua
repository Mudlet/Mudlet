-- Every test here empties or replaces the whole map, so the map is saved first and put back after.

describe("Tests the events raised when a map is loaded or deleted", function()
  local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
  assert(specDirectory, "MapEvents_spec.lua has to be run from a file so that it can find its fixtures")
  local fixtureMap = specDirectory .. "/fixtures/maps/minimal-map.xml"

  -- Scratch names nothing else writes. A run that died before its teardown leaves them behind, so they are
  -- cleared on the way in rather than trusted.
  local backupPath = getMudletHomeDir() .. "/map/mapevents_spec_backup.dat"
  local datPath = getMudletHomeDir() .. "/mapevents_spec.dat"
  local jsonPath = getMudletHomeDir() .. "/mapevents_spec.json"
  local brokenXmlPath = getMudletHomeDir() .. "/mapevents_spec_broken.xml"
  local scratchFiles = {backupPath, datPath, jsonPath, brokenXmlPath}

  local events, handlerIds

  setup(function()
    for _, path in ipairs(scratchFiles) do
      os.remove(path)
    end
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
    -- saveJsonMap() and loadJsonMap() refuse to run until a map window exists
    openMapWidget("r")

    deleteMap()
    addRoom(createRoomID())
    assert.is_true(saveMap(datPath))
    assert.is_true(saveJsonMap(jsonPath))
    local file = assert(io.open(brokenXmlPath, "w"))
    file:write('<map><rooms><room id="1"')
    file:close()
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    for _, path in ipairs(scratchFiles) do
      os.remove(path)
    end
  end)

  before_each(function()
    deleteMap()
    events = {}
    handlerIds = {}
    for _, name in ipairs({"sysMapLoaded", "sysMapDeleted"}) do
      -- how many rooms the map holds by the time the handler runs
      handlerIds[#handlerIds + 1] = registerAnonymousEventHandler(name, function(event)
        events[#events + 1] = event .. ":" .. table.size(getRooms())
      end)
    end
  end)

  after_each(function()
    for _, id in ipairs(handlerIds) do
      killAnonymousEventHandler(id)
    end
  end)

  it("raises sysMapDeleted once deleteMap() has emptied the map", function()
    addRoom(createRoomID())
    assert.is_true(deleteMap())
    assert.are.same({"sysMapDeleted:0"}, events)
  end)

  it("raises sysMapDeleted for a map that was already empty", function()
    assert.is_true(deleteMap())
    assert.are.same({"sysMapDeleted:0"}, events)
  end)

  it("raises sysMapLoaded once a binary map has loaded", function()
    assert.is_true(loadMap(datPath))
    assert.are.same({"sysMapLoaded:1"}, events)
  end)

  it("raises sysMapLoaded once a JSON map has loaded", function()
    assert.is_true(loadJsonMap(jsonPath))
    assert.are.same({"sysMapLoaded:1"}, events)
  end)

  it("raises sysMapLoaded once an XML map has loaded", function()
    assert.is_true(loadMap(fixtureMap))
    assert.are.same({"sysMapLoaded:2"}, events)
  end)

  it("raises nothing for a load that fails", function()
    assert.is_false(loadMap(getMudletHomeDir() .. "/mapevents_spec_missing.dat"))
    assert.is_nil(loadMap(getMudletHomeDir() .. "/mapevents_spec_missing.xml"))
    assert.is_nil(loadMap(brokenXmlPath))
    assert.is_nil(loadJsonMap(getMudletHomeDir() .. "/mapevents_spec_missing.json"))
    assert.are.same({}, events)
  end)
end)
