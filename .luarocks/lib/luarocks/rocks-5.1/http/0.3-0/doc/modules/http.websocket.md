## http.websocket

### `new_from_uri(uri, protocols)` <!-- --> {#http.websocket.new_from_uri}

Creates a new `http.websocket` object of type `"client"` from the given URI.

  - `protocols` (optional) should be a lua table containing a sequence of protocols to send to the server


### `new_from_stream(stream, headers)` <!-- --> {#http.websocket.new_from_stream}

Attempts to create a new `http.websocket` object of type `"server"` from the given request headers and stream.

  - [`stream`](#http.h1_stream) should be a live HTTP 1 stream of the `"server"` type.
  - [`headers`](#http.headers) should be headers of a suspected websocket upgrade request from an HTTP 1 client.

This function does **not** have side effects, and is hence okay to use tentatively.


### `websocket.close_timeout` <!-- --> {#http.websocket.close_timeout}

Amount of time (in seconds) to wait between sending a close frame and actually closing the connection.
Defaults to `3` seconds.


### `websocket:accept(options, timeout)` <!-- --> {#http.websocket:accept}

Completes negotiation with a websocket client.

  - `options` is a table containing:
	  - `headers` (optional) a [headers](#http.headers) object to use as a prototype for the response headers
	  - `protocols` (optional) should be a lua table containing a sequence of protocols to allow from the client

Usually called after a successful [`new_from_stream`](#http.websocket.new_from_stream)


### `websocket:connect(timeout)` <!-- --> {#http.websocket:connect}

Connect to a websocket server.

Usually called after a successful [`new_from_uri`](#http.websocket.new_from_uri)


### `websocket:receive(timeout)` <!-- --> {#http.websocket:receive}

Reads and returns the next data frame plus its opcode.
Any ping frames received while reading will be responded to.

The opcode `0x1` will be returned as `"text"` and `0x2` will be returned as `"binary"`.


### `websocket:each()` <!-- --> {#http.websocket:each}

Iterator over [`websocket:receive()`](#http.websocket:receive).


### `websocket:send_frame(frame, timeout)` <!-- --> {#http.websocket:send_frame}

Low level function to send a raw frame.


### `websocket:send(data, opcode, timeout)` <!-- --> {#http.websocket:send}

Send the given `data` as a data frame.

  - `data` should be a string
  - `opcode` can be a numeric opcode, `"text"` or `"binary"`. If `nil`, defaults to a text frame.
    Note this `opcode` is the websocket frame opcode, not an application specific opcode. The opcode should be one from the [IANA registry](https://www.iana.org/assignments/websocket/websocket.xhtml#opcode).


### `websocket:send_ping(data, timeout)` <!-- --> {#http.websocket:send_ping}

Sends a ping frame.

  - `data` is optional


### `websocket:send_pong(data, timeout)` <!-- --> {#http.websocket:send_pong}

Sends a pong frame. Works as a unidirectional keep-alive.

  - `data` is optional


### `websocket:close(code, reason, timeout)` <!-- --> {#http.websocket:close}

Closes the websocket connection.

  - `code` defaults to `1000`
  - `reason` is an optional string
