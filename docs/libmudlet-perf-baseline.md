# libmudlet performance baseline

The libmudlet refactor (extracting Qt Widgets from `mudlet_core`, issue #9011)
carries a **"no more than 10% throughput loss"** gate. That gate is
unenforceable without a committed baseline, so this document records one and
explains how to reproduce and compare it.

The harness is the `PipelineBenchmark` functional test
(`test/functional_tests/PipelineBenchmark.cpp`). It drives a real Mudlet profile
with a fixed, deterministically-generated corpus (mixed plain text, ANSI SGR
colour and UTF-8) through the production
`cTelnet::processSocketData -> TBuffer::translateToPlainText -> TConsole ->
TriggerUnit` path via `cTelnet::loopbackTest()` - the same code an online
session runs - and measures:

- **text pipeline throughput** - no triggers active (`text_*` metrics)
- **trigger engine throughput** - the same corpus with a realistic ~34-trigger
  set active covering every matcher kind: plain substring, Perl regex with
  capture groups, begin-of-line substring, ANSI colour and multiline
  (`trigger_*` metrics)
- **peak resident set size** for the whole process, from `/proc/self/status`
  `VmHWM` on Linux (`peak_rss_kb`)

It is **report-only**: it makes no timing assertions (absolute speed varies
wildly between machines and CI runners) and always passes as long as the
pipeline actually processed data. Each phase feeds the corpus several times and
reports the **fastest single pass** - the least-disturbed pass isolates the
code's intrinsic speed from transient CPU contention, which is exactly what a
before/after comparison needs. That makes the numbers stable (~2% run-to-run)
even on a shared/CI box running other builds alongside it.

## How to run

```bash
cd <build-dir>
# single run, human-readable + METRIC lines:
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
  ./test/functional_tests/PipelineBenchmark

# or via ctest (uses the same offscreen/ASAN env from CMake):
ctest -R PipelineBenchmark -V
```

On a machine with other functional-test runs (e.g. parallel worktrees) wrap the
command in `flock /tmp/mudlet-functional-tests.lock ...` so stub ports and the
shared config directory do not collide.

Results are printed one per line as `METRIC <name> <value>`, so runs can be
diffed mechanically:

```
METRIC text_lines_per_sec 6470.39
METRIC text_mb_per_sec 0.37
METRIC trigger_lines_per_sec 4610.18
METRIC peak_rss_kb 1323136
...
```

## How to compare (the 10% gate)

1. Capture a "before" run on the branch point (this baseline) **on the machine
   you will test on**, with the **same build configuration** (see caveats).
2. Capture an "after" run of the refactored build on the **same machine**.
3. For each throughput metric: `after / before` must be `>= 0.90` (no more than
   10% loss). `peak_rss_kb` should not grow materially either.

Always compare same-machine, same-config. The absolute numbers below are only
meaningful as a relative reference on identical hardware and build flags.

## Committed baseline

Captured 2026-07-25, best pass of 6 per phase, mean of 3 back-to-back runs.

| Metric | Value |
| --- | --- |
| `text_lines_per_sec` | ~6,440 |
| `text_mb_per_sec` | ~0.37 |
| `text_best_pass_ms` | ~3,880 (25,000 lines/pass) |
| `trigger_lines_per_sec` | ~4,600 |
| `trigger_mb_per_sec` | ~0.26 |
| `trigger_best_pass_ms` | ~5,430 |
| `trigger_count` | 34 |
| `peak_rss_kb` | ~1,322,000 |
| `text_corpus_lines` | 25,000 |
| `text_corpus_bytes` | 1,436,934 |

The realistic trigger set costs ~29% of text-pipeline throughput
(6,440 -> 4,600 lines/sec), so the trigger metric is sensitive enough to catch a
regression in the matching engine.

Run-to-run spread across the 3 runs (max-min over mean): `text_lines_per_sec`
2.3%, `trigger_lines_per_sec` 2.1%, `peak_rss_kb` 0.2% - all far inside the 10%
gate's noise budget.

### Machine and build context

| | |
| --- | --- |
| CPU | AMD Ryzen 7 9800X3D (8C/16T) |
| RAM | 62 GiB |
| OS / kernel | Ubuntu 24.04.4 LTS / 6.8.0-134-generic |
| Qt | 6.12.0 |
| Compiler | GCC 11.5.0 |
| Base commit | `dabd95df3` (origin/development) |
| Build | functional-test build, **AddressSanitizer enabled**, `QT_QPA_PLATFORM=offscreen` |

**Caveats.**

- These are **ASan-instrumented, offscreen** numbers - what the functional-test
  build produces (`EnableSanitizers.cmake` turns ASan on for non-Windows). ASan
  and the offscreen platform dominate the absolute figures; a release build is
  far faster and uses far less memory. `peak_rss_kb` in particular is heavily
  inflated by ASan shadow memory. None of that matters for the gate, which is a
  **relative** same-config comparison - but do not read these as real-world
  throughput.
- The whole corpus is fed as one `loopbackTest()` packet per pass rather than in
  network-sized chunks; this measures processing cost, not socket delivery.
- If you re-baseline on different hardware or build flags, replace the whole
  table above - never compare across configurations.
