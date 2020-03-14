local altname = require"_openssl.x509.altname"
local auxlib = require"openssl.auxlib"

altname.interpose("__tostring", function (self)
	local t = { }

	for k, v in auxlib.pairs(self) do
		t[#t + 1] = k .. ":" .. v
	end

	return table.concat(t, ", ")
end)

return altname
