-- IPv4

local lpeg = require "lpeg"
local P = lpeg.P
local R = lpeg.R
local Cg = lpeg.Cg

local core = require "lpeg_patterns.core"
local DIGIT = core.DIGIT

local dec_octet = (
		P"1"  * DIGIT * DIGIT
		+ P"2"  * (R"04"*DIGIT + P"5"*R"05")
		+ DIGIT * DIGIT^-1
	) / tonumber

local IPv4_methods = {}
local IPv4_mt = {
	__name = "lpeg_patterns.IPv4";
	__index = IPv4_methods;
}

local function new_IPv4 ( o1 , o2 , o3 , o4 )
	return setmetatable({o1, o2, o3, o4}, IPv4_mt)
end

function IPv4_methods:unpack()
	return self[1], self[2], self[3], self[4]
end

function IPv4_methods:binary()
	return string.char(self:unpack())
end

function IPv4_mt:__tostring ( )
	return string.format("%d.%d.%d.%d", self:unpack())
end

local IPv4address = Cg ( dec_octet * P"." * dec_octet * P"." * dec_octet * P"." * dec_octet ) / new_IPv4

return {
	IPv4_methods = IPv4_methods;
	IPv4_mt = IPv4_mt;
	IPv4address = IPv4address;
}
