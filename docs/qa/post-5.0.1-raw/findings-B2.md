# B2 findings

## Summary
Covered all 13 commits in scope on 85d814292: MXP mode-switch auto-detection, `<FRAME>`/`<DEST>`
redirect and the EOF clear-with-selection crash, MSDP shapes/control codes/split reads/subscribe-on-connect,
NEW-ENVIRON off-and-on mid-session, telnet channel 102 wire format and IAC escaping, `getNetworkLatency()`
under a blocked event loop, and the starter UI's MSDP seeding. Evidence comes from a real socket (a scripted
Python telnet server in `work-B2/server.py` / `server2.py` that logs the exact bytes Mudlet sends) plus
`feedTelnet()` for the offline cases. Every commit's own test case passed.
One real bug found, and it is not from this range: an MSDP **array of tables** (the MSDP specification's own
`GROUP` example) and an array of arrays produce invalid JSON and the whole variable is dropped. Present in
5.0.1 by source inspection, so a long-standing bug rather than a regression.
Not run here: TLS (no commit in scope touches it and the fixture server is plaintext), MSP sound playback
(needs media files), ATCP beyond the handshake.

## Findings
| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-B2-1 | Major | (adjacent to 137e0d14d / 048a85b71) | MSDP array of tables / array of arrays is dropped: no separator between the elements makes invalid JSON | New bug (also in 5.0.1) |
| F-B2-2 | Minor | f49c8c133 | MXP mode-switch escape split across two socket reads is not detected | Note, Known: #10658 |
| F-B2-3 | - | (adjacent, #10362) | GMCP bare-number payloads arrive as a number here, not a function | Note |

### F-B2-1: MSDP array of tables / array of arrays is dropped, the variable never reaches Lua
**Steps** (`work-B2/repro-F-B2-1.lua`, also `work-B2/t1-msdp.lua` item 6 and `work-B2/t2-msdp2.lua` items A and C):

```
HOME=$(mktemp -d) MUDLET_TEST_MODE=1 DISPLAY=:83 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 \
DBUS_SESSION_BUS_ADDRESS=disabled: ./build-linux-debug-nosan/src/mudlet \
  --profile "Mudlet self-test" --offline --mirror
# dismiss the tour, click the input line at (440,704), then:
lua dofile("<scratchpad>/qa/work-B2/repro-F-B2-1.lua")
```

The script feeds the MSDP specification's `GROUP` example - `MSDP_VAR GROUP MSDP_VAL ARRAY_OPEN
TABLE_OPEN …TABLE_CLOSE TABLE_OPEN …TABLE_CLOSE ARRAY_CLOSE` - and an array holding two arrays.

**Expected** `msdp.GROUP` is a two-element table of tables (`msdp.GROUP[1].NAME == "Fred"`), the way
`msdp.ONEG[1].NAME` works for a single-element array.

**Actual** both variables are `nil` and the profile's error console shows a decoder failure. `msdp2Lua()`
emits `{` for `MSDP_TABLE_OPEN` and `[` for `MSDP_ARRAY_OPEN` without ever writing the `,` that JSON needs
between two sibling elements, so the JSON it hands yajl is `[{"NAME":"Fred",…}{"NAME":"Barney",…}]`:

```
errors_Mudlet self-test|  object:<JSON decoder error:> function:<json_to_value>
errors_Mudlet self-test|  <could not decode msdp.GROUP - any previous value is kept: InvalidJSONInput:
                          parse error: after array element, I expect ',' or ']'
                          {"NAME":"Fred","HEALTH":"90"}{"NAME":"Barney","HEALTH":"50"}
                       (right here) ------^ at lua_yajl.c line 345>
```

and for the array of arrays, `[["a"]["b"]]` with the same error. Both `MSDP_VAR` and `MSDP_VAL` do write a
separator when the previous marker was a `TABLE_CLOSE`/`ARRAY_CLOSE` (`src/TLuaInterpreter.cpp:4157` and
`:4217`); the two structure-open cases (`:4118` and `:4135`) do not.

**Evidence** `work-B2/mudlet.log` lines 65-103 (T1 item 6, msdp.ROOMS) and the `T2| A.` / `T2| C.` lines;
error-console text quoted above. Not a regression: `git show Mudlet-5.0.1:src/TLuaInterpreter.cpp` has the
same two cases with no separator, so 5.0.1 builds the same invalid JSON.

**Cross-check** #10189 and #10136 are both closed and neither covers this shape; a semantic issue search
("MSDP array of tables fails to decode, invalid JSON missing comma") returns nothing open. No open PR
matches. Not previously filed as far as I can see.

**Commits** none in scope introduced it, but it sits squarely in what 137e0d14d ("MSDP tables keep their
shape") set out to fix, so it is worth filing against the same area.

### F-B2-2: MXP mode-switch escape split across two socket reads is not detected
`work-B2/t10-mxpdetect.lua` case B: `feedTelnet("some text \27")` followed by `feedTelnet("[1z<send>look</send>\r\n")`
leaves `getConfig("promptForMXPProcessorOn")` false, where the same bytes in one read turn it true.
f49c8c133's single-pass scan keeps the old per-read behaviour exactly (5.0.1's eight `find()` calls could
not span reads either), so this is not a regression - it is the already-filed **#10658**, confirmed live on
this tree. A read ending in a bare `ESC`, in `ESC[`, or in `ESC[1` is handled safely (no crash, no
mis-detection): `work-B2/mudlet.log` `TA| B1/B2/C1/C2/D`.

### F-B2-3: GMCP bare-number payloads (open issue #10362) do not reproduce here
`feedTelnet("<T_IAC><T_SB><O_GMCP>Core.Ping 20<T_IAC><T_SE>")` gives `type(gmcp.Core.Ping) == "number"`,
value 20; `Spec.Str "hi"` gives a string and `Spec.Bool true` a boolean (`work-B2/mudlet3.log` `TG|`,
`work-B2/mudlet.log` `T2| H./H2./H3.`). #10362's "arrives as a function" is lua-yajl-version dependent, the
same way 137e0d14d found for MSDP item 3; on this build it does not happen. Recorded so the issue can be
retested on the version that shows it rather than closed blind.

## Coverage
| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| f49c8c133 | Scan each read once for the MXP mode switch escape | Fixed & verified | `work-B2/t10-mxpdetect.lua`: `ESC[0z`..`ESC[7z` each turn `promptForMXPProcessorOn`/`specialForceMXPProcessorOn` true, `ESC[8z` does not, and a switch buried in a 200 KB read is still found (`mudlet.log` `TA| A.` .. `TA| E8.`). Truncated escapes at a read boundary neither crash nor mis-fire. Split-read limitation = F-B2-2. |
| 14e63e080 | Turning off NEW-ENVIRON mid-session stops sending data | Fixed & verified | Real socket (`work-B2/srv1/recv.log`). `IAC SB 39 SEND USERVAR CLIENT_NAME …` is answered `IS USERVAR CLIENT_NAME VAL MUDLET …`; after `setConfig("enableNEWENVIRON", false)` the identical SEND draws **no** reply at all; after re-enabling on the same connection it is answered again. `IPADDRESS` comes back as USERVAR with no VAL (undefined), which is the RFC 1572 half of the commit. |
| 76f9d7304 | getNetworkLatency() no longer reports a busy client as lag | Fixed & verified | `work-B2/t8-latency.lua` against `server2.py` (auto-replies text + `IAC GA` after 50 ms). Four clean probes publish 0.053-0.061 s (`T9|`), matching the fixture's real round trip. A `send()` followed by `while os.clock()-t<1 do end` across the reply publishes **0**, i.e. the contaminated reading is dropped and the previous one stands (`T8| C.`, `T8| D.`), not ~1 s. Status bar showed `N:0.061 S:0.000` (`shots-B2/05-mxp-state.png`). |
| 137e0d14d | MSDP tables keep their shape, unfinished variables stop half-arriving | Fixed & verified | The commit's own four-case script run verbatim (`work-B2/t1-msdp.lua`): `SHAPE.Z: plain`, `LIST[2]: b  SOLO: solo`, no `!!` event lines for `CUT`/`OVER`, `WHOLE`/`NEXT` still arrive, and the error console names both dropped variables. Nested tables two deep and sibling tables also keep their shape (`T1| 5.`, `T2| D.`). Array-of-tables gap = F-B2-1. |
| 048a85b71 | MSDP values containing control codes no longer go missing | Fixed & verified | `work-B2/t1-msdp.lua` item 8: a value holding every byte 0x07-0x1f survives intact (len 25, first byte 7, last 31) **and** its sibling `CTRL.B` still arrives. Item 9: `back\slash"quote` round-trips and the following variable is unharmed. Item 10: 0x7f plus a multibyte UTF-8 character survives. |
| a3cae638a | MSDP scripts that subscribe on connect work again | Fixed & verified | `work-B2/t3-connect.lua` calls `sendMSDP("REPORT","HEALTH")` from `sysConnectionEvent`; it returns true and the server logged the bytes `IAC SB 69 VAR REPORT VAL HEALTH IAC SE` as the very first thing received, before negotiation. `sendGMCP` from the same handler still refuses with "GMCP is not currently enabled", as the commit intends. |
| 6ed3cb4df | Starter interface no longer turns MSDP back on at every profile open | Fixed & verified | Profile with `mudlet-base-ui` installed: `setConfig("enableMSDP", false)`, `saveProfile()`, `closeMudlet()`, relaunch same HOME -> `getConfig("enableMSDP")` is **false** (`mudlet2.log` `TD|`). Deleting `base_ui_settings.lua` and relaunching -> **true** again (`mudlet3.log` `TE|`). No `there is no look called "nil"` in either log. The shipped `mudlet-base-ui.mpackage` really carries the fix (`msdpSeeded` present, config version 1.7.1). |
| 6e5cc384f | UB from resetting the telnet session too early | Verified no regression | Behaviourally inert by design. Three cold profile loads in this session were clean; `TelnetReconnectStateTest` and the rest of the telnet ctest set are green (36/36); connect, disconnect and reconnect to two different fixture servers in one session renegotiated correctly each time with no state carried over (`srv1/recv.log`, `srv2/recv.log`). Not run under UBSan (no sanitiser build available without a rebuild). |
| 1889009e9 | Move three core helpers off the main window | Fixed & verified | Channel 102 on the wire: `sendTelnetChannel102("\255b")` produced `ff fa 66 ff ff 62 ff f0` and `("ab")` produced `ff fa 66 61 62 ff f0` - payload IAC doubled, the SB/SE framing not, exactly as `TelnetChannel102WireTest` asserts. `escapeIac`: with ISO 8859-1 encoding, `send("A\xffB\xff\xffC")` reached the server as `41 ff ff 42 ff ff ff ff 43 0d 0a`. `work-B2/srv1/recv.log`, `T6|`/`T7|`. |
| 045d2cb6a | Remove two always-true size checks in MXP frame layout | Verified no regression | `<FRAME Name="qa" Align="right" Width="30%" Height="50%" Title="QA frame">` over a real socket produced a correctly sized, titled, right-aligned frame with its padding intact (`shots-B2/05-mxp-state.png`), and it kept its geometry across four clear/refill cycles (`shots-B2/09-crop.png`, `10-crop.png`, `11-crop.png`). |
| ce537afd9 | Route MXP frame redirects through a write-only print sink | Fixed & verified | `<DEST qa>…</DEST>` redirected into the frame and the main window resumed immediately afterwards (`shots-B2/05-mxp-state.png`). `<DEST qa EOF></DEST>` emptied the frame (`shots-B2/08-crop.png`) while the main window kept its scrollback and took a new line right after (`MAIN WINDOW SENTINEL after clears`, `shots-B2/12-crop.png`). EOL's "drops only the unfinished line" is not reachable from a pure redirect (a redirect closes its own last line), so that half rests on the new `MXP_spec.lua` cases, which pass. |
| 40c805352 (MXP frame half) | Frame consoles built by the main console | Verified no regression | The frame sub-console was created with the right name and placed inside its frame; text routed into it, mouse selection worked in it, the wheel put it into split-screen scrollback and back, and typing into the main command line kept working throughout (`shots-B2/04`..`12`). |
| 9720638ef | A frame emptied by the game no longer keeps a selection over the deleted text | Fixed & verified | Exactly the commit's test case on a debug build: frame created, four lines redirected in, all four drag-selected (`shots-B2/06-crop.png`), then `<DEST qa EOF></DEST>` followed by Ctrl+C. Mudlet stayed alive (pid 11997 still `Sl` afterwards) with no assert in `mudlet.log`, and the frame came back empty. Scroll half also checked: 40 lines, scrolled up into split view (`shots-B2/09-crop.png`), EOF, then two new lines - the frame follows new output again (`shots-B2/10-crop.png`). |

Not covered, with reasons:
- **TLS / secure-connection prompt**: no commit in scope touches it, and the fixture servers are plaintext;
  setting up a certificate chain was out of the time box.
- **MSP**: negotiation verified (`WILL MSP` -> `DO MSP`, `sysProtocolEnabled: MSP`); sound playback not
  exercised - it needs media files and a sound device.
- **ATCP**: handshake verified - with GMCP on, Mudlet correctly answers `WILL ATCP` with `DONT ATCP`
  (by design, `ctelnet.cpp:3193`); with `enableGMCP` false it answers `DO ATCP` and sends
  `IAC SB 200 "hello Mudlet 5.0.0-dev-85d814292\ncomposer 1\n…" IAC SE`. No game-side ATCP payload
  handling beyond that.
- **UBSan re-check of 6e5cc384f**: would need a sanitiser build; not rebuilt (shared tree rules).

## Automated
ctest (`-R 'Telnet|TMxp|TEntity|TOsc|Msdp|MxpFrame|MxpDest|MxpWatchdog|TLinkStore|UntrustedText|LuaLiteral|NawsWidth'`):
**36 passed / 0 failed**. Log: `work-B2/ctest-B2.log`.

Lua specs (`TESTS_DIRECTORY` holding MXP, MXPTags, GMCP, MSP, Networking, TBufferOSC and Telnet specs):
**415 successes / 0 failures / 0 errors / 29 pending**. Log: `work-B2/specs.log`.

Both match baseline (220/220 ctest, 4702/0/0 specs) - no failure in this area, and none of the baseline's
zero failures is in it.
