---
name: improve-test-coverage
description: >-
  Raise Mudlet's C++ test coverage to a reasonable level: measure line coverage, pick
  targets that pay with the user, write specs that reach C++, prove each new test bites.
  Read before writing tests for any subsystem.
license: GPL-2.0-or-later
user-invocable: true
argument-hint: Optional subsystem or file to cover (e.g. "ctelnet", "the mapper")
---

Runs out of the box on Claude Code for the web, whose containers are Linux (the
SessionStart hook provisions the toolchain); elsewhere read the `build-mudlet` skill
first. Commands below default to Linux; where macOS or Windows differs it is called out
inline. For busted mechanics, see `src/mudlet-lua/tests/README.md`.

## Procedure

1. **Measure before writing.** Folklore about what is untested goes stale: at the first
   real baseline (2026-08), two of three "known gaps" were already covered. Build one
   instrumented tree, run BOTH harnesses on it, then rank (recipe below).
2. **Pick a target that pays.** If the user named a subsystem, cover that. Otherwise show
   them the top of the uncovered-mass ranking and let them pick. Skip what does not pay
   (list below).
3. **Write specs first.** A busted spec is the default; a C++ functional test needs one of
   the specific justifications in `CLAUDE.md`'s Tests section. Specs reach further than
   they look (below).
