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

  -- The box lays itself out when a child arrives, and has to do the same when
  -- one leaves: without it the survivors keep the geometry computed for the old
  -- child count and the box is left with a permanent hole. contains_fixed is
  -- false for a box of plain labels, so reposition() does not heal it either.
  describe("Geyser.VBox:remove", function()
    local box

    before_each(function()
      box = track(Geyser.VBox:new({name = "gvbShrink", x = 0, y = 0, width = 50, height = 600}))
      track(Geyser.Label:new({name = "gvbShrinkA"}, box))
      track(Geyser.Label:new({name = "gvbShrinkB"}, box))
    end)

    it("re-stacks the column when a child is deleted", function()
      local third = track(Geyser.Label:new({name = "gvbShrinkC"}, box))
      third:delete()
      assert.are.same({"gvbShrinkA", "gvbShrinkB"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 50, height = 300}, geometry("gvbShrinkA"))
      assert.are.same({x = 0, y = 300, width = 50, height = 300}, geometry("gvbShrinkB"))
    end)

    it("re-stacks the column when a child is removed by hand", function()
      box:remove(box.windowList.gvbShrinkB)
      assert.are.same({"gvbShrinkA"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 50, height = 600}, geometry("gvbShrinkA"))
    end)

    it("re-stacks the column a child left for another container", function()
      local elsewhere = track(Geyser.Container:new({name = "gvbElsewhere", x = 100, y = 0, width = 100, height = 100}))
      box.windowList.gvbShrinkB:changeContainer(elsewhere)
      assert.are.same({"gvbShrinkA"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 50, height = 600}, geometry("gvbShrinkA"))
    end)

    -- an emptied box has no children to divide its height between, and
    -- organize() still has to come through that without raising
    it("survives losing its last child", function()
      assert.has_no.errors(function()
        box:remove(box.windowList.gvbShrinkA)
        box:remove(box.windowList.gvbShrinkB)
      end)
      assert.are.same({}, box.windows)
    end)

    it("holds the layout back while updates are deferred", function()
      local third = track(Geyser.Label:new({name = "gvbShrinkDeferred"}, box))
      local heightOfThree = geometry("gvbShrinkA").height
      box.defer_updates = true
      third:delete()
      assert.are.equal(heightOfThree, geometry("gvbShrinkA").height)
      box.defer_updates = false
      box:reposition()
      assert.are.equal(300, geometry("gvbShrinkA").height)
    end)

    it("deletes a box that still holds children", function()
      assert.has_no.errors(function() box:delete() end)
      assert.is_nil(getWindowGeometry("gvbShrinkA"))
      assert.is_nil(getWindowGeometry("gvbShrinkB"))
      assert.is_nil(Geyser.windowList.gvbShrink)
    end)

    -- one layout pass per child is what makes tearing a box down quadratic. The
    -- fixed child is here because contains_fixed short circuits reposition()'s
    -- check of the deferral, which could let the cost back in for boxes like it
    it("does not stack the column again for each child it deletes", function()
      track(Geyser.Label:new({name = "gvbShrinkCostFixed", height = 100, v_policy = Geyser.Fixed}, box))
      for i = 1, 3 do
        track(Geyser.Label:new({name = "gvbShrinkCost" .. i}, box))
      end
      local organizes = 0
      local organize = box.organize
      box.organize = function(...)
        organizes = organizes + 1
        return organize(...)
      end
      box:delete()
      assert.are.equal(0, organizes)
      assert.is_nil(getWindowGeometry("gvbShrinkA"))
      assert.is_nil(getWindowGeometry("gvbShrinkCost1"))
    end)

    -- the deferral belongs to the container being deleted, so a box losing a
    -- whole subtree - one that defers itself on the way out - still re-stacks
    it("re-stacks the column when a nested box of its own is deleted", function()
      local nested = track(Geyser.HBox:new({name = "gvbShrinkNested"}, box))
      track(Geyser.Label:new({name = "gvbShrinkNestedA"}, nested))
      track(Geyser.Label:new({name = "gvbShrinkNestedB"}, nested))
      nested:delete()
      assert.are.same({"gvbShrinkA", "gvbShrinkB"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 50, height = 300}, geometry("gvbShrinkA"))
      assert.are.same({x = 0, y = 300, width = 50, height = 300}, geometry("gvbShrinkB"))
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

    it("starts the whole stack hidden when the box constraints ask for it", function()
      local box = track(Geyser.VBox:new({name = "gvbHiddenNew", x = 0, y = 0, width = 100, height = 100, hidden = true}))
      track(Geyser.Label:new({name = "gvbHiddenNewA"}, box))
      track(Geyser.Label:new({name = "gvbHiddenNewB"}, box))
      assert.is_true(box.hidden)
      assert.is_false(windowVisible("gvbHiddenNewA"))
      assert.is_false(windowVisible("gvbHiddenNewB"))
      box:show()
      assert.is_true(windowVisible("gvbHiddenNewA"))
      assert.is_true(windowVisible("gvbHiddenNewB"))
    end)
  end)
end)
