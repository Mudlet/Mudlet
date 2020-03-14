## http.client

Deals with obtaining a connection to an HTTP server.

### `negotiate(socket, options, timeout)` <!-- --> {#http.client.negotiate}

Negotiates the HTTP settings with the remote server. If TLS has been specified, this function instantiates the encryption tunnel. Parameters are as follows:

  - `socket` is a cqueues socket object
  - `options` is a table containing:
	- `.tls` (boolean, optional): Should TLS be used?  
	  defaults to `false`
	- `.ctx` (userdata, optional): the `SSL_CTX*` to use if `.tls` is `true`.  
	  If `.ctx` is `nil` then a default context will be used.
	- `.sendname` (string|boolean, optional): the [TLS SNI](https://en.wikipedia.org/wiki/Server_Name_Indication) host to send.  
	  defaults to `true`
	  - `true` indicates to copy the `.host` field as long as it is **not** an IP
	  - `false` disables SNI
	- `.version` (`nil`|1.0|1.1|2): HTTP version to use.
	  - `nil`: attempts HTTP 2 and falls back to HTTP 1.1
	  - `1.0`
	  - `1.1`
	  - `2`
	- `.h2_settings` (table, optional): HTTP 2 settings to use. See [*http.h2_connection*](#http.h2_connection) for details


### `connect(options, timeout)` <!-- --> {#http.client.connect}

This function returns a new connection to an HTTP server. Once a connection has been opened, a stream can be created to start a request/response exchange. Please see [`h1_stream.new_stream`](#h1_stream.new_stream) and [`h2_stream.new_stream`](#h2_stream.new_stream) for more information about creating streams.

  - `options` is a table containing the options to [`http.client.negotiate`](#http.client.negotiate), plus the following:
	  - `family` (integer, optional): socket family to use.  
		defaults to `AF_INET`
	  - `host` (string): host to connect to.  
		may be either a hostname or an IP address
	  - `port` (string|integer): port to connect to in numeric form  
		e.g. `"80"` or `80`
	  - `path` (string): path to connect to (UNIX sockets)
	  - `v6only` (boolean, optional): if the `IPV6_V6ONLY` flag should be set on the underlying socket.
	  - `bind` (string, optional): the local outgoing address and optionally port to bind in the form of `"address[:port]"`, IPv6 addresses may be specified via square bracket notation. e.g. `"127.0.0.1"`, `"127.0.0.1:50000"`, `"[::1]:30000"`.
  - `timeout` (optional) is the maximum amount of time (in seconds) to allow for connection to be established.  
	This includes time for DNS lookup, connection, TLS negotiation (if TLS enabled) and in the case of HTTP 2: settings exchange.

#### Example {#http.client.connect-example}

Connect to a local HTTP server running on port 8000

```lua
local http_client = require "http.client"
local myconnection = http_client.connect {
	host = "localhost";
	port = 8000;
	tls = false;
}
```
