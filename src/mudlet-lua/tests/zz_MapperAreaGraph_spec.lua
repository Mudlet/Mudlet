-- The zz_ prefix runs this after Mapper_spec: it empties the map and loads saved ones, and puts
-- back the map it found when done.

describe("Tests the area graph and its world layout", function()
  local mapDirectory = getMudletHomeDir() .. "/map"
  local backupPath = mapDirectory .. "/area_graph_spec_backup.dat"
  local savePath = mapDirectory .. "/area_graph_spec_saved.dat"
  local jsonPath = mapDirectory .. "/area_graph_spec_saved.json"

  -- A <-> B by two normal exits, B -> C by a special exit, C -> A going up, D on its own
  local function buildMap()
    local ids = {}
    for _, name in ipairs({"GraphSpecA", "GraphSpecB", "GraphSpecC", "GraphSpecD"}) do
      ids[name] = addAreaName(name)
    end
    local function room(area, x, y)
      local id = createRoomID()
      assert.is_true(addRoom(id))
      assert.is_true(setRoomArea(id, ids[area]))
      setRoomCoordinates(id, x, y, 0)
      return id
    end
    ids.a1 = room("GraphSpecA", 0, 0)
    ids.a2 = room("GraphSpecA", 5, 3)
    ids.b1 = room("GraphSpecB", 0, 0)
    ids.b2 = room("GraphSpecB", 1, 0)
    ids.c1 = room("GraphSpecC", 0, 0)
    ids.d1 = room("GraphSpecD", 0, 0)
    setExit(ids.a1, ids.b1, "e")
    setExit(ids.b1, ids.a1, "w")
    setExit(ids.b1, ids.b2, "e")
    addSpecialExit(ids.b2, ids.c1, "climb rope")
    setExit(ids.c1, ids.a2, "up")
    return ids
  end

  local function linkSet(graph)
    local links = {}
    for _, link in ipairs(graph.links) do
      links[link.from .. "-" .. link.to] = link.exits
    end
    return links
  end

  local function positions(graph)
    local result = {}
    for id, area in pairs(graph.areas) do
      result[id] = {x = area.x, y = area.y, pinned = area.pinned}
    end
    return result
  end

  setup(function()
    os.remove(backupPath)
    os.remove(savePath)
    assert.is_true(saveMap(backupPath), "the map to be replaced could not be saved first")
  end)

  teardown(function()
    assert.is_true(loadMap(backupPath), "the map this block replaced could not be put back")
    openMapWidget("r")
    os.remove(backupPath)
    os.remove(savePath)
    os.remove(jsonPath)
  end)

  before_each(function()
    deleteMap()
  end)

  it("has an area per area and a link per pair of areas joined by exits", function()
    local ids = buildMap()
    local graph = getAreaGraph()
    local A, B, C, D = ids.GraphSpecA, ids.GraphSpecB, ids.GraphSpecC, ids.GraphSpecD

    assert.are.equal("GraphSpecA", graph.areas[A].name)
    assert.are.equal(2, graph.areas[A].rooms)
    assert.are.equal(1, graph.areas[D].rooms)
    -- the default area is only an area of the graph once it has rooms
    assert.is_nil(graph.areas[-1])
    assert.are.same({[A .. "-" .. B] = 2, [B .. "-" .. C] = 1, [A .. "-" .. C] = 1}, linkSet(graph))
    for i = 2, #graph.links do
      local previous, current = graph.links[i - 1], graph.links[i]
      assert.is_true(previous.from < current.from or (previous.from == current.from and previous.to < current.to))
    end

    removeSpecialExit(ids.b2, "climb rope")
    assert.are.same({[A .. "-" .. B] = 2, [A .. "-" .. C] = 1}, linkSet(getAreaGraph()))
  end)

  it("lays the same map out the same way every time", function()
    buildMap()
    local first = positions(getAreaGraph())
    for _, area in pairs(first) do
      assert.is_true(area.x == area.x and area.y == area.y and math.abs(area.x) < 1e6 and math.abs(area.y) < 1e6)
      assert.is_false(area.pinned)
    end

    deleteMap()
    buildMap()
    assert.are.same(first, positions(getAreaGraph()))

    assert.is_true(resetAreaGraphLayout())
    assert.are.same(first, positions(getAreaGraph()))
  end)

  local function assertApart(areas)
    local list = {}
    for _, area in pairs(areas) do
      list[#list + 1] = area
    end
    for i = 1, #list do
      for j = i + 1, #list do
        local distance = math.sqrt((list[i].x - list[j].x) ^ 2 + (list[i].y - list[j].y) ^ 2)
        local needed = list[i].radius + list[j].radius
        assert.is_true(distance >= needed, list[i].name .. " and " .. list[j].name .. " are " .. distance .. " apart, need " .. needed)
      end
    end
  end

  local function addRoomAt(area, x, y)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, area)
    setRoomCoordinates(id, x, y, 0)
    return id
  end

  it("keeps areas apart", function()
    buildMap()
    assertApart(getAreaGraph().areas)
  end)

  it("has nothing for an empty map, and one unlinked area for a map of one", function()
    local graph = getAreaGraph()
    assert.is_nil(next(graph.areas))
    assert.are.same({}, graph.links)

    local only = addAreaName("GraphSpecOnly")
    addRoomAt(only, 0, 0)
    graph = getAreaGraph()
    assert.are.equal(only, (next(graph.areas)))
    assert.is_nil(next(graph.areas, only))
    assert.are.same({}, graph.links)
    assert.are.equal(2, graph.areas[only].radius)
  end)

  it("sizes each area by its rooms, following them as they move", function()
    local ids = buildMap()
    local function expectedRadius(rooms)
      local minX, maxX, minY, maxY = math.huge, -math.huge, math.huge, -math.huge
      for _, room in ipairs(rooms) do
        local x, y = getRoomCoordinates(room)
        minX, maxX, minY, maxY = math.min(minX, x), math.max(maxX, x), math.min(minY, y), math.max(maxY, y)
      end
      return math.max(2, 0.5 * math.sqrt((maxX - minX + 1) ^ 2 + (maxY - minY + 1) ^ 2))
    end
    local function assertRadius(area, rooms)
      local radius = getAreaGraph().areas[area].radius
      assert.is_true(math.abs(radius - expectedRadius(rooms)) < 1e-9, "radius " .. radius .. ", expected " .. expectedRadius(rooms))
    end
    local A = ids.GraphSpecA
    assertRadius(A, {ids.a1, ids.a2})
    setRoomCoordinates(ids.a2, 30, -20, 4)
    assertRadius(A, {ids.a1, ids.a2})
    setRoomArea(ids.a2, ids.GraphSpecD)
    assertRadius(A, {ids.a1})
    assertRadius(ids.GraphSpecD, {ids.d1, ids.a2})
  end)

  it("makes room for an area that grows after it was placed", function()
    local A, B = addAreaName("GraphSpecA"), addAreaName("GraphSpecB")
    local a1, b1 = addRoomAt(A, 0, 0), addRoomAt(B, 0, 0)
    setExit(a1, b1, "e")
    assertApart(getAreaGraph().areas)
    for i = 1, 400 do
      addRoomAt(A, i % 40, math.floor(i / 10))
    end
    assertApart(getAreaGraph().areas)
  end)

  it("shows the default area once it has rooms, keeping its place while it is empty", function()
    local ids = buildMap()
    local loose = createRoomID()
    addRoom(loose)
    local graph = getAreaGraph()
    assert.are.equal(1, graph.areas[-1].rooms)
    setExit(loose, ids.d1, "n")
    assert.are.same(1, linkSet(getAreaGraph())["-1-" .. ids.GraphSpecD])

    assert.is_true(setAreaGraphPosition(-1, 100, 100))
    setRoomArea(loose, ids.GraphSpecD)
    assert.is_nil(getAreaGraph().areas[-1])
    resetRoomArea(loose)
    local area = getAreaGraph().areas[-1]
    assert.are.same({100, 100, true}, {area.x, area.y, area.pinned})
  end)

  it("has an area that so far only has a name", function()
    buildMap()
    getAreaGraph()
    local id = 9999
    assert.is_true(setAreaName(id, "GraphSpecNameOnly"))
    local area = getAreaGraph().areas[id]
    assert.are.same({"GraphSpecNameOnly", 0}, {area.name, area.rooms})
    assert.is_true(setAreaGraphPosition(id, 7, 7))
  end)

  it("keeps an area whose rooms all moved away, with its place", function()
    local ids = buildMap()
    local D = ids.GraphSpecD
    assert.is_true(setAreaGraphPosition(D, -60, 40))
    setRoomArea(ids.d1, ids.GraphSpecA)
    local area = getAreaGraph().areas[D]
    assert.are.same({0, -60, 40, true}, {area.rooms, area.x, area.y, area.pinned})
    setRoomArea(ids.d1, D)
    area = getAreaGraph().areas[D]
    assert.are.same({1, -60, 40, true}, {area.rooms, area.x, area.y, area.pinned})
  end)

  it("leaves placed areas where they are when another area is added", function()
    buildMap()
    local before = positions(getAreaGraph())
    local newArea = addAreaName("GraphSpecE")
    local room = createRoomID()
    addRoom(room)
    setRoomArea(room, newArea)
    local after = positions(getAreaGraph())
    assert.is_not_nil(after[newArea])
    after[newArea] = nil
    assert.are.same(before, after)
  end)

  it("pins an area that is moved", function()
    local ids = buildMap()
    local D = ids.GraphSpecD
    assert.is_true(setAreaGraphPosition(D, 500.5, -300))
    local area = getAreaGraph().areas[D]
    assert.are.same({500.5, -300, true}, {area.x, area.y, area.pinned})

    assert.is_true(resetAreaGraphLayout(true))
    area = getAreaGraph().areas[D]
    assert.are.same({500.5, -300, true}, {area.x, area.y, area.pinned})

    assert.is_true(resetAreaGraphLayout())
    assert.is_false(getAreaGraph().areas[D].pinned)
  end)

  it("moves unpinned areas out of the way of one that is moved onto them", function()
    local ids = buildMap()
    local A = getAreaGraph().areas[ids.GraphSpecA]
    assert.is_true(setAreaGraphPosition(ids.GraphSpecD, A.x, A.y))
    local areas = getAreaGraph().areas
    assert.are.same({A.x, A.y, true}, {areas[ids.GraphSpecD].x, areas[ids.GraphSpecD].y, areas[ids.GraphSpecD].pinned})
    assertApart(areas)
  end)

  it("keeps the layout through a map save and load", function()
    local ids = buildMap()
    assert.is_true(setAreaGraphPosition(ids.GraphSpecB, -42.25, 17))
    local before = positions(getAreaGraph())
    assert.is_true(saveMap(savePath))

    deleteMap()
    assert.is_true(loadMap(savePath))
    assert.are.same(before, positions(getAreaGraph()))
  end)

  it("does not carry a layout over to a JSON map that has none", function()
    openMapWidget()
    buildMap()
    assert.is_true(saveJsonMap(jsonPath))
    local fromFile = positions(getAreaGraph())
    assert.is_true(setAreaGraphPosition(next(fromFile), 500, 500))

    -- the file was written before the move, so it must come back unmoved
    assert.is_true(loadJsonMap(jsonPath))
    assert.are.same(fromFile, positions(getAreaGraph()))
  end)

  it("keeps a new linked area clear of areas it is not linked to", function()
    local ids = buildMap()
    local graph = getAreaGraph()
    local D = ids.GraphSpecD
    -- surround A with pinned unlinked areas, so a new neighbour of A has nowhere free close by
    local ax, ay = graph.areas[ids.GraphSpecA].x, graph.areas[ids.GraphSpecA].y
    for k = 0, 7 do
      local ring = addAreaName("GraphSpecRing" .. k)
      local room = createRoomID()
      addRoom(room)
      setRoomArea(room, ring)
      assert.is_true(setAreaGraphPosition(ring, ax + 8 * math.cos(k * math.pi / 4), ay + 8 * math.sin(k * math.pi / 4)))
    end
    local newArea = addAreaName("GraphSpecE")
    local room = createRoomID()
    addRoom(room)
    setRoomArea(room, newArea)
    addSpecialExit(room, ids.a1, "go")

    graph = getAreaGraph()
    local placed = graph.areas[newArea]
    for id, area in pairs(graph.areas) do
      if id ~= newArea and id ~= D then
        local distance = math.sqrt((area.x - placed.x) ^ 2 + (area.y - placed.y) ^ 2)
        assert.is_true(distance >= 4, area.name .. " is " .. distance .. " from the new area")
      end
    end
  end)

  it("lays out afresh an area that takes the id of a deleted one", function()
    local ids = buildMap()
    local D = ids.GraphSpecD
    assert.is_true(setAreaGraphPosition(D, 500, 500))
    assert.is_true(deleteArea(D))
    assert.is_true(setAreaName(D, "GraphSpecReused"))
    local room = createRoomID()
    addRoom(room)
    assert.is_true(setRoomArea(room, D))

    local area = getAreaGraph().areas[D]
    assert.are.equal("GraphSpecReused", area.name)
    assert.is_false(area.pinned)
    assert.is_false(area.x == 500 and area.y == 500)
  end)

  it("sizes an area by the full span of its rooms", function()
    local wide = addAreaName("GraphSpecWide")
    for _, x in ipairs({2147483647, -2147483647}) do
      local room = createRoomID()
      addRoom(room)
      setRoomArea(room, wide)
      setRoomCoordinates(room, x, 0, 0)
    end
    local other = addAreaName("GraphSpecOther")
    local room = createRoomID()
    addRoom(room)
    setRoomArea(room, other)

    local areas = getAreaGraph().areas
    local distance = math.sqrt((areas[wide].x - areas[other].x) ^ 2 + (areas[wide].y - areas[other].y) ^ 2)
    assert.is_true(distance > 2e9, "the areas are only " .. distance .. " apart")
    assert.is_true(setAreaGraphPosition(wide, areas[wide].x, areas[wide].y), "a position the layout gave was refused")
  end)

  it("only gives positions that can be set back", function()
    local ids = buildMap()
    assert.is_true(setAreaGraphPosition(ids.GraphSpecA, 1e15, -1e15))
    local newArea = addAreaName("GraphSpecE")
    local room = createRoomID()
    addRoom(room)
    setRoomArea(room, newArea)
    addSpecialExit(room, ids.a1, "go")
    local lonely = addAreaName("GraphSpecF")
    room = createRoomID()
    addRoom(room)
    setRoomArea(room, lonely)

    local areas = getAreaGraph().areas
    assertApart(areas)
    for _, id in ipairs({newArea, lonely}) do
      assert.is_true(setAreaGraphPosition(id, areas[id].x, areas[id].y), areas[id].name .. " was given " .. areas[id].x .. ", " .. areas[id].y)
    end
  end)

  it("keeps areas apart from one pinned at the limit", function()
    -- Which way a new area is first pushed depends on its id, so try a range of ids at each spot.
    for _, corner in ipairs({{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {0, -1}}) do
      for padding = 0, 39 do
        deleteMap()
        local pinned = addAreaName("GraphSpecPinned")
        local pinnedRoom = addRoomAt(pinned, 0, 0)
        assert.is_true(setAreaGraphPosition(pinned, corner[1] * 1e15, corner[2] * 1e15))
        for i = 1, padding do
          addAreaName("GraphSpecPadding" .. i)
        end
        local newArea = addAreaName("GraphSpecNew")
        setExit(addRoomAt(newArea, 0, 0), pinnedRoom, "n")
        assertApart(getAreaGraph().areas)
      end
    end
  end)

  it("refuses bad arguments", function()
    local ids = buildMap()
    local ok, message = setAreaGraphPosition(999999, 0, 0)
    assert.is_nil(ok)
    assert.is_truthy(message:find("999999", 1, true))
    ok = setAreaGraphPosition(ids.GraphSpecA, 0 / 0, 0)
    assert.is_nil(ok)
    ok = setAreaGraphPosition(ids.GraphSpecA, math.huge, 0)
    assert.is_nil(ok)
    ok = setAreaGraphPosition(ids.GraphSpecA, 0, 1e300)
    assert.is_nil(ok)
    assert.has_error(function() setAreaGraphPosition("GraphSpecA", 0, 0) end)
    assert.has_error(function() setAreaGraphPosition(ids.GraphSpecA, 0) end)
    assert.has_error(function() resetAreaGraphLayout("yes") end)
  end)
end)
