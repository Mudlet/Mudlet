local lpeg = require "lpeg"
local http_patts = require "lpeg_patterns.http"
local uri_patts = require "lpeg_patterns.uri"
local basexx = require "basexx"
local client = require "http.client"
local new_headers = require "http.headers".new
local http_cookie = require "http.cookie"
local http_hsts = require "http.hsts"
local http_socks = require "http.socks"
local http_proxies = require "http.proxies"
local http_util = require "http.util"
local http_version = require "http.version"
local monotime = require "cqueues".monotime
local ce = require "cqueues.errno"

local default_user_agent = string.format("%s/%s", http_version.name, http_version.version)
local default_hsts_store = http_hsts.new_store()
local default_proxies = http_proxies.new():update()
local default_cookie_store = http_cookie.new_store()

local default_h2_settings = {
	ENABLE_PUSH = false;
}

local request_methods = {
	hsts = default_hsts_store;
	proxies = default_proxies;
	cookie_store = default_cookie_store;
	is_top_level = true;
	site_for_cookies = nil;
	expect_100_timeout = 1;
	follow_redirects = true;
	max_redirects = 5;
	post301 = false;
	post302 = false;
}

local request_mt = {
	__name = "http.request";
	__index = request_methods;
}

local EOF = lpeg.P(-1)
local sts_patt = http_patts.Strict_Transport_Security * EOF
local uri_patt = uri_patts.uri * EOF
local uri_ref = uri_patts.uri_reference * EOF

local function new_from_uri(uri_t, headers)
	if type(uri_t) == "string" then
		uri_t = assert(uri_patt:match(uri_t), "invalid URI")
	else
		assert(type(uri_t) == "table")
	end
	local scheme = assert(uri_t.scheme, "URI missing scheme")
	assert(scheme == "https" or scheme == "http" or scheme == "ws" or scheme == "wss", "scheme not valid")
	local host = assert(uri_t.host, "URI must include a host")
	local port = uri_t.port or http_util.scheme_to_port[scheme]
	local is_connect -- CONNECT requests are a bit special, see http2 spec section 8.3
	if headers == nil then
		headers = new_headers()
		headers:append(":method", "GET")
		is_connect = false
	else
		is_connect = headers:get(":method") == "CONNECT"
	end
	if is_connect then
		assert(uri_t.path == nil or uri_t.path == "", "CONNECT requests cannot have a path")
		assert(uri_t.query == nil, "CONNECT requests cannot have a query")
		assert(headers:has(":authority"), ":authority required for CONNECT requests")
	else
		headers:upsert(":authority", http_util.to_authority(host, port, scheme))
		local path = uri_t.path
		if path == nil or path == "" then
			path = "/"
		end
		if uri_t.query then
			path = path .. "?" .. uri_t.query
		end
		headers:upsert(":path", path)
		headers:upsert(":scheme", scheme)
	end
	if uri_t.userinfo then
		local field
		if is_connect then
			field = "proxy-authorization"
		else
			field = "authorization"
		end
		local userinfo = http_util.decodeURIComponent(uri_t.userinfo) -- XXX: this doesn't seem right, but it's the same behaviour as curl
		headers:upsert(field, "basic " .. basexx.to_base64(userinfo), true)
	end
	if not headers:has("user-agent") then
		headers:append("user-agent", default_user_agent)
	end
	return setmetatable({
		host = host;
		port = port;
		tls = (scheme == "https" or scheme == "wss");
		headers = headers;
		body = nil;
	}, request_mt)
end

local function new_connect(uri, connect_authority)
	local headers = new_headers()
	headers:append(":authority", connect_authority)
	headers:append(":method", "CONNECT")
	return new_from_uri(uri, headers)
end

