----------------------------------------------------------------------------------
--- Mudlet String Utils
----------------------------------------------------------------------------------



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.cut
function string:cut(maxLen)
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
    -- return only the trimmed string, and not the # of replacements done as well
    local trimmed = string.gsub(self, "^%s*(.-)%s*$", "%1")
    return trimmed
  else
    return self
  end
end

--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.patternEscape
function string.patternEscape(self)
  local gsub = string.gsub
  local selfType = type(self)
  if selfType ~= "string" then
    printError(f"string.patternEscape: bad argument #1 type (string to escape as string expected, got {selfType})", true, true)
  end
  local replacements = {
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
  local escaped = gsub(self, ".", replacements)
  return escaped
end

--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#utf8.patternEscape
function utf8.patternEscape(self)
  local gsub = utf8.gsub
  local selfType = type(self)
  if selfType ~= "string" then
    printError(f"utf8.patternEscape: bad argument #1 type (string to escape as string expected, got {selfType})", true, true)
  end
  local replacements = {
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
  local escaped = gsub(self, ".", replacements)
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

-- long and inconvenient variable name is to help avoid collisions
-- str (what it was before) was causing f("Hello {str}") to return "Hello Hello {str}"
function f(supersecretstringvariablenocollision)
  local outer_env = _ENV or getfenv(1)
  return (supersecretstringvariablenocollision:gsub("%b{}", function(block)
    local code = block:match("{(.*)}")
    local exp_env = {}
    setmetatable(exp_env, {
      __index = function(_, k)
        local stack_level = 5
        while debug.getinfo(stack_level, "") ~= nil do
          local i = 1
          repeat
            local name, value = debug.getlocal(stack_level, i)
            if name == k then
              return value
            end
            i = i + 1
          until name == nil
          stack_level = stack_level + 1
        end
        return rawget(outer_env, k)
      end,
    })
    local fn, err = load("return " .. code, "expression `" .. code .. "`", "t", exp_env)
    if fn then
      return tostring(fn())
    else
      error(err, 0)
    end
  end))
end
