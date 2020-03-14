--[[
Data structures useful for HSTS (HTTP Strict Transport Security)
HSTS is described in RFC 6797
]]

local binaryheap = require "binaryheap"
local http_util = require "http.util"

local store_methods = {
	time = function() return os.time() end;
	max_items = (1e999);
}

local store_mt = {
	__name = "http.hsts.store";
	__index = store_methods;
}

local store_item_methods = {}
local store_item_mt = {
	__name = "http.hsts.store_item";
	__index = store_item_methods;
}

local function new_store()
	return setmetatable({
		domains = {};
		expiry_heap = binaryheap.minUnique();
		n_items = 0;
	}, store_mt)
end

function store_methods:clone()
	local r = new_store()
	r.time = rawget(self, "time")
	r.n_items = rawget(self, "n_items")
	r.expiry_heap = binaryheap.minUnique()
	for host, item in pairs(self.domains) do
		r.domains[host] = item
		r.expiry_heap:insert(item.expires, item)
	end
	return r
end

function store_methods:store(host, directives)
	local now = self.time()
	local max_age = directives["max-age"]
	if max_age == nil then
		return nil, "max-age directive is required"
	elseif type(max_age) ~= "string" or max_age:match("[^0-9]") then
		return nil, "max-age directive does not match grammar"
	else
		max_age = tonumber(max_age, 10)
	end

	-- Clean now so that we can assume there are no expired items in store
	self:clean()

	if max_age == 0 then
		return self:remove(host)
	else
		if http_util.is_ip(host) then
			return false
		end
		-- add to store
		local old_item = self.domains[host]
		if old_item then
			self.expiry_heap:remove(old_item)
		else
			local n_items = self.n_items
			if n_items >= self.max_items then
				return false
			end
			self.n_items = n_items + 1
		end
		local expires = now + max_age
		local item = setmetatable({
			host = host;
			includeSubdomains = directives.includeSubdomains;
			expires = expires;
		}, store_item_mt)
		self.domains[host] = item
		self.expiry_heap:insert(expires, item)
	end
	return true
end

function store_methods:remove(host)
	local item = self.domains[host]
	if item then
		self.expiry_heap:remove(item)
		self.domains[host] = nil
		self.n_items = self.n_items - 1
	end
	return true
end

function store_methods:check(host)
	if http_util.is_ip(host) then
		return false
	end

	-- Clean now so that we can assume there are no expired items in store
	self:clean()

	local h = host
	repeat
		local item = self.domains[h]
		if item then
			if host == h or item.includeSubdomains then
				return true
			end
		end
		local n
		h, n = h:gsub("^[^%.]+%.", "", 1)
	until n == 0
	return false
end

function store_methods:clean_due()
	local next_expiring = self.expiry_heap:peek()
	if not next_expiring then
		return (1e999)
	end
	return next_expiring.expires
end

function store_methods:clean()
	local now = self.time()
	while self:clean_due() < now do
		local item = self.expiry_heap:pop()
		self.domains[item.host] = nil
		self.n_items = self.n_items - 1
	end
	return true
end

return {
	new_store = new_store;
	store_mt = store_mt;
	store_methods = store_methods;
}
