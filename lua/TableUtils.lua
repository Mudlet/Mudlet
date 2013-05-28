----------------------------------------------------------------------------------
--- Mudlet Table Utils
----------------------------------------------------------------------------------


--- Tests if a table is empty: this is useful in situations where you find
--- yourself wanting to do 'if my_table == {}' and such.
---
--- @usage Testing if the table is empty.
---   <pre>
---   myTable = {}
---   if table.is_empty(myTable) then
---      echo("myTable is empty")
---   end
---   </pre>
function table.is_empty(tbl)
	if next(tbl) == nil then return true else return false end
end



--- Lua debug function that prints the content of a Lua table on the screen, split up in keys and values.
--- Useful if you want to see what the capture groups contain i. e. the Lua table "matches".
---
--- @see display
function printTable( map )
	echo("-------------------------------------------------------\n");
	for k, v in pairs( map ) do
		echo( "key=" .. k .. " value=" .. v .. "\n" )
	end
	echo("-------------------------------------------------------\n");
end



-- NOT LUADOC
-- This is supporting function for printTable().
function __printTable( k, v )
	insertText ("\nkey = " .. tostring (k) .. " value = " .. tostring( v )  )
end



--- Lua debug function that prints the content of a Lua table on the screen. <br/>
--- There are currently 3 functions with similar behaviour.
---
--- @see display
--- @see printTable
function listPrint( map )
	echo("-------------------------------------------------------\n");
	for k,v in ipairs( map ) do
		echo( k .. ". ) "..v .. "\n" );
	end
	echo("-------------------------------------------------------\n");
end



--- <b><u>TODO</u></b> listAdd( list, what )
function listAdd( list, what )
	table.insert( list, what );
end



--- <b><u>TODO</u></b> listRemove( list, what )
function listRemove( list, what )
	for k,v in ipairs( list ) do
		if v == what then
			table.remove( list, k )
		end
	end
end



--- Gets the actual size of non-index based tables. <br/><br/>
---
--- For index based tables you can get the size with the # operator: <br/>
--- This is the standard Lua way of getting the size of index tables i.e. ipairs() type of tables with
--- numerical indices. To get the size of tables that use user defined keys instead of automatic indices
--- (pairs() type) you need to use the function table.size() referenced above.
---   <pre>
---   myTableSize = # myTable
---   </pre>
function table.size(t)
	if not t then
		return 0
	end
	local i = 0
	for k, v in pairs(t) do
		i = i + 1
	end
	return i
end



--- Determines if a table contains a value as a key or as a value (recursive).
function table.contains(t, value)
	for k, v in pairs(t) do
		if v == value then
			return true
		elseif k == value then
			return true
		elseif type(v) == "table" then
			if table.contains(v, value) then return true end
		end
	end
	return false
end



--- Table Union.
---
--- @return Returns a table that is the union of the provided tables. This is a union of key/value
---   pairs. If two or more tables contain different values associated with the same key,
---   that key in the returned table will contain a subtable containing all relevant values.
---   See table.n_union() for a union of values. Note that the resulting table may not be
---   reliably traversable with ipairs() due to the fact that it preserves keys. If there
---   is a gap in numerical indices, ipairs() will cease traversal.
---
--- @usage Example:
---   <pre>
---   tableA = {
---      [1] = 123,
---      [2] = 456,
---      ["test"] = "test",
---   }
---
---   tableB = {
---      [1] = 23,
---      [3] = 7,
---      ["test2"] = function() return true end,
---   }
---
---   tableC = {
---      [5] = "c",
---   }
---
---   table.union(tableA, tableB, tableC) will return:
---   {
---      [1] = {
---         123,
---         23,
---      },
---      [2] = 456,
---      [3] = 7,
---      [5] = "c",
---      ["test"] = "test",
---      ["test2"] = function() return true end,
---   }
---   </pre>
function table.union(...)
	local sets = {...}
	local union = {}

	for _, set in ipairs(sets) do
		for key, val in pairs(set) do
			if union[key] and union[key] ~= val then
				if type(union[key]) == 'table' then
					table.insert(union[key], val)
				else
					union[key] = { union[key], val }
				end
			else
				union[key] = val
			end
		end
	end

	return union
end



