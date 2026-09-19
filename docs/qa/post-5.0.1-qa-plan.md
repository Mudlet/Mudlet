# QA plan: development since 5.0.1

Date: 2026-09-19. Branch under test: `development` at `dbbf040c3`.

## Scope

`Mudlet-5.0.1` was cut from a release branch, so `Mudlet-5.0.1..HEAD` on a plain
clone undercounts. The real set is every commit on `development` after the branch
point that 5.0.1 does not already carry (cherry-picks excluded by patch id):

```bash
git merge-base Mudlet-5.0.1 HEAD          # 01e9402e7, 2026-08-19
git log --oneline --cherry-pick --right-only --no-merges Mudlet-5.0.1...HEAD
```

That is **309 commits**: 120 `Fix`, 40 `Improve`, 8 `Add`, 1 `Remove`, 140 `Infra`
(of which about 40 touch shipped C++ or Lua and so need regression coverage, the
rest are tests and CI). Net source change against 5.0.1: 536 files, +107k/-11k lines.

Everything is exercised on Linux: the fixes that are Windows- or macOS-only in
effect (stack size on Windows, scroll bar on Windows, Apple speech) are checked
for the parts that run everywhere and marked "platform-only, not verifiable
here" in the report rather than silently skipped.

## Environment

- Build: `cmake --build --preset linux-debug-nosan -j 4` (Debug, no sanitizer).
  Binary: `build-linux-debug-nosan/src/mudlet`. 4 cores, 15 GB RAM.
- C++ tests: `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R <regex> --output-on-failure`
  (220 tests). A grouped binary also runs one class directly, e.g.
  `build-linux-debug-nosan/test/functional_tests/functional_map_tests MapMouseInteractionTest`.
- Lua specs: `.claude/scripts/run-lua-tests.sh` (busted in the self-test profile,
  starts its own fixture servers, safe to run concurrently). `TESTS_DIRECTORY=<dir>`
  points it at a directory holding a subset of `src/mudlet-lua/tests/*_spec.lua`.
- Visual: one Xvfb per agent (`DISPLAY=:8N`, `Xvfb :8N -screen 0 1280x800x24`,
  `openbox`), a throwaway `HOME`, `xdotool` to drive, `import -window root` to
  screenshot, then read the PNG. `QT_QPA_PLATFORM=xcb GDK_BACKEND=x11`.
- Fastest way into a running profile with Lua available:

  ```bash
  HOME=$(mktemp -d) ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror
  ```

  `--mirror` copies every console to stdout so output is read from the log, not
  from pixels. `lua <code>` in the input line runs Lua (the `run-lua-code`
  default package); long scripts go through `lua dofile("/abs/path.lua")`.
- ccache is shared by every shell and worktree: `/etc/ccache.conf` sets
  `cache_dir=/root/.cache/ccache`, a 15 GB cap and `base_dir=/home/user`, so a
  build in `/home/user/worktrees/<name>` or in a harness worktree under
  `.claude/worktrees/` reuses the objects the main checkout compiled. An agent
  that must build (to bisect, or to confirm a fix) never builds in the main
  checkout; it makes a worktree, initialises the submodules, configures with the
  preset and builds only the app, then deletes the build directory (the test
  binaries alone are 13 GB and the disk is small):

  ```bash
  git -C /home/user/Mudlet worktree add --detach /home/user/worktrees/<area> <ref>
  cd /home/user/worktrees/<area> && git submodule update --init --recursive
  cmake --preset linux-debug-nosan -DUSE_ALTERNATE_LINKER=mold
  nice cmake --build --preset linux-debug-nosan -j 2 --target mudlet
  ```

  Verified on 2026-09-19: after one warm-up build from a worktree, a clean
  rebuild of the app target in another worktree path finished in 13 s with
  367 of 367 compiles served from the cache (all direct hits). The cache
  holds about 0.7 GB per full app build, well under the cap.
- Fixture servers in `CI/`: `telnet-fixture-server.py` (silent, records what the
  client sends), `http-fixture-server.py`, `mmcp-peer.py`, `discord-ipc-fixture.py`.
  Where an area needs a talking server (GMCP `Char.Login`, IRC, MSDP), the agent
  writes a small Python one in its scratch directory.

## Ground rules for every agent

