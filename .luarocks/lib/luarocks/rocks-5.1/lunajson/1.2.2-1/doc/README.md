# ![Lunajson](logo/lunajson.png)
[![CircleCI](https://circleci.com/gh/grafi-tt/lunajson.svg?style=shield)](https://circleci.com/gh/grafi-tt/lunajson)

Lunajson features SAX-style JSON parser and simple JSON decoder/encoder. It is tested on Lua 5.1, Lua 5.2, Lua 5.3, and LuaJIT 2.0.
It is written only in pure Lua and has no dependencies. Even so, decoding speed matches lpeg-based JSON implementations because it is carefully optimized.
The parser and decoder reject input that is not conformant to the JSON specification (ECMA-404), and the encoder always yields conformant output.
The parser and decoder also handle UTF/Unicode surrogate pairs correctly.

## Install
	luarocks install lunajson

Or you can download source manually and copy `src/*` into somewhere on your `package.path`.

## Simple Usage
	local lunajson = require 'lunajson'
	local jsonstr = '{"Hello":["lunajson",1.5]}'
	local t = lunajson.decode(jsonstr)
	print(t.Hello[2]) -- prints 1.5
	print(lunajson.encode(t)) -- prints {"Hello":["lunajson",1.5]}

## API
### lunajson.decode(jsonstr, [pos, [nullv, [arraylen]]])
Decode `jsonstr`. If `pos` is specified, it starts decoding from `pos` until the JSON definition ends, otherwise the entire input is parsed as JSON. `null` inside `jsonstr` will be decoded as the optional sentinel value `nullv` if specified, and discarded otherwise. If `arraylen` is true, the length of an array `ary` will be stored in `ary[0]`. This behavior is useful when empty arrays should not be confused with empty objects.

This function returns the decoded value if `jsonstr` contains valid JSON,  otherwise an error will be raised. If `pos` is specified it also returns the position immediately after the end of decoded JSON.

### lunajson.encode(value, [nullv])
Encode `value` into a JSON string and return it. If `nullv` is specified, values equal to `nullv` will be encoded as `null`.

This function encodes a table `t` as a JSON array if a value `t[1]` is present or a number `t[0]` is present. If `t[0]` is present, its value is considered as the length of the array. Then the array may contain `nil` and those will be encoded as `null`. Otherwise, this function scans non `nil` values starting from index 1, up to the first `nil` it finds. When the table `t` is not an array, it is an object and all of its keys must be strings.

### lunajson.newparser(input, saxtbl)
### lunajson.newfileparser(filename, saxtbl)
Create and return a sax-style parser context, which parses `input` or a file specified by `filename`. `input` can be a string to be parsed, or a function that returns the next chunk of a data as a string to be parsed (or `nil` when all data is yielded). An example function for `input` follows (this sample is essentially same as the implementation of `newfileparser`). Note that `input` will never be called once it has returned `nil`.

	local fp = io.open("myfavorite.json")
	local function input()
		local s
		if fp then
			s = fp:read(8192)
			if not s then
				fp:close()
				fp = nil
			end
		end
		return s
	end

`saxtbl` is a table of callbacks. It can have the following functions. Those functions will be called on corresponding events, if it is in the table.

- startobject()
- key(s)
- endobject()
- startarray()
- endarray()
- string(s)
- number(n)
- boolean(b)
- null()

A parser context maintains the current parse position, initially 1.

#### parsercontext.run()
Start parsing from current position. If valid JSON is parsed, the position moves to just after the end of this JSON. Otherwise it errors.

#### parsercontext.tellpos()
Return current position.

#### parsercontext.tryc()
Return the byte of current position as a number. If input is ended, it returns `nil`. It does not change current position.

#### parsercontext.read(n)
Return the `n`-length string starting from current position, and increase the index by `n`. If the input ends, the returned string and the updated position will be truncated.

## Benchmark
Following graphs are the results of the benchmark, decoding [`simple.json`](test/decodeparse/benchjson/simple.json) (about 750KiB) 100 times and encoding [`simple.lua`](test/encode/benchdata/simple.lua) (the decoded result of `simple.json`) 100 times. I conducted benchmarks of lunajson 1.0, [dkjson 2.5](http://dkolf.de/src/dkjson-lua.fsl/home) and [Lua CJSON 2.1.0](http://www.kyne.com.au/~mark/software/lua-cjson.php). Dkjson is a popular JSON encoding/decoding library in Lua, which is written in Lua and optionally uses [lpeg](http://www.inf.puc-rio.br/~roberto/lpeg/) to spped up decoding. Lua CJSON is a JSON encoding/decoding library implemented in C and is inherently fast.

![The graph of decoding benchmark results](result/decode-simple.png)

![The graph of encoding benchmark results](result/encode-simple.png)

This benchmark is conducted in my desktop machine that equips Core i5 3550K and DDR3-1600 memory. Lua implementations and concerning modules are compiled by GCC 4.9.2 with `-O2 -march=ivybridge -mtune=ivybridge` options. The versions of lua implementations are the newest official releases at the time of benchmark. The version of lpeg is 0.12.2.

In this benchmark Lunajson performs well considering that it is implemented only in standard Lua, especially in LuaJIT 2.0 benchmark. Lunajson also supplies incremental parsing in a SAX-style API, therfore you don't have to load whole large JSON files into memory in order to scan the information you're interested in from them. Lunajson is especially useful when non-standard libraries cannot be used easily or incremental parsing is favored.
