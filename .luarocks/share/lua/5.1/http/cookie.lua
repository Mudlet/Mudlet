--[[
Data structures useful for Cookies
RFC 6265
]]

local http_patts = require "lpeg_patterns.http"
local binaryheap = require "binaryheap"
local http_util = require "http.util"
local has_psl, psl = pcall(require, "psl")

local EOF = require "lpeg".P(-1)
local sane_cookie_date = http_patts.IMF_fixdate * EOF
local Cookie = http_patts.Cookie * EOF
local Set_Cookie = http_patts.Set_Cookie * EOF

local function bake(name, value, expiry_time, domain, path, secure_only, http_only, same_site)
	-- This function is optimised to only do one concat operation at the end
	local cookie = { name, "=", value }
	local n = 3
	if expiry_time and expiry_time ~= (1e999) then
		-- Prefer Expires over Max-age unless it is a deletion request
		if expiry_time == (-1e999) then
			n = n + 1
			cookie[n] = "; Max-Age=0"
		else
			n = n + 2
			cookie[n-1] = "; Expires="
			cookie[n] = http_util.imf_date(expiry_time)
		end
	end
	if domain then
		n = n + 2
		cookie[n-1] = "; Domain="
		cookie[n] = domain
	end
	if path then
		n = n + 2
		cookie[n-1] = "; Path="
		cookie[n] = http_util.encodeURI(path)
	end
	if secure_only then
		n = n + 1
		cookie[n] = "; Secure"
	end
	if http_only then
		n = n + 1
		cookie[n] = "; HttpOnly"
	end
	-- https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2
	if same_site then
		local v
		if same_site == "strict" then
			v = "; SameSite=Strict"
		elseif same_site == "lax" then
			v = "; SameSite=Lax"
		else
			error('invalid value for same_site, expected "strict" or "lax"')
		end
		n = n + 1
		cookie[n] = v
	end
	return table.concat(cookie, "", 1, n)
end

local function parse_cookie(cookie_header)
	return Cookie:match(cookie_header)
end

local function parse_cookies(req_headers)
	local cookie_headers = req_headers:get_as_sequence("cookie")
	local cookies
	for i=1, cookie_headers.n do
		local header_cookies = parse_cookie(cookie_headers[i])
		if header_cookies then
			if cookies then
				for k, v in pairs(header_cookies) do
					cookies[k] = v
				end
			else
				cookies = header_cookies
			end
		end
	end
	return cookies or {}
end

local function parse_setcookie(setcookie_header)
	return Set_Cookie:match(setcookie_header)
end

local canonicalise_host
if has_psl then
	canonicalise_host = psl.str_to_utf8lower
else
	canonicalise_host = function(str)
		-- fail on non-ascii chars
		if str:find("[^%p%w]") then
			return nil
		end
		return str:lower()
	end
end

