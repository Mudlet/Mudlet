--[[
https://tools.ietf.org/html/rfc7230
https://tools.ietf.org/html/rfc7231
]]

local lpeg = require "lpeg"
local core = require "lpeg_patterns.core"
local email = require "lpeg_patterns.email"
local language = require "lpeg_patterns.language"
local uri = require "lpeg_patterns.uri"
local util = require "lpeg_patterns.util"

local C = lpeg.C
local Cc = lpeg.Cc
local Cf = lpeg.Cf
local Cg = lpeg.Cg
local Cs = lpeg.Cs
local Ct = lpeg.Ct
local Cmt = lpeg.Cmt
local P = lpeg.P
local R = lpeg.R
local S = lpeg.S
local V = lpeg.V

local _M = {}

local T_F = S"Tt" * Cc(true) + S"Ff" * Cc(false)

local function no_rich_capture(patt)
	return C(patt) / function(a) return a end
end

local function case_insensitive(str)
	local patt = P(true)
	for i=1, #str do
		local c = str:sub(i, i)
		patt = patt * S(c:upper() .. c:lower())
	end
	return patt
end

-- Helper function that doesn't match if there are duplicate keys
local function no_dup_cmt(s, i, t, name, value, ...)
	local old = t[name]
	if old then
		return false
	end
	t[name] = value
	if ... then
		return no_dup_cmt(s, i, t, ...)
	elseif t["max-age"] then -- max-age is required
		return true, t
	end
	-- else return nil
end

local function no_dup(patt)
	return Cmt(Ct(true) * patt, no_dup_cmt)
end

-- RFC 7230 Section 3.2.3
_M.OWS = (core.SP + core.HTAB)^0
_M.RWS = (core.SP + core.HTAB)^1
_M.BWS = _M.OWS

-- RFC 5023
local slugtext = _M.RWS / " "
	+ P"%" * (core.HEXDIG * core.HEXDIG / util.read_hex) / string.char
	+ R"\32\126"
_M.SLUG = Cs(slugtext^0)

-- RFC 6454
-- discard captures from scheme, host, port and just get whole string
local serialized_origin = C(uri.scheme * P"://" * uri.host * (P":" * uri.port)^-1/function() end)
local origin_list = serialized_origin * (core.SP * serialized_origin)^0
local origin_list_or_null = P"null" + origin_list
_M.Origin = _M.OWS * origin_list_or_null * _M.OWS

-- Analogue to RFC 7230 Section 7's ABNF extension of '#'
-- Also documented as `#rule` under RFC 2616 Section 2.1
local comma_sep, comma_sep_trim do
	local sep = _M.OWS * lpeg.P "," * _M.OWS
	local optional_sep = (lpeg.P"," + core.SP + core.HTAB)^0
	comma_sep = function(element, min, max)
		local extra = sep * optional_sep * element
		local patt = element
		if min then
			for _=2, min do
				patt = patt * extra
			end
		else
			min = 0
			patt = patt^-1
		end
		if max then
			local more = max-min-1
			patt = patt * extra^-more
		else
			patt = patt * extra^0
		end
		return patt
	end
	-- allows leading + trailing
	comma_sep_trim = function (...)
		return optional_sep * comma_sep(...) * optional_sep
	end
end

-- RFC 7034
_M.X_Frame_Options = case_insensitive "deny" * Cc("deny")
	+ case_insensitive "sameorigin" * Cc("sameorigin")
	+ case_insensitive "allow-from" * _M.RWS * serialized_origin

-- RFC 7230 Section 2.6
local HTTP_name = P"HTTP"
local HTTP_version = HTTP_name * P"/" * (core.DIGIT * P"." * core.DIGIT / util.safe_tonumber)

-- RFC 7230 Section 2.7
local absolute_path = (P"/" * uri.segment )^1
local partial_uri = Ct(uri.relative_part * (P"?" * uri.query)^-1)

