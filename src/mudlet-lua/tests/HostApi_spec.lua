-- Profile-level (Host) behaviour reached from Lua that the per-subsystem specs
-- leave out: stopwatch bookkeeping by ID and by the empty name, the echo modes
-- of sent commands, the main console's font size, and openUserWindow's refusals
-- and docking areas.

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

local function textFrom(mark)
  return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
end

describe("Stopwatch bookkeeping", function()
  local made = {}
  local function track(id)
    made[#made + 1] = id
    return id
  end

  after_each(function()
    for _, id in ipairs(made) do
      deleteStopWatch(id)
    end
    made = {}
  end)

  local function unusedId()
    local id = 900000
    while getStopWatches()[id] do
      id = id + 1
    end
    return id
  end

  -- the empty name means "the first unnamed stopwatch", so these need there to be none left over
  local function assertNoUnnamedStopWatches()
    for id, watch in pairs(getStopWatches()) do
      assert.is_true(watch.name ~= "", "precondition: unnamed stopwatch " .. id .. " left over by another spec")
    end
  end

  describe("by numeric ID", function()
    it("resetStopWatch zeroes a stopwatch, and says so if it already was", function()
      local id = track(createStopWatch(false))
      adjustStopWatch(id, 12)
      assert.equals(12, getStopWatchTime(id))
      assert.is_true(resetStopWatch(id))
      assert.equals(0, getStopWatchTime(id))
      local ok, err = resetStopWatch(id)
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d was already reset"):format(id), err)
    end)

    it("resetStopWatch reports an ID with no stopwatch", function()
      local id = unusedId()
      local ok, err = resetStopWatch(id)
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d not found"):format(id), err)
    end)

    it("startStopWatch with only an ID restarts from zero, as it always did", function()
      local id = track(createStopWatch(false))
      adjustStopWatch(id, 50)
      assert.is_true(startStopWatch(id))
      local elapsed = getStopWatchTime(id)
      assert.is_true(elapsed >= 0 and elapsed < 50, "elapsed was " .. tostring(elapsed))
      assert.is_true(getStopWatches()[id].isRunning)
    end)

    it("startStopWatch with an ID and false keeps the time already on the clock", function()
      local id = track(createStopWatch(false))
      adjustStopWatch(id, 50)
      assert.is_true(startStopWatch(id, false))
      assert.is_true(getStopWatchTime(id) >= 50)
      assert.is_true(getStopWatches()[id].isRunning)
    end)

    it("startStopWatch reports an ID with no stopwatch, restarting or not", function()
      local id = unusedId()
      local ok, err = startStopWatch(id)
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d not found"):format(id), err)
      ok, err = startStopWatch(id, false)
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d not found"):format(id), err)
    end)

    it("stopStopWatch says so when the stopwatch was already stopped", function()
      local id = track(createStopWatch(false))
      local ok, err = stopStopWatch(id)
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d was already stopped"):format(id), err)
    end)

    it("setStopWatchName reports an ID with no stopwatch", function()
      local id = unusedId()
      local ok, err = setStopWatchName(id, "hostSpecNoSuchWatch")
      assert.is_nil(ok)
      assert.equals(("stopwatch with id %d not found"):format(id), err)
    end)

    it("setStopWatchName refuses a name another stopwatch has, but not the stopwatch's own", function()
      local suffix = ("%d%d"):format(os.time(), math.random(100000))
      local taken = "hostSpecTaken" .. suffix
      local takenId = track(createStopWatch(taken))
      local id = track(createStopWatch("hostSpecOther" .. suffix))
      local ok, err = setStopWatchName(id, taken)
      assert.is_nil(ok)
      assert.equals(("the name '%s' is already in use for another stopwatch (id:%d)"):format(taken, takenId), err)
      assert.equals("hostSpecOther" .. suffix, getStopWatches()[id].name)
      assert.is_true(setStopWatchName(takenId, taken))
      assert.equals(taken, getStopWatches()[takenId].name)
    end)
  end)

  describe("by name", function()
    it("startStopWatch says so when a named stopwatch is already running", function()
      local name = "hostSpecRunning" .. os.time() .. math.random(100000)
      local id = track(createStopWatch(name, true))
      assert.is_true(getStopWatches()[id].isRunning)
      local ok, err = startStopWatch(name)
      assert.is_nil(ok)
      assert.equals(("stopwatch with name '%s' (id:%d) was already running"):format(name, id), err)
    end)

    it("resetStopWatch says so when a named stopwatch was already reset", function()
      local name = "hostSpecReset" .. os.time() .. math.random(100000)
      local id = track(createStopWatch(name))
      local ok, err = resetStopWatch(name)
      assert.is_nil(ok)
      assert.equals(("stopwatch with name '%s' (id:%d) was already reset"):format(name, id), err)
    end)

    it("setStopWatchName by name refuses a name another stopwatch has", function()
      local suffix = ("%d%d"):format(os.time(), math.random(100000))
      local taken = "hostSpecTakenByName" .. suffix
      local current = "hostSpecCurrent" .. suffix
      local takenId = track(createStopWatch(taken))
      local id = track(createStopWatch(current))
      local ok, err = setStopWatchName(current, taken)
      assert.is_nil(ok)
      assert.equals(("the name '%s' is already in use for another stopwatch (id:%d)"):format(taken, takenId), err)
      assert.equals(current, getStopWatches()[id].name)
      -- renaming to the name it already has is no change, and fine
      assert.is_true(setStopWatchName(current, current))
      assert.equals(current, getStopWatches()[id].name)
    end)

    it("the empty name finds nothing when every stopwatch has a name", function()
      assertNoUnnamedStopWatches()
      for fname, call in pairs({
        startStopWatch = function() return startStopWatch("") end,
        stopStopWatch = function() return stopStopWatch("") end,
        resetStopWatch = function() return resetStopWatch("") end,
        setStopWatchName = function() return setStopWatchName("", "hostSpecNamed") end,
      }) do
        local ok, err = call()
        assert.is_nil(ok, fname)
        assert.equals("no unnamed stopwatches found", err, fname)
      end
    end)

    it("the empty name reaches the first unnamed stopwatch", function()
      assertNoUnnamedStopWatches()
      local id = track(createStopWatch(false))
      local ok, err = resetStopWatch("")
      assert.is_nil(ok)
      assert.equals(("the first unnamed stopwatch (id:%d) was already reset"):format(id), err)
      adjustStopWatch(id, 7)
      assert.is_true(resetStopWatch(""))
      assert.equals(0, getStopWatchTime(id))
    end)
  end)
end)

describe("showSentText", function()
  local originalMode

  before_each(function()
    originalMode = getConfig("showSentText", true)
  end)

  after_each(function()
    setConfig("showSentText", originalMode)
  end)

  it("'always' shows a command even when the script asked for it not to be", function()
    assert.is_true(setConfig("showSentText", "always"))
    local mark = getLastLineNumber("main")
    assert.is_true(send("hostSpecAlwaysShown", false))
    assert.is_true(contains(textFrom(mark), "hostSpecAlwaysShown"), textFrom(mark))
  end)

  it("'never' hides a command even when the script asked for it to be shown", function()
    assert.is_true(setConfig("showSentText", "never"))
    local mark = getLastLineNumber("main")
    assert.is_true(send("hostSpecNeverShown", true))
    assert.is_false(contains(textFrom(mark), "hostSpecNeverShown"), textFrom(mark))
  end)
end)

describe("setFontSize on the main console", function()
  local originalSize

  setup(function()
    originalSize = getFontSize()
  end)

  teardown(function()
    setFontSize(originalSize)
  end)

  it("changes the size getFontSize reports, and says so with sysSettingChanged", function()
    local seen = {}
    local handler = registerAnonymousEventHandler("sysSettingChanged", function(_, setting, family, size)
      if setting == "main window font" then
        seen[#seen + 1] = {family = family, size = size}
      end
    end)
    finally(function() killAnonymousEventHandler(handler) end)
    local newSize = originalSize + 3
    assert.is_true(setFontSize(newSize))
    assert.equals(newSize, getFontSize())
    assert.equals(newSize, getFontSize("main"))
    assert.equals(1, #seen)
    assert.equals(newSize, seen[1].size)
    assert.equals(getFont(), seen[1].family)
  end)

  it("raises no event when the size does not change", function()
    local count = 0
    local handler = registerAnonymousEventHandler("sysSettingChanged", function(_, setting)
      if setting == "main window font" then
        count = count + 1
      end
    end)
    finally(function() killAnonymousEventHandler(handler) end)
    assert.is_true(setFontSize(getFontSize()))
    assert.equals(0, count)
  end)
end)

describe("openUserWindow", function()
  local suffix = ("%d%d"):format(os.time(), math.random(100000))
  local opened = {}
  local function name(tag)
    local n = "hostSpecDock" .. tag .. suffix
    opened[#opened + 1] = n
    return n
  end

  teardown(function()
    for _, n in ipairs(opened) do
      hideWindow(n)
    end
  end)

  it("docks at the top or bottom by short or long name, in any case", function()
    assert.is_true(openUserWindow(name("T"), false, true, "t"))
    assert.is_true(openUserWindow(name("Top"), false, true, "top"))
    assert.is_true(openUserWindow(name("B"), false, true, "b"))
    assert.is_true(openUserWindow(name("Bottom"), false, true, "BOTTOM"))
  end)

  it("refuses an area it does not know, listing the ones it does", function()
    local ok, err = openUserWindow(name("Nowhere"), false, true, "middle")
    assert.is_nil(ok)
    assert.is_true(contains(err, [[docking option "middle" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating]]), tostring(err))
  end)

  it("refuses an empty name", function()
    local ok, err = openUserWindow("")
    assert.is_nil(ok)
    assert.equals("an userwindow cannot have an empty string as its name", err)
  end)

  it("refuses the name of a mini console, which is no user window", function()
    local miniName = name("Mini")
    assert.is_true(createMiniConsole(miniName, 0, 0, 50, 50))
    local ok, err = openUserWindow(miniName, false)
    assert.is_nil(ok)
    assert.equals(("userwindow '%s' already exists"):format(miniName), err)
  end)
end)