1. Read-only on the repository. Findings, scripts, fixtures and screenshots go
   under the scratchpad `qa/` directory. No `cmake`, `ninja`, `make`: the build
   tree is shared and rebuilt only by the coordinator.
2. Own display number, own `HOME`, own temp directory; kill every process you
   started before reporting.
3. For each commit in scope: read `git log -1 --format=%B <hash>` (most carry a
   **Test case:** section), run that case by hand, then try the two or three
   adjacent things a user would do next. Verifying the fix is half the job;
   the other half is the regression around it.
4. Screenshot after every interaction. Coordinates come from the previous shot.
5. A test that cannot be run here is reported as such with the reason, never
   dropped.
6. Report in `qa/findings-<area>.md` using the format below. Keep the agent
   short-lived: one scope, one report, done.

## Findings format

```
# <Area> findings

## Summary
<3-6 lines: what was covered, what broke, what could not be run>

## Findings
| ID | Severity | Commit | Title | Status |
Severity: Blocker (crash, data loss, hang) / Major / Minor / Cosmetic
Status: Regression (worked in 5.0.1) / New bug / Fix incomplete / Note

### F-<area>-<n>: <title>
Steps, expected, actual, evidence (screenshot path or log excerpt), commit(s).

## Coverage
| Commit | Verdict | How |
Verdict: Fixed & verified / Verified no regression / Could not test (reason) / Bug (see F-…)
```

## Batches

At most three agents run at once. A batch starts only after the previous one
has reported. Each agent is Opus, gets one area, and is not reused afterwards.
Areas are ordered so that crash, hang and data-loss fixes are checked first.

### Batch 1

**A1 Mapper: interaction, labels, custom exit lines, context menu, panning** (visual)

Commits: 683a44258 f585cb5f7 977900fc6 fa54ef132 6aecd138a 342cfc6c3 2f5e3b366
553c91251 c9669fba6 bad7d7f4c c881de953 ace14e781 a7cf7a2db 30519ac23 0c66eb275
3280a5502 (test-only, but its PR body names four dialog bugs #10542-#10545 whose
fixes are not in this range: check whether they are still live).

Recipe: build a small map from Lua (`addAreaName`, `createRoomID`/`addRoom`,
`setRoomCoordinates`, `setExit`, `addCustomLine`, `createMapLabel`), open the
mapper (`openMapWidget`), switch to editing mode from the right-click menu, then
work every item on the room context menu: Create label (drag out in all four
directions, click without drag), pick up and drag a label, Move to position,
Move to area, Spread, Shrink, Configure areas (create, rename, refuse a
duplicate, delete, cancel), Create new map from both modes, Exits dialog (edit,
delete the special exit being edited, apply lock twice, check the map is redrawn
and marked unsaved). Middle-click hands-free pan, then left-click a room. Clear
the map with the label dialog open, then cancel and type into it. Right-click
the map twenty times and confirm the menu does not accumulate actions. Run
`ctest -R 'MapMouseInteraction|RoomExits|MapContextMenu|MapLockNoOp|MapAreaSelection|RoomProperties'`.

**B1 Game text pipeline: encoding, buffer, compression, replay, wrapping, bursts**

Commits: efe9414f2 2ed3475a7 bca5af8a2 398493580 40ca0b7e6 7773ae9d6 200f53fef
e6a709534 7a4ec89fa 5f7ff68ab 1095a434c acd1dbab6 230c0e2fb 76e39f889 ea7fe237f
22ff316fc 561c80876 875f8ec67 ac8e389b2 c917d443c.

Recipe: with the profile open, feed game text through `feedTelnet()` in split
reads: a UTF-8, GBK, Big5 and EUC-KR multibyte character cut between two calls
with a 400 ms pause, an SGR sequence cut in half, a 2 MB burst, IAC sequences
split at every byte. Set each server encoding (`setServerEncoding`) and check
the text, colours and prompt land intact. Record a replay of MCCP-compressed
data (write a zlib-compressed stream to the telnet fixture, or synthesise a
replay file with an empty chunk) and load it with `loadReplay`; check pace and
that a truncated header is refused. Resize the window and switch profiles while
long wrapped lines are on screen; check wrap width. `deleteLine`, `replace`,
`copy2decho` on the last line. Compare memory after 100k lines (200f53fef) via
`/proc/<pid>/status`. Check `feedTelnet`/`feedTriggers` error text is one line.
Run `ctest -R 'cTelnetBuffer|TelnetLargeBurst|TEncodingHelper|WrapLineRewrap|NarrowWindowWrap|RecolouredLineCache|CsiCursorForward|ScrollLostOnPartialRepaint'`
and the `TBufferEncoding`, `ServerEncoding`, `ConsoleWrap`, `ServerWrap`,
`Miscallaneous`, `TelnetTriggerFuzz` and `BufferManipFuzz` specs.

