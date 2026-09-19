# Verification of batch 1: mapper (A1), game-text pipeline (B1), startup/credentials/profiles (E1)

Verifier V1, display :81, tree 85d814292 (= development 12b373743), binaries
`/home/user/Mudlet/build-linux-debug-nosan/src/mudlet`.
Work dir `<scratchpad>/qa/work-V1/`, screenshots `<scratchpad>/qa/shots-V1/`.
Core dumps were enabled for every run (`ulimit -c unlimited`, `/proc/sys/kernel/core_pattern` = `core`,
runs started from `work-V1/cores/`); the mapper session also ran under
`gdb -batch -ex run -ex bt -ex 'info threads'` with `MUDLET_TEST_MODE=1`.

## Verdicts

| Finding | Reported severity | Verdict | Your severity | Notes |
| --- | --- | --- | --- | --- |
| F-A1-1 | Major | CONFIRMED (code) | Major | `slot_configureAreas` on HEAD is byte-identical (200/200 lines) to `Mudlet-5.0.1`, 0 `setUnsaved` calls. Pre-existing, not a regression. Known: #10543, Fix pending: PR #10597 (draft). |
| F-A1-2 | Major | PARTIAL | Major | Delete really is unconfirmed and destructive (area + its 5 rooms gone, room count 6 -> 1). The second half - "map goes blank while the dropdown names a different area" - did NOT reproduce: dropdown and canvas both moved coherently to Beta. Known: #10545. |
| F-A1-3 | Major (Note) | NOT REPRODUCED | - | Six replays of the sequence (3 + 3) under gdb with cores enabled. Process alive throughout, no core file, no signal, no backtrace. |
| F-A1-4 | Minor | PARTIAL | Cosmetic | The code defect is real (combo row index used as a list row; the list always holds the default area) but the dialog preselected the CORRECT row in both configurations I tried. It only bites when "Default Area" sorts before the shown area AND the combo hides it. Known: #10542. |
| F-A1-5 | Minor | CONFIRMED | Minor | Renamed the unshown area Beta -> BetaRenamed while Alpha was drawn: canvas kept Alpha's 5 rooms, Area dropdown switched to "BetaRenamed". Shot 66. Known: #10544. |
| F-A1-6 | Minor (Note) | CANNOT RUN | - | Not re-run inside the time box; A1's own shots 26-28 are present and consistent with the claim. Not counted as verified. |
| F-B1-1 | Major | CONFIRMED | Major | ESC as the last byte before the 300 ms flush: the parameter bytes print as literal text. Independent of the coordinator's run. Related: #10766 (closed) - Fix incomplete stands. |
| F-B1-2 | Minor (Note) | CONFIRMED | Minor | 2.4 MB MCCP2 burst: 9,757 of 30,000 lines, no end marker, "recursion depth exceeded" warning. Pre-existing (cap is in 5.0.1's `ctelnet.h`). Known: #10659 (open, same symptom, same threshold). |
| F-B1-3 | Cosmetic | CONFIRMED | Cosmetic | Split IAC GA leaves an empty line between `iac1:` and `after1`. |
| F-B1-4 | Cosmetic | CONFIRMED (sharpened) | Cosmetic | Exact breakdown below: print/echo/cecho/decho/hecho mirror; feedTelnet/feedTriggers text does not. The coordinator's "print() is mirrored" does not contradict B1. |
| F-B1-5 | Cosmetic | CANNOT RUN | - | Subset spec run not repeated (would have been a second concurrent Mudlet). Harness artefact, not product. |
| F-E1-1 | Major | FIXED-BY-12b373743 | - | `ctest -R '^(CredentialManagerKeychainTest\|HomeUntouchedTest)$'` -> 2/2 Passed (12.22 s / 23.38 s). |
| F-E1-2 | Minor | FIXED-BY-12b373743 | - | Same run. |
| F-E1-3 | Minor | CANNOT RUN | - | Needs a two-profile create/rename/restart cycle through the connection dialog; not reached in the time box. E1's shots 16/17 exist. |
| F-E1-4 | Minor (Note) | CONFIRMED, pre-existing | Minor | `env -u DISPLAY QT_QPA_PLATFORM= mudlet --version` -> exit 134. 5.0.1 `src/main.cpp` has the same structure. |
| F-E1-5 | Minor (Note) | CONFIRMED, pre-existing | Minor | Neither version sets permissions; `SecureStringUtils.cpp` is byte-identical to 5.0.1. Files land at 0644 under umask 022. Closed: #2325 ("Secure rights to password file") - so this is a fix that did not stick, not a new regression. |
| F-E1-6 | Cosmetic | CANNOT RUN | - | Needs the keychain-unavailable password round trip through the dialog; not reached. |

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
| --- | --- | --- | --- | --- | --- |
| A1 | 17 | 17/17 (all cited shots 01-35 exist; 23-map2.png is a 263-byte all-black PNG, consistent with the claim) | 8 (f585cb5f7, 977900fc6, c9669fba6, c881de953, a7cf7a2db, 2f5e3b366, 6aecd138a, 3280a5502) | 8 | none |
| B1 | 20 | 20/20 | 5 (efe9414f2, bca5af8a2, 7773ae9d6/acd1dbab6 via the burst, 875f8ec67 partially, 40ca0b7e6 via the split feeds) | 5 | none |
| E1 | 27 | 27/27 | 7 (0b43837e4 ctest, 72d96a5ca ctest, 59e79709f, 524ec6452, 84f998451, c7f2591d3, 577f4188a) | 7 | none |

A1's six admitted-untested rows were all driven by hand this run (details below) and all held up.
Five of E1's eight "Could not test" rows were run and all held up; three could not be reached
(0905eca3b/4edf41c04 GMCP Char.Login server, 982a0d15e ten-package timing, b66feea61 preferences toggle).

## New findings

None. Nothing I re-ran failed in a way the three reports had not already described, and no row
that I re-ran contradicted its evidence. One observation that is not a finding is recorded under
"Details / f585cb5f7".

## Details

### F-E1-1 / F-E1-2 - fixed on the new tree
```
1/2 Test #21: CredentialManagerKeychainTest ....   Passed   12.22 sec
2/2 Test #41: HomeUntouchedTest ................   Passed   23.38 sec
100% tests passed, 0 tests failed out of 2
```
Record as FIXED-BY-12b373743. E1's diagnosis (test-harness use-after-free) is consistent with the fix.

### F-B1-1 - ESC as the last byte before the flush (CONFIRMED)
Script `work-V1/v1-b1.lua` (my own, five `feedTelnet` calls with 450 ms gaps, then a
`getCurrentLine()`/`getFgColor()` dump). Mirror log `work-V1/mudlet1.log`:
```
main| V1SGR[10] fg=192,192,192 text=whole1:RED1:end     <- control, one read: correct
main| V1SGR[11] fg=255,0,0     text=csiA:
main| V1SGR[12] fg=255,0,0     text=[1;31mRED1:end      <- ESC was the last byte: parameters printed as text
main| V1SGR[13] fg=255,0,0     text=csiB:
main| V1SGR[14] fg=255,0,0     text=RED1:end            <- ESC first byte of the 2nd read: correct
```
Screenshot `shots-V1/03-sgr.png`. The blamed commit efe9414f2 is right: it guards the `mGotCSI`
scan with `localBufferDecodableLength` and leaves the `mGotESC` block above it unguarded.
Status: Fix incomplete; the closed issue #10766 describes the same visible defect.
Tracker: #10766 CLOSED (recent-issues.md:46); semantic search also surfaced #10658 (open, MXP
`ESC[nz` split across reads shows markup raw) and #9912 - same family, different decoder.

