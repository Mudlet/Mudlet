# Setting up for tests

## Ubuntu

Have Mudlet and [Busted](https://lunarmodules.github.io/busted/) installed:

```sh
  sudo apt-get install luarocks
  sudo luarocks --lua-version 5.1 install busted
```

## macOS

```sh
  brew install luarocks
  luarocks --lua-version 5.1 install busted
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

- Open a command prompt and enter `luarocks --lua-version 5.1 install busted`
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

## Running tests headlessly (and in parallel)

The suite can also be run without touching the GUI, which is how CI runs it:

```sh
AUTORUN_BUSTED_TESTS=true \
MUDLET_TEST_MODE=1 \
QUIT_MUDLET_AFTER_TESTS=true \
TESTS_DIRECTORY=<full path>/src/mudlet-lua/tests \
xvfb-run -a ./build/src/mudlet --profile "Mudlet self-test" --mirror --offline
```

A failure writes a marker file (`/tmp/busted-tests-failed` on Linux/macOS) so the
caller can detect it.

On a **Wayland** desktop, prefix that with `QT_QPA_PLATFORM=xcb GDK_BACKEND=x11`.
Xvfb is an X server and neither toolkit targets it by itself there: Qt takes the
wayland plugin over `xvfb-run`'s `DISPLAY`, and the GTK3 platform theme Qt loads
under GNOME calls `gtk_init()`, which exits the process when it cannot open a
display - so the run dies before Mudlet starts, with exit code 1, nothing on
stdout and no marker file.

`--offline` opens the profile without connecting to its game server, which is
what lets a spec use `feedTelnet()` - that function only injects while the telnet
socket is unconnected. Specs that rely on it fail without the flag, so keep it on
every invocation; when opening the profile through the GUI instead, use the
connection dialog's **Offline** button.

By default Mudlet reads and writes `~/.config/mudlet`, so two runs started at the
same time (for example from different checkouts) share one `Mudlet self-test`
profile and collide on its sqlite databases, and leftover state from a previous
run is not cleaned up. To give a run its own pristine, isolated config root:

- `XDG_CONFIG_HOME` - Mudlet uses `$XDG_CONFIG_HOME/mudlet` as its config root
  (profiles, sqlite databases, settings, and password storage). Because an
  existing `~/.config/mudlet` holding profiles otherwise wins (so a system-wide
  `XDG_CONFIG_HOME` export never strands real profiles), a test harness must
  **pre-create** `$XDG_CONFIG_HOME/mudlet/profiles` to opt in. The `mudlet`
  directory on its own is not enough - other tooling creates that by accident,
  and treating it as an opt-in would hide the user's real profiles.
- `MUDLET_TEST_FAILURE_MARKER` - absolute path for the failure marker, so it is
  not shared either.

```sh
CONFIG_DIR=$(mktemp -d)
mkdir -p "$CONFIG_DIR/mudlet/profiles"   # pre-create to opt into the isolated config root
AUTORUN_BUSTED_TESTS=true \
MUDLET_TEST_MODE=1 \
QUIT_MUDLET_AFTER_TESTS=true \
TESTS_DIRECTORY=<full path>/src/mudlet-lua/tests \
XDG_CONFIG_HOME="$CONFIG_DIR" \
MUDLET_TEST_FAILURE_MARKER="$CONFIG_DIR/busted-tests-failed" \
xvfb-run -a ./build/src/mudlet --profile "Mudlet self-test" --mirror --offline
```

## Running the fuzzers

`TelnetTriggerFuzz_spec.lua` drives random bytes through the telnet parser and
random patterns through the trigger engine; `BufferManipFuzz_spec.lua` drives the
console text and selection API with deliberately out-of-range line and column
indices. Both are diagnostic specs rather than pass/fail tests: they are meant to
be run against a sanitizer build, where a memory error or undefined behaviour
aborts the run instead of corrupting silently.

Because busted recurses into this whole directory, both bail out before
registering anything unless `MUDLET_FUZZ` is set, so a normal run - CI included -
skips them. Set it to opt in:

```sh
MUDLET_FUZZ=1 MUDLET_FUZZ_SEED=1 \
AUTORUN_BUSTED_TESTS=true MUDLET_TEST_MODE=1 QUIT_MUDLET_AFTER_TESTS=true \
TESTS_DIRECTORY=<full path>/src/mudlet-lua/tests \
xvfb-run -a ./build/src/mudlet --profile "Mudlet self-test" --mirror --offline
```

Build the binary with `cmake --preset linux-debug` (AddressSanitizer) or
`--preset linux-debug-ubsan` (UndefinedBehaviorSanitizer) to get that coverage.
The telnet fuzzer needs `--offline`, which is what makes `feedTelnet()` run the
real socket-data state machine.

Every random choice comes from a seeded, self-contained PRNG, so a seed replays
byte-for-byte. Each fuzzer takes a seed and iteration counts, and a dump path
that records the last input before it is fed, so the tail of that file is the
crashing input after an abort - see the header comment in each spec for the full
list of variables.

Some seeds abort today: the fuzzers reach bugs that are still open, so treat an
abort as a finding to reduce and report rather than as a broken harness.

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

### Cleaning up

Around 4000 tests run in one process, so anything a test creates - a trigger, an alias, a
window, a room, a global - has to be undone even when an assertion fails partway through.
`finally()` is the tool for that, but busted keeps only **one** callback per `it()`: a
second `finally()` silently replaces the first, and the cleanup it was holding never runs.
Do all of a test's cleanup in a single `finally()`.
