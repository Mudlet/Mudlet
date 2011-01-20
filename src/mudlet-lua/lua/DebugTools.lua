----------------------------------------------------------------------------------
--- Mudlet Debug Tools
----------------------------------------------------------------------------------



--- Function colorizes all matched regex capture groups on the screen.
--- This is very handy if you make complex regex and want to see what really matches in the text.
---
--- @see matches
function showCaptureGroups()
	for k, v in pairs ( matches ) do
		selectCaptureGroup( tonumber(k) )
		setFgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
		setBgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
	end
end



--- Prints the content of the table multimatches[n][m] to the screen. This is meant
--- as a tool to help write multiline trigger scripts. This helps you to easily see
--- what your multiline trigger actually captured in all regex. You can use these values
--- directly in your script by referring to it with multimatches[regex-number][capturegroup].
---
--- @usage Just call this s function from your trigger to show the info.
---   <pre>
---   showMultimatches()
---   </pre>
---
--- @see multimatches
function showMultimatches()
	echo("\n-------------------------------------------------------");
	echo("\nThe table multimatches[n][m] contains:");
	echo("\n-------------------------------------------------------");
	for k,v in ipairs(multimatches) do
		echo("\nregex " .. k .. " captured: (multimatches["..k .."][1-n])");
		for k2,v2 in ipairs(v) do
			echo("\n          key="..k2.." value="..v2);
		end
	end
	echo("\n-------------------------------------------------------\n");
end



--- Pretty display function will try to print out content of any table. <br/><br/>
---
--- Note: This version of display can lead to infinite loop depending on your data. There is also
--- <i>Geyser.display()</i>, which will use only one level of recursion.
---
--- @usage display(mytable)
function display(what, numformat, recursion)
	recursion = recursion or 0

	if recursion == 0 then
		echo("\n")
	end
	echo(printable(what, numformat))

	-- Do all the stuff inside a table
	if type(what) == 'table' then
		echo(" {")

		local firstline = true   -- a kludge so empty tables print on one line
		if getmetatable(what) and getmetatable(what).isPhpTable == true then
			for k, v in what:pairs() do
				if firstline then echo("\n"); firstline = false end
				echo(indent(recursion))
				echo(printable(k))
				echo(": ")
				display(v, numformat, recursion + 1)
			end
		else
			for k, v in pairs(what) do
				if firstline then echo("\n"); firstline = false end
				echo(indent(recursion))
				echo(printable(k))
				echo(": ")
				if not (k == _G) then display(v, numformat, recursion + 1) end
			end
		end

		-- so empty tables print as {} instead of {..indent..}
		if not firstline then
			echo(indent(recursion - 1))
		end
		echo("}")
	end

	echo("\n")
	if recursion == 0 then
	end
end



--- Basically like tostring(), except takes a numformat and is a little better suited
--- for working with display().
---
--- @see display
function printable(what, numformat)
	local ret
	if type(what) == 'string' then
		ret = "'"..what.."'"
	elseif type(what) == 'number' then
		if numformat then
			ret = string.format(numformat, what)
		else
			ret = what
		end
	elseif type(what) == 'boolean' then
		ret = tostring(what)
	elseif type(what) == 'table' then
		ret = what.__customtype or type(what)
	else
		ret = type(what)
	end
	return ret
end



-- simulate a static variable
do local indents = {}

	--- This function handles indentation for display function.
	---
	--- @see display
	function indent(num)
		if not indents[num] then
			indents[num] = ""
			for i = 0, num do
				indents[num] = indents[num].."  "
			end
		end
		return indents[num]
	end

end


