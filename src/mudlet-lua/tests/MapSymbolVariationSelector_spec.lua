-- Emoji are often a base character plus a variation selector, or several joined
-- by a zero width joiner. Fonts seldom have a glyph for those two, but they
-- only steer how the rest is drawn, so a symbol holding one has to be drawn
-- from its other characters rather than swapped for the replacement character.
describe("Tests room symbols with a variation selector or zero width joiner", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/variation_selector_symbol_spec_backup.dat"
  local mapPath = mapDirectory .. "/variation_selector_symbol_spec.dat"
  local exportCount = 0
  local exportedPaths = {}

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
    os.remove(mapPath)
    for _, path in ipairs(exportedPaths) do
      os.remove(path)
    end
  end)

  -- The save runs on a pool thread, so wait for the IEND chunk it writes last
  local function exportedImage(areaId)
    exportCount = exportCount + 1
    local imagePath = string.format("%s/variation_selector_symbol_spec_%d.png", getMudletHomeDir(), exportCount)
    exportedPaths[#exportedPaths + 1] = imagePath
    os.remove(imagePath)
    assert.is_false(io.exists(imagePath), "a stale " .. imagePath .. " could not be removed")
    assert.is_true(exportAreaImage(areaId, imagePath))
    for _ = 1, 100 do
      local file = io.open(imagePath, "rb")
      if file then
        local contents = file:read("*a")
        file:close()
        if contents:sub(-8) == "IEND\174B`\130" then
          pumpEvents(100)
          os.remove(imagePath)
          return contents
        end
      end
      pumpEvents(50)
    end
    error("the area image was not written")
  end

  -- A joiner can steer the letter to a different fallback font, so all it is
  -- held to is not turning the symbol into the replacement character
  local followers = {
    {name = "text presentation selector", text = utf8.char(0xFE0E), drawnAsLetter = true},
    {name = "emoji presentation selector", text = utf8.char(0xFE0F), drawnAsLetter = true},
    {name = "zero width joiner", text = utf8.char(0x200D), drawnAsLetter = false},
  }

  for _, onlySelectedFont in ipairs({false, true}) do
    local fontChoice = onlySelectedFont and "only the selected font" or "any font"
    it("draws a letter followed by each of them as the letter, from " .. fontChoice, function()
      deleteMap()
      setConfig("mapSymbolFontOnlyUseSelected", onlySelectedFont)
      local areaId = addAreaName("VariationSelectorSymbolSpec")
      local roomId = createRoomID()
      addRoom(roomId)
      setRoomArea(roomId, areaId)
      setRoomCoordinates(roomId, 0, 0, 0)
      -- the symbol font is only set up for drawing when a map is loaded
      assert.is_true(saveMap(mapPath))
      assert.is_true(loadMap(mapPath))
      openMapWidget("r")
      assert.is_true(setConfig("mapSymbolFont", "Ubuntu Mono"))

      local blank = exportedImage(areaId)
      setRoomChar(roomId, "A")
      local letter = exportedImage(areaId)
      assert.is_true(blank ~= letter, "an ordinary letter left the room blank, so the comparison proves nothing")
      setRoomChar(roomId, utf8.char(0xFFFD))
      local replacement = exportedImage(areaId)
      assert.is_true(letter ~= replacement, "the replacement character draws like the letter, so the comparison proves nothing")

      for _, follower in ipairs(followers) do
        setRoomChar(roomId, "A" .. follower.text)
        local drawn = exportedImage(areaId)
        assert.is_true(drawn ~= replacement, "the letter and its " .. follower.name .. " were drawn as the replacement character")
        assert.is_true(drawn ~= blank, "the letter and its " .. follower.name .. " left the room blank")
        if follower.drawnAsLetter then
          assert.is_true(drawn == letter, "the letter and its " .. follower.name .. " were not drawn as the letter")
        end
        -- setConfig names the symbols the font has no glyph for
        assert.are.same({true}, {setConfig("mapSymbolFont", "Ubuntu Mono")}, "the " .. follower.name .. " was reported as missing from the font")
      end
    end)
  end
end)
