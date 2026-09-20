# Character-Data (Vitals/Stats) Capture in Mudlet Packages: Audit -> Design Catalog

**Scope:** 247 packages audited; **63 capture character data** (vitals/stats), across **451
mechanisms**. This document turns that audit into the design reference for the vitals gauges in
Mudlet's built-in starter UI (`base-ui`), which today reads only `gmcp.Char.Vitals` hp/maxhp and
mp/maxmp and paints two gauges.

**Provenance convention:** every load-bearing claim cites the package name; verbatim patterns
cite file + line + the regex/GMCP path as written in the package. Source of record is
`vitals-findings.json` (keyed by package, one entry per mechanism with kind/stat/pattern/
maxima_source/routing/file/line/notes).

**Honesty caveat up front:** as with the chat corpus, this one is IRE-skewed *at the package
level* - Achaea/Aetolia/Lusternia account for a large share of the packages. To keep convergence
claims honest, everything below counts **independent games**, not packages. A generic-filename
aggregate entry (`package.xml`, 85 distinct community packages sharing that filename) is
attributed per-mechanism by its real `file` prefix, not lumped. Two entries are excluded from
capture counts as non-captures: `GUIFlex.xml` (a layout template whose "Health" gauge is fed
`math.random(100)` against a hardcoded max of 100 - a *labelled* gauge with no data feed) and
`rop-mudlet-v.1.4.0-NOTE` (0 mechanisms).

**The one surprise that reframes everything (vs the chat catalog):** for *chat*, GMCP was an
IRE-family story. For *vitals it is not.* Of 200 GMCP mechanisms, **139 come from non-IRE games
and only 61 from IRE**; **14 of the ~20 GMCP games are non-IRE** (Materia Magica, CoffeeMUD,
Medievia, Icesus, Ishar, Edge of Midnight, Procedural Realms, Federation 2, LOTJ, Rites of
Passage, Discworld, ClessidraMUD, MUME, plus generic templates). `Char.Vitals`-style GMCP has
genuinely spread beyond Iron Realms. The catch, quantified in section 2: the naive
`gmcp.Char.Vitals.hp/maxhp` reader base-ui ships today cleanly matches **only the IRE core**
(Achaea/Aetolia/IRE) - every non-IRE game diverges on casing, key name, or where `maxhp` lives.
So GMCP's reach is real, but capturing it requires a dialect-normalising reader, not the
current fixed-path one.

---

## 1. Mechanism taxonomy, ranked by prevalence

Aggregate kind counts across all 451 mechanisms in the 63 vitals packages:
**gmcp 200, prompt-regex 86, trigger-regex 78, msdp 38, score-parse 33, event-handler 6,
atcp 1** (plus 9 "other" enabling/handshake mechanisms). The independent-game count per kind is
the honest denominator and is given in each row.

| Kind | Mechanisms | Independent games | IRE share of games |
|---|---|---|---|
| gmcp | 200 | **20** | ~6 IRE / 14 non-IRE |
| prompt-regex | 86 | **15** | 3 IRE / 12 non-IRE |
| trigger-regex | 78 | **20** | ~5 IRE / 15 non-IRE |
| msdp | 38 | **3** | 0 IRE / 3 non-IRE |
| score-parse | 33 | **14** | 3 IRE / 11 non-IRE |
| atcp | 1 | 1 | 1 (obsolete) |

### 1.1 gmcp (200 mech, 20 games) - dominant, portable, but dialect-fractured

Structured out-of-band vitals via GMCP event handlers. Dominates by mechanism count and, unlike
chat, is broad across games. **Games:** Achaea, Aetolia, IRE-generic, Lusternia (via score not
gmcp), CoffeeMUD, Materia Magica, Medievia, Icesus, Ishar, Edge of Midnight, Procedural Realms,
Federation 2, LOTJ, Rites of Passage, Discworld, ClessidraMUD, MUME, plus `generic`/`Muxlet`
user-configurable readers.
- **Canonical IRE:** `gmcp.Char.Vitals.hp / .maxhp / .mp / .maxmp` - `Achaean System.xml:19097`,
  `Earthshaker-v0.1.4.xml:21893` (Aetolia), `tls-mud-chat/package.xml:524` (IRE). Values are
  **strings**; every IRE package `tonumber()`s them.
- **Reliability/portability:** highest - server pre-computes cur and max, no regex, no gagging,
  inherently additive (there is no line in the main window to delete). This is why it is Layer 1.
  But see 2: the *key/value shape* is not portable; six distinct dialects exist.

### 1.2 prompt-regex (86 mech, 15 games) - the trigger workhorse for non-GMCP games

Perl-regex triggers on the recurring game prompt. **Games:** Achaea, Lusternia, Avatar MUD,
Dark Mists, DSL, CKMud, CoffeeMUD, Materia Magica, NannyMUD, Realms of the Dragon, Two Towers,
Aardwolf, ErionMud, MUD2, generic-Diku. This is where the maxima problem lives (section 5): most
default game prompts carry **current values only**. Structural families are catalogued in
section 4.
- **Reliability/portability:** each regex is game-specific; several **require the user to
  reconfigure their in-game prompt** first (DSL, Avatar, Diku, Nanny) or have the package
  **auto-install a custom prompt** (ErionMud, Aardwolf). FP risk is generally low because
  prompts are anchored, but the whole family is fragile to prompt-format drift.