### F-B1-2 - MCCP2 burst (CONFIRMED)
`work-V1/mccp-server.py 4499 30000 compress` + `connectToServer("127.0.0.1", 4499)`; scan script
`work-V1/v1-mccp.lua`. Server log: `compressed bytes 84468`, one `sendall`.
```
cTelnet::processSocketData(...) WARNING - recursion depth exceeded, dropping remaining data
main| Too much data to process at once, some may have been lost.
main| V1MCCP count=9757 last=0 end=false lineCount=9802
```
9,757 of 30,000 lines, end marker absent. Matches B1's 9,756 within one line.
Tracker: **Known: #10659** (open) - "MCCP2: a read that inflates past about 800 kB drops the rest of
the input and garbles the session". B1's "looks like #10659" is correct; I confirm it is live.

### F-B1-3 - split IAC GA (CONFIRMED)
`work-V1/v1-iac.lua`:
```
main| V1IAC[31]=[iac1:]
main| V1IAC[32]=[]
main| V1IAC[33]=[after1]
```
Tracker: nothing specific; #3512 (open, "Handling GA suppression / Telnet negotiation") is adjacent.

### F-B1-4 - what `--mirror` actually mirrors (CONFIRMED, sharpened)
Seven probes in one script (`work-V1/v1-b1.lua`), all with distinct markers, one launch with
`--mirror` and `MUDLET_TEST_MODE=1`. `grep V1MIRROR work-V1/mudlet1.log`:
```
main| V1MIRROR_PRINT      <- print()        mirrored
main| V1MIRROR_ECHO       <- echo()         mirrored
main| V1MIRROR_CECHO      <- cecho()        mirrored
main| V1MIRROR_DECHO      <- decho()        mirrored
main| V1MIRROR_HECHO      <- hecho()        mirrored
(no line for V1MIRROR_FEEDTELNET)
(no line for V1MIRROR_FEEDTRIGGERS)
```
Both `feedTelnet` and `feedTriggers` text landed in the buffer - a later dump read them back as
`V1SGR[8] text=V1MIRROR_FEEDTELNET` and `V1SGR[9] text=V1MIRROR_FEEDTRIGGERS` - but neither reached
stdout on its own. So: everything that goes through `TConsole::print`/`printFormatted` is mirrored
(print, echo, cecho, decho, hecho); nothing arriving on the telnet path is. B1's claim holds exactly,
and the coordinator's observation (Lua `print()` mirrored) is the same fact seen from the other side.
`mudlet.h`'s help text ("mirror everything shown in any console to stdout") overstates it.
Tracker: no issue found; semantic search returned nothing relevant.

