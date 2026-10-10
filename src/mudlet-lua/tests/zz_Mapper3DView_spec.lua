-- Busted runs spec files in byte order, so this lowercase name runs last. Parenting
-- a GL view into the main window switches that window to GL composition for the rest
-- of the run, which on the macOS runner's software renderer more than doubled the
-- time of every spec after it. Specs that create a 3D view belong here.

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
