# E2 findings

## Summary
Covered the redesigned settings dialog end to end (all 13 sidebar entries screenshotted in
light mode, search across five terms, instant apply, live sync, dark mode), the
`sysSettingChanged` contract for all seven new keys, the MMCP Esc regression, the first-launch
interface tour, the Mudlet Tutorial intro panels, two-profile tab bar in dark mode, tab detach
and the detached window's Alt+W / Alt+O. Three problems found: the interface tour stops
responding to the keyboard once its Next button is clicked with the mouse (and PageUp then
reaches the console behind it), a tab detaches after a ~33 px drag rather than the documented
80 px because the threshold is measured from the tab bar's *centre*, and one clipped error
label on the Editor settings page. Could not test: the update dialog (no update entry in this
build's menus and no network), the detached window's Discord button and toolbar icon size (the
main toolbar is off by default and the run ran out of time), the Czech/Spanish translation pass
(0237e8d46), and the tutorial's `give` command (requires playing the quest through).
ctest for the area: 34/34 passed.

## Findings

| ID | Severity | Commit | Title | Status |
| --- | --- | --- | --- | --- |
| F-E2-1 | Minor | ec0fedbb9 | Interface tour stops taking the keyboard after its Next button is clicked; PageUp then scrolls the console behind the overlay and Escape no longer closes the tour | Fix incomplete |
| F-E2-2 | Minor | 52b0d4b02 | Tab detach threshold is measured from the tab bar's centre, so a tab away from the centre detaches after a ~33 px drag instead of 80 px | Fix incomplete |
| F-E2-3 | Cosmetic | 0f70a691f | Editor settings page: the "Could not update themes: …" error label is clipped at the card edge instead of wrapping | New bug |

### F-E2-1: Interface tour stops taking the keyboard after its Next button is clicked
**Steps** (`work-E2/repro-tour-keys.sh`):
1. `HOME=$(mktemp -d)`, `DISPLAY=:81 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled: ./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror`
2. Wait ~9 s. The tour shows "1 of 6".
3. Press `Right` (no mouse used yet) - advances to "2 of 6". Keyboard works.
4. Click the tour's **Next** button with the mouse - advances one step.
5. Press `Right` twice, then `Prior` (PageUp), then `Escape`.

**Expected**: per ec0fedbb9 and the tour's own intro text ("Use Next or the arrow keys to move
through it"), arrows keep advancing it, nothing reaches the window behind the overlay, and
Escape closes the tour.

**Actual**: after step 4 the arrow keys do nothing to the tour; `PageUp` is not consumed by the
overlay and reaches the main console, which prints
`[ INFO ] - Split-screen scrollback activated. Press <CTRL>+<ENTER> to cancel.` and splits the
view behind the tour; `Escape` does not close the tour. Only clicking Next / Skip tour still
works. (Root cause is visible in `src/TUiTour.cpp:368` - `keyPressEvent` is on the overlay, but
after the click keyboard focus sits on the child `Next` QPushButton, so the overlay never sees
the key.)

**Evidence**: `shots-E2/11-tour-kbd-right2.png` ("3 of 6", reached by keyboard alone) →
`shots-E2/12-tour-after-mouse-next.png` (mouse Next → "4 of 6") →
`shots-E2/13-tour-right-after-mouse.png` (two Right presses, still "4 of 6") →
`shots-E2/14-tour-pageup-after-mouse.png` (split-screen scrollback now active behind the tour)
→ `shots-E2/15-tour-escape.png` (Escape pressed, tour still on "4 of 6").
Log line: `work-E2/mudlet2.log`, `main| [ INFO ] - Split-screen scrollback activated.`

**Commits**: ec0fedbb9 (fixed the mouse half of exactly this class - presses, drags and double
clicks reaching the main window - but keys still propagate and the tour's own key handling dies
after any click).

**Tracker**: no matching issue or PR (`search_issues` for tour/overlay/keyboard: no hits;
`recent-issues.md` and `open-prs.md`: no hits).

### F-E2-2: A tab detaches after a ~33 px drag, not the intended 80 px
**Steps** (`work-E2/repro-tab-detach.sh`):
1. Fresh `HOME`, launch `mudlet --mirror` with no `--profile`.
2. New profile "QA Two" → Offline. Then Games ▸ Play ▸ All games ▸ "Mudlet Tutorial" → Offline.
   Two tabs: "QA Two" (index 0, centred x≈389) and "Mudlet Tutorial" (index 1, centred x≈869),
   tab bar at y≈145-167 in a 960 px-wide window.
3. Press on the "Mudlet Tutorial" tab at (869, 155) and drag straight down to (869, 188) - 33 px -
   then release.

**Expected**: per 52b0d4b02's test case, "drag a tab downward, away from the tab bar, by roughly
60-70px - it should snap back into place rather than detaching."

**Actual**: the tab detaches at 33 px. A window
`Mudlet - Mudlet Tutorial (Detached) - Disconnected` (800x600) appears.

The header constant is indeed 80 (`src/TTabBar.h:176`), but `TTabBar::mouseMoveEvent`
(`src/TTabBar.cpp:459-466`) compares it against the manhattan distance from the tab bar's
**centre**, not from the bar:
```cpp
const QPoint distanceFromBar = globalPos - tabBarGlobalRect.center();
const int distanceFromBarManhattan = distanceFromBar.manhattanLength();
if (distanceFromBarManhattan > DETACH_DISTANCE_THRESHOLD) {
```
For a tab at x≈869 with the bar centred at x≈640 that term is already ≈229 px before any
vertical movement, so the only real gate left is "cursor left the tab bar rect" (~12 px) plus
the 60 % vertical ratio. The 80 px only bites for a tab sitting within 80 px of the bar's centre;
raising 50→80 therefore changed nothing for most tabs. Conversely the first tab (index 0) can
never be dragged out at all - `mudlet::slot_tabDetachRequested` returns early for `index < 1`,
by design ("ensure at least one tab is present in the main window"), so the commit's test case
also cannot be performed on the left-most tab.

**Evidence**: `shots-E2/68-drag60-result.png` and `shots-E2/70-drag-far-result.png` (index-0 tab,
60 px and 265 px drags, no detach - the early return), then the 33 px drag on index 1 produced
`4194491 : Mudlet - Mudlet Tutorial (Detached) - Disconnected : Geometry: 800x600`
(`shots-E2/71-tab2-drag33.png`).

**Commits**: 52b0d4b02.

**Tracker**: no matching issue or PR.

### F-E2-3: Editor settings page clips its theme-update error
**Steps**: open settings (Alt+P) ▸ Editor, with no working network.
**Expected**: the error under the theme preview wraps inside the card.
**Actual**: it is drawn as one line that runs to the card edge and is cut:
`Could not update themes: Error transferring https://github.com/Colorsublime/Colorsublime-Themes/archive`
- the reason (HTTP status) is never visible at the dialog's default width.
**Evidence**: `shots-E2/25-page-228.png`.
**Commits**: 0f70a691f (page layout), pre-existing string.

## Notes (not bugs)

- **N1 - "Mudlet support" is a link, not a page.** The last sidebar entry does not switch the
  content pane; it opens `https://wiki.mudlet.org`. Here that fails with
  `Unable to detect a web browser to launch 'https://wiki.mudlet.org'` (no browser in the
  container). It is visually separated from the pages by a divider, so this reads as intended.
  `shots-E2/33-page-550.png`, `shots-E2/34-mudlet-support-click.png`.
- **N2 - Starter UI on an offline profile.** A brand-new profile ("QA Two") does get
  `mudlet-base-ui` installed (the package directory and `base_ui_settings.lua` are written), but
  with no game and no GMCP/MSDP data nothing of the interface is drawn - consistent with
  21bb616fd's description of the dock being event-driven. `shots-E2/59-profile2-open.png`.
- **N3 - `sysSettingChanged` for the main display font** carries the font *family*, not the size,
  and fires once per spin-box step (6 events for 14→20). Pre-existing key, outside c715215db.
- **N4 - Package manager heading elides hard.** Opened from the detached window, the package
  title rendered as `Mu…` for "Mudlet Tutorial" with the whole card width free.
  `shots-E2/73-detached-alt-o.png`. G1's area; recorded here because it was seen.
- **N5 - The "My games" tab of the connection dialog does not list "Mudlet self-test"** even
  though its details fill the right-hand pane; a normal profile ("QA Two") does appear. Looks
  deliberate (special profile) - E1's area. `shots-E2/54-connection-dialog-dark.png`,
  `shots-E2/62-mygames-tab.png`.

## Coverage

| Commit | Subject | Verdict | How verified |
| --- | --- | --- | --- |
| 0f70a691f | Redesigned settings dialog with search and instant apply | Fixed & verified (one cosmetic: F-E2-3) | All 13 sidebar entries opened and read: `shots-E2/21-page-84.png` … `33-page-550.png`. No empty page, no overlapping widget, no unlabelled control, no sideways scrolling at the default size. Search: `font`, `mute`, `proxy`, `timestamp`, `schriftart`, `dark` - results grouped by category with the hit highlighted, and a clean "No results in settings for …" empty state (`36-search-font.png`, `37-search-mute.png`, `38-search-proxy.png`, `40-search-schriftart.png`). Instant apply: main display font size 14→20 changed the console text immediately with the dialog still open (`43-maindisplay-moved.png` → `44-fontsize-20.png`). Live sync: `setConfig` from Lua flipped `announceIncomingText`/`advertiseScreenReader`, and re-activating the dialog showed the new state (`47-dialog-livesync.png`). Esc applies and closes (`52-after-escape.png`, no "Profile preferences" window left). Lucide credited in `src/dlgAboutDialog.cpp:280`. |
| 7a3174476 | Dismissing the settings no longer turns MMCP auto-accept calls off | Fixed & verified | Opened Preferences (Alt+P) in the self-test profile, worked in it, dismissed it with Esc, then closed Mudlet so the profile was written. `…/Mudlet self-test/current/2026-09-19#15-17-58.xml` still contains `autoAcceptCalls="yes"` (and `allowPeekRequests="no"`). The redesigned Chat and sharing page has no MMCP auto-accept control, as the commit states (`27-page-300.png`). |
| c715215db | Scripts are told when mute and other settings change | Fixed & verified | `registerAnonymousEventHandler("sysSettingChanged", …)` (`work-E2/evt.lua`), then all seven keys driven through `setConfig` (`work-E2/evt2.lua`) and two of them through the Preferences checkboxes. Log (`work-E2/mudlet2.log`): `EVT sysSettingChanged announceIncomingText = false`, `advertiseScreenReader = true` (Preferences path), then `muteMediaAPI = true`, `muteMediaGame = true`, `compactInputLine = true`, `mapperPanelVisible = false`, `enableClosedCaption = true`, `advertiseScreenReader = false`, `announceIncomingText = true` - each event followed by a `SET <key> <old> -> <new>` line proving `getConfig` reads the new value. `mapperPanelVisible` worked with no mapper open. |
| 631bbe1e4 | Profile tab bar clipping and theme switching glitches | Verified no regression | Switched appearance to Dark from the settings while a profile was open (applied instantly, `49-dark-appearance.png`/`50-dark-general.png`), then opened a second profile so two tabs existed. Tab bar renders with no clipped descenders, no grey band between tabs and console, tab text fully visible in dark mode (`64-two-tabs.png`, `68-drag60-result.png`). The macOS-specific half of the fix is platform-only and not verifiable here. |
| 52b0d4b02 | Tab detach threshold silently stayed at 50px instead of 80px | Bug: F-E2-2 | Header constant is 80, but the comparison is against the distance from the tab bar's centre; a 33 px drag on the second tab detached it. The first tab cannot be detached at all (`slot_tabDetachRequested` early-returns for index 0), so the commit's stated test case is not performable on it. |
| 41c41e428 | Discord button works again in a detached window | Could not test (out of time; "Show main toolbar" defaults to Never in this build, so the detached window had no toolbar to carry the Discord button, and wiring the `CI/discord-ipc-fixture.py` plus a game offering an invite did not fit the time box) | - |
| 37152ee0d | Detached windows follow the toolbar icon size setting | Could not test (same reason: no toolbar shown with the default "Show main toolbar: Never"; the Appearance page's "Icon size toolbars" spin box is present and reads 3, `22-page-120.png`) | - |
| c35dcd05f | Fix Alt+W and Alt+O doing nothing in a detached profile window | Fixed & verified | Detached "Mudlet Tutorial" into its own window, activated it, pressed Alt+O - the Package Manager for that profile opened (`73-detached-alt-o.png`). Pressed Alt+W - the detached window closed the profile and went away (`xdotool search --name "Detached"` then returns 0 windows; `74-detached-alt-w.png`). |
| c2387bab2 | Every new profile gets the starter interface, with chat capture | Verified (partial) | A profile created from the connection dialog ("QA Two") has `mudlet-base-ui` and `base_ui_settings.lua` in its profile directory, so the package is installed for an ordinary new profile. Nothing is drawn because the profile is offline (see N2). The chat-capture half needs a game feeding `You chat, "test."` and was not run. |
| 21bb616fd | Starter UI appears at once on a game that already sent data | Could not test (needs a GMCP/MSDP game already mid-session; no such fixture in this run) | Covered by `StarterUiProtocolCatchUpTest` in the ctest run below (passed). |
| ec0fedbb9 | Only the Next button advances the interface tour | Fixed & verified for the mouse; bug on the keyboard: F-E2-1 | On a fresh HOME the tour opened at "1 of 6" and stayed there with no input at 2 s, 6 s and 12 s (`01-tour-t2.png`, `02-tour-t6.png`, `03-tour-t12.png` are byte-identical) - the coordinator's "4 of 6 with no interaction" did **not** reproduce. Four stray clicks on empty overlay area and a 260 px drag across the overlay left it on "1 of 6" (`04-…`, `05-tour-after-drag.png`); only Next advanced it (`06-tour-next1.png`). The intro text says "Use Next or the arrow keys" and no longer says "Click anywhere". |
| 2b9fcd785 | Mudlet Tutorial's give command, hints and intro panels | Verified (partial) | Opened the Mudlet Tutorial profile: the first two intro panels each carry a **Next** button and their text wraps inside the panel with nothing running off the edge (`64-two-tabs.png`, `65-tutorial-next.png`), and a click on empty console area does not advance them (`66-tutorial-stray-click.png`). The `give` wording/hints half needs playing the quest (`north, north, buy apple, …`) and was not run. |
| 54025f766 | Missing colon on toolbar visibility label | Fixed & verified | Appearance page shows "Show menu bar:" and "Show main toolbar:" side by side, both with the colon (`22-page-120.png`). |
| ed9b6ee11 | Placeholder text below WCAG AA in dark mode on older Qt | Verified no regression (contrast not measured) | This build links Qt 6.9.0 (`/opt/qt/6.9.0/gcc_64/lib`), i.e. the pre-6.10 case the fix targets. Placeholder hints were legible in dark mode wherever seen (settings search field "Find in settings", proxy Address/port/username/password fields: `50-dark-general.png`, `38-search-proxy.png`). Per the commit the settings dialog is explicitly out of scope for this palette fix (stylesheet-resolved), and the trigger/alias pattern fields it does target were not sampled pixel-by-pixel here - no measurement, so no claim of the 5.1:1 figure. |
| f75bb43b0 | Update window no longer offers a version that is out of date | Could not test (no update entry: the About menu holds only "About Mudlet", `75-about-menu.png`; no updater UI reachable in this build and no network) | Covered by `FeedChecksumRaceTest` - not in the ctest regex for this area. |
| de041bbeb | Host tells its dialogs about changes through signals | Verified no regression | The settings dialog restyled/refonted live while other dialogs were open: the font change reached the console immediately and the Package Manager opened cleanly from the detached window afterwards. The editor/notepad half of the commit's test case belongs to D1 and was not driven here. |
| 0237e8d46 | New Crowdin updates | Could not test (out of time) | Only "English (American)" was exercised. The interface-language combo is present and populated on the General page (`21-page-84.png`); switching to Czech and Spanish and re-screenshotting the connection dialog, settings and editor was not reached within the time box. |
| 6ced949da | [ImgBot] Optimize images | Verified no regression | All 13 settings category icons render in the sidebar in both light and dark mode, and the password show/hide and game banner images render in the connection dialog (`20-settings-open.png`, `50-dark-general.png`, `56-allgames-dark.png`, `61-connect-dialog2.png`). |
| d48ac5161 | Improve: Fed2 logo image | Verified no regression | Replaces `src/icons/fed2-logo.png` only. The "Federation 2 Community Edition" tile renders in the connection dialog's All games list at the right size, unstretched, with legible text in dark mode (`shots-E2/56-allgames-dark.png`, `shots-E2/61-connect-dialog2.png`). |

41890b480 is A2's and was not touched here.

## Automated
ctest: 34 passed / 0 failed, run as
`QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R 'Settings|StarterUi|Tutorial|FeatureCallout|ExperiencedPlayerGate|DetachedWindow|TabDetachThreshold|MainWindowSizeReport|NewReleaseDialog|ReleaseChangelog|Milestone|Release' --output-on-failure`
(log: `work-E2/ctest.log`). Matches the baseline (220/220). Notably `TabDetachThresholdTest`
passes while F-E2-2 reproduces in the real UI - the test drives
`TTabBar::mouseMoveEvent` from a point near the tab bar's centre, where the 80 px does gate.
Specs: not run for this area (no spec file covers the settings dialog, tour, tab bar or detached
windows); the baseline busted run was 4702/0/0.
