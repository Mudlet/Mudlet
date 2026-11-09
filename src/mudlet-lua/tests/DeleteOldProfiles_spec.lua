describe("Tests the deleteOldProfiles package", function()
  if type(deleteOldProfiles) ~= "function" then
    it("needs the deleteOldProfiles package installed", function()
      pending("the deleteOldProfiles package is not installed in this profile")
    end)
    return
  end

  local realGetMudletHomeDir = getMudletHomeDir
  local specHome = realGetMudletHomeDir():gsub("\\", "/") .. "/delete_old_profiles_spec"
  local savesFolder = specHome .. "/current"
  local names = {"2020-03-01#12-00-00.xml", "2020-03-10#12-00-00.xml", "2020-03-20#12-00-00.xml"}

  local function makeSave(name, day)
    local path = savesFolder .. "/" .. name
    local file = assert(io.open(path, "w"))
    file:write("<MudletPackage/>")
    file:close()
    local when = os.time({year = 2020, month = 3, day = day, hour = 12})
    assert.is_true(lfs.touch(path, when, when))
  end

  local function exists(name)
    return lfs.attributes(savesFolder .. "/" .. name, "mode") == "file"
  end

  setup(function()
    lfs.mkdir(specHome)
    lfs.mkdir(savesFolder)
  end)

  teardown(function()
    _G.getMudletHomeDir = realGetMudletHomeDir
    for _, name in ipairs(names) do
      os.remove(savesFolder .. "/" .. name)
    end
    lfs.rmdir(savesFolder)
    lfs.rmdir(specHome)
  end)

  it("keeps the newest save, which Mudlet loads, even when it is older than the cut-off", function()
    makeSave(names[1], 1)
    makeSave(names[2], 10)
    makeSave(names[3], 20)

    _G.getMudletHomeDir = function() return specHome end
    local ok, err = pcall(deleteOldProfiles, nil, "profiles")
    _G.getMudletHomeDir = realGetMudletHomeDir
    assert.is_true(ok, err)

    assert.is_true(exists(names[1]), "the month's first save should be kept")
    assert.is_false(exists(names[2]), "an old save that is neither its month's first nor the newest should go")
    assert.is_true(exists(names[3]), "the newest save, the one the profile loads, should be kept")
  end)
end)
