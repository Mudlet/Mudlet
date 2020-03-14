local loader = function(loader, ...)
	local cqueues = require"_cqueues"
	local errno = require"_cqueues.errno"
	local auxlib = require"_cqueues.auxlib"
	local coroutine = require"coroutine"

	--
	-- auxlib.tostring
	--
	-- Yieldable tostring. Implemented in C for Lua 5.2 and above.
	--
	-- Lua 5.1 API doesn't allow us to implement a yieldable tostring
	-- routine. Fortunately, LuaJIT's tostring permits yielding.
	--
	auxlib.tostring = auxlib.tostring or tostring

	--
	-- auxlib.resume
	-- auxlib.wrap
	--
	-- Wrappers for multilevel coroutine management to allow I/O polling
	-- of coroutine-wrapped code. The code checks for a special value
	-- returned by cqueues.poll, and will propogate a yield on I/O.
	-- Everything else should behave as usual.
	--
	local _POLL = cqueues._POLL -- magic internal value
	local l_resume = coroutine.resume -- take reference to permit monkey patching
	local l_yield = coroutine.yield

	local function c_resume(co, ok, arg1, ...)
		if ok and arg1 == _POLL then
			return auxlib.resume(co, l_yield(_POLL, ...))
		else
			return ok, arg1, ...
		end
	end -- c_resume

	function auxlib.resume(co, ...)
		return c_resume(co, l_resume(co, ...))
	end -- auxlib.resume

	local function c_wrap(co, ok, ...)
		if ok then
			return ...
		else
			error((...), 0)
		end
	end -- c_wrap

	function auxlib.wrap(f)
		local co = coroutine.create(f)

		return function(...)
			return c_wrap(co, c_resume(co, l_resume(co, ...)))
		end
	end -- auxlib.wrap

	--
	-- auxlib.assert
	--
	-- If condition is false, locate and use the first non-false,
	-- non-nil value as the reason. If an integer value, attempt to
	-- convert to string using strerror.
	--
	-- RATIONALE: Many routines return only an integer error number.
	-- Errors like EAGAIN are very common and constantly pushing a new
	-- string on the stack from C would be inefficient.
	--
	-- Also, unlike the standard Lua idiom, the failure mode for some
	-- routines will return multiple false or nil values preceding the
	-- error number so user code doesn't need to use variable names like
	-- "value_or_error" for routines which can be expected to fail
	-- regularly in the normal course of operation and where simply
	-- using the assert idiom would create spaghetti code.
	--
	local tostring = auxlib.tostring

	local function findwhy(v, ...)
		if v then
			if type(v) == "number" then
				-- return string and number for auxlib.fileresult
				return (errno.strerror(v) or tostring(v)), v
			else
				return tostring(v), ...
			end
		elseif select("#", ...) > 0 then
			return findwhy(...)
		else
			return
		end
	end

	function auxlib.assert(c, ...)
		if c then
			return c, ...
		end

		return error(findwhy(...), 2)
	end -- auxlib.assert

	--
	-- auxlib.assert[23456789]
	--
	-- Like auxlib.assert, but use error(message, level) where level is
	-- the numeric suffix of the assert function name.
	--
	local function makeassert(level)
		return function (c, ...)
			if c then
				return c, ...
			end

			return error(findwhy(...), level)
		end
	end

	for n=2,9 do
		auxlib[string.format("assert%d", n)] = makeassert(n)
	end

	--
	-- auxlib.fileresult
	--
	function auxlib.fileresult(c, ...)
		if c then
			return c, ...
		else
			return c, findwhy(...)
		end
	end -- auxlib.fileresult

	auxlib.loader = loader

	return auxlib
end -- loader

return loader(loader, ...)
