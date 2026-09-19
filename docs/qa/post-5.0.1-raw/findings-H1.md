# H1 findings

## Summary
Covered the refactor seams end to end on a running profile (`MUDLET_TEST_MODE=1`,
display :81, throwaway HOME): `getMudletHomeDir()` and the profile layout on
disk, `startLogging` and the log file, the Central Debug Console fed by two open
profiles, an MXP `<FRAME>` console, an action toolbar surviving a profile
reload, the spell-check dictionary surviving a restart, the map-audit flag, and
Discord presence against `CI/discord-ipc-fixture.py`. All of them work; none of
the moved-code commits regressed the behaviour they carry.
Packaging: `dpkg-deb` and `docker` exist here, `rpmbuild`, `patchelf` and
`fakeroot` do not, and the docker daemon is not running, so `CI/linux-packages/build`
and both `verify-*.sh` could not be run. `deb/mkdeb.sh` itself was run outside
docker against the existing Release build and produced a real 25 MB .deb; that
package ships QTagEdit's static library, header and CMake export files
(F-H1-1). Of the twelve workflow files changed in the range, the eleven that
still exist all parse as YAML (`cleanup-caches.yml` was deleted); the `publish`
index generator writes an `index.html` into every directory; and the one docs
link the range fixes (aff0aab03) resolves. Both full suites were re-run on the
unchanged tree and match the baseline exactly: ctest 220/220, busted
4702/0/0/86 pending. One new bug (Minor), two pre-existing Notes, no Blockers
and no Majors.
Could not run here: `verify-deb.sh` / `verify-rpm.sh` / `mkrpm.sh` (no docker
daemon, no rpmbuild), the full `CI/linux-packages/build` (would need a container
compile and ~10 GB of disk we do not have), and the publish upload path (needs
R2 credentials).

## Findings

| ID | Severity | Commit | Title | Status |
|----|----------|--------|-------|--------|
| F-H1-1 | Minor | f4849ed77 (16ac754c1 unaffected) | The .deb (and, by construction, the .rpm) ships QTagEdit's static library, header and CMake export files into an end-user package | New bug |
| F-H1-2 | Cosmetic | pre-5.0.1 | `docs/CONTRIBUTING.md` links to `UI-design-philosophy.md` relative to `docs/`, but the file is at the repository root | Note (pre-existing) |
| F-H1-3 | Cosmetic | bae14c127 / f50bb3bd9 | `saveProfile()` returns a path with a doubled separator (`current//<file>.xml`) | Note (pre-existing, identical in 5.0.1) |

### F-H1-1: the Linux package ships 3rd-party development files

**Steps** (replayable; script `work-H1/repro-F-H1-1.sh`):

```sh
mkdir -p /build/Mudlet
ln -sfn /home/user/Mudlet/build-linux-release /build/Mudlet/build
ln -sfn /home/user/Mudlet/COPYING            /build/Mudlet/COPYING
ln -sfn /home/user/Mudlet/translations       /build/Mudlet/translations
# the host has no patchelf (the deb image installs one); shim it out and point
# dpkg-shlibdeps at the host Qt so the script can reach its dpkg-deb call
mkdir -p /tmp/shim && printf '#!/bin/sh\nexit 0\n' > /tmp/shim/patchelf && chmod +x /tmp/shim/patchelf
cd /home/user/Mudlet
PATH=/tmp/shim:$PATH LD_LIBRARY_PATH=/opt/qt/6.9.0/gcc_64/lib \
PKG_VERSION=5.0.0 PKG_SNAPSHOT=20260919.85d814292 PKG_RELEASE=1 BUILD_COMMIT=85d814292 \
  bash CI/linux-packages/deb/mkdeb.sh
dpkg-deb -c /build/out/mudlet_*.deb | grep -E 'usr/(include|lib)/'
```

**Expected**: an end-user package contains the application, its data and its
documentation only. Development files (a static library, a C++ header and CMake
package-config files for a vendored widget) belong in a `-dev` package or
nowhere.

