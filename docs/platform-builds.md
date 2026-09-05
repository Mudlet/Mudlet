# Platform builds and debugging defines

## Building on macOS

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_macOS

**Essential build commands:**

```bash
cd /path/to/Mudlet
# wait up to 10mins for a full build
cmake --preset macos-debug
cmake --build --preset macos-debug

# Run Mudlet
./build/src/mudlet.app/Contents/MacOS/mudlet
```

Run `cmake --list-presets` to see the presets available on your machine; alongside `macos-debug`
there are `-nosan`, `-tsan` and `-ubsan` variants, a `macos-static-analysis` preset and a
`macos-release` one carrying the flags CI ships to players. Variants build into
`build-<preset-name>/` rather than `build/`, so several configurations can coexist without
invalidating each other.

The presets do not pin a Qt location, relying on CMake's default search path. If Qt is not found,
pass it explicitly: `cmake --preset macos-debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt6)"`.

**Do not use `cmake --build . --parallel` without a job count in a Makefiles build tree.** A bare
`--parallel` passes `-j` with no number to make, which imposes no limit on concurrent jobs; make
will start as many compilers as the dependency graph allows, exhausting RAM and swap and finishing
slower than a bounded build. Ninja, which the presets use, defaults to a bounded job count. In an
existing Makefiles tree, use `make -j $(sysctl -n hw.ncpu)`.

ccache is enabled automatically whenever it is installed. A full cache evicts objects continuously,
so branch switches can trigger near-full rebuilds — run `ccache -s`, and if `Cache size` has
reached `Max cache size`, raise it with `ccache -M <n>G`.

## Building on Windows

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_Windows

Builds run under MSYS2, in the **CLANG64** environment — open a CLANG64 shell, not MINGW64, and
check it is a real MSYS2 shell rather than Git for Windows' bash carrying an inherited `MSYSTEM`
(`MSYSTEM_PREFIX` is empty in the latter). `CI/setup-windows-sdk.sh` and
`CI/build-mudlet-for-windows.sh` exit with an error on any other `MSYSTEM`, including the
`CLANGARM64` environment native to ARM64 hosts.

The toolchain has to be current enough to carry libc++ 22: the trigger match pool sleeps its helper
threads in `std::atomic::wait`, which older libc++ builds implement on Windows as a polling loop, and
`src/TriggerMatchPool.cpp` refuses them at compile time with a message saying so. `pacman -Syu`
brings MSYS2 up to date.

The `windows-debug` preset reads `MSYSTEM_PREFIX`, which MSYS2 sets in each of its shells, so the
preset follows whichever environment is provisioned:

