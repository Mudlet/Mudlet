describe("Tests starting the generic mapper", function()
  if type(map) ~= "table" or type(map.start_mapping) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- map.set takes nil, but a table of saved values cannot hold one, hence the list
  local mapperState = {"mapping", "currentRoom", "currentArea", "currentName", "currentExits"}
  local defaultArea = "New area"
  local saved, savedPlayerRoom, areaExisted

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedPlayerRoom = getPlayerRoom()
    areaExisted = getAreaTable()[defaultArea] ~= nil
    map.set("mapping", false)
    map.set("currentRoom", nil)
    map.set("currentArea", nil)
    map.set("currentName", "Where the first map begins")
    map.set("currentExits", {})
  end)

  after_each(function()
    local areaId = getAreaTable()[defaultArea]
    if areaId and not areaExisted then
      deleteArea(areaId)
    end
    for _, key in ipairs(mapperState) do
      map.set(key, saved[key])
    end
    if savedPlayerRoom and roomExists(savedPlayerRoom) then
      centerview(savedPlayerRoom)
    end
  end)

  for _, areaName in ipairs({"", false}) do
    it(string.format("starts mapping in a default area when given %s for the first area", areaName and "an empty name" or "no name"), function()
      assert.has_no.errors(function()
        map.start_mapping(areaName or nil)
      end)
      assert.is_true(map.mapping)
      local areaId = getAreaTable()[defaultArea]
      assert.is_not_nil(areaId)
      assert.are.equal(areaId, map.currentArea)
      assert.are.equal(areaId, getRoomArea(map.currentRoom))
    end)
  end

  it("still uses the area name it is given", function()
    local named = "genericMapperStartMappingSpec"
    finally(function()
      local areaId = getAreaTable()[named]
      if areaId then
        deleteArea(areaId)
      end
    end)
    map.start_mapping(named)
    assert.are.equal(getAreaTable()[named], map.currentArea)
  end)
end)
