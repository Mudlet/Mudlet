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
