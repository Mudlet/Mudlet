describe("Tests the generic mapper's map path command", function()
  if type(map) ~= "table" or type(map.find_path) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local areaId, otherAreaId, startRoom, streetRoom
  local savedCurrentRoom, savedPath, savedDir
  local echoes

  local function makeRoom(name, y, area)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, area or areaId)
    setRoomCoordinates(id, 0, y, 0)
    setRoomName(id, name)
    return id
  end

  before_each(function()
    savedCurrentRoom = map.currentRoom
    savedPath, savedDir = _G.speedWalkPath, _G.speedWalkDir
    areaId = addAreaName("Map Path Spec Town")
    otherAreaId = addAreaName("Map Path Spec Village")
    startRoom = makeRoom("Map path spec start", 0)
    local alley = makeRoom("Map path spec alley", -1)
    streetRoom = makeRoom("Map path spec street", -2)
    setExit(startRoom, alley, "s")
    setExit(alley, streetRoom, "s")
    -- a nearer room of the same name, but in another area
    local villageStreet = makeRoom("Map path spec street", 1, otherAreaId)
    setExit(startRoom, villageStreet, "n")
    map.set("currentRoom", startRoom)
    echoes = {}
    stub(map, "echo", function(text, _, err)
      echoes[#echoes + 1] = {text = text, err = err}
    end)
  end)

  after_each(function()
    map.echo:revert()
    map.set("currentRoom", savedCurrentRoom)
    _G.speedWalkPath, _G.speedWalkDir = savedPath, savedDir
    deleteArea(areaId)
    deleteArea(otherAreaId)
  end)

  -- the form map help path gives as its example
  it("finds the room when there is a space before the semicolon", function()
    expandAlias("map path map path spec street ; map path spec town", false)
    assert.are.equal(1, #echoes)
    assert.are.equal("Path to map path spec street in map path spec town: s, s", echoes[1].text)
  end)

  it("finds the room without spaces around the semicolon", function()
    expandAlias("map path map path spec street;map path spec town", false)
    assert.are.equal(1, #echoes)
    assert.are.equal("Path to map path spec street in map path spec town: s, s", echoes[1].text)
  end)

  it("finds the nearest room of that name in any area without an area", function()
    expandAlias("map path map path spec street", false)
    assert.are.equal(1, #echoes)
    assert.are.equal("Path to map path spec street: n", echoes[1].text)
  end)

  it("finds no room in an area that does not exist", function()
    expandAlias("map path map path spec street ; no such map path spec area", false)
    assert.are.equal(1, #echoes)
    assert.are.equal("No path found to map path spec street in no such map path spec area.", echoes[1].text)
    assert.is_true(echoes[1].err)
  end)

  it("returns the path within the named area to scripts", function()
    local path, dirs = map.find_path("Map path spec street", "map path spec town", true)
    assert.are.same({"s", "s"}, dirs)
    assert.are.equal(streetRoom, tonumber(path[#path]))
  end)
end)
