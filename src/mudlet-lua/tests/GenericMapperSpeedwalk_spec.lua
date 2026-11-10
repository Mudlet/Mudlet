describe("Tests that the generic mapper handles a walk to the room the player is already in", function()
  if type(map) ~= "table" or type(map.speedwalk) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local areaId, hereRoom, northRoom
  local savedCurrentRoom, savedPath, savedDir, savedPlayerRoom
  local echoes, sent

  local function makeRoom(name, y)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, 0, y, 0)
    setRoomName(id, name)
    return id
  end

  before_each(function()
    savedCurrentRoom = map.currentRoom
    savedPath, savedDir = _G.speedWalkPath, _G.speedWalkDir
    savedPlayerRoom = getPlayerRoom()
    areaId = addAreaName("genericMapperSpeedwalkSpec")
    hereRoom = makeRoom("Speedwalk spec start", 0)
    northRoom = makeRoom("Speedwalk spec north", 1)
    setExit(hereRoom, northRoom, "n")
    map.set("currentRoom", hereRoom)
    echoes, sent = {}, {}
    stub(map, "echo", function(text, _, err)
      echoes[#echoes + 1] = {text = text, err = err}
    end)
    stub(_G, "send", function(command)
      sent[#sent + 1] = command
    end)
  end)

  after_each(function()
    map.echo:revert()
    send:revert()
    map.set("currentRoom", savedCurrentRoom)
    _G.speedWalkPath, _G.speedWalkDir = savedPath, savedDir
    if savedPlayerRoom and roomExists(savedPlayerRoom) then
      centerview(savedPlayerRoom)
    end
    deleteArea(areaId)
  end)

  it("says the player is already there when asked to speedwalk to the current room", function()
    map.speedwalk(hereRoom)
    assert.are.equal(1, #echoes)
    assert.are.equal("You are already in that room.", echoes[1].text)
    assert.is_falsy(echoes[1].err)
    assert.are.same({}, sent)
  end)

  it("says the player is already there when a paused walk is resumed at its goal", function()
    -- Mudlet fills speedWalkPath with room IDs as strings
    _G.speedWalkPath, _G.speedWalkDir = {tostring(hereRoom)}, {"n"}
    -- what map.resumeSpeedwalk() calls
    map.speedwalk(nil)
    assert.are.same({}, _G.speedWalkPath)
    assert.are.same({}, _G.speedWalkDir)
    assert.are.equal(1, #echoes)
    assert.are.equal("You are already in that room.", echoes[1].text)
    assert.is_falsy(echoes[1].err)
    assert.are.same({}, sent)
  end)

  it("says the player is already there when gotoRoom() is asked for the player's room", function()
    centerview(hereRoom)
    gotoRoom(hereRoom)
    assert.are.equal(1, #echoes)
    assert.are.equal("You are already in that room.", echoes[1].text)
    assert.is_falsy(echoes[1].err)
    assert.are.same({}, sent)
  end)

  it("says the player is already there when asked for a path to the current room's name", function()
    map.find_path("Speedwalk spec start")
    assert.are.equal(1, #echoes)
    assert.are.equal("You are already in Speedwalk spec start.", echoes[1].text)
    assert.is_falsy(echoes[1].err)
  end)

  it("still gives the path to another room", function()
    map.find_path("Speedwalk spec north")
    assert.are.equal(1, #echoes)
    assert.are.equal("Path to Speedwalk spec north: n", echoes[1].text)
  end)

  it("still returns no tables for the current room when a script asks for them", function()
    local path, dirs = map.find_path("Speedwalk spec start", nil, true)
    assert.is_nil(path)
    assert.is_nil(dirs)
    assert.are.same({}, echoes)
  end)
end)
