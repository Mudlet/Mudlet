local loader = function(loader)

local socket = require("_cqueues.socket")
local cqueues = require("cqueues")
local errno = require("cqueues.errno")

local poll = cqueues.poll
local monotime = cqueues.monotime

local AF_INET = socket.AF_INET
local AF_INET6 = socket.AF_INET6
local AF_UNIX = socket.AF_UNIX
local SOCK_STREAM = socket.SOCK_STREAM
local SOCK_DGRAM = socket.SOCK_DGRAM

local EAGAIN = errno.EAGAIN
local EPIPE = errno.EPIPE
local ETIMEDOUT = errno.ETIMEDOUT
local ENOTCONN = errno.ENOTCONN
local ENOTSOCK = errno.ENOTSOCK
local strerror = errno.strerror

local format = string.format


--
-- H E L P E R  R O U T I N E S
--
-- ========================================================================

local function timed_poll(self, deadline)
	if deadline then
		local curtime = monotime()

		if deadline <= curtime then
			return false
		end

		poll(self, deadline - curtime)

		return true
	else
		poll(self)

		return true
	end
end -- timed_poll


local function logname(so)
	local af, addr, port = so:peername()

	if af == AF_INET or af == AF_INET6 then
		return format("%s.%s", addr, port)
	elseif af == AF_UNIX then
		return format("unix:%s", addr or "unnamed")
	end
end -- logname


--
-- E R R O R  M A N A G E M E N T
--
-- All errors in the I/O routines are first passed to a per-socket error
-- handler, which can choose to return or throw them.
--
-- The default error handler is not actually installed with any socket, as
-- that would create needless churn in the registry index on socket
-- instantiation. Instead we interpose socket.onerror and socket:onerror and
-- return our default handler if none was previously installed.
--
-- ========================================================================

-- default error handler
local function def_onerror(self, op, why, lvl)
	if why == EPIPE then
		return EPIPE
	elseif why == ETIMEDOUT then
		return ETIMEDOUT
	else
		local addr = logname(self)
		local msg

		if addr then
			msg = format("[%s]:%s: %s", addr, op, strerror(why))
		else
			msg = format("socket:%s: %s", op, strerror(why))
		end

		error(msg, lvl)
	end
end -- def_onerror


do
	local _onerror = socket.onerror; socket.onerror = function(...)
		return _onerror(...) or def_onerror
	end
end

do
	local _onerror; _onerror = socket.interpose("onerror", function(...)
		return _onerror(...) or def_onerror
	end)
end


--
-- On buffered I/O we need to preserve errors across calls, otherwise
-- unchecked transient errors might lead to unexpected behavior by
-- application code. This is particularly true regarding timeouts, and
-- especially so when mixed with iterators like socket:lines--doubly so when
-- reading MIME headers, which could terminate on ETIMEDOUT, EPIPE, or just
-- when reaching the end of the headers section.
--
-- Why not just always throw on such errors? One reason is that we partially
-- mimic Lua's file objects, which will return such errors. (And we might
-- change our semantics to fully mimic Lua in the future.)
--
-- Another reason is that it's very common to want to deal with timeouts
-- inline. For example, maybe you want to write a keep-alive message after a
-- read timeout. Timeouts are exceptional but not necessarily errors.
--
local preserve = {
	read = "r", lines = "r", fill = "r", unpack = "r",
	write = "w", flush = "w", pack = "w",

	-- these too for good measure, even though they're not buffered
	recvfd = "r", sendfd = "w",
}

-- drop EPIPE errors on input channel
local nopipe = {
	read = true, lines = true, fill = true, unpack = true, recvfd = true
}

local function oops(self, op, why, level)
	local onerror = self:onerror() or def_onerror

	if why == EPIPE and nopipe[op] then
		return -- EOF
	elseif preserve[op] then
		self:seterror(preserve[op], why)
	end

	-- NOTE: There's normally no need to increment on a tail-call
	-- (except when directly calling the error() routine), but we
	-- increment here so the callee has the correct stack level to pass
	-- to error() directly, without making adjustments for its own
	-- activation record.
	return onerror(self, op, why, (level or 2) + 1)
