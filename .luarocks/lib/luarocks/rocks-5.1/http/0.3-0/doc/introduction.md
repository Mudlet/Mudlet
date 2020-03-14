# Introduction

lua-http is an performant, capable HTTP and WebSocket library for Lua 5.1, 5.2, 5.3 and LuaJIT. Some of the features of the library include:

  - Support for HTTP versions 1, 1.1 and 2 as specified by [RFC 7230](https://tools.ietf.org/html/rfc7230) and [RFC 7540](https://tools.ietf.org/html/rfc7540)
  - Provides both client and server APIs
  - Fully asynchronous API that does not block the current thread when executing operations that typically block
  - Support for WebSockets as specified by [RFC 6455](https://tools.ietf.org/html/rfc6455) including ping/pong, binary data transfer and TLS encryption
  - Transport Layer Security (TLS) - lua-http supports HTTPS and WSS via [luaossl](https://github.com/wahern/luaossl).
  - Easy integration into other event-loop or scripts

### Why lua-http?

The lua-http library was written to fill a gap in the Lua ecosystem by providing an HTTP and WebSocket library with the following traits:

  - Asynchronous and performant
  - Can be used without forcing the developer to follow a specific pattern. Conversely, the library can be adapted to many common patterns.
  - Can be used at a very high level without need to understand the transportation of HTTP data (other than connection addresses).
  - Provides a rich low level API, if desired, for creating powerful HTTP based tools at the protocol level.

As a result of these design goals, the library is simple and unobtrusive and can accommodate tens of thousands of connections on commodity hardware.

lua-http is a flexible HTTP and WebSocket library that allows developers to concentrate on line-of-business features when building Internet enabled applications. If you are looking for a way to streamline development of an internet enabled application, enable HTTP networking in your game, create a new Internet Of Things (IoT) system, or write a performant custom web server for a specific use case, lua-http has the tools you need.


### Portability

lua-http is pure Lua code with dependencies on the following external libraries:

  - [cqueues](http://25thandclement.com/~william/projects/cqueues.html) - Posix API library for Lua
  - [luaossl](http://25thandclement.com/~william/projects/luaossl.html) - Lua bindings for TLS/SSL
  - [lua-zlib](https://github.com/brimworks/lua-zlib) - Optional Lua bindings for zlib

lua-http can run on any operating system supported by cqueues and openssl, which at the time of writing is GNU/Linux, FreeBSD, NetBSD, OpenBSD, OSX and Solaris.


## Common Use Cases

The following are two simple demonstrations of how the lua-http library can be used:

### Retrieving a Document

The highest level interface for clients is [*http.request*](#http.request). By constructing a [*request*](#http.request) object from a URI using [`new_from_uri`](#http.request.new_from_uri) and immediately evaluating it, you can easily fetch an HTTP resource.

```lua
local http_request = require "http.request"
local headers, stream = assert(http_request.new_from_uri("http://example.com"):go())
local body = assert(stream:get_body_as_string())
if headers:get ":status" ~= "200" then
	error(body)
end
print(body)
```


### WebSocket Communications {#http.websocket-example}

To request information from a WebSocket server, use the `websocket` module to create a new WebSocket client.

```lua
local websocket = require "http.websocket"
local ws = websocket.new_from_uri("wss://echo.websocket.org")
assert(ws:connect())
assert(ws:send("koo-eee!"))
local data = assert(ws:receive())
assert(data == "koo-eee!")
assert(ws:close())
```


## Asynchronous Operation

lua-http has been written to perform asynchronously so that it can be used in your application, server or game without blocking your main loop. Asynchronous operations are achieved by utilizing cqueues, a Lua/C library that incorporates Lua yielding and kernel level APIs to reduce CPU usage. All lua-http operations including DNS lookup, TLS negotiation and read/write operations will not block the main application thread when run from inside a cqueue or cqueue enabled "container". While sometimes it is necessary to block a routine (yield) and wait for external data, any blocking API calls take an optional timeout to ensure good behaviour of networked applications and avoid unresponsive or "dead" routines.

Asynchronous operations are one of the most powerful features of lua-http and require no effort on the developers part. For instance, an HTTP server can be instantiated within any Lua main loop and run alongside application code without adversely affecting the main application process. If other cqueue enabled components are integrated within a cqueue loop, the application is entirely event driven through kernel level polling APIs.

cqueues can be used in conjunction with lua-http to integrate other features into your lua application and create powerful, performant, web enabled applications. Some of the examples in this guide will use cqueues for simple demonstrations. For more resources about cqueues, please see:

  - [The cqueues website](http://25thandclement.com/~william/projects/cqueues.html) for more information about the cqueues library.
  - cqueues examples can be found with the cqueues source code available through [git or archives](http://www.25thandclement.com/~william/projects/cqueues.html#download) or accessed online [here](https://github.com/wahern/cqueues/tree/master/examples).
  - For more information on integrating cqueues with other event loop libraries please see [integration with other event loops](https://github.com/wahern/cqueues/wiki/Integrations-with-other-main-loops).
  - For other libraries that use cqueues such as asynchronous APIs for Redis and PostgreSQL, please see [the cqueues wiki entry here](https://github.com/wahern/cqueues/wiki/Libraries-that-use-cqueues).


## Conventions

The following is a list of API conventions and general reference:

### HTTP

  - HTTP 1 request and status line fields are passed around inside of _[headers](#http.headers)_ objects under keys `":authority"`, `":method"`, `":path"`, `":scheme"` and `":status"` as defined in HTTP 2. As such, they are all kept in string form (important to remember for the `:status` field).
  - Header fields should always be used with lower case keys.


### Errors

  - Invalid function parameters will throw a lua error (if validated).
  - Errors are returned as `nil`, error, errno unless noted otherwise.
  - Some HTTP 2 operations return/throw special [http 2 error objects](#http.h2_error).


### Timeouts

All operations that may block the current thread take a `timeout` argument.
This argument is always the number of seconds to allow before returning `nil, err_msg, ETIMEDOUT` where `err_msg` is a localised error message such as `"connection timed out"`.


## Terminology

Much lua-http terminology is borrowed from HTTP 2.

_[Connection](#connection)_ - An abstraction over an underlying TCP/IP socket. lua-http currently has two connection types: one for HTTP 1, one for HTTP 2.

_[Stream](#stream)_ - A request/response on a connection object. lua-http has two stream types: one for [*HTTP 1 streams*](#http.h1_stream), and one for [*HTTP 2 streams*](#http.h2_stream). The common interfaces is described in [*stream*](#stream).