--[[
A string domain-matches a given domain string if at least one of the following
conditions hold:
  - The domain string and the string are identical. (Note that both the domain
    string and the string will have been canonicalized to lower case at this point.)
  - All of the following conditions hold:
	  - The domain string is a suffix of the string.
	  - The last character of the string that is not included in the domain string
	    is a %x2E (".") character.
	  - The string is a host name (i.e., not an IP address).
]]
local function domain_match(domain_string, str)
	return str == domain_string or (
		str:sub(-#domain_string) == domain_string
		and str:sub(-#domain_string-1, -#domain_string-1) == "."
		and not http_util.is_ip(str)
	)
end

--[[ A request-path path-matches a given cookie-path if at least one of the following conditions holds:
  - The cookie-path and the request-path are identical.
  - The cookie-path is a prefix of the request-path, and the last
    character of the cookie-path is %x2F ("/").
  - The cookie-path is a prefix of the request-path, and the first
    character of the request-path that is not included in the cookie-path is a %x2F ("/") character.
]]
local function path_match(path, req_path)
	if path == req_path then
		return true
	elseif path == req_path:sub(1, #path) then
		if path:sub(-1, -1) == "/" then
			return true
		elseif req_path:sub(#path + 1, #path + 1) == "/" then
			return true
		end
	end
	return false
end

local cookie_methods = {}
local cookie_mt = {
	__name = "http.cookie.cookie";
	__index = cookie_methods;
}

function cookie_methods:netscape_format()
	return string.format("%s%s\t%s\t%s\t%s\t%d\t%s\t%s\n",
		self.http_only and "#HttpOnly_" or "",
		self.domain or "unknown",
		self.host_only and "TRUE" or "FALSE",
		self.path,
		self.secure_only and "TRUE" or "FALSE",
		math.max(0, math.min(2147483647, self.expiry_time)),
		self.name,
		self.value)
end


local default_psl
if has_psl and psl.latest then
	default_psl = psl.latest()
elseif has_psl then
	default_psl = psl.builtin()
end
local store_methods = {
	psl = default_psl;
	time = function() return os.time() end;
	max_cookie_length = (1e999);
	max_cookies = (1e999);
	max_cookies_per_domain = (1e999);
}

local store_mt = {
	__name = "http.cookie.store";
	__index = store_methods;
}

local function new_store()
	return setmetatable({
		domains = {};
		expiry_heap = binaryheap.minUnique();
		n_cookies = 0;
		n_cookies_per_domain = {};
	}, store_mt)
end

local function add_to_store(self, cookie, req_is_http, now)
	if cookie.expiry_time < now then
		-- This was all just a trigger to delete the old cookie
		self:remove(cookie.domain, cookie.path, cookie.name)
	else
		local name = cookie.name
		local cookie_length = #name + 1 + #cookie.value
		if cookie_length > self.max_cookie_length then
			return false
		end

		local domain = cookie.domain
		local domain_cookies = self.domains[domain]
		local path_cookies
		local old_cookie
		if domain_cookies ~= nil then
			path_cookies = domain_cookies[cookie.path]
			if path_cookies ~= nil then
				old_cookie = path_cookies[name]
			end
		end

		-- If the cookie store contains a cookie with the same name,
		-- domain, and path as the newly created cookie:
		if old_cookie then
			-- If the newly created cookie was received from a "non-HTTP"
			-- API and the old-cookie's http-only-flag is set, abort these
			-- steps and ignore the newly created cookie entirely.
			if not req_is_http and old_cookie.http_only then
				return false
			end

			-- Update the creation-time of the newly created cookie to
			-- match the creation-time of the old-cookie.
			cookie.creation_time = old_cookie.creation_time

			-- Remove the old-cookie from the cookie store.
			self.expiry_heap:remove(old_cookie)
		else
			if self.n_cookies >= self.max_cookies or self.max_cookies_per_domain < 1 then
				return false
			end

			-- Cookie will be added
			if domain_cookies == nil then
				path_cookies = {}
				domain_cookies = {
					[cookie.path] = path_cookies;
				}
				self.domains[domain] = domain_cookies
				self.n_cookies_per_domain[domain] = 1
			else
				local n_cookies_per_domain = self.n_cookies_per_domain[domain]
				if n_cookies_per_domain >= self.max_cookies_per_domain then
					return false
				end
				path_cookies = domain_cookies[cookie.path]
				if path_cookies == nil then
					path_cookies = {}
					domain_cookies[cookie.path] = path_cookies
				end
				self.n_cookies_per_domain[domain] = n_cookies_per_domain
			end

			self.n_cookies = self.n_cookies + 1
		end

		path_cookies[name] = cookie
		self.expiry_heap:insert(cookie.expiry_time, cookie)
	end

	return true
end

function store_methods:store(req_domain, req_path, req_is_http, req_is_secure, req_site_for_cookies, name, value, params)
	assert(type(req_domain) == "string")
	assert(type(req_path) == "string")
	assert(type(name) == "string")
	assert(type(value) == "string")
	assert(type(params) == "table")

	local now = self.time()

	req_domain = assert(canonicalise_host(req_domain), "invalid request domain")

	-- Clean now so that we can assume there are no expired cookies in store
	self:clean()

	-- RFC 6265 Section 5.3
	local cookie = setmetatable({
		name = name;
		value = value;
		expiry_time = (1e999);
		domain = req_domain;
		path = nil;
		creation_time = now;
		last_access_time = now;
		persistent = false;
		host_only = true;
		secure_only = not not params.secure;
		http_only = not not params.httponly;
		same_site = nil;
	}, cookie_mt)

	-- If a cookie has both the Max-Age and the Expires attribute, the Max-
	-- Age attribute has precedence and controls the expiration date of the
	-- cookie.
	local max_age = params["max-age"]
	if max_age and max_age:find("^%-?[0-9]+$") then
		max_age = tonumber(max_age, 10)
		cookie.persistent = true
		if max_age <= 0 then
			cookie.expiry_time = (-1e999)
		else
			cookie.expiry_time = now + max_age
		end
	elseif params.expires then
		local date = sane_cookie_date:match(params.expires)
		if date then
			cookie.persistent = true
			cookie.expiry_time = os.time(date)
		end
	end

	local domain = params.domain or "";

	-- If the first character of the attribute-value string is %x2E ("."):
	-- Let cookie-domain be the attribute-value without the leading %x2E (".") character.
	if domain:sub(1, 1) == "." then
		domain = domain:sub(2)
	end

	-- Convert the cookie-domain to lower case.
	domain = canonicalise_host(domain)
	if not domain then
		return false
	end

	-- If the user agent is configured to reject "public suffixes" and
	-- the domain-attribute is a public suffix:
	if domain ~= "" and self.psl and self.psl:is_public_suffix(domain) then
		-- If the domain-attribute is identical to the canonicalized request-host:
		if domain == req_domain then
			-- Let the domain-attribute be the empty string.
			domain = ""
		else
			-- Ignore the cookie entirely and abort these steps.
			return false
		end
	end

	-- If the domain-attribute is non-empty:
	if domain ~= "" then
		-- If the canonicalized request-host does not domain-match the
		-- domain-attribute:
		if not domain_match(domain, req_domain) then
			-- Ignore the cookie entirely and abort these steps.
			return false
		else
			-- Set the cookie's host-only-flag to false.
			cookie.host_only = false
			-- Set the cookie's domain to the domain-attribute.
			cookie.domain = domain
		end
	end

	-- RFC 6265 Section 5.2.4
	-- If the attribute-value is empty or if the first character of the
	-- attribute-value is not %x2F ("/")
	local path = params.path or ""
	if path:sub(1, 1) ~= "/" then
		-- Let cookie-path be the default-path.
		local default_path
		-- RFC 6265 Section 5.1.4
		-- Let uri-path be the path portion of the request-uri if such a
		-- portion exists (and empty otherwise).  For example, if the
		-- request-uri contains just a path (and optional query string),
		-- then the uri-path is that path (without the %x3F ("?") character
		-- or query string), and if the request-uri contains a full
		-- absoluteURI, the uri-path is the path component of that URI.

		-- If the uri-path is empty or if the first character of the uri-
		-- path is not a %x2F ("/") character, output %x2F ("/") and skip
		-- the remaining steps.
		-- If the uri-path contains no more than one %x2F ("/") character,
		-- output %x2F ("/") and skip the remaining step.
		if req_path:sub(1, 1) ~= "/" or not req_path:find("/", 2, true) then
			default_path = "/"
		else
			-- Output the characters of the uri-path from the first character up
			-- to, but not including, the right-most %x2F ("/").
			default_path = req_path:match("^([^?]*)/")
		end
		cookie.path = default_path
	else
		cookie.path = path
	end

	-- If the scheme component of the request-uri does not denote a
	-- "secure" protocol (as defined by the user agent), and the
	-- cookie's secure-only-flag is true, then abort these steps and
	-- ignore the cookie entirely.
	if not req_is_secure and cookie.secure_only then
		return false
	end

	-- If the cookie was received from a "non-HTTP" API and the
	-- cookie's http-only-flag is set, abort these steps and ignore the
	-- cookie entirely.
	if not req_is_http and cookie.http_only then
		return false
	end

	-- If the cookie's secure-only-flag is not set, and the scheme
	-- component of request-uri does not denote a "secure" protocol,
	if not req_is_secure and not cookie.secure_only then
		-- then abort these steps and ignore the cookie entirely if the
		-- cookie store contains one or more cookies that meet all of the
		-- following criteria:
		for d, domain_cookies in pairs(self.domains) do
			-- See '3' below
			if domain_match(cookie.domain, d) or domain_match(d, cookie.domain) then
				for p, path_cookies in pairs(domain_cookies) do
					local cmp_cookie = path_cookies[name]
					-- 1. Their name matches the name of the newly-created cookie.
					if cmp_cookie
						-- 2. Their secure-only-flag is true.
						and cmp_cookie.secure_only
						-- 3. Their domain domain-matches the domain of the newly-created
						-- cookie, or vice-versa.
						-- Note: already checked above in domain_match
						-- 4. The path of the newly-created cookie path-matches the path
						-- of the existing cookie.
						and path_match(p, cookie.path)
					then
						return false
					end
				end
			end
		end
	end

	-- If the cookie-attribute-list contains an attribute with an
	-- attribute-name of "SameSite", set the cookie's same-site-flag to
	-- attribute-value (i.e. either "Strict" or "Lax").  Otherwise, set
	-- the cookie's same-site-flag to "None".
	local same_site = params.samesite
	if same_site then
		same_site = same_site:lower()
		if same_site == "lax" or same_site == "strict" then
			-- If the cookie's "same-site-flag" is not "None", and the cookie
			-- is being set from a context whose "site for cookies" is not an
			-- exact match for request-uri's host's registered domain, then
			-- abort these steps and ignore the newly created cookie entirely.
			if req_domain ~= req_site_for_cookies then
				return false
			end

			cookie.same_site = same_site
		end
	end

	-- If the cookie-name begins with a case-sensitive match for the
	-- string "__Secure-", abort these steps and ignore the cookie
	-- entirely unless the cookie's secure-only-flag is true.
	if not cookie.secure_only and name:sub(1, 9) == "__Secure-" then
		return false
	end

	-- If the cookie-name begins with a case-sensitive match for the
	-- string "__Host-", abort these steps and ignore the cookie
	-- entirely unless the cookie meets all the following criteria:
	-- 1.  The cookie's secure-only-flag is true.
	-- 2.  The cookie's host-only-flag is true.
	-- 3.  The cookie-attribute-list contains an attribute with an
	--     attribute-name of "Path", and the cookie's path is "/".
	if not (cookie.secure_only and cookie.host_only and cookie.path == "/") and name:sub(1, 7) == "__Host-" then
		return false
	end

	return add_to_store(self, cookie, req_is_http, now)
end

function store_methods:store_from_request(req_headers, resp_headers, req_host, req_site_for_cookies)
	local set_cookies = resp_headers:get_as_sequence("set-cookie")
	local n = set_cookies.n
	if n == 0 then
		return true
	end

	local req_scheme = req_headers:get(":scheme")
	local req_authority = req_headers:get(":authority")
	local req_domain
	if req_authority then
		req_domain = http_util.split_authority(req_authority, req_scheme)
	else -- :authority can be missing for HTTP/1.0 requests; fall back to req_host
		req_domain = req_host
	end
	local req_path = req_headers:get(":path")
	local req_is_secure = req_scheme == "https"

	for i=1, n do
		local name, value, params = parse_setcookie(set_cookies[i])
		if name then
			self:store(req_domain, req_path, true, req_is_secure, req_site_for_cookies, name, value, params)
		end
	end
	return true
end

function store_methods:get(domain, path, name)
	assert(type(domain) == "string")
	assert(type(path) == "string")
	assert(type(name) == "string")

	-- Clean now so that we can assume there are no expired cookies in store
	self:clean()

	local domain_cookies = self.domains[domain]
	if domain_cookies then
		local path_cookies = domain_cookies[path]
		if path_cookies then
			local cookie = path_cookies[name]
			if cookie then
				return cookie.value
			end
		end
	end
	return nil
end

function store_methods:remove(domain, path, name)
	assert(type(domain) == "string")
	assert(type(path) == "string" or (path == nil and name == nil))
	assert(type(name) == "string" or name == nil)
	local domain_cookies = self.domains[domain]
	if not domain_cookies then
		return
	end
	local n_cookies = self.n_cookies
	if path == nil then
		-- Delete whole domain
		for _, path_cookies in pairs(domain_cookies) do
			for _, cookie in pairs(path_cookies) do
				self.expiry_heap:remove(cookie)
				n_cookies = n_cookies - 1
			end
		end
		self.domains[domain] = nil
		self.n_cookies_per_domain[domain] = nil
	else
		local path_cookies = domain_cookies[path]
		if path_cookies then
			if name == nil then
				-- Delete all names at path
				local domains_deleted = 0
				for _, cookie in pairs(path_cookies) do
					self.expiry_heap:remove(cookie)
					domains_deleted = domains_deleted + 1
				end
				domain_cookies[path] = nil
				n_cookies = n_cookies - domains_deleted
				if next(domain_cookies) == nil then
					self.domains[domain] = nil
					self.n_cookies_per_domain[domain] = nil
				else
					self.n_cookies_per_domain[domain] = self.n_cookies_per_domain[domain] - domains_deleted
				end
			else
				-- Delete singular cookie
				local cookie = path_cookies[name]
				if cookie then
					self.expiry_heap:remove(cookie)
					n_cookies = n_cookies - 1
					self.n_cookies_per_domain[domain] = self.n_cookies_per_domain[domain] - 1
					path_cookies[name] = nil
					if next(path_cookies) == nil then
						domain_cookies[path] = nil
						if next(domain_cookies) == nil then
							self.domains[domain] = nil
							self.n_cookies_per_domain[domain] = nil
						end
					end
				end
			end
		end
	end
	self.n_cookies = n_cookies
end

--[[ The user agent SHOULD sort the cookie-list in the following order:
  - Cookies with longer paths are listed before cookies with shorter paths.
  - Among cookies that have equal-length path fields, cookies with earlier
	creation-times are listed before cookies with later creation-times.
]]
local function cookie_cmp(a, b)
	if #a.path ~= #b.path then
		return #a.path > #b.path
	end
	if a.creation_time ~= b.creation_time then
		return a.creation_time < b.creation_time
	end
	-- Now order doesn't matter, but have to be consistent for table.sort:
	-- use the fields that make a cookie unique
	if a.domain ~= b.domain then
		return a.domain < b.domain
	end
	return a.name < b.name
end

local function cookie_match(cookie, req_domain, req_is_http, req_is_secure, req_is_safe_method, req_site_for_cookies, req_is_top_level)
	-- req_domain should be already canonicalized

	if cookie.host_only then -- Either:
		-- The cookie's host-only-flag is true and the canonicalized
		-- request-host is identical to the cookie's domain.
		if cookie.domain ~= req_domain then
			return false
		end
	end
	-- Or:
	-- The cookie's host-only-flag is false and the canonicalized
	-- request-host domain-matches the cookie's domain.

	-- already done domain_match and path_match

	-- If the cookie's http-only-flag is true, then exclude the
	-- cookie if the cookie-string is being generated for a "non-
	-- HTTP" API (as defined by the user agent).
	if cookie.http_only and not req_is_http then
		return false
	end

	if cookie.secure_only and not req_is_secure then
		return false
	end

	-- If the cookie's same-site-flag is not "None", and the HTTP
	-- request is cross-site (as defined in Section 5.2) then exclude
	-- the cookie unless all of the following statements hold:
	if cookie.same_site and req_site_for_cookies ~= req_domain and not (
		-- 1. The same-site-flag is "Lax"
		cookie.same_site == "lax"
		-- 2. The HTTP request's method is "safe".
		and req_is_safe_method
		-- 3. The HTTP request's target browsing context is a top-level browsing context.
		and req_is_top_level
	) then
		return false
	end

	return true
end

function store_methods:lookup(req_domain, req_path, req_is_http, req_is_secure, req_is_safe_method, req_site_for_cookies, req_is_top_level, max_cookie_length)
	req_domain = assert(type(req_domain) == "string" and canonicalise_host(req_domain), "invalid request domain")
	assert(type(req_path) == "string")
	if max_cookie_length ~= nil then
		assert(type(max_cookie_length) == "number")
	else
		max_cookie_length = self.max_cookie_length
	end

	local now = self.time()

	-- Clean now so that we can assume there are no expired cookies in store
	self:clean()

	local list = {}
	local n = 0
	for domain, domain_cookies in pairs(self.domains) do
		if domain_match(domain, req_domain) then
			for path, path_cookies in pairs(domain_cookies) do
				if path_match(path, req_path) then
					for _, cookie in pairs(path_cookies) do
						if cookie_match(cookie, req_domain, req_is_http, req_is_secure, req_is_safe_method, req_site_for_cookies, req_is_top_level) then
							cookie.last_access_time = now
							n = n + 1
							list[n] = cookie
						end
					end
				end
			end
		end
	end
	table.sort(list, cookie_cmp)
	local cookie_length = -2 -- length of separator ("; ")
	for i=1, n do
		local cookie = list[i]
		-- TODO: validate?
		local cookie_pair = cookie.name .. "=" .. cookie.value
		local new_length = cookie_length + #cookie_pair + 2
		if new_length > max_cookie_length then
			break
		end
		list[i] = cookie_pair
		cookie_length = new_length
	end
	return table.concat(list, "; ", 1, n)
end

function store_methods:lookup_for_request(req_headers, req_host, req_site_for_cookies, req_is_top_level, max_cookie_length)
	local req_method = req_headers:get(":method")
	if req_method == "CONNECT" then
		return ""
	end
	local req_scheme = req_headers:get(":scheme")
	local req_authority = req_headers:get(":authority")
	local req_domain
	if req_authority then
		req_domain = http_util.split_authority(req_authority, req_scheme)
	else -- :authority can be missing for HTTP/1.0 requests; fall back to req_host
		req_domain = req_host
	end
	local req_path = req_headers:get(":path")
	local req_is_secure = req_scheme == "https"
	local req_is_safe_method = http_util.is_safe_method(req_method)
	return self:lookup(req_domain, req_path, true, req_is_secure, req_is_safe_method, req_site_for_cookies, req_is_top_level, max_cookie_length)
end

function store_methods:clean_due()
	local next_expiring = self.expiry_heap:peek()
	if not next_expiring then
		return (1e999)
	end
	return next_expiring.expiry_time
end

function store_methods:clean()
	local now = self.time()
	while self:clean_due() < now do
		local cookie = self.expiry_heap:pop()
		self.n_cookies = self.n_cookies - 1
		local domain = cookie.domain
		local domain_cookies = self.domains[domain]
		if domain_cookies then
			self.n_cookies_per_domain[domain] = self.n_cookies_per_domain[domain] - 1
			local path_cookies = domain_cookies[cookie.path]
			if path_cookies then
				path_cookies[cookie.name] = nil
				if next(path_cookies) == nil then
					domain_cookies[cookie.path] = nil
					if next(domain_cookies) == nil then
						self.domains[domain] = nil
						self.n_cookies_per_domain[domain] = nil
					end
				end
			end
		end
	end
	return true
end

-- Files in 'netscape format'
-- curl's lib/cookie.c is best reference for the format
local function parse_netscape_format(line, now)
	if line == "" then
		return
	end
	local i = 1
	local http_only = false
	if line:sub(1, 1) == "#" then
		if line:sub(1, 10) == "#HttpOnly_" then
			http_only = true
			i = 11
		else
			return
		end
	end

	local domain, host_only, path, secure_only, expiry, name, value =
		line:match("^%.?([^\t]+)\t([^\t]+)\t([^\t]+)\t([^\t]+)\t(%d+)\t([^\t]+)\t(.+)", i)
	if not domain then
		return
	end
	domain = canonicalise_host(domain)
	if domain == nil then
		return
	end

	if host_only == "TRUE" then
		host_only = true
	elseif host_only == "FALSE" then
		host_only = false
	else
		return
	end

	if secure_only == "TRUE" then
		secure_only = true
	elseif secure_only == "FALSE" then
		secure_only = false
	else
		return
	end

	expiry = tonumber(expiry, 10)

	return setmetatable({
		name = name;
		value = value;
		expiry_time = expiry;
		domain = domain;
		path = path;
		creation_time = now;
		last_access_time = now;
		persistent = expiry == 0;
		host_only = host_only;
		secure_only = secure_only;
		http_only = http_only;
		same_site = nil;
	}, cookie_mt)
end

function store_methods:load_from_file(file)
	local now = self.time()

	-- Clean now so that we don't hit storage limits
	self:clean()

	local cookies = {}
	local n = 0
	while true do
		local line, err, errno = file:read()
		if not line then
			if err ~= nil then
				return nil, err, errno
			end
			break
		end
		local cookie = parse_netscape_format(line, now)
		if cookie then
			n = n + 1
			cookies[n] = cookie
		end
	end
	for i=1, n do
		local cookie = cookies[i]
		add_to_store(self, cookie, cookie.http_only, now)
	end
	return true
end

function store_methods:save_to_file(file)
	do -- write a preamble
		local ok, err, errno = file:write [[
# Netscape HTTP Cookie File
# This file was generated by lua-http

]]
		if not ok then
			return nil, err, errno
		end
	end
	for _, domain_cookies in pairs(self.domains) do
		for _, path_cookies in pairs(domain_cookies) do
			for _, cookie in pairs(path_cookies) do
				local ok, err, errno = file:write(cookie:netscape_format())
				if not ok then
					return nil, err, errno
				end
			end
		end
	end
	return true
end

return {
	bake = bake;

	parse_cookie = parse_cookie;
	parse_cookies = parse_cookies;
	parse_setcookie = parse_setcookie;

	new_store = new_store;
	store_mt = store_mt;
	store_methods = store_methods;
}
