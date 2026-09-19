# Verification of batch 6: H1 (refactor seams, packaging, release infrastructure)

Tree under test: 85d814292 (`git diff 85d814292 HEAD -- src CI .github CMakeLists.txt 3rdparty`
is empty; HEAD 66e95d024 differs only in `docs/qa/`). Binary title bar in every
screenshot reads `Mudlet 5.0.0-dev-85d814292`, so the replays ran on the same code
H1 tested. Display :81, throwaway HOMEs, `MUDLET_TEST_MODE=1`. Per the brief I did
not re-run the full suites (H1's logs were audited instead).

## Verdicts

| Finding | Reported severity | Verdict | Your severity | Notes |
|---------|-------------------|---------|---------------|-------|
| F-H1-1 the .deb ships QTagEdit dev files | Minor / New bug | **CONFIRMED** | Minor (agree) | Re-ran `work-H1/repro-F-H1-1.sh` verbatim: exit 0, same four artefacts in the package (`usr/include/QTagEdit/qtagedit.hpp`, `usr/lib/libQTagEdit.a`, two `usr/lib/cmake/QTagEdit/*.cmake`). Cause and "New bug" classification both check out: `CI/linux-packages` does not exist in `Mudlet-5.0.1`. Tracker: not known. |
| F-H1-2 `docs/CONTRIBUTING.md` link to `UI-design-philosophy.md` | Cosmetic / Note (pre-existing) | **CONFIRMED** | Cosmetic (agree) | Link at line 7, file is at repo root, identical line in `git show Mudlet-5.0.1:docs/CONTRIBUTING.md`. My own independent link checker over the 19 changed `.md` files agrees this is the only real unresolved relative link. Tracker: adjacent open #10599 (wiki links), not the same link. |
| F-H1-3 `saveProfile()` returns `current//<file>.xml` | Cosmetic / Note (pre-existing) | **CONFIRMED** | Cosmetic (agree) | My run printed `.../current//2026-09-19#16-11-13.xml`. `MudletPaths.cpp` `profileXmlFilesPath` returns a trailing `/` and `Host::saveProfile()` appends `%1/%2.xml`; `Mudlet-5.0.1:src/mudlet.cpp:5900` and `Mudlet-5.0.1:src/Host.cpp:1052` are identical, so pre-existing is right. Tracker: adjacent open #10633 (`saveProfile(dir,name)` returns a path for an export never written) - different symptom, same function, worth linking. |

No finding of H1's failed to reproduce, and I found nothing new worth a finding
(no F-V-n).

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
|--------|------|-----------|--------|---------|-----------------------|
| findings-H1.md | 26 (25 named commits + the CI-only group) | 26/26 - every cited screenshot exists and shows what is claimed; every cited log line is present | 16 | 16 | none |

Rows re-run by me: bae14c127, f50bb3bd9, 1187379af, efaf9164d, 411737ebe,
ed43d2fea, bbae10e5b, 5c1c3b907, 56a87d2f7, 7383ed6c9, e89b55e6f, 3de265ac8,
f4849ed77, 16ac754c1, aff0aab03, 08b61b387, plus the CI-only workflow-lint group
(17 counting the group). That is over 60% of the table and covers every row my
assignment named.

Rows not re-run: de041bbeb and 07201697f (evidence screenshots audited instead -
`shots-H1/21` shows the editor tree with `generic_mapper`, `shots-H1/23` shows it
gone after `uninstallPackage`, exactly as claimed), e268326a4 (map-audit flag -
needs an ini edit and a second restart; evidence in `work-H1/mudlet.log` /
`mudlet2.log` is present and consistent), 6bce997a9, and the five build-only rows.

**Honesty of the build-only rows**: correct. e9dad5f2b, a842ad1fe, 57e9014b3,
d8372441c and 57bba619b are each marked `Verified no regression (build-only)` and
say in the "How verified" column that the evidence is only that the tree built and
both suites matched the baseline, with "No behavioural test invented" spelled out.
6bce997a9 is not in that group and does carry real reasoning (the one behavioural
hunk, `SpeechAudioCapture.cpp`, is tied to named specs and ctest cases; I read the
commit and the other two hunks are indeed comment-only). H1's "Not run here" table
is likewise accurate: `rpmbuild`, `patchelf` and `fakeroot` are absent from this
host and `docker info` fails, exactly as claimed.

One scope gap, not H1's fault: the plan's "Listed twice on purpose" paragraph
names 72d96a5ca ("Tests no longer leave key files in your home directory") among
the refactors, but it is missing from the H1 commit list, so no row exists for it.
It is covered indirectly - `HomeUntouchedTest` passed in H1's release subset.

## Details

### F-H1-1 - packaging (CONFIRMED)
Replayed `work-H1/repro-F-H1-1.sh` unchanged. Log: `work-V6/mkdeb.log` (exit 0),
package `mudlet_5.0.0~git20260919.85d814292-1~noble_amd64.deb`, tail of the run:

```
./usr/include/QTagEdit/qtagedit.hpp        5044
./usr/lib/cmake/QTagEdit/QTagEditTargets-release.cmake   867
./usr/lib/cmake/QTagEdit/QTagEditTargets.cmake          4136
./usr/lib/libQTagEdit.a                                93726
```

Cause verified independently: `3rdparty/qt-tags-widget/CMakeLists.txt:40-58` has
unconditional `install(TARGETS …)` / `install(FILES …)` / `install(EXPORT …)`;
`rpm/mkrpm.sh:24` runs the same `cmake --install "$SRC/build" --prefix "$STAGE/usr"`
and then builds `%files` from `find "$STAGE"`, so the rpm inherits it - H1's
"by construction" claim is sound. `git ls-tree Mudlet-5.0.1 CI/linux-packages` is
empty, so this reaches users only because of f4849ed77: **New bug** is right.
The `Depends:` caveat H1 recorded reproduces too (host Qt in `/opt/qt` has no dpkg
owner). I removed `/build` and the shim afterwards; disk went back to 6.7 GB free.

Tracker: `recent-issues.md` / `open-prs.md` have nothing on Linux packaging;
`search_issues` "Linux deb rpm package includes development files static library
headers cmake config" returned only closed #7983 ("Missing requirement for Debian
build"); `search_pull_requests` for open deb/rpm/linux-package PRs returned none.
Status stands as a new, unknown bug.

### F-H1-2 / F-H1-3 (both CONFIRMED, both pre-existing)
See the verdict table. My link checker (`work-V6/link-check.txt`) reports 3
"BROKEN" lines, of which two are the same regex artefact H1 described - the source
really is malformed nested markup, `[clazy]([url](https://github.com/KDE/clazy))`
at `docs/CONTRIBUTING.md:72` and the same shape at line 125. Both are present
verbatim in `Mudlet-5.0.1:docs/CONTRIBUTING.md` (lines 70 and 122), and they render
as a live (if ugly) link rather than a 404, so I agree with H1 that they are not a
finding; recording them here as an observation only.

### Refactor seams re-run (all held up)
One profile, fresh HOME, then a second profile, then a restart on the same HOME.

- **bae14c127 / f50bb3bd9** - `getMudletHomeDir()` →
  `<HOME>/.config/mudlet/profiles/Mudlet self-test`, mode `directory`. After a
  clean shutdown the directory holds `profile.ini profile.dic profile.aff current/
  log/ map/ map downloads/ description port url command_history_main
  base_ui_settings.lua` and one directory per installed package
  (`work-V6/profile-layout.txt`). Note for the record: `profile.ini` is written at
  shutdown, not by `saveProfile()`, so a mid-session listing does not show it -
  H1's listing was taken after a save+exit and is right.
- **1187379af** - `startLogging(true)` → `.../log/2026-09-19#16-11-13.txt`;
  `startLogging(false)` returned the same path; the file
  (`work-V6/profile-log-sample.txt`) holds `Log session starting at 16:11:13 …`,
  the `feedTriggers` line (`V6 log line alpha`), the `echo` and `cecho` lines, and
  `Log session ending at 16:11:35 …`. Note that `feedTriggers` text reaches the log
  file but not the `--mirror` stream.
- **efaf9164d / 411737ebe** - with two profiles open the Central Debug Console
  printed `[✱] 2 profiles active now …`, `[A] = "Mudlet self-test"`,
  `[B] = "V6 second"`, and the profile tabs relabelled themselves `[A] …` / `[B] …`
  (`shots-V6/12-debug-console.png`, `13-main-raised.png`). Firing the same alias in
  each profile produced interleaved `[B] Alias name=85(^v6ping$) matched.` and
  `[A] Alias name=90(^v6ping$) matched.` lines, each stamped `HH:MM:SS.zzz`
  (`shots-V6/15-debug-two-profiles.png`). `[✱] Profile 'X' ended.` for both at
  shutdown. H1's caveat is correct and I reproduced it: `debugc()` goes to the
  profile's own Errors view (`errors_V6 second| [DEBUG:] debugc from V6 second`),
  not to this console, so the console's evidence is the [A]/[B] traffic.
- **ed43d2fea / bbae10e5b / 5c1c3b907 (and the behaviour of 40c805352)** - an MXP
  `<FRAME V6Frame …>` built a real console: `getColumnCount("V6Frame")` 19,
  `getRowCount` 2, and `<DEST V6Frame>` text landed in it
  (`V6Frame| hello from V6 frame`, visible in `shots-V6/03-frame-toolbar.png`).
  `tempButtonToolbar("V6Bar",0,1)` + `tempButton("V6Bar","V6Button",1)` drew the
  toolbar in the same shot.
- **Toolbar and dictionary across a restart (5c1c3b907 / 442b4ac88)** - Games >
  Close Mudlet logged `Saving profile's own Hunspell dictionary... Saved an extra 1
  words in dictionary.`; `profile.dic` on disk is `1\nzylophrax`. After relaunching
  on the same HOME: `RELOAD-BUTTON-EXISTS: 1`, `RELOAD-BAR-EXISTS: 1`,
  `SPELL-AFTER-RESTART: true`, toolbar visible again
  (`shots-V6/17-reloaded.png`).
- **56a87d2f7 / 07201697f** - second profile "V6 second" created and opened Offline
  from the connection dialog; tab bar, per-profile editor (Alt+E, its own trigger
  tree) and a single clean shutdown of both, no warnings in `work-V6/mudlet.log`.
- **7383ed6c9 / e89b55e6f** - `getColumnCount`/`getRowCount` on a named console,
  `spellCheckWord` from the profile dictionary, and the decorated system lines
  (`[ OK ] - Profile "…" loaded in offline mode.`, the MXP auto-enable notice) all
  as described.
- **3de265ac8** - `CI/discord-ipc-fixture.py` + `libdiscord-rpc.so`:
  `Discord integration loaded` in `work-V6/mudlet3.log`, and
  `work-V6/discord-frames.jsonl` ends
  `{"activity": {"assets": {"large_image": "mudlet"}, "details": "V6 detail text",
  "state": "V6 state text", …}, "cmd": "SET_ACTIVITY", "nonce": "9"}` - the same
  frame sequence H1 captured. One practical note for future replays: the fixture's
  runtime dir must be short (`/tmp/mdV6-XXXX`); under the scratchpad path it dies
  with `socket path … too long for AF_UNIX`.

### Packaging and CI static checks re-run
- Workflow YAML: 12 files changed in `Mudlet-5.0.1...85d814292`, 11 exist and all
  load with `yaml.safe_load`, `cleanup-caches.yml` was deleted. Exactly H1's count.
- 08b61b387: `lua-error-strands.yml` has no `concurrency:` key.
- aff0aab03: `docs/GITHUB.md:42` links `AI-ASSISTANTS.md`; `docs/AI-ASSISTANTS.md`
  exists.
- 16ac754c1: I extracted `write_indexes()` from `CI/linux-packages/publish` myself
  and ran it against a 13-directory synthetic site: 13/13 `index.html` written,
  every directory link names `index.html`, non-root pages carry `../index.html`,
  the root page carries `curl -fsSL <BASE_URL>/install.sh | sudo sh`. `globstar` is
  set at `publish:11`.
- `bash -n` clean on `mkdeb.sh`, `mkrpm.sh`, `verify-deb.sh`, `verify-rpm.sh`,
  `publish`, `build`; `install.sh.in`'s only placeholder is `@BASE_URL@`.
- Tool availability on this host, checked myself: `dpkg-deb` and `docker` present;
  `rpmbuild`, `patchelf`, `fakeroot` absent; `docker info` fails. H1's "could not
  run" list is accurate.

### Automated suites
Not re-run (coordinator instruction; disk ~6.6 GB). I audited H1's logs instead:
`work-H1/full-ctest.log` ends `100% tests passed, 0 tests failed out of 220`
(334.85 s) and `work-H1/full-busted.log` ends
`4702 successes / 0 failures / 0 errors / 86 pending`, both matching the baseline.

## Artefacts
`work-V6/`: `repro-V6.sh` (replay recipe), `v6a.lua`, `v6a2.lua`, `v6b.lua`,
`v6c.lua`, `v6d.lua`, `alias.lua`, `idx.sh` (extracted `write_indexes`),
`mudlet.log`, `mudlet2.log`, `mudlet3.log`, `fixture.log`, `discord-frames.jsonl`,
`mkdeb.log`, `link-check.txt`, `workflows-changed.txt`, `profile-layout.txt`,
`profile-log-sample.txt`, `profile.dic.copy`, `pids.txt`.
`shots-V6/`: 01-start, 02-v6a, 03-frame-toolbar, 04-games-menu, 05-connect-dialog,
06-new-profile, 07-second-profile, 08-toolbox-menu, 09-editor, 10-editor-overflow,
11-editor-max, 12-debug-console, 13-main-raised, 14-debugB, 15-debug-two-profiles,
16-after-close, 17-reloaded, 18-discord-start.
The built .deb, the `/build` staging tree and both throwaway HOMEs were deleted
after the checks; disk left at 6.7 GB free. Processes: Xvfb :81 and openbox (PIDs
in `work-V6/pids.txt`), three Mudlets and one Discord fixture, all started and
killed by PID by me.
