-- RFC 5646 Section 2.1

local lpeg = require "lpeg"
local core = require "lpeg_patterns.core"

local C = lpeg.C
local P = lpeg.P
local R = lpeg.R
local Cg = lpeg.Cg
local Ct = lpeg.Ct
local Cmt = lpeg.Cmt

local M = {}

local alphanum = core.ALPHA + core.DIGIT

local extlang = core.ALPHA * core.ALPHA * core.ALPHA * -#alphanum
	* (P"-" * core.ALPHA * core.ALPHA * core.ALPHA * -#alphanum)^-2

local language = Cg(core.ALPHA * core.ALPHA * core.ALPHA * core.ALPHA * core.ALPHA * core.ALPHA^-3, "language")
	+ Cg(core.ALPHA * core.ALPHA * core.ALPHA * core.ALPHA, "language")
	+ Cg(core.ALPHA * core.ALPHA * core.ALPHA^-1, "language") * (P"-" * Cg(extlang, "extlang"))^-1

local script = core.ALPHA * core.ALPHA * core.ALPHA * core.ALPHA
	* -#alphanum -- Prevent intepretation of a 'variant'

local region = (
	core.ALPHA * core.ALPHA
	+ core.DIGIT * core.DIGIT * core.DIGIT
) * -#alphanum -- Prevent intepretation of a 'variant'

local variant = core.DIGIT * alphanum * alphanum * alphanum
	+ alphanum * alphanum * alphanum * alphanum * alphanum * alphanum^-3

local singleton = core.DIGIT + R("AW", "YZ", "aw", "yz")

local extension = C(singleton) * Ct((P"-" * (alphanum*alphanum*alphanum^-6 / string.lower))^1)

M.privateuse = P"x" * Ct((P"-" * C(alphanum*alphanum^-7))^1)

M.langtag = language
	* (P"-" * Cg(script, "script"))^-1
	* (P"-" * Cg(region, "region"))^-1
	* Cg(Ct((P"-" * C(variant))^1), "variant")^-1
	* Cg(Cmt(Ct((P"-" * Ct(extension))^1), function(_, _, c)
		-- Can't use a fold with rawset as we want the pattern to not match if there is a duplicate extension
		local r = {}
		for _, v in ipairs(c) do
			local a, b = v[1], v[2]
			if r[a] then
				-- duplicate extension
				return false
			end
			r[a] = b
		end
		return true, r
	end), "extension")^-1
	* (P"-" * Cg(M.privateuse, "privateuse"))^-1

local irregular = P"en-GB-oed"
	+ P"i-ami"
	+ P"i-bnn"
	+ P"i-default"
	+ P"i-enochian"
	+ P"i-hak"
	+ P"i-klingon"
	+ P"i-lux"
	+ P"i-mingo"
	+ P"i-navajo"
	+ P"i-pwn"
	+ P"i-tao"
	+ P"i-tay"
	+ P"i-tsu"
	+ P"sgn-BE-FR"
	+ P"sgn-BE-NL"
	+ P"sgn-CH-DE"

M.Language_Tag = C((M.langtag
	+ M.privateuse
	+ irregular) / function() end) -- capture the whole tag. throws away decomposition

return M
