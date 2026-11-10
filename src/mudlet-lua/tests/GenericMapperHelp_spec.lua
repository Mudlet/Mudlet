describe("Tests the generic mapper's map help command", function()
  if type(map) ~= "table" or type(map.show_help) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  local echoes

  before_each(function()
    echoes = {}
    stub(map, "echo", function(text)
      echoes[#echoes + 1] = text
    end)
  end)

  after_each(function()
    map.echo:revert()
  end)

  it("says there is no help for a topic it does not know", function()
    assert.has_no.errors(function() map.show_help("no such help topic") end)
    assert.are.same({"No help file on that command."}, echoes)
  end)

  it("still shows the help for a topic it knows", function()
    assert.has_no.errors(function() map.show_help("map path") end)
    assert.are.same({}, echoes)
  end)
end)