4. **Prove every test red before trusting it green** (see "Prove the test tests
   something"). Both harnesses fail silently when set up wrong - a misconfigured run is
   green.
5. **Stop at reasonable** (see "Know when to stop"). Re-run gcovr and record before/after
   for the files touched.
6. **Deliver**: a pull request via the `open-pr` skill, quoting the coverage
   before/after. Bugs found along the way are filed separately, each with an empirical
   repro - a probe that fails on one side of the change and passes on the other - never
   from code reading alone: a second code path routinely compensates for the defect a
   reading predicts.

## Measuring

Needs `gcovr` and `jq` (apt on Linux, `brew` on macOS; the web containers get them from
the SessionStart hook).

**Measure on Linux.** The recipe below is gcc + gcov, and the ranking it produces is
platform-independent - the code Mudlet compiles is very nearly the same everywhere, so
there is nothing to gain from a second platform's numbers and quite a lot of yak-shaving
to lose. If you are on macOS anyway, the same recipe works with `macos-debug-nosan`
provided gcovr is told to use clang's gcov shim
(`gcovr --gcov-executable "llvm-cov gcov" ...`); mixing its output with a gcc run's is
meaningless, so pick one and stay there. Windows is unexplored: MSYS2 has both compilers,
so `--coverage` may well work, but nobody has run it - do not present a Windows number as
a baseline without first proving the `.gcda` files appear.

```bash
# gcov instrumentation wants gcc, the Linux default. See build-mudlet for parallelism limits.
cmake --preset linux-debug-nosan -B build-coverage \
  -DCMAKE_C_FLAGS=--coverage -DCMAKE_CXX_FLAGS=--coverage \
  -DCMAKE_EXE_LINKER_FLAGS=--coverage
cmake --build build-coverage

# run-lua-tests.sh is Linux-only (xvfb-run, GNU timeout); see
# src/mudlet-lua/tests/README.md for the per-platform equivalent.
.claude/scripts/run-lua-tests.sh build-coverage/src/mudlet
(cd build-coverage && ctest --output-on-failure)

# Restrict the search paths as below: pointed at the whole build tree, gcovr
# trips over oniguruma's gperf-generated sources and aborts.
gcovr -r . build-coverage/src/CMakeFiles/mudlet_core.dir \
           build-coverage/src/CMakeFiles/mudlet_executable.dir \
      --csv coverage.csv --json coverage.json
```

Reading the numbers:

- gcov "lines" are instrumented lines, roughly half the physical line count, and the
  branch percentage is dragged down by compiler-generated branches. Use both only
  relatively.
- Check that `.gcda` files exist under `build-coverage/test/` before trusting numbers for
  code only the standalone test binaries exercise; runs have been observed where they
  deposited none, leaving such code looking exercised only as far as specs reached it.
- Rank files by uncovered-line mass, not percentage:
  `lua .agents/skills/improve-test-coverage/rollup.lua coverage.csv` writes `rollup.md`
  with a per-subsystem rollup and a per-file table sorted by uncovered mass.
  `.agents/skills/improve-test-coverage/check-lines.sh coverage.json src/Foo.cpp 100 200`
  answers "is this specific path covered" from the JSON export.

## Know when to stop

Complete coverage is not the goal - a reasonable level is. Chasing the last percent
produces brittle tests that mirror the implementation, costing more in maintenance than
they catch.

- A test must pin behaviour someone could plausibly break. If the strongest assertion you
  can write restates the code, do not write it.
- Poor value per uncovered line, skip: the 3D mapper and `T2DMap` paint paths, the
  zero-coverage `dlg*` dialogs, `CustomLine` mouse handlers - all driven by GUI events
  that tests cannot cheaply synthesise.
- Prefer breadth across load-bearing subsystems over depth in one file.
- Calibration, from the 2026-08 baseline: `src/` was 52.6% lines overall - specs alone
  gave 36.1%, functional tests another 16.5 points. The weakest heavily-used file was
  `ctelnet.cpp` at 44%; one wave of specs plus a functional-test group took it to 67.5%,
  and that was a good place to stop.

## Reaching C++ from a spec

The self-test profile runs the real application, so specs reach much further than they
look:

- `feedTelnet()` drives the real telnet parser (`cTelnet::processSocketData`): IAC
  negotiation, subnegotiation (GMCP/MSDP/MSSP/CHARSET), ANSI/CSI/OSC, encodings. It needs
  the profile loaded with `--offline`, which `run-lua-tests.sh` already passes.
- `feedTriggers()` exercises the trigger engine end to end. It is a local feed
  (`isFromServer` false), so server-only paths such as incomplete-sequence carry-over need
  `feedTelnet()` instead.
- `pumpEvents(ms)` makes deferred slots and timer work observable within one spec.

Still unreachable even offline: bytes Mudlet SENDS (there is no socket to observe), and
`connectIt()`'s Host-to-cTelnet config sync - a spec that toggles such config must verify
the toggle actually bites.

Sandbox gotcha: busted sandboxes each file, so a counter shared across callbacks must be
`_G.`-qualified or it silently reads zero and the assertion goes vacuously green.

## Functional tests, when a spec cannot reach

- Join a `*_GROUP_TEST_SOURCES` group in `test/functional_tests/CMakeLists.txt` (the lists
  there document how); a standalone binary costs ~250MB in every build tree.
- Insert the new filename at a random position in the source list, not at the bottom -
  parallel branches appending to the same last line all merge-conflict.
- Bind `TelnetServerStub` to port 0 and read `serverPort()` back; hardcoded ports collide
  when suites run in parallel.
- Register the bundled fonts from Qt resources in `initTestCase()` and assert
  `QFontInfo(font).family()` equals what was requested. `QTEST_MAIN` never runs `main()`,
  so the fonts it would install are absent and Qt substitutes per-platform - metrically
  identical on Linux, proportional on macOS and Windows, so a metrics test passes on one
  platform and fails on the rest.
- Take verdicts from ctest's own Passed/Failed summary: QTest can print every case as PASS
  and the process still exit non-zero (a LeakSanitizer report, for one).
- moc trap: a raw string literal containing `"` or `]]` can make moc emit an empty `.moc`,
  surfacing at link time as `undefined symbol: vtable for <TestClass>`. Use escaped string
  literals in files moc parses.

## Prove the test tests something

- Fail-without-fix, always: sabotage the code the test claims to cover and watch it go
  red. Cut EVERY mechanism that could produce the behaviour, not just the obvious one - a
  second path can keep the behaviour correct and make the test look wrong instead.
- Pin preconditions: assert the state is NOT already the target before acting, then act,
  then assert. A field with a plausible default satisfies the final assertion without the
  code under test ever running.
- After a C++ sabotage leg, rebuild once the source is restored - restoring the source
  does not restore the binary, and a stale tree then fails exactly the new tests, which
  reads like a real regression.