### F-A1-1 / F-A1-4 / F-A1-5 / F-A1-2 - Configure areas
Code: `work-V1/ca-501.txt` vs `work-V1/ca-dev.txt` - `diff` is empty, 200 lines each, so all four are
pre-existing and none is a regression. `grep -c setUnsaved ca-dev.txt` = 0 (F-A1-1 confirmed by code;
no Lua getter exposes the unsaved flag, so no behavioural check is possible).

By hand, map built from `work-V1/mkmap.lua` (areas Alpha/Beta, 5 rooms in Alpha, 1 in Beta):
- **F-A1-4 did not reproduce.** With the mapper showing Alpha (combo index 0) the dialog listed
  `Alpha (1) / Beta (2) / Default Area (-1)` and preselected **Alpha**, with Rename and Delete
  enabled (`shots-V1/36-configure-areas.png`, `64-ca2.png`). Repeated after a restart: same.
  Reading the code, `repopulate()` fills the list from `getMapperAreaNamesSorted()` - which sorts by
  NAME - while the preselect uses `getCurrentShownAreaIndex()` (a combo index). The off-by-one
  therefore only appears when "Default Area" sorts before the shown area *and* the combo omits it;
  with areas named Alpha/Beta it sorts last and the indices coincide. A1's mechanism is right, its
  trigger condition is narrower than the finding states -> PARTIAL, severity Cosmetic.
- **F-A1-5 reproduced exactly.** Showing Alpha, selected Beta in the dialog, Rename -> `BetaRenamed`.
  Canvas still drew Alpha's five rooms; the Area dropdown read `BetaRenamed`
  (`shots-V1/65-rename-dialog.png` -> `66-after-rename.png`).
- **F-A1-2 reproduced in part.** With Alpha shown, Delete: no confirmation dialog of any kind, the
  area and its rooms were gone immediately (`V1ROOMCOUNT=1 areas=Default Area,Beta`, down from 6
  rooms / 3 areas). The blank-map half did not occur: the dropdown moved to `Beta` and the canvas
  drew Beta's room (`shots-V1/37-after-delete-area.png`). Severity still Major on the data-loss half.
Tracker: recent-issues.md lines 211-214 list #10542-#10545 all OPEN; open-prs.md:45 lists
**PR #10597 (draft)** "Fix: Configure areas dialog no longer preselects the wrong area, mislinks
renames, autosaves, or blanks the map". So all four are `Known: #1054x, Fix pending: PR #10597`.

