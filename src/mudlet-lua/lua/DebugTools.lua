----------------------------------------------------------------------------------
--- Mudlet Debug Tools
----------------------------------------------------------------------------------


-- maintain backwards compatibility with prettywrite
prettywrite = inspect


-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showCaptureGroups
function showCaptureGroups()
  for k, v in pairs( matches ) do
    selectCaptureGroup( tonumber(k) )
    setFgColor( math.random(0, 255), math.random(0, 255), math.random(0, 255) )
    setBgColor( math.random(0, 255), math.random(0, 255), math.random(0, 255) )
  end
end


-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showMultimatches
function showMultimatches()
  echo("\n-------------------------------------------------------");
  echo("\nThe table multimatches[n][m] contains:");
  echo("\n-------------------------------------------------------");
  for k, v in ipairs(multimatches) do
    echo("\nregex " .. k .. " captured: (multimatches[" .. k .. "][1-n])");
    for k2, v2 in pairs(v) do
      echo("\n          key=" .. k2 .. " value=" .. v2 .. " ");
    end
  end
  echo("\n-------------------------------------------------------\n");
end


-- Documentation: https://wiki.mudlet.org/index.php?title=Manual:Lua_Functions#display
function display(...)
  local arg = {...}
  arg.n = table.maxn(arg)
  if arg.n > 1 then
    for i = 1, arg.n do
      display(arg[i])
    end
  else
    echo((inspect(arg[1]) or 'nil') .. '\n')
  end
end

local errc
-- leave errorc in the global table if and only if this is the mudlet self-test profile for running Busted tests
-- this is because we need to spy on it to for testing.
if getProfileName() ~= "Mudlet self-test" then
  errc = errorc
  _G.errorc = nil       -- and set to nil since it is internal only for the following functions.
end
-- undocumented, internal function
local function printX(options)
  local errorc = errc and errc or errorc
  local func = options.func or debugc
  local showTrace = options.showTrace
  local msg = options.msg or ""
  local halt = options.halt
  local stackTable = debug.traceback():gsub("\t", "  "):gsub("%[string ",""):split("\n")
  -- the table.removes below remove the printX and printError or printDebug calls from the stacktrace
  -- decided to do this as they aren't the information the user is likely to be interested in
  table.remove(stackTable,2)
  table.remove(stackTable,2)
  local level = #stackTable + 1
  local dinfo = debug.getinfo(level)
  local header = string.format("(%s:line %s)", dinfo.source, dinfo.currentline)
  if halt then
    header = "\n" .. header
  end
  local traceback = showTrace and "\n" .. table.concat(stackTable, "\n") or ""
  if func ~= errorc then
    msg = string.format("%s %s%s", halt and "" or header, msg, traceback)
    if halt then
      func(msg, level)
    end
    func(msg)
    return
  end
  msg = msg .. traceback
  func(msg, header)
end

-- Documentation: https://wiki.mudlet.org/index.php?title=Manual:Lua_Functions#printError
function printError(msg, showTrace, haltExecution)
  local func = haltExecution and error or (errc and errc or errorc) -- if running automated tests, errc is undefined, use the exposed global.
  local options = {
    msg = msg,
    showTrace = showTrace,
    halt = haltExecution,
    func = func,
  }
  printX(options)
end

-- Documentation: https://wiki.mudlet.org/index.php?title=Manual:Lua_Functions#printDebug
function printDebug(msg, showTrace)
  local options = {
    msg = msg,
    showTrace = showTrace,
    halt = false,
    func = debugc
  }
  printX(options)
end