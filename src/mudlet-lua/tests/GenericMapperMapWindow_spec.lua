describe("Tests the generic mapper's map window", function()
  if type(map) ~= "table" or type(map.showMap) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- Geyser.Mapper is stubbed because Mudlet has one map per profile: an
  -- embedded mapper would take the map from the dock widget the other mapper
  -- specs rely on, and the real map widget cannot report how it was opened.
  local realNew, realCloseMapWidget, savedWindow, savedDefaults, savedSilentUpdate, savedPrevious, made

  local function stubMapper(cons)
    local mapper = {cons = cons, embedded = cons.embedded, calls = {}}
    for _, method in ipairs({"move", "resize", "show", "hide"}) do
      mapper[method] = function()
        mapper.calls[#mapper.calls + 1] = method
      end
    end
    return mapper
  end

  -- in place, as map.configs.map_window is map.defaults.map_window itself until a configs file exists
  local function restore(window, saved)
    for key in pairs(window) do
      window[key] = nil
    end
    for key, value in pairs(saved) do
      window[key] = value
    end
  end

  before_each(function()
    savedWindow = table.deepcopy(map.configs.map_window)
    savedDefaults = table.deepcopy(map.defaults.map_window)
    savedSilentUpdate = map.silentUpdate
    savedPrevious = Geyser.windowList.my_mapper
    Geyser.windowList.my_mapper = nil
    realNew = Geyser.Mapper.new
    realCloseMapWidget = closeMapWidget
    made = {}
    Geyser.Mapper.new = function(_, cons)
      local mapper = stubMapper(cons)
      made[#made + 1] = mapper
      return mapper
    end
  end)

  after_each(function()
    Geyser.Mapper.new = realNew
    closeMapWidget = realCloseMapWidget
    Geyser.windowList.my_mapper = savedPrevious
    restore(map.defaults.map_window, savedDefaults)
    restore(map.configs.map_window, savedWindow)
    map.silentUpdate = savedSilentUpdate
  end)

  it("embeds the map in the main window by default", function()
    assert.is_true(map.defaults.map_window.embedded)
    map.configs.map_window.embedded = map.defaults.map_window.embedded
    map.configs.map_window.dock = map.defaults.map_window.dock
    map.showMap(true)
    assert.are.equal(1, #made)
    assert.is_true(made[1].cons.embedded)
    assert.is_nil(made[1].cons.dockPosition)
    assert.are.same({"resize", "move", "show"}, made[1].calls)
  end)

  it("opens a floating map window placed by the window configs when not embedded", function()
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "floating"
    map.showMap(true)
    assert.are.equal(1, #made)
    assert.is_false(made[1].cons.embedded)
    assert.are.equal("floating", made[1].cons.dockPosition)
    assert.are.equal(map.configs.map_window.w, made[1].cons.width)
    assert.are.equal(map.configs.map_window.h, made[1].cons.height)
    -- the constructor has placed it already
    assert.are.same({"show"}, made[1].calls)
  end)

  it("docks the map window where the dock config says", function()
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "left"
    map.showMap(true)
    assert.are.equal(1, #made)
    assert.is_false(made[1].cons.embedded)
    assert.are.equal("left", made[1].cons.dockPosition)
    -- moving or resizing the map widget floats it, which would undo the dock
    assert.are.same({"show"}, made[1].calls)
  end)

  it("keeps the map where it was first shown until the profile is opened again", function()
    Geyser.windowList.my_mapper = stubMapper({embedded = true})
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "floating"
    map.showMap(true)
    assert.are.equal(1, #made)
    assert.is_true(made[1].cons.embedded)
    assert.is_nil(made[1].cons.dockPosition)
  end)

  it("hides the map it already has rather than opening another", function()
    local previous = stubMapper({embedded = false})
    Geyser.windowList.my_mapper = previous
    closeMapWidget = function() return true end
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "left"
    map.configs.map_window.shown = true
    map.showMap()
    assert.is_false(map.configs.map_window.shown)
    assert.are.equal(0, #made)
    assert.is_true(previous.hidden)
    assert.are.same({}, previous.calls)
  end)

  it("brings back a map window that was closed from its title bar", function()
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "floating"
    map.showMap(true)
    Geyser.windowList.my_mapper = made[1]
    closeMapWidget = function() return nil, "map widget already closed" end
    map.showMap()
    assert.is_true(map.configs.map_window.shown)
    assert.are.equal(1, #made)
    assert.are.same({"show", "show"}, made[1].calls)
  end)

  it("leaves an open map window where the player put it unless told to replace it", function()
    map.configs.map_window.embedded = false
    map.configs.map_window.dock = "left"
    map.showMap(true)
    Geyser.windowList.my_mapper = made[1]
    map.showMap(true)
    assert.are.equal(1, #made)
    assert.are.same({"show", "show"}, made[1].calls)
    map.configs.map_window.dock = "right"
    map.showMap(true, true)
    assert.are.equal(2, #made)
    assert.are.equal("right", made[2].cons.dockPosition)
  end)

  describe("the map window alias", function()
    local configsPath = getMudletHomeDir():gsub("\\", "/") .. "/map downloads/configs.lua"
    local savedFile, realShowMap, showMapCalls

    before_each(function()
      local file = io.open(configsPath, "rb")
      if file then
        savedFile = file:read("*a")
        file:close()
      else
        savedFile = nil
      end
      -- setConfigs reloads the configs, which reopens the map when it is set to be shown
      realShowMap = map.showMap
      showMapCalls = {}
      map.showMap = function(shown, replace)
        showMapCalls[#showMapCalls + 1] = {shown, replace}
      end
    end)

    after_each(function()
      map.showMap = realShowMap
      if savedFile then
        local file = io.open(configsPath, "wb")
        file:write(savedFile)
        file:close()
      else
        os.remove(configsPath)
      end
    end)

    it("sets embedded off and back on", function()
      expandAlias("map window embedded false", false)
      assert.is_false(map.configs.map_window.embedded)
      expandAlias("map window embedded true", false)
      assert.is_true(map.configs.map_window.embedded)
    end)

    it("toggles embedded when given no value", function()
      map.configs.map_window.embedded = true
      expandAlias("map window embedded", false)
      assert.is_false(map.configs.map_window.embedded)
    end)

    it("reopens an open map window only when its own settings change", function()
      Geyser.windowList.my_mapper = stubMapper({embedded = false})
      map.configs.map_window.shown = true
      expandAlias("map config debug", false)
      expandAlias("map config debug", false)
      assert.are.same({}, showMapCalls)
      expandAlias("map window dock top", false)
      assert.are.same({{true, true}}, showMapCalls)
    end)

    it("applies window settings changed while the map window was hidden when it is next shown", function()
      map.showMap = realShowMap
      Geyser.windowList.my_mapper = stubMapper({embedded = false})
      closeMapWidget = function() return true end
      map.configs.map_window.embedded = false
      map.configs.map_window.dock = "floating"
      map.configs.map_window.shown = false
      expandAlias("map window dock left", false)
      assert.are.equal(0, #made)
      map.showMap()
      assert.are.equal(1, #made)
      assert.are.equal("left", made[1].cons.dockPosition)
      assert.are.same({"show"}, made[1].calls)
      Geyser.windowList.my_mapper = made[1]
      map.showMap()
      map.showMap()
      assert.are.equal(1, #made)
    end)

    it("sets the dock position", function()
      expandAlias("map window dock Right", false)
      assert.are.equal("right", map.configs.map_window.dock)
    end)

    it("refuses a dock position the map window does not have", function()
      map.configs.map_window.dock = "left"
      expandAlias("map window dock centre", false)
      assert.are.equal("left", map.configs.map_window.dock)
      expandAlias("map window dock 1", false)
      assert.are.equal("left", map.configs.map_window.dock)
    end)

    it("refuses an embedded or shown setting that is not true or false", function()
      map.configs.map_window.embedded = false
      expandAlias("map window embedded 0", false)
      assert.is_false(map.configs.map_window.embedded)
      map.configs.map_window.shown = false
      expandAlias("map window shown yes", false)
      assert.is_false(map.configs.map_window.shown)
    end)
  end)
end)
