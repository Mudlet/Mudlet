---
name: verify-mudlet
description: >-
  Run Mudlet headless to check a change: the busted specs (all, some, or an ad hoc probe spec), the
  same specs on the merge base and this checkout to see which tests changed outcome, or a recorded
  game session replayed into both builds to diff the console, sends, events and Lua errors. Linux,
  macOS and Windows (MSYS2). Read before running specs or claiming a change behaves as intended.
license: GPL-2.0-or-later
---

## When to use

- After a change, before calling it done: run the specs it touches, then `diff` against the base.
- To check a claim about behaviour (a review finding, a bug report): write a probe spec and run it.
- To show a new test bites: `diff specs path/to/new_spec.lua` must show it failing on the base.

Everything runs in a throwaway profile with its own HOME and config directory, under Xvfb on
Linux (or `--offscreen`), with the test fixtures CI uses. It never touches your real profiles.
Results are a checking aid, not proof: QA still happens on top.

## Commands

```bash
S=.agents/skills/verify-mudlet/scripts/verify-mudlet.sh
$S specs                                  # whole suite
$S specs Mapper_spec.lua /tmp/probe_spec.lua
$S diff specs                             # base's suite on both sides; what changed outcome
$S diff specs src/mudlet-lua/tests/New_spec.lua   # fails on base, passes here?
$S diff replay                            # svof + a recorded Achaea session, both sides
$S diff replay --log session.raw --package my.mpackage
```

`--help` lists every option. Exit status: 0 clean, 1 failures or differences, 2 harness problem,
3 Mudlet died before the specs finished, 124 timeout. Output is a short summary; everything else
(log, `results.tsv` with one line per test, failure tracebacks, replay streams and diffs) is under
`--out DIR`, or a temp directory the summary names.

## What it does for you

- **Binary**: this checkout's build (`build-<platform>-debug-nosan`, then `build`, ...), or
  `--binary`. A binary from another worktree is fine for a Lua-only change: it is staged so that it
  runs *this* checkout's `src/mudlet-lua`, and `MudletBusted_spec.lua` confirms the right copy
  loaded. Its C++ and its compiled-in resources (`src/packages`, `utf8_filenames.lua`) are still
  whatever it was built from.
- **Crash-safe results**: each test's outcome is written the moment it ends, so a crash or a
  timeout still says which test it happened in.
- **Base side** (`diff`): the merge base with `<Mudlet remote>/development`, or `--base REF`.
  When nothing since the base reaches the binary, the base side is the same binary running the
  base's Lua - no build. Otherwise the base is built once into a reusable worktree at
  `$(git rev-parse --git-common-dir)/mudlet-verify-base` with the head build's Qt; set
  `MUDLET_VERIFY_JOBS` for its job count (default 8; it goes through `build-slot` when that is
  installed), or pass `--base-binary` to skip it.
- **Noise**: `diff specs` re-runs every spec file with a difference and keeps only differences
  that reproduce (the rest go to `flaky.tsv`). `diff replay` runs each side twice and compares
  them as sets of lines: a line counts as changed only when both runs of each side agree and the
  sides disagree. Lines that vary between runs of one side are compared with their numbers ignored.

## Writing a probe spec

A probe is an ordinary busted file ending in `_spec.lua`, anywhere on disk. It runs inside the
offline self-test profile, so the whole Lua API is there; `feedTelnet()` plays game output in,
`pumpEvents(ms)` lets timers and queued events run. Assert what the claim says should happen.
Lua errors raised by scripts outside a test (timers, event handlers) don't fail it, but appear in
the log as `<profile>.errors_<profile>| ...` lines.

## Replay

`diff replay` feeds the session synchronously in 100 KB chunks, so both sides see identical
input timing. It records every console line with its colours (`--plain` for text only, faster),
everything scripts sent with the line it happened on, every event raised with its arguments, and
the Lua errors. A changed line is a behaviour change somewhere between the base and this checkout;
whether it is the intended one is for you to judge. Each change prints as the base line, then the
head line that replaced it. Lines that differ only in their digits are listed separately and do not
fail the run: script timings land there (svof prints hundreds), but so would a real numeric change.
A line that only moved is not reported. Without `--log` it uses the svof benchmark bundle
(downloaded once, checksum-verified; see `test/svof-replay/README.md`), about two minutes for the
four runs. A replay only exercises what the session's scripts actually reach: svof shows a change
to `decho`, but not one to `table.size` or `string.title`.

## Platform notes

- **macOS**: run from the shell that can open GUI apps; the binary must be inside its `.app`.
  The staged copy of a bundle is an APFS clone, so it costs no space.
- **Windows**: an MSYS2 CLANG64 shell, with a Windows `python`. Only the HTTP fixture runs (as on
  CI), so specs that need the MMCP, telnet or Discord fixtures pend. A borrowed binary is staged by
  copying its DLLs and Qt plugin folders beside a copy of the exe.
- A `portable.txt` beside the binary would redirect its config, so such a binary is staged without it.
