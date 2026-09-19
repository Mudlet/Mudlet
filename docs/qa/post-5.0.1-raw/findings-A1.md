# A1 findings

Area: Mapper interaction, labels, custom exit lines, context menu, panning.
Build: `development` @ dbbf040c3, `build-linux-debug-nosan/src/mudlet`, Xvfb :81.

## Summary
Drove the 2D mapper by hand under Xvfb: built a map from Lua (areas, rooms, exits,
a custom exit line, a map label), opened the map widget, worked the context menu in
both viewing and editing mode, created a label by dragging a box out, cleared the map
underneath the open label dialog, created a new map from both modes, and ran the
hands-free middle-button pan. The crash fix 683a44258, the menu-lifetime fix 2f5e3b366,
the label-placement fix fa54ef132 and the editing-mode fix 6aecd138a all behave as their
commit messages claim. Every ctest class named for this area passes (7/7).
Not verified by hand inside the time box: Move to position / Spread / Shrink carrying
custom lines (f585cb5f7, 977900fc6), dragging a label (553c91251), the Exits dialog
(c9669fba6, c881de953) and the area-switch selection (a7cf7a2db) - all are covered by
the ctest classes that pass, and that is recorded as such rather than as a pass by hand.
Two things to flag: the four Configure-areas bugs #10542-#10545 are still live and are
NOT regressions (T2DMap::slot_configureAreas is byte-identical to 5.0.1), and one Mudlet
process disappeared mid-session with no crash text in its log and could not be reproduced.

## Findings

| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-A1-1 | Major | (none - pre-existing) | Configure areas: create/rename/delete never marks the map unsaved, so the change is not autosaved (#10543) | Note |
| F-A1-2 | Major | (none - pre-existing) | Configure areas: Delete destroys the area and its rooms with no confirmation, and the map goes blank (#10545) | Note |
| F-A1-3 | Major | unknown | Mudlet process disappeared during mapper interaction, no crash text, not reproduced | Note |
| F-A1-4 | Minor | (none - pre-existing) | Configure areas: preselected row is off by one when the default area is hidden (#10542) | Note |
| F-A1-5 | Minor | (none - pre-existing) | Configure areas: renaming an area that is not shown moves the dropdown but not the map (#10544) | Note |
| F-A1-6 | Minor | 0c66eb275 / 342cfc6c3 | Hands-free middle-button pan accelerates so fast the map is empty within a second | Note |

### F-A1-1: Configure areas: create/rename/delete never marks the map unsaved (#10543)
**Steps:** mapper right-click menu -> `Configure areas...` -> Create, Rename or Delete an area.
**Expected:** the map is marked as needing a save, the way `addAreaName`/`setAreaName`/`deleteArea`
and every other mapper edit do, so `Host::autoSaveMap` writes it out.
**Actual:** none of the three handlers calls `TMap::setUnsaved`, so the edit survives only if some
other action marks the map. An autosave (or a crash) after an area rename keeps the old name.
**Evidence:** `git show dbbf040c3:src/T2DMap.cpp` - `slot_configureAreas` contains no `setUnsaved`
call (`grep -c setUnsaved` over the extracted function = 0). Upstream issue #10543 describes the
same. The function body is identical to `git show Mudlet-5.0.1:src/T2DMap.cpp` (diff of the two
extracted function bodies is empty, 200 lines each) - so this is NOT a regression against 5.0.1.
Files: `/tmp/claude-0/-home-user-Mudlet/3123fe1e-ea5a-5043-97a2-95f34f40ac2d/scratchpad/qa/work-A1/ca-501.txt`,
`.../work-A1/ca-dev.txt`.
**Commits:** none in range fixes it; 3280a5502 only added tests and its PR body says the fixes
"follow in a stacked PR" (PR #10597), which is not in `Mudlet-5.0.1..dbbf040c3`.

### F-A1-2: Configure areas: Delete destroys an area and its rooms with no confirmation (#10545)
**Steps:** pick area B in the mapper dropdown, right-click a room -> `Configure areas...` -> Delete.
**Expected:** a confirmation for a destructive action, and the map keeps showing something.
**Actual:** the handler's own comment is `// Delete immediately without confirmation`; it calls
`TRoomDB::removeArea` straight away, which takes the area's rooms with it (3280a5502's own test
case is described as "Delete removing the area and its rooms everywhere"). Afterwards
`T2DMap::mAreaID` still names the deleted area, so `paintEvent` draws nothing and the dropdown
falls back to its first entry - the map is blank while the dropdown names a different area.
**Evidence:** the delete handler in `.../work-A1/ca-dev.txt`; upstream issue #10545. Identical
code in 5.0.1 (see F-A1-1 evidence), so not a regression.
**Commits:** none in range.

### F-A1-3: Mudlet process disappeared during mapper interaction, not reproduced
**Steps (what led up to it):** editing mode, map cleared with `deleteMap()`, `Create new map` used
from the no-map context menu, then the empty-space context menu opened, `Escape`, a left click in
the main console, `ctrl+a`, `Delete`, then typing a `lua dofile(...)` line.
**Expected:** Mudlet keeps running.
**Actual:** the next screenshot was an entirely black root window; no Mudlet window remained on
display :81 and the process was gone. `--mirror` log ends cleanly on the previous command's output
(`main| true` from `lua deleteMap()`) with no Qt fatal, no assertion and no backtrace; stderr was
redirected into the same file, so a `qFatal` would have been captured. A SIGSEGV would leave no
text, so a silent crash cannot be excluded.
**Evidence:** `.../work-A1/mudlet-run1.log` (last lines quoted above); last good screenshot
`.../shots-A1/22-viewmode-after-newmap.png`, first bad one `.../shots-A1/23-map2.png` (all black).
**Not reproduced:** the session was restarted (with the exit status wrapped) and the same
sequence of menu/Escape/console-click actions did not kill it again within the time box.
Recorded as a Note, not a bug, because there is no reproduction and no crash evidence.
**Commits:** unknown.

### F-A1-4: Configure areas preselects the wrong row when the default area is hidden (#10542)
**Steps:** map with two areas, default area hidden (the default) -> `Configure areas...`.
**Expected:** the row for the area the mapper is showing is selected.
**Actual:** the dialog uses `mpMap->mpMapper->getCurrentShownAreaIndex()` as a row index into a
list that always includes the default area, so the row is off by one; on the first area the
dialog starts on "Default Area (-1)" with Rename and Delete greyed out.
**Evidence:** `.../work-A1/ca-dev.txt` (the `currentAreaIndex` block); issue #10542. Same code in
5.0.1, so not a regression. **Commits:** none in range.

### F-A1-5: Renaming an unshown area moves the dropdown but not the map (#10544)
**Steps:** show area A, `Configure areas...`, select area B in the list, Rename it.
**Expected:** the dropdown keeps naming the area the map is drawing.
**Actual:** the rename handler ends with `comboBox_showArea->setCurrentText(newName)` regardless of
which area is shown; `setCurrentText` does not emit `activated`, so the map stays on A while the
dropdown reads B's new name.
**Evidence:** `.../work-A1/ca-dev.txt` (the rename handler's last block); issue #10544. Same code
in 5.0.1, so not a regression. **Commits:** none in range.

### F-A1-6: Hands-free middle-button pan accelerates too fast to click anything
**Steps:** editing mode, middle-click once without holding at the widget centre, then move the
mouse about 160 px to one side.
**Expected:** the map drifts that way at a speed that lets you steer it.
**Actual:** with the cursor 160 px from centre the whole map (3 rooms plus a label) leaves the
570 px-wide viewport in under a second; at 250 px it is gone almost immediately. A room the
cursor was aimed at has moved out from under it before a click lands, which is why the
"click a room to stop the pan" case below could only be half-demonstrated.
**Evidence:** `.../shots-A1/26-before-pan.png` (rooms centred) -> `.../shots-A1/27-panning.png`
and `.../shots-A1/28-panning2.png` (empty viewport, pan cursor still shown), 1.5 s apart.
Arguable as a design choice, so filed as a Note. **Commits:** 0c66eb275 (tests only), 342cfc6c3.

## Coverage

| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| 683a44258 | Crash cancelling the map label dialog after the map was cleared | Fixed & verified | Editing mode -> `Create label...` -> dragged a box out -> with the dialog open ran `lua deleteMap()` (log: `TRoomDB::clearMapDB() run time: 0.00017052 sec.`), typed `STALE` into the dialog's text box, then pressed Cancel. Process alive throughout (`ALIVE after typing`, `ALIVE after cancel`); the replacement empty map was untouched. Shots 14-17. |
| f585cb5f7 | Move to position takes a room's custom exit lines with it | Verified by ctest only | `MapMouseInteractionTest` passes (5.71 s, 0 failures). The by-hand Move-to-position run did not fit the time box; the room menu item was confirmed present (shot 09/25). |
| 977900fc6 | Spread and Shrink no longer skew custom exit lines | Verified by ctest only | `MapMouseInteractionTest` passes. Not driven by hand (time box). |
| fa54ef132 | A label dragged out upwards or to the right lands where it was drawn | Fixed & verified | `Create label...`, then dragged a box from (920,588) up and to the right to (1000,540). The label preview landed inside that box (x 920-1000, y 543-585), not a box-height below it. Shots 12-14. Click-without-drag not exercised by hand; its case is in the passing ctest class. |
| 6aecd138a | Creating a new map no longer flips the mapper into viewing mode | Fixed & verified | From editing mode: `deleteMap()`, `Create new map` from the no-map menu, right click -> menu offers "Switch to viewing mode", i.e. still editing (shot 20). Repeated from viewing mode: after `Create new map` the menu again offers "Switch to viewing mode", i.e. it lands in editing (shot 22). Both halves of the commit's test case. |
| 342cfc6c3 | Clicking a room while editing ends a hands-free middle-button pan | Partially verified | Middle-clicked once without holding, moved the mouse off centre so the map drifted, then left clicked: the drift stopped (shots 34 and 35, taken 1.5 s apart, differ by 0 pixels under `compare -metric AE`). The "room is selected" half could not be shown: the drift (F-A1-6) moves the room out from under the cursor before the click lands, and `getMapSelection().rooms` came back empty. `MapMouseInteractionTest` covers it. |
| 2f5e3b366 | Mapper context menu items no longer pile up on the map widget | Fixed & verified | Right-clicked empty map 20x and a room 20x (Escape between each), then opened both menus again: empty-space menu still exactly 5 items (Create new room here / Configure areas... / Create label... / Export area to image... / Switch to viewing mode), room menu still exactly 12, no duplicates. Shots 09 and 11. `MapContextMenuLifetimeTest` passes. |
| 553c91251 | A map label you pick up follows the mouse again | Verified by ctest only | `MapMouseInteractionTest` passes; label drag not driven by hand (time box). |
| c9669fba6 | Editing a room's exits redraws the map and marks it unsaved | Verified by ctest only | `RoomExitsDialogTest` passes (0.28 s). |
| bad7d7f4c | Room exits dialog no longer leaks its two column delegates | Verified by ctest only | Test-only commit; `RoomExitsDialogTest` passes with no XPASS reported. |
| c881de953 | Crash after deleting the special exit being edited | Verified by ctest only | `RoomExitsDeletedEditItemTest` passes (0.20 s). |
| ace14e781 | Re-applying a room or exit lock no longer marks the map as changed | Verified by ctest only | `MapLockNoOpTest` passes (1.12 s). No Lua getter exposes the map's unsaved flag, so a by-hand check would need the central debug console; not done inside the time box. |
| a7cf7a2db | Switching map areas no longer leaves rooms selected off-screen | Verified by ctest only | `MapAreaSelectionTest` passes (0.23 s). Observed in passing that right-clicking empty space while rooms are selected still offers the room menu (expected: the menu acts on the selection). |
| 30519ac23 | Infra: tests for the mapper's custom exit lines | Verified no regression | Test commit plus small handler changes; `MapMouseInteractionTest` passes. Custom exit lines render and are created from Lua (`addCustomLine` with a point list returns true and the line appears in `getCustomLines`). |
| 0c66eb275 | Infra: tests for the mapper's middle-button pan | Verified no regression | Hands-free pan armed by a single middle click drifts in the direction of the cursor and stops on the next button press, as the commit describes. See F-A1-6 for the speed. |
| 3280a5502 | Infra: mapper tests for the Configure areas dialog | Bug: F-A1-1, F-A1-2, F-A1-4, F-A1-5 | The dialog's four known bugs (#10542-#10545) are still live: no fix for them is in `Mudlet-5.0.1..dbbf040c3` (the stacked PR #10597 has not landed). `T2DMap::slot_configureAreas` is byte-identical between `Mudlet-5.0.1` and `dbbf040c3`, so all four are pre-existing, not regressions - recorded as Notes. |

## Automated
ctest (`-R 'MapMouseInteraction|RoomExits|MapContextMenu|MapLockNoOp|MapAreaSelection|RoomProperties'`,
offscreen): **7 passed / 0 failed** - MapContextMenuLifetimeTest, RoomExitsDialogTest,
MapAreaSelectionTest, MapMouseInteractionTest, MapLockNoOpTest, RoomPropertiesDialogTest,
RoomExitsDeletedEditItemTest. Log: `<scratchpad>/qa/work-A1/ctest-A1.log`.
Neither baseline failure (CredentialManagerKeychainTest, HomeUntouchedTest) is in this area.
No Lua specs were run for this area: the mapper specs (`Mapper`, `MapViewportLimits`, `RoomDB`,
`GeyserMapper`) belong to A2 and the full run was green in the baseline (4698/0/0).

## Not testable here
Nothing in this area is platform-specific. What is missing is by-hand coverage of
f585cb5f7, 977900fc6, 553c91251, c9669fba6, c881de953 and a7cf7a2db, which ran out of the
45-minute time box rather than being impossible; each is covered by a ctest class that passes.
