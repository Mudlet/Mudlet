# C1 findings

Area: trigger, alias, timer, key and script engine. Tree under test 85d814292
(= development 12b373743). Display :83. Work dir `<scratchpad>/qa/work-C1/`,
screenshots `<scratchpad>/qa/shots-C1/`.

Launch used for everything below (throwaway HOME, test mode on so `feedTriggers`
and `feedTelnet` exist):

```bash
Xvfb :83 -screen 0 1280x800x24 & DISPLAY=:83 openbox &
HOME=$(mktemp -d) DISPLAY=:83 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 \
  DBUS_SESSION_BUS_ADDRESS=disabled: MUDLET_TEST_MODE=1 \
  ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror \
  > work-C1/mudlet.log 2>&1 &
# then: work-C1/send.sh 'lua dofile("<abs path>.lua")'   (clears the input line, types, Return)
```

`work-C1/send.sh` is the exact xdotool driver used for every step.

## Summary
Built the full trigger matrix as data (`work-C1/matrix1.lua`, `matrix2.lua`,
`work-C1/c1pkg.xml`): substring, perl regex with numbered and named groups,
begin-of-line, exact, Lua-condition, colour, line spacer, multiline/AND at line
delta 0 and 2, and match-all, fed matching and non-matching lines including
accented and CJK text before the match. Every capture, `multimatches`, `line`
and `command` value came back correct, so the nine performance/infra commits are
clean as far as behaviour goes, and the n-gram prefilter is a very large win
(3000 non-matching substring triggers add 3 ms to a 1000-line feed: 0.030 s ->
0.033 s). All the crash and correctness fixes in scope reproduce as fixed:
expandAlias nesting three deep, alias self-expansion capped at 50 with a clear
message, match-all captures past a multibyte character, `selectCaptureGroup`
twice, `replace()` on a reversed selection, `deleteLine()` then echo, stopwatch
overflow, emergency-stop resume, `permGroup(...,"key")`, `appendScript` on a
missing script.
Eleven findings: one new hang (a match-all trigger is quadratic in line length -
a 5 MB line never finished and I killed the client after 15 min), nine open
tracker issues that still reproduce on this tree, and one commit message that no
longer matches the shipped limit.
Not run here: deleting a **script** or a **button** from its own script - Lua has
no `killScript`/`killButton`, so that half of 698dd9a86 is editor-only; and
9720638ef (MXP `<DEST frame EOF>`) needs an MXP-speaking server, not reachable
through `feedTriggers`.

## Findings

| ID | Severity | Commit | Title | Status |
|----|----------|--------|-------|--------|
| F-C1-1 | Major | ec90c3893 | A nested `feedTriggers()` still wipes the calling script's `matches`/`multimatches` | Known: #10796 |
| F-C1-2 | Major | (d0a7e6485 adjacent) | A 0-second repeating `tempTimer` pins a core at 100% CPU | Known: #10824 |
| F-C1-3 | Major | (d0a7e6485 adjacent) | A repeating `tempTimer` with an empty code string spams errors forever | Known: #10795 |
| F-C1-4 | Major | (trigger matrix) | Multiline AND trigger whose script is a Lua function gets neither `matches` nor `multimatches` | Known: #10736 |
| F-C1-5 | Major | cc72e3026 / 663c937a9 | A match-all trigger is quadratic in line length; a 5 MB line freezes Mudlet indefinitely | New bug |
| F-C1-6 | Minor | 744ea965e | A key bound to Alt+E never fires; Mudlet's own shortcut wins, with no warning | Known: #10765 |
| F-C1-7 | Minor | ec90c3893 | `expandAlias(cmd, nil)` suppresses the echo that `expandAlias(cmd)` produces | Known: #10749 |
| F-C1-8 | Minor | 663c937a9 | An unset trailing capture group is absent from `matches`; an unset named group is never set | Known: #10738 |
| F-C1-9 | Minor | (trigger matrix) | `tempRegexTrigger`/`tempAlias` accept an uncompilable pattern and return a normal id | Known: #10733 |
| F-C1-10 | Minor | c460994ba | `showCaptureGroups()` raises a Lua error when the pattern has a named capture group | Known: #10737 |
| F-C1-11 | Cosmetic | 9e6ef57a8 | Commit message's stated stopwatch boundary (9e15 s accepted) is not what shipped (1e12 s) | Note |

