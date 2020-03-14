## http.cookie

A module for working with cookies.

### `bake(name, value, expiry_time, domain, path, secure_only, http_only, same_site)` <!-- --> {#http.cookie.bake}

Returns a string suitable for use in a `Set-Cookie` header with the passed parameters.


### `parse_cookie(cookie)` <!-- --> {#http.cookie.parse_cookie}

Parses the `Cookie` header contents `cookie`.

Returns a table containing `name` and `value` pairs as strings.


### `parse_cookies(req_headers)` <!-- --> {#http.cookie.parse_cookies}

Parses all `Cookie` headers in the [*http.headers*](#http.headers) object `req_headers`.

Returns a table containing `name` and `value` pairs as strings.


### `parse_setcookie(setcookie)` <!-- --> {#http.cookie.parse_setcookie}

Parses the `Set-Cookie` header contents `setcookie`.

Returns `name`, `value` and `params` where:

  - `name` is a string containing the cookie name
  - `value` is a string containing the cookie value
  - `params` is the a table where the keys are cookie attribute names and values are cookie attribute values


### `new_store()` <!-- --> {#http.cookie.new_store}

Creates a new cookie store.

Cookies are unique for a tuple of domain, path and name;
although multiple cookies with the same name may exist in a request due to overlapping paths or domains.


### `store.psl` <!-- --> {#http.cookie.store.psl}

A [lua-psl](https://github.com/daurnimator/lua-psl) object to use for checking against the Public Suffix List.
Set the field to `false` to skip checking the suffix list.

Defaults to the [latest](https://rockdaboot.github.io/libpsl/libpsl-Public-Suffix-List-functions.html#psl-latest) PSL on the system. If lua-psl is not installed then it will be `nil`.


### `store.time()` <!-- --> {#http.cookie.store.time}

A function used by the `store` to get the current time for expiries and such.

Defaults to a function based on [`os.time`](https://www.lua.org/manual/5.3/manual.html#pdf-os.time).


### `store.max_cookie_length` <!-- --> {#http.cookie.store.max_cookie_length}

The maximum length (in bytes) of cookies in the store; this value is also used as default maximum cookie length for `:lookup()`.
Decreasing this value will only prevent new cookies from being added, it will not remove old cookies.

Defaults to infinity (no maximum size).


### `store.max_cookies` <!-- --> {#http.cookie.store.max_cookies}

The maximum number of cookies allowed in the `store`.
Decreasing this value will only prevent new cookies from being added, it will not remove old cookies.

Defaults to infinity (any number of cookies is allowed).


### `store.max_cookies_per_domain` <!-- --> {#http.cookie.store.max_cookies_per_domain}

The maximum number of cookies allowed in the `store` per domain.
Decreasing this value will only prevent new cookies from being added, it will not remove old cookies.

Defaults to infinity (any number of cookies is allowed).


### `store:store(req_domain, req_path, req_is_http, req_is_secure, req_site_for_cookies, name, value, params)` <!-- --> {#http.cookie.store:store}

Attempts to add a cookie to the `store`.

  - `req_domain` is the domain that the cookie was obtained from
  - `req_path` is the path that the cookie was obtained from
  - `req_is_http` is a boolean flag indicating if the cookie was obtained from a "non-HTTP" API
  - `req_is_secure` is a boolean flag indicating if the cookie was obtained from a "secure" protocol
  - `req_site_for_cookies` is a string containing the host that should be considered as the "site for cookies" (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2)), can be `nil` if unknown.
  - `name` is a string containing the cookie name
  - `value` is a string containing the cookie value
  - `params` is the a table where the keys are cookie attribute names and values are cookie attribute values

Returns a boolean indicating if a cookie was stored.


### `store:store_from_request(req_headers, resp_headers, req_host, req_site_for_cookies)` <!-- --> {#http.cookie.store:store_from_request}

Attempt to store any cookies found in the response headers.

  - `req_headers` is the [*http.headers*](#http.headers) object for the outgoing request
  - `resp_headers` is the [*http.headers*](#http.headers) object received in response
  - `req_host` is the host that your query was directed at (only used if `req_headers` is missing a `Host` header)
  - `req_site_for_cookies` is a string containing the host that should be considered as the "site for cookies" (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2)), can be `nil` if unknown.


### `store:get(domain, path, name)` <!-- --> {#http.cookie.store:get}

Returns the cookie value for the cookie stored for the passed `domain`, `path` and `name`.


### `store:remove(domain, path, name)` <!-- --> {#http.cookie.store:remove}

Deletes the cookie stored for the passed `domain`, `path` and `name`.

If `name` is `nil` or not passed then all cookies for the `domain` and `path` are removed.

If `path` is `nil` or not passed (in addition to `name`) then all cookies for the `domain` are removed.


### `store:lookup(domain, path, is_http, is_secure, is_safe_method, site_for_cookies, is_top_level, max_cookie_length)` <!-- --> {#http.cookie.store:lookup}

Finds cookies visible to suitable for passing to an entity.

  - `domain` is the domain that will be sent the cookie
  - `path` is the path that will be sent the cookie
  - `is_http` is a boolean flag indicating if the destination is a "non-HTTP" API
  - `is_secure` is a boolean flag indicating if the destination will be communicated with over a "secure" protocol
  - `is_safe_method` is a boolean flag indicating if the cookie will be sent via a safe HTTP method (See also [http.util.is_safe_method](#http.util.is_safe_method))
  - `site_for_cookies` is a string containing the host that should be considered as the "site for cookies" (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2)), can be `nil` if unknown.
  - `is_top_level` is a boolean flag indicating if this request is a "top level" request (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2))
  - `max_cookie_length` is the maximum cookie length to allow (See also [`store.max_cookie_length`](#http.cookie.store.max_cookie_length))

Returns a string suitable for use in a `Cookie` header.


### `store:lookup_for_request(headers, host, site_for_cookies, is_top_level, max_cookie_length)` <!-- --> {#http.cookie.store:lookup_for_request}

Finds cookies suitable for adding to a request.

  - `headers` is the [*http.headers*](#http.headers) object for the outgoing request
  - `host` is the host that your query was directed at (only used if `headers` is missing a `Host` header)
  - `site_for_cookies` is a string containing the host that should be considered as the "site for cookies" (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2)), can be `nil` if unknown.
  - `is_top_level` is a boolean flag indicating if this request is a "top level" request (See [RFC 6265bis-02 Section 5.2](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.2))
  - `max_cookie_length` is the maximum cookie length to allow (See also [`store.max_cookie_length`](#http.cookie.store.max_cookie_length))

Returns a string suitable for use in a `Cookie` header.


### `store:clean_due()` <!-- --> {#http.cookie.store:clean_due}

Returns the number of seconds until the next cookie in the `store` expires.


### `store:clean()` <!-- --> {#http.cookie.store:clean}

Remove all expired cookies from the `store`.


### `store:load_from_file(file)` <!-- --> {#http.cookie.store:load_from_file}

Loads cookie data from the file object `file` into `store`.
The file should be in the Netscape Cookiejar format.
Invalid lines in the file are ignored.

Returns `true` on success or passes along `nil, err, errno` if a `:read` call fails.


### `store:save_to_file(file)` <!-- --> {#http.cookie.store:save_to_file}

Writes the cookie data from `store` into the file object `file` in the Netscape Cookiejar format.
`file` is not `seek`-ed or truncated before writing.

Returns `true` on success or passes along `nil, err, errno` if a `:write` call fails.
