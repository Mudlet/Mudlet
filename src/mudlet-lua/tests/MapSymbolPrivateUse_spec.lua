-- Icon fonts put their glyphs in the Private Use Area. The symbol font is set
-- up for drawing when a map is loaded, so the room under test is saved and
-- loaded back before it is drawn; the map this file found is put back after.
describe("Tests room symbols from a font's Private Use Area", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/private_use_symbol_spec_backup.dat"
  local exportCount = 0
  local exportedPaths = {}
  -- the Ubuntu logo, in the Ubuntu Mono font Mudlet bundles
  local ubuntuLogo = utf8.char(0xE0FF)

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
    for _, path in ipairs(exportedPaths) do
      os.remove(path)
    end
  end)

  -- Each export gets a file of its own, so a delete or save that fails (as
  -- Windows can when something holds the file) cannot hand back an earlier
  -- image. The save runs on a pool thread, so wait for the IEND chunk it writes
  -- last, then a little longer for the export to report back before the next
  local function exportedImage(areaId)
    exportCount = exportCount + 1
    local imagePath = string.format("%s/private_use_symbol_spec_%d.png", getMudletHomeDir(), exportCount)
    exportedPaths[#exportedPaths + 1] = imagePath
    os.remove(imagePath)
    assert.is_false(io.exists(imagePath), "a stale " .. imagePath .. " could not be removed")
    assert.is_true(exportAreaImage(areaId, imagePath))
    local contents
    for _ = 1, 100 do
      local file = io.open(imagePath, "rb")
      if file then
        contents = file:read("*a")
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

  local binary = {name = "binary", path = mapDirectory .. "/private_use_symbol_spec.dat", save = saveMap, load = loadMap}
  local json = {name = "JSON", path = mapDirectory .. "/private_use_symbol_spec.json", save = saveJsonMap, load = loadJsonMap}
  for _, case in ipairs({
    {format = binary, onlySelectedFont = false},
    {format = json, onlySelectedFont = false},
    {format = binary, onlySelectedFont = true},
  }) do
    local format = case.format
    local fontChoice = case.onlySelectedFont and "only the selected font" or "any font"
    it("draws a glyph the symbol font has there, from " .. fontChoice .. ", in a map loaded from a " .. format.name .. " file", function()
      finally(function() os.remove(format.path) end)
      deleteMap()
      setConfig("mapSymbolFontOnlyUseSelected", case.onlySelectedFont)
      local areaId = addAreaName("PrivateUseSymbolSpec")
      local roomId = createRoomID()
      addRoom(roomId)
      setRoomArea(roomId, areaId)
      setRoomCoordinates(roomId, 0, 0, 0)
      assert.is_true(format.save(format.path))
      assert.is_true(format.load(format.path))
      assert.are.equal(case.onlySelectedFont, getConfig("mapSymbolFontOnlyUseSelected"))
      openMapWidget("r")
      assert.is_true(setConfig("mapSymbolFont", "Ubuntu Mono"))

      local blank = exportedImage(areaId)
      setRoomChar(roomId, "A")
      assert.are_not.equal(blank, exportedImage(areaId), "an ordinary letter left the room blank, so the comparison proves nothing")
      setRoomChar(roomId, ubuntuLogo)
      -- setConfig names the symbols the font has no glyph for
      assert.are.same({true}, {setConfig("mapSymbolFont", "Ubuntu Mono")})
      local logo = exportedImage(areaId)
      assert.are_not.equal(blank, logo, "the Private Use Area glyph was not drawn")
      setRoomChar(roomId, utf8.char(0xFFFD))
      assert.are_not.equal(exportedImage(areaId), logo, "the glyph was swapped for the replacement character")
    end)
  end
end)
