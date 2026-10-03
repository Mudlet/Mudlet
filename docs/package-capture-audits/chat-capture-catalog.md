# Chat Capture in Mudlet Packages: Audit -> Design Catalog

**Scope:** 242 packages catalogued (216 real packages plus deduped file-name entries),
79 capture game text, 281 capture mechanisms total. This document turns that audit into
the design reference for chat capture in Mudlet's built-in starter UI (`base-ui`), which
today captures only `gmcp.Comm.Channel.Text`.

**Provenance convention:** every load-bearing claim cites the package name; verbatim
patterns cite file + the regex/substring as written in the package. Two source files feed
this: `findings.json` (full corpus, keyed by package) and `firstrun-recheck-findings.jsonl`
(deeper second pass over the 53 chat-heaviest packages, richer channel/tab notes). Where the
two disagree the recheck notes win; duplicate entries keyed by filename
(e.g. `AchaeaChatTabs.xml` vs `AchaeaChatTabs`) are merged.

**Honesty caveat up front:** the corpus is severely IRE/Achaea-skewed. Of ~24 games with any
chat capture, Achaea alone accounts for dozens of packages, while every non-IRE game is
usually represented by exactly ONE package - i.e. one author's pattern set. Convergence claims
below are counted across *independent games*, not packages, precisely to avoid inflating
coverage that a pile of Achaea packages would create.

---

## 1. Mechanism taxonomy, ranked by prevalence

Aggregate kind counts across all 281 mechanisms (many are NOT chat - see the split in each
row): trigger-regex 141, gmcp 40, trigger-substring 25, raw-line-hook 22, event-handler 12,
trigger-prompt 11, trigger-color 7, msdp 5, trigger-exact 3, alias 3, atcp 1.

### 1.1 trigger-regex (141) - the workhorse, but mostly NOT chat

Perl-regex triggers dominate the corpus, but they split three ways:

- **Chat capture** (the relevant slice): per-channel regexes that copy a matched comm line
  into a tab. Examples:
  - `avatar-mud-package` (Avatar MUD, YATCO), `package.xml` line 391:
    `^\w+ tells you '.*'$` -> `selectString(line,1); copy(); ChatStuff.append('Tells'); deleteLine()`.
  - `ErionMud-UI` (ErionMUD), `ErionMud-UI.xml` line 2921: ~40 bracket-channel regexes
    (`^\[chat\]`, `^\[newbie\]`, `^(.*) tells you\,`, `^You (?<verb>ask|exclaim|say|chuckle|yell)`)
    -> `ChatCaptures()` copies + `appendBuffer("ui.ChatMC")`.
  - `Earthshaker-v0.1.4.xml` (Aetolia) line 8918: `^\((\w+)\)\:([\s]+)([A-Za-z-]+)...(says|say), "`
    per-channel paren format -> `echoChat()`.
  - `PRS.xml` (Procedural Realms, EMCO) line 161: `^< Chat \| (?<sender>.+) > (?<msg>.+)$`.
- **Map/prompt/combat capture** (the majority): ASCII-map redirects
  (`achaea-fancy-gui`, `Achaean System` Wilderness/Ocean, `MedUI`, `mag-mudlet-aardwolf-gui`
  MiniMap), prompt gauges (`avatar-mud-package` Prompt, `ErionUI` PromptCapture), combat/DPS
  (`dpstest`, `cofudlet` `^(?<name>.+) is DEAD!!!`), inventory/status scrapes
  (`ire-ab-plus`, `inventory-lister`, `wisr-reporter`).
- **Reliability/portability:** highest precision when the game's chat format is stable, but
  every regex is game-specific - none port across games unchanged. FP risk scales with how
  loose the anchor is (a bare `.* says, '.*'` will eat NPC room speech).

### 1.2 gmcp (40) - structured, portable, but only ~a third is chat

GMCP event handlers. The chat-relevant subset is the `Comm.Channel` family plus a few custom
namespaces; the rest is `Room.Info` (mapper), `Char.Vitals`/`char.vitals` (gauges),
`Char.Items` (inventory).

- **Canonical chat:** `gmcp.Comm.Channel.Text` with `{channel, text, talker}`. This is the
  single most reused chat mechanism in the corpus:
  - `AchaeaChatTabs` (Achaea) `AchaeaChatTabs.xml` line 263 - **THE canonical chat package**;
    `registerAnonymousEventHandler("gmcp.Comm.Channel.Text", chatCapture)`, `ansi2decho` then
    `chatEMCO:decho(channel, txt)`. Notably: "Nothing gagged from main window - pure additive
    capture."
  - `Achaean System` line 27277, `LusterniaChatTabs.xml` line 149, `Chatter.xml` line 745
    (Threshold RPG), `Icesus.xml` line 1329, `Ishar.xml` line 3481.
- **Reliability/portability:** by far the most reliable and portable - the server pre-splits
  channel from text, so no regex fragility, works across every IRE game and any game that
  implements the IRE GMCP `Comm.Channel` module. This is why it is Layer 1 of the design.
  Requires an opt-in handshake (see 4.1).

