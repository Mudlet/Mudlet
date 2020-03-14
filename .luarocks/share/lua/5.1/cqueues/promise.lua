local loader = function(loader, ...)
	local cqueues = require"cqueues"
	local condition = require"cqueues.condition"
	local auxlib = require"cqueues.auxlib"
	local assert3 = auxlib.assert3
	local unpack = assert(table.unpack or unpack)
	local pcall = pcall
	local error = error
	local getmetatable = getmetatable
	local tostring = auxlib.tostring

	local promise = {}

	promise.__index = promise -- keep things simple

	function promise.new(f, ...)
		local self = setmetatable({
			pollfd = condition.new(),
			state = "pending",
		}, promise)

		if f then
			cqueues.running():wrap(function(f, ...)
				self:set(pcall(f, ...))
			end, f, ...)
		end

		return self
	end -- promise.new

	function promise.type(self)
		local mt = getmetatable(self)

		return (mt == promise and "promise") or nil
	end -- promise.type

	function promise:set(ok, ...)
		assert3(self.state == "pending", "attempt to set value of resolved promise")

		if not ok then
			self.state = "rejected"
			self.reason = ...
		else
			self.state = "fulfilled"
			self.n = select("#", ...)

			if self.n == 1 then
				self.tuple = ...
			else
				self.tuple = { ... }
			end
		end

		self.pollfd:signal()
		self.timeout = 0
	end -- promise:set

	function promise:wait(timeout)
		if self.state == "pending" then
			self.pollfd:wait(timeout)
		end

		return (self.state ~= "pending" and self) or nil
	end -- promise:wait

	function promise:get(timeout)
		self:wait(timeout)

		if self.state == "fulfilled" then
			if self.n == 1 then
				return self.tuple
			else
				return unpack(self.tuple)
			end
		elseif self.state == "rejected" then
			return error(self.reason, 2)
		end
	end -- promise:get

	function promise:status()
		return self.state
	end -- promise:status

	-- NOTE: Only LuaJIT supports metamethod yielding.
	function promise:__tostring()
		return tostring(self:get())
	end -- promise:__tostring

	function promise:__call()
		return self:get()
	end -- promise:__call

	promise.loader = loader

	return promise
end

return loader(loader, ...)