-- RFC 7230 Section 3.2.6
local tchar = S "!#$%&'*+-.^_`|~" + core.DIGIT + core.ALPHA
_M.token = C(tchar^1)
local obs_text = R("\128\255")
_M.qdtext = core.HTAB + core.SP + P"\33" + R("\35\91", "\93\126") + obs_text
local quoted_pair = Cs(P"\\" * C(core.HTAB + core.SP + core.VCHAR + obs_text) / "%1")
_M.quoted_string = core.DQUOTE * Cs((_M.qdtext + quoted_pair)^0) * core.DQUOTE

local ctext = core.HTAB + core.SP + R("\33\39", "\42\91", "\93\126") + obs_text
_M.comment = P { P"(" * ( ctext + quoted_pair + V(1) )^0 * P")" }

-- RFC 7230 Section 3.2
_M.field_name = _M.token / string.lower -- case insensitive
local field_vchar = core.VCHAR + obs_text
local field_content = field_vchar * (( core.SP + core.HTAB )^1 * field_vchar)^-1
local obs_fold = ( core.SP + core.HTAB )^0 * core.CRLF * ( core.SP + core.HTAB )^1 / " "
-- field_value is not correct, see Errata: https://www.rfc-editor.org/errata_search.php?rfc=7230&eid=4189
_M.field_value = Cs((field_content + obs_fold)^0)
_M.header_field = _M.field_name * P":" * _M.OWS * _M.field_value * _M.OWS

-- RFC 7230 Section 3.3.2
_M.Content_Length = core.DIGIT^1

-- RFC 7230 Section 4
-- See https://www.rfc-editor.org/errata_search.php?rfc=7230&eid=4683
local transfer_parameter = (_M.token - S"qQ" * _M.BWS * P"=") * _M.BWS * P"=" * _M.BWS * ( _M.token + _M.quoted_string )
local transfer_extension = Cf(Ct(_M.token / string.lower) -- case insensitive
	* ( _M.OWS * P";" * _M.OWS * Cg(transfer_parameter) )^0, rawset)
local transfer_coding = transfer_extension

-- RFC 7230 Section 3.3.1
_M.Transfer_Encoding = comma_sep_trim(transfer_coding, 1)

-- RFC 7230 Section 4.1.1
local chunk_ext_name = _M.token
local chunk_ext_val = _M.token + _M.quoted_string
-- See https://www.rfc-editor.org/errata_search.php?rfc=7230&eid=4667
_M.chunk_ext = ( P";" * chunk_ext_name * ( P"=" * chunk_ext_val)^-1 )^0

-- RFC 7230 Section 4.3
local rank = (P"0" * ((P"." * core.DIGIT^-3) / util.safe_tonumber + Cc(0)) + P"1" * ("." * (P"0")^-3)^-1) * Cc(1)
local t_ranking = _M.OWS * P";" * _M.OWS * S"qQ" * P"=" * rank -- q is case insensitive
local t_codings = (transfer_coding * t_ranking^-1) / function(t, q)
	if q then
		t["q"] = q
	end
	return t
end
_M.TE = comma_sep_trim(t_codings)

-- RFC 7230 Section 4.4
_M.Trailer = comma_sep_trim(_M.field_name, 1)

-- RFC 7230 Section 5.3
local origin_form = Cs(absolute_path * (P"?" * uri.query)^-1)
local absolute_form = no_rich_capture(uri.absolute_uri)
local authority_form = no_rich_capture(uri.authority)
local asterisk_form = C"*"
_M.request_target = asterisk_form + origin_form + absolute_form + authority_form

-- RFC 7230 Section 3.1.1
local method = _M.token
_M.request_line = method * core.SP * _M.request_target * core.SP * HTTP_version * core.CRLF

-- RFC 7230 Section 5.4
_M.Host = uri.host * (P":" * uri.port)^-1

