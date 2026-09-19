# D1 findings

## Summary
Covered the script editor (pattern list layout, Advanced options, colour triggers,
tree icons, window position, missing theme), the Variables view, the notepad and
the Central Debug Console on development @ 85d814292 under Xvfb :83, plus the
16-test ctest set for the area (16/16 pass, matching baseline).
Fourteen of the sixteen in-scope commits verified in the real UI; the two
sanitizer-only ones (8d7ce35c6, f4d3ac5a9) have no observable behaviour to check
and were covered by their ctest classes instead.
Two live defects found: pasting newline-only text into a trigger pattern field
still kills Mudlet (#10337, a Blocker, reproduced under gdb with a backtrace),
and the editor's autocomplete now offers - and inserts - wiki markup in every
function name, a regression against 5.0.1 (669 entries).
Could not run: the main-toolbar "Triggers" entry point for a80ca8472 (the main
toolbar is hidden by default in this profile, so the button does not exist on
screen; its ctest case passes), #10829 (Windows-only), and the tail of the
open-issue list (#10812, #10779, #10778, #10774, #10401, #10579) which the
time-box ran out on.

## Findings
| ID | Severity | Commit | Title | Status |
|---|---|---|---|---|
| F-D1-1 | Blocker | (pre-existing, `SingleLineTextEdit.cpp:69`) | Pasting newline-only text into a trigger pattern field aborts Mudlet | Known: #10337 |
| F-D1-2 | Major | 5f6ac2513 (in-area, not in scope list) | Autocomplete offers and inserts `[edit \| edit source]` wiki markup in every function name | Regression vs 5.0.1; Known: #10816 |
| F-D1-3 | Minor | d8fc91b54 | The editor's Debug button appeared to need two clicks to show the Central Debug Console | Note |
| F-D1-4 | Cosmetic | d8fc91b54 | `debugc()` output never reaches the Central Debug Console - it goes to the editor's Errors view | Note |

### F-D1-1: Pasting newline-only text into a trigger pattern field aborts Mudlet
**Steps** (script: `work-D1/repro-10337.sh`, run as `bash work-D1/repro-10337.sh :83`)
1. `HOME=$(mktemp -d) ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
   with `DISPLAY=:83 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled:`
2. Dismiss the tour (Escape), click the input line, run `lua setClipboardText("\n")`
3. Alt+E to open the editor, click **Add Trigger**, click pattern row 1
4. Ctrl+V

**Expected** the paste is ignored, or inserts nothing (the widget exists to refuse
multi-line pastes).
**Actual** Mudlet dies immediately. Debug build:
`ASSERT: "!isEmpty()" in file /opt/qt/6.9.0/gcc_64/include/QtCore/qlist.h, line 676`.
Every unsaved edit in the editor goes with it. In a release build the same call is
`QList::first()` on an empty list, i.e. undefined behaviour rather than a clean abort.

**Evidence** `work-D1/gdb-10337.log` (full backtrace), `work-D1/crash-10337-tail.log`
(last 30 mirror lines of the first, non-gdb reproduction). Key frames:
```
#9  QList<QString>::first (this=0x7fffffffb5e0) at .../qlist.h:676
#10 SingleLineTextEdit::insertFromMimeData (this=0x5555597037b0, source=0x555559c73c20)
      at /home/user/Mudlet/src/SingleLineTextEdit.cpp:69
#13 QPlainTextEdit::keyPressEvent(QKeyEvent*)
#14 SingleLineTextEdit::keyPressEvent at /home/user/Mudlet/src/SingleLineTextEdit.cpp:55
```
Cause, at `src/SingleLineTextEdit.cpp:69`:
```cpp
QString firstLine = text.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts).first();
```
`SkipEmptyParts` on text that is only line breaks yields an empty list, and `.first()`
is then called on it. Reproduced twice, once outside gdb and once under it.
**Commits** Not introduced by anything in scope - the line predates 5.0.1 - but it is
live on this tree and sits in the middle of D1's area, so it is recorded here.
Tracker: #10337, open.

### F-D1-2: Autocomplete offers and inserts `[edit | edit source]` wiki markup
**Steps** (`work-D1/repro-autocomplete.sh`)
1. Launch as above, Alt+E, **Add Trigger**, click the Lua code box
2. Type `setBackgroundCol`

**Expected** the completion list offers `setBackgroundColor`, and accepting it inserts
`setBackgroundColor`.
**Actual** the list entry reads `setBackgroundColor[edit | edit source]`; pressing Return
inserts that literal string into the script, which is not valid Lua. The argument
hint pane beside it is correct (`setBackgroundColor([windowName], r, g, b, [transparency])`),
so arguments *are* shown here - #10816's headline symptom did not reproduce, but the
same data file is broken in a way that matches its "regression" tag.

**Evidence** `shots-D1/48-autocomplete.png` (popup), `shots-D1/49-crop.png` (what the
Return key inserted). Static proof and blast radius:
```
$ grep -c 'edit | edit source' src/lua-function-list.json                       # 669
$ git show Mudlet-5.0.1:src/lua-function-list.json | grep -c 'edit | edit source' # 0
$ git log --oneline -1 -S 'edit | edit source' -- src/lua-function-list.json
5f6ac2513 Infrastructure: Update autocompletion data in Mudlet (#10356)
```
669 of the autogenerated entries carry the wiki's "edit | edit source" link text in the
key. 5.0.1 has none, so this is a regression introduced after the branch point by the
autocompletion-data refresh, not by the edbee bump in scope (805918f48).
**Commits** 5f6ac2513 (in D1's area, outside D1's commit list). Tracker: #10816, open.

### F-D1-3: Debug button appeared to need two clicks
**Steps** Editor open, click **Debug** in the editor's left sidebar, wait 3 s.
**Expected** the Central Debug Console appears.
**Actual** after the first click no such window existed
(`xdotool search --name "Central Debug Console"` empty, `shots-D1/50-debug-window.png`
shows only the status-bar hint); a second click produced it
(`shots-D1/51-debug-window2.png`). This may simply be the first click landing on a
button that had not yet taken focus, and it is a toggle, so a genuine "on then off"
would have left it hidden rather than shown. Recorded as a Note, not a bug: I did not
re-run it enough times to separate the two explanations.
**Evidence** `shots-D1/50-debug-window.png`, `shots-D1/51-debug-window2.png`.
**Commits** d8fc91b54.

### F-D1-4: `debugc()` does not reach the Central Debug Console
**Steps** With the Central Debug Console open, run
`lua dofile("work-D1/debug300.lua")` (300 `debugc("D1 debug line "..i)` calls).
**Expected (per the QA recipe)** a few hundred lines in the debug console.
**Actual** all 300 lines went to the editor's **Errors** console; the debug console
showed only the alias-match traffic for the command itself. This is by design -
`TLuaInterpreter::debug()` (`src/TLuaInterpreter.cpp:1682`) prints to
`mpEditorDialog->mpErrorConsole` and has never touched `smpDebugConsole` - so it is a
correction to the recipe rather than a defect. The console's own volume handling was
checked with real debug traffic instead (see Coverage, d8fc91b54).
**Evidence** mirror log prefixes: `grep -o "^[a-zA-Z]*|" work-D1/mudlet2.log | sort | uniq -c`
gives `centralDebug|` and `main|` only, while the 300 lines appear under
`errors_Mudlet self-test|`. `shots-D1/54-debug-300.png`.
**Commits** d8fc91b54.

## Coverage
| Commit | Subject | Verdict | How verified |
|---|---|---|---|
| 18de78473 | Editor shows five pattern rows and opens on the first (#10813) | Fixed & verified | Trigger with 8 substring patterns, Advanced collapsed: rows 1-5 fully visible with row 6 clipped, list scrolled to row 1 (`shots-D1/05-eight-patterns.png`). Expanded then re-collapsed: same five rows (`06`, `07`). Scrolled the list to the bottom, selected another trigger, reselected: back at row 1 (`09`). Colour trigger at an 800 px-wide editor: a horizontal scrollbar appears and both colour caption buttons stay fully visible and clickable (`12`); clicking one opens the colour dialog and the choice writes back (`13`, `14`). ctest TriggerPatternListLayoutTest passed. |
| 888be6504 | Editor icons no longer lag after enableTrigger()/disableTrigger() (#10605) | Fixed & verified | Editor open on the Triggers tree, `lua disableTrigger("x")` from the input line: icon was already an empty box in a screenshot taken 0.8 s later (`shots-D1/18-tree-crop.png`). `lua enableTrigger("x") disableAlias("myalias")`: trigger icon ticked again within 0.7 s (`19-tree-crop.png`) and the alias showed empty in the Aliases tree (`21-alias-crop.png`). ctest TriggerEditorTest passed. |
| 3183df6a9 | Label the advanced options toggle (#10366) | Fixed & verified | The control reads **Advanced options** with a framed button at rest, sits to the right of the Command field, and the arrow points right when collapsed / down when expanded (`shots-D1/05-eight-patterns.png` vs `06-advanced-expanded.png`). ctest TriggerEditorDisclosureTest passed. |
| 8d7ce35c6 | Trigger pattern highlighter read an uninitialised flag (#10228) | Verified no regression | Sanitizer-only diagnostic with no observable behaviour, and no UBSan build was made here. Pattern rows construct and highlight normally across eight rows and a colour row (`shots-D1/05`, `12`); ctest TriggerPatternListLayoutTest / TriggerEditorTest passed. Claimed, not evidenced for the UBSan report itself. |
| f4d3ac5a9 | UB when the editor's undo stack is destroyed (#10232) | Verified no regression | Sanitizer-only; the commit states no test can go red without it. Closed the editor after editing a trigger (pattern type change, colour choice, save) and after typing in the code box - clean close both times, no abort (`shots-D1/22-editor-closed.png`, `23-closed.png`), and the app stayed alive afterwards. ctest dlgTriggerEditorUndoRedoTest passed. Undo/redo keystrokes before the close were not separately exercised. |
| f45cf7816 | Crash opening the editor when the profile's theme is missing (#10417) | Fixed & verified | Rewrote the saved profile's `mEditorTheme`/`mEditorThemeFile` to `Gone Theme`/`GoneTheme.tmTheme`, restarted, opened the editor and selected a trigger with pattern rows: no crash, rows render with the fallback theme, log carries `"texttheme.cpp" @ 207 Theme not set: "Gone Theme"` three times (`shots-D1/65-missing-theme.png`, `66-missing-theme-patterns.png`, `work-D1/mudlet4.log`). ctest MissingEditorThemeTest passed. |
| a80ca8472 | Editor reopens where it was left, not centred (#10317) | Fixed & verified (one entry point unreachable) | Moved the editor to 341,150 (700x520), closed it with its title-bar X, reopened from **Toolbox > Script editor**: `xdotool getwindowgeometry` reported 341,150 700x520 again (`shots-D1/25-reopened-toolbox.png`). The main-toolbar **Triggers** button could not be used: `mpMainToolBar` is hidden for this profile (`mudlet::adjustToolBarVisibility()`), so the button is not on screen; ctest EditorWindowPositionTest (18 cases, all eight entry points) passed. |
| e5eeddff9 | Variables with quotes or spaces can be edited (#10131) | Fixed & verified | `qa['say "hi"'] = 'before2'`; selecting it in the Variables view showed no warning banner, Name `say "hi"`, value `before2` (`shots-D1/32-sayhi-selected.png`). Edited the value to `after2` and clicked another variable; `pairs(qa)` then gave `QA[say "hi"]=after2` with `my var`, `plain` and `5` untouched (`work-D1/mudlet.log`, lines 57-68). ctest TVariableEditorTest + VariableEditorWriteBackTest passed. |
| 5edb079ed | Renaming a variable no longer loses it or runs its key as Lua (#10114) | Fixed & verified | Renamed `qa[5]` ("keepme") to `five` with key type changed to `key (string)`: tree shows `five`, `pairs(qa)` gives `QB[five]=keepme` and the other three members survive, no Lua error left on the stack (`shots-D1/37-crop.png`, mirror log lines 71-82). Also saved a variable without changing its name (the self-rename path the commit says used to delete it): `qa.plain` still `p` (`SELFRENAME p` in the log). |
| 54528e0cc | Rename a shadowed loop variable in the script view (#10125) | Verified no regression | `git show` confirms a pure rename of a loop-local `pItem` to `pHandlerItem` inside `slot_scriptsSelected()`, three lines, no other change. The Scripts view with registered event handlers was not opened in the UI (time-box). |
| 1aeaf8976 | Tests for the editor's dialogs and tree (#10405) | Verified no regression | ColorTriggerDialogTest, EditorClipboardXmlTest, EditorSearchTest, TreeWidgetItemMoveTest all passed. The colour-trigger dialog was also driven live: 16 basic colours, Default/Ignore/Cancel/More colours present, picking Red [1] wrote `Foreground color [ANSI 1]` back onto the pattern row (`shots-D1/13-color-dialog.png`, `14-color-chosen.png`). |
| b9e66364f | Tests for the notepad (#10489) | Verified no regression | ctest NotepadTest passed, and the notepad was driven end to end: typed two lines including quotes into the "Notes" tab, added a second tab, closed the profile - `notes.json` on disk holds both tabs, both names, the text with its newline and quotes, and `"activeTab": 1`. Reopened the profile: both tabs came back, active tab restored, text intact (`shots-D1/62`, `68`, `69-crop.png`). |
| 07201697f | Host.h no longer depends on the editor dialog (#10471) | Verified no regression | Editor opens, all seven views switch, search box and its option dropdown present and usable (`shots-D1/28`, `29`, `38`); ctest EditorSearchTest and EditorBannerViewSwitchTest passed. The save/reload round-trip of the search-option flags was not driven by hand (ProfileRoundTripTest covers it and is outside this regex). |
| d8fc91b54 | Filters, pause and clear for the debug window (#9688) | Fixed & verified | Console opens with the toolbar (Pause, Clear, Show, an item combo, a text filter, Aa) and announces `[*] 4 kind(s) of message are hidden` (`shots-D1/51`). The Show menu lists 14 categories with exactly the four noisy ones unchecked - every line from the game, trigger capture groups and match state, scripts that ran without errors, text selection calls - plus Show all / Hide all / Quiet (`58-show-menu.png`). Pause turned into Resume and reported `5 message(s) held` with nothing new drawn (`55-crop.png`); Clear emptied the console. See F-D1-4 for where `debugc()` output actually goes. |
| efaf9164d | Debug messages reach the console through a sink (#10494) | Verified no regression | Debug traffic (alias match lines, capture groups, profile start) reached the console after the sink was installed, and the paused queue replayed on Resume carrying its original arrival timestamps - `15:10:30.911` for lines drawn at `15:10:40` (`shots-D1/56-crop.png`). ctest DebugConsoleFilterTest passed. |
| 805918f48 | Bump edbee-lib 9a91551 -> 64960cd (#10614) | Fixed & verified | The bump's headline fix is edbee #180, "assert when tabbing at the very first offset of a document", which is Mudlet #10342. On this tree, Ctrl+Home then Tab in the code box - both on an empty-ish document and on `local x = 1` - indents instead of aborting, and the process survives (`shots-D1/46-tab-at-zero.png`, `47-crop.png`, process still `Sl` afterwards). #10342 is still open on GitHub and looks like a candidate to close. Typing, Lua syntax highlighting and the autocomplete popup all work; see F-D1-2 for what the popup now contains. Search/replace and Format All were not reached inside the time-box. |

### Open issues checked
| Issue | Result |
|---|---|
| #10337 pasting newline-only text crashes Mudlet | **Reproduced**, see F-D1-1. Still live. |
| #10342 Tab at the very start of a script aborts debug builds | **Does not reproduce** - fixed by the edbee bump 805918f48. Issue still open. |
| #10816 autocomplete not showing function arguments | Arguments *are* shown in the hint pane; but the completion entries carry wiki markup - see F-D1-2. |
| #5310 / PR #10780 autocomplete steals focus | Not reproduced in this run: after the popup appeared, typing and Return both worked. Not probed deeply. |
| #10812, #10779, #10778, #10774, #10401, #10579 | Not tested - time-box. |
| #10829 crash clearing the editor's undo history when saving a script | Windows-only, not verifiable on Linux. |

## Automated
ctest (`-R 'dlgTriggerEditor|EdbeeReinit|EditorClipboardXml|EditorBannerViewSwitch|ColorTriggerDialog|EditorWindowPosition|MissingEditorTheme|EditorSearch|TreeWidgetItemMove|TriggerEditorDisclosure|TriggerEditorTest|TriggerPatternListLayout|VariableEditorWriteBack|TVariableEditor|Notepad|DebugConsoleFilter'`):
16 passed / 0 failed. Log: `work-D1/ctest.log`. Matches the 220/220 baseline.
Specs: not re-run for this area (no D1-specific spec files; the baseline run was
4702 / 0 / 0 and nothing here touches Lua-visible behaviour).

## Artefacts
- Screenshots: `shots-D1/` (69 files, named by step).
- Repro scripts: `work-D1/repro-10337.sh`, `work-D1/repro-autocomplete.sh`.
- Fixtures: `work-D1/setup.lua` (8-pattern trigger, alias), `work-D1/vars.lua`
  (quoted / spaced / numeric variable keys), `work-D1/debug300.lua`.
- Logs: `work-D1/mudlet.log` … `mudlet4.log`, `work-D1/gdb-10337.log`,
  `work-D1/crash-10337-tail.log`, `work-D1/ctest.log`.
- Process bookkeeping: `work-D1/pids.txt` (all killed at the end of the run).