**Actual**:

```
drwxr-xr-x root/root         0 ./usr/include/QTagEdit/
-rw-r--r-- root/root      5044 ./usr/include/QTagEdit/qtagedit.hpp
drwxr-xr-x root/root         0 ./usr/lib/cmake/QTagEdit/
-rw-r--r-- root/root       867 ./usr/lib/cmake/QTagEdit/QTagEditTargets-release.cmake
-rw-r--r-- root/root      4136 ./usr/lib/cmake/QTagEdit/QTagEditTargets.cmake
-rw-r--r-- root/root     93726 ./usr/lib/libQTagEdit.a
```

**Cause**: `CMakeLists.txt:315` does a plain `add_subdirectory(3rdparty/qt-tags-widget)`,
and `3rdparty/qt-tags-widget/CMakeLists.txt:40-58` carries unconditional
`install(TARGETS …)`, `install(FILES ${QTAGEDIT_HEADERS} …)` and
`install(EXPORT QTagEditTargets …)` rules. Both `deb/mkdeb.sh:24` and
`rpm/mkrpm.sh:24` stage the whole tree with `cmake --install "$SRC/build" --prefix "$STAGE/usr"`,
and `mkrpm.sh` then builds its `%files` list from everything it finds under the
stage directory (`mkrpm.sh:76-84`), so the rpm carries the same files. On
Debian/Ubuntu the static library also lands in `/usr/lib` rather than the
multiarch `/usr/lib/x86_64-linux-gnu`, which lintian flags.

**Scope**: `3rdparty/qt-tags-widget` is a submodule in 5.0.1 as well, so
`cmake --install` already produced these files before; what is new is that
f4849ed77 turns that install tree into a package users install. The AppImage
path is unaffected.

**Evidence**: `work-H1/mkdeb2.log` (full run, exit 0, `dpkg-deb --info` output),
`work-H1/deb-contents.txt` (full `dpkg-deb -c` listing),
`work-H1/mudlet_5.0.0~git20260919.85d814292-1~noble_amd64.deb`.

**Tracker**: `recent-issues.md` and `open-prs.md` have nothing on Linux
packaging; `search_issues` for "Linux deb rpm package ships development files
static library headers cmake config QTagEdit" returned only #3024 and #1077
(qmake-era, closed), and `search_pull_requests` for open linux/deb/rpm PRs
returned none. Not known.

**Commits**: f4849ed77, 16ac754c1 (the repository layout commit is not itself at
fault).

**Caveat on the same run**: the `Depends:` line the host run produced lists only
`libqt6core6t64` of the Qt packages, because the host's Qt lives in `/opt/qt`
with no dpkg owner and `mkdeb.sh` passes `--ignore-missing-info`. Inside the
deb image the distribution Qt6 packages supply that information, so this is an
artefact of running outside the container, not a packaging defect. Likewise
the `~noble` suffix: the host is Ubuntu 24.04, while `build` targets 26.04.

### F-H1-2: docs/CONTRIBUTING.md link to UI-design-philosophy.md does not resolve

**Steps**: `python3 work-H1/check-links.py` (walks every relative Markdown link
in the 20 `.md` files changed in `Mudlet-5.0.1...HEAD`).

**Expected**: every relative link resolves.
**Actual**: `docs/CONTRIBUTING.md:7` has
`[UI design philosophy](UI-design-philosophy.md)`; the file is at
`/home/user/Mudlet/UI-design-philosophy.md`, so from `docs/` the link 404s on
GitHub. The other two "BROKEN" lines the checker prints are its own regex
tripping over nested `[text][url](https://…)` markup, not real breakage.

**Pre-existing**: `git show Mudlet-5.0.1:docs/CONTRIBUTING.md` has the identical
line 7, so this is not a regression and not caused by aff0aab03 (whose own fix,
`docs/GITHUB.md` → `AI-ASSISTANTS.md` → `docs/AI-ASSISTANTS.md`, resolves).

