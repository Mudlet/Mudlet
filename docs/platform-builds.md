# Platform builds and debugging defines

## Building on macOS

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#macOS

**Essential build commands:**

```bash
cd /path/to/Mudlet
# wait up to 10mins for a full build
cmake --preset macos-debug
cmake --build --preset macos-debug

# Run Mudlet - as an application, not as the binary inside the bundle.
# macos-debug is an AddressSanitizer preset, and run-mudlet launches the binary
# directly there so the sanitizer's environment reaches it; build the -nosan
# preset when you want the bundle launch described below.
cmake --build --preset macos-debug-nosan --target run-mudlet
```

That target runs `open`, so macOS holds Mudlet responsible for its own permission
requests. Started from a shell instead, the responsible process is the application owning
the terminal, and asking for speech recognition kills Mudlet outright - the report names a
usage description that Mudlet's own `Info.plist` has carried all along. `open` detaches, so
the target puts `qDebug()` and `qWarning()` output in `build/mudlet-run.log` rather than
losing it.

A sanitizer build runs the binary directly instead, which the target does for you: `open`
hands the launch to launchd rather than passing your shell's environment on, so
`ASAN_OPTIONS` would be ignored and the report would go to a terminal nothing is reading.
That is why the run command above names `macos-debug-nosan`: under a sanitizer preset the
target launches the binary, the terminal stays responsible, and speech refuses to ask for
permission rather than showing the dialogs.
Only the built-in macOS speech backend is affected by the launch method at all, and it
declines to ask for permission rather than dying when it finds something else responsible
for the process.

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

Checkouts and worktrees share one cache: unless ccache has a `base_dir` configured, CMake sets it
to each checkout. Debug builds share only with `hash_dir` off (`ccache --set-config=hash_dir=false`),
and then a debugger may show source from whichever checkout first compiled an object. Keep absolute
paths out of compile definitions that reach many files, as `base_dir` does not rewrite a path
inside a `-D` value. Setting `QT_RCC_SOURCE_DATE_OVERRIDE=1` in the environment lets the
compiled-in resources (fonts, images) share as well, as rcc otherwise records each file's mtime.

## Building on Windows

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Windows

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
| `MUDLET_PGO` | empty | `GENERATE` for the first pass on macOS (Windows passes it with `-D`); see below |

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

## Profile-guided optimisation

A PGO build lays out and inlines code by what a training run measured. It is off by default and
takes two builds of the *same* tree, since GCC finds each object's profile by the object's path:

```bash
cmake --preset linux-release -DMUDLET_PGO=GENERATE
cmake --build build-linux-release --target PipelineBenchmark   # the instrumented program
CI/pgo-train.sh build-linux-release       # runs PipelineBenchmark, writes the profile
cmake -B build-linux-release -DMUDLET_PGO=USE
cmake --build build-linux-release --target mudlet_executable
```

GCC and Clang are both supported; for Clang, `CI/pgo-train.sh` merges the raw profiles with
`llvm-profdata` (`LLVM_PROFDATA` picks a particular one), and on Linux the instrumented link needs
the compiler-rt profile runtime (`libclang-rt-<version>-dev` on Ubuntu). The profile lands in
`MUDLET_PGO_DIR`, `<build>/pgo-profile` unless set. The training script clears it before each run,
as Clang names each raw profile after the binary that wrote it and would otherwise merge an older
build's in. It also fails when a slot the training is for did not run, since a skipped slot still
leaves a profile - one that marks its code cold. Keep the sources the same between the two builds: a
function whose code changed loses its profile, which GCC refuses as an error and Clang reports as a
`hash mismatch` warning.

The training is PipelineBenchmark: text decoding, the trigger engine, the default packages and
console painting. Functions it never enters are compiled as if there were no profile under GCC,
thanks to `-fprofile-partial-training`, and treated as cold under Clang. Measured as time taken
relative to a plain Release build of the same compiler at `-O3` - below 1 is faster - interleaved
and paired per round, with svof (an Achaea combat system of 3,086 triggers), the 2D mapper and the
pathfinder kept out of the training:

| | GCC 13 | Clang 18 |
| --- | --- | --- |
| trigger engine (trained) | 0.99 | 0.81 |
| text pipeline (trained) | 0.98 | 0.83 |
| console painting (trained) | noise | 0.95 |
| svof triggers (held out) | 0.98 | noise |
| pathfinder graph build (held out) | noise | 0.82 |
| 2D map rendering (held out) | noise | noise |

