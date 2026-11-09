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
    modules = specRoot .. "/moduleBackups",
  }
  local made = {}

  local function makeFile(folder, name, day)
    local path = folders[folder] .. "/" .. name
    local file = assert(io.open(path, "w"))
    file:write("<MudletPackage/>")
    file:close()
    local when = os.time({year = 2020, month = 3, day = day, hour = 12})
    assert.is_true(lfs.touch(path, when, when))
    made[#made + 1] = path
  end

  local function exists(folder, name)
    return lfs.attributes(folders[folder] .. "/" .. name, "mode") == "file"
  end

  local function deleteOld(folder)
    _G.getMudletHomeDir = function() return specHome end
    local ok, err = pcall(deleteOldProfiles, nil, folder)
    _G.getMudletHomeDir = realGetMudletHomeDir
    assert.is_true(ok, err)
  end

  setup(function()
    lfs.mkdir(specRoot)
    lfs.mkdir(specRoot .. "/profiles")
    lfs.mkdir(specHome)
    lfs.mkdir(folders.profiles)
    lfs.mkdir(folders.modules)
  end)

  teardown(function()
    _G.getMudletHomeDir = realGetMudletHomeDir
    for _, path in ipairs(made) do
      os.remove(path)
    end
    lfs.rmdir(folders.profiles)
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
    assert.is_false(exists("modules", "Beta2020-03-15#12-00-00"))
  end)
end)
