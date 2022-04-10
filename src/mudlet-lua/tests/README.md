# Setting up for tests

**Ubuntu**, have Mudlet and [Busted](http://olivinelabs.com/busted/) installed:

	sudo apt-get install luarocks
	sudo luarocks install busted

On **macOS**:

	brew install luarocks
	luarocks install busted
	
On **Windows**

1. Download and unzip [LuaRocks](https://luarocks.org/releases/luarocks-3.8.0-windows-32.zip)

  - Open the x86 Native Tools Command Prompt that comes with Visual Studio in administrator mode (regular command prompt may work but is untested; the x64 Native Tools Command Prompt will *not* work)
	- Navigate to the folder containing the unzipped files
	- `install /P <install_path> /SELFCONTAINED /L` (omit angular brackets)
	  - You may need to include the `/F` option if you have previously installed LuaRocks
	- Write down the recommend `LUA_PATH` and `LUA_CPATH` somewhere. You will need them in the running tests section
	  - Do *not* set `LUA_PATH` or `LUA_CPATH` system environment variables as suggested by the installer output. It will interfere with Mudlet's package loader
	  - Setting `Path` and `PATHEXT` is fine
2. Install Busted
	- Open a command prompt and enter `luarocks install busted`
	- If you get a command not recognized message
	  - You may need to add the LuaRocks directory to your `Path` system environment variable and restart
		- Alternatively, navigate the command prompt to the LuaRocks directory to run LuaRocks commands

You're now ready to run the tests.

# Running tests

1. Open the `Mudlet self-test` profile by typing the name in the connection dialog ([example](https://wiki.mudlet.org/images/4/4d/Opening_Mudlet_self-test_profile.webm
)).
  - If you are following the instruction on Windows, add a script that appends the LuaRocks `LUA_PATH` and `LUA_CPATH` to `package.path` and `package.cpath`, respectively
	  - This should be placed above the "test scripts" script
	  - Example: `package.cpath = package.cpath .. [[;%APPDATA%\LuaRocks\lib\lua\5.1\?.dll;d:\l\luarocks\systree\lib\lua\5.1\?.dll]]`
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