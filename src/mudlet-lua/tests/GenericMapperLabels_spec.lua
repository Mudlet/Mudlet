describe("Tests the generic mapper's area label listing", function()
  if type(map) ~= "table" or type(map.areaLabels) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local areaName = "GenericMapperLabelsSpecArea"

  local wasMapping

  before_each(function()
    wasMapping = map.mapping
    map.set("mapping", true)
  end)

  after_each(function()
    map.set("mapping", wasMapping)
    deleteArea(areaName)
  end)

  local realEchoLink, realEcho = _G.echoLink, map.echo

  -- busted keeps only the last finally() a test registers, so one restores both stubs
  local function restoreStubs()
    _G.echoLink, map.echo = realEchoLink, realEcho
  end

  -- the mapper reads its globals from _G, which this spec's own environment only shadows
  local function deleteLinkFor(area)
    local link
    _G.echoLink = function(text, command)
      if text == "delete" then
        link = command
      end
    end
    map.echo = function() end
    map.areaLabels(area, true)
    _G.echoLink, map.echo = realEchoLink, realEcho
    return link
  end

  it("its delete link reports the refusal when the label is already gone", function()
    finally(restoreStubs)
    local area = assert(addAreaName(areaName))
    local labelId = createMapLabel(area, "GenericMapperLabelsSpecLabel", 1, 1, 0, 255, 0, 0, 0, 0, 0)
    assert.is_true(labelId >= 0, "createMapLabel could not make the label the link deletes")
    local link = assert(deleteLinkFor(areaName), "map.areaLabels listed no delete link")
    local click = assert(loadstring(link))

    local echoes = {}
    map.echo = function(what, _, err)
      echoes[#echoes + 1] = { text = tostring(what), err = err and true or false }
    end

    click()
    click()
    map.echo = realEcho

    assert.are.same({ text = "Deleted label #" .. labelId, err = false }, echoes[1])
    assert.are.same({ text = ("labelID %d does not exist in area with areaID %d"):format(labelId, area), err = true }, echoes[2])
  end)

  it("leaves the mapper's own echo and the global echoLink in place for later specs", function()
    assert.are.equal(realEcho, map.echo)
    assert.are.equal(realEchoLink, _G.echoLink)
  end)
end)
