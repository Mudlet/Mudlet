-- Named to sort first (busted runs its files in sorted order), before any spec
-- saves the profile: reading a saved profile or map copies the profile's ANSI
-- colours into the map again, which hides the ones its map started out with
describe("Tests the environment colours a profile starts out with", function()
  local function savedFiles(folder)
    local path = getMudletHomeDir() .. "/" .. folder
    local names = {}
    if lfs.attributes(path, "mode") == "directory" then
      for name in lfs.dir(path) do
        if name ~= "." and name ~= ".." then
          names[#names + 1] = name
        end
      end
    end
    return names
  end

  it("starts environments 257 to 272 out as the profile's ANSI colours", function()
    local atStart = getCustomEnvColorTable()
    for _, folder in ipairs({"current", "map"}) do
      assert.are.same({}, savedFiles(folder),
        "this profile was loaded from a saved " .. folder .. " file, which copies the colours again, so it can no longer show the ones it started out with")
    end
    -- a new profile's defaults, in the order the map files them
    local defaults = {
      {128, 0, 0, 255}, {0, 128, 0, 255}, {128, 128, 0, 255}, {0, 0, 128, 255},
      {128, 0, 128, 255}, {0, 128, 128, 255}, {192, 192, 192, 255}, {0, 0, 0, 255},
      {255, 0, 0, 255}, {0, 255, 0, 255}, {255, 255, 0, 255}, {0, 0, 255, 255},
      {255, 0, 255, 255}, {0, 255, 255, 255}, {255, 255, 255, 255}, {128, 128, 128, 255},
    }
    assert.is_true(deleteMap())
    local copiedAgain = getCustomEnvColorTable()
    for env = 257, 272 do
      assert.are.same(defaults[env - 256], atStart[env], "environment " .. env)
      assert.are.same(copiedAgain[env], atStart[env], "environment " .. env)
    end
  end)
end)
