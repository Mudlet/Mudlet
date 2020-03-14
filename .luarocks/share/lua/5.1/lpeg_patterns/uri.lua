-- URI
-- RFC 3986

local lpeg = require "lpeg"
local P = lpeg.P
local S = lpeg.S
local C = lpeg.C
local Cc = lpeg.Cc
local Cg = lpeg.Cg
local Cs = lpeg.Cs
local Ct = lpeg.Ct

local util = require "lpeg_patterns.util"

local core = require "lpeg_patterns.core"
local ALPHA = core.ALPHA
local DIGIT = core.DIGIT
local HEXDIG = core.HEXDIG

local IPv4address = require "lpeg_patterns.IPv4".IPv4address
local IPv6address = require "lpeg_patterns.IPv6".IPv6address

local _M = {}

_M.sub_delims = S"!$&'()*+,;=" -- 2.2
local unreserved  = ALPHA + DIGIT + S"-._~" -- 2.3
_M.pct_encoded = P"%" * (HEXDIG * HEXDIG / util.read_hex) / function(n)
	local c = string.char(n)
	if unreserved:match(c) then
		-- always decode unreserved characters (2.3)
		return c
	else
		-- normalise to upper-case (6.2.2.1)
		return string.format("%%%02X", n)
	end
end -- 2.1

_M.scheme = ALPHA * (ALPHA + DIGIT + S"+-.")^0 / string.lower -- 3.1

_M.userinfo = Cs((unreserved + _M.pct_encoded + _M.sub_delims + P":")^0) -- 3.2.1

-- Host 3.2.2

local IPvFuture_mt = {
	__name = "lpeg_patterns.IPvFuture";
}
function IPvFuture_mt:__tostring()
	return string.format("v%x.%s", self.version, self.string)
end
local function new_IPvFuture(version, string)
	return setmetatable({version=version, string=string}, IPvFuture_mt)
end
local IPvFuture = S"vV" * (HEXDIG^1/util.read_hex) * P"." * C((unreserved+_M.sub_delims+P":")^1) / new_IPvFuture

-- RFC 6874
local ZoneID = Cs((unreserved + _M.pct_encoded)^1)
local IPv6addrz   = IPv6address * (P"%25" * ZoneID)^-1 / function(IPv6, zoneid)
	IPv6:setzoneid(zoneid)
	return IPv6
end

_M.IP_literal = P"[" * (IPv6addrz + IPvFuture) * P"]"
local IP_host = (_M.IP_literal + IPv4address) / tostring
local reg_name = Cs((
	unreserved / string.lower
	+ _M.pct_encoded / function(s) return s:sub(1,1) == "%" and s or string.lower(s) end
	+ _M.sub_delims
)^1) + Cc(nil)
_M.host = IP_host + reg_name

_M.port = DIGIT^0 / tonumber -- 3.2.3

-- Path 3.3
local pchar = unreserved + _M.pct_encoded + _M.sub_delims + S":@"
local segment = pchar^0
_M.segment = Cs(segment)
local segment_nz = pchar^1
local segment_nz_nc = (pchar - P":")^1

-- an empty path is nil instead of the empty string
local path_empty    = Cc(nil)
local path_abempty = Cs((P"/" * segment)^1) + path_empty
local path_rootless = Cs(segment_nz * (P"/" * segment)^0)
local path_noscheme = Cs(segment_nz_nc * (P"/" * segment)^0)
local path_absolute = Cs(P"/" * (segment_nz * (P"/" * segment)^0)^-1)

_M.query = Cs( ( pchar + S"/?" )^0 ) -- 3.4
_M.fragment = _M.query -- 3.5

-- Put together with named captures
_M.authority = ( Cg(_M.userinfo, "userinfo") * P"@" )^-1
	* Cg(_M.host, "host")
	* ( P":" * Cg(_M.port, "port") )^-1

local hier_part = P"//" * _M.authority * Cg (path_abempty, "path")
	+ Cg(path_absolute + path_rootless + path_empty, "path")

_M.absolute_uri = Ct (
	( Cg(_M.scheme, "scheme") * P":" )
	* hier_part
	* ( P"?" * Cg(_M.query, "query"))^-1
)

_M.uri = Ct (
	( Cg(_M.scheme, "scheme") * P":" )
	* hier_part
	* ( P"?" * Cg(_M.query, "query"))^-1
	* ( P"#" * Cg(_M.fragment, "fragment"))^-1
)

_M.relative_part = P"//" * _M.authority * Cg(path_abempty, "path")
	+ Cg(path_absolute + path_noscheme + path_empty, "path")

local relative_ref = Ct (
	_M.relative_part
	* ( P"?" * Cg(_M.query, "query"))^-1
	* ( P"#" * Cg(_M.fragment, "fragment"))^-1
)
_M.uri_reference = _M.uri + relative_ref

_M.path = path_abempty + path_absolute + path_noscheme + path_rootless + path_empty

-- Create a slightly more sane host pattern
-- scheme is optional
-- the "//" isn't required
	-- if missing, the host needs to at least have a "." and end in two alpha characters
-- an authority is always required
local sane_host_char = unreserved / string.lower
local hostsegment = (sane_host_char - P".")^1
local dns_entry   = Cs ( ( hostsegment * P"." )^1 * ALPHA^2 )
_M.sane_host = IP_host + dns_entry
_M.sane_authority = ( Cg(_M.userinfo, "userinfo") * P"@" )^-1
	* Cg(_M.sane_host, "host")
	* ( P":" * Cg(_M.port, "port") )^-1
local sane_hier_part = (P"//")^-1 * _M.sane_authority * Cg(path_absolute + path_empty, "path")
_M.sane_uri = Ct (
	( Cg(_M.scheme, "scheme") * P":" )^-1
	* sane_hier_part
	* ( P"?" * Cg(_M.query, "query"))^-1
	* ( P"#" * Cg(_M.fragment, "fragment"))^-1
)

return _M