**E1 Profiles, connection dialog, credentials and sign-in, startup and shutdown**

Commits: 0b43837e4 0905eca3b 4edf41c04 f60a6952f 0be6f4f29 d5e899dab (offline
profile half) bc4c8f9e8 db9aa24c0 6e396b329 6c96ebf9c 59e79709f 524ec6452
84f998451 982a0d15e bae14c127 f50bb3bd9 56a87d2f7 8073d3444 c7f2591d3 b66feea61
442b4ac88 fcca8e2d0 577f4188a 5847232e2 72d96a5ca 4c6a1d230.

Recipe: start with an empty `HOME`; walk the connection dialog: type to pick a
game, create a profile with no address and open it with Offline (check the
notification text), a profile with a 60-character name and a long character
name, save a password (headless: keychain absent, so the encrypted-file path;
check the error text matches the commit), reopen and check it is still there.
Corrupt `profile.ini` and check the report. Write a Python telnet server that
offers GMCP `Char.Login` with `success` as `1`, `true` and `"true"`, and check a
saved sign-in survives each; forget the sign-in while a save is in flight.
Set a font the system resolves by alias (Helvetica) and a font that does not
exist. Add a word to the dictionary, restart, check it is there. Close Mudlet
during profile load and immediately after start (loop it 20 times, look for a
crash). Install ten packages and time profile open. Check `--version` and a
malformed version string in the updater path (8073d3444). Confirm no files are
left in the real `$HOME` by any test (72d96a5ca).
Run `ctest -R 'Credential|PasswordMigration|GMCPCharLogin|SignInStoreReconciler|OAuthClientFlow|ConnectionDialog|Profile|MissingDisplayFont|Dictionary|HomeUntouched|XdgRecipe|AppStartupTeardown|CloseDuringProfileLoad|HostManagerAccessor|ConfigDirOverride'`.

### Batch 2

**A2 Mapper: rendering, large maps, coordinate limits, pathfinding, level colours, map buttons, download**

Commits: 4d6a3ae23 714efcaf9 82342c735 7bd8ac2c1 95cdbf20d 41890b480 df2a5beb9
f2c2681fd 676a1b321 bea8e7e61 b150e51d4 e268326a4 ed43d2fea.

Recipe: generate a 50k-room map in Lua across several areas and z-levels; zoom
out fully, pan, switch areas, and time repaints (screenshots plus `--mirror`
timings via `getStopWatchTime`). Put a room at x = 2147483647, centre on it,
zoom to 400 with the wheel and `setMapZoom(1e40)`; save and `loadMap` that
file; the mapper must not spin. `getPath` between far rooms on the big map,
before and after adding rooms, timing each. Set level colours with alpha and
check the rendering. Cancel a map download mid-way and check the loaded map
stays. `setConfig("mapperButton", …)` for each value and press the toolbar Map
button; open a scripted mapper (`createMapper`) then press the button; close one
view and use the other. Room symbol font via `setConfig`/`getConfig`. Run the
map audit on the big map (`auditMap` or the load-time audit) and time it.
Run `ctest -R 'TAreaGridIndex|TAreaSpanIndex|TAreaZLevelIndex|MapCoordinateLimit|MapLevelOfDetail|MapDownload|MapperButtonConfig|MapperPanel|EmbeddedMapper|MapSymbolFont|MapRoundTrip|MapFileStats|MapRoomGeometry|MapAreaReassignment|MapOffscreenCustomLine|MapProgressDialogSeam|MapCloseDuringImport'`
and the `Mapper`, `MapViewportLimits`, `RoomDB`, `GeyserMapper` specs.

**C1 Trigger, alias, timer, key and script engine**

