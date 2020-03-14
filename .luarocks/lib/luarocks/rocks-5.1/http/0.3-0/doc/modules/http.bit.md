## http.bit

An abstraction layer over the various lua bit libraries.

Results are only consistent between underlying implementations when parameters and results are in the range of `0` to `0x7fffffff`.

### `band(a, b)` <!-- --> {#http.bit.band}

Bitwise And operation.


### `bor(a, b)` <!-- --> {#http.bit.bor}

Bitwise Or operation.


### `bxor(a, b)` <!-- --> {#http.bit.bxor}

Bitwise XOr operation.


### Example {#http.bit-example}

```lua
local bit = require "http.bit"
print(bit.band(1, 3)) --> 1
```
