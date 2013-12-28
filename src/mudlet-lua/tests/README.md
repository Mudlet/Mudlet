# Setting up for tests

**Ubuntu**, have Mudlet installed:

	sudo apt-get install luarocks
	sudo luarocks install busted

You're now ready to run the tests.

**Ubuntu**, haven't got Mudlet installed:

	sudo apt-get install lua5.1
	sudo apt-get install liblua5.1-filesystem0
	sudo apt-get install liblua5.1-rex-pcre0
	sudo apt-get install liblua5.1-sql-sqlite3-2
	sudo apt-get install liblua5.1-zip0

You're now ready to run the tests.

**Other platforms**:

Tests are only run on Ubuntu by the maintainer and CI at the moment, so the extra effort do add other platforms to instructions hasn't been done (time is short, and there's many other things to do). If you'd like to add instructions here, and maintain them, for another platform - please fork!

mudlet-lua tests depend on [Busted](http://olivinelabs.com/busted) (that's the test framework), [Lua](http://www.lua.org/) and some Lua libraries Mudlet makes use of: [LuaFilesystem](http://keplerproject.github.io/luafilesystem), [Lrexlib](http://rrthomas.github.io/lrexlib/), [LuaSQL](http://www.keplerproject.org/luasql/), and [LuaZip](http://www.keplerproject.org/luazip/). Once you have all of that installed, you'll be able to run the tests.

# Running tests

**Ubuntu**:

	cd tests
	busted -l lua DB.lua
	busted -l lua Other.lua

Ideally we'd be able to use just one command for all files, but Busted doesn't handle `busted -l lua *.lua` and I couldn't get [`.busted`](http://olivinelabs.com/busted/#usage) to comply.

# Creating tests

See [Busted manual](http://olivinelabs.com/busted/) and currently existing tests for examples on how to write tests.

## Test structure

Each file in `tests/` should mimic its companion in `lua/` - i.e., ``tests/DB.lua`` tests all the functionality that is present in ``lua/DB.lua``.

Tests should be logically grouped in `describe()` blocks, running one or more tests described by `it()` that are relevant to the block and tests all possible cases.