### 1.3 trigger-regex (78 mech, 20 games) - events, affects, combat, level-ups

Non-prompt line triggers: affect on/off strings, level-up lines, gold/xp gain, enemy
health-descriptions, and protocol-in-band lines (GemStone/Lich `<progressBar>` XML,
`Aardwolf {stats}` statmon). **Games:** 20, the widest spread of any kind - but most are a
*single package per game* capturing affects or combat prose, not core hp/mp scalars.
- Notable: **LichConnect** (GemStone IV/DragonRealms via Lich proxy) parses Stormfront XML
  `<progressBar id='health' value='..' text='..'/>` at `LichConnect/package.xml:541` - `value`
  is already a **0-100 percentage**, `maxima=hardcoded`. **Aardwolf** `mag-mudlet-aardwolf-gui`
  parses the `statmon` protocol `^{stats}(.*)$` into a positional array; hp = field 20 (cur) /
  21 (max), **same-line** (`package.xml:889`).
- **Reliability/portability:** none port across games; affect-string sets are entirely
  per-game and large (Materia Magica ~25-30 buff triggers, MedUI ~19 add/remove pairs).

### 1.4 msdp (38 mech, only 3 games) - thin, but standardized where present

MSDP variable handlers + `REPORT` subscription. **Only 3 games:** Realms of Despair, CKMud, and
AbandonedRealms. Realms of Despair uses the **standard MSDP names** (`HEALTH`/`HEALTH_MAX`/
`MANA`/`MANA_MAX`/`MOVEMENT`/`MOVEMENT_MAX`/`OPPONENT_HEALTH`...) at `realms-of-despair-ui/
package.xml:360`; CKMud uses **game-themed names** (`POWERLEVEL`/`KI`/`FATIGUE` +`_MAX`) at
`CK/CK.xml:2728`; AbandonedRealms requests **no vitals at all** (only AFFECTS/time/map).
- **Reliability/portability:** where a game speaks MSDP, `X`/`X_MAX` pairs are as clean as GMCP.
  But the corpus is too thin (3 games) to call MSDP a broad win - detail and honest assessment
  in section 3.

### 1.5 score-parse (33 mech, 14 games) - the maxima bootstrap layer

Parses the `score`/`status`/`fes` command screen, almost always to learn **maxima** the prompt
lacks (see section 5). **Games:** Achaea, Lusternia, Dark Mists, ROTD, ErionMud, Materia Magica,
DSL, MUME, CKMud, CoffeeMUD, NannyMUD, unknown-ROM, MUD2. Half of these **auto-send `score`**
(consent implications in section 5).

### 1.6 event-handler (6) / atcp (1) - marginal

Event-handler: GMCP negotiation sinks and cross-profile mirroring (Icesus `Core.Supports.Set`
burst, `Icesus.xml:1292`). ATCP appears exactly once and is obsolete (superseded by GMCP), same
as in the chat corpus.

---

## 2. GMCP dialect map

Every `Char.Vitals`-like shape observed, with the exact provenance. **This is the most
design-critical section for Layer 1**, because base-ui's current fixed reader
(`gmcp.Char.Vitals.hp/maxhp/mp/maxmp`) matches only row A cleanly.

### 2.1 Namespace / key shapes

| # | Shape | Games (provenance) | Value format |
|---|---|---|---|
| A | `gmcp.Char.Vitals.hp/.maxhp/.mp/.maxmp` (PascalCase, max **inside** Vitals) | Achaea `Achaean System.xml:19097`; Aetolia `Earthshaker-v0.1.4.xml:21893`; IRE `tls-mud-chat/package.xml:524`; CoffeeMUD-clean `cofudlet.xml:4511`; Ishar `Ishar.xml:940` (hp/mp but **no** maxhp/maxmp) | **strings** on IRE (need `tonumber`); numbers on CoffeeMUD |
| B | **lowercase** `gmcp.char.vitals.hp` | Edge of Midnight `Edge of Midnight.xml:995`; Materia Magica `mm_package.xml:2218`; CoffeeMUD `XAMM for CoffeeMud.xml:2280`; Federation 2 `fed2ui.xml:2265` | numbers |
| C | max in a **separate node** `gmcp.char.maxstats.maxhp` | Materia Magica `mm_package.xml:2246`; Edge of Midnight `Edge of Midnight.xml:992` (`char.maxstats.maxhp/maxmana/maxmoves`) | numbers |
| D | max in **`Char.Maxstats`** (Pascal, separate node) | Icesus `Icesus.xml:960` (`.maxhp/.maxmana/.maxmoves/.maxpsp`) | numbers |
| E | **camelCase** max `gmcp.Char.Vitals.maxHp/maxMana` | Medievia `MedUI.xml:802`, `MultiBot Core v1.11.xml:1709`; LOTJ `Mudlet-LOTJ-Client.xml:379` (`maxHp`) | numbers |
| F | **entirely different namespace** `gmcp.Char.player.hp/.maxHp` | Procedural Realms `PRS.xml:4235` (energy/maxEnergy for mana, stamina/maxStamina) | numbers |
| G | **group/member arrays** `gmcp.Group.Info.members[].hp/.hp_max` | Rites of Passage `rop-mudlet-candidate.xml:224`; CoffeeMUD group `cofudlet.xml:4396` (`.info.hp/.mhp`, `.mn/.mmn`) | numbers |
| H | **nested cur/max tables** `gmcp.char.vitals.stamina.cur/.max` | Federation 2 `fed2-tools.xml:32322`, `fed2ui.xml:2265` | numbers |

