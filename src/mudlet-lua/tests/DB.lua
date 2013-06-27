describe("test DB functionality", function()
  setup(function()
    -- add in the location of our files
    package.path = "../lua/?.lua;"

    -- add in the location of Lua libraries on Ubuntu 12.04
    package.path = package.path .. "/usr/local/share/lua/5.1/?.lua;/usr/local/share/lua/5.1/?/init.lua;/usr/local/lib/lua/5.1/?.lua;/usr/local/lib/lua/5.1/?/init.lua;/usr/share/lua/5.1/?.lua;/usr/share/lua/5.1/?/init.lua"

    rex = require"rex_pcre"
    require"luasql.sqlite3"

    -- define some common Mudlet functions essential to operation
    function getMudletHomeDir() return "." end

    require"TableUtils"
  end)

  it("tests that DB.lua was loaded", function()
    require"DB"
    assert.truthy(db)
  end)

  describe("test basic db:create() and db:add()", function()
    setup(function()
      mydb = db:create("people", {
        friends={"name", "city", "notes"},
        enemies={
          name="",
          city="",
          notes="",
          enemied="",
          kills=0,
          _index = { "city" },
          _unique = { "name" },
          _violations = "REPLACE"
        }
      })
    end)

    it("tests basic db:add", function()
      db:add(mydb.enemies, {name="Bob", city="Sacramento"})
      local results = db:fetch(mydb.enemies)
      assert.is_true(#results == 1)
    end)

    it("tests db:add unique replace constraint", function()
      db:add(mydb.enemies, {name="Bob", city="San Francisco"})
      local results = db:fetch(mydb.enemies)
      assert.is_true(#results == 1)
      assert.is_true(results[1].city == "San Francisco")
    end)

    it("tests inserting multiple values at once with db:add", function()
      db:add(mydb.friends,
        {name="Ixokai", city="Magnagora"},
        {name="Vadi", city="New Celest"},
        {name="Heiko", city="Hallifax", notes="The Boss"}
      )

      local results = db:fetch(mydb.friends)
      assert.is_true(#results == 3)

      assert.is_true(results[1].name == "Ixokai")
      assert.is_true(results[1].city == "Magnagora")
      assert.is_true(results[1].notes == "")

      assert.is_true(results[2].name == "Vadi")

      assert.is_true(results[3].name == "Heiko")
    end)

    teardown(function()
      os.remove("Database_people.db")
    end)
  end)
end)
