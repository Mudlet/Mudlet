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
