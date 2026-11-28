-- The starter UI's chat window opens on a line saying what will collect there.
-- A console keeps the wrap a line had when it was printed, so a welcome printed
-- while the window was still small stays narrow after the window grows, unless
-- the starter UI prints it again. Only the welcome is ever printed again: chat
-- the game sent keeps the lines it arrived on.
--
-- The self-test profile has no starter UI, so it is installed here for these
-- specs and taken away again after them.

describe("Tests the starter UI's chat welcome as the window is resized", function()
  local packageName = "mudlet-base-ui"
  local pane = "BaseUI_chat_all"
  local welcome = "Tells, says and channel chat from the game will be collected here."
  local windowWidth, windowHeight
  local installedHere = false

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

  -- see UI_spec's "Toolbar buttons"
  local function waitForProfileSaveToPass()
    local _, err = installPackage("")
    return not tostring(err):find("still in progress", 1, true)
  end

  -- see Actions_spec: setMainWindowSize() sizes the whole window and
  -- getMainWindowSize() reads the console, so the difference is the chrome
  local function sizeMainWindow(width, height)
    setMainWindowSize(width, height)
    pumpEvents(50)
    local innerWidth, innerHeight = getMainWindowSize()
    setMainWindowSize(width + (width - innerWidth), height + (height - innerHeight))
    pumpEvents(200)
  end

  -- getLineCount() is the index of the empty line the buffer sits on
  local function paneLines()
    return getLines(pane, 0, getLineCount(pane)) or {}
  end

  -- a wrap drops the space it breaks at
  local function stripped(text)
    return (text:gsub("%s", ""))
  end

  local function welcomeRows()
    local rows = {}
    for _, line in ipairs(paneLines()) do
      if line ~= "" then
        rows[#rows + 1] = line
      end
    end
    return rows
  end

  setup(function()
    windowWidth, windowHeight = getMainWindowSize()
    if not packageIsInstalled() then
      assert.is_true(waitForProfileSaveToPass(), "a profile save never finished")
      local installed, reason = installPackage(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage")
      assert.is_true(installed, "could not install the starter UI: " .. tostring(reason))
      assert.is_true(waitUntil(packageIsInstalled, 5000), packageName .. " did not turn up in getPackages()")
      installedHere = true
    end
  end)

  teardown(function()
    if installedHere and packageIsInstalled() then
      assert.is_true(waitUntil(function() return uninstallPackage(packageName) == true end, 5000),
        packageName .. " could not be uninstalled")
      assert.is_true(waitUntil(function() return not packageIsInstalled() end, 5000),
        packageName .. " was still installed after being uninstalled")
    end
    -- the save the uninstall queues must finish before Mudlet shuts down
    pumpEvents(300)
    assert.is_true(waitForProfileSaveToPass(), "the profile save the uninstall queued never finished")
    sizeMainWindow(windowWidth, windowHeight)
  end)

  it("prints the welcome again at the wrap of a window that grew after it was built", function()
    if not installedHere or BaseUI.container then
      pending("the starter UI was already in this profile")
      return
    end
    sizeMainWindow(800, 600)
    BaseUI.build()
    pumpEvents(100)
    local narrowWrap = getWindowWrap(pane)
    local narrowRows = welcomeRows()
    assert.equals(stripped(welcome), stripped(table.concat(narrowRows)))

    sizeMainWindow(1600, 600)
    local wideWrap = getWindowWrap(pane)
    assert.is_true(wideWrap > narrowWrap, "the chat window did not get wider: " .. narrowWrap .. " then " .. wideWrap)
    local wideRows = welcomeRows()
    assert.equals(stripped(welcome), stripped(table.concat(wideRows)))
    assert.is_true(#wideRows < #narrowRows, table.concat(wideRows, "|"))
    assert.is_true(#wideRows[1] > narrowWrap, table.concat(wideRows, "|"))
  end)

  it("leaves the chat window as it is once the game has sent chat to it", function()
    if not installedHere or not BaseUI.chats then
      pending("the starter UI was already in this profile")
      return
    end
    BaseUI.chats.all:echo("Ana tells you, 'meet me at the fountain when the bells ring for the third time today'\n")
    local before = paneLines()
    local wideWrap = getWindowWrap(pane)

    sizeMainWindow(800, 600)
    assert.is_true(getWindowWrap(pane) < wideWrap, "the chat window did not get narrower")
    assert.are.same(before, paneLines())
  end)
end)
