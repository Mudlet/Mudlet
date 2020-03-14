-- Email Addresses
-- RFC 5322 Section 3.4.1

local lpeg = require "lpeg"
local P = lpeg.P
local R = lpeg.R
local S = lpeg.S
local V = lpeg.V
local C = lpeg.C
local Cg = lpeg.Cg
local Ct = lpeg.Ct
local Cs = lpeg.Cs

local core = require "lpeg_patterns.core"
local CHAR = core.CHAR
local CRLF = core.CRLF
local CTL = core.CTL
local DQUOTE = core.DQUOTE
local WSP = core.WSP
local VCHAR = core.VCHAR

local obs_NO_WS_CTL = R("\1\8", "\11\12", "\14\31") + P"\127"

local obs_qp = Cg(P"\\" * C(P"\0" + obs_NO_WS_CTL + core.LF + core.CR))
local quoted_pair = Cg(P"\\" * C(VCHAR + WSP)) + obs_qp

-- Folding White Space
local FWS = (WSP^0 * CRLF)^-1 * WSP^1 / " " -- Fold whitespace into a single " "

-- Comments
local ctext   = R"\33\39" + R"\42\91" + R"\93\126"
local comment = P {
	V"comment" ;
	ccontent = ctext + quoted_pair + V"comment" ;
	comment = P"("* (FWS^-1 * V"ccontent")^0 * FWS^-1 * P")";
}
local CFWS = ((FWS^-1 * comment)^1 * FWS^-1 + FWS ) / function() end

-- Atom
local specials      = S[=[()<>@,;:\".[]]=]
local atext         = CHAR-specials-P" "-CTL
local atom          = CFWS^-1 * C(atext^1) * CFWS^-1
local dot_atom_text = C(atext^1 * ( P"." * atext^1 )^0)
local dot_atom      = CFWS^-1 * dot_atom_text * CFWS^-1

-- Quoted Strings
local qtext              = S"\33"+R("\35\91","\93\126")
local qcontent           = qtext + quoted_pair
local quoted_string_text = DQUOTE * Cs((FWS^-1 * qcontent)^0 * FWS^-1) * DQUOTE
local quoted_string      = CFWS^-1 * quoted_string_text * CFWS^-1

-- Miscellaneous Tokens
local word = atom + quoted_string
local obs_phrase = C(word * (word + P"." + CFWS)^0 / function() end)
local phrase = obs_phrase -- obs_phrase is more broad than `word^1`, it's really the same but allows "."

-- Addr-spec
local obs_dtext = obs_NO_WS_CTL + quoted_pair
local dtext = R("\33\90", "\94\126") + obs_dtext
local domain_literal_text = P"[" * Cs((FWS^-1 * dtext)^0 * FWS^-1) * P"]"

local domain_text     = dot_atom_text + domain_literal_text
local local_part_text = dot_atom_text + quoted_string_text
local addr_spec_text  = local_part_text * P"@" * domain_text

local domain_literal = CFWS^-1 * domain_literal_text * CFWS^-1
local obs_domain = Ct(atom * (C"." * atom)^0) / table.concat
local domain = obs_domain + dot_atom + domain_literal
local obs_local_part = Ct(word * (C"." * word)^0) / table.concat
local local_part = obs_local_part + dot_atom + quoted_string
local addr_spec      = local_part * P"@" * domain

local display_name = phrase
local obs_domain_list = (CFWS + P",")^0 * P"@" * domain
	* (P"," * CFWS^-1 * (P"@" * domain)^-1)^0
local obs_route = Cg(Ct(obs_domain_list) * P":", "route")
local obs_angle_addr = CFWS^-1 * P"<" * obs_route * addr_spec * P">" * CFWS^-1
local angle_addr = CFWS^-1 * P"<" * addr_spec * P">" * CFWS^-1
	+ obs_angle_addr
local name_addr = Cg(display_name, "display")^-1 * angle_addr
local mailbox = name_addr + addr_spec

return {
	local_part = local_part;
	domain = domain;
	email = addr_spec;
	name_addr = name_addr;
	mailbox = mailbox;

	-- A variant that does not allow comments or folding whitespace
	local_part_nocfws = local_part_text;
	domain_nocfws = domain_text;
	email_nocfws = addr_spec_text;
}
