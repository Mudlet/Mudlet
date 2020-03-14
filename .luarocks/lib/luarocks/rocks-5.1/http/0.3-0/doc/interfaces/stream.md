## stream

An HTTP *stream* is an abstraction of a request/response within a HTTP connection. Within a stream there may be a number of "header" blocks as well as data known as the "body".

All stream types expose the following fields and functions:

### `stream.connection` <!-- --> {#stream.connection}

The underlying [*connection*](#connection) object.


### `stream:checktls()` <!-- --> {#stream:checktls}

Convenience wrapper equivalent to [`stream.connection:checktls()`](#connection:checktls)


### `stream:localname()` <!-- --> {#stream:localname}

Convenience wrapper equivalent to [`stream.connection:localname()`](#connection:localname)


### `stream:peername()` <!-- --> {#stream:peername}

Convenience wrapper equivalent to [`stream.connection:peername()`](#connection:peername)


### `stream:get_headers(timeout)` <!-- --> {#stream:get_headers}

Retrieves the next complete headers object (i.e. a block of headers or trailers) from the stream.


### `stream:write_headers(headers, end_stream, timeout)` <!-- --> {#stream:write_headers}

Write the given [*headers*](#http.headers) object to the stream. The function takes a flag indicating if this is the last chunk in the stream, if `true` the stream will be closed. If `timeout` is specified, the stream will wait for the send to complete until `timeout` is exceeded.


### `stream:write_continue(timeout)` <!-- --> {#stream:write_continue}

Sends a 100-continue header block.


### `stream:get_next_chunk(timeout)` <!-- --> {#stream:get_next_chunk}

Returns the next chunk of the http body from the socket, potentially yielding for up to `timeout` seconds. On error, returns `nil`, an error message and an error number.


### `stream:each_chunk()` <!-- --> {#stream:each_chunk}

Iterator over [`stream:get_next_chunk()`](#stream:get_next_chunk)


### `stream:get_body_as_string(timeout)` <!-- --> {#stream:get_body_as_string}

Reads the entire body from the stream and return it as a string. On error, returns `nil`, an error message and an error number.


### `stream:get_body_chars(n, timeout)` <!-- --> {#stream:get_body_chars}

Reads `n` characters (bytes) of body from the stream and return them as a string. If the stream ends before `n` characters are read then returns the partial result. On error, returns `nil`, an error message and an error number.


### `stream:get_body_until(pattern, plain, include_pattern, timeout)` <!-- --> {#stream:get_body_until}

Reads in body data from the stream until the [lua pattern](http://www.lua.org/manual/5.3/manual.html#6.4.1) `pattern` is found and returns the data as a string. `plain` is a boolean that indicates that pattern matching facilities should be turned off so that function does a plain "find substring" operation, with no characters in pattern being considered magic. `include_patterns` specifies if the pattern itself should be included in the returned string. On error, returns `nil`, an error message and an error number.


### `stream:save_body_to_file(file, timeout)` <!-- --> {#stream:save_body_to_file}

Reads the body from the stream and saves it to the [lua file handle](http://www.lua.org/manual/5.3/manual.html#6.8) `file`. On error, returns `nil`, an error message and an error number.


### `stream:get_body_as_file(timeout)` <!-- --> {#stream:get_body_as_file}

Reads the body from the stream into a temporary file and returns a [lua file handle](http://www.lua.org/manual/5.3/manual.html#6.8). On error, returns `nil`, an error message and an error number.


### `stream:unget(str)` <!-- --> {#stream:unget}

Places `str` back on the incoming data buffer, allowing it to be returned again on a subsequent command ("un-gets" the data). Returns `true` on success. On error, returns `nil`, an error message and an error number.


### `stream:write_chunk(chunk, end_stream, timeout)` <!-- --> {#stream:write_chunk}

Writes the string `chunk` to the stream. If `end_stream` is true, the body will be finalized and the stream will be closed. `write_chunk` yields indefinitely, or until `timeout` is exceeded. On error, returns `nil`, an error message and an error number.


### `stream:write_body_from_string(str, timeout)` <!-- --> {#stream:write_body_from_string}

Writes the string `str` to the stream and ends the stream. On error, returns `nil`, an error message and an error number.


### `stream:write_body_from_file(options|file, timeout)` <!-- --> {#stream:write_body_from_file}

  - `options` is a table containing:
	- `.file` (file)
	- `.count` (positive integer): number of bytes of `file` to write  
	  defaults to infinity (the whole file will be written)

Writes the contents of file `file` to the stream and ends the stream. `file` will not be automatically seeked, so ensure it is at the correct offset before calling. On error, returns `nil`, an error message and an error number.


### `stream:shutdown()` <!-- --> {#stream:shutdown}

Closes the stream. The resources are released and the stream can no longer be used.
