----------------------------------------------------------------------------------
--- Mudlet String Utils                                                                 
----------------------------------------------------------------------------------


--- Enclose string by long brackets. <br/>
--- <b><u>TODO</u></b> what is purpose of this function?
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


--- Test if string is ending with specified suffix.
---
--- @see starts
function string.ends(String, Suffix)
	return Suffix=='' or string.sub(String,-string.len(Suffix))==Suffix
end


--- Splits a string into a table by the given delimiter.
--- 
--- @usage
---   <pre>
---   names = "Alice, Bob, Peter"
---   name_table = names:split(", ")
---   display(name_table)
---   </pre>
---   
---   Previous code will print out:
---   <pre>
---   table {
---     1: 'Alice'
---     2: 'Bob'
---     3: 'Peter'
---   }
---   </pre>
---   
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


--- Test if string is starting with specified prefix.
---
--- @see ends
function string.starts(String, Prefix)
	return string.sub(String,1,string.len(Prefix))==Prefix
end


--- Capitalize first character in a string.
---
--- @usage Variable testname is now Anna.
---   <pre>
---   testname = string.title("anna")
---   </pre>
--- @usage Example will set test to "Bob".
---   <pre>
---   test = "bob"
---   test = string.title(test)
---   </pre>
function string:title()
	self = self:gsub("^%l", string.upper, 1)
	return self
end
