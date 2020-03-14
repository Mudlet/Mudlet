-- IPv6

local unpack = table.unpack or unpack -- luacheck: ignore 113 143

local lpeg = require "lpeg"
local P = lpeg.P
local V = lpeg.V
local Cc = lpeg.Cc
local Cg = lpeg.Cg

local util = require "lpeg_patterns.util"

local core = require "lpeg_patterns.core"
local HEXDIG = core.HEXDIG

local IPv4address = require "lpeg_patterns.IPv4".IPv4address

local IPv6_methods = {}
local IPv6_mt = {
	__name = "lpeg_patterns.IPv6";
	__index = IPv6_methods;
}

local function new_IPv6(o1, o2, o3, o4, o5, o6, o7, o8, zoneid)
	return setmetatable({
		o1, o2, o3, o4, o5, o6, o7, o8,
		zoneid = zoneid;
	}, IPv6_mt)
end

function IPv6_methods:unpack()
	return self[1], self[2], self[3], self[4], self[5], self[6], self[7], self[8], self.zoneid
end

function IPv6_methods:binary()
	local t = {}
	for i=1, 8 do
		local lo = self[i] % 256
		t[i*2-1] = (self[i] - lo) / 256
		t[i*2] = lo
	end
	-- TODO: append zoneid.
	-- In a struct sockaddr_in6 it is the numeric index of the scope, so need to lookup?
	return string.char(unpack(t, 1, 16))
end

function IPv6_methods:setzoneid(zoneid)
	self.zoneid = zoneid
end

function IPv6_mt:__tostring()
	local fmt_str
	if self.zoneid then
		fmt_str = "%x:%x:%x:%x:%x:%x:%x:%x%%%s"
	else
		fmt_str = "%x:%x:%x:%x:%x:%x:%x:%x"
	end
	return string.format(fmt_str, self:unpack())
end

-- RFC 3986 Section 3.2.2
-- This is written as a grammar to reduce memory usage
local raw_IPv6address = Cg(P{
	h16 = HEXDIG * HEXDIG^-3 / util.read_hex;
	h16c = V"h16" * P":";
	ls32 = ( V"h16c" * V"h16" ) + IPv4address / function ( ipv4 )
		local o1, o2, o3, o4 = ipv4:unpack()
		return o1*2^8 + o2 , o3*2^8 + o4
	end;

	mh16c_1 = V"h16c";
	mh16c_2 = V"h16c" * V"h16c";
	mh16c_3 = V"h16c" * V"h16c" * V"h16c";
	mh16c_4 = V"h16c" * V"h16c" * V"h16c" * V"h16c";
	mh16c_5 = V"h16c" * V"h16c" * V"h16c" * V"h16c" * V"h16c";
	mh16c_6 = V"h16c" * V"h16c" * V"h16c" * V"h16c" * V"h16c" * V"h16c";

	mcc_1 = P"::" * Cc(0);
	mcc_2 = P"::" * Cc(0, 0);
	mcc_3 = P"::" * Cc(0, 0, 0);
	mcc_4 = P"::" * Cc(0, 0, 0, 0);
	mcc_5 = P"::" * Cc(0, 0, 0, 0, 0);
	mcc_6 = P"::" * Cc(0, 0, 0, 0, 0, 0);
	mcc_7 = P"::" * Cc(0, 0, 0, 0, 0, 0, 0);
	mcc_8 = P"::" * Cc(0, 0, 0, 0, 0, 0, 0, 0);

	mh16_1 = V"h16";
	mh16_2 = V"mh16c_1" * V"h16";
	mh16_3 = V"mh16c_2" * V"h16";
	mh16_4 = V"mh16c_3" * V"h16";
	mh16_5 = V"mh16c_4" * V"h16";
	mh16_6 = V"mh16c_5" * V"h16";
	mh16_7 = V"mh16c_6" * V"h16";

	                         V"mh16c_6" * V"ls32"
	+             V"mcc_1" * V"mh16c_5" * V"ls32"
	+             V"mcc_2" * V"mh16c_4" * V"ls32"
	+ V"h16"    * V"mcc_1" * V"mh16c_4" * V"ls32"
	+             V"mcc_3" * V"mh16c_3" * V"ls32"
	+ V"h16"    * V"mcc_2" * V"mh16c_3" * V"ls32"
	+ V"mh16_2" * V"mcc_1" * V"mh16c_3" * V"ls32"
	+             V"mcc_4" * V"mh16c_2" * V"ls32"
	+ V"h16"    * V"mcc_3" * V"mh16c_2" * V"ls32"
	+ V"mh16_2" * V"mcc_2" * V"mh16c_2" * V"ls32"
	+ V"mh16_3" * V"mcc_1" * V"mh16c_2" * V"ls32"
	+             V"mcc_5" * V"h16c"    * V"ls32"
	+ V"h16"    * V"mcc_4" * V"h16c"    * V"ls32"
	+ V"mh16_2" * V"mcc_3" * V"h16c"    * V"ls32"
	+ V"mh16_3" * V"mcc_2" * V"h16c"    * V"ls32"
	+ V"mh16_4" * V"mcc_1" * V"h16c"    * V"ls32"
	+             V"mcc_6"              * V"ls32"
	+ V"h16"    * V"mcc_5"              * V"ls32"
	+ V"mh16_2" * V"mcc_4"              * V"ls32"
	+ V"mh16_3" * V"mcc_3"              * V"ls32"
	+ V"mh16_4" * V"mcc_2"              * V"ls32"
	+ V"mh16_5" * V"mcc_1"              * V"ls32"
	+             V"mcc_7" * V"h16"
	+ V"h16"    * V"mcc_6" * V"h16"
	+ V"mh16_2" * V"mcc_5" * V"h16"
	+ V"mh16_3" * V"mcc_4" * V"h16"
	+ V"mh16_4" * V"mcc_3" * V"h16"
	+ V"mh16_5" * V"mcc_2" * V"h16"
	+ V"mh16_6" * V"mcc_1" * V"h16"
	+             V"mcc_8"
	+ V"mh16_1" * V"mcc_7"
	+ V"mh16_2" * V"mcc_6"
	+ V"mh16_3" * V"mcc_5"
	+ V"mh16_4" * V"mcc_4"
	+ V"mh16_5" * V"mcc_3"
	+ V"mh16_6" * V"mcc_2"
	+ V"mh16_7" * V"mcc_1"
})

local IPv6address = raw_IPv6address / new_IPv6

local ZoneID = P(1)^1 -- ZoneIDs can be any character
local IPv6addrz = raw_IPv6address * (P"%" * ZoneID)^-1 / new_IPv6

return {
	IPv6_methods = IPv6_methods;
	IPv6_mt = IPv6_mt;
	IPv6address = IPv6address;
	IPv6addrz = IPv6addrz;
}
