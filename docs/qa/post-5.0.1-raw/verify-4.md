# Verification of batch 4: E2 (settings redesign, tour, tabs, detached windows) and B2 (telnet/MXP/MSDP)

Verifier V4, display :82, tree 85d814292 (= development 12b373743). Work dir `work-V4/`,
screenshots `shots-V4/`. A/B binary: `/home/user/worktrees/v501/build-linux-debug-nosan/src/mudlet`
(5.0.1-dev-592821c8c), launched with the same env and a throwaway HOME. Nothing was built.

## Verdicts

| Finding | Reported severity | Verdict | Your severity | Notes |
| --- | --- | --- | --- | --- |
| F-E2-1 | Minor (Fix incomplete) | CONFIRMED | Minor | Reproduced step for step: after one mouse click on **Next** the arrow keys stop reaching the tour, `PageUp` falls through to the console behind the overlay (split-screen scrollback activates) and Escape no longer closes the tour. Tracker: no match. |
| F-E2-2 | Minor (Fix incomplete) | PARTIAL | Minor | Substance confirmed - an off-centre tab detaches far below 80 px, and the left-most tab cannot be detached at all (200 px drag, no detach). The *number* is wrong: measured threshold is **18 px** on development (16 px does not detach, 18 px does), not "~33 px". A/B on 5.0.1: **14 px**. So 52b0d4b02 did change behaviour (14 -> 18 px) but nowhere near the intended 80 px, and the centre-relative measurement is pre-existing, not introduced by it. Tracker: no match. |
| F-E2-3 | Cosmetic (New bug) | CONFIRMED | Cosmetic | Editor page's "Could not update themes: Error transferring https://github.com/Colorsublime/Colorsublime-Themes/archive" is cut at the card edge, no wrap, HTTP reason never visible. Tracker: no match. |
| F-E2 N1..N5 | Notes | N4 CONFIRMED, N5 CONFIRMED, N1/N2/N3 not re-run | Note | N4: package-manager heading elided to `Str...` for "StressinatorDisplayBench" with the card width free (`shots-V4/27-detached-alt-o.png`). N5: "Moje hry" lists only the user profile, not "Mudlet self-test", while the right-hand pane describes self-test (`shots-V4/19-czech-connect-dialog.png`, `22-connect-again.png`). |
| F-B2-1 | Major (New bug, also in 5.0.1) | CONFIRMED | Major | Reproduced on development, and **also reproduced on the 5.0.1 binary** - pre-existing, as reported. Tracker: no match (#10189/#10136 closed, different shapes). |
| F-B2-2 | Note, Known #10658 | CONFIRMED as a Note | Note | A read ending in a bare ESC leaves the mode switch undetected; the same bytes in one read are found. #10658 is open in `recent-issues.md` line 152 and describes exactly this. Correctly classified as pre-existing, not a regression. |
| F-B2-3 | Note (#10362 does not reproduce) | CONFIRMED | Note | `gmcp.Core.Ping` from a bare-number payload is a **number** here, not a function - #10362's symptom is absent on this build, as B2 said. |

## Coverage audit

| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |
| --- | --- | --- | --- | --- | --- |
| findings-E2.md | 19 | 19/19 - every cited screenshot exists (`11-tour-kbd-right2`..`75-about-menu`, all present) | 10 (0f70a691f, 7a3174476, 631bbe1e4, 52b0d4b02, c35dcd05f, 41c41e428, 37152ee0d, 54025f766, 0237e8d46, ec0fedbb9) | 10 | none - two "Could not test" rows (41c41e428, 37152ee0d) were upgraded, see below |
| findings-B2.md | 13 | 13/13 - `shots-B2/05..12` present, `work-B2/mudlet.log` carries the quoted `T1|`/`T2|`/`TA|`/`TD|` lines | 5 (f49c8c133, 14e63e080, 137e0d14d, 9720638ef, plus the F-B2-1 repro) | 5 | none |

Re-run detail is in **Details** below. Two E2 rows that said "Could not test" can now be filled in,
and two new Notes fall out of that work (F-V4-1, F-V4-2).

## New findings

### F-V4-1: The redesigned settings dialog is largely untranslated in a 100 %-complete locale (Note / Minor)
**Steps** (`work-V4/` session 2, screenshots `shots-V4/16`, `28`, `29`):
1. Fresh HOME, launch `mudlet --profile "Mudlet self-test" --offline --mirror`.
2. Alt+P -> General -> Interface language -> **Čeština** (shown with the "complete" star).
3. Close Mudlet, relaunch with the same HOME, open Alt+P again.

**Expected**: with a starred (complete) language, the settings dialog reads in Czech.

**Actual**: after the restart the *old* .ui strings are Czech ("Automaticky uložit při ukončení",
"Jazyk rozhraní:", "Možnosti logování", "Zobrazovat řádek menu:"), but everything the settings
redesign added stays English: the whole sidebar (General, Appearance, Main display, Input line,
Editor, Mapper, Chat and sharing, Connection, Privacy and security, Accessibility, Shortcuts,
Advanced, Mudlet support), the page title, the search placeholder "Find in settings", the
"Same settings, new look!" banner and its "Got it" button, and the section headings
("Saving and notifications", "Language", "Icons and toolbars", "Theme", "Profile tabs",
"Web search"). The relabelled "Show main toolbar:" from 54025f766 also reverted to English
while its neighbour "Zobrazovat řádek menu:" is translated (`shots-V4/29-toolbar-combo.png`).

The strings *are* translatable (`src/dlgProfilePreferences.cpp:1098`, `:1203`, `:1848` all use
`tr()`), so this is translation-data lag: 0f70a691f's and 54025f766's new/changed strings have
not been through Crowdin yet. Recorded as a Note, not a code bug - but it is what a Czech user
sees today, and it is the part of 0237e8d46 E2 could not reach.
**Evidence**: `shots-V4/16-settings-czech.png` (live switch), `shots-V4/28-prefs2.png` (after a
full restart), `shots-V4/29-toolbar-combo.png`, `shots-V4/19-czech-connect-dialog.png` (the
connection dialog by contrast is fully Czech). **Tracker**: no match.

### F-V4-2: "Show main toolbar" does not reach an already-detached window (Note / Cosmetic)
**Steps**: detach a profile into its own window while "Show main toolbar" is *Never*, then set it
to *Always* in the settings.
**Expected**: the detached window gains the toolbar, as the main window does.
**Actual**: the main window shows the toolbar at once (`shots-V4/31-main-toolbar.png`) while the
already-detached window still has none (`shots-V4/32c.png`). A window detached *after* the change
does get one (`shots-V4/35c.png`). Cosmetic; it is also what blocked E2 from testing 41c41e428
and 37152ee0d.
**Commits**: adjacent to 37152ee0d / 41c41e428. **Tracker**: no match.

## Details

### F-E2-1 - tour keyboard (CONFIRMED)
Fresh HOME, `--profile "Mudlet self-test" --offline --mirror`, 11 s wait. Tour opened at "1 of 6"
(`shots-V4/01-tour-start.png`). One `Right` with no mouse used -> "2 of 6"
(`shots-V4/02c.png`), so the keyboard works before any click. Mouse click on **Next** at (797,460)
-> "3 of 6" (`shots-V4/03-tour-mouse-next.png`). Then:
- `Right`, `Right`: still "3 of 6" (`shots-V4/04-tour-right-after-mouse.png`).
- `Prior` (PageUp): `work-V4/tour.log` gains
  `main| Split-screen scrollback activated. Press <CTRL>+<ENTER> to cancel.` - the key reached the
  console *behind* the overlay (grep count 1, expected 0).
- `Escape`: tour still shown on "3 of 6", now over a split view
  (`shots-V4/06-tour-escape.png` - the requested split-view screenshot).
Matches E2's description exactly, including the cause (focus sits on the child QPushButton after
the click, so `TUiTour`'s `keyPressEvent` never sees the key).

### F-E2-2 - tab detach threshold (PARTIAL: substance right, distance wrong)
Two tabs, "V4 Two" (index 0, centre x=389) and "Mudlet self-test" (index 1, centre x=869), tab row
centre y=210 on development / y=212 on 5.0.1 (identical 960 px window, toolbar shown).
Probe script `work-V4/probe-detach.sh <x> <y> <dy...>`: press on the tab, wait 0.4 s (past
`TAB_REORDER_DELAY_MS` = 150), step down 2 px at a time to `y+dy`, release, then look for a
detached window (the window title is localised, so the search matches `odpojeno|Detached`).

    development 85d814292, tab index 1:
      dy= 6,9,11,12,13,14,16  -> no detach
      dy=18                   -> DETACHED ("Mudlet – Mudlet self-test (odpojeno) - Odpojeno")
    Mudlet-5.0.1 build,       tab index 1:
      dy= 6,9,11,12,13        -> no detach
      dy=14                   -> DETACHED
    development, tab index 0 (left-most):
      dy=20,40,60,80,120,200  -> never detaches (`shots-V4/37c.png`, both tabs still there)

So the real threshold is 18 px on development and 14 px on 5.0.1 - E2's "~33 px" is an upper bound
they happened to test, not the boundary. The cause E2 gives is right and I confirmed it in the
source: `TTabBar::mouseMoveEvent` (`src/TTabBar.cpp:459-466`) compares `DETACH_DISTANCE_THRESHOLD`
against the manhattan distance from the tab bar's **centre**, which for a tab at x=869 with the bar
centred at x=640 is already ~229 px before any vertical movement. The only live gates are
"cursor left the tab bar rect" (~12 px) and the 60 % vertical ratio.
`git show Mudlet-5.0.1:src/TTabBar.h` has the identical code with `= 50`, so this is
**pre-existing, not a regression**; 52b0d4b02's 50->80 bought 4 px. E2's "Fix incomplete" status is
right. The left-most tab's early return is `mudlet::slot_tabDetachRequested`
(`src/mudlet.cpp:9086`, `if (index < 1 || index >= mpTabBar->count())`), so 52b0d4b02's own test
case ("drag a tab downward by 60-70 px - it should snap back") cannot be performed on it at all.

### F-E2-3 - clipped theme-update error (CONFIRMED)
Alt+P -> Editor with no network: `shots-V4/page-228.png` shows
"Could not update themes: Error transferring https://github.com/Colorsublime/Colorsublime-Themes/archive"
running into the card edge on one line. Cosmetic, as reported.

### F-B2-1 - MSDP array of tables (CONFIRMED on both binaries)
`work-B2/repro-F-B2-1.lua` run verbatim via `lua dofile(...)` on the self-test profile with
`MUDLET_TEST_MODE=1`.

Development (`work-V4/czech.log`):

    errors| <could not decode msdp.GROUP - any previous value is kept: InvalidJSONInput: parse error:
             after array element, I expect ',' or ']'
             {"NAME":"Fred","HEALTH":"90"}{"NAME":"Barney","HEALTH":"50"}
    main| F1| GROUP = nil      main| F1| AOA = nil      main| F1| ONEG[1].NAME = Solo

5.0.1 binary (`work-V4/v501.log`), same profile, same script:

    errors| <Lua error:InvalidJSONInput: parse error: after array element, I expect ',' or ']'
             {"NAME":"Fred","HEALTH":"90"}{"NAME":"Barney","HEALTH":"50"}
    main| F1| GROUP = nil      main| F1| AOA = nil      main| F1| ONEG[1].NAME = Solo

Identical failure on both, so B2's "not a regression" is right. Worth noting for the tracker entry:
137e0d14d *did* improve the diagnostics - development names the variable and says the previous value
is kept, 5.0.1 only prints the raw yajl error.

### Coverage re-runs, E2
- **0f70a691f** (mandated): dialog opened with Alt+P. Eleven pages visited and screenshotted
  (`shots-V4/page-120,156,192,228,264,300,353,389,425,461,497.png`) plus General
  (`08-settings-open.png`); read in detail: Main display, Editor, Chat and sharing, Connection,
  Accessibility, Shortcuts, Appearance, General. No empty page, no overlapping widget, no
  unlabelled control, no sideways scroll. Search for `font`, `proxy`, `timestamp` each grouped the
  hits by category with the matching row highlighted (`search-proxy.png`, `search-timestamp.png`),
  and `zzqqxx` gave the "No results in settings for …" state (`search-none.png`). Instant apply:
  the settings window was moved clear of the console and Main display font size stepped 14 -> 20;
  the console text grew immediately with the dialog still open (`10-moved.png` -> `12c.png`).
  Held up; the one cosmetic defect is F-E2-3.
- **7a3174476** (mandated): fresh profile (default `mMMCPAutoAcceptCalls = true`), Alt+P, worked in
  the dialog, dismissed with Escape (0 preference windows left), then `saveProfile()`. The saved
  `.../Mudlet self-test/current/2026-09-19#15-45-21.xml` still reads
  `autoAcceptCalls="yes"` (with `autostartServer="no" allowPeekRequests="no"`). Held up.
- **631bbe1e4** (mandated): with two tabs open, Appearance -> Vzhled -> Tmavý. Applied instantly;
  tab bar and toolbar go dark, both tab labels fully legible, no clipped descenders and no grey
  band between the tabs and the console (`shots-V4/40-dark-two-tabs.png`, `40c.png`). Held up.
- **c35dcd05f** (mandated): detached "Mudlet self-test" into its own window, activated it,
  `Alt+O` -> "Správce balíčků – Mudlet self-test" opened (`shots-V4/27-detached-alt-o.png`);
  `Alt+W` -> the detached window closed the profile and disappeared (window list afterwards holds
  only the main window). Held up. A/B: the 5.0.1 detached window still shows the underlined
  mnemonics on its six menu titles (`shots-V4/51-v501-detached.png`), i.e. the collision the commit
  removed.
- **41c41e428** (mandated; E2 could not test): I set "Show main toolbar: Always" and then detached a
  profile, which gives the detached window a toolbar. On development its Discord entry is a proper
  default action - Discord icon plus the "Discord" label (`shots-V4/35c.png`). On the 5.0.1 binary
  the same detached window shows bare text "Discord" with **no icon** (`shots-V4/51-v501-detached.png`),
  exactly the symptom the commit describes. The click behaviour (opening a game's invite) still
  needs a game offering one, so that third of the commit is untested. Row upgraded from
  "Could not test" to **Fixed & verified (partial, A/B against 5.0.1)**.
- **37152ee0d**: same setup shows a detached window *does* carry the toolbar when it is created with
  the setting on; the icon size itself was not measured, so the row stays "Could not test" for its
  own claim, but the premise E2 recorded ("no toolbar in a detached window") is wrong - see F-V4-2.
- **54025f766**: "Zobrazovat řádek menu:" / "Show main toolbar:" both carry the colon
  (`shots-V4/29-toolbar-combo.png`, and in English `page-120.png`). Held up.
- **0237e8d46** (mandated; E2 could not test): switched to Čeština. Connection dialog after a
  restart is fully Czech - title "Vyberte profil, se kterým se chcete připojit", tabs "Moje hry" /
  "Všechny hry", "Připojit k" / "Možnosti", buttons Odebrat / Kopírovat / Nový / Připojit / Offline /
  Zrušit (`shots-V4/19-czech-connect-dialog.png`). Menu bar, console messages, the keyboard-switching
  callout and the package manager are Czech too. The settings dialog is the exception - F-V4-1.
- **ec0fedbb9**: the mouse half holds up (the tour sat at "1 of 6" until a real Next click, and only
  Next advanced it); the keyboard half is F-E2-1.

### Coverage re-runs, B2
All on development, fresh HOME, `MUDLET_TEST_MODE=1`, script `work-V4/v4-b2.lua`, mirror log
`work-V4/b2.log`:

    main| V4| A. after first half: SPLITV = nil
    main| V4| B. after second half: SPLITV = hello world  (expected 'hello world')
    main| V4| C. split table: SPLITT.A=1 SPLITT.B=2
    main| V4| D. 200KB read with ESC[5z: prompt=true force=true
    main| V4| E. ESC[8z (undefined): prompt=false force=false  (both must be false)

- **137e0d14d** (mandated): A/B/C above - an MSDP variable split across two reads arrives whole,
  nothing half-arrives, and a table split across the boundary keeps its shape. Held up.
- **f49c8c133** (mandated): D/E above - the mode-switch escape buried in a 200 KB read is found in
  one pass, and the undefined `ESC[8z` fires nothing. Held up.
- **9720638ef** (mandated): with `specialForceMXPProcessorOn`, `feedTriggers` created
  `<FRAME Name="qa" …>` and four `<DEST qa>` lines (`shots-V4/60-frame.png`), all four were
  drag-selected with the mouse (`shots-V4/61c.png` shows the highlight), a `tempTimer` then sent
  `<DEST qa EOF></DEST>` while the selection stood, and Ctrl+C was pressed over the frame. The
  debug build **did not abort**: pid 31497 still `Sl` afterwards, `b2.log` has zero
  assert/abort/fatal lines, and the frame came back empty with the selection gone
  (`shots-V4/63c.png`). Held up.
- **14e63e080** (mandated): real socket, B2's `work-B2/server.py` in `work-V4/srv/` offering
  `DO NEWENV`. `work-V4/srv/recv.log`:

        SENT fffd27                                             (server: DO NEW-ENVIRON)
        RECV fffb27                                             (Mudlet: WILL NEW-ENVIRON)
        SENT fffa270103434c49454e545f4e414d45fff0               (SB SEND USERVAR "CLIENT_NAME" SE)
        RECV fffa270003434c49454e545f4e414d45014d55444c4554fff0 (SB IS USERVAR CLIENT_NAME VAL MUDLET SE)
        -- setConfig("enableNEWENVIRON", false); getConfig -> false --
        SENT fffa270103434c49454e545f4e414d45fff0               (same request)
        (no RECV at all)
        -- setConfig("enableNEWENVIRON", true) --
        SENT fffa270103434c49454e545f4e414d45fff0
        RECV fffa270003434c49454e545f4e414d45014d55444c4554fff0 (answered again)

  Off mid-session answers nothing, on the same connection; re-enabling restores it. Held up.

## Automated
No ctest or busted run of my own - E2 (34/34) and B2 (36/36 ctest, 415 spec successes) both match
the 220/220 and 4702/0/0 baseline, and nothing I saw suggests otherwise.
