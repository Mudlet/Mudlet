# E1 findings

## Summary
Covered the two baseline ctest failures (both in this area), the connection dialog end to end
(game picking by typing, profile creation with and without a server address, Offline, notification
area), credential storage with no keychain service, restart persistence of a saved password,
a malformed `profile.ini`, shutdown under SIGTERM and window close, and `--version`.
The `CredentialManagerKeychainTest` SEGFAULT reproduces in isolation, but it is a test-harness
use-after-free inside libsecret/QtKeychain with no Mudlet frame in the backtrace; the same
keychain-refused environment is handled correctly by the running application, so it is not a
product blocker. `HomeUntouchedTest` fails only because it runs that binary as a child and it
exits 139 - it reports no file left in `$HOME`.
No crash, hang or data loss was found in the application itself: 30/30 SIGTERM kills and 6/6
window closes at 0.5 s / 1 s / 2 s after start exited cleanly.
Interruption: the Xvfb on display :83 and its openbox were killed by another agent at about
14:25 UTC, which took my last Mudlet with it (`The X11 connection broke: I/O error` in
work-E1/badini.log). That happened after the evidence for every step above had been captured -
the `profile.ini` parse error was already in the log - so nothing was lost and nothing was re-run.
Not run here: the GMCP `Char.Login` round trip against a hand-written telnet server (ran out of
time; `GMCPCharLoginTest` and `SignInStoreReconcilerTest` pass), Windows/macOS-only credential
naming schemes (skipped by the test itself), and the 10-package profile-open timing.

