describe("Tests the generic mapper's area label listing", function()
  if type(map) ~= "table" or type(map.areaLabels) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local areaName = "GenericMapperLabelsSpecArea"

  after_each(function()
    deleteArea(areaName)
  end)

  -- the mapper reads its globals from _G, which this spec's own environment only shadows
  local function deleteLinkFor(area)
    local realEchoLink, realEcho = _G.echoLink, map.echo
    local link
    finally(function()
      _G.echoLink, map.echo = realEchoLink, realEcho
    end)
    _G.echoLink = function(text, command)
      if text == "delete" then
        link = command
      end
    end
    map.echo = function() end
    map.areaLabels(area, true)
    return link
  end

  it("its delete link reports the refusal when the label is already gone", function()
    local area = assert(addAreaName(areaName))
    local labelId = createMapLabel(area, "GenericMapperLabelsSpecLabel", 1, 1, 0, 255, 0, 0, 0, 0, 0)
    assert.is_true(labelId >= 0, "createMapLabel could not make the label the link deletes")
    local link = assert(deleteLinkFor(areaName), "map.areaLabels listed no delete link")
    local click = assert(loadstring(link))

    local echoes = {}
    local realEcho = map.echo
    finally(function()
      map.echo = realEcho
    end)
    map.echo = function(what, _, err)
      echoes[#echoes + 1] = { text = tostring(what), err = err and true or false }
    end

    click()
    click()
    map.echo = realEcho

    assert.are.same({ text = "Deleted label #" .. labelId, err = false }, echoes[1])
    assert.are.same({ text = ("labelID %d does not exist in area with areaID %d"):format(labelId, area), err = true }, echoes[2])
  end)
end)
