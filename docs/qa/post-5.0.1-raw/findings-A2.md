# A2 findings

Area: Mapper - rendering, large maps, coordinate limits, pathfinding, level
colours, map buttons, map download, symbols. Tree 85d814292. Display :82.
Work dir: `<scratchpad>/qa/work-A2/`, screenshots `<scratchpad>/qa/shots-A2/`.

## Summary

Built maps of 5k / 20k / 21k rooms from Lua in the running profile and drove the
mapper through them: zoom in and fully out, hands-free pan, area switch, save,
reload with the load-time audit, and pathfinding before and after adding rooms.
The headline fix of the batch (4d6a3ae23, the coordinate-limit freeze) holds:
a room at x = 2147483647 and one at INT_MIN, centred at zoom 400 and at
`setMapZoom(1e40)`, and a `saveMap`/`loadMap` round trip of that map, all return
promptly with the process at 0% CPU afterwards. Pathfinding (7bd8ac2c1) is fast
and correct: a 199-step walk on a 21k-room map costs ~1 ms and returns the same
199 steps on every run, and 20 consecutive `getPath()` calls on a hand-built map
with a decoy detour all return the shortest route (#10180 not reproduced here).
Two findings: one Major that is a confirmed-live known issue (#10666, `createMapper()`
dead after `openMapWidget()`+`closeMapWidget()`), and one Minor performance Note -
bulk map building is still superlinear, which is why the 50k-room target could not
be reached inside the time-box (20k rooms took 187 s).
Not run here: the interactive map-download Abort (needs an MMP map URL from a
game; no Lua entry point), the Preferences colour picker's alpha control, and
the main-toolbar Map button under each `mapperButton` mode (the main toolbar is
not shown in the self-test profile) - all noted in Coverage with the reason.
ctest for the area: 17/17 pass. Area specs: 429 successes / 0 failures / 0 errors.

## Findings

| ID | Severity | Commit | Title | Status |
|----|----------|--------|-------|--------|
| F-A2-1 | Major | 676a1b321 / b150e51d4 (adjacent) | `createMapper()` is refused for the rest of the session after `openMapWidget()` then `closeMapWidget()` | Note - Known: #10666 (confirmed live) |
| F-A2-2 | Minor | 714efcaf9 / 82342c735 (adjacent) | Bulk map building is still superlinear: per-room cost grows with the map and with the target area | Note - Known: #9768, Closed: #9759, Fix pending: PR #10581 and draft PR #10591 |

### F-A2-1: `createMapper()` is refused for the rest of the session after `openMapWidget()` then `closeMapWidget()`

Steps (script `work-A2/mapper2.lua`, driven with `work-A2/send.sh`):

```
HOME=$(mktemp -d) DISPLAY=:82 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 \
  DBUS_SESSION_BUS_ADDRESS=disabled: MUDLET_TEST_MODE=1 \
  ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror
# dismiss the tour, then in the input line:
lua openMapWidget()
lua dofile("<work>/mapper2.lua")     -- closeMapWidget(); createMapper(10,40,400,300); repeat
```

Expected: after the map widget is closed, a script can create its own embedded
mapper with `createMapper()`.

Actual: every `createMapper()` after the first `openMapWidget()` returns `nil`
plus `cannot create mapper. Do you already use a map window?`, and re-opening
and re-closing the widget does not clear it:

```
main| M2 closeMapWidget ->            main| true
main| M2 createMapper after close ->  main| nil
main| cannot create mapper. Do you already use a map window?
main| M2 openMapWidget again ->       main| true
main| M2 closeMapWidget 2 ->          main| true
main| M2 createMapper 2 ->            main| nil
main| cannot create mapper. Do you already use a map window?
```

Evidence: `work-A2/mudlet-b2.log` (grep `M2 `), screenshot
`shots-A2/24-createmapper.png`.

Commits: not caused by anything in this batch - 676a1b321 and b150e51d4 are the
two commits that repair the neighbouring "which widget owns `TMap::mpMapper`"
problems, and this route is the one they leave unfixed. Recorded because the
area notes name it and it is live on this tree. Severity Major because a UI
package that ever meets an opened-and-closed map widget cannot build its mapper
until the profile is restarted.

### F-A2-2: Bulk map building is still superlinear in map size and in area size

Steps (scripts `work-A2/genmap2.lua` - explicit room ids, no `createRoomID()` -
and `work-A2/addrooms.lua` - 1000 rooms in a brand-new area using `createRoomID()`):

```
lua AREAS=20; ROOMS_PER_AREA=250;  dofile("<work>/genmap2.lua")   # 5 000 rooms
lua AREAS=20; ROOMS_PER_AREA=1000; dofile("<work>/genmap2.lua")   # 20 000 rooms (fresh profile)
lua PROBE_NAME="Probe1"; dofile("<work>/addrooms.lua")            # +1 000 rooms
```

Measured (Debug build, `getStopWatchTime`, quoted from the log):

| what | rooms | time | per 1000 rooms |
|---|---|---|---|
| `genmap2` 20 areas x 250, explicit ids | 5 000 | 10.18 s | 2.04 s |
| `genmap2` 20 areas x 1000, explicit ids | 20 000 | 186.56 s | 9.33 s |
| `addrooms` 1000 rooms, new area, on a 20k map | 1 000 | 24.99 s | 24.99 s |
| first attempt, 20 areas x 250 with `createRoomID()` | 5 000 | 11.92 s | 2.38 s |

So 4x the rooms costs 18x the time, and the same 1000-room batch costs 2.0 s on
a small map and 25.0 s on a 20k one. `TMap::createNewRoomID()` (`src/TMap.cpp:562`)
still walks ids from 1 on every call, which is issue #9768 and is unchanged since
2010 (`git log -S 'while (mpRoomDB->getRoom(++_id));' Mudlet-5.0.1..HEAD -- src/TMap.cpp`
is empty), but the explicit-id rows above show a second, per-area-size cost that
`createRoomID()` does not explain.

Expected: adding a room costs about the same whatever the map already holds.
Actual: as above. **Not a regression** - no 5.0.1 build was made, and the code
behind the `createRoomID()` half predates 5.0.1 by years. Recorded because it is
the reason the 50k-room target in the area notes could not be built inside the
time-box, and because issue #9759 ("setRoomArea() is O(rooms already in the
source area), making bulk map building quadratic") is closed while a per-area-size
cost is still measurable on this tree.

