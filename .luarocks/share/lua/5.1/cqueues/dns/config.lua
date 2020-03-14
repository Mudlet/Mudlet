local loader = function(loader, ...)
	local config = require"_cqueues.dns.config"

	config.loadfile = function (file, syntax)
		local cfg = config.new()

		cfg:loadfile(file, syntax)

		return cfg
	end

	config.loadpath = function (path, syntax)
		local cfg = config.new()

		cfg:loadpath(path, syntax)

		return cfg
	end

	local new = config.new; config.new = function(init)
		local cfg = new()

		if init then
			cfg:set(init)
		end

		return cfg
	end

	local stub = config.stub; config.stub = function(init)
		local cfg = stub()

		if init then
			cfg:set(init)
		end

		return cfg
	end

	local root = config.root; config.root = function(init)
		local cfg = root()

		if init then
			cfg:set(init)
		end

		return cfg
	end

	config.interpose("set", function (self, init)
		if init.nameserver then
			self:setns(init.nameserver)
		end

		if init.search then
			self:setsearch(init.search)
		end

		if init.lookup then
			self:setlookup(init.lookup)
		end

		local opts = init.options or init.opts or { }
		local copy = {
			"edns0", "ndots", "timeout", "attempts",
			"rotate", "recurse", "smart", "tcp"
		}

		for i, k in ipairs(copy) do
			if opts[k] == nil and init[k] ~= nil then
				opts[k] = init[k];
			end
		end

		self:setopts(opts)

		if init.interface then
			self:setiface(init.interface)
		end
	end)

	config.interpose("get", function (self)
		return {
			nameserver = self:getns(),
			search = self:getsearch(),
			lookup = self:getlookup(),
			options = self:getopts(),
			interface = self:getiface(),
		}
	end)

	config.loader = loader

	return config
end

return loader(loader, ...)
