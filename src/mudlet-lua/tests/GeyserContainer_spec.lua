-- Geyser resolves its constraints against the live main window, whose size
-- differs between machines, so expectations are computed from
-- getMainWindowSize() at assert time rather than hardcoded. Mudlet truncates
-- the doubles handed to moveWindow()/resizeWindow() (static_cast<int>), which
-- is why the expectations are floored.
local function floor(value)
  return math.floor(value)
end

-- Containers themselves have no Mudlet widget, so geometry is read back from a
-- child label - the widget Geyser actually moves and resizes.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.Container", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  -- A cascading parent delete unlinks its children from its windowList, which
  -- is how we tell an object has already been deleted and must not be deleted
  -- again.
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

  describe("Geyser.Container:new and Geyser.Container:new2", function()
    it("generates a name and defaults the type to container", function()
      local container = track(Geyser.Container:new())
      assert.are.equal("container", container.type)
      assert.is_truthy(container.name:find("^anon_window_%d+$"))
      assert.are.same({}, container.windows)
    end)

    it("registers a top level container with the root Geyser window list", function()
      local container = track(Geyser.Container:new({name = "gcsRegistered"}))
      assert.are.equal(container, Geyser.windowList.gcsRegistered)
      assert.is_truthy(table.index_of(Geyser.windows, "gcsRegistered"))
      assert.are.equal(Geyser, container.container)
      assert.are.equal("main", container.windowname)
    end)

    it("has no Mudlet widget of its own", function()
      track(Geyser.Container:new({name = "gcsNoWidget", x = 0, y = 0, width = 100, height = 100}))
      local result, message = getWindowGeometry("gcsNoWidget")
      assert.is_nil(result)
      assert.is_truthy(message:find("gcsNoWidget", 1, true))
      assert.is_nil(windowType("gcsNoWidget"))
    end)

    it("adds a child to the container given as the second argument", function()
      local parent = track(Geyser.Container:new({name = "gcsParent", x = 0, y = 0, width = 100, height = 100}))
      local child = track(Geyser.Container:new({name = "gcsChild"}, parent))
      assert.are.equal(parent, child.container)
      assert.are.equal(child, parent.windowList.gcsChild)
      assert.are.same({"gcsChild"}, parent.windows)
      assert.is_nil(Geyser.windowList.gcsChild)
    end)

    it("new2 marks the container as using add2", function()
      local container = track(Geyser.Container:new2({name = "gcsAdd2", x = 0, y = 0, width = 50, height = 50}))
      assert.is_true(container.useAdd2)
      assert.is_false(container.hidden)
      assert.is_false(container.auto_hidden)
    end)

    it("raises an error when the container argument is not a container", function()
      local ok, message = pcall(function()
        return Geyser.Container:new({name = "gcsBadParent"}, "notacontainer")
      end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("add", 1, true))
    end)
  end)

  describe("Geyser.calc_constraints and Geyser.set_constraints", function()
    it("places a child at the pixel position it was given", function()
      track(Geyser.Label:new({name = "gcsPixels", x = 12, y = 34, width = 120, height = 56}))
      assert.are.same({x = 12, y = 34, width = 120, height = 56}, geometry("gcsPixels"))
    end)

    it("uses Geyser's defaults when no constraints are given", function()
      track(Geyser.Label:new({name = "gcsDefaults"}))
      assert.are.same({x = 10, y = 10, width = 300, height = 200}, geometry("gcsDefaults"))
    end)

    it("resolves percentages against the main window", function()
      local mainWidth, mainHeight = getMainWindowSize()
      track(Geyser.Label:new({name = "gcsPercent", x = "10%", y = "20%", width = "50%", height = "25%"}))
      assert.are.same({
        x = floor(0.1 * mainWidth),
        y = floor(0.2 * mainHeight),
        width = floor(0.5 * mainWidth),
        height = floor(0.25 * mainHeight),
      }, geometry("gcsPercent"))
    end)

    it("adds a pixel offset to a percentage constraint", function()
      local mainWidth = getMainWindowSize()
      track(Geyser.Label:new({name = "gcsOffset", x = "50%+10", y = 0, width = "10%-5", height = 20}))
      local actual = geometry("gcsOffset")
      assert.are.equal(floor(0.5 * mainWidth + 10), actual.x)
      assert.are.equal(floor(0.1 * mainWidth - 5), actual.width)
    end)

    it("measures negative pixel constraints from the far edge", function()
      local mainWidth, mainHeight = getMainWindowSize()
      track(Geyser.Label:new({name = "gcsNegative", x = "-100px", y = "-50px", width = "100px", height = "50px"}))
      assert.are.same({x = mainWidth - 100, y = mainHeight - 50, width = 100, height = 50}, geometry("gcsNegative"))
    end)

    it("scales character constraints with the font size", function()
      local charWidth, charHeight = calcFontSize(9)
      track(Geyser.Label:new({name = "gcsChars", x = 0, y = 0, width = "10c", height = "2c", fontSize = 9}))
      local actual = geometry("gcsChars")
      assert.are.equal(10 * charWidth, actual.width)
      assert.are.equal(2 * charHeight, actual.height)
    end)

    it("resolves a child's percentages against its container, not the main window", function()
      local container = track(Geyser.Container:new({name = "gcsBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.Label:new({name = "gcsBoxChild", x = "50%", y = "50%", width = "50%", height = "50%"}, container))
      assert.are.same({x = 300, y = 150, width = 200, height = 100}, geometry("gcsBoxChild"))
    end)

    it("resolves percentages through two levels of nesting", function()
      local outer = track(Geyser.Container:new({name = "gcsOuter", x = 100, y = 50, width = 400, height = 200}))
      local middle = track(Geyser.Container:new({name = "gcsMiddle", x = "50%", y = 0, width = "50%", height = "100%"}, outer))
      track(Geyser.Label:new({name = "gcsLeaf", x = "50%", y = "50%", width = "50%", height = "50%"}, middle))
      -- middle spans x 300..500, y 50..250, so the leaf starts halfway into it
      assert.are.same({x = 400, y = 150, width = 100, height = 100}, geometry("gcsLeaf"))
    end)
  end)

  describe("Geyser.Container:move and Geyser.Container:resize", function()
    local container

    before_each(function()
      container = track(Geyser.Container:new({name = "gcsMover", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.Label:new({name = "gcsMoverChild", x = "50%", y = "50%", width = "50%", height = "50%"}, container))
    end)

    it("drags the children of the container along", function()
      container:move(200, 60)
      assert.are.same({x = 400, y = 160, width = 200, height = 100}, geometry("gcsMoverChild"))
    end)

    it("re-resolves the size of percentage children", function()
      container:resize(200, 100)
      assert.are.same({x = 200, y = 100, width = 100, height = 50}, geometry("gcsMoverChild"))
    end)

    it("keeps the constraint that was passed as nil", function()
      container:move(nil, 150)
      -- numeric constraints are normalised to a pixel string as they are applied
      assert.are.equal("100px", container.x)
      assert.are.equal("150px", container.y)
      container:resize(nil, 100)
      assert.are.equal("400px", container.width)
      assert.are.equal("100px", container.height)
      assert.are.same({x = 300, y = 200, width = 200, height = 50}, geometry("gcsMoverChild"))
    end)

    it("moves a label to the pixels it was given", function()
      local label = track(Geyser.Label:new({name = "gcsMoveLabel", x = 0, y = 0, width = 40, height = 20}))
      label:move(70, 80)
      label:resize(90, 30)
      assert.are.same({x = 70, y = 80, width = 90, height = 30}, geometry("gcsMoveLabel"))
    end)
  end)

  describe("Geyser.Container:hide, show, hide_impl and show_impl", function()
    local container

    before_each(function()
      container = track(Geyser.Container:new({name = "gcsVisible", x = 0, y = 0, width = 100, height = 100}))
      track(Geyser.Label:new({name = "gcsVisibleChild"}, container))
    end)

    it("hides and shows the widgets of its children", function()
      assert.is_true(windowVisible("gcsVisibleChild"))
      container:hide()
      assert.is_false(windowVisible("gcsVisibleChild"))
      container:show()
      assert.is_true(windowVisible("gcsVisibleChild"))
    end)

    it("marks children hidden by their container as auto_hidden", function()
      local child = container.windowList.gcsVisibleChild
      container:hide()
      assert.is_true(container.hidden)
      assert.is_false(child.hidden)
      assert.is_true(child.auto_hidden)
      container:show()
      assert.is_false(child.auto_hidden)
    end)

    it("refuses to show a child while its container is hidden", function()
      local child = container.windowList.gcsVisibleChild
      container:hide()
      assert.is_false(child:show())
      assert.is_false(windowVisible("gcsVisibleChild"))
      -- the request is remembered, so the child reappears with its container
      assert.is_false(child.hidden)
      container:show()
      assert.is_true(windowVisible("gcsVisibleChild"))
    end)

    it("hides and shows a label directly", function()
      local label = track(Geyser.Label:new({name = "gcsSelfHide", x = 0, y = 0, width = 30, height = 30}))
      label:hide()
      assert.is_true(label.hidden)
      assert.is_false(windowVisible("gcsSelfHide"))
      label:show()
      assert.is_false(label.hidden)
      assert.is_true(windowVisible("gcsSelfHide"))
    end)
  end)

  describe("Geyser.Container:raise, lower, raiseAll and lowerAll", function()
    -- Mudlet exposes no z-order readback, so these assert the ordering Geyser
    -- keeps in container.windows - the order it replays z-order changes from.
    local container

    before_each(function()
      container = track(Geyser.Container:new({name = "gcsStack", x = 0, y = 0, width = 100, height = 100}))
      track(Geyser.Label:new({name = "gcsStack1"}, container))
      track(Geyser.Label:new({name = "gcsStack2"}, container))
    end)

    it("moves a raised window to the end of its container's ordering", function()
      assert.are.same({"gcsStack1", "gcsStack2"}, container.windows)
      container.windowList.gcsStack1:raise()
      assert.are.same({"gcsStack2", "gcsStack1"}, container.windows)
    end)

    it("leaves the ordering alone when the topmost window is raised", function()
      container.windowList.gcsStack2:raise()
      assert.are.same({"gcsStack1", "gcsStack2"}, container.windows)
    end)

    it("moves a lowered window to the front of the ordering", function()
      container.windowList.gcsStack2:lower()
      assert.are.same({"gcsStack2", "gcsStack1"}, container.windows)
    end)

    it("leaves the ordering alone when the bottom window is lowered", function()
      container.windowList.gcsStack1:lower()
      assert.are.same({"gcsStack1", "gcsStack2"}, container.windows)
    end)

    it("keeps the relative order when raising or lowering the whole container", function()
      container:raiseAll()
      assert.are.same({"gcsStack1", "gcsStack2"}, container.windows)
      assert.is_nil(Geyser.Container.windowTable)
      container:lowerAll()
      assert.are.same({"gcsStack1", "gcsStack2"}, container.windows)
      assert.is_nil(Geyser.Container.windowTable)
    end)
  end)

  describe("Geyser.Container:delete", function()
    it("deletes the widgets of its children", function()
      local container = Geyser.Container:new({name = "gcsDelete", x = 0, y = 0, width = 100, height = 100})
      Geyser.Label:new({name = "gcsDeleteChild"}, container)
      assert.is_not_nil(getWindowGeometry("gcsDeleteChild"))
      container:delete()
      assert.is_nil(getWindowGeometry("gcsDeleteChild"))
      assert.are.same({}, container.windowList)
      assert.are.same({}, container.windows)
    end)

    it("unregisters a top level container from the root Geyser lists", function()
      local container = Geyser.Container:new({name = "gcsDeleteRoot", x = 0, y = 0, width = 10, height = 10})
      container:delete()
      assert.is_nil(Geyser.windowList.gcsDeleteRoot)
      assert.is_nil(table.index_of(Geyser.windows, "gcsDeleteRoot"))
    end)

    it("unregisters a child from its parent", function()
      local container = track(Geyser.Container:new({name = "gcsDeleteParent", x = 0, y = 0, width = 100, height = 100}))
      local child = Geyser.Label:new({name = "gcsDeleteMe"}, container)
      child:delete()
      assert.is_nil(container.windowList.gcsDeleteMe)
      assert.are.same({}, container.windows)
      assert.is_nil(getWindowGeometry("gcsDeleteMe"))
    end)
  end)

  describe("Geyser.Container:setFontSize", function()
    it("rejects a font size that is not a number", function()
      local container = track(Geyser.Container:new({name = "gcsFont", x = 0, y = 0, width = 100, height = 100}))
      local ok, message = pcall(function() container:setFontSize("nope") end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("fontSize must be a number", 1, true))
      assert.are.equal(8, container.fontSize)
    end)

    it("re-resolves character sized constraints for its children", function()
      local container = track(Geyser.Container:new({name = "gcsFontBox", x = 0, y = 0, width = "20c", height = "4c", fontSize = 8}))
      track(Geyser.Label:new({name = "gcsFontChild", x = 0, y = 0, width = "100%", height = "100%"}, container))
      local smallWidth, smallHeight = calcFontSize(8)
      assert.are.same({x = 0, y = 0, width = 20 * smallWidth, height = 4 * smallHeight}, geometry("gcsFontChild"))
      container:setFontSize(16)
      local bigWidth, bigHeight = calcFontSize(16)
      assert.are.equal(16, container.fontSize)
      assert.are.same({x = 0, y = 0, width = 20 * bigWidth, height = 4 * bigHeight}, geometry("gcsFontChild"))
    end)
  end)

  describe("Geyser.Container:calculate_dynamic_window_size", function()
    it("returns the full size when the container holds at most one window", function()
      local container = track(Geyser.Container:new({name = "gcsDyn1", x = 0, y = 0, width = 300, height = 200}))
      assert.are.same({width = 300, height = 200}, container:calculate_dynamic_window_size())
      track(Geyser.Label:new({name = "gcsDyn1Child"}, container))
      assert.are.same({width = 300, height = 200}, container:calculate_dynamic_window_size())
    end)

    it("splits the space between dynamic windows", function()
      local container = track(Geyser.Container:new({name = "gcsDyn2", x = 0, y = 0, width = 300, height = 200}))
      track(Geyser.Label:new({name = "gcsDyn2A"}, container))
      track(Geyser.Label:new({name = "gcsDyn2B"}, container))
      assert.are.same({width = 150, height = 100}, container:calculate_dynamic_window_size())
    end)

    it("leaves fixed windows out of the split", function()
      local container = track(Geyser.Container:new({name = "gcsDyn3", x = 0, y = 0, width = 300, height = 200}))
      track(Geyser.Label:new({
        name = "gcsDyn3Fixed",
        width = 100,
        height = 50,
        h_policy = Geyser.Fixed,
        v_policy = Geyser.Fixed,
      }, container))
      track(Geyser.Label:new({name = "gcsDyn3Dynamic"}, container))
      assert.are.same({width = 200, height = 150}, container:calculate_dynamic_window_size())
    end)

    it("accounts for a stretch factor", function()
      local container = track(Geyser.Container:new({name = "gcsDyn4", x = 0, y = 0, width = 400, height = 400}))
      track(Geyser.Label:new({name = "gcsDyn4A", v_stretch_factor = 3}, container))
      track(Geyser.Label:new({name = "gcsDyn4B"}, container))
      -- the stretch factor counts as three shares against one, so a share is a quarter
      assert.are.equal(100, container:calculate_dynamic_window_size().height)
    end)
  end)

  describe("Geyser.Container:flash", function()
    it("puts a flash label over the container's geometry", function()
      local container = track(Geyser.Container:new({name = "gcsFlash", x = 20, y = 30, width = 80, height = 40}))
      container:flash(0.1)
      assert.are.same({x = 20, y = 30, width = 80, height = 40}, geometry("gcsFlash_dimensions_flash"))
      assert.is_true(windowVisible("gcsFlash_dimensions_flash"))
      -- the flash label belongs to no Geyser container, so remove it by hand
      deleteLabel("gcsFlash_dimensions_flash")
    end)

    it("creates nothing when told not to flash", function()
      local container = track(Geyser.Container:new({name = "gcsNoFlash", x = 20, y = 30, width = 80, height = 40}))
      container:flash(0.1, false)
      assert.is_nil(getWindowGeometry("gcsNoFlash_dimensions_flash"))
    end)
  end)

  describe("Geyser:add, Geyser:base_add and Geyser:add2", function()
    it("tracks an added window once, even when it is added twice", function()
      local container = track(Geyser.Container:new({name = "gcsAdd", x = 0, y = 0, width = 100, height = 100}))
      local label = track(Geyser.Label:new({name = "gcsAdded"}, container))
      container:add(label)
      assert.are.same({"gcsAdded"}, container.windows)
      assert.are.equal(label, container.windowList.gcsAdded)
    end)

    it("takes a window away from its previous container", function()
      local first = track(Geyser.Container:new({name = "gcsAddFrom", x = 0, y = 0, width = 100, height = 100}))
      local second = track(Geyser.Container:new({name = "gcsAddTo", x = 200, y = 0, width = 100, height = 100}))
      local label = track(Geyser.Label:new({name = "gcsAddMoved", x = 0, y = 0, width = "100%", height = "100%"}, first))
      second:add(label)
      assert.is_nil(first.windowList.gcsAddMoved)
      assert.are.same({}, first.windows)
      assert.are.equal(label, second.windowList.gcsAddMoved)
      assert.are.equal(200, geometry("gcsAddMoved").x)
    end)

    it("keeps a new child of a hidden add2 container hidden", function()
      local container = track(Geyser.Container:new2({name = "gcsAdd2Box", x = 0, y = 0, width = 100, height = 100}))
      container:hide()
      local label = track(Geyser.Label:new2({name = "gcsAdd2Child"}, container))
      assert.is_true(label.auto_hidden)
      assert.is_false(windowVisible("gcsAdd2Child"))
      container:show()
      assert.is_true(windowVisible("gcsAdd2Child"))
    end)
  end)

  describe("Geyser:remove", function()
    it("drops the window from both of the container's lists", function()
      local container = track(Geyser.Container:new({name = "gcsRemove", x = 0, y = 0, width = 100, height = 100}))
      local label = track(Geyser.Label:new({name = "gcsRemoved"}, container))
      container:remove(label)
      assert.is_nil(container.windowList.gcsRemoved)
      assert.are.same({}, container.windows)
      -- removing only unhooks the bookkeeping, the widget stays alive
      assert.is_not_nil(getWindowGeometry("gcsRemoved"))
      deleteLabel("gcsRemoved")
    end)
  end)

  describe("Geyser:changeContainer", function()
    local from, to

    before_each(function()
      from = track(Geyser.Container:new({name = "gcsFrom", x = 0, y = 0, width = 200, height = 200}))
      to = track(Geyser.Container:new({name = "gcsTo", x = 300, y = 100, width = 200, height = 200}))
    end)

    it("re-resolves the window's constraints against its new container", function()
      local label = track(Geyser.Label:new({name = "gcsChanging", x = "50%", y = "50%", width = "50%", height = "50%"}, from))
      assert.are.same({x = 100, y = 100, width = 100, height = 100}, geometry("gcsChanging"))
      label:changeContainer(to)
      assert.are.equal(to, label.container)
      assert.is_nil(from.windowList.gcsChanging)
      assert.are.same({x = 400, y = 200, width = 100, height = 100}, geometry("gcsChanging"))
    end)

    it("returns nil and a message when the window is already in that container", function()
      local label = track(Geyser.Label:new({name = "gcsSameContainer"}, from))
      local result, message = label:changeContainer(from)
      assert.is_nil(result)
      assert.is_truthy(message:find("already in this container", 1, true))
    end)

    it("returns nil and a message for something that is not a container", function()
      local label = track(Geyser.Label:new({name = "gcsBadContainer"}, from))
      local result, message = label:changeContainer("notacontainer")
      assert.is_nil(result)
      assert.are.equal("didn't get a valid container", message)
      assert.are.equal(from, label.container)
      local nilResult, nilMessage = label:changeContainer(nil)
      assert.is_nil(nilResult)
      assert.are.equal("didn't get a valid container", nilMessage)
    end)

    it("refuses to put a container inside itself", function()
      local result, message = from:changeContainer(from)
      assert.is_nil(result)
      assert.are.equal("didn't get a valid container", message)
    end)

    it("moves a window back to the root window when passed \"main\"", function()
      local label = track(Geyser.Label:new({name = "gcsBackToMain", x = "50%", y = 0, width = 10, height = 10}, from))
      label:changeContainer("main")
      assert.are.equal(Geyser, label.container)
      assert.are.equal(floor(0.5 * getMainWindowSize()), geometry("gcsBackToMain").x)
    end)
  end)

  describe("Geyser:begin_update, Geyser:end_update and Geyser:reposition", function()
    it("toggles the deferred update flag", function()
      Geyser:begin_update()
      assert.is_true(Geyser.defer_updates)
      Geyser:end_update()
      assert.is_false(Geyser.defer_updates)
    end)
  end)

  describe("GeyserReposition", function()
    it("restores geometry that was changed behind Geyser's back", function()
      local mainWidth, mainHeight = getMainWindowSize()
      track(Geyser.Label:new({name = "gcsReposition", x = 10, y = 10, width = 100, height = 50}))
      moveWindow("gcsReposition", 400, 400)
      resizeWindow("gcsReposition", 20, 20)
      assert.are.same({x = 400, y = 400, width = 20, height = 20}, geometry("gcsReposition"))
      GeyserReposition("sysWindowResizeEvent", mainWidth, mainHeight)
      assert.are.same({x = 10, y = 10, width = 100, height = 50}, geometry("gcsReposition"))
    end)

    it("ignores events that are not window resizes", function()
      track(Geyser.Label:new({name = "gcsNoReposition", x = 10, y = 10, width = 100, height = 50}))
      moveWindow("gcsNoReposition", 300, 300)
      GeyserReposition("sysSomeOtherEvent", 100, 100)
      assert.are.equal(300, geometry("gcsNoReposition").x)
    end)
  end)

  describe("Geyser.nameGen", function()
    it("hands out a new name every time", function()
      local first = Geyser.nameGen()
      local second = Geyser.nameGen()
      assert.are_not.equal(first, second)
      assert.is_truthy(first:find("^anon_window_%d+$"))
    end)

    it("uses the type it is given in the name", function()
      assert.is_truthy(Geyser.nameGen("gauge"):find("^anon_gauge_%d+$"))
    end)
  end)

  describe("Geyser.copyTable", function()
    it("copies the entries of a table", function()
      local source = {name = "x", width = "10px"}
      local copy = Geyser.copyTable(source)
      assert.are.same(source, copy)
      copy.name = "y"
      assert.are.equal("x", source.name)
    end)

    it("shares nested tables that do not ask to be cloned", function()
      local nested = {1, 2}
      local copy = Geyser.copyTable({nested = nested})
      assert.are.equal(nested, copy.nested)
    end)

    it("clones a nested table that provides __clone", function()
      local nested = {__clone = function() return {cloned = true} end}
      local copy = Geyser.copyTable({nested = nested})
      assert.are_not.equal(nested, copy.nested)
      assert.is_true(copy.nested.cloned)
    end)

    it("returns an empty table for nil", function()
      assert.are.same({}, Geyser.copyTable(nil))
    end)
  end)

  describe("Geyser.hideAll and Geyser.showAll", function()
    it("only touches windows of the type it is given", function()
      -- a private type keeps the sweep away from widgets other specs own
      local mine = track(Geyser.Container:new({name = "gcsSweep", type = "gcsprobe", x = 0, y = 0, width = 50, height = 50}))
      track(Geyser.Label:new({name = "gcsSweepChild"}, mine))
      track(Geyser.Label:new({name = "gcsUnswept", x = 0, y = 0, width = 20, height = 20}))
      Geyser.hideAll("gcsprobe")
      assert.is_false(windowVisible("gcsSweepChild"))
      assert.is_true(windowVisible("gcsUnswept"))
      Geyser.showAll("gcsprobe")
      assert.is_true(windowVisible("gcsSweepChild"))
      assert.is_true(windowVisible("gcsUnswept"))
      assert.is_not_nil(mine.windowList.gcsSweepChild)
    end)
  end)

  describe("Geyser reuses a name that is already taken", function()
    it("replaces the tracked window without duplicating the ordering entry", function()
      local first = track(Geyser.Label:new({name = "gcsDuplicate", x = 0, y = 0, width = 30, height = 30}))
      local windowCount = #Geyser.windows
      local second = track(Geyser.Label:new({name = "gcsDuplicate", x = 5, y = 5, width = 60, height = 60}))
      assert.are.equal(windowCount, #Geyser.windows)
      assert.are.equal(second, Geyser.windowList.gcsDuplicate)
      assert.are.same({x = 5, y = 5, width = 60, height = 60}, geometry("gcsDuplicate"))
      -- both objects drive the same widget, which is why reusing a name is a trap
      first:move(11, 12)
      assert.are.same({x = 11, y = 12, width = 30, height = 30}, geometry("gcsDuplicate"))
    end)
  end)
end)
