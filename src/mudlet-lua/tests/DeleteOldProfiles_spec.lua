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

  local function removeMade()
    for _, path in ipairs(made) do
      os.remove(path)
    end
    made = {}
  end

  setup(function()
    lfs.mkdir(specHome)
    lfs.mkdir(folders.profiles)
  end)

  after_each(removeMade)

  teardown(function()
    _G.getMudletHomeDir = realGetMudletHomeDir
    removeMade()
    lfs.rmdir(folders.profiles)
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

  it("keeps every save tied for the newest, as Mudlet may load any of them", function()
    makeFile("profiles", "2020-03-01#12-00-00.xml", 1)
    makeFile("profiles", "2020-03-20#12-00-00.xml", 20)
    makeFile("profiles", "autosave.xml", 20)

    deleteOld("profiles")

    assert.is_true(exists("profiles", "2020-03-20#12-00-00.xml"))
    assert.is_true(exists("profiles", "autosave.xml"))
  end)
end)
