--[[
HTTP Header data structure/type

Design criteria:
  - the same header field is allowed more than once
      - must be able to fetch seperate occurences (important for some headers e.g. Set-Cookie)
      - optionally available as comma seperated list
  - http2 adds flag to headers that they should never be indexed
  - header order should be recoverable

I chose to implement headers as an array of entries.
An index of field name => array indices is kept.
]]

local unpack = table.unpack or unpack -- luacheck: ignore 113 143

local entry_methods = {}
local entry_mt = {
	__name = "http.headers.entry";
	__index = entry_methods;
}

local never_index_defaults = {
	authorization = true;
	["proxy-authorization"] = true;
	cookie = true;
	["set-cookie"] = true;
}

local function new_entry(name, value, never_index)
	if never_index == nil then
		never_index = never_index_defaults[name] or false
	end
	return setmetatable({
		name = name;
		value = value;
		never_index = never_index;
	}, entry_mt)
end

function entry_methods:modify(value, never_index)
	self.value = value
	if never_index == nil then
		never_index = never_index_defaults[self.name] or false
	end
	self.never_index = never_index
end

function entry_methods:unpack()
	return self.name, self.value, self.never_index
end

function entry_methods:clone()
	return new_entry(self.name, self.value, self.never_index)
end


local headers_methods = {}
local headers_mt = {
	__name = "http.headers";
	__index = headers_methods;
}

local function new_headers()
	return setmetatable({
		_n = 0;
		_data = {};
		_index = {};
	}, headers_mt)
end

function headers_methods:len()
	return self._n
end
headers_mt.__len = headers_methods.len

function headers_mt:__tostring()
	return string.format("http.headers{%d headers}", self._n)
end

local function add_to_index(_index, name, i)
	local dex = _index[name]
	if dex == nil then
		dex = {n=1, i}
		_index[name] = dex
	else
		local n = dex.n + 1
		dex[n] = i
		dex.n = n
	end
end

local function rebuild_index(self)
	local index = {}
	for i=1, self._n do
		local entry = self._data[i]
		add_to_index(index, entry.name, i)
	end
	self._index = index
end

function headers_methods:clone()
	local index, new_data = {}, {}
	for i=1, self._n do
		local entry = self._data[i]
		new_data[i] = entry:clone()
		add_to_index(index, entry.name, i)
	end
	return setmetatable({
		_n = self._n;
		_data = new_data;
		_index = index;
	}, headers_mt)
end

function headers_methods:append(name, ...)
	local n = self._n + 1
	self._data[n] = new_entry(name, ...)
	add_to_index(self._index, name, n)
	self._n = n
end

function headers_methods:each()
	local i = 0
	return function(self) -- luacheck: ignore 432
		if i >= self._n then return end
		i = i + 1
		local entry = self._data[i]
		return entry:unpack()
	end, self
end
headers_mt.__pairs = headers_methods.each

function headers_methods:has(name)
	local dex = self._index[name]
	return dex ~= nil
end

function headers_methods:delete(name)
	local dex = self._index[name]
	if dex then
		local n = dex.n
		for i=n, 1, -1 do
			table.remove(self._data, dex[i])
		end
		self._n = self._n - n
		rebuild_index(self)
		return true
	else
		return false
	end
end

function headers_methods:geti(i)
	local e = self._data[i]
	if e == nil then return nil end
	return e:unpack()
end

function headers_methods:get_as_sequence(name)
	local dex = self._index[name]
	if dex == nil then return { n = 0; } end
	local r = { n = dex.n; }
	for i=1, r.n do
		r[i] = self._data[dex[i]].value
	end
	return r
end

function headers_methods:get(name)
	local r = self:get_as_sequence(name)
	return unpack(r, 1, r.n)
end

function headers_methods:get_comma_separated(name)
	local r = self:get_as_sequence(name)
	if r.n == 0 then
		return nil
	else
		return table.concat(r, ",", 1, r.n)
	end
end

function headers_methods:modifyi(i, ...)
	local e = self._data[i]
	if e == nil then error("invalid index") end
	e:modify(...)
end

function headers_methods:upsert(name, ...)
	local dex = self._index[name]
	if dex == nil then
		self:append(name, ...)
	else
		assert(dex[2] == nil, "Cannot upsert multi-valued field")
		self:modifyi(dex[1], ...)
	end
end

local function default_cmp(a, b)
	if a.name ~= b.name then
		-- Things with a colon *must* be before others
		local a_is_colon = a.name:sub(1,1) == ":"
		local b_is_colon = b.name:sub(1,1) == ":"
		if a_is_colon and not b_is_colon then
			return true
		elseif not a_is_colon and b_is_colon then
			return false
		else
			return a.name < b.name
		end
	end
	if a.value ~= b.value then
		return a.value < b.value
	end
	return a.never_index
end

function headers_methods:sort()
	table.sort(self._data, default_cmp)
	rebuild_index(self)
end

function headers_methods:dump(file, prefix)
	file = file or io.stderr
	prefix = prefix or ""
	for name, value in self:each() do
		assert(file:write(string.format("%s%s: %s\n", prefix, name, value)))
	end
	assert(file:flush())
end

return {
	new = new_headers;
	methods = headers_methods;
	mt = headers_mt;
}
