--- Assorted Geyser utilities.
-- @author guy
-- @module Geyser.Util

--- Generate a window name unique to this session.
function Geyser.nameGen (type)
  local index = Geyser.i
  local t = type or "window"
  Geyser.i = Geyser.i + 1
  return "anon_" .. t .. "_" .. index
end

-- Geyser's files all load from this one's directory, which tells its own
-- frames apart from the code that asked it for an element
local geyserSource = debug.getinfo(1, "S").source:match("^@.*[/\\]")
local warnedLocations = {}

local function creatorLocation()
  if not geyserSource then
    return nil
  end
  local level = 3
  while true do
    local info = debug.getinfo(level, "Sl")
    if not info then
      return nil
    end
    if (info.what == "Lua" or info.what == "main") and info.source:sub(1, #geyserSource) ~= geyserSource then
      -- short_src is cut to fit LUA_IDSIZE, so scripts with long names can share it
      return info.short_src .. ":" .. info.currentline, info.source .. ":" .. info.currentline
    end
    level = level + 1
  end
end

--- Generate a name for an element created without one, warning through debugc() that
-- running the code that made it again will make another element rather than reuse it.
-- The warning is given once for each line of code that does this.
-- @param kind the element's type, as the warning names it
-- @param nameType the type passed on to Geyser.nameGen
function Geyser.anonymousName(kind, nameType)
  local name = Geyser.nameGen(nameType)
  local location, key = creatorLocation()
  if key then
    if warnedLocations[key] then
      return name
    end
    warnedLocations[key] = true
  end
  debugc(string.format("Geyser: a '%s' element was created without a name%s, so it was called '%s'. Each time that code runs again it makes another one instead of reusing this one, so duplicates pile up - give it a name to avoid that.",
    kind, location and (" at " .. location) or "", name))
  return name
end

--- Hide all managed windows.
function Geyser.hideAll(type)
  for _, v in pairs(Geyser.windowList) do
    if v.type == type or not type then
      v:hide()
    end
  end
end

--- Show all managed windows.
function Geyser.showAll(type)
  for _, v in pairs(Geyser.windowList) do
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
    for k, v in pairs(table) do
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
    for k, v in pairs(table) do
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
