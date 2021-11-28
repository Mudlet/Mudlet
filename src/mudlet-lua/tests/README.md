# Setting up for tests

**Ubuntu**, have Mudlet and [Busted](http://olivinelabs.com/busted/) installed:

	sudo apt-get install luarocks
	sudo luarocks install busted

On **macOS**:

	brew install luarocks
	luarocks install busted

You're now ready to run the tests.

# Running tests

1. Open the `Mudlet self-test` profile by typing the name in the connection dialog ([example](https://wiki.mudlet.org/images/4/4d/Opening_Mudlet_self-test_profile.webm
)).
2. Use the `runTests` either with the location of the folder with all tests, or a specific test:
```
-- run all tests in the folder:
runTests <full path>/src/mudlet-lua/tests

-- run a specific test
runTests <full path>/src/mudlet-lua/tests/StringUtils_spec.lua
```

# Creating tests

See [Busted manual](http://olivinelabs.com/busted/) and currently existing tests for examples on how to write tests.

## Test structure

Each file in `tests/` should mimic its companion in `lua/` - i.e., ``tests/DB.lua`` tests all the functionality that is present in ``lua/DB.lua``.

Tests should be logically grouped in `describe()` blocks, running one or more tests described by `it()` that are relevant to the block and tests all possible cases. Have a look at existing tests for inspiration.
