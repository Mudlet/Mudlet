# G1 findings

Area: Packages, modules, add-on commands (`addCommand`), speech to text, text to speech.
Tree under test: 85d814292 (= `development` 12b373743). Display `:81`.
Work dir: `<scratchpad>/qa/work-G1/`, screenshots `<scratchpad>/qa/shots-G1/`.
Comparison binary: `/home/user/worktrees/v501/build-linux-debug-nosan/src/mudlet` (5.0.1 Debug).

## Summary

All eleven commits in scope were exercised by hand in the real UI plus the area's
ctest and spec suites; ctest 19/19 passed, the five specs 602/0/0 (7 pending), both
matching baseline. The `addCommand` API (menu entry, nested `menuPath`, toolbar button,
shortcut, checkmark, pulse, refusal texts, per-window visibility, `setCommandPinned`)
behaves as its commits and `docs/addon-ui-api.md` claim, as does the 1-based
`ttsSpeechQueued` index and the whole documented `stt.*` return contract.
One Blocker fell out: a package that calls `uninstallPackage()` on **itself** from its
own install-time script segfaults Mudlet in `XMLimport::importPackage()`; 5.0.1 crashes
identically, so it is a long-standing hole rather than a regression, and it is not in
the tracker. Two known-issue confirmations are live on this tree (#10626 Zip Slip,
#10758 addCommand residue after uninstall) and one more (#10208/#10639) reproduces.
Not runnable here: any real speech recognition or microphone path (no engine, no audio
device), the sherpa-onnx and macOS engines, `sysSTTHandover` between two profiles
(needs a live microphone hand-off), and detaching a profile into its own window
(the tab drag is not reproducible with synthetic xdotool motion under Xvfb — the tab
reorders instead of detaching).

## Findings

| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-G1-1 | Blocker | (pre-dates scope; hit via b9b2c1518/3c3277079 package paths) | A package that uninstalls itself from its own install-time script segfaults Mudlet | New bug (not a regression: 5.0.1 crashes identically) |
| F-G1-2 | Major | (pre-dates scope) | Zip Slip: a package archive entry with `../` is written outside the profile directory | Known: #10626 |
| F-G1-3 | Major | 6ff1a7b75 / e68b3829e | Uninstalling a package leaves its `addCommand` menu entry and command id alive | Known: #10758 |
| F-G1-4 | Minor | 3c3277079 | `installPackage()` returns `true` for a failed install whenever a profile save is in flight | Known: #10208, #10639 |
| F-G1-5 | Minor | 3c3277079 | A package whose script fails to compile is registered and raises `sysInstall`/`sysInstallPackage` as a success | Note |

### F-G1-1: A package that uninstalls itself from its own install-time script segfaults Mudlet

**Severity:** Blocker (crash, on a path a real "one-shot installer" package would take).
**Status:** New bug. **Not a regression** — the 5.0.1 Debug binary crashes at the same
line with the same stack, so this has been live since before 5.0.1. No tracker match
found (searched "package uninstalls itself", "XMLimport importPackage segfault";
#9557's fixture covers only the *event-handler* case, which is fixed, and
`PackageSelfRemovalTest` / `ActionSelfRemovalTest` pass).

**Steps (replayable):**
1. Build the fixtures: `bash <work>/make-packages.sh <work>/packages`
   (makes `g1-selfremove.mpackage`, whose only script body is
   `uninstallPackage("g1-selfremove")`).
2. `bash <work>/repro-selfremove.sh <work>` — starts Xvfb-hosted Mudlet under
   `gdb -batch -ex run -ex bt`, dismisses the tour, and types
   `lua installPackage("<work>/packages/g1-selfremove.mpackage")`.
   (`<work>/repro-selfremove-501.sh <work>` is the same against the 5.0.1 binary.)

**Expected:** the package installs and removes itself, or the self-removal is refused
with a reason. Mudlet stays up.

**Actual:** the package's script runs, prints `uninstallPackage returned true`, and
Mudlet dies with SIGSEGV. The window vanishes with nothing on the console.

**Evidence** — `<work>/repro-gdb.log` (development):

```
main| g1-selfremove: uninstalling myself
main| g1-selfremove: uninstallPackage returned true

Thread 1 "mudlet" received signal SIGSEGV, Segmentation fault.
0x000055555893e4cb in XMLimport::importPackage (this=0x7fffffffaa90, pfile=0x7fffffffa860,
    packName=..., moduleFlag=0, pVersionString=0x0) at ../src/XMLimport.cpp:196
196             delete mpTrigger;
#0  XMLimport::importPackage (...) at ../src/XMLimport.cpp:196
#1  Host::installPackage (..., thing=enums::PackageModuleType::Package, quiet=true) at ../src/Host.cpp:2972
#2  TLuaInterpreter::installPackage (L=...) at /home/user/Mudlet/src/TLuaInterpreter.cpp:2725
...
#10 TAlias::execute (...) at /home/user/Mudlet/src/TAlias.cpp:417
```

`<work>/repro-gdb-501.log` (5.0.1, same crash, same line, different line number because
the file has since moved):

```
Thread 1 "mudlet" received signal SIGSEGV, Segmentation fault.
0x000055555873bfed in XMLimport::importPackage (...) at ../src/XMLimport.cpp:188
188             delete mpTrigger;
#1  Host::installPackage (...) at ../src/Host.cpp:2311
```

The first observation is also in `<work>/mudlet.log` (last line
`g1-selfremove: uninstallPackage returned true`, process gone) with
`<shots>/11-after-selfremove-crash.png` showing the empty screen.

**Reading of the cause (not verified by a fix):** `XMLimport::importPackage()` keeps
`mpTrigger` / `mpTimer` / … placeholders across the parse and deletes the unused ones at
the end; the package's own script runs mid-import and `uninstallPackage()` tears the
profile's units down under it, so the trailing `delete mpTrigger` frees a pointer the
unit already destroyed.

**Commits:** none in scope introduced it; it is reached through the package install path
that b9b2c1518 and 3c3277079 both touch, and through `installPackage()` itself.

### F-G1-2: Zip Slip — a package archive writes a file outside the profile directory

**Severity:** Major. **Status:** Known: #10626 (open) — confirmed live on this tree.

**Steps:** `bash <work>/make-packages.sh <work>/packages` builds
`g1-zipslip.mpackage`, a valid package whose third zip entry is
`../../../../../../tmp/g1-zipslip-escaped.txt`. Then, in a profile:
`lua installPackage("<work>/packages/g1-zipslip.mpackage")`.

**Expected:** the traversing entry is rejected (or clamped inside the package folder).
**Actual:** the install succeeds, the package is listed by `getPackages()`, and
`/tmp/tmp/g1-zipslip-escaped.txt` exists — outside `$HOME/.config/mudlet/profiles/…`.
(The archive's leading `..` segments are followed until they run out of path; the `tmp/`
tail of the entry then creates `/tmp/tmp/`. A shallower profile path, or an entry with
more `..`, reaches a chosen directory.)

**Evidence:**
```
$ find / -name g1-zipslip-escaped.txt -not -path '/proc/*'
/tmp/tmp/g1-zipslip-escaped.txt
main| pkgs: run-lua-code, …, g1-valid, g1-zipslip
```

### F-G1-3: Uninstalling a package leaves its `addCommand` menu entry and command id alive

**Severity:** Major. **Status:** Known: #10758 (open) — confirmed live on this tree,
against the current `addCommand` implementation.

**Steps:** `lua dofile("<work>/t7_10758c.lua")` — installs `g1-valid.mpackage`, whose
script does `addCommand{name = "G1 Pkg Cmd", menuPath = "G1Pkg", …}`, waits 1.5 s, then
`uninstallPackage("g1-valid")`. Open Options → Extensions.

**Expected:** the package's command and its `G1Pkg` submenu go with the package
(`docs/addon-ui-api.md`: "Every command belongs to the profile that created it and is
removed when that profile closes or resets" — and a package's own controls should not
outlive the package).

**Actual:** `getPackages()` no longer lists `g1-valid`, but Options → Extensions still
shows **G1Pkg**, and `removeCommand(<that id>)` still returns `true` (a second call
returns `false`, so the id was genuinely still registered).

**Evidence:** `<shots>/10-menu-residue-after-uninstall.png` (Extensions → A, Taker,
**G1Pkg** after the uninstall; the console behind it reads
`T7: uninstalled g1-valid, id 7 left in place; PKGS = … mudlet-base-ui`), plus
`<work>/mudlet.log`:
```
main| T6: uninstalled; PKGS = … mudlet-base-ui
main| T6: removeCommand(id) after uninstall = true
main| T6: second removeCommand(id) = false
```

### F-G1-4: `installPackage()` returns `true` for a failed install while a profile save is in flight

**Severity:** Minor. **Status:** Known: #10208 ("A package install postponed by a profile
save is announced as successful before it has installed") and #10639 ("A second
installPackage() in one chunk returns true before installing"). Confirmed live.

**Steps:** `lua dofile("<work>/t3_packages.lua")` — six `installPackage()` calls in one
chunk: a valid package, a non-zip, an archive with no `config.lua`, a package whose
script does not compile, an empty zip, and a path that does not exist.

**Expected:** the failing five return `false`/`nil` with a reason (the Lua entry point
passes `quiet = true` precisely so the reason comes back as a return value).
**Actual:** every call returns `true, nil`. The first install starts a profile save, so
each later call takes `Host::installPackage()`'s save-deferral branch, which returns
`{true, QString()}` before anything is attempted (`src/Host.cpp:2500-2528`; the code's own
comment acknowledges "this caller was handed `{true, ""}` long before the failure
happened"). The reason does reach the console, which is 3c3277079 working:

```
main| INSTALL notazip -> true , nil
…
main| [ ERROR ] - Package install failed for ".../g1-notazip.mpackage": could not unzip package
main| [ ERROR ] - Package install failed for ".../g1-empty.mpackage": no package found in … - no Mudlet package file in it could be read
main| [ ERROR ] - Package install failed for ".../g1-does-not-exist.mpackage": could not open file '…'
```

The same deferral exists in 5.0.1 (`Host.cpp` there has the identical
`currentlySavingProfile()` branch), so this is not a regression; it is the one gap left
in 3c3277079's "an install that fails now says why".

**Evidence:** `<work>/mudlet.log` lines 195-245, `<work>/t3_packages.lua`.

### F-G1-5: A package whose script fails to compile is registered and announced as installed

**Severity:** Minor. **Status:** Note (adjacent to #10625, which is about XML that cannot
be parsed; this one parses but its script does not compile).

**Steps:** `lua installPackage("<work>/packages/g1-errorscript.mpackage")` — a valid
archive whose only script calls an undefined global at load time.

**Actual:** the compile error is logged, and the package is then registered:
`sysInstall` and `sysInstallPackage` are both raised with the package name, and
`getPackages()` lists `g1-errorscript`.
```
XMLimport::readScript(...) ERROR - can not compile script's lua code for "g1ErrorScript";
  reason: Lua syntax error: … attempt to call global 'thisFunctionDoesNotExist_G1' (a nil value)
main| EVT sysInstall: g1-errorscript | nil
main| EVT sysInstallPackage: g1-errorscript | …/g1-errorscript.mpackage
```
Arguable rather than plainly wrong (an unloadable script is not an unloadable package),
so recorded as a Note. A package manager acting on `sysInstallPackage` cannot tell this
apart from a healthy install.

## Coverage

| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| 6ff1a7b75 | Add: Toolbar buttons and menu items for packages (#9983) | Fixed & verified | `<work>/t1_addcommand.lua` and `t9_toolbar.lua`. Menu entry appears at Options → Extensions → A → B → Demo (`<shots>/04,05,06`) and clicking it prints `CLICKED id=1 isDemo=true` (`<work>/mudlet.log:97`). With the toolbar on (`Mudlet.ini` `toolBarVisibility=3`) the same command is a native toolbar button, checked by `setCommandChecked`, next to a `TbOnly` button pulsing red from `setCommandPulse` (`<shots>/16-overflow-open.png`); both toolbar clicks raise the event (`mudlet2.log` `CLICKED id=1`, `CLICKED id=2`). Every documented refusal reproduced verbatim, including "the main toolbar is hidden, so a toolbar-only command would be invisible…", "a command needs a name to show", "menuPath has to be a string and this one is a table", "'holodeck' is not a surface this client has", "surfaces is empty…", "a shortcut needs a menu item to hang on…", "that is not a key sequence Qt understands", `"Demo" is already a command in this menu, so it cannot also be a submenu`, `"notacolour" is not a colour Qt recognises`, `interval must be greater than zero, got 0`. Unknown ids answer `false` (`remove=false enable=false pin=false`). See also F-G1-3. |
| 72d42b93e | Add: Speech-to-text backend for scripts and packages (#9982) | Verified no regression (engine/microphone paths not testable here) | `<work>/t11_stt.lua`, `t11b_stt.lua`, `t11c_stt.lua`. `stt.getInfo()` returns the documented table (`available`, `backend`, `capabilities{biasing,grammar,onDevice,sensitivityTuning,words}`, `initialized`, `listening`, `modelPath`, `searchPaths`, `sensitivity`, `silenceTimeout`, `state`) with no UI appearing. 18 `stt.*` functions exported. The Vosk stub from `build-linux-debug-nosan/test/functional_tests/vosk-stub/libvosk.so` was picked up by copying it to `$HOME/.config/mudlet/vosk-lib/` (the path `VoskRecognizer::userLibraryPath()` names and `getInfo().searchPaths` lists) — `available` then reads `true` and `sysSTTCapabilitiesChanged` fires on load. Return arity matches `docs/stt-api.md` exactly: `stt.init()` with no model → `nil, "no model path provided and no language model is installed - install one into …/vosk-models"`; `stt.init("/nonexistent/model/path")` → `nil, "model path does not exist: …"`; `stt.start()` uninitialised → `nil, "speech recognizer not initialized with a model - call stt.init() first"`; `stt.toggle()` → `nil, …`; `stt.stop()` with nothing listening → `true`; `setSilenceTimeout(-5)` and `setSensitivity("bogus")` → `nil` + a message naming the legal values; `setVocabulary` → `false` (backend cannot). Each refusal also arrives as `sysSTTError`. Not testable: real recognition, microphone capture, `sysSTTResult`/`sysSTTPartial`/`sysSTTWords` (no audio device or model). |
| e68b3829e | Add: More speech recognition engines, incl. macOS's (#10332) | Fixed & verified (addCommand half); engines and handover Could not test | Per-window commands: with two profiles as tabs in one window, the toolbar shows only the front profile's commands — `Demo`/`TbOnly` present with "Mudlet self-test" in front (`<shots>/16`), absent with "G1second" in front (`<shots>/22-g1second-overflow.png`, toolbar ends at Full Screen). `setCommandPinned(1, true)` from the owning profile then makes `Demo` visible from G1second while the unpinned `TbOnly` stays hidden (`<shots>/24-pinned-in-g1second.png`), and clicking it still raises `sysCommandClicked` (`mudlet2.log:362`). Could not test here: sherpa-onnx and the macOS recogniser (neither library present; the log shows `SherpaRecognizer: Failed to load sherpa-onnx library`), `sysSTTHandover` (needs two profiles contending for a real microphone), and dragging a profile into its own window (two scripted tab drags with intermediate motion only reordered the tabs — `<shots>/26`, `<shots>/27`; `SpeechAcrossProfilesTest` and `AddonControlsTest` cover the detached-window path programmatically and both pass). |
| d5e899dab | Fix: Key bindings taken by packages… (#10791) — addCommand shortcut half | Fixed & verified | `<work>/t2_shortcut.lua`. `tempKey(mudlet.keymodifier.Alt, mudlet.key.F9, …)` then `addCommand{name="Steal", shortcut="Alt+F9"}` → `nil, "Alt+F9 is already taken by a key binding in this profile"`. A named `permKey("MyBinding", …, Alt+F10, …)` is named in the refusal: `Alt+F10 is already taken by the "MyBinding" key binding`. Mudlet's own key is named too: `Alt+P is already taken by "Preferences"`. A second command asking for a command's key: `Alt+F7 is already taken by "Taker"`. A 5-step sequence: `a key sequence can be 4 step(s) long at most`. The reverse direction is a warning not a refusal — a `tempKey` made over command `Taker`'s Alt+F7 was still created (id 12); the warning goes to the editor (`KeyUnit::warnIfAddonCommandHoldsKey`), which was not open, so it was not seen on screen. |
| cfc23303a | Fix: ttsSpeechQueued reports a position the rest of the queue API accepts (#10805) | Fixed & verified | `<work>/t10_tts.lua`. A speech engine is present in this container, so the queue is live. `ttsQueue("first")` → `EVT ttsSpeechQueued text='first' index=1 ttsGetQueue(index)='first'` — 1-based and round-tripping, where the issue reported `0  false`. Head insert: `ttsQueue("head", 1)` → `index=1`, `ttsGetQueue(1)='head'`. Clamped past-the-end: `ttsQueue("past", 99)` → `index=2`, `ttsGetQueue(2)='past'`, and `ttsGetQueue()` then reads `[1=head, 2=past]` — the event's index agrees with the list. `ttsGetQueue(0)` and `ttsGetQueue(99)` return `false`, `ttsClearQueue()` empties it. |
| b9b2c1518 | Fix: re-land the package uninstall and download fixes (#10323) | Verified no regression | Uninstall: `uninstallPackage("g1-valid")` drops it from `getPackages()`, raises `sysUninstall` and `sysUninstallPackage`, and removes the on-disk folder (the residue that remains is the `addCommand` control — F-G1-3, a different defect). Download: a local `python3 -m http.server` on 127.0.0.1:8731 serving `g1-valid.mpackage`, then `installPackage("http://127.0.0.1:8731/g1-valid.mpackage")` — console shows `[ INFO ] - Downloading package from …`, the package's script runs (`g1-valid installed, command id 3`), and `[ OK ] - Package 'g1-valid.mpackage' installed successfully.`; no progress window was left on screen (`<shots>/28`). ctest `DualPackageModuleTest`, `ModuleManagerListingTest`, `PackageManagerRemovalTest`, `PackageRemovalSaveTeardownTest`, `PackageRepositoryDownloadTest` all pass. |
| 3c3277079 | fix: an install that fails now says why (#10005) | Fixed & verified, with F-G1-4 | Every failure path now names its reason on the profile's console: `could not unzip package` (non-zip), `no package found in … - no Mudlet package file in it could be read` (empty zip), `could not open file '…'` (missing file). Quoted in F-G1-4. The remaining gap is the Lua return value on the save-deferred path (F-G1-4). `ActionSelfRemovalTest`, which this commit extended, passes. |
| 90859cdba | improve: shrink mpackage icons (#10080) | Verified no regression | The seven re-packed default packages install on a fresh profile and are listed: `getPackages()` → `gui-drop, run-tests, StressinatorDisplayBench, enable-accessibility, run-lua-code, echo, deleteOldProfiles, generic_mapper, mudlet-base-ui`; `run-lua-code`'s `lua` alias drove every test in this report, `mudlet-base-ui` and `generic_mapper` loaded without error, and the profile opened in 0.27-0.30 s. `DefaultPackagesTest` passes. |
| a2cdf94af | Fix: Crash restyling Mudlet after a package download dialog was replaced (#10435) | Verified no regression (the exact Sentry path Claimed, not evidenced) | `<work>/t12_style.lua`: three `setAppStyleSheet(…, "g1sheet")` calls in a row (set, replace, clear) with a package downloaded from the HTTP fixture in the same session — no crash, Mudlet still running afterwards. The precise crash path (a *superseded* `Client.GUI` download dialog freed while an application style sheet is active) needs a game sending two `Client.GUI` blocks and was not reachable by hand here; its regression test `PackageDownloadDialogRestyleTest` passes. |
| 40c805352 | Infra: Action toolbars and MXP frame consoles built by the main console (#10475) — toolbar half | Verified no regression | The main toolbar builds, wraps to a second row when narrowed, and docks correctly with addon buttons on it (`<shots>/12,14,16`); the toolbar survives a second profile opening, tab switches and two tab drags (`<shots>/21,22,26,27`) with no missing or duplicated bars. ctest `ActionSelfRemovalTest` (which this commit extended with the reparent-with-no-console, removal-with-console and right-side-toolbar cases) and `MxpFramePlacementTest` pass. Creating a button bar from Lua was not managed — `permGroup(name, "button"/"action", "")` is refused by `Other.lua:205` as an invalid type — so the script-created bar case rests on the functional tests. |
| 9b0d8fc5f | Infra: Make the libmudlet progress audit fail loudly (#10374) | Fixed & verified | `bash cmake/audit-core-widgets.sh --enforce` in this container first exercised a new abort path for real: it exited **2** with `could not locate Qt headers with a QtWidgets/ subdirectory. point at it with --qt-include DIR or the QT_INCLUDE_DIR environment variable` — i.e. it refused to guess rather than reporting a too-low count. Re-run as `--enforce --qt-include /opt/qt/6.9.0/gcc_64/include` it exits **0** with `mudlet_core Qt Widgets audit: 149 offending files (baseline 149)`, the number the commit message states. Output in `<work>/audit.log`; `git status` in /home/user/Mudlet stayed clean. |

## Automated

**ctest** (`QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R
'AddonControls|Package|Module|DefaultPackages|UnzipBadArchive|Speech|Sherpa|Tts|ActionSelfRemoval'`):
**19 passed / 0 failed**, 33.9 s. Full list in `<work>/ctest-G1.log`:
AddonControlsTest, SpeechAcrossProfilesTest, PackageExporterTest, ActionSelfRemovalTest,
DefaultPackagesTest, DualPackageModuleTest, ModuleManagerListingTest,
ModuleSaveTeardownTest, PackageDownloadDialogRestyleTest, PackageExporterDialogTest,
PackageFontsOnRefusedArchiveTest, PackageManagerRemovalTest,
PackageRemovalSaveTeardownTest, PackageRepositoryDownloadTest, PackageSelfRemovalTest,
UnzipBadArchiveTest, SherpaEngineTest, SpeechRecognizerContractTest,
TtsInterruptingSpeakTest. Matches baseline (220/220).

**Specs** (`TESTS_DIRECTORY=<work>/specs .claude/scripts/run-lua-tests.sh`, holding
symlinks to `AddonCommand_spec.lua`, `Media_spec.lua`, `Other_spec.lua`,
`Package_spec.lua`, `STT_spec.lua` plus the `fixtures` directory):
**602 successes / 0 failures / 0 errors / 7 pending**, 51.7 s (`<work>/specs2.log`).
Matches baseline. Note for anyone repeating this: a `TESTS_DIRECTORY` without a
`fixtures` symlink makes `Package_spec.lua` error out on missing fixtures rather than
fail (`<work>/specs.log`) — a case of the "fails silently rather than red" the project
docs warn about.

## Repro assets

- `<work>/make-packages.sh` — rebuilds every fixture archive used above.
- `<work>/repro-selfremove.sh`, `<work>/repro-selfremove-501.sh` — F-G1-1 under gdb,
  against development and against 5.0.1.
- `<work>/send.sh`, `<work>/send2.sh` — type a line into Mudlet's input box
  (click, Ctrl+A, Delete, type, Return); `send2.sh` takes x/y for the wider window.
- `<work>/t1..t12*.lua` — the Lua run by each test above.
- Launch line used throughout:
  `HOME=<fresh> DISPLAY=:81 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled: MUDLET_TEST_MODE=1 ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
  The main toolbar is off by default; it was turned on by writing
  `[General]\ntoolBarVisibility=3\nshowToolbar=true\nmenuBarVisibility=3\nuiTourShown=true`
  to `$HOME/.config/mudlet/Mudlet.ini` before launch (`uiTourShown` also skips the tour popup).
