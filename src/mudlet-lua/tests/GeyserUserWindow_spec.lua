-- A Geyser.UserWindow is a Geyser.MiniConsole living in a dock widget of its
-- own. getWindowGeometry reports that dock, which is what move()/resize() drive,
-- while getUserWindowSize reports the usable area inside it - always a little
-- smaller, because the dock spends pixels on its frame and title bar. The exact
-- difference is a style detail, so it is never hardcoded here.
--
-- Geyser gives every user window an extra root container named
-- "<name>Container" whose size tracks the real user window; the user window is
-- that container's only child, which is why it is not in Geyser.windowList
-- itself.
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

  -- Deleting a user window leaves its "<name>Container" root container behind
  -- (see the pending below), so sweep those out by hand to keep repeat runs of
  -- this file against the same profile identical.
  after_each(function()
    local names = {}
    for _, object in ipairs(created) do
      if object.type == "userwindow" then
        names[#names + 1] = object.name .. "Container"
      end
      if alive(object) then
        object:delete()
      end
    end
    for _, name in ipairs(names) do
      local orphan = Geyser.windowList[name]
      if orphan then
        orphan:delete()
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
      local usableWidth, usableHeight = getUserWindowSize("guwRoot")
      assert.are.equal(usableWidth, container.get_width())
      assert.are.equal(usableHeight, container.get_height())
      -- the usable area is inside the dock, so smaller than it but still real
      assert.is_true(usableWidth > 0 and usableWidth < 300)
      assert.is_true(usableHeight > 0 and usableHeight < 200)
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

    it("new2 marks the user window as using add2", function()
      local userWindow = track(Geyser.UserWindow:new2({name = "guwNew2", x = 10, y = 10, width = 200, height = 150}))
      assert.is_true(userWindow.useAdd2)
      assert.are.equal("userwindow", windowType("guwNew2"))
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

  -- Geyser.UserWindow:show() (GeyserUserWindow.lua:52) forwards to its parent
  -- without the `auto` flag its container passes down, so an automatic show
  -- clears self.hidden as if the user had asked for it. A user window hidden by
  -- hand therefore reappears the moment its root container is shown, where a
  -- Geyser.MiniConsole in the same position correctly stays hidden.
  pending("Geyser.UserWindow stays hidden when its root container is shown - Geyser.UserWindow:show() drops the auto flag")

  describe("Geyser.UserWindow:setTitle/resetTitle", function()
    it("remembers the title it was given, and empties it again on reset", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwTitle", x = 10, y = 20, width = 200, height = 150, titleText = "My window"}))
      assert.are.equal("My window", userWindow.titleText)
      userWindow:setTitle("Renamed")
      assert.are.equal("Renamed", userWindow.titleText)
      userWindow:resetTitle()
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

  pending("Geyser.UserWindow scrollBar constraint shows the console's scroll bar - scroll bar visibility is not readable from Lua, needs a scroll bar getter")

  pending("Geyser.UserWindow:setDockPosition/enableAutoDock/disableAutoDock - which edge a user window is docked to, and whether it docks by itself, are not readable from Lua")

  describe("Geyser.UserWindow:delete", function()
    it("closes the dock and unregisters the user window", function()
      local userWindow = track(Geyser.UserWindow:new({name = "guwDelete", x = 10, y = 20, width = 200, height = 150}))
      assert.are.equal("userwindow", windowType("guwDelete"))
      userWindow:delete()
      assert.is_nil(windowType("guwDelete"))
      assert.is_nil(getWindowGeometry("guwDelete"))
      assert.is_nil(Geyser.parentWindows.guwDelete)
    end)
  end)

  -- Geyser.Container:new (GeyserContainer.lua:361) makes the "<name>Container"
  -- root container for a user window, but Geyser.Container:delete only unhooks
  -- the user window from it, so the container is left registered in
  -- Geyser.windowList and Geyser.windows for the rest of the session.
  pending("deleting a Geyser.UserWindow also removes the root container it created")
end)