Commits: ec90c3893 40f64a5d5 663c937a9 cc72e3026 ab41f5e11 788e91170 cf1d28e53
83f16f8e5 07d816f8c 7876128c3 70f1dab3c 0c8cbba1a e46382824 9e6ef57a8 d0a7e6485
744ea965e 698dd9a86 c460994ba aab5e3806 4281e05d8 9720638ef ea3d43a9b 5c1c3b907
1cc778443 72c223f65 93e4c582d.

Recipe: every trigger type (substring, perl regex, begin-of-line, exact, Lua
function, colour, prompt, line spacer, multiline/AND with line delta, match-all)
fed both matching and non-matching lines including accented text before the
match; check captures, `multimatches`, `selectCaptureGroup`, `replace` forwards
and backwards, `deleteLine` then write. `expandAlias` from inside an alias and
check the outer captures survive; an alias that expands into itself (must not
crash, must report). `permGroup` containing keys; press them. Emergency stop
on, then off: timers resume. `adjustStopWatch` by ±1e9 ms. 3000 highlight
triggers and time a 1000-line feed. `appendScript` on a missing script. Trigger
editor icons after `enableTrigger`/`disableTrigger` (with the editor open).
Deleting an item from inside its own script (698dd9a86).
Run `ctest -R 'CaptureGroupParking|BigramFilter|CorruptTriggerPatterns|EmergencyStopResume|MultilineTriggerReentrancy|SetScriptCallback|TFeedTriggersRecursion|UnitDeferredDelete|UnitProcessingDepth|UnitsReachHost|ScriptEventHandlerLifetime'`
and the `Trigger`, `Alias`, `KeyBinds`, `EnableDisableByName`, `Regex`,
`ReversedSelection`, `EmptyBufferOps`, `InsertTextNewline`, `LuaApiContracts`
specs.

**F1 Console, labels, Geyser containers, links, user windows** (visual)

Commits: 6686b97ea a21391878 d3f5f873b 1f159bf2b 95a20c600 2fe1f047c 2492e0efc
55871e6f6 373b68265 35f0a72b9 5830a3bdb d5645e23d caa26b20f f04e3f483 1d17661d5
ae7c09832 a42bd25fb b030ceb1c 6fd2bb035 011b79d8d dc07ffb23 c0309561b ee1729a73
2fe2f8961 51d4baef3 ba8e7eaa0 7383ed6c9 e89b55e6f c459afe96 4c987f2bb 0578b6122.

Recipe: an adjustable container attached to each border; drag the main window
with xdotool and screenshot mid-drag (text must not lag), resize from a script's
own resize handler, detach and re-attach, remove an element from the container
and check it survives, toggle auto save and check the layout file. Miniconsole
and user window at explicit sizes, check `getWindowSize`-style getters. Labels:
click callback with a link inside, background colour after `setLabelStyleSheet`,
a label whose text grows to 50 kB (time updates), gauges. Links: `echoLink`,
`insertLink`, styled `<a>` web link, hidden links after `deleteLine`, hover
cursor leaves the hand after moving off. Move a user window over the main
output and check no text vanishes. `copy2decho` on several lines. Drag inside
the input line. Scroll box and text box after their window is deleted.
Run `ctest -R 'LabelAnchorInteraction|LabelBackgroundPaint|LabelMovieRefusal|HyperlinkModelSplit|PastedLinkState|TrackedLinkTrim|WindowBackground|WindowLayoutSave|WindowStateGetters|SubCommandLineLifetime|ProfileSwitchMiniconsole|MainConsoleSelection|ConsoleSearchBar|CopyAsImage|FramePacing|GlyphOverflow|TextEditAccessible|CommandLineScrollRange|CommandLineKeyHandling|CaretNavigation|ScrollBarContrast'`
and every `Geyser*`, `SubConsoleGeometry`, `GUIUtils`, `UI`, `Spawn`,
`CursorShapes` spec.

### Batch 3

**D1 Script editor, variables, notepad, debug window** (visual)

Commits: 18de78473 888be6504 3183df6a9 8d7ce35c6 f4d3ac5a9 f45cf7816 a80ca8472
e5eeddff9 5edb079ed 54528e0cc 1aeaf8976 b9e66364f 07201697f d8fc91b54 efaf9164d
805918f48 (edbee-lib bump: the editor widget itself changed underneath).

