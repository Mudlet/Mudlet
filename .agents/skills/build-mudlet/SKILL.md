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

## First: does this need a build at all?

On Linux, a change confined to `src/mudlet-lua/lua/` or `src/mudlet-lua/tests/` needs none. Those
are read from disk at startup, so a Mudlet binary built anywhere on the machine can run *this*
worktree's Lua and specs:

```bash
.claude/scripts/run-lua-tests.sh ../otherworktree/build-debug/src/mudlet
```

The script detects a binary from another build tree, shims around it with a `note:` line, and
fails loudly if this worktree's Lua is not what ended up loading.

These look Lua-only but still need a build:

- `src/packages/` and `src/mudlet-lua/lua/utf8_filenames.lua` are compiled into the binary as Qt
  resources (`src/mudlet.qrc`), so editing them cannot affect a borrowed binary.
- On macOS the build copies mudlet-lua into the `.app` bundle and that copy is preferred over
  `src/`, so a Lua change does need a rebuild there. The script is Linux-only regardless - it
  drives Mudlet under `xvfb-run`.
- Anything under `src/*.cpp` or `src/*.h`.

Choose a donor whose branch already contains the C++ the specs rely on - one missing it fails
specs in a way that reads exactly like a regression in the change under test. Prefer a
preset build (in a plain `./build/src/` directory) that does not (now) contain any
sanitizer, so leaks or other issues won't be reported.

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
| `<platform>-debug` | macOS / Linux / Windows (MSYS2) | Ninja, Debug, No sanitizers — faster to build and to run |
| `<platform>-debug-asan` | macOS / Linux / Windows (MSYS2, CLANG64) | AddressSanitizer |
| `<platform>-debug-tsan` | macOS / Linux | ThreadSanitizer |
| `<platform>-debug-ubsan` | macOS / Linux / Windows (MSYS2, CLANG64) | UndefinedBehaviorSanitizer |
| `<platform>-static-analysis` | macOS / Linux / Windows (MSYS2, CLANG64) | Runs clang-tidy and cppcheck during compilation |
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

## Running the result

```bash
# macOS
./build/src/mudlet.app/Contents/MacOS/mudlet

# Linux
./build/src/mudlet

# Windows
./build/src/mudlet.exe

# replace 'build' with the suffixed form for CMake presets other than the
# base one.
```

Mudlet is a graphical desktop application; launching it opens a window. Variant presets put the
binary under `build-<preset-name>/` instead. Allow up to 10 minutes for a full build, or even longer
on low specification platforms.

## Claude Code on the web (remote sessions)

The `.claude/hooks/session-start.sh` SessionStart hook provisions the remote Ubuntu container:
apt dependencies, Qt 6.9.0 via aqtinstall under `/opt/qt` (Ubuntu's packaged Qt 6.4 is older
than the 6.8.2 minimum), submodules, and a ccache warm-up build of the `linux-debug`
preset. The hook exports `CMAKE_PREFIX_PATH` pointing at the aqt Qt, so the documented preset
commands work unchanged. On a warm container the hook finishes in seconds and a full build is
mostly ccache hits — measured 5m25s wall for all targets at 99% hit rate, most of it linking —
versus ~25 minutes cold. If the container cache is cold the hook itself takes ~30 minutes, once.
The hook also pre-configures the build with `-DUSE_ALTERNATE_LINKER=mold`:
linking is the bulk of a warm rebuild and mold shrinks it dramatically (PR #9927 measured a CI
link tail of 4m13s → 29s). Keep that flag if you reconfigure the tree from scratch.
Run Mudlet headlessly there with `QT_QPA_PLATFORM=offscreen`.

Both test harnesses work in the remote container (validated: 112/112 ctest, 3202 busted
successes):

