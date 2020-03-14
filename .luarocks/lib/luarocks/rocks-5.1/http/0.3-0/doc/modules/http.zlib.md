## http.zlib

An abstraction layer over the various lua zlib libraries.

### `engine` <!-- --> {#http.zlib.engine}

Currently either [`"lua-zlib"`](https://github.com/brimworks/lua-zlib) or [`"lzlib"`](https://github.com/LuaDist/lzlib)


### `inflate()` <!-- --> {#http.zlib.inflate}

Returns a closure that inflates (uncompresses) a zlib stream.

The closure takes a string of compressed data and an end of stream flag (`boolean`) as parameters and returns the inflated output as a string. The function will throw an error if the input is not a valid zlib stream.


### `deflate()` <!-- --> {#http.zlib.deflate}

Returns a closure that deflates (compresses) a zlib stream.

The closure takes a string of uncompressed data and an end of stream flag (`boolean`) as parameters and returns the deflated output as a string.


### Example {#http.zlib-example}

```lua
local zlib = require "http.zlib"
local original = "the racecar raced around the racecar track"
local deflater = zlib.deflate()
local compressed = deflater(original, true)
print(#original, #compressed) -- compressed should be smaller
local inflater = zlib.inflate()
local uncompressed = inflater(compressed, true)
assert(original == uncompressed)
```
