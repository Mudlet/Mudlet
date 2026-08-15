---
name: build-mudlet
description: >-
  Read before running any cmake, ninja or make command in this repository, and when asked to
  build, rebuild, compile, or run Mudlet locally. Gives the correct per-platform configure and
  build invocation, and the parallelism pitfalls that make builds slow or thrash the machine.
license: GPL-2.0-or-later
---

## When to use

Read this **before** typing a build command, not after one fails. The correct invocation differs
per platform, and the wrong one is not merely slower — see Pitfalls.

## Use a preset — every platform, one command

`CMakePresets.json` in the repository root encodes the generator, build type and sanitizer
settings, so no platform-specific flags need to be remembered or typed. Run
`cmake --list-presets` to see the ones available on the current machine.

Presets need CMake 3.25.1 or newer, which is the same floor `CMakeLists.txt` already sets for
building Mudlet at all. An older CMake reports an unknown argument or an unsupported preset
version rather than anything informative, so check `cmake --version` if `--preset` is rejected.

```bash
cmake --preset macos-debug            # configure
cmake --build --preset macos-debug    # build
ctest --preset macos-debug            # run the test suite
```

| Preset | Platform | Notes |
| --- | --- | --- |
| `macos-debug` / `linux-debug` | macOS / Linux | Ninja, Debug, AddressSanitizer on |
| `windows-debug` | Windows | MSYS2 CLANG64, Ninja, Debug |
| `<platform>-debug-nosan` | macOS / Linux | No sanitizers — faster to build and to run |
| `<platform>-debug-tsan` | macOS / Linux | ThreadSanitizer instead of AddressSanitizer |
| `<platform>-debug-ubsan` | macOS / Linux | UndefinedBehaviorSanitizer |
| `<platform>-static-analysis` | macOS / Linux | Runs clang-tidy and cppcheck during compilation |
| `linux-lowspec` | Linux | No sanitizers, no updater, no 3D mapper, 2 jobs — Raspberry Pi and similar |

Every configure preset has a matching build and test preset of the same name, and all three are
conditioned on the host system — so `cmake --list-presets` on macOS will not offer `linux-debug`,
and `ctest --preset X` always runs against the tree that `cmake --build --preset X` produced.

The plain `<platform>-debug` presets build into `build/`. Every variant builds into
`build-<preset-name>/` instead, so an AddressSanitizer tree and a sanitizer-free tree can coexist
without forcing each other to rebuild. The `/build*` entry in `.gitignore` covers all of them.

### Qt discovery

The presets do not pin a Qt location. CMake installed via Homebrew or a distribution package
finds Qt on its default search path. If configuring fails to find Qt — likely with a CMake from
the Qt online installer — pass the prefix explicitly:

```bash
cmake --preset macos-debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt6)"
```

### Windows

Builds run under MSYS2. Use the **CLANG64** environment: `CI/setup-windows-sdk.sh`, which installs
the dependencies, accepts only that one and exits on any other `MSYSTEM`. Open a CLANG64 shell, not
MINGW64, and make sure it is a real MSYS2 shell — Git for Windows' bash can carry an inherited
`MSYSTEM` that makes it look like one, in which case `MSYSTEM_PREFIX` is empty.

The preset itself is not tied to a particular environment: it reads `MSYSTEM_PREFIX`, which MSYS2
sets in each of its shells, so it follows whichever one is provisioned. On an ARM64 host the native
environment is `CLANGARM64`, which the setup script does not currently handle, so dependencies have
to come from a CLANG64 shell.

Sanitizers are not enabled on Windows, so there is no `-nosan` variant.

## Running the result

```bash
# macOS
./build/src/mudlet.app/Contents/MacOS/mudlet

# Linux
./build/src/mudlet
```

Mudlet is a graphical desktop application; launching it opens a window. Variant presets put the
binary under `build-<preset-name>/` instead. Allow up to 10 minutes for a full build.

## Claude Code on the web (remote sessions)

The `.claude/hooks/session-start.sh` SessionStart hook provisions the remote Ubuntu container:
apt dependencies, Qt 6.9.0 via aqtinstall under `/opt/qt` (Ubuntu's packaged Qt 6.4 is older
than the 6.8.2 minimum), submodules, and a ccache warm-up build of the `linux-debug-nosan`
preset. The hook exports `CMAKE_PREFIX_PATH` pointing at the aqt Qt, so the documented preset
commands work unchanged. On a warm container the hook is a sub-second no-op and a full build is
mostly ccache hits — measured 5m25s wall for all targets at 99% hit rate, most of it linking —
versus ~25 minutes cold. If the container cache is cold the hook itself takes ~30 minutes, once.
Run Mudlet headlessly there with `QT_QPA_PLATFORM=offscreen`.

