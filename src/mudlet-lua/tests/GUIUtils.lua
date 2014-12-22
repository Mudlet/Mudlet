describe("Tests the GUI utilities as far as possible without mudlet", function()

  setup(function()
    -- add in the location of our files
    package.path = "../lua/?.lua;"

    -- add in the location of Lua libraries on Ubuntu 12.04
    package.path = package.path ..
"/usr/local/share/lua/5.1/?.lua;/usr/local/share/lua/5.1/?/init.lua;/usr/local/lib/lua/5.1/?.lua;/usr/local/lib/lua/5.1/?/init.lua;/usr/share/lua/5.1/?.lua;/usr/share/lua/5.1/?/init.lua"

    rex    = require"rex_pcre"

    -- define some common Mudlet functions essential to operation

    -- load rrequired other functions
    require"StringUtils"

    -- load the functions
    require"GUIUtils"
  end)

  describe("Test the operation of the ansi2decho function", function()

    it("Should have loaded the function successfully", function()
      assert.truthy(ansi2decho)
    end)

    it("Should convert the ANSI reset sequence correctly", function()
      local sequence = "\27[0m"
      local expectedResult = "<r>"
      local actualResult = ansi2decho(sequence)
      assert.are.same(expectedResult, actualResult)
    end)

  end)

end)
