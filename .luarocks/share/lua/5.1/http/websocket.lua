--[[
WebSocket module
Specified in RFC-6455

This code is partially based on MIT/X11 code Copyright (C) 2012 Florian Zeitz

Design criteria:
  - Client API must work without an event loop
  - Borrow from the Browser Javascript WebSocket API when sensible
  - server-side API should mirror client-side API
  - avoid reading data from the socket when the application doesn't want it
	(and loosing our TCP provided backpressure)


## Notes on websocket pings:

  - You MAY not receive a pong for every ping you send.
  - You MAY receive extra pongs

These two facts together mean that you can't track pings.
Hence pings are only useful to know if the peer is still connected.
If the peer is sending *anything*, then you know they are still connected.
]]

local basexx = require "basexx"
local spack = string.pack or require "compat53.string".pack -- luacheck: ignore 143
local sunpack = string.unpack or require "compat53.string".unpack -- luacheck: ignore 143
local unpack = table.unpack or unpack -- luacheck: ignore 113 143
local utf8 = utf8 or require "compat53.utf8" -- luacheck: ignore 113
local cqueues = require "cqueues"
local monotime = cqueues.monotime
local ce = require "cqueues.errno"
local lpeg = require "lpeg"
local http_patts = require "lpeg_patterns.http"
local rand = require "openssl.rand"
local digest = require "openssl.digest"
local bit = require "http.bit"
local onerror  = require "http.connection_common".onerror
local new_headers = require "http.headers".new
local http_request = require "http.request"

local EOF = lpeg.P(-1)
local Connection = lpeg.Ct(http_patts.Connection) * EOF
local Sec_WebSocket_Protocol_Client = lpeg.Ct(http_patts.Sec_WebSocket_Protocol_Client) * EOF
local Sec_WebSocket_Extensions = lpeg.Ct(http_patts.Sec_WebSocket_Extensions) * EOF

local websocket_methods = {
	-- Max seconds to wait after sending close frame until closing connection
	close_timeout = 3;
}

local websocket_mt = {
	__name = "http.websocket";
	__index = websocket_methods;
}

function websocket_mt:__tostring()
	return string.format("http.websocket{type=%q;readyState=%d}",
		self.type, self.readyState)
end

local magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

-- a nonce consisting of a randomly selected 16-byte value that has been base64-encoded
local function new_key()
	return basexx.to_base64(rand.bytes(16))
end

local function base64_sha1(str)
	return basexx.to_base64(digest.new("sha1"):final(str))
end

-- trim12 from http://lua-users.org/wiki/StringTrim
local function trim(s)
	local from = s:match"^%s*()"
	return from > #s and "" or s:match(".*%S", from)
end

--[[ this value MUST be non-empty strings with characters in the range U+0021
to U+007E not including separator characters as defined in [RFC2616] ]]
local function validate_protocol(p)
	return p:match("^[\33\35-\39\42\43\45\46\48-\57\65-\90\94-\122\124\126\127]+$")
end

local function validate_utf8(s)
	local ok, pos = utf8.len(s)
	if not ok then
		return nil, pos
	end
	-- UTF-16 surrogates not allowed
	for p, c in utf8.codes(s) do
		if c >= 0xD800 and c <= 0xDFFF then
			return nil, p
		end
	end
	return true
end

