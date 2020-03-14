local auxlib = {}

if _VERSION == "Lua 5.1" then
	local _pairs = pairs

	function auxlib.pairs(t)
		if type(t) == "userdata" then
			local mt = getmetatable(t)

			if mt and mt.__pairs then
				return mt.__pairs(t)
			else
				return _pairs(t)
			end
		end
	end
else
	auxlib.pairs = pairs
end

return auxlib
