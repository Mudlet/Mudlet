---
name: improve-performance
description: >-
  Make Mudlet faster: pick a subsystem with the user, measure, find the real cost, fix it
  and prove the win. Read before benchmarking anything or evaluating a performance claim.
license: GPL-2.0-or-later
user-invocable: true
argument-hint: Optional subsystem to speed up (e.g. "the 2D mapper", "triggers")
---

Runs out of the box on Claude Code for the web, whose containers are Linux (the
SessionStart hook provisions the toolchain); elsewhere read the `build-mudlet` skill
first. Commands below default to Linux; where macOS or Windows differs it is called out
inline.

## Procedure

1. **Pick the battlefield with the user.** If they named a subsystem, that is the target;
   otherwise ask them to pick rather than choosing silently. Proven-fertile ground: telnet
   + text pipeline throughput, the trigger engine, 2D mapper rendering, map pathfinding,
   profile load and startup time. "You pick" is also an answer: profile a busy session and
   take the top entry.
2. **Build a benchmark-safe tree.** Never benchmark a sanitizer build - ASan roughly
   halves text throughput - and the default is to have one, because
   `src/cmake/EnableSanitizers.cmake` defaults `USE_SANITIZER` to `address`.
   - Linux: `linux-debug-nosan` (plain `linux-debug` IS an ASan build).
   - macOS: `macos-debug-nosan`, same trap with the same preset naming.
   - Windows: `windows-debug` is already safe - `src/CMakeLists.txt` skips
     `EnableSanitizers.cmake` entirely under `WIN32`, so no Windows build has one.

   Variant presets build into `build-<preset-name>/`, so the binary is
   `build-linux-debug-nosan/src/mudlet`,
   `build-macos-debug-nosan/src/mudlet.app/Contents/MacOS/mudlet` or
   `build/src/mudlet.exe`. Confirm rather than assume before quoting any number -
   `grep USE_SANITIZER <build>/CMakeCache.txt` must print an empty value on Linux and
   macOS and nothing at all on Windows. A whole A/B campaign has been published off two
   ASan binaries because nobody checked.
3. **Establish a baseline** with an in-tree benchmark (below), or a small harness
   following the patterns below. Record the exact invocation - an unreproducible number is
   worthless.
4. **Find where the time goes**: detect by shape, identify by ablation or profiler, never
   by code reading (rules below).
5. **Fix and re-measure A/B.** Keep the fix only if the end-to-end number moves, not just
   the microbenchmark.
6. **Prove no behaviour change** - run the busted specs
   (`.claude/scripts/run-lua-tests.sh` on Linux, see below for elsewhere) and `ctest` -
   then open a pull request with the
   `open-pr` skill, quoting before/after numbers and the exact invocation so a reviewer
   can reproduce them.

## Keep the numbers honest

- **Benchmark on a quiet machine, and prove it was quiet.** A concurrent build roughly
  doubles every timing, and load that comes and goes mid-run leaves a plausible-looking
  number rather than an obviously broken one. So look, before you start, at what else is
  running: other builds and test suites, a second agent's Mudlet, browsers, VMs and
  containers, and the platform's own background work - Spotlight (`mds`) indexing a fresh
  build tree or Time Machine on macOS, Defender scanning one or Windows Search indexing
  it, an `updatedb` or backup job on Linux. On a laptop, mains power rather than battery:
  both macOS and Windows down-clock aggressively on battery, and a thermally throttled
  machine produces a smooth downward drift that looks exactly like a regression.
- **Never close, kill or suspend any of it yourself.** Those are the user's processes and
  quite possibly their work in progress - a VM mid-session, a build someone is waiting on.
  Report what you found and ask them to quiet the machine, or wait for it to finish, or
  benchmark anyway and say in the write-up what was running. Only work this session
  started itself is yours to stop.
