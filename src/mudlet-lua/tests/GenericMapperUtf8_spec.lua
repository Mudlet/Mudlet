local function upvalue(fn, name)
  local index = 1
  while true do
    local upvalueName, value = debug.getupvalue(fn, index)
    if not upvalueName then
      return nil
    end
    if upvalueName == name then
      return value
    end
    index = index + 1
  end
end

describe("Tests the generic mapper leaves the utf8 library alone", function()
  if type(map) ~= "table" or type(map.showMap) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local borrowed = {"format", "trim", "starts", "split", "ends"}
  local mapperString = upvalue(map.showMap, "string")

  for _, name in ipairs(borrowed) do
    it("does not give utf8 the string library's " .. name, function()
      assert.are_not.equal(_G.string[name], _G.utf8[name])
    end)
  end

  it("keeps a string table of the mapper's own that is not utf8", function()
    assert.is_table(mapperString, "map.showMap no longer uses the mapper's string table - read it from another mapper function")
    assert.is_false(rawequal(mapperString, _G.utf8))
  end)

  it("still hands the mapper the string library's borrowed functions", function()
    assert.is_table(mapperString)
    for _, name in ipairs(borrowed) do
      assert.are.equal(_G.string[name], mapperString[name], name)
    end
  end)

  it("still hands the mapper utf8 for everything else", function()
    assert.is_table(mapperString)
    for _, name in ipairs({"find", "gmatch", "gsub", "len", "lower", "match", "sub", "upper"}) do
      assert.are.equal(_G.utf8[name], mapperString[name], name)
    end
  end)
end)
