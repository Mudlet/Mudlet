-- Busted sorts spec files by name, so the zz_ prefix runs this last. Docking a GL view
-- switches the main window to GL composition for the rest of the run, which is slow on
-- software GL; specs that create a 3D view belong here.

-- Floating and redocking give the modern 3D view a new GL context, and switching
-- renderers replaces the widget; neither may crash or leave the 3D view hidden
describe("Tests the modern 3D mapper surviving context rebuilds", function()
  it("floats, redocks and switches back to the classic view", function()
    if type(setMapPerspective) ~= "function" then
      pending("needs a Mudlet built with the 3D mapper")
      return
    end
    -- a GL context leaks in the leak-checking job's GL driver, as Media_spec explains
    if (os.getenv("ASAN_OPTIONS") or ""):find("detect_leaks=1", 1, true) then
      pending("a 3D view's GL context leaks in this job's GL driver")
      return
    end
    local wasModern = getConfig("experiment.3dmap.modernmapper")
    local was3d = getConfig("show3dMapView")
    finally(function()
      setConfig("experiment.3dmap.modernmapper", wasModern == true)
      setConfig("show3dMapView", was3d == true)
    end)
    assert.is_true(openMapWidget())
    -- with the experiment on, the mapper builds its 3D view as the modern widget
    assert.is_true(setConfig("experiment.3dmap.modernmapper", true))
    assert.is_true(getConfig("experiment.3dmap.modernmapper"))
    assert.is_true(setConfig("show3dMapView", true))
    assert.is_true(getConfig("show3dMapView"))
    assert.is_true(openMapWidget("f"))
    assert.is_true(getConfig("show3dMapView"))
    assert.is_true(openMapWidget("r"))
    assert.is_true(getConfig("show3dMapView"))
    assert.is_true(setConfig("experiment.3dmap.modernmapper", false))
    assert.is_false(getConfig("experiment.3dmap.modernmapper"))
    assert.is_true(getConfig("show3dMapView"))
  end)
end)

