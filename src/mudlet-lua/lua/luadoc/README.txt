
Mudlet taglet and doclet are extension to LuaDoc which is
  - supporting <pre> formatted text (see simple example bellow)
  - generating master index of function (we are assuming that all function names are unique)
  - correctly linking to functions from different file
  - keeping link menu accessible in the top left corner all the time
  - improving visual side of generated documentation

based on LuaDoc 3.0.1



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