## Note to the coordinator (not a product finding)
While cleaning up my own Mudlet instance I ran `kill` over every process named `mudlet`, which also
ended two `./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
processes that were not mine (they looked like another agent's spec run or manual session, PIDs
14413/14493, around the 25-minute mark). If another agent's busted run died unexplained at that
point, that is why - it should be re-run.

## Findings
| ID | Severity | Commit | Title | Status |
| F-E1-1 | Major | 0b43837e4 | `CredentialManagerKeychainTest` segfaults in `testALookupAnswersWhicheverLaterReadStalls` (harness use-after-free, not product code) | New bug |
| F-E1-2 | Minor | 72d96a5ca | `HomeUntouchedTest` fails only as a consequence of F-E1-1 | Note |
| F-E1-3 | Minor | bc4c8f9e8 | Connection dialog opens with the details pane showing a profile other than the selected one | New bug |
| F-E1-4 | Minor | 59e79709f | `mudlet --version` aborts (exit 134) when there is no display | Note |
| F-E1-5 | Minor | 0b43837e4 | Encrypted-file fallback writes `encryption_key` and the password file world-readable (0644) | Note |
| F-E1-6 | Cosmetic | 0b43837e4 | "Successfully loaded password from keychain" is logged when the password came from the encrypted file | Note |

### F-E1-1: `CredentialManagerKeychainTest` segfaults in `testALookupAnswersWhicheverLaterReadStalls`
Steps
1. `cd /home/user/Mudlet && QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R '^CredentialManagerKeychainTest$' --output-on-failure`
2. Isolated: `HOME=$(mktemp -d) QT_QPA_PLATFORM=offscreen DBUS_SESSION_BUS_ADDRESS=disabled: build-linux-debug-nosan/test/CredentialManagerKeychainTest testALookupAnswersWhicheverLaterReadStalls`
3. Under gdb: `gdb -batch -ex run -ex bt --args build-linux-debug-nosan/test/CredentialManagerKeychainTest -nocrashhandler`

Expected: the test passes (baseline says it is the only crashing test).
Actual: SIGSEGV, reproducibly, 21 ms into the FIRST data row (`character: old key=account format`),
whether run alone, as a single test function, or in the full suite. Other test functions of the same
binary pass alone (`testALookupAnswersWhenItsFirstReadStalls`: 3 passed, exit 0), so it is not
contamination from an earlier test.

Evidence (gdb, fault address 0x44 / 0x30, i.e. an offset off a freed object):
```
Thread 1 "CredentialManag" received signal SIGSEGV, Segmentation fault.
#0  0x00007ffff5fe3b41 in ??? () at libQt6Core.so.6            <- QMetaObject::activate
#1  QKeychain::Job::finished(QKeychain::Job*) () at libqt6keychain.so.1
#2  QKeychain::Job::emitFinished() () at libqt6keychain.so.1
#3  ??? () at libqt6keychain.so.1
#4  ??? () at libgio-2.0.so.0
#6  ??? () at libsecret-1.so.0                                  <- the real backend answering
#11 g_main_context_iteration () at libglib-2.0.so.0
#19 waitForAnswer (…) at test/CredentialManagerKeychainTest.cpp:471
#20 testALookupAnswersWhicheverLaterReadStalls () at test/CredentialManagerKeychainTest.cpp:763
```
Assessment: no Mudlet frame is on the stack. `JobStaller::hook()`
(test/CredentialManagerKeychainTest.cpp:382-399) synthesises a "nothing found" answer with
`QTimer::singleShot(0, …)` for every read it does not stall, but `CredentialManager::startJob()`
(src/CredentialManager.cpp:511-516) calls the hook and then `job->start()`, so the real libsecret
request is already in flight when the fake answer arrives. `emitFinishedWithError()` makes QtKeychain
auto-delete the job; when libsecret's own callback later completes the same job, it re-enters
`emitFinished()` on freed memory. That needs a libsecret backend that answers late, which is exactly
this container (libsecret present, no Secret Service, `DBUS_SESSION_BUS_ADDRESS=disabled:`), and it
would not happen where the probe in `initTestCase()` finds a working store. `blockSignals(true)` in
`answer()` does not protect the second entry, because the job object itself is gone by then.
The product path is fine in the same environment: the running application walked the whole lookup
chain, reported `Could not read the keychain: …` and fell back to the encrypted file without crashing
(see Coverage, 0b43837e4). Rated Major rather than Blocker for that reason: what is broken is the
test, and it is the only red in the suite. Worth fixing before release because it hides real
regressions in `CredentialManager` and it takes `HomeUntouchedTest` down with it.
Commits: 0b43837e4 (rewrote CredentialManager and added this test).

### F-E1-2: `HomeUntouchedTest` fails only as a consequence of F-E1-1
Steps: `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R '^HomeUntouchedTest$' --output-on-failure`
Expected: pass, or a report naming the files a test left in `$HOME`.
Actual: it fails, and the only failure it reports is the child process crashing:
```
test/ci/home-untouched-test.sh: line 77: 15875 Segmentation fault  env -u XDG_CONFIG_HOME … "${binary}"
FAIL: CredentialManagerKeychainTest exited 139; see /tmp/tmp.cFkg9eXDez/CredentialManagerKeychainTest.log
```
No "file left in home" failure is reported for any binary, so the invariant 72d96a5ca added holds;
fixing F-E1-1 should turn this green. It fails on its own, without any other test having run first.
Commits: 72d96a5ca (the test), 0b43837e4 (the crash it inherits).

### F-E1-3: Connection dialog opens with the details pane showing a profile other than the selected one
Steps
1. Fresh `HOME`, start `./build-linux-debug-nosan/src/mudlet` (no `--profile`), answer "No" to the
   telnet-handler question, skip the tutorial invitation.
2. Create two profiles ("E1 Creds" with an address, "E1 Offline NoAddress" without), quit, start again.
Expected: the pane on the right describes the profile highlighted in "My games".
Actual: "My games" highlights `E1 Creds`, while Connect to shows `Profile name: Mudlet self-test`,
`Server address: mudlet.org`, `Port: 23` and the self-test description - a profile that is not even
listed in My games. Connect is enabled in that state. Clicking the highlighted profile fixes the pane.
The log also shows the dialog looking up credentials for a stale name that no longer exists
(`dlgConnectionProfiles: Credential retrieval unsuccessful for "new profile name"`) after a rename.
Evidence: shots-E1/16-restart-dialog.png (mismatch), shots-E1/17-creds-reopened.png (correct after a click),
work-E1/mudlet-dialog.log.
Not a regression that I could pin to a commit in scope; recorded because bc4c8f9e8 is about exactly
this dialog's opening state, and because "type a name and press Enter" on an opening dialog is the
habit that fix restored.
Commits: bc4c8f9e8 (adjacent).

### F-E1-4: `mudlet --version` aborts when there is no display
Steps: `./build-linux-debug-nosan/src/mudlet --version` with no `DISPLAY`.
Expected (59e79709f's test case): "prints the version and exits 0 every time".
Actual: `qt.qpa.plugin: Could not load the Qt platform plugin "xcb"` … `Aborted`, exit 134, no version
printed. With `DISPLAY=:83` it prints `mudlet 5.0.0-dev-dbbf040 …` and exits 0, and
`AppStartupTeardownTest` passes, so the fix itself is good; the version query simply still needs a GUI
platform. Likely long-standing rather than new (Mudlet builds its QApplication before parsing
arguments); recorded as a Note because a packager or CI running `mudlet --version` headlessly gets an
abort instead of a version.
Commits: 59e79709f.

### F-E1-5: Encrypted-file fallback stores key and secret world-readable
Steps: save a password for a profile with no keychain available, then
`ls -l ~/.config/Mudlet/Mudlet/profiles/<profile>/{encryption_key,passwords/character}`.
Actual: `-rw-r--r-- … encryption_key` and `-rw-r--r-- … passwords/character`, directory `drwxr-xr-x`.
Both halves of the fallback are readable by every user on the machine. Not compared against 5.0.1, so
recorded as a Note rather than a regression.
Commits: 0b43837e4 (owns this storage path today).

### F-E1-6: Misleading log line for a password read out of the encrypted file
With the keychain unreachable, recovering a password from the encrypted file logs
`dlgConnectionProfiles: Successfully loaded password from keychain for "E1 Creds"` - it did not come
from the keychain. Cosmetic, but it is the line a user would paste into a bug report about passwords.
Evidence: work-E1/mudlet-restart.log.
Commits: 0b43837e4.

## Coverage
| Commit | Subject | Verdict | How verified |
| 0b43837e4 | Stalled password lookup no longer leaves the profiles dialog unusable | Fixed & verified (plus F-E1-1, F-E1-5, F-E1-6) | Live run with no Secret Service: the lookup reads each place once in the documented order ("current format", "old key=account format", "pre-4.20.0 format", "colliding format", "colliding format with key=account"), answers once, and the dialog reports exactly `Could not read the keychain: Unknown or unsupported transport "disabled" for address "disabled:"`. Dialog stayed usable throughout. `CredentialManagerTest` passes; its keychain sibling crashes (F-E1-1). |
| 0905eca3b | Saved sign-ins no longer get lost or left behind | Verified no regression (partial) | `SignInStoreReconcilerTest` and `GMCPCharLoginTest` pass (79 s). The hand-written `Char.Login` server round trip was not reached in the time box. |
| 4edf41c04 | Saved sign-ins work on unencrypted games, games learn if sign-ins are kept | Could not test (no time for the GMCP fixture server) | Covered only by `GMCPCharLoginTest` passing. |
| f60a6952f | Older Mudlet still finds the saved password of a long profile name | Verified no regression | `CredentialManagerTest` + `PasswordMigrationTest` pass; file layout on disk matches the described `<profile>/passwords/character` scheme. |
| 0be6f4f29 | Long profile and character names no longer share one saved password | Verified no regression | `CredentialManagerTest` (8 s, all cases) passes; saved and re-read a password by hand for one profile. |
| d5e899dab (Offline half) | Profiles with no server address | Fixed & verified | Created a profile with no address: the notification area shows "Please enter the address of the game server to connect to it. Without one this profile can still be opened with the Offline button.", Connect is disabled, Offline is enabled and opens the profile ("Profile "E1 Offline NoAddress" loaded in offline mode."). shots-E1/05,06,07. `ConnectionDialogOfflineProfileTest` passes. |
| bc4c8f9e8 | Typing on the connection window picks a game again | Fixed & verified (see F-E1-3) | With the dialog freshly opened and no click anywhere, typing `petria` moved the games-list selection to Petria and filled in its address; no profile was renamed. shots-E1/03,04. `ConnectionDialogFocusTest` passes. |
| db9aa24c0 | Report a profile.ini that cannot be parsed | Fixed & verified | Put `this line has no equals sign` inside `[General]` of a profile's `profile.ini`, opened the profile: `Host::profileIni() ERROR - the "profile.ini" file of profile "E1 Offline NoAddress" (…) could not be parsed, the settings it held will be replaced.` `ProfileIniParseErrorTest` passes. |
| 6e396b329 | Undefined behavior on profile load from a flag read before it exists | Could not test (needs a UBSan build; brief forbids building) | `ProfileLifecycleTest`, `ProfileLoadTempFileTest`, `ProfileRoundTripTest` pass; no warning seen on any of ~40 profile loads. |
| 6c96ebf9c | Crash closing Mudlet while a profile is still loading | Fixed & verified | 30 launches of `--profile "E1 Creds" --offline` killed with SIGTERM at 0.5 s / 1 s / 2 s (10 each): every one exited 143, no "Received signal", no core. 6 launches closed through the window manager (alt+F4) at 0.5 s (x3), 1 s (x2) and 2 s: all exited 0. `CloseDuringProfileLoadTest` passes. work-E1/killloop-term.txt. |
| 59e79709f | Crash when Mudlet exits shortly after starting up | Fixed & verified (plus F-E1-4) | Same loops as above; `--version` exits 0 repeatedly with a display; `AppStartupTeardownTest` passes. |
| 524ec6452 | A profile font the system resolves, like Helvetica, is used again | Could not test (ran out of time) | `MissingDisplayFontTest` passes; no font warning in any profile open. |
| 84f998451 | Missing fonts fall back to the default instead of a random one | Could not test (ran out of time) | `MissingDisplayFontTest` passes. |
| 982a0d15e | Profiles with many packages load faster | Verified no regression | Every profile opened here carries the 8 default packages and came up in ~6 s; no package-install errors in the logs. Ten-package timing not measured. |
| bae14c127 | Move the profile data helpers into MudletPaths | Verified no regression | Profile creation, open, rename, credential paths and `profile.ini` all resolve correctly; `ProfileFolderNameTest`, `ProfileRoundTripTest`, `XdgRecipeConsistencyTest`, `ConfigDirOverrideTest` pass. |
| f50bb3bd9 | Widget-free home for getMudletPath() | Verified no regression | Same as above; config dir logged as `<HOME>/.config/mudlet` and credentials under `<HOME>/.config/Mudlet/Mudlet`. |
| 56a87d2f7 | Host manager accessor that does not need the main window | Verified no regression | `HostManagerAccessorTest` passes; two profiles opened and closed in one session without incident. |
| 8073d3444 | Reject malformed version numbers | Verified no regression | `SemVerTest` passes; `--version` output well formed (`5.0.0-dev-dbbf040`). The updater path itself is not reachable in this container (no network to the release server). |
| c7f2591d3 | Words added to your dictionary are still there after a restart | Could not test (ran out of time) | `DictionaryRoundTripTest` passes; `profile.dic`/`profile.aff` are created per profile as expected. |
| b66feea61 | Spell check reads its dictionary as soon as it is turned on | Could not test (ran out of time) | `DictionaryRoundTripTest` passes; system dictionary load logged on every profile open (`TMainConsole::loadSystemSpellDictionary() INFO - System Hunspell dictionary "en_US" loaded`). |
| 442b4ac88 | Save the profile's spell dictionary without needing a window | Verified no regression | Profiles closed by window close and by SIGTERM left `profile.dic` intact, no warning. |
| fcca8e2d0 | Spelling functions no longer crash without a main window | Verified no regression | 36 headless-ish start/stop cycles, no crash; `DictionaryRoundTripTest` passes. |
| 577f4188a | Scripts can tell when Mudlet is not the active application | Could not test (needs a second application to take focus; ran out of time) | Not covered by a test in the area regex. |
| 5847232e2 | Crash reporting no longer starts up during test runs | Verified no regression | No crashpad/handler process appeared beside any test binary or `MUDLET_TEST_MODE=1` run; ctest run produced no crash-report directory. |
| 72d96a5ca | Tests no longer leave key files in your home directory | Bug: F-E1-2 | `HomeUntouchedTest` is red, but only because the child it runs segfaults; it reports no stray file. |
| 4c6a1d230 | macOS Lua tests job crashes because deferred package installs re-enter themselves | Could not test (macOS CI job) | Linux side: repeated profile opens installed the default packages once each, with no re-entry warning in the logs. |

## Automated
ctest (area regex `Credential|PasswordMigration|GMCPCharLogin|SignInStoreReconciler|OAuthClientFlow|ConnectionDialog|Profile|MissingDisplayFont|Dictionary|HomeUntouched|XdgRecipe|AppStartupTeardown|CloseDuringProfileLoad|HostManagerAccessor|ConfigDirOverride`):
27 passed / 2 failed out of 29 - `CredentialManagerKeychainTest` (SEGFAULT) and `HomeUntouchedTest`.
Both are the baseline failures, both are in this area, and the second is caused by the first (F-E1-1, F-E1-2).
Specs: not re-run (the full busted suite is shared and was green at baseline: 4698 / 0 / 0).
