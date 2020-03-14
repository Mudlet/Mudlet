local lpeg = require "lpeg"
local http_patts = require "lpeg_patterns.http"
local IPv4_patts = require "lpeg_patterns.IPv4"
local IPv6_patts = require "lpeg_patterns.IPv6"

local EOF = lpeg.P(-1)

-- Encodes a character as a percent encoded string
local function char_to_pchar(c)
	return string.format("%%%02X", c:byte(1,1))
end

-- encodeURI replaces all characters except the following with the appropriate UTF-8 escape sequences:
-- ; , / ? : @ & = + $
-- alphabetic, decimal digits, - _ . ! ~ * ' ( )
-- #
local function encodeURI(str)
	return (str:gsub("[^%;%,%/%?%:%@%&%=%+%$%w%-%_%.%!%~%*%'%(%)%#]", char_to_pchar))
end

-- encodeURIComponent escapes all characters except the following: alphabetic, decimal digits, - _ . ! ~ * ' ( )
local function encodeURIComponent(str)
	return (str:gsub("[^%w%-_%.%!%~%*%'%(%)]", char_to_pchar))
end

-- decodeURI unescapes url encoded characters
-- excluding characters that are special in urls
local decodeURI do
	local decodeURI_blacklist = {}
	for char in ("#$&+,/:;=?@"):gmatch(".") do
		decodeURI_blacklist[string.byte(char)] = true
	end
	local function decodeURI_helper(str)
		local x = tonumber(str, 16)
		if not decodeURI_blacklist[x] then
			return string.char(x)
		end
		-- return nothing; gsub will not perform the replacement
	end
	function decodeURI(str)
		return (str:gsub("%%(%x%x)", decodeURI_helper))
	end
end

-- Converts a hex string to a character
local function pchar_to_char(str)
	return string.char(tonumber(str, 16))
end

-- decodeURIComponent unescapes *all* url encoded characters
local function decodeURIComponent(str)
	return (str:gsub("%%(%x%x)", pchar_to_char))
end

-- An iterator over query segments (delimited by "&") as key/value pairs
-- if a query segment has no '=', the value will be `nil`
local function query_args(str)
	local iter, state, first = str:gmatch("([^=&]+)(=?)([^&]*)&?")
	return function(state, last) -- luacheck: ignore 431
		local name, equals, value = iter(state, last)
		if name == nil then return nil end
		name = decodeURIComponent(name)
		if equals == "" then
			value = nil
		else
			value = decodeURIComponent(value)
		end
		return name, value
	end, state, first
end

-- Converts a dictionary (string keys, string values) to an encoded query string
local function dict_to_query(form)
	local r, i = {}, 0
	for name, value in pairs(form) do
		i = i + 1
		r[i] = encodeURIComponent(name).."="..encodeURIComponent(value)
	end
	return table.concat(r, "&", 1, i)
end

-- Resolves a relative path
local function resolve_relative_path(orig_path, relative_path)
	local t, i = {}, 0

	local is_abs
	if relative_path:sub(1,1) == "/" then
		-- "relative" argument is actually absolute. ignore orig_path argument
		is_abs = true
	else
		is_abs = orig_path:sub(1,1) == "/"
		-- this will skip empty path components due to +
		-- the / on the end ignores trailing component
		for segment in orig_path:gmatch("([^/]+)/") do
			i = i + 1
			t[i] = segment
		end
	end

	for segment in relative_path:gmatch("([^/]+)") do
		if segment == ".." then
			-- if we're at the root, do nothing
			if i > 0 then
				-- discard a component
				i = i - 1
			end
		elseif segment ~= "." then
			i = i + 1
			t[i] = segment
		end
	end

	-- Make sure leading slash is kept
	local s
	if is_abs then
		if i == 0 then return "/" end
		t[0] = ""
		s = 0
	else
		s = 1
	end
	-- Make sure trailing slash is kept
	if relative_path:sub(-1, -1) == "/" then
		i = i + 1
		t[i] = ""
	end
	return table.concat(t, "/", s, i)
end

local safe_methods = {
	-- RFC 7231 Section 4.2.1:
	-- Of the request methods defined by this specification, the GET, HEAD,
	-- OPTIONS, and TRACE methods are defined to be safe.
	GET = true;
	HEAD = true;
	OPTIONS = true;
	TRACE = true;
}
local function is_safe_method(method)
	return safe_methods[method] or false
