describe("Tests TableUtils.lua functions", function()

  describe("table.is_empty(tbl)", function()
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

  describe("Tests table.n_filter() method", function()
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

  describe("Tests table.n_flatten() method", function()
    it("Should flatten nested tables", function()
      local t1 = {1, 2, {3, 4}};
      local t2 = {1, 2, {3, 4, {5, 6}}};
      local t3 = {1, 2, {3, 4, {5, 6, {7, 8, {9, 10}}}}};
      assert.are.same(table.n_flatten(t1), {1, 2, 3, 4})
      assert.are.same(table.n_flatten(t2), {1, 2, 3, 4, 5, 6})
      assert.are.same(table.n_flatten(t3), {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
    end)
  end)

  -- methods skipped here: printTable, _printTable, listPrint, listAdd, listRemove
  -- they are undocumented and unused in our own code.

  describe("table.size(tbl)", function()

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

  describe("table.contains(tbl, item)", function()

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

  end)

  describe("table.index_of(tbl,item)", function()
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

    it("should retrun nil if the item is not found in the table", function()
      local tbl = {
        "one",
        2,
        "three"
      }
      assert.equals(nil, table.index_of(tbl, 5))
    end)
  end)

  describe("table.deepcopy(tbl)", function()
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

  describe("table.keys(tbl)", function()
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

  describe("table.union(tblA, tblB, ...)", function()
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
  end)

  describe("table.n_union(tblA, tblB, ...)", function()
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

  describe("table.intersection(tblA, tblB, ...)", function()
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

  describe("table.n_intersection(tblA, tblB, ...)", function()
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

  describe("table.complement(tblA, tblB)", function()
    it("should return the complement of key value pairs of two maps", function()
      local t1 = {key = 1,1,2,3}
      local t2 = {key = 2,1,1,1}
      local expected = { key = 1,[2] = 2, [3] = 3 }
      local actual = table.complement(t1,t2)
      assert.same(expected, actual)
    end)
  end)

  describe("table.n_complement(tblA, tblB)", function()
    it("should return the complement of values of two lists", function()
      local t1 = {1,2,3,4,5,6}
      local t2 = {2,4,6}
      local expected = {1,3,5}
      local actual = table.n_complement(t1,t2)
      assert.same(expected, actual)
    end)
  end)

  describe("table.update(tblA, tblB)", function()
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
  end)
end)
