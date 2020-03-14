local loader = function(loader, ...)
	local thread = require"_cqueues.thread"

	--
	-- thread.start
	--
	local cache = {}

	local function dump(fn)
		if type(fn) == "string" then
			return fn
		else
			if not cache[fn] then
				cache[fn] = string.dump(fn)
			end

			return cache[fn]
		end
	end

	local include = {
		"cqueues",
		"cqueues.errno",
		"cqueues.socket",
		"cqueues.signal",
		"cqueues.thread",
		"cqueues.notify",
	}

	local start = thread.start; thread.start = function(enter, ...)
		local function init(self, pipe, nloaders, ...)
			local function loadblob(chunk, source, ...)
				if _VERSION == "Lua 5.1" then
					return loadstring(chunk, source)
				else
					return load(chunk, source, ...)
				end
			end

			local function preload(name, code)
				local loader = loadblob(code, nil, "bt", _ENV)
				package.loaded[name] = loader(loader, name)
			end

			local function unpack(n, ...)
				if n > 0 then
					local name, code = select(1, ...)
					preload(name, code)
					return unpack(n - 1, select(3, ...))
				else
					return ...
				end
			end

			nloaders = tonumber(nloaders)

			local enter = unpack(nloaders, ...)

			return enter(pipe, select(nloaders * 2 + 2, ...))
		end

		local function pack(i, enter, ...)
			if i == 1 then
				return init, #include, pack(i + 1, enter, ...)
			elseif include[i - 1] then
				return include[i - 1], dump(require(include[i - 1]).loader), pack(i + 1, enter, ...)
			else
				return enter, ...
			end
		end

		return start(pack(1, enter, ...))
	end


	--
	-- thread:join
	--
	local monotime = require"cqueues".monotime
	local poll = require"cqueues".poll
	local EAGAIN = require"cqueues.errno".EAGAIN
	local ETIMEDOUT = require"cqueues.errno".ETIMEDOUT

	local join; join = thread.interpose("join", function (self, timeout)
		local deadline = timeout and (monotime() + timeout)

		while true do
			local ok, why = join(self)

			if ok then
				return true, why
			elseif why ~= EAGAIN then
				return false, why
			else
				if deadline then
					local curtime = monotime()

					if curtime >= deadline then
						return false, ETIMEDOUT
					else
						poll(self, deadline - curtime)
					end
				else
					poll(self)
				end
			end
		end
	end)


	thread.loader = loader

	return thread
end -- loader

return loader(loader, ...)
