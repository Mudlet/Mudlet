# Plan: a dedicated password field instead of masking the command line

Issue: https://github.com/Mudlet/Mudlet/issues/11024
Branch: `claude/password-masking-architecture-78y96u`

This is an execution plan for coding agents. Read it end to end before touching code. Every
section that says "verify" means: read the code, do not trust this document.

## 1. The one idea

Today a password prompt (`IAC WILL ECHO`) turns the shared main command line into a password box
by painting asterisks over ordinary text (`TCommandLine::paintEvent`). The password is still text
in `TCommandLine`, so history, Tab completion, spell-check, the context menu, `getCmdLine()`,
aliases and `sysDataSendRequest` all see it, and each one needs its own "not while a prompt is
open" guard that reads its own idea of the state. Issue #11024 lists the fifteen or so rounds of
patching that has cost.

The new design has a single structural rule:

> **The password never enters `TCommandLine`.** While the game asks for hidden input, a
> separate `QLineEdit` in password echo mode sits over the command line and takes the keyboard.
> Enter hands its text to one dedicated send path that goes straight to the wire. Nothing that is
> attached to the command line can see the password because the password was never there.

Consequences that fall out for free, with no per-feature guard:

| Leak today | Why it cannot happen after |
| --- | --- |
| history, Up/Down (#10965) | the field has no history |
| Tab / autocomplete rewriting the password (#10971, #10972) | the field has neither |
| typed-ahead command vs. password (#7921, #8127, #10973, #10978) | the command line is not touched, so there is nothing to split or restore |
| copy / cut / drag / "Add to dictionary" (#10966) | Qt's password echo mode refuses copy, cut and drag; the field has no spell-check; its context menu is off |
| `getCmdLine("main")` returning the password | the command line never holds it |
| aliases, `sysDataSendRequest`, the `command` global (#10968) | the field's send path never runs the alias pass and never raises the event |
| screen readers reading the password (#6089) | a native password field is announced as protected |
| the guards disagreeing (widget flag vs. `isRemoteEchoingActive()` vs. `mDisablePasswordMasking`) | one derived state, `Host::passwordEntryWanted()`, read by one consumer |

What it does **not** change: how Mudlet decides a prompt is a password prompt. That stays in
`cTelnet` (WILL/WONT ECHO, the anomaly latch, character-at-a-time recognition, the login-phase
safety timeout, the auto-login). A wrong guess now shows up as a password box the player can
dismiss with Esc instead of silently breaking scripting.

## 2. Where things are today (orientation, verify each)

- `src/TCommandLine.cpp`
  - ctor (~line 72): creates the eye toggle `mpPasswordToggleButton` for `MainCommandLine`; ~line 132
    connects `Host::signal_remoteEchoChanged` to `setEchoSuppression()`.
  - `processNormalKey()` (~138): sets `mUserTypedDuringEchoSuppression`.
  - `enterCommand()` (~1033): history guard `!isRemoteEchoingActive() || mDisablePasswordMasking`.
  - `setEchoSuppression()` (~1715): the save/clear/restore heuristics, ~140 lines.
  - `paintEvent()` (~1857): paints asterisks. `slot_togglePasswordVisibility()`,
    `updatePasswordToggleButton()`, `positionPasswordToggleButton()`, `resizeEvent()` serve the toggle.
  - `event()` (~193): all key handling, including key bindings via `mpKeyUnit->processDataStream()`.
  - `focusInEvent()` (~639): `mpHost->recordActiveCommandLine(this)`.
- `src/TCommandLine.h`: members `mIsEchoSuppressed`, `mPasswordVisible`, `mpPasswordToggleButton`,
  `mTextToRestoreAfterEchoSuppression`, `mRestoredTextShouldBeSelected`, `mUserTypedDuringEchoSuppression`.
- `src/Host.h` / `src/Host.cpp`
  - `mIsRemoteEchoingActive`, `setRemoteEchoingActive()` (emits `signal_remoteEchoChanged`),
    `isRemoteEchoingActive()`; `mDisablePasswordMasking` (bare public bool, written by
    `dlgProfilePreferences` and `XMLimport`).
  - `Host::send()` (~1938): suppresses the local echo of a command while the game echoes (telnet
    semantics, keep), splits on the command separator, runs the alias pass, calls `mTelnet.sendData()`.
- `src/ctelnet.cpp`
  - `sendData(QString&, permitDataSendRequestEvent, isGameCommand)` (~1683): raises
    `sysDataSendRequest`, honours `mAllowToSendCommand`, arms character-mode detection and the mask
    timeout for game commands sent under ECHO.
  - WILL ECHO (~3480) / WONT ECHO (~3620) handling; `checkCharacterModePattern()` (~6416),
    `checkEchoAnomalyPattern()`, `restartPasswordMaskTimeout()`, `slot_passwordMaskTimeout()`.
  - `slot_send_pass()` (~917): auto-login password via `sendData(pass, false)`.
  - `mCharacterModeDetected` (ctelnet.h ~677), read in five places in ctelnet.cpp and by two tests.
- `src/TConsole.cpp`: main command line created ~393 as a child that `layoutLayer2->addWidget()`
  reparents into `layerCommandLine` (~687). Focus proxies: console and both panes → `mpCommandLine`
  (~453, ~459, ~809). `printCommand()` (~2226) skips echo while the game echoes (keep).
- `src/TLuaInterpreter.cpp` `callCmdLineAction()` (~4850): refuses to run a sub command line's Lua
  action while `isRemoteEchoingActive()`. Wrong under the new design; remove.
- `src/TTextEdit.cpp` (~4147): caret mode forwards a printable key to the command line with
  `qApp->sendEvent(mpConsole->mpCommandLine, ...)` - a synthetic key path that bypasses focus.
- `src/TLabel.cpp` (~733): `prompt:` links put text into the command line and focus it.
- `src/mudlet.cpp` (~4071, ~8281): `mpCommandLine->setFocus()` / `repaint()` on profile activation.
- `src/Host.cpp` `setFocusOnHostActiveCommandLine()` (~6195): focuses the last-used command line.
- Tests: `test/functional_tests/CommandLineKeyHandlingTest.cpp` (WINDOW group, has a
  `RecordingTelnetServer`, `type()`, `press()`, `runLua()`, `waitForServerToReceive()`),
  `TelnetPasswordMaskTimeoutTest.cpp` and `TelnetLatePasswordTest.cpp` (TELNET group),
  `src/mudlet-lua/tests/CommandLine_spec.lua` (drives ECHO with `feedTelnet`, needs `--offline`).
- Open PRs that this supersedes or changes: #10965, #10972, #10978 (superseded), #10968 (its
  "scripts keep their aliases" tests stay relevant; its withholding guards become unnecessary).

## 3. Design

### 3.1 Invariants (the contract every piece of code must keep)

1. No code path writes the password into `TCommandLine`, its history, its document or its
   selection. Not on open, not on Enter, not on Esc, not on WONT ECHO, not on timeout.
2. Text in the field is sent by exactly one function, `Host::sendPasswordEntry()`, which calls
   `cTelnet::sendData(text, /*permitDataSendRequestEvent=*/false, /*isGameCommand=*/true)`.
   No alias pass, no `sysDataSendRequest`, no command-separator split, no local echo, no history.
3. When the field closes for any reason, its text is discarded. It is never moved anywhere.
4. Only one function reads the inputs that decide whether the field is up:
   `Host::passwordEntryWanted()`. No feature reads `isRemoteEchoingActive()`,
   `mDisablePasswordMasking` or the character-mode flag to protect a password. (Telnet-semantic
   reads of `isRemoteEchoingActive()` for local echo suppression stay; they are not protection.)
5. While the field is open it is the keyboard focus target for everything that means "the main
   command line": `mpCommandLine->setFocus()`, the console's focus proxies,
   `setFocusOnHostActiveCommandLine()`, and the synthetic key forwarders. Achieved with
   `mpCommandLine->setFocusProxy(field)`, not with per-caller edits.
6. The field exists only while it is wanted. It is created on open and deleted on close, so no
   text, undo state or toggle state carries from one prompt to the next.
7. Everything else about `TCommandLine` behaves exactly as with no prompt open: history, Tab,
   aliases, `sysDataSendRequest`, sub command lines, `callCmdLineAction()`. When the field is not
   shown under ECHO (preference off, Esc, character mode), typed input goes the ordinary way and
   that is by design - the player chose it.

### 3.2 Components

#### A. `Host`: the policy and the send path (`src/Host.h`, `src/Host.cpp`)

```cpp
// The one place the four inputs to "should the password field be up" are read.
bool passwordEntryWanted() const;               // echo && !preferenceOff && !characterMode && !dismissed
void setRemoteEchoingActive(bool);              // exists; on false also clears mPasswordEntryDismissed
void setDisablePasswordMasking(bool);           // new setter; dlgProfilePreferences and XMLimport use it
bool disablePasswordMasking() const;
void setCharacterModeRecognised(bool);          // cTelnet reports here (moved from cTelnet::mCharacterModeDetected)
bool characterModeRecognised() const;
void dismissPasswordEntry();                    // Esc; holds until the game releases ECHO
void sendPasswordEntry(QString);                // invariant 2
signals:
    void signal_passwordEntryWantedChanged(bool);   // emitted only when the derived value changes
```

- Every mutator recomputes `passwordEntryWanted()` and emits the signal on change. Keep
  `signal_remoteEchoChanged` (other listeners may want it; verify with grep, it currently has one
  consumer - the command line - which goes away).
- `mCharacterModeDetected` moves from `cTelnet` to `Host` (`mCharacterModeRecognised`). `cTelnet`
  sets it in `checkCharacterModePattern()`, clears it where it resets `setRemoteEchoingActive(false)`
  (`reset()` ~265-274), and reads it through the getter in `sendOutstandingAutoLoginPassword()`,
  `sendData()`, `checkCharacterModePattern()`, `restartPasswordMaskTimeout()`,
  `slot_passwordMaskTimeout()`. Two tests poke the member directly
  (`TelnetPasswordMaskTimeoutTest.cpp` ~198, ~424) - update them. The point: one flag, no copy.
- `sendPasswordEntry(QString password)`:
  ```cpp
  mUserSentInputThisConnection = true;            // as Host::send() does, for the GMCP auth path
  mTelnet.sendData(password, false, true);        // no event, but a game command: character-mode
                                                  // detection and the mask timeout must see it
  SecureStringUtils::secureStringClear(password);
  ```
  An empty string sends an empty line (some games accept a blank password / "press Enter").
  `denyCurrentSend()` cannot apply because no event is raised; say so in the comment.
- `mDisablePasswordMasking` becomes private with the setter/getter above. The XML attribute name
  `disablePasswordMasking` is unchanged.

#### B. `TPasswordEntry` (new, `src/TPasswordEntry.h`, `src/TPasswordEntry.cpp`)

A `QLineEdit` subclass. Small on purpose: ~150 lines.

- Constructor takes `Host*` (for `KeyUnit` and the closing-down check) and a parent widget.
- `setEchoMode(QLineEdit::Password)`, `setInputMethodHints(Qt::ImhSensitiveData |
  Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase)` (Qt adds `ImhHiddenText` itself),
  `setContextMenuPolicy(Qt::NoContextMenu)`, `setClearButtonEnabled(false)`, `setFrame(true)`.
- Show/hide toggle: `addAction(QIcon(":/icons/password-show-on.png"), QLineEdit::TrailingPosition)`,
  flipping `echoMode()` between `Password` and `Normal` and the icon to `password-show-off.png`.
  Set the action's `text()` and `toolTip()` (`tr("Show password")` / `tr("Hide password")`, with
  `//:` translator comments) so the button has an accessible name.
- Accessible name `tr("Password")`; accessible description explaining Enter sends it straight to
  the game and Esc answers in the command line instead. Placeholder text `tr("Password")`.
- Font: the command line's. Palette: the command line's `mRegularPalette` (public).
- Signals: `submitted(const QString&)`, `dismissed()`.
- Key handling, in `event()` (not only `keyPressEvent`, because Tab is consumed by
  `QWidget::event` before `keyPressEvent`):
  - `Key_Return` / `Key_Enter` with no modifier (keypad modifier allowed for Enter): emit
    `submitted(text())`, `clear()`, accept. Not offered to key bindings, matching the command line.
  - `Key_Escape` with no modifier: `clear()`, emit `dismissed()`, accept.
  - `Key_Tab`, `Key_Backtab`, `Key_Up`, `Key_Down`, `Key_PageUp`, `Key_PageDown`: accept and do
    nothing (no focus change, no history, no completion).
  - Every other `KeyPress`: if `mpKeyUnit->processDataStream(key, modifiers)` returns true, accept
    (a binding ran - an F-key bound to a login alias keeps working); otherwise
    `QLineEdit::event()`.
  - **Always accept the event before returning**, whether or not `QLineEdit` handled it. An ignored
    key event propagates to the parent chain; the field's parent is `layerCommandLine`, never
    `TCommandLine`, but accepting is the belt to that brace.
  - Skip everything above when `mpHost->isClosingDown()`.
- No history, no completion, no spell-check, no drag. Paste (Ctrl+V, middle-click) works - password
  managers need it.
- Destructor: `clear()`. Do not promise more: `QLineEdit`'s internal buffer cannot be scrubbed.

#### C. `TMainConsole`: open, close, focus, geometry (`src/TMainConsole.h`, `src/TMainConsole.cpp`)

The main console owns the field because it owns the layout the field sits in and the focus
proxies that point at the command line. Members: `QPointer<TPasswordEntry> mpPasswordEntry`.

- ctor: `connect(pH, &Host::signal_passwordEntryWantedChanged, this, &TMainConsole::slot_passwordEntryWanted)`.
- `slot_passwordEntryWanted(bool wanted)`: `wanted ? openPasswordEntry() : closePasswordEntry()`.
  Idempotent: opening when open or closing when closed does nothing.
- `openPasswordEntry()`:
  1. `mpPasswordEntry = new TPasswordEntry(mpHost, layerCommandLine)` - a **sibling** of
     `mpCommandLine`, same parent, so `mpCommandLine->geometry()` is directly usable and an
     ignored key event can never bubble into `TCommandLine::event()`.
  2. `mpPasswordEntry->setGeometry(mpCommandLine->geometry()); raise(); show();`
  3. `mpCommandLine->installEventFilter(this)` (or a tiny filter object): on `Resize`/`Move` of the
     command line, copy its geometry again. (`adjustHeight()` can change it while a script prints
     into the command line under the prompt.) Remove the filter on close.
  4. `mpCommandLine->setFocusProxy(mpPasswordEntry)` (invariant 5).
  5. Take focus only if nothing has it or the main command line has it:
     `QWidget* f = QApplication::focusWidget(); if (!f || f == mpCommandLine) mpPasswordEntry->setFocus(Qt::OtherFocusReason);`
     Do not pull focus out of the editor, a sub command line, or a console pane in caret mode; a
     printable key typed there already reaches the field through the proxy (see D).
  6. connect `submitted` → `mpHost->sendPasswordEntry(text)`; `dismissed` → `mpHost->dismissPasswordEntry()`
     (which flips `passwordEntryWanted()` false and closes the field through the signal - one path).
- `closePasswordEntry()`:
  1. `const bool hadFocus = mpPasswordEntry->hasFocus();`
  2. `mpCommandLine->setFocusProxy(nullptr)` **before** the field is deleted.
  3. remove the event filter; `mpPasswordEntry->clear(); mpPasswordEntry->deleteLater(); mpPasswordEntry = nullptr;`
  4. `if (hadFocus) mpCommandLine->setFocus(Qt::OtherFocusReason);`
- Console teardown: `QPointer` plus parent ownership; the field dies with `layerCommandLine`.
  `Host::mpConsole` can be reset (`resetMainConsole`); verify nothing dangles.

#### D. `TCommandLine`: removals plus one structural line (`src/TCommandLine.h`, `src/TCommandLine.cpp`)

Remove: `setEchoSuppression()`, `paintEvent()`, `slot_togglePasswordVisibility()`,
`updatePasswordToggleButton()`, `positionPasswordToggleButton()`, `resizeEvent()`, the toggle
button and the `signal_remoteEchoChanged` connection in the ctor, the six members listed in §2,
the history guard in `enterCommand()`, the tracking line in `processNormalKey()`, and now-unused
includes (`QToolButton`, `QResizeEvent`, `QPainter`; verify each).

Add, at the top of `TCommandLine::event()`:

```cpp
// A widget with a focus proxy is not the keyboard target; Qt already routes real key presses to
// the proxy. Synthetic ones - the caret-mode forwarder in TTextEdit::keyPressEvent() sends
// straight to this widget - must go the same way, or the first character of a password typed
// from the output pane lands here in the clear.
if (QWidget* proxy = focusProxy(); proxy && (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride)) {
    return QApplication::sendEvent(proxy, event);
}
```

That is the only password-related line left in `TCommandLine`, and it is about focus, not
passwords. `focusProxy()` is non-null only while the field is open.

#### E. Small consumers

- `TLuaInterpreter::callCmdLineAction()`: delete the `isRemoteEchoingActive()` refusal. Sub command
  lines never hold the password; refusing their actions during a prompt was collateral.
- `dlgProfilePreferences`: write the preference through `setDisablePasswordMasking()` so toggling it
  mid-prompt opens or closes the field. Reword the checkbox text and tooltip in
  `src/ui/profile_preferences.ui` to describe the field (suggested: "Do not show a password field
  when the game asks for hidden input"); keep the object name and the search synonyms.
- `XMLimport`: use the setter (no signal side effects matter at load time, but keep one write path).
- `cTelnet`: only the flag move in A. No change to negotiation, timers, anomaly latch or auto-login.

### 3.3 Behaviour matrix

| Situation | What happens |
| --- | --- |
| WILL ECHO arrives, command line holds a typed-ahead command | field opens over the line and takes focus; the command stays in the line, untouched and hidden behind the field until the prompt ends |
| Player types password, Enter | text goes out via `sendPasswordEntry()`; field clears and stays open until the game releases ECHO (RFC 857: the game is still echoing) |
| Game sends WONT ECHO | field closes; any text in it is discarded; focus returns to the command line if the field had it; the typed-ahead command is still there |
| Password rejected, game re-prompts with ECHO still held | field is still up; the retry is protected (this is the case that sinks the "close on Enter" alternative, see §4) |
| Password rejected, game toggles WONT then WILL | field closes and a fresh one opens; dismissal state was cleared by the WONT |
| Player presses Esc | text discarded, field closes, focus to the command line; no field again until the game releases ECHO and asks again; meanwhile typed input is ordinary (aliases, history, event) - the player said "this is not a password prompt" |
| Auto-login | WILL ECHO opens the field; `slot_send_pass()` sends the stored password by its own path as today; WONT closes the field. Typed-ahead text in the command line survives (#7921) |
| A trigger/script/key binding sends the password | goes through `Host::send()` as today, alias pass and all (#10968's requirement); the field closes when the game releases ECHO |
| F-key bound to a login alias pressed while the field has focus | forwarded to `KeyUnit`, binding runs |
| Game holds ECHO for the session and negotiates SGA (GoMud style) | first line goes out through the field; character-mode recognition (~3 s after that line) flips `passwordEntryWanted()` false, field closes with its existing warning; ordinary input from then on |
| Game holds ECHO, no SGA | field stays until Esc, the 60 s login-phase timeout after the last line sent, or disconnect - all existing behaviour, now visible and escapable |
| ECHO anomaly latch (5 toggles in 5 s) | cTelnet refuses ECHO as today; no field; input in the clear, as today |
| Preference "disable" toggled on mid-prompt | field closes, text discarded; typed input ordinary, history works (#8902) |
| Preference toggled off mid-prompt | field opens |
| Sub command line / miniconsole command line during a prompt | untouched, and its Lua action now runs (E) |
| Script calls `printCmdLine("main", ...)`, `clearCmdLine`, `getCmdLine("main")` during a prompt | operate on the command line; never see the field |
| `prompt:` link clicked during a prompt | text goes into the command line (hidden behind the field until it closes); focus lands on the field via the proxy |
| Caret mode: printable key on the output pane during a prompt | `TTextEdit` forwards to the command line; the proxy redirect in D hands it to the field |
| Profile tab switched away and back, or detached into its own window | field is a child of the console's `layerCommandLine`, so it moves and hides with it; `activateProfile()`'s `setFocus()` on the command line lands on the field |
| Main console reset (`resetMainConsole`) or profile closed with the field open | field dies with its parent; `Host` state is per profile |
| Two profiles, one at a prompt | each `Host`/`TMainConsole` pair has its own state and field |
| Password contains the command separator (`;;`) | sent whole (an improvement: `Host::send()` would have split it) |
| Paste into the field | works |
| Empty field, Enter | empty line sent |

### 3.4 What stays in cTelnet, and why the plan does not touch it

`isRemoteEchoingActive()` still means "the game echoes" and `Host::send()` /
`TConsole::printCommand()` still skip the local echo while it is true. That is RFC 857 behaviour,
not password protection, and it applies whether or not the field is shown. The anomaly latch,
character-mode recognition, the login-phase timeout and the late-password logic are inputs to the
policy, unchanged. Anyone tempted to "improve" them in this PR: do not; that is #10967-land and a
separate conversation with the people who own it.

## 4. Decisions and rejected alternatives

1. **Field stays open until the game releases ECHO, rather than closing on Enter.** The issue text
   says "then the field goes away". Closing on Enter would leave the *retry* after a rejected
   password in the clear on games that hold ECHO across attempts - typed into the command line, in
   history, through aliases and `sysDataSendRequest`: every leak this design exists to remove. The
   cost of staying open: on a slow link, text typed after the password but before WONT arrives is
   discarded when the field closes. Visible, harmless, and exactly what today's code does (it
   `clear()`s on unsuppress). "Field up ⇔ game says it is echoing" is also the only rule with no
   heuristic in it.
2. **Sibling of the command line, not a child.** A child would receive geometry for free, but an
   ignored key event bubbles from a child to `TCommandLine::event()`, which would run
   `historyMove()` on Up or insert text on anything `QLineEdit` does not handle. Fifteen lines of
   geometry tracking are cheaper than that class of bug.
3. **Focus proxy, not per-caller edits.** There are at least six places that focus the main
   command line by pointer. `setFocusProxy()` makes all of them right without touching them.
4. **Fresh widget per prompt.** Deleting on close is the simplest proof that nothing (text, undo,
   reveal toggle) carries over. The cost is a widget allocation per prompt, i.e. nothing.
5. **`sendData(..., isGameCommand=true)`.** The auto-login sends with `false` and arms the timeout
   itself. A typed password must keep arming character-mode detection and the safety timeout
   exactly as a line from the command line did, or the GoMud-style recognition regresses.
6. **No Lua API.** No `sysPasswordEntry*` events, no getter. Nothing needs them; add later if a
   package does. (`getCmdLine("main")` not returning the password is the point.)
7. **Key bindings run from the field; Return and Escape do not reach them.** Same as the command
   line, where an unmodified Return/Escape is never offered to `KeyUnit`.
8. **Character-mode flag lives in Host.** Two copies of one fact is how the current guards drifted.
9. **Not implemented on purpose:** Ctrl+C copying the *console* selection from the field, Ctrl+F,
   PageUp/PageDown scrolling the console. The field is a password box, not a second command line.
   Esc gets the player the real one in one keystroke.

## 5. Work breakdown

One PR, in commits that each build and pass `ctest` on their own. Read
`.agents/skills/build-mudlet/SKILL.md` before building; use the `linux-debug-nosan` preset that is
already configured in `build-linux-debug-nosan/`. Run `clang-format -i` on every C++ file touched.
Commit trailers per `CLAUDE.md`: `Assisted-by: Claude:<model id>`; **no** `Signed-off-by` - ask the
human.

### Commit 1 - Host policy and send path (no behaviour change yet)

Files: `src/Host.h`, `src/Host.cpp`, `src/ctelnet.h`, `src/ctelnet.cpp`,
`src/dlgProfilePreferences.cpp`, `src/XMLimport.cpp`,
`test/functional_tests/TelnetPasswordMaskTimeoutTest.cpp`.

- Add everything in §3.2 A. Move the character-mode flag. Route the preference through the setter.
- `TCommandLine` keeps working as before at this commit (it still listens to
  `signal_remoteEchoChanged`).
- Test (functional, TELNET group, new file `HostPasswordEntryPolicyTest.cpp` or a case added to
  `TelnetPasswordMaskTimeoutTest`): drive `setRemoteEchoingActive`, `setDisablePasswordMasking`,
  `setCharacterModeRecognised`, `dismissPasswordEntry` and assert `passwordEntryWanted()` and the
  signal count (one emission per change of the derived value, none for a no-op). Assert a WONT
  clears the dismissal. Assert `sendPasswordEntry("x")` reaches the server, raises no
  `sysDataSendRequest` (Lua handler records nothing), and runs no alias whose pattern is `^x$`.

### Commit 2 - The field, and the masking goes

Files: new `src/TPasswordEntry.{h,cpp}`, `src/CMakeLists.txt` (add sources; check `mudlet.pro`
is gone - it is - and any other source list), `src/TMainConsole.{h,cpp}`,
`src/TCommandLine.{h,cpp}`, `src/TLuaInterpreter.cpp`, `src/ui/profile_preferences.ui`,
`src/dlgProfilePreferences.cpp` (label text only).

- Implement B, C, D, E. This is one commit because the removal in D is what makes the command line
  untouched; splitting it would leave a commit where both mechanisms fire.
- Update `src/mudlet-lua/tests/CommandLine_spec.lua` in the same commit (the current cases assert
  the *old* behaviour and would go red):
  - "keeps the command line untouched while the game asks for a password": `printCmdLine("main", x)`,
    WILL ECHO, `getCmdLine("main") == x`; sub command line untouched; WONT ECHO, still `x`.
  - "a script can still print into the command line during a prompt": WILL ECHO,
    `printCmdLine("main", y)`, `getCmdLine("main") == y`, WONT, still `y`.
  - keep the anomaly-budget comment: at most four ECHO toggles per describe block.
  - the spec cannot see the field (no Lua access, by design) - say so in a comment.

### Commit 3 - Functional tests for the field

File: `test/functional_tests/CommandLineKeyHandlingTest.cpp` (WINDOW group; it already has the
recording server and key helpers). If the class gets unwieldy, a new `PasswordEntryTest.cpp` in the
same group is fine - it must join the group, not be a standalone binary (see the
`*_GROUP_TEST_SOURCES` lists and the note about link cost in `CLAUDE.md`).

Helpers to add: `serverEcho(bool)` through `mpHost->mTelnet.loopbackTest()` (see #10978's diff
for a working version), `passwordEntry()` returning `mpHost->mpConsole->findChild<TPasswordEntry*>()`.
Respect the anomaly budget: at most four ECHO toggles per case, and `cleanup()` must
`setRemoteEchoingActive(false)` so a failed case cannot mask the next.

Cases (each must be shown to fail when the line it protects is reverted - note which line in the
test's comment):

1. WILL ECHO opens a `TPasswordEntry` over the command line with the same geometry; the command
   line's text (typed ahead) is unchanged; `QApplication::focusWidget()` is the field.
2. Type + Return in the field: server receives the text; command line text unchanged; Up in the
   command line afterwards recalls the previous command, not the password; a `sysDataSendRequest`
   handler saw nothing; an alias `^<password>$` did not fire; Lua `command` global unchanged.
3. Field stays open after Return until WONT; WONT closes it and focus returns to the command line.
4. WONT while the field holds text: nothing sent, text gone, command line unchanged.
5. Esc: text gone, field gone, focus on the command line; typing a line now goes the ordinary way
   (an alias fires) although ECHO is still on; WONT then WILL opens a fresh field.
6. Preference on: no field on WILL ECHO; typed line reaches history (#8902).
7. Character-mode recognition closes the field: server WILL SGA + WILL ECHO, submit a line, wait
   for `characterModeRecognised()` (~3 s, `QTRY_VERIFY` with a 5 s timeout), field is gone, no
   further field while ECHO is held.
8. A key binding fires from the field (`permKey` on F7 doing `send("frombinding")`), the server
   receives it, and the field is still open.
9. Reveal toggle: triggering the trailing action flips `echoMode()`.
10. Up/Down/Tab in the field: text unchanged, focus unchanged.
11. Synthetic key path: `qApp->sendEvent(mpCommandLine, keyPress('a'))` while the field is open ends
    with `a` in the field and the command line unchanged (the D redirect).
12. `mpCommandLine->setFocus()` while the field is open lands on the field (the proxy).
13. Resize the console; the field's geometry follows the command line's.
14. Sub command line action runs during a prompt (E).
15. Paste: clipboard text + Ctrl+V lands in the field and Return sends it.
16. Closing the profile with the field open does not crash (add to the TEARDOWN group if that is
    where such cases live; verify).

Existing tests that must still pass unchanged: `TelnetLatePasswordTest`, the six cases in
`TelnetPasswordMaskTimeoutTest` (with the flag rename), everything else in the WINDOW group.

### Commit 4 - Wording and docs

- Preference label/tooltip (§3.2 E). Translator comments on every new `tr()`.
- If `docs/` or the wiki text in the repo mentions password masking, update it; the manual lives on
  the wiki - note the needed wiki edit in the PR body.
- PR title (Danger enforces the prefix): `Improve: Passwords are typed into a dedicated field
  instead of a masked command line`. Body per `.github/PULL_REQUEST_TEMPLATE.md`, with a
  `**Test case:**` line, and a demo video per `docs/demo-videos.md` if feasible (before: asterisks
  over the command line; after: the field).
- In the PR body list #10965, #10972, #10978 as superseded and #10968 as reduced to its
  "scripts keep aliases" tests, and say why.

## 6. Test strategy notes

- Specs first where Lua can see the behaviour (the command line being untouched); functional
  tests for everything key-driven or about focus/geometry, since Lua cannot press keys.
- Both harnesses fail silently when set up wrong: for every new case, break the code once and
  watch the case go red before trusting it. Record which line was broken in the case's comment.
- ECHO anomaly budget: five toggles in five seconds latch the process. Four per case, and never
  rely on an earlier case having left ECHO in any state.
- `feedTelnet()` in specs needs the self-test profile started with `--offline`; the README in
  `src/mudlet-lua/tests/` says how, and `.claude/scripts/run-lua-tests.sh` runs the suite.

## 7. Risks and open questions (for the reviewer and the red team)

1. `QWidget::setFocusProxy()` while the widget has focus: Qt moves the application focus widget to
   the new proxy but may not send focus events; hence the explicit `setFocus()` in C.5. Verify on
   all three platforms that the field shows a caret and receives keys after opening.
2. `QLineEdit` in `Password` echo mode: confirm with a test that `copy()` and `cut()` are no-ops and
   that dragging is refused (Qt documents copy/cut; drag is believed to be Normal-mode only).
3. The 31 px min/max height on `layerCommandLine` (TConsole.cpp ~487) vs. a command line that has
   grown to several rows via `adjustHeight()`: the field copies the command line's geometry, so it
   grows with it, but check it is not clipped by the layer.
4. Stylesheets: `setCmdLineStyleSheet("main", ...)` styles a `QPlainTextEdit`; the field will not
   pick that up. It uses the command line's palette and font, which covers the common case
   (background/foreground colours). Document as a known cosmetic limit.
5. Games that ask for a *non-secret* line under ECHO (a name prompt on a server that holds ECHO
   from connect): the player sees a password box for their name. Esc, or character-mode
   recognition after the first line, resolves it; and today the same server masks the same line.
6. Players who type an alias name (`pw`) at the prompt on purpose: the field sends `pw` literally.
   Esc gets them the command line; the preference removes the field entirely. The tooltip and the
   accessible description must say so. Consider a one-time `[ INFO ]` line the first time a field
   opens in a profile explaining Esc (cheap, discoverable; decide in review).
7. Focus stealing when the player is in the editor when a prompt arrives: by C.5 the field does not
   take focus then. Confirm that is the behaviour people want versus always focusing.
8. Mudlet Web shares the specs; the rewritten `CommandLine_spec.lua` cases use `feedTelnet`, which
   Web presumably stubs or skips as it does today. Check how the current password cases are handled
   there before relying on the new ones.
9. The move of `mCharacterModeDetected` touches `cTelnet`, which is under active work in other PRs
   (#10964 and friends). Keep the diff there mechanical and small to ease rebases.