describe("Tests the 3D map perspective functions with the modern 3D view showing", function()
  if not setMapPerspective then
    pending("setMapPerspective and shiftMapPerspective need a Mudlet built with the 3D mapper")
    return
  end

  local areaId, rA1

  setup(function()
    assert.is_true(openMapWidget())
    areaId = addAreaName("Mapper3DViewSpecArea")
    rA1 = createRoomID()
    addRoom(rA1)
    setRoomArea(rA1, areaId)
    setRoomCoordinates(rA1, 0, 0, 0)
  end)

  teardown(function()
    deleteRoom(rA1)
    deleteArea("Mapper3DViewSpecArea")
  end)

  it("both are accepted while the modern 3D view shows, and refused for the classic view", function()
    -- a GL context leaks in the leak-checking job's GL driver, as Media_spec explains
    if (os.getenv("ASAN_OPTIONS") or ""):find("detect_leaks=1", 1, true) then
      pending("a 3D view's GL context leaks in this job's GL driver")
      return
    end
    local wasModern = getConfig("experiment.3dmap.modernmapper")
    local shownArea = getRoomArea(rA1)
    local wasGrid = getGridMode(shownArea)
    finally(function()
      setGridMode(shownArea, wasGrid)
      setConfig("show3dMapView", false)
      setConfig("experiment.3dmap.modernmapper", wasModern == true)
    end)
    assert.is_true(centerview(rA1))
    assert.is_true(setGridMode(shownArea, false))
    assert.is_true(setConfig("experiment.3dmap.modernmapper", true))
    assert.is_true(setConfig("show3dMapView", true))
    assert.is_true(getConfig("show3dMapView"))
    local ok, err = setMapPerspective(2, 45, 270)
    -- only a missing GL context, as on Windows CI, excuses the modern view; any other reason it could not start,
    -- such as shaders that will not compile, is a failure
    local noContext = ok == nil and err:find("could not create an OpenGL 3.3 context", 1, true) ~= nil
    if noContext then
      local shifted, shiftErr = shiftMapPerspective(0, 30, 0)
      assert.is_nil(shifted)
      assert.are.equal(err, shiftErr)
    else
      assert.is_true(ok, err)
      assert.is_true(shiftMapPerspective(0, 30, 0))

      -- the 3D view draws a grid-mode area looking straight down, whatever a script asks; only the distance applies
      assert.is_true(setGridMode(shownArea, true))
      ok, err = setMapPerspective(2, 45, 270)
      assert.is_nil(ok)
      assert.are.equal("the area shown in the 3D view is in grid mode, which always looks straight down, so only the distance was applied", err)
      ok, err = shiftMapPerspective(0, 30, 0)
      assert.is_nil(ok)
      assert.are.equal("the area shown in the 3D view is in grid mode, which always looks straight down", err)
      assert.is_true(setGridMode(shownArea, false))
    end

    assert.is_true(setConfig("experiment.3dmap.modernmapper", false))
    assert.is_true(getConfig("show3dMapView"))
    ok, err = setMapPerspective(2, 45, 270)
    assert.is_nil(ok)
    assert.are.equal("the modern 3D map view is not showing", err)
    ok, err = shiftMapPerspective(0, 30, 0)
    assert.is_nil(ok)
    assert.are.equal("the modern 3D map view is not showing", err)
    if noContext then
      pending("no GL context here, so only the refusals were checked")
    end
  end)

  it("setMapZoom keeps its contract while the modern 3D view shows", function()
    -- a GL context leaks in the leak-checking job's GL driver, as Media_spec explains
    if (os.getenv("ASAN_OPTIONS") or ""):find("detect_leaks=1", 1, true) then
      pending("a 3D view's GL context leaks in this job's GL driver")
      return
    end
    local wasModern = getConfig("experiment.3dmap.modernmapper")
    local before = getMapZoom(areaId)
    finally(function()
      setConfig("show3dMapView", false)
      setConfig("experiment.3dmap.modernmapper", wasModern == true)
      setMapZoom(before, areaId)
    end)
    -- the 3D view zooms only for the area it shows
    assert.is_true(centerview(rA1))
    assert.is_true(setConfig("experiment.3dmap.modernmapper", true))
    assert.is_true(setConfig("show3dMapView", true))
    assert.is_true(getConfig("show3dMapView"))

    assert.is_true(setMapZoom(15, areaId))
    assert.are.equal(15, getMapZoom(areaId))
    local ok, err = setMapZoom(2.9, areaId)
    assert.is_nil(ok)
    assert.is_truthy(err:find("it must be at least 3", 1, true), err)
    ok, err = setMapZoom(0 / 0, areaId)
    assert.is_nil(ok)
    assert.is_truthy(err:find("it must be a finite number", 1, true), err)
    assert.are.equal(15, getMapZoom(areaId))
  end)

  -- Paints the room drawing code with a real GL context, which no C++ test does in CI. It can only
  -- check that the view started, not that a frame was drawn or what it shows. Pending under leak
  -- detection and where there is no GL context, so in CI only macOS paints, without sanitizers;
  -- Windows CI has no OpenGL 3.3
  it("paints rooms with a symbol, a highlight and a border, in and out of grid mode", function()
    if (os.getenv("ASAN_OPTIONS") or ""):find("detect_leaks=1", 1, true) then
      pending("a 3D view's GL context leaks in this job's GL driver")
      return
    end
    local wasModern = getConfig("experiment.3dmap.modernmapper")
    local wasGrid = getGridMode(areaId)
    local neighbour = createRoomID()
    finally(function()
      setConfig("show3dMapView", false)
      setConfig("experiment.3dmap.modernmapper", wasModern == true)
      setGridMode(areaId, wasGrid)
      unHighlightRoom(rA1)
      setRoomChar(rA1, "")
      deleteRoom(neighbour)
    end)
    assert.is_true(addRoom(neighbour))
    setRoomArea(neighbour, areaId)
    setRoomCoordinates(neighbour, 1, 0, 0)
    assert.is_true(setRoomChar(neighbour, "$"))
    assert.is_true(setRoomChar(rA1, "@"))
    assert.is_true(highlightRoom(rA1, 255, 0, 0, 0, 0, 255, 1.5, 200, 100))
    assert.is_true(setRoomBorderColor(neighbour, 0, 255, 0))
    assert.is_true(setRoomBorderThickness(neighbour, 3))
    assert.is_true(centerview(rA1))
    assert.is_true(setGridMode(areaId, false))
    assert.is_true(setConfig("experiment.3dmap.modernmapper", true))
    assert.is_true(setConfig("show3dMapView", true))
    pumpEvents(200)
    -- A view that could not start refuses camera moves rather than painting; only a missing GL context excuses it
    local ok, err = shiftMapPerspective(0, 0, 0)
    if ok == nil and err:find("could not create an OpenGL 3.3 context", 1, true) then
      pending(err)
      return
    end
    assert.is_true(ok, err)

    assert.is_true(setGridMode(areaId, true))
    pumpEvents(200)
    ok, err = shiftMapPerspective(0, 0, 0)
    assert.is_nil(ok)
    assert.are.equal("the area shown in the 3D view is in grid mode, which always looks straight down", err)
  end)
end)