### F-C1-1: A nested `feedTriggers()` still wipes the calling script's captures
**Steps** (`work-C1/matrix3.lua`, "#10796" block):
```lua
tempRegexTrigger([[^ftinner (\w+)$]], function() end)
tempRegexTrigger([[^ftouter (\w+) (\w+)$]], function()
  feedTriggers("ftinner nested\n")
  _G.C1ft = {matches[1], matches[2], matches[3]}
end)
feedTriggers("ftouter aa bb\n")
```
**Expected** (the protection ec90c3893 gives `expandAlias`): `m1='ftouter aa bb' m2='aa' m3='bb'`.
**Actual** (`work-C1/out3.txt`): `after nested feedTriggers: m1="nil" m2="nil" m3="nil"`.
For contrast, the same script shape with `expandAlias()` in place of
`feedTriggers()` *is* protected: `trigger after expandAlias: m1="trgexp red blue"
m2="red" m3="blue"`.
**Evidence**: `work-C1/out3.txt`; the commit itself files this as #10796.
**Commits**: ec90c3893 (fixed the `expandAlias` half only).

### F-C1-2: A 0-second repeating `tempTimer` pins a core at 100% CPU
**Steps** (`work-C1/matrix7b.lua`): `tempTimer(0, function() ... end, true)`, then
sample `/proc/<mudlet pid>/stat` fields 14+15 over 6 s.
**Expected**: a timer that cannot keep up should not spin the main thread.
**Actual**: `CPU ticks before=323 after=937 delta=614 over ~6s` — 102 ticks/s,
i.e. one full core. After `killTimer(_G.C1zeroTimer)` the same 6-second sample
gives `delta=2`. The counter had reached 7,731,318 ticks in ~50 s.
**Evidence**: shell output quoted above, reproducible with
`work-C1/matrix7b.lua` + the `/proc` sampling one-liner in this report.
**Commits**: none in scope caused it; adjacent to d0a7e6485 (timer engine).

### F-C1-3: A repeating `tempTimer` with an empty code string errors forever
**Steps**: `tempTimer(0.5, "", true)` (`work-C1/matrix7b.lua`).
**Expected**: refused at creation, or a single error.
**Actual**: a valid id (22) and then, every 500 ms for as long as it lives,
```
errors_Mudlet self-test|  object:<error in anonymous Lua function> function:<func reference not found by Lua, func cannot be called>
errors_Mudlet self-test|         <Lua error:>
```
59 such lines accumulated in `work-C1/mudlet.log` before I killed it.
**Evidence**: `work-C1/mudlet.log` lines 405-445.
**Commits**: adjacent to d0a7e6485.

### F-C1-4: Multiline AND trigger with a Lua-function script gets no captures
**Steps** (`work-C1/matrix7.lua`, "#10736" block):
```lua
tempComplexRegexTrigger("C1MultiFn", [[^mlfn a (\w+)$]], function()
    _G.M = {mm = (multimatches and #multimatches or -1), m = (matches and #matches or -1)}
  end, 1, -1, -1, 0, 0, -1, -1, 0, 0, 2)
tempComplexRegexTrigger("C1MultiFn", [[^mlfn b (\w+)$]], function() end, 1, -1, -1, 0, 0, -1, -1, 0, 0, 2)
feedTriggers("\nmlfn a one\nmlfn b two\n")
```
**Expected**: `#multimatches == 2`, `multimatches[1][2] == "one"`.
**Actual** (`work-C1/out7.txt`): `#multimatches=nil #matches=nil mm[1][2]=nil`.
The identical trigger with a **script string** instead of a function is fine -
the package-installed `C1 and delta2` in `work-C1/c1pkg.xml` returned
`mm[1]={"c1and2 a one","one"} mm[2]={"c1and2 b two","two"}` (`work-C1/out2.txt`).
So the defect is specific to the Lua-function callback path.
**Evidence**: `work-C1/out7.txt` vs `work-C1/out2.txt`.

### F-C1-5: A match-all trigger is quadratic in line length and hangs the client
**Steps** (`work-C1/mem.lua`, then `work-C1/scale.lua`):
```lua
tempComplexRegexTrigger("C1Big", [[(\w*)]], [[]], 0, -1, -1, 0, 1, -1, -1, 0, 0, 0)
feedTriggers(string.rep("word ", 1000000) .. "\n")   -- 5 MB, one line
```
**Expected**: slow, but it finishes and the UI comes back.
**Actual**: the client went to 100% CPU and never returned. I sampled it at
10-second intervals for 15 min 48 s (`ps -o etime`, CPU delta 998 ticks/10 s =
100% of a core the whole time) and then `kill -9`'d that pid. VmRSS was flat at
331 MB throughout, so it is CPU, not a leak.
Scaling measured on a fresh client (`work-C1/out11.txt`), same trigger,
one line of the given size:

