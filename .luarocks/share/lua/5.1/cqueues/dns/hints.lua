local loader = function(loader, ...)
	local hints = require"_cqueues.dns.hints"

	return hints
end

return loader(loader, ...)