Both test harnesses work in the remote container (validated: 112/112 ctest, 3202 busted
successes):

- **C++ tests**: `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan`. The functional
  tests load `LuaGlobal.lua`, which needs the `--local` Lua rocks on `LUA_PATH`/`LUA_CPATH` —
  the hook exports both; without them ~12 tests fail with `attempt to index global 'yajl'` or
  `'rex'` errors.
- **Lua specs (busted)**: `.claude/scripts/run-lua-tests.sh` — starts the HTTP/Discord/MMCP
  fixtures from `CI/` and runs the self-test profile under xvfb exactly like the
  "(Linux) Run Lua tests" CI step. Concurrent runs are safe (one per worktree, or even the
  same tree): fixtures bind ephemeral ports handed over via a per-run temp directory, cleanup
  kills only that run's fixture PIDs, and each run gets a private HOME so no two Mudlets — nor
  leftovers of an aborted run — share the self-test profile's saved state. Sharing a profile
  tree is not survivable: stale state fails ~38 Networking specs with "Expected objects to be
  the same" at the `ensurePeer` assertion.
- The egress proxy blocks GitHub codeload tarballs (403), so `luarocks install` of rocks whose
  rockspecs point at tarballs fails; the hook falls back to `git clone` + `luarocks make`
  (git-protocol GitHub access is allowed). `LuaSQL-SQLite3` must stay pinned at 2.6.1 — 2.8.0
  breaks `DB.lua`'s `PRAGMA table_info` handling and errors 8 DB specs.
- **GUI automation without Mudlet changes**: the busted run's Xvfb is a real X server, so
  `xdotool` (installed by the hook) can drive Mudlet from outside — a spec launches it in the
  background with `os.execute("(xdotool mousemove --window $(xdotool search --class mudlet | head -1) X Y; xdotool click 1) &")`
  and pumps `pumpEvents()` until the effect lands; `xdotool key`/`type` reach the focused
  widget. Validated: a real XTEST click fires a `Geyser.Label` click callback. Linux/X11 only —
  a repo spec using this needs a gate, since CI's macOS busted legs have no xdotool or X11.

The `docker/` directory is a separate developer convenience (QtCreator-in-container); its
Ubuntu 22.04 base only offers Qt 6.2 from apt, so it cannot build current Mudlet until it is
modernised — do not reach for it in remote sessions.

## Pitfalls

**Never pass `--parallel` without a job count on a Makefiles build.** `cmake --build . --parallel`
with no number passes a bare `-j` to make, which imposes no limit on concurrent jobs: make starts
as many compilers as the dependency graph allows, exhausting RAM and swap. Ninja defaults to a
bounded job count, which is why the presets use it. In a pre-existing Makefiles tree, use
`make -j $(nproc)` on Linux or `make -j $(sysctl -n hw.ncpu)` on macOS.

**ccache is wired in automatically** — `CMakeLists.txt` sets it as the compiler launcher whenever
ccache is installed. A full cache evicts objects continuously, so switching branches can trigger a
near-full rebuild. Run `ccache -s`; if `Cache size` has reached `Max cache size`, raise it with
`ccache -M <n>G`.

**Sanitizers are on by default** on every non-Windows build, regardless of build type
(`src/cmake/EnableSanitizers.cmake` defaults `USE_SANITIZER` to `address`). They cost both compile
time and runtime speed. Use a `-nosan` preset when not chasing a memory bug.

For a combination the presets do not cover, pass a **CMake list — semicolon-separated, not
comma-separated**: `-DUSE_SANITIZER="Address;Undefined"`. A comma-separated value is treated as one
name, which silently skips the per-sanitizer options such as `-fno-omit-frame-pointer`.

Usable names are `Address`, `Thread` and `Undefined` on macOS, plus `Memory` and `Leak` on Linux.
`MemoryWithOrigins` appears in the `USE_SANITIZER` cache docstring but has no mapping declared, so
it always fails. An unavailable or incompatible selection raises a `SEND_ERROR`: configure runs to
completion, but generation is blocked.

**Static analysis** (`<platform>-static-analysis`) needs clang-tidy and cppcheck on `PATH`. They
are independent: whichever is present runs. A missing clang-tidy produces a CMake warning, but a
missing cppcheck only prints a `STATUS` line that is easy to miss — so check the configure output
rather than assuming both ran. On macOS, Homebrew's llvm is keg-only, so clang-tidy is not on
`PATH` by default.

**Configuring a second generator in an existing build directory fails.** CMake cannot switch
generator in place; configure into a fresh directory instead.

## Related

- `docs/platform-builds.md` — platform detail and the compile-time debugging defines
- <https://wiki.mudlet.org/w/Compiling_Mudlet> — full setup, including dependency installation
