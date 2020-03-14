local loader = function(loader, ...)
	local cqueues = require"cqueues"
	local resolver = require"_cqueues.dns.resolver"
	local config = require"cqueues.dns.config"
	local record = require"cqueues.dns.record"
	local errno = require"cqueues.errno"
	local EAGAIN = errno.EAGAIN
	local ETIMEDOUT = errno.ETIMEDOUT
	local monotime = cqueues.monotime

	local _new = resolver.new; resolver.new = function (resconf, hosts, hints)
		if type(resconf) == "table" then
			resconf = config.new(resconf)
		end

		return _new(resconf, hosts, hints)
	end

	resolver.stub = function (init)
		return resolver.new(config.stub(init), nil, nil)
	end

	resolver.root = function (init)
		return resolver.new(config.root(init), nil, nil)
	end

	local function toconst(id, map, what, lvl)
		local n

		if id == nil then
			return
		elseif type(id) == "number" then
			n = map[id] and id
		elseif type(id) == "string" then
			n = map[id] or map[string.upper(id)]
		end

		if not n then
			error((tostring(id) .. ": unknown DNS " .. what), lvl + 1)
		end

		return n
	end -- toconst

	local _submit; _submit = resolver.interpose("submit", function (self, name, type, class)
		type = toconst(type, record.type, "type", 2)
		class = toconst(class, record.class, "class", 2)

		return _submit(self, name, type, class)
	end)

	resolver.interpose("query", function (self, name, type, class, timeout)
		local deadline = timeout and (monotime() + timeout)
		local ok, why, answer

		ok, why = self:submit(name, type, class)

		if not ok then
			return nil, why
		end

		repeat
			answer, why = self:fetch()

			if not answer then
				if why == EAGAIN then
					if deadline then
						local curtime = monotime()

						if deadline <= curtime then
							return nil, ETIMEDOUT
						else
							cqueues.poll(self, math.min(deadline - curtime, 1))
						end
					else
						cqueues.poll(self, 1)
					end
				else
					return nil, why
				end
			end
		until answer

		return answer
	end)

	resolver.loader = loader

	return resolver
end

return loader(loader, ...)
