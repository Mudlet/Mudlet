-- Geyser.Mapper drives Mudlet's one map per profile. The dockable map widget
-- has no window name of its own, so windowType/getWindowGeometry cannot see it;
-- what is observable is the Geyser object's resolved constraints plus
-- closeMapWidget(), which reports "map widget already closed" when the widget
-- is not on screen and so doubles as a visibility probe.
--
-- Every mapper here is created with embedded = false or a dock position, which
-- is the map widget rather than a mapper drawn into the main console: see the
-- pending below for why the embedded form cannot be exercised in this suite.
--
-- Host::closeMapWidget() hides the dock widget and records the close, so the map
-- window functions answer as they do for a profile that never opened one; what
-- it does not do is destroy the dock. busted runs its files in sorted order, so
-- this file opens the widget ahead of Mapper_spec.lua and its opening block
-- therefore keeps its "there is no map widget" premise but loses its "registered
-- before the widget was opened" one. Nothing there fails, but the deferred
-- registration path that block meant to cover is no longer reached from here on.
-- Restoring it means moving it into an earlier sorting file of its own.

-- Reports whether the map widget is currently on screen, without leaving it in
-- a different state than it was found in.
local function mapWidgetVisible()
  local closed = closeMapWidget()
  if closed then
    assert.is_true(openMapWidget(), "could not put the map widget back after probing it")
    return true
  end
  return false
end

