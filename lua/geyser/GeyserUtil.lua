--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Generate a window name unique to this session.
function Geyser.nameGen (type)
   local index = Geyser.i
   local t = type or "window"
   Geyser.i = Geyser.i + 1
   return "anon_" .. t .. "_" .. index
end

--- Hide all managed windows.
function Geyser.hideAll(type)
   for _,v in pairs(Geyser.windowList) do
      if v.type == type or not type then
         v:hide()
      end
   end
end

--- Show all managed windows.
function Geyser.showAll(type)
   for _,v in pairs(Geyser.windowList) do
      if v.type == type or not type then
         v:show()
      end
   end
end

--- Non-recursive display of an item, because the normal 'display' was
-- causing Mudlet to hang. Not sure why.
function Geyser.display (table)
	echo("------ " .. type(table) .. " ------\n")
	if type(table) == "table" then
		for k,v in pairs(table) do
			echo("'" .. tostring(k) .. "' - " .. tostring(v) .. "\n")
		end
	else
		echo(tostring(table) .. "\n")
	end
end

--- Clone a table, for good fun and profit.
function Geyser.copyTable (table)
	local copy = {}
    if table then
		for k,v in pairs(table) do
			-- do deep copy on a table if it requests one by having the __clone function defined.
			if type(v) == "table" and v.__clone then
				copy[k] = v.__clone()
			else
				copy[k] = v
			end
		end
	end
	return copy
end