### 2.2 Mana key naming (a silent breaker)

Even among games that DO put max inside Vitals, the mana key diverges: `.mp` (Achaea, Aetolia,
Ishar, IRE), `.mana` (Icesus, Medievia, CoffeeMUD, Edge of Midnight, Mudlet-LOTJ), `.sp` (Materia
Magica - "spell points", `mm_package.xml:2218`), `.energy` (Procedural Realms). A reader keyed on
`.mp` alone silently drops mana for **most non-IRE games**.

### 2.3 What breaks a naive `gmcp.Char.Vitals.hp/maxhp/mp/maxmp` reader

Quantified against the hp/mp GMCP shapes: base-ui's fixed reader **fully matches only Achaea,
Aetolia, and IRE-generic** (row A, with `tonumber`). It fails at least one field for:
- **Casing:** Edge of Midnight, Materia Magica, CoffeeMUD, Federation 2 (lowercase `char.vitals`).
- **maxhp location:** Materia Magica, Edge of Midnight (`char.maxstats`), Icesus (`Char.Maxstats`).
- **mana key:** Icesus/Medievia/CoffeeMUD/Edge-of-Midnight (`.mana`), Materia Magica (`.sp`),
  Procedural Realms (`.energy`).
- **camelCase max:** Medievia, MultiBot, LOTJ (`maxHp`/`maxMana`).
- **namespace:** Procedural Realms (`Char.player`), Rites of Passage / CoffeeMUD-group
  (`Group...members[]`).
- **string values:** Achaea/Aetolia send hp as `"3480"`; arithmetic without `tonumber` fails.

### 2.4 Extra stats offered over GMCP (breadth beyond hp/mp)

Beyond hp/mp, GMCP games expose a wide, game-specific stat surface that a starter UI *could* map
but should treat as optional: movement/moves/stamina (Icesus, MedUI, cofudlet, fed2), IRE
willpower/endurance/ego/power (`Achaean System.xml:21038`, `tls-mud-chat/package.xml:528`),
psp (Icesus), Fury/blood/spark/essence/devotion (Aetolia `Earthshaker`), balance/equilibrium
as `"0"/"1"` strings (Achaea/Aetolia), limb states (`left_arm`/`right_arm`), xp/tnl in many
shapes (`.nl`, `.tnl`, `.tnlpct`, `xpForNextLevel`), enemy/target hp (`gmcp.IRE.Target.Info.hpperc`
Achaea; `char.status.enemypct` CoffeeMUD; `opponent_hp_pct` Ishar), and gold in many currencies
(Achaea `.gold/.bank/.unboundcredits/...`). **Only hp and mp are near-universal**; everything else
needs the per-game map (section 6, L3).

---

## 3. MSDP variable map

### 3.1 Variables real packages consume

| Game (package) | Vital variables consumed | Provenance |
|---|---|---|
| Realms of Despair (`realms-of-despair-ui`) | **`HEALTH`/`HEALTH_MAX`, `MANA`/`MANA_MAX`, `MOVEMENT`/`MOVEMENT_MAX`, `EXPERIENCE`/`_MAX`/`_MIN`/`_TNL`, `OPPONENT_HEALTH`/`_MAX`/`_NAME`/`_LEVEL`**, plus `MONEY`, `LEVEL`, `AFFECTS`, `STR..LCK` (+`_PERM`), `AC/HITROLL/DAMROLL/...` | `package.xml:360-374` (gauges), subscription `:97` |
| CKMud (`CK`) | `POWERLEVEL`/`POWERLEVEL_MAX`, `KI`/`KI_MAX`, `FATIGUE`/`FATIGUE_MAX`, `GODKI`/`_MAX`, `DARK_ENERGY`/`MAX_DENERGY`, `OPPONENT_HEALTH`/`_MAX`, +30 more | `CK/CK.xml:2728-2850`, subscription list `:2667` |
| AbandonedRealms (`AbandonedRealms Gui Add-on`) | **no vitals** - only `AFFECTS`, `WORLD_TIME`, `ROOM_MAP/NAME`, `AREA_NAME` | subscription `:22` |

Realms of Despair is the **only corpus package using the standardized MSDP vital names**
(`HEALTH`/`HEALTH_MAX`/`MANA`/`MANA_MAX`/`MOVEMENT`/`MOVEMENT_MAX`). CKMud proves the names are
**not guaranteed standard** - a Dragon Ball MUD renames health to `POWERLEVEL`, mana to `KI`.

### 3.2 REPORT subscription mechanics (who sends what, and when)

MSDP data only flows after the client subscribes with `sendMSDP("REPORT", <var>, ...)`. The
corpus shows two trigger points:
- **On connection, host-gated:** `realms-of-despair-ui` sends its ~60-var REPORT on
  `sysConnectionEvent`, **only if** `getConnectionInfo()` host == `realmsofdespair.com`
  (`package.xml:97`). This is the cleanest pattern - subscribe automatically, but only for the
  game that speaks it.
