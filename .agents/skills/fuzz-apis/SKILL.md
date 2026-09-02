---
name: fuzz-apis
description: >-
  Fuzz Mudlet's parsers and scripting API for crashes and undefined behaviour: build a
  sanitizer binary, run the seeded in-process fuzzers with fresh seeds, extend them to new
  surfaces, triage what falls out. Read before triaging any sanitizer report.
license: GPL-2.0-or-later
user-invocable: true
argument-hint: Optional surface to fuzz (e.g. "GMCP handlers", "the buffer API")
---

Runs out of the box on Claude Code for the web, whose containers are Linux (the
SessionStart hook provisions the toolchain); elsewhere read the `build-mudlet` skill
first. Commands below default to Linux; where macOS or Windows differs it is called out
inline.

## Procedure

1. **Build a sanitizer binary**: `cmake --preset linux-debug` for AddressSanitizer;
   `linux-debug-ubsan` for a second pass that catches what ASan cannot (loads of invalid
   bool and enum values - including from uninitialised memory, see the gotchas below -
   misaligned access, integer overflow). `macos-debug` and `macos-debug-ubsan` are the
   same pair on macOS. **Windows cannot fuzz**: `src/CMakeLists.txt` skips
   `EnableSanitizers.cmake` under `WIN32` ("Sanitizers disabled on Windows" in the
   configure output), so a Windows build reports nothing however hard you fuzz it - do
   this work on Linux or macOS, and confirm a Windows-specific suspicion by other means.
2. **Run the existing fuzzers with a handful of fresh seeds** (invocation below). Vary the
   seed, not just the iteration count - each seed explores a different path prefix.
3. **On an abort, triage it** (rules below): replay the seed, reduce the recorded input,
   search the issue tracker - some seeds reach bugs that are still open by design, so an
   abort is a finding to check, not a broken harness. A new find is the deliverable: file
   it with the minimal repro, or fix it and open a PR (via the `open-pr` skill) carrying a
   regression spec.
4. **All clean? That is a result too**, and the explored space is finite - extending beats
   re-rolling seeds forever. Extend a generator (new option sequences, new API calls) or
   aim at a fresh surface, with the user picking where; candidates that have never been
   fuzzed: deeper GMCP/MSDP dispatch, the map file loaders on corrupt input, package
   installation on malformed archives, OSC 8 hyperlink parsing. Write it with the pattern
   below.
5. **Report what ran either way**: sanitizer, seeds, iteration counts, and what fell out -
   a clean run over a stated space is information the next run builds on.

## The approach

Mudlet needs no libFuzzer or AFL harness: the busted self-test profile runs the whole real
application headless, so a spec can feed the real parsers directly. A fuzzer here is an
ordinary spec with three properties:

- every random choice derives from a seeded PRNG, so any run replays byte-for-byte;
- it records each input before feeding it, so after an abort the tail of the dump IS the
  crashing input;
- it runs against a sanitizer build, which turns silent corruption into an abort at the
  faulting line.

## The existing fuzzers

`src/mudlet-lua/tests/TelnetTriggerFuzz_spec.lua` (telnet parser + trigger engine) and
`BufferManipFuzz_spec.lua` (console text and selection API, deliberately out-of-range
indices). They arrive with PR #10221, together with a "Running the fuzzers" section in
`src/mudlet-lua/tests/README.md` - if neither is in your tree yet, fetch them from that
PR. Both bail out unless `MUDLET_FUZZ=1` is set, so normal runs and CI skip them. Seed,
iteration counts and dump path are environment variables listed in each spec's header
comment.

```bash
# point at the binary the preset actually built: build/src/mudlet for
# linux-debug, build-linux-debug-ubsan/src/mudlet for the ubsan preset
MUDLET_FUZZ=1 MUDLET_FUZZ_SEED=7 .claude/scripts/run-lua-tests.sh build/src/mudlet
```

That runner is Linux-only (`xvfb-run`, GNU `timeout`). On macOS set the same environment
by hand and launch the bundle's binary
(`build-macos-debug-ubsan/src/mudlet.app/Contents/MacOS/mudlet`) windowed or with
`-platform offscreen`; `src/mudlet-lua/tests/README.md` carries the per-platform busted
setup and the isolated-config-root invocation.

Calibration: by 2026-08 the telnet generator had run ~48k feeds and ~19k compiled patterns
across 6 seeds clean under ASan - that surface is well explored at default settings.

## Sanitizer gotchas

- UBSan's "load of value N, which is not a valid value for type 'bool'" fires only when the
  leftover memory happens to be non-boolean: it is nondeterministic run to run, and a
  report VANISHING is not proof of a fix - unrelated changes shift heap layout and silence
  it by luck. `MALLOC_PERTURB_=1` fills fresh allocations with 0xfe and makes it fire every
  run. That works because plain UBSan keeps glibc's malloc; ASan replaces it, so use the
  ubsan preset for this. `MALLOC_PERTURB_` is a glibc feature and does nothing on macOS;
  libmalloc's equivalents are `MallocPreScribble=1` (0xaa into fresh allocations) and
  `MallocScribble=1` (0x55 into freed ones), likewise ignored under ASan.
- Verify a fix by grepping the run for the SYMBOL under test, not the file:line - the fix
  itself shifts line numbers.
- Under ASan, a run where every spec printed green but the runner still exited non-zero is
  usually a LeakSanitizer report at process exit, not a fuzz abort - check the output tail
  for `LeakSanitizer` before hunting a lost crash dump.

## Writing a new fuzzer

- PRNG: Lua 5.1 has no integer or bit operations; MINSTD (`s = (16807 * s) % 2147483647`)
  stays exact in a double. Take the seed from an environment variable and print it.
- `feedTelnet()` C-string-truncates on a raw NUL and decodes `<...>` escapes: build input
  with `string.char`, escaping only NUL as `<00>`, `<` as `<<` and `>` as `>>`.
- Gate the spec behind `MUDLET_FUZZ` like the existing ones. Fuzz specs mutate persistent
  state (telnet negotiation, encodings), so they must not join the always-on suite.
- Aim at state, not just parsing. The telnet byte parser itself came back clean; the real
  findings clustered in surrounding state machinery: writing to a console emptied by
  `deleteLine()` crashed three separate paths (#10207), `Tree<T>` construction invoked UB
  on every trigger/alias/timer ever created (#10227), a pattern-row flag was read
  uninitialised (#10228), and profile load reset the telnet session before Host's members
  existed (#10229).

## Triage

- Reduce before reporting: replay the seed, shrink the recorded input, and confirm the
  minimal case aborts on a fresh run.
- Measure "unbounded" claims before filing them. Cautionary example: `TTrigger::match`
  passes a null match context to `pcre2_match`, which reads like unbounded ReDoS
  backtracking - empirically PCRE2 applies its default MATCH_LIMIT (10 million steps) even
  with a null context, and `(a+)+$` on adversarial input plateaus at ~9-15ms. The
  code-reading conclusion was wrong; the measurement settled it.
- File with an empirical repro, never from code reading, and search the issue tracker
  first - several crashes a fuzzer can reach are already known.
