## http.util

### `encodeURI(str)` <!-- --> {#http.util.encodeURI}


### `encodeURIComponent(str)` <!-- --> {#http.util.encodeURIComponent}


### `decodeURI(str)` <!-- --> {#http.util.decodeURI}


### `decodeURIComponent(str)` <!-- --> {#http.util.decodeURIComponent}


### `query_args(str)` <!-- --> {#http.util.query_args}

Returns an iterator over the pairs in `str`

#### Example

```lua
local http_util = require "http.util"
for name, value in http_util.query_args("foo=bar&baz=qux") do
	print(name, value)
end
--[[ prints:
"foo", "bar"
"baz", "qux"
]]
```


### `dict_to_query(dict)` <!-- --> {#http.util.dict_to_query}

Converts a dictionary (table with string keys) with string values to an encoded query string.

#### Example

```lua
local http_util = require "http.util"
print(http_util.dict_to_query({foo = "bar"; baz = "qux"})) --> "baz=qux&foo=bar"
```


### `resolve_relative_path(orig_path, relative_path)` <!-- --> {#http.util.resolve_relative_path}


### `is_safe_method(method)` <!-- --> {#http.util.is_safe_method}

Returns a boolean indicating if the passed string `method` is a "safe" method.
See [RFC 7231 section 4.2.1](https://tools.ietf.org/html/rfc7231#section-4.2.1) for more information.


### `is_ip(str)` <!-- --> {#http.util.is_ip}

Returns a boolean indicating if the passed string `str` is a valid IP.


### `scheme_to_port` <!-- --> {#http.util.scheme_to_port}

Map from schemes (as strings) to default ports (as integers).


### `split_authority(authority, scheme)` <!-- --> {#http.util.split_authority}

Splits an `authority` into host and port components.
If the authority has no port component, will attempt to use the default for the `scheme`.

#### Example

```lua
local http_util = require "http.util"
print(http_util.split_authority("localhost:8000", "http")) --> "localhost", 8000
print(http_util.split_authority("example.com", "https")) --> "localhost", 443
```


### `to_authority(host, port, scheme)` <!-- --> {#http.util.to_authority}

Joins the `host` and `port` to create a valid authority component.
Omits the port if it is the default for the `scheme`.


### `imf_date(time)` <!-- --> {#http.util.imf_date}

Returns the time in HTTP preferred date format (See [RFC 7231 section 7.1.1.1](https://tools.ietf.org/html/rfc7231#section-7.1.1.1))

`time` defaults to the current time


### `maybe_quote(str)` <!-- --> {#http.util.maybe_quote}

  - If `str` is a valid `token`, return it as-is.
  - If `str` would be valid as a `quoted-string`, return the quoted version
  - Otherwise, returns `nil`