function request_methods:clone()
	return setmetatable({
		host = self.host;
		port = self.port;
		bind = self.bind;
		tls = self.tls;
		ctx = self.ctx;
		sendname = self.sendname;
		version = self.version;
		proxy = self.proxy;

		headers = self.headers:clone();
		body = self.body;

		hsts = rawget(self, "hsts");
		proxies = rawget(self, "proxies");
		cookie_store = rawget(self, "cookie_store");
		is_top_level = rawget(self, "is_top_level");
		site_for_cookies = rawget(self, "site_for_cookies");
		expect_100_timeout = rawget(self, "expect_100_timeout");
		follow_redirects = rawget(self, "follow_redirects");
		max_redirects = rawget(self, "max_redirects");
		post301 = rawget(self, "post301");
		post302 = rawget(self, "post302");
	}, request_mt)
end

function request_methods:to_uri(with_userinfo)
	local scheme = self.headers:get(":scheme")
	local method = self.headers:get(":method")
	local path
	if scheme == nil then
		scheme = self.tls and "https" or "http"
	end
	local authority
	local authorization_field
	if method == "CONNECT" then
		authorization_field = "proxy-authorization"
		path = ""
	else
		path = self.headers:get(":path")
		local path_t
		if method == "OPTIONS" and path == "*" then
			path = ""
		else
			path_t = uri_ref:match(path)
			assert(path_t, "path not a valid uri reference")
		end
		if path_t and path_t.host then
			-- path was a full URI. This is used for proxied requests.
			scheme = path_t.scheme or scheme
			path = path_t.path or ""
			if path_t.query then
				path = path .. "?" .. path_t.query
			end
			authority = http_util.to_authority(path_t.host, path_t.port, scheme)
		else
			authority = self.headers:get(":authority")
			-- TODO: validate authority can fit in a url
		end
		authorization_field = "authorization"
	end
	if authority == nil then
		authority = http_util.to_authority(self.host, self.port, scheme)
	end
	if with_userinfo and self.headers:has(authorization_field) then
		local authorization = self.headers:get(authorization_field)
		local auth_type, userinfo = authorization:match("^%s*(%S+)%s+(%S+)%s*$")
		if auth_type and auth_type:lower() == "basic" then
			userinfo = basexx.from_base64(userinfo)
			userinfo = http_util.encodeURI(userinfo)
			authority = userinfo .. "@" .. authority
		else
			error("authorization cannot be converted to uri")
		end
	end
	return scheme .. "://" .. authority .. path
end

