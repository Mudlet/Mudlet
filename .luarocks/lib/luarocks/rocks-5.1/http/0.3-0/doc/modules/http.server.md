## http.server

*http.server* objects are used to encapsulate the `accept()` and dispatch of http clients. Each new client request will invoke the `onstream` callback in a new cqueues managed coroutine. In addition to constructing and returning a HTTP response, an `onstream` handler may decide to take ownership of the connection for other purposes, e.g. upgrade from a HTTP 1.1 connection to a WebSocket connection.

For examples of how to use the server library, please see the [examples directory](https://github.com/daurnimator/lua-http/tree/master/examples) in the source tree.

### `new(options)` <!-- --> {#http.server.new}

Creates a new instance of an HTTP server listening on the given socket.

  - `.socket` (*cqueues.socket*): the socket that `accept()` will be called on
  - `.onerror` (*function*): Function that will be called when an error occurs (default handler throws an error). See [server:onerror()](#http.server:onerror)
  - `.onstream` (*function*): Callback function for handling a new client request. The function receives the [*server*](#http.server) and the new [*stream*](#stream) as parameters. If the callback throws an error it will be reported from [`:step()`](#http.server:step) or [`:loop()`](#http.server:loop)
  - `.tls` (*boolean*): Specifies if the system should use Transport Layer Security. Values are:
	  - `nil`: Allow both tls and non-tls connections
	  - `true`: Allows tls connections only
	  - `false`: Allows non-tls connections only
  - `.ctx` (*context object*): An `openssl.ssl.context` object to use for tls connections. If `nil` is passed, a self-signed context will be generated.
  - `.connection_setup_timeout` (*number*): Timeout (in seconds) to wait for client to send first bytes and/or complete TLS handshake. Default is 10 seconds.
  - `.intra_stream_timeout` (*number*): Timeout (in seconds) to wait for a new [*stream*](#stream) on an idle connection before giving up and closing the connection
  - `.version` (*number*): The http version to allow to connect (default: any)
  - `.cq` (*cqueue*): A cqueues controller to use as a main loop. The default is a new controller for the server.
  - `.max_concurrent` (*number*): Maximum number of connections to allow live at a time. Default is infinity.


### `listen(options)` <!-- --> {#http.server.listen}

Creates a new socket and returns an HTTP server that will accept() from it.
Parameters are the same as [`new(options)`](#http.server.new) except instead of `.socket` you provide the following:

  - `.host` (*string*): Local IP address in dotted decimal or IPV6 notation. This value is required if `.path` is not specified.
  - `.port` (*number*): IP port for the local socket. Specify 0 for automatic port selection. Ports 1-1024 require the application has root privilege to run. Maximum value is 65535. If `.tls == nil` then this value is required. Otherwise, the defaults are:
	  - `80` if `.tls == false`
	  - `443` if `.tls == true`
  - `.path` (*string*): Path to UNIX a socket. This value is required if `.host` is not specified.
  - `.family` (*string*): Protocol family. Default is `"AF_INET"`
  - `.v6only` (*boolean*): Specify `true` to limit all connections to ipv6 only (no ipv4-mapped-ipv6). Default is `false`.
  - `.mode` (*string*): `fchmod` or `chmod` socket after creating UNIX domain socket.
  - `.mask` (*boolean*): Set and restore umask when binding UNIX domain socket.
  - `.unlink` (*boolean*): `true` means unlink socket path before binding.
  - `.reuseaddr` (*boolean*): Turn on `SO_REUSEADDR` flag.
  - `.reuseport` (*boolean*): Turn on `SO_REUSEPORT` flag.


### `server:onerror(new_handler)` <!-- --> {#http.server:onerror}

If called with parameters, the function replaces the current error handler function with `new_handler` and returns a reference to the old function. Calling the function with no parameters returns the current error handler. The default handler throws an error. The `onerror` function for the server can be set during instantiation through the `options` table passed to the [`server.listen(options)`](#server.listen) function.


### `server:listen(timeout)` <!-- --> {#http.server:listen}

Initializes the server socket and if required, resolves DNS. `server:listen()` is required if [*localname*](#http.server:localname) is called before [*step*](#http.server:step) or [*loop*](#http.server:loop). On error, returns `nil`, an error message and an error number.


### `server:localname()` <!-- --> {#http.server:localname}

Returns the connection information for the local socket. Returns address family, IP address and port for an external socket. For Unix domain sockets, the function returns AF_UNIX and the path. If the connection object is not connected, returns AF_UNSPEC (0). On error, returns `nil`, an error message and an error number.


### `server:pause()` <!-- --> {#http.server:pause}

Cause the server loop to stop processing new clients until [*resume*](#http.server:resume) is called. Existing client connections will run until closed.


### `server:resume()` <!-- --> {#http.server:resume}

Resumes a [*paused*](#http.server:pause) `server` and processes new client requests.


### `server:close()` <!-- --> {#http.server:close}

Shutdown the server and close the socket. A closed server cannot be reused.


### `server:pollfd()` <!-- --> {#http.server:pollfd}

Returns a file descriptor (as an integer) or `nil`.

The file descriptor can be passed to a system API like `select` or `kqueue` to wait on anything this server object wants to do. This method is used for integrating with other main loops, and should be used in combination with [`:events()`](#http.server:events) and [`:timeout()`](#http.server:timeout).


### `server:events()` <!-- --> {#http.server:events}

Returns a string indicating the type of events the object is waiting on: the string will contain `"r"` if it wants to be *step*ed when the file descriptor returned by [`pollfd()`](#http.server:pollfd) has had POLLIN indicated; `"w"` for POLLOUT or `"p"` for POLLPRI.

This method is used for integrating with other main loops, and should be used in combination with [`:pollfd()`](#http.server:pollfd) and [`:timeout()`](#http.server:timeout).


### `server:timeout()` <!-- --> {#http.server:timeout}

The maximum time (in seconds) to wait before calling [`server:step()`](#http.server:step).

This method is used for integrating with other main loops, and should be used in combination with [`:pollfd()`](#http.server:pollfd) and [`:events()`](#http.server:events).


### `server:empty()` <!-- --> {#http.server:empty}

Returns `true` if the master socket and all client connection have been closed, `false` otherwise.


### `server:step(timeout)` <!-- --> {#http.server:step}

Step once through server's main loop: any waiting clients will be `accept()`-ed, any pending streams will start getting processed, and each `onstream` handler will get be run at most once. This method will block for *up to* `timeout` seconds. On error, returns `nil`, an error message and an error number.

This can be used for integration with external main loops.


### `server:loop(timeout)` <!-- --> {#http.server:loop}

Run the server as a blocking loop for up to `timeout` seconds. The server will continue to listen and accept client requests until either [`:pause()`](#http.server:pause) or [`:close()`](#http.server:close) is called, or an error is experienced.


### `server:add_socket(socket)` <!-- --> {#http.server:add_socket}

Add a new connection socket to the server for processing. The server will use the current `onstream` request handler and all `options` currently specified through the [`server.listen(options)`](#http.server.listen) constructor. `add_socket` can be used to process connection sockets obtained from an external source such as:

  - Another cqueues thread with some other master socket.
  - From inetd for start on demand daemons.
  - A Unix socket with `SCM_RIGHTS`.


### `server:add_stream(stream)` <!-- --> {#http.server:add_stream}

Add an existing stream to the server for processing.
