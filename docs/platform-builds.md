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

## Reproducing a CI build

`CMakePresets.json` also carries the presets CI configures with — `ci-linux`, `ci-macos`,
`ci-windows` and `ci-codeql` — so a build that fails only on a runner can be reproduced with
`cmake --preset ci-linux` instead of transcribing flags out of the workflow. The values a run
varies by tag or matrix entry come from the environment, and leaving one unset is *not* the same
as what CI passes — set them to match the job being reproduced:

| Variable | Pull request build | `Mudlet-*` release tag |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | empty | `Release` |
| `USE_SANITIZER` | `Address` on Linux, empty on macOS | empty |
| `WITH_SENTRY` | `ON` | `ON` |
| `SENTRY_SEND_DEBUG` | `0` | `1` |

```bash
USE_SANITIZER=Address cmake --preset ci-linux
```

`WITH_SENTRY=ON` builds sentry-native from the submodule, so leave it unset unless the failure
involves Sentry; `SENTRY_DSN` is a repository secret and cannot be matched locally at all.

These presets build outside the checkout, into `../b/ninja`, because that is where the workflows'
ctest and packaging steps look — `ci-windows` is the exception and uses `build-$MSYSTEM/`.

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

Seven feature modules are declared through `include_optional_module` in `CMakeLists.txt`: the
updater, fonts, 3D mapper, MCP server, shader hot-reloading, memory tracking and the build-type
splash screen. Each has a
`USE_*` option and a `WITH_*` name, and they are **not** interchangeable — `cmake/IncludeOptionalModule.cmake`
reads the `WITH_*` name from the **environment** only. So `-DWITH_UPDATER=NO` on the command line is
accepted by CMake and silently ignored; use `-DUSE_UPDATER=OFF`, or set `WITH_UPDATER=NO` in the
environment. Note that the MCP server, shader hot-reloading and memory tracking default to OFF, the
rest to ON.

This applies only to those seven. Other `WITH_*` names are ordinary options: `WITH_SENTRY` and
`SENTRY_SEND_DEBUG` are declared with `option()` and are set on the command line as normal.

### MCP server: extra Qt modules

The MCP server is the one optional module that needs Qt modules the rest of Mudlet does not, so it
is the one that fails at configure time rather than silently doing nothing when they are missing:
`src/CMakeLists.txt` does `find_package(Qt6 COMPONENTS HttpServer REQUIRED)` only under
`USE_MCPSERVER`. Qt HttpServer in turn pulls in Qt WebSockets, and distributions package the two
separately, so install both:

| Environment | Packages |
| --- | --- |
| Debian / Ubuntu | `qt6-httpserver-dev` `qt6-websockets-dev` |
| Arch | `qt6-httpserver` `qt6-websockets` |
| Fedora | `qt6-qthttpserver-devel` `qt6-qtwebsockets-devel` |
| MSYS2 (CLANG64) | `${MINGW_PACKAGE_PREFIX}-qt6-httpserver` `${MINGW_PACKAGE_PREFIX}-qt6-websockets` — `CI/setup-windows-sdk.sh` installs both |
| macOS (Homebrew) | included in the `qt` formula, nothing extra to install |
| aqtinstall (what CI uses) | `-m qthttpserver qtwebsockets` alongside the existing modules |

Then build with the module on — remembering that the `WITH_*` name is read from the environment:

```bash
WITH_MCPSERVER=ON cmake --preset linux-debug
```

`-DUSE_MCPSERVER=ON` on the command line works too. Leaving it off is the default and needs
neither package; the sources and the `TMCPServerTest` / `TMCPBridgeTest` ctest cases are simply
not built.

## Debugging options

`src/CMakeLists.txt` contains commented debugging defines for development (search "Debugging code inclusions"):

- `DEBUG_TELNET` - Telnet protocol debugging
- `DEBUG_UTF8_PROCESSING` - UTF-8 decoding messages
- `DEBUG_SGR_PROCESSING` - ANSI color sequence debugging
- `DEBUG_WINDOW_HANDLING` - UI window operations
- And others for encoding, MXP, map autosave, etc.

**Usage**: Uncomment the relevant `target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC DEBUG_XXX)` lines when debugging specific areas. **Important**: Do not commit uncommented debug lines to git.
