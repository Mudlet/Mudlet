## http.headers

An ordered list of header fields.
Each field has a *name*, a *value* and a *never_index* flag that indicates if the header field is potentially sensitive data.

Each headers object has an index by field name to efficiently retrieve values by key. Keep in mind that there can be multiple values for a given field name. (e.g. an HTTP server may send two `Set-Cookie` headers).

As noted in the [Conventions](#conventions) section, HTTP 1 request and status line fields are passed around inside of headers objects under keys `":authority"`, `":method"`, `":path"`, `":scheme"` and `":status"` as defined in HTTP 2. As such, they are all kept in string form (important to remember for the `":status"` field).

### `new()` <!-- --> {#http.headers.new}

Creates and returns a new headers object.


### `headers:len()` <!-- --> {#http.headers:len}

Returns the number of headers.

Also available as `#headers` in Lua 5.2+.


### `headers:clone()` <!-- --> {#http.headers:clone}

Creates and returns a clone of the headers object.


### `headers:append(name, value, never_index)` <!-- --> {#http.headers:append}

Append a header.

  - `name` is the header field name. Lower case is the convention. It will not be validated at this time.
  - `value` is the header field value. It will not be validated at this time.
  - `never_index` is an optional boolean that indicates if the `value` should be considered secret. Defaults to true for header fields: authorization, proxy-authorization, cookie and set-cookie.


### `headers:each()` <!-- --> {#http.headers:each}

An iterator over all headers that emits `name, value, never_index`.

#### Example

```lua
local http_headers = require "http.headers"
local myheaders = http_headers.new()
myheaders:append(":status", "200")
myheaders:append("set-cookie", "foo=bar")
myheaders:append("connection", "close")
myheaders:append("set-cookie", "baz=qux")
for name, value, never_index in myheaders:each() do
	print(name, value, never_index)
end
--[[ prints:
":status", "200", false
"set-cookie", "foo=bar", true
"connection", "close", false
"set-cookie", "baz=qux", true
]]
```


### `headers:has(name)` <!-- --> {#http.headers:has}

Returns a boolean indicating if the headers object has a field with the given `name`.


### `headers:delete(name)` <!-- --> {#http.headers:delete}

Removes all occurrences of a field name from the headers object.


### `headers:geti(i)` <!-- --> {#http.headers:geti}

Return the `i`-th header as `name, value, never_index`


### `headers:get_as_sequence(name)` <!-- --> {#http.headers:get_as_sequence}

Returns all headers with the given name in a table. The table will contain a field `.n` with the number of elements.

#### Example

```lua
local http_headers = require "http.headers"
local myheaders = http_headers.new()
myheaders:append(":status", "200")
myheaders:append("set-cookie", "foo=bar")
myheaders:append("connection", "close")
myheaders:append("set-cookie", "baz=qux")
local mysequence = myheaders:get_as_sequence("set-cookie")
--[[ mysequence will be:
{n = 2; "foo=bar"; "baz=qux"}
]]
```


### `headers:get(name)` <!-- --> {#http.headers:get}

Returns all headers with the given name as multiple return values.


### `headers:get_comma_separated(name)` <!-- --> {#http.headers:get_comma_separated}

Returns all headers with the given name as items in a comma separated string.


### `headers:modifyi(i, value, never_index)` <!-- --> {#http.headers:modifyi}

Change the `i`-th's header to a new `value` and `never_index`.


### `headers:upsert(name, value, never_index)` <!-- --> {#http.headers:upsert}

If a header with the given `name` already exists, replace it. If not, [`append`](#http.headers:append) it to the list of headers.

Cannot be used when a header `name` already has multiple values.


### `headers:sort()` <!-- --> {#http.headers:sort}

Sort the list of headers by their field name, ordering those starting with `:` first. If `name`s are equal then sort by `value`, then by `never_index`.


### `headers:dump(file, prefix)` <!-- --> {#http.headers:dump}

Print the headers list to the given file, one per line.
If `file` is not given, then print to `stderr`.
`prefix` is prefixed to each line.
