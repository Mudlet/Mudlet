# Verification of batch 5: G1 (packages, addCommand, STT/TTS) and G2 (media, MSP, IRC, Discord, DB)

Verifier V5. Display `:83`, work dir `<scratchpad>/qa/work-V5/`, screenshots `<scratchpad>/qa/shots-V5/`.
Binary under test: `/home/user/Mudlet/build-linux-debug-nosan/src/mudlet` on tree 7c4d43e81, which differs
from the agents' 85d814292 only in `docs/qa/*.md` (`git diff --stat 85d814292 HEAD` = 2 doc files), and the
window title in every screenshot reads `Mudlet 5.0.0-dev-85d814292`. 5.0.1 comparison binary:
`/home/user/worktrees/v501/build-linux-debug-nosan/src/mudlet`.

All fixtures were rebuilt from scratch (`work-V5/make-packages-v5.sh`), not reused from the agents' dirs;
only G2's IRC server (`irc-server.py`) and its 10 s `tone10.wav` were reused, as the brief allows.
Every launch used a throwaway HOME created under `work-V5/` and `MUDLET_TEST_MODE=1`.

## Verdicts

| Finding | Reported severity | Verdict | Your severity | Notes |
| --- | --- | --- | --- | --- |
| F-G1-1 | Blocker | CONFIRMED | Blocker | Own gdb run: SIGSEGV in `XMLimport::importPackage` at `src/XMLimport.cpp:196`. G1's 5.0.1 log is genuine (v501 worktree binary, same crash at `XMLimport.cpp:188`). Pre-existing, not tracked. |
| F-G1-2 | Major | CONFIRMED | Major | File written outside the profile dir into a directory I created; package still registers. Known: #10626 (open). |
| F-G1-3 | Major | CONFIRMED | Major | `V5Pkg` submenu still in Options -> Extensions after uninstall; `removeCommand(id)` = true then false. Known: #10758 (open). |
| F-G1-4 | Minor | CONFIRMED | Minor | Five failing installs in one chunk all return `true, nil`; reasons only on the console. Known: #10208, #10639 (both open). |
| F-G1-5 | Minor / Note | CONFIRMED | Minor (Note) | Uncompilable script -> package registered, `sysInstall` + `sysInstallPackage` raised as success. No tracker entry (nearest #10625). |
| F-G2-1 | Minor | CONFIRMED | Minor | `! op kicked bystander` - kick reason "you were spamming" dropped, in the window and in `sysIrcMessage`. Fix incomplete on closed #10534. |
| F-G2-2 | Minor | CONFIRMED | Minor | Self-kick: `#t` disappears from the buffer list, `srv` pane has no kick line; only Lua is served. Fix incomplete on closed #10534. |
| F-G2-3 | Major | CONFIRMED | Major | `safe_name('v5test1')` = `safe_name('v5test2')` = `vtest`, one `Database_vtest.db`, `v5test2` fetch returns both rows. Known: #10727 (open). |
| F-G2-4 | Minor | CONFIRMED (stronger) | Minor | Typo'd `_uniqe` accepted silently **and the constraint is genuinely lost** - two identical rows inserted (G2 only showed silence). Known: #9818 (open). |
| F-G2-5 | Minor / Note | CONFIRMED | Note | 60 rapid plays of an undecodable file -> exactly 8 `sysMediaFinished`, 0 players left, VmRSS +12 kB. Pre-existing. |
| F-G2-6 | Cosmetic | CONFIRMED | Cosmetic | 9 "needs fixing by Mudlet Makers" lines in one ordinary session: Numeric x5, Names x1, Join x1, Kick x2. Known: #5492 (open). |
| G2's claim: #10535 fixed | - | CONFIRMED FIXED | - | `[INFO] <b>not bold</b> &amp; more` renders literally in the srv pane (`shots-V5/09`). Issue still open - closable. |
| G2's claim: #10536 fixed | - | CONFIRMED FIXED | - | `sysIrcMessage` delivered `Fish & Chips <here> &amp; &lt;b&gt;` byte for byte. Issue still open - closable. |

No finding was downgraded and no new defect fell out, so there are no F-V-* findings this batch.

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
| --- | --- | --- | --- | --- | --- |
| G1 | 11 (6 "Fixed & verified") | 11/11 cited files present (`work-G1/audit.log`, `ctest-G1.log`, `shots-G1/10,16,22,24` all exist and show what is claimed) | 5 of 6 (6ff1a7b75, e68b3829e, d5e899dab, 3c3277079, cfc23303a) | 5/5 | none |
| G2 | 12 (11 "Fixed & verified") | 12/12 cited files present (`work-G2/discord-frames.jsonl` holds 5 SET_ACTIVITY frames, `ctest.log` 11/11, `shots-G2/04,06` exist) | 8 of 11 (614c3b888, d27cb219e, b1cc19dc9, 9a579224c, d8b5ffa2a, 89507afba, 12f5d8101, ee174c8b7/3b46db87b, de6795b19) | 8/8 | none |

Both reports' automated numbers match the cited logs (G1 ctest 19/19, G2 ctest 11/11; both baselines).

## Details

### F-G1-1 - a self-uninstalling package segfaults the importer
`work-V5/repro-selfremove-v5.sh` (fresh fixture `packages/v5-selfremove.mpackage`, whose only script body is
`uninstallPackage("v5-selfremove")`), run under `gdb -batch -ex run -ex bt`. Log `work-V5/repro-gdb2.log`:

```
main| v5-selfremove: uninstalling myself
main| v5-selfremove: uninstallPackage returned true
Thread 1 "mudlet" received signal SIGSEGV, Segmentation fault.
0x000055555893e4cb in XMLimport::importPackage (...) at ../src/XMLimport.cpp:196
#1  Host::installPackage (..., thing=enums::PackageModuleType::Package, quiet=true) at ../src/Host.cpp:2972
#2  TLuaInterpreter::installPackage (L=...) at /home/user/Mudlet/src/TLuaInterpreter.cpp:2725
```
Same line and stack as the coordinator's run. G1's 5.0.1 claim checks out: `work-G1/repro-selfremove-501.sh`
runs `cd /home/user/worktrees/v501` and its log's config line names that worktree's `src` directory, with the
crash at `XMLimport.cpp:188` -> `Host.cpp:2311`. So: crash is real, pre-dates 5.0.1, correctly tagged New bug
rather than Regression. Tracker searches ("package that uninstalls itself during install crashes Mudlet")
return only adjacent entries (#10214 reinstall-from-`sysUninstall`, #7820 silent install failures, #10629
double `closeMudlet()`), none of them this.

### F-G1-2 - Zip Slip
Contained repro: HOME was a throwaway directory under `work-V5/`, and the archive entry was rewritten to
`../` x24 + the absolute path of `work-V5/zipslip-target/` (a directory I created), so nothing outside my own
work dir could be touched. `work-V5/a4_zipslip.lua`:
```
main| V5 zipslip install -> true , nil
main| V5 zipslip registered = true | PKGS = ... mudlet-base-ui, v5-errorscript, v5-zipslip
main| V5 zipslip escaped file readable from Lua = true
main| V5 escaped content: V5 ESCAPED THE PROFILE DIRECTORY
```
`work-V5/zipslip-target/v5-zipslip-escaped.txt` exists on disk, outside
`$HOME/.config/mudlet/profiles/Mudlet self-test/`. **Yes, the file lands outside the profile, and yes the
package still registers** (`getPackages()` lists `v5-zipslip`). Cause is `utils::unzip()`
(`src/utils.cpp:114`): `QFile fd(qsl("%1%2").arg(destination, entryInArchive))` with no cleaning or
containment check, so the OS resolves the `..` segments (surplus `..` clamp at `/`, which is why an absolute
tail can be targeted exactly). Known: #10626, open; no open PR matches a path-traversal fix.

### F-G1-3 - addCommand residue after uninstall
`work-V5/a5_10758.lua` and `a5b.lua`:
```
main| V5 T: PKGS after = ... mudlet-base-ui, v5-errorscript      (v5-valid gone)
main| V5 T: removeCommand(id) after uninstall = true
main| V5 T: second removeCommand(id) = false
```
`shots-V5/03-extensions.png` (V5Pkg present while installed) vs `shots-V5/05-extensions-residue.png`
(package uninstalled a second time, `getPackages()` no longer lists it, **V5Pkg still in Options ->
Extensions**). Known: #10758, open.

### F-G1-4 / 3c3277079 - failed installs in one chunk
`work-V5/a1_packages.lua`, five calls after one successful install:
```
main| V5 INSTALL valid -> true , nil
main| V5 INSTALL notazip -> true , nil
main| V5 INSTALL empty -> true , nil
main| V5 INSTALL errorscript -> true , nil
main| V5 INSTALL missing -> true , nil
```
and the console does name each reason (3c3277079's half works): `could not unzip package`,
`no package found in ... - no Mudlet package file in it could be read`, `could not open file '...'`, each
preceded by `Host::installPackage() deferred install of ... failed:`. So the return value is the only gap,
exactly as G1 describes. Known: #10208 and #10639, both open.

### F-G1-5 - uncompilable script still installs
`work-V5/a3_errorscript_install.lua` with handlers from `a2_errorscript_events.lua`:
```
XMLimport::readScript(...) ERROR - can not compile script's lua code for "v5-errorscriptScript"; reason: ... attempt to call global 'thisFunctionDoesNotExist_V5' (a nil value)
main| V5 errorscript install -> true , nil
main| V5 EVT sysInstall: v5-errorscript
main| V5 EVT sysInstallPackage: v5-errorscript
main| V5 errorscript in getPackages() = true
```
Agreed as a Note: a package manager listening on `sysInstallPackage` cannot tell this from a healthy install.

### d5e899dab - shortcut refusals (coverage re-run)
`work-V5/a6_shortcut.lua`; every refusal names the holder:
```
V5 SC tempKey-taken  -> nil , Alt+F9 is already taken by a key binding in this profile
V5 SC permKey-taken  -> nil , Alt+F10 is already taken by the "V5Binding" key binding
V5 SC mudlet-taken   -> nil , Alt+P is already taken by "Preferences"
V5 SC command-taken  -> nil , Alt+F7 is already taken by "V5Taker"
V5 SC 5-step         -> nil , a key sequence can be 4 step(s) long at most
```
Holds.

### cfc23303a - ttsSpeechQueued (coverage re-run, both engine states)
Engine idle (`work-V5/a7_tts.lua`): each `ttsQueue` reports `index=1` and `ttsGetQueue(1)` returns that text,
because the queue drains between calls. Engine busy (`work-V5/a7b_tts.lua`, a `ttsSpeak` first):
```
V5 EVT ttsSpeechQueued text=first index=1 ttsGetQueue(index)=first
V5 EVT ttsSpeechQueued text=head  index=1 ttsGetQueue(index)=head
V5 EVT ttsSpeechQueued text=past  index=3 ttsGetQueue(index)=past
V5 TTSB queue = [1=head, 2=first, 3=past]   ttsGetQueue(0)=false
```
1-based and round-tripping in both states; the past-the-end insert is clamped to the end and the event's
index agrees with `ttsGetQueue()`. G1's row said `past -> index=2`; that is the same behaviour with one
fewer entry still queued, not a discrepancy. Row holds.

### 6ff1a7b75 / e68b3829e - per-profile controls and setCommandPinned (coverage re-run)
Two profiles ("Mudlet self-test" and a copied "V5second") in one window via two `--profile` arguments,
toolbar forced on in `Mudlet.ini`. `addCommand` ids 1 (menu+toolbar) and 2 (toolbar only) created from
self-test:
- `shots-V5/14-overflow-selftest.png`: second toolbar row holds **V5Demo** and **V5TbOnly**.
- `shots-V5/17-overflow-v5second.png` (Ctrl+Tab to V5second): the same row holds only About and Full Screen -
  both commands gone.
- `setCommandPinned(1, true)` / `setCommandPinned(2, false)` -> `V5PIN: true / true`; back in V5second,
  `shots-V5/18-pinned-in-v5second.png` shows **V5Demo** and still no V5TbOnly.
Holds exactly as claimed.

### 614c3b888 - media start position (coverage re-run)
`work-V5/b2_start3000.lua` / `b3_start20000.lua` on the 10 s `tone10.wav`:
```
main| V5M: playSoundFile start=3000 -> true nil
main| V5M: FINISHED tone10.wav after 7.009 s
main| V5M: playSoundFile start=20000 (past end) -> true nil
main| V5M: FINISHED tone10.wav after 10.005 s
```
Start position honoured; a past-the-end start plays the whole file from the beginning. Holds.

### d27cb219e - unplayable requests refused, no player leaked (coverage re-run, 30x)
`work-V5/b4_badmedia30.lua` feeds 30 rounds of three malformed `Client.Media.Play` forms (unknown type,
`"."`, no name) over `feedTelnet`, three times in a row:
```
V5M: bad x30 -> started=0 finished=0 playing=0 VmRSS 163628 -> 165568 kB (delta 1940)
V5M: bad x30 -> started=0 finished=0 playing=0 VmRSS 166888 -> 167572 kB (delta 684)
V5M: bad x30 -> started=0 finished=0 playing=0 VmRSS 167608 -> 168060 kB (delta 452)
V5M: follow-up good play -> true      V5M: after good, started=1 playing=1
```
90 refusals per round, no `sysMediaStarted`/`sysMediaFinished`, `getPlayingSounds()` = 0, and the RSS delta
shrinks round on round (console/allocator churn, not a per-request leak). A well-formed request still plays.
Holds.

### b1cc19dc9 - every sendIrc/setIrcNick injection vector (coverage re-run)
`work-V5/c2_inject.lua` against `work-V5/irc-server.py`; wire diff taken from `work-V5/ircdir/raw.log`.
Refused, each naming the offending value: message with CRLF, message with LF only, message with CR only,
target with CRLF, target with LF, empty target (`no target given, name the channel or the nick to send the
message to`), empty message (`no message given to send`), `setIrcNick` with CRLF and with LF
(`nick name "..." must be a single word, without a line break or a null character`).
Accepted: plain text, a leading `/join #evil` (sent literally), and the comma target list. The wire shows
**exactly three lines** for the whole run:
```
C> b'PRIVMSG #t :plainmessage'
C> b'PRIVMSG #t :/join #evil'
C> b'PRIVMSG #t,#t2 :commalist'
```
No QUIT, JOIN, PART or NICK. Holds.

### 9a579224c - a stray 372 *before* registration (coverage re-run; the case G2 could not drive)
Second server instance started with `--delay-registration 14`; the `372` was appended to its send file as
soon as the client's `USER` line arrived, i.e. before `001`. `work-V5/ircdir2/raw.log`:
```
C> b'NICK qa'
C> b'USER mudlet hostname servername :Mudlet 5.0.0-dev-85d814292'
S> b':srv 372 qa :stray motd line BEFORE registration\r\n'
```
Mudlet stayed up (PID still alive afterwards) and delivered the line:
`main| V5IRC: MSG nick=srv chan=127.0.0.1 msg=[MOTD] stray motd line BEFORE registration`.
Holds, and now covers the pre-registration variant as well.

### d8b5ffa2a and the two open IRC issues
Third-party kick: `:op!u@h KICK #t bystander :you were spamming` ->
`main| V5IRC: MSG nick=op chan=#t msg=! op kicked bystander`, and `shots-V5/08-irc-raised.png` shows
`! op kicked bystander` in the `#t` pane with no reason. Self-kick: `shots-V5/10-irc-after-selfkick.png` -
the `#t` buffer is gone from the list, the `srv` pane's last line is still the 001 numeric.
#10535: `shots-V5/09-irc-srv-buffer.png` shows `[INFO] <b>not bold</b> &amp; more` rendered as literal text
(not bold, entity not resolved) - **fixed on this tree, issue still open**.
#10536: the `PRIVMSG` body `Fish & Chips <here> &amp; &lt;b&gt;` reached Lua unchanged - **fixed on this
tree, issue still open**. Both can be closed by the maintainers.

### DB commits (coverage re-run)
`work-V5/b6_db.lua` and `d2_db_cov.lua`:
- 89507afba: `db:add` after close -> `nil , can not add to vh because the database is closed.`;
  `db:fetch` raises with the same shape. (Name mangled by F-G2-3, as G2 noted.)
- 12f5d8101: `"nickname" is a key, but a sheet given as a list takes its column names as list members...`
  and `[7] is not a position in this 2 item list, so it names no column...`, both fatal.
- ee174c8b7 + 3b46db87b: `db:create - people - _unique names "nosuchcol", which is not one of the sheet's
  columns: that constraint is skipped.` - the database is created, and the good half of the compound
  constraint is still attached: a duplicate `name` leaves `rows=1` with
  `LuaSQL: UNIQUE constraint failed: people.name`.
- de6795b19: `phpTable({flag=false})` -> after `t.flag = true` the key appears once, after `t.flag = nil`
  the count is 0.
- F-G2-3 / F-G2-4 evidence:
```
V5DB: v5test2 rows = one,two
V5DB: safe_name('v5test1')=vtest safe_name('v5test2')=vtest
V5DB: db files = Database_vtest.db
V5DB: unique-typo ok=true r1=table: ... r2=nil        (no message of any kind)
V5DB: v5f rows after two identical adds = 2           (constraint silently lost)
```
All rows hold.

## Process
PIDs recorded in `work-V5/pids.txt`; only those were killed. Xvfb `:83` and openbox were started by me and
left running for the next agent. Artefacts kept small (screenshots ~1 MB total, no builds run).