Recipe: open the editor; select a trigger with eight patterns, collapse and
expand Advanced options and count visible pattern rows (five) and the scroll
position (row 1); a colour trigger at 800 px editor width (horizontal scroll
bar must not hide the caption buttons). Copy from the pattern right-click menu.
Delete the profile's theme file and open the editor. Move the editor, close,
reopen: same place. Variables: create `my var`, `"quoted"`, rename one, check
neither runs as Lua and nothing is lost. Undo/redo then close the editor
(f4d3ac5a9). Notepad: type, close profile, reopen. Debug window: open it,
filters, pause, clear, feed a few hundred lines with `debugc`.
Run `ctest -R 'dlgTriggerEditor|EdbeeReinit|EditorClipboardXml|EditorBannerViewSwitch|ColorTriggerDialog|EditorWindowPosition|MissingEditorTheme|EditorSearch|TreeWidgetItemMove|TriggerEditorDisclosure|TriggerEditorTest|TriggerPatternListLayout|VariableEditorWriteBack|TVariableEditor|Notepad|DebugConsoleFilter'`.

**E2 Settings dialog, starter UI, tutorial, detached windows, tab bar, update dialog, translations** (visual)

Commits: 0f70a691f 7a3174476 c715215db 631bbe1e4 52b0d4b02 41c41e428 37152ee0d
c35dcd05f c2387bab2 21bb616fd ec0fedbb9 2b9fcd785 54025f766 ed9b6ee11 f75bb43b0
de041bbeb 0237e8d46 6ced949da d48ac5161.

Recipe: open the redesigned settings; visit every page and screenshot each in
light and dark; use search for five settings and follow the deep link; change a
setting and watch it apply instantly, then dismiss with Escape and check MMCP
auto-accept did not flip; check `sysSettingsChanged`-style events reach a
script when mute changes. Switch language to Czech and Spanish and screenshot
the settings, connection dialog and editor for clipping. Open a second profile
(Offline), detach it into its own window by dragging the tab (threshold 80 px),
check Alt+W/Alt+O, the Discord button and the toolbar icon size setting there.
Theme switching with two tabs open. Starter UI on a fresh profile; tour Next
and other buttons; the tutorial package's give command. Update dialog offered
version (f75bb43b0) if reachable offline.
Run `ctest -R 'Settings|StarterUi|Tutorial|FeatureCallout|ExperiencedPlayerGate|DetachedWindow|TabDetachThreshold|MainWindowSizeReport|NewReleaseDialog|ReleaseChangelog|Milestone|Release'`.

**B2 Protocols: MXP, MSDP, NEW-ENVIRON, channel 102, ATCP/MSP, TLS, latency**

Commits: f49c8c133 14e63e080 76f9d7304 137e0d14d 048a85b71 a3cae638a 6ed3cb4df
6e5cc384f 1889009e9 045d2cb6a ce537afd9 40c805352 (MXP frame half).

Recipe: write a Python telnet server that negotiates each option; connect the
profile to it. NEW-ENVIRON: turn it off mid-session and check nothing more is
sent. MSDP: nested tables, arrays, values with control codes, a variable split
across two reads, subscribe from `sysConnectionEvent`. MXP: mode switch escape
in the middle of a large read, `<FRAME>` creation and redirect into it, dest
frames cleared. Channel 102 wire format. `getNetworkLatency` while a script
busy-loops for a second. The starter UI must not re-enable MSDP on each open.
Run `ctest -R 'Telnet|TMxp|TEntity|TOsc|Msdp|MxpFrame|MxpDest|MxpWatchdog|TLinkStore|UntrustedText|LuaLiteral|NawsWidth'`
and the `MXP`, `MXPTags`, `GMCP`, `MSP`, `Networking`, `TBufferOSC` specs.

### Batch 4

**G1 Packages, modules, add-on commands, speech to text, text to speech** (visual)

Commits: 6ff1a7b75 72d42b93e e68b3829e d5e899dab (addCommand half) cfc23303a
b9b2c1518 3c3277079 90859cdba a2cdf94af 40c805352 (toolbar half) 9b0d8fc5f.

