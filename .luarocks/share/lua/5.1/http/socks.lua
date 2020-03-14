--[[
This module implements a subset of SOCKS as defined in RFC 1928.

SOCKS5 has different authentication mechanisms,
currently this code only supports username+password auth (defined in RFC 1929).

URI format is taken from curl:
  - socks5:// is SOCKS5, resolving the authority locally
  - socks5h:// is SOCKS5, but let the proxy resolve the hostname
]]

local cqueues = require "cqueues"
local monotime = cqueues.monotime
local ca = require "cqueues.auxlib"
local ce = require "cqueues.errno"
local cs = require "cqueues.socket"
local spack = string.pack or require "compat53.string".pack -- luacheck: ignore 143
local sunpack = string.unpack or require "compat53.string".unpack -- luacheck: ignore 143
local IPv4 = require "lpeg_patterns.IPv4"
local IPv6 = require "lpeg_patterns.IPv6"
local uri_patts = require "lpeg_patterns.uri"
local http_util = require "http.util"

local EOF = require "lpeg".P(-1)
local IPv4address = require "lpeg_patterns.IPv4".IPv4address
local IPv6address = require "lpeg_patterns.IPv6".IPv6address
local IPaddress = (IPv4address + IPv6address) * EOF

local socks_methods = {}
local socks_mt = {
	__name = "http.socks";
	__index = socks_methods;
}

local function onerror(socket, op, why, lvl) -- luacheck: ignore 212
	return string.format("%s: %s", op, ce.strerror(why)), why
end

local function new()
	return setmetatable({
		version = 5;
		socket = nil;
		family = nil;
		host = nil;
		port = nil;
		needs_resolve = false;
		available_auth_methods = { "\0", ["\0"] = true; };
		username = nil;
		password = nil;
		dst_family = nil;
		dst_host = nil;
		dst_port = nil;
	}, socks_mt)
end

local function connect(socks_uri)
	if type(socks_uri) == "string" then
		socks_uri = assert(uri_patts.uri:match(socks_uri), "invalid URI")
	end
	local self = new()
	if socks_uri.scheme == "socks5" then
		self.needs_resolve = true
	elseif socks_uri.scheme ~= "socks5h" then
		error("only SOCKS5 proxys supported")
	end
	assert(socks_uri.path == nil, "path not expected")
	local username, password
	if socks_uri.userinfo then
		username, password = socks_uri.userinfo:match("^([^:]*):(.*)$")
		if username == nil then
			error("invalid username/password format")
		end
	end
	self.host = socks_uri.host
	self.port = socks_uri.port or 1080
	if username then
		self:add_username_password_auth(username, password)
	end
	return self
end

local function fdopen(socket)
	local self = new()
	socket:onerror(onerror)
	self.socket = socket
	return self
end

function socks_methods:clone()
	if self.socket then
		error("cannot clone live http.socks object")
	end
	local clone = new()
	clone.family = self.family
	clone.host = self.host
	clone.port = self.port
	clone.needs_resolve = self.needs_resolve
	if self.username then
		clone:add_username_password_auth(self.username, self.password)
	end
	return clone
end

function socks_methods:add_username_password_auth(username, password)
	self.username = http_util.decodeURIComponent(username)
	self.password = http_util.decodeURIComponent(password)
	if not self.available_auth_methods["\2"] then
		table.insert(self.available_auth_methods, "\2")
		self.available_auth_methods["\2"] = true
	end
	return true
end

-- RFC 1929
local function username_password_auth(self, deadline)
	do
		local data = spack("Bs1s1", 1, self.username, self.password)
		local ok, err, errno = self.socket:xwrite(data, "bn", deadline and deadline-monotime())
		if not ok then
			return nil, err, errno
		end
	end
	do
		local version, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not version then
			if err == nil then
				return nil, "username_password_auth: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		end
		if version ~= "\1" then
			return nil, "username_password_auth: invalid username/password auth version", ce.EILSEQ
		end
	end
	do
		local ok, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not ok then
			if err == nil then
				return nil, "username_password_auth: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		end
		if ok ~= "\0" then
			return nil, "username_password_auth: "..ce.strerror(ce.EACCES), ce.EACCES
		end
	end
	return true
end

