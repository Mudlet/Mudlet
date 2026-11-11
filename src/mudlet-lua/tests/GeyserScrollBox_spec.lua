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

    it("starts out hidden when the constraints ask for it", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbHiddenNew", x = 0, y = 0, width = 50, height = 50, hidden = true}))
      assert.is_true(scrollBox.hidden)
      assert.is_false(windowVisible("gsbHiddenNew"))
      scrollBox:show()
      assert.is_false(scrollBox.hidden)
      assert.is_true(windowVisible("gsbHiddenNew"))
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

    it("gives a child its whole pixel size when the box has a fractional one", function()
      local fractional = track(Geyser.ScrollBox:new({name = "gsbFractional", x = "0.6px", y = 0, width = "100.6px", height = "50.6px"}))
      track(Geyser.Label:new({name = "gsbFractionalChild", x = 0, y = 0, width = "100%", height = "100%"}, fractional))
      local box = geometry("gsbFractional")
      -- edges at 0.6 and 101.2 round to 1 and 101
      assert.are.same({x = 1, y = 0, width = 100, height = 51}, box)
      assert.are.same({x = 0, y = 0, width = box.width, height = box.height}, geometry("gsbFractionalChild"))
      fractional:resize("120.6px", nil)
      box = geometry("gsbFractional")
      assert.are.equal(120, box.width)
      assert.are.same({x = 0, y = 0, width = box.width, height = box.height}, geometry("gsbFractionalChild"))
    end)

    it("follows its constraints between repositions", function()
      local holder = track(Geyser.Container:new({name = "gsbFollowHolder", x = 0, y = 0, width = 200, height = 100}))
      local inner = Geyser.ScrollBox:new({name = "gsbFollow", x = 0, y = 0, width = "50%", height = "50%"}, holder)
      assert.are.equal(100, inner:get_width())
      holder.width = "400px"
      Geyser.calc_constraints(holder, holder, holder.container)
      assert.are.equal(200, inner:get_width())
    end)

    it("gives a child its whole size when its size is measured back from the far edge", function()
      local mainWidth, mainHeight = getMainWindowSize()
      local box = track(Geyser.ScrollBox:new({name = "gsbNegative", x = "50px", y = "40px", width = "-10px", height = "-20px"}))
      track(Geyser.Label:new({name = "gsbNegativeChild", x = 0, y = 0, width = "100%", height = "100%"}, box))
      assert.are.same({x = 50, y = 40, width = mainWidth - 60, height = mainHeight - 60}, geometry("gsbNegative"))
      assert.are.same({x = 0, y = 0, width = mainWidth - 60, height = mainHeight - 60}, geometry("gsbNegativeChild"))
      assert.are.equal(mainWidth - 60, box:get_width())
      assert.are.equal(mainHeight - 60, box:get_height())
      box:reposition()
      assert.are.same({x = 50, y = 40, width = mainWidth - 60, height = mainHeight - 60}, geometry("gsbNegative"))
    end)

    it("is placed by constraints that read its own size and origin", function()
      local mainWidth = getMainWindowSize()
      local box
      local function centred()
        return box and (mainWidth - box:get_width()) / 2 or 0
      end
      local function halfX()
        return box and box:get_x() / 2 or 0
      end
      local function twiceHeight()
        return box and box:get_height() * 2 or 0
      end
      box = track(Geyser.ScrollBox:new({name = "gsbCentred", x = centred, y = halfX, width = twiceHeight, height = 100}))
      box:reposition()
      local x = (mainWidth - 200) / 2
      local function edge(value)
        return math.floor(value + 0.5)
      end
      assert.are.same({x = edge(x), y = edge(x / 2), width = edge(x + 200) - edge(x), height = edge(x / 2 + 100) - edge(x / 2)}, geometry("gsbCentred"))
    end)

    it("is placed by constraints that read its unrounded size through a helper", function()
      local mainWidth = getMainWindowSize()
      local box
      local function widthOf(window)
        return window:get_width()
      end
      local function heightOf(window)
        return window:get_height()
      end
      local function lowered()
        return box and heightOf(box) / 100 or 0
      end
      local function twiceWidth()
        return box and widthOf(box) * 2 or 0
      end
      box = track(Geyser.ScrollBox:new({name = "gsbProportioned", x = 0, y = lowered, width = "-10.5px", height = twiceWidth}))
      box:reposition()
      local width = mainWidth - 10.5
      local y = width * 2 / 100
      local function edge(value)
        return math.floor(value + 0.5)
      end
      assert.are.same({x = 0, y = edge(y), width = edge(width), height = edge(y + width * 2) - edge(y)}, geometry("gsbProportioned"))
    end)

    it("leaves room for the next child when it is a fixed size child of an HBox", function()
      local box = track(Geyser.HBox:new({name = "gsbFixedBox", x = 0, y = 0, width = 300, height = 20}))
      Geyser.ScrollBox:new({name = "gsbFixed", width = "30%", h_policy = Geyser.Fixed}, box)
      Geyser.Label:new({name = "gsbFixedNext"}, box)
      box:resize(600, nil)
      local fixed = geometry("gsbFixed")
      assert.are.equal(fixed.x + fixed.width, geometry("gsbFixedNext").x)
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

    it("refuses to move into a container nested inside it", function()
      local inner = track(Geyser.Container:new({name = "gsbCycleInner", x = 0, y = 0, width = "50%", height = "50%"}, scrollBox))
      local innerBox = track(Geyser.ScrollBox:new({name = "gsbCycleBox", x = 0, y = 0, width = "50%", height = "50%"}, inner))
      for _, destination in ipairs({inner, innerBox}) do
        local result, message = scrollBox:changeContainer(destination)
        assert.is_nil(result)
        assert.are.equal("can't move gsbHolder into one of its own children", message)
        assert.are.equal(Geyser, scrollBox.container)
      end
      assert.are.same({x = 10, y = 20, width = 200, height = 150}, geometry("gsbHolder"))
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

    it("places itself through a pixel_geometry override", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbOverridden", x = 30, y = 40, width = 100, height = 80}))
      scrollBox.pixel_geometry = function(self)
        local x, y, width, height = Geyser.Container.pixel_geometry(self)
        self.placedAt = x
        return x + 5, y, width - #self.name, height
      end
      scrollBox:reposition()
      assert.are.same({x = 35, y = 40, width = 100 - #"gsbOverridden", height = 80}, geometry("gsbOverridden"))
      assert.are.equal(30, scrollBox.placedAt)
    end)

    it("puts its own origin back to zero after repositioning", function()
      local scrollBox = track(Geyser.ScrollBox:new({name = "gsbOriginKept", x = 30, y = 40, width = 100, height = 80}))
      scrollBox:reposition()
      assert.are.equal(0, scrollBox.get_x())
      assert.are.equal(0, scrollBox.get_y())
      assert.are.same({x = 30, y = 40, width = 100, height = 80}, geometry("gsbOriginKept"))
    end)
  end)

  describe("Geyser.ScrollBox after one of its constraints raised", function()
    for _, dimension in ipairs({"width", "height"}) do
      it("places a child from its own origin after measuring its " .. dimension .. " for a child raised an error", function()
        local size = 300
        local cons = {name = "gsbRaised", x = 100, y = 50, width = 300, height = 200}
        cons[dimension] = function() return size end
        local box = track(Geyser.ScrollBox:new(cons))
        local label = track(Geyser.Label:new({name = "gsbRaisedChild", x = 0, y = 0, width = 50, height = 50}, box))
        size = nil
        local raised = not pcall(label.resize, label, dimension == "width" and "50%" or 50, dimension == "height" and "50%" or 50)
        size = 300
        label:resize(50, 50)
        label:move(10, 20)
        assert.is_true(raised)
        assert.are.same({x = 10, y = 20, width = 50, height = 50}, geometry("gsbRaisedChild"))
        assert.are.equal(0, box:get_x())
        assert.are.equal(0, box:get_y())
      end)
    end

    -- a direct reposition finds the box's getters framed, a move to a new place has just replaced them
    local relayouts = {
      {"reposition", function(box) box:reposition() end, 100},
      {"move", function(box) box:move(120) end, 120},
    }
    for _, entry in ipairs(relayouts) do
      local name, relayout, boxX = entry[1], entry[2], entry[3]
      it("lays a child out inside it after its own " .. name .. " raised an error", function()
        local mainWidth = getMainWindowSize()
        local height = 200.4
        local box = track(Geyser.ScrollBox:new({name = "gsbRaisedBox", x = 100, y = "50.3px", width = "-10px", height = function() return height end}))
        local label = track(Geyser.Label:new({name = "gsbRaisedBoxChild", x = 0, y = 0, width = 50, height = 50}, box))
        height = nil
        local raised = not pcall(relayout, box)
        height = 200.4
        label:resize("100%", "100%")
        label:move(10, 20)
        assert.is_true(raised)
        -- the far edge width still needs the box's real x while the box measures itself,
        -- and the height is the whole pixels its widget spans, 50 to 251
        assert.are.same({x = 10, y = 20, width = mainWidth - 10 - boxX, height = 201}, geometry("gsbRaisedBoxChild"))
      end)
    end

    it("measures itself as before after its own reposition raised an error", function()
      local failing = false
      local box
      local function tripleWidth()
        if failing then
          error("not ready")
        end
        return box and box:get_width() * 3 or 0
      end
      box = track(Geyser.ScrollBox:new({name = "gsbRemeasured", x = "100.4px", y = 0, width = "-10px", height = tripleWidth}))
      local before = box:get_height()
      failing = true
      local raised = not pcall(box.reposition, box)
      failing = false
      assert.is_true(raised)
      assert.are.equal(before, box:get_height())
    end)

    it("raises exactly what the constraint raised", function()
      local raising, failure = false, nil
      local function size()
        if raising then
          error(failure)
        end
        return 100
      end
      local function raisedBy(raise, value)
        raising, failure = true, value
        local ok, raised = pcall(raise)
        raising = false
        assert.is_false(ok)
        return raised
      end
      local box = track(Geyser.ScrollBox:new({name = "gsbRaisedError", x = 0, y = 0, width = size, height = size}))
      local raisers = {
        function() return box.get_width() end,
        function() return box.get_height() end,
        function() box:reposition() end,
      }
      local object = {}
      for _, raise in ipairs(raisers) do
        assert.are.equal(raisedBy(size, "not ready"), raisedBy(raise, "not ready"))
        assert.are.equal(object, raisedBy(raise, object))
        assert.is_nil(raisedBy(raise, nil))
      end
    end)

    it("hands a traceback the frame its constraint raised in", function()
      local failing = false
      local function width()
        if failing then
          error("not ready")
        end
        return 300
      end
      local box = track(Geyser.ScrollBox:new({name = "gsbTraced", x = 0, y = 0, width = width, height = 200}))
      local raisers = {
        function() return box.get_width() end,
        function() box:reposition() end,
      }
      for _, raise in ipairs(raisers) do
        failing = true
        local _, traceback = xpcall(raise, debug.traceback)
        failing = false
        local origin = traceback:match("^(.-): not ready")
        assert.is_truthy(traceback:match("stack traceback:(.*)"):find(origin .. ":", 1, true), traceback)
      end
    end)
  end)

  describe("Geyser.ScrollBox measured inside a coroutine", function()
    it("lets its constraint yield, and lays children out from its own origin afterwards", function()
      local mainWidth = getMainWindowSize()
      local pausing = false
      local function height()
        if pausing then
          coroutine.yield()
        end
        return 200
      end
      local box = track(Geyser.ScrollBox:new({name = "gsbYielding", x = 100, y = 50, width = "-10px", height = height}))
      local label = track(Geyser.Label:new({name = "gsbYieldingChild", x = 0, y = 0, width = 50, height = 50}, box))
      local measures = {
        function() return box.get_height() end,
        function() box:reposition() end,
      }
      for _, measure in ipairs(measures) do
        pausing = true
        local co = coroutine.create(measure)
        local yielded, yieldFailure = coroutine.resume(co)
        pausing = false
        assert.is_true(yielded, yieldFailure)
        assert.are.equal("suspended", coroutine.status(co))
        local finished, failure = coroutine.resume(co)
        assert.is_true(finished, failure)
        assert.are.equal("dead", coroutine.status(co))
      end
      label:resize("100%", 50)
      label:move(10, 20)
      assert.are.same({x = 10, y = 20, width = mainWidth - 110, height = 50}, geometry("gsbYieldingChild"))
    end)

    -- runs on the main thread, or inside a coroutine of its own
    local function runOn(where, work)
      if where == "the main thread" then
        work()
      else
        coroutine.wrap(work)()
      end
    end

    for _, where in ipairs({"the main thread", "a coroutine"}) do
      it("gives its own origin to a coroutine its constraint resumes, laid out on " .. where, function()
        local box
        local seen = {}
        local function width()
          if box then
            seen[#seen + 1] = coroutine.wrap(function() return box:get_x() end)()
          end
          return 300
        end
        box = track(Geyser.ScrollBox:new({name = "gsbNestedRead", x = 100, y = 50, width = width, height = 200}))
        runOn(where, function()
          track(Geyser.Label:new({name = "gsbNestedReadChild", x = 0, y = 0, width = "50%", height = 50}, box))
          box:reposition()
        end)
        assert.is_true(#seen >= 2)
        for _, x in ipairs(seen) do
          assert.are.equal(100, x)
        end
      end)
    end

    for _, dimension in ipairs({"width", "height"}) do
      it("places a child from its own origin inside a coroutine after measuring its " .. dimension, function()
        local box = track(Geyser.ScrollBox:new({name = "gsbLaidOutInCoroutine", x = 100, y = 50, width = "-10px", height = 200}))
        local cons = {name = "gsbLaidOutInCoroutineChild", x = 0, y = 0, width = 50, height = 50}
        cons[dimension] = "50%"
        local label = track(Geyser.Label:new(cons, box))
        box:reposition()
        label:resize(50, 50)
        coroutine.wrap(function() label:move(10, 20) end)()
        assert.are.same({x = 10, y = 20, width = 50, height = 50}, geometry("gsbLaidOutInCoroutineChild"))
      end)
    end

    local raisers = {
      {"its width", "width", function(_, label) label:resize("50%", 50) end},
      {"its height", "height", function(_, label) label:resize(50, "50%") end},
      {"its own reposition", "height", function(box) box:reposition() end},
    }
    for _, raiser in ipairs(raisers) do
      local what, dimension, raise = raiser[1], raiser[2], raiser[3]
      for _, where in ipairs({"the main thread", "another coroutine"}) do
        it("places a child from its own origin on " .. where .. " after " .. what .. " raised inside a coroutine", function()
          local size = 300
          local cons = {name = "gsbRaisedInCoroutine", x = 100, y = 50, width = "-10px", height = 200}
          cons[dimension] = function() return size end
          local box = track(Geyser.ScrollBox:new(cons))
          local label = track(Geyser.Label:new({name = "gsbRaisedInCoroutineChild", x = 0, y = 0, width = 50, height = 50}, box))
          size = nil
          local raising = coroutine.create(function() raise(box, label) end)
          local finished = coroutine.resume(raising)
          size = 300
          local placed, widened
          runOn(where, function()
            label:resize(50, 50)
            label:move(10, 20)
            placed = geometry("gsbRaisedInCoroutineChild")
            label:resize("100%", 50)
            widened = geometry("gsbRaisedInCoroutineChild")
          end)
          assert.is_false(finished)
          assert.are.equal("dead", coroutine.status(raising))
          assert.are.same({x = 10, y = 20, width = 50, height = 50}, placed)
          -- the far edge width needs the box's real x while it measures itself
          local width = dimension == "width" and 300 or getMainWindowSize() - 110
          assert.are.same({x = 10, y = 20, width = width, height = 50}, widened)
        end)
      end
    end

    it("does not keep alive a coroutine a measure raised in", function()
      local size = 300
      local box = track(Geyser.ScrollBox:new({name = "gsbRaisedCoroutineGone", x = 100, y = 50, width = function() return size end, height = 200}))
      local label = track(Geyser.Label:new({name = "gsbRaisedCoroutineGoneChild", x = 0, y = 0, width = 50, height = 50}, box))
      local alive = setmetatable({}, {__mode = "k"})
      local function raiseInCoroutine()
        local co = coroutine.create(function() label:resize("50%", 50) end)
        alive[co] = true
        return coroutine.resume(co)
      end
      size = nil
      local finished = raiseInCoroutine()
      size = 300
      collectgarbage()
      collectgarbage()
      assert.is_false(finished)
      assert.is_nil(next(alive))
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
    -- a console, so Mudlet's scroll bar API cannot reach it: TMainConsole::consoleNamed
    -- finds only the main console and the sub-console map, and a scroll box is in neither.
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
