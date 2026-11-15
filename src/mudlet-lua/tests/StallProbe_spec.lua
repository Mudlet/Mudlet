-- TEMPORARY, not for merging: repeats what precedes the flaky delayed speedwalk
-- spec on CI (#12926) - a profile with a chat server is called and closed - then
-- times delayed walks straight after, so one CI run is likely to catch the stall.
describe("stall probe for #12926", function()
  local name = "mudlet-spec-stall-probe"
  local peerName = "StallProbePeer"
  local profileDirectory = getMudletHomeDir():match("^(.*)[/\\]") .. "/" .. name
  local fixtureMarker = profileDirectory .. "/mudlet-spec-fixture"

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

  local function waitUntil(predicate)
    for _ = 1, 100 do
      if predicate() then
        return true
      end
      pumpEvents(50)
    end
    return predicate()
  end

  local function peerConnected()
    for _, client in ipairs(mmcp.getClientList() or {}) do
      if client.name == peerName then
        return true
      end
    end
    return false
  end

  local function closeAChattingProfile(port)
    if lfs.attributes(fixtureMarker) then
      removeTree(profileDirectory)
    end
    lfs.mkdir(profileDirectory)
    io.open(fixtureMarker, "w"):close()
    lfs.mkdir(profileDirectory .. "/current")
    local file = io.open(profileDirectory .. "/current/2026-01-01#00-00-00.xml", "w")
    file:write(string.format([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
  <HostPackage>
    <Host>
      <name>%s</name>
      <MMCP chatName="%s" chatPort="%d" chatPrefix="" autostartServer="yes" allowPeekRequests="no" prefixEmotes="no" chatMessageNewline="no" autoAcceptCalls="yes" snoopInMain="no"/>
    </Host>
  </HostPackage>
</MudletPackage>
]], name, peerName, port))
    file:close()

    assert.is_true(loadProfile(name, true))
    assert.is_true(mmcp.call("127.0.0.1", port))
    assert.is_true(waitUntil(peerConnected), "the other profile's chat server did not accept the call")
    assert.is_true(closeProfile(name))
    assert.is_true(waitUntil(function() return not loaded() end))
    mmcp.chatAll("are you still there?")
    assert.is_true(waitUntil(function() return not peerConnected() end))
  end

  it("times delayed walks straight after a chatting profile closes", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("needs MUDLET_TEST_MODE for waitForEvent")
      return
    end
    finally(function()
      if peerConnected() then
        mmcp.disconnect(peerName)
      end
      if loaded() then
        closeProfile(name)
        waitUntil(function() return not loaded() end)
      end
      if lfs.attributes(fixtureMarker) then
        removeTree(profileDirectory)
      end
    end)

    local worst = 0
    for round = 1, 8 do
      closeAChattingProfile(20000 + (os.time() + round) % 40000)
      local times = {}
      for walk = 1, 10 do
        local started = getEpoch()
        assert.is_true(speedwalk("n;e;s", false, 0.01, false))
        local got = waitForEvent("sysSpeedwalkFinished", 8000)
        local took = math.floor((getEpoch() - started) * 1000)
        times[#times + 1] = tostring(took) .. (got and "" or "!")
        if took > worst then
          worst = took
        end
        if not got then
          stopSpeedwalk()
        end
      end
      print(string.format("PROBE round %d at %s: walk ms %s", round, os.date("!%H:%M:%S"), table.concat(times, " ")))
      pumpEvents(300)
    end
    assert.is_true(worst < 2000, string.format("a 3-step walk with 10 ms steps took %d ms straight after a profile closed", worst))
  end)
end)
