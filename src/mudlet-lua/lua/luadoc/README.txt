
Taglet is extension to LuaDoc Standard Taglet
    which is supporting <pre> formatted text (see simple example bellow)
	based on  standard.lua,v 1.39 2007/12/21 17:50:48 tomas Exp $
	(Note: we are (mis) using modules for building global function index.)


Doclet is extension to LuaDoc HTML Doclet
    with few minor visual changes.



-------------------------------------------------------------------------------
-- This is just copy of standard.lua with support for <pre> tag in 
-- documentation. See example bellow:
--
-- <pre>
--	if info ~= nil then
--		assert(info.name, "function name undefined")
--	end
-- </pre>
--
function test(a, b, c) end

