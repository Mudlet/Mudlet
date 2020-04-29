----------------------------------------------------------------------------------
--- Mudlet Unsorted Stuff
----------------------------------------------------------------------------------

mudlet = mudlet or {}
mudlet.supports = {
  coroutines = true
}

-- enforce uniform locale so scripts don't get
-- tripped up on number representation differences (. vs ,)
os.setlocale("C")

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
  local args = { ... }
  local echo = true
  if type(args[#args]) == 'boolean' then
    echo = table.remove(args, #args)
  end
  for i, v in ipairs(args) do
    if type(v) == 'string' then
      send(v, echo)
    end
  end
end


--- Table of functions used by permGroup to create the appropriate group, based on itemtype.
local group_creation_functions = {
  timer = function(name, parent)
    return not (permTimer(name, parent, 0, "") == -1)
  end,
  trigger = function(name, parent)
    return not (permSubstringTrigger(name, parent, {}, "") == -1)
  end,
  alias = function(name, parent)
    return not (permAlias(name, parent, "", "") == -1)
  end,
  script = function(name, parent)
    return not (permScript(name, parent, "", "") == -1)
  end
}

--- Creates a group of a given type that will persist through sessions.
---
--- @param name name of the item
--- @param itemtype type of the item - can be trigger, alias, or timer
--- @param parent optional name of existing item which the new item
---   will be created as a child of
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
function permGroup(name, itemtype, parent)
  assert(type(name) == "string", "permGroup: need a name for the new thing")
  parent = parent or ""
  assert(group_creation_functions[itemtype], "permGroup: " .. tostring(itemtype) .. " isn't a valid type")
  return group_creation_functions[itemtype](name, parent)
end

--- Appends code to an existing script
---
--- @param name name of the script item
--- @param luaCode
function appendScript(name, luaCode, pos)
  pos = pos or 1
  assert(type(name) == "string", "appendScript: bad argument #1 type (script name as string expected, got "..type(name).."!)")
  assert(type(luaCode) == "string", "appendScript: bad argument #2 type (lua code as string expected, got "..type(luaCode).."!)")
  return setScript(name, getScript(name, pos).."\n"..luaCode, pos)
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
function io.exists(item)
  return lfs.attributes(item) and true or false
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
  if string.char(getMudletHomeDir():byte()) == "/" then
    _sep = "/" else _sep = "\\"
  end
  local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
  local lt_VariableHolder = {}
  if (io.exists(l_SettingsFile)) then
    table.load(l_SettingsFile, lt_VariableHolder)
    for k, v in pairs(lt_VariableHolder) do
      _G[k] = v
    end
  end
end



--- This function should primarily be used by Mudlet. It saves the contents of _saveTable into a file for persistence.
---
--- @see loadVars
function saveVars()
  if string.char(getMudletHomeDir():byte()) == "/" then
    _sep = "/" else _sep = "\\"
  end
  local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
  for k, _ in pairs(_saveTable) do
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
  local file, msg = io.open( sfile, "w" )
  if not file then return nil, msg end
  file:write( "return {" )
  for i, v in ipairs( tables ) do
    table.pickle( v, file, tables, lookup )
  end
  file:write( "}" )
  file:close()
end



--- <b><u>TODO</u></b> table.pickle( t, file, tables, lookup )
function table.pickle( t, file, tables, lookup )
  file:write( "{" )
  for i, v in pairs( t ) do
    -- escape functions
    if type( v ) ~= "function" and type( v ) ~= "userdata" and (i ~= "string" and i ~= "xpcall" and i ~= "package" and i ~= "os" and i ~= "io" and i ~= "math" and i ~= "debug" and i ~= "coroutine" and i ~= "_G" and i ~= "_VERSION" and i ~= "table") then
      -- handle index
      if type( i ) == "table" then
        if not lookup[i] then
          table.insert( tables, i )
          lookup[i] = table.maxn( tables )
        end
        file:write( "[{" .. lookup[i] .. "}] = " )
      else
        local index = ( type( i ) == "string" and "[ " .. string.enclose( i, 50 ) .. " ]" ) or string.format( "[%d]", i )
        file:write( index .. " = " )
      end
      -- handle value
      if type( v ) == "table" then
        if not lookup[v] then
          table.insert( tables, v )
          lookup[v] = table.maxn( tables )
        end
        file:write( "{" .. lookup[v] .. "}," )
      else
        local value = ( type( v ) == "string" and string.enclose( v, 50 ) ) or tostring( v )
        file:write( value .. "," )
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
  for i, v in pairs( t ) do
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
function speedwalktimer(walklist, walkdelay, show)
  send(walklist[1], show)
  table.remove(walklist, 1)
  if #walklist > 0 then
    tempTimer(walkdelay, function()
      speedwalktimer(walklist, walkdelay, show)
    end)
  end
end



--- <b><u>TODO</u></b> speedwalk(dirString, backwards, delay, optional show)
function speedwalk(dirString, backwards, delay, show)
  local dirString = dirString:lower()
  local walkdelay = delay
  if show ~= false then show = true end
  local walklist = {}
  local long_dir = {north = 'n', south = 's', east = 'e', west = 'w', up = 'u', down = 'd'}
  for k,v in pairs(long_dir) do
    dirString = dirString:gsub(k,v)
  end
  local reversedir = {
    n = "s",
    en = "sw",
    e = "w",
    es = "nw",
    s = "n",
    ws = "ne",
    w = "e",
    wn = "se",
    u = "d",
    d = "u",
    ni = "out",
    tuo = "in"
  }
  if not backwards then
    for count, direction in string.gmatch(dirString, "([0-9]*)([neswudio][ewnu]?t?)") do
      count = (count == "" and 1 or count)
      for i = 1, count do
        if delay then
          walklist[#walklist + 1] = direction
        else send(direction, show)
        end
      end
    end
  else
    for direction, count in string.gmatch(dirString:reverse(), "(t?[ewnu]?[neswudio])([0-9]*)") do
      count = (count == "" and 1 or count)
      for i = 1, count do
        if delay then
          walklist[#walklist + 1] = reversedir[direction]
        else send(reversedir[direction], show)
        end
      end
    end
  end
  if walkdelay then
    speedwalktimer(walklist, walkdelay, show)
  end
end



--- <b><u>TODO</u></b> _comp(a, b)
function _comp(a, b)
  if type(a) ~= type(b) then
    return false
  end
  if type(a) == 'table' then
    local a_size = 0
    for k, v in pairs(a) do
      a_size = a_size + 1
      if not b[k] then
        return false
      end
      if not _comp(v, b[k]) then
        return false
      end
    end
    if a_size ~= table.size(b) then
      return false
    end
  else
    if a ~= b then
      return false
    end
  end
  return true
end



--- <b><u>TODO</u></b> phpTable(...) - abuse to: http://richard.warburton.it
function phpTable(...)
  local newTable, keys, values = {}, {}, {}
  newTable.pairs = function(self)
    -- pairs iterator
    local count = 0
    return function()
      count = count + 1
      return keys[count], values[keys[count]]
    end
  end
  setmetatable(newTable, {
    __newindex = function(self, key, value)
      if not self[key] then
        table.insert(keys, key)
      elseif value == nil then
        -- Handle item delete
        local count = 1
        while keys[count] ~= key do
          count = count + 1
        end
        table.remove(keys, count)
      end
      values[key] = value -- replace/create
    end,
    __index = function(self, key)
      return values[key]
    end,
    isPhpTable = true,
  })
  local args = { ... }
  for x = 1, #args do
    for k, v in pairs(args[x]) do
      newTable[k] = v
    end
  end
  return newTable
end



--- <b><u>TODO</u></b> getColorWildcard(color)
function getColorWildcard(color)
  local color, results, startc, endc = tonumber(color), {}, nil, nil

  for i = 0, string.len(line) do
    selectSection(i, 1)
    if isAnsiFgColor(color) then
      if not startc then
        startc = i + 1
        endc = i + 1
      else
        endc = i + 1
        if i == line:len() then
          results[#results + 1] = line:sub(startc, endc)
        end
      end
    elseif startc then
      results[#results + 1] = line:sub(startc, endc)
      startc = nil
    end
  end
  return results[1] and results or false
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
    if type(direction) == "string" and not exitmap[direction] then
      return false
    end

    return oldlockExit(from, type(direction) == "string" and exitmap[direction] or direction, status)
  end

  function hasExitLock(from, direction)
    if type(direction) == "string" and not exitmap[direction] then
      return false
    end

    return oldhasExitLock(from, type(direction) == "string" and exitmap[direction] or direction)
  end
end

if not _TEST then
  -- special exception, as overwriting print() messes up printing in the test environment
  ioprint = print
  function print(...)
    local t, echo, tostring = { ... }, echo, tostring
    for i = 1, #t do
      echo((tostring(t[i]) or '?') .. "    ")
    end
    echo("\n")
  end
end

function deleteFull()
  deleteLine()
  tempLineTrigger(1, 1, [[if isPrompt() then deleteLine() end]])
end

function shms(seconds, bool)
  local seconds = tonumber(seconds)
  assert(type(seconds) == "number", "Assertion failed for function 'shms' - Please supply a valid number.")

  local s = seconds
  local ss = string.format("%02d", math.fmod(s, 60))
  local mm = string.format("%02d", math.fmod((s / 60 ), 60))
  local hh = string.format("%02d", (s / (60 * 60)))

  if bool then
    cecho("<green>" .. s .. " <grey>seconds converts to: <green>" .. hh .. "<white>h,<green> " .. mm .. "<white>m <grey>and<green> " .. ss .. "<white>s.")
  else
    return hh, mm, ss
  end
end

-- returns true if your Mudlet is older than the given version
-- for example, it'll return true if you're on 2.1 and you do mudletOlderThan(3,1)
-- it'll return false if you're on 4.0 and you do mudletOlderThan(4,0,0)
function mudletOlderThan(inputmajor, inputminor, inputpatch)
  local mudletmajor, mudletminor, mudletpatch = getMudletVersion("table")
  local type, sformat = type, string.format

  assert(type(inputmajor) == "number", sformat("bad argument #1 type (major version as number expected, got %s!)", type(inputmajor)))
  assert(inputminor == nil or type(inputminor) == "number", sformat("bad argument #2 type (optional minor version as number expected, got %s!)", type(inputminor)))
  assert(inputpatch == nil or type(inputpatch) == "number", sformat("bad argument #3 type (optional patch version as number expected, got %s!)", type(inputpatch)))

  if mudletmajor < inputmajor then
    return true
  elseif mudletmajor > inputmajor then
    return false
  elseif inputminor then
    if mudletminor < inputminor then
      return true
    elseif mudletminor > inputminor then
      return false
    elseif inputpatch and (mudletpatch < inputpatch) then
        return true
    end
  end
  return false
end

-- condendenses the output from map loading if no map load errors
-- were encountered
-- returns: the amount of time map loading took or nil+msg
-- if it failed
function condenseMapLoad()
  local linestodelete
  local startswith = string.starts

  -- first, figure out how many lines back do we need to delete
  -- this isn't a static amount due to line wrapping
  for i = 1, 30 do
    moveCursor(0, getLineCount() - i)
    local line = getCurrentLine()
    if line:find("ALERT") or line:find("WARN") or line:find("ERROR") then
      return nil, "an alert, warning, or error that the user must see is present"
    elseif startswith(line, "[ INFO ]  - Reading map") then
      linestodelete = i
      moveCursorEnd()
      break
    end
  end

  if not linestodelete then
    return nil, "couldn't find the starting line for map load output"
  end

  local loadtime = 0
  for i = linestodelete, 1, -1 do
    moveCursor(0, getLineCount() - i)
    local time = getCurrentLine():match("([%.%d]+)s")
    if time then
      loadtime = loadtime + tonumber(time)
    end
    deleteLine()
  end

  return loadtime
end

do
  -- management things

  -- Dictionary with events as keys and lists of lua functions as values to dispatch events
  -- to the right functions.
  local handlers = {}

  -- Remember highest hander ID to avoid ID reuse.
  local highestHandlerId = 0
  -- Helps us finding the right event handler from an ID.
  local handlerIdsToHandlers = {}

  -- C functions that get overwritten.
  local origRegisterAnonymousEventHandler = registerAnonymousEventHandler

  -- helper function to find an already existing string event handler
  -- This function may not the most performant one as it uses debug.getinfo,
  -- but since event handlers are only rarely registered, this may be ok.
  local function findStringEventHandler(existingHandlers, functionString)
    local functionExists = false
    if existingHandlers then
      for index, handlerFunction in pairs(existingHandlers) do
        local info = debug.getinfo(handlerFunction, "S")
        if info.source == functionString then
          functionExists = index
          break
        end
      end
    end
    return functionExists
  end

  function registerAnonymousEventHandler(event, func, isOneShot)
    if type(event) ~= "string" then
      error(
      string.format(
      "registerAnonymousEventHandler: bad argument #1 type (event name as string expected, got %s!)",
      type(event)
      )
      )
    end

    if type(func) ~= "function" and type(func) ~= "string" then
      error(
      string.format(
      "registerAnonymousEventHandler: bad argument #2 type (function as string or function type expected, got %s!)",
      type(func)
      )
      )
    end

    local existinghandlers = handlers[event]
    if type(func) == "string" then
      local functionString = string.format("return %s(...)", func)
      local functionExists = findStringEventHandler(existinghandlers, functionString)

      if not functionExists then
        func = assert(loadstring(functionString))
      else
        -- find and return the ID of existing event handlers
        for id, findObject in pairs(handlerIdsToHandlers) do
          if findObject.event == event and findObject.index == functionExists then
            return id
          end
        end
      end

    end

    local eventHandlerId
    if isOneShot then
      -- wrap the original function to remove itself from the event handler list.
      local origFunc = func
      func = function(...)
        local keepEvaluating = origFunc(...)
        if not keepEvaluating then
          killAnonymousEventHandler(eventHandlerId)
        end
      end
    end

    if not existinghandlers then
      existinghandlers = {}
      handlers[event] = existinghandlers
      origRegisterAnonymousEventHandler(event, "dispatchEventToFunctions")
    end
    local newId = #existinghandlers + 1
    existinghandlers[newId] = func
    -- Above may fill gaps if handlers have been deleted, but that's okay.
    highestHandlerId = highestHandlerId + 1
    handlerIdsToHandlers[highestHandlerId] = {
      event = event,
      index = newId
    }
    -- do not remove the line below as it must be part of the closure for one shot event handlers.
    eventHandlerId = highestHandlerId
    return eventHandlerId
  end

  function killAnonymousEventHandler(id)
    if type(id) ~= "number" then
      error(
      string.format(
      "killAnonymousEventHandler: bad argument #1 type (handler ID as number expected, got %s!)",
      type(id)
      )
      )
    end

    local findObject = handlerIdsToHandlers[id]
    if not findObject then
      return nil, string.format("Handler with ID '%s' not found.", id)
    end

    handlerIdsToHandlers[id] = nil
    handlers[findObject.event][findObject.index] = nil
    return true
  end

  -- Dispatches an event to the registered lua functions.
  -- The order of registered events is not preserved.
  -- name: The name of the event that was fired.
  -- ...:  All arguments passed to the raised event.
  function dispatchEventToFunctions(event, ...)
    if handlers[event] then
      for _, func in pairs(handlers[event]) do
        func(event, ...)
      end
    end
  end
end

local timeframetable = {}

function timeframe(vname, true_time, nil_time, ...)
  local format = string.format

  assert(type(vname) == "string" or type(vname) == "function", format("timeframe: bad argument #1 type (vname as a string or function expected, got %s!", type(vname)))
  assert(type(true_time) == "number" or type(true_time) == "table", format("timeframe: bad argument #2 type (true time as a number or table expected, got %s!)", type(true_time)))
  assert(type(nil_time) == "nil" or type(nil_time) == "number" or type(nil_time) == "table", format("timeframe: bad argument #3 type (nil time as a number or table expected, got %s!)", type(nil_time)))

  -- aggregate timerlist data
  local timerlist = {
    {0, nil},
    type(true_time) == "number" and {true_time, true} or type(true_time) == "table" and true_time,
    type(nil_time) == "number" and {nil_time, nil} or type(nil_time) == "table" and nil_time,
    ...
  }

  -- reinitialise timeframe for vname
  killtimeframe(vname)
  timeframetable[vname] = {}

  local vtype = type(vname)

  -- loop through timerlist and create tempTimers
  local maxtime = 0
  local vcount = 1
  for step, data in ipairs(timerlist) do
    assert(type(data) == "table", format("timeframe: bad argument #%d type (timerlist data as a table expected, got %s!", step, type(data)))
    local time, value = data[1], data[2]
    assert(type(time) == "number", format("timeframe: bad argument #%d type (timerlist data table argument #1 as a number expected, got %s!", step, type(time)))

    maxtime = (time > maxtime) and time or maxtime

    local fun
    if vtype == "function" then
      fun = function()
        local s,m = pcall(vname, value)
        if not s then error(m) end
      end
    else
      assert(type(value) == "string" or type(value) == "number" or type(value) == "boolean" or type(value) == "nil", format("timeframe: bad argument #%d type (timerlist data argument #2 expects a string, number or boolean value; got %s!", step, type(value)))
      fun = assert(loadstring(format("%s = %s", vname, type(value) == "string" and ("'" .. value .. "'") or tostring(value))))
    end

    if time <= 0 then
      fun()
    else
      timeframetable[vname][vcount] = tempTimer(time, fun)
      vcount = vcount + 1
    end
  end

  -- add final tempTimer to kill the timeframe
  timeframetable[vname][vcount] = tempTimer(maxtime + 0.1, function()
    killtimeframe(vname)
  end)

  -- return vname as id
  return vname
end

function killtimeframe(vname)
  if timeframetable[vname] then
    for _, timerId in ipairs(timeframetable[vname]) do
      killTimer(timerId); _G["Timer" .. timerId] = nil
    end
    timeframetable[vname] = nil
  end
end

-- replace line from MUD with colour-tagged string
creplaceLine = function(str)
	selectString(line,1)
	replace("")
	cinsertText(str)
end

function translateTable(data, language)
  language = language or mudlet.translations.interfacelanguage
  assert(type(data) == "table", string.format("translateTable: bad argument #1 type (input as table expected, got %s!)", type(data)))

  local t, translations = {}, mudlet.translations[language]

  if not translations then
    return nil, language.." doesn't have any translations for it"
  end

  for i = 1, #data do
    local key = data[i]
    t[#t+1] = translations[key] or key
  end

  return t
end

-- internal function to get the right keys from the translation json file
local function getTranslationTable(inputTable, packageName)
  local outputTable = {}
  for k, v in pairs(inputTable) do
      if k:match("^"..packageName.."%.") then
          outputTable[k:gsub("^.*%.", "")] = inputTable[k]
      end
  end
  return outputTable
end

--internal function to read table from Json file
local function readJsonFile(input)
  local filePointer = io.open(input, "r")
  local str = filePointer:read("*all")
  if str == "" then
    return {}
  end
  return yajl.to_value(str)
end

--- loads Translations located in the /translations folder
-- @param packageName name of the lua package which needs the translations, for example "AdjustableContainer"
-- @param fileName file name of the translations .json file, defaults to "mudlet-lua" [optional]
-- @param languageCode for example de_DE for German, if not given it will take translations from the default file [optional]
-- @param folder folder where your translations can be found, if not given it defaults to the default location [optional]
-- Folder needs to be like (Default File) yourFolder/yourFileName.json (Translated files) yourFolder/translated/yourFileName_lang_code.json
function loadTranslations(packageName, fileName, languageCode, folder)
  fileName = fileName or "mudlet-lua"
  languageCode = languageCode or mudlet.translations.interfacelanguage
  -- get the right folder
  folder = folder or io.exists("../translations/lua") and "../translations/lua/"
  folder = folder or io.exists(luaGlobalPath.."/../../translations/lua") and luaGlobalPath.."/../../translations/lua/"
  folder = folder or luaGlobalPath.."/translations/"

  assert(type(packageName) == "string", string.format("loadTranslations: bad argument #1 type (packageName as string expected, got %s)", type(packageName)))
  assert(type(fileName) == "string", string.format("loadTranslations: bad argument #2 type (fileName as string expected, got %s)", type(fileName)))
  assert(type(languageCode) == "string", string.format("loadTranslations: bad argument #3 type (languageCode as string expected, got %s)", type(languageCode)))
  assert(type(folder) == "string", string.format("loadTranslations: bad argument #4 type (folder path as string expected, got %s)", type(folder)))

  local langFile = io.exists(folder.."translated/"..fileName.."_"..languageCode..".json") and folder.."translated/"..fileName.."_"..languageCode..".json"
  local defaultFile = io.exists(folder..fileName..".json") and folder..fileName..".json"
  if not defaultFile and not langFile then
    return nil, "unable to find '"..fileName..".json' in '"..folder.."'"
  end
  local translation = {}
  if langFile then
      translation = readJsonFile(langFile)
      translation = getTranslationTable(translation, packageName)
  end
  if table.is_empty(translation) and defaultFile then
      translation = readJsonFile(defaultFile)
      translation = getTranslationTable(translation, packageName)
  end
  if table.is_empty(translation) then
      return nil, "couldn't find translations for '"..packageName.."'"
  end
  return translation
end

--- Installs packages which are dropped on MainConsole or UserWindow
-- @param event Drag and Drop Event
-- @param fileName name and location of the file
-- @param suffix suffix of the file
function packageDrop(event, fileName, suffix)
  local acceptable_suffix = {"xml", "mpackage", "zip"}
  if not table.contains(acceptable_suffix, suffix) then
    return
  end
  installPackage(fileName)
end
registerAnonymousEventHandler("sysDropEvent", "packageDrop")
