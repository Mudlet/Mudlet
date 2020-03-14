local cqueues = require "cqueues"
local monotime = cqueues.monotime
local ca = require "cqueues.auxlib"
local cc = require "cqueues.condition"
local ce = require "cqueues.errno"
local cs = require "cqueues.socket"
local connection_common = require "http.connection_common"
local onerror = connection_common.onerror
local h1_connection = require "http.h1_connection"
local h2_connection = require "http.h2_connection"
local http_tls = require "http.tls"
local http_util = require "http.util"
local openssl_bignum = require "openssl.bignum"
local pkey = require "openssl.pkey"
local openssl_rand = require "openssl.rand"
local openssl_ssl = require "openssl.ssl"
local openssl_ctx = require "openssl.ssl.context"
local x509 = require "openssl.x509"
local name = require "openssl.x509.name"
local altname = require "openssl.x509.altname"

local hang_timeout = 0.03

-- Sense for TLS or SSL client hello
-- returns `true`, `false` or `nil, err`
local function is_tls_client_hello(socket, timeout)
	-- reading for 6 bytes should be safe, as no HTTP version
	-- has a valid client request shorter than 6 bytes
	local first_bytes, err, errno = socket:xread(6, timeout)
	if first_bytes == nil then
		return nil, err or ce.EPIPE, errno
	end
	local use_tls = not not (
		first_bytes:match("^[\21\22]\3[\1\2\3]..\1") or -- TLS
		first_bytes:match("^[\128-\255][\9-\255]\1") -- SSLv2
	)
	local ok
	ok, errno = socket:unget(first_bytes)
	if not ok then
		return nil, onerror(socket, "unget", errno, 2)
	end
	return use_tls
end

-- Wrap a bare cqueues socket in an HTTP connection of a suitable version
-- Starts TLS if necessary
-- this function *should never throw*
local function wrap_socket(self, socket, timeout)
	local deadline = timeout and monotime()+timeout
	socket:setmode("b", "b")
	socket:onerror(onerror)
	local version = self.version
	local use_tls = self.tls
	if use_tls == nil then
		local err, errno
		use_tls, err, errno = is_tls_client_hello(socket, deadline and (deadline-monotime()))
		if use_tls == nil then
			return nil, err, errno
		end
	end
	if use_tls then
		local ok, err, errno = socket:starttls(self.ctx, deadline and (deadline-monotime()))
		if not ok then
			return nil, err, errno
		end
		local ssl = assert(socket:checktls())
		if http_tls.has_alpn then
			local proto = ssl:getAlpnSelected()
			if proto then
				if proto == "h2" and (version == nil or version == 2) then
					version = 2
				elseif proto == "http/1.1" and (version == nil or version < 2) then
					version = 1.1
				elseif proto == "http/1.0" and (version == nil or version == 1.0) then
					version = 1.0
				else
					return nil, "unexpected ALPN protocol: " .. proto, ce.EILSEQNOSUPPORT
				end
			end
		end
	end
	-- Still not sure if incoming connection is an HTTP1 or HTTP2 connection
	-- Need to sniff for the h2 connection preface to find out for sure
	if version == nil then
		local is_h2, err, errno = h2_connection.socket_has_preface(socket, true, deadline and (deadline-monotime()))
		if is_h2 == nil then
			return nil, err or ce.EPIPE, errno
		end
		version = is_h2 and 2 or 1.1
	end
	local conn, err, errno
	if version == 2 then
		conn, err, errno = h2_connection.new(socket, "server", nil)
	else
		conn, err, errno = h1_connection.new(socket, "server", version)
	end
	if not conn then
		return nil, err, errno
	end
	return conn
end

local function server_loop(self)
	while self.socket do
		if self.paused then
			cqueues.poll(self.pause_cond)
		elseif self.n_connections >= self.max_concurrent then
			cqueues.poll(self.connection_done)
		else
			local socket, accept_errno = self.socket:accept({nodelay = true;}, 0)
			if socket == nil then
				if accept_errno == ce.ETIMEDOUT then
					-- Yield this thread until a client arrives
					cqueues.poll(self.socket, self.pause_cond)
				elseif accept_errno == ce.EMFILE then
					-- Wait for another request to finish
					if cqueues.poll(self.connection_done, hang_timeout) == hang_timeout then
						-- If we're stuck waiting, run a garbage collection sweep
						-- This can prevent a hang
						collectgarbage()
					end
				else
					self:onerror()(self, self, "accept", ce.strerror(accept_errno), accept_errno)
				end
			else
				self:add_socket(socket)
			end
		end
	end
end

