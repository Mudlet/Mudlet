describe("Tests custom map event and menu functions", function()

  setup(function()
    openMapWidget()
  end)

  after_each(function()
    removeMapEvent("testEvent1")
    removeMapEvent("testEvent2")
    removeMapMenu("TestMenu")
    removeMapMenu("TestSubMenu")
  end)

  describe("Tests addMapEvent and getMapEvents", function()
    it("should add a top-level map event", function()
      addMapEvent("testEvent1", "myEvent", "", "Test Event 1")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("myEvent", events.testEvent1["event name"])
      assert.are.equal("", events.testEvent1["parent"])
      assert.are.equal("Test Event 1", events.testEvent1["display name"])
    end)

    it("should add a map event under a menu", function()
      addMapMenu("TestMenu")
      addMapEvent("testEvent1", "myEvent", "TestMenu", "Test Event 1")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("TestMenu", events.testEvent1["parent"])
    end)

    it("should use unique name as display text when not provided", function()
      addMapEvent("testEvent1", "myEvent")

      local events = getMapEvents()
      assert.is_not_nil(events.testEvent1)
      assert.are.equal("testEvent1", events.testEvent1["display name"])
    end)
  end)

  describe("Tests addMapMenu and getMapMenus", function()
    it("should add a top-level menu", function()
      addMapMenu("TestMenu")

      local menus = getMapMenus()
      assert.are.equal("top-level", menus["TestMenu"])
    end)

    it("should add a nested submenu", function()
      addMapMenu("TestMenu")
      addMapMenu("TestSubMenu", "TestMenu")

      local menus = getMapMenus()
      assert.are.equal("top-level", menus["TestMenu"])
      assert.are.equal("TestMenu", menus["TestSubMenu"])
    end)
  end)

  describe("Tests removeMapEvent", function()
    it("should remove an event", function()
      addMapEvent("testEvent1", "myEvent", "", "Test Event 1")
      removeMapEvent("testEvent1")

      local events = getMapEvents()
      assert.is_nil(events.testEvent1)
    end)
  end)

  describe("Tests removeMapMenu", function()
    it("should remove a menu and its children", function()
      addMapMenu("TestMenu")
      addMapMenu("TestSubMenu", "TestMenu")
      removeMapMenu("TestMenu")

      local menus = getMapMenus()
      assert.is_nil(menus["TestMenu"])
      assert.is_nil(menus["TestSubMenu"])
    end)
  end)

end)

describe("Tests per-room border functions", function()

  local testRoomId

  setup(function()
    -- Create a test area and room
    local areaId = addAreaName("TestBorderArea")
    testRoomId = createRoomID()
    addRoom(testRoomId)
    setRoomArea(testRoomId, areaId)
    setRoomCoordinates(testRoomId, 0, 0, 0)
  end)

  teardown(function()
    -- Clean up test room and area
    deleteRoom(testRoomId)
    deleteArea("TestBorderArea")
  end)

  describe("Tests setRoomBorderColor", function()
    it("should set border color with RGB", function()
      local result = setRoomBorderColor(testRoomId, 255, 0, 0)
      assert.is_true(result)

      local r, g, b, a = getRoomBorderColor(testRoomId)
      assert.are.equal(255, r)
      assert.are.equal(0, g)
      assert.are.equal(0, b)
      assert.are.equal(255, a) -- default alpha
    end)

    it("should set border color with RGBA", function()
      local result = setRoomBorderColor(testRoomId, 0, 255, 0, 128)
      assert.is_true(result)

      local r, g, b, a = getRoomBorderColor(testRoomId)
      assert.are.equal(0, r)
      assert.are.equal(255, g)
      assert.are.equal(0, b)
      assert.are.equal(128, a)
    end)

    it("should return nil for invalid color values", function()
      local result, err = setRoomBorderColor(testRoomId, 256, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)

      result, err = setRoomBorderColor(testRoomId, -1, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for invalid room ID", function()
      local result, err = setRoomBorderColor(-999, 255, 0, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

  describe("Tests getRoomBorderColor", function()
    it("should return nil when no custom color is set", function()
      clearRoomBorderColor(testRoomId)
      local result = getRoomBorderColor(testRoomId)
      assert.is_nil(result)
    end)
  end)

  describe("Tests clearRoomBorderColor", function()
    it("should clear the border color", function()
      setRoomBorderColor(testRoomId, 255, 0, 0)
      local result = clearRoomBorderColor(testRoomId)
      assert.is_true(result)
      assert.is_nil(getRoomBorderColor(testRoomId))
    end)
  end)

  describe("Tests setRoomBorderThickness", function()
    it("should set valid thickness", function()
      local result = setRoomBorderThickness(testRoomId, 5)
      assert.is_true(result)
      assert.are.equal(5, getRoomBorderThickness(testRoomId))
    end)

    it("should return nil for thickness below 1", function()
      local result, err = setRoomBorderThickness(testRoomId, 0)
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for thickness above 10", function()
      local result, err = setRoomBorderThickness(testRoomId, 11)
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

  describe("Tests getRoomBorderThickness", function()
    it("should return nil when using global default", function()
      clearRoomBorderThickness(testRoomId)
      local result = getRoomBorderThickness(testRoomId)
      assert.is_nil(result)
    end)
  end)

  describe("Tests clearRoomBorderThickness", function()
    it("should clear the thickness", function()
      setRoomBorderThickness(testRoomId, 3)
      local result = clearRoomBorderThickness(testRoomId)
      assert.is_true(result)
      assert.is_nil(getRoomBorderThickness(testRoomId))
    end)
  end)

end)

describe("Tests map info functions", function()

  describe("Tests getMapInfo", function()
    it("should return a table with contributor states", function()
      local info = getMapInfo()
      assert.is_table(info)
      -- "Short" is a built-in contributor and should exist
      assert.is_not_nil(info["Short"])
    end)

    it("should reflect enabled/disabled state", function()
      enableMapInfo("Short")
      local info = getMapInfo()
      assert.is_true(info["Short"])

      disableMapInfo("Short")
      info = getMapInfo()
      assert.is_false(info["Short"])

      -- Re-enable for clean state
      enableMapInfo("Short")
    end)
  end)

  describe("Tests enableMapInfo and disableMapInfo", function()
    it("should return nil for non-existent contributor", function()
      local result, err = enableMapInfo("NonExistentContributor")
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("should return nil for non-existent contributor on disable", function()
      local result, err = disableMapInfo("NonExistentContributor")
      assert.is_nil(result)
      assert.is_string(err)
    end)
  end)

end)
