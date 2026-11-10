describe("Tests reporting errors raised with something other than a string", function()
  -- Nothing in Lua can read back the suite's own error console or turn on
  -- echoing errors to its main console, so this loads a profile that has it
  -- on and reads that profile's main console back
  local name = "mudlet-spec-lua-error-objects"
  local directory = getMudletHomeDir():match("^(.*)[/\\]") .. "/" .. name
  -- written into the profile made here, so a profile of the same name that
  -- this spec did not make is never deleted
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

  local function loaded()
    local entry = getProfiles()[name]
    return entry ~= nil and entry.loaded
  end

  it("should report each one with the type of what was raised", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("waiting for the other profile needs pumpEvents(), which is refused outside MUDLET_TEST_MODE")
      return
    end
    if lfs.attributes(marker) then
      removeTree(directory)
    end
    assert.is_nil(lfs.attributes(directory), "a profile named " .. name .. " already exists and is not this spec's to delete")

    -- closing a profile saves the shared window layout beside the profiles
    -- directory, which later specs and the next Mudlet start read, so put it back afterwards
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
    local consoleText
    local handler = registerAnonymousEventHandler("mudletSpecLuaErrorObjectsConsole", function(_, text)
      consoleText = text
    end)
    finally(function()
      killAnonymousEventHandler(handler)
      if loaded() then
        closeProfile(name)
      end
      for _ = 1, 100 do
        if not loaded() then
          break
        end
        pumpEvents(50)
      end
      if lfs.attributes(marker) then
        removeTree(directory)
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
    assert(io.open(marker, "w")):close()
    assert(lfs.mkdir(directory .. "/current"), "could not create " .. directory .. "/current")
    -- a timer given a function, a timer given code, an event handler, a trigger's code,
    -- a script's own GMCP decoder and GMCP merge
    local script = [[
function mudletSpecErrorObjectHandler() error(true) end
tempTrigger("mudletSpecErrorObjectLine", "error(print)")
tempTimer(0.05, function() feedTriggers("mudletSpecErrorObjectLine\n") end)
tempTimer(0.05, function() error({}) end)
tempTimer(0.05, "error()")
tempTimer(0.05, function() raiseEvent("mudletSpecErrorObjectEvent") end)
tempTimer(0.05, function()
  local stockDecoder = json_to_value
  json_to_value = function() error({}) end
  feedTelnet("\60T_IAC>\60T_SB>\60O_GMCP>Spec.Doc {}\60T_IAC>\60T_SE>")
  json_to_value = stockDecoder
  feedTelnet("\60T_IAC>\60T_SB>\60O_GMCP>Char.Status {}\60T_IAC>\60T_SE>")
  __gmcp_merge_gmcp_sub_tables = function() error(false) end
  feedTelnet("\60T_IAC>\60T_SB>\60O_GMCP>Char.Status {}\60T_IAC>\60T_SE>")
end)
tempTimer(0.5, function() raiseGlobalEvent("mudletSpecLuaErrorObjectsConsole", table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")) end)]]
    local file = assert(io.open(directory .. "/current/2026-01-01#00-00-00.xml", "w"))
    file:write([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
  <HostPackage><Host mEchoLuaErrors="yes"></Host></HostPackage>
  <ScriptPackage>
    <Script isActive="yes" isFolder="no">
      <name>mudletSpecErrorObjectHandler</name>
      <packageName></packageName>
      <script>]] .. script .. [[</script>
      <eventHandlerList>
        <string>mudletSpecErrorObjectEvent</string>
      </eventHandlerList>
    </Script>
  </ScriptPackage>
</MudletPackage>
]])
    file:close()

    assert.is_true(loadProfile(name, true))
    for _ = 1, 100 do
      if consoleText then
        break
      end
      pumpEvents(50)
    end

    assert.is_string(consoleText, "the other profile never reported its console")
    -- word wrap breaks a long error line across console lines
    local unwrapped = consoleText:gsub("\n", "")
    assert.is_truthy(unwrapped:find("error object is a table value", 1, true), consoleText)
    assert.is_truthy(unwrapped:find("error object is a nil value", 1, true), consoleText)
    assert.is_truthy(unwrapped:find("error object is a boolean value", 1, true), consoleText)
    assert.is_truthy(unwrapped:find("error object is a function value", 1, true), consoleText)
    assert.is_truthy(unwrapped:find("any previous value is kept: error object is a table value", 1, true), consoleText)
    assert.is_truthy(unwrapped:find("GMCP merge error: error object is a boolean value", 1, true), consoleText)
  end)
end)