### F-A1-3 - process disappeared (NOT REPRODUCED)
Script `work-V1/a13-seq.sh`, run against a Mudlet started under
`gdb -batch -ex run -ex bt -ex 'info threads'` with `MUDLET_TEST_MODE=1`, cwd `work-V1/cores/`,
`ulimit -c unlimited`. Each iteration: `deleteMap()` -> right-click the empty map -> the no-map menu
-> `Create new map` -> right-click empty space -> Escape -> left click in the main console -> ctrl+a
-> Delete -> type a `lua dofile(...)` line.
Ran 3 iterations with the menu click landing on `Load map...` (a file dialog opened and was escaped -
`shots-V1/40-a13-1-nomapmenu.png`, `41-a13-1-newmap.png`), then fixed the coordinate and ran 3 more
that hit `Create new map` (`shots-V1/41-a13-2-newmap.png` shows the new map, `1 (Default Area)`).
All six: `ALIVE after iteration N`. `work-V1/cores/` is empty. `grep -i 'signal|SIGSEGV|Program received'`
over `work-V1/mudlet2.log` returns nothing, and gdb printed no backtrace, i.e. the process never
faulted. A1's own note ("not reproduced", recorded as a Note) is the right call; I add that it also
does not reproduce under a debugger with cores enabled.

### A1's six untested coverage rows - all driven by hand
- **f585cb5f7 (Move to position carries custom exit lines) - HELD UP.** Room 1 at (3,0) with a green
  three-point custom east line at (3.5,1)(4.5,1)(5,0). Right-click -> `Move to position...` -> 10,10.
  After: `coords=10,10,0 lines={e={... points={0={y=11,x=10.5},1={y=11,x=11.5},2={y=10,x=12}}}}` -
  every point moved by exactly the room's delta (+7,+10). `shots-V1/23-movetoposition.png`, `24-after-move.png`.
  *Observation, not a finding:* the Lua `setRoomCoordinates()` does **not** move the custom line -
  after moving the room back to (3,0) from Lua the line stayed at x 10.5-12 (`shots-V1/25-reset.png`).
  That is a different code path from the UI action this commit fixed, and may be intended.
- **977900fc6 (Spread/Shrink do not skew custom exit lines) - HELD UP.** Rooms 1,2,3 selected
  (`shots-V1/28-selall-menu.png` shows Spread.../Shrink... in the menu), Spread by 3 centred on
  room 1. Room 2 went (5,0) -> (9,0); the custom line's points went (10.5,11)(11.5,11)(12,10) ->
  (25.5,33)(28.5,33)(30,30), i.e. `x' = 3 + (x-3)*3`, `y' = y*3` - exactly the rooms' transform, no skew.
- **553c91251 (label drag) - still not driven by hand** (my `createMapLabel` call failed on an
  argument-type error and I ran out of the time box). Covered by `MapMouseInteractionTest`.
- **c9669fba6 (exits dialog redraws and marks unsaved) - HELD UP.** `Set exits...` on room 1 showed
  N=3 / E=2 / S=5 (`shots-V1/17-exits-dialog.png`), typed `4` into Up, Save; the canvas redrew and
  `getRoomExits(1)` read back `V1EXITS up=4 n=3`. The dialog title also grew a `*` as soon as the
  first edit landed (`shots-V1/20-...png`), which is the dirty marker.
- **c881de953 (crash after deleting the special exit being edited) - HELD UP.** `Add special exit`,
  set Exit Room ID 2, opened the Command cell editor and typed into it, then re-opened the Exit Room
  ID cell and set it to 0 (removing the exit that was being edited). No crash; the row collapsed to
  `0` and the dialog stayed usable. `shots-V1/18,19,20-*.png`.
- **a7cf7a2db (area switch clears the selection) - HELD UP.** `V1SEL_BEFORE=2` rooms selected, chose
  `Beta` in the Area dropdown, `V1SEL_AFTER=0`. No off-screen selection survived.
- Also re-checked **2f5e3b366**: the empty-space menu is exactly 5 items and the room menu exactly 12,
  with no duplicates, after all the menu opening this session (`shots-V1/16-roommenu-edit.png`,
  `35-menu-for-ca.png`, `63-menu.png`). And **6aecd138a**: `Create new map` from the no-map menu left
  the mapper in editing mode across all three scripted iterations.

