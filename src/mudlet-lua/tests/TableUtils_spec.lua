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
    pending("should have some tests", function()
    end)
  end)

  describe("table.n_union(tblA, tblB, ...)", function()
    pending("should have some tests", function()
    end)
  end)

  describe("table.intersection(tblA, tblB, ...)", function()
    pending("should have some tests", function()
    end)
  end)

  describe("table.n_intersection(tblA, tblB, ...)", function()
    pending("should have some tests", function()
    end)
  end)

  describe("table.complement(tblA, tblB)", function()
    pending("should have some tests", function()
    end)
  end)

  describe("table.n_complement(tblA, tblB)", function()
    pending("should have some tests", function()
    end)
  end)

  describe("table.update(tblA, tblB)", function()
    pending("should have some tests", function()
    end)
  end)
end)
