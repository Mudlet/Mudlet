## http.proxies

### `new()` <!-- --> {#http.proxies.new}

Returns an empty 'proxies' object


### `proxies:update(getenv)` <!-- --> {#http.proxies:update}

`getenv` defaults to [`os.getenv`](http://www.lua.org/manual/5.3/manual.html#pdf-os.getenv)

Reads environmental variables that are used to control if requests go through a proxy.

  - `http_proxy` (or `CGI_HTTP_PROXY` if running in a program with `GATEWAY_INTERFACE` set): the proxy to use for normal HTTP connections
  - `https_proxy` or `HTTPS_PROXY`: the proxy to use for HTTPS connections
  - `all_proxy` or `ALL_PROXY`: the proxy to use for **all** connections, overridden by other options
  - `no_proxy` or `NO_PROXY`: a list of hosts to **not** use a proxy for

Returns `proxies`.


### `proxies:choose(scheme, host)` <!-- --> {#http.proxies:choose}

Returns the proxy to use for the given `scheme` and `host` as a URI.
