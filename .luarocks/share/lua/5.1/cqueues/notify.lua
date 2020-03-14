local loader = function(loader, ...)
	local cqueues = require("cqueues")
	local notify = require("_cqueues.notify")
	local strerror = require("cqueues.errno").strerror
	local ALL = notify.ALL

	local function oops(self, op, why)
		local msg = string.format("notify.%s: %s", op, strerror(why))
		error(msg)
	end

	--
	-- notify:get
	--
	local get; get = notify.interpose("get", function(self, timeout)
		local deadline = timeout and (cqueues.monotime() + timeout)
		local changes, filename
		local ready = function()
			local okay, why = self:step()

			if not okay then
				oops(self, "get", why)
			end

			changes, filename = get(self)

			return changes
		end

		while not ready() do
			if deadline then
				local curtime = cqueues.monotime()
				if curtime >= deadline then
					return nil
				else
					cqueues.poll(self, deadline - curtime)
				end
			else
				cqueues.poll(self)
			end
		end

		return changes, filename
	end)

	--
	-- notify:changes
	--
	notify.interpose("changes", function(self, timeout)
		return function()
			return self:get(timeout)
		end
	end)

	--
	-- notify:add
	--
	local add; add = notify.interpose("add", function(self, name, flags)
		local okay, why = add(self, name, flags or ALL)

		if not okay then
			oops(self, "add", why)
		end

		return true
	end)

	notify.loader = loader

	return notify
end

return loader(loader, ...)