--- Table Union.
---
--- @return Returns a numerically indexed table that is the union of the provided tables. This is
---   a union of unique values. The order and keys of the input tables are not preserved.
function table.n_union(...)
	local sets = {...}
	local union = {}
	local union_keys = {}

	for _, set in ipairs(sets) do
		for key, val in pairs(set) do
			if not union_keys[val] then
				union_keys[val] = true
				table.insert(union, val)
			end
		end
	end

	return union
end



--- Table Intersection.
---
--- @return Returns a table that is the intersection of the provided tables. This is an
---   intersection of key/value pairs. See table.n_intersection() for an intersection of values.
---   Note that the resulting table may not be reliably traversable with ipairs() due to
---   the fact that it preserves keys. If there is a gap in numerical indices, ipairs() will
---   cease traversal.
---
--- @usage Example:
---   <pre>
---   tableA = {
---      [1] = 123,
---      [2] = 456,
---      [4] = { 1, 2 },
---      [5] = "c",
---      ["test"] = "test",
---   }
---
---   tableB = {
---      [1] = 123,
---      [2] = 4,
---      [3] = 7,
---      [4] = { 1, 2 },
---      ["test"] = function() return true end,
---   }
---
---   tableC = {
---      [1] = 123,
---      [4] = { 1, 2 },
---      [5] = "c",
---   }
---
---   table.intersection(tableA, tableB, tableC) will return:
---   {
---      [1] = 123,
---      [4] = { 1, 2 },
---   }
---   </pre>
function table.intersection(...)
	sets = {...}
	if #sets < 2 then return false end

	local intersection = {}

	local function intersect(set1, set2)
		local result = {}
		for key, val in pairs(set1) do
			if set2[key] then
				if _comp(val, set2[key]) then result[key] = val end
			end
		end
		return result
	end

	intersection = intersect(sets[1], sets[2])

	for i, _ in ipairs(sets) do
		if i > 2 then
			intersection = intersect(intersection, sets[i])
		end
	end

	return intersection
end



--- Table Intersection.
---
--- @return Returns a numerically indexed table that is the intersection of the provided tables.
---   This is an intersection of unique values. The order and keys of the input tables are
---   not preserved.
function table.n_intersection(...)
	sets = {...}
	if #sets < 2 then return false end

	local intersection = {}

	local function intersect(set1, set2)
		local intersection_keys = {}
		local result = {}
		for _, val1 in pairs(set1) do
			for _, val2 in pairs(set2) do
				if _comp(val1, val2) and not intersection_keys[val1] then
					table.insert(result, val1)
					intersection_keys[val1] = true
				end
			end
		end
		return result
	end

	intersection = intersect(sets[1], sets[2])

	for i, _ in ipairs(sets) do
		if i > 2 then
			intersection = intersect(intersection, sets[i])
		end
	end

	return intersection
end



--- Table Complement.
---
--- @return Returns a table that is the relative complement of the first table with respect to
---   the second table. Returns a complement of key/value pairs.
function table.complement(set1, set2)
	if not set1 and set2 then return false end
	if type(set1) ~= 'table' or type(set2) ~= 'table' then return false end

	local complement = {}

	for key, val in pairs(set1) do
		if not _comp(set2[key], val) then
			complement[key] = val
		end
	end
	return complement
end



--- Table Complement.
---
--- @return Returns a table that is the relative complement of the first table with respect to
---   the second table. Returns a complement of values.
function table.n_complement(set1, set2)
	if not set1 and set2 then return false end

	local complement = {}

	for _, val1 in pairs(set1) do
		local insert = true
		for _, val2 in pairs(set2) do
			if _comp(val1, val2) then
				insert = false
			end
		end
		if insert then table.insert(complement, val1) end
	end

	return complement
end


function table.update(t1, t2)
        local tbl = {}
        for k,v in pairs(t1) do
                tbl[k] = v
        end
        for k,v in pairs(t2) do
                if type(v) == "table" then
                        tbl[k] = table.update(tbl[k] or {}, v)
                else
                        tbl[k] = v
                end
        end
        return tbl
end

---Returns the index of the value in a table
function table.index_of(table, element)
  for index, value in ipairs(table) do
    if value == element then
      return index
    end
  end
  return nil
end