- **On a protocol-ready trigger:** `AbandonedRealms` waits for `[INFO] MXP version 1.0 detected`
  then sends REPORT (`:22`); an affects script also force-enables MSDP via
  `setConfig('enableMSDP', true)` on connect.
- **On connect + re-subscribe on staleness:** `CK` sends `REPORT` for all vars on connect and
  **re-subscribes if `msdp.UPDATE_EPOCH` goes stale** (`CK/CK.xml:2667/2678`) - a nice
  self-healing idiom.

Note Mudlet must have MSDP enabled (`setConfig('enableMSDP', true)`) for any of this; two packages
force it on.

### 3.3 Could MSDP be a zero-config second layer for the starter UI? Honest assessment

**Partially, and only with a normaliser - do not overclaim.**
- **For:** where a game speaks standardized MSDP (Realms of Despair), `HEALTH/HEALTH_MAX` etc are
  a drop-in cur/max pair as clean as GMCP, and subscription is a single automatic `sendMSDP
  REPORT` on connect. MSDP reaches an ecosystem (SMAUG/ROM/Diku-derivatives) that largely has
  **no GMCP** - so it is genuinely additive coverage, not overlap.
- **Against:** the corpus proves MSDP vitals for **only 3 games**, one of which (AbandonedRealms)
  requests no vitals and another (CKMud) uses **non-standard variable names**. So a base-ui MSDP
  layer must (a) subscribe defensively (`REPORT HEALTH HEALTH_MAX MANA MANA_MAX MOVEMENT
  MOVEMENT_MAX`) and (b) accept that games renaming those (CKMud) still need a per-game map.
  Subscribing costs almost nothing (one telnet sub-negotiation), so a **defensive standardized
  REPORT on connect is low-risk and worth shipping** - just don't advertise universal coverage.

---

## 4. Prompt-shape families (86 regexes -> structural families)

Grouped by structure. For each: converging games, whether **max is same-line**, FP risk, and
whether the family is **zero-config** or requires the user to set/allow a specific in-game prompt.

### Family A - IRE stat-suffix `(\d+)h, (\d+)m, (\d+)e, (\d+)w` (Achaea) / `+ p, en` (Lusternia)
- **Games (3, IRE):** Achaea `Achaea Fancy GUI 1.0.xml:595`, `achaea-rat-counter/package.xml:743`;
  Lusternia `lusternia-fancy-gui-v2.xml:606` (`h,m,e,p,en,w`).
- **Max:** NOT same-line -> `score-command-parse` (the dominant IRE pattern).
- **Zero-config?** The *prompt* is the IRE default (no user setup), BUT maxima need a `score`
  bootstrap (section 5). FP risk **LOW** - the `Nh, Nm, Ne, Nw` suffix format is highly specific.
  Trailing single letters (`x`/`e`/`b`) are affect/balance flags, parsed by filter-children.

### Family B - angle-bracket slash-pairs `<-cur/maxhp cur/maxmp cur/maxmv ... tnl-> <cur/max>`
- **Games (1):** Avatar MUD `package.xml:506`.
- **Max: SAME-LINE** (cur/max per stat, plus a monitored-target `<cur/max>`). FP **LOW**.
- **Zero-config? NO** - "assumes the user has set the Avatar MUD prompt to the exact form".
  Also gags the prompt with `deleteLine()`.

### Family C - paren slash-pairs `(cur/max)(cur/max)(cur/max)` (Diku/ROM)
- **Games (1, generic-Diku):** `diku-prompt-handler` `package.xml:214`.
- **Max: SAME-LINE.** FP **LOW** (heavily anchored). **Zero-config? NO** - "Requires user to run a
  Diku prompt formatted `(hp/maxhp)(mana/maxmana)(mov/maxmov) ...`".

### Family D - pipe-delimited multi-field `<name|curHP|maxHP|curM|maxM|...>` (~27 fields)
- **Games (1, DSL):** `DSL PNP 4` `DSL_PNP_Statusbar.lua:277`, and duplicate `PNP`.
- **Max: SAME-LINE** (field3/field4 = curhp/maxhp). FP **LOW**. **Zero-config? NO** - "REQUIRES
  user to set the exact in-game prompt `prompt <%t|%h|%H|...`". `curhp` may be `?` when blinded.

### Family E - labeled `** HP: cur/max   SP: cur/max` (LPMud)
- **Games (1):** NannyMUD `nannymud-starter-module/package.xml:59`.
- **Max: SAME-LINE.** FP **LOW**. **Zero-config? NO** - "Requires the user to configure the
  in-game prompt to emit `** HP: h/H   SP: s/S`".

### Family F - bracket `[HP:cur EP:cur]>>` (Two Towers)
- **Games (1, but 3 packages):** Two Towers `importThis.xml:67`, `tab-chat-and-bars-for-t2t/
  package.xml:25`, `the-two-towers-exits-window/importThis.xml:67`.
- **Max: current only -> HARDCODED 240.** FP **LOW**. Zero-config for capture (default t2t
  prompt) BUT the hardcoded max is **wrong for any character whose real max differs** - a
  cautionary example, not a model.

