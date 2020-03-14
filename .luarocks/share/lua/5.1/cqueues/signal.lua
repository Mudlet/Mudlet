local loader = function(loader, ...)
	local cqueues = require("cqueues")
	local signal = require("_cqueues.signal")

	--
	-- signal:wait
	--
	local wait; wait = signal.interpose("wait", function(self, timeout)
		local deadline = timeout and (cqueues.monotime() + timeout)
		local signo
		local ready = function()
			signo = wait(self)
			return signo
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

		return signo
	end)

	signal.loader = loader

	return signal
end

return loader(loader, ...)
