describe("test DB functionality", function()
  setup(function()
    -- add in the location of our files
    package.path = "../lua/?.lua;"

    -- add in the location of Lua libraries on Ubuntu 12.04
    package.path = package.path .. "/usr/local/share/lua/5.1/?.lua;/usr/local/share/lua/5.1/?/init.lua;/usr/local/lib/lua/5.1/?.lua;/usr/local/lib/lua/5.1/?/init.lua;/usr/share/lua/5.1/?.lua;/usr/share/lua/5.1/?/init.lua"

    rex = require"rex_pcre"
    require"luasql.sqlite3"
  end)

  it("tests that DB.lua was loaded", function()
    require"DB"
    assert.truthy(db)
  end)
end)
