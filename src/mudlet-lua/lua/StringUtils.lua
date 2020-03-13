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
  local from = 1
  local delim_from, delim_to = string.find( self, delimiter, from  )
  while delim_from do
    table.insert( result, string.sub( self, from, delim_from - 1 ) )
    from = delim_to + 1
    delim_from, delim_to = string.find( self, delimiter, from  )
  end
  table.insert( result, string.sub( self, from  ) )
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
