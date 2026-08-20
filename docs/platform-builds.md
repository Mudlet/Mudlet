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
there are `-asan`, `-tsan` and `-ubsan` variants and a `macos-static-analysis` preset. Variants
build into `build-<preset-name>/` rather than `build/`, so several configurations can coexist
without invalidating each other.

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

The `windows-debug` preset reads `MSYSTEM_PREFIX`, which MSYS2 sets in each of its shells, so the
preset follows whichever environment is provisioned:

```bash
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Some sanitizers are theoretically available on Windows but not all of them seem
to work on Windows -- further investigation is needed.

## Sanitizers and static analysis

Sanitizers are available but not enabled on every build; the environmental variable
`WITH_SANITIZERS` is the master switch (defaults to `NO`) - if it is set to `YES`
the environmental variable `MUDLET_SANTIZERS` needs to be set to one (or more,
separated by semi-colons) of the sanitizers to include.

Use the `-asan` / `-tsan` / `-ubsan` presets or pass something in that variable
such as `MUDLET_SANITIZER="address;undefined"` as prefix to the commandline
that calls `cmake`. Invalid sanitizers or combinations will be detected and cause
`cmake` to emit a warning and not be included.

Usable names are `address`, `thread` and `undefined` on macOS, plus `memory` and
`leak` on Linux. `memoryWithOrigins` appears in the internal `USE_SANITIZER`
cache docstring and is also available but both that and the parent `memory`
sanitizer requires all libraries - including "system" ones - to also be compiled
with it, so it is not a trivial exercise to make use of it.

Static analysis (clang-tidy and cppcheck) runs during compilation with the
`<platform>-static-analysis` presets, which set `ENABLE_STATIC_ANALYSIS=ON`. The
two tools are independent — whichever is on `PATH` runs. A missing clang-tidy
produces a CMake warning, but a missing cppcheck only emits a `STATUS` line, so
read the configure output rather than assuming both are active.

Because IDEs read `CMakePresets.json` natively, selecting one of these presets
in CLion, VS Code or Qt Creator is enough — no per-IDE sanitizer configuration
is needed.

## Optional feature modules

Seven feature modules are declared through `include_optional_module` in
`CMakeLists.txt`: the updater, fonts, 3D mapper, shader hot-reloading, memory
tracking, the build-type splash screen and overall control of the use of
sanitizers. Each has a `USE_*` CMake variable and a `WITH_*` environmental
variable, and they are **not** interchangeable — `cmake/IncludeOptionalModule.cmake`
reads the `WITH_*` value from the **environment** only. So `-DWITH_UPDATER=NO` on
the command line is accepted by CMake as a CMake variable and silently ignored;
use `-DUSE_UPDATER=OFF`, or preferable set `WITH_UPDATER=NO` in the environment.
Note that shader hot-reloading, memory tracking and sanitizers default to OFF,
the rest to ON.

This applies only to those seven. There are other `WITH_*` names (`WITH_CCACHE`
and `WITH_SENTRY`) the former is used internally and the latter is currently
(and confusingly) all of: an environmental variable, a CMake variable and also
a compilation symbol seen by compilers. It is planned to resolve that into a
`WITH_SENTRY`/`USE_SENTRY`/`INCLUDE_SENTRY` triplet.

`SENTRY_SEND_DEBUG` is a CMake variable and declared with `option()` and is
currently set on the command line. It may also change to a `WITH_XXXX`/`USE_XXXX`
environmental/CMake variable pair in a future revision.

There was previously a `WITH_OWN_QTKEYCHAIN` environmental variable that
controlled the setting of the CMake `USE_OWN_QTKEYCHAIN` - which was used to
determine whether the "QtKeyChain" library was built from source code (default
to an affirmative) or sourced from the build system's packaged version. This was
removed and `USE_OWN_QTKEYCHAIN` is now managed internally on an OS dependent
manner.

## Debugging options

`src/CMakeLists.txt` contains commented debugging defines for development (search "Debugging code inclusions"):

- `DEBUG_TELNET` - Telnet protocol debugging
- `DEBUG_UTF8_PROCESSING` - UTF-8 decoding messages
- `DEBUG_SGR_PROCESSING` - ANSI color sequence debugging
- `DEBUG_WINDOW_HANDLING` - UI window operations

with others for encoding, MXP, map autosave, etc. Some additional ones may be
found in individual class files, currently this is only: `DEBUG_DISCORD` and
`DEBUG_RECORDING`.

**Usage**: Uncomment the relevant `target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC DEBUG_XXX)` lines when debugging specific areas. **Important**: Do not commit uncommented debug lines to git.