Evidence: `work-A2/mudlet-b2.log` and `work-A2/mudlet-big.log` (grep `GENMAP2`,
`PROBE`), `work-A2/mudlet-main.log` (grep `GENMAP`).

## Notes (not findings)

- **A\* heuristic still does `int` arithmetic at the coordinate limits.**
  `src/TAstar.h:88-90` computes `CostType dx = m_location[...]->x() - m_location[...]->x();`
  - the subtraction happens in `int` and only then converts, so it overflows for
  extreme coordinates. That is the second half of open issue #10688. I could not
  turn it into a user-visible defect: `getPath()` over a 3-room chain ending on
  the x = 2147483647 room returned the correct 3-step route
  (`EXTREME2 ok=true steps=3 t=0.1860 s path=21599,21600,21601`) and the process
  stayed at 0% CPU. Code reading only, not reproduced.
- `setMapZoom()` has a documented lower bound of 3 ("zoom 0.2 is invalid, it must
  be at least 3") and no upper bound, as 4d6a3ae23 says it intends. Larger values
  are further out; `setMapZoom(500)` on the 21k map drew the reduced-detail tier
  in 0.002 s (`shots-A2/16-zoom500-21k.png`).
- There is no `auditMap()` Lua function; the audit was timed through the
  load-time route instead (0.39 s for 21 602 rooms).

## Coverage

| Commit | Subject | Verdict | How verified |
|---|---|---|---|
| 4d6a3ae23 | 2D mapper no longer freezes at the coordinate limit | Fixed & verified | `work-A2/coordlimit.lua` built a 600-room area plus a room at x=2147483647 and one at x=-2147483648 (log: `LIMIT coords max: 2147483647`). `centerview(21601)` + `setMapZoom(400)` returned and printed `STEP done400`; `setMapZoom(1e40)` printed `STEP done1e40`; the same at the INT_MIN room printed `STEP donemin`. `work-A2/cpuwatch.sh` sampled `/proc/<pid>/stat` for 20 s after each: 0-1 ticks/s (idle), files `work-A2/cpu-zoom400.txt`, `cpu-zoom1e40.txt`, `cpu-min.txt`. `saveMap` of that state wrote 2.8 MB and `loadMap` of it returned in 0.613 s with 21 602 rooms (`STEP loadtime 0.613 s rooms=21602`), audit 0.39 s, CPU idle after (`cpu-load.txt`). Screenshots `shots-A2/20-intmax-zoom400.png`, `21-intmax-zoom1e40.png`, `22-intmin-zoom400.png`, `23-after-loadmap.png`. `ctest -R 'TAreaGridIndex\|MapCoordinateLimit'` passed; `MapViewportLimits_spec` passed. |
| 714efcaf9 | Much faster 2D map rendering on very large maps | Verified no regression | 21k-room map over 20 areas and 5 z-levels. Close zoom (3 and 10) draws rooms, exits and the player highlight normally (`shots-A2/14-zoom10.png`, `15-zoom3.png`); fully zoomed out at 500 the 1000-room area renders as the reduced-detail block with the mapper's own overlay reporting `render time: 0.002S` (`shots-A2/16-zoom500-21k.png`). Middle-button hands-free pan and the area dropdown stayed responsive (`shots-A2/17-panned.png`). `MapLevelOfDetailTest`, `MapOffscreenCustomLineTest`, `MapRoundTripTest` passed. Not verified: the published 2.3M-room benchmark numbers (that map is not available here). |
| 82342c735 | Faster still when zoomed out (LOD exit index) | Verified no regression | Same 21k map: added 1000 rooms and new exits (`addrooms.lua`, `setExit` calls in the EXTREME2 step) while zoomed out; exits kept drawing and later `getPath` used them, so the per-room re-filing of the index stayed in step. Covered by `MapLevelOfDetailTest` (passed). Byte-identical-frame claim not re-measured. |
| 7bd8ac2c1 | Pathfinding no longer slows down as your map grows | Fixed & verified | On the 20k map, `work-A2/path2.lua`: `getPath_far_run1/2/3 = 0.00100 / 0.00100 / 0.00000 s -> true steps=199` and `getPath_1step = 0.00000 s -> true steps=1`; the mapper's own log line reads `TMap::findPath( 1 , 6 ) INFO: time elapsed in A*: 0.015518 ms`. After adding 1000 rooms the first call costs 0.171 s (the expected graph rebuild) and the next two return to 0.001 s / 0.000 s, still `steps=199` - same route length before and after. Correctness: `work-A2/shortest.lua` builds an 8-room map with a 4-step detour decoy and runs `getPath(1,5)` 20 times: `SHORTEST route true:4:2,3,4,5 x20` (#10180 not reproduced). |
| 95cdbf20d | Alpha channel for map level colours | Fixed & verified (export/import); dialog half not driven | `saveProfile()` then grep of the written profile XML shows `<mLowerLevelColor alpha="255">#808080</mLowerLevelColor>` and the matching `mUpperLevelColor` - the attribute the commit adds. Import half covered by `ProfileRoundTripTest` in the passing ctest run. Not driven: the Preferences colour picker's alpha slider (needs the settings dialog; E2's area). |
| 41890b480 | Stop building unused translated exit names during map audit | Verified no regression | Load-time audit of the 21 602-room map completed with `[  OK  ] - Auditing of map completed (0.39s)` and no spurious warnings; rooms, exits and the 600-room limit area survived the round trip (`getRooms()` = 21602 after load). The 7-8% speed claim is below the noise of a Debug build and was not re-measured. |
| df2a5beb9 | Keep the loaded map when a download is canceled or returns no map | Fixed & verified (no-map half); cancel half could not be driven | With 8 rooms loaded, `loadMap("<work>/notamap.xml")` (an HTML page) returned `nil` plus `loadMap: the file: "..." does not contain a map, so the current map has been left as it was.` and `getRooms()` was still 8 afterwards (log `DL rooms before 8` / `DL rooms after 8`). `MapDownloadTest` (3 cases) and `Mapper_spec` passed. Could not test interactively: pressing Abort on the download progress dialog - the download is only reachable through an MMP map URL supplied by a game, and there is no Lua entry point (`downloadMap` is not registered; only `downloadFile` is), so no local HTTP server can start it. The stalling server `work-A2/httpstall.py` was written but not used for this reason. |
| f2c2681fd | Map room symbol font settings in setConfig/getConfig | Fixed & verified | `work-A2/misc.lua`: `getConfig("mapSymbolFont")` -> `Bitstream Vera Sans Mono`; `setConfig("mapSymbolFont","DejaVu Sans")` -> `true` and reads back `DejaVu Sans`; `setConfig("mapSymbolFontScaling",0.5)` -> `true`, reads back `0.5`; `setConfig("mapSymbolFontScaling",99)` -> `nil` + `mapSymbolFontScaling 99 is out of range, it must be between 0.50 and 2.00` and the stored value stays `0.5`; `mapSymbolFontOnlyUseSelected` set and read back `true`. All three work with no mapper open. `MapSymbolFontTest` and `Other_spec` passed. Not seen: the glyph-missing warning string (the fonts tried all had glyphs for the symbols in use). |
| 676a1b321 | Map button no longer builds a second mapper over a scripted one | Could not test (main toolbar not available) / see F-A2-1 | The self-test profile shows no main toolbar, so the toolbar Map button could not be pressed, and `createMapper()` could not be reached at all after a map widget had been opened and closed (F-A2-1), which is the state this commit is about. `EmbeddedMapperCreationTest` passed in ctest. Claimed, not evidenced by hand. |
| bea8e7e61 | mapperButton setting decides what the built-in map buttons do | Fixed & verified (API half); button press not driven | `getConfig("mapperButton")` -> `default`; `setConfig` accepted `default`, `scripted` and `disabled` and each read back, and `bogus` was refused with `mapperButton must be "default", "scripted" or "disabled", got "bogus"` leaving the previous mode in place. `MapperButtonConfigTest` passed. Not driven: pressing a built-in map button in each mode and catching `sysMapperButtonAction` - no main toolbar in this profile. |
| b150e51d4 | Embedded mapper keeps working after another map view closes | Could not test (blocked by F-A2-1) | The scenario needs a `createMapper()` mapper plus a second view; `createMapper()` is refused once a map widget has been opened and closed in the session, and opening the widget is how a second view is made here. `EmbeddedMapperCreationTest` passed in ctest. Claimed, not evidenced by hand. |
| e268326a4 | Map-audit flag homed on TMap | Verified no regression | The audit still runs and still reports through the profile console on `loadMap` (`[  OK  ] - Auditing of map completed (0.39s)` in `work-A2/mudlet-b2.log`), which is the behaviour the moved flag gates. The Preferences tick-box and its persistence across restart were not driven (settings dialog is E2's area). |
| ed43d2fea | Host no longer builds its user-window and mapper widgets | Verified no regression | `openMapWidget()` built the docked map and `openMapWidget(180,140,900,560)` floated and sized it (`shots-A2/03-mapwidget-5k.png`, `04-map-float-5k.png`); the map drew, took a new area from the dropdown and survived `closeMapWidget()`/`openMapWidget()` cycles. `createMapper()` itself could not be exercised (F-A2-1). `MapperPanelTest`, `EmbeddedMapperCreationTest` passed. |

## Automated

ctest (`-R 'TAreaGridIndex|TAreaSpanIndex|TAreaZLevelIndex|MapCoordinateLimit|MapLevelOfDetail|MapDownload|MapperButtonConfig|MapperPanel|EmbeddedMapper|MapSymbolFont|MapRoundTrip|MapFileStats|MapRoomGeometry|MapAreaReassignment|MapOffscreenCustomLine|MapProgressDialogSeam|MapCloseDuringImport'`):
**17 passed / 0 failed** - TAreaSpanIndexTest, TAreaZLevelIndexTest, TAreaGridIndexTest,
EmbeddedMapperCreationTest, MapAreaReassignmentTest, MapCloseDuringImportTest,
MapCoordinateLimitTest, MapDownloadTest, MapLevelOfDetailTest,
MapOffscreenCustomLineTest, MapFileStatsTest, MapperButtonConfigTest,
MapperPanelTest, MapProgressDialogSeamTest, MapRoomGeometryTest,
MapRoundTripTest, MapSymbolFontTest. Log: `work-A2/ctest-A2.log`.

Specs (`TESTS_DIRECTORY=work-A2/specs` over `Mapper_spec`, `MapViewportLimits_spec`,
`RoomDB_spec`, `GeyserMapper_spec`, with `fixtures/` symlinked in):
**429 successes / 0 failures / 0 errors / 17 pending** in 6.86 s.
Log: `work-A2/specs-A2b.log`. (The first run, `specs-A2.log`, showed 8 errors
caused only by the missing `fixtures/` symlink in my subset directory - not a
product failure.)

Both match the baseline (220/220 ctest, 4702/0/0 busted); no failure in this area.
