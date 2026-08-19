-- A Geyser.UserWindow is a Geyser.MiniConsole living in a dock widget of its
-- own. getWindowGeometry reports that dock, which is what move()/resize() drive,
-- while getUserWindowSize reports the usable area inside it. How much of the
-- dock that area leaves out is a platform matter: Qt draws a floating dock's
-- title bar and frame itself on X11 and Wayland, so there they are taken out of
-- the usable area, while Windows and macOS let the window manager decorate the
-- dock and so draw them outside it, leaving the whole dock usable. The usable
-- area is therefore never bigger than the dock, but only strictly shorter than
-- it where Qt draws the title bar. No size difference is hardcoded here.
--
-- Geyser gives every user window an extra root container named
-- "<name>Container" whose size tracks the real user window; the user window is
-- that container's only child, which is why it is not in Geyser.windowList
-- itself.
local dockDecoratedByWindowManager = getOS() == "windows" or getOS() == "mac"

local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

-- Selects the line last written by a newline terminated echo. getLineCount
-- returns the index of the last line rather than a count, and the trailing
-- newline leaves the cursor on that still empty line, so the text is one above.
local function lastLine(name)
  local index = getLineCount(name) - 1
  assert.is_true(index >= 0, "nothing has been echoed to " .. name .. " yet")
  moveCursor(name, 0, index)
  selectCurrentLine(name)
  return getCurrentLine(name)
end