### 1.3 trigger-substring (25) - simple begin-of-line/substring tags

Used where a channel has a fixed literal prefix. Chat examples:
- `MedUI.xml` (Medievia): `[CLAN]`, `[FORM]`, `[TOWN]`, `[CHAT about anything]`
  (begin-of-line substrings, type 2) -> `MedChat.appendToChatPanel()` -> EMCO tabs.
- `Mudlet-LOTJ-Client` `Mudlet-LOTJ-Client.xml` line 21: a single 24-pattern matcher
  (`(OOC)`, `CommNet`, `(NEWBIE)`, `(IMM)`, `[Incoming Transmission`, ...) -> one `Chatbox`
  MiniConsole.
- `Akayan GUI Creator` (Achaea): `(Newbie):`, `(Market):`, `(Party):` -> Vyzor.Chat tabs.
- `tab-chat-and-bars-for-t2t` (The Two Towers): `You tell`, `^ (OOC) `, `[ PoT ]`, `+ Guild`.
- **Reliability/portability:** cheap and robust when the tag is truly literal and unique, but
  tags are entirely game-specific and short substrings collide (LOTJ's `{` pattern "may catch
  prompt", per its own author note).

### 1.4 raw-line-hook (22) - one always-on trigger, parse in Lua

A single catch-all trigger (`return true`, `^(.*)$`, or `tempLineTrigger`) that hands every
line to Lua `string.match` parsers. Chat example is the most sophisticated hand-rolled system
in the corpus:
- `DarkMistsCompanion.xml` (Dark Mists) line 17842: `onNewLine` (`return true`) ->
  `handleCommunicationLine(line)` which runs ~15 `string.match` channel parsers
  (say/tell/yell/gtell/ooc/newbie/house + telepathic variants) and raises
  `dmapi.communication.*` events; a `ChatHistory` sink `cecho`s them into one MiniConsole.
- Others are non-chat: `generic_mapper` (room-title buffer), `simple-logger` (disk logging),
  `delay-scrolling`, `agnosticDB` table scrapes, `TelegramConnector` (relays to Telegram).
- **Reliability/portability:** flexible (all logic in Lua, easy multi-line handling) but the
  always-on `return true` trigger runs on *every* line - a real per-line cost, and the parser
  set is still game-specific.

### 1.5 event-handler (12) - non-GMCP event sinks

Named/anonymous handlers on custom or protocol events. Chat examples:
- `MedUI` / `MMChat`: `sysMMCPChatMessage`/`sysMMCPMessage` (MudMaster Chat Protocol peer
  chat) -> EMCO `MMCP` tab.
- `DarkMistsCompanion` line 16925: sink binding ~27 `dmapi.communication.*` events.
- `nannymud-starter-module`: `multiplayer_share_communication` mirrors chat across open
  profiles.

### 1.6 trigger-prompt (11) - prompt lines, almost entirely NOT chat

Prompt-type or prompt-detector triggers feeding gauges/status
(`avatar-mud-package`, `ErionMud-UI` 30-field `#N` prompt, `nannymud-starter-module`,
`MUDKIP_Mud2`, `diku-prompt-handler`). None are chat; listed to keep the split honest. These
routinely `deleteLine()` the prompt - the exact behaviour the chat design must avoid (see 2).

### 1.7 trigger-color (7) - colour-gated capture, NOT chat

`isColorTrigger`/`isColorizerTrigger` matches by ANSI colour: `DarkMistsCompanion` room-name
(bright yellow), `mag-mudlet-aardwolf-gui` teal remote-socials, `MUDKIP_Mud2` dreamword,
`mm_package` novice-clan (also colorizes). Mostly recolor-in-place or map/status, not tab chat.

### 1.8 msdp (5) - all HUD/affects/map, zero chat

`AbandonedRealms` (AFFECTS/ROOM_MAP/ROOM_NAME/WORLD_TIME) and `realms-of-despair-ui`
(~50 `msdp.*` into gauges/labels). **No package in the corpus captures chat over MSDP.**
Relevant conclusion: MSDP is not a chat transport worth targeting for the starter UI.

### 1.9 trigger-exact (3) / alias (3) / atcp (1) - marginal

Exact-match gates for capture blocks (`HelpBrowsinator` `BROWSINATOR START`,
`ire-ab-plus`); aliases for outgoing self-echo (`fed2ui`/`fed2-tools` `^say`, `^tell`,
`^com`); one legacy ATCP `RoomBrief` (`achaea-rat-counter`). ATCP is effectively dead - one
occurrence, superseded by GMCP.

---

## 2. Routing patterns - how captured text is moved

Five routing idioms recur. This matters more than the matching mechanism, because the routing
choice is what makes capture *safe* or *destructive*.

### 2.1 Additive copy: `selectCurrentLine() + copy() + appendBuffer(tab)` (NO deleteLine)

The dominant *safe* pattern, and the one the canonical package uses. Copies the current line
as full rich text (colour preserved) into the tab MiniConsole, leaving the original in the
main window untouched.
- `AchaeaChatTabs` - "Nothing gagged from main window - pure additive capture."
- `basic-materia-magica-ui-and-gmcp-mapper`/`mm_package.xml` line 1026: `Caps` trigger,
  `selectCurrentLine(); copy(); my_miniconsole:appendBuffer()` - "no deleteLine, so the line
  also remains in the main console."
- `ErionMud-UI` `ChatCaptureNoDelete` (line 3253) - `bDeleteLine=true` path keeps the line in
  main.
- `Mudlet-LOTJ-Client` - `deleteLine()` shipped but commented out, "kept enabled so main
  window still logs."
- **Preserves colour:** yes (rich-text copy). **Safe under logging/prompt features:** yes.

### 2.2 Gag + redirect: `copy() + appendBuffer(tab) + deleteLine()/deleteFull()`

Same copy, then removes the line from main. The most common *trigger-based* tab-chat idiom.
- `avatar-mud-package` (YATCO), `Akayan GUI Creator` (Vyzor), `ErionMud-UI` `ChatCapture`,
  `mag-mudlet-aardwolf-gui` (`MAGU.moveSelected`), `DSL PNP 4`, `tab-chat-and-bars-for-t2t`,
  `materia-magica-gui`, `Earthshaker` (`deleteFull()` in `echoChat`), `XAMM for CoffeeMud`.
- **Preserves colour:** yes. **Breaks under deleteLine-sensitive features:** yes - this is the
  hazardous idiom. It removes the line from the main-window log, can interfere with
  `promptEnd`/prompt detection, and starves any downstream trigger that expected the line. If
  a game also emits the same chat over GMCP, gagging here plus a GMCP tab yields double
  handling and a main window that silently drops lines.

### 2.3 Gag + re-echo compaction: `deleteLine()` then `cecho()` a reformatted line

Delete the raw line and print a condensed replacement, usually back into MAIN (not a tab).
Mostly NON-chat: `achaea-inventory-organizer`, `ire-ab-plus` (skill table),
`tracking-script` (10 track-age triggers), `transmutation-calculator`, `alertness-*`,
`plant-harvester`, `inventory-lister`. Same deleteLine hazards as 2.2; relevant only as an
anti-pattern for the starter UI.

### 2.4 decho/cecho into tabs (the GMCP path)

GMCP hands you a ready string, so there is no line to `copy()` - you `decho`/`cecho` directly
into the tab.
- `AchaeaChatTabs`, `LusterniaChatTabs`: `ansi2decho(text)` then `chatEMCO:decho(channel,txt)`
  - **colour-preserving** because the server's ANSI is converted to decho.
- `Achaean System`: `ansi2decho` + strip MXP/RGB + `[HH:MM]` timestamp -> `decho(window,text)`.
- `Chatter`: `ansi2decho` -> `Chatter.tabs[name].console:decho()`.
- `cofudlet`: `stripColors` then `cecho(color..msg)` - **loses per-char colour** (deliberate,
  applies one per-channel colour).
- `Ishar`: `setFgColor` + `echo(text)` raw (never cecho) so `<`,`>`,`#` render literally -
  a reminder that GMCP text can contain markup you must NOT interpret.
- **Never gags** in these packages (there is nothing in the main window to gag - GMCP is
  out-of-band), which is exactly why the GMCP path is inherently additive and safe.

### 2.5 EMCO / YATCO / Vyzor frameworks

Prebuilt tabbed-MiniConsole engines that own the routing:
- **EMCO** (demonnic, MDK) - the current standard. `EMCO:append(tab)` does
  `selectCurrentLine(); copy(); console:appendBuffer(); allTab:appendBuffer(); if self.gag
  then deleteLine()` (`emco.lua` line 1660). Used by `AchaeaChatTabs`, `LusterniaChatTabs`,
  `MedUI`, `PRS`, `HelpBrowsinator`, `EMCOChat`, `fed2-tools`. Auto-logs each tab to
  `log/Chatbox/YYYY/MM/DD/tab.html`. Gag is opt-in and off by default.
- **YATCO** (older, `ChatStuff`/`demonnic.chat`) - `avatar-mud-package`, `tls-mud-chat`,
  `tab-chat-and-bars-for-t2t`.
- **Vyzor.Chat** - `Akayan GUI Creator`, `materia-magica-gui`.
- All three converge on the same core: `selectCurrentLine+copy+appendBuffer` into per-tab
  MiniConsoles plus an aggregate "All" tab, with optional (off-by-default) gag.

**Dominant idiom:** additive `selectCurrentLine+copy+appendBuffer` (colour-preserving), fed
either by a trigger (2.1) or by EMCO (2.5), and `ansi2decho + decho` for GMCP (2.4). Gagging
(2.2/2.3) is common in trigger packages but is the deleteLine-sensitive hazard the starter UI
should not adopt by default.

---

## 3. Chat-pattern library, by channel semantics

Verbatim patterns as written in the packages, grouped by channel. Each entry: game, package,
file, pattern. Structural shapes and cross-game convergence counts follow each group.

### 3.1 say / says (room speech)

- Dark Mists - `DarkMistsCompanion.xml`:5922 - `^(.*) says, '(.*)'$` ; own `^You say, '(.*)'$`
- Avatar MUD - `avatar-mud-package/package.xml`:453 - `^You say '.*'$` ; `^\w+ says '.*'$`
- Aetolia - `Earthshaker-v0.1.4.xml`:9258 - `^[^<>()].+ says, ".+.\"$`
- Procedural Realms - `PRS.xml`:224 - `^(?<sender>.+) say(?<s>s)?, '(?<msg>.+)$`
- Materia Magica - `materia-magica-gui` (MMKilla.xml):257 - `^You say, '.*'$` ; `.* says, '.*'$`
- Federation 2 - `fed2-tools.xml`:41305 - `^(\w+) (says|asks), "(.+)$`
- Icesus - `Icesus.xml`:1327 - GMCP `gmcp.Room.Speech` (kind=say), rendered `talker: text`

**Shape:** `^Name says, '<quote>'` (single or double quote varies; IRE uses `"`, Diku/ROM/LP
use `'`). **Convergence: 6 independent games** on the verb-quote shape. **FP hazard: HIGHEST of
any channel** - NPC room speech uses the identical format (`The guard says, 'Halt!'`). Icesus
*deliberately does not subscribe* to `Room.Ambient` to keep NPC narration out of the say pane
(`Icesus.xml` note). Any generic say trigger will capture NPC speech unless the game separates
it out of band.

### 3.2 tell - incoming

- Dark Mists - `DarkMistsCompanion.xml`:5897 - `^(.*) tells you, '(.*)'$`
- Avatar MUD - `avatar-mud-package/package.xml`:391 - `^\w+ tells you '.*'$`
- Achaea - `Achaean System.xml`:10513 - `^tells you\, "(.+)"$`
- Aetolia - `Earthshaker-v0.1.4.xml`:8958 - `^([a-zA-z]+) tells you(| in .+), "`
- ErionMUD - `ErionMud-UI.xml`:2921 - `^(.*) tells you\,`
- Materia Magica - `materia-magica-gui`:352 - `^\w.* tells you '.*'$`
- NannyMUD - `nannymud-starter-module/package.xml`:248 - `(.+) (tells) you: (.+)` (colon form)
- Akayan/Achaea - `Akayan GUI Creator.xml`:35 - `^(.*?) tells you(.*?)`

**Shape:** `^Name tells you[,:]? <delim>message`. The phrase `tells you` is the invariant;
what follows (`, '...'`, `, "..."`, `: ...`, or bare) varies. **Convergence: 8 independent
games.** **FP hazard: LOW-MODERATE** - much safer than say. NPCs rarely "tell you" with a
quoted string; requiring a following quote or colon (`tells you[,:]\s*['"]`) removes almost
all remaining NPC false positives. This is the single strongest cross-game candidate.

### 3.3 tell - outgoing

- Dark Mists - `DarkMistsCompanion.xml`:5897 note - `^You tell (.*), '(.*)'$`
- Avatar MUD - `avatar-mud-package/package.xml`:391 - `^You tell \w+ '.*'$`
- Achaea - `Achaean System.xml`:10513 - `^You tell (\w+)\, "(.*)"$`
- Aetolia - `Earthshaker` Tells group - `^(You) tell `
- Materia Magica - `materia-magica-gui`:352 - `^You tell \w+ '.*'$`
- Fed2 (comm) - `fed2-tools.xml`:43594 alias - `^(?:tb|tell)\s+(\w+)\s+(.+)`

**Shape:** `^You tell <name> <delim>`. **Convergence: 6 independent games.** **FP hazard: LOW**,
with one caveat: `You tell the group` is a guild/party channel, not a private tell (see 3.5) -
a naive `^You tell ` grabs both, so order the group pattern first.

### 3.4 shout / yell

- Dark Mists - `DarkMistsCompanion.xml`:6038 - `^(.*) yells, '(.*)'$` (+ `yells in panic`)
- Materia Magica - `materia-magica-gui`:504 - `^You yell '.*'$` ; `.* yells '.*'$`
- Aetolia - `Earthshaker-v0.1.4.xml`:9002 - ` shouts, "` (substring; ~35 shout/thunder/bellow
  variants)
- Aetolia - `Earthshaker` AllChat - `^.+ yells\, ".+"$`

**Shape:** `^Name yells, '<quote>'` / ` shouts, "`. **Convergence: 3 independent games.** FP
hazard MODERATE (some NPCs yell). Thin evidence - defer to per-game packs.

### 3.5 guild / clan / house / order / formation (org channels)

- Dark Mists (house) - `DarkMistsCompanion.xml`:6111 - `^%[(.*)%] (.*)%: (.*)$`, gated by a
  `HOUSE_CHANNELS` whitelist (CONCLAVE/CRUSADER/LIGHT/BRETHREN/...) so it does not match every
  `[X] Y: Z` line.
- Avatar MUD (group) - `package.xml`:426 - `^You tell the group '.*'$` ; `^.+ tells the group '.*'$`
- Aetolia (guild) - `Earthshaker-v0.1.4.xml`:9332 - `^\((\w+)\)\:...say, ".*"$` (paren-channel)
- Materia Magica (clan) - `mm_package.xml`:1026 - `^\[CLAN\] (.*)$` ;
  `(.*) tells the formation (.*)` ; `^You tell the formation (.*)`
- Medievia - `MedUI.xml`:3700 - `[CLAN]`, `[FORM]` substrings
- Aardwolf - `mag-mudlet-aardwolf-gui/package.xml`:289 - `^({chan ch=[^}]*})(.*)$` (`{}` markers)
- ErionMUD - `ErionMud-UI.xml` - `^\[faith\]`, `^\[secrets\]`, `^\(Friend\)`, `^\(Gtell\)\:`

**Two shapes here.** (a) Paren-channel `(<Chan>): Name says, "..."` - IRE-specific
(Achaea/Aetolia). (b) Bracket-tag `[<TAG>] Name: message` - Dark Mists, Materia Magica,
Aardwolf (with `{}`), ErionMUD. **Convergence on bracket-tag: 4-5 independent games**, but the
tag alphabet is entirely per-game and bracket-tags collide with prompt/status lines
(`[HP: ...]`, `[Exits: ...]`), which is why Dark Mists uses an explicit channel whitelist.
Too game-specific for a generic trigger; ideal for per-game packs (Layer 3).

### 3.6 newbie

- Dark Mists - `DarkMistsCompanion.xml`:6061 - `^%[NEWBIE%] (.*)%: (.*)$` (+ Discord relay
  variant `^%[NEWBIE via Discord%] ...`)
- Akayan - `Akayan GUI Creator.xml`:58 - `(Newbie):` (begin-of-line substring)
- Aetolia - `Earthshaker-v0.1.4.xml`:8895 - `^\((Newbie)\)\:...(says|say), "`
- ErionMUD - `ErionMud-UI.xml` - `^\[newbie\]`

**Shape:** bracket/paren-prefixed `[Newbie]` / `(Newbie):`. **Convergence: 4 independent
games** on the literal word "Newbie" in a bracket/paren prefix. The word is stable but the
delimiter is not. Per-game pack material.

### 3.7 market / trade / auction

- Procedural Realms - `PRS.xml`:203 - `^< Trade \| (?<sender>.+) > (?<msg>.+)$`
- Akayan - `Akayan GUI Creator.xml`:79 - `(Market):`
- Aetolia - `Earthshaker-v0.1.4.xml`:9075 - `^\((Market)\)\:...`
- Materia Magica - `materia-magica-gui`:296 - `^AUCTION: .*`
- Federation 2 - `fed2-tools.xml`:41406 - `^\+{3} The exchange display shows the prices for (.+) \+{3}$`

**Shape:** literal `Market`/`Trade`/`AUCTION` in a bracket/paren/prefix. **Convergence: 4
games** but with four different delimiters. Per-game pack material.

### 3.8 OOC / general chat

- Dark Mists - `DarkMistsCompanion.xml`:6097 - `^%[OOC%] (.*)%: (.*)$` (+ own `^%[OOC%] to`)
- Medievia - `MedUI.xml`:3808 - `[CHAT about anything]`
- Edge of Midnight - `Edge of Midnight.xml`:1117 - `gmcp.comm.channel` where `.isooc` is true
- ErionMUD - `ErionMud-UI.xml` - `^\[chat\]`
- The Two Towers - `tab-chat-and-bars-for-t2t/package.xml`:197 - `^ (OOC) `

**Shape:** literal `OOC`/`chat` prefix, delimiter varies. **Convergence: 5 games.** Same story
- stable keyword, unstable framing. Per-game pack material.

### 3.9 Structural-shape summary

| Shape | Example | Independent games | Generic-trigger viability |
|---|---|---|---|
| `^Name tells you[,:] '<q>'` (incoming tell) | `^(.*) tells you, '(.*)'$` | 8 | **Viable** (low FP with quote/colon anchor) |
| `^You tell <name> ...` (outgoing tell) | `^You tell \w+ '.*'$` | 6 | **Viable** (order after group) |
| `^Name says, '<q>'` (say) | `^(.*) says, '(.*)'$` | 6 | **Risky** (NPC speech = same format) |
| `[<TAG>] Name: msg` (org/newbie/ooc) | `^%[(.*)%] (.*)%: (.*)$` | 5 | **No** (per-game tags, prompt collisions) |
| `(<Chan>): Name says, "..."` (IRE paren) | `^\((\w+)\)\: ...says, "` | 2 (Achaea, Aetolia) | **No** (IRE-only; use GMCP instead) |
| `< Chan \| Name > msg` (angle-pipe) | `^< Chat \| (.+) > (.+)$` | 1 (PRS) | **No** (single game) |

---

## 4. GMCP reality check

### 4.1 Namespaces observed

- **Canonical:** `gmcp.Comm.Channel.Text` with `{channel, text, talker}`.
  `AchaeaChatTabs`, `Achaean System`, `LusterniaChatTabs`, `Chatter` (Threshold), `Icesus`.
  Enabled by a login handshake: `AchaeaChatTabs` / `LusterniaChatTabs` register on
  `gmcp.Char.Name` and `sendGMCP('Core.Supports.Add ["Comm.Channel 1"]')`.
- **Canonical, split sub-nodes:** Icesus subscribes `Comm.Channel.Text`, `Comm.Channel.Tell`
  *and* `Room.Speech` separately (`Icesus.xml`:1327-1329, `Core.Supports.Set 'Comm.Channel.Tell 1'`...).
  Ishar uses `gmcp.Comm.Channel` (no `.Text` leaf) with `{channel, text, time}` and a login
  backlog replay (`Ishar.xml`:3481).
- **Lowercase variant:** CoffeeMUD's `cofudlet` handles `gmcp.comm.channel` with `.chan`/`.msg`
  fields (NOT `.channel`/`.text`) and subscribes via `Core.Supports.Add ["comm.channel 1"]`
  (`cofudlet.xml`:4465). `Edge of Midnight` uses `gmcp.comm.channel` + `gmcp.comm.tell` with an
  `.isooc` flag (`Edge of Midnight.xml`:1117). `Federation 2` (`fed2ui.xml`:2328) splits into
  `gmcp.comm.com`, `gmcp.comm.tell`, `gmcp.comm.say`.
- **Fully custom namespaces:** `cleftofdimensions` pushes pre-rendered ANSI over
  `gmcp.cleft.send.n92`/`n91`/`n93` (`cleftofdimensions.xml`:539). `rop-mudlet` uses
  `gmcp.Chat.Message` with `{channel, speaker, text}` (`rop-mudlet-candidate.xml`:1519).
  `PRS` gets its ASCII map (not chat) over `gmcp.Char.Output`.
- **MSDP / ATCP:** zero chat. MSDP (`AbandonedRealms`, `realms-of-despair-ui`) is all
  HUD/affects/map; ATCP appears once (`achaea-rat-counter` `RoomBrief`) and is obsolete.

### 4.2 GMCP vs triggers - rough split

Counting *independent games* with any chat capture (~22-24 total):
- **GMCP-based chat:** Achaea, Lusternia, Aetolia (IRE, via `Comm.Channel`), Threshold, Icesus,
  Ishar, CoffeeMUD, Edge of Midnight, Federation 2, Cleft, ROP - **~11 games**. But note almost
  all are either IRE (sharing the same `Comm.Channel` module) or a single bespoke server
  implementation.
- **Trigger-based chat:** Avatar MUD, Dark Mists, DSL, Materia Magica, Aardwolf, ErionMUD, LOTJ,
  Medievia, Procedural Realms, NannyMUD, The Two Towers - **~12 games**. These are the classic
  Diku/ROM/LP/custom servers with no GMCP comm module.

So roughly half the *games* need GMCP and half need triggers - but the corpus is dominated at
the *package* level by IRE/Achaea GMCP packages. Practically: GMCP covers the IRE family (a
large share of Mudlet's user base) with one handler; triggers are unavoidable for everyone
else, and each of those games is thinly evidenced (one package each). Do not over-extrapolate a
generic trigger set from that thin non-IRE tail.

---

## 5. Starter-UI chat-capture recommendation (`base-ui`)

A three-layer design. Layer 1 exists today; Layers 2-3 are the additions. The guiding
principle from the corpus: **capture additively, never gag by default** (the canonical
`AchaeaChatTabs` does exactly this).

### Layer 1 - GMCP (exists today)

Keep the `gmcp.Comm.Channel.Text` handler. Harden it with what the corpus shows:

1. **Keep the handshake.** `base-ui` already negotiates this (it calls
   `gmod.enableModule("BaseUI", "Comm.Channel")` on GMCP enable, verified on the wire as
   `Core.Supports.Add ["Comm 1","Comm.Channel 1"]` during testing) - matching what
   `AchaeaChatTabs`/`LusterniaChatTabs` do. Without that handshake most IRE games send
   nothing on the channel, so keep it when refactoring. The real coverage gap is games
   that have no Comm.Channel at all - that is what Layers 2-3 address.
2. **Accept the lowercase variant.** Also register/read `gmcp.comm.channel` with `.chan`/`.msg`
   (CoffeeMUD/`cofudlet`) and subscribe `Core.Supports.Add ["comm.channel 1"]`. Normalise
   `{channel|chan, text|msg, talker|speaker}` into one internal shape before routing.
3. **Colour path:** `ansi2decho(text)` then `decho` into the tab (as `AchaeaChatTabs`), which
   preserves the server's ANSI. Do NOT `cecho`/interpret markup on raw GMCP text - Ishar's
   handler echoes raw precisely so `<`, `>`, `#` are not swallowed.
4. Route to a channel->tab map with an aggregate "All" tab, mirroring the well-worn
   `channelToTab` tables (`AchaeaChatTabs`: `ct/armytell->City`, `ht/hnt->House`,
   `gt/party->Group`, `tell->Tells`, `says/emotes->Local`, `ot->Order`, `newbie/market->Misc`).

### Layer 2 - a small, defensible generic trigger set (additive only)

Ship ONLY the shapes that multiple independent games converge on with acceptable FP, and ONLY
as additive capture (no gagging). Per the analysis in section 3.9, that is precisely two
patterns, with `say` offered as opt-in:

| # | Channel | Proposed pattern | Games | FP risk | Default |
|---|---|---|---|---|---|
| 1 | tell in | `^(\w[\w'-]*) tells you[,:]\s*['"]` | 8 | low (quote/colon anchor rejects most NPC lines) | ON |
| 2 | tell out | `^You tell (?:the group )?(\w[\w'-]*)[,:]?\s*['"]` (test group first, route accordingly) | 6 | low (`the group` -> group tab; else Tells) | ON |
| 3 | say | `^(\w[\w'-]*) says[,:]\s*['"]` / `^You say[,:]\s*['"]` | 6 | **HIGH - NPC room speech is identical** | **OFF (opt-in)** |

Rationale for the cutoffs:
- **Tell in/out are the only broadly-convergent, low-FP shapes.** The `tells you`/`You tell`
  phrasing is near-universal across Diku/ROM/LP; the quote-or-colon anchor is what keeps NPC
  narration out.
- **`say` is deliberately opt-in.** Six games use it, but it is the single highest-FP channel:
  NPC speech is byte-identical (`The guard says, 'Halt!'`). Icesus's authors chose to drop
  `Room.Ambient` for this exact reason. Enabling say by default would flood the Tells/Local
  tab with mob chatter and immediately sour first-run impressions.
- **Everything else (org/newbie/market/ooc/shout, bracket-tag, paren-channel, angle-pipe) is
  excluded from the generic set.** Their keywords are stable but their framing
  (`[TAG]` vs `(Chan):` vs `< Chan | >`) is per-game, and bracket-tags collide with prompt and
  status lines. These belong in Layer 3.

**Implementation approach.** Because `base-ui` is a Lua package, register the triggers at init
(mirroring `DSL PNP 4`, `agnosticDB`, `simple-logger`, which all use `tempRegexTrigger` at
runtime) rather than shipping an XML `TriggerGroup`. Concretely:

```lua
-- at package init / sysLoadEvent, guarded so it installs once
baseUI.chat.triggerIds = baseUI.chat.triggerIds or {}
local function routeChat(tab)
  selectCurrentLine()
  copy()                       -- rich-text copy: preserves colour
  baseUI.chat.console[tab]:appendBuffer()
  baseUI.chat.console.All:appendBuffer()
  -- NO deleteLine(): additive capture, main window and logging untouched
end
table.insert(baseUI.chat.triggerIds,
  tempRegexTrigger([[^\w[\w'-]* tells you[,:]\s*['"]]], function() routeChat("Tells") end))
```

Store the IDs so the set can be toggled/killed when the game is IRE (GMCP already covers it) or
when the user disables it. `appendBuffer` after `selectCurrentLine+copy` is the colour-
preserving primitive every framework in the corpus (EMCO/YATCO/Vyzor) is built on
(`emco.lua`:1660). Use `tempRegexTrigger` (not `tempTrigger`) so the anchors hold.

A permanent XML `TriggerGroup` is the alternative if the team prefers the triggers to be
user-visible/editable in the editor - trade-off is that a shipped XML group is harder to gate
per-game than runtime IDs.

### Layer 3 - per-game pattern packs (ship as data)

Model the per-game regexes as data keyed like Mudlet's built-in `TGameDetails` game registry
(game identity -> pattern set), loaded only when the connected profile matches. Structure:

```lua
baseUI.chat.packs = {
  ["Dark Mists"] = {          -- from DarkMistsCompanion.xml
    say   = {[[^(.*) says, '(.*)'$]], [[^You say, '(.*)'$]]},
    tell  = {[[^(.*) tells you, '(.*)'$]]},
    yell  = {[[^(.*) yells, '(.*)'$]]},
    ooc   = {[[^%[OOC%] (.*): (.*)$]]},        -- (Lua-pattern form as shipped)
    house = {[[^%[(.*)%] (.*): (.*)$]], whitelist = {"CONCLAVE","CRUSADER","LIGHT",...}},
  },
  ["Aardwolf"]  = { org = {[[^(\{chan ch=[^}]*\})(.*)$]]}, ... },  -- from mag-mudlet-aardwolf-gui
  ["ErionMUD"]  = { chat = {[[^\[chat\]]]}, newbie = {[[^\[newbie\]]]}, ... },
  -- ...
}
```

**Games with proven, corpus-sourced pattern sets ready to lift verbatim:**

| Game | Source package | What it provides |
|---|---|---|
| Achaea / Lusternia / Aetolia / Imperian (IRE) | `AchaeaChatTabs`, `LusterniaChatTabs`, `Achaean System` | GMCP `Comm.Channel` + full channel->tab maps (Layer 1 covers; no triggers needed) |
| Aetolia | `Earthshaker-v0.1.4.xml` | paren-channel say/tells/city/house/guild/market/clans/shout regexes |
| Avatar MUD | `avatar-mud-package/package.xml` | tell/group/public YATCO regexes |
| Dark Mists | `DarkMistsCompanion.xml` | say/tell/yell/gtell/ooc/newbie/house Lua-match set (+ telepathic variants) |
| Materia Magica | `mm_package.xml`, `materia-magica-gui` | clan/formation/tell/relay/auction/yell regexes |
| Aardwolf | `mag-mudlet-aardwolf-gui/package.xml` | `{say}`/`{tell}`/`{chan ch=}` tag markers |
| ErionMUD | `ErionMud-UI.xml`, `ErionUI 1.0.xml` | ~40 `[tag]`/`(tag)` bracket-channel regexes |
| Medievia | `MedUI.xml` | `[CLAN]`/`[FORM]`/`[TOWN]`/`[CHAT]` substrings (+ MMCP event) |
| Procedural Realms | `PRS.xml` | `< Chat\|Newbie\|Trade\|... >` angle-pipe regexes |
| LOTJ | `Mudlet-LOTJ-Client.xml` | 24 comm/transmission substring patterns |
| NannyMUD | `nannymud-starter-module/package.xml` | tells/says/whispers colon-format |
| The Two Towers | `tab-chat-and-bars-for-t2t/package.xml` | You tell / (OOC) / [PoT] / +Guild substrings |
| CoffeeMUD | `cofudlet.xml` | `gmcp.comm.channel` lowercase handler + channel map |
| Federation 2 | `fed2ui.xml`, `fed2-tools.xml` | `gmcp.comm.com/say/tell` handlers + comm-unit regexes |
| DSL | `DSL PNP 4/PNP/DSL_PNP_Chat.lua` | dynamically-assembled `channel_patterns` table |

Each pack's provenance is a real, shipping community package - so the patterns are field-tested
against the actual game, not guessed. Ship packs additively (same `routeChat`, no gag). Community
can contribute new packs as plain data.

### Anti-goals (learned from the corpus)

1. **Do not gag (`deleteLine`/`deleteFull`) by default.** It removes lines from the main-window
   log, can break `promptEnd`/prompt detection, and starves downstream triggers. The canonical
   `AchaeaChatTabs` gags nothing; `Mudlet-LOTJ-Client` and `mm_package` keep lines in main on
   purpose. Gag must be strictly opt-in, off by default.
2. **Do not delete or rewrite the prompt.** The prompt-handling packages (`avatar-mud-package`,
   `ErionMud-UI`, `MUDKIP_Mud2`, `diku-prompt-handler`) all `deleteLine()` the prompt; that is a
   gauge/HUD concern, not chat, and it interacts badly with logging and prompt-end features.
3. **Do not ship a monolithic catch-all `^(.*)$` / `.*` chat trigger.** Only whole-screen
   consumers do that (`DarkMistsCompanion` line hook, `delay-scrolling`, `tts-*`,
   `simple-logger`), and it runs Lua on every single line. A generic chat set must be a few
   anchored patterns, not one megamatcher.
4. **Do not enable `say` by default** - NPC room speech is indistinguishable from player say by
   format alone; Icesus dropped `Room.Ambient` for this reason.
5. **Do not assume canonical GMCP casing/shape.** Handle lowercase `gmcp.comm.channel`
   (`.chan`/`.msg`), split sub-nodes (Icesus/Ishar), and custom namespaces gracefully; normalise
   before routing.
6. **Do not build one giant say/tell regex per game inside base-ui.** Keep per-game patterns as
   external data (Layer 3), so adding a game is a data PR, not a code change.

---

## Appendix: honest coverage limits

- The corpus proves chat capture for ~22-24 games, but only Achaea/IRE is deeply sampled. Every
  non-IRE game is a single package = one author's patterns; treat those regexes as a strong
  starting point, not a validated spec.
- No corpus package captures chat over MSDP or (meaningfully) ATCP. Targeting those for chat
  would be inventing coverage the corpus does not support.
- The generic Layer-2 set is intentionally minimal (2 patterns + opt-in say). The temptation to
  add org/newbie/market generically is not supported: those channels converge on a *keyword* but
  not on a *format*, so a generic trigger would either miss most games or over-match. They are
  correctly Layer-3 per-game data.