-- RFC 7230 Section 6.7
local protocol_name = _M.token
local protocol_version = _M.token
local protocol = protocol_name * (P"/" * protocol_version)^-1 / "%0"
_M.Upgrade = comma_sep_trim(protocol)

-- RFC 7230 Section 5.7.1
local received_protocol = (protocol_name * P"/" + Cc("HTTP")) * protocol_version / "%1/%2"
local pseudonym = _M.token
-- workaround for https://lists.w3.org/Archives/Public/ietf-http-wg/2016OctDec/0527.html
local received_by = uri.host * ((P":" * uri.port) + -lpeg.B(",")) / "%0" + pseudonym
_M.Via = comma_sep_trim(Ct(
	Cg(received_protocol, "protocol")
	* _M.RWS * Cg(received_by, "by")
	* (_M.RWS * Cg(_M.comment, "comment"))^-1
), 1)

-- RFC 7230 Section 6.1
local connection_option = _M.token / string.lower -- case insensitive
_M.Connection = comma_sep_trim(connection_option)

-- RFC 7231 Section 3.1.1
local content_coding = _M.token / string.lower -- case insensitive
_M.Content_Encoding = comma_sep_trim(content_coding, 1)

-- RFC 7231 Section 3.1.2
local type = _M.token / string.lower -- case insensitive
local subtype = _M.token / string.lower -- case insensitive
local parameter = _M.token / string.lower -- case insensitive
	* P"=" * (_M.token + _M.quoted_string)
local media_type = Cg(type, "type") * P"/" * Cg(subtype, "subtype")
	* Cg(Cf(Ct(true) * (_M.OWS * P";" * _M.OWS * Cg(parameter))^0, rawset), "parameters")
local charset = _M.token / string.lower -- case insensitive
_M.Content_Type = Ct(media_type)

-- RFC 7231 Section 3.1.3
_M.Content_Language = comma_sep_trim(language.Language_Tag, 1)

-- RFC 7231 Section 3.1.4.2
_M.Content_Location = uri.absolute_uri + partial_uri

-- RFC 7231 Section 5.1.1
_M.Expect = P"100-"*S"cC"*S"oO"*S"nN"*S"tT"*S"iI"*S"nN"*S"uU"*S"eE" * Cc("100-continue")

-- RFC 7231 Section 5.1.2
_M.Max_Forwards = core.DIGIT^1 / tonumber

-- RFC 7231 Section 5.3.1
local qvalue = rank -- luacheck: ignore 211
local weight = t_ranking

-- RFC 7231 Section 5.3.2
local media_range = (P"*/*"
	+ (Cg(type, "type") * P"/*")
	+ (Cg(type, "type") * P"/" * Cg(subtype, "subtype"))
) * Cg(Cf(Ct(true) * (_M.OWS * ";" * _M.OWS * Cg(parameter) - weight)^0, rawset), "parameters")
local accept_ext = _M.OWS * P";" * _M.OWS * _M.token * (P"=" * (_M.token + _M.quoted_string))^-1
local accept_params = Cg(weight, "q") * Cg(Cf(Ct(true) * Cg(accept_ext)^0, rawset), "extensions")
_M.Accept = comma_sep_trim(Ct(media_range * (accept_params+Cg(Ct(true), "extensions"))))

-- RFC 7231 Section 5.3.3
_M.Accept_Charset = comma_sep_trim((charset + P"*") * weight^-1, 1)

-- RFC 7231 Section 5.3.4
local codings = content_coding + "*"
_M.Accept_Encoding = comma_sep_trim(codings * weight^-1)

-- RFC 4647 Section 2.1
local alphanum = core.ALPHA + core.DIGIT
local language_range = (core.ALPHA * core.ALPHA^-7 * (P"-" * alphanum * alphanum^-7)^0) + P"*"
-- RFC 7231 Section 5.3.5
_M.Accept_Language = comma_sep_trim(language_range * weight^-1, 1)