describe("Tests functionality of Geyser.UserWindow", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  local function alive(object)
    if not object or not object.container or not object.container.windowList then
      return false
    end
    return object.container.windowList[object.name] == object
  end

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.UserWindow:new/new2", function()
    it("opens a user window at the geometry it was given", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwNew", x = 20, y = 30, width = 300, height = 200}))
      assert.are.equal("userwindow", userWindow.type)
      assert.are.equal("userwindow", windowType("guwNew"))
      assert.are.same({x = 20, y = 30, width = 300, height = 200}, geometry("guwNew"))
      assert.is_true(windowVisible("guwNew"))
    end)

    it("resets its own constraints to fill the window it opened", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwFilled", x = 10, y = 10, width = 250, height = 180}))
      assert.are.equal("0px", userWindow.x)
      assert.are.equal("0px", userWindow.y)
      assert.are.equal("100%", userWindow.width)
      assert.are.equal("100%", userWindow.height)
      local usableWidth, usableHeight = getUserWindowSize("guwFilled")
      assert.are.equal(usableWidth, userWindow:get_width())
      assert.are.equal(usableHeight, userWindow:get_height())
    end)

    it("gives itself a root container sized to the user window", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwRoot", x = 10, y = 10, width = 300, height = 200}))
      local container = userWindow.container
      assert.are.equal("guwRootContainer", container.name)
      assert.are.equal(container, Geyser.windowList.guwRootContainer)
      -- the user window belongs to that container, not to the root window list
      assert.is_nil(Geyser.windowList.guwRoot)
      assert.are.equal(userWindow, container.windowList.guwRoot)
      -- that the container really is sized to the user window is read off a
      -- child widget filling it, rather than off the same getter the container
      -- was given as its own get_width/get_height
      track(Geyser.Label:new({name = "guwRootChild", x = 0, y = 0, width = "100%", height = "100%"}, userWindow))
      local usableWidth, usableHeight = getUserWindowSize("guwRoot")
      assert.are.same({x = 0, y = 0, width = usableWidth, height = usableHeight}, geometry("guwRootChild"))
      -- the dock is the size it was asked for, and the usable area is real and
      -- inside it - by however much this platform's dock decoration costs
      local dock = geometry("guwRoot")
      assert.are.equal(300, dock.width)
      assert.are.equal(200, dock.height)
      assert.is_true(usableWidth > 0 and usableWidth <= dock.width,
                     string.format("usable width %d is not inside the dock width %d", usableWidth, dock.width))
      assert.is_true(usableHeight > 0 and usableHeight <= dock.height,
                     string.format("usable height %d is not inside the dock height %d", usableHeight, dock.height))
      if not dockDecoratedByWindowManager then
        assert.is_true(usableHeight < dock.height,
                       string.format("Qt draws the dock title bar here, so it must cost height: usable %d, dock %d",
                                     usableHeight, dock.height))
      end
    end)

    it("registers itself as a parent window so children can be put in it", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwParent", x = 10, y = 10, width = 200, height = 150}))
      assert.are.equal(userWindow, Geyser.parentWindows.guwParent)
      assert.are.equal("guwParent", userWindow.windowname)
    end)

    it("defaults to an undocked, auto docking window that does not restore a layout", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwDefaults", x = 10, y = 10, width = 200, height = 150}))
      assert.is_false(userWindow.docked)
      assert.is_false(userWindow.restoreLayout)
      assert.is_true(userWindow.autoDock)
      assert.are.equal("floating", userWindow.dockPosition)
    end)

    it("takes the font size and wrap it was given", function()
      track(Geyser.UserWindow:new({name = "guwFont", x = 10, y = 10, width = 250, height = 180, fontSize = 12, wrapAt = 40}))
      assert.are.equal(12, getFontSize("guwFont"))
      assert.are.equal(40, getWindowWrap("guwFont"))
    end)

    -- Ubuntu Mono is asked for rather than a system font like Courier New:
    -- Mudlet ships and loads it itself, so it is there to be had on every
    -- platform, where a bare Linux CI image has no Courier New and Qt quietly
    -- substitutes the nearest match. It is also not the console default
    -- (Bitstream Vera Sans Mono), so a font that never reached the widget still
    -- fails this.
    it("takes the font family it was given", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwFamily", x = 10, y = 10, width = 250, height = 180, font = "Ubuntu Mono"}))
      assert.are.equal("Ubuntu Mono", getFont("guwFamily"))
      assert.are.equal("Ubuntu Mono", userWindow.font)
    end)

    it("derives an auto wrap from its usable width", function()
      track(Geyser.UserWindow:new({name = "guwAutoWrap", x = 10, y = 10, width = 300, height = 200, wrapAt = "auto"}))
      local usableWidth = getUserWindowSize("guwAutoWrap")
      local charWidth = calcFontSize("guwAutoWrap")
      assert.are.equal(math.floor(usableWidth / charWidth), getWindowWrap("guwAutoWrap"))
    end)

    -- Mudlet cannot report whether the scroll bar is on screen, but
    -- Geyser.MiniConsole:resetAutoWrap keeps 15 pixels clear for one when it
    -- is, so an auto wrapping console wraps that much earlier - which is
    -- readable, and is what proves the constraint reached the widget.
    it("keeps room for the scroll bar it was asked for when wrapping", function()
      track(Geyser.UserWindow:new({name = "guwScrollBar", x = 10, y = 10, width = 300, height = 200, wrapAt = "auto", scrollBar = true}))
      local usableWidth = getUserWindowSize("guwScrollBar")
      local charWidth = calcFontSize("guwScrollBar")
      assert.are.equal(math.floor((usableWidth - 15) / charWidth), getWindowWrap("guwScrollBar"))
    end)

    it("ignores the geometry it was given when it is asked to start docked", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwDocked", x = 10, y = 10, width = 300, height = 200, docked = true}))
      -- a docked window keeps the dock position it was opened with instead of
      -- being floated, and the dock area decides its geometry, not the
      -- constraints, so only the position it kept is asserted here
      assert.are.equal("r", userWindow.dockPosition)
      assert.are.equal("userwindow", windowType("guwDocked"))
      assert.is_true(windowVisible("guwDocked"))
    end)

    -- A window that is about to be floated must not be docked on the way there:
    -- docking takes the dock's size off the main window, and the percentage
    -- constraints the constructor resolves straight afterwards are measured
    -- against the main window. Which dock position was asked for is what says
    -- so; how much the main window shrinks by, and when, is Qt's business and
    -- is not the same on every platform.
    it("opens a window it is going to float as floating, not docked first", function()
      local openWindow = spy.on(_G, "openUserWindow")
      finally(function() openWindow:revert() end)
      local mainWidth, mainHeight = getMainWindowSize()
      track(Geyser.UserWindow:new({name = "guwPercent", x = "25%", y = "10%", width = "30%", height = "30%"}))
      assert.spy(openWindow).was.called_with("guwPercent", false, true, "floating")
      assert.are.same({
        x = math.floor(mainWidth * 0.25),
        y = math.floor(mainHeight * 0.1),
        width = math.floor(mainWidth * 0.3),
        height = math.floor(mainHeight * 0.3),
      }, geometry("guwPercent"))
    end)

    it("still docks a window that was asked to start docked", function()
      local openWindow = spy.on(_G, "openUserWindow")
      finally(function() openWindow:revert() end)
      local userWindow = track(Geyser.UserWindow:new({name = "guwStaysDocked", x = 10, y = 10, width = 300, height = 200, docked = true, dockPosition = "left"}))
      assert.spy(openWindow).was.called_with("guwStaysDocked", false, true, "left")
      -- a docked window keeps the position it was opened with, where a floated
      -- one has its dockPosition rewritten to "floating"
      assert.are.equal("left", userWindow.dockPosition)
      assert.is_true(windowVisible("guwStaysDocked"))
    end)

    it("new2 marks the user window as using add2", function()
      local userWindow = track(Geyser.UserWindow:new2({name = "guwNew2", x = 10, y = 10, width = 200, height = 150}))
      assert.is_true(userWindow.useAdd2)
      assert.are.equal("userwindow", windowType("guwNew2"))
    end)

    it("starts out hidden when the constraints ask for it, and shows again", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwHiddenNew", x = 10, y = 10, width = 200, height = 150, hidden = true}))
      assert.is_true(userWindow.hidden)
      assert.is_false(windowVisible("guwHiddenNew"))
      userWindow:show()
      assert.is_false(userWindow.hidden)
      assert.is_true(windowVisible("guwHiddenNew"))
    end)

    it("reopens a user window that was opened under the same name before", function()
      local first = track(Geyser.UserWindow:new({name = "guwReused", x = 10, y = 10, width = 200, height = 150}))
      local trackedWindows = #Geyser.windows
      first:delete()
      local second = track(Geyser.UserWindow:new({name = "guwReused", x = 40, y = 50, width = 260, height = 190}))
      assert.are.same({x = 40, y = 50, width = 260, height = 190}, geometry("guwReused"))
      assert.are.equal(trackedWindows, #Geyser.windows)
      assert.are.equal("guwReusedContainer", second.container.name)
    end)
  end)

  describe("Geyser.UserWindow:move/resize", function()
    local userWindow

    before_each(function()
      userWindow = track(Geyser.UserWindow:new({name = "guwMove", x = 10, y = 20, width = 300, height = 200}))
    end)

    it("moves and resizes the dock", function()
      userWindow:move(60, 70)
      userWindow:resize(320, 210)
      assert.are.same({x = 60, y = 70, width = 320, height = 210}, geometry("guwMove"))
    end)

    it("goes back to filling itself after a move", function()
      userWindow:move(60, 70)
      assert.are.equal("0px", userWindow.x)
      assert.are.equal("100%", userWindow.width)
      local usableWidth = getUserWindowSize("guwMove")
      assert.are.equal(usableWidth, userWindow:get_width())
    end)

    it("re-resolves the size of percentage children when it is resized", function()
      track(Geyser.Label:new({name = "guwMoveChild", x = 0, y = 0, width = "50%", height = "100%"}, userWindow))
      local firstWidth, firstHeight = getUserWindowSize("guwMove")
      assert.are.same({x = 0, y = 0, width = math.floor(firstWidth / 2), height = firstHeight}, geometry("guwMoveChild"))
      userWindow:resize(400, 260)
      local secondWidth, secondHeight = getUserWindowSize("guwMove")
      assert.is_true(secondWidth > firstWidth)
      assert.are.same({x = 0, y = 0, width = math.floor(secondWidth / 2), height = secondHeight}, geometry("guwMoveChild"))
    end)
  end)

  describe("Geyser.UserWindow children", function()
    local userWindow

    before_each(function()
      userWindow = track(Geyser.UserWindow:new({name = "guwHolder", x = 10, y = 20, width = 300, height = 200}))
    end)

    it("creates children inside the user window, sized against its usable area", function()
      local label = track(Geyser.Label:new({name = "guwLabel", x = "50%", y = 0, width = "50%", height = "100%"}, userWindow))
      assert.are.equal("guwHolder", label.windowname)
      local usableWidth, usableHeight = getUserWindowSize("guwHolder")
      assert.are.same({
        x = math.floor(usableWidth / 2),
        y = 0,
        width = math.floor(usableWidth / 2),
        height = usableHeight,
      }, geometry("guwLabel"))
      assert.is_true(windowVisible("guwLabel"))
    end)

    it("holds a command line of its own", function()
      track(Geyser.CommandLine:new({name = "guwCmdLine", x = 0, y = 0, width = 80, height = 20}, userWindow))
      assert.are.equal("commandline", windowType("guwCmdLine"))
      assert.are.same({x = 0, y = 0, width = 80, height = 20}, geometry("guwCmdLine"))
    end)

    it("holds a scroll box, which becomes a parent window inside it", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "guwScrollBox", x = 0, y = 0, width = "100%", height = "50%"}, userWindow))
      assert.are.equal("scrollbox", windowType("guwScrollBox"))
      -- the scroll box takes over as parent window, remembering the user window
      assert.are.equal("guwScrollBox", scrollBox.windowname)
      assert.are.equal("guwHolder", scrollBox.parentWindowName)
      local usableWidth, usableHeight = getUserWindowSize("guwHolder")
      assert.are.same({x = 0, y = 0, width = usableWidth, height = math.floor(usableHeight / 2)}, geometry("guwScrollBox"))
      -- and a child of the scroll box is placed in the scroll box's own space
      local label = track(Geyser.Label:new({name = "guwScrollBoxLabel", x = 0, y = 0, width = "50%", height = "100%"}, scrollBox))
      assert.are.equal("guwScrollBox", label.windowname)
      assert.are.equal(math.floor(usableWidth / 2), geometry("guwScrollBoxLabel").width)
    end)

    it("deletes its children with itself", function()
      track(Geyser.Label:new({name = "guwDoomedLabel", x = 0, y = 0, width = 20, height = 20}, userWindow))
      userWindow:delete()
      assert.is_nil(windowType("guwHolder"))
      assert.is_nil(windowType("guwDoomedLabel"))
    end)
  end)

  describe("Geyser.UserWindow echo", function()
    it("echoes into the console the user window contains", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwEcho", x = 10, y = 20, width = 300, height = 200}))
      userWindow:echo("into the user window\n")
      assert.are.equal("into the user window\n", userWindow.message)
      assert.are.equal("into the user window", lastLine("guwEcho"))
    end)
  end)

  describe("Geyser.UserWindow:hide/show", function()
    it("hides and shows the dock", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwHide", x = 10, y = 20, width = 200, height = 150}))
      userWindow:hide()
      assert.is_true(userWindow.hidden)
      assert.is_false(windowVisible("guwHide"))
      userWindow:show()
      assert.is_false(userWindow.hidden)
      assert.is_true(windowVisible("guwHide"))
    end)

    it("hides the children in it along with the dock", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwHideChild", x = 10, y = 20, width = 200, height = 150}))
      track(Geyser.Label:new({name = "guwHiddenLabel", x = 0, y = 0, width = "100%", height = "100%"}, userWindow))
      userWindow:hide()
      assert.is_false(windowVisible("guwHiddenLabel"))
      userWindow:show()
      assert.is_true(windowVisible("guwHiddenLabel"))
    end)
  end)

  -- Geyser.UserWindow:show() forwards to its parent, and has to pass on the
  -- `auto` flag its container hands down: the base class picks which of the two
  -- hidden bits to clear from it. Without the flag an automatic show clears
  -- self.hidden as if the user had asked for it, and never clears auto_hidden.
  describe("Geyser.UserWindow show cascade", function()
    it("stays hidden when its root container is shown after a hand hide", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwHandHidden", x = 10, y = 20, width = 200, height = 150}))
      userWindow:hide()
      assert.is_true(userWindow.hidden)
      assert.is_false(windowVisible("guwHandHidden"))
      userWindow.container:show()
      assert.is_true(userWindow.hidden)
      assert.is_false(windowVisible("guwHandHidden"))
    end)

    it("comes back when the container that hid it is shown again", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwAutoHidden", x = 10, y = 20, width = 200, height = 150}))
      userWindow.container:hide()
      assert.is_true(userWindow.auto_hidden)
      assert.is_false(windowVisible("guwAutoHidden"))
      userWindow.container:show()
      assert.is_false(userWindow.auto_hidden)
      assert.is_true(windowVisible("guwAutoHidden"))
    end)
  end)

  describe("Geyser.UserWindow:setTitle/resetTitle", function()
    -- setUserWindowTitle answers nil and a message rather than raising when it
    -- cannot find the window, so the return value is what says the title
    -- reached the right dock; without it a wrapper naming the wrong window
    -- would still leave titleText looking right.
    it("remembers the title it was given, and empties it again on reset", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwTitle", x = 10, y = 20, width = 200, height = 150, titleText = "My window"}))
      assert.are.equal("My window", userWindow.titleText)
      assert.is_true(userWindow:setTitle("Renamed"))
      assert.are.equal("Renamed", userWindow.titleText)
      assert.is_true(userWindow:resetTitle())
      assert.are.equal("", userWindow.titleText)
    end)

    it("starts with an empty title when it was not given one", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwNoTitle", x = 10, y = 20, width = 200, height = 150}))
      assert.are.equal("", userWindow.titleText)
    end)
  end)

  pending("Geyser.UserWindow:setTitle/resetTitle put the text on the dock's title bar - needs a getUserWindowTitle getter")

  describe("Geyser.UserWindow:setStyleSheet", function()
    it("remembers the stylesheet it was given", function()
      local userWindow = track(Geyser.UserWindow:new({
        name = "guwCss",
        x = 10, y = 20, width = 200, height = 150,
        stylesheet = "border: 1px solid red;",
      }))
      assert.are.equal("border: 1px solid red;", userWindow.stylesheet)
      userWindow:setStyleSheet("background-color: green;")
      assert.are.equal("background-color: green;", userWindow.stylesheet)
    end)
  end)

  pending("Geyser.UserWindow:setStyleSheet applies the stylesheet to the dock - needs a getUserWindowStyleSheet getter")

  pending("Geyser.UserWindow scrollBar constraint puts a scroll bar on screen - the wrap it leaves room for is covered above, but the scroll bar's own visibility is not readable from Lua and needs a scroll bar getter")

  describe("Geyser.UserWindow:enableAutoDock/disableAutoDock", function()
    it("turns automatic docking off and on again without disturbing the window", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwAutoDock", x = 10, y = 20, width = 300, height = 200}))
      assert.is_true(userWindow:disableAutoDock())
      assert.is_false(userWindow.autoDock)
      -- both of these reopen the window, so the dock must survive them intact
      assert.is_true(windowVisible("guwAutoDock"))
      assert.are.same({x = 10, y = 20, width = 300, height = 200}, geometry("guwAutoDock"))
      assert.is_true(userWindow:enableAutoDock())
      assert.is_true(userWindow.autoDock)
      assert.is_true(windowVisible("guwAutoDock"))
      assert.are.same({x = 10, y = 20, width = 300, height = 200}, geometry("guwAutoDock"))
    end)
  end)

  pending("Geyser.UserWindow:setDockPosition - which edge a user window ended up docked to, and whether it docks by itself when dragged, are not readable from Lua")

  -- restoreLayout = true makes the constructor reopen the window from the
  -- layout saved in the profile and skip the move/resize it was given. Running
  -- that here would write this file's window layouts into the shared self-test
  -- profile, so repeat runs against the same profile would stop matching.
  pending("Geyser.UserWindow restoreLayout reopens the window where it was last left")

  describe("Geyser.UserWindow:delete", function()
    it("closes the dock and unregisters the user window", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwDelete", x = 10, y = 20, width = 200, height = 150}))
      assert.are.equal("userwindow", windowType("guwDelete"))
      userWindow:delete()
      assert.is_nil(windowType("guwDelete"))
      assert.is_nil(getWindowGeometry("guwDelete"))
      assert.is_nil(Geyser.parentWindows.guwDelete)
    end)

    -- Deleting a user window has to take its dock widget with it, not only the
    -- console inside it. That the name can be used again is covered by
    -- "reopens a user window that was opened under the same name before"; what
    -- is read here is the dock itself. getUserWindowSize answers from the dock
    -- registry, falling back to the main window size when the name is not in
    -- it, so a dock left behind gives itself away by answering with its own
    -- size instead.
    it("takes its dock widget with it, so nothing stale answers for the name", function()
      local mainWidth, mainHeight = getMainWindowSize()
      local userWindow = track(Geyser.UserWindow:new({name = "guwReopen", x = 10, y = 20, width = 200, height = 150}))
      assert.is_true(getUserWindowSize("guwReopen") < mainWidth, "a user window that reports the main window's size has no dock of its own")

      userWindow:delete()
      assert.is_nil(windowType("guwReopen"))
      -- a dock left behind is still holding a live widget here, so it would
      -- answer with its own size rather than the fallback
      assert.are.same({mainWidth, mainHeight}, {getUserWindowSize("guwReopen")})
      -- and once the console's deferred deletion has run that widget is freed,
      -- which is the moment the query used to dereference it
      pumpEvents(50)
      assert.are.same({mainWidth, mainHeight}, {getUserWindowSize("guwReopen")})

      track(Geyser.UserWindow:new({name = "guwReopen", x = 10, y = 20, width = 200, height = 150}))
      assert.are.equal("userwindow", windowType("guwReopen"))
      assert.is_true(getUserWindowSize("guwReopen") < mainWidth, "the reopened window has no dock of its own")
    end)
  end)

  -- Geyser.Container:new makes the "<name>Container" root container for a user
  -- window. An orphaned one is not inert: its get_width/get_height ask
  -- getUserWindowSize for a window that is gone, which falls back to the main
  -- window size, so every leftover claims the whole main window in every
  -- layout pass.
  describe("Geyser.UserWindow root container cleanup", function()
    it("removes the root container it created", function()
      local trackedWindows = #Geyser.windows
      local userWindow = track(Geyser.UserWindow:new({name = "guwRootGone", x = 10, y = 20, width = 200, height = 150}))
      assert.are.equal(userWindow.container, Geyser.windowList.guwRootGoneContainer)
      userWindow:delete()
      assert.is_nil(Geyser.windowList.guwRootGoneContainer)
      assert.is_nil(table.index_of(Geyser.windows, "guwRootGoneContainer"))
      assert.are.equal(trackedWindows, #Geyser.windows)
    end)

    it("leaves nothing behind over repeated create and delete cycles", function()
      local trackedWindows = #Geyser.windows
      for index = 1, 5 do
        Geyser.UserWindow:new({name = "guwCycle" .. index, x = 10, y = 20, width = 200, height = 150}):delete()
      end
      assert.are.equal(trackedWindows, #Geyser.windows)
    end)

    -- anything else the user put in the root container is still using it, so it
    -- has to survive the user window being deleted out of it
    it("leaves a root container that still holds something else", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwRootShared", x = 10, y = 20, width = 200, height = 150}))
      local root = userWindow.container
      local lodger = track(Geyser.Label:new({name = "guwRootLodger", x = 0, y = 0, width = 20, height = 20}, root))
      userWindow:delete()
      assert.are.equal(root, Geyser.windowList.guwRootSharedContainer)
      assert.are.equal(lodger, root.windowList.guwRootLodger)
      root:delete()
      assert.is_nil(Geyser.windowList.guwRootSharedContainer)
    end)

    -- a user window moved out of the root container Geyser made for it still
    -- has to take that container with it, and the container is no longer the
    -- one the user window reports as its own
    it("removes the root container even after the user window was moved out of it", function()
      local elsewhere = track(Geyser.Container:new({name = "guwNewHome", x = 0, y = 0, width = 200, height = 200}))
      local userWindow = track(Geyser.UserWindow:new({name = "guwMovedOut", x = 10, y = 20, width = 200, height = 150}))
      userWindow:changeContainer(elsewhere)
      assert.are.equal(elsewhere, userWindow.container)
      userWindow:delete()
      assert.is_nil(Geyser.windowList.guwMovedOutContainer)
    end)

    -- deleting the root container deletes the user window inside it, which
    -- reaches back for the root container it is being deleted by, so that
    -- cascade has to come apart cleanly rather than recursing
    it("comes apart cleanly when the root container is the one deleted", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwRootKept", x = 10, y = 20, width = 200, height = 150}))
      local root = userWindow.container
      track(Geyser.Label:new({name = "guwRootKeptLabel", x = 0, y = 0, width = 20, height = 20}, userWindow))
      assert.are.equal(root, Geyser.windowList.guwRootKeptContainer)
      root:delete()
      assert.is_nil(Geyser.windowList.guwRootKeptContainer)
      assert.is_nil(windowType("guwRootKept"))
      assert.is_nil(windowType("guwRootKeptLabel"))
    end)
  end)
end)

-- set_uwconstr is what move() and resize() call before they touch the dock: it
-- re-reads the window's own x/y/width/height against the main window rather
-- than against the user window's insides, which is what every other Geyser
-- object's set_constraints does. resetWindow() puts the usual behaviour back,
-- so the two are specced against each other here.
describe("Tests Geyser.UserWindow:set_uwconstr", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  local function alive(object)
    if not object or not object.container or not object.container.windowList then
      return false
    end
    return object.container.windowList[object.name] == object
  end

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  it("resolves percentages against the main window", function()
    local userWindow = track(Geyser.UserWindow:new({name = "guwConstr", x = 10, y = 20, width = 200, height = 150}))
    local mainWidth, mainHeight = getMainWindowSize()

    userWindow.x, userWindow.y = "50%", "25%"
    userWindow.width, userWindow.height = "20%", "10%"
    userWindow:set_uwconstr()

    assert.are.equal(mainWidth * 0.5, userWindow:get_x())
    assert.are.equal(mainHeight * 0.25, userWindow:get_y())
    assert.are.equal(mainWidth * 0.2, userWindow:get_width())
    assert.are.equal(mainHeight * 0.1, userWindow:get_height())
  end)

  it("resolves pixels as pixels", function()
    local userWindow = track(Geyser.UserWindow:new({name = "guwConstrPixels", x = 10, y = 20, width = 200, height = 150}))

    userWindow.x, userWindow.y = 120, 130
    userWindow.width, userWindow.height = 240, 260
    userWindow:set_uwconstr()

    assert.are.equal(120, userWindow:get_x())
    assert.are.equal(130, userWindow:get_y())
    assert.are.equal(240, userWindow:get_width())
    assert.are.equal(260, userWindow:get_height())
  end)

  it("is undone by resetWindow, which sizes the window against itself again", function()
    local userWindow = track(Geyser.UserWindow:new({name = "guwConstrReset", x = 10, y = 20, width = 200, height = 150}))
    local mainWidth = getMainWindowSize()

    userWindow.width = "100%"
    userWindow:set_uwconstr()
    assert.are.equal(mainWidth, userWindow:get_width())

    userWindow:resetWindow()
    -- back to filling itself: "100%" now means the usable area inside the dock,
    -- which is nothing like the main window's width
    local usableWidth = getUserWindowSize("guwConstrReset")
    assert.are.equal(usableWidth, userWindow:get_width())
    assert.is_true(usableWidth < mainWidth)
  end)

  it("is what move and resize position the dock with", function()
    local userWindow = track(Geyser.UserWindow:new({name = "guwConstrMove", x = 10, y = 20, width = 200, height = 150}))

    -- move()/resize() hand their arguments to set_uwconstr and then move the
    -- real dock to what it worked out, so a percentage has to land on the same
    -- pixel that set_uwconstr resolves it to
    userWindow.x, userWindow.y = "10%", "10%"
    userWindow:set_uwconstr()
    local expectedX, expectedY = userWindow:get_x(), userWindow:get_y()

    userWindow:move("10%", "10%")
    local x, y = getWindowGeometry("guwConstrMove")
    assert.are.equal(math.floor(expectedX), x)
    assert.are.equal(math.floor(expectedY), y)
  end)
end)
