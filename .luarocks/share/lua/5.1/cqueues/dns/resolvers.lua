local loader = function(loader, ...)
	local resolver = require"cqueues.dns.resolver"
	local config = require"cqueues.dns.config"
	local condition = require"cqueues.condition"
	local monotime = require"cqueues".monotime
	local random = require"cqueues.dns".random
	local errno = require"cqueues.errno"
	local ETIMEDOUT = errno.ETIMEDOUT


	local function todeadline(timeout)
		return (timeout and (monotime() + timeout)) or nil
	end -- todeadline

	local function totimeout(deadline)
		return (deadline and math.max(0, deadline - monotime())) or nil
	end -- totimeout


	--
	-- NOTE: Keep track of an unordered collection of objects, and in
	-- particular a count of objects in the collection. If an object is
	-- garbage collected automatically decrement the count and signal
	-- the condition variable.
	--
	local alive = {}

	function alive.new(condvar)
		local self = setmetatable({}, { __index = alive })

		self.n = 0
		self.table = setmetatable({}, { __mode = "k" })
		self.condvar = condvar
		self.hooks = {}
		self.hookmt = { __gc = function (hook)
			self.n = self.n - 1
			self.condvar:signal()

			if self.leakcb then
				pcall(self.leakcb)
			end
		end }

		return self
	end -- alive.new


	function alive:newhook()
		if not self._newhook then
			if _G._VERSION == "Lua 5.1" then
				-- Lua 5.1 does not support __gc on tables, so we need to use newproxy
				self._newhook = function(mt)
					local u = newproxy(false)
					debug.setmetatable(u, mt)
					return u
				end
			else
				self._newhook = function(mt)
					return setmetatable({}, mt)
				end
			end
		end

		return self._newhook(self.hookmt)
	end -- alive:newhook


	function alive:add(x)
		if not self.table[x] then
			local hook = self.hooks[#self.hooks]

			if hook then
				self.hooks[#self.hooks] = nil
			else
				hook = self:newhook()
			end

			self.table[x] = hook
			self.n = self.n + 1
		end
	end -- alive:add


	function alive:delete(x)
		if self.table[x] then
			self.hooks[#self.hooks + 1] = self.table[x]
			self.table[x] = nil
			self.n = self.n - 1
			self.condvar:signal()
		end
	end -- alive:delete


	function alive:check()
		local n = 0

		for _ in pairs(self.table) do
			n = n + 1
		end

		return assert(n == self.n, "resolver registry corrupt")
	end -- alive:check


	function alive:onleak(f)
		local old = self.onleak
		self.leakcb = f
		return old
	end -- alive:onleak


	local pool = {}

	local function getby(self, deadline)
		local res
		while true do
			local cache_len = #self.cache
			if cache_len > 1 then
				res = self.cache[cache_len]
				self.cache[cache_len] = nil
				if res then
					break
				else
					if deadline and deadline <= monotime() then
						return nil, ETIMEDOUT
					end
					self.condvar:wait(totimeout(deadline))
				end
			elseif self.alive.n < self.hiwat then
				local why
				res, why = resolver.new(self.resconf, self.hosts, self.hints)
				if not res then
					return nil, why
				end
				break
			end
		end
		self.alive:add(res)
		return res
	end -- getby


	function pool:get(timeout)
		return getby(self, todeadline(timeout))
	end -- pool:get


	function pool:put(res)
		self.alive:delete(res)

		local cache_len = #self.cache
		if cache_len < self.lowat and res:stat().queries < self.querymax then
			if not self.lifo and cache_len > 0 then
				local i = random(cache_len+1) + 1

				self.cache[cache_len+1] = self.cache[i]
				self.cache[i] = res
			else
				self.cache[cache_len+1] = res
			end
		else
			res:close()
		end
	end -- pool:put


	function pool:signal()
		self.condvar:signal()
	end -- pool:signal


	function pool:query(name, type, class, timeout)
		local deadline = todeadline(timeout or self.timeout)
		local res, why = getby(self, deadline)

		if not res then
			return nil, why
		end

		local r, y = res:query(name, type, class, totimeout(deadline))
		self:put(res)
		if not r then
			return nil, y
		end

		return r
	end -- pool:query


	function pool:check()
		return self.alive:check()
	end -- pool:check


	function pool:onleak(f)
		return self.alive:onleak(f)
	end -- pool:onleak


	local resolvers = {}

	resolvers.lowat = 1
	resolvers.hiwat = 32
	resolvers.querymax = 2048
	resolvers.onleak = nil
	resolvers.lifo = false

	function resolvers.new(resconf, hosts, hints)
		local self = {}

		self.resconf = (type(resconf) == "table" and config.new(resconf)) or resconf
		self.hosts = hosts
		self.hints = hints
		self.condvar = condition.new()
		self.lowat = resolvers.lowat
		self.hiwat = resolvers.hiwat
		self.timeout = resolvers.timeout
		self.querymax = resolvers.querymax
		self.onleak = resolvers.onleak
		self.lifo = resolvers.lifo
		self.cache = {}
		self.alive = alive.new(self.condvar)

		return setmetatable(self, { __index = pool })
	end -- resolvers.new


	function resolvers.stub(cfg)
		return resolvers.new(config.stub(cfg))
	end -- resolvers.stub


	function resolvers.root(cfg)
		return resolvers.new(config.root(cfg))
	end -- resolvers.root


	function resolvers.type(o)
		local mt = getmetatable(o)

		if mt and mt.__index == pool then
			return "dns resolver pool"
		end
	end -- resolvers.type


	resolvers.loader = loader

	return resolvers
end

return loader(loader, ...)