So it pays mostly for Clang builds. Held-out work can gain too, as the pathfinder did, but a big
trigger package like svof gained little from a profile of the benchmark's triggers, and nothing
measured slower. Hand-placed `Q_LIKELY`/`Q_UNLIKELY` hints are not a substitute - removing all of
Mudlet's, or adding more to the hottest branches, measured as noise.

CI builds the macOS and Windows releases this way, both of which use Clang. Linux stays on plain
GCC: against it, a profile-guided Clang build was faster on the trained paths (triggers 0.90, text
0.86) but no faster on svof (0.98, noise) and slower at 2D map rendering (1.08 to 1.24), plain
Clang being slower than GCC there to begin with.

The first pass builds only an instrumented `PipelineBenchmark` - building `mudlet` would send the
instrumented binary's debug files to Sentry - and the second builds everything against the profile,
so the tests that follow run on what ships. Tagged releases and the nightly PTBs both build this way,
so a broken profile-guided build shows up the morning after it lands rather than on release day.
macOS PTBs keep their own build type, which is unoptimised, so there the nightly run proves the
steps work but not that the optimised compile does - before tagging a release, run the *Build
Mudlet* workflow by hand with its `pgo` input set to `true`, which builds macOS as Release the way
a tag does (*Build Mudlet (windows)* takes the same input, though its PTBs are Release already).
Pull requests never build profile-guided, and no profile-guided run saves its objects to the
shared ccache.

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

When a single chunk from the game carries many lines, `TriggerMatchPool` (`src/TriggerMatchPool.h`) spreads the "can this trigger match this line?" question over a few helper threads, for the triggers with a Perl regex pattern - the one pattern kind whose evaluation costs a search; substring, begin-of-line and exact-match patterns are answered on the main thread in a few instructions, so nothing is gained by handing them over. Four knobs tune it, read once when the pool starts; none are needed in normal use. Each has a key in `Mudlet.ini` (in the `[General]` section, alongside the other settings there) for a player who wants to keep a setting, and an environment variable that overrides the file for one run, which is what the tests and benchmarks use:

| `Mudlet.ini` key | Environment variable | Default | Meaning |
| --- | --- | --- | --- |
| `triggerMatchThreads` | `MUDLET_MATCH_THREADS` | `min(4, cores / 2)` | Threads sharing a batch, the main thread included. Capped at the core count. Below 2 the pool is off, so `0` disables it and the trigger engine runs exactly as it did before the pool existed. |
| `triggerMatchThreshold` | `MUDLET_MATCH_THRESHOLD` | `128` | Fewest regex searches the previous line must have run before this line's batch is shared out. Searches rather than triggers: a trigger that is disabled, multiline, or settled by an earlier pattern of its own runs none, and only work the pool would actually share out counts. `0` or below falls back to the default. |
| `triggerMatchFloodLines` | `MUDLET_MATCH_FLOOD_LINES` | `8` | Fewest lines one incoming chunk must carry to count as a flood. `0` or below falls back to the default. |
| `triggerMatchSpinMicroseconds` | `MUDLET_MATCH_SPIN_US` | `100` | Microseconds a helper keeps spinning after a batch before it parks. `0` parks at once, which is the setting for stressing the wake-up path. |

A value that is set but does not parse as an integer, or is out of range, is refused with a warning on the console and the default is used. For example, to turn the pool off for good:

```ini
[General]
triggerMatchThreads=0
```

Setting the threshold and flood lines to `1` puts every line through the pool, which is the way to run `src/mudlet-lua/tests/TriggerFlood_spec.lua` and the rest of the trigger specs against both paths; `MUDLET_MATCH_SPIN_US=0` on top makes every one of those lines a cold start. No checked-in CI job does this yet, so it is a local run, and the pool has to be on for it to mean anything - `MUDLET_MATCH_THREADS=2` on a small machine. `PipelineBenchmark` reads `MUDLET_BENCH_TRIGGERS` and `MUDLET_BENCH_CHUNK_LINES` to sweep trigger counts and chunk sizes against these thresholds - see the comment at the top of `test/functional_tests/PipelineBenchmark.cpp`.
