----------------------------------------------------------------------------------
--- Mudlet String Utils
--- Used by both LuaGlobal.lua and generate-changelog.lua
----------------------------------------------------------------------------------



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.cut
function string:cut(maxLen)
  -- generate-changelog.lua loads this file in plain Lua with an empty utf8 table
  local characters = utf8.len and utf8.sub and utf8.len(self)
  if characters then
    if characters > maxLen then
      return utf8.sub(self, 1, maxLen)
    end
    return self
  end

  if string.len(self) > maxLen then
    return string.sub(self, 1, maxLen)
  else
    return self
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.enclose
function string:enclose(maxlevel)
  self = "[" .. self .. "]"
  local level = 0
  while 1 do
    if maxlevel and level == maxlevel then
      error( "error: maxlevel too low, " .. maxlevel )
    elseif string.find( self, "%[" .. string.rep( "=", level ) .. "%[" ) or string.find( self, "]" .. string.rep( "=", level ) .. "]" ) then
      level = level + 1
    else
      return "[" .. string.rep( "=", level ) .. self .. string.rep( "=", level ) .. "]"
    end
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.ends
function string:ends(suffix)
  return suffix == '' or string.sub(self, -string.len(suffix)) == suffix
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.genNocasePattern
function string:genNocasePattern()
  local s = string.gsub(self, "%a",
  function(c)
    return string.format("[%s%s]", string.lower(c), string.upper(c))
  end)
  return s
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.findPattern
function string:findPattern(pattern)
  if string.find(self, pattern, 1) then
    return string.sub(self, string.find(self, pattern, 1))
  else
    return nil
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.split
function string:split(delimiter)
  delimiter = delimiter or " "
  local result = { }

  if delimiter == "" then
    for i = 1, #self do
      result[i] = self:sub(i,i)
    end
  else
    local from = 1
    local delim_from, delim_to = string.find( self, delimiter, from  )
    while delim_from do
      result[#result+1] = string.sub(self, from, delim_from - 1)
      from = delim_to + 1
      delim_from, delim_to = string.find( self, delimiter, from  )
    end
    result[#result+1] = string.sub(self, from)
  end
  return result
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.starts
function string:starts(prefix)
  return string.sub(self, 1, string.len(prefix)) == prefix
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.title
function string:title()
  local strType = type(self)
  assert(strType == "string", string.format("string.title: bad argument #1 type (string to title as string expected, got %s!)", strType))
  self = self:gsub("^%l", string.upper, 1)
  return self
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.trim
function string:trim()
  if self then
    -- Not "^%s*(.-)%s*$", which retries the trailing %s*$ at every space of
    -- every run inside the line - quadratic on the long runs that tabular game
    -- output is full of. An all-space string is answered first, as the second
    -- pattern would backtrack through it the same way.
    if string.match(self, "^%s*$") then
      return ""
    end
    return string.match(self, "^%s*(.*%S)")
  else
    return self
  end
end

local patternEscapes = {
  ["%"] = "%%",
  ["^"] = "%^",
  ["$"] = "%$",
  ["("] = "%(",
  [")"] = "%)",
  ["["] = "%[",
  ["]"] = "%]",
  ["."] = "%.",
  ["*"] = "%*",
  ["+"] = "%+",
  ["-"] = "%-",
  ["?"] = "%?",
}

--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.patternEscape
function string.patternEscape(self)
  local gsub = string.gsub
  local selfType = type(self)
  if selfType ~= "string" then
    printError(f"string.patternEscape: bad argument #1 type (string to escape as string expected, got {selfType})", true, true)
  end
  -- every magic character is punctuation, so %p visits only the bytes that
  -- could need escaping; any other punctuation is not in the table, and gsub
  -- leaves a match the table has no entry for as it was
  local escaped = gsub(self, "%p", patternEscapes)
  return escaped
end

--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#utf8.patternEscape
function utf8.patternEscape(self)
  local gsub = utf8.gsub
  local selfType = type(self)
  if selfType ~= "string" then
    printError(f"utf8.patternEscape: bad argument #1 type (string to escape as string expected, got {selfType})", true, true)
  end
  local escaped = gsub(self, ".", patternEscapes)
  return escaped
end

-- following functions fiddled with from https://github.com/hishamhm/f-strings/blob/master/F.lua and https://hisham.hm/2016/01/04/string-interpolation-in-lua/
-- first bit patches load for lua 5.1.
local load = load

if _VERSION == "Lua 5.1" then
  load = function(code, name, _, env)
    local fn, err = loadstring(code, name)
    if fn then
      setfenv(fn, env)
      return fn
    end
    return nil, err
  end
end

-- Compiling a {} block costs far more than running it, so each one is compiled
-- once and handed this call's environment to run in. Weak, so expressions built
-- from changing text cannot pile up.
local fstring_compiled = setmetatable({}, { __mode = "v" })

local function fstring_settle(fn, previous_env, ...)
  setfenv(fn, previous_env)
  return ...
end

-- long and inconvenient variable name is to help avoid collisions
-- str (what it was before) was causing f("Hello {str}") to return "Hello Hello {str}"
function f(supersecretstringvariablenocollision)
  local supersecretstringvariablenocollisiontype = type(supersecretstringvariablenocollision)
  if supersecretstringvariablenocollisiontype ~= "string" then
    error("f: bad argument #1 type (str as string expected, got " .. supersecretstringvariablenocollisiontype .. ")")
  end
  local outer_env = _ENV or getfenv(1)
  -- looks the name up afresh on every read, so one serves every block
  local lookup = function(_, k)
    -- From the frame above this f(): below it are this function, the
    -- expression, the gsub callback, gsub itself and this f()
    local stack_level = 6
    while debug.getinfo(stack_level, "") ~= nil do
      local name, value = debug.getlocal(stack_level, 1)
      -- An f() and its gsub callback, here or further up the stack, are told
      -- by their first parameter, so none of their locals shadow the caller's
      if name ~= "supersecretstringvariablenocollision" and name ~= "supersecretblocknocollision" then
        local i = 1
        while name do
          if name == k then
            return value
          end
          i = i + 1
          name, value = debug.getlocal(stack_level, i)
        end
      end
      stack_level = stack_level + 1
    end
    -- Mudlet leaves these out of the globals table until they are first read
    if k == "matches" or k == "multimatches" or k == "line" then
      return outer_env[k]
    end
    return rawget(outer_env, k)
  end
  return (supersecretstringvariablenocollision:gsub("%b{}", function(supersecretblocknocollision)
    local code = supersecretblocknocollision:match("{(.*)}")
    local exp_env = {}
    setmetatable(exp_env, { __index = lookup })
    if not setfenv then
      local fn, err = load("return " .. code, "expression `" .. code .. "`", "t", exp_env)
      if fn then
        return tostring(fn())
      else
        error(err, 0)
      end
    end
    local fn = fstring_compiled[code]
    if not fn then
      local err
      fn, err = load("return " .. code, "expression `" .. code .. "`", "t", exp_env)
      if not fn then
        error(err, 0)
      end
      fstring_compiled[code] = fn
    end
    -- the same expression can be running further up the stack, from an f() it
    -- called, and must get its own environment back afterwards
    local previous_env = getfenv(fn)
    setfenv(fn, exp_env)
    return tostring(fstring_settle(fn, previous_env, fn()))
  end))
end
