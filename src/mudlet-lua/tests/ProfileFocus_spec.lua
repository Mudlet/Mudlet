-- setActiveProfile() brings another loaded profile to the front, as clicking
-- its tab does, and sysProfileFocusChangeEvent tells both profiles of it.

describe("setActiveProfile", function()
  -- Opens a second profile whose script forwards what it hears, runs check(t)
  -- and closes it again; t.heard lists "this <gained>" and "other <gained>"
  local function withOtherProfile(check)
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waiting for the other profile needs pumpEvents(), which is refused outside MUDLET_TEST_MODE")
      return
    end
    local me = getProfileName()
    local name = "mudlet-spec-profile-focus"
    local directory = getMudletHomeDir():match("^(.*)[/\\]") .. "/" .. name
    local marker = directory .. "/mudlet-spec-fixture"
    local function removeTree(path)
      if lfs.attributes(path, "mode") ~= "directory" then
        os.remove(path)
        return
      end
      for entry in lfs.dir(path) do
        if entry ~= "." and entry ~= ".." then
          removeTree(path .. "/" .. entry)
        end
      end
      lfs.rmdir(path)
    end
    -- the marker goes last, so a fixture that could not be fully deleted is
    -- still known to be this spec's and the next run can finish the job
    local function removeFixture()
      for entry in lfs.dir(directory) do
        if entry ~= "." and entry ~= ".." and entry ~= "mudlet-spec-fixture" then
          removeTree(directory .. "/" .. entry)
        end
      end
      os.remove(marker)
      lfs.rmdir(directory)
    end
    local function loaded()
      local entry = getProfiles()[name]
      return entry ~= nil and entry.loaded
    end
    if lfs.attributes(marker) then
      removeFixture()
    end
    assert.is_nil(lfs.attributes(marker), "could not delete this spec's leftover profile at " .. directory)
    assert.is_nil(lfs.attributes(directory), "a profile named " .. name .. " already exists and is not this spec's to delete")
    -- closing a profile saves the shared window layout beside the profiles
    -- directory, which the next Mudlet start reads, so put it back afterwards
    local configurationDirectory = directory:match("^(.*)[/\\]profiles[/\\]")
    local layoutFiles = {
      configurationDirectory .. "/windowLayout.dat",
      configurationDirectory .. "/windowLayoutGeometry.dat",
      configurationDirectory .. "/windowLayoutHeadless.dat",
    }
    local layoutBefore = {}
    for _, path in ipairs(layoutFiles) do
      local handle = io.open(path, "rb")
      if handle then
        layoutBefore[path] = handle:read("*a")
        handle:close()
      end
    end

    local t = {me = me, name = name, heard = {}, refusedOnExit = {}}
    local otherReady = false
    local handlers = {
      registerAnonymousEventHandler("sysProfileFocusChangeEvent", function(_, gained)
        t.heard[#t.heard + 1] = "this " .. tostring(gained)
      end),
      registerAnonymousEventHandler("mudletSpecProfileFocus", function(_, gained)
        t.heard[#t.heard + 1] = "other " .. tostring(gained)
      end),
      registerAnonymousEventHandler("mudletSpecProfileFocusOnExit", function(_, answer)
        t.refusedOnExit[#t.refusedOnExit + 1] = answer
      end),
      registerAnonymousEventHandler("mudletSpecProfileFocusReady", function()
        otherReady = true
      end),
    }
    t.handlers = handlers
    finally(function()
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      if loaded() then
        -- the profile in front goes with its tab, so this leaves the front as
        -- it was before the spec: with this profile in a main window, and with
        -- none at all with no main window
        setActiveProfile(name)
        closeProfile(name)
      end
      for _ = 1, 100 do
        if not loaded() then
          break
        end
        pumpEvents(50)
      end
      if lfs.attributes(marker) then
        removeFixture()
      end
      for _, path in ipairs(layoutFiles) do
        if layoutBefore[path] then
          local handle = assert(io.open(path, "wb"))
          handle:write(layoutBefore[path])
          handle:close()
        else
          os.remove(path)
        end
      end
    end)
    assert(lfs.mkdir(directory), "could not create " .. directory)
    assert(io.open(marker, "w"), "could not create " .. marker):close()
    assert(lfs.mkdir(directory .. "/current"), "could not create " .. directory .. "/current")
    local script = [[
registerAnonymousEventHandler("sysProfileFocusChangeEvent", function(_, gained) raiseGlobalEvent("mudletSpecProfileFocus", gained) end)
registerAnonymousEventHandler("sysExitEvent", function()
  local ok, err = setActiveProfile(getProfileName())
  raiseGlobalEvent("mudletSpecProfileFocusOnExit", ok and "brought forward" or tostring(err))
end)
tempTimer(0, function() raiseGlobalEvent("mudletSpecProfileFocusReady") end)]]
    local file = assert(io.open(directory .. "/current/2026-01-01#00-00-00.xml", "w"))
    file:write([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
  <HostPackage><Host></Host></HostPackage>
  <ScriptPackage>
    <Script isActive="yes" isFolder="no">
      <name>mudlet-spec-profile-focus</name>
      <packageName></packageName>
      <script>]] .. script .. [[</script>
      <eventHandlerList />
    </Script>
  </ScriptPackage>
</MudletPackage>
]])
    file:close()

    assert.is_true(loadProfile(name, true))
    for _ = 1, 100 do
      if otherReady then
        break
      end
      pumpEvents(50)
    end
    assert.is_true(otherReady, "the other profile's script never ran")
    -- A main window brings a profile it loads to the front, and with none nothing is in front yet
    assert.is_true(setActiveProfile(me))
    t.heard = {}
    check(t)
  end

  it("tells the profile it leaves and the one it brings forward, and neither when nothing changes", function()
    withOtherProfile(function(t)
      assert.is_true(setActiveProfile(t.name))
      assert.are.same({"this false", "other true"}, t.heard)

      t.heard = {}
      assert.is_true(setActiveProfile(t.name))
      assert.are.same({}, t.heard)

      assert.is_true(setActiveProfile(t.me))
      assert.are.same({"other false", "this true"}, t.heard)
    end)
  end)

  it("switches once when the profile it leaves asks for the same switch again", function()
    withOtherProfile(function(t)
      local asked = 0
      t.handlers[#t.handlers + 1] = registerAnonymousEventHandler("sysProfileFocusChangeEvent", function(_, gained)
        -- bounded, so a switch that keeps starting over fails here rather than overflowing the stack
        if not gained and asked < 3 then
          asked = asked + 1
          setActiveProfile(t.name)
        end
      end)
      assert.is_true(setActiveProfile(t.name))
      assert.are.same({"this false", "other true"}, t.heard)
      assert.are.equal(1, asked)
    end)
  end)

  it("refuses a profile whose close has started and keeps the one in front", function()
    if getProfileTabNumber() then
      pending("a main window's tab bar still brings a closing profile forward")
      return
    end
    withOtherProfile(function(t)
      closeProfile(t.name)
      local ok, err = setActiveProfile(t.name)
      assert.is_falsy(ok)
      assert.is_string(err)
      assert.are.same({}, t.heard)
      assert.are.equal(1, #t.refusedOnExit)
      assert.are_not.equal("brought forward", t.refusedOnExit[1])
    end)
  end)

  it("keeps the profile in front when the one it was bringing forward closes on the way", function()
    if getProfileTabNumber() then
      pending("a main window's tab bar still brings a closing profile forward")
      return
    end
    withOtherProfile(function(t)
      t.handlers[#t.handlers + 1] = registerAnonymousEventHandler("sysProfileFocusChangeEvent", function(_, gained)
        if not gained then
          closeProfile(t.name)
        end
      end)
      local ok, err = setActiveProfile(t.name)
      assert.is_falsy(ok)
      assert.is_string(err)
      assert.are.same({"this false", "this true"}, t.heard)
    end)
  end)
end)
