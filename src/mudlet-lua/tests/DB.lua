describe("Tests DB.lua functions", function()
  setup(function()
    -- add in the location of our files
    package.path = "../lua/?.lua;"

    -- add in the location of Lua libraries on Ubuntu 12.04
    package.path = package.path .. "/usr/local/share/lua/5.1/?.lua;/usr/local/share/lua/5.1/?/init.lua;/usr/local/lib/lua/5.1/?.lua;/usr/local/lib/lua/5.1/?/init.lua;/usr/share/lua/5.1/?.lua;/usr/share/lua/5.1/?/init.lua"

    luasql = require"luasql.sqlite3"
    rex    = require"rex_pcre"

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
      assert.are.same("some data", newrow.row1)
      assert.are.same("", newrow.row3)
    end)
  end)

  describe("Tests, if options are correctly recognised and applied",
  function()

    before_each(function()
      mydb = db:create("mydb",
        {
          sheet = {
            name = "", id = 0,
            _index = { "name" },
            _unique = { "id" },
            _violations = "FAIL"
          }
        })
    end)


    after_each(function()
      db:close()
      os.remove("Database_mydb.db")
      mydb = nil
    end)

    it("should correctly filter the options on creation.",
      function()

        db:add(mydb.sheet, {id = 0, name = "Bob"})
        local rows = db:fetch(mydb.sheet)

        assert.equals(3, table.size(rows[1])) -- We expect 2 columns plus a
                                              -- _row_id

        assert.are.same({ _row_id = 1, id = 0, name = "Bob" }, rows[1])

      end)

    it("should apply all indexes correctly.",
      function()

        local conn = db.__conn.mydb
        local cur = conn:execute("SELECT * FROM sqlite_master" ..
                                 " WHERE type = 'index'")
        local results = {}

        if cur and cur ~= 0 then
          local row = cur:fetch({}, "a")

          while row do
            results[#results+1] = row
            row = cur:fetch({}, "a")
          end
          cur:close()
        end

        assert.equals(3, #results)

        for _, v in ipairs(results) do

          v.rootpage = nil -- skip the rootpage, as this is nothing we can
                                 -- change

          local expected

          if v.name == "sqlite_autoindex_sheet_1" then
            expected = { type = "index", name = "sqlite_autoindex_sheet_1",
                         tbl_name = "sheet" }
          elseif v.name == "idx_sheet_c_name" then
            expected = { type = "index", name = "idx_sheet_c_name",
                         tbl_name = "sheet",
                         sql = 'CREATE INDEX idx_sheet_c_name ' ..
                               'ON sheet ("name")'
                       }
          elseif v.name == "idx_sheet_c_id" then
            expected = { type = "index", name = "idx_sheet_c_id",
                         tbl_name = "sheet",
                         sql = 'CREATE UNIQUE INDEX idx_sheet_c_id ' ..
                               'ON sheet ("id")'
                       }
          end

          assert.are.same(expected, v)

        end

      end)

  end)

  describe("Tests, if columns are deleted successfully",
  function()

    before_each(function()
      mydb = db:create("mydb",
        {
          sheet = {
            name = "", id = 0, blubb = "",
            _index = { "name" },
            _unique = { "id" },
            _violations = "FAIL"
          }
        })
    end)


    after_each(function()
      db:close()
      os.remove("Database_mydb.db")
      mydb = nil
    end)

    it("should successfully delete columns in an empty table.",
    function()
      mydb = db:create("mydb", { sheet = { name = "", id = 0 }})
      local test = { name = "foo", id = 500 }
      db:add(mydb.sheet, test)
      local res = db:fetch(mydb.sheet)
      assert.are.equal(1, #res)
      res[1]._row_id = nil --we get the row id back, which we don't need
      assert.are.same(test, res[1])
    end)

    it("should successfully delete columns in a non empty table.",
    function()
      local test = { name = "foo", id = 500, blubb = "bar" }
      db:add(mydb.sheet, test)
      mydb = db:create("mydb", { sheet = { name = "", id = 0 }})
      local res = db:fetch(mydb.sheet)
      test.blubb = nil -- we expect the blubb gets deleted
      assert.are.equal(1, #res)
      res[1]._row_id = nil --we get the row id back, which we don't need
      assert.are.same(test, res[1])
    end)

  end)


  describe("Tests that queries by example work",
  function()

    before_each(function()
      mydb = db:create("mydb",
        {
          sheet = {
            name = "", id = 0, city = "",
            _index = { "name" },
            _unique = { "id" },
            _violations = "FAIL"
          }
        })
      test_data = {
        {name="Ixokai", city="Magnagora", id=1},
        {name="Vadi", city="New Celest", id=2},
        {name="Heiko", city="Hallifax", id=3},
        {name="Keneanung", city="Hashan", id=4},
        {name="Carmain", city="Mhaldor", id=5},
      }
      db:add(mydb.sheet, unpack(test_data))
    end)


    after_each(function()
      db:close()
      os.remove("Database_mydb.db")
      mydb = nil
      test_data = nil
    end)

    it("should successfully return all rows on an empty example.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, {}))
      assert.are.equal(#test_data, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(test_data, res)
    end)

    it("should successfully return a single row for a simple pattern.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { name = "Ixokai"}))
      assert.are.equal(1, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(test_data[1], res[1])
    end)

    it("should successfully return all matching rows for operator '<'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "< 3"}))
      local exp_res = { test_data[1], test_data[2] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '>'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "> 3"}))
      local exp_res = { test_data[4], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '>='.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = ">= 3"}))
      local exp_res = { test_data[3], test_data[4], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '<='.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "<= 3"}))
      local exp_res = { test_data[1], test_data[2], test_data[3] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '!='.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "!= 3"}))
      local exp_res = { test_data[1], test_data[2], test_data[4], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '<>'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "<> 3"}))
      local exp_res = { test_data[1], test_data[2], test_data[4], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows for operator '||'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "1||3||5"}))
      local exp_res = { test_data[1], test_data[3], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows with placeholder '_'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { name = "V_di"}))
      local exp_res = { test_data[2] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows with placeholder '%'.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { city = "M%"}))
      local exp_res = { test_data[1], test_data[5] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should successfully return all matching rows with ranges.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = "2::4"}))
      local exp_res = { test_data[2], test_data[3], test_data[4] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should return no row on an empty string.",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { name = ""}))
      local exp_res = { }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

    it("should handle non-string field values gracefully by converting "
    .. "them to a string (lua functionality).",
    function()
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, { id = 3}))
      local exp_res = { test_data[3] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)


    it("should correctly combine a query for a specific item.",
    function()
      db:add(mydb.sheet, { name = "Kenanung", city = "Mhaldor", id = 6 })
      local res = db:fetch(mydb.sheet, db:query_by_example(mydb.sheet, 
                    {name = "Keneanung", city = "Hashan" }))
      local exp_res = { test_data[4] }
      assert.are.equal(#exp_res, #res)
      for k, v in ipairs(res) do
         res[k]._row_id = nil --we get the row id back, which we don't need
      end
      assert.are.same(exp_res, res)
    end)

  end)

  describe("Tests, if the aggregate function works as intended",
  function()
    
    before_each(function()
      mydb = db:create("mydb",
        {
          sheet = {
            name = "", count = 0,
            _index = { "name" },
            _violations = "FAIL"
          }
        })
      test_data = {
        {name="Ixokai", count=11},
        {name="Vadi", count=2},
        {name="Heiko", count=15},
        {name="Keneanung", count=22},
        {name="Carmain", count=50},
        {name="Lynara", count=50},
      }
      db:add(mydb.sheet, unpack(test_data))
    end)


    after_each(function()
      db:close()
      os.remove("Database_mydb.db")
      mydb = nil
      test_data = nil
    end)
    
    it("should successfully sum all counts up.",
    function()
      local total = db:aggregate(mydb.sheet.count, "total")
      local exp_total = 0
      for _, v in ipairs(test_data) do
        exp_total = v.count + exp_total
      end
      assert.is.same(exp_total, total)
    end)
  
    it("should successfully calculate the average of all numbers.",
    function()
      local avg = db:aggregate(mydb.sheet.count, "avg")
      local exp_total, count = 0, 0
      for _, v in ipairs(test_data) do
        exp_total = exp_total + v.count
        count = count + 1
      end
      assert.is.same(exp_total / count, avg)
    end)
      
    it("should successfully calculate the minimum value of the numbers.",
    function()
      local min = db:aggregate(mydb.sheet.count, "min")
      local exp_min = 1000
      for _, v in ipairs(test_data) do
        if v.count < exp_min then
          exp_min = v.count
        end
      end
      assert.is.same(exp_min, min)
    end)
      
    it("should successfully calculate the minimum value of the names.",
    function()
      local min = db:aggregate(mydb.sheet.name, "min")
      local exp_min = "ZZZZZZZZZZZZZZ"
      for _, v in ipairs(test_data) do
        if v.name < exp_min then
          exp_min = v.name
        end
      end
      assert.is.same(exp_min, min)
    end)
      
    it("should successfully calculate the maximum value of the numbers.",
    function()
      local max = db:aggregate(mydb.sheet.count, "max")
      local exp_max = 0
      for _, v in ipairs(test_data) do
        if v.count > exp_max then
          exp_max = v.count
        end
      end
      assert.is.same(exp_max, max)
    end)
      
    it("should successfully calculate the maximum value of the names.",
    function()
      local max = db:aggregate(mydb.sheet.name, "max")
      local exp_max = "A"
      for _, v in ipairs(test_data) do
        if v.name > exp_max then
          exp_max = v.name
        end
      end
      assert.is.same(exp_max, max)
    end)
     
    it("should successfully calculate the count of the names.",
    function()
      local count = db:aggregate(mydb.sheet.name, "count")
      assert.is.same(#test_data, count)
    end)
     
    it("should successfully sum all counts greater than 11 up.",
    function()
      local total = db:aggregate(mydb.sheet.count, "total",
                                 db:gt(mydb.sheet.count, 11))
      local exp_total = 0
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          exp_total = v.count + exp_total
        end
      end
      assert.is.same(exp_total, total)
    end)
  
    it("should successfully calculate the average of all numbers greater than"
    .. "11.",
    function()
      local avg = db:aggregate(mydb.sheet.count, "avg",
                               db:gt(mydb.sheet.count, 11))
      local exp_total, count = 0, 0
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          exp_total = exp_total + v.count
          count = count + 1
        end
      end
      assert.is.same(exp_total / count, avg)
    end)
      
    it("should successfully calculate the minimum value of the numbers greater"
    .. " than 11.",
    function()
      local min = db:aggregate(mydb.sheet.count, "min",
                               db:gt(mydb.sheet.count, 11))
      local exp_min = 1000
      for _, v in ipairs(test_data) do
        if v.count < exp_min then
          if v.count > 11 then
            exp_min = v.count
          end
        end
      end
      assert.is.same(exp_min, min)
    end)
      
    it("should successfully calculate the minimum value of the names with count"
    .. " greater than 11.",
    function()
      local min = db:aggregate(mydb.sheet.name, "min",
                               db:gt(mydb.sheet.count, 11))
      local exp_min = "ZZZZZZZZZZZZZZ"
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          if v.name < exp_min then
            exp_min = v.name
          end
        end
      end
      assert.is.same(exp_min, min)
    end)
      
    it("should successfully calculate the maximum value of the numbers greater "
    .. "than 11.",
    function()
      local max = db:aggregate(mydb.sheet.count, "max",
                               db:gt(mydb.sheet.count, 11))
      local exp_max = 0
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          if v.count > exp_max then
            exp_max = v.count
          end
        end
      end
      assert.is.same(exp_max, max)
    end)
      
    it("should successfully calculate the maximum value of the names with count greater "
    .. "than 11.",
    function()
      local max = db:aggregate(mydb.sheet.name, "max",
                               db:gt(mydb.sheet.count, 11))
      local exp_max = "A"
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          if v.name > exp_max then
            exp_max = v.name
          end
        end
      end
      assert.is.same(exp_max, max)
    end)
     
    it("should successfully calculate the count of the names with count greater than 11.",
    function()
      local count = db:aggregate(mydb.sheet.name, "count",
                                 db:gt(mydb.sheet.count, 11))
      local exp_count = 0
      for _, v in ipairs(test_data) do
        if v.count > 11 then
          exp_count = exp_count + 1
        end
      end
      assert.is.same(exp_count, count)
    end)
    
    it("should successfully sum all unique counts up.",
    function()
      local total = db:aggregate(mydb.sheet.count, "total", nil, true)
      local exp_total = 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if not table.contains(seen_values, v.count) then
          exp_total = v.count + exp_total
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_total, total)
    end)
  
    it("should successfully calculate the average of all unique numbers.",
    function()
      local avg = db:aggregate(mydb.sheet.count, "avg", nil, true)
      local exp_total, count = 0, 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if not table.contains(seen_values, v.count) then
          exp_total = exp_total + v.count
          count = count + 1
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_total / count, avg)
    end)

    it("should successfully calculate the count of the unique numbers.",
    function()
      local count = db:aggregate(mydb.sheet.count, "count", nil, true)
      local exp_count = 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if not table.contains(seen_values, v.count) then
          exp_count = exp_count + 1
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_count, count)
    end)
     
    it("should successfully sum all unique counts greater than 11 up.",
    function()
      local total = db:aggregate(mydb.sheet.count, "total",
                                 db:gt(mydb.sheet.count, 11), true)
      local exp_total = 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if v.count > 11 and not table.contains(seen_values, v.count) then
          exp_total = v.count + exp_total
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_total, total)
    end)
  
    it("should successfully calculate the average of all unique numbers greater than"
    .. " 11.",
    function()
      local avg = db:aggregate(mydb.sheet.count, "avg",
                               db:gt(mydb.sheet.count, 11), true)
      local exp_total, count = 0, 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if v.count > 11 and not table.contains(seen_values, v.count) then
          exp_total = exp_total + v.count
          count = count + 1
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_total / count, avg)
    end)

    it("should successfully calculate the count of the unique numbers greater than 11.",
    function()
      local count = db:aggregate(mydb.sheet.count, "count",
                                 db:gt(mydb.sheet.count, 11), true)
      local exp_count = 0
      local seen_values = { }
      for _, v in ipairs(test_data) do
        if v.count > 11 and not table.contains(seen_values, v.count) then
          exp_count = exp_count + 1
          seen_values[#seen_values + 1] = v.count
        end
      end
      assert.is.same(exp_count, count)
    end)       
  end)

end)
