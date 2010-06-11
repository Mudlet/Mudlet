----------------------------------------------------------------------------------
--- Mudlet Unsorted Stuff
----------------------------------------------------------------------------------


-- some undocumented stuff
atcp = {}

walklist = {}
walkdelay = 0

SavedVariables = { }

-- Extending default libraries makes Babelfish happy.
setmetatable( _G, {
	["__call"] = function(func, ...)
		if type(func) == "function" then
			return func(...)
		else
			local h = metatable(func).__call
			if h then
				return h(func, ...)
			elseif _G[type(func)][func] then
				_G[type(func)][func](...)
			end
		end
	end,
	})




--- Sends a list of commands to the MUD. You can use this to send some things at once instead of having 
--- to use multiple send() commands one after another.
--- 
--- @param ... list of commands
--- @param echoTheValue optional boolean flag (default value is true) which determine if value should 
---   be echoed back on client.
---
--- @usage Use sendAll instead of multiple send commands.
---   <pre>
---   sendAll("stand", "wield shield", "say ha!")
---   
---   -- instead of calling:
---   send ("stand")
---   send ("wield shield")
---   send ("say ha!")
---   </pre>
function sendAll(...)
	local args = {...}
	local echo = true
	if type(args[#args]) == 'boolean' then
		echo = table.remove(args, #args)
	end
	for i, v in ipairs(args) do
		if type(v) == 'string' then send(v, echo) end
	end
end


--- Checks to see if a given file or folder exists. If it exists, it’ll return the Lua true boolean value, otherwise false.
--- 
--- @usage
---   <pre>
---   if io.exists("/home/user/Desktop") then
---      echo("This folder exists!")
---   else
---      echo("This folder doesn't exist.")
---   end
---   
---   if io.exists("/home/user/Desktop/file.txt") then
---      echo("This file exists!")
---   else
---      echo("This file doesn't exist.")
---   end
---   </pre>
---   
--- @return true or false
function io.exists(fileOfFolderName)
	local f = io.open(fileOfFolderName)
	if f then
		io.close(f)
		return true
	end
	return false
end


--- Boolean exclusive or implementation.
---
--- @usage All following will return false.
---   <pre>
---   xor(false, false)
---   xor(true, true)
---   </pre>
---   
--- @return true or false
function xor(a, b)
	if (a and (not b)) or (b and (not a)) then
		return true
	else
		return false
	end
end


--- Determine operating system.
---
--- @usage
---   <pre>
---   if "linux" == getOS() then 
---	     echo("We are using GNU/Linux!")
---   end
---   </pre>
---
--- @return "linux", "mac" or "windows" string
function getOS()
	if string.char(getMudletHomeDir():byte()) == "/" then
		if string.find(os.getenv("HOME"), "home") == 2 then
			return "linux"
		else
			return "mac"
		end
	else
		return "windows"
	end
end


--- Opens the default OS browser for the given URL.
---
--- @usage openUrl("www.mudlet.org")
--- @usage openUrl("http://www.mudlet.org/")
function openURL(url)
	local os = getOS()
	if os == "linux" then _G.os.execute("xdg-open " .. url)
	elseif os == "mac" then _G.os.execute("open " .. url)
	elseif os == "windows" then _G.os.execute("start " .. url) end
end


--- This function flags a variable to be saved by Mudlet's variable persistence system.
--- Variables are automatically unpacked into the global namespace when the profile is loaded.
--- They are saved to "SavedVariables.lua" when the profile is closed or saved.
---
--- @usage remember("varName")
---
--- @see loadVars
function remember(varName)
	if not _saveTable then
		_saveTable = {}
	end
    _saveTable[varName] = _G[varName]
end


--- This function should be primarily used by Mudlet. It loads saved settings in from the Mudlet home directory
--- and unpacks them into the global namespace.
---
--- @see remember
function loadVars()
	if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
	local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
	local lt_VariableHolder = {}
	if (io.exists(l_SettingsFile)) then
		table.load(l_SettingsFile, lt_VariableHolder)
		for k,v in pairs(lt_VariableHolder) do
				_G[k] = v
		end
	end
end


--- This function should primarily be used by Mudlet. It saves the contents of _saveTable into a file for persistence.
---
--- @see loadVars
function saveVars()
	if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
	local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
    for k,_ in pairs(_saveTable) do
        remember(k)
    end
	table.save(l_SettingsFile, _saveTable)
end


--- <b><u>TODO</u></b> 
--- Save & Load Variables.
--- <pre>
--- The below functions can be used to save individual Lua tables to disc and load 
--- them again at a later time e.g. make a database, collect statistical information etc. 
--- These functions are also used by Mudlet to load & save the entire Lua session variables 
--- 
--- table.load(file)   - loads a serialized file into the globals table (only Mudlet should use this) 
--- table.load(file, table) - loads a serialized file into the given table 
--- table.save(file)  - saves the globals table (minus some lua enviroment stuffs) into a file (only Mudlet should use this) 
--- table.save(file, table) - saves the given table into the given file 
--- 
--- Original code written by CHILLCODE™ on https://board.ptokax.ch, distributed under the same terms as Lua itself. 
--- 
--- Notes: 
---  Userdata and indices of these are not saved 
---  Functions are saved via string.dump, so make sure it has no upvalues 
---  References are saved 
--- </pre>
function table.save( sfile, t )
	if t == nil then 
		t = _G 
	end
	local tables = {}
	table.insert( tables, t )
	local lookup = { [t] = 1 }
	local file = io.open( sfile, "w" )
	file:write( "return {" )
	for i,v in ipairs( tables ) do
		table.pickle( v, file, tables, lookup )
	end
	file:write( "}" )
	file:close()
end


--- <b><u>TODO</u></b> table.pickle( t, file, tables, lookup )
function table.pickle( t, file, tables, lookup )
	file:write( "{" )
	for i,v in pairs( t ) do
		-- escape functions
		if type( v ) ~= "function" and type( v ) ~= "userdata" and (i ~= "string" and i ~= "xpcall" and i ~= "package" and i ~= "os" and i ~= "io" and i ~= "math" and i ~= "debug" and i ~= "coroutine" and i ~= "_G" and i ~= "_VERSION" and i ~= "table") then
			-- handle index
			if type( i ) == "table" then
				if not lookup[i] then
					table.insert( tables, i )
					lookup[i] = table.maxn( tables )
				end
				file:write( "[{"..lookup[i].."}] = " )
			else
				local index = ( type( i ) == "string" and "[ "..string.enclose( i, 50 ).." ]" ) or string.format( "[%d]", i )
				file:write( index.." = " )
			end
			-- handle value
			if type( v ) == "table" then
				if not lookup[v] then
					table.insert( tables, v )
					lookup[v] = table.maxn( tables )
				end
				file:write( "{"..lookup[v].."}," )
			else
				local value =  ( type( v ) == "string" and string.enclose( v, 50 ) ) or tostring( v )
				file:write( value.."," )
			end
		end
	end
	file:write( "},\n" )
end


--- <b><u>TODO</u></b> table.load( sfile, loadinto )
--- @see table.save
function table.load( sfile, loadinto )
	local tables = dofile( sfile )
	if tables then
		if loadinto ~= nil and type(loadinto) == "table" then
			table.unpickle( tables[1], tables, loadinto )
		else
			table.unpickle( tables[1], tables, _G )
		end
	end
end


--- <b><u>TODO</u></b> table.unpickle( t, tables, tcopy, pickled )
function table.unpickle( t, tables, tcopy, pickled )
	pickled = pickled or {}
	pickled[t] = tcopy
	for i,v in pairs( t ) do
		local i2 = i
		if type( i ) == "table" then
			local pointer = tables[ i[1] ]
			if pickled[pointer] then
				i2 = pickled[pointer]
			else
				i2 = {}
				table.unpickle( pointer, tables, i2, pickled )
			end
		end
		local v2 = v
		if type( v ) == "table" then
			local pointer = tables[ v[1] ]
			if pickled[pointer] then
				v2 = pickled[pointer]
			else
				v2 = {}
				table.unpickle( pointer, tables, v2, pickled )
			end
		end
		tcopy[i2] = v2
	end
end


--- <b><u>TODO</u></b> speedwalktimer()
function speedwalktimer()
	send(walklist[1])
	table.remove(walklist, 1)
	if #walklist>0 then
		tempTimer(walkdelay, [[speedwalktimer()]])
	end
end


--- <b><u>TODO</u></b> speedwalk(dirString, backwards, delay)
function speedwalk(dirString, backwards, delay)
	local dirString		= dirString:lower()
	walklist			= {}
	walkdelay			= delay
	local reversedir	= {
		n	= "s",
		en	= "sw",
		e	= "w",
		es	= "nw",
		s	= "n",
		ws	= "ne",
		w	= "e",
		wn	= "se",
		u	= "d",
		d	= "u",
		ni	= "out",
		tuo	= "in"
	}
	if not backwards then
		for count, direction in string.gmatch(dirString, "([0-9]*)([neswudio][ewnu]?t?)") do
			count = (count == "" and 1 or count)
			for i=1, count do
				if delay then walklist[#walklist+1] = direction
				else send(direction)
				end
			end
		end
	else
		for direction, count in string.gmatch(dirString:reverse(), "(t?[ewnu]?[neswudio])([0-9]*)") do
			count = (count == "" and 1 or count)
			for i=1, count do
				if delay then walklist[#walklist+1] = reversedir[direction]
				else send(reversedir[direction])
				end
			end
		end
	end
	if walkdelay then 
		speedwalktimer() 
	end
end


--- <b><u>TODO</u></b> _comp(a, b)
function _comp(a, b)
	if type(a) ~= type(b) then return false end
	if type(a) == 'table' then
		for k, v in pairs(a) do
			if not b[k] then return false end
			if not _comp(v, b[k]) then return false end
		end
	else
		if a ~= b then return false end
	end
	return true
end


--- <b><u>TODO</u></b> SavedVariables:Add(tbl)
function SavedVariables:Add(tbl)
	if type(tbl) == 'string' then
		self[tbl] = _G[tbl]
	elseif type(tbl) == 'table' then
		for k,v in pairs(_G) do
			if _comp(v, tbl) then
				self[k] = tbl
			end
		end
	else
		hecho"|cff0000Error registering table for persistence: invalid argument to SavedVariables:Add()"
	end
end


--- <b><u>TODO</u></b> phpTable(...) - abuse to: http://richard.warburton.it
function phpTable(...)
	local newTable, keys, values = {}, {}, {}
	newTable.pairs = function(self) -- pairs iterator
		local count = 0
		return function()
			count = count + 1
			return keys[count], values[keys[count]]
		end
	end
	setmetatable(newTable, {
		__newindex = function(self, key, value)
			if not self[key] then table.insert(keys, key)
			elseif value == nil then -- Handle item delete
				local count = 1
				while keys[count] ~= key do count = count + 1 end
				table.remove(keys, count)
			end
			values[key] = value -- replace/create
		end,
		__index=function(self, key) return values[key] end,
		isPhpTable = true,
	})
	local args = {...}
	for x=1, #args do
		for k, v in pairs(args[x]) do newTable[k] = v end
	end
	return newTable
end

