----------------------------------------------------------------------------------
--- Mudlet String Utils
--- Used by both LuaGlobal.lua and generate-changelog.lua
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

local find, sub, concat, type = string.find, string.sub, table.concat, type
local getinfo, getlocal = debug.getinfo, debug.getlocal

-- Compiling a {} block costs far more than running it, so each one is compiled
-- once and handed this call's environment to run in. Weak, so expressions built
-- from changing text cannot pile up.
local fstring_compiled = setmetatable({}, { __mode = "v" })

-- Weak for the same reason: parsed templates built from changing text must not pile up.
local fstring_templates = setmetatable({}, { __mode = "v" })

-- A bare name skips compilation; keywords such as {nil} or {true} are expressions, not names.
local fstring_keywords = {}
for word in ("and break do else elseif end false for function if in local nil not or repeat return then true until while goto"):gmatch("%a+") do
  fstring_keywords[word] = true
end

local function fstring_settle(fn, previous_env, ...)
  setfenv(fn, previous_env)
  return ...
end

-- f's first local is its parameter; a frame that starts with it is f's own, whose
-- internals must not be readable by an f nested in an expression. Rename both together.
local fstring_param = "supersecretstringvariablenocollision"

-- `level` is counted from this function's own frame. Returns whether it found a local, and its value.
local function fstring_local(k, level)
  while getinfo(level, "") ~= nil do
    local i = 1
    repeat
      local name, value = getlocal(level, i)
      if i == 1 and name == fstring_param then
        break
      end
      if name == k then
        return true, value
      end
      i = i + 1
    until name == nil
    level = level + 1
  end
  return false
end

-- fstring_local for every name in the set `names` at once, for a template that
-- is nothing but several names: one walk of the stack rather than one each.
local function fstring_locals(names, count, level, values, seen)
  while count > 0 and getinfo(level, "") ~= nil do
    local i = 1
    repeat
      local name, value = getlocal(level, i)
      if i == 1 and name == fstring_param then
        break
      end
      if names[name] and not seen[name] then
        seen[name] = true
        values[name] = value
        count = count - 1
      end
      i = i + 1
    until name == nil or count == 0
    level = level + 1
  end
end

-- Mudlet leaves these out of the globals table until they are first read
local function fstring_global(outer_env, k)
  if k == "matches" or k == "multimatches" or k == "line" then
    return outer_env[k]
  end
  return rawget(outer_env, k)
end

local fstring_self

local function fstring_parse(template)
  local parts, count, position = { names = {}, nameCount = 0, onlyNames = true }, 0, 1
  while true do
    local first, last = find(template, "%b{}", position)
    if not first then
      break
    end
    if first > position then
      count = count + 1
      parts[count] = sub(template, position, first - 1)
    end
    local code = sub(template, first + 1, last - 1)
    local name = code:match("^[%a_][%w_]*$") and not fstring_keywords[code] and code or nil
    if name and not parts.names[name] then
      parts.names[name] = true
      parts.nameCount = parts.nameCount + 1
    end
    if not name then
      parts.onlyNames = false
    end
    count = count + 1
    parts[count] = { code = code, name = name }
    position = last + 1
  end
  if count > 0 and position <= #template then
    parts[count + 1] = sub(template, position)
  end
  return parts
end

-- Must match fstring_param: an unusual name, so no other function's frame is mistaken for f's
function f(supersecretstringvariablenocollision)
  local supersecretstringvariablenocollisiontype = type(supersecretstringvariablenocollision)
  if supersecretstringvariablenocollisiontype ~= "string" then
    error("f: bad argument #1 type (str as string expected, got " .. supersecretstringvariablenocollisiontype .. ")")
  end
  if not find(supersecretstringvariablenocollision, "{", 1, true) then
    return supersecretstringvariablenocollision
  end
  local parts = fstring_templates[supersecretstringvariablenocollision]
  if not parts then
    parts = fstring_parse(supersecretstringvariablenocollision)
    fstring_templates[supersecretstringvariablenocollision] = parts
  end
  if #parts == 0 then
    return supersecretstringvariablenocollision
  end
  local outer_env = _ENV or getfenv(1)
  local lookup, values, seen
  -- Reading every name up front would miss a block that changes a local before a later name is read
  if parts.onlyNames and parts.nameCount > 1 then
    values, seen = {}, {}
    fstring_locals(parts.names, parts.nameCount, 3, values, seen)
  end
  local out = {}
  for i = 1, #parts do
    local part = parts[i]
    if type(part) == "string" then
      out[i] = part
    elseif part.name then
      local name = part.name
      local found, value
      if seen then
        found, value = seen[name], values[name]
      else
        found, value = fstring_local(name, 3)
      end
      if not found then
        value = fstring_global(outer_env, name)
      end
      out[i] = tostring(value)
      if seen then
        -- A __tostring can change a local that a later name reads
        local kind = type(value)
        if kind == "table" or kind == "userdata" then
          seen = nil
        end
      end
    else
      local code = part.code
      -- Made on first use and shared by this call's blocks; it reads the stack afresh on every access.
      lookup = lookup or function(_, k)
        -- An expression can read from inside functions of its own, so f's frame is
        -- found rather than counted to, and its caller's locals are the ones searched.
        local level = 2
        local info = getinfo(level, "f")
        while info and info.func ~= fstring_self do
          level = level + 1
          info = getinfo(level, "f")
        end
        local found, value = fstring_local(k, level + 2)
        if found then
          return value
        end
        return fstring_global(outer_env, k)
      end
      local exp_env = setmetatable({}, { __index = lookup })
      local fn, err
      if not setfenv then
        fn, err = load("return " .. code, "expression `" .. code .. "`", "t", exp_env)
        if not fn then
          error(err, 0)
        end
        out[i] = tostring(fn())
      else
        fn = fstring_compiled[code]
        if not fn then
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
        out[i] = tostring(fstring_settle(fn, previous_env, fn()))
      end
    end
  end
  return concat(out)
end

-- Set after f exists; lookup finds f's frame by comparing against it.
fstring_self = f