function request_methods:handle_redirect(orig_headers)
	local max_redirects = self.max_redirects
	if max_redirects <= 0 then
		return nil, "maximum redirects exceeded", ce.ELOOP
	end
	local location = orig_headers:get("location")
	if not location then
		return nil, "missing location header for redirect", ce.EINVAL
	end
	local uri_t = uri_ref:match(location)
	if not uri_t then
		return nil, "invalid URI in location header", ce.EINVAL
	end
	local new_req = self:clone()
	new_req.max_redirects = max_redirects - 1
	local method = new_req.headers:get(":method")
	local is_connect = method == "CONNECT"
	local new_scheme = uri_t.scheme
	if new_scheme then
		if not is_connect then
			new_req.headers:upsert(":scheme", new_scheme)
		end
		if new_scheme == "https" or new_scheme == "wss" then
			new_req.tls = true
		elseif new_scheme == "http" or new_scheme == "ws" then
			new_req.tls = false
		else
			return nil, "unknown scheme", ce.EINVAL
		end
	else
		if not is_connect then
			new_scheme = new_req.headers:get(":scheme")
		end
		if new_scheme == nil then
			new_scheme = self.tls and "https" or "http"
		end
	end
	local orig_target
	local target_authority
	if not is_connect then
		orig_target = self.headers:get(":path")
		orig_target = uri_ref:match(orig_target)
		if orig_target and orig_target.host then
			-- was originally a proxied request
			local new_authority
			if uri_t.host then -- we have a new host
				new_authority = http_util.to_authority(uri_t.host, uri_t.port, new_scheme)
				new_req.headers:upsert(":authority", new_authority)
			else
				new_authority = self.headers:get(":authority")
			end
			if new_authority == nil then
				new_authority = http_util.to_authority(self.host, self.port, new_scheme)
			end
			-- prefix for new target
			target_authority = new_scheme .. "://" .. new_authority
		end
	end
	if target_authority == nil and uri_t.host then
		-- we have a new host and it wasn't placed into :authority
		new_req.host = uri_t.host
		if not is_connect then
			new_req.headers:upsert(":authority", http_util.to_authority(uri_t.host, uri_t.port, new_scheme))
		end
		new_req.port = uri_t.port or http_util.scheme_to_port[new_scheme]
		new_req.sendname = nil
	end -- otherwise same host as original request; don't need change anything
	if is_connect then
		if uri_t.path ~= nil and uri_t.path ~= "" then
			return nil, "CONNECT requests cannot have a path", ce.EINVAL
		elseif uri_t.query ~= nil then
			return nil, "CONNECT requests cannot have a query", ce.EINVAL
		end
	else
		local new_path
		if uri_t.path == nil or uri_t.path == "" then
			new_path = "/"
		else
			new_path = uri_t.path
			if new_path:sub(1, 1) ~= "/" then -- relative path
				if not orig_target then
					return nil, "base path not valid for relative redirect", ce.EINVAL
				end
				local orig_path = orig_target.path or "/"
				new_path = http_util.resolve_relative_path(orig_path, new_path)
			end
		end
		if uri_t.query then
			new_path = new_path .. "?" .. uri_t.query
		end
		if target_authority then
			new_path = target_authority .. new_path
		end
		new_req.headers:upsert(":path", new_path)
	end
	if uri_t.userinfo then
		local field
		if is_connect then
			field = "proxy-authorization"
		else
			field = "authorization"
		end
		new_req.headers:upsert(field, "basic " .. basexx.to_base64(uri_t.userinfo), true)
	end
	if not new_req.tls and self.tls then
		--[[ RFC 7231 5.5.2: A user agent MUST NOT send a Referer header field in an
		unsecured HTTP request if the referring page was received with a secure protocol.]]
		new_req.headers:delete("referer")
	else
		new_req.headers:upsert("referer", self:to_uri(false))
	end
	-- Change POST requests to a body-less GET on redirect?
	local orig_status = orig_headers:get(":status")
	if (orig_status == "303"
		or (orig_status == "301" and not self.post301)
		or (orig_status == "302" and not self.post302)
		) and method == "POST"
	then
		new_req.headers:upsert(":method", "GET")
		-- Remove headers that don't make sense without a body
		-- Headers that require a body
		new_req.headers:delete("transfer-encoding")
		new_req.headers:delete("content-length")
		-- Representation Metadata from RFC 7231 Section 3.1
		new_req.headers:delete("content-encoding")
		new_req.headers:delete("content-language")
		new_req.headers:delete("content-location")
		new_req.headers:delete("content-type")
		-- Other...
		local expect = new_req.headers:get("expect")
		if expect and expect:lower() == "100-continue" then
			new_req.headers:delete("expect")
		end
		new_req.body = nil
	end
	return new_req
end

