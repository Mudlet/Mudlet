describe("Tests TableUtils.lua functions", function()

  describe("Tests the functionality of spairs", function()
    it("should sort by basic sorted keys by default", function()
      local tbl = { Tom = 40, Mary = 50, Joe = 24 }
      local expected = "Joe has 24 thingies\nMary has 50 thingies\nTom has 40 thingies\n"
      local actual = ""
      for name, thingies in spairs(tbl) do
        actual = actual .. string.format("%s has %d thingies\n", name, thingies)
      end
      assert.are.equal(expected,actual)
    end)

    it("should sort mixed keys by default", function()
      local tbl = { [1] = 30, Tom = 40, Mary = 50, Joe = 24 }
      local expected_key_order = { 1, "Joe", "Mary", "Tom" }

      local i = 1
      for k, _ in spairs(tbl) do
        assert.are.equal(k, expected_key_order[i])
        i = i + 1;
      end
    end)

    it("should sort based on a given function", function()
      local tbl = { Tom = 40, Mary = 50, Joe = 23 }
      local expected = "Joe has 23 thingies\nTom has 40 thingies\nMary has 50 thingies\n"
      local actual = ""
      for name, thingies in spairs(tbl, function(t,a,b) return t[a] < t[b] end) do --iterate from lowest value to highest
        actual = actual .. string.format("%s has %d thingies\n", name, thingies)
      end
      assert.are.equal(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.is_empty", function()
    it("Should return false if the table has an entry in it", function()
      assert.is_false(table.is_empty({"one"}))
      assert.is_false(table.is_empty({one = 1}))
    end)

    it("should return true if the table has no entries in it", function()
      assert.is_true(table.is_empty({}))
    end)

    it("Should error if passed a string", function()
      local errfn = function()
        table.is_empty("string")
      end
      assert.has_error(errfn, "table.is_empty: bad argument #1 type (table expected, got string!)")
    end)
  end)

  describe("Tests the functionality of table.n_filter", function()
    it("Should filter out small values", function()
      local function isBigEnough(value) return value >= 10 end
      local filtered = table.n_filter({12, 5, 8, 130, 44}, isBigEnough)
      assert.are.same(filtered, {12, 130, 44})
    end)

    it("Should filter out invalid entries", function()
      local invalidEntries = 0
      local entries = {
        { id = 15 }, { id = -1 }, { id = 0 }, { id = 3 },
        { id = 12.2 }, { }, { id = nil }, { id = false },
        { id = 'not a number' }
      }

      local function isNumber(t) return t and type(t) == 'number' end
      local function filterByID(item)
        if isNumber(item.id) and item.id ~= 0 then
          return true
        end
        invalidEntries = invalidEntries + 1
        return false
      end

      local entriesByID = table.n_filter(entries, filterByID)
      assert.are.equal(invalidEntries, 5)
      assert.are.same(entriesByID, {
        { id = 15 }, { id = -1 }, { id = 3 }, { id = 12.2 }
      })
    end)

    it("Should filter out content based on search criteria", function()
      local fruits = {'apple', 'banana', 'grapes', 'mango', 'orange'}
      local function filterItems(t, query)
        return table.n_filter(t, function(item)
          return item:lower():find(query:lower())
        end)
      end
      assert.are.same(filterItems(fruits, 'ap'), {'apple', 'grapes'})
      assert.are.same(filterItems(fruits, 'an'), {'banana', 'mango', 'orange'})
    end)
  end)

  describe("Tests the functionality of table.n_flatten", function()
    it("Should flatten nested tables", function()
      local t1 = {1, 2, {3, 4}};
      local t2 = {1, 2, {3, 4, {5, 6}}};
      local t3 = {1, 2, {3, 4, {5, 6, {7, 8, {9, 10}}}}};
      assert.are.same(table.n_flatten(t1), {1, 2, 3, 4})
      assert.are.same(table.n_flatten(t2), {1, 2, 3, 4, 5, 6})
      assert.are.same(table.n_flatten(t3), {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
    end)
  end)

  -- printTable, listPrint, __printTable, listAdd and listRemove are covered
  -- near the end of this file.

  describe("Tests the functionality of table.size", function()

    it("should return the same as #t for lists", function()
      local tbl = { "One", "Two", "Three" }
      assert.are.equals(table.size(tbl), #tbl)
    end)

    it("should return number of keys in a map", function()
      local tbl = {
        one = 1,
        two = 2,
        three = 3,
      }
      assert.are.equal(table.size(tbl), 3)
    end)

    it("should work with mixed string and number keys", function()
      local tbl = { "one", "Two", "three" }
      tbl.four = 4
      assert.are.equal(table.size(tbl), 4)
    end)
  end)

  describe("Tests the functionality of table.collect", function()
    it("should collect all key-value pairs from tbl for which func(key,value) returns true", function()
      local tbl = {
        this = "that",
        the = "other"
      }
      local func = function(key, value)
        if string.match(value, "%a") then return true end
      end
      local expected = {
        this = "that",
        the = "other"
      }
      local actual = table.collect(tbl, func)
      assert.are.same(expected, actual)
    end)

    it("should return an empty table if no items in tbl cause func(key,value) to return true", function()
      local tbl = {
        this = "that",
        the = "other"
      }
      local func = function(key, value)
        if string.match(value, "%d") then return true end
      end
      local expected = {}
      local actual = table.collect(tbl, func)
      assert.are.same(expected, actual)
    end)

    it("should have another test because the existing ones seem insufficient", function()
      local tbl = {
        hp = 99,
        mana = 30,
        endurance = 73,
        willpower = 13
      }
      local func = function(key,value)
        if value < 50 then return true end
      end
      local expected = {
        mana = 30,
        willpower = 13
      }
      local actual = table.collect(tbl, func)
      assert.are.same(expected, actual)
    end)

    it("should throw an error if you give a non-table as the first argument", function()
      local tbl = "not a table"
      local func = function() end
      local errfn = function()
        table.collect(tbl, func)
      end
      assert.has_error(errfn, "table.collect: bad argument #1 type (table to collect items from as table expected, got string)") 
    end)

    it("should throw an error if you give a non-function as the second argument", function()
      local tbl = {}
      local func = "function() end"
      local errfn = function()
        table.collect(tbl, func)
      end
      assert.has_error(errfn, "table.collect: bad argument #2 type (function to run against each item in tbl as function expected, got string)") 
    end)
  end)

  describe("Tests the functionality of table.n_collect", function()
    it("should return a table of unique values for which func(value) returns true", function()
      local tbl = {
        this = "that",
        the = "other",
        three = 3,
      }
      local func = function(value)
        if type(value) == "number" then return true end
      end
      local expected = { 3 }
      local actual = table.n_collect(tbl, func)
      assert.are.same(expected, actual)
    end)
    
    it("should return an empty table if no values return true", function()
      local tbl = {
        this = "that",
        the = "other"
      }
      local func = function(value)
        if type(value) == "number" then return true end
      end
      local expected = {}
      local actual = table.n_collect(tbl, func)
      assert.are.same(expected, actual)
    end)

    it("should work on lists as well as maps", function()
      local tbl = {
        10,
        20,
        25,
        53,
        1829,
        1800
      }
      local func = function(value)
        if value % 10 == 0 then return true end
      end
      local expected = {
        10,
        20,
        1800
      }
      local actual = table.n_collect(tbl, func)
      table.sort(actual)
      assert.are.same(expected,actual)
    end)

    it("should throw an error if you give a non-table as the first argument", function()
      local tbl = "not a table"
      local func = function() end
      local errfn = function()
        table.n_collect(tbl, func)
      end
      assert.has_error(errfn, "table.n_collect: bad argument #1 type (table to collect items from as table expected, got string)") 
    end)

    it("should throw an error if you give a non-function as the second argument", function()
      local tbl = {}
      local func = "function() end"
      local errfn = function()
        table.n_collect(tbl, func)
      end
      assert.has_error(errfn, "table.n_collect: bad argument #2 type (function to run against each item in tbl as function expected, got string)")
    end)

    it("should keep a value that is equal to an index already collected", function()
      local actual = table.n_collect({ "a", 1 }, function() return true end)
      table.sort(actual, function(a, b) return tostring(a) < tostring(b) end)
      assert.are.same({ 1, "a" }, actual)
    end)

    it("should keep a value that also appears inside a nested table", function()
      local actual = table.n_collect({ { "z" }, "z" }, function() return true end)
      assert.are.equal(2, #actual)
      local nested, plain
      for _, value in ipairs(actual) do
        if type(value) == "table" then nested = value else plain = value end
      end
      assert.are.same({ "z" }, nested)
      assert.are.equal("z", plain)
    end)

    it("should still drop real duplicates", function()
      local actual = table.n_collect({ 5, "x", 5, "x" }, function() return true end)
      assert.are.equal(2, #actual)
      table.sort(actual, function(a, b) return tostring(a) < tostring(b) end)
      assert.are.same({ 5, "x" }, actual)
    end)
  end)

  describe("Tests the functionality of table.matches", function()
    it("should return an empty table of no values math", function()
      local tbl = { 
        this = "that",
        the = "other"
       }
      local actual = table.matches(tbl, "%d")
      assert.is_true(table.is_empty(actual))
    end)

    it("should return a table containing all the items which match", function()
      local tbl = {
        this = "that",
        the = "other",
        number = "1234",
        one = "1"
      }
      local expected = {
        number = "1234",
        one = "1"
      }
      local actual = table.matches(tbl, "%d")
      assert.are.same(expected, actual)
      expected = {
        this = "that",
        the = "other"
      }
      actual = table.matches(tbl, "%a+")
      assert.are.same(expected, actual)
    end)

    it("should check both keys and values if check_keys is true", function()
      local tbl = {
        hp = 50,
        maxhp = 100,
        mana = 300,
        maxmana = 1000,
      }
      local expected = {
        hp = 50,
        maxhp = 100
      }
      local actual = table.matches(tbl, "hp", true)
      assert.are.same(expected, actual)
    end)

    it("should check multiple patterns if passed", function()
      local tbl = {
        hp = 50,
        mana = 50,
        wakefulness = "awake",
        title = "Lord High Muckity",
        name = "SuchAndSuch"
      }
      local expected = {
        hp = 50,
        mana = 50,
        title = "Lord High Muckity"
      }
      local actual = table.matches(tbl, "^%d+$", "title", true)
      assert.are.same(expected, actual)
    end)

    it("should throw an errow if the first parameter is not a table", function()
      local not_tbl = "not a table"
      local errfn = function()
        table.matches(not_tbl, "%d")
      end
      assert.has_error(errfn, "table.matches: bad argument #1 type (table to check using string.match as table expected, got string)") 
    end)

    it("should throw an error if the pattern passed is not a string", function()
      local tbl = {}
      local not_string = 4
      local errfn = function()
        table.matches(tbl, not_string)
      end
      assert.has_error(errfn, "table.matches: bad argument #2 type (pattern to check as string expected, got number)")
      errfn = function()
        table.matches(tbl, "a string", not_string)
      end
      assert.has_error(errfn, "table.matches: bad argument #3 type (pattern to check as string expected, got number)")
    end)

    it("should not error when things which are not strings or numbers are passed in", function()
      local tbl = {
        this = "that",
        test = "passed",
        tbl = {},
        func = function() end,
      }
      local expected = {
        test = "passed"
      }
      local actual = table.matches(tbl, "pass.+")
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.n_matches", function()
    it("should return a list of values which string.match a given pattern", function()
      local tbl = {
        this = "that",
        the = "other",
        [1] = "blue"
      }
      local expected = {
        "blue",
        "other"
      }
      local actual = table.n_matches(tbl, "e")
      table.sort(actual)
      assert.are.same(expected, actual)
    end)

    it("should only add any given value once", function()
      local tbl = {
        "this",
        "this",
        "that",
        "other",
        "something"
      }
      local expected = {
        "other",
        "something",
        "that",
        "this",
      }
      local actual = table.n_matches(tbl, "%a")
      table.sort(actual)
      assert.are.same(expected, actual)
    end)

    it("should error if the first argument is not a table", function()
      local not_tbl = "not a table"
      local errfn = function()
        table.n_matches(not_tbl, "%d")
      end
      assert.has_error(errfn, "table.n_matches: bad argument #1 type (table to check using string.match as table expected, got string)")
    end)

    it("should throw an error if the pattern passed is not a string", function()
      local tbl = {}
      local not_string = 4
      local errfn = function()
        table.n_matches(tbl, not_string)
      end
      assert.has_error(errfn, "table.n_matches: bad argument #2 type (pattern to check as string expected, got number)")
      errfn = function()
        table.n_matches(tbl, "a string", not_string)
      end
      assert.has_error(errfn, "table.n_matches: bad argument #3 type (pattern to check as string expected, got number)")
    end)

    it("should not error if it contains non-string, non-number values", function()
      local tbl = {
        this = "that",
        test = "passed",
        tbl = {},
        func = function() end,
      }
      local expected = { "passed" }
      local actual = table.n_matches(tbl, "pass.+")
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.contains", function()

    it("should return true if the table has a value that matches item", function()
      local tbl = { "One", "Two", "Three" }
      assert.is_true(table.contains(tbl, "One"))
    end)

    it("should return true if the table has a key which matches item", function()
      local tbl = {
        one = 1,
        two = 2
      }
      assert.is_true(table.contains(tbl, "one"))
    end)

    it("should check tables recursively", function()
      local tbl = {
        one = 1,
        two = 2,
        three = {
          {
            ludicrous = {
              levels = {
                of = {
                  buried = "beeblebrox"
                }
              }
            }
          }
        }
      }
      assert.is_true(table.contains(tbl, "beeblebrox"))
      assert.is_true(table.contains(tbl, "levels"))
    end)

    it("should return true if any of the passed arguments is in the table", function()
      local tbl = {
        one = 1,
        two = 2,
        three = {
          {
            ludicrous = {
              levels = {
                of = {
                  buried = "beeblebrox"
                }
              }
            }
          }
        }
      }
      assert.is_true(table.contains(tbl, "transparent", "things", "buried"))
    end)

    it("should return false if item is not in the table at all", function()
      local tbl = {
        one = 1,
        two = 2,
        three = {
          {
            ludicrous = {
              levels = {
                of = {
                  buried = "beeblebrox"
                }
              }
            }
          }
        }
      }
      assert.is_false(table.contains(tbl, "five"))
    end)

    it("should cope with a table that holds itself", function()
      local tbl = {one = 1}
      tbl.self = tbl
      assert.is_true(table.contains(tbl, "one"))
      assert.is_false(table.contains(tbl, "five"))
    end)

    it("should cope with a cycle between two tables", function()
      local first, second = {}, {}
      first.second = second
      second.first = first
      second.needle = "found me"
      assert.is_true(table.contains(first, "found me"))
      assert.is_false(table.contains(first, "not in here"))
    end)

    it("should cope with a Geyser object, which always holds itself", function()
      -- a label knows its container and the container's windowList knows the
      -- label, so this is the cycle ordinary scripts hit
      local label = Geyser.Label:new({
        name = "tableUtilsSpecCycleLabel", x = 0, y = 0, width = 50, height = 20,
      })
      finally(function() label:delete() end)
      -- the search below is only worth anything while that is really a cycle
      assert.are.equal(label, label.container.windowList[label.name])

      assert.is_true(table.contains(label, "tableUtilsSpecCycleLabel"))
      assert.is_false(table.contains(label, "no Geyser object holds this"))
    end)

  end)

  -- table.contains is a loop over table._contains, one pass per value it was
  -- asked about. Everything the search itself does lives in _contains, and it
  -- is the only one of the two that reports being handed something that is not
  -- a table: table.contains treats that report as "not found".
  describe("Tests the functionality of table._contains", function()

    it("should return true for a value in the table", function()
      assert.is_true(table._contains({"one", "two"}, "two"))
    end)

    it("should return true for a key in the table", function()
      assert.is_true(table._contains({one = 1, two = 2}, "two"))
    end)

    it("should find a value nested inside another table", function()
      assert.is_true(table._contains({outer = {inner = {"needle"}}}, "needle"))
      assert.is_true(table._contains({outer = {inner = {"needle"}}}, "inner"))
    end)

    it("should return false for something the table does not hold", function()
      assert.is_false(table._contains({one = 1}, "two"))
    end)

    it("should return false for an empty table", function()
      assert.is_false(table._contains({}, "anything"))
    end)

    it("should report being handed something that is not a table", function()
      local found, message = table._contains("not a table", "anything")
      assert.is_nil(found)
      assert.are.equal("first parameter passed isn't a table", message)

      found, message = table._contains(nil, "anything")
      assert.is_nil(found)
      assert.are.equal("first parameter passed isn't a table", message)
    end)

    it("should let table.contains turn that report into a plain false", function()
      -- the caller of table.contains never sees the message, so a script that
      -- wants to know it passed a table has to ask _contains
      assert.is_false(table.contains("not a table", "anything"))
    end)

    it("should search for exactly one value, unlike table.contains", function()
      -- table.contains loops over its extra arguments, _contains ignores them
      assert.is_false(table._contains({"one"}, "two", "one"))
      assert.is_true(table.contains({"one"}, "two", "one"))
    end)

    it("should find a false value stored in the table", function()
      -- returning the search result rather than the value found is what makes
      -- a stored false distinguishable from "not there"
      assert.is_true(table._contains({flag = false}, false))
      assert.is_false(table._contains({flag = true}, false))
    end)
  end)

  describe("Tests the functionality of table.index_of", function()
    it("should return the index of the item being searched", function()
      local tbl = {
        "one",
        "two",
        "three",
        4,
        function() end,
        false,
      }
      for index, item in pairs(tbl) do
        assert.equals(index, table.index_of(tbl, item))
      end
    end)

    it("should return nil if the item is not found in the table", function()
      local tbl = {
        "one",
        2,
        "three"
      }
      assert.equals(nil, table.index_of(tbl, 5))
    end)

    it("should only search the array part, so a hash value is not found", function()
      -- it walks with ipairs, so there is no index it could return for a keyed
      -- entry; table.contains is the function that finds those
      assert.is_nil(table.index_of({name = "found me"}, "found me"))
      assert.is_true(table.contains({name = "found me"}, "found me"))
    end)

    it("should stop at the first nil, so entries past a hole are not found", function()
      local sparse = {"one", nil, "three"}
      assert.equals(1, table.index_of(sparse, "one"))
      assert.is_nil(table.index_of(sparse, "three"))
    end)
  end)

  describe("Tests the functionality of table.deepcopy", function()
    setup(function()
      tblA = { "one", "two", 3, 4 }
    end)
    teardown(function()
      tblA = nil
    end)
    it("should produce a copy of the table", function()
      local tblB = table.deepcopy(tblA)
      assert.are.same(tblA, tblB)
    end)

    it("should produce an actual copy, not the original table reference", function()
      local tblB = table.deepcopy(tblA)
      assert.are_not.equal(tblA, tblB)
    end)
  end)

  describe("Tests the functionality of table.keys", function()
    setup(function()
      testtbl = { 
        one = 1,
        two = 2,
        this = "that",
        thing = {},
        otherThing = function() end,
        [1] = 1
      }
      sortfn = function(a,b) return tostring(a) < tostring(b) end
      expected = { "one", "two", "this", "thing", "otherThing", 1 }
      actual = table.keys(testtbl)
      table.sort(expected, sortfn)
      table.sort(actual, sortfn)
    end)
    teardown(function()
      testtbl = nil
      sortfn = nil
      expected = nil
    end)
    it("should return a table with all the keys from tbl", function()
      assert.are.same(expected, actual)
    end)

    it("should always return a table of the same size as tbl", function()
      local origSize = table.size(testtbl)
      local keysSize = table.size(actual)
      assert.equals(origSize, keysSize)
    end)
  end)

  describe("Tests the functionality of table.union", function()
    setup(function()
      tblA = {
        [1] = 123,
        [2] = 456,
        ["test"] = "test",
      }
      tblB = {
        [1] = 23,
        [3] = 7,
        ["test2"] = { "a", "b" },
      }
      tblC = {
        [5] = "c",
        ["hammer"] = "head",
      }
    end)
    teardown(function()
      tblA = nil
      tblB = nil
      tblC = nil
    end)
    it("should return the union of two simple tables without collisions", function()
     local expected = {
       [1] = 123,
       [2] = 456,
       [5] = "c",
       ["test"] = "test",
       ["hammer"] = "head",
     }
     local actual = table.union(tblA, tblC)
     assert.same(expected, actual)
    end)

    it("should return tables of values for keys which have value collisions", function()
      local expected = {
        [1] = { 123, 23 },
        [2] = 456,
        [3] = 7,
        ["test"] = 'test',
        ["test2"] = { "a", "b" }
      }
      local actual = table.union(tblA, tblB)
      assert.same(expected,actual)
    end)

    it("should work for more than two tables", function()
      local expected = {
        [1] = { 123, 23 },
        [2] = 456,
        [3] = 7,
        [5] = "c",
        ["test"] = 'test',
        ["test2"] = { "a", "b" },
        ["hammer"] = "head",
      }
      local actual = table.union(tblA, tblB, tblC)
      assert.same(expected,actual)
    end)

    it("should not modify a table it was given", function()
      local first = { key = { 1, 2 } }
      local actual = table.union(first, { key = 5 })
      assert.same({ { 1, 2 }, 5 }, actual.key)
      assert.same({ 1, 2 }, first.key)
      assert.is_false(rawequal(actual.key, first.key))
    end)

    it("should collect a colliding false into a subtable", function()
      local actual = table.union({ key = false }, { key = 7 })
      assert.same({ false, 7 }, actual.key)
    end)

    it("should append a third colliding value to the same subtable", function()
      local actual = table.union({ key = 1 }, { key = 2 }, { key = 3 })
      assert.same({ 1, 2, 3 }, actual.key)
    end)
  end)

  describe("Tests the functionality of table.n_union", function()
    setup(function()
      tblA = { "bob", "mary" }
      tblB = { "august", "justinian" }
      tblC = { 3, { "recursive", "tables" } }
      sortfn = function(a,b) return tostring(a) < tostring(b) end
    end)
    teardown(function()
      tblA = nil
      tblB = nil
      tblC = nil
      sortfn = nil
    end)
    it("should return the union of values between two lists", function()
      local expected = { "bob", "mary", "august", "justinian" }
      local actual = table.n_union(tblA, tblB)
      table.sort(expected, sortfn)
      table.sort(actual,sortfn)
      assert.same(expected,actual)
    end)

    it("should return the union of values between more than two lists", function()
      local expected = { "bob", "mary", "august", "justinian", 3, {"recursive", "tables"}}
      local actual = table.n_union(tblA, tblB, tblC)
      table.sort(expected, sortfn)
      table.sort(actual, sortfn)
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.intersection", function()
    it("should return the relative intersection of key value pairs of two tables", function()
      local t1 = {key = 1,1,2,3}
      local t2 = {key = 1,1,1,1}
      local expected = { key = 1, 1 }
      local actual = table.intersection(t1,t2)
      assert.same(expected, actual)
    end)

    it("should be able to do the same for three tables", function()
      local t1 = {key = 1,1,2,3}
      local t2 = {key = 1,1,1,3}
      local t3 = {key = 1,1,"two",3}
      local expected = { 
        key = 1,
        [1] = 1,
        [3] = 3
      }
      local actual = table.intersection(t1,t2,t3)
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.n_intersection", function()
    it("should produce a table which is the relative intersection of values of two tables", function()
      local t1 = {1,2,3,4,5,6}
      local t2 = {2,4,6,8}
      local expected = {2,4,6}
      local actual = table.n_intersection(t1,t2)
      assert.same(expected, actual)
    end)

    it("should produce a table which is the relative intersection of values of more than two tables", function()
      local t1 = {1,2,3,4,5,6}
      local t2 = {2,4,6,8}
      local t3 = {10, 2, 6}
      local expected = {2,6}
      local actual = table.n_intersection(t1,t2,t3)
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.complement", function()
    it("should return the complement of key value pairs of two maps", function()
      local t1 = {key = 1,1,2,3}
      local t2 = {key = 2,1,1,1}
      local expected = { key = 1,[2] = 2, [3] = 3 }
      local actual = table.complement(t1,t2)
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.n_complement", function()
    it("should return the complement of values of two lists", function()
      local t1 = {1,2,3,4,5,6}
      local t2 = {2,4,6}
      local expected = {1,3,5}
      local actual = table.n_complement(t1,t2)
      assert.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of table.update", function()
    it("should return a table that is tblA but with updated values from tblB", function()
      local tblA = {a = 1, b = 2, c = 3}
      local tblB = {b = 4}
      local expected = {a = 1, b = 4, c = 3}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should insert keys from tblB which do not exist in tblA", function()
      local tblA = {a = 1, b = 2, c = 3}
      local tblB = {b = 4, d = 10}
      local expected = {a = 1, b = 4, c = 3, d = 10}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    -- Tests for #8694: table.update should not error when t1 has non-table and t2 has table
    it("should replace non-table value with table value at same key", function()
      local tblA = {x = 1}
      local tblB = {x = {y = 2}}
      local expected = {x = {y = 2}}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should replace table value with non-table value at same key", function()
      local tblA = {x = {y = 2}}
      local tblB = {x = 1}
      local expected = {x = 1}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should merge nested tables when both have table at same key", function()
      local tblA = {x = {a = 1, b = 2}}
      local tblB = {x = {b = 3, c = 4}}
      local expected = {x = {a = 1, b = 3, c = 4}}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should handle string value being replaced by table", function()
      local tblA = {config = "old"}
      local tblB = {config = {setting = true}}
      local expected = {config = {setting = true}}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should handle boolean value being replaced by table", function()
      local tblA = {enabled = true}
      local tblB = {enabled = {feature1 = true, feature2 = false}}
      local expected = {enabled = {feature1 = true, feature2 = false}}
      local actual = table.update(tblA, tblB)
      assert.same(expected, actual)
    end)

    it("should return a new table and leave both arguments as they were", function()
      -- the name reads like an in-place update, so callers that rely on it
      -- returning a copy would be broken by a well meant optimisation
      local tblA = {a = 1}
      local tblB = {b = 2}
      local actual = table.update(tblA, tblB)
      assert.are_not.equal(tblA, actual)
      assert.are_not.equal(tblB, actual)
      assert.same({a = 1}, tblA)
      assert.same({b = 2}, tblB)
      actual.c = 3
      assert.is_nil(tblA.c)
      assert.is_nil(tblB.c)
    end)

    it("should merge nested tables into a new table rather than into tblA's", function()
      local tblA = {nested = {a = 1}}
      local tblB = {nested = {b = 2}}
      local actual = table.update(tblA, tblB)
      assert.same({a = 1, b = 2}, actual.nested)
      assert.same({a = 1}, tblA.nested)
      assert.are_not.equal(tblA.nested, actual.nested)
    end)
  end)

  describe("Tests the functionality of table.deepcopy nested independence", function()
    it("should copy nested tables so mutating the copy does not affect the original", function()
      local original = { a = 1, nested = { b = 2, deep = { c = 3 } } }
      local copy = table.deepcopy(original)
      copy.nested.b = 20
      copy.nested.deep.c = 30
      assert.equals(2, original.nested.b)
      assert.equals(3, original.nested.deep.c)
      -- the nested tables are distinct references
      assert.are_not.equal(original.nested, copy.nested)
      assert.are_not.equal(original.nested.deep, copy.nested.deep)
    end)

    it("should preserve the metatable of the copied table", function()
      local mt = { __index = function() return "default" end }
      local original = setmetatable({}, mt)
      local copy = table.deepcopy(original)
      assert.equals(mt, getmetatable(copy))
      assert.equals("default", copy.anything)
    end)

    it("should return non-table values unchanged", function()
      assert.equals(5, table.deepcopy(5))
      assert.equals("text", table.deepcopy("text"))
    end)

    it("should preserve the metatable of a nested table too", function()
      local mt = {__index = function() return "inherited" end}
      local original = {inner = setmetatable({}, mt)}
      local copy = table.deepcopy(original)
      assert.are_not.equal(original.inner, copy.inner)
      assert.equals(mt, getmetatable(copy.inner))
      assert.equals("inherited", copy.inner.anything)
    end)

    -- table._contains guards against this with a "seen" set; deepcopy has none
    pending("table.deepcopy copies a table that reaches itself - it recurses until the Lua stack overflows - issue #10414")
  end)

  describe("Tests the functionality of spairs on an empty table", function()
    it("should iterate zero times over an empty table", function()
      local count = 0
      for _ in spairs({}) do
        count = count + 1
      end
      assert.equals(0, count)
    end)
  end)

  describe("Tests the functionality of listAdd", function()
    it("should append an item to the end of the list", function()
      local list = { "one", "two" }
      listAdd(list, "three")
      assert.same({ "one", "two", "three" }, list)
    end)

    it("should append to an empty list", function()
      local list = {}
      listAdd(list, "only")
      assert.same({ "only" }, list)
    end)
  end)

  describe("Tests the functionality of listRemove", function()
    it("should remove a matching item from the list", function()
      local list = { "one", "two", "three" }
      listRemove(list, "two")
      assert.same({ "one", "three" }, list)
    end)

    it("should leave the list unchanged when the item is not present", function()
      local list = { "one", "two" }
      listRemove(list, "missing")
      assert.same({ "one", "two" }, list)
    end)

    it("should leave an empty list empty", function()
      local list = {}
      listRemove(list, "x")
      assert.same({}, list)
    end)

    it("should remove the sole element when it matches", function()
      local list = { "x" }
      listRemove(list, "x")
      assert.same({}, list)
    end)

    -- #9546: removal used to happen during an ipairs loop, so deleting index i
    -- shifted i+1 down into i, which the loop then skipped, leaving one of each
    -- run of consecutive duplicates behind.
    it("should remove a pair of consecutive duplicate matches", function()
      local list = { "a", "x", "x", "b" }
      listRemove(list, "x")
      assert.same({ "a", "b" }, list)
    end)

    it("should remove a run of three or more consecutive duplicates", function()
      local list = { "x", "x", "x" }
      listRemove(list, "x")
      assert.same({}, list)
    end)

    it("should remove every match whether the duplicates are adjacent or apart", function()
      local list = { "x", "a", "x", "x", "b", "x" }
      listRemove(list, "x")
      assert.same({ "a", "b" }, list)
    end)
  end)

  describe("Tests the contract of printTable", function()
    -- printTable/listPrint write to the screen via echo; we spy on the real
    -- echo (pass-through) to assert the framing lines without mocking it.
    it("should echo a header, a line per key/value pair and a footer", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      printTable({ alpha = "one", beta = "two" })
      -- header + 2 pairs + footer; header and footer are the same dashed string,
      -- so the count is what pins that both framing lines are present
      assert.spy(echo).was.called(4)
      assert.spy(echo).was.called_with("-------------------------------------------------------\n")
      assert.spy(echo).was.called_with("key=alpha value=one\n")
      assert.spy(echo).was.called_with("key=beta value=two\n")
    end)

    it("should render a value that is neither a string nor a number", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      local nested = {}
      printTable({ flag = true, nested = nested, fn = print })
      assert.spy(echo).was.called(5)
      assert.spy(echo).was.called_with("key=flag value=true\n")
      assert.spy(echo).was.called_with("key=nested value=" .. tostring(nested) .. "\n")
      assert.spy(echo).was.called_with("key=fn value=" .. tostring(print) .. "\n")
    end)

    it("should render a key that is neither a string nor a number", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      local key = {}
      printTable({ [key] = "one", [true] = "two" })
      assert.spy(echo).was.called(4)
      assert.spy(echo).was.called_with("key=" .. tostring(key) .. " value=one\n")
      assert.spy(echo).was.called_with("key=true value=two\n")
    end)

    it("should not raise on a table of mixed value types", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      assert.has_no.errors(function() printTable({ 1, "two", true, {}, print }) end)
      assert.has_no.errors(function() printTable({}) end)
    end)

    it("should name itself when it is not given a table", function()
      assert.has_error(function() printTable(nil) end,
        'printTable: bad argument #1 type (table expected, got nil!)')
      assert.has_error(function() listPrint("not a table") end,
        'listPrint: bad argument #1 type (table expected, got string!)')
    end)
  end)

  describe("Tests the contract of listPrint", function()
    it("should echo a numbered line for each list entry framed by dashed lines", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      listPrint({ "first", "second" })
      -- header + 2 entries + footer
      assert.spy(echo).was.called(4)
      assert.spy(echo).was.called_with("1. ) first\n")
      assert.spy(echo).was.called_with("2. ) second\n")
    end)

    it("should render entries that are neither strings nor numbers", function()
      local echo = spy.on(_G, "echo")
      finally(function() echo:revert() end)
      local nested = {}
      listPrint({ true, nested })
      assert.spy(echo).was.called(4)
      assert.spy(echo).was.called_with("1. ) true\n")
      assert.spy(echo).was.called_with("2. ) " .. tostring(nested) .. "\n")
    end)
  end)

  describe("Tests the contract of __printTable", function()
    -- __printTable is documented as printTable's helper but printTable never
    -- calls it; it is a standalone one pair formatter reachable from scripts,
    -- writing into the main console at the cursor
    it("should insert a newline terminated key and value pair", function()
      local insertText = spy.on(_G, "insertText")
      finally(function() insertText:revert() end)
      __printTable("alpha", "one")
      assert.spy(insertText).was.called(1)
      assert.spy(insertText).was.called_with("\nkey = alpha value = one")
    end)

    it("should tostring both the key and the value", function()
      local insertText = spy.on(_G, "insertText")
      finally(function() insertText:revert() end)
      __printTable(3, true)
      assert.spy(insertText).was.called_with("\nkey = 3 value = true")
    end)

    it("should land the pair in the main console buffer", function()
      clearWindow()
      echo("a line for the cursor to sit on\n")
      moveCursorEnd()
      __printTable("visible", "value")
      local text = table.concat(getLines("main", 0, getLastLineNumber("main") + 1), "\n")
      assert.is_truthy(text:find("key = visible value = value", 1, true))
    end)
  end)
end)
