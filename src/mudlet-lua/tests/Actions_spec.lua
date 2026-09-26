-- Buttons, toolbars and their menus. UI_spec's "Toolbar buttons" block covers
-- the Lua API for a plain button bar; this file installs the button layouts
-- that API cannot make - rotated, custom-placed, push-down buttons already
-- pressed, nested menus, a switched-off floating toolbar, a button whose Lua
-- does not compile - so building every kind of bar is exercised, and pins what
-- Lua can read back of them.

local function specFilePath(name)
  return ("%s/%s"):format(getMudletHomeDir(), name)
end

local function writeSpecFile(path, contents)
  local handle = io.open(path, "wb")
  assert.is_not_nil(handle, "could not open " .. path .. " for writing")
  assert.is_not_nil(handle:write(contents), "could not write " .. path)
  handle:close()
end

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

describe("Floating toolbars made from Lua", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local toolbar = "actionSpecFloatingTemp" .. suffix
  local button = "actionSpecFloatingTempButton" .. suffix

  -- tempButtonToolbar() shifts every location above 0 up by one, so its 3 is
  -- the floating setting, 4
  it("tempButtonToolbar makes a floating toolbar that tempButton can add to", function()
    assert.equals(0, exists(toolbar, "button"))
    local toolbarId = tempButtonToolbar(toolbar, 3, 1)
    assert.is_number(toolbarId)
    local buttonId = tempButton(toolbar, button, 0)
    assert.is_number(buttonId)
    assert.equals(1, exists(button, "button"))
    local chain = ancestors(buttonId, "button")
    assert.equals(1, #chain)
    assert.equals(toolbarId, chain[1].id)
    assert.equals(toolbar, chain[1].name)
  end)

  it("hideToolBar refuses it as floating rather than missing", function()
    local ok, err = hideToolBar(toolbar)
    assert.is_nil(ok)
    assert.equals(("toolbar '%s' is set to float, which showToolBar() and hideToolBar() do not move"):format(toolbar), err)
  end)

  it("setButtonStyleSheet restyles a button on it", function()
    assert.is_true(setButtonStyleSheet(button, "QPushButton { color: rgb(1,2,3); }"))
  end)
end)

if not os.getenv("MUDLET_TEST_MODE") then

describe("Packaged toolbar layouts", function()
  it("needs test mode", function()
    pending("these specs install a package, which needs pumpEvents()")
  end)
end)

else

describe("Packaged toolbar layouts", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local packageName = "mudlet-spec-actions" .. suffix
  local packageFile = specFilePath(packageName .. ".xml")
  -- every item name carries this marker, so findItems() can be asked for part of it
  local marker = "ActSpecMark" .. suffix
  local function item(name)
    return marker .. name
  end
  local installAnswer, installReason

  -- attributes the XML takes, defaulted so each item only names what it is about
  local function action(fields, children)
    local f = setmetatable(fields, {__index = {
      active = "yes", folder = "no", push = "no", custom = "no",
      script = "", css = "", orientation = 0, location = 0, rotation = 0,
      sizeX = 0, sizeY = 0, state = 1, columns = 1, filler = 0, posX = 0, posY = 0,
    }})
    return ([[<Action isActive="%s" isFolder="%s" isPushButton="%s" isFlatButton="no" useCustomLayout="%s">
      <name>%s</name>
      <script>%s</script>
      <css>%s</css>
      <commandButtonUp></commandButtonUp>
      <commandButtonDown></commandButtonDown>
      <icon></icon>
      <orientation>%d</orientation>
      <location>%d</location>
      <buttonRotation>%d</buttonRotation>
      <sizeX>%d</sizeX>
      <sizeY>%d</sizeY>
      <mButtonState>%d</mButtonState>
      <buttonColumn>%d</buttonColumn>
      <buttonFillerOffset>%d</buttonFillerOffset>
      <posX>%d</posX>
      <posY>%d</posY>
      %s
    </Action>]]):format(f.active, f.folder, f.push, f.custom, f.name, f.script, f.css,
      f.orientation, f.location, f.rotation, f.sizeX, f.sizeY, f.state, f.columns,
      f.filler, f.posX, f.posY, table.concat(children or {}, "\n"))
  end

  -- one of each kind of button, for a bar of the given location
  local function buttonSet(prefix)
    return {
      action({name = item(prefix .. "Pressed"), push = "yes", state = 2}),
      action({name = item(prefix .. "Released"), push = "yes", state = 1}),
      action({name = item(prefix .. "Upright"), rotation = 1}),
      action({name = item(prefix .. "Upended"), rotation = 2}),
      action({name = item(prefix .. "Disabled"), active = "no"}),
      action({name = item(prefix .. "Menu"), folder = "yes"}, {
        action({name = item(prefix .. "MenuPressed"), push = "yes", state = 2}),
        action({name = item(prefix .. "MenuDisabled"), active = "no"}),
        action({name = item(prefix .. "SubMenu"), folder = "yes"}, {
          action({name = item(prefix .. "SubMenuItem")}),
        }),
      }),
    }
  end

  local function packageXml()
    return table.concat({
      [[<?xml version="1.0" encoding="UTF-8"?>]],
      [[<!DOCTYPE MudletPackage>]],
      [[<MudletPackage version="1.001">]],
      [[<ActionPackage>]],
      action({name = item("Floating"), folder = "yes", location = 4, orientation = 1, columns = 2, filler = 1,
              css = "QWidget { background-color: rgb(10,20,30); }"}, buttonSet("Floating")),
      action({name = item("FloatingPlaced"), folder = "yes", location = 4, custom = "yes", sizeX = 200, sizeY = 80, posX = 5, posY = 5},
             {action({name = item("FloatingPlacedButton"), sizeX = 90, sizeY = 25, posX = 4, posY = 4})}),
      action({name = item("FloatingOff"), folder = "yes", location = 4, active = "no"},
             {action({name = item("FloatingOffButton")})}),
      action({name = item("Left"), folder = "yes", location = 2, orientation = 1, columns = 2, filler = 1}, buttonSet("Left")),
      action({name = item("Top"), folder = "yes", location = 0, filler = 2}, {
        action({name = item("TopButton")}),
        action({name = item("TopBroken"), script = "this is not lua("}),
      }),
      action({name = item("TopPlaced"), folder = "yes", location = 0, custom = "yes", sizeX = 200, sizeY = 40, posX = 0, posY = 0},
             {action({name = item("TopPlacedButton"), sizeX = 90, sizeY = 25, posX = 4, posY = 4})}),
      [[</ActionPackage>]],
      [[</MudletPackage>]],
    }, "\n")
  end

  local function waitUntil(condition, timeoutMilliseconds)
    local waited = 0
    while waited < timeoutMilliseconds do
      if condition() then
        return true
      end
      pumpEvents(50)
      waited = waited + 50
    end
    return condition() and true or false
  end

  local function packageIsInstalled()
    return table.contains(getPackages(), packageName)
  end

  -- see UI_spec's "Toolbar buttons": installPackage() gives a running profile
  -- save away by postponing even the empty path it would otherwise refuse
  local function waitForProfileSaveToPass()
    return waitUntil(function() return installPackage("") == nil end, 5000)
  end

  setup(function()
    writeSpecFile(packageFile, packageXml())
    assert.is_true(waitForProfileSaveToPass(), "a profile save was already running, so this install would be postponed")
    installAnswer, installReason = installPackage(packageFile)
    assert.is_true(installAnswer, "could not install " .. packageFile)
    assert.is_true(waitUntil(packageIsInstalled, 5000), packageName .. " did not turn up in getPackages()")
    -- let the bars be laid out and painted
    pumpEvents(200)
  end)

  teardown(function()
    if packageIsInstalled() then
      assert.is_true(waitUntil(function() return uninstallPackage(packageName) == true end, 5000),
        packageName .. " could not be uninstalled")
      assert.is_true(waitUntil(function() return not packageIsInstalled() end, 5000),
        packageName .. " was still installed after being uninstalled")
    end
    -- the save the uninstall queues must finish before Mudlet shuts down
    pumpEvents(300)
    assert.is_true(waitForProfileSaveToPass(), "the profile save the uninstall queued never finished")
    pumpEvents(100)
    assert.is_true(waitForProfileSaveToPass(), "another profile save was queued behind the first")
    os.remove(packageFile)
  end)

  it("installs every item, whether it is switched on or not", function()
    for _, name in ipairs({"Floating", "FloatingPressed", "FloatingOff", "FloatingOffButton", "FloatingMenuDisabled",
                           "FloatingSubMenuItem", "FloatingPlacedButton", "Left", "LeftSubMenuItem", "Top",
                           "TopBroken", "TopPlacedButton"}) do
      assert.equals(1, exists(item(name), "button"), name)
    end
  end)

  it("reads a pressed push-down button in as pressed, wherever it sits", function()
    for _, name in ipairs({"FloatingPressed", "FloatingMenuPressed", "LeftPressed", "LeftMenuPressed"}) do
      assert.is_true(getButtonState(item(name)), name)
    end
    for _, name in ipairs({"FloatingReleased", "LeftReleased"}) do
      assert.is_false(getButtonState(item(name)), name)
    end
  end)

  it("keeps a pressed state set from Lua across the bars being rebuilt", function()
    finally(function() setButtonState(item("LeftReleased"), false) end)
    assert.is_true(setButtonState(item("LeftReleased"), true))
    -- restyling a button rebuilds every bar
    assert.is_true(setButtonStyleSheet(item("TopButton"), "QPushButton { color: rgb(4,5,6); }"))
    pumpEvents(50)
    assert.is_true(getButtonState(item("LeftReleased")))
    assert.is_true(getButtonState(item("LeftPressed")))
  end)

  it("names a button whose Lua does not compile in what installPackage() answers", function()
    assert.is_true(contains(installReason, item("TopBroken")), tostring(installReason))
    assert.is_true(contains(installReason, '[string "Button: ' .. item("TopBroken") .. '"]'), tostring(installReason))
    -- the buttons that compiled are not named
    assert.is_false(contains(installReason, item("TopButton")), tostring(installReason))
  end)

  it("reports a switched-off toolbar as inactive and its button's ancestors as not all active", function()
    assert.equals(0, isActive(item("FloatingOff"), "button"))
    local buttonId = findItems(item("FloatingOffButton"), "button")[1]
    assert.is_number(buttonId)
    assert.is_false(isAncestorsActive(buttonId, "button"))
    local liveId = findItems(item("FloatingSubMenuItem"), "button")[1]
    assert.is_true(isAncestorsActive(liveId, "button"))
  end)

  it("lists a nested menu item's ancestors up to the package", function()
    local id = findItems(item("LeftSubMenuItem"), "button")[1]
    local names = {}
    for _, ancestor in ipairs(ancestors(id, "button")) do
      names[#names + 1] = ancestor.name
    end
    assert.same({item("LeftSubMenu"), item("LeftMenu"), item("Left"), packageName}, names)
  end)

  describe("findItems for buttons", function()
    -- two bars of eleven, FloatingPlaced, FloatingOff and TopPlaced of two, Top of three
    local everything = 2 * 11 + 3 * 2 + 3

    it("matches only the whole name by default", function()
      assert.same({}, findItems(marker, "button"))
      assert.equals(1, #findItems(item("TopButton"), "button"))
    end)

    it("matches part of a name when exact matching is off", function()
      assert.equals(everything, #findItems(marker, "button", false))
      -- Floating with its ten buttons and menu items, FloatingPlaced, FloatingOff and their buttons
      assert.equals(15, #findItems(item("Floating"), "button", false))
    end)

    it("ignores case when asked to, for a part or a whole name", function()
      assert.equals(0, #findItems(marker:upper(), "button", false))
      assert.equals(everything, #findItems(marker:upper(), "button", false, false))
      assert.equals(1, #findItems(item("TopButton"):lower(), "button", true, false))
    end)
  end)
end)

end