### E1's "Could not test" rows - five of eight run
- **524ec6452 / 84f998451 (fonts) - HELD UP.** `setFont('main','Helvetica')` -> `true`, and
  `getFont('main')` then read `Nimbus Sans [UKWN]`, i.e. the system's Helvetica substitute was
  resolved and used rather than refused. `setFont('main','NoSuchFontXYZ')` -> `nil` and the font
  stayed `Nimbus Sans [UKWN]` - refused, no random substitute. `shots-V1/50-font.png`; the change
  also survived the restart (`shots-V1/60-map2.png` shows the proportional console font).
- **c7f2591d3 (dictionary survives a restart) - HELD UP.** `addWordToDictionary('Zorblaxx')` -> true;
  `'bad/word'`, a word with a tab, and `''` -> all `nil` (refused), which is exactly what the commit
  message claims it added. Closed the window; `profile.dic` on disk then held `1\nZorblaxx`.
  Relaunched with the same HOME: `V1DICT2 n=1 first=Zorblaxx check=true`. `shots-V1/51-dict-restart.png`.
- **577f4188a (sysApplicationFocusChangeEvent) - HELD UP.** Registered an anonymous handler; no other
  X client exists in this container, so I took focus away with `xdotool windowminimize` and gave it
  back with `windowactivate`/`windowmap`:
  `main| V1FOCUS focus=false` then `main| V1FOCUS focus=true`. Both edges fire.
- **b66feea61 (dictionary read when spell check is switched on) - not run**: needs the preferences
  dialog toggle; the round-trip above plus `DictionaryRoundTripTest` is what I have.
- **0905eca3b / 4edf41c04 (GMCP `Char.Login` round trip) - not run**: writing and driving a telnet
  server that negotiates GMCP and answers `Char.Login` did not fit what was left of the time box
  after the mapper work. Still uncovered by hand, as E1 said.
- **982a0d15e (ten-package profile open timing) - not run**: same reason.
- **6e396b329 (UBSan) - not run**: would need a sanitizer build, which the brief forbids.

### F-E1-4 - `--version` with no display (CONFIRMED, pre-existing)
Reproduced independently on 85d814292: exit 134 with the xcb plugin message; `QT_QPA_PLATFORM=offscreen`
prints the version and exits 0. 5.0.1's `src/main.cpp` builds the QApplication before reading the
option in the same way. Note, not a regression.

### F-E1-5 - encrypted-file fallback permissions (CONFIRMED, pre-existing)
- `git show Mudlet-5.0.1:src/SecureStringUtils.cpp` vs `src/SecureStringUtils.cpp`: **identical**.
  `storeEncryptionKeyToFile()` opens a `QSaveFile` with `WriteOnly|Unbuffered` and never calls
  `setPermissions` - in either version.
- The credential file writer changed shape (5.0.1 `QFile` + `WriteOnly|Text` at line 1218; HEAD
  `writeCredentialFile()` with `QSaveFile` + `commit()`), but neither sets permissions either, and
  `grep -n -i 'permission|setPermissions'` over both files returns nothing.
- On disk, everything Mudlet writes lands at the umask default: `644 .../profile.dic`,
  `644 .../profile.ini`, directories `drwxr-xr-x`, with `umask 0022`. E1's `-rw-r--r--` on
  `encryption_key` and `passwords/character` is the same default and is not specific to those files.
So: **pre-existing, not a regression** - E1's "Note" is right. Tracker: semantic search found
**#2325 "Secure rights to password file" (CLOSED)** and #1917 "Password file not encrypted" (CLOSED).
Tag it `Closed: #2325` - the tracker believes this was dealt with, so it is a fix that did not stick
rather than a new defect. Worth re-opening #2325 rather than filing fresh.

## Process hygiene
All processes I started were recorded in `work-V1/pids.txt` (Xvfb 25684, openbox 25689, Mudlet 26483,
mccp-server 26709, gdb 26976 + its child 26983, Mudlet 29410) and killed by PID only. No `pkill`,
no `killall`, no pattern kills. Three foreign `src/mudlet --profile` processes were visible during
the run and were left alone.
