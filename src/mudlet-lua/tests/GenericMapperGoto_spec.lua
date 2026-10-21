describe("Tests the generic mapper's goto alias", function()
  if type(map) ~= "table" or type(map.eventHandler) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local areaId, start, east, farEast, west, island
  local savedCurrentRoom, savedFeatures, savedDoSpeedWalk
  local walkedTo

  local function makeRoom(name, x)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, x, 0, 0)
    setRoomName(id, name)
    return id
  end

  local function link(from, to, dir, back)
    setExit(from, to, dir)
    setExit(to, from, back)
  end

  before_each(function()
    savedCurrentRoom = map.currentRoom
    savedFeatures = getMapUserData("mapFeatures")
    savedDoSpeedWalk = _G.doSpeedWalk
    walkedTo = nil
    _G.doSpeedWalk = function()
      walkedTo = tonumber(speedWalkPath[#speedWalkPath])
    end

    areaId = addAreaName("genericMapperGotoSpec")
    start = makeRoom("Where the walk starts", 0)
    east = makeRoom("One step east", 1)
    farEast = makeRoom("Two steps east", 2)
    west = makeRoom("One step west", -1)
    island = makeRoom("Nowhere to be reached", 10)
    link(start, east, "e", "w")
    link(east, farEast, "e", "w")
    link(start, west, "w", "e")

    map.createMapFeature("spec fountain")
    map.createMapFeature("spec fountain of youth")
    map.createMapFeature("spec shop")
    map.roomCreateMapFeature("spec fountain", farEast)
    map.roomCreateMapFeature("spec fountain", west)
    map.roomCreateMapFeature("spec fountain of youth", farEast)
    map.roomCreateMapFeature("spec shop", island)
    map.set("currentRoom", start)
  end)

  after_each(function()
    _G.doSpeedWalk = savedDoSpeedWalk
    map.set("currentRoom", savedCurrentRoom)
    setMapUserData("mapFeatures", savedFeatures or "")
    deleteArea(areaId)
  end)

  it("walks to a room by its number", function()
    expandAlias("goto " .. farEast, false)
    assert.equals(farEast, walkedTo)
  end)

  it("walks to the nearest room with a feature", function()
    expandAlias("goto spec fountain", false)
    assert.equals(west, walkedTo)
  end)

  it("takes the feature named exactly over a longer one containing it", function()
    assert.is_true(map.gotoFeature("SPEC FOUNTAIN"))
    assert.equals(west, walkedTo)
  end)

  it("walks to a feature given part of its name", function()
    expandAlias("goto youth", false)
    assert.equals(farEast, walkedTo)
  end)

  it("does not walk when part of a name matches several features", function()
    assert.is_nil(map.gotoFeature("spec"))
    assert.is_nil(walkedTo)
  end)

  it("does not walk to a feature that does not exist", function()
    assert.is_nil(map.gotoFeature("spec tavern"))
    assert.is_nil(walkedTo)
  end)

  it("does not walk when the current room already has the feature", function()
    map.set("currentRoom", west)
    assert.is_nil(map.gotoFeature("spec fountain"))
    assert.is_nil(walkedTo)
  end)

  it("does not walk when no room with the feature can be reached", function()
    assert.is_nil(map.gotoFeature("spec shop"))
    assert.is_nil(walkedTo)
  end)

  it("does not walk to a room that does not exist", function()
    deleteRoom(island)
    assert.is_nil(map.gotoRoom(island))
    assert.is_nil(walkedTo)
  end)

  it("does not walk to a room it cannot reach", function()
    assert.is_nil(map.gotoRoom(island))
    assert.is_nil(walkedTo)
  end)

  it("does not walk when it does not know where the player is", function()
    map.set("currentRoom", nil)
    assert.is_nil(map.gotoRoom(farEast))
    assert.is_nil(walkedTo)
  end)
end)