| line | elapsed | VmRSS after |
|---|---|---|
| 25 kB | 0.410 s | 154 MB |
| 50 kB | 1.489 s | 158 MB |
| 100 kB | 5.781 s | 166 MB |
| 200 kB | 22.814 s | 182 MB |

Each doubling costs ~3.9x, i.e. O(n^2). Extrapolating, the 5 MB line is about
four hours. A game that emits one very long line (a 5 MB paste, a runaway `tell`)
with any `/g` trigger armed therefore freezes the client with no way out.
**Not compared against 5.0.1** - I did not build the release to bisect, so I
cannot say whether the exponent changed; the capture-parking commits in scope
(07d816f8c, cc72e3026) touch exactly this loop, which is why I am flagging it.
**cc72e3026's own claim checks out**: after the big line, 20 ordinary fires
brought VmRSS from 182 MB back to 163 MB, i.e. the parked capacity is released.
**Evidence**: `work-C1/out11.txt`, `work-C1/out10.txt`, the `ps`/`/proc` samples above.
**Commits**: cc72e3026, 07d816f8c, 663c937a9 (all in the match-all loop).

### F-C1-6: A key bound to Alt+E never fires
**Steps** (`work-C1/keys.lua`): `permKey("alt e test", "", mudlet.keymodifier.Alt,
mudlet.key.E, [[echo("C1KEY-ALTE-FIRED\n")]])` — returns a normal id (19) with no
warning. Then `DISPLAY=:83 xdotool key --clearmodifiers alt+e`.
**Expected**: either the binding fires, or creating it warns that Alt+E is taken.
**Actual**: the script editor opens (Mudlet's own Alt+E shortcut) and
`C1KEY-ALTE-FIRED` never appears in the log.
**Evidence**: `shots-C1/05-alt-e.png` (editor window open), `work-C1/mudlet.log`
has no `C1KEY-ALTE` line. Screenshots `shots-C1/09` through `14` were taken with
the editor at its default size, which clips the trigger tree to three rows -
disregard the apparently missing items there and use `shots-C1/16`/`17`.
**Commits**: 744ea965e (key bindings).

### F-C1-7: `expandAlias(cmd, nil)` suppresses the echo
**Steps** (`work-C1/matrix7b.lua`): with `tempAlias("^echotest$", function() end)`,
call `expandAlias("echotest")`, then `expandAlias("echotest", nil)`, then
`expandAlias("echotest", false)`, each preceded by a marker echo.
**Expected**: `nil` for a missing optional argument behaves as the one-argument form.
**Actual** (`work-C1/mudlet.log` lines 132-143): the line `echotest` appears only
after `C1MARK-A` (one argument). Nothing between `C1MARK-B` and `C1MARK-C`.
**Evidence**: log excerpt above.

### F-C1-8: Unset capture groups are missing from `matches`
**Steps** (`work-C1/matrix7b.lua`):
```lua
tempRegexTrigger([[^ucg (a)?(b)?(c)?$]], function() ... end) ; feedTriggers("\nucg b\n")
tempRegexTrigger([[^ucgn (?<first>a)?(?<second>b)?$]], function() ... end) ; feedTriggers("\nucgn b\n")
```
**Expected**: four numeric entries (full match + three groups), the unset ones `""`;
`matches.first == ""`.
**Actual** (`work-C1/out7b.txt`):
`#matches=3 m1=ucg b m2= m3=b m4=nil` — the mid-pattern unset group is `""` but
the trailing one is absent entirely; and `matches.first=nil matches.second=b`
— an unset **named** group is never set at all.
**Evidence**: `work-C1/out7b.txt`.

### F-C1-9: An uncompilable pattern is accepted and given a normal id
**Steps**: `tempRegexTrigger([[^(unclosed]], function() end)` and
`tempAlias([[^(unclosed]], function() end)` (`work-C1/matrix7b.lua`).
**Expected**: `nil` plus a message naming the PCRE error.
**Actual**: `tempRegexTrigger bad pattern -> 79`, `tempAlias bad pattern -> 103`.
The items are dead; nothing tells the script so.
**Evidence**: `work-C1/out7b.txt`.

### F-C1-10: `showCaptureGroups()` raises on a named capture group
**Steps** (`work-C1/matrix7.lua`): inside a trigger on `^scg (?<who>\w+)$`, call
`showCaptureGroups()` under `pcall`.
**Expected**: it prints the groups.
**Actual** (`work-C1/out7.txt`):
`showCaptureGroups: false / selectCaptureGroup: bad argument #1 type (capture group as number or capture group name as string expected, got nil!)`
**Evidence**: `work-C1/out7.txt`.

### F-C1-11: 9e6ef57a8's message states a boundary the code does not have
**Steps** (`work-C1/matrix5.lua`): `adjustStopWatch(id, 9e15)`.
**Commit says**: "The boundary is now covered at 9e15 s (must be accepted) and
9.3e15 s (must be refused)".
**Actual** (`work-C1/out5.txt`):
`adjust 9e15: ret=nil msg=modification in seconds must be a finite number from -1000000000000 to 1000000000000, got 9000000000000000`
The shipped limit is +/-1e12 s, and `src/mudlet-lua/tests/Other_spec.lua:782-805`
pins exactly that (including saturating accumulation), so the code and its specs
agree with each other and only the commit message is stale. Behaviour itself is
correct: 2147484 s stores 2147484 (not -2147483.648), NaN and +/-inf are refused.
**Status**: Note, no functional impact.

## Coverage

| Commit | Subject | Verdict | How verified |
|---|---|---|---|
| ec90c3893 | expandAlias() no longer wipes the caller's captures | Fixed & verified | `work-C1/matrix3.lua`; `out3.txt`: `after: command="outer thing" m1="outer thing" m2="thing"`, plus three-deep nesting (each level keeps its own `command`/`matches`) and the trigger-calls-expandAlias case (`m1="trgexp red blue" m2="red" m3="blue"`). The sibling defect #10796 is F-C1-1, #10749 is F-C1-7 |
| 40f64a5d5 | Crash from an alias that keeps expanding into itself | Fixed & verified | `work-C1/matrix3.lua`; the client survived, `out3.txt` `selfloop ran 49 times` (the 50th dispatch is the outer `run lua code` alias) and `mudlet.log` carries `AliasUnit::processDataStream(...) aborting: alias processing recursion reached the limit of 50` plus the user-facing "Alias processing stopped to prevent a crash: "selfloop" was expanded ... 50 times in a row" |
| 663c937a9 | match-all triggers and aliases keep captures after an accented character | Fixed & verified | `work-C1/matrix2.lua`; `out2.txt`: match-all `(\d*)` on `café 9` -> `matchall accented captured the 9: true`, on CJK `日本語 7 x` -> `true`, and the alias half (`expandAlias("café 9")` on a `(\d*)` alias) -> `true`. Unset-group gap is F-C1-8 |
| cc72e3026 | Match-all triggers no longer hold onto memory from their biggest line | Fixed & verified (memory) / Bug: F-C1-5 (time) | `work-C1/scale.lua`; `out11.txt`: VmRSS 182 MB after a 200 kB match-all line falls to 163 MB after 20 ordinary fires, i.e. the parked capacity is released. The elapsed time on a long line is F-C1-5 |
| ab41f5e11 | Substring triggers skip lines they cannot match | Verified no regression | Full matrix `matrix1.lua`/`matrix2.lua`: substring triggers matched `this has alpha inside`, `café has alpha inside`, `日本語 alpha here` and correctly did not match `this has alfa inside` |
| 788e91170 | Line colors copied only when a trigger changes them | Verified no regression | `c1pkg.xml` colour trigger (`ANSI_COLORS_F{002}_B{IGNORE}`) fired on `\27[32m...` and not on `\27[31m...` or plain text; highlight triggers in `perf.lua` recoloured 3,000,000 times without incident |
| cf1d28e53 | Color triggers check single-colored lines faster | Verified no regression | Same colour-trigger matrix rows (`out2.txt`, "colour fg2 match" / "colour fg1 NON-match" / "colour plain NON-match") |
| 83f16f8e5 | Substring triggers match faster by preparing each pattern's search once | Verified no regression | `perf.lua`: 3000 substring triggers, 1000-line feed, all matches accounted for (`hits=3000000`, exactly 3000 x 1000) |
| 07d816f8c | Trigger capture groups reach scripts faster | Verified no regression | Every matrix row checks `matches`, named keys and `multimatches` content, not just firing (`out1.txt`, `out2.txt`) |
| 7876128c3 | Faster trigger dispatch on every incoming line | Verified no regression | Whole matrix plus `perf.lua` timings |
| 70f1dab3c | Triggers match faster on every line | Verified no regression | Whole matrix plus `perf.lua` timings |
| 0c8cbba1a | Game text and color triggers are faster | Verified no regression | Whole matrix; colour rows as above |
| e46382824 | Profiles with thousands of highlight triggers no longer stall | Fixed & verified | `work-C1/perf.lua`; `out9.txt`: 1000-line feed takes 0.030 s with no extra triggers and 0.033 s with 3000 non-matching root substring triggers (the prefilter files them out); with 3000 *matching* ones it is 8.557 s and fires exactly 3,000,000 times, so nothing is being filtered away wrongly |
| 9e6ef57a8 | Large stopwatch adjustments no longer flip negative | Fixed & verified | `work-C1/matrix5.lua`; `out5.txt`: `adjust 2147484: ret=true time=2147484`, `+/-1e9 s` stored exactly, NaN / +inf / -inf each refused with the limit message. Stale message text is F-C1-12 |
| d0a7e6485 | Timers keep running after the emergency stop is switched off | Fixed & verified | `work-C1/timers2.lua` writes a timestamped line per tick to `work-C1/ticks.txt` from both a **function** timer and a **script-string** timer; toolbar bomb at (985,705). `ticks.txt` shows ticks up to epoch 1789828782, none between 1789828782 and 1789828788 (stop held), then both resume from 1789828788. `shots-C1/03-emergency-stop.png` shows the armed bomb. Also checked: a temp timer killed before the stop was not resurrected (`killedFired=0`), a pending one-shot fired after resume (`oneShot=true`), a self-killing timer stayed dead (`selfKillRan=1`) - `work-C1/out6.txt`. Adjacent open issues #10794/#10795/#10559: #10795 reproduces (F-C1-3) |
| 744ea965e | Key bindings inside a permGroup("…","key") group now work | Fixed & verified | `work-C1/keys.lua` + `xdotool key F5/F6`; `out8.txt`: `group isActive: 2`, `child isAncestorsActive: true`, and `mudlet.log` has `C1KEY-F5-FIRED` and `C1KEY-F6-FIRED` (the F6 one is two group levels deep). `out8.txt` also pins creation state per type: trigger 1, alias 1, timer 0, script 0, key active. #10765 is F-C1-6 |
| 698dd9a86 | UB creating or deleting any trigger/alias/timer/key/script/button | Fixed & verified (4 of 6 types) | `work-C1/matrix5.lua` + `work-C1/scale.lua`: a trigger killing itself from its own script returns true and does not fire again; same for an alias; a `tempTimer` killing itself ran exactly once (`selfKillRan=1`, `out6.txt`); a `tempKey` killing itself logged `C1TEMPKEY-SELFKILL:true` on the first F11 and nothing on the second (`mudlet2.log`). **Scripts and buttons could not be tested**: Lua registers no `killScript`/`killButton` (`out5.txt` records both as nil), so that path is editor-only. Note: `killKey`/`killTrigger` only remove *temporary* items by design (`KeyUnit::killKey` skips `!isTemporary()`), so `killKey` on a `permKey` correctly returns false |
| c460994ba | Selecting a capture group no longer breaks the next selection | Fixed & verified | `work-C1/matrix4.lua`; `out4.txt`: on `Hello World Again` with `^(Hello) (World) (Again)$`, group 3 selects `World`, then group 1 selects the whole `Hello World Again` and group 2 selects `Hello` — the pre-fix symptom was group 1 coming back truncated. Second case without a `deselect()` between: group 2 `Alpha Beta`, then group 1 `Alpha Beta Gamma`. Out-of-range returns -1 and leaves the selection alone. #10737 is F-C1-10 |
| aab5e3806 | replace() no longer crashes on a backwards selection | Fixed & verified | `work-C1/matrix3.lua`: `selectSection(5,-3)` returns **false** (the negative length is refused at the source) and the following `replace("XX")` returns nothing and does not crash; the client stayed up for every later test in the same session |
| 4281e05d8 | Writing to a console emptied by deleteLine() no longer crashes | Fixed & verified | `work-C1/matrix5.lua`; `out5.txt` `miniconsole deleteLine+echo ok=true` after driving `echo`, `cecho`, `decho`, `hecho`, `insertText` and `echoLink` at an emptied miniconsole, and `main deleteLine+echo+send ok=true` for the gagging-trigger path (deleteLine, then echo, then send from the trigger). The written text is in `mudlet.log` (`C1mc| after delete`, `colour after delete`, …) and visible in `shots-C1/03-emergency-stop.png` |
| 9720638ef | A frame emptied by the game no longer keeps a selection | Could not test (needs an MXP-speaking server) | The repro needs `<FRAME>`/`<DEST f EOF>` from the game side; `feedTriggers` does not carry MXP and no telnet fixture that speaks MXP was available in the time box. Claimed, not evidenced |
| ea3d43a9b | appendScript() says the script is missing | Fixed & verified | `work-C1/matrix3.lua`/`matrix5.lua`; `out3.txt`: `appendScript ok=false err=appendScript: cannot append to script (script "no such script" at position 1 not found)` — no `-1` and no bogus syntax-error. Success path still works: `out5.txt` shows `getScript` returning `x = 1\ny = 2` after an append |
| 5c1c3b907 | Infra: triggers, timers and actions reach the console through Host | Verified no regression | Every trigger/timer/alias in the matrix reached the console: `echo`/`cecho`/`decho`/`hecho` from trigger scripts, `send()` from a trigger after `deleteLine()`, timer output, and the alias error banner all appeared in `mudlet.log` |
| 1cc778443 | Infra: match colour triggers without a window | Verified no regression | The `C1 colour` package trigger (`ANSI_COLORS_F{002}_B{IGNORE}`) matched `\27[32m` text and rejected `\27[31m` and uncoloured text (`out2.txt`) |
| 72c223f65 | Infra: colour changes reach the buffer without a window | Verified no regression | Highlight triggers recoloured live text (visible red/`C1mc` lines in `shots-C1/03-emergency-stop.png`); `perf.lua`'s 3000 highlight triggers ran 3,000,000 recolour passes without error |
| (area check) | Trigger editor icons after enableTrigger/disableTrigger with the editor open | Verified no regression | `permSubstringTrigger("C1IconTest","",{"iconline"},…)` at the root, editor open (Alt+E) and enlarged to 1200x760 so the tree is not clipped. `shots-C1/16-editor-large2.png` shows the item with an **unchecked** box moments after a timer ran `disableTrigger("C1IconTest")`; `shots-C1/17-editor-reenabled.png` shows it **checked** again after a timer ran `enableTrigger("C1IconTest")`. The editor follows Lua state changes live. (The smaller default editor window clips the tree to three rows, which is why `shots-C1/12`/`14` appear to be missing the item - not a defect) |
| 93e4c582d | Infra: catch memory leaks from Lua argument errors in CI | Verified no regression | Exercised the argument-error paths by hand (`tempComplexRegexTrigger` bad args, `isAncestorsActive` given a string, `appendScript` on a missing name, `adjustStopWatch` with NaN/inf, `tempColorTrigger`-style refusals via `LuaApiContracts_spec`); each raised or refused cleanly and the session continued. CI-only leak detection itself is not observable from here |

## Automated
ctest (`-R 'CaptureGroupParking|BigramFilter|CorruptTriggerPatterns|EmergencyStopResume|MultilineTriggerReentrancy|SetScriptCallback|TFeedTriggersRecursion|UnitDeferredDelete|UnitProcessingDepth|UnitsReachHost|ScriptEventHandlerLifetime'`):
**11 passed / 0 failed out of 11** (`work-C1/ctest-C1.log`). Matches baseline.

Lua specs (`TESTS_DIRECTORY=work-C1/specs .claude/scripts/run-lua-tests.sh`, with
`Trigger`, `Alias`, `KeyBinds`, `EnableDisableByName`, `Regex`,
`ReversedSelection`, `EmptyBufferOps`, `InsertTextNewline`, `LuaApiContracts`):
**352 successes / 0 failures / 0 errors / 2 pending** (`work-C1/specs2.log`).
Matches baseline. The two pending ones are declared in the specs themselves:
`Trigger_spec.lua:2038` (expiry accounted after `execute()`, so a self-refeeding
trigger overshoots `expireAfter`) and `Trigger_spec.lua:2245` (Mudlet issue
#10403: a multiline trigger with an expiry count loses its named captures).
Note for anyone re-running the subset: symlink `src/mudlet-lua/tests/fixtures`
into the subset directory as well, or the package-fixture specs error out
(`work-C1/specs.log` shows that failure mode).
