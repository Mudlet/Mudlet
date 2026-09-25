# Plan: a dedicated password field instead of masking the command line

Issue: https://github.com/Mudlet/Mudlet/issues/11024
Branch: `claude/password-masking-architecture-78y96u`
Status: v3, after two red-team rounds (five reviewers, then two). §9 records what changed and why.

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

And one rule about when protection ends:

> **Protection is removed only by the game or by the player.** The field closes when the game
> releases ECHO (WONT, the existing safety timeout, disconnect), when the player steps past it
> with Esc, or when the player turns it off in the profile's settings. Mudlet's own guesses
> (character-at-a-time recognition) never close a field; they only change what Esc does and what
> the field says. So a wrong guess can never put a password in the clear.

What falls out, with no per-feature guard:

| Leak today | Why it cannot happen after |
| --- | --- |
| history, Up/Down (#10965) | the field has no history |
| Tab / autocomplete rewriting the password (#10971, #10972) | the field has neither |
| a typed-ahead command being sent as part of the password, or dropped (#7921, #8127, #10973, #10978) | the command line is not emptied or restored, so there is nothing to guess |
| copy / cut / drag / "Add to dictionary" (#10966) | while masked, Qt's password echo mode refuses copy, cut and drag; the field swallows the Copy/Cut/Undo/Redo shortcuts even when revealed; it has no spell-check; its context menu offers Paste only |
| `getCmdLine("main")` returning the password | the command line never holds it |
| aliases, `sysDataSendRequest`, the `command` global (#10968) | the field's send path never runs the alias pass and never raises the event |
| screen readers reading the password (#6089) | a native password field is announced as protected |
| the guards disagreeing (widget flag vs. `isRemoteEchoingActive()` vs. `mDisablePasswordMasking`) | the inputs are combined in one function, `Host::passwordEntryWanted()`; everything else reads its output |

What it does **not** change: how Mudlet decides a prompt is a password prompt. That stays in
`cTelnet` (WILL/WONT ECHO, the anomaly latch, character-at-a-time recognition, the login-phase
safety timeout, the auto-login timers). The field is a *view* of the game's ECHO state.

What it also does not fix, stated up front so nobody claims otherwise in the PR:

- A password typed and submitted *before* WILL ECHO is processed (fast typist, laggy link) goes
  through `Host::send()` in the clear, exactly as today. The window is one round trip. Nothing
  without a heuristic can help, and heuristics are what this design removes.
- Memory hygiene. Copies of the plaintext exist in Qt's line control, the encoder's byte array,
  the socket buffers and TLS. Qt zero-fills the buffer a password-mode line edit still holds when
  it is destroyed, and the send path zeroes its own local; the rest is not addressed. Do not write
  "secure" anywhere in the code.
- A game that hides all input for the whole session (character-at-a-time, or a line-mode game
  that masks everything) shows a hidden-input box until the player presses Esc, at most twice per
  session, or turns the field off for that profile. The info line and the placeholder say how.

## 2. Where things are today (orientation, verify each)

- `src/TCommandLine.cpp`
  - ctor (~72): creates the eye toggle `mpPasswordToggleButton`; ~132 connects
    `Host::signal_remoteEchoChanged` to `setEchoSuppression()`.
  - `processNormalKey()` (~138-156): sets `mUserTypedDuringEchoSuppression`.
  - `mousePressEvent()` (~984-1000): refuses selection/drag while masked, keeps right-click Paste;
    builds the context menu itself (the widget uses `Qt::PreventContextMenu`, ~117).
  - `enterCommand()` (~1033): emits `commandSubmitted()` first, then sends, then the history guard
    `!isRemoteEchoingActive() || mDisablePasswordMasking` (~1062), then `clear()` (auto-clear on)
    or `selectAll()` (off).
  - `setEchoSuppression()` (~1715-1855): the save/clear/restore heuristics.
  - `paintEvent()` (~1857), `slot_togglePasswordVisibility()`, `updatePasswordToggleButton()`,
    `positionPasswordToggleButton()`, `resizeEvent()`: the mask and its toggle.
  - `event()` (~193-637): all key handling. ShortcutOverride claims for the caret shortcut and for
    a user binding on the profile-switch shortcut (~200-219); Tab/F6 turning caret mode on
    (~306-341); Ctrl+digit tab switching (`handleCtrlTabChange`); Escape = select all (~518-528);
    PageUp/PageDown scroll the console; keypad keys offered to `KeyUnit` first (~273). Edits happen
    on several branches: `default:` via `processNormalKey`, Backspace/Delete (~397-440) calling
    `QPlainTextEdit::event` directly, Shift+Return `insertBlock` (~445), Space falling through to
    the base at ~637, Tab completion, plus `InputMethod` events, `insertFromMimeData` (paste, drop,
    middle-click) and the spell-check popup.
  - `focusInEvent()` (~639-653): `mpHost->recordActiveCommandLine(this)`; `mousePressEvent` /
    `mouseReleaseEvent` call `mudlet::self()->activateProfile(mpHost)`.
- `src/TCommandLine.h`: members `mIsEchoSuppressed`, `mPasswordVisible`, `mpPasswordToggleButton`,
  `mTextToRestoreAfterEchoSuppression`, `mRestoredTextShouldBeSelected`,
  `mUserTypedDuringEchoSuppression`; `mRegularPalette` is public; `mpKeyUnit` is fetched from the
  Host (~129). `Host.h` includes this header.
- `src/Host.h` / `src/Host.cpp`
  - `mIsRemoteEchoingActive`, `setRemoteEchoingActive()` (~6376: **acts only when the value
    changes**, so a call with the current value does nothing), `isRemoteEchoingActive()`.
    `signal_remoteEchoChanged`'s only listener is the command line.
  - `mDisablePasswordMasking` (~699): a bare public bool. Readers/writers: `TCommandLine.cpp` ~1062
    and ~1724, `ctelnet.cpp` ~6489, `dlgProfilePreferences.cpp` ~4046 and ~6509, `XMLimport.cpp`
    ~780 (through a `bool&` lambda parameter), `XMLexport.cpp` ~409,
    `test/functional_tests/SettingsRoundTripTest.cpp` ~333/354/419,
    `TelnetPasswordMaskTimeoutTest.cpp` ~197/214/381.
  - `Host::send()` (~1938): skips the local echo while the game echoes (RFC 857, keep), splits on
    the command separator, runs the alias pass, calls `mTelnet.sendData()`. Called by the command
    lines (any type without an action), by scripts, triggers, keys, buttons, `expandAlias()`.
  - `hasAutoLoginCredentials()` (Host.h ~222: a login and either a password or a pending keychain
    lookup), `securedPasswordAnswered()` (~4753), `caretShortcutMatches()` (~6179),
    `setCaretEnabled()`, `setFocusOnHostActiveCommandLine()` (~6195, focuses `activeCommandLine()`
    with 0/10/50 ms retries), `recordActiveCommandLine()`, `mUserSentInputThisConnection`
    (Host.h ~1226), `writeProfileData()`/`readProfileData()` (~426).
  - `mAllowToSendCommand` (Host.h ~702) set false by Lua `denyCurrentSend()`
    (`TLuaInterpreter.cpp` ~756); read and reset by `sendData()`.
- `src/ctelnet.cpp` / `src/ctelnet.h`
  - `sendData(QString&, permitDataSendRequestEvent, isGameCommand)` (~1683-1790): raises
    `sysDataSendRequest` when permitted; honours `mAllowToSendCommand`; **posts an encoding warning
    that quotes the data** (~1701-1705 and ~1726-1731; the once-only flag resets on an encoding
    change, ~450); for game commands resets `mAutoLoginPasswordOutstanding` (~1747-1754), arms
    character-mode detection (~1766-1779, timer created lazily at ~1771) and the mask timeout
    (~1781-1783). The only callers that withhold the event are the two auto-login password sends
    (~925, ~986).
  - WILL ECHO (~3470-3495): a repeated WILL while ECHO is already on is ignored (~3475).
    WONT ECHO (~3620-3645). WILL SGA (~3453-3457) sets `mServerRequestedSGA` even though Mudlet
    refuses SGA.
  - `checkCharacterModePattern()` (~6416-6448): fires 3 s after the *most recent* game-command line
    sent under ECHO with SGA offered, at most once per connection; sets `mCharacterModeDetected`,
    raises `sysCharacterModeDetected`, posts a warning at most three times ever and never to
    experienced players. Its own comment calls it advisory. The flag is read at ~974, ~1766,
    ~6433, ~6489, ~6505 and cleared only in `reset()` (~274, next to `setRemoteEchoingActive(false)`
    at ~265). It is poked by `TelnetPasswordMaskTimeoutTest.cpp` ~198/424.
  - `checkEchoAnomalyPattern()` (~6451-6470): the count rises while consecutive toggles are less
    than 5 s apart and only resets after a longer gap; five latch the process until `reset()`.
    WONT counts too.
  - `restartPasswordMaskTimeout()` (~6482-6501): 60 s, restarted by every line sent under ECHO,
    armed only inside the first 5 min of a connection and never once recognition fired;
    `slot_passwordMaskTimeout()` (~6503) sends DONT ECHO, resets the announced state and clears
    the echo state, so a later WILL is honoured.
  - Auto-login: `mTimerLogin` starts at connect (~1079); `slot_send_login()` (~904) sends the name
    and starts `mTimerPass` only when `hasAutoLoginCredentials()`; `slot_send_pass()` (~916, a
    public slot) sends via `sendData(pass, false)` or marks a password outstanding;
    `sendOutstandingAutoLoginPassword()` (~943) sends a keychain password that arrived late while
    `stillAtPrompt` (~974); `cancelLoginTimers()` (~417) stops both timers and clears the
    outstanding marker, and `GMCPAuthenticator` calls it (~1151, ~1340).
  - `friend class Host` (ctelnet.h ~384); tests are friends too (~363-379). `loopbackTest()` is
    public.
- `src/TConsole.cpp` / `src/TConsole.h`: `mpCommandLine` (a `QPointer`), `layerCommandLine`,
  `layoutLayer2`, `mpButtonMainLayer` are all public. The main command line is created ~393 and
  reparented into `layerCommandLine` by `layoutLayer2->addWidget()` ~687; `mpButtonMainLayer` moves
  on to `commandSplitter` ~720, so the command line is the layer's only child widget.
  `adjustHeight()` (TCommandLine ~701-740) sets the layer's min/max height synchronously and
  re-runs the console layout, so the 31 px at ~487 is only initial. Focus proxies: console and both
  panes → `mpCommandLine` (~453, ~459, ~809). `setProxyForFocus()` (~3199-3208) sets them and
  fires a `QAccessible::Focus` event for the command line. `setCaretMode(true)` grabs the keyboard
  on Linux (~3272) and releases it on a 0 ms timer (~3283). `printCommand()` (~2226) skips echo
  while the game echoes (keep).
- `src/TMainConsole.cpp`: ctor ~25-70 (has connects); `resetMainConsole()` (~525-574) removes
  docks, sub command lines and labels, **not** `layerCommandLine`. The only `new TMainConsole` is
  `mudlet.cpp` ~3885. `registerSubCommandLine()` (~1087).
- `src/TLuaInterpreter.cpp` `callCmdLineAction()` (~4852-4860): refuses a sub command line's Lua
  action while `isRemoteEchoingActive()`.
- Lua writers of the main line: `printCmdLine` (`TLuaInterpreterMudletObjects.cpp` ~1481),
  `appendCmdLine` (~280), `clearCmdLine` (~304), `getCmdLine` (~656) live in
  `TLuaInterpreterMudletObjects.cpp`; `selectCmdLineText` in `TLuaInterpreterUI.cpp` (~2778). Both
  files have a static `isMain()` and a copy of the `COMMANDLINE` macro. `sendCmdLine` goes through
  `Host::sendCmdLine()` → `TMainConsole::setCommandLineText()` (~1174); MXP `prompt:` links become
  `sendCmdLine(...)` (`TBuffer.cpp` ~4084-4088). `TLabel.cpp` ~731-745 writes to the command line
  directly for a label's own `prompt:` scheme. Nothing else writes to the main line.
- `src/TTextEdit.cpp` (~4140-4155): caret mode forwards a printable key with
  `mpHost->setFocusOnHostActiveCommandLine()` then `qApp->sendEvent(mpConsole->mpCommandLine, ...)`.
- `src/mudlet.cpp`: `activateProfile` ~4070 (`mpCommandLine->setFocus()`), ~8281 (`repaint()`),
  `changeEvent` ~8795-8812 (remembers `QApplication::focusWidget()` across deactivation in a
  `QPointer`), profile-switch shortcuts ~2278, `announce(text, processing, isPlain)` (mudlet.h
  ~190; the default treats the text as HTML), `experiencedMudletPlayer()`.
- Tests: `test/functional_tests/CommandLineKeyHandlingTest.cpp` (WINDOW group; recording server,
  `type()`, `press()`, `runLua()`, `waitForServerToReceive()`, `TUiTour::rememberShown()` before
  `init()` at ~213 because the first-run tour's application-wide event filter swallows keys; its
  `test_aPasswordIsNotKeptInTheHistory` ~486-500 asserts the history guard this plan removes);
  `TelnetPasswordMaskTimeoutTest.cpp` (TELNET group; its `init()`/`cleanup()` ~170-205 show how to
  reset the anomaly counters, both timers, the SGA and recognition flags and the ECHO state
  through friend access); `TelnetLatePasswordTest.cpp`; `HostChildTeardownTest.cpp` (TEARDOWN
  group, `forceClose()` / `requestClose()` / `deleteHost()` patterns ~259-301);
  `SubCommandLineLifetimeTest.cpp` ~335 (`activateWindow()` then `qWaitForWindowActive`);
  `CaretNavigationTest.cpp` ~102 (reads `mpConsole->buffer.lineBuffer`); `RecordingTelnetServer.h`
  (header-only; `TelnetServerStub` does not expose what it received);
  `src/mudlet-lua/tests/CommandLine_spec.lua` (drives ECHO with `feedTelnet`, needs `--offline`;
  its two prompt cases pin today's clear/restore behaviour). A grouped test class is its own file
  named after the class (test CMakeLists ~31). CI runs ctest under `QT_QPA_PLATFORM=offscreen`; the
  harness never shows the main window.
- Open PRs this supersedes or changes: #10965, #10972, #10978 (superseded); #10968 (its
  "scripts keep their aliases at a prompt" tests stay valid; its withholding guards are unnecessary);
  #10964 (touches the auto-login lines near ~974; keep the cTelnet diff here small).

## 3. Design

### 3.1 Invariants (the contract every piece of code must keep)

1. No code path writes text *out of* the field into `TCommandLine`, its history, its document or
   its selection. Not on Enter, not on Esc, not on WONT ECHO, not on timeout, not on close.
   (Text may move *into* the field from the command line when it opens, C.3; that is text the
   player already saw on screen, going into protection.)
2. Text in the field is sent by exactly one function, `Host::sendPasswordEntry()`, which calls
   `cTelnet::sendData(text, /*permitDataSendRequestEvent=*/false, /*isGameCommand=*/true)`.
   No alias pass, no `sysDataSendRequest`, no command-separator split, no local echo, no history.
3. In `src/`, `QLineEdit::text()` is called on the field in exactly one place: the Return branch
   of `TPasswordEntry`. There is no accessor and no text-carrying signal, and no close path reads
   it. Tests may read it. Put `grep -rn "text()" src/TPasswordEntry.cpp` and a grep for the field
   across the rest of `src/` in the PR body's test case.
4. The inputs to "should the field be up" are combined in one function,
   `Host::passwordEntryWanted()`, and its own mutators. Other code reads outputs -
   `passwordEntryWanted()`, `TMainConsole::passwordEntry()`, `TCommandLine::focusProxy()` - never
   an input. (Telnet-semantic reads of `isRemoteEchoingActive()` that suppress local echo, and
   cTelnet's own reads of its own flags, stay; §3.4 lists them.)
5. While the field is open it is the keyboard target for everything that means "the main command
   line": `mpCommandLine->setFocus()`, the console's focus proxies,
   `setFocusOnHostActiveCommandLine()`, and the synthetic key forwarders. Achieved with
   `mpCommandLine->setFocusProxy(field)` plus one redirect line, not per-caller edits.
6. The field exists only while it is wanted. It is created on open and deleted on close, so no
   text, undo state or reveal state carries from one prompt to the next. It is deleted in password
   echo mode, so Qt zero-fills whatever it still holds.
7. Only the game or the player ends protection. No timer, guess or inference of Mudlet's closes an
   open field or prevents the next one from opening, with one deliberate exception: after the
   auto-login has sent the stored password under the game's mask, no field opens until the game
   releases ECHO (§4.5 says why and what it costs).
8. Everything else about `TCommandLine` behaves exactly as with no prompt open: history, Tab,
   aliases, `sysDataSendRequest`, sub command lines, `callCmdLineAction()`. When no field is shown
   under ECHO, typed input goes the ordinary way, and that is the player's choice every time.

### 3.2 Components

#### A. `Host`: the policy and the send path (`src/Host.h`, `src/Host.cpp`)

```cpp
// Should the hidden-input field be up? Five inputs, combined here and nowhere else. One signal,
// emitted only when the derived value changes.
bool passwordEntryWanted() const;
//   = mIsRemoteEchoingActive
//     && !mDisablePasswordMasking
//     && !mPasswordEntrySuppressed        // until the game releases ECHO
//     && !mPasswordEntryDismissed         // until the next line goes to the game
//     && !mTelnet.autoLoginPending();     // the auto-login still intends to send a password

// The state of the current ECHO hold, and how it is written:
void setRemoteEchoingActive(bool);          // exists. On false: clears the three flags below
                                            // UNCONDITIONALLY, before its change guard, then
                                            // recomputes. On true: marks the hold if
                                            // mTelnet.characterModeDetected() already is.
void setDisablePasswordMasking(bool);       // new; the only write path for the preference
bool disablePasswordMasking() const;
void suppressPasswordEntryUntilEchoReleased(); // the auto-login answered under the mask, or Esc
                                               // on an empty field during a marked hold
void dismissPasswordEntry();                // Esc on an empty field: marked hold → suppress;
                                            // otherwise mPasswordEntryDismissed = true
void clearPasswordEntryDismissal();         // Host::send() calls it: a line went to the game.
                                            // If ECHO is still held, the hold becomes marked
                                            // (the game hid input across a line the player sent
                                            // normally). Recompute is deferred one event-loop
                                            // turn (QTimer::singleShot(0)), so a re-open never
                                            // happens inside enterCommand() or a Lua send().
void markPasswordEntryHold();               // cTelnet's recognition hook; also set by the two
                                            // paths above. Emits signal_passwordEntryHoldMarked()
                                            // on the false→true edge so an open field can change
                                            // its placeholder. Not an input to passwordEntryWanted().
bool passwordEntryHoldMarked() const;
void recomputePasswordEntryWanted();        // cTelnet calls this when autoLoginPending changes
void passwordEntryEdited();                 // first edit in the field: mTelnet.cancelLoginTimers()
bool sendPasswordEntry(QString);            // invariant 2
signals:
    void signal_passwordEntryWantedChanged(bool);
    void signal_passwordEntryHoldMarked();
```

- The three flags (`mPasswordEntrySuppressed`, `mPasswordEntryDismissed`,
  `mPasswordEntryHoldMarked`) are cleared by every call of `setRemoteEchoingActive(false)`,
  including the one `cTelnet::reset()` makes while ECHO is already off, so nothing outlives a
  hold or a connection. Put the clearing *before* the existing "value unchanged → return" guard.
- `sendPasswordEntry(QString line)`:
  ```cpp
  mUserSentInputThisConnection = true;   // as Host::send() does, for the GMCP auth path
  // No event was raised for this line, so nothing could have denied it. A denyCurrentSend()
  // called outside a handler leaves a stale refusal that sendData() would otherwise apply here
  // and drop the password without a word.
  mAllowToSendCommand = true;
  // isGameCommand stays true on purpose: a typed password must keep arming character-at-a-time
  // detection and the mask safety timeout, and must keep cancelling a late keychain password
  // (sendData ~1747), exactly as a line from the command line did.
  const bool sent = mTelnet.sendData(line, false, true);
  mTelnet.cancelLoginTimers();           // the player answered; no timer or late password may too
  line.fill(QChar());                    // the field already dropped its copy, so this is the last
                                         // owner; best effort, nothing more
  return sent;
  ```
  An empty string sends an empty line (a blank password, or "press Enter to continue").
- `mDisablePasswordMasking` becomes private behind the getter/setter. Every site in §2 changes:
  readers use the getter, `XMLimport` reads into a local and calls the setter, tests call the
  setter. The XML attribute name `disablePasswordMasking` is unchanged.
- `Host::send()` calls `clearPasswordEntryDismissal()` once per call (a no-op unless dismissed).
  Any line to the game ends a dismissal: the player's, a trigger's, a key binding's.
- Remove `signal_remoteEchoChanged` in the commit that removes its only listener.

#### B. `TPasswordEntry` (new, `src/TPasswordEntry.h`, `src/TPasswordEntry.cpp`)

A `QLineEdit` subclass. Keep it small; the red team estimates ~200 lines with headers.

1. Constructor takes `Host*` (held as `QPointer<Host>`, KeyUnit fetched per event as
   `TCommandLine.h` ~129 does), the `TCommandLine*` it stands in for, and the parent widget.
2. `setEchoMode(QLineEdit::Password)`. Then OR the hints in, and again after every echo-mode
   change, because `setEchoMode(Normal)` clears them: `setInputMethodHints(inputMethodHints() |
   Qt::ImhHiddenText | Qt::ImhSensitiveData | Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase)`.
   Keep `Qt::DefaultContextMenu` and override `contextMenuEvent()` to show a menu with Paste only
   (`Qt::PreventContextMenu` would stop the event from being delivered at all; `TCommandLine` gets
   away with it by building its menu in `mousePressEvent`). That covers right-click, the Menu key
   and Shift+F10. `setClearButtonEnabled(false)`, `setFrame(true)`, `setDragEnabled(false)`.
3. Reveal toggle: `addAction(QIcon(qsl(":/icons/password-show-on.png")), QLineEdit::TrailingPosition)`
   flipping between `Password` and `Normal`, icon to `password-show-off.png`, hints re-applied.
   Set the action's `text()` and `toolTip()` (`tr("Show password")` / `tr("Hide password")` with
   `//:` translator comments); the button Qt makes takes its accessible name from them. It cannot
   take focus, as today's toggle cannot. After a submit, put the action back to "show".
   Revealed text can be selected into the X11 primary selection by mouse or keyboard; that is what
   reveal means (§7).
4. Accessible name and description follow the `signal_adjustAccessibleNames` pattern
   (`TCommandLine.cpp` ~1504): name `tr("Hidden input")` (with the profile name when several are
   open); description saying Enter sends it straight to the game without aliases or history, and
   Esc empties it and, when it is already empty, closes it to use the command line instead.
   Placeholder text, three states, each with a `//:` comment:
   - fresh, unmarked hold: `tr("Hidden input - Esc to answer in the command line instead")`;
   - after a submit while the game still holds ECHO: `tr("Sent - waiting for the game")`;
   - marked hold (B.5.3 says what Esc does then): `tr("This game seems to hide everything you type
     - Esc to stop hiding it until the game says otherwise")`.
   Font and palette from the command line (`font()`, `mRegularPalette`).
5. Key handling in `event()`, because `QWidget::event` consumes Tab before `keyPressEvent`:
   - `ShortcutOverride`: claim it (accept, return true) when `mpHost->caretShortcutMatches(ke)` or
     when a user key binding matches the profile-switch shortcut - the same two checks
     `TCommandLine::event()` makes at ~200-219. Lift them into one shared helper (a static on
     `TCommandLine`, or `Host::inputShortcutOverrideClaims(const QKeyEvent*)`) rather than copying.
     Never accept any other ShortcutOverride: that would kill every application shortcut.
   - `KeyPress`, in this order:
     1. caret shortcut → `mpHost->setCaretEnabled(true)`; accept. (Screen-reader users must be able
        to leave the field to re-read the prompt without losing it.)
     2. `Key_Return`/`Key_Enter`, no modifier (keypad allowed for Enter) → submit (B.6).
     3. `Key_Escape`, no modifier → if the field has text: `setText(QString())` (start over; also
        clears the undo history). If empty: emit `dismissed()`. Accept.
     4. `Key_Tab`, `Key_Backtab`, `Key_Up`, `Key_Down` → accept, do nothing (no focus change, no
        history, no completion). On macOS arrows carry `KeypadModifier`; treat it as "no modifier"
        exactly as `TCommandLine` does (~453-500).
     5. `Key_PageUp`/`Key_PageDown`, no modifier → scroll the console as `TCommandLine` does.
     6. `ke->matches(QKeySequence::Copy | Cut | Undo | Redo)` → accept, do nothing, in both echo
        modes.
     7. Ctrl+digit tab switching: reuse `handleCtrlTabChange` if it lifts out of `TCommandLine`
        cheaply; otherwise list it in §4.10 as dropped while the field is open.
     8. Everything else. Let `text = ke->text()`, `printable = !text.isEmpty() &&
        text.front().isPrint()`, `mods = ke->modifiers() & (Ctrl|Alt|Meta)`, and `altGr = (mods &
        Ctrl) && (mods & Alt)` (Windows AltGr arrives as Ctrl+Alt; on macOS Option arrives as Alt).
        If `printable && (mods == 0 || altGr || (macOS && mods == Alt))` → type it: `QLineEdit::event()`.
        Otherwise offer it to `KeyUnit::processDataStream()` first - an F-key bound to a login alias
        runs, Ctrl+letter bindings run, and a numpad digit bound to a direction does not eat a digit
        of the password; if nothing ran, `QLineEdit::event()`. This is stricter than the command
        line, which offers every key; say so in the comment.
     9. Always `accept()` a KeyPress before returning, handled or not. An ignored key would
        propagate to the parent chain; the parent is `layerCommandLine`, never `TCommandLine`, and
        no ancestor has a key handler, but accepting is the belt to that brace.
   - IME and dead-key input arrive as `InputMethod` events and go to `QLineEdit` untouched.
   - Skip all of the above when `!mpHost || mpHost->isClosingDown()`.
6. Submit: `QString line = text(); line.remove(QChar::CarriageReturn); line.remove(QChar::LineFeed);`
   (a pasted line break never makes a second line; `sendData` strips only LF), then
   `setEchoMode(Password)` and the reveal action back to "show" (so VoiceOver does not read the
   removed text aloud and a later destruction zero-fills), `setText(QString())` (clears the undo
   history), re-apply hints, `mpHost->sendPasswordEntry(line)`, switch the placeholder to "Sent",
   emit `submitted()` (no arguments). That is the only `text()` call.
7. First edit: connect `textEdited` once → `mpHost->passwordEntryEdited()`, so a keychain password
   arriving late cannot be typed over a player who has started answering.
8. `focusInEvent()` with a reason other than `ActiveWindowFocusReason` →
   `mpHost->recordActiveCommandLine(mpCommandLine)`, so `setFocusOnHostActiveCommandLine()` and
   the caret forwarder keep landing here through the proxy. `mousePressEvent`/`mouseReleaseEvent`
   call `mudlet::self()->activateProfile(mpHost)` as the command line does.
9. `setHoldMarked()` switches the placeholder to the marked text. Signals: `submitted()`,
   `dismissed()`. No text anywhere in the API.
10. Destructor: nothing. Do not `clear()` before destruction: Qt zero-fills the buffer it still
    holds only when the echo mode is not Normal, and `clear()` would leave characters in spare
    capacity. (Submitted text is handed to the send path, which zeroes its local; the field's own
    copy is released by `setText(QString())`, not zeroed. §1 says so.)

#### C. `TMainConsole`: open, close, focus, geometry (`src/TMainConsole.h`, `src/TMainConsole.cpp`)

Owns the field because it owns the layout the field sits in and the focus proxies that point at
the command line. Members: `QPointer<TPasswordEntry> mpPasswordEntry`; accessor
`TPasswordEntry* passwordEntry() const` for tests.

- ctor: `connect(pH, &Host::signal_passwordEntryWantedChanged, this, &TMainConsole::slot_passwordEntryWanted)`,
  `connect(pH, &Host::signal_passwordEntryHoldMarked, ...)` → `mpPasswordEntry->setHoldMarked()` if
  open, then `slot_passwordEntryWanted(pH->passwordEntryWanted())` to sync with the current state.
- `slot_passwordEntryWanted(bool)`: open or close; idempotent; no-op when `mpCommandLine` is null
  (it is a `QPointer`), `!mpHost`, or `mpHost->isClosingDown()`.
- `openPasswordEntry()`:
  1. `mpPasswordEntry = new TPasswordEntry(mpHost, mpCommandLine, layerCommandLine)` - a **sibling**
     of `mpCommandLine`, same parent, so `mpCommandLine->geometry()` is directly usable and an
     ignored key can never bubble into `TCommandLine::event()`.
  2. `setGeometry(mpCommandLine->geometry()); raise(); show();` and
     `mpCommandLine->installEventFilter(this)`: on `Resize`/`Move` copy the geometry again (window
     resizes, font changes, a label `prompt:` link growing the line).
  3. Typed-ahead text: if `mpCommandLine->playerTypedLine()` (D.3), move its text into the field
     (`setText`, line breaks stripped) and `mpCommandLine->clear()` (drops the command line's undo
     history; being programmatic, it also resets the flag). Otherwise leave the command line alone.
     Rationale in §4.4.
  4. `mpCommandLine->setFocusProxy(mpPasswordEntry)` (invariant 5). Qt moves focus to the proxy if
     the command line had it, with normal focus events.
  5. Focus rule: `QWidget* f = window()->focusWidget();` (the window's focus child, valid even
     while Mudlet is not the active application - `QApplication::focusWidget()` is null then).
     If `f` is null, `mpCommandLine`, or any `TCommandLine` of this profile,
     `mpPasswordEntry->setFocus(Qt::OtherFocusReason)`. Otherwise do not steal (editor, dialogs,
     another profile's widgets in multi-view, an output pane in caret mode) and call
     `mudlet::self()->announce(text, {}, true)` with a short "the game asks for hidden input" line
     so a screen-reader user knows the field is there. A printable key typed on the pane in caret
     mode reaches the field through the proxy (D.2).
  6. connect `dismissed` → `mpHost->dismissPasswordEntry()`, which flips `passwordEntryWanted()`
     false and closes the field through the one signal. `submitted` needs no connection.
  7. If `mpHost->passwordEntryHoldMarked()` already, `setHoldMarked()`.
  8. First time in this profile (`readProfileData("passwordEntryIntroduced")` empty): post one
     `[ INFO ]` line naming Enter, Esc (once to start over, again on an empty box to step past),
     that aliases do not apply in the box, and the profile setting for games that hide all input;
     then `writeProfileData(...)`. Post it through `QTimer::singleShot(0)`: the open happens in
     the middle of a telnet parse, and a synchronous line would land inside the prompt line.
- `closePasswordEntry()`, in this order:
  1. `const bool hadFocus = window()->focusWidget() == mpPasswordEntry;`
  2. `mpCommandLine->setFocusProxy(nullptr)`.
  3. `if (hadFocus) mpCommandLine->setFocus(Qt::OtherFocusReason);` (sets the window's focus child
     even while inactive, so reactivation lands on the command line).
  4. remove the event filter; `mpPasswordEntry->setEchoMode(QLineEdit::Password)`;
     `mpPasswordEntry->hide()`; `mpPasswordEntry->deleteLater()`; `mpPasswordEntry = nullptr`.
     Hide before deleteLater: a WONT and a WILL in one read would otherwise leave two fields
     alive, one dying. Clear the proxy and move focus *before* hiding, or `hide()` runs
     `focusNextPrevChild` itself.
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
2. Add, at the top of `event()` for `KeyPress` only:
   ```cpp
   // A widget with a focus proxy is not the keyboard target; Qt routes real key presses to the
   // proxy. Synthetic ones - the caret-mode forwarder in TTextEdit::keyPressEvent() sends straight
   // to this widget - must go the same way, or the first character of a password typed from the
   // output pane lands here in the clear. (For a synthetic key Qt sends the ShortcutOverride to
   // this widget too; both widgets claim the same keys, so that is harmless.)
   if (QWidget* proxy = focusProxy(); proxy && event->type() == QEvent::KeyPress) {
       return QApplication::sendEvent(proxy, event);
   }
   ```
   `focusProxy()` is non-null only while the field is open. This is the only password-related
   line left in `TCommandLine`, and it is about focus.
3. Add a fact: `bool playerTypedLine() const` - "everything on the line was typed or pasted by
   the player, starting from an empty or wholly selected line, and nothing else has changed it
   since". Not a heuristic about history or timing. Implementation:
   - a `mUserEditInProgress` guard set around every path where the player edits: the whole of
     `event()` for `KeyPress` and `InputMethod` events (that covers `processNormalKey`, Backspace,
     Delete, Space, Shift+Return, Tab completion), and an `insertFromMimeData()` override (paste,
     drop, middle-click). Record `mEditStartedOnBlankLine = toPlainText().isEmpty() ||
     (the selection covers the whole document)` when the guard is raised.
   - a `QTextDocument::contentsChange` handler: if `!mUserEditInProgress` → `mPlayerTypedLine =
     false` (a script's `setPlainText`, a `clear()`, a history recall changed the line); else if
     `mEditStartedOnBlankLine` → `mPlayerTypedLine = true`; else leave it (typing more into a line
     keeps whatever it was).
   - `enterCommand()` sets `mPlayerTypedLine = false` as its first statement, before
     `emit commandSubmitted()` and the send (an alias or handler may open the field synchronously),
     and its own `clear()`/`selectAll()` runs with the guard lowered. `historyMove()` and
     `handleTabCompletion()`'s programmatic writes, and anything else that writes the document from
     inside a guarded section, lower the guard first so the change counts as non-user.
   - The spell-check suggestion replacement counts as a user edit; the guard covers `slot_popupMenu`.
   C.3 reads it once, when the field opens.

#### E. Small consumers

- `TLuaInterpreter::callCmdLineAction()`: delete the `isRemoteEchoingActive()` refusal. Sub
  command lines never hold the password; refusing their actions during a prompt was collateral, and
  it left the text sitting visibly in the sub line.
- Lua writes to `"main"` while the field is open go into the field: after the `COMMANDLINE` macro
  resolves the widget, `if (auto* pField = qobject_cast<TPasswordEntry*>(pN->focusProxy()))` -
  no accessor and no duplicate lookup. `printCmdLine` → `setText`, `appendCmdLine` → `insert` at
  the end, `clearCmdLine` → `setText(QString())`, `selectCmdLineText` → `selectAll`, and
  `TMainConsole::setCommandLineText()` (`Host::sendCmdLine()`, MXP `prompt:` links) → `setText` +
  `selectAll`. `getCmdLine("main")` keeps reading the command line: reads never see the field.
  Rationale in §4.6. Both `TLuaInterpreterMudletObjects.cpp` and `TLuaInterpreterUI.cpp` change.
  `TLabel`'s own `prompt:` scheme writes to the command line directly and is left alone; a label
  link pre-filling a password is not a thing, and the text waits behind the field.
- `cTelnet` (keep every change here mechanical and small; #10964 and friends are open nearby):
  1. In `sendData()`'s two encoding warnings, quote the data only when the event was permitted.
     The only callers that withhold it are the two auto-login password sends and, now, the field;
     say so in a comment. Text: "Tried to send hidden input to the game, but it is unlikely to
     understand it" with a `//:` comment. This also fixes the auto-login leak.
  2. `checkCharacterModePattern()`: after setting the flag, `mpHost->markPasswordEntryHold()`.
     Add `bool characterModeDetected() const` so `Host::setRemoteEchoingActive(true)` can mark a
     hold that begins after recognition already fired on this connection.
  3. `slot_send_pass()` and `sendOutstandingAutoLoginPassword()`: after a successful send,
     `if (mpHost->isRemoteEchoingActive()) mpHost->suppressPasswordEntryUntilEchoReleased();`.
     Never while ECHO is off: there would be no WONT to end it.
  4. `bool autoLoginPending() const` backed by an explicit member, written only through
     `setAutoLoginPending(bool)`, which calls `mpHost->recomputePasswordEntryWanted()`. Set to
     `mpHost->hasAutoLoginCredentials()` where `mTimerLogin` starts (~1079) and again at the end
     of `slot_send_login()`; to false in `slot_send_pass()` (both branches), `cancelLoginTimers()`
     and `reset()`. `Host::securedPasswordAnswered()` on a denied lookup calls
     `mTelnet.setAutoLoginPending(hasAutoLoginCredentials() && mTelnet.autoLoginTimersRunning())`
     so a refused keychain does not hold the field back. `cancelLoginTimers()` already does what
     "abandon the auto-login" needs (stops both timers, clears the outstanding marker); reuse it.
  5. `restartPasswordMaskTimeout()` reads the preference through the getter.
  No change to negotiation, the anomaly latch, the timers' logic or the recognition flag's home.
- `dlgProfilePreferences` / `profile_preferences.ui`: write the preference through the setter.
  Reword the checkbox text, tooltip and `accessibleDescription` (`.ui` strings take an
  `extracomment`, not `//:`): suggested "Do not open a hidden-input box when the game asks for
  hidden input", tooltip "Turn this on for a game that hides everything you type, not only
  passwords. Everything you type then goes through the normal command line." Drop the "not
  recommended for security reasons" scare line: for those games it is the right setting.
- `XMLimport`/`XMLexport`: setter/getter.

### 3.3 Behaviour matrix

| Situation | What happens |
| --- | --- |
| WILL ECHO arrives; the command line holds a command the player typed and left (selected, auto-clear off; or recalled from history) | field opens over the line and takes focus; the command line is untouched and hidden behind the field until the prompt ends |
| WILL ECHO arrives; the command line holds text the player has been typing since their last Enter (`hunt` of `hunter2`, typed before the prompt showed) | that text moves into the field, masked, and the command line is emptied; the player keeps typing `er2` and Enter sends `hunter2` whole |
| Player types password, Enter | text goes out via `sendPasswordEntry()`; field empties, placeholder says it was sent, and it stays open until the game releases ECHO (RFC 857: the game is still echoing) |
| Game sends WONT ECHO | field closes; any text in it is discarded; focus returns to the command line if the field had it; the hold's flags are cleared; a typed-ahead command left in the line is still there |
| Password rejected, game re-prompts with ECHO still held | field is still up; the retry is protected, however long the player takes (this sinks the "close on Enter" alternative, §4.1) |
| Password rejected, game toggles WONT then WILL (Circle/tba style) | field closes and a fresh one opens. Note the anomaly latch: the count rises while consecutive toggles are under 5 s apart and only resets after a longer gap, so a fast fourth attempt (fifth toggle) gets no field and is typed in the clear, as today. Documented; not this PR's to change |
| Esc with text in the field | text discarded, field stays (start over) |
| Esc on an empty field, unmarked hold | field closes, focus to the command line; the next line to reach the game from anywhere (the command line, a trigger, a key) ends the dismissal. If ECHO is still held then, the hold becomes *marked* and a fresh field opens, one event-loop turn later, saying so |
| Esc on an empty field, marked hold | no field until the game releases ECHO. On a game that hides everything, the player turns the preference on for that profile; the one-time info line says so |
| Game negotiates SGA and holds ECHO past a submitted line (GoMud, or a line-mode game masking both prompts) | recognition fires ~3 s after the most recent line; the open field stays and switches to the marked placeholder. Nothing closes. Esc on the empty field then stops the field until the game releases ECHO. A wrong first password on such a game is retried inside the field |
| Game holds ECHO, no SGA | same as above without recognition: Esc, one line, the field comes back marked, Esc again. Two Escs per session, or the preference |
| Auto-login with stored credentials | no field while the auto-login still intends to send a password; the stored password goes out by its own path; if the game had ECHO up at that moment, no field until it releases ECHO; typed-ahead text stays in the command line (#7921). A late keychain password, once sent under ECHO, suppresses the same way. Cost (invariant 7's one exception): if the game rejects the stored password and holds ECHO, the retry is typed in the clear - into history, through aliases and the event, on screen |
| Auto-login on a game that never negotiates ECHO at login | nothing is suppressed (ECHO was off at the send), so a mid-session WILL ECHO an hour later gets a field |
| Keychain prompt unanswered or refused, player types the password | a field opens (nothing is pending); its first edit cancels the auto-login, so the late password cannot be sent on top |
| A trigger/script/key binding sends the password | goes through `Host::send()` as today, alias pass and all (#10968's requirement); the field closes when the game releases ECHO |
| F-key bound to a login alias pressed while the field has focus | forwarded to `KeyUnit`, binding runs, field stays |
| ECHO anomaly latch | cTelnet refuses ECHO as today; no field; input in the clear, as today |
| Preference turned on mid-prompt | field closes now, text discarded; typed input ordinary, history works (#8902) |
| Preference turned off mid-prompt | field opens, unless the hold is suppressed (auto-login answered, or Esc on a marked hold) |
| The 60 s timeout or a disconnect while the player is away with text in the field | field closes, text discarded; a paste made afterwards lands in the command line, visibly. Same as today's `clear()` on unsuppress |
| Sub command line / miniconsole command line during a prompt | untouched, and its Lua action now runs (E). The field takes focus from it when it opens, so the password does not land there by momentum. A line it sends to the game ends a dismissal like any other |
| Script calls `printCmdLine("main", pw)` / `sendCmdLine(pw)` / MXP `prompt:` link during a prompt | the text goes into the field; Enter sends it through the field's path; `getCmdLine("main")` still reads the command line; WONT discards it as it discards anything in the field |
| Caret mode: printable key on the output pane during a prompt | `TTextEdit` forwards to the command line; the D.2 redirect hands it to the field |
| Caret shortcut (Tab / Ctrl+Tab / F6) while the field has focus | caret mode turns on, focus goes to the pane; the field stays; typing a printable key comes back to it |
| Profile tab switched away and back, or detached into its own window | the field is a child of the console's `layerCommandLine`, so it moves and hides with it; `activateProfile()`'s `setFocus()` on the command line lands on the field |
| Mudlet not the active application when the field opens or closes | on open, focus follows the window's focus child rule; on close the command line becomes the window's focus child so reactivation lands there |
| Profile reset (`resetMainConsole`) | the field survives, as the layer does; the Host flags are unchanged |
| Profile closed with the field open | field dies with its parent; `closePasswordEntry` no-ops once the host is closing |
| Two profiles, one at a prompt (including multi-view) | each `Host`/`TMainConsole` pair has its own state and field; a field steals focus only from its own profile's command lines |
| Password contains the command separator (`;;`) | sent whole (an improvement: `Host::send()` would have split it) |
| Paste, including a trailing `\r\n` from a password manager | pasted; both line-break characters are stripped on submit; exactly one line goes out |
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
- `Host::setRemoteEchoingActive(true)` reads `mTelnet.characterModeDetected()` once, at the start
  of a hold, to mark it. An input event, not a guard.

The anomaly latch, character-mode recognition, the login-phase timeout and the late-password logic
are unchanged. Anyone tempted to improve them in this PR: do not.

## 4. Decisions and rejected alternatives

1. **Field stays open until the game releases ECHO, not closed on Enter.** Closing on Enter would
   leave the *retry* after a rejected password in the clear on games that hold ECHO across attempts
   - typed into the command line, into history, through aliases and `sysDataSendRequest`. Cost of
   staying open: on a slow link, text typed after the password but before WONT is discarded when
   the field closes. Visible, and what today's code does. The placeholder switching to "Sent -
   waiting for the game" after a submit tells the player what the lingering box is.
2. **Recognition never closes anything.** v1 fed cTelnet's character-mode flag straight into the
   policy and every reviewer rejected it: the flag is advisory, fires 3 s after any line on any
   server that merely *offers* SGA, and lasts the connection. v2 made it close the field one line
   later; round two showed that still put a retry in the clear whenever recognition was a false
   positive (a line-mode game that masks both prompts and stalls a wrong password). v3: recognition
   only *marks* the hold. The field says the game seems to hide everything, and Esc on the empty
   field then lasts until the game releases ECHO rather than one line. The player decides; a wrong
   guess costs a placeholder, never a password.
3. **Esc is "start over", then "step past", then "stop".** With text, Esc empties the field. On an
   empty field it dismisses until the next line reaches the game. If the game still holds ECHO
   after that line, it evidently hid a line the player sent normally, so the hold is marked and the
   field returns saying so; a second Esc then lasts until the game releases ECHO. One line also
   serves the player who types an alias name at the prompt: Esc, `pw`, Enter, and the field is
   back for whatever comes next. A game that hides everything is a per-game property, so its
   lasting answer is the per-profile preference.
4. **Text the player was typing when the prompt arrives moves into the field.** v1 left it in the
   command line, which split a fast typist's password (`hunt` behind the field, `er2` sent) and
   failed the login. The rule is a fact - every character on the line came from the player's keys
   or paste, starting from an empty or wholly selected line - not a guess about history. A command
   left selected by auto-clear-off, or recalled with Up, or put there by a script, is not moved.
   The counter-case, a command typed ahead into a silence broken by an unexpected mid-session
   prompt, becomes part of the password and fails visibly. With the auto-login pending no field
   opens, so #7921's typed-ahead commands stay put.
5. **After the auto-login has answered under the mask, no field until the game releases ECHO.**
   Without this, every auto-login player on a game that hides all input would meet a hidden-input
   box right after logging in, type their first command into it masked, and need two Escs per
   session. The cost is invariant 7's one exception: a *wrong stored password* on such a game is
   retried in the clear. That is the auto-login's own failure, on a rare game class, and the player
   sees it happen. The suppression is only set when ECHO was up at the send, so a game that never
   negotiates ECHO at login keeps its field for a later prompt.
6. **Lua writes to "main" go into the field; reads do not.** A script that pre-fills the line for
   the player to press Enter wants that text where the keyboard is. Reads keep the "getCmdLine
   never returns the password" guarantee; the existing spec case about the prompt ending keeps its
   final assertion because the field discards on WONT.
7. **Sibling of the command line, not a child.** A child would get geometry for free, but an
   ignored key event bubbles from a child to `TCommandLine::event()`. Fifteen lines of geometry
   tracking are cheaper than that class of bug.
8. **Focus proxy, not per-caller edits.** At least six places focus the main command line by
   pointer. `setFocusProxy()` makes all of them right without touching them. The window's focus
   child, not `QApplication::focusWidget()`, is what open/close consult, so an inactive Mudlet
   behaves.
9. **Fresh widget per prompt.** Deleting on close is the simplest proof that nothing (text, undo,
   reveal) carries over, and Qt zero-fills a password-mode line control on destruction.
10. **`sendData(..., isGameCommand=true)`, event withheld, warning never quotes withheld data.**
    The auto-login sends with the event withheld already; "withheld means sensitive" is the rule
    the code already lived by without saying so. The field's line must keep arming detection and
    the timeout as a command-line line did.
11. **Key surface is deliberately small.** Return, Esc, the caret shortcut, PageUp/Down, key
    bindings on non-printing or modified keys, Paste. Not: Ctrl+C copying the *console* selection,
    Ctrl+F, Ctrl+digit tab switching unless it lifts out cheaply. The field is a password box.
12. **No new Lua API** (`showPasswordEntry()` for games that never negotiate ECHO was proposed;
    it would answer "why no field on my game?", and it can come later without touching this
    design). No "hold and ask if the text matches an alias": that runs the alias patterns over the
    password, which is the leak.
13. **The recognition flag stays in cTelnet.** Its companions are reset together in `reset()` and
    every reader is in cTelnet. One call out plus one getter is smaller and does not collide with
    #10964.
14. **The auto-login-pending input is an explicit flag, not a timer query.** Its transitions are
    many (connect, name sent, password sent, GMCP takeover, keychain refused, reset); a flag with
    one setter that recomputes is auditable, a `QTimer::isActive()` read at recompute time is not.

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
`test/functional_tests/TelnetPasswordMaskTimeoutTest.cpp` (getter/setter at ~197/214/381),
plus a new `test/functional_tests/PasswordEntryPolicyTest.cpp` in the TELNET group (own file
named after the class; `friend class PasswordEntryPolicyTest` in both `ctelnet.h` and `Host.h`).

- Everything in A and the cTelnet items in E. `TCommandLine` keeps working as before (it still
  listens to `signal_remoteEchoChanged`; that signal goes in commit 2).
- Test (`PasswordEntryPolicyTest`, with a `RecordingTelnetServer` and the reset pattern from
  `TelnetPasswordMaskTimeoutTest::init()`): drive each input and assert `passwordEntryWanted()`
  and the signal count (one emission per change, none for a no-op). WONT clears all three flags;
  so does `reset()` while ECHO is already off (call `setRemoteEchoingActive(false)` twice).
  `markPasswordEntryHold()` changes nothing about wanted and emits its own signal once. Esc on an
  unmarked hold is undone by `Host::send()` (after the deferred recompute: `QTRY_`), which also
  marks the hold if ECHO is still up; Esc on a marked hold lasts until WONT. `setAutoLoginPending`
  transitions: start `mTimerPass` with `start(0ms)` (not by calling `slot_send_pass()` directly,
  which would leave the timer live for the next case) and `QTRY_` the suppression; with ECHO off
  at the send, nothing is suppressed. `sendPasswordEntry("x")` reaches the server, raises no
  `sysDataSendRequest`, runs no alias `^x$`, leaves the Lua `command` global alone, and goes out
  even after a stray `denyCurrentSend()`. The encoding warning: set `ASCII` through the path that
  runs `encodingChanged()` (or reset `mEncodingWarningIssued` through friend access), send an
  unencodable auto-login password, read `mpConsole->buffer.lineBuffer` as `CaretNavigationTest`
  does, assert the warning *appears* and does *not* contain the password.

### Commit 2 - The field, and the masking goes

Files: new `src/TPasswordEntry.{h,cpp}` (add to both `mudlet_SRCS` ~43 and `mudlet_HDRS` ~311 in
`src/CMakeLists.txt`; nothing else lists sources, lupdate scans `src/`), `src/TMainConsole.{h,cpp}`,
`src/TConsole.cpp` (`setProxyForFocus`, and any direct include the header trim exposes),
`src/TCommandLine.{h,cpp}`, `src/TLuaInterpreter.cpp`, `src/TLuaInterpreterMudletObjects.cpp`,
`src/TLuaInterpreterUI.cpp`, `src/Host.{h,cpp}` (`send()` clearing the dismissal, drop
`signal_remoteEchoChanged`), `src/ui/profile_preferences.ui`, `src/dlgProfilePreferences.cpp`,
`test/functional_tests/CommandLineKeyHandlingTest.cpp` (delete
`test_aPasswordIsNotKeptInTheHistory`; it pins the guard this commit removes),
`src/mudlet-lua/tests/CommandLine_spec.lua`.

- Implement B, C, D, the rest of E. One commit, because the removal in D is what makes the command
  line untouched; splitting it would leave a tree where both mechanisms fire.
- Spec rewrite (`CommandLine_spec.lua`):
  - "keeps a left-over command in the command line while the game asks for hidden input":
    `printCmdLine("main", x)`, `selectCmdLineText("main")` (a selected left-over, so the
    typed-ahead rule does not apply; and a script write is never typed-ahead anyway), WILL ECHO,
    `getCmdLine("main") == x`; a sub command line untouched; WONT, still `x`. Fails on today's code,
    which clears the line at WILL.
  - "does not leave the password behind when the prompt ends": WILL, `printCmdLine("main", pw)`,
    then `getCmdLine("main") == ""` (the write went into the field; say in a comment that this
    proves nothing about the field's contents, which Lua cannot see by design), WONT,
    `getCmdLine("main") == ""`.
  - Reword the header comment (the mask is no longer painted over the document) and keep the
    anomaly-budget comment: at most four ECHO toggles per describe block.

### Commit 3 - Functional tests for the field

New `test/functional_tests/PasswordEntryTest.cpp` in the WINDOW group (join
`WINDOW_GROUP_TEST_SOURCES`; do not make a standalone binary), `friend class PasswordEntryTest;`
in `ctelnet.h` and `Host.h`. Copy `CommandLineKeyHandlingTest`'s `initTestCase` verbatim -
including `TUiTour::rememberShown()` before `init()`, without which the first-run tour's
application-wide filter eats keys once the window is shown - then `show()` the main window,
`activateWindow()`, and `QVERIFY(QTest::qWaitForWindowActive(...))` as `SubCommandLineLifetimeTest`
does. Reach the field through `TMainConsole::passwordEntry()`, never `findChild` (a dying field
may still be a child). Hold the old field in a `QPointer` when asserting "a fresh one": the
allocator can reuse the address.

`init()`/`cleanup()` isolation, all of it, every case (the main line and the process-wide telnet
state are shared): zero `mEchoToggleCount`, `mEchoAnomalyDetected`, invalidate `mEchoToggleTimer`;
stop `mTimerPasswordModeTimeout`, `mTimerCharacterModeDetect`, `mTimerLogin`, `mTimerPass`; reset
`mServerRequestedSGA`, `mCharacterModeDetected`, `setAutoLoginPending(false)`; if ECHO is
negotiated send WONT through `loopbackTest()` - never `setRemoteEchoingActive(false)` alone, which
leaves cTelnet believing ECHO is on so the next case's WILL is ignored - and clear the Host flags
through friend access anyway (a case that never negotiated ECHO leaves them as they were);
`setCaretEnabled(false)` and restore `mCaretShortcut` (a leftover keyboard grab eats every later
key); clear the main line's text, selection and typed flag; `setLogin`/`setPass` empty;
`setDisablePasswordMasking(false)`; kill temp aliases, switch off perm keys, kill event handlers;
`QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete)`.

Harness facts the red team established:
- Under offscreen nothing has focus and hidden widgets get no Resize/Move events until the window
  is shown and active. Use `QTRY_COMPARE` for focus (`Host::setFocusOnHostActiveCommandLine`
  retries at 0/10/50 ms).
- Cases that prove the *proxy* must deliver keys to `window()->windowHandle()` with
  `QTest::keyClick(QWindow*, ...)` (exists; one character at a time, there is no `keyClicks` for
  a window), which goes to the focus object; a key sent to the `TCommandLine*` goes through the
  D.2 redirect and would pass without the proxy.
- Drive ECHO with `setRemoteEchoingActive()` where the negotiation is not the subject, and with
  `loopbackTest()` where it is.
- Recognition: arm through a real submitted line under loopback WILL SGA + WILL ECHO, then
  null-check and `mTimerCharacterModeDetect->start(0ms)`; no 3 s wait.
- Wire reads: `waitForServerToReceive()` spins the event loop; #10978 reported that synthetic keys
  then stopped reaching the main line. With `TUiTour::rememberShown()` in place that is believed
  explained; write a ten-line probe first anyway, and if it reproduces put wire-reading cases in
  their own class.
- At most four ECHO toggles per case.

Cases. Each comment names the line whose revert turns it red, or says "smoke" / "belt and brace"
where no single line does:

1. WILL ECHO opens a field over the command line with its geometry; a selected left-over command
   in the command line is unchanged; the window's focus widget is the field.
2. Typed-ahead: type `hunt` into the main line, WILL ECHO; the field holds `hunt`, the command line
   is empty; type `er2`, Return; the server receives `hunter2`. Then: recall a command with Up,
   WILL ECHO: it is not moved.
3. Type + Return in the field: server receives the text; command line unchanged; a
   `sysDataSendRequest` handler saw nothing; an alias `^<password>$` did not fire; the Lua `command`
   global is unchanged; after WONT, Up in the command line recalls the previous command, not the
   password (no single revert line; belt and brace).
4. Field stays open after Return until WONT; its placeholder changed; WONT closes it and focus
   returns to the command line.
5. WONT while the field holds text: nothing sent, text gone, command line unchanged.
6. Esc with text empties the field and keeps it; Esc on the empty field closes it; a line typed
   into the command line now expands an alias although ECHO is held; after that line a fresh field
   (different `QPointer`) is open and marked; Esc on it closes it and nothing re-opens after a
   further line; loopback WONT then WILL opens one again.
7. Preference on: no field on WILL ECHO (the history half of #8902 is not red-able: today's guard
   already allows it; assert it anyway).
8. Recognition with a field open: loopback WILL SGA + WILL ECHO, a submitted line, fire the
   detector; the field is still open and marked; a further Return keeps it open; Esc on the empty
   field closes it; no field re-opens while ECHO is held; loopback WONT then WILL opens a new one,
   already marked because recognition fired on this connection; Esc on it closes it again.
9. Auto-login: with credentials set and `mTimerPass` started, WILL ECHO opens no field; fire the
   timer with `start(0ms)` and `QTRY_` that the password went out; still no field until WONT;
   typed-ahead text in the command line survived. Then the same with ECHO off at the send: a
   later WILL opens a field.
10. A key binding on F7 doing `send("frombinding")` fires from the field, the server receives it,
    the field is still open; a binding on plain `a` does not fire and `a` is typed.
11. Reveal toggle flips `echoMode()`; select all, Ctrl+C in the revealed field leaves the
    clipboard unchanged; Ctrl+Z after a submit does not bring the text back (two layers, no single
    revert line).
12. Tab in the field: text unchanged, focus unchanged (Up/Down cannot go red: `QLineEdit` ignores
    them anyway; assert as belt and brace). Caret shortcut (set `mCaretShortcut` to F6 for the
    case) moves focus to the pane and leaves the field open; a printable key typed on the pane
    lands in the field.
13. Synthetic key path: `qApp->sendEvent(mpCommandLine, keyPress('a'))` while the field is open ends
    with `a` in the field and the command line unchanged (D.2).
14. Focus the sub command line first, *then* open the field: `mpCommandLine->setFocus()` and
    `Host::setFocusOnHostActiveCommandLine()` both land on the field (proxy + B.8).
15. Resize the console; the field's geometry follows the command line's.
16. Sub command line action runs during a prompt (E).
17. `printCmdLine("main", x)` and `sendCmdLine(x)` during a prompt land in the field;
    `getCmdLine("main")` still returns the command line's text.
18. Paste: clipboard text `pw\r\n`, Ctrl+V, Return; the wire holds exactly `asSent({"pw"})`
    (`sendData` strips LF on its own, so only the CR proves B.6).
19. Context menu: call `contextMenuEvent`; exactly one action, Paste.
20. Window inactive: activate a second top-level `QWidget`, `QTRY_` that Mudlet is not the active
    window, close the field (WONT); reactivate; the command line has focus.
21. Closing the profile with the field open (`HostChildTeardownTest`, TEARDOWN group:
    `setRemoteEchoingActive(true)`, then `forceClose()`, `requestClose()`, `deleteHost()`; smoke).

Existing tests that must still pass: `TelnetLatePasswordTest`, `TelnetPasswordMaskTimeoutTest`,
`SettingsRoundTripTest`, the rest of the WINDOW group minus the deleted case.

### Commit 4 - Wording and docs

- Preference label/tooltip/accessible description (E). Translator comments on every new `tr()`.
- If `docs/` mentions password masking, update it; the manual lives on the wiki - note the wiki
  edit in the PR body. Delete this plan file in the final commit, or move its §1 and §3.3 into
  `docs/` if the maintainers want the design recorded.
- PR: read `.agents/skills/open-pr/SKILL.md`. Title `Improve: Passwords are typed into a dedicated
  field instead of a masked command line`. Body per the template, with `**Test case:**` steps, the
  `Assisted-by` trailer in the body (a squash merge drops commit trailers), the greps from
  invariant 3, an NVDA/VoiceOver check in the manual test steps, and a demo video per
  `docs/demo-videos.md` if feasible. List #10965, #10972, #10978 as superseded and #10968 as
  reduced to its "scripts keep aliases" tests, and say why. Open as draft until the human has
  tested it.

## 6. Test strategy notes

- Specs where Lua can see the behaviour (the command line being untouched); functional tests for
  everything key-driven or about focus/geometry, since Lua cannot press keys.
- Both harnesses fail silently when set up wrong: for every new case, break the code once and
  watch the case go red before trusting it. Record which line was broken in the case's comment.
- The ECHO anomaly counter, the recognition flag, caret mode's keyboard grab and the Host flags
  are all process- or profile-wide state; reset every one of them per case.
- `feedTelnet()` in specs needs the self-test profile started with `--offline`; the README in
  `src/mudlet-lua/tests/` says how, and `.claude/scripts/run-lua-tests.sh` runs the suite.

## 7. Risks and open questions (for the reviewer and the human tester)

1. Accessibility bridges: whether AT-SPI/IA2 forward the raw characters of Qt's text-insert
   events for a password-mode line edit is unverified. Test with NVDA (Windows) and VoiceOver
   (macOS) that typed characters are not spoken, and with Accerciser or `dbus-monitor` on Linux.
2. Whether Qt enables macOS secure event input for a password-mode `QLineEdit`, and whether that
   depends on `ImhHiddenText` (B.2 keeps the hint set in both echo modes).
3. Revealed text can be selected into the X11 primary selection, and Lua can read the clipboard.
   Reveal is the player's deliberate act; documented, not prevented.
4. Stylesheets: `setCmdLineStyleSheet("main", ...)` styles a `QPlainTextEdit`; the field will not
   pick that up, and a profile-wide `QLineEdit` rule will. It uses the command line's palette and
   font. Known cosmetic limit.
5. A first-time player on a game that holds ECHO from connect sees a hidden-input box at the name
   prompt. The placeholder names Esc, and the one-time info line names the preference. Whether that
   is enough is a question for the human tester.
6. The typed-ahead move (C.3, D.3) is the one place the design touches the command line's text. It
   is fact-based, but it is the most intricate part of the change (the guard must wrap every user
   edit path and be lowered around every programmatic write inside one). If it proves fragile, the
   fallback is to leave the text in place and accept the split-password failure it was added to
   avoid; §4.4 records that trade.
7. The AltGr/Option rule in B.5.8 is from memory of how those modifiers arrive; check on Windows
   and macOS that `@`, `€` and accented characters type into the field.
8. Mudlet Web shares the specs; nothing in the tests directory references it, and the rewritten
   cases use `feedTelnet` exactly as the current ones do.

## 8. Size, honestly

Removed from `TCommandLine`: ~290 lines (the 1694-1936 block, the ctor, `processNormalKey`,
`mousePressEvent`, the history guard, ~20 header lines) and the six members; ~10 lines elsewhere.
Added: `TPasswordEntry` ~220 with headers, `TMainConsole` ~100, `Host` ~110, `cTelnet` ~30, the
redirect and the typed-ahead fact ~40, Lua redirects ~30. Net product code roughly +200; tests
~+700. What is gone is the guessing (history scan, selection bookkeeping, typed-during tracking)
and the per-feature guards; what is added is a widget with a defined key surface, a policy function
with five named inputs, and one fact about the command line. Five inputs is not "one state", and
the plan does not claim it; the claim is that the five are combined in one function and read
nowhere else, and that nothing Mudlet infers can ever remove protection.

## 9. Red-team log

### Round one (v1 → v2): five reviewers - telnet/server compatibility, Qt mechanics, leak paths, feasibility/tests, skeptical maintainer

Accepted: recognition closing the field on a timer and for the connection; Esc dismissal until
WONT; the encoding warning quoting the password (also fixes the auto-login); typed-ahead password
split; caret-mode shortcut unreachable from the field, PageUp/Down, profile-switch override; focus
bookkeeping while Mudlet is inactive, `recordActiveCommandLine` going stale, the accessibility
focus event naming the wrong widget, `deleteLater` without `hide`; undo after reveal, hints
cleared by `setEchoMode(Normal)`, Copy/Cut while revealed, `secureStringClear` being theatre,
`clear()` before destruction defeating Qt's zero-fill; sub command lines as a place the password
lands by momentum; Lua writes to "main" during a prompt; auto-login races; stale
`denyCurrentSend()`; Paste-only context menu; multi-line paste; numpad bindings eating digits;
the flag move; the incomplete preference-site list; the WINDOW test that goes red; harness
isolation and the hidden window under offscreen; `findChild` on a dying field.

Rejected, reasons in §4: close-on-Enter; a heuristic for the Enter-before-WILL-ECHO window;
"hold and ask if the text matches an alias"; a `showPasswordEntry()` Lua API; moving
`mCharacterModeDetected` into Host; requiring three lines before recognition counts.

### Round two (v2 → v3): two reviewers - policy state machine, implementation and tests

Accepted: auto-login suppression set while ECHO was off and surviving `reset()` through the
change guard (A: unconditional clearing, suppress only under ECHO); recognition-after-next-line
still dropping protection on a false positive (§4.2: recognition only marks); the auto-login
input changing at transitions that never recomputed (§4.14: explicit flag; reuse
`cancelLoginTimers()`); `typedSinceLastSubmit` not being a fact (D.3: player-typed line from a
blank or wholly selected start, `contentsChange` handler, guard lowered around programmatic
writes); sub command lines and scripts ending a dismissal (A: `Host::send()`); the no-SGA
all-hiding game needing an Esc per line (§4.3: a marked re-open, then Esc lasts until release);
the late keychain password typed over a half-typed field (B.7); the spec's middle assertion;
`Qt::PreventContextMenu` suppressing `contextMenuEvent`; the Lua writers living in
`TLuaInterpreterMudletObjects.cpp`; four test cases that passed without the fix; the double send
in the auto-login case; the incomplete isolation list; `TUiTour::rememberShown()` and
`activateWindow()`; invariant 3's grep scoped to `src/`; teardown guards; the zero-fill wording;
the info line landing inside the prompt; null focus widget as "steal"; wording of the SGA and
preference rows.

Rejected: ending an auto-login suppression at the next submitted line (moot: it is no longer set
while ECHO is off); a link in the info line that flips the preference (no `setConfig` key exists;
name the setting instead).