- **C++ tests**: `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug`. The functional
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
- **Seeing and driving the real UI**: to verify a feature or fix visually, run Mudlet on a
  virtual display and work it like a user — the hook installs the whole toolchain from
  `docs/demo-videos.md` (xvfb, openbox, xdotool, imagemagick, ffmpeg):

  ```bash
  export DISPLAY=:78
  Xvfb :78 -screen 0 1280x800x24 & sleep 2; openbox & sleep 1
  HOME=$(mktemp -d) ./build/src/mudlet & sleep 8
  xdotool mousemove <x> <y> click 1        # or: xdotool key Return, xdotool type "text"
  import -window root /tmp/shot.png        # screenshot; read it to verify, then iterate
  ```

  A throwaway `HOME` keeps the real profile tree untouched. Screenshot after every
  interaction — coordinates come from looking at the previous shot, not from guessing. The
  same display serves `docs/demo-videos.md`'s before/after recording workflow via ffmpeg.
  All of this is Linux/X11-only, and XTEST events work headlessly on Xvfb only.

The `docker/` directory is a separate developer convenience (QtCreator-in-container); its
Ubuntu 22.04 base only offers Qt 6.2 from apt, so it cannot build current Mudlet until it is
modernised — do not reach for it in remote sessions.

## Pitfalls

**Never pass `--parallel` without a job count on a Makefiles build.** `cmake --build . --parallel`
with no number passes a bare `-j` to make, which imposes no limit on concurrent jobs: make starts
as many compilers as the dependency graph allows, exhausting RAM and swap. Ninja defaults to a
bounded job count, which is why the presets use it. In a pre-existing Makefiles tree, use
`make -j $(nproc)` on Linux / Windows or `make -j $(sysctl -n hw.ncpu)` on macOS.

**ccache is wired in automatically** — `CMakeLists.txt` sets it as the compiler launcher whenever
ccache is installed. A full cache evicts objects continuously, so switching branches can trigger a
near-full rebuild. Run `ccache -s`; if `Cache size` has reached `Max cache size`, raise it with
`ccache -M <n>G`.

**Sanitizers are off by default** on every build, regardless of build type
(`CMakeLists.txt` defaults `USE_SANITIZERS` to `OFF` in the absence of a `WITH_SANITIZER`
environmental variable set to `YES`), they will however be enabled by the relevant
entries in the `CMakePresets.json` file that configures `WITH_SANITIZERS` and
`MUDLET_SANITIZERS` as appropriate for the selected preset.
Sanitizers cost both compile time and runtime speed. Use the `<platform>-debug` preset
when not chasing a bug.

For a combination the presets do not cover, set the **Environmental variable** to
a **semicolon**-separated (not comma-separated) list e.g.: `MUDLET_SANITIZERS="address;undefined"`.
Either `export` it and `WITH_SANITIZERS="YES"` into the current shell environment
or provide both of them as prefixes in the command line when `cmake ...` is called.
Note that a comma-separated value for `MUDLET_SANITIZERS` is treated as one name
which will be rejected as will an incorrectly (not "camelCased") sanitizer name.

Usable names are `address`, `undefined` on all three main supported OSes, with
`thread` on macOS and Linux, plus additionally `memory` and `leak` on Linux.
`memoryWithOrigins` and `type` appear in the CMake `cmake/EnableSanitizers.cmake`
file; the former is derived from the parent `memory` sanitizer but both of those
require all libraries - including "system" ones - to also be compiled with it,
so it is not a trivial exercise to make use of them. An unavailable or incompatible
selection raises a CMake error and configuration aborts.

**Static analysis** (`<platform>-static-analysis`) needs clang-tidy and cppcheck
in `PATH`; except that on macOS, Homebrew's llvm is keg-only, so clang-tidy is
not in `PATH` by default.

Static analysis (clang-tidy and cppcheck) is available on all three platforms and
they run during compilation with the `<platform>-static-analysis` presets, which
set `ENABLE_STATIC_ANALYSIS=ON`. The two tools are independent — whichever is on
`PATH` runs. If one of them is missing a CMake warning will be produced, but if
both are absent when requested this will be elevated to an error and configuration
will fail.

**Configuring a second generator in an existing build directory fails.** CMake cannot switch
generator in place; configure into a fresh directory instead.

## Related

- `docs/platform-builds.md` — platform detail and the compile-time debugging defines
- <https://wiki.mudlet.org/w/Compiling_Mudlet> — full setup, including dependency installation