### Family G - labeled verbose `Hp: N Gp: N Xp: N` (LPMud) + brief `Hp: cur(max)`
- **Games (1):** Realms of the Dragon `ROTD_GUI/ROTD_GUI.xml:207` (verbose, current-only) and
  `:276` (brief `Hp: cur(max)`, **same-line**). Verbose max from `score` ("You have N (N) hit
  points", `:244`) with a persisted-file fallback `ROTD_maxHP.txt`, else hardcoded 1000.
- Also ships a **user-configurable prompt parser** (`setPromptPattern`, `:1742`) with
  `hpIndex`/`maxHPIndex` defaults 1/2 - the one package that explicitly generalises.

### Family H - angle-bracket stat-suffix `<123hp 456sp 789st>` / `<100Hp 50m 30mv>`
- **Games (2):** Materia Magica `MMKilla.xml:207` (max from `gmcp.char.maxstats`); CoffeeMUD
  default prompt `<100Hp 50m 30mv>` documented at `XAMM for CoffeeMud.xml:643` (but only detected,
  routes user to `xamm set prompt`). **Max: not same-line** (gmcp/score). FP **LOW-MODERATE**.

### Family I - custom marker-delimited `#1<CHP>#2<MHP>#3<CMP>#4<MMP>...` (ErionMud)
- **Games (1):** ErionMud `ErionMud-UI.xml:3617`, `ErionUI 1.0.xml:148`.
- **Max: SAME-LINE** (fields #1/#2 = cur/max hp). **Zero-config? NO, but AUTO-INSTALLED** - the
  package **auto-sends** `prompt #1%h#2%H#3%m#4%M...` to reconfigure the game prompt, warning
  the user "This UI will change your prompt". Deletes the prompt line from main.

### Family J - numeric-suffix space-separated `<500hp 300mn 200mv 20420tnl>` + percent variant
- **Games (1):** Dark Mists `DarkMistsCompanion.xml:4984` (numeric, current-only, max from
  score), plus a **percent variant** `<75%hp 60%mn 100%mv>` (`:5042`) and a regen-annotated
  variant `<500hp(+25) ...>` (`:4951`). The percent variant is notable: already 0-100, **no max
  needed for display** (converted to absolute only if a score-derived max exists, else warns).

### Family K - protocol-in-band (not a "prompt" but a per-tick line)
- GemStone/DragonRealms (Lich) `<progressBar id='health' value='N' text='..'/>` - value is a
  **0-100 percentage**, max hardcoded (`LichConnect/package.xml:541`). Aardwolf `statmon`
  `^{stats}(.*)$` positional array, cur/max **same-line** (`mag-mudlet-aardwolf-gui/
  package.xml:889`). Included for completeness; these are per-game protocol packs (L3), not a
  generic regex family.

### Family L/M - non-vitals prompts
- MUD2 bare `*` prompt (`MUDKIP_Mud2.xml:1823`) - carries no vitals; hp/stamina come from the
  `fes`/`qs` commands (score-parse). CKMud `[Pl: N,NNN,NNN | ...]` (`CK/CK.xml:3486`) - PL shown
  **with commas** (`[0-9,]+`) but real cur/max from MSDP. Both are reminders that a prompt may be
  a combat-state signal, not a data source.

### 4.1 Prompt-family summary

| Family | Structure | Games | Max same-line? | Zero-config? | Generic-reader viability |
|---|---|---|---|---|---|
| A | `Nh, Nm, Ne, Nw` (IRE) | 3 (IRE) | No (score) | Prompt yes / max no | GMCP covers IRE better; skip |
| B | `<cur/maxhp ...>` (Avatar) | 1 | **Yes** | **No - user sets prompt** | Per-game pack only |
| C | `(cur/max)(cur/max)(cur/max)` | 1 | **Yes** | **No - user sets prompt** | Per-game pack only |
| D | `<name|curhp|maxhp|...>` (DSL) | 1 | **Yes** | **No - user sets prompt** | Per-game pack only |
| E | `** HP: cur/max SP: cur/max` | 1 | **Yes** | **No - user sets prompt** | Per-game pack only |
| F | `[HP:cur EP:cur]` (t2t) | 1 | No (hardcoded) | Yes (capture) | Cautionary - hardcoded max |
| G | `Hp: N Gp: N Xp: N` / brief `Hp: cur(max)` | 1 | Brief yes / verbose no | Yes | Configurable-parser model |
| H | `<Nhp Nsp Nst>` (MM/CoffeeMud) | 2 | No (gmcp/score) | Yes (capture) | GMCP covers these games |
| I | `#1..#2..` markers (ErionMud) | 1 | **Yes** | **No - auto-installed** | Per-game pack only |
| J | `<Nhp Nmn Nmv Ntnl>` + `%` variant | 1 | No (score) / percent needs none | Yes (capture) | Percent variant is clean |

**The load-bearing finding:** every prompt family that carries **cur AND max on one line**
(B, C, D, E, I) is a **non-default prompt the user must set or the package must auto-install**.
Every family that works on the **default prompt** (A, F, G-verbose, H, J-numeric) carries
**current values only** and needs a max from elsewhere. So there is no free lunch: a zero-config
generic prompt reader gets *current* values from default prompts but **cannot get max** without
either a bootstrap command or a percentage prompt. This directly shapes section 5 and 6.

---

## 5. The maxima problem (design-critical)

**212 of 451 mechanisms (47%) see only current values** (`none-current-only`) - though many of
those are non-scalar (affects, balance flags, gold, names, positions) that need no max anyway.
For the scalar vitals that DO drive gauges, the corpus uses six maxima strategies. Full
distribution: **none-current-only 212, gmcp-field 83, hardcoded 55, same-line 43,
score-command-parse 30, msdp-field 18, observed-max 8.**

### 5.1 gmcp-field (83) / msdp-field (18) - max is a sibling protocol field. **Best.**
Max arrives out-of-band alongside current, always fresh, no command sent. gmcp: all row-A games
plus the separate-node dialects (`char.maxstats`, `Char.Maxstats`). msdp: `HEALTH_MAX` etc
(Realms of Despair), `POWERLEVEL_MAX` (CKMud). **Failure modes:** only the dialect divergence of
section 2 (max may live in a different node/casing) - not a data-freshness problem. This is the
target the starter UI should prefer.

### 5.2 same-line cur/max (43) - `<100/120hp>`. **Best among triggers.**
Both values in one regex capture: Avatar (B), Diku (C), DSL (D), Nanny (E), ErionMud (I),
ROTD-brief, Aardwolf statmon, MUD2 `fes`/`qs`, and the `score` lines themselves. **Failure
modes:** requires the specific prompt (families B/C/D/E/I need user setup); `curhp` can be `?`
when blinded (DSL guards this).

### 5.3 score-command-parse (30) - bootstrap max from the `score` screen. **Common, hazardous.**
The default-prompt games (Family A, G-verbose, J) learn maxima by parsing a `score`/`sc`/`qsc`
screen: `^\| Health  : *\d+/(\d+)` (`Achaea Fancy GUI 1.0.xml:37`), `^Health:\s+(\d+)/(\d+)\s+
Mana:...` (`achaea-rat-counter/package.xml:245`, `lusternia-fancy-gui-v2.xml:33`), `^You have
(%d+)/(%d+) hit, ...` (`DarkMistsCompanion.xml:5105`).

**Who auto-sends `score` (and when):**
- **On login:** `achaea-fancy-gui` ("auto-SENDS `score` on login so the max* score-parse
  triggers fire", `Achaea Fancy GUI 1.0.xml:79`); `achaea-rat-counter` (`package.xml:285`);
  `lusternia-fancy-gui` ("On login the package SENDS `score`; until score runs, gauges have no
  maxima", `:114`); `MultiBot Core v1.02/v1.11` (SENDS `score` on login, `:24`).
- **On world-enter / periodically:** `DarkMistsCompanion` sends `score` on `dmapi.world.enter`
  (`:6904`); `CK` sends `status` ~every 120s when not fighting (`:2422`); `MultiBot` sends `stat`
  every 120s (anti-whisk); `MUDKIP_Mud2` sends `fes` on a 10s timer.
- **On demand only (no auto-send):** `DSL PNP 4` (user types `score`; package only auto-sends
  `whoami` for name); `MumeSpellTimers` (elicited when the user types score/affects);
  `xamm-for-coffeemud` (only via a `score` alias the user runs); `ErionMud` `SilentScoreCapture()`
  sends `score` but between border-markers on demand.

**Failure modes (all three are real and observed):**
1. **Nil/zero max until bootstrap.** Gauges are blank or wrongly scaled until the first `score`
   completes. `DarkMistsCompanion` explicitly logs a warning "if hpMax==1" and the percent-prompt
   path can't convert to absolute (`:5042`).
2. **Stale after a level-up.** Max HP rises on level, but the cached score-max does not until the
   next `score`. Dark Mists mitigates with a **`You gain X/Y hp...` level-up trigger** (same-line
   cur/max, `:4892`); DSL parses "You raise a level!! You gain N hit points" (`:160`). Without
   such a trigger the bar under-reads after every level.
3. **Sending commands without consent.** Auto-sending `score`/`status`/`fes` injects output into
   the user's screen (some packages **gag** it, some don't) and traffic to the server the user
   did not type. `CK` gags the status block; several IRE GUIs let the `score` screen scroll past
   on login. **This is the anti-goal for base-ui** (section 6).

### 5.4 observed-max (8) - track the highest value seen. **Consent-free, imprecise.**
No command, no max field - the package remembers the largest current value it has witnessed and
uses that as the bar maximum. `DarkMistsCompanion` xp bar tracks `StatusBar.maxTnl` (`:8114`);
MUD2 `updateQs` uses observed-max for magic when the `qs` line omits it; XAMM tnl (`:2301`);
LichConnect roundTime/castTime. **Failure modes:** under-reads until the player has been seen at
full (a fresh character shows a "full" bar that is actually partial), and can over-read if the
true max ever drops (debuff, form change). But it is the **only strategy that needs neither a
protocol field nor an injected command** - which makes it the right default for a zero-config
generic reader (with a UX note that values calibrate as you play).

### 5.5 hardcoded (55) - a fixed constant. **Wrong by construction, avoid.**
`hpMAX=240` for Two Towers (`tab-chat-and-bars-for-t2t/package.xml`, three packages), alignment
max 1000 / tnl max 1333 (Avatar), align+10000 max 20000 (CoffeeMUD), LichConnect `<progressBar>`
value already a percentage so "max" is a nominal 100. Fine for genuinely bounded 0-100
percentages; **wrong for any real hp/mp** where the constant will not match the player's actual
max. The t2t packages are the clearest example of the failure: the gauge is simply incorrect for
any character whose max HP isn't 240.

### 5.6 Maxima strategy ranking for the starter UI

1. **gmcp-field / msdp-field** - fresh, consent-free, out-of-band. Use whenever available.
2. **same-line cur/max** - only when the game's *default* prompt provides it (rare) or via L3
   per-game packs where the user opts into a prompt.
3. **observed-max** - the safe zero-config fallback for current-only default prompts and
   percentage prompts, with a "calibrates as you play" UX note.
4. **score-command-parse** - powerful but requires auto-sending a command; **only behind explicit
   user consent** in base-ui.
5. **hardcoded** - never for hp/mp; acceptable only for genuinely bounded 0-100 percentages.

---

## 6. Starter-UI gauge recommendation (`base-ui`)

A three-layer design mirroring the chat catalog. Layer 1 exists today (as a fixed-path reader);
the work is to **harden L1 with the dialect map**, add **L2 MSDP**, and add a **conservative L3**.
Guiding principle from the corpus: **read-only and additive - never gag the prompt, never
auto-send a command without consent.**

### Layer 1 - GMCP, hardened with the section-2 dialect map (biggest, cheapest win)

Replace the fixed `gmcp.Char.Vitals.hp/maxhp/mp/maxmp` reader with a **normaliser** that resolves,
in order, a small candidate table into one internal `{hp,maxhp,mp,maxmp,...}` shape:

```lua
-- resolve current/max across observed dialects, tonumber-coerced
local V = gmcp.Char and (gmcp.Char.Vitals or gmcp.Char.player) or gmcp.char and gmcp.char.vitals
local MAX = (gmcp.Char and gmcp.Char.Maxstats) or (gmcp.char and gmcp.char.maxstats)
hp    = tonumber(V and V.hp)
maxhp = tonumber((V and (V.maxhp or V.maxHp)) or (MAX and (MAX.maxhp or MAX.maxHp)))
mp    = tonumber(V and (V.mp or V.mana or V.sp or V.energy))
maxmp = tonumber((V and (V.maxmp or V.maxMana or V.maxsp or V.maxEnergy)) or (MAX and (MAX.maxmana or MAX.maxsp)))
-- nested cur/max (Federation 2): V.stamina.cur / V.stamina.max
```

Concretely it must handle: (1) **lowercase** `gmcp.char.vitals` (Edge of Midnight, Materia Magica,
CoffeeMUD, fed2); (2) **max in a separate node** `char.maxstats`/`Char.Maxstats` (Materia Magica,
Edge of Midnight, Icesus); (3) **mana key** `.mp|.mana|.sp|.energy`; (4) **camelCase** `maxHp`
(Medievia, LOTJ); (5) **string values** -> always `tonumber` (IRE); (6) **nested cur/max**
(fed2). Leave the alternate-namespace shapes (Procedural Realms `Char.player`, group arrays) to
L3 - they are single-game.

This single change takes GMCP coverage from **3 games (IRE core) to ~12-14 games** with no new
protocol, no triggers, and inherent safety (out-of-band, nothing to gag).

### Layer 2 - MSDP defensive subscription (small, honest, additive)

On connect, if MSDP negotiates, send a **standardized REPORT** and wire the standard pairs:

```lua
-- on sysConnectionEvent, after MSDP is enabled
sendMSDP("REPORT", "HEALTH","HEALTH_MAX","MANA","MANA_MAX","MOVEMENT","MOVEMENT_MAX")
-- handlers: msdp.HEALTH/HEALTH_MAX -> hp gauge; msdp.MANA/MANA_MAX -> mp gauge  (tonumber)
```

**Honest scope:** the corpus proves MSDP vitals for only 3 games, and one (CKMud) renames HEALTH
to POWERLEVEL - so this reaches the **standardized-MSDP** slice (Realms of Despair and the
SMAUG/ROM ecosystem that uses those names) but not renamers. Subscribing costs one sub-negotiation
and is harmless if the game ignores it, so it is worth shipping as a **zero-config second layer** -
just documented as "covers games using standard MSDP names", not universal. Re-subscribe on
`UPDATE_EPOCH` staleness (CKMud's idiom) if reliability warrants.

### Layer 3 - generic prompt reader + per-game packs (conservative, consent-aware)

**3a. Generic default-prompt reader (current-only + observed-max).** Ship a *few* anchored,
low-FP prompt patterns that work on **default** prompts and feed gauges with **observed-max**
scaling (never auto-`score`):
- **Percentage prompts are the cleanest** and should be first-class: `(\d+)%hp` style
  (Dark Mists percent variant `:5042`), Aardwolf/`\d+%`, `<progressBar value>` percentages.
  A 0-100 value needs **no max at all** - paint it directly.
- **Current-only numeric default prompts** (IRE `Nh, Nm, Ne, Nw`; Dark Mists `<Nhp Nmn Nmv>`;
  ROTD `Hp: N ...`): capture current, scale the bar with **observed-max**, and show a small
  "values calibrate as you play" hint. Do **not** auto-send `score` to resolve max.

**3b. Per-game packs as data** (keyed like Mudlet's `TGameDetails` registry, loaded when the
profile matches). These carry the same-line-max and prompt-config families that are unsafe to
generalise. Corpus-sourced, field-tested packs ready to lift verbatim:

| Game | Source package | What it provides | Note |
|---|---|---|---|
| Achaea/Aetolia/Lusternia/Imperian (IRE) | `Achaean System`, `earthshaker-*`, `lusternia-fancy-gui` | GMCP `Char.Vitals` hp/mp/ep/wp + `score` maxima parsers | L1 covers hp/mp; pack adds ep/wp + score-max |
| Realms of Despair | `realms-of-despair-ui` | standard MSDP REPORT + gauges | L2 covers; pack adds enemy/xp |
| CKMud | `CK` | MSDP `POWERLEVEL/KI/FATIGUE` renamed pairs | renamer - needs the map |
| Avatar MUD | `avatar-mud-package` | `<cur/maxhp ...>` same-line prompt (requires prompt set) | opt-in prompt |
| DSL | `DSL PNP 4` | pipe-delimited `<...|curhp|maxhp|...>` (requires prompt set) | opt-in prompt |
| NannyMUD | `nannymud-starter-module` | `** HP: cur/max SP: cur/max` (requires prompt set) | opt-in prompt |
| Two Towers | `importThis`/`tab-chat-and-bars-for-t2t` | `[HP:cur EP:cur]` (hardcoded max - fix to observed) | replace hardcoded 240 |
| Realms of the Dragon | `ROTD_GUI` | `Hp: N Gp: N Xp: N` + brief `Hp: cur(max)` + configurable parser | configurable-parser model |
| Materia Magica | `materia-magica-gui`, `mm_package` | `<Nhp Nsp Nst>` prompt + `char.maxstats` GMCP | L1 covers with dialect map |
| Aardwolf | `mag-mudlet-aardwolf-gui` | `statmon {stats}` positional cur/max | protocol pack |
| GemStone/DragonRealms | `LichConnect` | Stormfront `<progressBar>` percentages | protocol pack |
| CoffeeMUD | `cofudlet`, `xamm-for-coffeemud` | lowercase GMCP + `<100Hp 50m 30mv>` prompt | L1 covers GMCP |
| Federation 2 | `fed2ui`, `fed2-tools` | nested `char.vitals.stamina.cur/.max` | L1 nested-table path |
| ErionMud | `ErionMud-UI` | `#N` marker prompt (auto-installs prompt) | opt-in, warns user |

### Anti-goals (learned from the corpus)

1. **Do not auto-send `score`/`status`/`fes` to bootstrap maxima without explicit user consent.**
   A third of the score-parse packages inject this traffic on login (`achaea-fancy-gui`,
   `lusternia-fancy-gui`, `MultiBot`, `DarkMistsCompanion`); some gag the output, some let it
   scroll past. For a starter UI that runs for *everyone*, auto-sending commands to the game on
   the user's behalf is surprising and undesirable. **Prefer observed-max + a "calibrates as you
   play" note; offer score-bootstrap only as an explicit opt-in.**
2. **Do not gag/`deleteLine` the prompt.** Avatar, ErionMud, XAMM delete the prompt line; that
   breaks logging and prompt-end detection (same hazard the chat catalog flagged). Read the
   prompt additively.
3. **Do not hardcode a max for hp/mp.** The Two Towers packages (`hpMAX=240`) produce a gauge
   that is simply wrong for any other character. Hardcoded maxima are acceptable *only* for
   genuinely bounded 0-100 percentages.
4. **Do not assume canonical GMCP casing/shape.** The naive `gmcp.Char.Vitals.hp/maxhp` reader
   matches only the IRE core; every non-IRE game diverges (section 2). Normalise before painting.
5. **Do not require the user to reconfigure their in-game prompt in the generic path.** Families
   B/C/D/E/I all demand a specific prompt; that is per-game opt-in (L3), never the default.
6. **Do not build per-game regex sets inside base-ui.** Keep them as external data (L3), so
   adding a game is a data PR, not a code change - same conclusion as the chat catalog.

---

## Appendix: honest coverage limits

- **63 games/packages is a real but uneven sample.** IRE is over-represented at the package
  level; most non-IRE games are a single package = one author's field-tested patterns. Treat
  those as strong starting points, not validated specs.
- **The vitals GMCP story is genuinely broader than chat's** (14 non-IRE games vs an
  IRE-dominated chat corpus) - this is the corpus's clearest signal and the strongest argument
  for hardening L1 rather than treating GMCP as "IRE only".
- **MSDP is thin (3 games).** Ship a defensive standardized REPORT, but do not advertise coverage
  the corpus does not support; renamers (CKMud) still need per-game maps.
- **The maxima problem has no universal zero-config solution.** Protocol fields solve it when
  present; otherwise every default prompt is current-only and the only consent-free option is
  observed-max (imprecise) or percentage prompts (clean but game-specific). Auto-`score` is
  effective but is an anti-goal for a default-on starter UI.
- **hp and mp are the only near-universal scalars.** Everything else (movement, willpower, ego,
  endurance, psp, stamina, blood, fury, enemy-hp, affects) is game-specific and belongs in the
  per-game map, not the generic gauges.