- Record the top CPU consumers into the log itself immediately before AND after each run,
  so a contaminated run is identifiable afterwards rather than merely suspected:

  ```bash
  # Linux and macOS
  ps -Ao pid,pcpu,comm | sort -k2 -rn | head -15
  ```
  ```powershell
  # Windows
  Get-Process | Sort-Object CPU -Descending | Select-Object -First 15 Name, CPU
  ```

  Attributing a live compiler to a checkout tells you whether it is your own build:

  ```bash
  # Linux
  for pid in $(pgrep -x 'cc1plus|cc1|ld|mold|ninja|cmake|make'); do readlink /proc/$pid/cwd; done
  # macOS - no /proc, so ask lsof for the cwd descriptor
  for pid in $(pgrep -x 'clang|clang++|ld|ninja|cmake|make'); do
    lsof -a -p "$pid" -d cwd -Fn | sed -n 's/^n//p'
  done
  ```

  Never gate on loadavg: it lags ~60s both ways, reading quiet in the trough between build
  steps and busy after the machine went idle. Windows has no loadavg at all. Discard a
  contaminated run; do not salvage it.
- **Cold and warm caches will fool you.** The first run after a build reads the binary,
  the Qt libraries, the fonts and the profile tree off disk; every run after that gets
  them from the OS page cache. Startup and profile-load timings routinely differ
  several-fold between the two for reasons that have nothing to do with the change, and
  Mudlet warms its own caches as well - font metrics, `drawForeground()`'s cached pixmap,
  the map render cache. So: discard the first run of each side as a warm-up, alternate
  afterwards so both sides see the same cache state, and never compare a freshly built
  side against one that has already run ten times. If cold start is itself the subject,
  drop the caches deliberately between runs instead of hoping - Linux
  `sync; echo 3 | sudo tee /proc/sys/vm/drop_caches`, macOS `sudo purge`, Windows
  Sysinternals `RAMMap -Ew` - and say in the write-up which of the two you measured. All
  three need elevation and empty the cache for everything on the machine, not just your
  run, so ask the user before running one.
- Alternate A and B runs rather than all of one then all of the other, and give each run
  its own config root so profile state cannot differ between sides - a profile with
  packages installed runs their triggers against every benchmark line. Mudlet keeps that
  root at `~/.config/mudlet` on every platform (`mudlet::setupConfig`), so a private
  `HOME` isolates a run on Linux and macOS; `XDG_CONFIG_HOME` pointed at a directory with
  `mudlet/profiles` pre-created is what the test harness uses, and dropping a
  `portable.txt` beside the executable is the one that works everywhere, Windows included.
- An A/B ratio survives a measurement defect both sides share; absolute figures do not.
  Validate the harness before quoting an absolute number. A cloud or CI container is
  noisier still and may be CPU-throttled: quote only ratios from one.

## Find where the time goes

- Fixed-cost detector: a timing flat across a large range of input sizes is setup cost,
  not the algorithm. Two measurements at different sizes find it, no profiler needed.
- That detects; it does not diagnose. Identify by ablation - patch out the suspect,
  rebuild, remeasure - or by a profiler, never by code reading: a written, plausible
  explanation for one 80ms mapper cost was simply wrong; a ten-minute ablation found the
  real one. The test of a diagnosis is whether the figure was predicted before it was
  measured, not fitted afterwards.
- After removing one fixed cost, a still-flat timing usually means a second fixed cost was
  hiding behind the first - not that the fix failed.
- Measure end to end before investing in an isolated win, and once you know where the time
  goes, see the next section for what to do about it.

## Where the wins actually come from

Ranked by what has paid in this codebase. Mudlet is single-threaded by design - every
profile, every trigger and the Lua engine share the main thread, and only Qt's networking
runs off it - so "move it to a worker thread" is almost never available. The work has to
get smaller, not relocated.

1. **Delete a fixed cost.** Every large win so far was setup work repeated per operation,
   not a slow inner loop: A* rebuilt its whole graph on each call (56ms for a one-step
   walk, now 0.003-0.141ms) and a mapper view-fit recomputed what it could hoist (150ms to
   55ms). The fixed-cost detector above finds these in two measurements.
