describe("Tests the deleteOldProfiles package", function()
  if type(deleteOldProfiles) ~= "function" then
    it("needs the deleteOldProfiles package installed", function()
      pending("the deleteOldProfiles package is not installed in this profile")
    end)
    return
  end

  local realGetMudletHomeDir = getMudletHomeDir
  local specHome = realGetMudletHomeDir():gsub("\\", "/") .. "/delete_old_profiles_spec"
  local folders = {
    profiles = specHome .. "/current",
    maps = specHome .. "/map",
  }
  local made = {}

  local function makeFile(folder, name, day, month)
    local path = folders[folder] .. "/" .. name
    local file = assert(io.open(path, "w"))
    file:write("<MudletPackage/>")
    file:close()
    local when = os.time({year = 2020, month = month or 3, day = day, hour = 12})
    assert.is_true(lfs.touch(path, when, when))
    made[#made + 1] = path
  end

  local function exists(folder, name)
    return lfs.attributes(folders[folder] .. "/" .. name, "mode") == "file"
  end

  local function makeDirectory(folder, name, day, month)
    local path = folders[folder] .. "/" .. name
    assert(lfs.mkdir(path))
    local when = os.time({year = 2020, month = month or 3, day = day, hour = 12})
    assert.is_true(lfs.touch(path, when, when))
    made[#made + 1] = path
  end

  local function deleteOld(folder)
    _G.getMudletHomeDir = function() return specHome end
    local ok, err = pcall(deleteOldProfiles, nil, folder)
    _G.getMudletHomeDir = realGetMudletHomeDir
    assert.is_true(ok, err)
  end

  local function removeMade()
    for _, path in ipairs(made) do
      if lfs.attributes(path, "mode") == "directory" then
        lfs.rmdir(path)
      else
        os.remove(path)
      end
    end
    made = {}
  end

  setup(function()
    lfs.mkdir(specHome)
    lfs.mkdir(folders.profiles)
    lfs.mkdir(folders.maps)
  end)

  after_each(removeMade)

  teardown(function()
    _G.getMudletHomeDir = realGetMudletHomeDir
    removeMade()
    lfs.rmdir(folders.profiles)
    lfs.rmdir(folders.maps)
    lfs.rmdir(specHome)
  end)

  it("keeps the newest save, which Mudlet loads, even when it is older than the cut-off", function()
    makeFile("profiles", "2020-03-01#12-00-00.xml", 1)
    makeFile("profiles", "2020-03-10#12-00-00.xml", 10)
    makeFile("profiles", "2020-03-20#12-00-00.xml", 20)
    -- what an interrupted save leaves behind, which Mudlet does not load
    makeFile("profiles", "2020-03-21#12-00-00.xml.AbCdEf", 21)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-03-01#12-00-00.xml"), "the month's first save should be kept")
    assert.is_false(exists("profiles", "2020-03-10#12-00-00.xml"), "an old save that is neither its month's first nor the newest should go")
    assert.is_true(exists("profiles", "2020-03-20#12-00-00.xml"), "the newest save, the one the profile loads, should be kept")
    assert.is_false(exists("profiles", "2020-03-21#12-00-00.xml.AbCdEf"), "a leftover temporary file is not a save, so it should not stand in for the newest one")
  end)

  it("deletes a leftover newer than the month's only save, which is the newest", function()
    makeFile("profiles", "2020-03-20#12-00-00.xml", 20)
    makeFile("profiles", "2020-03-21#12-00-00.xml.AbCdEf", 21)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-03-20#12-00-00.xml"), "the newest save should be kept")
    assert.is_false(exists("profiles", "2020-03-21#12-00-00.xml.AbCdEf"), "the newest save is its month's kept file, so the leftover should go")
  end)

  it("keeps a month's first real save rather than an older leftover", function()
    makeFile("profiles", "2020-02-01#12-00-00.xml.AbCdEf", 1, 2)
    makeFile("profiles", "2020-02-05#12-00-00.xml", 5, 2)
    makeFile("profiles", "2020-02-09#12-00-00.xml", 9, 2)
    makeFile("profiles", "2020-03-20#12-00-00.xml", 20)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-02-05#12-00-00.xml"), "February's first real save should be kept")
    assert.is_false(exists("profiles", "2020-02-01#12-00-00.xml.AbCdEf"), "a leftover is not a save, so it should not take February's spot")
    assert.is_false(exists("profiles", "2020-02-09#12-00-00.xml"))
    assert.is_true(exists("profiles", "2020-03-20#12-00-00.xml"))
  end)

  it("keeps every map tied for the newest, whatever its format or letter case, but not a newer leftover", function()
    makeFile("maps", "2020-03-01#12-00-00map.dat", 1)
    makeFile("maps", "2020-03-10#12-00-00map.dat", 10)
    makeFile("maps", "2020-03-20#12-00-00map.DAT", 20)
    makeFile("maps", "2020-03-20#12-00-00map.json", 20)
    makeFile("maps", "2020-03-21#12-00-00map.dat.AbCdEf", 21)

    deleteOld("maps")

    assert.is_true(exists("maps", "2020-03-01#12-00-00map.dat"), "the month's first map should be kept")
    assert.is_false(exists("maps", "2020-03-10#12-00-00map.dat"), "an old map that is neither its month's first nor the newest should go")
    assert.is_true(exists("maps", "2020-03-20#12-00-00map.DAT"), "a binary map tied for the newest should be kept")
    assert.is_true(exists("maps", "2020-03-20#12-00-00map.json"), "a JSON map tied for the newest should be kept")
    assert.is_false(exists("maps", "2020-03-21#12-00-00map.dat.AbCdEf"), "a leftover temporary file is not a map")
  end)

  it("keeps every save tied for the newest, as Mudlet may load any of them", function()
    makeFile("profiles", "2020-03-01#12-00-00.xml", 1)
    makeFile("profiles", "2020-03-20#12-00-00.xml", 20)
    makeFile("profiles", "autosave.xml", 20)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-03-20#12-00-00.xml"))
    assert.is_true(exists("profiles", "autosave.xml"))
  end)

  it("keeps a leftover from an interrupted save when there is no completed save to load", function()
    makeFile("profiles", "2020-03-01#12-00-00.xml.AbCdEf", 1)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-03-01#12-00-00.xml.AbCdEf"), "with no completed save, the leftover may be all that is left of the profile")
  end)

  it("lets an XML map, which Map history can load, keep its month's spot", function()
    makeFile("maps", "2020-02-05#12-00-00map.xml", 5, 2)
    makeFile("maps", "2020-03-20#12-00-00map.dat", 20)

    deleteOld("maps")

    assert.is_true(exists("maps", "2020-02-05#12-00-00map.xml"), "February's only map should be kept")
    assert.is_true(exists("maps", "2020-03-20#12-00-00map.dat"))
  end)

  it("leaves folders alone", function()
    makeDirectory("maps", "old maps", 1)
    makeFile("maps", "2020-03-05#12-00-00map.dat", 5)

    deleteOld("maps")

    assert.is_true(lfs.attributes(folders.maps .. "/old maps", "mode") == "directory", "a folder is not a backup, so it should not be removed")
    assert.is_true(exists("maps", "2020-03-05#12-00-00map.dat"))
  end)
end)
