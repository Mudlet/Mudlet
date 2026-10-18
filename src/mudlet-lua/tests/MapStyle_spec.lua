local function readFile(path)
  local handle = io.open(path, "rb")
  if not handle then
    return nil
  end
  local contents = handle:read("*a")
  handle:close()
  return contents
end

local testMode = os.getenv("MUDLET_TEST_MODE")

describe("Tests the modern 3D mapper's style through setConfig and getConfig", function()
  local originalStyle

  setup(function()
    originalStyle = getConfig("mapStyle")
  end)

  teardown(function()
    setConfig("mapStyle", originalStyle)
  end)

  it("names the style in use", function()
    assert.equals("string", type(getConfig("mapStyle")))
  end)

  it("starts a profile on the tiles style", function()
    assert.equals("tiles", originalStyle)
  end)

  it("sets a style that exists", function()
    assert.is_true(setConfig("mapStyle", "tiles"))
    assert.equals("tiles", getConfig("mapStyle"))
  end)

  it("switches between the tiles and blueprint styles", function()
    assert.is_true(setConfig("mapStyle", "blueprint"))
    assert.equals("blueprint", getConfig("mapStyle"))
    assert.equals("blueprint", getConfig()["mapStyle"])
    assert.is_true(setConfig("mapStyle", "tiles"))
    assert.equals("tiles", getConfig("mapStyle"))
    assert.is_true(setConfig("mapStyle", "blueprint"))
    assert.equals("blueprint", getConfig("mapStyle"))
  end)

  it("switches to the terminal style and back from either of the others", function()
    for _, from in ipairs({"tiles", "blueprint"}) do
      assert.is_true(setConfig("mapStyle", from))
      assert.is_true(setConfig("mapStyle", "terminal"))
      assert.equals("terminal", getConfig("mapStyle"))
      assert.equals("terminal", getConfig()["mapStyle"])
      assert.is_true(setConfig("mapStyle", from))
      assert.equals(from, getConfig("mapStyle"))
    end
  end)

  it("switches to the transit style and back from each of the others", function()
    for _, from in ipairs({"tiles", "blueprint", "terminal"}) do
      assert.is_true(setConfig("mapStyle", from))
      assert.is_true(setConfig("mapStyle", "transit"))
      assert.equals("transit", getConfig("mapStyle"))
      assert.equals("transit", getConfig()["mapStyle"])
      assert.is_true(setConfig("mapStyle", from))
      assert.equals(from, getConfig("mapStyle"))
    end
  end)

  it("refuses a style that does not exist, naming the ones that do, and keeps the one in use", function()
    assert.is_true(setConfig("mapStyle", "blueprint"))
    local ok, err = setConfig("mapStyle", "no such style")
    assert.is_nil(ok)
    assert.equals('mapStyle must be "tiles", "blueprint", "terminal" or "transit", got "no such style"', err)
    assert.equals("blueprint", getConfig("mapStyle"))
  end)

  it("matches style names exactly", function()
    assert.is_true(setConfig("mapStyle", "tiles"))
    assert.is_nil(setConfig("mapStyle", "Tiles"))
    assert.is_nil(setConfig("mapStyle", "Blueprint"))
    assert.is_nil(setConfig("mapStyle", "Terminal"))
    assert.is_nil(setConfig("mapStyle", "TERMINAL"))
    assert.is_nil(setConfig("mapStyle", "Transit"))
    assert.equals("tiles", getConfig("mapStyle"))
  end)

  it("is listed among every setting getConfig() reports", function()
    assert.equals(getConfig("mapStyle"), getConfig()["mapStyle"])
  end)

  it("keeps the style while the map widget closes and opens", function()
    assert.is_true(setConfig("mapStyle", "blueprint"))
    closeMapWidget()
    assert.is_true(openMapWidget())
    assert.equals("blueprint", getConfig("mapStyle"))
    closeMapWidget()
  end)

  -- Only saveProfile() without a file name writes the profile's own settings. The write runs on a pool
  -- thread, so the file turns up after saveProfile() has answered, and only test mode can pump the
  -- event loop to wait for it
  for _, style in ipairs({"terminal", "transit"}) do
    it("saves the " .. style .. " style with the profile", function()
      if not testMode then
        return
      end
      assert.is_true(setConfig("mapStyle", style))
      local folder = getMudletHomeDir() .. "/mapstyle-spec"
      lfs.mkdir(folder)
      local saved, path
      for _ = 1, 100 do
        saved, path = saveProfile(folder)
        if saved or not tostring(path):find("a save is already in progress", 1, true) then
          break
        end
        pumpEvents(50)
      end
      assert.is_true(saved, path)
      local contents
      for _ = 1, 200 do
        contents = readFile(path)
        if contents and contents:find("</MudletPackage>", 1, true) then
          break
        end
        pumpEvents(50)
      end
      os.remove(path)
      lfs.rmdir(folder)
      assert.is_truthy(contents and contents:find('mapStyle="' .. style .. '"', 1, true), "the saved profile does not name the " .. style .. " style")
    end)
  end
end)