end -- oops


--
-- A P I  E X T E N S I O N S
--
-- The core sockets implementation in C will not yield on I/O, or throw
-- recoverable errors. These things are done in Lua code for simplicitly and
-- portability--Lua 5.1/LuaJIT doesn't support resumption of C routines.
--
-- ========================================================================

--
-- Extended socket.pair
--
local _pair = socket.pair; socket.pair = function(type)
	if type == "stream" then
		type = SOCK_STREAM
	elseif type == "dgram" then
		type = SOCK_DGRAM
	end

	return _pair(type)
end


--
-- Throwable socket:setbufsiz
--
local _setbufsiz; _setbufsiz = socket.interpose("setbufsiz", function(self, input_, output_)
	local input, output, why = _setbufsiz(self, input_, output_)

	if not input then
		return nil, nil, oops(self, "setbufsiz", why)
	end

	return input, output
end)


--
-- Yielding socket:listen
--
local _listen; _listen = socket.interpose("listen", function(self, timeout)
	if not timeout then
		timeout = self:timeout()
	end
	local deadline = timeout and (monotime() + timeout)
	local ok, why = _listen(self)

	while not ok do
		if why == EAGAIN then
			if not timed_poll(self, deadline) then
				return nil, oops(self, "listen", ETIMEDOUT)
			end
		else
			return nil, oops(self, "listen", why)
		end

		ok, why = _listen(self)
	end

	return self
end)


--
-- Yielding socket:accept
--
local _accept; _accept = socket.interpose("accept", function(self, opts, timeout)
	-- :accept used to take just a timeout as argument
	if type(opts) == "number" then
		timeout, opts = opts, nil
	else
		timeout = timeout or self:timeout()
	end
	local deadline = timeout and (monotime() + timeout)
	local con, why = _accept(self, opts)

	while not con do
		if why == EAGAIN then
			if not timed_poll(self, deadline) then
				return nil, oops(self, "accept", ETIMEDOUT)
			end
		else
			return nil, oops(self, "accept", why)
		end

		con, why = _accept(self, opts)
	end

	return con
end)


--
-- Add socket:clients
--
socket.interpose("clients", function(self, opts, timeout)
	return function() return self:accept(opts, timeout) end
end)


--
-- Yielding socket:connect
--
local _connect; _connect = socket.interpose("connect", function(self, timeout)
	if not timeout then
		timeout = self:timeout()
	end
	local deadline = timeout and (monotime() + timeout)
	local ok, why = _connect(self)

	while not ok do
		if why == EAGAIN then
			if not timed_poll(self, deadline) then
				return nil, oops(self, "connect", ETIMEDOUT)
			end
		else
			return nil, oops(self, "connect", why)
		end

		ok, why = _connect(self)
	end

	return self
end)


--
-- Yielding socket:starttls
--
local _starttls; _starttls = socket.interpose("starttls", function(self, arg1, arg2)
	local ctx, timeout

	if type(arg1) == "userdata" then
		ctx = arg1
	elseif type(arg2) == "userdata" then
		ctx = arg2
	end

	if type(arg1) == "number" then
		timeout = arg1
	elseif type(arg2) == "number" then
		timeout = arg2
	else
		timeout = self:timeout()
	end

	local deadline = timeout and monotime() + timeout
	local ok, why = _starttls(self, ctx)

	while not ok do
		if why == EAGAIN then
			if not timed_poll(self, deadline) then
				return nil, oops(self, "starttls", ETIMEDOUT)
			end
		else
			return nil, oops(self, "starttls", why)
		end

		ok, why = _starttls(self, ctx)
	end

	return self
end)


--
-- Smarter socket:checktls
--
local havessl, whynossl

local _checktls; _checktls = socket.interpose("checktls", function(self)
	if not havessl then
		if havessl == false then
			return nil, whynossl
		end

		havessl, whynossl = pcall(require, "openssl.ssl")

		if not havessl then
			return nil, whynossl
		end
	end

	return _checktls(self)
end)


--
-- Yielding socket:flush
--
local _flush;

