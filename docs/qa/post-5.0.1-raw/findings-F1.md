# F1 findings

Area: Console, labels, Geyser containers, links, user windows (visual, Xvfb :81)
Tree under test: 85d814292 (= development 12b373743). Binary: `build-linux-debug-nosan/src/mudlet`.
Work dir: `<scratchpad>/qa/work-F1/`  Screenshots: `<scratchpad>/qa/shots-F1/`

## Summary

All 32 commits in scope were exercised; 14 carry a directly checkable user-visible
claim and every one of those held up (negative wrap indent refused, miniconsole rows
correct at creation, label click callback with a link inside, label background across a
stylesheet, 50 kB label echo at 0.06 ms, percent-sized attached containers scaling with
the window, console text following the main window through a drag, no text lost when a
user window is dragged over the output, `copy2decho` off-line, `startLogging` failure
reported, no crash using a scroll box/command line after its window went). Nine
defects are live on this tree; eight are already on the tracker and one (F-F1-2) is an
incomplete fix for an issue the tracker believes closed. ctest 23/23 and 1870 spec
successes / 0 failures, matching baseline. Not runnable here: 35f0a72b9 (Windows 11
scroll-bar style, platform-gated) and the mouse-cursor *shape* half of caa26b20f
(`import -window root` does not capture the X cursor image).

## Findings

| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-F1-1 | Major | 6686b97ea | Console keeps painting at the old border after a container's own edge drag, clipping text | Known: #10593 / Fix pending: PR #10594 |
| F-F1-2 | Major | c0309561b | A scroll box outlives its deleted user window as an undeletable widget over the main console | Fix incomplete (Closed: #10319) |
| F-F1-3 | Major | d3f5f873b | Two containers on opposite borders still reserve the whole axis, leaving the console 0 rows | Known: #10617 |
| F-F1-4 | Minor | 6686b97ea | `hide()` on an attached container leaves its border reserved as a blank strip | Known: #10747 |
| F-F1-5 | Minor | 6686b97ea | A second `minimize()` discards the real height, `restore()` stays collapsed | Known: #10744 |
| F-F1-6 | Minor | 95a20c600 | `setPadding(-5)` / `setPadding("abc")` raises and still writes the bad value | Known: #10745 |
| F-F1-7 | Minor | 95a20c600 | `lockContainer("notastyle")` silently accepted; `lockContainer(99)` raises | Known: #10746 |
| F-F1-8 | Minor | a21391878 | `attachToBorder("Left")` raises but leaves the container believing it is attached | Known: #10618 |
| F-F1-9 | Minor | 2492e0efc | A label swallows right-clicks, so the console context menu is unreachable under it | Known: #10753 |

### F-F1-1: Console keeps painting at the old border after an attached container's own edge drag

Steps (replay: `work-F1/t11.lua`, then the xdotool block below):
1. Launch: `HOME=$(mktemp -d) DISPLAY=:81 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled: MUDLET_TEST_MODE=1 ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
2. Dismiss the interface tour (click "Skip tour").
3. `lua dofile("<work>/t11.lua")` - creates `Adjustable.Container` "cpct" (25% x 60%) with a
   magenta child label and `attachToBorder("left")`.
4. Fill the console: `lua clearWindow("main"); for i=1,40 do feedTriggers(string.format("LINE%02d %s\n", i, string.rep("=#", 26))) end`
5. Drag the container's own right edge outwards, then DO NOT touch the window again:
   ```
   xdotool mousemove 589 385; xdotool mousedown 1
   for x in 610 640 670 700 730; do xdotool mousemove $x 385; sleep 0.08; done
   xdotool mouseup 1; sleep 3.5; import -window root shot.png
   ```

Expected: the console's left border follows the container, so the first character column
starts to the right of the container's edge.

Actual: `getBorderLeft()` reports the new value (230 -> 355 -> 496) and `getColumnCount()`
drops accordingly, but the text pane keeps painting at the *previous* border. Text is
drawn under the container and its first two characters are hidden ("NE29" instead of
"LINE29"), on every row the container covers *and* on the rows below it. It stays that
way indefinitely (3.5 s observed with no input); any later interaction - a click in the
window, typing in the input line - repaints it correctly.

Evidence:
- `shots-F1/16-edge-drag2-wait3s.png` - 3.5 s after mouseup, no interaction: container right
  edge at x=730, text begins at x=705, rows read "NE29"/"NE30"...
- `shots-F1/17-after-click-heal.png` - same state after one click: text begins at x=737
  (= window left 241 + border 496), rows read "LINE29"/"LINE30".
- `shots-F1/14-after-edge-drag.png` - the same effect on the first, smaller drag.
- Log: `T13| before edge drag: borderLeft=230 CPwidth=225 cols=57`,
  `T15| after 2nd edge drag: borderLeft=496 CPwidth=491.99976 cols=34`.

Commits: 6686b97ea (which fixed the same class of staleness for a *window* drag - that
half is verified working, F-F1 coverage below). Tracker: #10593 open, PR #10594 pending.

### F-F1-2: A scroll box outlives its deleted user window as an undeletable widget over the main console

Steps (replay: `work-F1/t23.lua`):
```lua
openUserWindow("uw3", true, false); resizeWindow("uw3", 320, 240)
createScrollBox("sbY", "uw3", 0, 0, 220, 140)
setBackgroundColor("sbY", 0, 160, 160, 255)
-- 0.8 s later:
deleteMiniConsole("uw3")          -- deletes the user window; returns true
-- 0.8 s later:
deleteScrollBox("sbY")
```

Expected: deleting the user window takes its scroll box with it, or the scroll box remains
addressable so a script can delete it.

Actual: `deleteMiniConsole("uw3") -> true`, then `deleteScrollBox("sbY") -> false /
scrollbox name 'sbY' not found` - yet the scroll box widget is still on screen, reparented
over the **main** console at (0,0), scroll bars and all, covering game text. There is no
name left to address it with, so nothing but a profile reload removes it. The same happens
for a `createCommandLine()` in the deleted window (`clX` -> `command line "clX" not found`).

c0309561b closed #10319 by deregistering these widgets on `destroyed()` - the crash is
indeed gone (F-F1 coverage row for c0309561b), but the widget itself is not destroyed with
its window, so the name is dropped while the widget lives on.

Evidence:
- `shots-F1/23-orphan-scrollbox.png` - the orphan at window-relative (0,0)-(210,138),
  clipping the console line "...le('uw3') -> true / nil".
- Log: `T23| deleteMiniConsole('uw3') -> true / nil`,
  `T23| deleteScrollBox('sbY') after its window was deleted -> false / scrollbox name 'sbY' not found`.
- `shots-F1/22-inputline-before-drag.png` shows the same orphan from the earlier `t21.lua` run.

Commits: c0309561b. Tracker: #10319 (closed).

### F-F1-3: Containers on opposite borders still reserve the whole axis, leaving the console 0 rows

Steps (replay: `work-F1/t9.lua`): attach a 12%-high container to the bottom border, then
attach a 100%-high container to the top border.

Expected: d3f5f873b bounds an attached container's reservation so the console always keeps
"two characters plus the pane's scroll bar".

Actual: the bound is per container, so the two together still take everything:
`T9| d3f5f873b borders with a 100%-height container on top: T458 B65 L149 R149` and
`T9| d3f5f873b console rows/cols: 0x56` - the console has **zero** rows and game output has
nowhere to go. The single-container case the commit targets does work
(`T10| d3f5f873b ALONE, 100% height on top: T458 B0 L0 R0 rows/cols=2x83`), so this is the
adjacent open issue rather than a failure of the commit. Attachment is saved with the
layout, so a profile saved in this state reopens in it.

Evidence: log lines above, `work-F1/t9.lua`, `work-F1/t10.lua`.
Commits: d3f5f873b. Tracker: #10617 open.

### F-F1-4: `hide()` on an attached container leaves its border reserved

Steps (replay: `work-F1/t22.lua`): with "cpct" attached to the left border, `CP:hide()`.
Expected: the border is handed back while the container is invisible.
Actual: `T22| 10747 before hide: T0 B0 L496 R0 cols=34` and
`T22| 10747 after hide(): T0 B0 L496 R0 cols=34` - a 496 px blank strip with nothing in it.
Tracker: #10747 open. Commits: 6686b97ea (`detach()` correctly gives the border back; `hide()` does not).

### F-F1-5: A second `minimize()` discards the real height

Steps (replay: `work-F1/t22.lua`): `CP:minimize()`, `CP:minimize()`, `CP:restore()`.
Actual: `T22| 10744 height start=60.00000% afterMin1=25px afterMin2=25px afterRestore=25px restored=false`
- the second minimize saves the already-minimized 25px as the height to restore.
Tracker: #10744 open.

### F-F1-6: `setPadding()` raises on a bad value and still writes it

Steps (replay: `work-F1/t22.lua`): `CP:setPadding(-5)` then `CP:setPadding("abc")`.
Actual: both raise
`GeyserSetConstraints.lua:106: attempt to perform arithmetic on local 'pos' (a nil value)`
and `CP.padding` is left holding `-5` / `abc`.
Tracker: #10745 open.

### F-F1-7: `lockContainer()` accepts an unknown style and raises on a bad number

Steps (replay: `work-F1/t22.lua`).
Actual: `lockContainer("notastyle")` returns without error, `locked=true`,
`lockStyle=standard` (the name is silently ignored); `lockContainer(99)` raises
`GeyserAdjustableContainer.lua:738: attempt to index field '?' (a nil value)`.
Tracker: #10746 open.

### F-F1-8: `attachToBorder("Left")` raises but marks the container attached

Steps (replay: `work-F1/t7.lua`).
Actual: `T7| 10618 attachToBorder('Left') ok=false err=.../GeyserAdjustableContainer.lua:486:
bad argument #1 to 'pairs' (table expected, got nil) attached=Left` - the raise leaves
`self.attached = "Left"`, so the container believes it is attached to a border it never got.
Tracker: #10618 open.

### F-F1-9: A label swallows right-clicks, hiding the console context menu

Steps: create any label over the console, right-click it; then right-click bare console.
Actual: no menu at all over the label (`shots-F1/05-rightclick-prose-label.png`); the
Copy / Copy HTML / Copy as image / Select all / Search menu appears normally on bare console
(`shots-F1/06-rightclick-console.png`). 2492e0efc deliberately stopped QLabel's own Qt menu
appearing and passes the event on "as a plain widget does", which is what leaves the gap.
Tracker: #10753 open.

## Notes (checked, not defects here)

- **#10458 (an indent close to the wrap width explodes a wrapped line) did not reproduce.**
  `setWindowWrap("main",40)` with indents 20/38/39/40/41 and a 112-character line gives 3-4
  lines each, no one-character lines, feed time 0.003-0.005 s (`work-F1/t3.lua`, log `T3|`).
  Indent >= wrap width is discarded (indent 40 and 41 both render unindented), as 78e33c43a
  intends. The rewrap path reached by `setWindowWrap()` alone does not re-wrap existing
  lines (`work-F1/t4.lua`), so a genuine window-resize rewrap at a large indent is the one
  variant left untried.
- **#10088 (`setBackgroundColor()` skipped on a label whose stylesheet has a border) did not
  reproduce.** `setLabelStyleSheet("lblbg","border: 2px solid yellow;")` then
  `setBackgroundColor("lblbg",30,30,200,255)` paints the label blue with a yellow border
  (`shots-F1/03-labels.png`), and `getBackgroundColor()` answers `30,30,200`. 55871e6f6 looks
  to have covered it.
- **#10619 (an attached container moved past its window edge sets a negative border) did not
  reproduce.** `CL:move(-400,"20%")` on a left-attached container gave
  `T10| 10619 after move(-400): T0 B0 L0 R0 leftNegative=false cols=83` - clamped to 0.
- `getUserWindowSize("uw1")` reports 298x222 after `resizeWindow("uw1",300,240)`; that is the
  dock's client area rather than the dock, and it is stable, so it is recorded as a note only.
- `showWindow()`/`moveWindow()` on a name whose widget is gone answer `true`/`nil` silently
  while `clearCmdLine()`/`deleteScrollBox()` answer with a reason (`work-F1/t21.lua`).
  Arguable, so a note rather than a bug.

## Coverage

| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| 6686b97ea | attached container's border follows the window as it is dragged | Fixed & verified; Bug: F-F1-1, F-F1-4 | Left-attached 25% container, main window dragged 220 px with 9 xdotool steps: console text keeps its 230 px offset through the drag, no lag or clipped strip (`shots-F1/10b-before-drag.png` -> `12-mid-drag2.png` -> `13-after-drag.png`). The container's own edge drag is F-F1-1; `hide()` is F-F1-4. |
| a21391878 | connected containers follow the window again | Fixed & verified; Bug: F-F1-8 | Container declared `width="25%"` keeps `"25%"` after `attachToBorder("left")` (not `"240px"`), and after the main window grows 960 -> 1180 px `get_width()` goes 240 -> 295 and `getBorderLeft()` 245 -> 300 (log `T11|`, `T12|`; `work-F1/t11.lua`). |
| d3f5f873b | attaching a container to a window edge leaves room for text | Fixed & verified; Bug: F-F1-3 | A 100%-height container alone on the top border reserves 458 of 502 px and the console keeps 2 rows (`T10| ... ALONE, 100% height on top: T458 B0 L0 R0 rows/cols=2x83`). Two containers on opposite edges is F-F1-3. |
| 1f159bf2b | element removed from a container is not deleted with it | Fixed & verified | `remove()` clears the child from `Inside.windowList`; after `container:delete()` the child is still live - `setBackgroundColor('cachild') -> true`, `getWindowGeometry('cachild') -> 538,60,316,120` (log `T8|`; `work-F1/t8.lua`). |
| 95a20c600 | turning auto save back on saves the layout again | Fixed & verified; Bug: F-F1-6, F-F1-7 | `autoSaveHandler` 53 -> nil -> 54 across disable/enable and `killAnonymousEventHandler(54) -> true`; `disableAutoSave()` safe when auto save was never on (log `T7|`; `work-F1/t7.lua`). |
| 2fe1f047c | miniconsoles and user windows are the size they were created at | Fixed & verified | 200x200 miniconsole reports 13 rows / 26 cols at creation and the same 13 after `resizeWindow` to the same size; a 200x40 console reports 3 rows and shows both fed lines (log `T5|`; `work-F1/t5.lua`). ctest `SubCommandLineLifetimeTest`, `WindowStateGettersTest` pass; `SubConsoleGeometry_spec` passes. |
| 2492e0efc | label click callbacks work with links, no stray Qt menu | Fixed & verified; Bug: F-F1-9 | Callback fires on the plain part of the label *and* on the `<a href>` inside it (`T6| CLICKCB fired n=1`, `n=2`); prose containing `<a\nb` gets no Qt Copy/Copy Link menu on right-click (`shots-F1/05-rightclick-prose-label.png`). ctest `LabelAnchorInteractionTest` passes. |
| 55871e6f6 | labels and gauges keep their background colour | Fixed & verified | `getBackgroundColor` reports `200,30,30,255` both before and after `setLabelStyleSheet()`, and a later `setBackgroundColor` paints blue through a border stylesheet (`shots-F1/03-labels.png`, log `T6|`). ctest `LabelBackgroundPaintTest` passes. |
| 373b68265 | label updates no longer slow down as text grows | Fixed & verified | 20 `echo()` calls each: 2.9 KB = 0.01 ms/call, 50 KB = 0.06 ms/call, 50 KB containing an `<a>` = 0.05 ms/call (log `T6|`; `work-F1/t6.lua`). Development's figure for 690 KB was 2.14 ms. |
| 35f0a72b9 | console scroll bar visible again on Windows | Could not test (Windows 11 style only; the commit leaves every other style alone) | ctest `ScrollBarContrastTest` passes here; Linux rendering unchanged by inspection of shots 01-23. |
| 5830a3bdb | moving a window over the main output no longer loses text | Fixed & verified | 200 lines fed at 20 ms intervals while the `uw1` floating user window was dragged 45 steps across the console: all 200 present (`T17| ... DRAGLINE present=200/200 missing=none`) and the rendered text is continuous with no gap (`shots-F1/18-uw-drag-mid.png`, `19-uw-drag-end.png`; `work-F1/t16.lua`, `t17.lua`). |
| d5645e23d | game text no longer vanishes behind a window moved over it | Verified no regression (dpr 1.0 here, fractional scaling not reachable under Xvfb) | Same drag as above at devicePixelRatio 1 loses nothing; the rounding half needs a 125/150% display. ctest `FramePacingTest`, `GlyphOverflowTest` pass. |
| caa26b20f | mouse cursor stops being a hand after leaving a link | Claimed, not evidenced (cursor shape not capturable) | `import -window root` does not include the X cursor image, so the shape could not be photographed. ctest `MainConsoleSelectionTest` (which carries the new case) and the `CursorShapes` spec both pass. |
| f04e3f483 | copy2decho no longer copies the wrong line's text | Fixed & verified | With the cursor on line 0, `copy2decho("alpha")` (same line) = `<192,192,192:0,0,0>alpha<r>`, `copy2decho("gamma")` (next line) = `""`, `copy2html("epsilon")` = `""`, `copy2decho("nosuch")` = `""` (log `T20|`; `work-F1/t20.lua`). `UI_spec` passes. |
| 1d17661d5 | a pasted link no longer runs a different link's command | Verified no regression (ctest) | ctest `PastedLinkStateTest` passes; `echoLink`/`insertLink` both render and are addressable in the buffer (log `T18|`). The two-store paste needs a second profile's buffer, not reachable in the time box. |
| ae7c09832 | clicking a styled web link no longer opens a mangled address | Could not test (the URL is only observable when a browser is launched) | No Lua read-back of the opened URL and no browser in the container. ctest `TLinkStore`-adjacent classes and the `TBufferOSC` spec pass. |
| a42bd25fb | a hidden link no longer overwrites text after lines are removed | Verified no regression (ctest + spec) | ctest `TrackedLinkTrimTest` passes; `TBufferOSC_spec` passes (1870 spec successes, 0 failures). |
| b030ceb1c | hidden links still reveal once older text has scrolled away | Verified no regression (ctest + spec) | Same two suites. |
| 6fd2bb035 | faster game text by not rescanning for links never created | Verified no regression | ctest `PastedLinkStateTest` passes; a 200-line burst plus a 40-line burst render correctly with no link state (`shots-F1/19-uw-drag-end.png`). |
| 011b79d8d | split the OSC 8 managers into model and view | Verified no regression | ctest `HyperlinkModelSplitTest`, `FrontendRefreshSeamTest`, `TrackedLinkTrimTest` pass; links echo and click normally. |
| dc07ffb23 | dragging inside the input line no longer scrolls its text | Fixed & verified | 55-character command typed into the input line (wraps to two rows), then click-dragged 10 steps from below the line to above it: both rows stay fully visible, nothing scrolled (`shots-F1/21-inputline-before-drag.png` vs `22-inputline-after-drag.png`). ctest `CommandLineScrollRangeTest`, `CommandLineKeyHandlingTest`, `CaretNavigationTest` pass. |
| c0309561b | crash using a scroll box or text box after its window was deleted | Fixed & verified (no crash); Bug: F-F1-2 | After `deleteMiniConsole("uw2")` took down the window holding `sbX`/`clX`/`mcInBox`, eleven follow-up Lua calls all returned cleanly and the process stayed alive (`T21| STILL-ALIVE-T21`; `work-F1/t21.lua`). The leftover widget is F-F1-2. |
| ee1729a73 | Lua UI functions no longer crash when a profile has no window | Could not test directly (no view-less Host reachable from a running profile) | The failure values are observable at the seams: `echo` into a dead console answers `console/label 'mcInBox' does not exist`, `clearCmdLine` answers `command line "clX" not found` (log `T21|`). ctest `WindowStateGettersTest`, `WindowBackgroundTest` pass. |
| 2fe2f8961 | index scroll boxes, command lines and text boxes in the core registry | Verified no regression | `createScrollBox`/`createCommandLine`/`createMiniConsole` into a scroll box all work, and the name-precedence path answers consistently (`work-F1/t21.lua`, `t23.lua`); ctest `SubCommandLineLifetimeTest` passes. |
| 51d4baef3 | index sub-consoles and user-window docks in the core registry | Verified no regression | Four user windows and five miniconsoles created, resized, moved, deleted across the session with correct `getRowCount`/`getColumnCount`/`getUserWindowSize`; ctest `WindowStateGettersTest`, `ProfileSwitchMiniconsoleTest` pass. |
| ba8e7eaa0 | look labels up in a core window registry | Verified no regression | Labels created, echoed into, restyled, click-callbacked, background-read and deleted by name throughout (`t6.lua`, `t7.lua`, `t8.lua`); ctest `LabelAnchorInteractionTest`, `LabelBackgroundPaintTest`, `LabelMovieRefusalTest` pass. |
| 7383ed6c9 | Lua API reads the main console through Host and its model | Fixed & verified | `startLogging(true)` returns `true` plus the file name normally; with a plain file where the `log` directory should be it returns `nil` plus `...could not be logged to file: <path>: Not a directory` and the console shows `Could not start logging to "<path>": Not a directory` (log `T24|`; `work-F1/t24.lua`). |
| e89b55e6f | core files print through Host instead of the console widget | Verified no regression | Every system message in this session ([ OK ], spell dictionary, logging error, Lua error routing to the error console) arrived on the right console with the right prefix (`work-F1/mudlet.log`). |
| c459afe96 | extract the main console's data model out of the widget | Verified no regression | `feedTriggers`, `getLines`, `getLineCount`, `clearWindow`, `moveCursor`, `getCurrentLine`, `selectString`, `deleteLine`-adjacent calls and wrapping all behave (`t1`-`t4`, `t17`-`t20`); ctest `MainConsoleSelectionTest`, `WrapLineRewrapTest` pass. |
| 4c987f2bb | test the tab indicator and hyperlink repaint wires | Verified no regression (test-only) | ctest `FrontendRefreshSeamTest` passes. |
| 0578b6122 | benchmark the console's cached-screen repaint path | Verified no regression (benchmark-only) | Not a shipped path; the overlay repaint it measures is exercised by F-F1's window-drag test above (no text lost). |
| 78e33c43a | a negative wrap indent no longer crashes Mudlet | Fixed & verified | `setWindowWrapIndent("main",-2)` -> `nil` + `indent -2 is not valid, it must be 0 or more`; `setWindowWrapHangingIndent("main",-3)` -> same with -3; `setWindowWrapIndent("main",4)` -> `true`; a 108-character line then wraps normally into 4 lines with no abort (log `T1|`; `work-F1/t1-wrap.lua`). The saved-profile route to a negative indent could not be built here (Lua now refuses it before it can be persisted), so the `TBuffer::wrapLine()` clamp rests on `WrapLineRewrapTest` and `WrapIndentBounds_spec`, both passing. |

## Automated

ctest (`-R 'LabelAnchorInteraction|LabelBackgroundPaint|LabelMovieRefusal|HyperlinkModelSplit|PastedLinkState|TrackedLinkTrim|WindowBackground|WindowLayoutSave|WindowStateGetters|SubCommandLineLifetime|ProfileSwitchMiniconsole|MainConsoleSelection|ConsoleSearchBar|CopyAsImage|FramePacing|GlyphOverflow|TextEditAccessible|CommandLineScrollRange|CommandLineKeyHandling|CaretNavigation|ScrollBarContrast|WrapLineRewrap|FrontendRefreshSeam'`):
**23 passed / 0 failed** (log `work-F1/ctest.log`).

Lua specs (`TESTS_DIRECTORY=work-F1/specs`, 23 files: every `Geyser*`, `SubConsoleGeometry`,
`GUIUtils`, `UI`, `Spawn`, `CursorShapes`, `WrapIndentBounds`, `TBufferOSC`):
**1870 successes / 0 failures / 0 errors / 27 pending** (log `work-F1/specs.log`).

Both match the baseline (ctest 220/220, busted 4702/0/0) - no failure in this area.
