# svof replay benchmark

Times a real trigger-heavy workload: the svof curing system (about 2,500
triggers in a tree six deep) running against a recorded Achaea session replayed over loopback telnet. Where
`PipelineBenchmark` measures the engine with a synthetic corpus and a small trigger set, this measures what a player
with a large system feels. Linux only.

## Inputs

None of these are in the repository: a session log carries other players' names and tells.

| Variable | What |
| --- | --- |
| `SVOF_CORPUS` | the replay stream, built from a plain log with `promptify.lua` (below) |
| `SVOF_PACKAGE` | `svof.mpackage`, built for any class |
| `SVOF_CONFIG` | the `svo` folder of a profile svof has run in, for its settings |
| `SVOF_NAMEDB` | optional `Database_namedb.db` from the same profile |
| `SVOF_PLAIN` | optional plain log the corpus was built from, turns on the identity gate |
| `SVOF_CLASS` | class written into svof's config, default `Infernal`; must match the corpus |
| `SVOF_RUNS` | where run folders go, default `$TMPDIR/svof-replay` |

The run also needs `Xvfb`, `flock`, `bc` and `lua5.1` with LuaSocket.

### Building the corpus

```bash
lua5.1 promptify.lua session.log corpus.txt
```

svof does almost nothing until it sees prompts, so the corpus must carry the game's protocol: an
`IAC GA` after every prompt and a GMCP `Char.Vitals` before it, plus a `Char.Status` at the start for the
class. `promptify.lua` adds these, and for a log captured with prompts gagged it inserts a synthetic prompt
every few lines. Without GA prompts svof's `getLines()` scans turn the replay quadratic.

`bench.lua` sets `fixUnnecessaryLinebreaks`, as a real Achaea profile does, or every prompt grows a blank
line. `run.sh` unpauses svof's saved config, as a player does at the start of a session; with curing left
to the server (`curemethod conconly`) svof then sends nothing back, which `client_sends 0` confirms.

## Running

```bash
SVOF_CORPUS=... SVOF_PACKAGE=... SVOF_CONFIG=... SVOF_PLAIN=... \
    test/svof-replay/run.sh before build-before/src/mudlet
```

Use a release build without sanitizers. The script takes the machine-wide benchmark lock
(`~/.cache/perf-bench.lock`), so concurrent runs queue rather than disturb each other. `SVOF_QPA=offscreen`
skips Xvfb, but then nothing paints.

Key output: `elapsed_s` is BEGIN to END marker as Mudlet saw it, `hb_max_ms` the longest event loop stall,
`maxrss_kb` peak memory. Compare builds by interleaving runs and taking medians: on one machine, single
runs can differ by several percent.

## Checking a run

`run.sh` prints the result, then exits non-zero when:

- `stop_reason` is not `end_marker`: the replay stalled or hit the sample limit;
- `svo_loaded` is not `true`, `svo_paused` not `false`, or `svo_class` not `SVOF_CLASS`: the package did not
  install, or `SVOF_CONFIG` holds a class or pause setting `run.sh` could not rewrite;
- with `SVOF_PLAIN` set, `gate.lua` cannot read the buffer or the plain log, or `game_lines` is 0.

## Checking both builds did the same work

One run has nothing to compare to, so this part is yours. There is no exact identity hash: svof's
wall-clock timers write into the buffer. Check instead that:

- `svo_prompts` is identical, and equals the corpus's prompt count;
- `client_sends` is 0;
- with `SVOF_PLAIN` set, `game_lines` (game lines that came through unchanged) agrees within 1%, and
  `added_lines` (svof's own output) is close.
