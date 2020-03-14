-- Two different lua libraries claim the require string "zlib":
-- lua-zlib and lzlib.
-- They have very different APIs, but both provide the raw functionality we need.
-- This module serves to normalise them to a single API

local zlib = require "zlib"

local _M = {}

if zlib._VERSION:match "^lua%-zlib" then
	_M.engine = "lua-zlib"

	function _M.inflate()
		local stream = zlib.inflate()
		local end_of_gzip = false
		return function(chunk, end_stream)
			-- at end of file, end_of_gzip should have been set on the previous iteration
			assert(not end_of_gzip, "stream closed")
			chunk, end_of_gzip = stream(chunk)
			if end_stream then
				assert(end_of_gzip, "invalid stream")
			end
			return chunk
		end
	end

	function _M.deflate()
		local stream = zlib.deflate()
		return function(chunk, end_stream)
			local deflated = stream(chunk, end_stream and "finish" or "sync")
			return deflated
		end
	end
elseif zlib._VERSION:match "^lzlib" then
	_M.engine = "lzlib"

	function _M.inflate()
		-- the function may get called multiple times
		local tmp
		local stream = zlib.inflate(function()
			local chunk = tmp
			tmp = nil
			return chunk
		end)
		return function(chunk, end_stream)
			-- lzlib doesn't report end of string
			tmp = chunk
			local data = assert(stream:read("*a"))
			if end_stream then
				stream:close()
			end
			return data
		end
	end

	function _M.deflate()
		local buf, n = {}, 0
		local stream = zlib.deflate(function(chunk)
			n = n + 1
			buf[n] = chunk
		end)
		return function(chunk, end_stream)
			stream:write(chunk)
			stream:flush()
			if end_stream then
				-- close performs a "finish" flush
				stream:close()
			end
			if n == 0 then
				return ""
			else
				local s = table.concat(buf, "", 1, n)
				buf, n = {}, 0
				return s
			end
		end
	end
else
	error("unknown zlib library")
end

return _M