```bash
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Sanitizers are not enabled on Windows (`src/CMakeLists.txt` guards them with `if(NOT WIN32)`),
so there is no `-nosan` variant. `windows-release` reads `MSYSTEM_PREFIX` the same way, adds
`CMAKE_BUILD_TYPE=Release` to match `CI/build-mudlet-for-windows.sh` - which builds Release on
every Windows run - and builds into `build-windows-release/`.

## Sanitizers and static analysis

Sanitizers are enabled on every non-Windows build; `USE_SANITIZER` defaults to `address`, and a
Release build type does not turn them off on its own - `<platform>-release` clears the variable
explicitly. Use the `-tsan` / `-ubsan` / `-nosan` presets to change the choice, or pass a CMake
list — semicolon-separated, not comma-separated — such as `-DUSE_SANITIZER="Address;Undefined"`.
A comma-separated value is read as a single name, which silently skips the per-sanitizer options
such as `-fno-omit-frame-pointer`.

Usable names are `Address`, `Thread` and `Undefined` on macOS, plus `Memory` and `Leak` on Linux.
`MemoryWithOrigins` appears in the `USE_SANITIZER` cache docstring but has no mapping declared in
`src/cmake/EnableSanitizers.cmake`, so it always fails. An unavailable or incompatible selection
raises a `SEND_ERROR`: configure finishes, but generation is blocked.

Static analysis (clang-tidy and cppcheck) runs during compilation with the
`<platform>-static-analysis` presets, which set `ENABLE_STATIC_ANALYSIS=ON`. The two tools are
independent — whichever is on `PATH` runs. A missing clang-tidy produces a CMake warning, but a
missing cppcheck only emits a `STATUS` line, so read the configure output rather than assuming both
are active.

Because IDEs read `CMakePresets.json` natively, selecting one of these presets in CLion, VS Code or
Qt Creator is enough — no per-IDE sanitizer configuration is needed.

## Optional feature modules

Six feature modules are declared through `include_optional_module` in `CMakeLists.txt`: the updater,
fonts, 3D mapper, shader hot-reloading, memory tracking and the build-type splash screen. Each has a
`USE_*` option and a `WITH_*` name, and they are **not** interchangeable — `cmake/IncludeOptionalModule.cmake`
reads the `WITH_*` name from the **environment** only. So `-DWITH_UPDATER=NO` on the command line is
accepted by CMake and silently ignored; use `-DUSE_UPDATER=OFF`, or set `WITH_UPDATER=NO` in the
environment. Note that shader hot-reloading and memory tracking default to OFF, the rest to ON.

This applies only to those six. Other `WITH_*` names are ordinary options: `WITH_SENTRY` and
`SENTRY_SEND_DEBUG` are declared with `option()` and are set on the command line as normal.

## Debugging options

`src/CMakeLists.txt` contains commented debugging defines for development (search "Debugging code inclusions"):

- `DEBUG_TELNET` - Telnet protocol debugging
- `DEBUG_UTF8_PROCESSING` - UTF-8 decoding messages
- `DEBUG_SGR_PROCESSING` - ANSI color sequence debugging
- `DEBUG_WINDOW_HANDLING` - UI window operations
- And others for encoding, MXP, map autosave, etc.

**Usage**: Uncomment the relevant `target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC DEBUG_XXX)` lines when debugging specific areas. **Important**: Do not commit uncommented debug lines to git.

## Runtime tuning: the trigger match pool

When a single chunk from the game carries many lines, `TriggerMatchPool` (`src/TriggerMatchPool.h`) spreads the "can this trigger match this line?" question over a few helper threads. Four environment variables tune it, read once at startup; none are needed in normal use:

| Variable | Default | Meaning |
| --- | --- | --- |
| `MUDLET_MATCH_THREADS` | `min(4, cores / 2)` | Threads sharing a batch, the main thread included. Capped at the core count. Below 2 the pool is off, so `0` disables it. |
| `MUDLET_MATCH_THRESHOLD` | `32` | Fewest pattern-bearing triggers a profile needs before a batch is shared out. `0` or below falls back to the default. |
| `MUDLET_MATCH_FLOOD_LINES` | `8` | Fewest lines one incoming chunk must carry to count as a flood. `0` or below falls back to the default. |
| `MUDLET_MATCH_SPIN_US` | `100` | Microseconds a helper keeps spinning after a batch before it parks. `0` parks at once, which is the setting for stressing the wake-up path. |

A value that is set but does not parse as an integer, or is out of range, is refused with a warning on the console and the default is used.

Setting the threshold and flood lines to `1` puts every line through the pool, which is the way to run `src/mudlet-lua/tests/TriggerFlood_spec.lua` and the rest of the trigger specs against both paths; `MUDLET_MATCH_SPIN_US=0` on top makes every one of those lines a cold start. No checked-in CI job does this yet, so it is a local run, and the pool has to be on for it to mean anything - `MUDLET_MATCH_THREADS=2` on a small machine. `PipelineBenchmark` reads `MUDLET_BENCH_TRIGGERS` and `MUDLET_BENCH_CHUNK_LINES` to sweep trigger counts and chunk sizes against these thresholds - see the comment at the top of `test/functional_tests/PipelineBenchmark.cpp`.
