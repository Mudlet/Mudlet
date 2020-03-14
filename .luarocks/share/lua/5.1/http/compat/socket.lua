--[[
Compatibility layer with luasocket's socket.http module
Documentation: http://w3.impa.br/~diego/software/luasocket/http.html

This module a few key differences:
  - The `.create` member is not supported
  - The user-agent will be from lua-http
  - lua-http features (such as HTTPS and HTTP2) will be used where possible
  - trailers are currently discarded
  - error messages are different
]]

local monotime = require "cqueues".monotime
local ce = require "cqueues.errno"
local request = require "http.request"
local version = require "http.version"
local reason_phrases = require "http.h1_reason_phrases"

local M = {
	PROXY = nil; -- default proxy used for connections
	TIMEOUT = 60; -- timeout for all I/O operations
	-- default user agent reported to server.
	USERAGENT = string.format("%s/%s (luasocket compatibility layer)",
		version.name, version.version);
}

local function ltn12_pump_step(src, snk)
    local chunk, src_err = src()
    local ret, snk_err = snk(chunk, src_err)
    if chunk and ret then return 1
    else return nil, src_err or snk_err end
end

local function get_body_as_string(stream, deadline)
	local body, err, errno = stream:get_body_as_string(deadline and deadline-monotime())
	if not body then
		if err == nil then
			return nil
		elseif errno == ce.ETIMEDOUT then
			return nil, "timeout"
		else
			return nil, err
		end
	end
	return body
end

local function returns_1()
	return 1
end

function M.request(reqt, b)
	local deadline = M.TIMEOUT and monotime()+M.TIMEOUT
	local req, proxy, user_headers, get_body
	if type(reqt) == "string" then
		req = request.new_from_uri(reqt)
		proxy = M.PROXY
		if b ~= nil then
			assert(type(b) == "string", "body must be nil or string")
			req.headers:upsert(":method", "POST")
			req:set_body(b)
            req.headers:upsert("content-type", "application/x-www-form-urlencoded")
		end
		get_body = get_body_as_string
	else
		assert(reqt.create == nil, "'create' option not supported")
		req = request.new_from_uri(reqt.url)
		proxy = reqt.proxy or M.PROXY
		if reqt.host ~= nil then
			req.host = reqt.host
		end
		if reqt.port ~= nil then
			req.port = reqt.port
		end
		if reqt.method ~= nil then
			assert(type(reqt.method) == "string", "'method' option must be nil or string")
			req.headers:upsert(":method", reqt.method)
		end
		if reqt.redirect == false then
			req.follow_redirects = false
		else
			req.max_redirects = 5 - (reqt.nredirects or 0)
		end
		user_headers = reqt.headers
		local step = reqt.step or ltn12_pump_step
		local src = reqt.source
		if src ~= nil then
			local co = coroutine.create(function()
				while true do
					assert(step(src, coroutine.yield))
				end
			end)
			req:set_body(function()
				-- Pass true through to coroutine to indicate success of last write
				local ok, chunk, err = coroutine.resume(co, true)
				if not ok then
					error(chunk)
				elseif err then
					error(err)
				else
					return chunk
				end
			end)
		end
		local sink = reqt.sink
		-- luasocket returns `1` when using a request table
		if sink ~= nil then
			get_body = function(stream, deadline) -- luacheck: ignore 431
				local function res_body_source()
					local chunk, err, errno = stream:get_next_chunk(deadline and deadline-monotime())
					if not chunk then
						if err == nil then
							return nil
						elseif errno == ce.EPIPE then
							return nil, "closed"
						elseif errno == ce.ETIMEDOUT then
							return nil, "timeout"
						else
							return nil, err
						end
					end
					return chunk
				end
				-- This loop is the same as ltn12.pump.all
				while true do
					local ok, err = step(res_body_source, sink)
					if not ok then
						if err then
							return nil, err
						else
							return 1
						end
					end
				end
			end
		else
			get_body = returns_1
		end
	end
	req.headers:upsert("user-agent", M.USERAGENT)
	req.proxy = proxy or false
	if user_headers then
		for name, field in pairs(user_headers) do
			name = name:lower()
			field = "" .. field .. "" -- force coercion in same style as luasocket
			if name == "host" then
				req.headers:upsert(":authority", field)
			else
				req.headers:append(name, field)
			end
		end
	end
	local res_headers, stream, errno = req:go(deadline and deadline-monotime())
	if not res_headers then
		if errno == ce.EPIPE or stream == nil then
			return nil, "closed"
		elseif errno == ce.ETIMEDOUT then
			return nil, "timeout"
		else
			return nil, stream
		end
	end
	local code = res_headers:get(":status")
	local status = reason_phrases[code]
	-- In luasocket, status codes are returned as numbers
	code = tonumber(code, 10) or code
	local headers = {}
	for name in res_headers:each() do
		if name ~= ":status" and headers[name] == nil then
			headers[name] = res_headers:get_comma_separated(name)
		end
	end
	local body, err = get_body(stream, deadline)
	stream:shutdown()
	if not body then
		return nil, err
	end
	return body, code, headers, status
end

return M