**Evidence**: `work-H1/check-links.py` output, quoted above.

### F-H1-3: saveProfile() reports a doubled separator

**Steps**: `lua print(saveProfile())` in a running profile.
**Actual**: `…/profiles/Mudlet self-test/current//2026-09-19#15-50-14.xml`.
**Pre-existing**: `MudletPaths.cpp:312` returns `"%1/profiles/%2/current/"` and
the caller appends another `/`; `git show Mudlet-5.0.1:src/mudlet.cpp:5900` has
the identical trailing slash, so bae14c127/f50bb3bd9 moved the code unchanged.
Cosmetic only - the file is written to the right place (verified by listing the
directory).
**Evidence**: `work-H1/mudlet.log`, the `SAVEPROFILE:` line.

## Coverage

| Commit | Subject | Verdict | How verified |
|--------|---------|---------|--------------|
| 56a87d2f7 | Host manager accessor that does not need the main window | Verified no regression | Two profiles open at once ("Mudlet self-test" + a new "H1 second" opened Offline from the connection dialog), tab bar and per-profile toolbars correct, profile switch by tab, then both closed in one shutdown with no warning. `shots-H1/09-second-profile.png`, `shots-H1/17-debug-raised.png`; `work-H1/mudlet2.log` ends with two `Saving profile's own Hunspell dictionary` lines and a clean `~mudlet`. `HostManagerAccessorTest` passed in the full ctest run. |
| bbae10e5b | Lua functions that drive the console widget move to the UI file | Verified no regression | `print`, `echo`, `cecho`, `feedTriggers`, `feedTelnet`, `getColumnCount`/`getRowCount` on a second console all behaved; `work-H1/seam2.lua`, `seam3.lua` and their output in `work-H1/mudlet.log` (`MXP-FRAME-COLUMNS: 9`, `MXP-FRAME-ROWS: 2`). |
| 5c1c3b907 | Triggers, timers and actions reach the console through Host | Verified no regression | `tempButtonToolbar("H1Bar",0,1)` + `tempButton("H1Bar","H1Button",1)` produced a visible action toolbar (`shots-H1/03-frame-and-toolbar.png`), the `tempTimer` chains in `seam3.lua`/`seam4.lua` all fired, and the toolbar came back after `saveProfile()` and a restart (`shots-H1/05-reloaded.png`, `RELOAD-BUTTON-EXISTS: 1`). |
| bae14c127 | Move the profile data helpers into MudletPaths | Fixed & verified | `getMudletHomeDir()` returned `<HOME>/.config/mudlet/profiles/Mudlet self-test`, the directory exists, and the saved tree has `profile.ini`, `profile.dic`, `profile.aff`, `current/`, `log/`, `map downloads/` and one directory per installed package - full listing in this run's transcript; `work-H1/seam1.lua` output in `work-H1/mudlet.log`. |
| f50bb3bd9 | getMudletPath() gets a widget-free home in MudletPaths | Fixed & verified | Same run: the profile XML went to `…/current/2026-09-19#15-50-14.xml`, the log to `…/log/2026-09-19#15-50-56.txt`, the fonts to `<HOME>/.config/mudlet/fonts/…`, all under `getMudletHomeDir()`. |
| de041bbeb | Host tells its dialogs about changes through signals | Fixed & verified | Script editor open showing three trigger groups (`run-tests`, `generic_mapper`, `mudlet-base-ui`); `uninstallPackage("generic_mapper")` typed into the main input line, editor untouched; the editor's tree repopulated itself and the group is gone. `shots-H1/21-editor-before-uninstall.png` vs `shots-H1/23-editor-raised.png`. |
| 07201697f | Host.h no longer depends on the trigger editor dialog | Verified no regression | Editor opened with Alt+E, maximised, its Debug button toggled the Central Debug Console, its tree repopulated on a package change, and it closed with the profile. `shots-H1/12-editor.png`, `13-editor-max.png`, `14-debug-console.png`, `23-editor-raised.png`. |
| 7383ed6c9 | Lua API reads the main console through Host and its model | Verified no regression | `getColumnCount`/`getRowCount` answered for a named console, `feedTriggers` text reached the buffer and the log file, `spellCheckWord` answered from the profile dictionary. `work-H1/seam3.lua`, `seam5.lua` output. |
| e89b55e6f | Core files print through Host instead of the console widget | Verified no regression | The decorated system lines still land on the main console: `[ OK ] - Profile "Mudlet self-test" loaded in offline mode.`, `[ INFO ] - Map audit starting...`, the MXP auto-enable notice. `shots-H1/03-frame-and-toolbar.png`, `shots-H1/06-games-menu.png`. |
| efaf9164d | Debug messages reach the Central Debug Console through a sink | Fixed & verified | With two profiles open the console registered `[A] = "Mudlet self-test"`, `[B] = "H1 second"` and then carried alias traces from both, each with its own prefix, and the `Profile … ended.` lines at shutdown. `shots-H1/14-debug-console.png`, `17-debug-raised.png`, `19-debug-showall.png`; `work-H1/mudlet2.log` `centralDebug\|` lines. (`debugc()` itself writes to the profile's Errors view, not this console - `TLuaInterpreter.cpp:1702` - so the [A]/[B] traffic above is the sink evidence.) |
| ed43d2fea | Host no longer builds its user-window and mapper widgets | Verified no regression | An MXP `<FRAME H1Frame …>` created a real console widget that answered `getColumnCount`/`getRowCount` and displayed text redirected into it with `<DEST>`; the action toolbar widget was built the same session. `shots-H1/03-frame-and-toolbar.png`. |
| e268326a4 | Map-audit flag gets a home that does not need the main window | Fixed & verified | With the default (`reportMapIssuesToConsole=false`) `auditAreas()` on a map with a dangling exit printed only `Map audit starting…` / `Auditing of map completed`. Setting `reportMapIssuesToConsole=true` in `<HOME>/.config/mudlet/Mudlet.ini` and restarting added `Area id numbering is satisfactory.` / `Room id numbering is satisfactory.`. `work-H1/seam4.lua` output in `mudlet.log` vs `seam5.lua` output in `mudlet2.log`. |
| 3de265ac8 | Discord integration gets a home that does not need the main window | Fixed & verified | `CI/discord-ipc-fixture.py` started first, Mudlet launched with `XDG_RUNTIME_DIR` and the bundled `libdiscord-rpc.so` on the library path (`Discord integration loaded` in `work-H1/mudlet3.log`); `setDiscordGame`/`setDiscordDetail`/`setDiscordState`/`setDiscordLargeIcon` each produced a `SET_ACTIVITY` frame in `work-H1/discord-frames.jsonl`, ending `{"activity": {"assets": {"large_image": "mudlet"}, "details": "H1 detail text", "state": "H1 state text", …}}`. Repro: `work-H1/repro-discord.sh` + `seam6.lua`. |
| 411737ebe | Home the debug switch and timestamp formats in core classes | Verified no regression | Central Debug Console stamps every line `HH:MM:SS.zzz` (`15:56:23.978`) - `shots-H1/17-debug-raised.png`; the log file opens `Log session starting at 15:50:56 on Saturday, 19 September 2026.` and closes with the matching `ending at` line. |
| e9dad5f2b | Split the Qt Widgets helpers out of utils.h | Verified no regression (build-only) | Header-split only, with no user-visible surface of its own; the tree builds and the full ctest and busted suites match the baseline. No behavioural test invented for it. |
| 1187379af | Move logging into the core console model so it works without a window | Fixed & verified | `startLogging(true)` reported the path, `startLogging(false)` reported the same path, and the file in `<home>/log/` holds the header, the `feedTriggers`, `echo` and `cecho` lines and the closing footer. `work-H1/seam2.lua`, `seam2b.lua`, and the log file contents quoted in the transcript. |
| d8372441c | Include what we use in profile objects and persistence (3 of 3) | Verified no regression (build-only) | Include-hygiene only. Evidence is that the tree under test built and both suites pass at the baseline numbers; no behavioural test invented. |
| 57e9014b3 | Include what we use in profile objects and persistence (2 of 3) | Verified no regression (build-only) | As above. |
| a842ad1fe | Include what we use in profile objects and persistence (1 of 3) | Verified no regression (build-only) | As above. |
| 6bce997a9 | Clear out the CodeQL code scanning alerts | Verified no regression | Three hunks. (1) `SpeechAudioCapture.cpp` - the only behavioural one, `consumedFrames * frameBytes` widened to `qsizetype` before `QByteArray::remove()`: exercised by `STT_spec.lua` in the full busted run (0 failures) and by `SpeechRecognizerContractTest`, `SpeechAcrossProfilesTest` and `SherpaEngineTest` in the full ctest run. (2) `SpeechRecognizerFactory.cpp` - both hunks replace a `// Future:` comment with prose, no code; read and confirmed comment-only. (3) `TBuffer.cpp` - a block-end comment reworded, no code; the CSI path it sits in was exercised by `CsiCursorForwardTest` and by the `TelnetTriggerFuzz` and `Miscallaneous` specs in the same runs. |
| 57bba619b | Silence GCC 16 -Wsfinae-incomplete false positives from AUTOMOC | Verified no regression (build-only) | Compiler-flag and include change only; the tree built and both suites pass. No behavioural test invented. |
| f4849ed77 | deb and rpm packages for Ubuntu, Debian and Fedora | Bug: F-H1-1 | `deb/mkdeb.sh` run outside docker against the existing Release tree ran to completion and produced `mudlet_5.0.0~git20260919.85d814292-1~noble_amd64.deb` (25 MB, `dpkg-deb --info` clean: control, 469 md5sums, postinst, postrm, copyright, gzipped changelog, desktop file, both icons, 407 Lua files, 27 translation files). It also ships QTagEdit dev files - F-H1-1. `rpm/mkrpm.sh` could not be run (no `rpmbuild`); read instead and shown to stage the same tree. `verify-deb.sh`/`verify-rpm.sh` and `CI/linux-packages/build` could not be run (docker binary present, daemon not running). `bash -n` clean on all six scripts and on `install.sh.in` after substitution; the only `@…@` placeholder is `@BASE_URL@`, written by `publish` line 120. |
| 16ac754c1 | The Linux package repository can be browsed in a web browser | Fixed & verified | `write_indexes()` extracted verbatim from `CI/linux-packages/publish` and run against a synthetic site tree (`work-H1/test-indexes.sh`, `work-H1/write_indexes.sh`): 13 of 13 directories got an `index.html`, every link names `index.html` explicitly, the root page carries the `curl … \| sudo sh` line, and every non-root page carries `../index.html`. `globstar` is set at `publish:12`, so the `**/` recursion is real. |
| aff0aab03 | Fix: path to AI-ASSISTANTS.md | Fixed & verified | `docs/GITHUB.md` now links `AI-ASSISTANTS.md`, which resolves to `docs/AI-ASSISTANTS.md` (present). Checked with `work-H1/check-links.py` over every `.md` changed in the range; the only real unresolved relative link is the pre-existing F-H1-2. |
| 08b61b387 | Remove concurrency limit from Check lua_error strands workflow | Fixed & verified | `.github/workflows/lua-error-strands.yml` has no `concurrency:` block, and the file parses as YAML. |
| CI-only commits (as a group) | workflows, CI scripts, test/spec additions, dependency and Crowdin bumps | Verified no regression | Eleven of the twelve workflow files changed in `Mudlet-5.0.1...HEAD` still exist and all load with `yaml.safe_load`; `cleanup-caches.yml` was deleted in the range. Full ctest and full busted re-run and compared with the baseline - see Automated. |

## Automated

Re-run on the unchanged tree, exactly as the baseline did, both suites in
parallel from `/home/user/Mudlet`:

- `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -j 2`
  → **100% tests passed, 0 tests failed out of 220** (334.85 s wall).
  Baseline: 220/220. No difference, no flakiness seen.
  Log: `work-H1/full-ctest.log`.
- `.claude/scripts/run-lua-tests.sh`
  → **4702 successes / 0 failures / 0 errors / 86 pending** (208.31 s).
  Baseline: 4702 / 0 / 0 / 86 pending. Identical.
  Log: `work-H1/full-busted.log`.

Release-infrastructure subset, run separately first:
`QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R 'Release|SemVer|Milestone|XdgRecipe|CMakeListsConsistency|HomeUntouched'`
→ 11/11 passed: `CMakeListsConsistencyTest`, `HomeUntouchedTest`,
`MilestoneResolutionTest`, `NewReleaseDialogTeardownTest`,
`ReleaseChangelogSpanTest`, `ReleaseChecksumPairingTest`,
`ReleaseChecksumsTest`, `ReleasePlatformAssetTest`, `ReleaseTagVersionTest`,
`SemVerTest`, `XdgRecipeConsistencyTest`.
Log: `work-H1/ctest-release.log`.

`--version` under `QT_QPA_PLATFORM=offscreen` (Release builds, throwaway HOME),
both exit 0:

```
mudlet 5.0.0 -dev-4dc7a5ce6      # /home/user/Mudlet/build-linux-release/src/mudlet
mudlet 5.0.1 -dev-592821c8c      # /home/user/worktrees/v501/build-linux-release/src/mudlet
```

Two notes on that, neither a regression: the development Release binary is
stamped `4dc7a5ce6`, an ancestor of HEAD whose only difference from the tree
under test (85d814292) is `docs/qa/` - `git diff 85d814292 HEAD -- src CI .github CMakeLists.txt`
is empty, so the binary is the tree under test. And the stray space in
`5.0.0 -dev-…` is present in the 5.0.1 binary too (`5.0.1 -dev-…`), so it
predates the range.

## Not run here, with the reason

| Thing | Reason |
|-------|--------|
| `CI/linux-packages/build deb|rpm` | needs a running docker daemon (`docker info` fails) and a full in-container compile; the disk has ~6.6 GB free |
| `deb/verify-deb.sh`, `rpm/verify-rpm.sh` | both run inside a clean base image (`apt-get install` / `dnf install` of the package); no docker daemon |
| `rpm/mkrpm.sh` | no `rpmbuild` on this host; read instead, and its `cmake --install … ` + generated `%files` list shown to carry the same tree as the .deb |
| `CI/linux-packages/publish sign|assemble|upload` end to end | needs gpg keys, `dpkg-scanpackages`/`createrepo_c` and R2 credentials; the `write_indexes` half was extracted and run (16ac754c1 row) |
| `install.sh` against a live repository | needs the published bucket; syntax-checked after `@BASE_URL@` substitution instead |
| The GCC 16 / Clang 21 warning counts for 57bba619b | this host builds with the project's preset compiler only |

## Artefacts

Everything cited lives under `<scratchpad>/qa/`:
`work-H1/` (launch logs `mudlet.log`..`mudlet4.log`, the `seam*.lua` probes, the
three `repro-*.sh` scripts, `check-links.py`, `write_indexes.sh` +
`test-indexes.sh`, `mkdeb.log` / `mkdeb2.log`, `deb-contents.txt`,
`discord-frames.jsonl`, `full-ctest.log`, `full-busted.log`,
`ctest-release.log`, and the built
`mudlet_5.0.0~git20260919.85d814292-1~noble_amd64.deb` kept as evidence) and
`shots-H1/` (01-23).

Processes: Xvfb :81, openbox and the Discord fixture were started by this agent,
their PIDs recorded in `work-H1/pids.txt`, and all were killed by PID at the
end. The scratch `/build` tree the packaging run needed was removed.
