# svof replay benchmark

Times a real trigger-heavy workload: the svof curing system (about 2,500
triggers in a tree six deep) running against a recorded Achaea session replayed over loopback telnet. Where
`PipelineBenchmark` measures the engine with a synthetic corpus and a small trigger set, this measures what a player
with a large system feels. Linux only.

## Inputs

Everything a run needs is in one file, `SvofBench.mpackage`, which is not in the repository:

- `svof.mpackage`, built for any class;
- svof's saved settings (the `config`, `defup+keepup`, `namedb`, `pipes`, `prios` and `rift+inv` folders of
  its `svo` folder), with svof set to play the class the corpus's GMCP names and unpaused, as a player does at
  the start of a session;
- the replay stream (`corpus.txt`), cut down to the text svof's triggers react to, with everything else
  replaced by filler of the same shape;
- `manifest.lua`: bundle version, `svof_class`, prompt count, line count, the corpus's md5, the counts of a
  reference run in each mode and how far a run may stray from them. `svof_class` is the class svof plays in
  the replay, which `promptify.lua` writes into the corpus (Infernal unless told otherwise); it is not
  the class of the character who recorded the log;
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
lua5.1 promptify.lua session.log corpus.txt <character-name>
(umask 077; head -c 32 /dev/urandom | base64 > salt)
lua5.1 make-package.lua --corpus corpus.txt --svof svof.mpackage --config <profile>/svo \
    --out SvofBench.mpackage --salt salt [--namedb <profile>/Database_namedb.db] [--names names.txt]
