# G2 findings

Area: Media, MSP, IRC, Discord, database, MMCP. Tree: 85d814292 (= development 12b373743).
Display :83. Work dir `<scratchpad>/qa/work-G2/`, screenshots `<scratchpad>/qa/shots-G2/`.

## Summary
All 12 commits in scope behave as their messages claim, and every one was exercised
live rather than only through the suites: the media `start` position, the refusal of
an unplayable/untyped `Client.Media.Play`, MSP `Off` actually stopping a sound, all
six `sendIrc`/`setIrcNick` injection vectors, the stray `372`, kicks, markup escaping
and entity round-trip, Discord `SET_ACTIVITY` frames reaching the IPC fixture, and
every DB case named in the recipe. ctest 11/11 and a busted subset (Media, MSP, DB,
Discord, Networking, Other, MudletBusted) 998/0/0/29 pending, both matching baseline.
What broke: nothing new and nothing that crashes. Six findings, all Minor or below -
two are incomplete halves of d8b5ffa2a's own motivation (the kick *reason* is never
shown, and a self-kick takes the channel tab away before it can be read), and the
rest are live pre-existing bugs found while working the area (#10727 mixes two
databases' rows, #9818 swallows a `_unique` typo, #5492's developer warnings appear
on ordinary server numerics).
Could not run here: the classic MSP wire form `!!SOUND(...)`/`!!SOUND(Off)` -
`receiveMSP()` refuses with "MSP is not currently enabled" in an offline profile and
MSP only turns on after IAC negotiation, so the same `stopMedia()` choke point was
driven through the MXP `<SOUND>` path instead (which the commit says is the other
entry to it) and `TelnetAtcpMspTest` covers the negotiated side. MMCP had no commit
in scope and was exercised only by the spec run.

## Findings

| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-G2-1 | Minor | d8b5ffa2a | IRC kick messages never show the reason for the kick | Fix incomplete (Closed: #10534) |
| F-G2-2 | Minor | d8b5ffa2a | Being kicked yourself removes the channel tab, so the kick line is never readable | Fix incomplete (Closed: #10534) |
| F-G2-3 | Major | 89507afba 12f5d8101 ee174c8b7 3b46db87b | `db:safe_name()` strips digits, so `qatest1` and `qatest2` share one file and one fetch returns the other's rows | New bug (Known: #10727) |
| F-G2-4 | Minor | ee174c8b7 3b46db87b | A typo'd `_unique` (`_uniqe`) is accepted in silence and the constraint is lost | New bug (Known: #9818) |
| F-G2-5 | Minor | 614c3b888 d27cb219e | 60 rapid `playSoundFile` calls on an undecodable file raise only 8 `sysMediaFinished` | Note |
| F-G2-6 | Cosmetic | d8b5ffa2a 9a579224c | Ordinary IRC server numerics print "this needs fixing by Mudlet Makers" | Note (Known: #5492) |

Also worth flagging to the maintainers: **#10535 and #10536 are still OPEN but are
fixed on this tree** by d8b5ffa2a - evidence in the coverage table below. They can be
closed.

### F-G2-1: IRC kick messages never show the reason for the kick

d8b5ffa2a's stated motivation is "A player who was kicked kept looking at a channel
they were no longer in, with nothing to say why". The kick is now *shown*, but the
reason the kicker gave is dropped, so the "why" is still missing.

**Steps** (`work-G2/repro-kick-reason.sh` runs the whole thing; server is
`work-G2/irc-server.py`):
1. `python3 work-G2/irc-server.py --dir work-G2/ircdir` and read its port.
2. Launch `./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
   with `HOME=$(mktemp -d) DISPLAY=:83 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled: MUDLET_TEST_MODE=1`,
   dismiss the interface tour, then in the input line:
   `lua setIrcServer("127.0.0.1", PORT); setIrcChannels({"#t"}); setIrcNick("qa"); sendIrc("#t","open")`
   with a `sysIrcMessage` handler printing nick/channel/message.
3. `printf ':bystander!u@h JOIN #t\n:op!u@h KICK #t bystander :you were spamming\n' >> work-G2/ircdir/send.txt`

**Expected:** the channel window and `sysIrcMessage` carry the reason, e.g.
`! op kicked bystander: you were spamming`.

**Actual:** both show `! op kicked bystander`. The reason is discarded.

**Evidence:** `shots-G2/06-irc-thirdparty-kick.png` - the `#t` pane shows
`[15:48:11] ! op kicked bystander` with the reason "third party out" absent. Log line
from the first session (reason was "get out"):
`main| G2: IRCMSG nick=op chan=#t msg=! op kicked qa`
(`work-G2/all-observations.txt`). Cause, `src/ircmessageformatter.cpp:179-183`:

```cpp
QString IrcMessageFormatter::formatKickMessage(IrcKickMessage* message, bool isForLua)
{
    Q_UNUSED(isForLua)
    return QObject::tr("! %1 kicked %2").arg(message->nick(), message->user());
}
```

`message->reason()` is never read. Every other composed formatter in the file that has
a reason (part, quit) does use it, so this is an omission rather than a style choice.

**Commits:** d8b5ffa2a (which made `formatKickMessage()` reachable for the first time -
before it, nothing was shown at all, so this is not a regression, it is the other half
of the same fix).

### F-G2-2: Being kicked yourself removes the channel tab, so the kick line is never readable

**Steps:** as F-G2-1, but kick the client's own nick:
`printf ':op!u@h KICK #t qa :get out\n' >> work-G2/ircdir/send.txt`

**Expected:** the user can read in the IRC window that they were kicked from `#t`.

**Actual:** `sysIrcMessage` fires with `! op kicked qa` (so scripts are served), but
libcommuni's `IrcBufferModel` drops the `#t` buffer on a self-kick and `dlgIRC` removes
its entry from the buffer list, so the formatted line has nowhere to be read. The
channel simply vanishes from the list with no trace in the remaining `srv` tab.

**Evidence:** `shots-G2/03-irc-window.png` (buffer list holds `srv` and `#t` before the
kick) vs `shots-G2/04-irc-kick-markup.png` (immediately after the kick: only `srv`, and
the `srv` pane's last line is the `[INFO]` numeric, no kick line). The Lua event in the
same window: `G2: IRCMSG nick=op chan=#t msg=! op kicked qa`. A third-party kick is
unaffected and does show in the channel (`shots-G2/06-irc-thirdparty-kick.png`), which
separates this from F-G2-1.

**Commits:** d8b5ffa2a. Arguable as a design point (the channel really is gone), hence
Minor - but it leaves the exact user in the commit's motivation with nothing on screen.

### F-G2-3: `db:safe_name()` strips digits, so two databases share one file

Live on this tree, and it silently mixes rows between two databases. Found while
exercising the four DB commits, all of which report through database names that come
out of `db:safe_name()` - the error text for a closed `g2h` reads "can not add to
**gh**", which is how this surfaced.

**Steps** (`work-G2/db3.lua`, run with `lua dofile("<abs path>")`):

```lua
local d1 = db:create("qatest1", {people = {name = ""}}); db:add(d1.people, {name = "one"})
local d2 = db:create("qatest2", {people = {name = ""}}); db:add(d2.people, {name = "two"})
print(#db:fetch(db:get_database("qatest2").people))   -- rows in "qatest2"
print(db:safe_name("qatest1"), db:safe_name("qatest2"))
```

**Expected:** `qatest2` holds one row, `two`. Two files on disk.

**Actual:**
```
main| G2DB: qatest2 rows = one,two
main| G2DB: safe_name('qatest1') = qatest safe_name('qatest2') = qatest
main| G2DB: db files = Database_ga.db Database_gb.db Database_gc.db Database_gf.db Database_gg.db Database_gh.db Database_qatest.db
```
One file, both databases' rows in it.

**Cause:** `src/mudlet-lua/lua/DB.lua:199` - `name = name:gsub("[^%ad]", "")`. The Lua
character class is "not a letter and not the literal character `d`", where
`[^%a%d]` (letters and digits) was meant. The doc comment three lines above says
"names are restricted to being alphanumeric".

**Not a regression:** `git show Mudlet-5.0.1:src/mudlet-lua/lua/DB.lua` carries the
identical line at 199; it dates to 8dda2da49 (#7306). Recorded here because the area is
mine, it is live, and it is Major in effect (one database reading another's rows).

**Status:** Known: #10727 ("db:safe_name() strips digits, so databases whose names
differ only by a number share one file"), still OPEN.

**Evidence:** `work-G2/db3.lua`, log lines above in `work-G2/all-observations.txt`.

### F-G2-4: A typo'd `_unique` is accepted in silence

**Steps** (`work-G2/db1.lua`, last two cases):
```lua
db:create("g2f", {people = {name = "", _uniqe = {"name"}}})   -- typo
db:create("g2g", {people = {name = "", unique = {"name"}}})   -- missing underscore
```

**Expected:** something says the option was not understood - ee174c8b7 and 3b46db87b
went to real lengths to report an *unresolvable column* inside `_unique`, so an
unrecognised option name deserves the same.

**Actual:** both return a handle with no message at all.
`main| G2DB: unique-typo | ok=true | r1=table: 0x55c8e209d2f0 | r2=nil`
`main| G2DB: unique-typo2 | ok=true | r1=table: 0x55c8e20d06a0 | r2=nil`
`src/mudlet-lua/lua/DB.lua:541-543` takes *any* key starting with `_` as a sheet option
without checking the name against the known set; a key without the underscore becomes a
column instead.

**Status:** Known: #9818, still OPEN. Not a regression (same in 5.0.1); named here
because the recipe asks for it and because it is the failure mode the two `_unique`
commits in scope are trying to make loud.

### F-G2-5: 60 rapid plays of an undecodable file raise only 8 `sysMediaFinished`

**Steps** (`work-G2/media4.lua` then `work-G2/media5.lua`): write a text file as
`<profile>/media/notmedia.wav`, then in one Lua turn call
`playSoundFile{name = "notmedia.wav"}` 60 times with a `sysMediaFinished` counter.

**Expected:** each accepted request ends, one way or another, exactly once -
`TMedia.cpp:1364-1394` exists specifically so "a script waiting on sysMediaFinished to
start the next one" does not wait forever.

**Actual:** 60 requests, all returning `true`, produced **8** `sysMediaFinished` events.
```
main| G2: notmedia 60x, first result: true/nil
main| G2: sysMediaFinished count after notmedia burst = 8
main| G2: getPlayingSounds = 0
```
Single requests are fine - one play of the same file raises exactly one event
(`work-G2/media6.lua`: `G2: FIN notmedia.wav`, `G2: single-play finished count = 1`),
so this is specific to re-claiming a player inside one turn (the `claimGeneration()`
guard in the deferred handler).

**No leak:** `VmRSS` 167252 kB before, 167396 kB after; the player list peaked at 25 and
`getPlayingSounds()` returned 0 afterwards, so the 60 requests neither leaked a player
nor left the media subsystem wedged - the next `playSoundFile` worked normally.

**Not a regression:** the `claimGeneration` machinery and this whole error path are
byte-for-byte present in `Mudlet-5.0.1:src/TMedia.cpp` (6 occurrences of
`claimGeneration`, same deferred-release comment). Recorded as a Note.

### F-G2-6: Ordinary IRC server numerics print "this needs fixing by Mudlet Makers"

Connecting to any IRC server writes several of these to the terminal / debug console:
```
dlgIRC::getMessageTarget(..., "127.0.0.1") WARNING - message of type: Numeric not explicitly handled, this needs fixing by Mudlet Makers...
dlgIRC::getMessageTarget(..., "#t") WARNING - message of type: Names not explicitly handled, this needs fixing by Mudlet Makers...
dlgIRC::getMessageTarget(..., "#t") WARNING - message of type: Join not explicitly handled, this needs fixing by Mudlet Makers...
dlgIRC::getMessageTarget(..., "#t") WARNING - message of type: Kick not explicitly handled, this needs fixing by Mudlet Makers...
```
They appear for `001`-`004`, `353`/`366`, JOIN and KICK - i.e. for a completely normal
registration. Alongside them, libcommuni prints
`IrcMessage::setEncoding(): unsupported encoding "ISO-8859-15"` once per message.

**Evidence:** `work-G2/mudlet2.log`, the block just before the `gmcp1.lua` line.
**Status:** Known: #5492 ("'needs fixing by Mudlet Makers' in IRC messages"), OPEN.
Cosmetic, and not a regression - but note d8b5ffa2a made KICK reach the formatter, so
"Kick not explicitly handled" is a *new* member of this set on this tree.

## Coverage

| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| 614c3b888 | media `start` position now starts the track there | Fixed & verified | `work-G2/media2.lua`: `playSoundFile{name="tone10.wav", start=3000}` on a generated 10 s WAV finished in **7.010 s** (`main\| G2: FINISHED tone10.wav after 7.010 s`). `work-G2/media3.lua`: `start=20000` (past the end) played from the beginning and finished in **10.005 s**, reusing the existing player ("Found existing player at index: 0"), i.e. the out-of-range guard both fires and does not strand the source. GMCP form with `start=8000` finished ~2 s after starting (`work-G2/gmcp1.lua`). |
| d27cb219e | a media request Mudlet cannot play is refused | Fixed & verified | `work-G2/gmcp1.lua` over `feedTelnet("<T_IAC><T_SB><O_GMCP>…")`: `Client.Media.Play {"name":"nosuchfile.wav","type":"banana"}`, `{"name":"."}`, `{"name":"./"}` and `{"key":"x"}` produced **0 `sysMediaStarted` and 0 `sysMediaFinished`** (`main\| G2M: after-bad started=0 finished=0`); the following well-formed request played normally, so the guard is not over-broad. |
| 959c6ee7f | MSP's Off request stops the sound it is playing | Fixed & verified | `work-G2/msp1.lua` / `msp2.lua`: `setConfig("specialForceMXPProcessorOn", true)`, `feedTriggers('<SOUND FName="tone10.wav">\n')` → `G2: MSP STARTED tone10.wav`; 1.5 s later `feedTriggers('<SOUND FName="Off">\n')` → `G2: after Off: started=1 finished=1`, i.e. the 10 s track stopped at ~1.5 s. Classic `!!SOUND(Off)` wire form not reachable offline (see Summary); `TelnetAtcpMspTest` passed. |
| b1cc19dc9 | game text can no longer make Mudlet send IRC commands | Fixed & verified | `work-G2/irc2.lua` against `work-G2/irc-server.py`, server raw log `work-G2/ircdir/raw.log`. Refused with nil + a message naming the value: `sendIrc("#t","a\r\nQUIT")`, `sendIrc("#t\r\nJOIN #x","hi")`, `sendIrc("#t","b\nPART #t")`, `sendIrc("","x")`, `sendIrc("#t","")`, `setIrcNick("bob\r\nQUIT")`. Accepted and sent as exactly one PRIVMSG each: plain text, `/join #evil` (literal, not a verb), `#t,#t2` comma list, and mIRC colour codes. The server received **only** `PRIVMSG` lines - no QUIT, JOIN, PART or NICK ever appeared on the wire. |
| 9a579224c | crash on a stray message-of-the-day line | Fixed & verified | With the MOTD block closed (376 already sent), `:srv 372 qa :stray motd line` was appended to the server's send file. Mudlet did not crash (`kill -0` on the PID: alive) and the line was **delivered**: `main\| G2: IRCMSG nick=srv chan=srv msg=[MOTD] stray motd line`, visible in `shots-G2/04-irc-kick-markup.png`. The pre-registration variant was not driven live (would need a registration-delaying server); `IrcMessageComposerTest` passed. |
| d8b5ffa2a | kicks shown, no markup injection, text as sent | Fixed & verified, plus F-G2-1 / F-G2-2 / F-G2-6 | Kick reaches the window and Lua (`shots-G2/06-irc-thirdparty-kick.png`, `! op kicked bystander`) - **#10534's symptom is gone** though the reason is still dropped (F-G2-1). `:srv 001 qa :<b>not bold</b> &amp; more` renders escaped in the IRC window, not as bold (`shots-G2/04-irc-kick-markup.png` shows the literal `<b>not bold</b> &amp; more`) - **#10535 is fixed but still open**. `:other!u@h PRIVMSG #t :Fish & Chips <here>` reached Lua as `Fish & Chips <here>`, not `Fish &amp; Chips &lt;here>` - **#10536 is fixed but still open**. |
| 89507afba | clearer errors when using a database after closing it | Fixed & verified | `work-G2/db2.lua` after `db:close`: `db:add` → `nil` + "can not add to gh because the database is closed."; `db:fetch`, `db:delete` and `db:merge_unique` each **raise** with the same shape ("can not fetch from / delete from / merge into … because the database is closed."), so the loud sites stayed loud and only `db:add` returns nil. `db:create` on the same name afterwards reopened it and `db:add` then returned true. (The name in every message is mangled by F-G2-3.) |
| 12f5d8101 | `db:create` says how to fix a list-form sheet | Fixed & verified | `db:create("g2d", {books = {"title","author", nickname = ""}})` → `"nickname" is a key, but a sheet given as a list takes its column names as list members. Write {"title", "author", "nickname"}, or use the {title = "", author = "", nickname = ""} form to give a column a default.` `db:create("g2e", {books = {[1]="title",[2]="author",[7]="oops"}})` → `[7] is not a position in this 2 item list, so it names no column. A sheet given as a list runs from 1 with no gaps.` Both still fatal (pcall false), as the commit intends. |
| ee174c8b7 | databases still created when a unique names a missing column | Fixed & verified | `_unique = {{"name","nosuchcol"}}` (compound) and `_unique = {"nosuchcol"}` (single) both returned a usable handle with the sheet present, printed `db:create - people - _unique names "nosuchcol", which is not one of the sheet's columns: that constraint is skipped.`, and a subsequent `db:add` succeeded - no `LuaSQL: no such table`. |
| 3b46db87b | a skipped `_unique` no longer freezes other constraints | Fixed & verified | `db:create("g2c", {people = {name="", city="", _unique = {"nosuchcol","name"}}})`: the good half of the entry **was** attached - adding `{name="dup"}` twice left `rows=1` and raised `LuaSQL: UNIQUE constraint failed: people.name`. That is the exact regression #10170 describes, and it does not reproduce. |
| de6795b19 | `phpTable` entries holding false can be updated and deleted | Fixed & verified | `work-G2/db2.lua`: `t = phpTable({flag=false}); t.flag = true` then `t:pairs()` yields the key **once** (`after-set=1`), and `t.flag = nil` clears it (`after-del=0`). Assigning `nil` to a key that never existed registers nothing (`phpTable-nil-new → count=0`). |
| 3de265ac8 | Discord integration reachable without the main window | Verified no regression | `CI/discord-ipc-fixture.py` run with a short `--runtime-dir` exactly as `run-lua-tests.sh` does; Mudlet launched with that `XDG_RUNTIME_DIR` and `LD_LIBRARY_PATH=3rdparty/discord/rpc/lib`. `work-G2/discord-frames.jsonl` shows the handshake, the three SUBSCRIBEs and four `SET_ACTIVITY` frames; `setDiscordGame("QA Test Game")`, `setDiscordState("QA state line")` and `setDiscordDetail("QA detail")` each produced a frame carrying the value, e.g. `{"cmd":"SET_ACTIVITY","args":{"activity":{"assets":{"large_image":"qa test game"},"details":"QA detail","state":"QA state line","instance":true},"pid":…}}`. `DiscordTest`, `TDiscordModeTest` and `DetachedWindowDiscordButtonTest` all passed. |

Adjacent open issues named in the brief that were **not** re-verified for lack of time,
and so are neither confirmed nor cleared here: #10808, #10404, #10043, #10194 (media),
#10798 (IRC stray 366/376 truncation), #10724, #10723 (Discord), #10729, #10728,
#10059 (DB). #10727 and #9818 were re-verified and are live (F-G2-3, F-G2-4).

## Automated

ctest `-R 'TMedia|VideoOutputHide|TtsInterrupting|Irc|Discord|TelnetAtcpMsp'`:
**11 passed / 0 failed** (DiscordTest, TMediaPathTraversalTest, IrcMessageFormatterTest,
IrcMessageComposerTest, DetachedWindowDiscordButtonTest, TelnetAtcpMspTest,
IrcMessageGuardTest, TDiscordModeTest, VideoOutputHideTest, TMediaLoopTest,
TtsInterruptingSpeakTest). Log: `work-G2/ctest.log`. Matches baseline (220/220).

busted subset (`TESTS_DIRECTORY` holding Media_spec, MSP_spec, DB_spec, Discord_spec,
Networking_spec, Other_spec, MudletBusted_spec, run through
`.claude/scripts/run-lua-tests.sh` so `MUDLET_TEST_REQUIRE_MEDIA=1` and the fixtures
are in place): **998 successes / 0 failures / 0 errors / 29 pending**, 90.6 s.
Log: `work-G2/busted.log`. No failures, consistent with the 4702/0/0 baseline.
