----------------------------------------------------------------------------------
--- Mudlet String Utils
----------------------------------------------------------------------------------



--- Cut string to specified maximum length.
---
--- @release post Mudlet 1.1.1 (<b><u>TODO update before release</u></b>)
---
--- @usage Following call will return 'abc'.
---   <pre>
---   string.cut("abcde", 3)
---   </pre>
--- @usage You can easily pad string to certain length.
---   Example bellow will print 'abcde     ' e.g. pad/cut string to 10 characters.
---   <pre>
---   local s = "abcde"
---   s = string.cut(s .. "          ", 10)   -- append 10 spaces
---   echo("'" .. s .. "'")
---   </pre>
function string.cut(s, maxLen)
	if string.len(s) > maxLen then
		return string.sub(s, 1, maxLen)
	else
		return s
	end
end



--- Enclose string by long brackets. <br/>
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
--- @see string.starts
function string.ends(String, Suffix)
	return Suffix=='' or string.sub(String,-string.len(Suffix))==Suffix
end



--- Generate case insensitive search pattern from string.
---
--- @release post Mudlet 1.1.1 (<b><u>TODO update before release</u></b>)
---
--- @return case insensitive pattern string
---
--- @usage Following example will generate and print <i>"123[aA][bB][cC]"</i> string.
---   <pre>
---   echo(string.genNocasePattern("123abc"))
---   </pre>
function string.genNocasePattern(s)
	s = string.gsub(s, "%a",
		function (c)
			return string.format("[%s%s]", string.lower(c), string.upper(c))
		end)
	return s
end



--- Return first matching substring or nil.
---
--- @release post Mudlet 1.1.1 (<b><u>TODO update before release</u></b>)
---
--- @return nil or first matching substring
---
--- @usage Following example will print: "I did find: Troll" string.
---   <pre>
---   local match = string.findPattern("Troll is here!", "Troll")
---   if match then
---      echo("I did find: " .. match)
---   end
---   </pre>
--- @usage This example will find substring regardless of case.
---   <pre>
---   local match = string.findPattern("Troll is here!", string.genNocasePattern("troll"))
---   if match then
---      echo("I did find: " .. match)
---   end
---   </pre>
---
--- @see string.genNocasePattern
function string.findPattern(text, pattern)
	if string.find(text, pattern, 1) then
		return string.sub(text, string.find(text, pattern, 1))
	else
		return nil
	end
end



--- Splits a string into a table by the given delimiter.
---
--- @usage Split string by ", " delimiter.
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
--- @see string.ends
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
	assert(type(self) == "string", "string.title(): no word given to capitalize")
	self = self:gsub("^%l", string.upper, 1)
	return self
end



--- Trim string (remove all white spaces around string).
---
--- @release post Mudlet 1.1.1 (<b><u>TODO update before release</u></b>)
---
--- @usage Example will print 'Troll is here!'.
---   <pre>
---   local str = string.trim("  Troll is here!  ")
---   echo("'" .. str .. "'")
---   </pre>
function string.trim(s)
	if s then
		return string.gsub(s, "^%s*(.-)%s*$", "%1")
	else
		return s
	end
end