```

svof does almost nothing until it sees prompts, so the corpus must carry the game's protocol: an
`IAC GA` after every prompt and a GMCP `Char.Vitals` before it, plus a `Char.Status` at the start with the
class svof is to play. `promptify.lua` adds these, and for a log captured with prompts gagged it inserts a
synthetic prompt every few lines. Without GA prompts svof's `getLines()` scans turn the replay quadratic.

`make-package.lua` then scrubs the log. Every random choice it makes, each filler letter and each
stand-in, comes from SHA-256 of a secret salt and what the choice is for, so the same inputs and salt build
the same file. It reads the salt from `--salt <file>` or the `SVOF_SCRUB_SALT` variable, and will not scrub
without one. Keep the salt beside the log it was made for, readable only by you, and never commit or ship
it: with it and a list of players, anyone could work out whose name each stand-in replaces. A new salt
makes a new corpus, so `--expect` results then need new runs. The scrub keeps what svof reacts to and
nothing else:

- for every line it works out which of svof's patterns match: all of its triggers' patterns of every kind,
  children where their parent lets them run, children of filter triggers on their captures, the
  triggers svof makes at runtime (its defences' messages, and strings it passes to temp triggers), and the
  text its scripts search the lines since the prompt for. Lua patterns that read only the line are run;
  those that read svof's state are taken to match, and are listed in the build's output;
- a line becomes filler unless that changes which patterns match it: every letter is replaced with a
  letter drawn at random in the same case, every digit with a digit, and spaces, punctuation, symbols,
  ANSI and telnet bytes stay, so a word keeps its length and nothing else, and the trigger engine does
  the same shape of work. The timings and illusion marks svof appended when the log was recorded stay
  too, and filler is never drawn as one, such as "(p)" as "(i)". Most lines go this way;
- in a line svof does match, svof's scripts read what its patterns match and capture, such as the herb in
  an eating message, so there a number stays, and a word stays when it is in the vocabulary: the words of
  svof's own package, and game words that are mostly lower case and appear in at least three different
  svof-matched messages, which no name or one-off does. Elsewhere in the line a word stays only if filler
  in its place loses one of the line's matches, and then only the part a pattern matches; filler that
  matches more than the word did is drawn again. Real names, words outside the vocabulary that svof's
  patterns capture, and capitalised words outside the vocabulary each get one stand-in in the same case,
  drawn like filler, the same one in every line svof matches and in GMCP, so svof sees the same names agree
  and differ as before. A stand-in is 4 to 10 letters long, drawn apart from its word, whose length would
  narrow down whose name it is. No svof pattern reads a name's length, but Mudlet wraps a line at 100
  columns, after svof has appended to it, and svof's buffer scans and the gate see the wrapped lines; so a
  stand-in in a kept line over 80 columns takes its word's length, as does one whose other length a pattern
  turns out to notice. Where a pattern matches inside such a word, as one for lines that start
  with "A" does, its stand-in keeps those letters everywhere, and a stand-in that matches more than its
  word is drawn again everywhere. The rest is filler;
- svof reads nothing of a `cwho` row but which of its patterns match it: its trigger takes the name from
  the text in one ANSI colour, and the rank and class only once it has a name, and a plain log has no
  colour. Filler of the same shape would still match a character's public title and class by their word
  lengths and punctuation, so a row is rebuilt: a name of one or two words and a class of one, of lengths
  drawn at random, around what svof's patterns need, such as `On`, a rank of 1 to 6, `(Dragon)` and a
  leading `A`. Its names get no stand-ins, so nothing ties a row to the rest of the log. The build stops
  on a log with colour, or if svof's trigger reads the rows some other way;
- real names come from svof's name database (`--namedb`, by default `Database_namedb.db` beside the `svo`
  folder), the who-lists in the log (`qw`, `cwho` and svof's own), the player's name in GMCP and
  `--names`, one per line;
- GMCP keeps its keys and `Char.Vitals` as it is. In `Char.Status` the class svof plays, race, level,
  gender and specialisation stay, the name gets its stand-in and every other value is filler, as is every
  value of any other package;
- an exported priority list in the config loses its author and time stamp.

The build then audits its output and stops unless every line matches exactly the svof patterns it did before
and has the marks it had, no listed name is left in the corpus or the config in any case, no capitalised
word outside the vocabulary is left as it was, each listed name and capitalised word outside the vocabulary
reads as one stand-in in every line svof matches, no title, name or surname of a `cwho` row is left, nor any
other word of one that filler can replace, and no run of four words from a line that became filler is
kept anywhere, other than in a line svof matches that needs every one of those words, to keep a match or in
the vocabulary in what it reads. It prints how many lines were kept, the size of the vocabulary, and which
svof triggers kept them; `--report <file>` writes every one with an example line, for a person to read
through.

A scrubbed package makes svof do what its raw log does. `fidelity.lua` checks this on one machine,
from `run.sh` runs of both packages: it counts the lines svof adds, by kind, and its lock warnings, reading
the scrub's stand-ins back to the words they replace and ignoring spacing, which svof pads some echoes
with by a line's length, and fails any kind whose scrubbed median is further from the raw median than the
raw runs' own spread. Where a socket read ends just before a line break, Mudlet's fix for the game's
newline after a prompt joins that line to the next; stand-ins of other lengths move where reads end, so
it counts such joins apart, without judging them. List the raw runs first:

```bash
lua5.1 fidelity.lua raw.mpackage SvofBench.mpackage raw1/result.txt raw2/result.txt ... -- scrubbed1/result.txt ...
```

Over 8 interleaved pairs, each run started once the rest of the 16-core machine was using under 2.5
cores, every kind was within the raw spread: 2,319 added lines against a spread of 4, and 5,303 lock
warnings against a spread of 86. Reads joined two lines once in every scrubbed run, and one to three
times in a raw one. The scrubbed runs took 0.954 times as long at the median and were faster in 7 of the
8 pairs: filler costs the trigger engine a little less than the text it replaces. A scrub that fills in
what svof's patterns capture fails it: one such package wrote 2,192 more lock warnings than its raw log.

A package is held to the counts of a reference run in each mode, within its slack (see below). Build it,
run it once in each mode you will use (the gate fails for want of a reference), then build again with the
results; the corpus does not change, so they still apply:

```bash
SVOF_BENCH=SvofBench.mpackage test/svof-replay/run.sh ref build/src/mudlet
lua5.1 make-package.lua ... --expect /tmp/svof-replay/ref/result.txt [--expect <profile>/SvofBench/result.txt]
```

`--no-scrub` packages the log as it is, for a package you keep to yourself. Building needs `zip`,
`unzip`, and to scrub, `sqlite3` and lrexlib-pcre.

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
different work. Opened again, such a profile reports `gate FAIL` instead of a run; the last run's
`result.txt` is removed first.

Standalone numbers are not interchangeable with `run.sh`'s: they skip Mudlet's socket reads, and the event
loop gets a turn after every chunk, so `elapsed_s` and the heartbeat figures differ. `run.sh` is the
reference; compare builds within one mode.

## Checking a run

The report ends with `gate pass`, or `gate FAIL:` and the reasons, and `run.sh` exits non-zero on a fail:

- `stop_reason` is not `end_marker`: the replay stalled or hit the sample limit;
- svof did not load, is paused, plays a class other than the manifest's `svof_class`, or saw a prompt count
  other than the manifest's;
- `lines`, `game_lines` or `added_lines` is further from the package's reference run in that mode than
  the manifest's `slack`, or the package has no reference. `game_lines` counts the corpus's lines that came
  through, each as often as the corpus has it, once the timings and illusion marks svof appends are taken
  off both. The log was recorded with svof, so it holds svof's own lines too, and these count only as often
  as the log has them. `added_lines` counts every other line svof wrote, other than its lock warnings,
  which it repeats at every prompt while it thinks it is locked, so their number swings by hundreds
  between runs; `lock_lines` counts those. The slack, 36 lines, 18 game lines and 32 added lines in
  about 141,840, is twice the widest spread measured on one machine. A heavily loaded machine can exceed
  it: svof's timers fire late, and it tags more lines.

## Checking both builds did the same work

One run has nothing to compare to, so this part is yours. There is no exact identity hash: svof's
wall-clock timers write into the buffer. Check instead that:

- `svo_prompts` is identical, and equals the manifest's prompt count;
- `client_sends` is 0;
- `game_lines` and `added_lines` agree within the gate's slack. `run.sh` keeps the buffer in
  `result.txt.buffer` for a closer look.
