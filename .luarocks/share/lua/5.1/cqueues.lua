local loader = function(loader, ...)
	local core = require"_cqueues"
	local errno = require"_cqueues.errno"
	local monotime = core.monotime
	local running = core.running
	local strerror = errno.strerror
	local unpack = assert(table.unpack or unpack) -- 5.1 compat

	-- lazily load auxlib to prevent circular or unused dependencies
	local auxlib = setmetatable({}, { __index = function (t, k)
		local v = require"cqueues.auxlib"[k]
		rawset(t, k, v)
		return v
	end })

	-- load deprecated APIs into shadow table to keep hidden unless used
	local notsupp = {}
	setmetatable(core, { __index = notsupp })

	--
	-- core.poll
	--
	-- Wrap the cqueues yield protocol to support polling across
	-- multilevel resume/yield. Requires explicit or implicit use
	-- (through monkey patching of coroutine.resume and coroutine.wrap)
	-- of auxlib.resume or auxlib.wrap.
	--
	-- Also supports polling from outside of a running event loop using
	-- a cheap hack. NOT RECOMMENDED.
	--
	local _POLL = core._POLL
	local yield = coroutine.yield
	local poller

	function core.poll(...)
		if running() then
			return yield(_POLL, ...)
		else
			local tuple

			poller = poller or auxlib.assert3(core.new())

			poller:wrap(function (...)
				tuple = { core.poll(...) }
			end, ...)

			-- NOTE: must step twice, once to call poll and
			-- again to wake up
			auxlib.assert3(poller:step())
			auxlib.assert3(poller:step())

			return unpack(tuple or {})
		end
	end -- core.poll

	--
	-- core.sleep
	--
	-- Sleep primitive.
	--
	function core.sleep(timeout)
		core.poll(timeout)
	end -- core.sleep

	--
	-- core:step
	--
	-- Wrap the low-level :step interface to make managing event loops
	-- slightly easier.
	--
	local step; step = core.interpose("step", function (self, timeout)
		if core.running() then
			core.poll(self, timeout)

			return step(self, 0.0)
		else
			return step(self, timeout)
		end
	end)
	--
	-- Lua 5.1 doesn't allow continuations in C
	-- so we have to emulate them in lua.
	--
	if _VERSION == "Lua 5.1" then
		local function checkstep(self, ok, ...)
			if ok == "yielded" then
				return checkstep(self, self:step_resume(coroutine.yield(...)))
			else
				return ok, ...
			end
		end
		local step_; step_ = core.interpose("step", function (self, timeout)
			return checkstep(self, step_(self, timeout))
		end)
	end -- core:step

	--
	-- core:loop
	--
	-- Step until an error is encountered.
	--
	local function todeadline(timeout)
		-- special case 0 timeout to avoid monotime call in totimeout
		return timeout and (timeout > 0 and monotime() + timeout or 0) or nil
	end -- todeadline

	local function totimeout(deadline)
		if not deadline then
			return nil, false
		elseif deadline == 0 then
			return 0, true
		else
			local curtime = monotime()

			if curtime < deadline then
				return deadline - curtime, false
			else
				return  0, true
			end
		end
	end -- totimeout

	core.interpose("loop", function (self, timeout)
		local function checkstep(self, deadline, ok, ...)
			local timeout, expired = totimeout(deadline)

			if not ok then
				return false, ...
			elseif expired or self:empty() then
				return true
			else
				return checkstep(self, deadline, self:step(timeout))
			end
		end

		local deadline = todeadline(timeout)
		return checkstep(self, deadline, self:step(timeout))
	end) -- core:loop

	--
	-- core:errors
	--
	-- Return iterator over core:loop.
	--
	core.interpose("errors", function (self, timeout)
		local deadline = todeadline(timeout)

		return function ()
			local timeout = totimeout(deadline)
			return select(2, self:loop(timeout))
		end
	end) -- core:errors

	--
	-- core.assert
	--
	-- DEPRECATED. See auxlib.assert.
	--
	function notsupp.assert(...)
		return auxlib.assert(...)
	end -- notsupp.assert

	--
	-- core.resume
	--
	-- DEPRECATED. See auxlib.resume.
	--
	function notsupp.resume(...)
		return auxlib.resume(...)
	end -- notsupp.resume

	--
	-- core.wrap
	--
	-- DEPRECATED. See auxlib.wrap.
	--
	function notsupp.wrap(...)
		return auxlib.wrap(...)
	end -- notsupp.wrap

	core.loader = loader

	return core
end -- loader

return loader(loader, ...)
