-- The kinds of ground the 3D map styles draw environments' rooms as. saveMap/loadMap
-- replace the whole map, so the map this file found is saved first and put back.
describe("Tests setMapEnvironmentKit, getMapEnvironmentKit and getMapEnvironmentKits", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/environment_kit_spec_backup.dat"
  local savePath = mapDirectory .. "/environment_kit_spec_roundtrip.dat"
  local userDataKey = "system.mapStyle.environmentKits"
  -- custom environments well clear of the 16 colours every map has
  local blue, green, grey, tan, red, nearBlack, nearWhite = 9310, 9311, 9312, 9313, 9314, 9315, 9316
  local brightOrange, darkBrown = 9317, 9318

  setup(function()
    os.remove(backupPath)
    os.remove(savePath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this file replaced could not be put back")
    -- loadMap shows the mapper wherever it last was, and the specs that run
    -- after this file are entitled to an open, right-docked widget
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(savePath)
  end)

  before_each(function()
    deleteMap()
    setCustomEnvColor(blue, 60, 120, 200)
    setCustomEnvColor(green, 90, 160, 70)
    setCustomEnvColor(grey, 170, 165, 155)
    setCustomEnvColor(tan, 196, 170, 128)
    setCustomEnvColor(red, 190, 70, 50)
    setCustomEnvColor(nearBlack, 12, 10, 20)
    setCustomEnvColor(nearWhite, 245, 245, 250)
    setCustomEnvColor(brightOrange, 230, 130, 20)
    setCustomEnvColor(darkBrown, 100, 60, 30)
  end)

  it("guesses a kit from an environment's colour until one is named", function()
    local guesses = {
      [blue] = "water", [green] = "forest", [grey] = "road", [tan] = "road",
      [red] = "building", [nearBlack] = "road", [nearWhite] = "building",
      -- either side of the line between a painted building and a brown road
      [brightOrange] = "building", [darkBrown] = "road",
    }
    for environment, expected in pairs(guesses) do
      local kit, named = getMapEnvironmentKit(environment)
      assert.are.equal(expected, kit, "environment " .. environment)
      assert.is_false(named)
    end
    assert.are.same({}, getMapEnvironmentKits())
  end)

  it("follows the environment's colour when it changes", function()
    assert.are.equal("water", (getMapEnvironmentKit(blue)))
    setCustomEnvColor(blue, 40, 150, 40)
    assert.are.equal("forest", (getMapEnvironmentKit(blue)))
  end)

  it("overrides a guess with a named kit, and clears it with nil or an empty name", function()
    assert.is_true(setMapEnvironmentKit(grey, "water"))
    assert.are.same({"water", true}, {getMapEnvironmentKit(grey)})
    assert.is_true(setMapEnvironmentKit(red, "Plain"))
    assert.are.same({[grey] = "water", [red] = "plain"}, getMapEnvironmentKits())

    assert.is_true(setMapEnvironmentKit(grey, nil))
    assert.are.same({"road", false}, {getMapEnvironmentKit(grey)})
    assert.is_true(setMapEnvironmentKit(red, ""))
    assert.are.same({"building", false}, {getMapEnvironmentKit(red)})
    assert.are.same({}, getMapEnvironmentKits())
    assert.is_nil(getMapUserData(userDataKey))

    -- clearing one that was never named is no error
    assert.is_true(setMapEnvironmentKit(green))
  end)

  it("refuses a kit it does not know, leaving the one named before", function()
    assert.is_true(setMapEnvironmentKit(green, "road"))
    local ok, message = setMapEnvironmentKit(green, "castle")
    assert.is_nil(ok)
    assert.truthy(message:find("castle", 1, true))
    assert.truthy(message:find("building", 1, true))
    assert.are.same({"road", true}, {getMapEnvironmentKit(green)})
  end)

  it("rejects arguments of the wrong type", function()
    assert.has_error(function() setMapEnvironmentKit("blue", "water") end)
    assert.has_error(function() setMapEnvironmentKit(blue, {}) end)
    assert.has_error(function() getMapEnvironmentKit() end)
    assert.has_error(function() getMapEnvironmentKit({}) end)
  end)

  it("keeps the named kits in the map's user data, and through saving and loading the map", function()
    assert.is_true(setMapEnvironmentKit(grey, "water"))
    assert.is_true(setMapEnvironmentKit(-1, "forest"))
    local stored = yajl.to_value(getMapUserData(userDataKey))
    assert.are.same({[tostring(grey)] = "water", ["-1"] = "forest"}, stored)

    assert.is_true(saveMap(savePath))
    assert.is_true(setMapEnvironmentKit(grey, nil))
    assert.is_true(setMapEnvironmentKit(-1, nil))
    assert.are.same({}, getMapEnvironmentKits())
    assert.is_true(loadMap(savePath))
    assert.are.same({[grey] = "water", [-1] = "forest"}, getMapEnvironmentKits())
    assert.are.same({"water", true}, {getMapEnvironmentKit(grey)})
  end)

  it("leaves out saved entries it cannot read, and keeps them when another is set", function()
    setMapUserData(userDataKey, '{"12":"water","x":"road","13":"castle"}')
    assert.are.same({[12] = "water"}, getMapEnvironmentKits())
    assert.are.same({"water", true}, {getMapEnvironmentKit(12)})
    assert.are.same({"road", false}, {getMapEnvironmentKit(grey)})

    assert.is_true(setMapEnvironmentKit(14, "road"))
    assert.are.same({[12] = "water", [14] = "road"}, getMapEnvironmentKits())
    assert.are.same({["12"] = "water", ["x"] = "road", ["13"] = "castle", ["14"] = "road"},
      yajl.to_value(getMapUserData(userDataKey)))

    assert.is_true(setMapEnvironmentKit(12, nil))
    assert.are.same({["x"] = "road", ["13"] = "castle", ["14"] = "road"}, yajl.to_value(getMapUserData(userDataKey)))
  end)

  it("reads saved kits that are not JSON as none, and will not overwrite them", function()
    setMapUserData(userDataKey, "not json")
    assert.are.same({}, getMapEnvironmentKits())
    assert.are.same({"water", false}, {getMapEnvironmentKit(blue)})

    local ok, message = setMapEnvironmentKit(14, "road")
    assert.is_nil(ok)
    assert.truthy(message:find(userDataKey, 1, true))
    assert.are.equal("not json", getMapUserData(userDataKey))
    assert.are.same({}, getMapEnvironmentKits())
  end)

  describe("unsetCustomEnvColor", function()
    it("removes a custom environment colour, and it stays removed through saving and loading the map", function()
      assert.is_not_nil(getCustomEnvColorTable()[blue])
      assert.is_true(unsetCustomEnvColor(blue))
      assert.is_nil(getCustomEnvColorTable()[blue])
      assert.is_not_nil(getCustomEnvColorTable()[green])

      assert.is_true(saveMap(savePath))
      assert.is_true(loadMap(savePath))
      assert.is_nil(getCustomEnvColorTable()[blue])
      assert.is_not_nil(getCustomEnvColorTable()[green])
    end)

    it("refuses an environment that has no custom colour", function()
      local ok, message = unsetCustomEnvColor(9399)
      assert.is_nil(ok)
      assert.truthy(message:find("9399", 1, true))
    end)

    it("refuses the 16 default colours", function()
      for environment = 257, 272 do
        local ok, message = unsetCustomEnvColor(environment)
        assert.is_nil(ok, "environment " .. environment)
        assert.truthy(message:find(tostring(environment), 1, true))
        assert.is_not_nil(getCustomEnvColorTable()[environment])
      end
    end)

    it("refuses a colour a room still uses, until the room stops using it", function()
      local roomId = createRoomID()
      assert.is_true(addRoom(roomId))
      assert.is_true(setRoomEnv(roomId, grey))

      local ok, message = unsetCustomEnvColor(grey)
      assert.is_nil(ok)
      assert.truthy(message:find("room ID " .. roomId, 1, true), message)
      assert.is_not_nil(getCustomEnvColorTable()[grey])

      assert.is_true(setRoomEnv(roomId, tan))
      assert.is_true(unsetCustomEnvColor(grey))
      assert.is_nil(getCustomEnvColorTable()[grey])
    end)

    it("rejects an environment ID that is not a number", function()
      assert.has_error(function() unsetCustomEnvColor("grey") end)
      assert.has_error(function() unsetCustomEnvColor() end)
    end)
  end)
end)
