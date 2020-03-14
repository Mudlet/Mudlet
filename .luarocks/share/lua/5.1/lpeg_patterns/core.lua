-- Core Rules
-- https://tools.ietf.org/html/rfc5234#appendix-B.1

local lpeg = require "lpeg"

local P = lpeg.P
local R = lpeg.R
local S = lpeg.S

local _M = { }

_M.ALPHA = R("AZ","az")
_M.BIT   = S"01"
_M.CHAR  = R"\1\127"
_M.CR    = P"\r"
_M.CRLF  = P"\r\n"
_M.CTL   = R"\0\31" + P"\127"
_M.DIGIT = R"09"
_M.DQUOTE= P'"'
_M.HEXDIG= _M.DIGIT + S"ABCDEFabcdef"
_M.HTAB  = P"\t"
_M.LF    = P"\n"
_M.OCTET = P(1)
_M.SP    = P" "
_M.VCHAR = R"\33\126"
_M.WSP   = S" \t"

_M.LWSP  = (_M.WSP + _M.CRLF*_M.WSP)^0

return _M
