----------------------------------------------------------------------------------
--- Mudlet Unsorted Stuff
----------------------------------------------------------------------------------


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



--- Mudlet's support for ATCP. This is primarily available on IRE-based MUDs, but Mudlets impelementation is generic enough
--- such that any it should work on others. <br/><br/>
---
--- The latest ATCP data is stored in the atcp table. Whenever new data arrives, the previous is overwritten. An event is also
--- raised for each ATCP message that arrives. To find out the available messages available in the atcp table and the event names,
--- you can use display(atcp). <br/><br/>
---
--- Note that while the typical message comes in the format of Module.Submodule, ie Char.Vitals or Room.Exits, in Mudlet the dot is
--- removed - so it becomes CharVitals and RoomExits. Here's an example:
---   <pre>
---   room_number = tonumber(atcp.RoomNum)
---   echo(room_number)
---   </pre>
---
--- Triggering on ATCP events: <br/>
--- If you'd like to trigger on ATCP messages, then you need to create scripts to attach handlers to the ATCP messages.
--- The ATCP handler names follow the same format as the atcp table - RoomNum, RoomExits, CharVitals and so on. <br/><br/>
---
--- While the concept of handlers for events is to be explained elsewhere in the manual, the quick rundown is this - place
--- the event name you'd like your script to listen to into the Add User Defined Event Handler: field and press the + button
--- to register it. Next, because scripts in Mudlet can have multiple functions, you need to tell Mudlet which function
--- should it call for you when your handler receives a message. You do that by setting the Script name: to the function
--- name in the script you'd like to be called. <br/><br/>
---
--- For example, if you'd like to listen to the RoomExits event and have it call the process_exits() function -
--- register RoomExits as the event handler, make the script name be process_exits, and use this in the script:
---   <pre>
---   function process_exits(event, args)
---       echo("Called event: " .. event .. "\nWith args: " .. args)
---   end
---   </pre>
---
--- Feel free to experiment with this to achieve the desired results. A ATCP demo package is also available on the forums
--- for using event handlers and parsing its messages into Lua datastructures. <br/>
---
--- @release Mudlet 1.0.6
---
--- @see sendATCP
---
--- @class function
--- @name atcp
atcp = {}



--- <b><u>TODO</u></b> Table walklist.
---
--- @class function
--- @name walklist
walklist = {}



--- <b><u>TODO</u></b> Variable walkdelay.
---
--- @class function
--- @name walkdelay
walkdelay = 0



--- <b><u>TODO</u></b> Table SavedVariables.
---
--- @class function
--- @name SavedVariables
SavedVariables = {}


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
---   </pre>
---   Instead of calling:
---   <pre>
---   send ("stand")
---   send ("wield shield")
---   send ("say ha!")
---   </pre>
--- @usage Use sendAll and do not echo sent commnad on the main window.
---   <pre>
---   sendAll("stand", "wield shield", "say ha!", false)
---   </pre>
---
--- @see send
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


--- Creates a group of a given type that will persist through sessions.
---
--- @param name name of the teim
--- @param itemtype type of the item - can be trigger, alias, or timer
---
--- @usage
--- <pre>
---   --create a new trigger group
---   permGroup("Combat triggers", "trigger")
--- </pre>
--- @usage
--- <pre>
---   --create a new alias group only if one doesn't exist already
---   if exists("Defensive aliases", "alias") == 0 then
---     permGroup("Defensive aliases", "alias")
---   end
--- </pre>
function permGroup(name, itemtype)
  assert(type(name) == "string", "permGroup: need a name for the new thing")

  local t = {
    timer = function(name)
        return (permTimer(name, "", 0, "") == -1) and false or true
       end,
    trigger = function(name)
        return (permSubstringTrigger(name, "", {""}, "") == -1) and false or true
      end,
    alias = function(name)
        return (permAlias(name, "", "", "") == -1) and false or true
      end
 }

 assert(t[itemtype], "permGroup: "..tostring(itemtype).." isn't a valid type")

 return t[itemtype](name)
end



--- Checks to see if a given file or folder exists. If it exists, it'll return the Lua true boolean value, otherwise false.
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



--- Implementation of boolean exclusive or.
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