-- RFC 7231 Section 5.5.1
_M.From = email.mailbox

-- RFC 7231 Section 5.5.2
_M.Referer = uri.absolute_uri + partial_uri

-- RFC 7231 Section 5.5.3
local product_version = _M.token
local product = _M.token * (P"/" * product_version)^-1
_M.User_Agent = product * (_M.RWS * (product + _M.comment))^0

-- RFC 7231 Section 7.1.1.1
-- Uses os.date field names
local day_name = Cg(P"Mon"*Cc(2)
	+ P"Tue"*Cc(3)
	+ P"Wed"*Cc(4)
	+ P"Thu"*Cc(5)
	+ P"Fri"*Cc(6)
	+ P"Sat"*Cc(7)
	+ P"Sun"*Cc(1), "wday")
local day = Cg(core.DIGIT * core.DIGIT / tonumber, "day")
local month = Cg(P"Jan"*Cc(1)
	+ P"Feb"*Cc(2)
	+ P"Mar"*Cc(3)
	+ P"Apr"*Cc(4)
	+ P"May"*Cc(5)
	+ P"Jun"*Cc(6)
	+ P"Jul"*Cc(7)
	+ P"Aug"*Cc(8)
	+ P"Sep"*Cc(9)
	+ P"Oct"*Cc(10)
	+ P"Nov"*Cc(11)
	+ P"Dec"*Cc(12), "month")
local year = Cg(core.DIGIT * core.DIGIT * core.DIGIT * core.DIGIT / tonumber, "year")
local date1 = day * core.SP * month * core.SP * year

local GMT = P"GMT"

local minute = Cg(core.DIGIT * core.DIGIT / tonumber, "min")
local second = Cg(core.DIGIT * core.DIGIT / tonumber, "sec")
local hour = Cg(core.DIGIT * core.DIGIT / tonumber, "hour")
-- XXX only match 00:00:00 - 23:59:60 (leap second)?

local time_of_day = hour * P":" * minute * P":" * second
_M.IMF_fixdate = Ct(day_name * P"," * core.SP * date1 * core.SP * time_of_day * core.SP * GMT)

local date2 do
	local year_barrier = 70
	local twodayyear = Cg(core.DIGIT * core.DIGIT / function(y)
		y = tonumber(y, 10)
		if y < year_barrier then
			return 2000+y
		else
			return 1900+y
		end
	end, "year")
	date2 = day * P"-" * month * P"-" * twodayyear
end
local day_name_l = Cg(P"Monday"*Cc(2)
	+ P"Tuesday"*Cc(3)
	+ P"Wednesday"*Cc(4)
	+ P"Thursday"*Cc(5)
	+ P"Friday"*Cc(6)
	+ P"Saturday"*Cc(7)
	+ P"Sunday"*Cc(1), "wday")
local rfc850_date = Ct(day_name_l * P"," * core.SP * date2 * core.SP * time_of_day * core.SP * GMT)

local date3 = month * core.SP * (day + Cg(core.SP * core.DIGIT / tonumber, "day"))
local asctime_date = Ct(day_name * core.SP * date3 * core.SP * time_of_day * core.SP * year)
local obs_date = rfc850_date + asctime_date

local HTTP_date = _M.IMF_fixdate + obs_date
_M.Date = HTTP_date

-- RFC 7231 Section 7.1.2
_M.Location = uri.uri_reference

-- RFC 7231 Section 7.1.3
local delay_seconds = core.DIGIT^1 / tonumber
_M.Retry_After = HTTP_date + delay_seconds

-- RFC 7231 Section 7.1.4
_M.Vary = P"*" + comma_sep(_M.field_name, 1)

-- RFC 7231 Section 7.4.1
_M.Allow = comma_sep_trim(method)

-- RFC 7231 Section 7.4.2
_M.Server = product * (_M.RWS * (product + _M.comment))^0

