describe("Tests that the generic mapper only edits the map while mapping", function()
  if type(map) ~= "table" or type(map.require_mapping) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- map.set takes nil, but a table of saved values cannot hold one, hence the list
  local mapperState = {"mapping", "currentRoom", "currentArea"}
  local mainAreaName, otherAreaName = "gmgMain", "gmgElsewhere"
  local emptyAreaName, addedAreaName, renamedAreaName = "gmgEmpty", "gmgAdded", "gmgRenamed"
  local realEcho, realEchoLink = map.echo, _G.echoLink
  local saved, savedFeatures, mainArea, roomA, roomB, echoes

  local function makeRoom(area, name, x)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, area)
    setRoomName(id, name)
    setRoomCoordinates(id, x, 0, 0)
    return id
  end

  before_each(function()
    saved = {}
    for _, key in ipairs(mapperState) do
      saved[key] = map[key]
    end
    savedFeatures = getMapUserData("mapFeatures")

    mainArea = addAreaName(mainAreaName)
    addAreaName(otherAreaName)
    addAreaName(emptyAreaName)
    roomA = makeRoom(mainArea, "Gate room A", 0)
    roomB = makeRoom(mainArea, "Gate room B", 1)
    setExit(roomA, roomB, "e")
    setExit(roomB, roomA, "w")
    addSpecialExit(roomA, roomB, "gmgpreexisting")
    createMapLabel(mainArea, "gmg existing label", 0, 1, 0, 255, 0, 0, 0, 0, 0)
    setMapUserData("mapFeatures", yajl.to_string({gmgpre = ""}))
    setRoomUserData(roomB, "feature-gmgpre", "true")

    map.set("currentRoom", roomA)
    map.set("currentArea", mainArea)

    echoes = {}
    map.echo = function(what, _, err)
      echoes[#echoes + 1] = {text = tostring(what), err = err and true or false}
    end
  end)

  after_each(function()
    map.echo, _G.echoLink = realEcho, realEchoLink
    map.deletingarea = nil
    for _, key in ipairs(mapperState) do
      map.set(key, saved[key])
    end
    setMapUserData("mapFeatures", savedFeatures or "")
    for _, name in ipairs({mainAreaName, otherAreaName, emptyAreaName, addedAreaName, renamedAreaName}) do
      if getAreaTable()[name] then
        deleteArea(name)
      end
    end
  end)

  local function labelCount()
    local labels = getMapLabels(mainArea)
    return type(labels) == "table" and table.size(labels) or 0
  end

  -- each command, and the map state it changes when mapping is on
  local commands = {
    {"rld", function() return "rld " .. roomB end, function() return roomExists(roomB) end},
    {"room coords", function() return "rc v" .. roomB .. " 5 5 0" end, function() return table.concat({getRoomCoordinates(roomB)}, ",") end},
    {"rw", function() return "rw " .. roomB .. " 7" end, function() return getRoomWeight(roomB) end},
    {"rwe", function() return "rwe " .. roomA .. " 5 e" end, function() return getExitWeights(roomA).e end},
    {"rlk", function() return "rlk " .. roomB .. " north" end, function() return getRoomExits(roomA).north end},
    {"urlk", function() return "urlk east" end, function() return getRoomExits(roomA).east end},
    {"rd", function() return "rd " .. roomA .. " e locked" end, function() return getDoors(roomA).e end},
    {"rcc", function() return "rcc X " .. roomA end, function() return getRoomChar(roomA) end},
    {"spe", function() return "spe " .. roomB .. " gmgjump" end, function() return getSpecialExitsSwap(roomA).gmgjump end},
    {"spev", function() return "spev " .. roomB .. " " .. roomA .. " gmgclimb" end, function() return getSpecialExitsSwap(roomB).gmgclimb end},
    {"spe clear", function() return "spe clear " .. roomA end, function() return getSpecialExitsSwap(roomA).gmgpreexisting end},
    {"room area", function() return "room area v" .. roomB .. " " .. otherAreaName end, function() return getRoomArea(roomB) end},
    {"room label", function() return "room label " .. roomA .. " gmg new label" end, labelCount},
    {"area add", function() return "area add " .. addedAreaName end, function() return getAreaTable()[addedAreaName] end},
    {"area delete", function() return "area delete " .. emptyAreaName end, function() return getAreaTable()[emptyAreaName] end},
    {"area rename", function() return "area rename " .. renamedAreaName end, function() return getRoomAreaName(mainArea) end},
    {"feature create", function() return "feature create gmgfeat" end, function() return map.getMapFeatures().gmgfeat end},
    {"rcf", function() return "rcf v" .. roomA .. " gmgpre" end, function() return getRoomUserData(roomA, "feature-gmgpre") end},
    {"rdf", function() return "rdf v" .. roomB .. " gmgpre" end, function() return getRoomUserData(roomB, "feature-gmgpre") end},
    {"feature delete", function() return "feature delete gmgpre" end, function() return map.getMapFeatures().gmgpre end},
  }

  local function refusals()
    local count = 0
    for _, echo in ipairs(echoes) do
      if echo.err and echo.text:find("Not mapping", 1, true) then
        count = count + 1
      end
    end
    return count
  end

  for _, command in ipairs(commands) do
    local name, input, state = unpack(command)

    it(name .. " refuses to edit the map when not mapping", function()
      map.set("mapping", false)
      local before = state()
      expandAlias(input(), false)
      assert.are.same(before, state())
      assert.are.equal(1, refusals())
    end)

    it(name .. " edits the map while mapping", function()
      map.set("mapping", true)
      local before = state()
      expandAlias(input(), false)
      assert.are_not.same(before, state())
      assert.are.equal(0, refusals())
    end)
  end

  -- the mapper reads its globals from _G, which this spec's own environment only shadows
  local function linksFrom(show)
    local links = {}
    _G.echoLink = function(text, command)
      links[text] = command
    end
    show()
    _G.echoLink = realEchoLink
    return links
  end

  local clickables = {
    {"area labels delete link", function() return linksFrom(function() map.areaLabels(mainAreaName, true) end).delete end, 1},
    {"room look clear area labels link", function() return linksFrom(function() map.roomLook(tostring(roomA)) end)["(in area)"] end, 1},
    {"room look clear map labels link", function() return linksFrom(function() map.roomLook(tostring(roomA)) end)["(whole map)"] end, 2},
  }

  for _, clickable in ipairs(clickables) do
    local name, getLink, clicks = unpack(clickable)

    it(name .. " refuses to edit the map when not mapping", function()
      local click = assert(loadstring(assert(getLink(), "no link was shown")))
      map.set("mapping", false)
      for _ = 1, clicks do
        click()
      end
      assert.are.equal(1, labelCount())
      assert.are.equal(clicks, refusals())
    end)
  end

  it("area labels delete link deletes while mapping", function()
    local click = assert(loadstring(assert(linksFrom(function() map.areaLabels(mainAreaName, true) end).delete)))
    map.set("mapping", true)
    click()
    assert.are.equal(0, labelCount())
  end)

  describe("afterwards", function()
    before_each(function()
      map.echo, _G.echoLink = realEcho, realEchoLink
    end)

    it("leaves the mapper's own echo and the global echoLink in place for later specs", function()
      assert.are.equal(realEcho, map.echo)
      assert.are.equal(realEchoLink, _G.echoLink)
    end)
  end)
end)
