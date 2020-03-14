local loader = function(loader, ...)
	local hosts = require"_cqueues.dns.hosts"

	hosts.loadfile = function (file)
		local hosts = hosts.new()

		hosts:loadfile(file)

		return hosts
	end

	hosts.loadpath = function (path)
		local hosts = hosts.new()

		hosts:loadpath(path)

		return hosts
	end

	hosts.stub = function ()
		return hosts.loadpath"/etc/hosts"
	end

	hosts.root = function ()
		return hosts.new()
	end

	return hosts
end

return loader(loader, ...)