-- RFC 5789
_M.Accept_Patch = comma_sep_trim(media_type, 1)

-- RFC 5987
local attr_char = core.ALPHA + core.DIGIT + S"!#$&+-.^_`|~"
-- can't use uri.pct_encoded, as it doesn't decode all characters
local pct_encoded = P"%" * (core.HEXDIG * core.HEXDIG / util.read_hex) / string.char
local value_chars = Cs((pct_encoded + attr_char)^0)
local parmname = C(attr_char^1)
local ext_value do
	-- ext-value uses charset from RFC 5987 instead
	local mime_charsetc = core.ALPHA + core.DIGIT + S"!#$%&+-^_`{}~"
	local mime_charset = C(mime_charsetc^1)
	ext_value = Cg(mime_charset, "charset") * P"'" * Cg(language.Language_Tag, "language")^-1 * P"'" * value_chars
end

do -- RFC 5988
	local ptokenchar = S"!#$%&'()*+-./:<=>?@[]^_`{|}~" + core.DIGIT + core.ALPHA
	local ptoken = ptokenchar^1
	local ext_name_star = parmname * P"*"
	local link_extension = ext_name_star * P"=" * ext_value
		+ parmname * (P"=" * (ptoken + _M.quoted_string))^-1
	-- See https://www.rfc-editor.org/errata_search.php?rfc=5988&eid=3158
	local link_param = link_extension
	local link_value = Cf(Ct(P"<" * uri.uri_reference * P">") * (_M.OWS * P";" * _M.OWS * Cg(link_param))^0, rawset)
	-- TODO: handle multiple ext_value variants...
	-- e.g. server might provide one title in english, one in chinese, client should be able to pick which one to display
	_M.Link = comma_sep_trim(link_value)
end

