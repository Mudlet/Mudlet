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
    assert.is_true(setMapPerspective(2, 45, 270))
    assert.is_true(shiftMapPerspective(0, 30, 0))

    -- the 3D view draws a grid-mode area looking straight down, whatever a script asks; only the distance applies
    assert.is_true(setGridMode(shownArea, true))
    local ok, err = setMapPerspective(2, 45, 270)
    assert.is_nil(ok)
    assert.are.equal("the area shown in the 3D view is in grid mode, which always looks straight down, so only the distance was applied", err)
    ok, err = shiftMapPerspective(0, 30, 0)
    assert.is_nil(ok)
    assert.are.equal("the area shown in the 3D view is in grid mode, which always looks straight down", err)
    assert.is_true(setGridMode(shownArea, false))

    assert.is_true(setConfig("experiment.3dmap.modernmapper", false))
    assert.is_true(getConfig("show3dMapView"))
    ok, err = setMapPerspective(2, 45, 270)
    assert.is_nil(ok)
    assert.are.equal("the modern 3D map view is not showing", err)
    ok, err = shiftMapPerspective(0, 30, 0)
    assert.is_nil(ok)
    assert.are.equal("the modern 3D map view is not showing", err)
  end)
end)