Recipe: `addCommand{name=…, shortcut=…}` and check the toolbar button and menu
item appear; a shortcut already taken by a key binding is refused and named;
with two profiles open the command shows only for the profile in front,
`setCommandPinned` keeps it; drag the profile into its own window. Install a
package that fails (bad zip, missing config) and read the error; uninstall a
package that removes itself from its own script; `installPackage` from the
HTTP fixture; the download progress dialog superseded by a second `Client.GUI`
under an application style sheet, then `setAppStyleSheet` again. `stt.getInfo()`,
`stt.init()` with no model, the `sysSTTHandover` contract with the Vosk stub
(`vosk-stub`). `ttsSpeechQueued` positions against the rest of the queue API.
Run `ctest -R 'AddonControls|Package|Module|DefaultPackages|UnzipBadArchive|Speech|Sherpa|Tts|ActionSelfRemoval'`
and the `Package`, `AddonCommand`, `STT`, `Other` specs.

**G2 Media, MSP, IRC, Discord, database, MMCP**

Commits: 614c3b888 d27cb219e 959c6ee7f b1cc19dc9 9a579224c d8b5ffa2a 89507afba
12f5d8101 ee174c8b7 3b46db87b de6795b19 3de265ac8.

Recipe: generate a 10 s WAV; `playSoundFile{start=3000}`, `start` past the end,
a file that is not media (must be refused, `sysMediaFinished` still sane);
MSP `!!SOUND(... )` then the Off request. IRC: run a Python IRC server that
logs raw lines; `sendIrc` with `\r\n` in message and target, a leading `/`,
empty target, `setIrcNick("a\r\nQUIT")`, a MOTD line arriving before
registration, a KICK, server text containing markup. Discord via the IPC
fixture. DB: `db:create` with a unique on a missing column, a list-form sheet,
`false` values in `phpTable`, use after `db:close`.
Run `ctest -R 'TMedia|VideoOutputHide|TtsInterrupting|Irc|Discord|TelnetAtcpMsp'`
and the `Media`, `DB`, `Discord`, `MudletBusted` specs.

**H1 Refactor regression sweep, packaging and release infrastructure**

Commits: 56a87d2f7 bbae10e5b 5c1c3b907 bae14c127 f50bb3bd9 de041bbeb 07201697f
7383ed6c9 e89b55e6f efaf9164d ed43d2fea e268326a4 3de265ac8 411737ebe e9dad5f2b
1187379af d8372441c 57e9014b3 a842ad1fe 6bce997a9 57bba619b f4849ed77 16ac754c1
aff0aab03 08b61b387 and the CI-only commits.

Recipe: the moved helpers, end to end: `getMudletHomeDir`, profile directory
layout on disk, log files (`startLogging` then read the file), the Central
Debug Console receiving `debugc` from two profiles, MXP frame consoles, action
toolbars after the profile is reloaded, spell-check dictionary save, map-audit
flag, Discord presence. Diff `git show 6bce997a9` for behaviour changes and
test each. Packaging: run `CI/linux-packages/deb/mkdeb.sh` and `rpm/mkrpm.sh`
as far as the container allows (`dpkg-deb`, `rpmbuild` availability), lint the
new workflows with a YAML parser, check `install.sh.in` substitutions, and
check `docs/` links (aff0aab03). Then run the full `ctest` and full busted
suite once more and compare with the baseline.

### Listed twice on purpose

The refactors that moved code off the main window (56a87d2f7 bbae10e5b 5c1c3b907
bae14c127 f50bb3bd9 de041bbeb 07201697f 7383ed6c9 e89b55e6f efaf9164d ed43d2fea
e268326a4 3de265ac8 72d96a5ca) appear under the area whose behaviour they carry
and again under H1. The area agent checks the feature; H1 checks the seams
between features. d5e899dab is split by half: the Offline-button half is E1,
the `addCommand` shortcut half is G1.

### Not tested one by one

The remaining 95 commits in the range add tests or specs, change CI workflows,
bump dependencies (sentry-native, edbee-lib, GitHub actions), update Crowdin
source text and autocompletion data, or edit documentation. They are covered as
a group: the full ctest and busted suites in the baseline must pass, H1 runs
them again at the end, and the dependency bumps are exercised by every area that
touches the editor (edbee) or crash reporting (sentry). The list is reproduced
by running the `git log` above and removing the hashes named in this plan.

## Baseline

Before batch 1 the coordinator runs the full `ctest` and busted suites once and
records the result in `qa/baseline.md`, so an agent can tell a pre-existing
failure from one it caused.

## Output

Each area's `qa/findings-<area>.md` is merged into
`docs/qa/post-5.0.1-qa-report.md`: verified fixes, regressions and new bugs
ranked by severity, and everything that could not be tested here with why.
