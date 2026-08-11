-- An HBox lays its children out left to right by rewriting their constraints
-- as percentages of the box, so the pixel expectations below are the box
-- geometry divided by the shares each child is entitled to.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.HBox", function()
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

  describe("Geyser.HBox:new/new2", function()
    it("defaults the type to hbox and starts empty", function()
      local box = track(Geyser.HBox:new({name = "ghbNew", x = 0, y = 0, width = 100, height = 100}))
      assert.are.equal("hbox", box.type)
      assert.are.same({}, box.windows)
      assert.is_nil(getWindowGeometry("ghbNew"))
    end)

    it("new2 marks the box as using add2", function()
      local box = track(Geyser.HBox:new2({name = "ghbNew2", x = 0, y = 0, width = 100, height = 100}))
      assert.is_true(box.useAdd2)
      assert.are.equal("hbox", box.type)
    end)

    it("lines the children of a new2 box up the same way new does", function()
      local box = track(Geyser.HBox:new2({name = "ghbNew2Layout", x = 0, y = 0, width = 200, height = 100}))
      -- the children arrive through add2 rather than add
      track(Geyser.Label:new2({name = "ghbNew2A", x = 10, y = 10, width = 300, height = 200}, box))
      track(Geyser.Label:new2({name = "ghbNew2B", x = 10, y = 10, width = 300, height = 200}, box))
      assert.are.same({x = 0, y = 0, width = 100, height = 100}, geometry("ghbNew2A"))
      assert.are.same({x = 100, y = 0, width = 100, height = 100}, geometry("ghbNew2B"))
    end)
  end)

  describe("Geyser.HBox:add/organize", function()
    it("gives a single child the whole box", function()
      local box = track(Geyser.HBox:new({name = "ghbOne", x = 10, y = 20, width = 200, height = 100}))
      track(Geyser.Label:new({name = "ghbOneChild"}, box))
      assert.are.same({x = 10, y = 20, width = 200, height = 100}, geometry("ghbOneChild"))
    end)

    it("splits the box evenly between two children", function()
      local box = track(Geyser.HBox:new({name = "ghbTwo", x = 0, y = 300, width = 200, height = 100}))
      track(Geyser.Label:new({name = "ghbTwoA"}, box))
      track(Geyser.Label:new({name = "ghbTwoB"}, box))
      assert.are.same({x = 0, y = 300, width = 100, height = 100}, geometry("ghbTwoA"))
      assert.are.same({x = 100, y = 300, width = 100, height = 100}, geometry("ghbTwoB"))
    end)

    it("re-splits the box when another child is added", function()
      local box = track(Geyser.HBox:new({name = "ghbFour", x = 0, y = 0, width = 400, height = 40}))
      track(Geyser.Label:new({name = "ghbFourA"}, box))
      track(Geyser.Label:new({name = "ghbFourB"}, box))
      assert.are.equal(200, geometry("ghbFourA").width)
      track(Geyser.Label:new({name = "ghbFourC"}, box))
      track(Geyser.Label:new({name = "ghbFourD"}, box))
      for index, name in ipairs({"ghbFourA", "ghbFourB", "ghbFourC", "ghbFourD"}) do
        assert.are.same({x = (index - 1) * 100, y = 0, width = 100, height = 40}, geometry(name))
      end
    end)

    it("stretches children over the full height of the box", function()
      local box = track(Geyser.HBox:new({name = "ghbTall", x = 0, y = 0, width = 200, height = 120}))
      track(Geyser.Label:new({name = "ghbTallChild", height = 20}, box))
      assert.are.equal("100%", box.windowList.ghbTallChild.height)
      assert.are.equal(120, geometry("ghbTallChild").height)
    end)

    it("keeps a fixed width child at its size and gives the rest away", function()
      local box = track(Geyser.HBox:new({name = "ghbFixed", x = 0, y = 0, width = 300, height = 60}))
      track(Geyser.Label:new({name = "ghbFixedChild", width = 100, h_policy = Geyser.Fixed}, box))
      track(Geyser.Label:new({name = "ghbDynamic"}, box))
      assert.is_true(box.contains_fixed)
      assert.are.same({x = 0, y = 0, width = 100, height = 60}, geometry("ghbFixedChild"))
      local dynamic = geometry("ghbDynamic")
      assert.are.equal(200, dynamic.width)
      -- the dynamic child should start at 100, where the fixed one ends, but
      -- organize() hands out positions as percentages: a third of 300px comes
      -- back as 99.999999999999 and Mudlet truncates it, leaving a one pixel
      -- gap. Pinned so the day the layout is fixed this spec says so.
      assert.are.equal(99, dynamic.x)
    end)

    it("gives a stretch factor its extra share of the width", function()
      local box = track(Geyser.HBox:new({name = "ghbStretch", x = 0, y = 0, width = 400, height = 100}))
      track(Geyser.Label:new({name = "ghbStretchA", h_stretch_factor = 3}, box))
      track(Geyser.Label:new({name = "ghbStretchB"}, box))
      assert.are.same({x = 0, y = 0, width = 300, height = 100}, geometry("ghbStretchA"))
      assert.are.same({x = 300, y = 0, width = 100, height = 100}, geometry("ghbStretchB"))
    end)
  end)

  -- The box lays itself out when a child arrives, and has to do the same when
  -- one leaves: without it the survivors keep the geometry computed for the old
  -- child count and the box is left with a permanent hole. contains_fixed is
  -- false for a box of plain labels, so reposition() does not heal it either.
  describe("Geyser.HBox:remove", function()
    local box

    before_each(function()
      box = track(Geyser.HBox:new({name = "ghbShrink", x = 0, y = 0, width = 600, height = 50}))
      track(Geyser.Label:new({name = "ghbShrinkA"}, box))
      track(Geyser.Label:new({name = "ghbShrinkB"}, box))
    end)

    it("re-splits the row when a child is deleted", function()
      local third = track(Geyser.Label:new({name = "ghbShrinkC"}, box))
      third:delete()
      assert.are.same({"ghbShrinkA", "ghbShrinkB"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 300, height = 50}, geometry("ghbShrinkA"))
      assert.are.same({x = 300, y = 0, width = 300, height = 50}, geometry("ghbShrinkB"))
    end)

    it("re-splits the row when a child is removed by hand", function()
      box:remove(box.windowList.ghbShrinkB)
      assert.are.same({"ghbShrinkA"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 600, height = 50}, geometry("ghbShrinkA"))
    end)

    it("re-splits the row a child left for another container", function()
      local elsewhere = track(Geyser.Container:new({name = "ghbElsewhere", x = 0, y = 100, width = 100, height = 100}))
      box.windowList.ghbShrinkB:changeContainer(elsewhere)
      assert.are.same({"ghbShrinkA"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 600, height = 50}, geometry("ghbShrinkA"))
    end)

    -- an emptied box has no children to divide its width between, and organize()
    -- still has to come through that without raising
    it("survives losing its last child", function()
      assert.has_no.errors(function()
        box:remove(box.windowList.ghbShrinkA)
        box:remove(box.windowList.ghbShrinkB)
      end)
      assert.are.same({}, box.windows)
    end)

    it("holds the layout back while updates are deferred", function()
      local third = track(Geyser.Label:new({name = "ghbShrinkDeferred"}, box))
      local widthOfThree = geometry("ghbShrinkA").width
      box.defer_updates = true
      third:delete()
      assert.are.equal(widthOfThree, geometry("ghbShrinkA").width)
      box.defer_updates = false
      box:reposition()
      assert.are.equal(300, geometry("ghbShrinkA").width)
    end)

    it("deletes a box that still holds children", function()
      assert.has_no.errors(function() box:delete() end)
      assert.is_nil(getWindowGeometry("ghbShrinkA"))
      assert.is_nil(getWindowGeometry("ghbShrinkB"))
      assert.is_nil(Geyser.windowList.ghbShrink)
    end)

    -- one layout pass per child is what makes tearing a box down quadratic. The
    -- fixed child is here because contains_fixed short circuits reposition()'s
    -- check of the deferral, which could let the cost back in for boxes like it
    it("does not lay the row out again for each child it deletes", function()
      track(Geyser.Label:new({name = "ghbShrinkCostFixed", width = 100, h_policy = Geyser.Fixed}, box))
      for i = 1, 3 do
        track(Geyser.Label:new({name = "ghbShrinkCost" .. i}, box))
      end
      local organizes = 0
      local organize = box.organize
      box.organize = function(...)
        organizes = organizes + 1
        return organize(...)
      end
      box:delete()
      assert.are.equal(0, organizes)
      assert.is_nil(getWindowGeometry("ghbShrinkA"))
      assert.is_nil(getWindowGeometry("ghbShrinkCost1"))
    end)

    -- the deferral belongs to the container being deleted, so a box losing a
    -- whole subtree - one that defers itself on the way out - still re-splits
    it("re-splits the row when a nested box of its own is deleted", function()
      local nested = track(Geyser.VBox:new({name = "ghbShrinkNested"}, box))
      track(Geyser.Label:new({name = "ghbShrinkNestedA"}, nested))
      track(Geyser.Label:new({name = "ghbShrinkNestedB"}, nested))
      nested:delete()
      assert.are.same({"ghbShrinkA", "ghbShrinkB"}, box.windows)
      assert.are.same({x = 0, y = 0, width = 300, height = 50}, geometry("ghbShrinkA"))
      assert.are.same({x = 300, y = 0, width = 300, height = 50}, geometry("ghbShrinkB"))
    end)

    -- a cascade that raises leaves the box in the tree, so a box left holding
    -- the cascade's deferral would silently never lay itself out again
    it("stops deferring the row when a child's delete raises", function()
      local doomed = box.windowList.ghbShrinkB
      local ownDelete = rawget(doomed, "delete")
      -- put the real delete back before after_each tries to clean the box up
      finally(function() doomed.delete = ownDelete end)
      doomed.delete = function() error("delete blew up") end
      assert.has_error(function() box:delete() end)
      assert.is_nil(rawget(box, "defer_updates"))
      local organizes = 0
      local organize = box.organize
      box.organize = function(...)
        organizes = organizes + 1
        return organize(...)
      end
      track(Geyser.Label:new({name = "ghbShrinkAfterRaise"}, box))
      assert.is_true(organizes > 0)
    end)

    -- the children the cascade did get through are gone, so the survivors are
    -- left holding the widths worked out for the child count it started with
    it("re-splits the row a half finished delete left behind", function()
      local ownRemove = rawget(box, "remove")
      -- restored so that after_each can still tear the box down
      finally(function() box.remove = ownRemove end)
      -- raising on the second unlink is what puts one child through and strands
      -- the other, whichever order the cascade happens to walk them in
      local removals = 0
      local remove = box.remove
      box.remove = function(...)
        removals = removals + 1
        if removals == 2 then
          error("remove blew up")
        end
        return remove(...)
      end
      assert.has_error(function() box:delete() end)
      assert.are.equal(1, #box.windows)
      assert.are.same({x = 0, y = 0, width = 600, height = 50}, geometry(box.windows[1]))
    end)
  end)

  describe("Geyser.HBox:reposition", function()
    local box

    before_each(function()
      box = track(Geyser.HBox:new({name = "ghbMove", x = 10, y = 20, width = 200, height = 100}))
      track(Geyser.Label:new({name = "ghbMoveA"}, box))
      track(Geyser.Label:new({name = "ghbMoveB"}, box))
    end)

    it("drags the row along when the box moves", function()
      box:move(50, 60)
      assert.are.same({x = 50, y = 60, width = 100, height = 100}, geometry("ghbMoveA"))
      assert.are.same({x = 150, y = 60, width = 100, height = 100}, geometry("ghbMoveB"))
    end)

    it("re-splits the row when the box is resized", function()
      box:resize(100, 50)
      assert.are.same({x = 10, y = 20, width = 50, height = 50}, geometry("ghbMoveA"))
      assert.are.same({x = 60, y = 20, width = 50, height = 50}, geometry("ghbMoveB"))
    end)

    it("keeps a fixed child flush against its neighbour after a resize", function()
      local fixedBox = track(Geyser.HBox:new({name = "ghbFixedMove", x = 0, y = 0, width = 200, height = 60}))
      track(Geyser.Label:new({name = "ghbFixedMoveA", width = 50, h_policy = Geyser.Fixed}, fixedBox))
      track(Geyser.Label:new({name = "ghbFixedMoveB"}, fixedBox))
      fixedBox:resize(250, 60)
      assert.are.same({x = 0, y = 0, width = 50, height = 60}, geometry("ghbFixedMoveA"))
      assert.are.same({x = 50, y = 0, width = 200, height = 60}, geometry("ghbFixedMoveB"))
    end)
  end)

  describe("Geyser.HBox visibility", function()
    it("hides and shows the whole row", function()
      local box = track(Geyser.HBox:new({name = "ghbHide", x = 0, y = 0, width = 100, height = 100}))
      track(Geyser.Label:new({name = "ghbHideA"}, box))
      track(Geyser.Label:new({name = "ghbHideB"}, box))
      box:hide()
      assert.is_false(windowVisible("ghbHideA"))
      assert.is_false(windowVisible("ghbHideB"))
      box:show()
      assert.is_true(windowVisible("ghbHideA"))
      assert.is_true(windowVisible("ghbHideB"))
    end)
  end)
end)
