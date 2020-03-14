## http.compat.socket

Provides compatibility with [luasocket's http.request module](http://w3.impa.br/~diego/software/luasocket/http.html).

Differences:

  - Will automatically be non-blocking when run inside a cqueues managed coroutine
  - lua-http features (such as HTTP2) will be used where possible


### Example {#http.compat.socket-example}

Using the 'simple' interface as part of a normal script:

```lua
local socket_http = require "http.compat.socket"
local body, code = assert(socket_http.request("http://lua.org"))
print(code, #body) --> 200, 2514
```
