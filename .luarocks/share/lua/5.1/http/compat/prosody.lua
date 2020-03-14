--[[
Compatibility module for prosody's net.http
Documentation: https://prosody.im/doc/developers/net/http

This has a few key differences:
  - `compat.prosody.request` must be called from within a running cqueue
      - The callback will be called from a different thread in the cqueue
  - The returned "request" object will be a lua-http request object
      - Same request object is passed to the callback on errors and as the 4th argument on success
  - The user-agent will be from lua-http
  - lua-http features (such as HTTP2) will be used where possible
]]

local new_from_uri = require "http.request".new_from_uri
local cqueues = require "cqueues"

local function do_request(self, callback)
	local headers, stream = self:go()
	if headers == nil then
		-- `stream` is error message
		callback(stream, 0, self)
		return
	end
	local response_body, err = stream:get_body_as_string()
	stream:shutdown()
	if response_body == nil then
		callback(err, 0, self)
		return
	end
	-- code might not be convertible to a number in http2, so need `or` case
	local code = headers:get(":status")
	code = tonumber(code, 10) or code
	-- convert headers to table with comma separated values
	local headers_as_kv = {}
	for key, value in headers:each() do
		if key ~= ":status" then
			local old = headers_as_kv[key]
			if old then
				headers_as_kv[key] = old .. "," .. value
			else
				headers_as_kv[key] = value
			end
		end
	end
	local response = {
		code = code;
		httpversion = stream.peer_version;
		headers = headers_as_kv;
		body = response_body;
	}
	callback(response_body, code, response, self)
end

local function new_prosody(url, ex, callback)
	local cq = assert(cqueues.running(), "must be running inside a cqueue")
	local ok, req = pcall(new_from_uri, url)
	if not ok then
		callback(nil, 0, req)
		return nil, "invalid-url"
	end
	req.follow_redirects = false -- prosody doesn't follow redirects
	if ex then
		if ex.body then
			req.headers:upsert(":method", "POST")
			req:set_body(ex.body)
			req.headers:append("content-type", "application/x-www-form-urlencoded")
		end
		if ex.method then
			req.headers:upsert(":method", ex.method)
		end
		if ex.headers then
			for k, v in pairs(ex.headers) do
				req.headers:upsert(k:lower(), v)
			end
		end
		if ex.sslctx then
			req.ctx = ex.sslctx
		end
	end
	cq:wrap(do_request, req, callback)
	return req
end

return {
	request = new_prosody;
}
