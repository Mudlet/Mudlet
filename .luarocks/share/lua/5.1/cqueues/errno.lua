local loader = function(loader, ...)
	local errno = require("_cqueues.errno")

	errno.loader = loader

	return errno
end

return loader(loader, ...)
