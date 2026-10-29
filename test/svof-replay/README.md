# svof replay benchmark

Times a real trigger-heavy workload: the svof curing system (about 2,500
triggers in a tree six deep) running against a recorded Achaea session replayed over loopback telnet. Where
`PipelineBenchmark` measures the engine with a synthetic corpus and a small trigger set, this measures what a player
with a large system feels. Linux only.

## Inputs

Everything a run needs is in one file, `SvofBench.mpackage`, which is not in the repository:

- `svof.mpackage`, built for any class;
- svof's saved settings (the `config`, `defup+keepup`, `namedb`, `pipes`, `prios` and `rift+inv` folders of
  its `svo` folder), with the class set to the corpus's and svof unpaused, as a player does at the start of a
  session;
- the replay stream (`corpus.txt`), with other players' names and words replaced;
- `manifest.lua`: bundle version, class, prompt count, line count and the corpus's md5;
- `bench.lua` and `gate.lua`, and the loader for standalone mode.

svof's name database (`Database_namedb.db`) is left out. It holds real players' names, the scrubbed corpus no
longer mentions them, and runs without it repeat more closely.

| Variable | What |
| --- | --- |
| `SVOF_BENCH` | the `SvofBench.mpackage` to run |
| `SVOF_CACHE` | where `run.sh` unpacks it, default `~/.cache/svof-replay` |
| `SVOF_RUNS` | where run folders go, default `$TMPDIR/svof-replay` |
| `SVOF_SETTLE` | seconds svof gets to start before the replay, default 4 |

A run needs `Xvfb`, `flock`, `bc`, `unzip`, `md5sum` and `lua5.1` with LuaSocket.

### Building a package from your own log

```bash
lua5.1 promptify.lua session.log corpus.txt
lua5.1 make-package.lua --corpus corpus.txt --svof svof.mpackage --config <profile>/svo \
    --out SvofBench.mpackage [--names names.txt]
```

svof does almost nothing until it sees prompts, so the corpus must carry the game's protocol: an
`IAC GA` after every prompt and a GMCP `Char.Vitals` before it, plus a `Char.Status` at the start for the
class. `promptify.lua` adds these, and for a log captured with prompts gagged it inserts a synthetic prompt
every few lines. Without GA prompts svof's `getLines()` scans turn the replay quadratic.

`make-package.lua` then scrubs the log, deterministically, so the same inputs build the same file:

- names get a stand-in of the same shape, the same everywhere, including `Char.Status`: speakers, people
  told something, people acting at the start of a line, any capitalised word never seen in lower case,
  and the names listed one per line in `--names`;
- quoted speech, channels and tells typed at the prompt, and emotes seen only once are replaced with filler
  of the same shape: letters stay letters of the same case, digits stay digits, punctuation stays;
- line count, byte count, prompts, GA and GMCP are unchanged, and the build stops unless every changed
  line still matches the same svof exact, begin and substring patterns (and, with lrexlib-pcre installed,
  the same top-level regexes). Words svof's patterns use are never renamed.

`--no-scrub` packages the log as it is, for a package you keep to yourself. Building needs `zip` and
`unzip`.

## Running

```bash
SVOF_BENCH=SvofBench.mpackage test/svof-replay/run.sh before build-before/src/mudlet
```

Use a release build without sanitizers. The script takes the machine-wide benchmark lock
(`~/.cache/perf-bench.lock`), so concurrent runs queue rather than disturb each other. `SVOF_QPA=offscreen`
skips Xvfb, but then nothing paints.

Key output: `elapsed_s` is BEGIN to END marker as Mudlet saw it, `hb_max_ms` the longest event loop stall,
`maxrss_kb` peak memory. Compare builds by interleaving runs and taking medians: on one machine, single
runs can differ by several percent.

`bench.lua` sets `fixUnnecessaryLinebreaks`, as a real Achaea profile does, or every prompt grows a blank
line. With curing left to the server (`curemethod conconly`) svof sends nothing back, which
`client_sends 0` confirms.

## Standalone mode

Without the script, Xvfb or a log server: create a new profile, open it offline with
`mudlet --profile <name> --offline`, and install `SvofBench.mpackage` from the Package Manager. Four
seconds after svof installs, the package feeds the corpus to Mudlet with `feedTelnet()`, one 100,000-byte
chunk per event loop turn, then prints the same report as `run.sh` to the main window and writes it to
`SvofBench/result.txt` in the profile's folder. With `SVOF_BENCH_QUIT=1` in the environment, Mudlet closes
afterwards. Use a fresh profile for every run: svof keeps state, so a profile it has already run in does
different work, and the package refuses to start in one.

Standalone numbers are not interchangeable with `run.sh`'s: they skip Mudlet's socket reads, and the event
loop gets a turn after every chunk, so `elapsed_s` and the heartbeat figures differ. `run.sh` is the
reference; compare builds within one mode.

## Checking a run

The report ends with `gate pass`, or `gate FAIL:` and the reasons, and `run.sh` exits non-zero on a fail:

- `stop_reason` is not `end_marker`: the replay stalled or hit the sample limit;
- svof did not load, is paused, has a class other than the manifest's, or saw a prompt count other than the
  manifest's;
- `game_lines` is 0: no line of the corpus came through unchanged.

## Checking both builds did the same work

One run has nothing to compare to, so this part is yours. There is no exact identity hash: svof's
wall-clock timers write into the buffer. Check instead that:

- `svo_prompts` is identical, and equals the manifest's prompt count;
- `client_sends` is 0;
- `game_lines` (corpus lines that came through unchanged) agrees within 1%, and `added_lines` (svof's own
  output) is close. `run.sh` keeps the buffer in `result.txt.buffer` for a closer look.
