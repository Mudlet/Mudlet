describe("Tests the deleteOldProfiles package", function()
  if type(deleteOldProfiles) ~= "function" then
    it("needs the deleteOldProfiles package installed", function()
      pending("the deleteOldProfiles package is not installed in this profile")
    end)
    return
  end

  local realGetMudletHomeDir = getMudletHomeDir
  local specRoot = realGetMudletHomeDir():gsub("\\", "/") .. "/delete_old_profiles_spec"
  -- module backups live two folders above a profile's home, as they do for real profiles
  local specHome = specRoot .. "/profiles/spec"
  local folders = {
    profiles = specHome .. "/current",
    maps = specHome .. "/map",
    modules = specRoot .. "/moduleBackups",
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
    made[#made + 1] = path
    local when = os.time({year = 2020, month = month or 3, day = day, hour = 12})
    -- the Windows C runtime may refuse to set a folder's times
    return lfs.touch(path, when, when) == true
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
    lfs.mkdir(specRoot)
    lfs.mkdir(specRoot .. "/profiles")
    lfs.mkdir(specHome)
    lfs.mkdir(folders.profiles)
    lfs.mkdir(folders.maps)
    lfs.mkdir(folders.modules)
  end)

  after_each(removeMade)

  teardown(function()
    _G.getMudletHomeDir = realGetMudletHomeDir
    removeMade()
    lfs.rmdir(folders.profiles)
    lfs.rmdir(folders.maps)
    lfs.rmdir(folders.modules)
    lfs.rmdir(specHome)
    lfs.rmdir(specRoot .. "/profiles")
    lfs.rmdir(specRoot)
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

  it("leaves folders alone, and does not let one take its month's spot", function()
    if not makeDirectory("maps", "old maps", 1) then
      pending("this platform cannot back-date a folder")
      return
    end
    makeFile("maps", "2020-03-05#12-00-00map.dat", 5)

    deleteOld("maps")

    assert.is_true(lfs.attributes(folders.maps .. "/old maps", "mode") == "directory", "a folder is not a backup, so it should not be removed")
    assert.is_true(exists("maps", "2020-03-05#12-00-00map.dat"), "March's only map should keep March's spot")
  end)

  it("keeps a backup a month for each module, not one for all of them", function()
    makeFile("modules", "Alpha2020-03-01#12-00-00", 1)
    makeFile("modules", "Beta2020-03-05#12-00-00", 5)
    makeFile("modules", "Alpha2020-03-10#12-00-00", 10)
    makeFile("modules", "Beta2020-03-15#12-00-00", 15)
    makeFile("modules", "Alpha2020-03-25#12-00-00", 25)

    deleteOld("modules")

    assert.is_true(exists("modules", "Alpha2020-03-01#12-00-00"), "Alpha's first backup of the month should be kept")
    assert.is_true(exists("modules", "Beta2020-03-05#12-00-00"), "Beta's first backup of the month should be kept")
    assert.is_false(exists("modules", "Alpha2020-03-10#12-00-00"))
    assert.is_true(exists("modules", "Beta2020-03-15#12-00-00"), "Beta's newest backup should be kept")
    assert.is_true(exists("modules", "Alpha2020-03-25#12-00-00"), "Alpha's newest backup should be kept")
  end)

  it("keeps each module's newest backup, even when another module was backed up later", function()
    makeFile("modules", "Alpha2020-02-01#12-00-00", 1, 2)
    makeFile("modules", "Alpha2020-02-10#12-00-00", 10, 2)
    makeFile("modules", "Alpha2020-02-20#12-00-00", 20, 2)
    makeFile("modules", "Beta2020-03-05#12-00-00", 5)

    deleteOld("modules")

    assert.is_true(exists("modules", "Alpha2020-02-01#12-00-00"), "Alpha's first backup of February should be kept")
    assert.is_false(exists("modules", "Alpha2020-02-10#12-00-00"))
    assert.is_true(exists("modules", "Alpha2020-02-20#12-00-00"), "Alpha's newest backup should be kept although Beta's is newer")
    assert.is_true(exists("modules", "Beta2020-03-05#12-00-00"))
  end)

  it("still thins out module backups named the way older Mudlet versions did", function()
    makeFile("modules", "Alpha01-02-2020#12-00-00", 1, 2)
    makeFile("modules", "Alpha05-02-2020#12-00-00", 5, 2)
    makeFile("modules", "Alpha09-02-2020#12-00-00", 9, 2)
    makeFile("modules", "notes", 10, 2)
    makeFile("modules", "more notes", 11, 2)
    makeFile("modules", "latest notes", 12, 2)
    makeFile("modules", "Alpha2020-03-05#12-00-00", 5)

    deleteOld("modules")

    assert.is_true(exists("modules", "Alpha01-02-2020#12-00-00"), "Alpha's first backup of February should be kept")
    assert.is_false(exists("modules", "Alpha05-02-2020#12-00-00"), "an old-style backup is still a backup of Alpha, so it should be thinned out")
    assert.is_false(exists("modules", "Alpha09-02-2020#12-00-00"), "Alpha's newest backup is the March one")
    assert.is_true(exists("modules", "notes"), "February's first file not named like a backup should be kept")
    assert.is_false(exists("modules", "more notes"), "files not named like a backup should not each be kept forever")
    assert.is_true(exists("modules", "latest notes"), "the newest file not named like a backup should be kept")
    assert.is_true(exists("modules", "Alpha2020-03-05#12-00-00"))
  end)
end)
