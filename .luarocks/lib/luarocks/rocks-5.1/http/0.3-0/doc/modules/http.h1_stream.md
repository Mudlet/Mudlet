## http.h1_stream

The *h1_stream* module adheres to the [*stream*](#stream) interface and provides HTTP 1.x specific operations.

The gzip transfer encoding is supported transparently.

### `h1_stream.connection` <!-- --> {#http.h1_stream.connection}

See [`stream.connection`](#stream.connection)


### `h1_stream.max_header_lines` <!-- --> {#http.h1_stream.max_header_lines}

The maximum number of header lines to read. Default is `100`.


### `h1_stream:checktls()` <!-- --> {#http.h1_stream:checktls}

See [`stream:checktls()`](#stream:checktls)


### `h1_stream:localname()` <!-- --> {#http.h1_stream:localname}

See [`stream:localname()`](#stream:localname)


### `h1_stream:peername()` <!-- --> {#http.h1_stream:peername}

See [`stream:peername()`](#stream:peername)


### `h1_stream:get_headers(timeout)` <!-- --> {#http.h1_stream:get_headers}

See [`stream:get_headers(timeout)`](#stream:get_headers)


### `h1_stream:write_headers(headers, end_stream, timeout)` <!-- --> {#http.h1_stream:write_headers}

See [`stream:write_headers(headers, end_stream, timeout)`](#stream:write_headers)


### `h1_stream:write_continue(timeout)` <!-- --> {#http.h1_stream:write_continue}

See [`stream:write_continue(timeout)`](#stream:write_continue)


### `h1_stream:get_next_chunk(timeout)` <!-- --> {#http.h1_stream:get_next_chunk}

See [`stream:get_next_chunk(timeout)`](#stream:get_next_chunk)


### `h1_stream:each_chunk()` <!-- --> {#http.h1_stream:each_chunk}

See [`stream:each_chunk()`](#stream:each_chunk)


### `h1_stream:get_body_as_string(timeout)` <!-- --> {#http.h1_stream:get_body_as_string}

See [`stream:get_body_as_string(timeout)`](#stream:get_body_as_string)


### `h1_stream:get_body_chars(n, timeout)` <!-- --> {#http.h1_stream:get_body_chars}

See [`stream:get_body_chars(n, timeout)`](#stream:get_body_chars)


### `h1_stream:get_body_until(pattern, plain, include_pattern, timeout)` <!-- --> {#http.h1_stream:get_body_until}

See [`stream:get_body_until(pattern, plain, include_pattern, timeout)`](#stream:get_body_until)


### `h1_stream:save_body_to_file(file, timeout)` <!-- --> {#http.h1_stream:save_body_to_file}

See [`stream:save_body_to_file(file, timeout)`](#stream:save_body_to_file)


### `h1_stream:get_body_as_file(timeout)` <!-- --> {#http.h1_stream:get_body_as_file}

See [`stream:get_body_as_file(timeout)`](#stream:get_body_as_file)


### `h1_stream:unget(str)` <!-- --> {#http.h1_stream:unget}

See [`stream:unget(str)`](#stream:unget)


### `h1_stream:write_chunk(chunk, end_stream, timeout)` <!-- --> {#http.h1_stream:write_chunk}

See [`stream:write_chunk(chunk, end_stream, timeout)`](#stream:write_chunk)


### `h1_stream:write_body_from_string(str, timeout)` <!-- --> {#http.h1_stream:write_body_from_string}

See [`stream:write_body_from_string(str, timeout)`](#stream:write_body_from_string)


### `h1_stream:write_body_from_file(options|file, timeout)` <!-- --> {#http.h1_stream:write_body_from_file}

See [`stream:write_body_from_file(options|file, timeout)`](#stream:write_body_from_file)


### `h1_stream:shutdown()` <!-- --> {#http.h1_stream:shutdown}

See [`stream:shutdown()`](#stream:shutdown)


### `h1_stream:set_state(new)` <!-- --> {#http.h1_stream:set_state}

Sets the state of the stream to `new`. `new` must be one of the following valid states:

  - `"open"`: have sent or received headers; haven't sent body yet
  - `"half closed (local)"`: have sent whole body
  - `"half closed (remote)"`: have received whole body
  - `"closed"`: complete

Not all state transitions are allowed.


### `h1_stream:read_headers(timeout)` <!-- --> {#http.h1_stream:read_headers}

Reads and returns a [header block](#http.headers) from the underlying connection. Does *not* take into account buffered header blocks. On error, returns `nil`, an error message and an error number.

This function should rarely be used, you're probably looking for [`:get_headers()`](#http.h1_stream:get_headers).


### `h1_stream:read_next_chunk(timeout)` <!-- --> {#http.h1_stream:read_next_chunk}

Reads and returns the next chunk as a string from the underlying connection. Does *not* take into account buffered chunks. On error, returns `nil`, an error message and an error number.

This function should rarely be used, you're probably looking for [`:get_next_chunk()`](#http.h1_stream:get_next_chunk).