local function handle_socket(self, socket)
	local error_operation, error_context
	local conn, err, errno = wrap_socket(self, socket, self.connection_setup_timeout)
	if not conn then
		socket:close()
		if err ~= ce.EPIPE -- client closed connection
			and errno ~= ce.ETIMEDOUT -- an operation timed out
			and errno ~= ce.ECONNRESET then
			error_operation = "wrap"
			error_context = socket
		end
	else
		local cond = cc.new()
		local idle = true
		local deadline
		conn:onidle(function()
			idle = true
			deadline = self.intra_stream_timeout + monotime()
			cond:signal(1)
		end)
		while true do
			local timeout = deadline and deadline-monotime() or self.intra_stream_timeout
			local stream
			stream, err, errno = conn:get_next_incoming_stream(timeout)
			if stream == nil then
				if (err ~= nil -- client closed connection
					and errno ~= ce.ECONNRESET
					and errno ~= ce.ENOTCONN
					and errno ~= ce.ETIMEDOUT) then
					error_operation = "get_next_incoming_stream"
					error_context = conn
					break
				elseif errno ~= ce.ETIMEDOUT or not idle or (deadline and deadline <= monotime()) then -- want to go around loop again if deadline not hit
					break
				end
			else
				idle = false
				deadline = nil
				self:add_stream(stream)
			end
		end
		-- wait for streams to complete
		if not idle then
			cond:wait()
		end
		conn:close()
	end
	self.n_connections = self.n_connections - 1
	self.connection_done:signal(1)
	if error_operation then
		self:onerror()(self, error_context, error_operation, err, errno)
	end
end

local function handle_stream(self, stream)
	local ok, err = http_util.yieldable_pcall(self.onstream, self, stream)
	stream:shutdown()
	if not ok then
		self:onerror()(self, stream, "onstream", err)
	end
end

-- Prefer whichever comes first
local function alpn_select(ssl, protos, version)
	for _, proto in ipairs(protos) do
		if proto == "h2" and (version == nil or version == 2) then
			-- HTTP2 only allows >= TLSv1.2
			-- allow override via version
			if ssl:getVersion() >= openssl_ssl.TLS1_2_VERSION or version == 2 then
				return proto
			end
		elseif (proto == "http/1.1" and (version == nil or version == 1.1))
			or (proto == "http/1.0" and (version == nil or version == 1.0)) then
			return proto
		end
	end
	return nil
end

-- create a new self signed cert
local function new_ctx(host, version)
	local ctx = http_tls.new_server_context()
	if http_tls.has_alpn then
		ctx:setAlpnSelect(alpn_select, version)
	end
	if version == 2 then
		ctx:setOptions(openssl_ctx.OP_NO_TLSv1 + openssl_ctx.OP_NO_TLSv1_1)
	end
	local crt = x509.new()
	crt:setVersion(3)
	-- serial needs to be unique or browsers will show uninformative error messages
	crt:setSerial(openssl_bignum.fromBinary(openssl_rand.bytes(16)))
	-- use the host we're listening on as canonical name
	local dn = name.new()
	dn:add("CN", host)
	crt:setSubject(dn)
	crt:setIssuer(dn) -- should match subject for a self-signed
	local alt = altname.new()
	alt:add("DNS", host)
	crt:setSubjectAlt(alt)
	-- lasts for 10 years
	crt:setLifetime(os.time(), os.time()+86400*3650)
	-- can't be used as a CA
	crt:setBasicConstraints{CA=false}
	crt:setBasicConstraintsCritical(true)
	-- generate a new private/public key pair
	local key = pkey.new({bits=2048})
	crt:setPublicKey(key)
	crt:sign(key)
	assert(ctx:setPrivateKey(key))
	assert(ctx:setCertificate(crt))
	return ctx
end

local server_methods = {
	version = nil;
	max_concurrent = math.huge;
	connection_setup_timeout = 10;
	intra_stream_timeout = 10;
}
local server_mt = {
	__name = "http.server";
	__index = server_methods;
}

function server_mt:__tostring()
	return string.format("http.server{socket=%s;n_connections=%d}",
		tostring(self.socket), self.n_connections)
end

