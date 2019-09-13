describe("Tests TableUtils.lua functions", function()
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
end)
