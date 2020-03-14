local function read_hex(hex_num)
	return tonumber(hex_num, 16)
end

local safe_tonumber do -- locale independent tonumber function
	local tolocale
	local function updatelocale()
		local decpoint = string.format("%f", 0.5):match "[^05]+"
		if decpoint == "." then
			tolocale = function(str)
				return str
			end
		else
			tolocale = function(str)
				str = str:gsub("%.", decpoint, 1)
				return str
			end
		end
	end
	updatelocale()
	safe_tonumber = function(str)
		local num = tonumber(tolocale(str))
		if num then
			return num
		else
			updatelocale()
			return tonumber(tolocale(str))
		end
	end
end

return {
	read_hex = read_hex;
	safe_tonumber = safe_tonumber;
}
