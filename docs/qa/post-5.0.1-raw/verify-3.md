# Verification of batch 3: F1 (console, labels, Geyser containers, links, user windows) and D1 (script editor, variables, notepad, debug console)

Verifier V3. Display :82, work dir `work-V3/`, screenshots `shots-V3/`.
Tree under test: 85d814292 (binary `/home/user/Mudlet/build-linux-debug-nosan/src/mudlet`, title bar reads
`Mudlet 5.0.0-dev-85d814292`; repo HEAD 1dba6b144 differs only in `docs/qa/*.md`).
A/B tree: `/home/user/worktrees/v501/build-linux-debug-nosan/src/mudlet` (`Mudlet 5.0.1-dev-592821c8c`).
Every finding replayed from the agents' own scripts under `work-F1/` and `work-D1/`.

## Verdicts

| Finding | Reported severity | Verdict | My severity | Notes |
| --- | --- | --- | --- | --- |
| F-F1-1 console paints at the old border after a container's edge drag | Major | CONFIRMED (blame wrong) | Major | Reproduced exactly, incl. the 3 s persistence and the heal-on-click. **5.0.1 does the identical thing**, so 6686b97ea did not introduce it - pre-existing, not a regression. Status `Known: #10593 / Fix pending: PR #10594` verified live. |
| F-F1-2 orphan scroll box over the main console after its user window is deleted | Major | PARTIAL | Major | The orphan reproduces exactly on development. But **5.0.1 leaves the same orphan and does not crash** on either `t23.lua` or `t21.lua`, so "the crash is indeed gone" is not demonstrated by this repro on either tree and the orphan is pre-existing, not created by c0309561b. "Fix incomplete (Closed: #10319)" is defensible for the orphan; the commit's own crash claim is untested by this repro. |
| F-F1-3 two containers on opposite borders leave the console 0 rows | Major | CONFIRMED | Major | dev `T458 B65 L149 R149`, `rows/cols 0x56`. 5.0.1 the same (`T507 B65 ...`, `0x56`) - pre-existing, `Known: #10617`. The single-container half of d3f5f873b is a genuine fix: dev keeps 2 rows, 5.0.1 keeps 0. |
| F-F1-4 `hide()` leaves the border reserved | Minor | CONFIRMED | Minor | `before hide: L416 cols=46` / `after hide(): L416 cols=46`. `Known: #10747`. |
| F-F1-5 second `minimize()` discards the real height | Minor | CONFIRMED | Minor | `start=60.00000% afterMin1=25px afterMin2=25px afterRestore=25px restored=false`. `Known: #10744`. |
| F-F1-6 `setPadding()` raises and still writes the bad value | Minor | CONFIRMED | Minor | Both `-5` and `"abc"` raise at `GeyserSetConstraints.lua:106` and remain in `CP.padding`. `Known: #10745`. |
| F-F1-7 `lockContainer()` accepts an unknown style, raises on a bad number | Minor | CONFIRMED | Minor | `('notastyle') ok=true locked=true lockStyle=standard`; `(99)` raises at `GeyserAdjustableContainer.lua:738`. `Known: #10746`. |
| F-F1-8 `attachToBorder("Left")` raises but marks the container attached | Minor | CONFIRMED | Minor | `ok=false err=...:486: bad argument #1 to 'pairs' ... attached=Left`. `Known: #10618`. |
| F-F1-9 a label swallows right-clicks | Minor | CONFIRMED | Minor | No menu over the label (the label's own click callback fires instead); full Copy/Copy HTML/Copy as image/Select all/Search menu on bare console. `Known: #10753`. |
| F-D1-1 paste of newline-only text into a pattern field aborts Mudlet | Blocker | CONFIRMED | Blocker (pre-existing) | Re-ran `work-D1/repro-10337.sh :82` under gdb: `ASSERT: "!isEmpty()" ... qlist.h:676`, frame #10 `SingleLineTextEdit::insertFromMimeData` at `src/SingleLineTextEdit.cpp:69`. The line is byte-identical in `Mudlet-5.0.1:src/SingleLineTextEdit.cpp` - not a regression. `Known: #10337`. |
| F-D1-2 autocomplete offers and inserts `[edit \| edit source]` | Major | CONFIRMED | Major | Popup entry `setBackgroundColor[edit \| edit source]`; Return inserts that literal text into the script. Argument hint pane is correct. 669 entries in `src/lua-function-list.json` vs 0 at `Mudlet-5.0.1`. `Regression; Known: #10816`. |
| F-D1-3 Debug button appeared to need two clicks | Note | NOT REPRODUCED | n/a | One click opened the Central Debug Console (`xdotool search` found it after click 1). D1 already flagged this as uncertain; it should be dropped. |
| F-D1-4 `debugc()` never reaches the Central Debug Console | Note (Cosmetic) | CONFIRMED | Note | 20 `debugc()` lines all arrived under the `errors_Mudlet self-test|` mirror prefix; only `centralDebug|` and `main|` otherwise. By design (`TLuaInterpreter::debug()` prints to `mpEditorDialog->mpErrorConsole`). A recipe correction, not a defect. |

### Extra check requested: open issue #10342 (Tab at the very start of a script aborts debug builds)
**D1's claim is CONFIRMED, and #10342 can be closed.** Clean A/B on the same repro:
- development 85d814292: Ctrl+Home then Tab, both on an empty code box and on `local x = 1`, indents the line; process stays `Sl` (`shots-V3/10-tab-empty-doc.png`, `11-crop.png`).
- 5.0.1: the same keystrokes kill the process - `ASSERT: "idx != std::string::npos" in file ../3rdparty/edbee-lib/edbee-lib/edbee/models/textdocument.cpp, line 390`, process GONE (`work-V3/v501a.log`).
So the edbee bump 805918f48 (edbee #180) is what fixed it.

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
| --- | --- | --- | --- | --- | --- |
| F1 | 32 (15 "Fixed & verified") | 14/15 evidence present; 1 filename typo (see below) | 10 of the 15 | 10 | none |
| D1 | 16 (8 "Fixed & verified") | 8/8 evidence present | 7 of the 8 | 7 | none |

Evidence spot-check: every cited screenshot and log line I looked for exists, with one exception -
F-F1-2 cites `shots-F1/22-inputline-before-drag.png`, which does not exist; the file is
`shots-F1/21-inputline-before-drag.png` (and `22-inputline-after-drag.png`). A filename typo in the report,
not missing evidence - both files are there and the dc07ffb23 row cites them correctly.

Re-run and held up, F1: 6686b97ea, a21391878, d3f5f873b, 1f159bf2b, 95a20c600, 2492e0efc, 55871e6f6,
373b68265, c0309561b, 78e33c43a. Not re-run (time-box): 2fe1f047c, 5830a3bdb, dc07ffb23, f04e3f483, 7383ed6c9.
Re-run and held up, D1: 18de78473, 3183df6a9, e5eeddff9, 5edb079ed, d8fc91b54, 805918f48, f45cf7816 (partial).
Not re-run: a80ca8472.

All the mandatory re-runs the coordinator named were done; details below.

## New findings
None. Nothing I re-ran failed to hold up, and nothing new fell out that is not already covered by an
existing finding or tracker issue. The two corrections I would make to the batch-3 reports are the
blame lines on F-F1-1 and F-F1-2 (both behaviours exist unchanged in 5.0.1) and dropping F-D1-3.

## Details

### F-F1-1 (edge drag) - CONFIRMED on both trees
Replay: `work-F1/t11.lua` (25% x 60% container "cpct", magenta child, `attachToBorder("left")`), then
`clearWindow` + 40 `LINE%02d` rows, then drag the container's own right edge from x=399 to x=570 at y=310
and touch nothing.
- At rest: `borderLeft=245 cols=61`, text starts at x=405 = window-left 160 + 245 (`shots-V3/33-before-edge-drag.png`).
- 1 s and 3 s after mouseup, no interaction: container right edge at x=571, text painting at x=548; rows the
  container covers read `NE23`, `NE26`, ... - the first two characters are under the container
  (`shots-V3/34-edgedrag-1s.png`, `35-edgedrag-3s.png`).
- After one interaction: `borderLeft=416 cols=46`, text starts at x=576 = 160+416, rows read `LINE23` again
  (`shots-V3/36-edgedrag-after-click.png`).
- **5.0.1, identical repro**: `V3B501 borderLeft=245 cols=61` at rest, same drag, 3 s later the container edge
  is at 571 and text is at 548 with `NE23`..`NE36` clipped (`shots-V3/53-v501-edgedrag-3s.png`); after a click
  `V3D501 ... borderLeft=416 cols=46` (`54-v501-edgedrag-after-click.png`).
So the defect is live and real, but pre-existing: the "Commits: 6686b97ea" attribution in F-F1-1 is wrong -
that commit fixed the *window*-drag half, which I re-verified separately (below).

### F-F1-2 (orphan scroll box) - PARTIAL
Replay `work-F1/t23.lua` verbatim on both trees.
- development: `deleteMiniConsole('uw3') -> true / nil`, `deleteScrollBox('sbY') ... -> false / scrollbox name 'sbY' not found`,
  and the widget is visibly still there, reparented onto the main console at (0,0) with both scroll bars,
  clipping the console line to `le('uw3') -> true / nil` (`shots-V3/03-dev-t23-orphan.png`).
- **5.0.1: exactly the same two return values and the same orphan widget in the same place**
  (`shots-V3/51-v501-t23.png`), no crash.
- I also ran `work-F1/t21.lua` (the eleven follow-up calls that #10319 is about) on 5.0.1: all eleven return
  cleanly, `T21| STILL-ALIVE-T21`, process `Sl`. So 5.0.1 does not crash on this repro either.
Conclusion: the orphan is real and worth reporting, but it predates c0309561b; and F1's coverage claim
"the crash is indeed gone" is not supported by this repro, because the repro does not crash 5.0.1 to begin with.

### F-F1-3 (0 rows) - CONFIRMED, pre-existing
`work-F1/t9.lua` / `t10.lua` verbatim.
- development: `T9| d3f5f873b borders with a 100%-height container on top: T458 B65 L149 R149`,
  `T9| d3f5f873b console rows/cols: 0x56`; alone on the top border `T10| ... ALONE ... T458 B0 L0 R0 rows/cols=2x83`.
- 5.0.1: `T9| ... T507 B65 L149 R149`, `0x56`; alone `T10| ... ALONE ... T507 B0 L0 R0 rows/cols=0x83`.
The single-container bound is a genuine fix (2 rows vs 0); the opposite-edges case is unchanged from 5.0.1.
Also confirms #10619 differs: dev clamps `move(-400)` to `L0`, 5.0.1 leaves `L197`; neither goes negative.

### F-F1-4..F-F1-8 - CONFIRMED
`work-F1/t22.lua` and `t7.lua`/`t8.lua` reproduced line for line; see the verdict table for the exact log lines.
`t7`/`t8` also re-confirm 1f159bf2b (`setBackgroundColor('cachild') -> true`, geometry `538,60,316,120` after
the container is deleted) and 95a20c600 (`autoSaveHandler 53 -> nil -> 54`, `killAnonymousEventHandler -> true`).

### F-F1-9 - CONFIRMED
`work-F1/t6.lua`, then right-click the prose label and right-click bare console.
Over the label: no menu; the label's own callback fires (`T6| PROSECB fired`) (`shots-V3/38-rightclick-label.png`).
On bare console: Copy / Copy HTML / Copy as image / Select all / Search on Google
(`shots-V3/39-rightclick-console.png`).

### F-D1-1 - CONFIRMED, pre-existing
`bash work-D1/repro-10337.sh :82`. Backtrace saved to `work-V3/gdb-10337-dev.log`:
`ASSERT: "!isEmpty()" in file /opt/qt/6.9.0/gcc_64/include/QtCore/qlist.h, line 676`,
`#9 QList<QString>::first`, `#10 SingleLineTextEdit::insertFromMimeData ... src/SingleLineTextEdit.cpp:69`.
`git show Mudlet-5.0.1:src/SingleLineTextEdit.cpp` carries the identical
`text.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts).first();`, so Regression is correctly *not* claimed.

### F-D1-2 - CONFIRMED
Editor -> a trigger's code box -> type `setBackgroundCol`. Popup entry reads
`setBackgroundColor[edit | edit source]` with a correct argument hint pane beside it
(`shots-V3/08-autocomplete.png`); Return inserts the literal `setBackgroundColor[edit | edit source]`
(`shots-V3/09-crop.png`), and the follow-on popup is full of the same markup.

### F-D1-3 - NOT REPRODUCED
Editor open, one click on **Debug** in the left sidebar, 3 s wait: `xdotool search --name "Central Debug Console"`
returned window 4194531 and the console is on screen with its full toolbar
(`shots-V3/12-debug-click1.png`). A second click did not change that. One click is enough here.

### F-D1-4 - CONFIRMED (Note)
`lua for i=1,20 do debugc("V3DEBUG line "..i) end`: all 20 lines appear under `errors_Mudlet self-test|` in
`work-V3/dev1.log`; mirror-prefix histogram for the whole session is `centralDebug| 2`,
`errors_Mudlet self-test| 47`, `main| 24`. Screenshot `shots-V3/14-debugc.png`.

### Mandatory coverage re-runs
- **78e33c43a** - `setWindowWrapIndent("main",-2) -> nil + "indent -2 is not valid, it must be 0 or more"`;
  same for `setWindowWrapHangingIndent("main",-3)`; `setWindowWrapIndent("main",4) -> true`.
  **I also closed F1's untested gap**: saved the profile, patched
  `<wrapIndentCount>-8</wrapIndentCount>` and `<wrapHangingIndentCount>-6</wrapHangingIndentCount>` into
  `current/2026-09-19#15-29-40.xml` (copy kept at `work-V3/patched-negative-indent.xml`), restarted with the
  same HOME: the profile loads, five 200-character lines wrap normally with no indent and no one-character
  lines, process alive (`shots-V3/32-negindent-fed.png`, log `V3ALIVE`). No abort. Held up.
- **6686b97ea window drag** - attached a fresh 25% x 55% container to the left border
  (`work-V3/reattach.lua`, `V3E attached borderLeft=245 cols=61`), fed 30 rows, then dragged the main window
  by its title bar in 9 steps. Mid-drag the window content edge is at x=-40 and text starts at x=205, i.e. the
  245 px border is intact and nothing is clipped (`shots-V3/44-mid-window-drag2.png`, `45-after-window-drag2.png`).
  Held up.
- **2492e0efc** - label containing `click me <a href="...">LINKTEXT</a> here` with a click callback: clicks on
  the plain text and on the anchor both fire it (`T6| CLICKCB fired n=1`, `n=2`, `n=3`); no stray Qt menu on
  the prose label. Held up.
- **1d17661d5** - not re-run in the UI (time-box). F1's row is itself "Verified no regression (ctest)"; I did
  not contradict it and did not re-run the ctest either. Recorded as not re-run.
- **18de78473** - trigger with 8 substring patterns, Advanced options collapsed: rows 1-5 fully visible,
  row 6 clipped at the bottom, list scrolled to row 1 (`shots-V3/07-eight-patterns.png`). The
  **Advanced options** control (3183df6a9) is labelled, framed and has a right-pointing arrow while collapsed.
  Held up.
- **f45cf7816** - partially re-run. I did not delete the theme file by hand, but every fresh-HOME launch in this
  session logs `"texttheme.cpp" @ 207 Theme not set: "Monokai"` (twice per launch, `work-V3/dev1.log`,
  `dev3.log`), i.e. the profile's configured theme is genuinely unavailable, and the editor opened and rendered
  the pattern rows and the Lua code box with the fallback theme with no crash (`shots-V3/07-eight-patterns.png`,
  `08-autocomplete.png`). Consistent with D1's row; the deliberate `Gone Theme` variant was not repeated.
- **e5eeddff9 / 5edb079ed** - `qa['say "hi"']` selects with no warning banner, Name `say "hi"`, Key type
  `key (string)`, value `before2` (`shots-V3/19-sayhi-selected.png`); edited to `after2` and committed by
  selecting another variable -> `QA[say "hi"]=after2` with `my var`, `plain`, `5` untouched. Renamed `qa[5]`
  ("keepme") to `five` with Key type switched to `key (string)` and Save Variable -> `QB[five]=keepme` plus the
  other three members intact, no Lua error raised (`shots-V3/23-keytype-combo.png`, `24-renamed.png`). Held up.
- **d8fc91b54** - Central Debug Console opens with Pause / Clear / Show / item combo / text filter / Aa and
  announces `[*] 4 kind(s) of message are hidden` (`shots-V3/12-debug-click1.png`). Held up (with F-D1-3 dropped).
- **805918f48** - see the #10342 A/B above. Held up, and it is now proven against 5.0.1.

### Automated
Not re-run. Batch 3's ctest sets (F1 23/23, D1 16/16) match the 220/220 baseline and neither report claims a
failure; with the time-box spent on the A/B builds I relied on the agents' logs (`work-F1/ctest.log`,
`work-D1/ctest.log`) rather than repeating them.

### Process hygiene
All seven PIDs I started (Xvfb :82, openbox, five Mudlet instances) are listed in `work-V3/pids.txt` and were
killed by PID at the end. The gdb-driven instance from `repro-10337.sh` aborted on its own; I confirmed no
Mudlet window was left on :82.
