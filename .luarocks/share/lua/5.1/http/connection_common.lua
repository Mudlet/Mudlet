local cqueues = require "cqueues"
local ca = require "cqueues.auxlib"
local ce = require "cqueues.errno"

local connection_methods = {}

local function onerror(socket, op, why, lvl) -- luacheck: ignore 212
	local err = string.format("%s: %s", op, ce.strerror(why))
	if op == "starttls" then
		local ssl = socket:checktls()
		if ssl and ssl.getVerifyResult then
			local code, msg = ssl:getVerifyResult()
			if code ~= 0 then
				err = err .. ":" .. msg
			end
		end
	end
	if why == ce.ETIMEDOUT then
		if op == "fill" or op == "read" then
			socket:clearerr("r")
		elseif op == "flush" then
			socket:clearerr("w")
		end
	end
	return err, why
end

function connection_methods:pollfd()
	if self.socket == nil then
		return nil
	end
	return self.socket:pollfd()
end

function connection_methods:events()
	if self.socket == nil then
		return nil
	end
	return self.socket:events()
end

function connection_methods:timeout()
	if self.socket == nil then
		return nil
	end
	return self.socket:timeout()
end

function connection_methods:onidle_() -- luacheck: ignore 212
end

function connection_methods:onidle(...)
	local old_handler = self.onidle_
	if select("#", ...) > 0 then
		self.onidle_ = ...
	end
	return old_handler
end

function connection_methods:connect(timeout)
	if self.socket == nil then
		return nil
	end
	local ok, err, errno = self.socket:connect(timeout)
	if not ok then
		return nil, err, errno
	end
	return true
end

function connection_methods:checktls()
	if self.socket == nil then
		return nil
	end
	return self.socket:checktls()
end

function connection_methods:localname()
	if self.socket == nil then
		return nil
	end
	return ca.fileresult(self.socket:localname())
end

function connection_methods:peername()
	if self.socket == nil then
		return nil
	end
	return ca.fileresult(self.socket:peername())
end

-- Primarily used for testing
function connection_methods:flush(timeout)
	return self.socket:flush("n", timeout)
end

function connection_methods:close()
	self:shutdown()
	if self.socket then
		cqueues.poll()
		cqueues.poll()
		self.socket:close()
	end
	return true
end

return {
	onerror = onerror;
	methods = connection_methods;
}