do -- RFC 6265
	local cookie_name = _M.token
	local cookie_octet = S"!" + R("\35\43", "\45\58", "\60\91", "\93\126")
	local cookie_value = core.DQUOTE * C(cookie_octet^0) * core.DQUOTE + C(cookie_octet^0)
	local cookie_pair = cookie_name * _M.BWS * P"=" * _M.BWS * cookie_value * _M.BWS

	local ext_char = core.CHAR - core.CTL - S";"
	ext_char = ext_char - core.WSP + core.WSP * #(core.WSP^0 * ext_char) -- No trailing whitespace
	-- Complexity is to make sure whitespace before an `=` isn't captured
	local extension_av = ((ext_char - S"=" - core.WSP) + core.WSP^1 * #(1-S"="))^0 / string.lower
			* _M.BWS * P"=" * _M.BWS * C(ext_char^0)
		+ (ext_char)^0 / string.lower * Cc(true)
	local cookie_av = extension_av
	local set_cookie_string = cookie_pair * Cf(Ct(true) * (P";" * _M.OWS * Cg(cookie_av))^0, rawset)
	_M.Set_Cookie = set_cookie_string

	local cookie_string = Cf(Ct(true) * Cg(cookie_pair) * (P";" * _M.OWS * Cg(cookie_pair))^0, rawset)
	_M.Cookie = cookie_string
end

do -- RFC 6266
	local disp_ext_type = _M.token / string.lower
	local disposition_type = disp_ext_type
	-- can't use 'token' here as we need to not include the "*" at the end
	local ext_token = C((tchar-P"*"*(-tchar))^1) * P"*"
	local value = _M.token + _M.quoted_string
	local disp_ext_parm = ext_token * _M.OWS * P"=" * _M.OWS * ext_value
		+ _M.token * _M.OWS * P"=" * _M.OWS * value
	local disposition_parm = disp_ext_parm
	_M.Content_Disposition = disposition_type * Cf(Ct(true) * (_M.OWS * P";" * _M.OWS * Cg(disposition_parm))^0, rawset)
end

-- RFC 6455
local base64_character = core.ALPHA + core.DIGIT + S"+/"
local base64_data = base64_character * base64_character * base64_character * base64_character
local base64_padding = base64_character * base64_character * P"=="
	+ base64_character * base64_character * base64_character * P"="
local base64_value_non_empty = (base64_data^1 * base64_padding^-1) + base64_padding
_M.Sec_WebSocket_Accept = base64_value_non_empty
_M.Sec_WebSocket_Key = base64_value_non_empty
local registered_token = _M.token
local extension_token = registered_token
local extension_param do
	local EOF = P(-1)
	local token_then_EOF = Cc(true) * _M.token * EOF
	-- the quoted-string must be a valid token
	local quoted_token = Cmt(_M.quoted_string, function(_, _, q)
		return token_then_EOF:match(q)
	end)
	extension_param = _M.token * ((P"=" * (_M.token + quoted_token)) + Cc(true))
end
local extension = extension_token * Cg(Cf(Ct(true) * (P";" * Cg(extension_param))^0, rawset), "parameters")
local extension_list = comma_sep_trim(Ct(extension))
_M.Sec_WebSocket_Extensions = extension_list
_M.Sec_WebSocket_Protocol_Client = comma_sep_trim(_M.token)
_M.Sec_WebSocket_Protocol_Server = _M.token
local NZDIGIT =  S"123456789"
-- Limited to 0-255 range, with no leading zeros
local version = (
	P"2" * (S"01234" * core.DIGIT + P"5" * S"012345")
	+ (P"1") * core.DIGIT * core.DIGIT
	+ NZDIGIT * core.DIGIT^-1
) / tonumber
_M.Sec_WebSocket_Version_Client = version
_M.Sec_WebSocket_Version_Server = comma_sep_trim(version)

-- RFC 6797
local directive_name = _M.token / string.lower
local directive_value = _M.token + _M.quoted_string
local directive = Cg(directive_name * ((_M.OWS * P"=" * _M.OWS * directive_value) + Cc(true)))
_M.Strict_Transport_Security = no_dup(directive^-1 * (_M.OWS * P";" * _M.OWS * directive^-1)^0)

-- RFC 7089
_M.Accept_Datetime = _M.IMF_fixdate
_M.Memento_Datetime = _M.IMF_fixdate

-- RFC 7232 Section 2.2
_M.Last_Modified = HTTP_date

-- RFC 7232 Section 2.3
local weak = P"W/" -- case sensitive
local etagc = P"\33" + R"\35\115" + obs_text
local opaque_tag = core.DQUOTE * etagc^0 * core.DQUOTE
local entity_tag = Cg(weak*Cc(true) + Cc(false), "weak") * C(opaque_tag)
_M.ETag = entity_tag

-- RFC 7232 Section 3.1
_M.If_Match = P"*" + comma_sep(entity_tag, 1)

-- RFC 7232 Section 3.2
_M.If_None_Match = P"*" + comma_sep(entity_tag, 1)

-- RFC 7232 Section 3.3
_M.If_Modified_Since = HTTP_date

-- RFC 7232 Section 3.4
_M.If_Unmodified_Since = HTTP_date

-- RFC 4918
local Coded_URL = P"<" * uri.absolute_uri * P">"
local extend = Coded_URL + _M.token
local compliance_class = P"1" + P"2" + P"3" + extend
_M.DAV = comma_sep_trim(compliance_class)
_M.Depth = P"0" * Cc(0)
	+ P"1" * Cc(1)
	+ case_insensitive "infinity" * Cc(math.huge)
local Simple_ref = uri.absolute_uri + partial_uri
_M.Destination = Simple_ref
local State_token = Coded_URL
local Condition = (case_insensitive("not") * Cc("not"))^-1
	* _M.OWS * (State_token + P"[" * entity_tag * P"]")
local List = P"(" * _M.OWS * (Condition * _M.OWS)^1 * P")"
local No_tag_list = List
local Resource_Tag = P"<" * Simple_ref * P">"
local Tagged_list = Resource_Tag * _M.OWS * (List * _M.OWS)^1
_M.If = (Tagged_list * _M.OWS)^1 + (No_tag_list * _M.OWS)^1
_M.Lock_Token = Coded_URL
_M.Overwrite = T_F
local DAVTimeOutVal = core.DIGIT^1 / tonumber
local TimeType = case_insensitive "Second-" * DAVTimeOutVal
	+ case_insensitive "Infinite" * Cc(math.huge)
_M.TimeOut = comma_sep_trim(TimeType)

-- RFC 5323
_M.DASL = comma_sep_trim(Coded_URL, 1)

-- RFC 6638
_M.Schedule_Reply = T_F
_M.Schedule_Tag = opaque_tag
_M.If_Schedule_Tag_Match = opaque_tag

-- RFC 7233
local bytes_unit = P"bytes"
local other_range_unit = _M.token
local range_unit = C(bytes_unit) + other_range_unit

local first_byte_pos = core.DIGIT^1 / tonumber
local last_byte_pos = core.DIGIT^1 / tonumber
local byte_range_spec = first_byte_pos * P"-" * last_byte_pos^-1
local suffix_length = core.DIGIT^1 / tonumber
local suffix_byte_range_spec = Cc(nil) * P"-" * suffix_length
local byte_range_set = comma_sep(byte_range_spec + suffix_byte_range_spec, 1)
local byte_ranges_specifier = bytes_unit * P"=" * byte_range_set

-- RFC 7233 Section 2.3
local acceptable_ranges = comma_sep_trim(range_unit, 1) + P"none"
_M.Accept_Ranges = acceptable_ranges

-- RFC 7233 Section 3.1
local other_range_set = core.VCHAR^1
local other_ranges_specifier = other_range_unit * P"=" * other_range_set
_M.Range = byte_ranges_specifier + other_ranges_specifier

-- RFC 7233 Section 3.2
_M.If_Range = entity_tag + HTTP_date

-- RFC 7233 Section 4.2
local complete_length = core.DIGIT^1 / tonumber
local unsatisfied_range = P"*/" * complete_length
local byte_range = first_byte_pos * P"-" * last_byte_pos
local byte_range_resp = byte_range * P"/" * (complete_length + P"*")
local byte_content_range = bytes_unit * core.SP * (byte_range_resp + unsatisfied_range)
local other_range_resp = core.CHAR^0
local other_content_range = other_range_unit * core.SP * other_range_resp
_M.Content_Range = byte_content_range + other_content_range

-- RFC 7234 Section 1.2.1
local delta_seconds = core.DIGIT^1 / tonumber

-- RFC 7234 Section 5.1
_M.Age = delta_seconds

-- RFC 7234 Section 5.2
local cache_directive = _M.token / string.lower * ((P"=" * (_M.token + _M.quoted_string)) + Cc(true))
_M.Cache_Control = comma_sep_trim(Cg(cache_directive), 1)

-- RFC 7234 Section 5.3
_M.Expires = HTTP_date

-- RFC 7234 Section 5.4
local extension_pragma = _M.token * (P"=" * (_M.token + _M.quoted_string))^-1
local pragma_directive = "no_cache" + extension_pragma
_M.Pragma = comma_sep_trim(pragma_directive, 1)

-- RFC 7234 Section 5.5
local warn_code = core.DIGIT * core.DIGIT * core.DIGIT
local warn_agent = (uri.host * (P":" * uri.port)^-1) + pseudonym
local warn_text = _M.quoted_string
local warn_date = core.DQUOTE * HTTP_date * core.DQUOTE
local warning_value = warn_code * core.SP * warn_agent * core.SP * warn_text * (core.SP * warn_date)^-1
_M.Warning = comma_sep_trim(warning_value, 1)

-- RFC 7235 Section 2
local auth_scheme = _M.token
local auth_param = Cg(_M.token / string.lower * _M.BWS * P"=" * _M.BWS * (_M.token + _M.quoted_string))
local token68 = C((core.ALPHA + core.DIGIT + P"-" + P"." + P"_" + P"~" + P"+" + P"/" )^1 * (P"=")^0)
-- TODO: each parameter name MUST only occur once per challenge
local challenge = auth_scheme * (core.SP^1 * (Cf(Ct(true) * comma_sep(auth_param), rawset) + token68))^-1
local credentials = challenge

-- RFC 7235 Section 4
_M.WWW_Authenticate = comma_sep_trim(Ct(challenge), 1)
_M.Authorization = credentials
_M.Proxy_Authenticate = _M.WWW_Authenticate
_M.Proxy_Authorization = _M.Authorization

-- RFC 7239 Section 4
local value = _M.token + _M.quoted_string
local forwarded_pair = _M.token * P"=" * value
local forwarded_element = forwarded_pair^-1 * (P";" * forwarded_pair^-1)^0
_M.Forwarded = comma_sep_trim(forwarded_element)

-- RFC 7469
local Public_Key_Directives = directive * (_M.OWS * P";" * _M.OWS * directive)^0
do
	local function pkp_cmt(pins, t, k, v, ...)
		-- duplicates are allowed if the directive name starts with "pin-"
		local pin_name = k:match("^pin%-(.+)")
		if pin_name then
			local hashes = pins[pin_name]
			if hashes then
				hashes[#hashes+1] = v
			else
				hashes = {v}
				pins[pin_name] = hashes
			end
		else
			local old = t[k]
			if old then
				return false
			end
			t[k] = v
		end
		if ... then
			return pkp_cmt(pins, t, ...)
		else
			return true
		end
	end
	_M.Public_Key_Pins = Cmt(Public_Key_Directives, function(_, _, ...)
		local pins = {}
		local t = {}
		local ok = pkp_cmt(pins, t, ...)
		if ok and t["max-age"] then
			return true, pins, t
		end
	end)
	_M.Public_Key_Pins_Report_Only = Cmt(Public_Key_Directives, function(_, _, ...)
		local pins = {}
		local t = {}
		local ok = pkp_cmt(pins, t, ...)
		if ok then
			return true, pins, t
		end
	end)
end

-- RFC 7486
_M.Hobareg = C"regok" + C"reginwork"

-- RFC 7615
_M.Authentication_Info = comma_sep_trim(auth_param)
_M.Proxy_Authentication_Info = comma_sep_trim(auth_param)

-- RFC 7639
local protocol_id = _M.token
_M.ALPN = comma_sep_trim(protocol_id, 1)

-- RFC 7809
_M.CalDAV_Timezones = T_F

-- RFC 7838
local clear = C"clear" -- case-sensitive
local alt_authority = _M.quoted_string -- containing [ uri_host ] ":" port
local alternative = protocol_id * P"=" * alt_authority
local alt_value = alternative * (_M.OWS * P";" * _M.OWS * parameter)^0
_M.Alt_Svc = clear + comma_sep_trim(alt_value, 1)
_M.Alt_Used = uri.host * (P":" * uri.port)^-1

-- https://tools.ietf.org/html/draft-ietf-httpbis-expect-ct-06#section-2.1
local expect_ct_directive = directive
_M.Expect_CT = no_dup(comma_sep_trim(expect_ct_directive))

-- https://www.w3.org/TR/referrer-policy/#referrer-policy-header
local policy_token = C"no-referrer"
	+ C"no-referrer-when-downgrade"
	+ C"strict-origin"
	+ C"strict-origin-when-cross-origin"
	+ C"same-origin"
	+ C"origin"
	+ C"origin-when-cross-origin"
	+ C"unsafe-url"
_M.Referrer_Policy = comma_sep_trim(policy_token, 1)

return _M