2. **Stop allocating.** Per-line churn dominates the text pipeline: one `std::deque` to
   `std::vector` swap cut allocations 38.6%. Look for containers rebuilt per line instead
   of `reserve()`d and reused, `QString`/`QList` copies that could be a `const&` or a
   `std::move`, implicit detaches from calling a non-const method on a shared container,
   and runtime-built literals that `qsl()` would fold into the binary.
3. **Index instead of scan.** A linear walk over every trigger, alias or room becomes a
   `QHash` lookup. Check the container's semantics while you are there: `QMultiMap::find`
   returns only the most recently inserted match, which is how duplicate names silently
   lost entries here - `equal_range` is the fix.
4. **Cache what is invariant, and name the event that clears it.** Font metrics, computed
   layout and rendered pixmaps all repay caching; a stale cache is a correctness bug, so
   pair every cache with the exact invalidation point rather than hoping. Caches also lie
   to benchmarks - see the cold/warm bullet above.
5. **Do less on the per-line path.** The line is the unit of work that scales with the
   game, so anything unconditional there is multiplied by traffic: 77 always-on starter-UI
   triggers made new-user throughput 2x-7.4x worse, and a single chat window made every
   captured line expensive. Making a per-line cost conditional, or moving it to the point
   of use, beats speeding it up. This is also why validation belongs in a comment, a test
   or a debug-only assert rather than a runtime check that every line pays for.
6. **Coalesce, but do not defer blindly.** Batching many small updates into one is fine.
   Postponing a single one usually is not: Qt already coalesces paints, and adding a
   deferral to Geyser updates bought nothing while breaking same-tick readback for
   scripts.

Not worth reaching for, on the evidence here:

- SIMD and hand-vectorised byte loops - 9.6x on the telnet decode loop in isolation, ~1%
  end to end.
- Removing "redundant" lookups from a hot loop when they hit a cache line the previous
  iteration already pulled in. Wins come from cold, scattered dereferences and allocation
  churn; "N redundant lookups per frame" is not, by itself, a cost model.
- Anything justified only by reading the code, however plausible it reads.

## In-tree benchmarks

Four live in `test/functional_tests/`, built alongside the functional tests into
`<build>/test/functional_tests/`:

- `PipelineBenchmark` (text pipeline, latin-1 decode, trigger engine, peak memory,
  default-package cost, display) is report-only: ctest runs it only when the tree was
  configured with `-DREGISTER_PERF_BENCHMARK=ON`, so plain `ctest` never will. For a
  before/after comparison run it through
  `test/compare-perf-baseline.py --run <before-binary> <after-binary>`, which reads the
  `METRIC` lines and gates on regressions.
- `MapRenderBenchmark` (2D mapper paint path) and `PathfindBenchmark` (A* speedwalk search
  plus the graph rebuild that feeds it) are never ctest-registered: each needs a real
  saved map, handed to it in `MUDLET_BENCH_MAP`, and skips without one:

  ```bash
  MUDLET_BENCH_MAP=/path/to/map.dat QT_QPA_PLATFORM=offscreen ./MapRenderBenchmark
  ```

  The binaries land in `<build>/test/functional_tests/` on every platform (`.exe` on
  Windows); the `VAR=value cmd` prefix form needs a shell that supports it, so on Windows
  run them from the MSYS2 shell rather than cmd or PowerShell.

  `compare-perf-baseline.py` understands MapRenderBenchmark's output too.
- `TelnetBenchmark` (small, medium and large payloads) is an ordinary grouped ctest case:
  `ctest -R TelnetBenchmark`.

When extending them:

- PipelineBenchmark's slots `QVERIFY(noTriggersAreRunningYet(host))` so a polluted profile
  fails loudly instead of silently benchmarking default-package triggers
  (`benchDefaultPackages` is the deliberate exception - default-package cost is its
  point). Keep that guard in new slots - the one slot that historically lacked it reported
  a +133% improvement that was really +46%. TelnetBenchmark has no such guard.
