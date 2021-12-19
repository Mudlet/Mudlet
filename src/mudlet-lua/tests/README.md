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

Tests for a specific function should be grouped within a describe block as follows:

```lua
describe("Tests the functionality of myFunctionName", function() 
  it("should handle situation 1", function()
	  -- test
	end)

	it ("should handle situation 2", function()
	  -- another test
	end)
end)
```

If you have tests which it makes sense to have but would not logically fall into a describe block like this specific one, that is fine, but we use the format of the describe message as part of our method for gathering some code coverage metrics so we would like to try and include one describe for each function tested, in addition to any other logical groups of tests necessary. See existing test files for examples and ask on Discord is you still need help.