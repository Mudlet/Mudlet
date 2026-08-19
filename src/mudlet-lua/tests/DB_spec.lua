describe("Tests DB.lua functions", function()

  describe("Tests that DB creation and deletion works", function()
    describe("Test the functionality of db:create", function()
      it("Should create a db", function()
        mydb = db:create("peoplettestingonly", {
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
    end)

    describe("Test the functionality of db:close", function()
      it("Should successfully shut down a DB", function()
        db:close()
      end)
    end)

    it("Should recreate a DB", function()
      mydb = db:create("peoplettestingonly", {
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
      mydb = db:create("peoplettestingonly", {
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
      local filename = getMudletHomeDir() .. "/Database_peoplettestingonly.db"
      os.remove(filename)
    end)
  end)

  describe("Tests basic db:create() and db:add()", function()
    before_each(function()
      mydb = db:create("peoplettestingonly", {
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
      local filename = getMudletHomeDir() .. "/Database_peoplettestingonly.db"
      os.remove(filename)
    end)

    describe("Test the functionality of db:add", function()
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
  end)

  describe("Tests db:fetch()'s sorting functionality", function()
    before_each(function()
      mydb = db:create("dslpnpdatattestingonly", {
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
      local filename = getMudletHomeDir() .. "/Database_dslpnpdatattestingonly.db"
      os.remove(filename)
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
      mydb = db:create("mydbttestingonly", {
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
      local filename = getMudletHomeDir() .. "/Database_mydbttestingonly.db"
      os.remove(filename)
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

      mydb = db:create("mydbttestingonly", { sheet = newschema })
      assert.are.same(db.__schema.mydbttestingonly.sheet.columns, {row1 = "", row2 = 0, row3 = 0})
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

      mydb = db:create("mydbttestingonly", { sheet = newschema })
      assert.are.same(db.__schema.mydbttestingonly.sheet.columns, {row1 = "", row2 = 0, row3 = ""})
    end)

    it("Should add a column successfully to a filled db", function()
      db:add(mydb.sheet, {row1 = "some data"})

      local sheet = {
        row1 = "",
        row2 = 0,
        row3 = "",
        _index = {"row1"},
        _unique = {"row1"},
        _violations = "REPLACE"
      }

      mydb = db:create("mydbttestingonly", { sheet = sheet })
      local newrow = db:fetch(mydb.sheet)[1]
      assert.are.same("some data", newrow.row1)
      assert.are.same("", newrow.row3)
    end)
  end)

  describe("Tests, if options are correctly recognised and applied",
  function()

    before_each(function()
      mydb = db:create("mydbttestingonly",
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
      local filename = getMudletHomeDir() .. "/Database_mydbttestingonly.db"
      os.remove(filename)
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

        local conn = db.__conn.mydbttestingonly
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

        assert.equals(2, #results)

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
          end

          assert.are.same(expected, v)

        end

      end)

  end)

  describe("Tests, if columns are deleted successfully",
  function()

    before_each(function()
      mydb = db:create("mydbttestingonly",
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
      local filename = getMudletHomeDir() .. "/Database_mydbttestingonly.db"
      os.remove(filename)
      mydb = nil
    end)

    it("should successfully delete columns in an empty table.",
    function()
      mydb = db:create("mydbttestingonly", { sheet = { name = "", id = 0 }})
      local test = { name = "foo", id = 500 }
      db:add(mydb.sheet, test)
      local res = db:fetch(mydb.sheet)
      assert.are.equal(1, #res)
      res[1]._row_id = nil --we get the row id back, which we don't need
      assert.are.same(test, res[1])
    end)

    it("should successfully delete columns in a non empty table if force argument is provided.",
    function()
      local test = { name = "foo", id = 500, blubb = "bar" }
      db:add(mydb.sheet, test)
      mydb = db:create("mydbttestingonly", { sheet = { name = "", id = 0 }}, true)
      local res = db:fetch(mydb.sheet)
      test.blubb = nil -- we expect the blubb gets deleted
      assert.are.equal(1, #res)
      res[1]._row_id = nil --we get the row id back, which we don't need
      assert.are.same(test, res[1])
    end)

    it("should fail to delete columns in a non empty table if force argument is not provided.",
    function()
      local test = { name = "foo", id = 500, blubb = "bar" }
      db:add(mydb.sheet, test)
      assert.has_error(function() db:create("mydbttestingonly", { sheet = { name = "", id = 0 }}) end)
    end)

    it("should successfully delete empty columns in a non empty table",
    function()
      local test = { name = "foo", id = 500, blubb = db:Null() }
      db:add(mydb.sheet, test)
      mydb = db:create("mydbttestingonly", { sheet = { name = "", id = 0 }}, true)
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
      mydb = db:create("mydbttestingonly",
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
      local filename = getMudletHomeDir() .. "/Database_mydbttestingonly.db"
      os.remove(filename)
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
      mydb = db:create("mydbttestingonly",
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
      local filename = getMudletHomeDir() .. "/Database_mydbttestingonly.db"
      os.remove(filename)
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

    it("should apply a db:exp query when aggregating.",
    function()
      local total = db:aggregate(mydb.sheet.count, "total", db:exp("count > 5"))
      local exp_total = 0
      for _, v in ipairs(test_data) do
        if v.count > 5 then
          exp_total = exp_total + v.count
        end
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

  describe("Tests, if NULL handling works as intended",
  function()
    before_each(function()
      mydb = db:create("mydbtnulltesting",
        {
          sheet = {
            name = "",
            level = 0,
            motto = "",
            _unique = { "name" },
          }
        })
    end)

    after_each(function()
      db:close()
      local filename = getMudletHomeDir() .. "/Database_mydbtnulltesting.db"
      os.remove(filename)
      mydb = nil
    end)

    it("should be able to insert NULL values",
    function()
      local test = {name = "Bellman", level = db:Null(), motto = ""}
      db:add(mydb.sheet, test)
      test.level = nil
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)
      results[1]._row_id = nil
      assert.are.same(results[1], test)
    end)

    it("should be able to fetch by NULL conditions",
    function()
      local test1 = {name = "Boots", level = 1, motto = ""}
      local test2 = {name = "Bellman", level = db:Null(), motto = ""}
      db:add(mydb.sheet, test1, test2)
      test2.level = nil
      local results_null = db:fetch(mydb.sheet, db:is_nil(mydb.sheet.level))
      assert.is_true(#results_null == 1)
      results_null[1]._row_id = nil
      assert.are.same(results_null[1], test2)
      local results_not_null = db:fetch(mydb.sheet, db:is_not_nil(mydb.sheet.level))
      assert.is_true(#results_not_null == 1)
      results_not_null[1]._row_id = nil
      assert.are.same(results_not_null[1], test1)
    end)

    it("should be able to set an existing field to NULL",
    function()
      local test = {name = "Bellman", level = 1, motto = "Four weeks to the month you may mark"}
      db:add(mydb.sheet, test)
      db:set(mydb.sheet.motto, db:Null(), db:eq(mydb.sheet.name, "Bellman"))
      test.motto = nil
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)
      results[1]._row_id = nil
      assert.are.same(results[1], test)
    end)
  end)

  describe("Tests, if default NULL works as intended",
  function()
    after_each(function()
      db:close()
      local filename = getMudletHomeDir() .. "/Database_mydbtnulltesting.db"
      os.remove(filename)
      mydb = nil
    end)

    it("should be able to create a table with default NULL",
    function()
      mydb = db:create("mydbtnulltesting",
      {
        sheet = {
          name = "",
          house = db:Null(),
          _unique = { "name" },
        }
      })
      local test1 = {name = "Hermione", house = "Griffindor"}
      local test2 = {name = "Viktor"}
      db:add(mydb.sheet, test1, test2)
      test2.house = nil
      results1 = db:fetch(mydb.sheet, db:is_not_nil(mydb.sheet.house))
      results2 = db:fetch(mydb.sheet, db:is_nil(mydb.sheet.house))
      assert.is_true(#results1 == 1)
      results1[1]._row_id = nil
      assert.are.same(results1[1], test1)
      assert.is_true(#results2 == 1)
      results2[1]._row_id = nil
      assert.are.same(results2[1], test2)
    end)

    it("should be able to add a column with default NULL to a table",
    function()
      mydb = db:create("mydbtnulltesting",
      {
        sheet = {
          name = "",
          level = 1,
          _unique = { "name" },
        }
      })

      mydb = db:create("mydbtnulltesting",
      {
        sheet = {
          name = "",
          level = 1,
          house = db:Null(),
          _unique = { "name" },
        }
      })
      local test =  { {name = "Laergon", level = 1, house = "Kharon"}, {name = "Mymla", level = 5} }
      db:add(mydb.sheet, test[1], test[2])
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 2)
      results[1]._row_id = nil
      results[2]._row_id = nil
      assert.is_true(table.size(results[1]) == 3)
      assert.is_true(table.size(results[2]) == 2)
      assert.are.same(results, test)
    end)
  end)

  describe("Tests, if timestamp handling works as intended",
  function()
    local input = {
      current = db:Timestamp("CURRENT_TIMESTAMP"),
      niled = db:Timestamp(nil),
      epoched = db:Timestamp(1748288082), -- 2025-05-26T19:34:42+00:00
      tabled = db:Timestamp({year=1970, month=1, day=1, hour=10, sec=1})
    }

    before_each(function()
      mydb = db:create("mydbttimestamptesting", { sheet = input })
    end)

    after_each(function()
      db:close()
      local filename = getMudletHomeDir() .. "/Database_mydbttimestamptesting.db"
      os.remove(filename)
      mydb = nil
    end)


    it("should fetch a timestamp for CURRENT_TIMESTAMP.",
    function()
      db:add(mydb.sheet, input)
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)

      local result = results[1]
      assert.is_true(result.current._timestamp ~= nil)
    end)

    it("should fetch the same epoch timestamp as what was put in.",
    function()
      db:add(mydb.sheet, input)
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)

      local result = results[1]
      assert.are.same(result.epoched:as_number(), input.epoched:as_number())
      assert.are.same(result.epoched:as_string(), input.epoched:as_string())
      assert.are.same(result.epoched:as_table(), input.epoched:as_table())
    end)

    it("should fetch the same table timestamp as what was put in.",
    function()
      db:add(mydb.sheet, input)
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)

      local result = results[1]
      assert.are.same(result.tabled:as_number(), input.tabled:as_number())
      assert.are.same(result.tabled:as_string(), input.tabled:as_string())
      assert.are.same(result.tabled:as_table(), input.tabled:as_table())
    end)

    it("should fetch the same niled timestamp as what was put in.",
    function()
      db:add(mydb.sheet, input)
      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)

      local result = results[1]
      assert.are.same(result.niled._timestamp, input.niled._timestamp)
    end)

    it("should update without changing a timestamp's value.",
    function()
      db:add(mydb.sheet, input)

      local results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)
      local first_result = results[1]

      db:update(mydb.sheet, results[1])

      results = db:fetch(mydb.sheet)
      assert.is_true(#results == 1)
      local second_result = results[1]

      assert.are.same(first_result.current:as_number(), second_result.current:as_number())
      assert.are.same(first_result.current:as_string(), second_result.current:as_string())
      assert.are.same(first_result.current:as_table(), second_result.current:as_table())

      assert.are.same(first_result.epoched:as_number(), second_result.epoched:as_number())
      assert.are.same(first_result.epoched:as_string(), second_result.epoched:as_string())
      assert.are.same(first_result.epoched:as_table(), second_result.epoched:as_table())

      assert.are.same(first_result.tabled:as_number(), second_result.tabled:as_number())
      assert.are.same(first_result.tabled:as_string(), second_result.tabled:as_string())
      assert.are.same(first_result.tabled:as_table(), second_result.tabled:as_table())

      assert.are.same(first_result.niled._timestamp, second_result.niled._timestamp)
    end)
  end)

  describe("Tests, if hanging indexes are removed", function()
    local test_db_name = db:safe_name("remove_indexes_test")
    local test_db_file = getMudletHomeDir() .. "/Database_" .. test_db_name .. ".db"
    local cur;


    after_each(function()
      if cur then
        cur:close()
      end
      db:close()
      os.remove(test_db_file)
    end)


    it("should remove an index", function()
      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",

            _index = { "name" }
          }
        }
      );
      db:close();

      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      local conn = db.__conn[test_db_name]
      cur, _ = conn:execute([[
        SELECT
          name,
          tbl_name,
          sql
        FROM sqlite_master
          WHERE type = 'index' AND tbl_name = 'people' AND sql is not NULL;
      ]])
      assert.is_nil(cur:fetch({}, "a"))
    end)


    it("should remove a compound index", function()
      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",

            _index = { {"name", "city" } }
          }
        }
      );
      db:close();

      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      local conn = db.__conn[test_db_name]
      cur, _ = conn:execute([[
        SELECT
          name,
          tbl_name,
          sql
        FROM sqlite_master
          WHERE type = 'index' AND tbl_name = 'people' AND sql is not NULL;
      ]])
      assert.is_nil(cur:fetch({}, "a"))
    end)


    it("should remove a unique index", function()
      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      -- simulate creating a unique index, as Mudlet no longer creates them.
      local conn = db.__conn[test_db_name]
      conn:execute([[CREATE UNIQUE INDEX idx_test_c_name ON people ("name")]])
      conn:commit()
      db:close();

      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      conn = db.__conn[test_db_name]
      cur, _ = conn:execute([[
        SELECT
          name,
          tbl_name,
          sql
        FROM sqlite_master
          WHERE type = 'index' AND tbl_name = 'people' AND sql is not NULL;
      ]])
      assert.is_nil(cur:fetch({}, "a"))
    end)

    it("should remove a unique compound index", function()
      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      -- simulate creating a unique index, as Mudlet no longer creates them.
      local conn = db.__conn[test_db_name]
      conn:execute([[CREATE UNIQUE INDEX idx_test_c_name_city ON people ("name", "city")]])
      conn:commit()
      db:close();

      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",
          }
        }
      );
      conn = db.__conn[test_db_name]
      cur, _ = conn:execute([[
        SELECT
          name,
          tbl_name,
          sql
        FROM sqlite_master
          WHERE type = 'index' AND tbl_name = 'people' AND sql is not NULL;
      ]])
      assert.is_nil(cur:fetch({}, "a"))
    end)

    it("should remove a unique compound index but keep normal index", function()
      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",

            _index = {"city"}
          }
        }
      );
      -- simulate creating a unique index, as Mudlet no longer creates them.
      local conn = db.__conn[test_db_name]
      conn:execute([[CREATE UNIQUE INDEX idx_test_c_name_city ON people ("name", "city")]])
      conn:commit()
      db:close();

      db:create(
        test_db_name,
        {
          people = {
            name = "",
            city = "",

            _index = {"city"}
          }
        }
      );
      conn = db.__conn[test_db_name]
      cur, _ = conn:execute([[
        SELECT
          sql
        FROM sqlite_master
          WHERE type = 'index' AND tbl_name = 'people' AND sql is not NULL;
      ]])
      assert.are.equal([[CREATE INDEX idx_people_c_city ON people ("city")]], cur:fetch({}, "a").sql);
      assert.is_nil(cur:fetch({}, "a"))
    end)
  end)

  describe("Tests that _violations changes trigger table migration", function()
    local test_db_name = "violations_migration_test"
    local test_db_file
    
    before_each(function()
      test_db_file = getMudletHomeDir() .. "/Database_" .. test_db_name .. ".db"
      -- Remove any existing test database
      os.remove(test_db_file)
    end)
    
    after_each(function()
      db:close()
      os.remove(test_db_file)
    end)
    
    it("should migrate table when _violations changes from FAIL to REPLACE", function()
      -- Create initial database with FAIL
      mydb = db:create(test_db_name, {
        people = {
          name = "",
          city = "",
          _unique = { "name" },
          _violations = "FAIL"
        }
      })
      
      -- Add some test data
      db:add(mydb.people, {name = "Alice", city = "Boston"})
      db:add(mydb.people, {name = "Bob", city = "Chicago"})
      
      -- Verify FAIL behavior: adding duplicate should error
      local result, err = db:add(mydb.people, {name = "Alice", city = "Denver"})
      assert.is_nil(result)
      assert.is_not_nil(err)
      
      -- Close and recreate with REPLACE
      db:close()
      mydb = db:create(test_db_name, {
        people = {
          name = "",
          city = "",
          _unique = { "name" },
          _violations = "REPLACE"
        }
      })
      
      -- Verify data is still there
      local results = db:fetch(mydb.people)
      assert.are.equal(2, #results)
      
      -- Now REPLACE behavior should work: adding duplicate should replace
      db:add(mydb.people, {name = "Alice", city = "Denver"})
      
      results = db:fetch(mydb.people)
      assert.are.equal(2, #results)
      
      -- Find Alice's record
      local alice = nil
      for _, person in ipairs(results) do
        if person.name == "Alice" then
          alice = person
          break
        end
      end
      
      assert.is_not_nil(alice)
      assert.are.equal("Denver", alice.city)
    end)
    
    it("should migrate table when _violations changes from REPLACE to IGNORE", function()
      -- Create initial database with REPLACE
      mydb = db:create(test_db_name, {
        items = {
          id = 0,
          value = "",
          _unique = { "id" },
          _violations = "REPLACE"
        }
      })
      
      -- Add initial data
      db:add(mydb.items, {id = 1, value = "first"})
      db:add(mydb.items, {id = 1, value = "second"}) -- Should replace
      
      local results = db:fetch(mydb.items)
      assert.are.equal(1, #results)
      assert.are.equal("second", results[1].value)
      
      -- Close and recreate with IGNORE
      db:close()
      mydb = db:create(test_db_name, {
        items = {
          id = 0,
          value = "",
          _unique = { "id" },
          _violations = "IGNORE"
        }
      })
      
      -- Add duplicate with IGNORE - should keep original
      db:add(mydb.items, {id = 1, value = "third"})
      
      results = db:fetch(mydb.items)
      assert.are.equal(1, #results)
      assert.are.equal("second", results[1].value) -- Should still be "second"
    end)
    
    it("should migrate table when _violations changes with multi-column unique constraint", function()
      -- Create initial database with FAIL on multi-column unique
      mydb = db:create(test_db_name, {
        records = {
          name = "",
          category = "",
          value = 0,
          _unique = { {"name", "category"} },
          _violations = "FAIL"
        }
      })
      
      -- Add test data
      db:add(mydb.records, {name = "Item1", category = "A", value = 10})
      db:add(mydb.records, {name = "Item1", category = "B", value = 20})
      
      -- Verify FAIL behavior
      local result, err = db:add(mydb.records, {name = "Item1", category = "A", value = 30})
      assert.is_nil(result)
      assert.is_not_nil(err)
      
      -- Close and recreate with REPLACE
      db:close()
      mydb = db:create(test_db_name, {
        records = {
          name = "",
          category = "",
          value = 0,
          _unique = { {"name", "category"} },
          _violations = "REPLACE"
        }
      })
      
      -- Now REPLACE should work
      db:add(mydb.records, {name = "Item1", category = "A", value = 30})
      
      local results = db:fetch(mydb.records, db:eq(mydb.records.name, "Item1"))
      assert.are.equal(2, #results)
      
      -- Verify the value was replaced
      local found_replaced = false
      for _, record in ipairs(results) do
        if record.category == "A" and record.value == 30 then
          found_replaced = true
          break
        end
      end
      assert.is_true(found_replaced)
    end)
    
    it("should error when constraint changes result in data loss", function()
      -- Create initial database where violations of compound unique constraint are FAILed.
      mydb = db:create(
        test_db_name,
        {
          records = {
            name     = "",
            category = "",
            value    =  0,

            _unique     = { {"name", "category"} },
            _violations = "FAIL"
          }
        }
      )
      
      -- Add test data
      db:add(mydb.records, {name = "Item1", category = "A", value = 10})
      db:add(mydb.records, {name = "Item1", category = "B", value = 20})
      db:close()

      assert.has_error(
        function()
          -- Re-create sheet where violations of non-compound unique constraint are REPLACEd.
          db:create(
            test_db_name,
            {
              records = {
                name     = "",
                category = "",
                value    =  0,

                _unique     = {"name", "category"},
                _violations = "REPLACE"
              }
            }
          )
        end
      )

    end)
    
    it("should allow forced data loss during constraint change migrations", function()
      -- Create initial database where violations of compound unique constraint are FAILed.
      mydb = db:create(
        test_db_name,
        {
          records = {
            name     = "",
            category = "",
            value    =  0,

            _unique     = { {"name", "category"} },
            _violations = "FAIL"
          }
        }
      )
      
      -- Add test data
      db:add(mydb.records, {name = "Item1", category = "A", value = 10})
      db:add(mydb.records, {name = "Item1", category = "B", value = 20})

      -- Re-create sheet where violations of non-compound unique constraint are REPLACEd forecfully.
      db:close()
      db:create(
        test_db_name,
        {
          records = {
            name     = "",
            category = "",
            value    =  0,

            _unique     = {"name", "category"},
            _violations = "REPLACE"
          }
        },
        true
      )
      
      local results = db:fetch(mydb.records, db:eq(mydb.records.name, "Item1"))
      assert.are.equal(1, #results)
    end)
  end)

  describe("Tests db query-expression builders against real fetches", function()
    local function names(results)
      local t = {}
      for _, row in ipairs(results) do
        t[#t + 1] = row.name
      end
      table.sort(t)
      return t
    end

    before_each(function()
      mydb = db:create("exprtestingonly", {
        people = {
          name = "",
          city = "",
          level = 0,
          _index = { "city" },
        }
      })
      db:add(mydb.people,
        {name = "Ada",   city = "Boston",  level = 10},
        {name = "Bram",  city = "Chicago", level = 20},
        {name = "Cyra",  city = "Boston",  level = 30},
        {name = "Drake", city = "Denver",  level = 40},
        {name = "Eve",   city = "Chicago", level = 50})
    end)

    after_each(function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_exprtestingonly.db")
      mydb = nil
    end)

    it("db:lt returns rows with a field below the value", function()
      assert.are.same({"Ada", "Bram"}, names(db:fetch(mydb.people, db:lt(mydb.people.level, 30))))
    end)

    it("db:lte is inclusive of the boundary", function()
      assert.are.same({"Ada", "Bram", "Cyra"}, names(db:fetch(mydb.people, db:lte(mydb.people.level, 30))))
    end)

    it("db:gt returns rows with a field above the value", function()
      assert.are.same({"Drake", "Eve"}, names(db:fetch(mydb.people, db:gt(mydb.people.level, 30))))
    end)

    it("db:gte is inclusive of the boundary", function()
      assert.are.same({"Cyra", "Drake", "Eve"}, names(db:fetch(mydb.people, db:gte(mydb.people.level, 30))))
    end)

    it("db:eq matches an exact value", function()
      assert.are.same({"Ada", "Cyra"}, names(db:fetch(mydb.people, db:eq(mydb.people.city, "Boston"))))
    end)

    it("db:eq with case_insensitive matches regardless of case", function()
      assert.are.same({"Ada", "Cyra"}, names(db:fetch(mydb.people, db:eq(mydb.people.city, "BOSTON", true))))
    end)

    it("db:not_eq excludes an exact value", function()
      assert.are.same({"Bram", "Drake", "Eve"}, names(db:fetch(mydb.people, db:not_eq(mydb.people.city, "Boston"))))
    end)

    it("db:not_eq with case_insensitive excludes regardless of case", function()
      assert.are.same({"Bram", "Drake", "Eve"}, names(db:fetch(mydb.people, db:not_eq(mydb.people.city, "BOSTON", true))))
    end)

    it("db:like matches SQL LIKE wildcards", function()
      assert.are.same({"Ada", "Cyra"}, names(db:fetch(mydb.people, db:like(mydb.people.city, "Bo%"))))
    end)

    it("db:not_like excludes SQL LIKE matches", function()
      assert.are.same({"Bram", "Drake", "Eve"}, names(db:fetch(mydb.people, db:not_like(mydb.people.city, "Bo%"))))
    end)

    it("db:between is inclusive of both bounds", function()
      assert.are.same({"Bram", "Cyra", "Drake"}, names(db:fetch(mydb.people, db:between(mydb.people.level, 20, 40))))
    end)

    it("db:not_between excludes the inclusive range", function()
      assert.are.same({"Ada", "Eve"}, names(db:fetch(mydb.people, db:not_between(mydb.people.level, 20, 40))))
    end)

    it("db:in_ matches any value in the list", function()
      assert.are.same({"Ada", "Cyra", "Drake"}, names(db:fetch(mydb.people, db:in_(mydb.people.city, {"Boston", "Denver"}))))
    end)

    it("db:not_in excludes every value in the list", function()
      assert.are.same({"Bram", "Eve"}, names(db:fetch(mydb.people, db:not_in(mydb.people.city, {"Boston", "Denver"}))))
    end)

    it("db:exp injects a raw SQL WHERE expression", function()
      assert.are.same({"Cyra", "Drake", "Eve"}, names(db:fetch(mydb.people, db:exp("level > 25"))))
    end)

    it("db:exp still works inside an implicitly-ANDed table query", function()
      local results = db:fetch(mydb.people, {
        db:exp("level > 25"),
        db:eq(mydb.people.city, "Chicago"),
      })
      assert.are.same({"Eve"}, names(results))
    end)

    it("db:exp still combines with db:AND and db:OR", function()
      local anded = db:fetch(mydb.people, db:AND(db:exp("level > 25"), db:eq(mydb.people.city, "Chicago")))
      assert.are.same({"Eve"}, names(anded))
      local ored = db:fetch(mydb.people, db:OR(db:exp("level < 15"), db:exp("level > 45")))
      assert.are.same({"Ada", "Eve"}, names(ored))
    end)

    it("db:AND requires all sub-expressions to match", function()
      local query = db:AND(db:eq(mydb.people.city, "Boston"), db:gt(mydb.people.level, 15))
      assert.are.same({"Cyra"}, names(db:fetch(mydb.people, query)))
    end)

    it("db:OR matches either sub-expression", function()
      local query = db:OR(db:eq(mydb.people.city, "Denver"), db:eq(mydb.people.city, "Chicago"))
      assert.are.same({"Bram", "Drake", "Eve"}, names(db:fetch(mydb.people, query)))
    end)

    it("a table-array query is implicitly ANDed", function()
      local results = db:fetch(mydb.people, {
        db:eq(mydb.people.city, "Chicago"),
        db:gt(mydb.people.level, 30),
      })
      assert.are.same({"Eve"}, names(results))
    end)

    it("db:is_nil and db:is_not_nil partition rows by NULL", function()
      db:set(mydb.people.city, db:Null(), db:eq(mydb.people.name, "Ada"))
      assert.are.same({"Ada"}, names(db:fetch(mydb.people, db:is_nil(mydb.people.city))))
      assert.are.same({"Bram", "Cyra", "Drake", "Eve"}, names(db:fetch(mydb.people, db:is_not_nil(mydb.people.city))))
    end)
  end)

  describe("Tests db:delete", function()
    before_each(function()
      mydb = db:create("deletetestingonly", {
        sheet = {
          name = "",
          city = "",
          _index = { "name" },
        }
      })
      db:add(mydb.sheet,
        {name = "Ada",   city = "Boston"},
        {name = "Bram",  city = "Chicago"},
        {name = "Cyra",  city = "Boston"},
        {name = "Drake", city = "Denver"})
    end)

    after_each(function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_deletetestingonly.db")
      mydb = nil
    end)

    it("deletes a single row by _row_id number", function()
      local ada = db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1]
      db:delete(mydb.sheet, ada._row_id)
      assert.are.equal(0, #db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada")))
      assert.are.equal(3, #db:fetch(mydb.sheet))
    end)

    it("deletes a single row given a fetched result table", function()
      local bram = db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Bram"))[1]
      db:delete(mydb.sheet, bram)
      assert.are.equal(0, #db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Bram")))
      assert.are.equal(3, #db:fetch(mydb.sheet))
    end)

    it("deletes every row matching an expression", function()
      db:delete(mydb.sheet, db:eq(mydb.sheet.city, "Boston"))
      local remaining = db:fetch(mydb.sheet)
      assert.are.equal(2, #remaining)
      local cities = {}
      for _, row in ipairs(remaining) do
        cities[row.city] = true
      end
      assert.is_nil(cities["Boston"])
    end)

    it("deletes every row matching a db:exp expression", function()
      db:delete(mydb.sheet, db:exp("city = 'Boston'"))
      local remaining = db:fetch(mydb.sheet)
      assert.are.equal(2, #remaining)
      local cities = {}
      for _, row in ipairs(remaining) do
        cities[row.city] = true
      end
      assert.is_nil(cities["Boston"])
    end)

    it("truncates the whole sheet when the query is true", function()
      db:delete(mydb.sheet, true)
      assert.are.equal(0, #db:fetch(mydb.sheet))
    end)

    it("errors when no query argument is passed", function()
      local ok, err = pcall(function() db:delete(mydb.sheet) end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "must pass a query argument", 1, true) ~= nil)
    end)

    it("errors when passed a table without a _row_id", function()
      local ok, err = pcall(function() db:delete(mydb.sheet, {name = "Ada"}) end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "non-result table", 1, true) ~= nil)
    end)
  end)

  describe("Tests db:merge_unique", function()
    before_each(function()
      mydb = db:create("mergetestingonly", {
        friends = {
          name = "",
          city = "",
          level = 0,
          _unique = { "name" },
          _violations = "REPLACE",
        }
      })
      db:add(mydb.friends,
        {name = "Ada",  city = "Boston",  level = 10},
        {name = "Bram", city = "Chicago", level = 20})
    end)

    after_each(function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_mergetestingonly.db")
      mydb = nil
    end)

    it("updates existing rows and inserts new ones in one call", function()
      local rows = db:fetch(mydb.friends)
      assert.are.equal(2, #rows)
      for _, row in ipairs(rows) do
        row.city = "Mutantville"
      end
      rows[#rows + 1] = {name = "Cyra", city = "Denver", level = 5}
      db:merge_unique(mydb.friends, rows)

      local after = db:fetch(mydb.friends)
      assert.are.equal(3, #after)
      local byName = {}
      for _, row in ipairs(after) do
        byName[row.name] = row
      end
      assert.are.equal("Mutantville", byName.Ada.city)
      assert.are.equal("Mutantville", byName.Bram.city)
      assert.are.equal(10, byName.Ada.level)
      assert.are.equal("Denver", byName.Cyra.city)
      assert.are.equal(5, byName.Cyra.level)
    end)

    it("does not duplicate a row when merging an existing unique key", function()
      db:merge_unique(mydb.friends, { {name = "Ada", city = "Rome"} })
      local rows = db:fetch(mydb.friends, db:eq(mydb.friends.name, "Ada"))
      assert.are.equal(1, #rows)
      assert.are.equal("Rome", rows[1].city)
      assert.are.equal(10, rows[1].level)
    end)

    it("errors when the data argument is not a table", function()
      local ok, err = pcall(function() db:merge_unique(mydb.friends, nil) end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "required table of data", 1, true) ~= nil)
    end)

    it("errors when a merged row is missing the unique key", function()
      local ok, err = pcall(function()
        db:merge_unique(mydb.friends, { {city = "Nowhere"} })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "does not have the unique key", 1, true) ~= nil)
    end)

    it("errors on a sheet whose unique index spans multiple columns", function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_mergetestingonly.db")
      mydb = db:create("mergetestingonly", {
        friends = {
          name = "",
          city = "",
          _unique = { {"name", "city"} },
        }
      })
      db:add(mydb.friends, {name = "Ada", city = "Boston"})
      local ok, err = pcall(function()
        db:merge_unique(mydb.friends, { {name = "Ada", city = "Boston"} })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "single unique index with a single column", 1, true) ~= nil)
    end)
  end)

  describe("Tests db transaction rollback", function()
    before_each(function()
      mydb = db:create("rollbacktestingonly", {
        sheet = {
          name = "",
          _index = { "name" },
        }
      })
      db:add(mydb.sheet, {name = "committed"})
    end)

    after_each(function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_rollbacktestingonly.db")
      mydb = nil
    end)

    it("discards uncommitted rows when rolled back", function()
      assert.are.equal(1, #db:fetch(mydb.sheet))
      mydb:_begin()
      db:add(mydb.sheet, {name = "pending1"})
      db:add(mydb.sheet, {name = "pending2"})
      assert.are.equal(3, #db:fetch(mydb.sheet))
      mydb:_rollback()
      mydb:_end()
      local after = db:fetch(mydb.sheet)
      assert.are.equal(1, #after)
      assert.are.equal("committed", after[1].name)
    end)

    it("persists committed rows across a close and reopen", function()
      mydb:_begin()
      db:add(mydb.sheet, {name = "pending"})
      mydb:_commit()
      mydb:_end()
      -- reopen so the assertion sees the on-disk state, not this connection's
      -- own uncommitted view - this is what discriminates commit from a no-op
      db:close()
      mydb = db:create("rollbacktestingonly", {
        sheet = {
          name = "",
          _index = { "name" },
        }
      })
      assert.are.equal(2, #db:fetch(mydb.sheet))
    end)
  end)

  describe("Tests db:update and db:set edge cases", function()
    before_each(function()
      mydb = db:create("updatetestingonly", {
        sheet = {
          name = "",
          city = "",
          kills = 0,
          _unique = { "name" },
          _violations = "REPLACE",
        }
      })
      db:add(mydb.sheet,
        {name = "Ada",  city = "Boston",  kills = 3},
        {name = "Bram", city = "Chicago", kills = 7})
    end)

    after_each(function()
      db:close()
      os.remove(getMudletHomeDir() .. "/Database_updatetestingonly.db")
      mydb = nil
    end)

    it("updates the changed field and preserves the rest", function()
      local ada = db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1]
      ada.city = "Rome"
      db:update(mydb.sheet, ada)
      local reread = db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1]
      assert.are.equal("Rome", reread.city)
      assert.are.equal("Ada", reread.name)
      assert.are.equal(3, reread.kills)
    end)

    it("errors when updating a table without a _row_id", function()
      local ok, err = pcall(function()
        db:update(mydb.sheet, {name = "Ada", city = "Rome"})
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "_row_id", 1, true) ~= nil)
    end)

    it("db:set changes a field for rows matching the query", function()
      db:set(mydb.sheet.city, "Rome", db:eq(mydb.sheet.name, "Ada"))
      assert.are.equal("Rome", db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1].city)
      assert.are.equal("Chicago", db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Bram"))[1].city)
    end)

    it("db:set without a query updates every row", function()
      db:set(mydb.sheet.kills, 0)
      local rows = db:fetch(mydb.sheet)
      assert.are.equal(2, #rows)
      for _, row in ipairs(rows) do
        assert.are.equal(0, row.kills)
      end
    end)

    it("db:set evaluates a db:exp value instead of storing it literally", function()
      db:set(mydb.sheet.kills, db:exp("kills + 1"), db:eq(mydb.sheet.name, "Ada"))
      assert.are.equal(4, db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1].kills)
      assert.are.equal(7, db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Bram"))[1].kills)
    end)

    it("db:set accepts a db:exp as the WHERE query", function()
      db:set(mydb.sheet.city, "Rome", db:exp("kills > 5"))
      assert.are.equal("Boston", db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Ada"))[1].city)
      assert.are.equal("Rome", db:fetch(mydb.sheet, db:eq(mydb.sheet.name, "Bram"))[1].city)
    end)
  end)

  describe("Tests db.Database:_drop", function()
    before_each(function()
      mydb = db:create("droptestingonly", {
        people = {
          name = "",
          city = "",
          _index = { "city" },
          _unique = { "name" },
        }
      })
      db:add(mydb.people,
        {name = "Ada",  city = "Boston"},
        {name = "Bram", city = "Chicago"})
    end)

    after_each(function()
      pcall(function() db:close() end)
      os.remove(getMudletHomeDir() .. "/Database_droptestingonly.db")
      mydb = nil
    end)

    it("drops the sheet's table and indexes without erroring", function()
      local ok, err = pcall(function() mydb:_drop("people") end)
      assert.is_true(ok, err)

      -- the table (and hence its rows and indexes) is really gone from the database
      local conn = db.__conn[mydb._db_name]
      local cur = conn:execute("SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'people'")
      local exists = cur and cur ~= 0 and cur:fetch({}, "a") ~= nil
      if cur and cur ~= 0 then
        cur:close()
      end
      assert.is_false(exists)
    end)

    it("drops a sheet whose _unique index is declared as a string", function()
      local sdb = db:create("dropstrtestingonly", {
        pets = {
          name = "",
          _unique = "name",
        }
      })
      local ok, err = pcall(function() sdb:_drop("pets") end)
      db:close("dropstrtestingonly")
      os.remove(getMudletHomeDir() .. "/Database_dropstrtestingonly.db")
      assert.is_true(ok, err)
    end)
  end)

  describe("Tests db:close contracts and reopen", function()
    after_each(function()
      pcall(function() db:close("closetestingonly") end)
      os.remove(getMudletHomeDir() .. "/Database_closetestingonly.db")
      mydb = nil
    end)

    it("closes a named database and reports success", function()
      db:create("closetestingonly", { sheet = { name = "" } })
      local ok, msg = db:close("closetestingonly")
      assert.is_true(ok)
      assert.are.equal("", msg)
    end)

    it("returns false when closing a database that does not exist", function()
      db:create("closetestingonly", { sheet = { name = "" } })
      local ok, msg = db:close("nonexistentdbxyz")
      assert.is_false(ok)
      assert.is_true(string.find(msg, "does not exist", 1, true) ~= nil)
      db:close("closetestingonly")
    end)

    it("returns false when called before any database environment exists", function()
      local saved_env = db.__env
      db.__env = nil
      local ok, msg = db:close("whatever")
      db.__env = saved_env
      assert.is_false(ok)
      assert.is_true(string.find(msg, "environment is nil", 1, true) ~= nil)
    end)

    it("errors when db_name is neither a string nor nil", function()
      db:create("closetestingonly", { sheet = { name = "" } })
      local ok, err = pcall(function() db:close(12345) end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "expected db_name to be string or nil", 1, true) ~= nil)
      db:close("closetestingonly")
    end)

    it("persists data across close and reopen", function()
      local d = db:create("closetestingonly", {
        sheet = { name = "", city = "", _index = {"name"} }
      })
      db:add(d.sheet, {name = "Ada", city = "Boston"})
      db:close("closetestingonly")

      local d2 = db:create("closetestingonly", {
        sheet = { name = "", city = "", _index = {"name"} }
      })
      local rows = db:fetch(d2.sheet)
      assert.are.equal(1, #rows)
      assert.are.equal("Ada", rows[1].name)
      assert.are.equal("Boston", rows[1].city)
      db:close("closetestingonly")
    end)
  end)

  describe("Tests db:create schema validation", function()
    after_each(function()
      pcall(function() db:close("badschematestingonly") end)
      os.remove(getMudletHomeDir() .. "/Database_badschematestingonly.db")
      mydb = nil
    end)

    it("errors on an unrecognised _violations option", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _unique = { "name" }, _violations = "NONSENSE" }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "_validations must be one of", 1, true) ~= nil)
    end)

    it("errors on a non-string _violations option", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _unique = { "name" }, _violations = 42 }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "_validations must be a string", 1, true) ~= nil)
    end)

    it("errors on a malformed _unique constraint", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _unique = { 123 } }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "must be a string or table", 1, true) ~= nil)
    end)

    it("errors when _unique is neither a string nor a table", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _unique = 42 }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "_unique must be a string or a table", 1, true) ~= nil)
    end)

    it("errors when _index is neither a string nor a table", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _index = 42 }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "_index must be a string or a table", 1, true) ~= nil)
    end)

    it("errors on a malformed _index", function()
      local ok, err = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _index = { 123 } }
        })
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "Members of _index must be a string or table", 1, true) ~= nil)
    end)

    it("takes a falsy _index as a sheet that wants no indexes", function()
      -- "_index = wanted and {'city'}" is false rather than nil when the
      -- condition does not hold, and that used to be, and stays, a valid schema
      local ok = pcall(function()
        db:create("badschematestingonly", {
          sheet = { name = "", _index = false }
        })
      end)
      assert.is_true(ok)
    end)
  end)

end)

describe("Tests db:echo_sql", function()
  local saved

  before_each(function()
    saved = db.debug_sql
  end)

  after_each(function()
    db.debug_sql = saved
  end)

  it("prints the statement it is handed when SQL debugging is on", function()
    db.debug_sql = true
    local printSpy = spy.on(_G, "print")
    finally(function() print:revert() end)
    db:echo_sql("SELECT 1;")
    assert.spy(printSpy).was.called(1)
    assert.spy(printSpy).was.called_with("SELECT 1;")
  end)

  it("stays silent while SQL debugging is off", function()
    db.debug_sql = false
    local printSpy = spy.on(_G, "print")
    finally(function() print:revert() end)
    db:echo_sql("SELECT 1;")
    assert.spy(printSpy).was_not_called()
  end)

  it("is silent by default", function()
    assert.is_falsy(saved)
  end)
end)

-- The helpers below all begin with an underscore: they are db's internals, not
-- its public API. They are specced directly because every public db function is
-- built out of them, so a change to one of them moves behaviour everywhere at
-- once, and because the SQL they produce is the only place the escaping and
-- quoting rules are actually written down.
describe("Tests db's internal SQL helpers", function()

  describe("Tests db:_sql_type", function()
    it("maps a number to REAL", function()
      assert.are.equal("REAL", db:_sql_type(0))
      assert.are.equal("REAL", db:_sql_type(-1.5))
    end)

    it("maps nil to NULL", function()
      assert.are.equal("NULL", db:_sql_type(nil))
    end)

    it("maps a timestamp to INTEGER, including the empty one", function()
      assert.are.equal("INTEGER", db:_sql_type(db:Timestamp(1234)))
      assert.are.equal("INTEGER", db:_sql_type(db:Timestamp("CURRENT_TIMESTAMP")))
      -- db:Timestamp(nil) stores false rather than nil, so it is still a
      -- timestamp column and must not fall through to TEXT
      assert.are.equal("INTEGER", db:_sql_type(db:Timestamp(nil)))
    end)

    it("maps db:Null to NULL", function()
      assert.are.equal("NULL", db:_sql_type(db:Null()))
    end)

    it("maps everything else, including a plain table, to TEXT", function()
      assert.are.equal("TEXT", db:_sql_type(""))
      assert.are.equal("TEXT", db:_sql_type("some text"))
      assert.are.equal("TEXT", db:_sql_type(true))
      assert.are.equal("TEXT", db:_sql_type({}))
    end)
  end)

  describe("Tests db:_sql_convert", function()
    it("double quotes a string default and doubles up single quotes in it", function()
      assert.are.equal('""', db:_sql_convert(""))
      assert.are.equal('"plain"', db:_sql_convert("plain"))
      assert.are.equal([["it''s"]], db:_sql_convert("it's"))
    end)

    it("renders nil and db:Null as the NULL keyword", function()
      assert.are.equal("NULL", db:_sql_convert(nil))
      assert.are.equal("NULL", db:_sql_convert(db:Null()))
    end)

    it("renders a timestamp as its raw epoch number", function()
      assert.are.equal("1234", db:_sql_convert(db:Timestamp(1234)))
    end)

    it("renders the empty timestamp as NULL rather than as false", function()
      assert.are.equal("NULL", db:_sql_convert(db:Timestamp(nil)))
    end)

    it("renders anything else with tostring, unquoted", function()
      assert.are.equal("42", db:_sql_convert(42))
      assert.are.equal("true", db:_sql_convert(true))
    end)
  end)

  describe("Tests db:_index_name", function()
    it("names a single column index after the sheet and the column", function()
      assert.are.equal("idx_people_c_city", db:_index_name("people", "city"))
    end)

    it("joins every column of a compound index into one name", function()
      assert.are.equal("idx_people_c_name_city", db:_index_name("people", {"name", "city"}))
    end)

    it("gives two different indexes on one sheet two different names", function()
      -- the names have to differ or CREATE INDEX IF NOT EXISTS silently keeps
      -- the first index and the second one is never made
      assert.are_not.equal(db:_index_name("people", "city"), db:_index_name("people", "name"))
      assert.are_not.equal(db:_index_name("people", {"name", "city"}), db:_index_name("people", {"city", "name"}))
    end)

    it("refuses anything that is not a string or a table", function()
      local ok, err = pcall(function() return db:_index_name("people", 42) end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "Indexes must be either a string or a table.", 1, true))
    end)
  end)

  describe("Tests db:_index_valid", function()
    local columns = {name = "TEXT", city = "TEXT"}

    it("accepts a single column index that names a real column", function()
      assert.is_true(db:_index_valid(columns, "city"))
    end)

    it("rejects a single column index that names a column the sheet lacks", function()
      assert.is_false(db:_index_valid(columns, "nosuchcolumn"))
    end)

    it("accepts a compound index whose columns all exist", function()
      assert.is_true(db:_index_valid(columns, {"name", "city"}))
    end)

    it("rejects a compound index as soon as one column is missing", function()
      assert.is_false(db:_index_valid(columns, {"name", "nosuchcolumn"}))
    end)

    it("accepts an empty compound index", function()
      assert.is_true(db:_index_valid(columns, {}))
    end)
  end)

  describe("Tests db:_sql_columns", function()
    it("lower cases and double quotes a single column name", function()
      assert.are.equal('"city"', db:_sql_columns("City"))
    end)

    it("comma separates a list of column names", function()
      assert.are.equal('"name","city"', db:_sql_columns({"name", "City"}))
    end)

    it("attaches a sort direction to the column before it instead of quoting it", function()
      -- db:fetch appends "DESC" as its own list entry, so it must not come out
      -- as a column name of its own
      assert.are.equal('"name" DESC', db:_sql_columns({"name", "DESC"}))
      assert.are.equal('"name" asc,"city" desc', db:_sql_columns({"name", "asc", "city", "desc"}))
    end)

    it("refuses anything that is not a string or a table", function()
      local ok, err = pcall(function() return db:_sql_columns(42) end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "Must specify either a table array or string for index, not number", 1, true))
    end)

    it("refuses a list member that is not a string", function()
      local ok, err = pcall(function() return db:_sql_columns({42}) end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "Column names must be strings, not number", 1, true))
    end)

    it("quotes a leading sort direction as the column name it has to be", function()
      -- there is no column in front of it to attach it to, and a sheet is
      -- allowed a column called desc
      assert.are.equal('"desc"', db:_sql_columns({"desc"}))
      assert.are.equal('"asc","name"', db:_sql_columns({"asc", "name"}))
    end)
  end)

  describe("Tests db:_sql_fields", function()
    it("wraps one quoted field name in parentheses", function()
      assert.are.equal('("name")', db:_sql_fields({name = "Bob"}))
    end)

    it("keeps the case of the field name, unlike db:_sql_columns", function()
      assert.are.equal('("Name")', db:_sql_fields({Name = "Bob"}))
    end)

    it("produces an empty list for an empty row", function()
      assert.are.equal("()", db:_sql_fields({}))
    end)
  end)

  describe("Tests db:_sql_values", function()
    it("single quotes a string and doubles up single quotes in it", function()
      assert.are.equal("('plain')", db:_sql_values({name = "plain"}))
      assert.are.equal("('it''s')", db:_sql_values({name = "it's"}))
    end)

    it("leaves a number unquoted", function()
      assert.are.equal("(42)", db:_sql_values({kills = 42}))
    end)

    it("turns CURRENT_TIMESTAMP into a call to sqlite's datetime", function()
      assert.are.equal("(datetime('now'))", db:_sql_values({when_ = db:Timestamp("CURRENT_TIMESTAMP")}))
    end)

    it("turns an epoch timestamp into a unixepoch conversion", function()
      assert.are.equal("(datetime('1234', 'unixepoch'))", db:_sql_values({when_ = db:Timestamp(1234)}))
    end)

    it("turns the empty timestamp and db:Null into NULL", function()
      assert.are.equal("(NULL)", db:_sql_values({when_ = db:Timestamp(nil)}))
      assert.are.equal("(NULL)", db:_sql_values({whatever = db:Null()}))
    end)

    it("produces an empty list for an empty row", function()
      assert.are.equal("()", db:_sql_values({}))
    end)
  end)

  describe("Tests db:_sql_fields and db:_sql_values together", function()
    it("lists the fields and the values of one row in the same order", function()
      -- this is the only thing that makes the pair usable: db:add writes
      -- "INSERT INTO sheet <fields> VALUES <values>", and both walk the row
      -- with pairs(), so the two walks have to agree or every column of every
      -- insert lands in the wrong one
      local row = {alpha = "a", bravo = "b", charlie = "c", delta = 4, echo = "e"}

      local fields = db:_sql_fields(row):match("^%((.*)%)$")
      local values = db:_sql_values(row):match("^%((.*)%)$")
      local names, contents = string.split(fields, ","), string.split(values, ",")

      assert.are.equal(5, #names)
      assert.are.equal(#names, #contents)
      for index, name in ipairs(names) do
        local column = name:match('^"(.*)"$')
        local expected = type(row[column]) == "string" and ("'" .. row[column] .. "'") or tostring(row[column])
        assert.are.equal(expected, contents[index], "column " .. column .. " did not line up with its value")
      end
    end)
  end)

  describe("Tests db:_validate_validations", function()
    it("accepts every documented conflict resolution", function()
      for _, option in ipairs({"ABORT", "FAIL", "IGNORE", "REPLACE", "ROLLBACK"}) do
        local valid, msg = db:_validate_validations(option)
        assert.is_true(valid, option .. " should be a valid _violations option")
        assert.are.equal("", msg)
      end
    end)

    it("rejects an option it does not know and says what it wanted", function()
      local valid, msg = db:_validate_validations("NONSENSE")
      assert.is_false(valid)
      assert.is_truthy(string.find(msg, "_validations must be one of", 1, true))
      assert.is_truthy(string.find(msg, "NONSENSE", 1, true))
    end)

    it("rejects a non-string and names the type it got", function()
      local valid, msg = db:_validate_validations(42)
      assert.is_false(valid)
      assert.are.equal("_validations must be a string. Received number", msg)
    end)

    it("is case sensitive", function()
      assert.is_false((db:_validate_validations("fail")))
    end)
  end)

  describe("Tests db:_validate_unique_contraints", function()
    it("accepts a bare column name", function()
      local valid, msg = db:_validate_unique_contraints("name")
      assert.is_true(valid)
      assert.are.equal("", msg)
    end)

    it("accepts a list of column names", function()
      assert.is_true((db:_validate_unique_contraints({"name", "city"})))
    end)

    it("accepts a compound constraint", function()
      assert.is_true((db:_validate_unique_contraints({{"name", "city"}})))
    end)

    it("accepts an empty list", function()
      assert.is_true((db:_validate_unique_contraints({})))
    end)

    it("rejects a compound constraint holding something other than a column name", function()
      local valid, msg = db:_validate_unique_contraints({{"name", 42}})
      assert.is_false(valid)
      assert.is_truthy(string.find(msg, "Multi-column definitions for _unique must be a list of strings", 1, true))
    end)

    it("rejects a member that is neither a string nor a table", function()
      local valid, msg = db:_validate_unique_contraints({42})
      assert.is_false(valid)
      assert.are.equal("Members of _unique must be a string or table. Received number.", msg)
    end)

    it("rejects a constraint that is neither a string nor a table", function()
      local valid, msg = db:_validate_unique_contraints(42)
      assert.is_false(valid)
      assert.are.equal("_unique must be a string or a table.  Received number.", msg)
    end)

    it("reports every bad member rather than only the first", function()
      local valid, msg = db:_validate_unique_contraints({42, true})
      assert.is_false(valid)
      assert.are.equal(2, #string.split(msg, "\n"))
    end)
  end)

  describe("Tests db:_validate_index", function()
    it("accepts a bare column name", function()
      local valid, msg = db:_validate_index("city")
      assert.is_true(valid)
      assert.are.equal("", msg)
    end)

    it("accepts a list of column names", function()
      assert.is_true((db:_validate_index({"name", "city"})))
    end)

    it("accepts a compound index", function()
      assert.is_true((db:_validate_index({{"name", "city"}})))
    end)

    it("accepts an empty list", function()
      assert.is_true((db:_validate_index({})))
    end)

    it("rejects a compound index holding something other than a column name", function()
      local valid, msg = db:_validate_index({{"name", 42}})
      assert.is_false(valid)
      assert.is_truthy(string.find(msg, "Multi-column definitions for _index must be a list of strings", 1, true))
    end)

    it("rejects a member that is neither a string nor a table", function()
      local valid, msg = db:_validate_index({42})
      assert.is_false(valid)
      assert.are.equal("Members of _index must be a string or table. Received number.", msg)
    end)

    it("rejects an index that is neither a string nor a table", function()
      local valid, msg = db:_validate_index(42)
      assert.is_false(valid)
      assert.are.equal("_index must be a string or a table.  Received number.", msg)
    end)

    it("reports every bad member rather than only the first", function()
      local valid, msg = db:_validate_index({42, true})
      assert.is_false(valid)
      assert.are.equal(2, #string.split(msg, "\n"))
    end)

    it("answers the same shapes as its _unique counterpart", function()
      -- the two options take the same shapes, so a schema that one accepts and
      -- the other refuses is a bug in whichever refused it
      for _, shape in ipairs({"city", {"city"}, {{"name", "city"}}, {}}) do
        assert.are.equal((db:_validate_unique_contraints(shape)), (db:_validate_index(shape)))
      end
      for _, shape in ipairs({42, {42}, {{42}}}) do
        assert.are.equal((db:_validate_unique_contraints(shape)), (db:_validate_index(shape)))
      end
    end)
  end)

  describe("Tests db:_extract_table_constraints", function()
    it("returns nothing for no SQL at all", function()
      assert.are.equal("", db:_extract_table_constraints(nil))
      assert.are.equal("", db:_extract_table_constraints(""))
    end)

    it("returns nothing for SQL that is not a CREATE TABLE", function()
      assert.are.equal("", db:_extract_table_constraints("SELECT * FROM people"))
    end)

    it("returns nothing for a table with no unique constraints", function()
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("name" TEXT NULL DEFAULT "")'))
    end)

    it("extracts a column level unique constraint", function()
      assert.are.equal("unique on conflict replace",
        db:_extract_table_constraints('CREATE TABLE people ("name" TEXT NULL DEFAULT "" UNIQUE ON CONFLICT REPLACE)'))
    end)

    it("extracts a table level unique constraint with its columns", function()
      assert.are.equal('unique("name", "city") on conflict fail',
        db:_extract_table_constraints('CREATE TABLE people ("name" TEXT NULL, "city" TEXT NULL, UNIQUE("name", "city") ON CONFLICT FAIL)'))
    end)

    it("ignores case, newlines and repeated whitespace", function()
      local oneLine = 'CREATE TABLE people ("name" TEXT NULL DEFAULT "" UNIQUE ON CONFLICT REPLACE)'
      local sprawling = 'create   table   people\n(\n  "name"   text   null   default ""\n  unique   on   conflict   replace\n)'
      assert.are.equal(db:_extract_table_constraints(oneLine), db:_extract_table_constraints(sprawling))
    end)

    it("orders the constraints so that the same table always compares equal", function()
      -- db:_migrate compares this string against the one it built to decide
      -- whether to rebuild the table, so two spellings of one schema must match
      local first = 'CREATE TABLE people ("a" TEXT UNIQUE ON CONFLICT FAIL, UNIQUE("b", "c") ON CONFLICT IGNORE)'
      local second = 'CREATE TABLE people (UNIQUE("b", "c") ON CONFLICT IGNORE, "a" TEXT UNIQUE ON CONFLICT FAIL)'
      assert.are.equal(db:_extract_table_constraints(first), db:_extract_table_constraints(second))
      assert.are.equal('unique on conflict fail|unique("b", "c") on conflict ignore', db:_extract_table_constraints(first))
    end)

    it("separates a change of conflict resolution from an unchanged one", function()
      local fail = 'CREATE TABLE people ("name" TEXT UNIQUE ON CONFLICT FAIL)'
      local replace = 'CREATE TABLE people ("name" TEXT UNIQUE ON CONFLICT REPLACE)'
      assert.are_not.equal(db:_extract_table_constraints(fail), db:_extract_table_constraints(replace))
    end)

    it("ignores a column that was added or removed", function()
      -- the whole point of comparing constraints instead of the whole statement
      local before = 'CREATE TABLE people ("name" TEXT UNIQUE ON CONFLICT FAIL)'
      local after = 'CREATE TABLE people ("name" TEXT UNIQUE ON CONFLICT FAIL, "city" TEXT NULL DEFAULT "")'
      assert.are.equal(db:_extract_table_constraints(before), db:_extract_table_constraints(after))
    end)

    it("sees a UNIQUE that carries no ON CONFLICT clause", function()
      -- sqlite defaults the conflict resolution to ABORT, so a table this
      -- module did not write can hold one of these; missing it makes a sheet
      -- with a unique constraint compare equal to one without
      assert.are.equal("unique",
        db:_extract_table_constraints('CREATE TABLE people ("name" TEXT NULL DEFAULT "" UNIQUE, "city" TEXT NULL)'))
      assert.are.equal('unique("name", "city")',
        db:_extract_table_constraints('CREATE TABLE people ("name" TEXT NULL, "city" TEXT NULL, UNIQUE("name", "city"))'))
    end)

    it("tells a bare UNIQUE apart from one with a conflict clause", function()
      local bare = 'CREATE TABLE people ("name" TEXT UNIQUE)'
      local resolved = 'CREATE TABLE people ("name" TEXT UNIQUE ON CONFLICT FAIL)'
      assert.are_not.equal(db:_extract_table_constraints(bare), db:_extract_table_constraints(resolved))
      assert.are_not.equal(db:_extract_table_constraints(bare), db:_extract_table_constraints('CREATE TABLE people ("name" TEXT)'))
    end)

    it("does not mistake a column named after the keyword for a constraint", function()
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("unique_id" TEXT NULL DEFAULT "")'))
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("uniqueness" TEXT NULL DEFAULT "")'))
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("unique" TEXT NULL DEFAULT "")'))
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("kind" TEXT NULL DEFAULT "unique")'))
      assert.are.equal("", db:_extract_table_constraints('CREATE TABLE people ("kind" TEXT NULL DEFAULT "a unique sword")'))
      -- and a column that is both named after the keyword and carries one
      assert.are.equal("unique on conflict fail",
        db:_extract_table_constraints('CREATE TABLE people ("unique" TEXT NULL DEFAULT "" UNIQUE ON CONFLICT FAIL)'))
    end)
  end)

  describe("Tests db:_build_create_table_sql", function()
    it("always gives the sheet an autoincrementing _row_id", function()
      local sql = db:_build_create_table_sql({columns = {name = ""}, options = {}}, "people")
      assert.are.equal('CREATE TABLE people ("_row_id" INTEGER PRIMARY KEY AUTOINCREMENT, "name" TEXT NULL DEFAULT "")', sql)
    end)

    it("types a column from its default value", function()
      local sql = db:_build_create_table_sql({columns = {kills = 0}, options = {}}, "people")
      assert.is_truthy(string.find(sql, '"kills" REAL NULL DEFAULT 0', 1, true))
    end)

    it("adds a column level unique constraint for a single unique column", function()
      local sql = db:_build_create_table_sql({columns = {name = ""}, options = {_unique = "name"}}, "people")
      assert.is_truthy(string.find(sql, '"name" TEXT NULL DEFAULT "" UNIQUE ON CONFLICT FAIL', 1, true))
    end)

    it("accepts the unique column as a one entry list too", function()
      local sql = db:_build_create_table_sql({columns = {name = ""}, options = {_unique = {"name"}}}, "people")
      assert.is_truthy(string.find(sql, 'UNIQUE ON CONFLICT FAIL', 1, true))
    end)

    it("adds a table level unique constraint for a compound one", function()
      local sql = db:_build_create_table_sql({columns = {name = ""}, options = {_unique = {{"name", "city"}}}}, "people")
      assert.is_truthy(string.find(sql, 'UNIQUE("name", "city") ON CONFLICT FAIL', 1, true))
    end)

    it("uses the sheet's conflict resolution rather than the default", function()
      local sql = db:_build_create_table_sql({columns = {name = ""}, options = {_unique = "name", _violations = "REPLACE"}}, "people")
      assert.is_truthy(string.find(sql, "ON CONFLICT REPLACE", 1, true))
      assert.is_nil(string.find(sql, "ON CONFLICT FAIL", 1, true))
    end)

    it("leaves a column that is not unique alone", function()
      local sql = db:_build_create_table_sql({columns = {city = ""}, options = {_unique = "name"}}, "people")
      assert.is_nil(string.find(sql, "UNIQUE", 1, true))
    end)
  end)
end)

-- These four run against a real sqlite database rather than against strings:
-- they are the parts of db:create that touch the file on disk.
describe("Tests db's internals against a real database", function()
  local dbName = "dbinternalstestingonly"
  local dbFile = getMudletHomeDir() .. "/Database_" .. dbName .. ".db"
  local mydb

  local function indexNames(sheetName)
    local conn = db.__conn[dbName]
    local cursor = conn:execute(
      "SELECT name FROM sqlite_master WHERE type = 'index' AND tbl_name = '" .. sheetName .. "' AND sql IS NOT NULL"
    )
    local names = {}
    local row = cursor:fetch({}, "a")
    while row do
      names[#names + 1] = row.name
      row = cursor:fetch({}, "a")
    end
    cursor:close()
    table.sort(names)
    return names
  end

  before_each(function()
    mydb = db:create(dbName, {
      people = {
        name = "",
        city = "",
        kills = 0,
        seen = db:Timestamp("CURRENT_TIMESTAMP"),
        _index = {"city"}
      }
    })
  end)

  after_each(function()
    db:close()
    os.remove(dbFile)
    mydb = nil
  end)

  describe("Tests db:_isActiveDBName", function()
    it("reports an open database whose file is on disk as active", function()
      assert.is_truthy(db:_isActiveDBName(dbName))
    end)

    it("sanitises the name it is given first", function()
      -- db:create sanitises too, so a caller passing the unsanitised name has
      -- to reach the same connection or db:create opens a second one
      assert.is_truthy(db:_isActiveDBName("DB Internals Testing Only"))
    end)

    it("reports a database that was never created as inactive", function()
      assert.is_falsy(db:_isActiveDBName("nosuchdatabaseatall"))
    end)

    it("reports a closed database as inactive", function()
      assert.is_true((db:close(dbName)))
      assert.is_falsy(db:_isActiveDBName(dbName))
    end)

    it("reports an open connection whose file has gone as inactive", function()
      -- the file is what db:create reconnects to, so a live handle to a deleted
      -- file must not count as active
      os.remove(dbFile)
      if io.exists(dbFile) then
        -- Windows will not unlink a file sqlite still has open, so there is no
        -- open-connection-without-a-file state to ask about there
        pending("this platform keeps a database file that is still open")
      end
      assert.is_falsy(db:_isActiveDBName(dbName))
    end)
  end)

  describe("Tests db:get_database", function()
    it("hands back a reference to a database that db:create already made", function()
      local reference = db:get_database(dbName)
      assert.is_table(reference)
      assert.are.equal("people", reference.people._sht_name)
      assert.are.equal("name", reference.people.name.name)
    end)

    it("sanitises the name it is given", function()
      assert.are.equal(dbName, db:get_database("DB Internals Testing Only")._db_name)
    end)

    it("hands back a reference that reads the same rows as db:create's", function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
      local rows = db:fetch(db:get_database(dbName).people)
      assert.are.equal(1, #rows)
      assert.are.equal("Bob", rows[1].name)
    end)

    it("refuses a database that does not exist", function()
      local ok, err = pcall(function() return db:get_database("nosuchdatabaseatall") end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "Attempt to access database that does not exist.", 1, true))
    end)

    it("refuses a sheet the database does not have", function()
      local ok, err = pcall(function() return db:get_database(dbName).nosuchsheet end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "does not exist", 1, true))
    end)
  end)

  describe("Tests db:fetch_sql", function()
    before_each(function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork", kills = 3})
      db:add(mydb.people, {name = "Carrot", city = "Ankh-Morpork", kills = 7})
    end)

    it("returns one coerced row per result", function()
      local rows = db:fetch_sql(mydb.people, "SELECT * FROM people ORDER BY name")
      assert.are.equal(2, #rows)
      assert.are.equal("Bob", rows[1].name)
      assert.are.equal("Carrot", rows[2].name)
    end)

    it("coerces the values it read to the types the sheet declares", function()
      local rows = db:fetch_sql(mydb.people, "SELECT * FROM people WHERE name = 'Bob'")
      assert.are.equal(3, rows[1].kills)
      assert.is_number(rows[1]._row_id)
      assert.is_number(rows[1].seen:as_number())
    end)

    it("returns an empty list rather than nil when nothing matched", function()
      local rows = db:fetch_sql(mydb.people, "SELECT * FROM people WHERE name = 'Nobody'")
      assert.are.same({}, rows)
    end)

    it("honours the SQL it is handed rather than fetching the whole sheet", function()
      local rows = db:fetch_sql(mydb.people, "SELECT * FROM people WHERE kills > 5")
      assert.are.equal(1, #rows)
      assert.are.equal("Carrot", rows[1].name)
    end)

    it("returns nil for SQL sqlite could not run", function()
      assert.is_nil(db:fetch_sql(mydb.people, "SELECT * FROM"))
      assert.is_nil(db:fetch_sql(mydb.people, "SELECT * FROM nosuchsheet"))
    end)
  end)

  describe("Tests db:_coerce", function()
    it("passes a raw expression through untouched", function()
      assert.are.equal("upper(name)", db:_coerce(mydb.people.name, db:exp("upper(name)")))
    end)

    it("renders db:Null as the NULL keyword", function()
      assert.are.equal("NULL", db:_coerce(mydb.people.name, db:Null()))
    end)

    it("leaves a number field's value as a number", function()
      assert.are.equal(7, db:_coerce(mydb.people.kills, 7))
      assert.are.equal(7, db:_coerce(mydb.people.kills, "7"))
    end)

    it("quotes a value a number field cannot hold", function()
      assert.are.equal("'lots'", db:_coerce(mydb.people.kills, "lots"))
    end)

    it("renders a datetime field's value through sqlite's datetime", function()
      assert.are.equal("datetime('now')", db:_coerce(mydb.people.seen, db:Timestamp("CURRENT_TIMESTAMP")))
      assert.are.equal("datetime('1234', 'unixepoch')", db:_coerce(mydb.people.seen, db:Timestamp(1234)))
      assert.are.equal("NULL", db:_coerce(mydb.people.seen, db:Timestamp(nil)))
    end)

    it("single quotes a text field's value and doubles up single quotes in it", function()
      assert.are.equal("'Bob'", db:_coerce(mydb.people.name, "Bob"))
      assert.are.equal("'it''s'", db:_coerce(mydb.people.name, "it's"))
    end)
  end)

  describe("Tests db:_coerce_sheet", function()
    it("returns nothing at all when there is no row", function()
      assert.is_nil(db:_coerce_sheet(mydb.people, nil))
    end)

    it("turns the sqlite text a row arrives as into the sheet's types", function()
      local row = db:_coerce_sheet(mydb.people, {_row_id = "4", name = "Bob", kills = "3", seen = "2020-01-02 03:04:05"})
      assert.are.equal(4, row._row_id)
      assert.are.equal(3, row.kills)
      assert.are.equal("Bob", row.name)
      assert.is_number(row.seen:as_number())
    end)

    it("leaves a number column that does not hold a number alone", function()
      local row = db:_coerce_sheet(mydb.people, {_row_id = "1", kills = "lots"})
      assert.are.equal("lots", row.kills)
    end)

    it("gives an empty datetime column an empty timestamp", function()
      local row = db:_coerce_sheet(mydb.people, {_row_id = "1", seen = nil}, {"seen"})
      assert.is_false(row.seen._timestamp)
      assert.is_nil((row.seen:as_number()))
    end)

    it("only converts the columns it is told about", function()
      local row = db:_coerce_sheet(mydb.people, {_row_id = "1", kills = "3", name = "Bob"}, {"name"})
      assert.are.equal("3", row.kills)
      assert.are.equal("Bob", row.name)
    end)
  end)

  describe("Tests db:_migrate", function()
    it("creates a sheet that the schema has but the file does not", function()
      db.__schema[dbName].pets = {columns = {name = "", legs = 0}, options = {}}
      db:_migrate(dbName, "pets")

      local pets = db:get_database(dbName).pets
      db:add(pets, {name = "Gaspode", legs = 4})
      local rows = db:fetch(pets)
      assert.are.equal(1, #rows)
      assert.are.equal(4, rows[1].legs)
    end)

    it("adds a column that the schema gained without losing the rows", function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
      db.__schema[dbName].people.columns.rank = ""
      db:_migrate(dbName, "people")

      local rows = db:fetch(db:get_database(dbName).people)
      assert.are.equal(1, #rows)
      assert.are.equal("Bob", rows[1].name)
      assert.are.equal("", rows[1].rank)
    end)

    it("runs again over an unchanged sheet without disturbing it", function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork", kills = 3})
      db:_migrate(dbName, "people")
      db:_migrate(dbName, "people")

      local rows = db:fetch(mydb.people)
      assert.are.equal(1, #rows)
      assert.are.equal(3, rows[1].kills)
    end)

    it("refuses to drop a column that still holds data unless forced", function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
      db.__schema[dbName].people.columns.city = nil

      local ok, err = pcall(function() db:_migrate(dbName, "people") end)
      assert.is_false(ok)
      assert.is_truthy(string.find(err, "data present in undefined columns", 1, true))
    end)

    it("drops that column when it is forced to", function()
      db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
      db.__schema[dbName].people.columns.city = nil
      db:_migrate(dbName, "people", true)

      local rows = db:fetch(db:get_database(dbName).people)
      assert.are.equal(1, #rows)
      assert.are.equal("Bob", rows[1].name)
      assert.is_nil(rows[1].city)
    end)

    it("rebuilds a sheet whose UNIQUE carries no conflict clause", function()
      -- sqlite defaults the conflict resolution to ABORT, so a sheet that this
      -- module did not write can hold a bare UNIQUE. Dropping _unique from the
      -- schema then has to rebuild the table, which it only does if the bare
      -- constraint is seen in the first place
      local schema = db.__schema[dbName].people
      local conn = db.__conn[dbName]

      schema.options._unique = {"name"}
      local legacy = db:_build_create_table_sql(schema, "people"):gsub(" ON CONFLICT %u+", "")
      assert.is_truthy(string.find(legacy, '"name" TEXT NULL DEFAULT "" UNIQUE', 1, true))
      conn:execute("DROP TABLE people")
      conn:execute(legacy)
      conn:commit()

      assert.is_true(db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"}))

      schema.options._unique = nil
      db:_migrate(dbName, "people")

      -- the uniqueness the schema no longer asks for is gone, and the row that
      -- was there came through the rebuild
      assert.is_true(db:add(mydb.people, {name = "Bob", city = "Lancre"}))
      local rows = db:fetch(db:get_database(dbName).people)
      assert.are.equal(2, #rows)
      assert.are.equal("Bob", rows[1].name)
    end)

    it("creates the indexes the schema asks for", function()
      local conn = db.__conn[dbName]
      conn:execute("DROP INDEX IF EXISTS " .. db:_index_name("people", "city"))
      assert.are.same({}, indexNames("people"))

      db:_migrate(dbName, "people")

      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)
  end)

  describe("Tests db:_drop_orphaned_indexes", function()
    it("keeps an index the schema still asks for", function()
      local schema = db.__schema[dbName].people
      local ok, err = db:_drop_orphaned_indexes(db.__conn[dbName], "people", schema)
      assert.is_true(ok)
      assert.is_nil(err)
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)

    it("drops every index once the schema asks for none", function()
      local schema = db.__schema[dbName].people
      schema.options._index = nil
      assert.is_true((db:_drop_orphaned_indexes(db.__conn[dbName], "people", schema)))
      assert.are.same({}, indexNames("people"))
    end)

    it("drops an index whose columns are no longer in the schema's index list", function()
      local schema = db.__schema[dbName].people
      schema.options._index = {"name"}
      assert.is_true((db:_drop_orphaned_indexes(db.__conn[dbName], "people", schema)))
      -- the city index is gone and the name one is not made here, only dropped
      assert.are.same({}, indexNames("people"))
    end)

    it("matches a compound index by its columns rather than by its name", function()
      local conn = db.__conn[dbName]
      conn:execute('CREATE INDEX IF NOT EXISTS idx_people_c_handmade ON people ("city", "name")')
      local schema = db.__schema[dbName].people
      schema.options._index = {{"name", "city"}}
      assert.is_true((db:_drop_orphaned_indexes(conn, "people", schema)))
      -- the column order differs and the name is nothing db would have picked,
      -- but the index covers what the schema asked for, so it stays
      assert.are.same({"idx_people_c_handmade"}, indexNames("people"))
    end)

    it("drops a unique index, which db does not make any more", function()
      local conn = db.__conn[dbName]
      conn:execute('CREATE UNIQUE INDEX IF NOT EXISTS idx_people_c_name ON people ("name")')
      local schema = db.__schema[dbName].people
      schema.options._index = {"name", "city"}
      assert.is_true((db:_drop_orphaned_indexes(conn, "people", schema)))
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)

    it("has nothing to do for a sheet that is not in the file", function()
      -- it asks sqlite_master which indexes the sheet has rather than the sheet
      -- itself, so an unknown sheet is an empty answer and not an error
      local ok, err = db:_drop_orphaned_indexes(db.__conn[dbName], "nosuchsheet", db.__schema[dbName].people)
      assert.is_true(ok)
      assert.is_nil(err)
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)
  end)

  describe("Tests db:_migrate_indexes", function()
    local columns = {name = "TEXT", city = "TEXT", kills = "REAL"}

    it("creates an index the sheet does not have yet", function()
      local conn = db.__conn[dbName]
      db:_migrate_indexes(conn, "people", {columns = {}, options = {_index = {"name"}}}, columns)
      assert.are.same({db:_index_name("people", "city"), db:_index_name("people", "name")}, indexNames("people"))
    end)

    it("creates a compound index under its compound name", function()
      local conn = db.__conn[dbName]
      db:_migrate_indexes(conn, "people", {columns = {}, options = {_index = {{"name", "city"}}}}, columns)
      assert.is_truthy(table.contains(indexNames("people"), db:_index_name("people", {"name", "city"})))
    end)

    it("skips an index that names a column the sheet does not have", function()
      -- silently, on purpose: db:create would otherwise be unable to run at all
      -- against a schema that lost a column
      local conn = db.__conn[dbName]
      db:_migrate_indexes(conn, "people", {columns = {}, options = {_index = {"nosuchcolumn"}}}, columns)
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)

    it("does nothing at all for a sheet with no indexes", function()
      local conn = db.__conn[dbName]
      db:_migrate_indexes(conn, "people", {columns = {}, options = {}}, columns)
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)

    it("runs again over an index that already exists without complaining", function()
      local conn = db.__conn[dbName]
      db:_migrate_indexes(conn, "people", {columns = {}, options = {_index = {"city"}}}, columns)
      assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    end)
  end)
end)

-- _index takes a single column name as well as a list of them, exactly as
-- _unique does. db:_index_name, db:_index_valid and db.Database:_drop each take
-- either shape, but db:_drop_orphaned_indexes walks _index with ipairs, so what
-- reaches db.__schema has to be the list.
describe("Tests db:create with a single column name as _index", function()
  local dbName = "indexstringtestingonly"
  local dbFile = getMudletHomeDir() .. "/Database_" .. dbName .. ".db"

  local function indexNames(sheetName)
    local conn = db.__conn[dbName]
    local cursor = conn:execute(
      "SELECT name FROM sqlite_master WHERE type = 'index' AND tbl_name = '" .. sheetName .. "' AND sql IS NOT NULL"
    )
    local names = {}
    local row = cursor:fetch({}, "a")
    while row do
      names[#names + 1] = row.name
      row = cursor:fetch({}, "a")
    end
    cursor:close()
    table.sort(names)
    return names
  end

  after_each(function()
    -- a db:create that raises part way through - which is exactly what this
    -- block guards against - leaves a cursor open, and closing then raises as
    -- well. Forget just this database in that case, so a regression here costs
    -- this block's specs rather than every db spec that runs after it.
    if not pcall(function() db:close(dbName) end) then
      db.__conn[dbName] = nil
    end
    os.remove(dbFile)
  end)

  it("creates the database instead of raising", function()
    local mydb
    local ok, err = pcall(function()
      mydb = db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    end)
    assert.is_true(ok, tostring(err))
    assert.is_table(mydb)
    db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
    assert.are.equal(1, #db:fetch(mydb.people))
  end)

  it("creates the index the string named", function()
    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("means exactly what the one entry list means", function()
    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    local fromString = indexNames("people")
    -- an empty list on both sides would compare equal without saying anything
    assert.are.equal(1, #fromString)
    db:close(dbName)
    os.remove(dbFile)

    db:create(dbName, {people = {name = "", city = "", _index = {"city"}}})
    assert.are.same(fromString, indexNames("people"))
  end)

  it("stores the string as a list, which is the shape every reader handles", function()
    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    assert.are.same({"city"}, db.__schema[dbName].people.options._index)
  end)

  it("keeps the index when the database is opened again", function()
    -- the second db:create runs the migration, and that is where an _index the
    -- migration cannot read costs you the index rather than an error
    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    db:close()

    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("still drops an index the string no longer names", function()
    db:create(dbName, {people = {name = "", city = "", _index = {"name", "city"}}})
    assert.are.equal(2, #indexNames("people"))
    db:close()

    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("keeps the rows that were already in the sheet", function()
    local mydb = db:create(dbName, {people = {name = "", city = "", _index = {"city"}}})
    db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
    db:close()

    mydb = db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    local rows = db:fetch(mydb.people)
    assert.are.equal(1, #rows)
    assert.are.equal("Bob", rows[1].name)
  end)

  it("takes a single column name for _unique at the same time", function()
    local mydb = db:create(dbName, {people = {name = "", city = "", _index = "city", _unique = "name"}})
    assert.is_true(db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"}))
    -- the index the string asked for, and only that one: the sqlite_autoindex
    -- the UNIQUE constraint builds has no sql of its own, so it is not listed
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("rebuilds the index when the sheet's constraints force a new table", function()
    -- a changed _violations makes db:_migrate drop and recreate the table,
    -- which takes every index with it; they come back from options._index, so
    -- this is the migration path that a string _index has to survive
    local mydb = db:create(dbName, {
      people = {name = "", city = "", _index = "city", _unique = "name", _violations = "FAIL"}
    })
    db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"})
    db:close(dbName)

    mydb = db:create(dbName, {
      people = {name = "", city = "", _index = "city", _unique = "name", _violations = "REPLACE"}
    })

    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
    local rows = db:fetch(mydb.people)
    assert.are.equal(1, #rows)
    assert.are.equal("Bob", rows[1].name)
  end)

  -- db:create reports an index it cannot make through printError and carries
  -- on, so what it says is collected rather than caught
  local function createCollectingWarnings(sheets)
    local collected = {}
    -- through _G: a spec file's globals are its own, so plain assignment would
    -- leave db:create with the printError it already has
    local originalPrintError = _G.printError
    _G.printError = function(msg) collected[#collected + 1] = msg end
    finally(function() _G.printError = originalPrintError end)

    local result = db:create(dbName, sheets)
    _G.printError = originalPrintError
    return result, table.concat(collected, "\n")
  end

  it("warns about a column the sheet does not have instead of refusing the sheet", function()
    -- an index on a column that is not there can never be created, but the rest
    -- of the sheet is sound and its data is reachable without the index
    db:create(dbName, {people = {name = "", city = "", _index = "city"}})
    assert.are.equal(1, #indexNames("people"))

    local mydb, warnings = createCollectingWarnings({people = {name = "", city = "", _index = "citty"}})
    assert.is_truthy(string.find(warnings, '_index names "citty", which is not one of the sheet\'s columns', 1, true))
    assert.is_true(db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"}))
    -- and the index the sheet had is still there: what is left of _index is no
    -- longer the whole set it asked for, so nothing is pruned against it
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))

    -- only for as long as the typo is there, though: a sheet that asks for an
    -- index it can have prunes the ones it no longer asks for, as ever
    db:create(dbName, {people = {name = "", city = "", _index = "name"}})
    assert.are.same({db:_index_name("people", "name")}, indexNames("people"))
  end)

  it("warns about a typo in a list or a compound index too", function()
    local mydb, warnings = createCollectingWarnings({people = {name = "", city = "", _index = {"city", "citty"}}})
    assert.is_table(mydb)
    assert.is_truthy(string.find(warnings, '_index names "citty"', 1, true))
    -- only the entry that names the column which is not there is dropped
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))

    mydb, warnings = createCollectingWarnings({people = {name = "", city = "", _index = {{"city", "citty"}}}})
    assert.is_table(mydb)
    assert.is_truthy(string.find(warnings, '_index names "citty"', 1, true))
    -- a compound index is wanted whole: an index on the half of it that names a
    -- real column is not the one that was asked for, and the single-column index
    -- the sheet already had is not dropped over it either
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("warns about a sort direction where a column name belongs", function()
    -- db:_sql_columns would render "name" desc, but db:_index_valid refuses the
    -- entry, so an index with a sort direction has never been created: saying so
    -- beats leaving the sheet with no index and no complaint
    local mydb, warnings = createCollectingWarnings({people = {name = "", city = "", _index = {{"name", "desc"}}}})
    assert.is_table(mydb)
    assert.is_truthy(string.find(warnings, "an index takes column names only, not a sort direction", 1, true))
    assert.are.same({}, indexNames("people"))

    -- a sheet that really has a column of that name is not warned about; what
    -- db:_sql_columns then makes of it is that function's business
    local other, otherWarnings = createCollectingWarnings({people = {name = "", desc = "", _index = {{"name", "desc"}}}})
    assert.is_table(other)
    assert.are.equal("", otherWarnings)
  end)

  it("warns about the _row_id no sheet definition names either", function()
    -- the sheet is given one, but no definition declares it: it is not there to
    -- index on the db:create that makes the sheet, and db:_drop_orphaned_indexes
    -- cannot match the leading underscore, so an index on it was dropped and
    -- made again on every db:create after that
    local mydb, warnings = createCollectingWarnings({people = {name = "", city = "", _index = "_row_id"}})
    assert.is_table(mydb)
    assert.is_truthy(string.find(warnings, '_index names "_row_id"', 1, true))
    assert.are.same({}, indexNames("people"))
  end)
end)

-- A sheet may be given as a list of its column names instead of a table of
-- names and defaults. The two forms take the same sheet options, which are keys
-- rather than list members in both.
describe("Tests db:create with a sheet given as a list of column names", function()
  local dbName = "indexarrayformtestingonly"
  local dbFile = getMudletHomeDir() .. "/Database_" .. dbName .. ".db"

  local function indexNames(sheetName)
    local conn = db.__conn[dbName]
    local cursor = conn:execute(
      "SELECT name FROM sqlite_master WHERE type = 'index' AND tbl_name = '" .. sheetName .. "' AND sql IS NOT NULL"
    )
    local names = {}
    local row = cursor:fetch({}, "a")
    while row do
      names[#names + 1] = row.name
      row = cursor:fetch({}, "a")
    end
    cursor:close()
    table.sort(names)
    return names
  end

  after_each(function()
    if not pcall(function() db:close(dbName) end) then
      db.__conn[dbName] = nil
    end
    os.remove(dbFile)
  end)

  it("takes the listed names as the columns, with no options among them", function()
    local mydb = db:create(dbName, {people = {"name", "city", _index = "city"}})
    assert.are.same({city = "", name = ""}, db.__schema[dbName].people.columns)
    assert.is_true(db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"}))
    assert.are.equal("Bob", db:fetch(mydb.people)[1].name)
  end)

  it("creates the index the list form asked for", function()
    db:create(dbName, {people = {"name", "city", _index = "city"}})
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("takes a list of index columns rather than raising", function()
    local ok, err = pcall(function()
      db:create(dbName, {people = {"name", "city", _index = {"city"}}})
    end)
    assert.is_true(ok, tostring(err))
    assert.are.same({db:_index_name("people", "city")}, indexNames("people"))
  end)

  it("takes _unique and _violations from the list form as well", function()
    local mydb = db:create(dbName, {people = {"name", "city", _unique = "name", _violations = "IGNORE"}})
    assert.are.equal("IGNORE", db.__schema[dbName].people.options._violations)
    -- and no column named after an option's value
    assert.are.same({city = "", name = ""}, db.__schema[dbName].people.columns)
    assert.is_true(db:add(mydb.people, {name = "Bob", city = "Ankh-Morpork"}))
    -- IGNORE rather than the default FAIL, so the second one is dropped quietly
    assert.is_true(db:add(mydb.people, {name = "Bob", city = "Lancre"}))
    assert.are.equal(1, #db:fetch(mydb.people))
  end)

  it("refuses a key that is neither a column name nor a sheet option", function()
    -- neither form on its own: "city" is keyed, and it is not a sheet option
    local ok, err = pcall(function()
      db:create(dbName, {people = {"name", city = ""}})
    end)
    assert.is_false(ok)
    assert.is_truthy(string.find(err, "city is neither one of the sheet's column names nor a sheet option", 1, true))
  end)

  it("refuses a listed column name that is not a string", function()
    -- it would otherwise reach the CREATE TABLE build, which raises from inside
    -- string.format naming neither the sheet nor the column
    local ok, err = pcall(function()
      db:create(dbName, {people = {"name", true}})
    end)
    assert.is_false(ok)
    assert.is_truthy(string.find(err, "column name #2 is a boolean", 1, true))
  end)
end)

-- db:_closeAll is what db:close() with no name does and what the profile calls
-- on shutdown, so the rest of this file already leans on it. These specs pin
-- the two things it reports and the state it leaves behind.
describe("Tests db:_closeAll", function()
  local first = "closealltestingonlyone"
  local second = "closealltestingonlytwo"

  local function makeDatabases()
    db:create(first, {sheet = {name = ""}})
    db:create(second, {sheet = {name = ""}})
  end

  after_each(function()
    -- the specs below leave the environment closed about half the time, and
    -- closing a closed one is an error rather than a no-op
    if db.__env then
      db:_closeAll()
    end
    os.remove(getMudletHomeDir() .. "/Database_" .. first .. ".db")
    os.remove(getMudletHomeDir() .. "/Database_" .. second .. ".db")
  end)

  it("closes every open database at once and says so", function()
    makeDatabases()
    local ok, msg = db:_closeAll()
    assert.is_true(ok)
    assert.are.equal("", msg)
    assert.are.same({}, db.__conn)
    assert.is_nil(db.__env)
  end)

  it("leaves the databases reopenable, with their rows intact", function()
    makeDatabases()
    local mydb = db:get_database(first)
    db:add(mydb.sheet, {name = "survivor"})
    db:_closeAll()

    local reopened = db:create(first, {sheet = {name = ""}})
    local rows = db:fetch(reopened.sheet)
    assert.are.equal(1, #rows)
    assert.are.equal("survivor", rows[1].name)
  end)

  it("refuses when there is no database environment to close", function()
    makeDatabases()
    db:_closeAll()
    local ok, msg = db:_closeAll()
    assert.is_false(ok)
    assert.are.equal("database environment is nil, did you forget to call db:create?", msg)
  end)

  it("names the database that was already closed behind its back", function()
    makeDatabases()
    db.__conn[first]:close()
    local ok, msg = db:_closeAll()
    assert.is_false(ok)
    assert.are.equal("database object for " .. first .. " is already closed.", msg)
    -- the rest still closed, and the environment is still gone
    assert.are.same({}, db.__conn)
    assert.is_nil(db.__env)
  end)

  it("is what db:close() with no name does", function()
    makeDatabases()
    assert.is_true((db:close()))
    assert.is_nil(db.__env)
  end)
end)
