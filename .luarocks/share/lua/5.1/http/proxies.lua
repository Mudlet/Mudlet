-- Proxy from e.g. environmental variables.

local proxies_methods = {}
local proxies_mt = {
	__name = "http.proxies";
	__index = proxies_methods;
}

local function new()
	return setmetatable({
		http_proxy = nil;
		https_proxy = nil;
		all_proxy = nil;
		no_proxy = nil;
	}, proxies_mt)
end

function proxies_methods:update(getenv)
	if getenv == nil then
		getenv = os.getenv
	end
	-- prefers lower case over upper case; except for http_proxy where no upper case
	if getenv "GATEWAY_INTERFACE" then -- Mitigate httpoxy. see https://httpoxy.org/
		self.http_proxy = getenv "CGI_HTTP_PROXY"
	else
		self.http_proxy = getenv "http_proxy"
	end
	self.https_proxy = getenv "https_proxy" or getenv "HTTPS_PROXY";
	self.all_proxy = getenv "all_proxy" or getenv "ALL_PROXY";
	self.no_proxy = getenv "no_proxy" or getenv "NO_PROXY";
	return self
end

-- Finds the correct proxy for a given scheme/host
function proxies_methods:choose(scheme, host)
	if self.no_proxy == "*" then
		return nil
	elseif self.no_proxy then
		-- cache no_proxy_set by overwriting self.no_proxy
		if type(self.no_proxy) == "string" then
			local no_proxy_set = {}
			-- wget allows domains in no_proxy list to be prefixed by "."
			-- e.g. no_proxy=.mit.edu
			for host_suffix in self.no_proxy:gmatch("%.?([^,]+)") do
				no_proxy_set[host_suffix] = true
			end
			self.no_proxy = no_proxy_set
		end
		-- From curl docs:
		-- matched as either a domain which contains the hostname, or the
		-- hostname itself. For example local.com would match local.com,
		-- local.com:80, and www.local.com, but not www.notlocal.com.
		for pos in host:gmatch("%f[^%z%.]()") do
			local host_suffix = host:sub(pos, -1)
			if self.no_proxy[host_suffix] then
				return nil
			end
		end
	end
	if scheme == "http" or scheme == "ws" then
		if self.http_proxy then
			return self.http_proxy
		end
	elseif scheme == "https" or scheme == "wss" then
		if self.https_proxy then
			return self.https_proxy
		end
	end
	return self.all_proxy
end

return {
	new = new;
	methods = proxies_methods;
	mt = proxies_mt;
}