- Display measurement: `TTextEdit` takes its row count from its visible region, so resize
  the main window and read `getScreenHeight()` - resizing a pane alone leaves it clipped
  by its parents, and `getRowCount()` is a font-metric estimate, not the paint path.
  Scroll strides must exceed one screenful, or `drawForeground()` serves frames from its
  cached pixmap and inflates throughput several-fold.
- Vary the text every iteration when echoing in a loop: several Qt paths
  (`QLabel::setText` among them) short-circuit on identical input, and the benchmark then
  measures nothing at all.

## A/B against released versions

Every release back to at least 4.14 is still downloadable from
`https://www.mudlet.org/wp-content/files/` (the GitHub tags carry no release assets); that
directory is browsable, so check the exact name rather than guessing it, because the
naming changed over time:

- Linux: `Mudlet-<ver>-linux-x64.AppImage.tar` throughout, joined by
  `-linux-x64-portable.tar.gz` from 4.21.0.
- macOS: `Mudlet-<ver>.dmg` up to 4.18.x, then per-architecture `-x86_64.dmg` and
  `-arm64.dmg`.
- Windows: `-windows-installer.exe` up to 4.17.2, then `-windows-32-installer.exe` and
  `-windows-64-installer.exe`, joined by `-windows-64-portable.zip` from 4.21.0.

One harness drives them all: hand-build a profile directory, drop a `<MudletPackage>` XML
containing a ScriptPackage into `<profile>/current/` - its script body runs at profile
load - and launch with `--profile <name>`. Write results with `io.open` so nothing is
parsed from stdout; a `tempTimer(0, ...)` chain re-arming itself is a version-agnostic
event-loop pump, and its tick count over a fixed wall-clock window includes painting.
Traps:

- Seed identical window geometry through `Mudlet.ini` (`[General]` `pos`, `size`,
  `maximized` - unchanged since 4.17.2); each version otherwise picks its own default size
  and paint numbers are not comparable.
- Get at the binary without installing anything, so the two versions stay side by side and
  neither disturbs an installed Mudlet: on Linux, in a container without FUSE (the web
  sessions included) an AppImage will not launch directly - run it once with
  `--appimage-extract` and use the extracted `AppRun`. On macOS, `hdiutil attach` the dmg
  and copy `Mudlet.app` out, then `hdiutil detach`. On Windows, prefer the portable zip
  where the version has one; older releases ship only an installer, so those have to be
  installed into separate directories.
- Isolate each version's data: a `portable.txt` next to the executable (or beside
  `Mudlet.app`'s binary) pointing at a per-version directory is the cross-platform way,
  and it also keeps an older version from migrating or rewriting the real
  `~/.config/mudlet`.
- Every new profile from 5.0 on carries the starter UI, whose trigger package is real
  per-line cost. A version that never shipped it is therefore not comparable on a default
  profile - decide explicitly which configuration is being compared.

## Lua-only changes need no rebuild

`src/mudlet-lua` is read from disk at startup, so competing Lua-side variants can be A/B'd
against ONE binary: write a spec per competing behaviour first, swap each candidate file
over the product file, run the specs, restore, and record the pass/fail matrix. That turns
"which fix is right?" into a measurement.

Two platform catches:

- On Linux and Windows the source tree wins, so the swap takes effect immediately. On
  macOS it does not: `loadGlobal()` tries `<exe>/../Resources/mudlet-lua/...` first, and
  the build copies `mudlet-lua` into the `.app` bundle, so the bundled copy shadows your
  edit. Re-run `cmake --build` after each swap - it is only a file copy, seconds, not a
  rebuild - or edit the copy inside the bundle directly.
- `.claude/scripts/run-lua-tests.sh` is Linux-only: it drives the run under `xvfb-run` and
  leans on GNU `timeout`. Elsewhere set the same environment by hand and launch the binary
  windowed or with `-platform offscreen`; `src/mudlet-lua/tests/README.md` has the
  per-platform busted setup and the isolated-config-root invocation.
