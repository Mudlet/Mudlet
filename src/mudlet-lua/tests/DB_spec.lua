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
