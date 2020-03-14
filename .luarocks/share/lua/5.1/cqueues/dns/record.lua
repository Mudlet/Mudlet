local loader = function(loader, ...)
	local record = require"_cqueues.dns.record"

	for k, v in pairs(record.class) do
		if type(k) == "string" then
			record[k] = v
		end
	end

	for k, v in pairs(record.type) do
		if type(k) == "string" then
			record[k] = v
		end
	end

	for k, v in pairs(require"cqueues.dns.packet".section) do
		if type(k) == "string" then
			record[k] = v
		end
	end

	return record
end

return loader(loader, ...)
