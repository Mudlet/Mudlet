local loader = function(loader, ...)
	local dns = require"_cqueues.dns"
	local assert = require"cqueues".assert

	--
	-- NOTE: defer loading dns.resolvers as it depends on us
	--
	local pool = nil

	function dns.setpool(p)
		local o = pool

		assert(require"cqueues.dns.resolvers".type(p), "not dns resolver pool")

		pool = p

		return o
	end -- dns.setpool


	function dns.getpool()
		if not pool then
			dns.setpool(assert(require"cqueues.dns.resolvers".stub()))
		end

		return pool
	end -- dns.getpool


	function dns.query(...)
		return dns.getpool():query(...)
	end -- dns.query


	dns.loader = loader

	return dns
end

return loader(loader, ...)
