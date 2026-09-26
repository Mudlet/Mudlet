-- Profile-level (Host) behaviour reached from Lua that the per-subsystem specs
-- leave out: the echo modes of sent commands, the main console's font size, and
-- openUserWindow's refusals and docking areas.

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

local function textFrom(mark)
  return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
end

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
    pumpEvents(500)
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
    pumpEvents(500)
  end)

  it("docks at the top or bottom by short or long name, in any case", function()
    assert.is_true(openUserWindow(name("T"), false, true, "t"))
    assert.is_true(openUserWindow(name("Top"), false, true, "top"))
    assert.is_true(openUserWindow(name("B"), false, true, "b"))
    assert.is_true(openUserWindow(name("Bottom"), false, true, "BOTTOM"))
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
