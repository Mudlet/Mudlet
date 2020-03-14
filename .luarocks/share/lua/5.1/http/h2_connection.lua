local cqueues = require "cqueues"
local monotime = cqueues.monotime
local cc = require "cqueues.condition"
local ce = require "cqueues.errno"
local rand = require "openssl.rand"
local new_fifo = require "fifo"
local band = require "http.bit".band
local connection_common = require "http.connection_common"
local onerror = connection_common.onerror
local h2_error = require "http.h2_error"
local h2_stream = require "http.h2_stream"
local known_settings = h2_stream.known_settings
local hpack = require "http.hpack"
local h2_banned_ciphers = require "http.tls".banned_ciphers
local spack = string.pack or require "compat53.string".pack -- luacheck: ignore 143
local sunpack = string.unpack or require "compat53.string".unpack -- luacheck: ignore 143

local assert = assert
if _VERSION:match("%d+%.?%d*") < "5.3" then
	assert = require "compat53.module".assert
end

local function xor(a, b)
	return (a and b) or not (a or b)
end

local preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

local default_settings = {
	[known_settings.HEADER_TABLE_SIZE] = 4096;
	[known_settings.ENABLE_PUSH] = true;
	[known_settings.MAX_CONCURRENT_STREAMS] = math.huge;
	[known_settings.INITIAL_WINDOW_SIZE] = 65535;
	[known_settings.MAX_FRAME_SIZE] = 16384;
	[known_settings.MAX_HEADER_LIST_SIZE] = math.huge;
	[known_settings.SETTINGS_ENABLE_CONNECT_PROTOCOL] = false;
	[known_settings.TLS_RENEG_PERMITTED] = 0;
}

local function merge_settings(tbl, new)
	for i=0x1, 0x6 do
		local v = new[i]
		if v ~= nil then
			tbl[i] = v
		end
	end
end

local connection_methods = {}
for k,v in pairs(connection_common.methods) do
	connection_methods[k] = v
end
local connection_mt = {
	__name = "http.h2_connection";
	__index = connection_methods;
}

function connection_mt:__tostring()
	return string.format("http.h2_connection{type=%q}",
		self.type)
end

