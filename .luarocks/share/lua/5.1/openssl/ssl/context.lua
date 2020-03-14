local ctx = require"_openssl.ssl.context"

local pack = table.pack or function(...) return { n = select("#", ...); ... } end

-- Allow passing a vararg of ciphers, or an array
local setCipherList; setCipherList = ctx.interpose("setCipherList", function (self, ciphers, ...)
	if (...) then
		local ciphers_t = pack(ciphers, ...)
		ciphers = table.concat(ciphers_t, ":", 1, ciphers_t.n)
	elseif type(ciphers) == "table" then
		ciphers = table.concat(ciphers, ":")
	end
	return setCipherList(self, ciphers)
end)

-- Allow passing a vararg of ciphersuites, or an array
local setCipherSuites = ctx.interpose("setCipherSuites", nil)
if setCipherSuites then
	ctx.interpose("setCipherSuites", function (self, ciphers, ...)
		if (...) then
			local ciphers_t = pack(ciphers, ...)
			ciphers = table.concat(ciphers_t, ":", 1, ciphers_t.n)
		elseif type(ciphers) == "table" then
			ciphers = table.concat(ciphers, ":")
		end
		return setCipherSuites(self, ciphers)
	end)
end

-- Allow passing a vararg of curves, or an array
local setGroups = ctx.interpose("setGroups", nil)
if setGroups then
	local function varargSetGroups(self, group, ...)
		if (...) then
			local group_t = pack(group, ...)
			group = table.concat(group_t, ":", 1, group_t.n)
		elseif type(group) == "table" then
			group = table.concat(group, ":")
		end
		return setGroups(self, group)
	end
	ctx.interpose("setGroups", varargSetGroups)
	ctx.interpose("setCurvesList", varargSetGroups)
end

return ctx
