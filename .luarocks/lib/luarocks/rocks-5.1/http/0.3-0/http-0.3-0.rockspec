package = "http"
version = "0.3-0"

description = {
	summary = "HTTP library for Lua";
	homepage = "https://github.com/daurnimator/lua-http";
	license = "MIT";
}

source = {
	url = "https://github.com/daurnimator/lua-http/archive/v0.3.zip";
	dir = "lua-http-0.3";
}

dependencies = {
	"lua >= 5.1";
	"compat53 >= 0.3"; -- Only if lua < 5.3
	"bit32"; -- Only if lua == 5.1
	"cqueues >= 20161214";
	"luaossl >= 20161208";
	"basexx >= 0.2.0";
	"lpeg";
	"lpeg_patterns >= 0.5";
	"binaryheap >= 0.3";
	"fifo";
	-- "psl"; -- Optional
}

build = {
	type = "builtin";
	modules = {
		["http.bit"] = "http/bit.lua";
		["http.client"] = "http/client.lua";
		["http.connection_common"] = "http/connection_common.lua";
		["http.cookie"] = "http/cookie.lua";
		["http.h1_connection"] = "http/h1_connection.lua";
		["http.h1_reason_phrases"] = "http/h1_reason_phrases.lua";
		["http.h1_stream"] = "http/h1_stream.lua";
		["http.h2_connection"] = "http/h2_connection.lua";
		["http.h2_error"] = "http/h2_error.lua";
		["http.h2_stream"] = "http/h2_stream.lua";
		["http.headers"] = "http/headers.lua";
		["http.hpack"] = "http/hpack.lua";
		["http.hsts"] = "http/hsts.lua";
		["http.proxies"] = "http/proxies.lua";
		["http.request"] = "http/request.lua";
		["http.server"] = "http/server.lua";
		["http.socks"] = "http/socks.lua";
		["http.stream_common"] = "http/stream_common.lua";
		["http.tls"] = "http/tls.lua";
		["http.util"] = "http/util.lua";
		["http.version"] = "http/version.lua";
		["http.websocket"] = "http/websocket.lua";
		["http.zlib"] = "http/zlib.lua";
		["http.compat.prosody"] = "http/compat/prosody.lua";
		["http.compat.socket"] = "http/compat/socket.lua";
	};
}
