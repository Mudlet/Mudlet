# Plan: a dedicated password field instead of masking the command line

Issue: https://github.com/Mudlet/Mudlet/issues/11024
Branch: `claude/password-masking-architecture-78y96u`
Status: v2, revised after a five-angle red-team review (see §9 for what changed and why).

This is an execution plan for coding agents. Read it end to end before touching code. Every
line reference is an orientation aid, not a fact: verify against the tree you are on.

## 1. The one idea

Today a password prompt (`IAC WILL ECHO`) turns the shared main command line into a password box
by painting asterisks over ordinary text (`TCommandLine::paintEvent`). The password is still text
in `TCommandLine`, so history, Tab completion, spell-check, the context menu, `getCmdLine()`,
aliases and `sysDataSendRequest` all see it, and each needs its own "not while a prompt is open"
guard reading its own idea of the state. Issue #11024 lists the fifteen-odd rounds that has cost.

The new design has one structural rule:

> **The password never enters `TCommandLine` as a password.** While the game asks for hidden
> input, a separate `QLineEdit` in password echo mode sits over the command line and takes the
> keyboard. Enter hands its text to one dedicated send path that goes straight to the wire.
> Nothing attached to the command line can see the password because the password was never there.

What falls out, with no per-feature guard:

| Leak today | Why it cannot happen after |
| --- | --- |
| history, Up/Down (#10965) | the field has no history |
| Tab / autocomplete rewriting the password (#10971, #10972) | the field has neither |
| a typed-ahead command being sent as part of the password, or dropped (#7921, #8127, #10973, #10978) | the command line is not emptied or restored, so there is nothing to guess |
| copy / cut / drag / "Add to dictionary" (#10966) | while masked, Qt's password echo mode refuses copy, cut and drag; the field swallows the Copy/Cut/Undo shortcuts even when revealed; it has no spell-check; its context menu offers Paste only |
| `getCmdLine("main")` returning the password | the command line never holds it |
| aliases, `sysDataSendRequest`, the `command` global (#10968) | the field's send path never runs the alias pass and never raises the event |
| screen readers reading the password (#6089) | a native password field is announced as protected |
| the guards disagreeing (widget flag vs. `isRemoteEchoingActive()` vs. `mDisablePasswordMasking`) | one derived state, `Host::passwordEntryWanted()`, read by one consumer |

What it does **not** change: how Mudlet decides a prompt is a password prompt. That stays in
`cTelnet` (WILL/WONT ECHO, the anomaly latch, character-at-a-time recognition, the login-phase
safety timeout, the auto-login timers). The field is a *view* of that decision. A wrong guess now
shows up as a visible box the player can step past, instead of silently breaking scripting.

What it also does not fix, stated up front so nobody claims otherwise in the PR:

- A password typed and submitted *before* WILL ECHO is processed (the fast-typist, laggy-link
  case) goes through `Host::send()` in the clear, exactly as today. The window is one round trip.
  Nothing without a heuristic can help, and heuristics are what this design removes.
- Memory hygiene. Copies of the plaintext exist in Qt's line control, the encoder's byte array,
  the socket buffers and TLS. Qt zero-fills its own buffer when a password-mode line edit is
  destroyed; that is the extent of it. Do not write "secure" anywhere in the code.

## 2. Where things are today (orientation, verify each)

- `src/TCommandLine.cpp`
  - ctor (~72): creates the eye toggle `mpPasswordToggleButton`; ~132 connects
    `Host::signal_remoteEchoChanged` to `setEchoSuppression()`.
  - `processNormalKey()` (~138-156): sets `mUserTypedDuringEchoSuppression`.
  - `mousePressEvent()` (~984-990): refuses selection/drag while masked, keeps right-click Paste.
  - `enterCommand()` (~1033): emits `commandSubmitted()`, then the history guard
    `!isRemoteEchoingActive() || mDisablePasswordMasking` (~1062).
  - `setEchoSuppression()` (~1715-1855): the save/clear/restore heuristics.
  - `paintEvent()` (~1857), `slot_togglePasswordVisibility()`, `updatePasswordToggleButton()`,
    `positionPasswordToggleButton()`, `resizeEvent()`: the mask and its toggle.
  - `event()` (~193-637): all key handling. Note ShortcutOverride claims for the caret shortcut and
    for a user binding on the profile-switch shortcut (~200-219); Tab/F6 turning caret mode on
    (~306-341); Ctrl+digit tab switching (`handleCtrlTabChange`); Escape = select all (~518-528);
    PageUp/PageDown scrolling the console; keypad keys offered to `KeyUnit` first (~273).
  - `focusInEvent()` (~639-653): `mpHost->recordActiveCommandLine(this)`; `mousePressEvent` /
    `mouseReleaseEvent` call `mudlet::self()->activateProfile(mpHost)`.
- `src/TCommandLine.h`: members `mIsEchoSuppressed`, `mPasswordVisible`, `mpPasswordToggleButton`,
  `mTextToRestoreAfterEchoSuppression`, `mRestoredTextShouldBeSelected`,
  `mUserTypedDuringEchoSuppression`; `mRegularPalette` is public. `Host.h` includes this header.
- `src/Host.h` / `src/Host.cpp`
  - `mIsRemoteEchoingActive`, `setRemoteEchoingActive()` (~6376, emits `signal_remoteEchoChanged`,
    whose only listener is the command line), `isRemoteEchoingActive()`.
  - `mDisablePasswordMasking` (~699): a bare public bool. Readers/writers: `TCommandLine.cpp` ~1062
    and ~1724, `ctelnet.cpp` ~6489, `dlgProfilePreferences.cpp` ~4046 and ~6509, `XMLimport.cpp`
    ~780 (through a `bool&` lambda parameter), `XMLexport.cpp` ~409,
    `test/functional_tests/SettingsRoundTripTest.cpp` ~333/354/419,
    `TelnetPasswordMaskTimeoutTest.cpp` ~197/214/381.
  - `Host::send()` (~1938): skips the local echo while the game echoes (RFC 857, keep), splits on
    the command separator, runs the alias pass, calls `mTelnet.sendData()`.
  - `caretShortcutMatches()` (~6179), `setCaretEnabled()`, `setFocusOnHostActiveCommandLine()`
    (~6195, focuses `activeCommandLine()` with 0/10/50 ms retries), `recordActiveCommandLine()`,
    `mUserSentInputThisConnection` (Host.h ~1226), `writeProfileData()`/`readProfileData()` (~426).
  - `mAllowToSendCommand` (Host.h ~702) set false by Lua `denyCurrentSend()`
    (`TLuaInterpreter.cpp` ~756); read and reset by `sendData()`.
- `src/ctelnet.cpp` / `src/ctelnet.h`
  - `sendData(QString&, permitDataSendRequestEvent, isGameCommand)` (~1683-1790): raises
    `sysDataSendRequest` when permitted; honours `mAllowToSendCommand`; **posts an encoding warning
    that quotes the data** (~1701-1705 and ~1726-1731); for game commands resets
    `mAutoLoginPasswordOutstanding` (~1747-1754), arms character-mode detection (~1766-1779) and the
    mask timeout (~1781-1783). The only callers that withhold the event are the two auto-login
    password sends (~925, ~986).
  - WILL ECHO (~3470-3495): a repeated WILL while ECHO is already on is ignored (~3475).
    WONT ECHO (~3620-3645). WILL SGA (~3453-3457) sets `mServerRequestedSGA` even though Mudlet
    refuses SGA.
  - `checkCharacterModePattern()` (~6416-6448): sets `mCharacterModeDetected`, raises
    `sysCharacterModeDetected`, posts a warning at most three times ever and never to experienced
    players (`mudlet::showCharacterModeWarning()`). Its own comment calls it advisory.
    `mCharacterModeDetected` is read at ~974, ~1766, ~6433, ~6489, ~6505 and cleared only in
    `reset()` (~274, alongside `setRemoteEchoingActive(false)` at ~265). It is poked by
    `TelnetPasswordMaskTimeoutTest.cpp` ~198/424.
  - `checkEchoAnomalyPattern()` (~6451-6470): the count rises while consecutive toggles are less
    than 5 s apart and only resets after a longer gap; five latch the process until `reset()`.
    WONT counts too.
  - `restartPasswordMaskTimeout()` (~6482-6501): 60 s, restarted by every line sent under ECHO,
    armed only inside the first 5 min of a connection; `slot_passwordMaskTimeout()` (~6503) sends
    DONT ECHO and clears the echo state.
  - Auto-login: `slot_send_login()` (~904) starts `mTimerPass`; `slot_send_pass()` (~916) sends via
    `sendData(pass, false)`; `sendOutstandingAutoLoginPassword()` (~943) sends a keychain password
    that arrived late while `stillAtPrompt` (~974). `mTimerLogin`/`mTimerPass` (ctelnet.h ~586).
  - `friend class Host` (ctelnet.h ~384); tests are friends too (~363-379).
  - `loopbackTest()` is public; WILL/WONT ECHO through it exercise the real negotiation.
- `src/TConsole.cpp` / `src/TConsole.h`: `mpCommandLine`, `layerCommandLine`, `layoutLayer2`,
  `mpButtonMainLayer` are all public. The main command line is created ~393 and reparented into
  `layerCommandLine` by `layoutLayer2->addWidget()` ~687; `mpButtonMainLayer` moves on to
  `commandSplitter` ~720, so the command line is the layer's only child widget. `adjustHeight()`
  (TCommandLine ~701-740) sets the layer's min/max height, so the 31 px at ~487 is only initial.
  Focus proxies: console and both panes → `mpCommandLine` (~453, ~459, ~809). `setProxyForFocus()`
  (~3199-3208) sets them and fires a `QAccessible::Focus` event for the command line.
  `printCommand()` (~2226) skips echo while the game echoes (keep).
- `src/TMainConsole.cpp`: ctor ~25-70 (has connects); `resetMainConsole()` (~525-574) removes
  docks, sub command lines and labels, **not** `layerCommandLine`. The only `new TMainConsole` is
  `mudlet.cpp` ~3885.
- `src/TLuaInterpreter.cpp` `callCmdLineAction()` (~4852-4860): refuses a sub command line's Lua
  action while `isRemoteEchoingActive()`.
- `src/TLuaInterpreterUI.cpp` (and the copy of the `COMMANDLINE` macro in
  `TLuaInterpreterMudletObjects.cpp` ~206): `printCmdLine`, `appendCmdLine`, `clearCmdLine`,
  `selectCmdLineText` write to `TCommandLine` directly; `sendCmdLine` goes through
  `Host::sendCmdLine()` → `TMainConsole::setCommandLineText()` (~1174). MXP `prompt:` links are
  turned into `sendCmdLine(...)` (`TBuffer.cpp` ~4084-4088). `TLabel.cpp` ~731-745 handles a
  label's own `prompt:` scheme by calling `setPlainText` on the command line directly.
- `src/TTextEdit.cpp` (~4140-4155): caret mode forwards a printable key with
  `mpHost->setFocusOnHostActiveCommandLine()` then `qApp->sendEvent(mpConsole->mpCommandLine, ...)`.
- `src/mudlet.cpp`: `activateProfile` ~4070 (`mpCommandLine->setFocus()`), ~8281 (`repaint()`),
  `changeEvent` ~8795-8812 (remembers `QApplication::focusWidget()` across deactivation in a
  `QPointer`), profile-switch shortcuts ~2278, `announce()` (mudlet.h ~190),
  `experiencedMudletPlayer()`.
- Tests: `test/functional_tests/CommandLineKeyHandlingTest.cpp` (WINDOW group; recording server,
  `type()`, `press()`, `runLua()`, `waitForServerToReceive()`; its
  `test_aPasswordIsNotKeptInTheHistory` ~486-500 asserts the history guard this plan removes);
  `TelnetPasswordMaskTimeoutTest.cpp` (TELNET group; its `init()` ~170-205 shows how to reset the
  anomaly counters, timers, SGA flag and ECHO state between cases through friend access);
  `TelnetLatePasswordTest.cpp`; `HostChildTeardownTest.cpp` (TEARDOWN group, `forceClose()` /
  `deleteHost()` patterns ~259-301); `src/mudlet-lua/tests/CommandLine_spec.lua` (drives ECHO with
  `feedTelnet`, needs `--offline`; its two prompt cases pin today's clear/restore behaviour).
  CI runs ctest under `QT_QPA_PLATFORM=offscreen`; the harness never shows the main window.
- Open PRs this supersedes or changes: #10965, #10972, #10978 (superseded); #10968 (its
  "scripts keep their aliases at a prompt" tests stay valid; its withholding guards are unnecessary);
  #10964 (touches the auto-login lines near ~974; keep the cTelnet diff here small).

## 3. Design

### 3.1 Invariants (the contract every piece of code must keep)

1. No code path writes text *out of* the field into `TCommandLine`, its history, its document or
   its selection. Not on Enter, not on Esc, not on WONT ECHO, not on timeout, not on close.
   (Text may move *into* the field from the command line when it opens, see B.7; that is text the
   player already saw on screen, going into protection.)
2. Text in the field is sent by exactly one function, `Host::sendPasswordEntry()`, which calls
   `cTelnet::sendData(text, /*permitDataSendRequestEvent=*/false, /*isGameCommand=*/true)`.
   No alias pass, no `sysDataSendRequest`, no command-separator split, no local echo, no history.
3. `QLineEdit::text()` is called in exactly one place in the whole tree: the Return branch of
   `TPasswordEntry`. There is no accessor, no text-carrying signal, and no close path reads it.
   A grep for `->text()` on the field in any other file must come back empty; write that grep into
   the PR body's test case.
4. Only one function reads the inputs that decide whether the field is up:
   `Host::passwordEntryWanted()`. No feature reads `isRemoteEchoingActive()`, the masking
   preference, the character-mode flag or the dismissal flags to protect a password.
   (Telnet-semantic reads of `isRemoteEchoingActive()` that suppress local echo, and cTelnet's own
   reads of its own flags, stay; §3.4 lists them.)
5. While the field is open it is the keyboard target for everything that means "the main command
   line": `mpCommandLine->setFocus()`, the console's focus proxies,
   `setFocusOnHostActiveCommandLine()`, and the synthetic key forwarders. Achieved with
   `mpCommandLine->setFocusProxy(field)` plus one redirect line, not per-caller edits.
6. The field exists only while it is wanted. It is created on open and deleted on close, so no
   text, undo state or reveal state carries from one prompt to the next. It is deleted in password
   echo mode, so Qt zero-fills its buffer.
7. Everything else about `TCommandLine` behaves exactly as with no prompt open: history, Tab,
   aliases, `sysDataSendRequest`, sub command lines, `callCmdLineAction()`. When no field is shown
   under ECHO, typed input goes the ordinary way. That is by design in every such case except one,
   which §3.3 names: after character-at-a-time recognition on a game that was in fact masking a
   line-mode password, a *retry* is typed in the clear, visibly.

### 3.2 Components

#### A. `Host`: the policy and the send path (`src/Host.h`, `src/Host.cpp`)

```cpp
// The one place the inputs to "should the password field be up" are read. Five inputs; one
// derived value; one signal, emitted only when the derived value changes.
bool passwordEntryWanted() const;
//   = mIsRemoteEchoingActive
//     && !mDisablePasswordMasking
//     && !mPasswordEntrySuppressed          // until the game releases ECHO
//     && !mPasswordEntryDismissed           // until the next line leaves the main command line
//     && !mTelnet.autoLoginAboutToAnswer(); // login or password timer running with credentials

void setRemoteEchoingActive(bool);            // exists; on false also clears both flags below
void setDisablePasswordMasking(bool);         // new; the only write path for the preference
bool disablePasswordMasking() const;
void suppressPasswordEntryUntilEchoReleased(); // the auto-login answered, or recognition fired
                                               // while no field was open
void suppressPasswordEntryAfterNextLine();     // recognition fired while a field is open
void dismissPasswordEntry();                   // Esc on an empty field
void clearPasswordEntryDismissal();            // a line left the main command line
void recomputePasswordEntryWanted();           // cTelnet calls this when a timer input changes
bool sendPasswordEntry(QString);               // invariant 2; returns sendData()'s result
signals:
    void signal_passwordEntryWantedChanged(bool);
```

- Every mutator recomputes and emits on change. `mPasswordEntrySuppressed`,
  `mPasswordEntryDismissed` and a private `mPasswordEntrySuppressAfterNextLine` are cleared by
  `setRemoteEchoingActive(false)`, which `cTelnet::reset()` calls on every connect and disconnect,
  so nothing outlives a connection.
- `sendPasswordEntry(QString text)`:
  ```cpp
  mUserSentInputThisConnection = true;   // as Host::send() does, for the GMCP auth path
  // No event was raised for this line, so nothing could have denied it. A denyCurrentSend()
  // called outside a handler leaves a stale refusal that sendData() would otherwise apply here
  // and drop the password without a word.
  mAllowToSendCommand = true;
  // isGameCommand stays true on purpose: a typed password must keep arming character-at-a-time
  // detection and the mask safety timeout, and must keep cancelling a late keychain password
  // (sendData ~1747), exactly as a line from the command line did.
  const bool sent = mTelnet.sendData(text, false, true);
  mTelnet.abandonAutoLogin();            // the player answered; no timer or late password may too
  if (mPasswordEntrySuppressAfterNextLine) { mPasswordEntrySuppressed = true; ... recompute }
  return sent;
  ```
  An empty string sends an empty line (a blank password, or "press Enter to continue").
- `dismissPasswordEntry()`: sets `mPasswordEntryDismissed`; also applies a pending
  suppress-after-next-line, since Esc ends the line too.
- `mDisablePasswordMasking` becomes private behind the getter/setter. Every site in §2 changes:
  readers use the getter, `XMLimport` reads into a local and calls the setter, tests call the
  setter. The XML attribute name `disablePasswordMasking` is unchanged.
- Remove `signal_remoteEchoChanged` in the commit that removes its only listener.

#### B. `TPasswordEntry` (new, `src/TPasswordEntry.h`, `src/TPasswordEntry.cpp`)

A `QLineEdit` subclass. Keep it small; the red team estimates ~200 lines with headers.

1. Constructor takes `Host*` (held as `QPointer<Host>`, KeyUnit fetched per event as
   `TCommandLine.h` ~129 does), the `TCommandLine*` it stands in for, and the parent widget.
2. `setEchoMode(QLineEdit::Password)`. Then OR the hints in, and again after every echo-mode
   change: `setInputMethodHints(inputMethodHints() | Qt::ImhHiddenText | Qt::ImhSensitiveData |
   Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase)`. (`setEchoMode(Normal)` clears them.)
   `setContextMenuPolicy(Qt::PreventContextMenu)` for the default menu; `contextMenuEvent()` shows
   a menu with Paste only, because today's masked line keeps right-click Paste and password-manager
   users need it. `setClearButtonEnabled(false)`, `setFrame(true)`, `setDragEnabled(false)`.
3. Reveal toggle: `addAction(QIcon(qsl(":/icons/password-show-on.png")), QLineEdit::TrailingPosition)`
   flipping between `Password` and `Normal`, icon to `password-show-off.png`, hints re-applied.
   Set the action's `text()` and `toolTip()` (`tr("Show password")` / `tr("Hide password")` with
   `//:` translator comments). The icon button Qt makes cannot take focus; that matches today's
   toggle. Revealed text can be mouse-selected into the X11 primary selection; that is what reveal
   means, and it is written down in §7.
4. Accessible name and description follow the `signal_adjustAccessibleNames` pattern
   (`TCommandLine.cpp` ~1504): name `tr("Hidden input")` (with the profile name when several are
   open), description saying Enter sends it straight to the game without aliases or history, and
   Esc empties it and, when it is already empty, closes it to use the command line instead.
   Placeholder text before the first submit: `tr("Hidden input - Esc for the normal command line")`;
   after a submit while the game still holds ECHO: `tr("Sent - waiting for the game")`.
   Font and palette from the command line (`font()`, `mRegularPalette`).
5. Key handling in `event()`, because `QWidget::event` consumes Tab before `keyPressEvent`:
   - `ShortcutOverride`: claim it (accept, return true) when `mpHost->caretShortcutMatches(ke)` or
     when a user key binding matches the profile-switch shortcut - the same two checks
     `TCommandLine::event()` makes at ~200-219. Lift them into a small shared helper (a static on
     `TCommandLine`, or `Host::inputShortcutOverrideClaims(const QKeyEvent*)`) rather than copying.
     Never accept any other ShortcutOverride: that would kill every application shortcut.
   - `KeyPress`, in this order:
     1. caret shortcut → `mpHost->setCaretEnabled(true)`; accept. (Screen-reader users must be able
        to leave the field to re-read the prompt without losing it.)
     2. `Key_Return`/`Key_Enter`, no modifier (keypad allowed for Enter) → submit (step 6).
     3. `Key_Escape`, no modifier → if the field has text: `setText(QString())` (start over; also
        clears the undo history); if empty: emit `dismissed()`. Accept.
     4. `Key_Tab`, `Key_Backtab`, `Key_Up`, `Key_Down` → accept, do nothing (no focus change, no
        history, no completion). On macOS arrows carry `KeypadModifier`; treat it as "no modifier"
        exactly as `TCommandLine` does (~453-500).
     5. `Key_PageUp`/`Key_PageDown`, no modifier → scroll the console as `TCommandLine` does.
     6. `QKeySequence::Copy`, `Cut`, `Undo`, `Redo` → accept, do nothing, in both echo modes.
     7. Ctrl+digit tab switching: reuse `handleCtrlTabChange` if it can be lifted out of
        `TCommandLine` cheaply; otherwise list it in §4.10 as dropped while the field is open.
     8. Everything else: offer to `KeyUnit::processDataStream()` only when the key produces no
        printable text or carries Ctrl/Alt/Meta - so an F-key bound to a login alias runs, and a
        numpad digit bound to a direction does not eat a digit of the password. If a binding ran,
        accept and return. Otherwise `QLineEdit::event()`.
     9. Always `accept()` a KeyPress before returning, handled or not. An ignored key event would
        propagate to the parent chain; the parent is `layerCommandLine`, never `TCommandLine`, and
        no ancestor has a key handler, but accepting is the belt to that brace.
   - Skip all of the above when `!mpHost || mpHost->isClosingDown()`.
6. Submit: `QString line = text(); line.remove(QChar::CarriageReturn); line.remove(QChar::LineFeed);`
   (a pasted line break never makes a second line; `sendData` strips only LF), then
   `setEchoMode(Password)` (so a later destruction zero-fills, and VoiceOver does not read the
   removed text aloud), `setText(QString())`, re-apply hints, `mpHost->sendPasswordEntry(line)`,
   switch the placeholder, emit `submitted()` (no arguments). That is the only `text()` call.
7. On open, the command line's typed-ahead text: see C.3.
8. `focusInEvent()` with a reason other than `ActiveWindowFocusReason` →
   `mpHost->recordActiveCommandLine(mpCommandLine)`, so `setFocusOnHostActiveCommandLine()` and
   the caret forwarder keep landing here through the proxy. `mousePressEvent`/`mouseReleaseEvent`
   call `mudlet::self()->activateProfile(mpHost)` as the command line does.
9. Signals: `submitted()`, `dismissed()`. No text anywhere in the API.
10. Destructor: nothing. Do not `clear()` before destruction: Qt's zero-fill covers the buffer it
    still holds, and `clear()` would leave the characters in spare capacity.

#### C. `TMainConsole`: open, close, focus, geometry (`src/TMainConsole.h`, `src/TMainConsole.cpp`)

Owns the field because it owns the layout the field sits in and the focus proxies that point at
the command line. Members: `QPointer<TPasswordEntry> mpPasswordEntry`; accessor
`TPasswordEntry* passwordEntry() const` for tests and the Lua redirect (E).

- ctor: `connect(pH, &Host::signal_passwordEntryWantedChanged, this, &TMainConsole::slot_passwordEntryWanted)`
  and `connect(mpCommandLine, &TCommandLine::commandSubmitted, pH, &Host::clearPasswordEntryDismissal, Qt::QueuedConnection)`
  (queued: a re-open must not happen inside `enterCommand()`).
- `slot_passwordEntryWanted(bool)`: open or close; idempotent.
- `openPasswordEntry()`:
  1. `mpPasswordEntry = new TPasswordEntry(mpHost, mpCommandLine, layerCommandLine)` - a **sibling**
     of `mpCommandLine`, same parent, so `mpCommandLine->geometry()` is directly usable and an
     ignored key can never bubble into `TCommandLine::event()`.
  2. `setGeometry(mpCommandLine->geometry()); raise(); show();` and
     `mpCommandLine->installEventFilter(this)`: on `Resize`/`Move` copy the geometry again
     (`adjustHeight()` changes it when a script prints into the command line under the prompt).
  3. Typed-ahead text: if `mpCommandLine->typedSinceLastSubmit()` (D.3) and the command line has
     no selection, move its text into the field (`setText`) and `mpCommandLine->clear()`
     (which also drops the command line's undo history). Otherwise leave the command line alone.
     Rationale in §4.4.
  4. `mpCommandLine->setFocusProxy(mpPasswordEntry)` (invariant 5). Qt moves focus to the proxy if
     the command line had it, with normal focus events.
  5. Focus rule: `QWidget* f = window()->focusWidget();` (the window's focus child, valid even
     while Mudlet is not the active application - `QApplication::focusWidget()` is null then).
     If `f` is `mpCommandLine` or any `TCommandLine` of this profile, `mpPasswordEntry->setFocus(Qt::OtherFocusReason)`.
     Otherwise do not steal (editor, dialogs, other profiles, an output pane in caret mode) and
     call `mudlet::self()->announce(...)` with a short "the game asks for hidden input" line so a
     screen-reader user knows the field is there. A printable key typed on the pane in caret mode
     reaches the field through the proxy (D.2).
  6. connect `submitted` → nothing beyond what B.6 already did; `dismissed` →
     `mpHost->dismissPasswordEntry()`, which flips `passwordEntryWanted()` false and closes the
     field through the one signal.
  7. First time in this profile (`readProfileData("passwordEntryIntroduced")` empty): post one
     `[ INFO ]` line naming Enter, Esc, that aliases do not apply in the field, and the preference
     for games that hide all input; then `writeProfileData(...)`.
- `closePasswordEntry()`, in this order:
  1. `const bool hadFocus = window()->focusWidget() == mpPasswordEntry;`
  2. `mpCommandLine->setFocusProxy(nullptr)`.
  3. `if (hadFocus) mpCommandLine->setFocus(Qt::OtherFocusReason);` (sets the window's focus child
     even while inactive, so reactivation lands on the command line).
  4. remove the event filter; `mpPasswordEntry->setEchoMode(QLineEdit::Password)`;
     `mpPasswordEntry->hide()`; `mpPasswordEntry->deleteLater()`; `mpPasswordEntry = nullptr`.
     Hide before deleteLater: a WONT and a WILL in one read would otherwise leave two fields
     alive, one dying.
- Teardown: parent ownership; the field dies with `layerCommandLine`. `resetMainConsole()` does
  not touch the layer, so the field survives a profile reset, which is fine.
- `TConsole::setProxyForFocus()`: fire its `QAccessible::Focus` event for the deepest focus proxy
  (`pCommandLine->focusProxy()` when set), not for the command line hidden behind the field, or a
  screen reader's typed-character echo may treat the focused object as unprotected.

#### D. `TCommandLine`: removals plus two small things (`src/TCommandLine.h`, `src/TCommandLine.cpp`)

1. Remove: `setEchoSuppression()`, `paintEvent()`, `slot_togglePasswordVisibility()`,
   `updatePasswordToggleButton()`, `positionPasswordToggleButton()`, `resizeEvent()`, the toggle
   button and the `signal_remoteEchoChanged` connection in the ctor, the six members in §2, the
   history guard in `enterCommand()`, the tracking line in `processNormalKey()`, the masked branch
   of `mousePressEvent()`, and now-unused includes (`QToolButton`, `QResizeEvent`, `QPainter` -
   verify each; `Host.h` includes this header, and `TConsole.cpp` ~852/~1058 construct a
   `QResizeEvent` without a direct include, so add direct includes where the removal exposes them).
2. Add, at the top of `event()` for `KeyPress` only (not ShortcutOverride, which Qt already routes
   to the deepest proxy and which must not be double-handled):
   ```cpp
   // A widget with a focus proxy is not the keyboard target; Qt routes real key presses to the
   // proxy. Synthetic ones - the caret-mode forwarder in TTextEdit::keyPressEvent() sends straight
   // to this widget - must go the same way, or the first character of a password typed from the
   // output pane lands here in the clear.
   if (QWidget* proxy = focusProxy(); proxy && event->type() == QEvent::KeyPress) {
       return QApplication::sendEvent(proxy, event);
   }
   ```
   `focusProxy()` is non-null only while the field is open. This is the only password-related
   line left in `TCommandLine`, and it is about focus.
3. Add a fact, not a heuristic: `bool typedSinceLastSubmit() const`. Set when a key press or a
   paste edits the document (compare `document()->revision()` before and after, as #10978 does, so a
   bare modifier or a cursor move does not count); cleared in `enterCommand()` and in
   `historyMove()`. A programmatic `setPlainText()` from a script leaves it as it was. C.3 reads it
   once, when the field opens.

#### E. Small consumers

- `TLuaInterpreter::callCmdLineAction()`: delete the `isRemoteEchoingActive()` refusal. Sub
  command lines never hold the password; refusing their actions during a prompt was collateral, and
  it left the text sitting visibly in the sub line.
- Lua writes to `"main"` while the field is open go into the field: `printCmdLine` → `setText`,
  `appendCmdLine` → `insert` at the end, `clearCmdLine` → `setText(QString())`, `selectCmdLineText`
  → `selectAll`, `Host::sendCmdLine()`/`TMainConsole::setCommandLineText()` → `setText` +
  `selectAll` (this is what MXP `prompt:` links call). `getCmdLine("main")` keeps reading the
  command line - reads never see the field. Rationale in §4.6. `TLabel`'s own `prompt:` scheme
  writes to the command line directly and is left alone; a label link pre-filling a password is
  not a thing, and the text simply waits behind the field.
- `cTelnet` (keep every change here mechanical and small; #10964 and friends are open nearby):
  1. In `sendData()`'s two encoding warnings, quote the data only when the event was permitted.
     The only callers that withhold it are the two auto-login password sends and, now, the field;
     say so in a comment. Text: "Tried to send hidden input to the game, but it is unlikely to
     understand it" with a `//:` comment. This also fixes the auto-login leak.
  2. `checkCharacterModePattern()`: after setting the flag, call
     `mpHost->passwordEntryCharacterModeRecognised()` (Host decides between suppress-now and
     suppress-after-next-line by whether `passwordEntryWanted()` is currently true).
  3. `slot_send_pass()` and `sendOutstandingAutoLoginPassword()`: after a successful send, call
     `mpHost->suppressPasswordEntryUntilEchoReleased()`.
  4. New `bool autoLoginAboutToAnswer() const` (`hasAutoLoginCredentials()` and `mTimerLogin` or
     `mTimerPass` active) and `void abandonAutoLogin()` (stop both timers, clear
     `mAutoLoginPasswordOutstanding`). `slot_send_login()` calls
     `mpHost->recomputePasswordEntryWanted()` after starting `mTimerPass`, and `slot_send_pass()`
     after it fires, so the "about to answer" input is re-read at its transitions.
  5. `restartPasswordMaskTimeout()` reads the preference through the getter.
  No change to negotiation, the anomaly latch, the timers' logic or the flag's home.
- `dlgProfilePreferences` / `profile_preferences.ui`: write the preference through the setter.
  Reword the checkbox text, tooltip and `accessibleDescription` (`.ui` strings take an
  `extracomment`, not `//:`): suggested "Do not open a password field when the game asks for
  hidden input", tooltip "Turn this on for a game that hides all of your input, not only
  passwords. Everything you type then goes through the normal command line." Drop the "not
  recommended for security reasons" scare line: for those games it is the right setting.
- `XMLimport`/`XMLexport`: setter/getter.

### 3.3 Behaviour matrix

| Situation | What happens |
| --- | --- |
| WILL ECHO arrives; the command line holds a command the player typed and left (selected, auto-clear off; or recalled from history) | field opens over the line and takes focus; the command line is untouched and hidden behind the field until the prompt ends |
| WILL ECHO arrives; the command line holds text the player has been typing since their last Enter (`hunt` of `hunter2`, typed before the prompt showed) | that text moves into the field, masked, and the command line is emptied; the player keeps typing `er2` and Enter sends `hunter2` whole |
| Player types password, Enter | text goes out via `sendPasswordEntry()`; field empties, placeholder says it was sent, and it stays open until the game releases ECHO (RFC 857: the game is still echoing) |
| Game sends WONT ECHO | field closes; any text in it is discarded; focus returns to the command line if the field had it; a typed-ahead command that was left in the line is still there |
| Password rejected, game re-prompts with ECHO still held | field is still up; the retry is protected (this sinks the "close on Enter" alternative, §4.1) |
| Password rejected, game toggles WONT then WILL (Circle/tba style) | field closes and a fresh one opens. Note the anomaly latch: the count rises while consecutive toggles are under 5 s apart and only resets after a longer gap, so a fast fourth attempt (fifth toggle) gets no field and is typed in the clear, as today. Documented; not this PR's to change |
| Esc with text in the field | text discarded, field stays (start over) |
| Esc on an empty field | field closes, focus to the command line; the next line the player submits from the command line goes the ordinary way (aliases, history, event); if ECHO is still held after that line, a fresh field opens for the next one. On a game that hides all input, the player turns the preference on for that profile; the one-time info line says so |
| Auto-login with stored credentials | no field while the login or password timer is pending; the stored password goes out by its own path; the field is suppressed until the game releases ECHO; typed-ahead text stays in the command line (#7921). A late keychain password, once sent, suppresses the same way. If the game rejects the stored password and holds ECHO, the retry is typed in the clear, visibly |
| Keychain prompt unanswered, player types the password | a field opens (nothing is pending); its submit abandons the auto-login, so the late password cannot be sent on top |
| A trigger/script/key binding sends the password | goes through `Host::send()` as today, alias pass and all (#10968's requirement); the field closes when the game releases ECHO |
| F-key bound to a login alias pressed while the field has focus | forwarded to `KeyUnit`, binding runs, field stays |
| Game negotiates SGA and holds ECHO past a submitted line (GoMud, or a line-mode game masking both prompts) | recognition fires ~3 s after the first line. If a field is open it stays for one more line (the password being typed) and closes at that Enter or Esc; if none is open, none opens. Suppression lasts until the game releases ECHO, not for the connection, so a slow server's late WONT does not cost every later prompt its field. Cost, stated in invariant 7: on a game that was masking a line-mode password, a *retry* after that is typed in the clear, visibly |
| Game holds ECHO, no SGA | field stays for the session unless Esc (one line at a time), the preference, or the 60 s login-phase timeout after the last line sent (which an active player never lets fire). The info line points at the preference |
| ECHO anomaly latch | cTelnet refuses ECHO as today; no field; input in the clear, as today |
| Preference turned on mid-prompt | field closes now, text discarded; typed input ordinary, history works (#8902) |
| Preference turned off mid-prompt | field opens |
| The 60 s timeout or a disconnect while the player is away with text in the field | field closes, text discarded; a paste made afterwards lands in the command line, visibly. Same as today's `clear()` on unsuppress |
| Sub command line / miniconsole command line during a prompt | untouched, and its Lua action now runs (E). The field takes focus from it when it opens, so the password does not land there by momentum |
| Script calls `printCmdLine("main", pw)` / `sendCmdLine(pw)` / MXP `prompt:` link during a prompt | the text goes into the field; Enter sends it through the field's path; `getCmdLine("main")` still reads the command line; WONT discards it as it discards anything in the field |
| Caret mode: printable key on the output pane during a prompt | `TTextEdit` forwards to the command line; the D.2 redirect hands it to the field |
| Caret shortcut (Tab / Ctrl+Tab / F6) while the field has focus | caret mode turns on, focus goes to the pane; the field stays; typing a printable key comes back to it |
| Profile tab switched away and back, or detached into its own window | the field is a child of the console's `layerCommandLine`, so it moves and hides with it; `activateProfile()`'s `setFocus()` on the command line lands on the field |
| Mudlet not the active application when the field opens or closes | no focus stealing on open; on close the command line becomes the window's focus child so reactivation lands there |
| Profile reset (`resetMainConsole`) | the field survives, as the layer does; the Host flags are unchanged |
| Profile closed with the field open | field dies with its parent; Host is per profile |
| Two profiles, one at a prompt | each `Host`/`TMainConsole` pair has its own state and field |
| Password contains the command separator (`;;`) | sent whole (an improvement: `Host::send()` would have split it) |
| Paste, including a trailing line break from a password manager | pasted; line breaks are stripped on submit; one line goes out |
| Empty field, Enter | empty line sent |
| Password typed and Enter pressed before WILL ECHO is processed | `Host::send()` in the clear, as today; §1 says why |

### 3.4 What stays in cTelnet and elsewhere, and why

`isRemoteEchoingActive()` still means "the game echoes". These reads stay, as telnet semantics
rather than password protection, and are the complete list:

- `Host::send()` ~1958 and `TConsole::printCommand()` ~2229: no local echo while the game echoes.
  The second also silences a miniconsole command line's own echo; accepted.
- `cTelnet` ~5348: game output is not forwarded to MMCP snoopers while the game echoes. A
  protection read, and a correct one; accepted.
- `cTelnet` ~974, ~1766, ~1781, ~6433, ~6505: inputs to the late-password, character-mode and
  timeout logic, cTelnet's own.
- `cTelnet` ~6489 reads the masking preference to decide whether to arm the un-stick timer; its own.

The anomaly latch, character-mode recognition, the login-phase timeout and the late-password logic
are inputs to the policy, unchanged. Anyone tempted to improve them in this PR: do not.

## 4. Decisions and rejected alternatives

1. **Field stays open until the game releases ECHO, not closed on Enter.** Closing on Enter would
   leave the *retry* after a rejected password in the clear on games that hold ECHO across attempts
   - typed into the command line, into history, through aliases and `sysDataSendRequest`. Cost of
   staying open: on a slow link, text typed after the password but before WONT is discarded when
   the field closes. Visible, and what today's code does. The placeholder switching to "Sent -
   waiting for the game" after a submit tells the player what the lingering box is.
2. **Character-mode recognition never closes a field on a timer, and never for the connection.**
   v1 fed cTelnet's flag straight into the policy; the whole red team rejected it: recognition is
   advisory, fires 3 s after any line on any server that merely *offers* SGA, and is cleared only
   on reset - so a slow server, or a player taking more than 3 s over a retry, lost the field
   mid-typing and every later prompt of the connection. Now: recognition suppresses the *next*
   open, or the one after the line in progress, and only until the game next releases ECHO.
   Deterministic given the sequence of lines, never given the clock.
3. **Esc is "start over", then "step past".** With text, Esc empties the field (Mudlet's Esc
   habit is "retype"). On an empty field it dismisses for one line: the next line the player sends
   from the command line. v1's "until WONT" meant an Esc at a name prompt on a game that holds ECHO
   from connect put the password in the clear. One line also serves the player who types an alias
   name at the prompt: Esc, `pw`, Enter, and the field is back for whatever comes next. A game that
   hides everything is a per-game property, so its answer is the per-profile preference, not Esc.
4. **Text the player was typing when the prompt arrives moves into the field.** v1 left it in the
   command line, which split a fast typist's password (`hunt` behind the field, `er2` sent) and
   failed the login. The rule is a fact - did a key or paste edit the line since the player's last
   Enter - not a guess about history. A command left selected by auto-clear-off, or recalled with
   Up, is not moved: the flag was cleared by the Enter or the recall. The counter-case, a command
   typed ahead into a silence broken by an unexpected mid-session prompt, becomes part of the
   password and fails visibly. With the auto-login pending no field opens, so #7921's typed-ahead
   commands stay put.
5. **Sibling of the command line, not a child.** A child would get geometry for free, but an
   ignored key event bubbles from a child to `TCommandLine::event()`. Fifteen lines of geometry
   tracking are cheaper than that class of bug.
6. **Lua writes to "main" go into the field; reads do not.** A script that pre-fills the line for
   the player to press Enter wants that text where the keyboard is. Reads keep the "getCmdLine
   never returns the password" guarantee, and the existing spec case "does not leave the password
   behind when the prompt ends" keeps passing because the field discards on WONT.
7. **Focus proxy, not per-caller edits.** At least six places focus the main command line by
   pointer. `setFocusProxy()` makes all of them right without touching them. The window's focus
   child, not `QApplication::focusWidget()`, is what open/close consult, so an inactive Mudlet
   behaves.
8. **Fresh widget per prompt.** Deleting on close is the simplest proof that nothing (text, undo,
   reveal) carries over, and Qt zero-fills a password-mode line control on destruction.
9. **`sendData(..., isGameCommand=true)`, event withheld, warning never quotes withheld data.**
   The auto-login sends with the event withheld already; "withheld means sensitive" is the rule
   the code already lived by without saying so. The field's line must keep arming detection and
   the timeout as a command-line line did.
10. **Key surface is deliberately small.** Return, Esc, the caret shortcut, PageUp/Down, key
    bindings on non-printing or modified keys, Paste. Not: Ctrl+C copying the *console* selection,
    Ctrl+F, Ctrl+digit tab switching unless it lifts out cheaply. The field is a password box.
11. **No new Lua API** (`showPasswordEntry()` for games that never negotiate ECHO was proposed;
    it would answer "why no field on my game?", and it can come later without touching this
    design). No "hold and ask if the text matches an alias": that runs the alias patterns over the
    password, which is the leak.
12. **The flag stays in cTelnet.** v1 moved `mCharacterModeDetected` into Host; its companions
    are reset together in `reset()` and every reader is in cTelnet. One call out is smaller and
    does not collide with #10964.

## 5. Work breakdown

One PR, in commits that each build and pass `ctest` on their own. Read
`.agents/skills/build-mudlet/SKILL.md` before building; use the `linux-debug-nosan` preset already
configured in `build-linux-debug-nosan/`. Run `clang-format -i` on every C++ file touched.
Commit trailers per `CLAUDE.md`: `Assisted-by: Claude:<model id>`; **no** `Signed-off-by` - the
human adds it after testing. Danger will warn about touching more than ten source files; that is
expected for this change and the PR body should say so.

### Commit 1 - Host policy and send path (no behaviour change yet)

Files: `src/Host.h`, `src/Host.cpp`, `src/ctelnet.h`, `src/ctelnet.cpp`, `src/TCommandLine.cpp`
(getter at the two preference reads only), `src/dlgProfilePreferences.cpp`, `src/XMLimport.cpp`,
`src/XMLexport.cpp`, `test/functional_tests/SettingsRoundTripTest.cpp`,
`test/functional_tests/TelnetPasswordMaskTimeoutTest.cpp`, plus a new
`test/functional_tests/PasswordEntryPolicyTest.cpp` in the TELNET group.

- Everything in A and the cTelnet items in E. `TCommandLine` keeps working as before (it still
  listens to `signal_remoteEchoChanged`; that signal goes in commit 2).
- Test (`PasswordEntryPolicyTest`, friend of `cTelnet`, with the reset pattern from
  `TelnetPasswordMaskTimeoutTest::init()`): drive each input and assert `passwordEntryWanted()`
  and the signal count (one emission per change, none for a no-op); WONT clears both flags;
  recognition with no field wanted suppresses now, with one wanted suppresses after
  `sendPasswordEntry()` or `dismissPasswordEntry()`; a pending `mTimerPass` suppresses;
  `sendPasswordEntry("x")` reaches the server, raises no `sysDataSendRequest`, runs no alias
  `^x$`, leaves the Lua `command` global alone, and goes out even after a stray
  `denyCurrentSend()`; the encoding warning for an unencodable auto-login password no longer
  quotes it (set the profile to ASCII, send `p€ss`, read the console buffer).

### Commit 2 - The field, and the masking goes

Files: new `src/TPasswordEntry.{h,cpp}` (add to both `mudlet_SRCS` ~43 and `mudlet_HDRS` ~311 in
`src/CMakeLists.txt`; nothing else lists sources, lupdate scans `src/`), `src/TMainConsole.{h,cpp}`,
`src/TConsole.cpp` (`setProxyForFocus`, and any direct include the header trim exposes),
`src/TCommandLine.{h,cpp}`, `src/TLuaInterpreter.cpp`, `src/TLuaInterpreterUI.cpp`,
`src/TLuaInterpreterMudletObjects.cpp` (the macro copy, if it needs the same redirect),
`src/Host.{h,cpp}` (`sendCmdLine`, drop `signal_remoteEchoChanged`), `src/ui/profile_preferences.ui`,
`src/dlgProfilePreferences.cpp`, `test/functional_tests/CommandLineKeyHandlingTest.cpp`
(delete `test_aPasswordIsNotKeptInTheHistory`; it pins the guard this commit removes),
`src/mudlet-lua/tests/CommandLine_spec.lua`.

- Implement B, C, D, the rest of E. One commit, because the removal in D is what makes the command
  line untouched; splitting it would leave a tree where both mechanisms fire.
- Spec rewrite (`CommandLine_spec.lua`; both new cases fail on today's code, which clears the line
  at WILL ECHO and at WONT):
  - "keeps a left-over command in the command line while the game asks for hidden input":
    `printCmdLine("main", x)`, `selectCmdLineText("main")` (selected, so it is not typed-ahead),
    WILL ECHO, `getCmdLine("main") == x`; a sub command line untouched; WONT, still `x`.
  - "does not leave the password behind when the prompt ends" - keep, as is: WILL, `printCmdLine`
    (which now writes into the field), WONT, `getCmdLine("main") == ""`.
  - Reword the header comment (the mask is no longer painted over the document; the field cannot
    be seen from Lua by design) and keep the anomaly-budget comment: at most four ECHO toggles per
    describe block.

### Commit 3 - Functional tests for the field

New `test/functional_tests/PasswordEntryTest.cpp` in the WINDOW group (join
`WINDOW_GROUP_TEST_SOURCES`; do not make a standalone binary), `friend class PasswordEntryTest;`
in `ctelnet.h`. Copy `CommandLineKeyHandlingTest`'s server and key helpers, and
`TelnetPasswordMaskTimeoutTest::init()`'s resets: zero the anomaly counters, invalidate the toggle
timer, stop `mTimerPasswordModeTimeout` and `mTimerCharacterModeDetect`, reset
`mServerRequestedSGA`, and send WONT through `loopbackTest()` if ECHO is negotiated - never
`setRemoteEchoingActive(false)` alone, which leaves cTelnet believing ECHO is on so the next
case's WILL is ignored. Reach the field through `TMainConsole::passwordEntry()`, never
`findChild` (a dying field may still be a child). Hold the old field in a `QPointer` when asserting
"a fresh one": the allocator can reuse the address.

Harness requirements the red team established:
- `initTestCase()` must `show()` the main window and `QVERIFY(QTest::qWaitForWindowActive(...))`;
  under offscreen nothing has focus and hidden widgets get no Resize/Move events otherwise.
  Use `QTRY_COMPARE` for focus (`Host::setFocusOnHostActiveCommandLine` retries at 0/10/50 ms).
- Cases that prove the *proxy* must deliver keys to `window()->windowHandle()` with
  `QTest::keyClick`, which goes to the focus object; a key sent to the `TCommandLine*` goes through
  the D.2 redirect and would pass without the proxy.
- Drive ECHO with `setRemoteEchoingActive()` where the negotiation is not the subject, and with
  `loopbackTest()` where it is (recognition, WONT-then-WILL in one read).
- #10978's test comment says that after `waitForServerToReceive()` spins the event loop, synthetic
  keys stopped reaching the *main* command line. Root cause unknown. Write a ten-line probe first
  (type into the main line after a `qWait`); if it reproduces, put wire-reading cases in their own
  class (each class is its own ctest process and costs only a compile) and order assertions so key
  presses come before any wire wait.
- Recognition: arm through a real submitted line under loopback WILL SGA + WILL ECHO, then
  `mTimerCharacterModeDetect->start(0ms)` as `TelnetPasswordMaskTimeoutTest` ~423 does; no 3 s wait.
- At most four ECHO toggles per case, and never rely on an earlier case's state.

Cases (each must be shown to fail when the line it protects is reverted; name that line in the
case's comment):

1. WILL ECHO opens a field over the command line with its geometry; a selected left-over command
   in the command line is unchanged; the window's focus widget is the field.
2. Typed-ahead: type `hunt` into the main line, WILL ECHO; the field holds `hunt`, the command line
   is empty; type `er2`, Return; the server receives `hunter2`.
3. Type + Return in the field: server receives the text; command line unchanged; a
   `sysDataSendRequest` handler saw nothing; an alias `^<password>$` did not fire; the Lua `command`
   global is unchanged; after WONT, Up in the command line recalls the previous command, not the
   password.
4. Field stays open after Return until WONT; its placeholder changed; WONT closes it and focus
   returns to the command line.
5. WONT while the field holds text: nothing sent, text gone, command line unchanged.
6. Esc with text empties the field and keeps it; Esc on the empty field closes it; a line typed
   into the command line now expands an alias although ECHO is held; after that line a fresh field
   (different `QPointer`) is open.
7. Preference on: no field on WILL ECHO; a typed line reaches history (#8902).
8. Recognition with a field open: after loopback WILL SGA + WILL ECHO and a submitted line, fire
   the detector; the field is still open; the next Return closes it; no field re-opens while ECHO
   is held; loopback WONT then WILL opens one again.
9. Recognition with no field open (preference on, then off): none opens until WONT.
10. Auto-login: with credentials set and `mTimerPass` started, WILL ECHO opens no field; after
    `slot_send_pass()` fires, still none until WONT; typed-ahead text in the command line survived.
11. A key binding on F7 doing `send("frombinding")` fires from the field, the server receives it,
    the field is still open; a binding on plain `a` does not fire and `a` is typed.
12. Reveal toggle flips `echoMode()`; Ctrl+C in the revealed field leaves the clipboard unchanged;
    Ctrl+Z after a submit does not bring the text back.
13. Up/Down/Tab in the field: text unchanged, focus unchanged. Caret shortcut (set
    `mCaretShortcut` to F6 for the case) moves focus to the pane and leaves the field open; a
    printable key typed on the pane lands in the field.
14. Synthetic key path: `qApp->sendEvent(mpCommandLine, keyPress('a'))` while the field is open ends
    with `a` in the field and the command line unchanged (D.2).
15. `mpCommandLine->setFocus()` and `Host::setFocusOnHostActiveCommandLine()` after a sub command
    line was last used both land on the field (proxy + B.8).
16. Resize the console; the field's geometry follows the command line's.
17. Sub command line action runs during a prompt (E).
18. `printCmdLine("main", x)` and `sendCmdLine(x)` during a prompt land in the field;
    `getCmdLine("main")` still returns the command line's text.
19. Paste: clipboard text with a trailing `\n`, Ctrl+V, Return; the server receives one line.
20. Window inactive: close the field (WONT) while `window()` is not active; on reactivation the
    command line has focus.
21. Closing the profile with the field open does not crash (in `HostChildTeardownTest`, TEARDOWN
    group, `forceClose()`/`deleteHost()` pattern).

Existing tests that must still pass: `TelnetLatePasswordTest`, `TelnetPasswordMaskTimeoutTest`,
`SettingsRoundTripTest`, the rest of the WINDOW group minus the deleted case.

### Commit 4 - Wording and docs

- Preference label/tooltip/accessible description (E). Translator comments on every new `tr()`.
- If `docs/` mentions password masking, update it; the manual lives on the wiki - note the wiki
  edit in the PR body. Delete this plan file in the final commit, or move its §1 and §3.3 into
  `docs/` if the maintainers want the design recorded.
- PR: read `.agents/skills/open-pr/SKILL.md`. Title `Improve: Passwords are typed into a dedicated
  field instead of a masked command line`. Body per the template, with `**Test case:**` steps, the
  `Assisted-by` trailer in the body (a squash merge drops commit trailers), the grep from
  invariant 3, an NVDA/VoiceOver check in the manual test steps, and a demo video per
  `docs/demo-videos.md` if feasible. List #10965, #10972, #10978 as superseded and #10968 as
  reduced to its "scripts keep aliases" tests, and say why. Open as draft until the human has
  tested it.

## 6. Test strategy notes

- Specs where Lua can see the behaviour (the command line being untouched); functional tests for
  everything key-driven or about focus/geometry, since Lua cannot press keys.
- Both harnesses fail silently when set up wrong: for every new case, break the code once and
  watch the case go red before trusting it. Record which line was broken in the case's comment.
- The ECHO anomaly counter is process-wide state; reset it per case through friend access.
- `feedTelnet()` in specs needs the self-test profile started with `--offline`; the README in
  `src/mudlet-lua/tests/` says how, and `.claude/scripts/run-lua-tests.sh` runs the suite.

## 7. Risks and open questions (for the reviewer and the human tester)

1. Accessibility bridges: whether AT-SPI/IA2 forward the raw characters of Qt's text-insert
   events for a password-mode line edit is unverified. Test with NVDA (Windows) and VoiceOver
   (macOS) that typed characters are not spoken, and with Accerciser or `dbus-monitor` on Linux.
2. Whether Qt enables macOS secure event input for a password-mode `QLineEdit`, and whether that
   depends on `ImhHiddenText` (B.2 keeps the hint set in both echo modes).
3. Revealed text can be mouse-selected into the X11 primary selection, and Lua can read the
   clipboard. Reveal is the player's deliberate act; documented, not prevented.
4. Stylesheets: `setCmdLineStyleSheet("main", ...)` styles a `QPlainTextEdit`; the field will not
   pick that up, and a profile-wide `QLineEdit` rule will. It uses the command line's palette and
   font. Known cosmetic limit.
5. A first-time player on a game that holds ECHO from connect sees a hidden-input box at the name
   prompt. The placeholder names Esc, and the one-time info line names the preference. Whether that
   is enough is a question for the human tester, not for code.
6. Players who type an alias name at the prompt: Esc, the alias, Enter. The info line and the
   tooltip say so. The preference is the answer for a game that hides everything.
7. The typed-ahead move (C.3) is the one place the design touches the command line's text. It is
   a fact-based rule, but it is a rule; if the human tester finds it surprising, the fallback is to
   leave the text in place and accept the split-password failure it was added to avoid.
8. Mudlet Web shares the specs; nothing in the tests directory references it, and the rewritten
   cases use `feedTelnet` exactly as the current ones do.
9. The #10978 event-loop constraint on synthetic keys (commit 3) is unexplained; the probe comes
   first.

## 8. Size, honestly

Removed from `TCommandLine`: ~290 lines (the 1694-1936 block, the ctor, `processNormalKey`,
`mousePressEvent`, the history guard, ~20 header lines) and the six members; ~10 lines elsewhere.
Added: `TPasswordEntry` ~200 with headers, `TMainConsole` ~90, `Host` ~90, `cTelnet` ~25, the
redirect and the typed-ahead fact ~15, Lua redirects ~30. Net product code roughly +150; tests
~+600. What is gone is the guessing (history scan, selection bookkeeping, typed-during tracking)
and the per-feature guards; what is added is a widget with a defined key surface and a policy
function with five named inputs. Five inputs is not "one state", and the plan does not claim it;
the claim is that the five are read in one function and nowhere else.

## 9. Red-team log (v1 → v2)

Five reviewers (telnet/server compatibility, Qt mechanics, leak paths, feasibility/tests,
skeptical maintainer) attacked v1. Accepted:

- Recognition closing the field on a timer and for the connection (all five): §4.2.
- Esc dismissal until WONT (three): §4.3.
- The encoding warning quoting the password (three): E; also fixes the auto-login.
- Typed-ahead password split (three): §4.4.
- Caret-mode shortcut unreachable from the field; PageUp/Down; profile-switch override (three): B.5.
- Focus bookkeeping while Mudlet is inactive; `recordActiveCommandLine` going stale; the accessibility
  focus event naming the wrong widget; `deleteLater` without `hide` (Qt reviewer): C, B.8.
- Undo after reveal; hints cleared by `setEchoMode(Normal)`; Copy/Cut while revealed;
  `secureStringClear` being theatre; `clear()` before destruction defeating Qt's zero-fill (leak
  reviewer): B.2, B.6, B.10.
- Sub command lines as a place the password lands by momentum (two): C.5 steals focus from them.
- Lua writes to "main" during a prompt (maintainer): §4.6.
- Auto-login races (telnet): E.3-4, §3.3.
- Stale `denyCurrentSend()` (three): A.
- Paste-only context menu; multi-line paste; numpad bindings eating digits; the flag move; the
  incomplete preference-site list; the WINDOW test that goes red; harness isolation and the hidden
  window under offscreen; `findChild` on a dying field; `Qt::PreventContextMenu` (various): folded
  in where they belong.

Rejected, with the reason recorded in §4: close-on-Enter (retry leak); a heuristic for the
Enter-before-WILL-ECHO window (none exists); "hold and ask if the text matches an alias" (runs
patterns over the password); a `showPasswordEntry()` Lua API (later, separately); moving
`mCharacterModeDetected` into Host (drift, #10964); requiring three lines before recognition
counts (cTelnet's heuristic is not this PR's).
