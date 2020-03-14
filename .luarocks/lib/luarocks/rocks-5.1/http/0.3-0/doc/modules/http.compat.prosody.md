## http.compat.prosody

Provides usage similar to [prosody's net.http](https://prosody.im/doc/developers/net/http)

### `request(url, ex, callback)` <!-- --> {#http.compat.prosody.request}

A few key differences to the prosody `net.http.request`:

  - must be called from within a running cqueue
  - The callback may be called from a different thread in the cqueue
  - The returned object will be a [*http.request*](#http.request) object
	  - This object is passed to the callback on errors and as the fourth argument on success
  - The default user-agent will be from lua-http (rather than `"Prosody XMPP Server"`)
  - lua-http features (such as HTTP2) will be used where possible


### Example {#http.compat.prosody-example}

```lua
local prosody_http = require "http.compat.prosody"
local cqueues = require "cqueues"
local cq = cqueues.new()
cq:wrap(function()
	prosody_http.request("http://httpbin.org/ip", {}, function(b, c, r)
		print(c) --> 200
		print(b) --> {"origin": "123.123.123.123"}
	end)
end)
assert(cq:loop())
```
