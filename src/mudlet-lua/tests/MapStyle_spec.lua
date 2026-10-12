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

  it("refuses a style that does not exist, naming the ones that do, and keeps the one in use", function()
    assert.is_true(setConfig("mapStyle", "tiles"))
    local ok, err = setConfig("mapStyle", "no such style")
    assert.is_nil(ok)
    assert.equals('mapStyle must be "tiles", got "no such style"', err)
    assert.equals("tiles", getConfig("mapStyle"))
  end)

  it("matches style names exactly", function()
    local ok = setConfig("mapStyle", "Tiles")
    assert.is_nil(ok)
    assert.equals("tiles", getConfig("mapStyle"))
  end)

  it("is listed among every setting getConfig() reports", function()
    assert.equals(getConfig("mapStyle"), getConfig()["mapStyle"])
  end)

  it("keeps the style while the map widget closes and opens", function()
    assert.is_true(setConfig("mapStyle", "tiles"))
    closeMapWidget()
    assert.is_true(openMapWidget())
    assert.equals("tiles", getConfig("mapStyle"))
    closeMapWidget()
  end)

  -- Only saveProfile() without a file name writes the profile's own settings. The write runs on a pool
  -- thread, so the file turns up after saveProfile() has answered, and only test mode can pump the
  -- event loop to wait for it
  it("saves the style with the profile", function()
    if not testMode then
      return
    end
    assert.is_true(setConfig("mapStyle", "tiles"))
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
    assert.is_truthy(contents and contents:find('mapStyle="tiles"', 1, true), "the saved profile has no mapStyle")
  end)
end)
