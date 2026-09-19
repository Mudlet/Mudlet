# Verification of batch 2: mapper (A2), trigger/alias/timer/key engine (C1), plus carry-over F-A1-6, F-E1-3, F-E1-6

Verifier V2. Tree 85d814292 (= development 12b373743). Display :82.
Work dir `<scratchpad>/qa/work-V2/`, screenshots `<scratchpad>/qa/shots-V2/`.
Four throwaway-HOME Mudlet sessions were used; every PID was recorded in
`work-V2/pids.txt` and only those were killed.

## Verdicts

| Finding | Reported severity | Verdict | Your severity | Notes |
|---|---|---|---|---|
| F-A2-1 `createMapper()` dead after openMapWidget+closeMapWidget | Major (Known #10666) | CONFIRMED | Major | Reproduced verbatim, twice in a row, same message. Known tag right (#10666 open). Also found the mirror guard: with a scripted mapper made first, `openMapWidget()` is refused ("cannot create map widget. Do you already use an embedded mapper?") - see Note V2-N1 |
| F-A2-2 bulk map building superlinear | Minor (Note, Known #9768) | CONFIRMED | Minor | Four identical 1000-room batches on a growing map: 0.556 / 1.456 / 2.431 / 3.152 s (map 0.6k -> 4.6k rooms). Cost per batch grows linearly with map size, i.e. the build is quadratic. Not a regression |
| F-C1-1 nested `feedTriggers()` wipes captures | Major (Known #10796) | CONFIRMED | Major | `m1=nil m2=nil m3=nil`; `expandAlias` in the same shape is protected. Known tag right |
| F-C1-2 0-second repeating timer at 100% CPU | Major (Known #10824) | CONFIRMED | Major | 499 ticks/5 s (99.8 ticks/s = one full core) with the timer on, 2 ticks/5 s after `killTimer`, 0 ticks/5 s idle baseline; 3.29M ticks counted in ~10 s. `ps %cpu` is misleading (3.3) because it averages over process life - /proc ticks are the right measure |
| F-C1-3 repeating timer with empty code string errors forever | Major (Known #10795) | CONFIRMED | Major | Valid id returned; 13 `func reference not found by Lua` error pairs in 6 s, stops only on `killTimer` |
| F-C1-4 multiline AND trigger with a Lua-function script | Major (Known #10736) | PARTIAL | Major | The defect is real, but the report's repro and symptom are both wrong - see Details. Correct result: function script gets `#multimatches == 0` and a populated `matches` (2 entries); the identical string script gets `#multimatches == 2` with both lines' captures |
| F-C1-5 match-all trigger quadratic in line length | Major (New bug) | CONFIRMED | Major | 25 kB 0.405 s, 50 kB 1.486 s on my own run (C1: 0.410/1.489; coordinator: 0.400/1.505) - 3.7x per doubling. 5 MB line not run, per instruction. No tracker issue found; open PR #10717 touches the prescan but is not this |
| F-C1-6 key bound to Alt+E never fires | Minor (Known #10765) | CONFIRMED | Minor | `permKey` returned id 15 with no warning; Alt+E opened the script editor (`shots-V2/03-alt-e.png`) and `V2KEY-ALTE-FIRED` never appeared |
| F-C1-7 `expandAlias(cmd, nil)` suppresses the echo | Minor (Known #10749) | CONFIRMED | Minor | Echo present only after `V2MARK-A`; nothing after `-B` (nil) or `-C` (false) |
| F-C1-8 unset capture groups | Minor (Known #10738) | CONFIRMED | Minor | `#matches=3 m2="" m3="b" m4=nil`; `matches.first=nil matches.second=b` |
| F-C1-9 uncompilable pattern accepted | Minor (Known #10733) | CONFIRMED | Minor | `tempRegexTrigger` -> 55, `tempAlias` -> 93, no message |
| F-C1-10 `showCaptureGroups()` raises on a named group | Minor (Known #10737) | CONFIRMED | Minor | `false / selectCaptureGroup: bad argument #1 type (... got nil!)`; same call on a numbered group returns `true` |
| F-C1-11 stale boundary in 9e6ef57a8's message | Cosmetic (Note) | CONFIRMED | Cosmetic | `adjustStopWatch(sw, 9e15)` -> nil + "must be a finite number from -1000000000000 to 1000000000000" |
| F-A1-6 hands-free middle-button pan accelerates too fast | Minor (Note) | CONFIRMED, worse than described | Minor | At a 50 px cursor offset (A1 used 160) the whole map leaves the ~220 px viewport within ~1.1 s. Click question unresolved by the selection API - see Details. Related open issue #4937 |
| F-E1-3 connection dialog details pane shows another profile | Minor (New bug) | CONFIRMED | Minor | On the very first frame after restart: "My games" highlights `V2 Creds`, the pane reads `Mudlet self-test` / `mudlet.org` / `23` plus the self-test description, and Connect is enabled. Clicking the highlighted row fixes it. Related closed issue #10818 |
| F-E1-6 "loaded password from keychain" for a file read | Cosmetic (Note) | CONFIRMED | Cosmetic | Code path confirmed: the retrieval stage list in `src/CredentialManager.cpp` includes an `encrypted file` stage and the portable/test branch reads the file, while `src/dlgConnectionProfiles.cpp:2895` logs "Successfully loaded password from keychain" on any success |

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
|---|---|---|---|---|---|
| findings-A2.md | 13 (6 "Fixed & verified", 2 "Could not test", 5 "no regression") | All cited files exist (`work-A2/*.log`, `shots-A2/*.png`, `cpu-*.txt` sampled) | 3: 4d6a3ae23, 676a1b321, b150e51d4 | 4d6a3ae23 held (full sequence completes in < 5 s, process idle afterwards); 676a1b321 now hand-verified as fixed, which upgrades A2's "Could not test" | none - but two rows change: 676a1b321 becomes Fixed & verified, b150e51d4 stays untestable for a *structural* reason (see V2-N1) |
| findings-C1.md | 25 (14 "Fixed & verified") | All cited `out*.txt` / `mudlet*.log` / `shots-C1` files present | 10: ec90c3893, 40f64a5d5, 663c937a9, c460994ba, aab5e3806, 4281e05d8, ea3d43a9b, 9e6ef57a8, 744ea965e, d0a7e6485 | all 10 | none |

Re-run detail (C1):
- **ec90c3893**: `command='v2outer thing' m1='v2outer thing' m2='thing'` after a nested `expandAlias` - protection present.
- **40f64a5d5**: self-expanding alias ran 49 times, client survived, log carries `alias processing recursion reached the limit of 50` and the user-facing banner.
- **663c937a9**: match-all `(\d*)` over `café 9` and `日本語 7 x` captures the digit (`13:9 14:9`, `15:7 16:7`) - captures survive the multibyte character. (My first attempt looked empty because of my own filter, not the product; re-run with full capture dumps.)
- **c460994ba**: group 3 -> "World", then group 1 -> "Hello World Again", group 2 -> "Hello", out-of-range -> -1. Matches C1.
- **aab5e3806**: `selectSection(5,-3)` -> false, following `replace("XX")` does not raise or crash.
- **4281e05d8**: `deleteLine()` then `echo`/`cecho`/`send` from the trigger -> ok, no crash.
- **ea3d43a9b**: `appendScript: cannot append to script (script "no such script" at position 1 not found)`.
- **9e6ef57a8**: 2147484 stored exactly; NaN refused with the ±1e12 message.
- **744ea965e**: `permGroup("V2 keys","key")` -> active (isActive 1), `permKey` F5 at one level and F6 two levels deep both fired under `xdotool key F5/F6` (`V2KEY-F5-FIRED`, `V2KEY-F6-FIRED` in `shots-V2/03-alt-e.png` and the log).
- **d0a7e6485**: two 1 s timers (one function, one script string) writing epoch stamps: ticks up to 1789830483, **none** while the emergency stop was armed (stop pressed 1789830484), both resume at 1789830493 (resume pressed 1789830492). `work-V2/ticks.txt`.

Re-run detail (A2):
- **4d6a3ae23 (mandated)**: `work-V2/v2coord.lua` under a 120 s watchdog - 600 ordinary rooms plus one at x=2147483647 and one at x=-2147483648; `centerview` on each, `setMapZoom(400)`, `setMapZoom(1e40)`, `saveMap`, `loadMap`. Whole sequence finished inside the first 5 s poll (`out-coord.txt`: every step `in 0.000 s`, saveMap 0.008 s, loadMap 0.018 s, 609 rooms back), CPU 6 ticks/5 s during and 1 tick/5 s after. Screenshot `shots-V2/14-coordlimit.png` shows the mapper alive with `render time: 0.000S`. **Held up.**
- **676a1b321 (mandated)**: worked around the toolbar being hidden by writing `toolBarVisibility=3` into `$HOME/.config/mudlet/Mudlet.ini` before launch. Order: `createMapper(10,40,400,300)` + 6 rooms **first** (`shots-V2/16-scripted-mapper-toolbar.png`), then the toolbar **Map** button: the embedded mapper is toggled off, **no second mapper window is created** (`xdotool` window list has only the main window, `shots-V2/17-after-map-button.png`); a second press brings it back and it immediately redraws a room added afterwards (`shots-V2/18-map-button-again-room-added.png`). The Toolbox entry reads **"Hide map"** while the mapper is on screen (`shots-V2/19-toolbox-menu.png`), which is the other half of the commit's claim. **Fixed & verified by hand.**
- **b150e51d4 (mandated)**: still could not be driven, for a reason A2 did not state: the two map views are mutually exclusive in *both* directions. With a scripted mapper present, `openMapWidget()` returns nil + "cannot create map widget. Do you already use an embedded mapper?"; with a map widget having existed, `createMapper()` returns nil (#10666). And since 676a1b321 the toolbar Map button toggles the embedded mapper instead of making a second view. So there is no Lua/UI route to "another map view closes while an embedded mapper exists" - the coordinator's suggested ordering does not unblock it. `EmbeddedMapperCreationTest` remains the only coverage.

## New findings

None that rise to a finding. One note:

**V2-N1 (Note): the `createMapper` / `openMapWidget` exclusion is symmetric.**
`openMapWidget()` is refused while a scripted mapper exists ("cannot create map widget. Do you already use an embedded mapper?"), the mirror image of #10666. This is plausibly by design (one `TMap::mpMapper`), but it means a package that calls `createMapper()` locks the user out of the map widget for the session, and it makes b150e51d4's scenario unreachable from Lua. Evidence: `work-V2/mudlet-map.log` lines 140-149.

## Details

### F-A2-1
`work-V2/v2map4.lua` in a session where the map widget had been opened first:
```
M4 createMapper after widget close -> nil / cannot create mapper. Do you already use a map window?
M4 openMapWidget again -> true ; M4 closeMapWidget 2 -> true
M4 createMapper 2 -> nil / cannot create mapper. Do you already use a map window?
```
`work-V2/mudlet-pan.log`. #10666 is open in `recent-issues.md` with the same title - tag correct.

### F-A2-2
`work-V2/v2probe.lua` (`out-probe.txt`), identical 1000-room batches into fresh areas:
0.556 s (map 1609) -> 1.456 s (2609) -> 2.431 s (3609) -> 3.152 s (4609). The per-batch
cost rises about linearly with the rooms already on the map, so a full build is quadratic;
consistent with A2's larger numbers and with `TMap::createNewRoomID()` walking ids from 1.

### F-C1-4 (why PARTIAL)
`tempComplexRegexTrigger(name, ...)` called twice with the same name does **not** add a
pattern to a live trigger: it reads the old patterns, `killTrigger`s it and builds a new
trigger whose script is *this* call's script (`src/TLuaInterpreterMudletObjects.cpp:2707-2752`).
C1's repro puts the recording function on the **first** call and an empty function on the
second, so the recorder is thrown away - which is why their `_G.M` was nil, and why my
replay of it showed the trigger "never fired" for the string script too (`out-c1c.txt`).
With the recorder on the last call (`work-V2/v2c1d.lua`), the real behaviour appears:
```
multiline AND, FUNCTION script: mm=0    mm11=nil mm12=nil mm22=nil m=2
multiline AND, STRING script  : mm=2    mm11=mls a one mm12=one mm22=two m=0
```
So #10736 is live and function-specific, but the symptom is "`multimatches` is empty and
`matches` holds only the last line", not "neither matches nor multimatches".

### F-C1-2
`/proc/<pid>/stat` fields 14+15, three 5 s windows: idle 0 ticks, timer running 499 ticks
(99.8/s = 100% of one core), after `killTimer` 2 ticks. Counter reached 3,293,619 ticks.

### F-C1-5
`work-V2/v2scale.lua` (C1's trigger `tempComplexRegexTrigger("V2Big", [[(\w*)]], ... matchall=1)`):
25 kB 0.405 s (VmRSS 161.8 MB), 50 kB 1.486 s (162.9 MB). Independent of C1's and the
coordinator's numbers to within 2%. Doubling the line costs 3.7x, i.e. ~O(n^1.9). The 5 MB
case was not run per instruction.

### F-A1-6
Map built from `work-A1/build.lua`, map widget docked right (viewport ~ x 893-1115),
editing mode via the context menu (`shots-V2/22-editing-mode.png`). Middle-click once at the
viewport centre (1004,420) without holding, then the cursor moved 50 px to (1054,420);
screenshots every ~0.55 s (`shots-V2/23-pan-0..5.png`). Frame 0 shows the five rooms, frame 1
is already nearly empty, frames 2-5 are pixel-identical to the empty end state
(`compare -metric AE` of the viewport crop: 1047, 608, 0, 0, 0 against frame 5). So with the
cursor only 50 px off centre the map crosses the whole ~220 px viewport in about one second -
faster than A1's 160 px description implies.
Whether a room can be left-clicked while it drifts: **cannot be decided through
`getMapSelection()`** - a left click on the player room with the map *standing still*
(`shots-V2/26-control-before-click.png`, room at (1007,420), clicked at (1004,420)) also
reports `rooms=nil`, so the API does not register single-click selection here and cannot
arbitrate. What is observable: the rooms are out of the viewport within about a second of
starting the pan, so there is nothing left under the cursor to click.

### F-E1-3
Fresh HOME, `mudlet` with no `--profile`, "No" to the telnet handler, "Skip - show me the
games list", two profiles created ("V2 Creds" with mudlet.org:23, "V2 Offline NoAddress"
with no address), quit, restart. First screenshot after the dialog appears,
`shots-V2/35-restart-dialog.png`: My games highlights **V2 Creds**; the Connect to pane reads
Profile name **Mudlet self-test**, Server address mudlet.org, Port 23, and the Information
box shows the self-test text ("This isn't a game profile, but a special one for testing
Mudlet itself using Busted") - a profile that is not in My games at all. Connect is enabled.
`work-V2/mudlet-e1b.log` line 14 confirms the dialog also runs the credential lookup for
"Mudlet self-test" before any click. After clicking the highlighted row the pane corrects
itself to V2 Creds (`shots-V2/crop-36.png`).

### F-E1-6
`src/CredentialManager.cpp:380-470`: the lookup stage list contains
`stages.push_back({qsl("encrypted file"), ...})`, and `shouldUseKeychain()` returns false in
portable/test mode so the password is read straight out of the file; in both cases the
callback reports `success = true`. `src/dlgConnectionProfiles.cpp:2895` then logs
"dlgConnectionProfiles: Successfully loaded password from keychain for <profile>". E1's
`work-E1/mudlet-restart.log:25` shows the line in practice after the keychain reads failed
(line 20 of the same file). Cosmetic, confirmed.

## Tracker cross-check
- Confirmed open and correctly tagged in `recent-issues.md`: #10666, #10796, #10824, #10795,
  #10736, #10765, #10749, #10738, #10733, #10737. A2's #9768 (createRoomID) also open.
- F-C1-5: semantic issue search ("match-all trigger slow on a long line, quadratic, freezes")
  returns nothing matching; the nearest open PR is #10717 (trigger prescan), which is not this.
  Stays **New bug, untracked**.
- F-A1-6: closest is open #4937 "Don't allow the mapper to be completely scrolled out of the
  view" - suggest tagging the note `Known: #4937 (related)`. #10531 (click ends the pan) is
  closed and is the commit under test.
- F-E1-3: closest is **closed** #10818 "Connection popup should show a closed profile's
  connection details automatically" - if that is the behaviour bc4c8f9e8 delivered, F-E1-3 is
  a **Fix incomplete** against it rather than a plain new bug.
- F-E1-6: no tracker match (only closed #1917 about password encryption).