end

local IPaddress = (IPv4_patts.IPv4address + IPv6_patts.IPv6addrz) * EOF
local function is_ip(str)
	return IPaddress:match(str) ~= nil
end

local scheme_to_port = {
	http = 80;
	ws = 80;
	https = 443;
	wss = 443;
}

-- Splits a :authority header (same as Host) into host and port
local function split_authority(authority, scheme)
	local host, port
	local h, p = authority:match("^[ \t]*(.-):(%d+)[ \t]*$")
	if p then
		authority = h
		port = tonumber(p, 10)
	else -- when port missing from host header, it defaults to the default for that scheme
		port = scheme_to_port[scheme]
		if port == nil then
			return nil, "unknown scheme"
		end
	end
	local ipv6 = authority:match("^%[([:%x]+)%]$")
	if ipv6 then
		host = ipv6
	else
		host = authority
	end
	return host, port
end

-- Reverse of `split_authority`: converts a host, port and scheme
-- into a string suitable for an :authority header.
local function to_authority(host, port, scheme)
	local authority = host
	if host:match("^[%x:]+:[%x:]*$") then -- IPv6
		authority = "[" .. authority .. "]"
	end
	local default_port = scheme_to_port[scheme]
	if default_port == port then
		port = nil
	end
	if port then
		authority = string.format("%s:%d", authority, port)
	end
	return authority
end

-- HTTP prefered date format
-- See RFC 7231 section 7.1.1.1
local function imf_date(time)
	return os.date("!%a, %d %b %Y %H:%M:%S GMT", time)
end

-- This pattern checks if its argument is a valid token, if so, it returns it as is.
-- Otherwise, it returns it as a quoted string (with any special characters escaped)
local maybe_quote do
	local patt = http_patts.token * EOF
		+ lpeg.Cs(lpeg.Cc'"' * ((lpeg.S"\\\"") / "\\%0" + http_patts.qdtext)^0 * lpeg.Cc'"') * EOF
	maybe_quote = function (s)
		return patt:match(s)
	end
end

-- A pcall-alike function that can be yielded over even in PUC 5.1
local yieldable_pcall
--[[ If pcall can already yield, then we want to use that.

However, we can't do the feature check straight away, Openresty breaks
coroutine.wrap in some contexts. See #98
Openresty nominally only supports LuaJIT, which always supports a yieldable
pcall, so we short-circuit the feature check by checking if the 'ngx' library
is loaded, plus that jit.version_num indicates LuaJIT 2.0.
This combination ensures that we don't take the wrong branch if:
  - lua-http is being used to mock the openresty environment
  - openresty is compiled with something other than LuaJIT
]]
if (
		package.loaded.ngx
		and type(package.loaded.jit) == "table"
		and type(package.loaded.jit.version_num) == "number"
		and package.loaded.jit.version_num >= 20000
	)
	-- See if pcall can be yielded over
	or coroutine.wrap(function()
		return pcall(coroutine.yield, true) end
	)() then
	yieldable_pcall = pcall
else
	local function handle_resume(co, ok, ...)
		if not ok then
			return false, ...
		elseif coroutine.status(co) == "dead" then
			return true, ...
		end
		return handle_resume(co, coroutine.resume(co, coroutine.yield(...)))
	end
	yieldable_pcall = function(func, ...)
		if type(func) ~= "function" or debug.getinfo(func, "S").what == "C" then
			local C_func = func
			-- Can't give C functions to coroutine.create
			func = function(...) return C_func(...) end
		end
		local co = coroutine.create(func)
		return handle_resume(co, coroutine.resume(co, ...))
	end
end

return {
	encodeURI = encodeURI;
	encodeURIComponent = encodeURIComponent;
	decodeURI = decodeURI;
	decodeURIComponent = decodeURIComponent;
	query_args = query_args;
	dict_to_query = dict_to_query;
	resolve_relative_path = resolve_relative_path;
	is_safe_method = is_safe_method;
	is_ip = is_ip;
	scheme_to_port = scheme_to_port;
	split_authority = split_authority;
	to_authority = to_authority;
	imf_date = imf_date;
	maybe_quote = maybe_quote;
	yieldable_pcall = yieldable_pcall;
}
