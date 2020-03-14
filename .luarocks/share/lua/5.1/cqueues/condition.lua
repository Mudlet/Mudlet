local loader = function(loader, ...)
	local core = require"_cqueues.condition"

	local function check(self, why, ...)
		if self == why then
			return true, ...
		else
			return false, why, ...
		end
	end

	local wait; wait = core.interpose("wait", function (self, ...)
		return check(self, wait(self, ...))
	end)

	return core
end

return loader(loader, ...)
