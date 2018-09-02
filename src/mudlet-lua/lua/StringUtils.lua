----------------------------------------------------------------------------------
--- Mudlet String Utils
----------------------------------------------------------------------------------



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.cut
function string.cut(s, maxLen)
  if string.len(s) > maxLen then
    return string.sub(s, 1, maxLen)
  else
    return s
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.enclose
function string.enclose(s, maxlevel)
  s = "[" .. s .. "]"
  local level = 0
  while 1 do
    if maxlevel and level == maxlevel then
      error( "error: maxlevel too low, " .. maxlevel )
    elseif string.find( s, "%[" .. string.rep( "=", level ) .. "%[" ) or string.find( s, "]" .. string.rep( "=", level ) .. "]" ) then
      level = level + 1
    else
      return "[" .. string.rep( "=", level ) .. s .. string.rep( "=", level ) .. "]"
    end
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.ends
function string.ends(String, Suffix)
  return Suffix == '' or string.sub(String, -string.len(Suffix)) == Suffix
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.genNocasePattern
function string.genNocasePattern(s)
  s = string.gsub(s, "%a",
  function(c)
    return string.format("[%s%s]", string.lower(c), string.upper(c))
  end)
  return s
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.findPattern
function string.findPattern(text, pattern)
  if string.find(text, pattern, 1) then
    return string.sub(text, string.find(text, pattern, 1))
  else
    return nil
  end
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.split
function string:split(delimiter)
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
function string.starts(String, Prefix)
  return string.sub(String, 1, string.len(Prefix)) == Prefix
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.title
function string:title()
  assert(type(self) == "string", "string.title(): no word given to capitalize")
  self = self:gsub("^%l", string.upper, 1)
  return self
end



--- Documentation: https://wiki.mudlet.org/w/Manual:String_Functions#string.trim
function string.trim(s)
  if s then
    return string.gsub(s, "^%s*(.-)%s*$", "%1")
  else
    return s
  end
end