-- Read bytes from the given socket looking for the http2 connection preface
-- optionally ungets the bytes in case of failure
local function socket_has_preface(socket, unget, timeout)
	local deadline = timeout and (monotime()+timeout)
	local bytes = ""
	local is_h2 = true
	while #bytes < #preface do
		-- read *up to* number of bytes left in preface
		local ok, err, errno = socket:xread(#bytes-#preface, deadline and (deadline-monotime()))
		if ok == nil then
			if err == nil then
				if #bytes == 0 then
					-- client immediately closed
					return
				end
				is_h2 = false
				break
			else
				return nil, err, errno
			end
		end
		bytes = bytes .. ok
		if bytes ~= preface:sub(1, #bytes) then
			is_h2 = false
			break
		end
	end
	if unget then
		local ok, errno = socket:unget(bytes)
		if not ok then
			return nil, onerror(socket, "unget", errno, 2)
		end
	end
	return is_h2
end

local function new_connection(socket, conn_type, settings)
	if conn_type ~= "client" and conn_type ~= "server" then
		error('invalid connection type. must be "client" or "server"')
	end

	local ssl = socket:checktls()
	if ssl then
		local cipher = ssl:getCipherInfo()
		if h2_banned_ciphers[cipher.name] then
			h2_error.errors.INADEQUATE_SECURITY("bad cipher: " .. cipher.name)
		end
	end

	local self = setmetatable({
		socket = socket;
		type = conn_type;
		version = 2; -- for compat with h1_connection

		streams = setmetatable({}, {__mode="kv"});
		n_active_streams = 0;
		onidle_ = nil;
		stream0 = nil; -- store separately with a strong reference

		has_confirmed_preface = false;
		has_first_settings = false;
		had_eagain = false;

		-- For continuations
		need_continuation = nil; -- stream
		promised_stream = nil; -- stream
		recv_headers_end_stream = nil;
		recv_headers_buffer = nil;
		recv_headers_buffer_pos = nil;
		recv_headers_buffer_pad_len = nil;
		recv_headers_buffer_items = nil;
		recv_headers_buffer_length = nil;

		highest_odd_stream = -1;
		highest_odd_non_idle_stream = -1;
		highest_even_stream = -2;
		highest_even_non_idle_stream = -2;
		send_goaway_lowest = nil;
		recv_goaway_lowest = nil;
		recv_goaway = cc.new();
		new_streams = new_fifo();
		new_streams_cond = cc.new();
		peer_settings = {};
		peer_settings_cond = cc.new(); -- signaled when the peer has changed their settings
		acked_settings = {};
		send_settings = {n = 0};
		send_settings_ack_cond = cc.new(); -- for when server ACKs our settings
		send_settings_acked = 0;
		peer_flow_credits = 65535; -- 5.2.1
		peer_flow_credits_change = cc.new();
		encoding_context = nil;
		decoding_context = nil;
		pongs = {}; -- pending pings we've sent. keyed by opaque 8 byte payload
	}, connection_mt)
	self:new_stream(0)
	merge_settings(self.peer_settings, default_settings)
	merge_settings(self.acked_settings, default_settings)
	self.encoding_context = hpack.new(default_settings[known_settings.HEADER_TABLE_SIZE])
	self.decoding_context = hpack.new(default_settings[known_settings.HEADER_TABLE_SIZE])

	socket:setvbuf("full", math.huge) -- 'infinite' buffering; no write locks needed
	socket:setmode("b", "bna") -- writes that don't explicitly buffer will now flush the buffer. autoflush on
	socket:onerror(onerror)
	if self.type == "client" then
		assert(socket:xwrite(preface, "f", 0))
	end
	assert(self.stream0:write_settings_frame(false, settings or {}, 0, "f"))
	-- note that the buffer is *not* flushed right now

	return self
end

function connection_methods:timeout()
	if not self.had_eagain then
		return 0
	end
	return connection_common.methods.timeout(self)
end

local function handle_frame(self, typ, flag, streamid, payload, deadline)
	if self.need_continuation and (typ ~= 0x9 or self.need_continuation.id ~= streamid) then
		return nil, h2_error.errors.PROTOCOL_ERROR:new_traceback("CONTINUATION frame expected"), ce.EILSEQ
	end
	local handler = h2_stream.frame_handlers[typ]
	-- http2 spec section 4.1:
	-- Implementations MUST ignore and discard any frame that has a type that is unknown.
	if handler then
		local stream = self.streams[streamid]
		if stream == nil then
			if xor(streamid % 2 == 1, self.type == "client") then
				return nil, h2_error.errors.PROTOCOL_ERROR:new_traceback("Streams initiated by a client MUST use odd-numbered stream identifiers; those initiated by the server MUST use even-numbered stream identifiers"), ce.EILSEQ
			end
			-- TODO: check MAX_CONCURRENT_STREAMS
			stream = self:new_stream(streamid)
			--[[ http2 spec section 6.8
			the sender will ignore frames sent on streams initiated by
			the receiver if the stream has an identifier higher than the included
			last stream identifier
			...
			After sending a GOAWAY frame, the sender can discard frames for
			streams initiated by the receiver with identifiers higher than the
			identified last stream.  However, any frames that alter connection
			state cannot be completely ignored.  For instance, HEADERS,
			PUSH_PROMISE, and CONTINUATION frames MUST be minimally processed to
			ensure the state maintained for header compression is consistent (see
			Section 4.3); similarly, DATA frames MUST be counted toward the
			connection flow-control window.  Failure to process these frames can
			cause flow control or header compression state to become
			unsynchronized.]]
			-- If we haven't seen this stream before, and we should be discarding frames from it,
			-- then don't push it into the new_streams fifo
			if self.send_goaway_lowest == nil or streamid <= self.send_goaway_lowest then
				self.new_streams:push(stream)
				self.new_streams_cond:signal(1)
			end
		end
		local ok, err, errno = handler(stream, flag, payload, deadline)
		if not ok then
			if h2_error.is(err) and err.stream_error and streamid ~= 0 and stream.state ~= "idle" then
				local ok2, err2, errno2 = stream:rst_stream(err, deadline and deadline-monotime())
				if not ok2 then
					return nil, err2, errno2
				end
			else -- connection error or unknown error
				return nil, err, errno
			end
		end
	end
	return true
end

function connection_methods:step(timeout)
	local deadline = timeout and monotime()+timeout
	if not self.has_confirmed_preface and self.type == "server" then
		local ok, err, errno = socket_has_preface(self.socket, false, timeout)
		self.had_eagain = false
		if ok == nil then
			if errno == ce.ETIMEDOUT then
				self.had_eagain = true
				return true
			end
			return nil, err, errno
		end
		if not ok then
			return nil, h2_error.errors.PROTOCOL_ERROR:new_traceback("invalid connection preface. not an http2 client?"), ce.EILSEQ
		end
		self.has_confirmed_preface = true
	end

	local ok, connection_error, errno
	local typ, flag, streamid, payload = self:read_http2_frame(deadline and deadline-monotime())
	if typ == nil then
		-- flag might be `nil` on EOF
		ok, connection_error, errno = nil, flag, streamid
	elseif not self.has_first_settings and typ ~= 0x4 then -- XXX: Should this be more strict? e.g. what if it's an ACK?
		ok, connection_error, errno = false, h2_error.errors.PROTOCOL_ERROR:new_traceback("A SETTINGS frame MUST be the first frame sent in an HTTP/2 connection"), ce.EILSEQ
	else
		ok, connection_error, errno = handle_frame(self, typ, flag, streamid, payload, deadline)
		if ok then
			self.has_first_settings = true
		end
	end

	if not ok and connection_error and errno ~= ce.ETIMEDOUT then
		if not self.socket:eof("w") then
			local code, message
			if h2_error.is(connection_error) then
				code, message = connection_error.code, connection_error.message
			else
				code = h2_error.errors.INTERNAL_ERROR.code
			end
			-- ignore write failure here; there's nothing that can be done
			self:write_goaway_frame(nil, code, message, deadline and deadline-monotime())
		end
		if errno == nil and h2_error.is(connection_error) and connection_error.code == h2_error.errors.PROTOCOL_ERROR.code then
			errno = ce.EILSEQ
		end
		return nil, connection_error, errno
	end

	return true
end

function connection_methods:empty()
	return self.socket:eof("r")
end

function connection_methods:loop(timeout)
	local deadline = timeout and monotime()+timeout
	while not self:empty() do
		local ok, err, errno = self:step(deadline and deadline-monotime())
		if not ok then
			return nil, err, errno
		end
	end
	return true
end

function connection_methods:shutdown()
	local ok, err, errno
	if self.send_goaway_lowest then
		ok = true
	else
		ok, err, errno = self:write_goaway_frame(nil, h2_error.errors.NO_ERROR.code, "connection closed", 0)
		if not ok and errno == ce.EPIPE then
			-- other end already closed
			ok, err, errno = true, nil, nil
		end
	end
	for _, stream in pairs(self.streams) do
		stream:shutdown()
	end
	self.socket:shutdown("r")
	return ok, err, errno
end

function connection_methods:new_stream(id)
	if id and self.streams[id] ~= nil then
		error("stream id already in use")
	end
	local stream = h2_stream.new(self)
	if id then
		stream:pick_id(id)
	end
	return stream
end

-- this function *should never throw*
function connection_methods:get_next_incoming_stream(timeout)
	local deadline = timeout and (monotime()+timeout)
	while self.new_streams:length() == 0 do
		if self.recv_goaway_lowest or self.socket:eof("r") then
			-- TODO? clarification required: can the sender of a GOAWAY subsequently start streams?
			-- (with a lower stream id than they sent in the GOAWAY)
			-- For now, assume not.
			return nil
		end
		local which = cqueues.poll(self.new_streams_cond, self.recv_goaway, self, timeout)
		if which == self then
			local ok, err, errno = self:step(0)
			if not ok then
				return nil, err, errno
			end
		elseif which == timeout then
			return nil, onerror(self.socket, "get_next_incoming_stream", ce.ETIMEDOUT)
		end
		timeout = deadline and (deadline-monotime())
	end

	local stream = self.new_streams:pop()
	return stream
end

-- On success, returns type, flags, stream id and payload
-- If the socket has been shutdown for reading, and there is no data left unread, returns nil
-- safe to retry on error
function connection_methods:read_http2_frame(timeout)
	local deadline = timeout and (monotime()+timeout)
	local frame_header, err, errno = self.socket:xread(9, timeout)
	self.had_eagain = false
	if frame_header == nil then
		if errno == ce.ETIMEDOUT then
			self.had_eagain = true
			return nil, err, errno
		elseif err == nil then
			if self.socket:pending() > 0 then
				self.socket:seterror("r", ce.EILSEQ)
				return nil, onerror(self.socket, "read_http2_frame", ce.EILSEQ)
			end
			return nil
		else
			return nil, err, errno
		end
	end
	local size, typ, flags, streamid = sunpack(">I3 B B I4", frame_header)
	if size > self.acked_settings[known_settings.MAX_FRAME_SIZE] then
		local ok, errno2 = self.socket:unget(frame_header)
		if not ok then
			return nil, onerror(self.socket, "unget", errno2, 2)
		end
		return nil, h2_error.errors.FRAME_SIZE_ERROR:new_traceback("frame too large"), ce.E2BIG
	end
	local payload, err2, errno2 = self.socket:xread(size, 0)
	self.had_eagain = false
	if payload and #payload < size then -- hit EOF
		local ok, errno4 = self.socket:unget(payload)
		if not ok then
			return nil, onerror(self.socket, "unget", errno4, 2)
		end
		payload = nil
	end
	if payload == nil then
		-- put frame header back into socket so a retry will work
		local ok, errno3 = self.socket:unget(frame_header)
		if not ok then
			return nil, onerror(self.socket, "unget", errno3, 2)
		end
		if errno2 == ce.ETIMEDOUT then
			self.had_eagain = true
			timeout = deadline and deadline-monotime()
			if cqueues.poll(self.socket, timeout) ~= timeout then
				return self:read_http2_frame(deadline and deadline-monotime())
			end
		elseif err2 == nil then
			self.socket:seterror("r", ce.EILSEQ)
			return nil, onerror(self.socket, "read_http2_frame", ce.EILSEQ)
		end
		return nil, err2, errno2
	end
	-- reserved bit MUST be ignored by receivers
	streamid = band(streamid, 0x7fffffff)
	return typ, flags, streamid, payload
end

-- If this times out, it was the flushing; not the write itself
-- hence it's not always total failure.
-- It's up to the caller to take some action (e.g. closing) rather than doing it here
function connection_methods:write_http2_frame(typ, flags, streamid, payload, timeout, flush)
	if #payload > self.peer_settings[known_settings.MAX_FRAME_SIZE] then
		return nil, h2_error.errors.FRAME_SIZE_ERROR:new_traceback("frame too large"), ce.E2BIG
	end
	local header = spack(">I3 B B I4", #payload, typ, flags, streamid)
	local ok, err, errno = self.socket:xwrite(header, "f", 0)
	if not ok then
		return nil, err, errno
	end
	return self.socket:xwrite(payload, flush, timeout)
end

function connection_methods:ping(timeout)
	local deadline = timeout and (monotime()+timeout)
	local payload
	-- generate a random, unique payload
	repeat -- keep generating until we don't have a collision
		payload = rand.bytes(8)
	until self.pongs[payload] == nil
	local cond = cc.new()
	self.pongs[payload] = cond
	assert(self.stream0:write_ping_frame(false, payload, timeout))
	while self.pongs[payload] do
		timeout = deadline and (deadline-monotime())
		local which = cqueues.poll(cond, self, timeout)
		if which == self then
			local ok, err, errno = self:step(0)
			if not ok then
				return nil, err, errno
			end
		elseif which == timeout then
			return nil, onerror(self.socket, "ping", ce.ETIMEDOUT)
		end
	end
	return true
end

function connection_methods:write_window_update(...)
	return self.stream0:write_window_update(...)
end

function connection_methods:write_goaway_frame(last_stream_id, err_code, debug_msg, timeout)
	if last_stream_id == nil then
		last_stream_id = math.max(self.highest_odd_stream, self.highest_even_stream)
	end
	return self.stream0:write_goaway_frame(last_stream_id, err_code, debug_msg, timeout)
end

function connection_methods:set_peer_settings(peer_settings)
	--[[ 6.9.2:
	In addition to changing the flow-control window for streams that are
	not yet active, a SETTINGS frame can alter the initial flow-control
	window size for streams with active flow-control windows (that is,
	streams in the "open" or "half-closed (remote)" state).  When the
	value of SETTINGS_INITIAL_WINDOW_SIZE changes, a receiver MUST adjust
	the size of all stream flow-control windows that it maintains by the
	difference between the new value and the old value.

	A change to SETTINGS_INITIAL_WINDOW_SIZE can cause the available
	space in a flow-control window to become negative.  A sender MUST
	track the negative flow-control window and MUST NOT send new flow-
	controlled frames until it receives WINDOW_UPDATE frames that cause
	the flow-control window to become positive.]]
	local new_window_size = peer_settings[known_settings.INITIAL_WINDOW_SIZE]
	if new_window_size then
		local old_windows_size = self.peer_settings[known_settings.INITIAL_WINDOW_SIZE]
		local delta = new_window_size - old_windows_size
		if delta ~= 0 then
			for _, stream in pairs(self.streams) do
				stream.peer_flow_credits = stream.peer_flow_credits + delta
				stream.peer_flow_credits_change:signal()
			end
		end
	end

	merge_settings(self.peer_settings, peer_settings)
	self.peer_settings_cond:signal()
end

function connection_methods:ack_settings()
	local n = self.send_settings_acked + 1
	self.send_settings_acked = n
	local acked_settings = self.send_settings[n]
	if acked_settings then
		self.send_settings[n] = nil
		merge_settings(self.acked_settings, acked_settings)
	end
	self.send_settings_ack_cond:signal()
end

function connection_methods:settings(tbl, timeout)
	local deadline = timeout and monotime()+timeout
	local n, err, errno = self.stream0:write_settings_frame(false, tbl, timeout)
	if not n then
		return nil, err, errno
	end
	-- Now wait for ACK
	while self.send_settings_acked < n do
		timeout = deadline and (deadline-monotime())
		local which = cqueues.poll(self.send_settings_ack_cond, self, timeout)
		if which == self then
			local ok2, err2, errno2 = self:step(0)
			if not ok2 then
				return nil, err2, errno2
			end
		elseif which == timeout then
			self:write_goaway_frame(nil, h2_error.errors.SETTINGS_TIMEOUT.code, "timeout exceeded", 0)
			return nil, onerror(self.socket, "settings", ce.ETIMEDOUT)
		end
	end
	return true
end

return {
	preface = preface;
	socket_has_preface = socket_has_preface;
	new = new_connection;
	methods = connection_methods;
	mt = connection_mt;
}