function socks_methods:negotiate(host, port, timeout)
	local deadline = timeout and monotime()+timeout

	assert(host, "host expected")
	port = assert(tonumber(port), "numeric port expected")

	if self.socket == nil then
		assert(self.host)
		local socket, err, errno = ca.fileresult(cs.connect {
			family = self.family;
			host = self.host;
			port = self.port;
			sendname = false;
			nodelay = true;
		})
		if socket == nil then
			return nil, err, errno
		end
		socket:onerror(onerror)
		self.socket = socket
	end

	local ip = IPaddress:match(host)
	if self.needs_resolve and not ip then
		-- Waiting on https://github.com/wahern/cqueues/issues/164
		error("NYI: need to resolve locally")
	end

	do
		local data = "\5"..string.char(#self.available_auth_methods)..table.concat(self.available_auth_methods)
		local ok, err, errno = self.socket:xwrite(data, "bn", deadline and deadline-monotime())
		if not ok then
			return nil, err, errno
		end
	end
	do
		local byte, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not byte then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		elseif byte ~= "\5" then
			return nil, "socks:negotiate: not SOCKS5", ce.EILSEQ
		end
	end
	local auth_method do
		local err, errno
		auth_method, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not auth_method then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		end
		if self.available_auth_methods[auth_method] == nil then
			return nil, "socks:negotiate: unknown authentication method", ce.EILSEQ
		end
	end
	if auth_method == "\0" then -- luacheck: ignore 542
		-- do nothing
	elseif auth_method == "\2" then
		local ok, err, errno = username_password_auth(self, deadline)
		if not ok then
			return nil, err, errno
		end
	else
		error("unreachable") -- implies `available_auth_methods` was edited while this was in progress
	end
	do
		local data
		if getmetatable(ip) == IPv4.IPv4_mt then
			data = spack(">BBx Bc4I2", 5, 1, 1, ip:binary(), port)
		elseif getmetatable(ip) == IPv6.IPv6_mt then
			data = spack(">BBx Bc16I2", 5, 1, 4, ip:binary(), port)
		else -- domain name
			data = spack(">BBx Bs1I2", 5, 1, 3, host, port)
		end
		local ok, err, errno = self.socket:xwrite(data, "bn", deadline and deadline-monotime())
		if not ok then
			return nil, err, errno
		end
	end
	do
		local byte, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not byte then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		elseif byte ~= "\5" then
			return nil, "socks:negotiate: not SOCKS5", ce.EILSEQ
		end
	end
	do
		local code, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not code then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		elseif code ~= "\0" then
			local num_code = code:byte()
			if num_code == 1 then
				err = "general SOCKS server failure"
			elseif num_code == 2 then
				err = "connection not allowed by ruleset"
				errno = ce.EACCES
			elseif num_code == 3 then
				err = "Network unreachable"
				errno = ce.ENETUNREACH
			elseif num_code == 4 then
				err = "Host unreachable"
				errno = ce.EHOSTUNREACH
			elseif num_code == 5 then
				err = "Connection refused"
				errno = ce.ECONNREFUSED
			elseif num_code == 6 then
				err = "TTL expired"
				errno = ce.ETIMEDOUT
			elseif num_code == 7 then
				err = "Command not supported"
				errno = ce.EOPNOTSUPP
			elseif num_code == 8 then
				err = "Address type not supported"
				errno = ce.EAFNOSUPPORT
			else
				err = "Unknown code"
				errno = ce.PROTO
			end
			return nil, string.format("socks:negotiate: remote error %d: %s", num_code, err), errno
		end
	end
	do
		local byte, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not byte then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		elseif byte ~= "\0" then
			return nil, "socks:negotiate: reserved field set to non-zero", ce.EILSEQ
		end
	end
	local dst_family, dst_host, dst_port do
		local atype, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
		if not atype then
			if err == nil then
				return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
			end
			return nil, err, errno
		end
		if atype == "\1" then
			local ipv4
			ipv4, err, errno = self.socket:xread(4, "b", deadline and deadline-monotime())
			if not ipv4 or #ipv4 < 4 then
				if err == nil then
					return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
				end
				return nil, err, errno
			end
			dst_family = cs.AF_INET
			dst_host = string.format("%d.%d.%d.%d", ipv4:byte(1, 4))
		elseif atype == "\4" then
			local ipv6
			ipv6, err, errno = self.socket:xread(16, "b", deadline and deadline-monotime())
			if not ipv6 or #ipv6 < 16 then
				if err == nil then
					return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
				end
				return nil, err, errno
			end
			dst_family = cs.AF_INET6
			dst_host = string.format("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
				ipv6:byte(1, 16))
		elseif atype == "\3" then
			local len
			len, err, errno = self.socket:xread(1, "b", deadline and deadline-monotime())
			if not len then
				if err == nil then
					return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
				end
				return nil, err, errno
			end
			dst_family = cs.AF_UNSPEC
			len = string.byte(len)
			dst_host, err, errno = self.socket:xread(len, "b", deadline and deadline-monotime())
			if not dst_host or #dst_host < len then
				if err == nil then
					return nil, "socks:negotiate: "..ce.strerror(ce.EPIPE), ce.EPIPE
				end
				return nil, err, errno
			end
		else
			return nil, "socks:negotiate: unknown address type", ce.EAFNOSUPPORT
		end
	end
	do
		local dst_port_bin, err, errno = self.socket:xread(2, "b", deadline and deadline-monotime())
		if not dst_port_bin then
			return nil, err or ce.EPIPE, errno
		end
		dst_port = sunpack(">I2", dst_port_bin)
	end
	self.dst_family = dst_family
	self.dst_host = dst_host
	self.dst_port = dst_port
	return true
end

function socks_methods:close()
	if self.socket then
		self.socket:close()
	end
end

function socks_methods:take_socket()
	local s = self.socket
	if s == nil then
		-- already taken
		return nil
	end
	self.socket = nil
	return s
end

return {
	connect = connect;
	fdopen = fdopen;
}
