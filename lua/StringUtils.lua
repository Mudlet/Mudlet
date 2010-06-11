
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------
--                                                                              --
-- String Utils                                                                 --
--                                                                              --
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------

--- Test if string is starting with specified prefix.
function string.starts(String, Prefix)
	return string.sub(String,1,string.len(Prefix))==Prefix
end


--- Test if string is ending with specified suffix.
function string.ends(String, Suffix)
	return Suffix=='' or string.sub(String,-string.len(Suffix))==Suffix
end


--- Splits a string by delimiter.
--- @return array with split strings
function string:split(delimiter)
	local result = { }
	local from  = 1
	local delim_from, delim_to = string.find( self, delimiter, from  )
	while delim_from do
		table.insert( result, string.sub( self, from , delim_from-1 ) )
		from  = delim_to + 1
		delim_from, delim_to = string.find( self, delimiter, from  )
	end
	table.insert( result, string.sub( self, from  ) )
	return result
end


--- Enclose string by long brackets. <br/>
--- TODO what is purpose of this function?
function string.enclose(s, maxlevel)
	s = "["..s.."]"
	local level = 0
	while 1 do
		if maxlevel and level == maxlevel then
			error( "error: maxlevel too low, "..maxlevel )
		elseif string.find( s, "%["..string.rep( "=", level ).."%[" ) or string.find( s, "]"..string.rep( "=", level ).."]" ) then
			level = level + 1
		else
			return "["..string.rep( "=", level )..s..string.rep( "=", level ).."]"
		end
	end
end


--- Capitalize first character in a string.
function string:title()
	self = self:gsub("^%l", string.upper, 1)
	return self
end

