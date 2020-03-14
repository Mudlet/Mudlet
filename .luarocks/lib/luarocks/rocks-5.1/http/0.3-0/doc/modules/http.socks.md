## http.socks

Implements a subset of the SOCKS proxy protocol.

### `connect(uri)` <!-- --> {#http.socks.connect}

`uri` is a string with the address of the SOCKS server. A scheme of `"socks5"` will resolve hosts locally, a scheme of `"socks5h"` will resolve hosts on the SOCKS server. If the URI has a userinfo component it will be sent to the SOCKS server as a username and password.

Returns a *http.socks* object.


### `fdopen(socket)` <!-- --> {#http.socks.fdopen}

This function takes an existing cqueues.socket as a parameter and returns a *http.socks* object with `socket` as its base.


### `socks.needs_resolve` <!-- --> {#http.socks.needs_resolve}

Specifies if the destination host should be resolved locally.


### `socks:clone()` <!-- --> {#http.socks:clone}

Make a clone of a given socks object.


### `socks:add_username_password_auth(username, password)` <!-- --> {#http.socks:add_username_password_auth}

Add username + password authorisation to the set of allowed authorisation methods with the given credentials.


### `socks:negotiate(host, port, timeout)` <!-- --> {#http.socks:negotiate}

Complete the SOCKS connection.

Negotiates a socks connection. `host` is a required string passed to the SOCKS server as the host address. The address will be resolved locally if [`.needs_resolve`](#http.socks.needs_resolve) is `true`. `port` is a required number to pass to the SOCKS server as the connection port. On error, returns `nil`, an error message and an error number.


### `socks:close()` <!-- --> {#http.socks:close}


### `socks:take_socket()` <!-- --> {#http.socks:take_socket}

Take possession of the socket object managed by the http.socks object. Returns the socket (or `nil` if not available).