local function timed_flush(self, mode, timeout, level)
	local ok, why = _flush(self, mode)

	if not ok then
		local deadline = timeout and (monotime() + timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return false, oops(self, "flush", ETIMEDOUT, level + 1)
				end
			else
				return false, oops(self, "flush", why, level + 1)
			end

			ok, why = _flush(self, mode)
		until ok
	end

	return true
end -- timed_flush

_flush = socket.interpose("flush", function (self, arg1, arg2)
	local mode, timeout

	if type(arg1) == "string" then
		mode = arg1
	elseif type(arg2) == "string" then
		mode = arg2
	end

	if type(arg1) == "number" then
		timeout = arg1
	elseif type(arg2) == "number" then
		timeout = arg2
	else
		timeout = self:timeout()
	end

	return timed_flush(self, mode, timeout, 2)
end)


--
-- Yielding socket:read
--
local function read(self, func, what, ...)
	if not what then
		return
	end

	local data, why = self:recv(what)

	if not data then
		local timeout = self:timeout()
		local deadline = timeout and (monotime() + timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return nil, oops(self, func, ETIMEDOUT, 2)
				end
			elseif why then
				return nil, oops(self, func, why, 2)
			else
				return -- EOF or end-of-headers
			end

			data, why = self:recv(what)
		until data
	end

	return data, read(self, func, ...)
end

socket.interpose("read", function(self, what, ...)
	if what then
		return read(self, "read", what, ...)
	else
		return read(self, "read", "*l")
	end
end)


--
-- Yielding socket:write
--
-- This is complicated by the fact that we want error messages to get the
-- correct stack trace, and also because on failure we want to return a list
-- of error values of indeterminate length.
--
local writeall; writeall = function(self, data, ...)
	if not data then
		return self
	end

	data = tostring(data)

	local i = 1

	while i <= #data do
		-- use only full buffering mode here to minimize socket I/O
		local n, why = self:send(data, i, #data, "f")

		i = i + n

		if i <= #data then
			if why == EAGAIN then
				local timeout = self:timeout()
				local deadline = timeout and (monotime() + timeout)

				if not timed_poll(self, deadline) then
					return nil, oops(self, "write", ETIMEDOUT, 3)
				end
			else
				return nil, oops(self, "write", why, 3)
			end
		end
	end

	return writeall(self, ...)
end

local function fileresult(self, ok, ...)
	if ok then
		return self
	else
		return nil, ...
	end
end -- fileresult

local function flushwrite(self, ok, ...)
	if not ok then
		return nil, ...
	end

	-- Flush the buffer here because we used full buffering mode in
	-- writeall. But pass empty mode so it uses the configured flushing
	-- mode instead of an implicit flush all.
	return fileresult(self, timed_flush(self, "", nil, 2))
end -- flushwrite

socket.interpose("write", function (self, ...)
	return flushwrite(self, writeall(self, ...))
end)


--
-- Add socket:lines
--
-- We optimize single-mode case so we're not unpacking tables all the time.
--
local unpack = assert(table.unpack or unpack)

socket.interpose("lines", function (self, mode, ...)
	if mode then
		local n = select("#", ...)
		if n > 0 then
			local args = { ... }

			return function ()
				return read(self, "lines", mode, unpack(args, 1, n))
			end
		end
	else
		mode = "*l"
	end

	return function ()
		return read(self, "lines", mode)
	end
end)


-- returns mode, timeout
local function xopts(arg1, arg2)
	if tonumber(arg1) then
		return arg2, arg1
	else
		return arg1, arg2
	end
end -- xopts


local function xdeadline(self, timeout)
	timeout = timeout or self:timeout()

	return timeout and (monotime() + timeout)
end -- xdeadline


--
-- Smarter socket:read
--
socket.interpose("xread", function (self, what, ...)
	local mode, timeout = xopts(...)

	local data, why = self:recv(what, mode)

	if not data then
		local deadline = xdeadline(self, timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return nil, oops(self, "read", ETIMEDOUT)
				end
			elseif why then
				return nil, oops(self, "read", why)
			else
				return --> EOF
			end

			data, why = self:recv(what, mode)
		until data
	end

	return data
end) -- xread


--
-- Smarter socket:write
--
socket.interpose("xwrite", function (self, data, ...)
	local mode, timeout = xopts(...)
	local i = 1

	--
	-- should we default to full-buffering here (and the :send below) if
	-- mode is nil?
	--
	local n, why = self:send(data, i, #data, mode)

	i = i + n

	if i <= #data then
		local deadline = xdeadline(self, timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return nil, oops(self, "write", ETIMEDOUT)
				end
			else
				return nil, oops(self, "write", why)
			end

			n, why = self:send(data, i, #data, mode)

			i = i + n
		until i > #data

		timeout = deadline and math.max(0, deadline - monotime())
	end

	return fileresult(self, self:flush(mode or "", timeout))
end)


--
-- Smarter socket:lines
--
socket.interpose("xlines", function (self, what, ...)
	local mode, timeout = xopts(...)

	return function ()
		return self:xread(what, mode, timeout)
	end
end)


--
-- Yielding socket:sendfd
--
local _sendfd; _sendfd = socket.interpose("sendfd", function (self, msg, fd, timeout)
	if not timeout then
		timeout = self:timeout()
	end
	local deadline = timeout and (monotime() + timeout)
	local ok, why

	repeat
		ok, why = _sendfd(self, msg, fd)

		if not ok then
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return false, oops(self, "sendfd", ETIMEDOUT)
				end
			else
				return false, oops(self, "sendfd", why)
			end
		end
	until ok

	return ok
end)


--
-- Yielding socket:recvfd
--
local _recvfd; _recvfd = socket.interpose("recvfd", function (self, prepbufsiz, timeout)
	if not timeout then
		timeout = self:timeout()
	end
	local deadline = timeout and (monotime() + timeout)
	local msg, fd, why

	repeat
		msg, fd, why = _recvfd(self, prepbufsiz)

		if not msg then
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return nil, nil, oops(self, "recvfd", ETIMEDOUT)
				end
			else
				return nil, nil, oops(self, "recvfd", why)
			end
		end
	until msg

	return msg, fd
end)


--
-- Yielding socket:pack
--
local _pack; _pack = socket.interpose("pack", function (self, num, nbits, mode)
	local ok, why = _pack(self, num, nbits, mode)

	if not ok then
		local timeout = self:timeout()
		local deadline = timeout and (monotime() + timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return false, oops(self, "pack", ETIMEDOUT)
				end
			else
				return false, oops(self, "pack", why)
			end

			ok, why = _pack(self, num, nbits, mode)
		until ok
	end

	return ok
end)


--
-- Yielding socket:unpack
--
local _unpack; _unpack = socket.interpose("unpack", function (self, nbits)
	local num, why = _unpack(self, nbits)

	if not num then
		local timeout = self:timeout()
		local deadline = timeout and (monotime() + timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return nil, oops(self, "unpack", ETIMEDOUT)
				end
			else
				return nil, oops(self, "unpack", why)
			end

			num, why = _unpack(self, nbits)
		until num
	end

	return num
end)


--
-- Yielding socket:fill
--
local _fill; _fill = socket.interpose("fill", function (self, size, timeout)
	local ok, why = _fill(self, size)

	if not ok then
		if not timeout then
			timeout = self:timeout()
		end
		local deadline = timeout and (monotime() + timeout)

		repeat
			if why == EAGAIN then
				if not timed_poll(self, deadline) then
					return false, oops(self, "fill", ETIMEDOUT)
				end
			else
				return false, oops(self, "fill", why)
			end

			ok, why = _fill(self, size)
		until ok
	end

	return true
end)


--
-- Extend socket:peername
--
local function getname(get, self)
	local af, r1, r2 = get(self)

	if af then
		return af, r1, r2
	elseif r1 == ENOTCONN or r1 == ENOTSOCK or r1 == EAGAIN then
		return 0
	else
		return nil, r1
	end
end

local _peername; _peername = socket.interpose("peername", function (self)
	return getname(_peername, self)
end)


--
-- Extend socket:localname
--
local _localname; _localname = socket.interpose("localname", function (self)
	return getname(_localname, self)
end)


socket.loader = loader

return socket

end -- loader

return loader(loader)