--[[ Creates a new server object

Takes a table of options:
  - `.cq` (optional): A cqueues controller to use
  - `.socket` (optional): A cqueues socket object to accept() from
  - `.onstream`: function to call back for each stream read
  - `.onerror`: function that will be called when an error occurs (default: throw an error)
  - `.tls`: `nil`: allow both tls and non-tls connections
  -         `true`: allows tls connections only
  -         `false`: allows non-tls connections only
  - `.ctx`: an `openssl.ssl.context` object to use for tls connections
  - `       `nil`: a self-signed context will be generated
  - `.version`: the http version to allow to connect (default: any)
  - `.max_concurrent`: Maximum number of connections to allow live at a time (default: infinity)
  - `.connection_setup_timeout`: Timeout (in seconds) to wait for client to send first bytes and/or complete TLS handshake (default: 10)
  - `.intra_stream_timeout`: Timeout (in seoncds) to wait between start of client streams (default: 10)
]]
local function new_server(tbl)
	local cq = tbl.cq
	if cq == nil then
		cq = cqueues.new()
	else
		assert(cqueues.type(cq) == "controller", "optional cq field should be a cqueue controller")
	end
	local socket = tbl.socket
	if socket ~= nil then
		assert(cs.type(socket), "optional socket field should be a cqueues socket")
	end
	local onstream = assert(tbl.onstream, "missing 'onstream'")
	if tbl.ctx == nil and tbl.tls ~= false then
		error("OpenSSL context required if .tls isn't false")
	end

	local self = setmetatable({
		cq = cq;
		socket = socket;
		onstream = onstream;
		onerror_ = tbl.onerror;
		tls = tbl.tls;
		ctx = tbl.ctx;
		version = tbl.version;
		max_concurrent = tbl.max_concurrent;
		n_connections = 0;
		pause_cond = cc.new();
		paused = false;
		connection_done = cc.new(); -- signalled when connection has been closed
		connection_setup_timeout = tbl.connection_setup_timeout;
		intra_stream_timeout = tbl.intra_stream_timeout;
	}, server_mt)

	if socket then
		-- Return errors rather than throwing
		socket:onerror(function(socket, op, why, lvl) -- luacheck: ignore 431 212
			return why
		end)
		cq:wrap(server_loop, self)
	end

	return self
end

--[[
Extra options:
  - `.family`: protocol family
  - `.host`: address to bind to (required if not `.path`)
  - `.port`: port to bind to (optional if tls isn't `nil`, in which case defaults to 80 for `.tls == false` or 443 if `.tls == true`)
  - `.path`: path to UNIX socket (required if not `.host`)
  - `.v6only`: allow ipv6 only (no ipv4-mapped-ipv6)
  - `.mode`: fchmod or chmod socket after creating UNIX domain socket
  - `.mask`: set and restore umask when binding UNIX domain socket
  - `.unlink`: unlink socket path before binding?
  - `.reuseaddr`: turn on SO_REUSEADDR flag?
  - `.reuseport`: turn on SO_REUSEPORT flag?
]]
local function listen(tbl)
	local tls = tbl.tls
	local host = tbl.host
	local path = tbl.path
	assert(host or path, "need host or path")
	local port = tbl.port
	if host and port == nil then
		if tls == true then
			port = "443"
		elseif tls == false then
			port = "80"
		else
			error("need port")
		end
	end
	local ctx = tbl.ctx
	if ctx == nil and tls ~= false then
		if host then
			ctx = new_ctx(host, tbl.version)
		else
			error("Custom OpenSSL context required when using a UNIX domain socket")
		end
	end
	local s, err, errno = ca.fileresult(cs.listen {
		family = tbl.family;
		host = host;
		port = port;
		path = path;
		mode = tbl.mode;
		mask = tbl.mask;
		unlink = tbl.unlink;
		reuseaddr = tbl.reuseaddr;
		reuseport = tbl.reuseport;
		v6only = tbl.v6only;
	})
	if not s then
		return nil, err, errno
	end
	return new_server {
		cq = tbl.cq;
		socket = s;
		onstream = tbl.onstream;
		onerror = tbl.onerror;
		tls = tls;
		ctx = ctx;
		version = tbl.version;
		max_concurrent = tbl.max_concurrent;
		connection_setup_timeout = tbl.connection_setup_timeout;
		intra_stream_timeout = tbl.intra_stream_timeout;
	}
end

function server_methods:onerror_(context, op, err, errno) -- luacheck: ignore 212
	local msg = op
	if err then
		msg = msg .. ": " .. tostring(err)
	end
	error(msg, 2)
end

function server_methods:onerror(...)
	local old_handler = self.onerror_
	if select("#", ...) > 0 then
		self.onerror_ = ...
	end
	return old_handler
end

-- Actually wait for and *do* the binding
-- Don't *need* to call this, as if not it will be done lazily
function server_methods:listen(timeout)
	if self.socket then
		local ok, err, errno = ca.fileresult(self.socket:listen(timeout))
		if not ok then
			return nil, err, errno
		end
	end
	return true
end

function server_methods:localname()
	if self.socket == nil then
		return
	end
	return ca.fileresult(self.socket:localname())
end

function server_methods:pause()
	self.paused = true
	self.pause_cond:signal()
	return true
end

function server_methods:resume()
	self.paused = false
	self.pause_cond:signal()
	return true
end

function server_methods:close()
	if self.cq then
		cqueues.cancel(self.cq:pollfd())
		cqueues.poll()
		cqueues.poll()
		self.cq = nil
	end
	if self.socket then
		self.socket:close()
		self.socket = nil
	end
	self.pause_cond:signal()
	self.connection_done:signal()
	return true
end

function server_methods:pollfd()
	return self.cq:pollfd()
end

function server_methods:events()
	return self.cq:events()
end

function server_methods:timeout()
	return self.cq:timeout()
end

function server_methods:empty()
	return self.cq:empty()
end

function server_methods:step(...)
	return self.cq:step(...)
end

function server_methods:loop(...)
	return self.cq:loop(...)
end

function server_methods:add_socket(socket)
	self.n_connections = self.n_connections + 1
	self.cq:wrap(handle_socket, self, socket)
	return true
end

function server_methods:add_stream(stream)
	self.cq:wrap(handle_stream, self, stream)
	return true
end

return {
	new = new_server;
	listen = listen;
	mt = server_mt;
}