function request_methods:set_body(body)
	self.body = body
	local length
	if type(self.body) == "string" then
		length = #body
	end
	if length then
		self.headers:upsert("content-length", string.format("%d", #body))
	end
	if not length or length > 1024 then
		self.headers:append("expect", "100-continue")
	end
	return true
end

local function non_final_status(status)
	return status:sub(1, 1) == "1" and status ~= "101"
end

function request_methods:go(timeout)
	local deadline = timeout and (monotime()+timeout)

	local cloned_headers = false -- only clone headers when we need to
	local request_headers = self.headers
	local host = self.host
	local port = self.port
	local tls = self.tls

	-- RFC 6797 Section 8.3
	if not tls and self.hsts and self.hsts:check(host) then
		tls = true

		if request_headers:get(":scheme") == "http" then
			-- The UA MUST replace the URI scheme with "https"
			if not cloned_headers then
				request_headers = request_headers:clone()
				cloned_headers = true
			end
			request_headers:upsert(":scheme", "https")
		end

		-- if the URI contains an explicit port component of "80", then
		-- the UA MUST convert the port component to be "443", or
		-- if the URI contains an explicit port component that is not
		-- equal to "80", the port component value MUST be preserved
		if port == 80 then
			port = 443
		end
	end

	if self.cookie_store then
		local cookie_header = self.cookie_store:lookup_for_request(request_headers, host, self.site_for_cookies, self.is_top_level)
		if cookie_header ~= "" then
			if not cloned_headers then
				request_headers = request_headers:clone()
				cloned_headers = true
			end
			-- Append rather than upsert: user may have added their own cookies
			request_headers:append("cookie", cookie_header)
		end
	end

	local connection

	local proxy = self.proxy
	if proxy == nil and self.proxies then
		assert(getmetatable(self.proxies) == http_proxies.mt, "proxies property should be an http.proxies object")
		local scheme = tls and "https" or "http" -- rather than :scheme
		proxy = self.proxies:choose(scheme, host)
	end
	if proxy then
		if type(proxy) == "string" then
			proxy = assert(uri_patt:match(proxy), "invalid proxy URI")
			proxy.path = nil -- ignore proxy.path component
		else
			assert(type(proxy) == "table" and getmetatable(proxy) == nil and proxy.scheme, "invalid proxy URI")
			proxy = {
				scheme = proxy.scheme;
				userinfo = proxy.userinfo;
				host = proxy.host;
				port = proxy.port;
				-- ignore proxy.path component
			}
		end
		if proxy.scheme == "http" or proxy.scheme == "https" then
			if tls then
				-- Proxy via a CONNECT request
				local authority = http_util.to_authority(host, port, nil)
				local connect_request = new_connect(proxy, authority)
				connect_request.proxy = false
				connect_request.version = 1.1 -- TODO: CONNECT over HTTP/2
				if connect_request.tls then
					error("NYI: TLS over TLS")
				end
				-- Perform CONNECT request
				local headers, stream, errno = connect_request:go(deadline and deadline-monotime())
				if not headers then
					return nil, stream, errno
				end
				-- RFC 7231 Section 4.3.6:
				-- Any 2xx (Successful) response indicates that the sender (and all
				-- inbound proxies) will switch to tunnel mode
				local status_reply = headers:get(":status")
				if status_reply:sub(1, 1) ~= "2" then
					stream:shutdown()
					return nil, ce.strerror(ce.ECONNREFUSED), ce.ECONNREFUSED
				end
				local sock = stream.connection:take_socket()
				local err, errno2
				connection, err, errno2 = client.negotiate(sock, {
					host = host;
					tls = tls;
					ctx = self.ctx;
					sendname = self.sendname;
					version = self.version;
					h2_settings = default_h2_settings;
				}, deadline and deadline-monotime())
				if connection == nil then
					sock:close()
					return nil, err, errno2
				end
			else
				if request_headers:get(":method") == "CONNECT" then
					error("cannot use HTTP Proxy with CONNECT method")
				end
				-- TODO: Check if :path already has authority?
				local old_url = self:to_uri(false)
				host = assert(proxy.host, "proxy is missing host")
				port = proxy.port or http_util.scheme_to_port[proxy.scheme]
				-- proxy requests get a uri that includes host as their path
				if not cloned_headers then
					request_headers = request_headers:clone()
					cloned_headers = true -- luacheck: ignore 311
				end
				request_headers:upsert(":path", old_url)
				if proxy.userinfo then
					request_headers:upsert("proxy-authorization", "basic " .. basexx.to_base64(proxy.userinfo), true)
				end
			end
		elseif proxy.scheme:match "^socks" then
			local socks = http_socks.connect(proxy)
			local ok, err, errno = socks:negotiate(host, port, deadline and deadline-monotime())
			if not ok then
				return nil, err, errno
			end
			local sock = socks:take_socket()
			connection, err, errno = client.negotiate(sock, {
				tls = tls;
				ctx = self.ctx;
				sendname = self.sendname ~= nil and self.sendname or host;
				version = self.version;
				h2_settings = default_h2_settings;
			}, deadline and deadline-monotime())
			if connection == nil then
				sock:close()
				return nil, err, errno
			end
		else
			error(string.format("unsupported proxy type (%s)", proxy.scheme))
		end
	end

	if not connection then
		local err, errno
		connection, err, errno = client.connect({
			host = host;
			port = port;
			bind = self.bind;
			tls = tls;
			ctx = self.ctx;
			sendname = self.sendname;
			version = self.version;
			h2_settings = default_h2_settings;
		}, deadline and deadline-monotime())
		if connection == nil then
			return nil, err, errno
		end
		-- Close the connection (and free resources) when done
		connection:onidle(connection.close)
	end

	local stream do
		local err, errno
		stream, err, errno = connection:new_stream()
		if stream == nil then
			return nil, err, errno
		end
	end

	local body = self.body
	do -- Write outgoing headers
		local ok, err, errno = stream:write_headers(request_headers, body == nil, deadline and deadline-monotime())
		if not ok then
			stream:shutdown()
			return nil, err, errno
		end
	end

	local headers
	if body then
		local expect = request_headers:get("expect")
		if expect and expect:lower() == "100-continue" then
			-- Try to wait for 100-continue before proceeding
			if deadline then
				local err, errno
				headers, err, errno = stream:get_headers(math.min(self.expect_100_timeout, deadline-monotime()))
				if headers == nil and (errno ~= ce.ETIMEDOUT or monotime() > deadline) then
					stream:shutdown()
					if err == nil then
						return nil, ce.strerror(ce.EPIPE), ce.EPIPE
					end
					return nil, err, errno
				end
			else
				local err, errno
				headers, err, errno = stream:get_headers(self.expect_100_timeout)
				if headers == nil and errno ~= ce.ETIMEDOUT then
					stream:shutdown()
					if err == nil then
						return nil, ce.strerror(ce.EPIPE), ce.EPIPE
					end
					return nil, err, errno
				end
			end
			if headers and headers:get(":status") ~= "100" then
				-- Don't send body
				body = nil
			end
		end
		if body then
			local ok, err, errno
			if type(body) == "string" then
				ok, err, errno = stream:write_body_from_string(body, deadline and deadline-monotime())
			elseif io.type(body) == "file" then
				ok, err, errno = body:seek("set")
				if ok then
					ok, err, errno = stream:write_body_from_file(body, deadline and deadline-monotime())
				end
			elseif type(body) == "function" then
				-- call function to get body segments
				while true do
					local chunk = body()
					if chunk then
						ok, err, errno = stream:write_chunk(chunk, false, deadline and deadline-monotime())
						if not ok then
							break
						end
					else
						ok, err, errno = stream:write_chunk("", true, deadline and deadline-monotime())
						break
					end
				end
			end
			if not ok then
				stream:shutdown()
				return nil, err, errno
			end
		end
	end
	if not headers or non_final_status(headers:get(":status")) then
		-- Skip through 1xx informational headers.
		-- From RFC 7231 Section 6.2: "A user agent MAY ignore unexpected 1xx responses"
		repeat
			local err, errno
			headers, err, errno = stream:get_headers(deadline and (deadline-monotime()))
			if headers == nil then
				stream:shutdown()
				if err == nil then
					return nil, ce.strerror(ce.EPIPE), ce.EPIPE
				end
				return nil, err, errno
			end
		until not non_final_status(headers:get(":status"))
	end

	-- RFC 6797 Section 8.1
	if tls and self.hsts and headers:has("strict-transport-security") then
		-- If a UA receives more than one STS header field in an HTTP
		-- response message over secure transport, then the UA MUST process
		-- only the first such header field.
		local sts = headers:get("strict-transport-security")
		sts = sts_patt:match(sts)
		-- The UA MUST ignore any STS header fields not conforming to the grammar specified.
		if sts then
			self.hsts:store(self.host, sts)
		end
	end

	if self.cookie_store then
		self.cookie_store:store_from_request(request_headers, headers, self.host, self.site_for_cookies)
	end

	if self.follow_redirects and headers:get(":status"):sub(1,1) == "3" then
		stream:shutdown()
		local new_req, err2, errno2 = self:handle_redirect(headers)
		if not new_req then
			return nil, err2, errno2
		end
		return new_req:go(deadline and (deadline-monotime()))
	end

	return headers, stream
end

return {
	new_from_uri = new_from_uri;
	new_connect = new_connect;
	methods = request_methods;
	mt = request_mt;
}
