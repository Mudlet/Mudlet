## http.h1_connection

The *h1_connection* module adheres to the [*connection*](#connection) interface and provides HTTP 1 and 1.1 specific operations.

### `new(socket, conn_type, version)` <!-- --> {#connection.new}

Constructor for a new connection. Takes a cqueues socket object, a [connection type string](#connection.type) and a numeric HTTP version number. Valid values for the connection type are `"client"` and `"server"`. Valid values for the version number are `1` and `1.1`. Returns the newly initialized connection object.


### `h1_connection.version` <!-- --> {#http.h1_connection.version}

Specifies the HTTP version used for the connection handshake. Valid values are:

  - `1.0`
  - `1.1`

See [`connection.version`](#connection.version)


### `h1_connection:pollfd()` <!-- --> {#http.h1_connection:pollfd}

See [`connection:pollfd()`](#connection:pollfd)


### `h1_connection:events()` <!-- --> {#http.h1_connection:events}

See [`connection:events()`](#connection:events)


### `h1_connection:timeout()` <!-- --> {#http.h1_connection:timeout}

See [`connection:timeout()`](#connection:timeout)


### `h1_connection:connect(timeout)` <!-- --> {#http.h1_connection:connect}

See [`connection:connect(timeout)`](#connection:connect)


### `h1_connection:checktls()` <!-- --> {#http.h1_connection:checktls}

See [`connection:checktls()`](#connection:checktls)


### `h1_connection:localname()` <!-- --> {#http.h1_connection:localname}

See [`connection:localname()`](#connection:localname)


### `h1_connection:peername()` <!-- --> {#http.h1_connection:peername}

See [`connection:peername()`](#connection:peername)


### `h1_connection:flush(timeout)` <!-- --> {#http.h1_connection:flush}

See [`connection:flush(timeout)`](#connection:flush)


### `h1_connection:shutdown(dir)` <!-- --> {#http.h1_connection:shutdown}

Shut down is as graceful as possible: pipelined streams are [shutdown](#http.h1_stream:shutdown), then the underlying socket is shut down in the appropriate direction(s).

`dir` is a string representing the direction of communication to shut down communication in. If it contains `"r"` it will shut down reading, if it contains `"w"` it will shut down writing. The default is `"rw"`, i.e. to shutdown communication in both directions.

See [`connection:shutdown()`](#connection:shutdown)


### `h1_connection:close()` <!-- --> {#http.h1_connection:close}

See [`connection:close()`](#connection:close)


### `h1_connection:new_stream()` <!-- --> {#http.h1_connection:new_stream}

In HTTP 1, only a client may initiate new streams with this function.

See [`connection:new_stream()`](#connection:new_stream) for more information.


### `h1_connection:get_next_incoming_stream(timeout)` <!-- --> {#http.h1_connection:get_next_incoming_stream}

See [`connection:get_next_incoming_stream(timeout)`](#connection:get_next_incoming_stream)


### `h1_connection:onidle(new_handler)` <!-- --> {#http.h1_connection:onidle}

See [`connection:onidle(new_handler)`](#connection:onidle)


### `h1_connection:setmaxline(read_length)` <!-- --> {#http.h1_connection:setmaxline}

Sets the maximum read buffer size (in bytes) to `read_length`. i.e. sets the maximum length lines (such as headers).

The default comes from the underlying socket, which gets the (changable) cqueues default at time of construction.
The default cqueues default is 4096 bytes.


### `h1_connection:clearerr(...)` <!-- --> {#http.h1_connection:clearerr}

Clears errors to allow for further read or write operations on the connection. Returns the error number of existing errors. This function is used to recover from known errors.


### `h1_connection:error(...)` <!-- --> {#http.h1_connection:error}

Returns the error number of existing errors.


### `h1_connection:take_socket()` <!-- --> {#http.h1_connection:take_socket}

Used to hand the reference of the connection socket to another object. Resets the socket to defaults and returns the single existing reference of the socket to the calling routine. This function can be used for connection upgrades such as upgrading from HTTP 1 to a WebSocket.


### `h1_connection:read_request_line(timeout)` <!-- --> {#http.h1_connection:read_request_line}

Reads a request line from the socket. Returns the request method, requested path and HTTP version for an incoming request. `:read_request_line()` yields until a `"\r\n"` terminated chunk is received, or `timeout` is exceeded. If the incoming chunk is not a valid HTTP request line, `nil` is returned. On error, returns `nil`, an error message and an error number.


### `h1_connection:read_status_line(timeout)` <!-- --> {#http.h1_connection:read_status_line}

Reads a line of input from the socket. If the input is a valid status line, the HTTP version (1 or 1.1), status code and reason description (if applicable) is returned. `:read_status_line()` yields until a `"\r\n"` terminated chunk is received, or `timeout` is exceeded. If the socket could not be read, returns `nil`, an error message and an error number.


### `h1_connection:read_header(timeout)` <!-- --> {#http.h1_connection:read_header}

Reads a CRLF terminated HTTP header from the socket and returns the header key and value. This function will yield until a MIME compliant header item is received or until `timeout` is exceeded. If the header could not be read, the function returns `nil` an error and an error message.


### `h1_connection:read_headers_done(timeout)` <!-- --> {#http.h1_connection:read_headers_done}

Checks for an empty line, which indicates the end of the HTTP headers. Returns `true` if an empty line is received. Any other value is pushed back on the socket receive buffer (unget) and the function returns `false`. This function will yield waiting for input from the socket or until `timeout` is exceeded. Returns `nil`, an error and an error message if the socket cannot be read.


### `h1_connection:read_body_by_length(len, timeout)` <!-- --> {#http.h1_connection:read_body_by_length}

Get `len` number of bytes from the socket. Use a negative number for *up to* that number of bytes. This function will yield and wait on the socket if length of the buffered body is less than `len`. Asserts if len is not a number.


### `h1_connection:read_body_till_close(timeout)` <!-- --> {#http.h1_connection:read_body_till_close}

Reads the entire request body. This function will yield until the body is complete or `timeout` is expired. If the read fails the function returns `nil`, an error message and an error number.


### `h1_connection:read_body_chunk(timeout)` <!-- --> {#http.h1_connection:read_body_chunk}

Reads the next available line of data from the request and returns the chunk and any chunk extensions. This function will yield until chunk size is received or `timeout` is exceeded. If the chunk size is indicated as `0` then `false` and any chunk extensions are returned. Returns `nil`, an error message and an error number if there was an error reading reading the chunk header or the socket.


### `h1_connection:write_request_line(method, path, httpversion, timeout)` <!-- --> {#http.h1_connection:write_request_line}

Writes the opening HTTP 1.x request line for a new request to the socket buffer. Yields until success or `timeout`. If the write fails, returns `nil`, an error message and an error number.

*Note the request line will not be flushed to the remote server until* [`write_headers_done`](#http.h1_connection:write_headers_done) *is called.*


### `h1_connection:write_status_line(httpversion, status_code, reason_phrase, timeout)` <!-- --> {#http.h1_connection:write_status_line}

Writes an HTTP status line to the socket buffer. Yields until success or `timeout`. If the write fails, the function returns `nil`, an error message and an error number.

*Note the status line will not be flushed to the remote server until* [`write_headers_done`](#http.h1_connection:write_headers_done) *is called.*


### `h1_connection:write_header(k, v, timeout)` <!-- --> {#http.h1_connection:write_header}

Writes a header item to the socket buffer as a `key:value` string. Yields until success or `timeout`. Returns `nil`, an error message and an error if the write fails.

*Note the header item will not be flushed to the remote server until* [`write_headers_done`](#http.h1_connection:write_headers_done) *is called.*


### `h1_connection:write_headers_done(timeout)` <!-- --> {#http.h1_connection:write_headers_done}

Terminates a header block by writing a blank line (`"\r\n"`) to the socket. This function will flush all outstanding data in the socket output buffer. Yields until success or `timeout`. Returns `nil`, an error message and an error if the write fails.


### `h1_connection:write_body_chunk(chunk, chunk_ext, timeout)` <!-- --> {#http.h1_connection:write_body_chunk}

Writes a chunk of data to the socket. `chunk_ext` must be `nil` as chunk extensions are not supported. Will yield until complete or `timeout` is exceeded. Returns true on success. Returns `nil`, an error message and an error number if the write fails.


### `h1_connection:write_body_last_chunk(chunk_ext, timeout)` <!-- --> {#http.h1_connection:write_body_last_chunk}

Writes the chunked body terminator `"0\r\n"` to the socket. `chunk_ext` must be `nil` as chunk extensions are not supported. Will yield until complete or `timeout` is exceeded. Returns `nil`, an error message and an error number if the write fails.

*Note that the connection will not be immediately flushed to the remote server; normally this will occur when trailers are written.*


### `h1_connection:write_body_plain(body, timeout)` <!-- --> {#http.h1_connection:write_body_plain}

Writes the contents of `body` to the socket and flushes the socket output buffer immediately. Yields until success or `timeout` is exceeded. Returns `nil`, an error message and an error number if the write fails.