describe("Tests functionality of Geyser.Mapper", function()
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

  -- The map widget outlives every mapper object, so put it away again rather
  -- than leaving it over the specs that run after this file.
  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
    closeMapWidget()
  end)

  describe("Geyser.Mapper:new/new2", function()
    it("registers a mapper that has no addressable widget of its own", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpNew", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.are.equal("mapper", mapper.type)
      assert.are.equal(mapper, Geyser.windowList.gmpNew)
      -- the map lives in a dock widget Mudlet does not name, so the window
      -- getters cannot reach it
      assert.is_nil(windowType("gmpNew"))
      local found, message = getWindowGeometry("gmpNew")
      assert.is_nil(found)
      assert.is_truthy(message:find("gmpNew", 1, true))
    end)

    it("opens the map widget", function()
      track(Geyser.Mapper:new({name = "gmpOpen", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.is_true(mapWidgetVisible())
    end)

    it("resolves its constraints like any other Geyser window", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpPixels", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.are.equal(10, mapper:get_x())
      assert.are.equal(20, mapper:get_y())
      assert.are.equal(300, mapper:get_width())
      assert.are.equal(200, mapper:get_height())
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gmpBox", x = 100, y = 50, width = 400, height = 200}))
      local mapper = track(Geyser.Mapper:new({name = "gmpInBox", x = "25%", y = "50%", width = "50%", height = "50%", embedded = false}, container))
      assert.are.equal(200, mapper:get_x())
      assert.are.equal(150, mapper:get_y())
      assert.are.equal(200, mapper:get_width())
      assert.are.equal(100, mapper:get_height())
    end)

    it("treats a mapper given a dock position as not embedded", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpDocked", x = 0, y = 0, width = 200, height = 150, dockPosition = "right"}))
      assert.is_false(mapper.embedded)
      assert.are.equal("right", mapper.dockPosition)
    end)

    it("shortens a floating dock position to f", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpFloating", x = 0, y = 0, width = 200, height = 150, dockPosition = "floating"}))
      assert.is_false(mapper.embedded)
      assert.are.equal("f", mapper.dockPosition)
    end)

    it("new2 marks the mapper as using add2", function()
      local mapper = track(Geyser.Mapper:new2({name = "gmpNew2", x = 0, y = 0, width = 200, height = 150, embedded = false}))
      assert.is_true(mapper.useAdd2)
      assert.are.equal("mapper", mapper.type)
    end)
  end)

  describe("Geyser.Mapper:move/resize", function()
    local mapper

    before_each(function()
      mapper = track(Geyser.Mapper:new({name = "gmpMove", x = 10, y = 20, width = 300, height = 200, embedded = false}))
    end)

    it("takes the new constraints and resolves them", function()
      mapper:move(60, 70)
      assert.are.equal("60px", mapper.x)
      assert.are.equal("70px", mapper.y)
      assert.are.equal(60, mapper:get_x())
      assert.are.equal(70, mapper:get_y())
      mapper:resize(150, 100)
      assert.are.equal("150px", mapper.width)
      assert.are.equal(150, mapper:get_width())
      assert.are.equal(100, mapper:get_height())
    end)

    it("refuses to move or resize while it is hidden", function()
      mapper:hide()
      mapper:move(200, 210)
      mapper:resize(50, 60)
      assert.are.equal("10px", mapper.x)
      assert.are.equal("20px", mapper.y)
      assert.are.equal(10, mapper:get_x())
      assert.are.equal(300, mapper:get_width())
      assert.are.equal(200, mapper:get_height())
    end)
  end)

  describe("Geyser.Mapper:hide/show", function()
    it("closes and reopens the map widget", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpHide", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.is_true(mapWidgetVisible())
      mapper:hide()
      assert.is_true(mapper.hidden)
      assert.is_false(mapWidgetVisible())
      mapper:show()
      assert.is_false(mapper.hidden)
      assert.is_true(mapWidgetVisible())
    end)

    it("reports that a closed map widget is already closed", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpClosed", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      mapper:hide()
      local closed, message = closeMapWidget()
      assert.is_nil(closed)
      assert.is_truthy(message:find("already closed", 1, true))
    end)
  end)

  describe("Geyser.Mapper:setDockPosition", function()
    it("puts a closed map widget back on screen", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpDockOpen", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      mapper:hide()
      assert.is_false(mapWidgetVisible())
      assert.is_true(mapper:setDockPosition("f"))
      assert.is_true(mapWidgetVisible())
    end)

    it("refuses a dock position that is not one of the five", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpDockBad", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      local result, message = mapper:setDockPosition("nonsense")
      assert.is_nil(result)
      assert.is_string(message)
      assert.is_truthy(message:find("not available", 1, true))
      -- refusing is not fatal, the widget stays where it was
      assert.is_true(mapWidgetVisible())
    end)
  end)

  describe("Geyser.Mapper:reposition", function()
    it("leaves the map widget alone when the main window is resized", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpReposition", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      local mainWidth, mainHeight = getMainWindowSize()
      -- a mapper has no window for moveWindow/resizeWindow to act on, which is
      -- why it overrides reposition to do nothing unless it is embedded. This
      -- is a regression guard: the constraints cannot move today, so the
      -- load-bearing assertion is that the widget is still on screen after.
      GeyserReposition("sysWindowResizeEvent", mainWidth, mainHeight)
      assert.are.equal(10, mapper:get_x())
      assert.are.equal(20, mapper:get_y())
      assert.are.equal(300, mapper:get_width())
      assert.are.equal(200, mapper:get_height())
      assert.is_true(mapWidgetVisible())
    end)
  end)

  describe("Geyser.Mapper:setTitle/resetTitle", function()
    it("remembers the title it was given, and empties it again on reset", function()
      local mapper = track(Geyser.Mapper:new({
        name = "gmpTitle",
        x = 10, y = 20, width = 300, height = 200,
        embedded = false,
        titleText = "My map",
      }))
      assert.are.equal("My map", mapper.titleText)
      -- setMapWindowTitle answers nil and a message rather than raising when
      -- there is no map window, so the return value is what says the title
      -- reached one; without it titleText alone would look right regardless
      assert.is_true(mapper:setTitle("Renamed"))
      assert.are.equal("Renamed", mapper.titleText)
      assert.is_true(mapper:resetTitle())
      assert.are.equal("", mapper.titleText)
    end)

    it("puts the title on the map window while it is on screen", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpLiveTitle", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      -- an untitled mapper resets the map window as it is built, so it starts
      -- on the generated default
      local generated = getMapWindowTitle()
      assert.is_truthy(generated:find(getProfileName(), 1, true))
      assert.is_true(mapper:setTitle("On screen"))
      assert.are.equal("On screen", getMapWindowTitle())
      assert.is_true(mapper:resetTitle())
      assert.are.equal(generated, getMapWindowTitle())
    end)

    it("titles the map window it opens with the title it was constructed with", function()
      -- nil means no map window is open, so the one the constructor opens below
      -- is the one the title is read back from
      assert.is_nil(getMapWindowTitle())
      track(Geyser.Mapper:new({
        name = "gmpBornTitled",
        x = 10, y = 20, width = 300, height = 200,
        embedded = false,
        titleText = "Titled at birth",
      }))
      assert.are.equal("Titled at birth", getMapWindowTitle())
    end)

    it("applies a title that was set while the mapper was hidden", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpHiddenTitle", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      mapper:hide()
      -- the map widget is off screen, so setMapWindowTitle has nothing to retitle
      assert.is_nil(mapper:setTitle("Named while hidden"))
      assert.are.equal("Named while hidden", mapper.titleText)
      mapper:show()
      assert.are.equal("Named while hidden", getMapWindowTitle())
    end)

    it("applies a reset that was made while the mapper was hidden", function()
      local mapper = track(Geyser.Mapper:new({
        name = "gmpHiddenReset",
        x = 10, y = 20, width = 300, height = 200,
        embedded = false,
        titleText = "Named before hiding",
      }))
      mapper:hide()
      assert.is_nil(mapper:resetTitle())
      assert.are.equal("", mapper.titleText)
      mapper:show()
      -- an empty titleText is a reset, not "no title to apply", so the map
      -- window has to come back with its generated default rather than the
      -- title it carried before the hide
      local title = getMapWindowTitle()
      assert.is_truthy(title:find(getProfileName(), 1, true))
      assert.are_not.equal("Named before hiding", title)
    end)

    it("does not overwrite a directly set title when a mapper is shown", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpNoClobber", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      mapper:hide()
      openMapWidget()
      assert.is_true(setMapWindowTitle("Set without Geyser"))
      mapper:show()
      -- this mapper never had a title of its own, so showing it has nothing to
      -- reapply and must leave the map window titled as it was found
      assert.are.equal("Set without Geyser", getMapWindowTitle())
      resetMapWindowTitle()
    end)

    it("starts with an empty title when it was not given one", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpNoTitle", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.are.equal("", mapper.titleText)
    end)
  end)

  pending("Geyser.Mapper:setDockPosition docks the map widget against the edge it names - which edge it ended up on is not readable from Lua")

  pending("Geyser.Mapper:raise/lower stack the map against the other windows - Mudlet exposes no z-order readback")

  -- An embedded mapper and the dockable map widget are mutually exclusive for
  -- the life of a profile (TMainConsole::createMapper and Host::openMapWidget
  -- each refuse when the other one exists), and neither can be destroyed once
  -- made. Creating an embedded mapper here would take the map widget away from
  -- Mapper_spec for the rest of the run.
  pending("Geyser.Mapper embedded in the main console - an embedded mapper cannot be undone, so it cannot be created inside this suite")

  describe("Geyser.Mapper:type_delete", function()
    it("closes the map widget and unregisters the mapper", function()
      local mapper = track(Geyser.Mapper:new({name = "gmpDelete", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      assert.is_true(mapWidgetVisible())
      mapper:delete()
      assert.is_nil(Geyser.windowList.gmpDelete)
      assert.is_false(mapWidgetVisible())
    end)

    it("goes away with the container it was put in", function()
      local container = track(Geyser.Container:new({name = "gmpOuter", x = 0, y = 0, width = 300, height = 200}))
      local mapper = track(Geyser.Mapper:new({name = "gmpNested", x = 0, y = 0, width = "100%", height = "100%", embedded = false}, container))
      assert.are.equal(mapper, container.windowList.gmpNested)
      assert.is_true(mapWidgetVisible())
      container:delete()
      -- the widget closing is what says the cascade reached the mapper:
      -- Geyser.Container:delete empties its own windowList either way
      assert.is_false(mapWidgetVisible())
      assert.is_nil(container.windowList.gmpNested)
    end)

    -- A profile has one map, so two Geyser.Mapper objects are two handles on
    -- the same widget: deleting either one closes it under the other. That is
    -- worth pinning down, because it is the trap a second mapper walks into.
    it("closes the one shared map widget even when another mapper still holds it", function()
      local first = track(Geyser.Mapper:new({name = "gmpShared", x = 10, y = 20, width = 300, height = 200, embedded = false}))
      local second = track(Geyser.Mapper:new({name = "gmpSharing", x = 0, y = 0, width = 200, height = 150, embedded = false}))
      assert.is_true(mapWidgetVisible())
      second:delete()
      assert.is_false(mapWidgetVisible())
      -- the surviving mapper is untouched as an object, and can reopen the map
      assert.are.equal(first, Geyser.windowList.gmpShared)
      first:show()
      assert.is_true(mapWidgetVisible())
    end)
  end)
end)