--- The below functions (table.save, table.load) can be used to save individual Lua tables to disc and load
--- them again at a later time e.g. make a database, collect statistical information etc.
--- These functions are also used by Mudlet to load & save the entire Lua session variables. <br/><br/>
---
--- Original code written by CHILLCODE™ on https://board.ptokax.ch, distributed under the same terms as Lua itself. <br/><br/>
---
--- Notes: <br/>
---  Userdata and indices of these are not saved <br/>
---  Functions are saved via string.dump, so make sure it has no upvalues <br/>
---  References are saved <br/>
---
--- @usage Saves the globals table (minus some lua enviroment stuffs) into a file (only Mudlet should use this).
---   <pre>
---   table.save(file)
---   </pre>
--- @usage Saves the given table into the given file.
---   <pre>
---   table.save(file, table)
---   </pre>
---
--- @see table.load
function table.save( sfile, t )
	assert(type(sfile) == "string", "table.save requires a file path to save to")
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



--- Restores a Lua table from a data file that has been saved with table.save().
---
--- @usage Loads a serialized file into the globals table (only Mudlet should use this).
---   <pre>
---   table.load(file)
---   </pre>
--- @usage Loads a serialized file into the given table.
---   <pre>
---   table.load(file, table)
---   </pre>
---
--- @see table.save
function table.load( sfile, loadinto )
	assert(type(sfile) == "string", "table.load requires a file path to load")
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



--- <b><u>TODO</u></b> getColorWildcard(color)
function getColorWildcard(color)
	local color = tonumber(color)
	local startc
	local endc
	local results = {}

	for i = 1, string.len(line) do
		selectSection(i, 1)
		if isAnsiFgColor(color) then
			if not startc then if i == 1 then startc = 1 else startc = i + 1 end
			else endc = i + 1
				if i == line:len() then results[#results + 1] = line:sub(startc, endc) end
			end
		elseif startc then
			results[#results + 1] = line:sub(startc, endc)
			startc = nil
		end
	end
	return results[1] and results or false
end


do
	local oldsetExit = setExit

	local exitmap = {
	  n = 1,
	  ne = 2,
	  nw = 3,
	  e = 4,
	  w = 5,
	  s = 6,
	  se = 7,
	  sw = 8,
	  u = 9,
	  d = 10,
	  ["in"] = 11,
	  out = 12
	}

	function setExit(from, to, direction)
	  if type(direction) == "string" and not exitmap[direction] then return false end

	  return oldsetExit(from, to, type(direction) == "string" and exitmap[direction] or direction)
	end
end

do
	local oldlockExit = lockExit
	local oldhasExitLock = hasExitLock

	local exitmap = {
	  n = 1,
	  north = 1,
	  ne = 2,
	  northeast = 2,
	  nw = 3,
	  northwest = 3,
	  e = 4,
	  east = 4,
	  w = 5,
	  west = 5,
	  s = 6,
	  south = 6,
	  se = 7,
	  southeast = 7,
	  sw = 8,
	  southwest = 8,
	  u = 9,
	  up = 9,
	  d = 10,
	  down = 10,
	  ["in"] = 11,
	  out = 12
	}

	function lockExit(from, direction, status)
	  if type(direction) == "string" and not exitmap[direction] then return false end

	  return oldlockExit(from, type(direction) == "string" and exitmap[direction] or direction, status)
	end

	function hasExitLock(from, direction)
	  if type(direction) == "string" and not exitmap[direction] then return false end

	  return oldhasExitLock(from, type(direction) == "string" and exitmap[direction] or direction)
	end
end

ioprint = print
function print(...)
  local t, echo, tostring = {...}, echo, tostring
  for i = 1, #t do
    echo((tostring(t[i]) or '?').."    ")
  end
  echo("\n")
end

function deleteFull()
	deleteLine()
	tempLineTrigger(1,1, [[if isPrompt() then deleteLine() end]])
end

function shms(seconds, bool)
	local seconds = tonumber(seconds)
	assert(type(seconds) == "number", "Assertion failed for function 'shms' - Please supply a valid number.")

	local s  = seconds
	local ss = string.format("%02d", math.fmod(s, 60))
	local mm = string.format("%02d", math.fmod((s / 60 ), 60))
	local hh = string.format("%02d", (s / (60 * 60)))

	if bool then
		cecho("<green>" .. s .. " <grey>seconds converts to: <green>" .. hh .. "<white>h,<green> " .. mm .. "<white>m <grey>and<green> " .. ss .. "<white>s.")
	else
		return hh, mm, ss
	end
end
