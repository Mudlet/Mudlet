-- A VBox stacks its children top to bottom by rewriting their constraints as
-- percentages of the box, so the pixel expectations below are the box geometry
-- divided by the shares each child is entitled to.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.VBox", function()
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

  describe("Geyser.VBox:new/new2", function()
    it("defaults the type to VBox and starts empty", function()
      local box = track(Geyser.VBox:new({name = "gvbNew", x = 0, y = 0, width = 100, height = 100}))
      assert.are.equal("VBox", box.type)
      assert.are.same({}, box.windows)
      -- a box is a container, so it has no widget of its own
      assert.is_nil(getWindowGeometry("gvbNew"))
    end)

    it("new2 marks the box as using add2", function()
      local box = track(Geyser.VBox:new2({name = "gvbNew2", x = 0, y = 0, width = 100, height = 100}))
      assert.is_true(box.useAdd2)
      assert.are.equal("VBox", box.type)
    end)

    it("stacks the children of a new2 box the same way new does", function()
      local box = track(Geyser.VBox:new2({name = "gvbNew2Layout", x = 0, y = 0, width = 200, height = 200}))
      -- the children arrive through add2 rather than add
      track(Geyser.Label:new2({name = "gvbNew2A", x = 10, y = 10, width = 300, height = 200}, box))
      track(Geyser.Label:new2({name = "gvbNew2B", x = 10, y = 10, width = 300, height = 200}, box))
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("gvbNew2A"))
      assert.are.same({x = 0, y = 100, width = 200, height = 100}, geometry("gvbNew2B"))
    end)

    it("lays out a new2 box nested in another new2 box", function()
      local outer = track(Geyser.HBox:new2({name = "gvbNestedOuter", x = 0, y = 0, width = 200, height = 200}))
      local inner = track(Geyser.VBox:new2({name = "gvbNestedInner"}, outer))
      track(Geyser.Label:new2({name = "gvbNestedSibling"}, outer))
      track(Geyser.Label:new2({name = "gvbNestedA"}, inner))
      track(Geyser.Label:new2({name = "gvbNestedB"}, inner))
      -- the inner box takes half the outer one, and splits it between its own two
      assert.are.same({x = 0, y = 0, width = 100, height = 100}, geometry("gvbNestedA"))
      assert.are.same({x = 0, y = 100, width = 100, height = 100}, geometry("gvbNestedB"))
      assert.are.same({x = 100, y = 0, width = 100, height = 200}, geometry("gvbNestedSibling"))
    end)
  end)

  describe("Geyser.VBox:add/organize", function()
    it("gives a single child the whole box", function()
      local box = track(Geyser.VBox:new({name = "gvbOne", x = 10, y = 20, width = 200, height = 100}))
      track(Geyser.Label:new({name = "gvbOneChild"}, box))
      assert.are.same({x = 10, y = 20, width = 200, height = 100}, geometry("gvbOneChild"))
    end)

    it("splits the box evenly between two children", function()
      local box = track(Geyser.VBox:new({name = "gvbTwo", x = 0, y = 0, width = 200, height = 200}))
      track(Geyser.Label:new({name = "gvbTwoA"}, box))
      track(Geyser.Label:new({name = "gvbTwoB"}, box))
      assert.are.same({x = 0, y = 0, width = 200, height = 100}, geometry("gvbTwoA"))
      assert.are.same({x = 0, y = 100, width = 200, height = 100}, geometry("gvbTwoB"))
    end)

    it("re-splits the box when another child is added", function()
      local box = track(Geyser.VBox:new({name = "gvbThree", x = 50, y = 60, width = 100, height = 100}))
      track(Geyser.Label:new({name = "gvbThreeA"}, box))
      track(Geyser.Label:new({name = "gvbThreeB"}, box))
      assert.are.equal(50, geometry("gvbThreeA").height)
      track(Geyser.Label:new({name = "gvbThreeC"}, box))
      -- a third of the box does not divide into whole pixels, and Mudlet
      -- truncates the pixel values it is handed
      for index, name in ipairs({"gvbThreeA", "gvbThreeB", "gvbThreeC"}) do
        local expectedY = math.floor(60 + (index - 1) * 100 / 3)
        assert.are.same({x = 50, y = expectedY, width = 100, height = 33}, geometry(name))
      end
    end)

    it("stretches children over the full width of the box", function()
      local box = track(Geyser.VBox:new({name = "gvbWide", x = 0, y = 0, width = 240, height = 100}))
      track(Geyser.Label:new({name = "gvbWideChild", width = 20}, box))
      assert.are.equal("100%", box.windowList.gvbWideChild.width)
      assert.are.equal(240, geometry("gvbWideChild").width)
    end)

    it("keeps a fixed height child at its size and splits the rest", function()
      local box = track(Geyser.VBox:new({name = "gvbFixed", x = 0, y = 0, width = 200, height = 300}))
      track(Geyser.Label:new({name = "gvbFixedChild", height = 60, v_policy = Geyser.Fixed}, box))
      track(Geyser.Label:new({name = "gvbDynamicA"}, box))
      track(Geyser.Label:new({name = "gvbDynamicB"}, box))
      assert.is_true(box.contains_fixed)
      assert.are.same({x = 0, y = 0, width = 200, height = 60}, geometry("gvbFixedChild"))
      assert.are.same({x = 0, y = 60, width = 200, height = 120}, geometry("gvbDynamicA"))
      assert.are.same({x = 0, y = 180, width = 200, height = 120}, geometry("gvbDynamicB"))
    end)

    it("gives a stretch factor its extra share of the height", function()
      local box = track(Geyser.VBox:new({name = "gvbStretch", x = 0, y = 0, width = 200, height = 400}))
      track(Geyser.Label:new({name = "gvbStretchA", v_stretch_factor = 3}, box))
      track(Geyser.Label:new({name = "gvbStretchB"}, box))
      -- three shares against one out of a four share pool
      assert.are.same({x = 0, y = 0, width = 200, height = 300}, geometry("gvbStretchA"))
      assert.are.same({x = 0, y = 300, width = 200, height = 100}, geometry("gvbStretchB"))
    end)
  end)

  describe("Geyser.VBox:reposition", function()
    local box

    before_each(function()
      box = track(Geyser.VBox:new({name = "gvbMove", x = 10, y = 20, width = 200, height = 200}))
      track(Geyser.Label:new({name = "gvbMoveA"}, box))
      track(Geyser.Label:new({name = "gvbMoveB"}, box))
    end)

    it("drags the stack along when the box moves", function()
      box:move(50, 60)
      assert.are.same({x = 50, y = 60, width = 200, height = 100}, geometry("gvbMoveA"))
      assert.are.same({x = 50, y = 160, width = 200, height = 100}, geometry("gvbMoveB"))
    end)

    it("re-splits the stack when the box is resized", function()
      box:resize(100, 100)
      assert.are.same({x = 10, y = 20, width = 100, height = 50}, geometry("gvbMoveA"))
      assert.are.same({x = 10, y = 70, width = 100, height = 50}, geometry("gvbMoveB"))
    end)

    it("keeps a fixed child flush against its neighbour after a resize", function()
      local fixedBox = track(Geyser.VBox:new({name = "gvbFixedMove", x = 0, y = 0, width = 200, height = 200}))
      track(Geyser.Label:new({name = "gvbFixedMoveA", height = 50, v_policy = Geyser.Fixed}, fixedBox))
      track(Geyser.Label:new({name = "gvbFixedMoveB"}, fixedBox))
      fixedBox:resize(200, 250)
      assert.are.same({x = 0, y = 0, width = 200, height = 50}, geometry("gvbFixedMoveA"))
      assert.are.same({x = 0, y = 50, width = 200, height = 200}, geometry("gvbFixedMoveB"))
    end)
  end)

  describe("Geyser.VBox visibility", function()
    it("hides and shows the whole stack", function()
      local box = track(Geyser.VBox:new({name = "gvbHide", x = 0, y = 0, width = 100, height = 100}))
      track(Geyser.Label:new({name = "gvbHideA"}, box))
      track(Geyser.Label:new({name = "gvbHideB"}, box))
      box:hide()
      assert.is_false(windowVisible("gvbHideA"))
      assert.is_false(windowVisible("gvbHideB"))
      box:show()
      assert.is_true(windowVisible("gvbHideA"))
      assert.is_true(windowVisible("gvbHideB"))
    end)
  end)
end)
