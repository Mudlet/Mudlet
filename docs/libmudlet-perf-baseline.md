# libmudlet performance baseline

The libmudlet refactor (extracting Qt Widgets from `mudlet_core`, issue #9011)
carries a **"no more than 10% throughput loss"** gate. Absolute benchmark
numbers are specific to the machine that produced them, so **none are committed
here as a target**. What matters is running the benchmark on an *older* and a
*newer* Mudlet **on the same machine** and comparing the two. This document
explains how to do that.

## The harness

The harness is the `PipelineBenchmark` functional test
(`test/functional_tests/PipelineBenchmark.cpp`). It drives a real Mudlet profile
with a fixed, deterministically-generated corpus (mixed plain text, ANSI SGR
colour, UTF-8 and long wrapping-heavy prose) through the production
`cTelnet::processSocketData -> TBuffer::translateToPlainText -> TConsole ->
TriggerUnit` path via `cTelnet::loopbackTest()` - the same code an online
session runs (that path internally calls `TMainConsole::printOnDisplay()`, so the
Lua `feedTriggers()` entry point is covered too) - and measures:

- **text pipeline throughput** - no triggers active (`text_*` metrics)
- **trigger engine throughput** - the same corpus with a realistic ~34-trigger
  set active covering the plain substring, Perl regex with capture groups,
  begin-of-line substring, ANSI colour and multiline matcher kinds
  (`trigger_*` metrics). Lua-code matchers are deliberately excluded to keep
  Lua execution out of the timed path, and prompt triggers need a GA signal a
  loopback feed cannot produce.
- **peak resident set size** for the whole process, from `/proc/self/status`
  `VmHWM` on Linux (`peak_rss_kb`)

It is **report-only**: it makes no timing assertions (absolute speed varies
wildly between machines and CI runners) and always passes as long as the
pipeline actually processed data - which is genuinely asserted: each phase
verifies the console buffer filled to its scrollback cap, the trigger phase
verifies every trigger compiled and registered, and an untimed sentinel trigger
proves the trigger engine consumes what the loopback path feeds. A
silently-disconnected pipeline fails the run instead of reporting
impressive-looking garbage. Each phase feeds the corpus several times and
reports the **fastest single pass** - the least-disturbed pass isolates the
code's intrinsic speed from transient CPU contention, which is exactly what a
before/after comparison needs. That makes the numbers stable (~2% run-to-run)
even on a shared/CI box running other builds alongside it.

## How to run

`PipelineBenchmark` is **built as part of the functional tests but deliberately
not registered with ctest by default** - it is report-only and feeds a huge
corpus many times, so it would only burn minutes on every CI run. Run it
directly instead (which is exactly what the compare script does):

```bash
cd <build-dir>
# single run, human-readable + METRIC lines:
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
  ./test/functional_tests/PipelineBenchmark
```

If you want to drive it through ctest (e.g. `ctest -R PipelineBenchmark -V`,
which reuses the offscreen/ASAN env from CMake), configure the build with the
opt-in first:

```bash
cmake -S . -B <build-dir> -DREGISTER_PERF_BENCHMARK=ON
ctest --test-dir <build-dir> -R PipelineBenchmark -V
```

On a machine with other functional-test runs (e.g. parallel worktrees) wrap the
command in `flock /tmp/mudlet-functional-tests.lock ...` so stub ports and the
shared config directory do not collide.

Results are printed one per line as `METRIC <name> <value>`, so runs can be
diffed mechanically (the values below are illustrative, not a target):

```
METRIC build_asan 1
METRIC text_lines_per_sec 4281.46
METRIC text_mb_per_sec 0.41
METRIC trigger_lines_per_sec 3323.25
METRIC trigger_overhead_ms 1683.64
METRIC peak_rss_kb 1402384
...
```

## The before/after workflow (the 10% gate)

The gate is a **relative, same-machine** comparison. Never compare numbers taken
on different hardware, or from an ASan build against a release build - only ever
*old vs new on one machine, built the same way*.

1. **Build the "before" tree.** Check out the branch point (before the change
   under test), configure and build the functional tests, and keep that build
   directory.
   ```bash
   git worktree add ../mudlet-before <base-commit>
   cmake -S ../mudlet-before -B ../mudlet-before/build -G Ninja
   cmake --build ../mudlet-before/build --target PipelineBenchmark
   ```
2. **Build the "after" tree** the same way from your changed branch (e.g. the
   current `build/`).
3. **Run and compare** with the helper, which runs both binaries and prints the
   per-metric delta with a PASS/FAIL against the threshold (default 10%):
   ```bash
   flock /tmp/mudlet-functional-tests.lock \
     test/compare-perf-baseline.py --run \
       ../mudlet-before/build/test/functional_tests/PipelineBenchmark \
       build/test/functional_tests/PipelineBenchmark
   ```
   Or capture each run to a file and compare the files (handy when the two
   builds live on different checkouts or you want to keep a record):
   ```bash
   QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
     ../mudlet-before/build/test/functional_tests/PipelineBenchmark > before.txt
   QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
     build/test/functional_tests/PipelineBenchmark > after.txt
   test/compare-perf-baseline.py before.txt after.txt
   ```

`compare-perf-baseline.py` gates on `text_lines_per_sec` and
`trigger_lines_per_sec` by default (the two throughput numbers); every other
metric is reported for context. It exits non-zero if any gated metric regressed
by more than the threshold, so it drops straight into a script or CI step. Tune
it with `--threshold 0.10` and `--gate metric,metric,...`. A `--threshold` of 1
or more is read as a percentage (e.g. `--threshold 10` means 10%), with a note
on stderr; `--threshold 0` or a negative value is rejected.

Because it is the arbiter of the gate, the script refuses (exit code 2) rather
than silently passing whenever it cannot trust the comparison:

- an invariant (`text_corpus_lines`, `text_corpus_bytes`, `trigger_count`,
  `build_asan`) is missing from either run, or differs between them - the two
  runs used different corpora, trigger sets or build flavours. `build_asan`
  specifically stops an ASan build being compared against a release build.
- a **gated** metric is missing from either run, or its "before" value is not
  positive (a valid throughput/time baseline must be greater than zero).
- a `--gate` name matches no metric in either run (usually a typo).
- any `METRIC` line fails to parse fully - a non-numeric, NaN/Inf or
  comma-decimal value, or a duplicate metric name. Such a line is never dropped
  silently, because a vanished gated metric would otherwise let the gate pass.

**Gating on `trigger_overhead_ms` (opt-in).** Trigger throughput includes the
text-pipeline cost, which dilutes a matcher-only regression roughly 4x.
`trigger_overhead_ms` (trigger best pass minus text best pass - valid because
both phases feed identical bytes) isolates the matching engine itself. It is
**not gated by default**, though, because it is the difference of two
independently-noisy best passes: their noise adds, so its worst-case run-to-run
spread (~16%) is wider than the 10% gate and it would fire on noise alone. When a
change specifically targets trigger matching, gate on it explicitly and confirm
the movement is real - `--gate text_lines_per_sec,trigger_lines_per_sec,trigger_overhead_ms`,
ideally over a couple of runs or with a slightly relaxed threshold.

## Companion: Stressinator (live GUI display path)

`PipelineBenchmark` deliberately stops at the core pipeline: it runs offscreen
and never paints a widget, so it does not measure the on-screen rendering and
echo path. That path needs a live window and is covered by the **Stressinator
display benchmark** (`src/packages/StressinatorDisplayBench/`),
pre-installed into the `mudlet.org` self-test profile.

- Interactively, in a running profile, type `stresstest 100000` to feed that many
  lines of prose through `feedTriggers()` and print the average per-line time.
- In CI it runs automatically: `.github/workflows/performance-analysis.yml`
  launches Mudlet on a fixed self-hosted machine with
  `AUTORUN_DISPLAY_BENCHMARK=true` and appends the per-line result to a
  spreadsheet, tracking display throughput over time.

The two are complementary. Use `PipelineBenchmark` for a deterministic, headless,
CI-able check of the telnet -> buffer -> trigger core (the piece the libmudlet
refactor moves), and run Stressinator on a live build when you need to confirm
the rendering/echo path did not regress. Between them they cover the pipeline
from bytes-off-the-socket to pixels-on-screen.

## Illustrative example output (NOT a target)

The table below is **an example of one run on one machine, kept only to show the
shape and rough ratios of the output**. Do not treat any figure here as a target
or a committed baseline - capture your own "before" on the machine you are
testing on and compare against that.

| Metric | Example value |
| --- | --- |
| `text_lines_per_sec` | ~4,270 |
| `text_mb_per_sec` | ~0.41 |
| `text_best_pass_ms` | ~5,850 (25,000 lines/pass) |
| `trigger_lines_per_sec` | ~3,320 |
| `trigger_mb_per_sec` | ~0.32 |
| `trigger_best_pass_ms` | ~7,530 |
| `trigger_overhead_ms` | ~1,670 |
| `trigger_count` | 34 |
| `peak_rss_kb` | ~1,402,000 |

On that example run the realistic trigger set cost ~22% of text-pipeline
throughput (4,270 -> 3,320 lines/sec), and the run-to-run spread stayed around
2% - comfortably inside the 10% gate's noise budget. Your own machine will land
somewhere else entirely; that is expected and is exactly why the numbers are not
committed as canonical.

The example was captured on an AMD Ryzen 7 9800X3D / Ubuntu 24.04 / Qt 6.12.0 /
GCC 11.5 **ASan-instrumented, offscreen** functional-test build. ASan and the
offscreen platform dominate the absolute figures (a release build is far faster
and leaner, and `peak_rss_kb` is heavily inflated by ASan shadow memory), which
is another reason to read these only as relative, same-config references.

## Caveats

- Always compare **same machine, same build configuration**. The functional-test
  build turns AddressSanitizer on for non-Windows; comparing an ASan build to a
  release build, or across hardware, is meaningless.
- The whole corpus is fed as one `loopbackTest()` packet per pass rather than in
  network-sized chunks; this measures processing cost, not socket delivery.
- Always compare full-binary runs: `peak_rss_kb` (VmHWM) is process-wide and
  monotonic, so filtering to individual test slots changes what it means.
- All benchmark triggers sit at the root of the trigger tree; real profiles nest
  most triggers under parent folders, so root iteration is slightly overweighted
  relative to real workloads - irrelevant for a relative gate.
- Run order can bias results thermally: whichever binary runs second may execute
  on a warmer, throttled CPU, nudging its numbers down. When a comparison lands
  close to the threshold, re-run with the order swapped (or let the machine cool)
  before trusting a borderline verdict.
