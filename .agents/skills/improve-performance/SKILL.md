---
name: improve-performance
description: >-
  Make Mudlet faster: pick a subsystem with the user, measure, find the real cost, fix it
  and prove the win. Read before benchmarking anything or evaluating a performance claim.
license: GPL-2.0-or-later
user-invocable: true
argument-hint: Optional subsystem to speed up (e.g. "the 2D mapper", "triggers")
---

Runs out of the box on Claude Code for the web (the SessionStart hook provisions the
toolchain); elsewhere read the `build-mudlet` skill first.

## Procedure

1. **Pick the battlefield with the user.** If they named a subsystem, that is the target;
   otherwise ask them to pick rather than choosing silently. Proven-fertile ground: telnet
   + text pipeline throughput, the trigger engine, 2D mapper rendering, map pathfinding,
   profile load and startup time. "You pick" is also an answer: profile a busy session and
   take the top entry.
2. **Build a benchmark-safe tree**: the `linux-debug-nosan` preset. Never benchmark a
   sanitizer build - ASan (the default `linux-debug` preset) roughly halves text
   throughput.
3. **Establish a baseline** with an in-tree benchmark (below), or a small harness
   following the patterns below. Record the exact invocation - an unreproducible number is
   worthless.
4. **Find where the time goes**: detect by shape, identify by ablation or profiler, never
   by code reading (rules below).
5. **Fix and re-measure A/B.** Keep the fix only if the end-to-end number moves, not just
   the microbenchmark.
6. **Prove no behaviour change** - run the busted specs
   (`.claude/scripts/run-lua-tests.sh`) and `ctest` - then open a pull request with the
   `open-pr` skill, quoting before/after numbers and the exact invocation so a reviewer
   can reproduce them.

## Keep the numbers honest

- Attribute every live compiler on the machine immediately before AND after each run, into
  the log itself (Linux; on macOS substitute `lsof -p <pid> | grep cwd` for the `/proc`
  read):

  ```bash
  for pid in $(pgrep -x 'cc1plus|cc1|ld|mold|ninja|cmake|make'); do readlink /proc/$pid/cwd; done
  ```

  Never gate on loadavg: it lags ~60s both ways, reading quiet in the trough between build
  steps and busy after the machine went idle. A concurrent build roughly doubles every
  timing, and one that comes and goes mid-run leaves a plausible-looking number rather
  than an obviously broken one. Discard a contaminated run; do not salvage it.
- Alternate A and B runs rather than all of one then all of the other, and give each run a
  private `HOME` so profile state cannot differ between sides - a profile with packages
  installed runs their triggers against every benchmark line.
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
- Removing hot-loop lookups buys nothing when they hit a cache line the previous iteration
  pulled in; wins come from cold, scattered dereferences and allocation churn. "N
  redundant lookups per frame" is not, by itself, a cost model.
- After removing one fixed cost, a still-flat timing usually means a second fixed cost was
  hiding behind the first - not that the fix failed.
- Measure end to end before investing in an isolated win: SIMD gave 9.6x on the telnet
  byte loop in isolation and ~1% on real traffic.
- Calibration from past wins: a 56ms flat pathfinding setup cost became 0.14ms; a mapper
  view-fit went 150ms to 55ms; per-line allocations dropped 38.6% from one deque-to-vector
  swap. All were fixed costs or churn; none were hot-loop micro-optimisation.

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
`https://www.mudlet.org/wp-content/files/` as `Mudlet-<version>-linux-x64.AppImage.tar`
(the GitHub tags carry no release assets). One harness drives them all: hand-build a
profile directory, drop a `<MudletPackage>` XML containing a ScriptPackage into
`<profile>/current/` - its script body runs at profile load - and launch with
`--profile <name>`. Write results with `io.open` so nothing is parsed from stdout; a
`tempTimer(0, ...)` chain re-arming itself is a version-agnostic event-loop pump, and its
tick count over a fixed wall-clock window includes painting. Traps:

- Seed identical window geometry through `Mudlet.ini` (`[General]` `pos`, `size`,
  `maximized` - unchanged since 4.17.2); each version otherwise picks its own default size
  and paint numbers are not comparable.
- In a container without FUSE (the web sessions included) an AppImage will not launch
  directly: run it once with `--appimage-extract` and use the extracted `AppRun`.
- Seeding any `Mudlet.ini` key also suppresses the 5.0 starter UI. That is what you want
  against versions that never had it, but the starter UI's trigger package is real
  per-line cost for a default new profile - decide explicitly which configuration is being
  compared.

## Lua-only changes need no rebuild

`src/mudlet-lua` is read from disk at startup on Linux, so competing Lua-side variants can
be A/B'd against ONE binary: write a spec per competing behaviour first, swap each
candidate file over the product file, run `.claude/scripts/run-lua-tests.sh`, restore, and
record the pass/fail matrix. That turns "which fix is right?" into a measurement.
