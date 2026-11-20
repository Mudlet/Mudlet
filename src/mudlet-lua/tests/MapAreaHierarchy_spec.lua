describe("Tests nesting areas under parent areas", function()
  local continent, zone, subzone, other
  local names = {"HierarchySpecContinent", "HierarchySpecZone", "HierarchySpecSubzone", "HierarchySpecOther"}

  local function contains(list, value)
    for _, item in ipairs(list) do
      if item == value then
        return true
      end
    end
    return false
  end

  setup(function()
    continent = addAreaName(names[1])
    zone = addAreaName(names[2])
    subzone = addAreaName(names[3])
    other = addAreaName(names[4])
  end)

  teardown(function()
    for _, name in ipairs(names) do
      deleteArea(name)
    end
  end)

  before_each(function()
    for _, id in ipairs({continent, zone, subzone, other}) do
      setAreaParent(id, nil)
    end
  end)

  describe("setAreaParent and getAreaParent", function()
    it("leaves a new area at the top level", function()
      assert.is_nil(getAreaParent(zone))
    end)

    it("nests an area and reads its parent back", function()
      assert.is_true(setAreaParent(zone, continent))
      assert.are.equal(continent, getAreaParent(zone))
    end)

    it("moves an area back to the top level for a nil parent", function()
      setAreaParent(zone, continent)
      assert.is_true(setAreaParent(zone, nil))
      assert.is_nil(getAreaParent(zone))
    end)

    it("refuses to make an area its own parent", function()
      local ok, message = setAreaParent(zone, zone)
      assert.is_nil(ok)
      assert.are.equal("an area cannot be its own parent", message)
      assert.is_nil(getAreaParent(zone))
    end)

    it("refuses a parent that is one of the area's own descendants", function()
      setAreaParent(zone, continent)
      setAreaParent(subzone, zone)
      local ok, message = setAreaParent(continent, subzone)
      assert.is_nil(ok)
      assert.is_truthy(message:find("is inside", 1, true))
      assert.is_nil(getAreaParent(continent))
    end)

    it("refuses an area or a parent that does not exist", function()
      local ok, message = setAreaParent(990000101, continent)
      assert.is_nil(ok)
      assert.is_string(message)
      ok, message = setAreaParent(zone, 990000102)
      assert.is_nil(ok)
      assert.is_string(message)
      assert.is_nil(getAreaParent(zone))
      ok, message = getAreaParent(990000103)
      assert.is_nil(ok)
      assert.is_string(message)
    end)

    it("refuses the default area as a parent", function()
      local ok, message = setAreaParent(zone, -1)
      assert.is_nil(ok)
      assert.are.equal("the default area cannot have other areas nested in it", message)
      assert.is_nil(getAreaParent(zone))
    end)

    it("raises an error for arguments that are not area IDs", function()
      assert.has_error(function() setAreaParent("zone", continent) end)
      assert.has_error(function() setAreaParent(zone, "continent") end)
      assert.has_error(function() getAreaParent("zone") end)
    end)

    it("nests an area that so far only has a name, and nests under one", function()
      local namedOnly = 990000104
      assert.is_true(setAreaName(namedOnly, "HierarchySpecNamedOnly"))
      finally(function() deleteArea("HierarchySpecNamedOnly") end)

      assert.is_true(setAreaParent(namedOnly, continent))
      assert.are.equal(continent, getAreaParent(namedOnly))
      assert.is_true(setAreaParent(namedOnly, nil))
      assert.is_true(setAreaParent(zone, namedOnly))
      assert.are.equal(namedOnly, getAreaParent(zone))
    end)
  end)

  describe("getAreaChildren", function()
    it("lists an area's children in order, or its whole subtree", function()
      setAreaParent(subzone, zone)
      setAreaParent(other, continent)
      setAreaParent(zone, continent)
      assert.are.same({zone, other}, getAreaChildren(continent))
      assert.are.same({zone, subzone, other}, getAreaChildren(continent, true))
      assert.are.same({}, getAreaChildren(subzone))
    end)

    it("lists the top level areas for a nil area", function()
      setAreaParent(zone, continent)
      local topLevel = getAreaChildren(nil)
      assert.is_true(contains(topLevel, continent))
      assert.is_true(contains(topLevel, other))
      assert.is_false(contains(topLevel, zone))
    end)
  end)

  describe("sysAreaParentChanged", function()
    it("is raised once per change, with the old and the new parent, and not for a parent the area already has", function()
      local seen = {}
      local handler = registerAnonymousEventHandler("sysAreaParentChanged", function(_, areaId, oldParent, newParent)
        seen[#seen + 1] = {areaId, oldParent, newParent}
      end)
      finally(function() killAnonymousEventHandler(handler) end)

      setAreaParent(zone, continent)
      setAreaParent(zone, continent)
      setAreaParent(zone, other)
      setAreaParent(zone, nil)

      assert.are.same({{zone, nil, continent}, {zone, continent, other}, {zone, other, nil}}, seen)
    end)
  end)

  describe("deleting an area that has children", function()
    it("moves its children up to its own parent", function()
      local middle = addAreaName("HierarchySpecMiddle")
      setAreaParent(middle, continent)
      setAreaParent(zone, middle)
      setAreaParent(subzone, middle)

      assert.is_true(deleteArea("HierarchySpecMiddle"))

      assert.are.equal(continent, getAreaParent(zone))
      assert.are.equal(continent, getAreaParent(subzone))
    end)

    it("raises sysAreaParentChanged for each child it moves up, from itself to its own parent", function()
      local middle = addAreaName("HierarchySpecMiddle")
      setAreaParent(middle, continent)
      setAreaParent(zone, middle)
      setAreaParent(subzone, middle)
      local seen = {}
      local handler = registerAnonymousEventHandler("sysAreaParentChanged", function(_, areaId, oldParent, newParent)
        seen[#seen + 1] = {areaId, oldParent, newParent}
      end)
      finally(function() killAnonymousEventHandler(handler) end)

      deleteArea("HierarchySpecMiddle")

      assert.are.same({{zone, middle, continent}, {subzone, middle, continent}}, seen)
    end)
  end)
end)

describe("Tests that nested areas survive saving and loading the map", function()
  local backupPath = getMudletHomeDir() .. "/hierarchy_spec_backup.dat"
  local datPath = getMudletHomeDir() .. "/hierarchy_spec.dat"
  local jsonPath = getMudletHomeDir() .. "/hierarchy_spec.json"

  setup(function()
    -- loading replaces the whole map, so the one the rest of the suite uses is put back afterwards
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this block replaced could not be put back")
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(datPath)
    os.remove(jsonPath)
  end)

  local function nestTwoAreas()
    local parent = addAreaName("HierarchySpecSavedParent")
    local child = addAreaName("HierarchySpecSavedChild")
    assert.is_true(setAreaParent(child, parent))
    return parent, child
  end

  it("keeps the parent through saveMap and loadMap", function()
    local parent, child = nestTwoAreas()
    assert.is_true(saveMap(datPath))
    assert.is_true(setAreaParent(child, nil))

    assert.is_true(loadMap(datPath))

    assert.are.equal(parent, getAreaParent(child))
  end)

  it("keeps the parent through saveJsonMap and loadJsonMap", function()
    local child = getAreaTable()["HierarchySpecSavedChild"]
    local parent = getAreaTable()["HierarchySpecSavedParent"]
    if not child then
      parent, child = nestTwoAreas()
    end
    assert.is_true(saveJsonMap(jsonPath))
    assert.is_true(setAreaParent(child, nil))

    assert.is_true(loadJsonMap(jsonPath))

    assert.are.equal(parent, getAreaParent(child))
  end)

  it("stops reporting children moved by a delete once a handler has loaded another map", function()
    local parent = addAreaName("HierarchySpecReplacedParent")
    local first = addAreaName("HierarchySpecReplacedFirst")
    local second = addAreaName("HierarchySpecReplacedSecond")
    assert.is_true(setAreaParent(first, parent))
    assert.is_true(setAreaParent(second, parent))
    assert.is_true(saveJsonMap(jsonPath))
    local loaded, movedAfterLoading = nil, {}
    local handler = registerAnonymousEventHandler("sysAreaParentChanged", function(_, areaId, oldParent)
      if loaded == nil then
        loaded = loadJsonMap(jsonPath)
      elseif oldParent == parent then
        movedAfterLoading[#movedAfterLoading + 1] = areaId
      end
    end)
    finally(function() killAnonymousEventHandler(handler) end)

    deleteArea(parent)

    assert.is_true(loaded)
    assert.are.same({}, movedAfterLoading)
    -- the map loaded is the one saved, with the parent still in it
    assert.are.equal(parent, getAreaParent(second))
  end)
end)
