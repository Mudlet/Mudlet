describe("Tests DB functions", function()
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

  it("Should load DB.lua", function()
    require"DB"
    assert.truthy(db)
  end)

  describe("Tests that DB creation and deletion works", function()
    it("Should create a db", function()
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

    it("should successfully shut down a DB", function()
      db:close()
    end)

    it("Should recreate a DB", function()
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

    it("Should successfully shut down a DB again", function()
      db:close()
    end)
  end)

  describe("Tests basic db:create() and db:add()", function()
    before_each(function()
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

    after_each(function()
      db:close()
    end)

    it("Should add one result to the db", function()
      db:add(mydb.enemies, {name="Bob", city="Sacramento"})
      local results = db:fetch(mydb.enemies)
      assert.is_true(#results == 1)
    end)

    it("Should replace a db entry if add_unique is used and the unique index matches", function()
      db:add(mydb.enemies, {name="Bob", city="Sacramento"})
      db:add(mydb.enemies, {name="Bob", city="San Francisco"})
      local results = db:fetch(mydb.enemies)
      assert.is_true(#results == 1)
      assert.is_true(results[1].city == "San Francisco")
    end)

    it("Should insert multiple values with a single db:add", function()
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
