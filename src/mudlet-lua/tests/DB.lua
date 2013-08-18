describe("Tests DB.lua functions", function()
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

    it("Should successfully shut down a DB", function()
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

    it("Should create and add a row", function()
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

      db:add(mydb.friends, {name = "test subject", city = "Golden City", notes = "fill in the blanks"})
    end)

    it("Should successfully shut down a DB after data has been added", function()
      db:close()
    end)

    teardown(function()
      os.remove("Database_people.db")
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

    teardown(function()
      os.remove("Database_people.db")
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
  end)

  describe("Tests db:fetch()'s sorting functionality", function()
    before_each(function()
      mydb = db:create("dslpnp_data", {
        people = {
          name = "",
          race = "",
          class = "",
          level = 0,
          org = "",
          org_type = "",
          status = "",
          keyword = "",
          _index = {"name"},
          _unique = {"keyword"},
          _violations = "REPLACE"
        }
      })
    end)

    after_each(function()
      db:close()
    end)

    teardown(function()
      os.remove("Database_dslpnpdata.db")
    end)

    it("Should sort the fields by level first and then name, both in descending order", function()
      db:add(mydb.people,
              {name="Bob",level=12,class="mage",race = "elf",keyword = "Bob"},
              {name="Bob",level=15,class="warrior",race = "human", keyword = "Bob"},
              {name="Boba",level=15,class="warrior",race = "human", keyword = "Boba"},
              {name="Bobb",level=15,class="warrior",race = "human", keyword = "Bobb"},
              {name="Bobc",level=15,class="warrior",race = "human", keyword = "Bobc"},
              {name="Frank",level=31,class="cleric",race = "ogre", keyword = "Frank"})

      local results = db:fetch(mydb.people,nil,{mydb.people.level, mydb.people.name}, true)
      assert.is_true(#results == 5)
      assert.is_true(results[1].name == "Frank" and results[1].level == 31)
      assert.is_true(results[2].name == "Bobc" and results[2].level == 15)
      assert.is_true(results[3].name == "Bobb" and results[3].level == 15)
      assert.is_true(results[#results].name == "Bob" and results[#results].level == 15)
    end)
  end)

  describe("Tests db:create() ability to add a new row to an existing database", function()
    before_each(function()
      mydb = db:create("mydb", {
        sheet = {
          row1 = "",
          row2 = 0,
          _index = {"row1"},
          _unique = {"row1"},
          _violations = "REPLACE"
        }
      })
    end)

    after_each(function()
      db:close()
      os.remove("Database_mydb.db")
      mydb = nil
    end)

    it("Should add a column of type number successfully to an empty db", function()
      local newschema = {
        row1 = "",
        row2 = 0,
        row3 = 0,
        _index = {"row1"},
        _unique = {"row1"},
        _violations = "REPLACE"
      }

      mydb = db:create("mydb", { sheet = newschema })
      assert.are.same(db.__schema.mydb.sheet.columns, newschema)
    end)

    it("Should add a column of type string successfully to an empty db", function()
      local newschema = {
        row1 = "",
        row2 = 0,
        row3 = "",
        _index = {"row1"},
        _unique = {"row1"},
        _violations = "REPLACE"
      }

      mydb = db:create("mydb", { sheet = newschema })
      assert.are.same(db.__schema.mydb.sheet.columns, newschema)
    end)

    it("Should add a column successfully to a filled db", function()
      db:add(mydb.sheet, {row1 = "some data"})

      local newschema = {
        row1 = "",
        row2 = 0,
        row3 = "",
        _index = {"row1"},
        _unique = {"row1"},
        _violations = "REPLACE"
      }

      mydb = db:create("mydb", { sheet = newschema })
      assert.are.same(db.__schema.mydb.sheet.columns, newschema)
      local newrow = db:fetch(mydb.sheet)[1]
      assert.is_true(newrow.row1 == "some data")
      assert.is_true(newrow.row3 == "")
    end)
  end)
end)
