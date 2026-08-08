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

Builds run under the MSYS2 **CLANG64** environment; `CI/setup-windows-sdk.sh` and
`CI/build-mudlet-for-windows.sh` both refuse any other `MSYSTEM` outright. Open a CLANG64 shell,
not MINGW64. `windows-debug` reads `MSYSTEM_PREFIX`, which MSYS2 sets in every such shell, so no
setup step is needed for the preset itself — but the dependencies still have to be installed
first, per <https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_Windows>. Sanitizers are not
enabled on Windows, so there is no `-nosan` variant.

## Running the result

```bash
# macOS
./build/src/mudlet.app/Contents/MacOS/mudlet

# Linux
./build/src/mudlet
```

Mudlet is a graphical desktop application; launching it opens a window. Variant presets put the
binary under `build-<preset-name>/` instead. Allow up to 10 minutes for a full build.

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
