-- A Geyser.ScrollBox is both a widget in its parent window and a parent window
-- of its own: it swaps its windowname for its own name so that everything added
-- to it is created inside the scroll box. Its children therefore report
-- geometry in the scroll box's coordinate space, not the main window's, which
-- is what lets a child be taller than the box and scroll.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.ScrollBox", function()
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

  describe("Geyser.ScrollBox:new/new2", function()
    it("creates a scroll box widget at the constrained geometry", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbNew", x = 10, y = 20, width = 200, height = 150}))
      -- Geyser's own type string is camel cased, Mudlet's windowType is not
      assert.are.equal("scrollBox", scrollBox.type)
      assert.are.equal("scrollbox", windowType("gsbNew"))
      assert.are.same({x = 10, y = 20, width = 200, height = 150}, geometry("gsbNew"))
      assert.is_true(windowVisible("gsbNew"))
      assert.are.equal(scrollBox, Geyser.windowList.gsbNew)
    end)

    it("becomes a parent window of its own, remembering the one it was made in", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbParent", x = 0, y = 0, width = 100, height = 100}))
      assert.are.equal("gsbParent", scrollBox.windowname)
      assert.are.equal("main", scrollBox.parentWindowName)
      assert.are.equal(scrollBox, Geyser.parentWindows.gsbParent)
    end)

    it("reports its own origin as zero so children are placed inside it", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbOrigin", x = 40, y = 60, width = 100, height = 100}))
      assert.are.equal(0, scrollBox.get_x())
      assert.are.equal(0, scrollBox.get_y())
      -- the widget itself is still where its constraints put it
      assert.are.same({x = 40, y = 60, width = 100, height = 100}, geometry("gsbOrigin"))
    end)

    it("uses Geyser's defaults when no constraints are given", function()
      track(Geyser.ScrollBox:new({name = "gsbDefaults"}))
      assert.are.same({x = 10, y = 10, width = 300, height = 200}, geometry("gsbDefaults"))
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gsbBox", x = 50, y = 60, width = 300, height = 200}))
      track(Geyser.ScrollBox:new({name = "gsbInBox", x = "10%", y = "10%", width = "80%", height = "80%"}, container))
      assert.are.same({x = 80, y = 80, width = 240, height = 160}, geometry("gsbInBox"))
    end)

    it("new2 marks the scroll box as using add2", function()
      local scrollBox = track(Geyser.ScrollBox:new2({name = "gsbNew2", x = 0, y = 0, width = 50, height = 50}))
      assert.is_true(scrollBox.useAdd2)
      assert.are.equal("scrollbox", windowType("gsbNew2"))
    end)
  end)

  describe("Geyser.ScrollBox children", function()
    local scrollBox

    before_each(function()
      scrollBox = track(Geyser.ScrollBox:new({name = "gsbHolder", x = 10, y = 20, width = 200, height = 150}))
    end)

    it("places children in its own coordinate space", function()
      local console = track(Geyser.MiniConsole:new({name = "gsbConsole", x = "10%", y = "10%", width = "80%", height = "50%"}, scrollBox))
      assert.are.equal("gsbHolder", console.windowname)
      -- 10%/10% of the box, not of the main window, and with no 10,20 offset
      assert.are.same({x = 20, y = 15, width = 160, height = 75}, geometry("gsbConsole"))
    end)

    it("holds a command line as well as a console", function()
      track(Geyser.CommandLine:new({name = "gsbCmdLine", x = 0, y = "80%", width = "100%", height = 25}, scrollBox))
      assert.are.equal("commandline", windowType("gsbCmdLine"))
      assert.are.same({x = 0, y = 120, width = 200, height = 25}, geometry("gsbCmdLine"))
    end)

    it("lets a child be taller than the box, which is what makes it scroll", function()
      track(Geyser.Label:new({name = "gsbTall", x = 0, y = 0, width = "100%", height = 2000}, scrollBox))
      assert.are.same({x = 0, y = 0, width = 200, height = 2000}, geometry("gsbTall"))
    end)

    it("re-lays its children out when it is resized", function()
      track(Geyser.MiniConsole:new({name = "gsbResized", x = "10%", y = "10%", width = "80%", height = "50%"}, scrollBox))
      scrollBox:resize(400, 300)
      assert.are.same({x = 10, y = 20, width = 400, height = 300}, geometry("gsbHolder"))
      assert.are.same({x = 40, y = 30, width = 320, height = 150}, geometry("gsbResized"))
    end)

    it("keeps children where they are when the box itself moves", function()
      track(Geyser.Label:new({name = "gsbFollower", x = "50%", y = 0, width = "50%", height = "100%"}, scrollBox))
      assert.are.same({x = 100, y = 0, width = 100, height = 150}, geometry("gsbFollower"))
      scrollBox:move(80, 90)
      assert.are.same({x = 80, y = 90, width = 200, height = 150}, geometry("gsbHolder"))
      -- the child rides along inside the widget, so its own coordinates do not move
      assert.are.same({x = 100, y = 0, width = 100, height = 150}, geometry("gsbFollower"))
    end)

    it("nests a container of its own inside the scroll box", function()
      local inner = track(Geyser.Container:new({name = "gsbInner", x = 0, y = 0, width = "50%", height = "50%"}, scrollBox))
      local label = track(Geyser.Label:new({name = "gsbInnerLabel", x = "50%", y = 0, width = "50%", height = "100%"}, inner))
      assert.are.equal("gsbHolder", label.windowname)
      assert.are.same({x = 50, y = 0, width = 50, height = 75}, geometry("gsbInnerLabel"))
    end)

    it("nests a scroll box inside a scroll box, each its own parent window", function()
      local inner = track(Geyser.ScrollBox:new({name = "gsbNestedBox", x = 10, y = 10, width = "50%", height = "50%"}, scrollBox))
      assert.are.equal("gsbNestedBox", inner.windowname)
      assert.are.equal("gsbHolder", inner.parentWindowName)
      assert.are.same({x = 10, y = 10, width = 100, height = 75}, geometry("gsbNestedBox"))
      -- a child of the inner box is placed in the inner box's own space again
      local label = track(Geyser.Label:new({name = "gsbNestedLabel", x = "50%", y = 0, width = "50%", height = "100%"}, inner))
      assert.are.equal("gsbNestedBox", label.windowname)
      assert.are.same({x = 50, y = 0, width = 50, height = 75}, geometry("gsbNestedLabel"))
    end)
  end)

  describe("Geyser.ScrollBox:hide/show", function()
    it("hides and shows the scroll box and its children", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbVisible", x = 0, y = 0, width = 200, height = 150}))
      track(Geyser.Label:new({name = "gsbVisibleChild", x = 0, y = 0, width = "100%", height = "100%"}, scrollBox))
      scrollBox:hide()
      assert.is_true(scrollBox.hidden)
      assert.is_false(windowVisible("gsbVisible"))
      assert.is_false(windowVisible("gsbVisibleChild"))
      scrollBox:show()
      assert.is_false(scrollBox.hidden)
      assert.is_true(windowVisible("gsbVisible"))
      assert.is_true(windowVisible("gsbVisibleChild"))
    end)
  end)

  describe("Geyser.ScrollBox:reposition", function()
    it("restores geometry that was changed behind Geyser's back", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbReposition", x = 10, y = 10, width = 100, height = 80}))
      track(Geyser.Label:new({name = "gsbRepositionChild", x = 5, y = 5, width = "50%", height = "50%"}, scrollBox))
      moveWindow("gsbReposition", 300, 300)
      resizeWindow("gsbReposition", 20, 20)
      assert.are.same({x = 300, y = 300, width = 20, height = 20}, geometry("gsbReposition"))
      local mainWidth, mainHeight = getMainWindowSize()
      GeyserReposition("sysWindowResizeEvent", mainWidth, mainHeight)
      assert.are.same({x = 10, y = 10, width = 100, height = 80}, geometry("gsbReposition"))
      assert.are.same({x = 5, y = 5, width = 50, height = 40}, geometry("gsbRepositionChild"))
    end)

    it("puts its own origin back to zero after repositioning", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbOriginKept", x = 30, y = 40, width = 100, height = 80}))
      scrollBox:reposition()
      assert.are.equal(0, scrollBox.get_x())
      assert.are.equal(0, scrollBox.get_y())
      assert.are.same({x = 30, y = 40, width = 100, height = 80}, geometry("gsbOriginKept"))
    end)
  end)

  -- Geyser:add2 runs from inside Geyser.Container:new, so the hide it asks for
  -- lands before createScrollBox has made the widget. Every Geyser widget
  -- constructor therefore hides itself again afterwards
  -- (GeyserCommandLine.lua:89, GeyserMiniConsole.lua:605, GeyserLabel.lua:998,
  -- GeyserTextEdit.lua:70, GeyserMapper.lua:133), and a Geyser.MiniConsole
  -- built the same way is the reference behaviour asserted alongside.
  describe("Geyser.ScrollBox created in a hidden container", function()
    it("stays off screen, and comes back when the container is shown", function()
      local parent = track(Geyser.Container:new2({name = "gsbHiddenParent", x = 0, y = 0, width = 300, height = 200}))
      parent:hide()
      local scrollBox = track(Geyser.ScrollBox:new2({name = "gsbBornHidden", x = 0, y = 0, width = "100%", height = "50%"}, parent))
      local console = track(Geyser.MiniConsole:new2({name = "gsbBornHiddenConsole", x = 0, y = "50%", width = "100%", height = "50%"}, parent))
      assert.is_true(scrollBox.auto_hidden)
      assert.is_true(console.auto_hidden)
      assert.is_false(windowVisible("gsbBornHiddenConsole"))
      assert.is_false(windowVisible("gsbBornHidden"))
      parent:show()
      assert.is_true(windowVisible("gsbBornHidden"))
      assert.is_true(windowVisible("gsbBornHiddenConsole"))
    end)

    -- a scroll box that came up on screen while its bookkeeping said hidden
    -- could not be taken off it again: Geyser.Container:hide skips hide_impl
    -- for anything that already believes itself hidden, so neither an explicit
    -- hide nor another parent:hide() reached it
    it("can be taken off screen by hand without being shown first", function()
      local parent = track(Geyser.Container:new2({name = "gsbHideParent", x = 0, y = 0, width = 300, height = 200}))
      parent:hide()
      local scrollBox = track(Geyser.ScrollBox:new2({name = "gsbHideByHand", x = 0, y = 0, width = "100%", height = "100%"}, parent))
      scrollBox:hide()
      assert.is_false(windowVisible("gsbHideByHand"))
      parent:hide()
      assert.is_false(windowVisible("gsbHideByHand"))
    end)

    -- add2 carries a hidden constraint through as well as an inherited one
    it("stays off screen when it was asked to start hidden", function()
      local scrollBox = track(Geyser.ScrollBox:new2({name = "gsbBornHiddenFlag", x = 0, y = 0, width = 100, height = 100, hidden = true}))
      assert.is_true(scrollBox.hidden)
      assert.is_false(windowVisible("gsbBornHiddenFlag"))
      scrollBox:show()
      assert.is_true(windowVisible("gsbBornHiddenFlag"))
    end)
  end)

  describe("Geyser.ScrollBox scroll bars", function()
    -- A scroll box scrolls by being a QScrollArea (TScrollBox.h), not by being
    -- a console, so Mudlet's scroll bar API cannot reach it: Host::findConsole
    -- only looks through the sub-console map, and a scroll box is not in it.
    -- Geyser.ScrollBox descends from Geyser.Window rather than
    -- Geyser.MiniConsole, so it offers no scroll bar method of its own either.
    -- Its scroll bars are Qt's, and appear on their own when a child overflows.
    it("is not reachable by the console scroll bar functions", function()
      track(Geyser.ScrollBox:new({name = "gsbScrollBar", x = 0, y = 0, width = 200, height = 150}))
      local enabled, enableMessage = enableScrollBar("gsbScrollBar")
      assert.is_nil(enabled)
      assert.is_truthy(enableMessage:find("gsbScrollBar", 1, true))
      local disabled, disableMessage = disableScrollBar("gsbScrollBar")
      assert.is_nil(disabled)
      assert.is_truthy(disableMessage:find("gsbScrollBar", 1, true))
      assert.is_nil(Geyser.ScrollBox.enableScrollBar)
      assert.is_nil(Geyser.ScrollBox.disableScrollBar)
    end)
  end)

  pending("Geyser.ScrollBox shows Qt's own scroll bar once a child overflows it - a scroll box is not a console, so no console scroll bar getter can report it; this needs a scroll box specific getter")

  pending("Geyser.ScrollBox:setStyleSheet - the method is commented out in GeyserScrollBox.lua because Mudlet has no setScrollBoxStyleSheet primitive to call")

  describe("Geyser.ScrollBox:type_delete", function()
    it("deletes the widget, its children and its parent window registration", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbDelete", x = 0, y = 0, width = 200, height = 150}))
      track(Geyser.Label:new({name = "gsbDeleteChild", x = 0, y = 0, width = "100%", height = "100%"}, scrollBox))
      scrollBox:delete()
      assert.is_nil(windowType("gsbDelete"))
      assert.is_nil(windowType("gsbDeleteChild"))
      assert.is_nil(Geyser.windowList.gsbDelete)
      assert.is_nil(Geyser.parentWindows.gsbDelete)
    end)

    it("goes away with the container it was put in", function()
      local container = track(Geyser.Container:new({name = "gsbOuter", x = 0, y = 0, width = 300, height = 200}))
      track(Geyser.ScrollBox:new({name = "gsbNested", x = 0, y = 0, width = "100%", height = "100%"}, container))
      container:delete()
      assert.is_nil(windowType("gsbNested"))
      assert.is_nil(Geyser.parentWindows.gsbNested)
    end)
  end)
end)
