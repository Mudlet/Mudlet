# Setting up for tests

## Ubuntu

Have Mudlet and [Busted](https://lunarmodules.github.io/busted/) installed:

```sh
  sudo apt-get install luarocks
  sudo luarocks install busted
```

## macOS

```sh
  brew install luarocks
  luarocks install busted
```

## Windows

### Download and unzip [LuaRocks](https://luarocks.org/releases/luarocks-3.8.0-windows-32.zip)

- Install Visual Studio (the [free community edition](https://visualstudio.microsoft.com/vs/community/) works)
- Open the x86 Native Tools Command Prompt that comes with Visual Studio in administrator mode (regular command prompt may work but is untested; the x64 Native Tools Command Prompt will *not* work)
  - Navigate to the folder containing the unzipped files
  - `install /P <install_path> /SELFCONTAINED /L` (omit angular brackets)
    - You may need to include the `/F` option if you have previously installed LuaRocks
  - Set the `LUA_PATH` or `LUA_CPATH` system environment variables as suggested by the installer output, ensuring that it ends in two `;;` characters. This causes Lua to insert additional paths needed by Mudlet and without them Mudlet won't be able to load its included lua libraries properly. For example:
  - LUA_PATH for me is set to `C:\Qt\Tools\mingw730_32\lib\luarocks\rocks-5.1\?.lua;C:\Qt\Tools\mingw730_32\lib\luarocks\rocks-5.1\?\init.lua;C:\Qt\Tools\mingw730_32\share\lua\5.1\?.lua;C:\Qt\Tools\mingw730_32\share\lua\5.1\?\init.lua;;`
  - LUA_CPATH for me is set to `C:\Qt\Tools\mingw730_32\lib\luarocks\rocks-5.1\?.dll;;`
    - Setting `Path` and `PATHEXT` is fine

### Install Busted

- Open a command prompt and enter `luarocks install busted`
- If you get a `'luarocks' is not recognized...` message:
  - You may need to add the LuaRocks directory to your `Path` system environment variable and restart
  - Alternatively, navigate the command prompt to the LuaRocks directory to run LuaRocks commands

You're now ready to run the tests.

## Running tests

1. Open the `Mudlet self-test` profile by typing the name in the connection dialog ([example](https://wiki.mudlet.org/images/4/4d/Opening_Mudlet_self-test_profile.webm
)).

2. Use the `runTests` either with the location of the folder with all tests, or a specific test:

```txt
-- run all tests in the folder:
runTests <full path>/src/mudlet-lua/tests

-- run a specific test
runTests <full path>/src/mudlet-lua/tests/StringUtils_spec.lua
```

## Creating tests

See [Busted manual](https://lunarmodules.github.io/busted/) and currently existing tests for examples on how to write tests.

### Test structure

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