-- XORs the string `str` with a 32bit key
local function apply_mask(str, key)
	assert(#key == 4)
	local data = {}
	for i = 1, #str do
		local key_index = (i-1)%4 + 1
		data[i] = string.char(bit.bxor(key[key_index], str:byte(i)))
	end
	return table.concat(data, "", 1, #str)
end

local function build_frame(desc)
	local data = desc.data or ""

	assert(desc.opcode and desc.opcode >= 0 and desc.opcode <= 0xF, "Invalid WebSocket opcode")
	if desc.opcode >= 0x8 then
		-- RFC 6455 5.5
		assert(#data <= 125, "WebSocket control frames MUST have a payload length of 125 bytes or less.")
	end

	local b1 = desc.opcode
	if desc.FIN then
		b1 = bit.bor(b1, 0x80)
	end
	if desc.RSV1 then
		b1 = bit.bor(b1, 0x40)
	end
	if desc.RSV2 then
		b1 = bit.bor(b1, 0x20)
	end
	if desc.RSV3 then
		b1 = bit.bor(b1, 0x10)
	end

	local b2 = #data
	local length_extra
	if b2 <= 125 then -- 7-bit length
		length_extra = ""
	elseif b2 <= 0xFFFF then -- 2-byte length
		b2 = 126
		length_extra = spack(">I2", #data)
	else -- 8-byte length
		b2 = 127
		length_extra = spack(">I8", #data)
	end

	local key = ""
	if desc.MASK then
		local key_a = desc.key
		if key_a then
			key = string.char(unpack(key_a, 1, 4))
		else
			key = rand.bytes(4)
			key_a = {key:byte(1,4)}
		end
		b2 = bit.bor(b2, 0x80)
		data = apply_mask(data, key_a)
	end

	return string.char(b1, b2) .. length_extra .. key .. data
end

local function build_close(code, message, mask)
	local data
	if code then
		data = spack(">I2", code)
		if message then
			assert(#message<=123, "Close reason must be <=123 bytes")
			data = data .. message
		end
	else
		data = ""
	end
	return {
		opcode = 0x8;
		FIN = true;
		MASK = mask;
		data = data;
	}
end

local function read_frame(sock, deadline)
	local frame, first_2 do
		local err, errno
		first_2, err, errno = sock:xread(2, "b", deadline and (deadline-monotime()))
		if not first_2 then
			return nil, err, errno
		elseif #first_2 ~= 2 then
			sock:seterror("r", ce.EILSEQ)
			local ok, errno2 = sock:unget(first_2)
			if not ok then
				return nil, onerror(sock, "unget", errno2)
			end
			return nil, onerror(sock, "read_frame", ce.EILSEQ)
		end
		local byte1, byte2 = first_2:byte(1, 2)
		frame = {
			FIN = bit.band(byte1, 0x80) ~= 0;
			RSV1 = bit.band(byte1, 0x40) ~= 0;
			RSV2 = bit.band(byte1, 0x20) ~= 0;
			RSV3 = bit.band(byte1, 0x10) ~= 0;
			opcode = bit.band(byte1, 0x0F);

			MASK = bit.band(byte2, 0x80) ~= 0;
			length = bit.band(byte2, 0x7F);

			data = nil;
		}
	end

	local fill_length = frame.length
	if fill_length == 126 then
		fill_length = 2
	elseif fill_length == 127 then
		fill_length = 8
	end
	if frame.MASK then
		fill_length = fill_length + 4
	end
	do
		local ok, err, errno = sock:fill(fill_length, 0)
		if not ok then
			local unget_ok1, unget_errno1 = sock:unget(first_2)
			if not unget_ok1 then
				return nil, onerror(sock, "unget", unget_errno1)
			end
			if errno == ce.ETIMEDOUT then
				local timeout = deadline and deadline-monotime()
				if cqueues.poll(sock, timeout) ~= timeout then
					-- retry
					return read_frame(sock, deadline)
				end
			elseif err == nil then
				sock:seterror("r", ce.EILSEQ)
				return nil, onerror(sock, "read_frame", ce.EILSEQ)
			end
			return nil, err, errno
		end
	end

	-- if `fill` succeeded these shouldn't be able to fail
	local extra_fill_unget
	if frame.length == 126 then
		extra_fill_unget = assert(sock:xread(2, "b", 0))
		frame.length = sunpack(">I2", extra_fill_unget)
		fill_length = fill_length - 2
	elseif frame.length == 127 then
		extra_fill_unget = assert(sock:xread(8, "b", 0))
		frame.length = sunpack(">I8", extra_fill_unget)
		fill_length = fill_length - 8 + frame.length
	end

	if extra_fill_unget then
		local ok, err, errno = sock:fill(fill_length, 0)
		if not ok then
			local unget_ok1, unget_errno1 = sock:unget(extra_fill_unget)
			if not unget_ok1 then
				return nil, onerror(sock, "unget", unget_errno1)
			end
			local unget_ok2, unget_errno2 = sock:unget(first_2)
			if not unget_ok2 then
				return nil, onerror(sock, "unget", unget_errno2)
			end
			if errno == ce.ETIMEDOUT then
				local timeout = deadline and deadline-monotime()
				if cqueues.poll(sock, timeout) ~= timeout then
					-- retry
					return read_frame(sock, deadline)
				end
			elseif err == nil then
				sock:seterror("r", ce.EILSEQ)
				return nil, onerror(sock, "read_frame", ce.EILSEQ)
			end
			return nil, err, errno
		end
	end

	if frame.MASK then
		local key = assert(sock:xread(4, "b", 0))
		frame.key = { key:byte(1, 4) }
	end

	do
		local data = assert(sock:xread(frame.length, "b", 0))
		if frame.MASK then
			frame.data = apply_mask(data, frame.key)
		else
			frame.data = data
		end
	end

	return frame
end

local function parse_close(data)
	local code, message
	if #data >= 2 then
		code = sunpack(">I2", data)
		if #data > 2 then
			message = data:sub(3)
		end
	end
	return code, message
end

function websocket_methods:send_frame(frame, timeout)
	if self.readyState < 1 then
		return nil, onerror(self.socket, "send_frame", ce.ENOTCONN)
	elseif self.readyState > 2 then
		return nil, onerror(self.socket, "send_frame", ce.EPIPE)
	end
	local ok, err, errno = self.socket:xwrite(build_frame(frame), "bn", timeout)
	if not ok then
		return nil, err, errno
	end
	if frame.opcode == 0x8 then
		self.readyState = 2
	end
	return true
end

function websocket_methods:send(data, opcode, timeout)
	assert(type(data) == "string")
	if opcode == "text" or opcode == nil then
		opcode = 0x1
	elseif opcode == "binary" then
		opcode = 0x2;
	end
	return self:send_frame({
		FIN = true;
		--[[ RFC 6455
		5.1: A server MUST NOT mask any frames that it sends to the client
		6.1.5: If the data is being sent by the client, the frame(s) MUST be masked]]
		MASK = self.type == "client";
		opcode = opcode;
		data = data;
	}, timeout)
end

local function close_helper(self, code, reason, deadline)
	if self.readyState < 1 then
		self.request = nil
		self.stream = nil
		self.readyState = 3
		-- return value doesn't matter; this branch cannot be called from anywhere that uses it
		return nil, ce.strerror(ce.ENOTCONN), ce.ENOTCONN
	elseif self.readyState == 3 then
		return nil, ce.strerror(ce.EPIPE), ce.EPIPE
	end

	if self.readyState < 2 then
		local close_frame = build_close(code, reason, self.type == "client")
		-- ignore failure
		self:send_frame(close_frame, deadline and deadline-monotime())
	end

	if code ~= 1002 and not self.got_close_code and self.readyState == 2 then
		-- Do not close socket straight away, wait for acknowledgement from server
		local read_deadline = monotime() + self.close_timeout
		if deadline then
			read_deadline = math.min(read_deadline, deadline)
		end
		repeat
			if not self:receive(read_deadline-monotime()) then
				break
			end
		until self.got_close_code
	end

	if self.readyState < 3 then
		self.socket:shutdown()
		self.readyState = 3
		cqueues.poll()
		cqueues.poll()
		self.socket:close()
	end

	return nil, reason, code
end

function websocket_methods:close(code, reason, timeout)
	local deadline = timeout and (monotime()+timeout)
	code = code or 1000
	close_helper(self, code, reason, deadline)
	return true
end

function websocket_methods:receive(timeout)
	if self.readyState < 1 then
		return nil, onerror(self.socket, "receive", ce.ENOTCONN)
	elseif self.readyState > 2 then
		return nil, onerror(self.socket, "receive", ce.EPIPE)
	end
	local deadline = timeout and (monotime()+timeout)
	while true do
		local frame, err, errno = read_frame(self.socket, deadline)
		if frame == nil then
			return nil, err, errno
		end

		-- Error cases
		if frame.RSV1 or frame.RSV2 or frame.RSV3 then -- Reserved bits non zero
			return close_helper(self, 1002, "Reserved bits not zero", deadline)
		end

		if frame.opcode < 0x8 then
			if frame.opcode == 0x0 then -- Continuation frames
				if not self.databuffer then
					return close_helper(self, 1002, "Unexpected continuation frame", deadline)
				end
				self.databuffer[#self.databuffer+1] = frame.data
			elseif frame.opcode == 0x1 or frame.opcode == 0x2 then -- Text or Binary frame
				if self.databuffer then
					return close_helper(self, 1002, "Continuation frame expected", deadline)
				end
				self.databuffer = { frame.data }
				self.databuffer_type = frame.opcode
			else
				return close_helper(self, 1002, "Reserved opcode", deadline)
			end
			if frame.FIN then
				local databuffer_type = self.databuffer_type
				local databuffer = table.concat(self.databuffer)
				if databuffer_type == 0x1 then
					databuffer_type = "text"
					--[[ RFC 6455 8.1
					When an endpoint is to interpret a byte stream as UTF-8 but finds
					that the byte stream is not, in fact, a valid UTF-8 stream, that
					endpoint MUST _Fail the WebSocket Connection_.]]
					local valid_utf8, err_pos = validate_utf8(databuffer)
					if not valid_utf8 then
						return close_helper(self, 1007, string.format("invalid utf-8 at position %d", err_pos))
					end
				elseif databuffer_type == 0x2 then
					databuffer_type = "binary"
				end
				self.databuffer_type, self.databuffer = nil, nil
				return databuffer, databuffer_type
			end
		else -- Control frame
			if frame.length > 125 then -- Control frame with too much payload
				return close_helper(self, 1002, "Payload too large", deadline)
			elseif not frame.FIN then -- Fragmented control frame
				return close_helper(self, 1002, "Fragmented control frame", deadline)
			end
			if frame.opcode == 0x8 then -- Close request
				if frame.length == 1 then
					return close_helper(self, 1002, "Close frame with payload, but too short for status code", deadline)
				end
				local status_code, message = parse_close(frame.data)
				if status_code == nil then
					--[[ RFC 6455 7.4.1
					1005 is a reserved value and MUST NOT be set as a status code in a
					Close control frame by an endpoint.  It is designated for use in
					applications expecting a status code to indicate that no status
					code was actually present.]]
					self.got_close_code = 1005
					status_code = 1000
				elseif status_code < 1000 then
					self.got_close_code = true
					return close_helper(self, 1002, "Closed with invalid status code", deadline)
				elseif ((status_code > 1003 and status_code < 1007) or status_code > 1011) and status_code < 3000 then
					self.got_close_code = true
					return close_helper(self, 1002, "Closed with reserved status code", deadline)
				else
					self.got_close_code = status_code
					if message then
						local valid_utf8, err_pos = validate_utf8(message)
						if not valid_utf8 then
							return close_helper(self, 1007, string.format("invalid utf-8 at position %d", err_pos))
						end
						self.got_close_message = message
					end
				end
				--[[ RFC 6455 5.5.1
				When sending a Close frame in response, the endpoint typically
				echos the status code it received.]]
				return close_helper(self, status_code, message, deadline)
			elseif frame.opcode == 0x9 then -- Ping frame
				local ok, err2 = self:send_pong(frame.data, deadline and (deadline-monotime()))
				if not ok and err2 ~= ce.EPIPE then
					return close_helper(self, 1002, "Pong failed", deadline)
				end
			elseif frame.opcode == 0xA then -- luacheck: ignore 542
				-- Received unexpected pong frame
			else
				return close_helper(self, 1002, "Reserved opcode", deadline)
			end
		end
	end
end

function websocket_methods:each()
	return function(self) -- luacheck: ignore 432
		return self:receive()
	end, self
end

function websocket_methods:send_ping(data, timeout)
	return self:send_frame({
		FIN = true;
		--[[ RFC 6455
		5.1: A server MUST NOT mask any frames that it sends to the client
		6.1.5: If the data is being sent by the client, the frame(s) MUST be masked]]
		MASK = self.type == "client";
		opcode = 0x9;
		data = data;
	}, timeout)
end

--[[ RFC 6455 Section 5.5.3:
A Pong frame MAY be sent unsolicited. This serves as a unidirectional heartbeat.
A response to an unsolicited Pong frame is not expected.]]
function websocket_methods:send_pong(data, timeout)
	return self:send_frame({
		FIN = true;
		--[[ RFC 6455
		5.1: A server MUST NOT mask any frames that it sends to the client
		6.1.5: If the data is being sent by the client, the frame(s) MUST be masked]]
		MASK = self.type == "client";
		opcode = 0xA;
		data = data;
	}, timeout)
end

local function new(type)
	assert(type == "client" or type == "server")
	local self = setmetatable({
		socket = nil;
		type = type;
		readyState = 0;
		databuffer = nil;
		databuffer_type = nil;
		got_close_code = nil;
		got_close_reason = nil;
		key = nil;
		protocol = nil;
		protocols = nil;
		-- only used by client:
		request = nil;
		headers = nil;
		-- only used by server:
		stream = nil;
	}, websocket_mt)
	return self
end

local function new_from_uri(uri, protocols)
	local request = http_request.new_from_uri(uri)
	local scheme = request.headers:get(":scheme")
	assert(scheme == "ws" or scheme == "wss", "scheme not websocket")
	local self = new("client")
	self.request = request
	self.request.version = 1.1
	self.request.headers:append("upgrade", "websocket")
	self.request.headers:append("connection", "upgrade")
	self.key = new_key()
	self.request.headers:append("sec-websocket-key", self.key, true)
	self.request.headers:append("sec-websocket-version", "13")
	if protocols then
		--[[ The request MAY include a header field with the name
		Sec-WebSocket-Protocol. If present, this value indicates one
		or more comma-separated subprotocol the client wishes to speak,
		ordered by preference. The elements that comprise this value
		MUST be non-empty strings with characters in the range U+0021 to
		U+007E not including separator characters as defined in
		[RFC2616] and MUST all be unique strings.]]
		local n_protocols = #protocols
		-- Copy the passed 'protocols' array so that caller is allowed to modify
		local protocols_copy = {}
		for i=1, n_protocols do
			local v = protocols[i]
			if protocols_copy[v] then
				error("duplicate protocol")
			end
			assert(validate_protocol(v), "invalid protocol")
			protocols_copy[v] = true
			protocols_copy[i] = v
		end
		self.protocols = protocols_copy
		self.request.headers:append("sec-websocket-protocol", table.concat(protocols_copy, ",", 1, n_protocols))
	end
	return self
end

--[[ Takes a response to a websocket upgrade request,
and attempts to complete a websocket connection]]
local function handle_websocket_response(self, headers, stream)
	assert(self.type == "client" and self.readyState == 0)

	if stream.connection.version < 1 or stream.connection.version >= 2 then
		return nil, "websockets only supported with HTTP 1.x", ce.EINVAL
	end

	--[[ If the status code received from the server is not 101, the
	client handles the response per HTTP [RFC2616] procedures.  In
	particular, the client might perform authentication if it
	receives a 401 status code; the server might redirect the client
	using a 3xx status code (but clients are not required to follow
	them), etc.]]
	if headers:get(":status") ~= "101" then
		return nil, "status code not 101", ce.EINVAL
	end

	--[[ If the response lacks an Upgrade header field or the Upgrade
	header field contains a value that is not an ASCII case-
	insensitive match for the value "websocket", the client MUST
	Fail the WebSocket Connection]]
	local upgrade = headers:get("upgrade")
	if not upgrade or upgrade:lower() ~= "websocket" then
		return nil, "upgrade header not websocket", ce.EINVAL
	end

	--[[ If the response lacks a Connection header field or the
	Connection header field doesn't contain a token that is an
	ASCII case-insensitive match for the value "Upgrade", the client
	MUST Fail the WebSocket Connection]]
	do
		local has_connection_upgrade = false
		local h = headers:get_comma_separated("connection")
		if not h then
			return nil, "invalid connection header", ce.EINVAL
		end
		local connection_header = Connection:match(h)
		for i=1, #connection_header do
			if connection_header[i] == "upgrade" then
				has_connection_upgrade = true
				break
			end
		end
		if not has_connection_upgrade then
			return nil, "connection header doesn't contain upgrade", ce.EINVAL
		end
	end

	--[[ If the response lacks a Sec-WebSocket-Accept header field or
	the Sec-WebSocket-Accept contains a value other than the
	base64-encoded SHA-1 of the concatenation of the Sec-WebSocket-
	Key (as a string, not base64-decoded) with the string "258EAFA5-
	E914-47DA-95CA-C5AB0DC85B11" but ignoring any leading and
	trailing whitespace, the client MUST Fail the WebSocket Connection]]
	local sec_websocket_accept = headers:get("sec-websocket-accept")
	if sec_websocket_accept == nil or
		trim(sec_websocket_accept) ~= base64_sha1(self.key .. magic)
	then
		return nil, "sec-websocket-accept header incorrect", ce.EINVAL
	end

	--[[ If the response includes a Sec-WebSocket-Extensions header field and
	this header field indicates the use of an extension that was not present
	in the client's handshake (the server has indicated an extension not
	requested by the client), the client MUST Fail the WebSocket Connection]]
	do -- For now, we don't support any extensions
		local h = headers:get_comma_separated("sec-websocket-extensions")
		if h then
			local extensions = Sec_WebSocket_Extensions:match(h)
			if not extensions then
				return nil, "invalid sec-websocket-extensions header", ce.EINVAL
			end
			return nil, "extensions not supported", ce.EINVAL
		end
	end

	--[[ If the response includes a Sec-WebSocket-Protocol header field and
	this header field indicates the use of a subprotocol that was not present
	in the client's handshake (the server has indicated a subprotocol not
	requested by the client), the client MUST Fail the WebSocket Connection]]
	local protocol = headers:get("sec-websocket-protocol")
	if protocol then
		local has_matching_protocol = self.protocols and self.protocols[protocol]
		if not has_matching_protocol then
			return nil, "unexpected protocol", ce.EINVAL
		end
	end

	-- Success!
	assert(self.socket == nil, "websocket:connect called twice")
	self.socket = assert(stream.connection:take_socket())
	self.socket:onerror(onerror)
	self.request = nil
	self.headers = headers
	self.readyState = 1
	self.protocol = protocol

	return true
end

function websocket_methods:connect(timeout)
	assert(self.type == "client" and self.readyState == 0)
	local headers, stream, errno = self.request:go(timeout)
	if not headers then
		return nil, stream, errno
	end
	return handle_websocket_response(self, headers, stream)
end

-- Given an incoming HTTP1 request, attempts to upgrade it to a websocket connection
local function new_from_stream(stream, headers)
	assert(stream.connection.type == "server")

	if stream.connection.version < 1 or stream.connection.version >= 2 then
		return nil, "websockets only supported with HTTP 1.x", ce.EINVAL
	end

	--[[ RFC 7230: A server MUST ignore an Upgrade header field that is
	received in an HTTP/1.0 request]]
	if stream.peer_version == 1.0 then
		return nil, "upgrade headers MUST be ignored in HTTP 1.0", ce.EINVAL
	end
	local upgrade = headers:get("upgrade")
	if not upgrade or upgrade:lower() ~= "websocket" then
		return nil, "upgrade header not websocket", ce.EINVAL
	end

	do
		local has_connection_upgrade = false
		local h = headers:get_comma_separated("connection")
		if not h then
			return nil, "invalid connection header", ce.EINVAL
		end
		local connection_header = Connection:match(h)
		for i=1, #connection_header do
			if connection_header[i] == "upgrade" then
				has_connection_upgrade = true
				break
			end
		end
		if not has_connection_upgrade then
			return nil, "connection header doesn't contain upgrade", ce.EINVAL
		end
	end

	local key = headers:get("sec-websocket-key")
	if not key then
		return nil, "missing sec-websocket-key", ce.EINVAL
	end
	key = trim(key)

	if headers:get("sec-websocket-version") ~= "13" then
		return nil, "unsupported sec-websocket-version", ce.EINVAL
	end

	local protocols_available
	if headers:has("sec-websocket-protocol") then
		local h = headers:get_comma_separated("sec-websocket-protocol")
		local client_protocols = Sec_WebSocket_Protocol_Client:match(h)
		if not client_protocols then
			return nil, "invalid sec-websocket-protocol header", ce.EINVAL
		end
		--[[ The request MAY include a header field with the name
		Sec-WebSocket-Protocol. If present, this value indicates one
		or more comma-separated subprotocol the client wishes to speak,
		ordered by preference. The elements that comprise this value
		MUST be non-empty strings with characters in the range U+0021 to
		U+007E not including separator characters as defined in
		[RFC2616] and MUST all be unique strings.]]
		protocols_available = {}
		for i, protocol in ipairs(client_protocols) do
			protocol = trim(protocol)
			if protocols_available[protocol] then
				return nil, "duplicate protocol", ce.EINVAL
			end
			if not validate_protocol(protocol) then
				return nil, "invalid protocol", ce.EINVAL
			end
			protocols_available[protocol] = true
			protocols_available[i] = protocol
		end
	end

	local self = new("server")
	self.key = key
	self.protocols = protocols_available
	self.stream = stream
	return self
end

function websocket_methods:accept(options, timeout)
	assert(self.type == "server" and self.readyState == 0)
	options = options or {}

	local response_headers
	if options.headers then
		response_headers = options.headers:clone()
	else
		response_headers = new_headers()
	end
	response_headers:upsert(":status", "101")
	response_headers:upsert("upgrade", "websocket")
	response_headers:upsert("connection", "upgrade")
	response_headers:upsert("sec-websocket-accept", base64_sha1(self.key .. magic))

	local chosen_protocol
	if self.protocols and options.protocols then
		--[[ The |Sec-WebSocket-Protocol| request-header field can be
		used to indicate what subprotocols (application-level protocols
		layered over the WebSocket Protocol) are acceptable to the client.
		The server selects one or none of the acceptable protocols and echoes
		that value in its handshake to indicate that it has selected that
		protocol.]]
		for _, protocol in ipairs(options.protocols) do
			if self.protocols[protocol] then
				response_headers:upsert("sec-websocket-protocol", protocol)
				chosen_protocol = protocol
				break
			end
		end
	end

	do
		local ok, err, errno = self.stream:write_headers(response_headers, false, timeout)
		if not ok then
			return ok, err, errno
		end
	end

	self.socket = assert(self.stream.connection:take_socket())
	self.socket:onerror(onerror)
	self.stream = nil
	self.readyState = 1
	self.protocol = chosen_protocol
	return true
end

return {
	new_from_uri = new_from_uri;
	new_from_stream = new_from_stream;
	methods = websocket_methods;
	mt = websocket_mt;

	new = new;
	build_frame = build_frame;
	read_frame = read_frame;
	build_close = build_close;
	parse_close = parse_close;
}
