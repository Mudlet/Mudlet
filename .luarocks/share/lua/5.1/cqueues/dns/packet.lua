local loader = function(loader, ...)
	local packet = require"_cqueues.dns.packet"
	local record = require"_cqueues.dns.record" -- dns.record depends on dns.packet

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


	local _push; _push = packet.interpose("push", function (self, section, name, type, class, ttl, rdata)
		section = toconst(section, packet.section, "section", 2)
		type = toconst(type, record.type, "type", 2)
		class = toconst(class, record.class, "class", 2)

		return _push(self, section, name, type, class, ttl, rdata)
	end) -- packet:push


	--
	-- TODO: Don't restrict ourselves to the C iteration interface,
	-- which is limited by fixed-sized fields. For example, you cannot
	-- specify multiple different record types because the type
	-- identifiers cannot be ORd together.
	--
	local _grep; _grep = packet.interpose("grep", function (self, opts)
		if opts then
			opts.type = toconst(opts.type, record.type, "type", 2)
			opts.class = toconst(opts.class, record.class, "class", 2)

			if type(opts.section) == "string" then
				local n = 0

				for s in string.gmatch(opts.section, "%a+") do
					n = n + toconst(s, packet.section, "section", 2)
				end

				opts.section = n
			elseif type(opts.section) == "table" then
				local n = 0

				for i=1,#opts.section do
					n = n + toconst(opts.section[i], packet.section, "section", 2)
				end

				opts.section = n
			end
		end

		return _grep(self, opts)
	end) -- packet:grep


	return packet
end

return loader(loader, ...)
