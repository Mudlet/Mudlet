# Running Mudlet with no window

`mudlet-headless` runs one Mudlet profile with no window: in a terminal, under a script, or on a
server. It is built from the same `mudlet_core` library as the GUI, so a profile behaves the same in
both: its triggers, aliases, timers, packages and map are the ones the GUI would load, and it is
saved to the same files. `THeadlessSession`, the class it is built on, is what an app of its own,
such as a phone app, uses to run a profile in the same way (#9011).

## Building it

It is on by default (`USE_HEADLESS`), so any build tree that builds the GUI can build it too:

```bash
cmake --build build --target mudlet_headless_executable
```

The executable is `build/src/mudlet-headless`. The `linux-core-minimal` preset builds only it, with
what a mobile build has to leave out turned off and Qt Widgets out of reach, so that CI fails if
`mudlet_core` needs either again; see "Leaving out what a mobile build cannot use" in
`platform-builds.md`.

With `-DUSE_GUI=OFF` (or `WITH_GUI=NO` in the environment) a build tree has no desktop GUI at all:
Qt Widgets need not be installed, and neither the `edbee-lib` nor the `qt-tags-widget` submodule
needs checking out. Only `mudlet_core`, `mudlet_headless` and `mudlet-headless` are built, with no
tests, as every test binary links the GUI; `WITH_SENTRY` needs the GUI too.

It needs no display. With `QT_QPA_PLATFORM` unset it uses Qt's `offscreen` platform, and does not
pass that on to the programs a script starts. Set `QT_QPA_PLATFORM` yourself to use another one.

## Running it

```bash
# Connect with a profile made in the GUI, as its connection dialog would
mudlet-headless -p "My MUD"

# Make a profile, or update one, from the command line
mudlet-headless -p "My MUD" --host mud.example.com --port 4000 --tls --login hero --password-env MUD_PASSWORD

# Run some Lua against a profile without connecting, and exit
mudlet-headless -p "My MUD" --offline --no-stdin -e 'display(getMudletVersion())' -e 'closeMudlet()'
```

`mudlet-headless --help` lists every option. It shares the GUI's config folder and keychain
entries, so a profile it saves opens in the GUI, and the other way round; do not run both on one
profile at once, as each saves over what the other saved. `Mudlet.ini` is shared by every profile,
so its save also writes the application-wide settings below as this run has them, over a change the
GUI made meanwhile.

`--config-dir <folder>` keeps the profiles, `Mudlet.ini` and the passwords stored for them in that
folder instead, made if need be, so a run there shares no profiles, settings or passwords with the
GUI, and takes its interface language from that `Mudlet.ini`; a relative path is taken from where the
runner is started. A folder that cannot be used is a command line error.

### Input and output

- **Standard output** gets each line the game sends, and each a script feeds with `feedTriggers()`,
  as it reaches the main console and before the profile's triggers run on it. On a terminal, other
  than on Windows, it is in the colours and styles the game asked for, unless `NO_COLOR` is set or
  `TERM` is `dumb`; `--color always` or `--color never` says otherwise, and pipes and files get
  plain text by default. The profile's own colours are left to the terminal's theme, and 24-bit
  colour is used when `COLORTERM` is `truecolor` or `24bit`, else the nearest of 256.
- **Standard error** gets what the GUI shows only on the profile's own screen: `echo()`, `print()`,
  the commands sent, Mudlet's own messages, and the error console.
- **Standard input** is read one line at a time, each as if typed on the command line, so the
  profile's aliases run on it. It is read once the connection is made, so nothing goes before the
  game can take it; with `--offline`, at once. Use `--no-stdin` for a background run.

Packages given with `--package` and Lua given with `-e` run once the profile has loaded, usually
before the connection is made, so what they send the game may be lost. Send it from a
`sysConnectionEvent` handler instead.

### The terminal's size

When standard output is a terminal, the runner gives its size to the profile as the main console's
size, as the GUI gives the main window's: the game is told it if the profile has NAWS on and the
game asks, no wider than the profile's wrap width, and scripts see it as the main window's size.
A resize (`SIGWINCH`) is passed on as a resize of the main window is, with a
`sysWindowResizeEvent`. Output to a file or a pipe keeps the default of 90 by 25. This is not done
on Windows.

### Stopping it and what is saved

The profile closes, and is saved, when:

- a script calls `closeMudlet()` or `closeProfile()` (the latter saves only if the profile's
  "Auto save on exit" setting is on);
- standard input ends, or standard output's reader goes away (as `| head` does);
- the connection ends, with `--exit-on-disconnect`;
- the runner gets `SIGINT`, `SIGTERM` or `SIGHUP`. A second `SIGINT` or `SIGTERM` ends the run at
  once, without the save.

The save is the GUI's end of session save: the profile, its synced modules, its map, `profile.ini`,
the command line histories, and the application-wide settings in `Mudlet.ini` that a profile with
no window can change, such as the media mutes and `showTabConnectionIndicators`. Settings only the
GUI has, such as its window position or toolbar, are left as the GUI last saved them. As in the
GUI, a command line history or `Mudlet.ini` that cannot be written is only reported on standard
error, and does not change the exit status; a history file that cannot be read is not written over.

A command typed on standard input goes into the main command line's history file, just as the GUI
keeps it: one entry for each line or paste, not for one typed while the game hides input (a
password prompt, which an app embedding the session hears of through `hiddenInputWanted`, below), so
the GUI's up arrow finds it next time.

### Exit status

| Status | Meaning |
| --- | --- |
| 0 | The profile closed, standard input ended, or standard output's reader went away. |
| 1 | The profile could not be opened or saved, a package or `-e` Lua failed, standard input could not be read, or a line of it, or a package or `-e` Lua without `--offline`, sent the game something while there was no connection. |
| 2 | The command line was wrong. |
| 3 | Standard output was closed when the run began, or could not be written. |
| 4 | The connection ended or could not be made, with `--exit-on-disconnect`. |
| 128+n | Signal n ended the run. |

Where more than one applies, `--help` says which is given.

## Embedding it: `THeadlessSession`

`src/headless/THeadlessSession.h` is the API an app of its own uses to run a profile. It touches no
terminal or standard stream, and, like everything in `mudlet_core`, it is used on the
application's thread only. Its header documents each call; in outline:

```cpp
#include "THeadlessSession.h"

// After the QGuiApplication exists (on the offscreen platform where there is no display), with the
// organisation and application names the GUI uses, as the session reads Mudlet.ini and the keychain
// under them
QCoreApplication::setOrganizationName(qsl("Mudlet"));
QCoreApplication::setApplicationName(qsl("Mudlet"));

auto session = std::make_unique<THeadlessSession>();
QObject::connect(session.get(), &THeadlessSession::outputLine,
                 [](const QString& profile, const QString& text, bool isPrompt, const QList<THeadlessSession::TextRun>& runs) {
    // A line from the game, to show; runs give its colours, bold, underline and so on, piece by piece
});
QObject::connect(session.get(), &THeadlessSession::localText, [](const QString& profile, const QString& text) {
    // echo(), print(), the commands sent and Mudlet's messages; does not always end a line
});
QObject::connect(session.get(), &THeadlessSession::quitRequested, [] {
    // A script closed the profile
});

THeadlessSession::Options options;
options.profile = qsl("My MUD");
if (session->start(options) != THeadlessSession::StartResult::started) {
    qWarning() << session->errorString();
}

// Later, from the app's own input and events
session->submit(qsl("look"));        // as typed on the command line
session->runLua(qsl("echo('hi')"));  // as a script
session->setScreenSize(80, 24);      // in characters, as a resize of the main window
session->save();                     // before the system may stop the app, as a phone does
session->shutdown(true);             // sysExitEvent, then the save
```

`options.configDir` names the folder to keep the profiles and `Mudlet.ini` in, in place of the GUI's
config folder, such as an app's own data folder; it is made if need be. The passwords stored for its
profiles are kept in it too, under `credentials/`, not in the system's keychain or the GUI's own
password files, so a profile there shares none with a GUI profile of the same name. It is the folder
until the process ends, so a later `start()` may name it again, but no other, nor leave it out for the
GUI's. Nor may it be any folder the GUI could keep its profiles in, whose passwords are kept elsewhere:
`$XDG_CONFIG_HOME/mudlet`, `~/.config/mudlet`, the Flatpak's `~/.var/app/org.mudlet.mudlet/config/mudlet`
and the folder a `portable.txt` beside the app's executable or in `~/.config/mudlet` names, by any
path that leads to one, even before it is made. Not caught: a portable install whose `portable.txt` is
beside another executable, and a GUI run under another HOME. Naming one of their folders shares its
profiles and `Mudlet.ini` with that GUI (not its passwords), so `start()` warns when a folder it accepts already holds profiles.

Each of `outputLine`'s runs covers part of the line, in order, with the colours and styles the game
asked for (`THeadlessSession::TextStyle`: bold, italic, underline, overline, strike-out, reverse, blink,
fast blink, concealed), and says whether each colour is the profile's own, which a frontend may swap
for its own theme's. Links in the line, and what `echo()` and the like print, come as plain text for now.

The signals' handlers are called while a line is being written, so they must not call the session
back or run the event loop; queue the work for later instead. `host()` reaches what the session
does not cover, such as settings and the map, and goes null once the profile closes, so hold it in a
`QPointer`.

A web page a script opens with `openWebPage()` or `openUrl()`, and a game's sign-in page, go to
`QDesktopServices::openUrl()` as in the GUI, so the system's browser opens them unless the app takes
their scheme first with `QDesktopServices::setUrlHandler()`, as a phone app might to show them itself.

`notificationRequested` carries a script's `showNotification()`, which answers `true` only while
something is connected to it, so an app that connects it and drops what it gets tells scripts their
notifications were shown. `attentionRequested` carries a script's `alert()` and the game's bell, with
how long to ask for the player's attention: 0 until they come back, and 3 seconds for a bell, as the
GUI asks. That is the GUI's flash of its window, not its beep, which it leaves out for a bell while the
game's sounds are muted. The session shows, flashes and beeps nothing itself. `announcementRequested`
carries what a script's `announce()`, Mudlet when it opens a sign-in page, or a game's OSC 8 link being
hidden or revealed asks screen readers to say, and whether the GUI would make it assertive, for an app to
hand to the platform's own accessibility service.

`secureConnectionOffered` carries a game's offer of its secure port, worded as the GUI's question, for
the player to answer with `answerSecureConnectionOffer()`. The offer lapses when its connection ends;
until then, or an answer, the profile offers nothing more. A yes stores the secure port with TLS on, as
the GUI does, so an app that passes its own port to a later `start()` passes `tls` with it.

`packageDownloadStarted`, `packageDownloadProgress` and `packageDownloadFinished` follow a GUI package
the game sends, as the GUI's progress dialog does, and `cancelPackageDownload()` is its Cancel.

`mapProgressStarted`, `mapProgressChanged` and `mapProgressFinished` follow a map export, import or
download, as the GUI's progress dialog does, each change carrying the whole label, the rooms done and the
rooms to go through. They are sent from the event loop. An export or import runs it as it goes, so its
start and changes can arrive while the script that began it, or the `start()` that loads a JSON map,
still runs, but its end arrives only once that has returned. A map download, which only an app can begin,
through `host()`, carries on after its script returns.

A map operation or package download still open when its profile closes, or when `shutdown()` or a later
`start()` lets the profile go, gets its end there, so every start an app is sent is followed by one end.
A session that is deleted sends none.

`cancelMapOperation()` is the dialog's Abort: a JSON export or import then gives its script `nil` and
"aborted by user", and a download stops. It works on the JSON map `start()` loads too, called from a
handler of its `mapProgressStarted`, and the profile then starts with no map, as in the GUI. `true` means
the request was made, not that the operation stopped: a JSON import with no areas has no step left at
which to stop, and completes. `mapProgressCancelDisabled` says when the dialog would drop its Abort: at
once for an XML import, and for an export writing its file or a download that has arrived. After it,
or with nothing running, the call answers `false`.

`mappingScriptMissing` says the map opened with no mapping script installed, where the GUI suggests the
player finds one, for an app that offers its own. As in the GUI, it comes only the first time the map
opens once the profile has loaded, so an app that connects it later may have missed it. The profile
marks it asked at that moment, whether or not anything listens, the reference runner included, and a
save keeps that, so it never comes again for that profile, in the GUI either. When `shutdown()` or a
later `start()` follows the map opening at once, it is still delivered once the event loop runs, as
long as the session has not been deleted by then.

`hiddenInputWanted` says when the game starts hiding what the player types, as for a password, and when
it stops, where the GUI opens and closes its hidden-input box, so an app can mask its own input line
meanwhile. A disconnect ends it too, as does the profile closing or being let go by `shutdown()` or a
later `start()`, though not the session being deleted. It never comes while the profile's "Show passwords as you type them"
setting is on, as the GUI then opens no box. It is sent while the game's data is read, so, like
`outputLine`, its handler must not call the session back. Meanwhile `submitHiddenInput()` sends a line
as the box does, straight to the game with no aliases, `sysDataSendRequest`, echo or history, while
`submit()` still runs aliases on it as it always has; `hiddenInputEdited()` is the player's first edit of
the masked line, which stops a saved password that arrives late from the keychain being sent for them, as
typing in the box does, so an app that moves text the player had typed ahead into its masked line calls
it then, as the GUI does when it moves such text into its box; and `dismissHiddenInput()` is the box's Esc,
which hands input back to `submit()`.
When `submitHiddenInput()` refuses a line, as the box has closed, the profile is not connected or the
game's encoding cannot carry it, an app drops that line, as the GUI drops what was typed into a box that
closes; resending it with `submit()` would echo it, keep it in the history and hand it to aliases.

`replayStarted`, `replaySpeedChanged` and `replayFinished` follow a replay of the session's profile, as
the GUI's replay toolbar does, whether a script's `loadReplay()` began it or the app did through
`host()`. A script's replay says nothing when it reaches the end of its file, so `replayFinished` is how
an app knows its output is over. A replay cut short, as by its file changing or becoming unreadable while
it plays, gets the same end, with the reason only in `localText`. A corrupt file, which `loadReplay()`
reads through before it plays anything, sends neither, and an empty one, which `loadReplay()` accepts,
sends its start and then its end. Every
replay starts at 1 times its speed, and a change clamped at 1 or 1024 times sends the same speed again.
They come from the event loop, so the start can arrive after the
script that began it has returned. One replay plays at a time over all profiles, and only those of the
session's own profile are sent. As for the map's progress, one still playing when its profile closes, or
when `shutdown()` or a later `start()` lets the profile go, gets its end there.

`pressKey()` is a key pressed on the GUI's input line: it offers the key to the profile's key bindings
where that line would, and its `KeyResult` says what came of it. The binding's script runs before the
call returns, as an alias does for `submit()`, so the session's signals may be sent meanwhile.

- `bindingRan`: a binding took the key, and the app does nothing more with it.
- `noBinding`: no binding took it, so the app does what the GUI's line does then. That is to type the
  character, a keypad digit included. Ctrl+1 to Ctrl+9 and Ctrl+0 switch to that profile tab, which
  the app can do with `setActiveProfile()`, and Ctrl+F opens the search of the main console.
- `keyKept`: the GUI's line acts on the key itself, so the app handles the key. A binding on it does
  not fire in the GUI either, save for a key on the keypad, which is offered to the bindings first and
  kept only once none takes it. On the command line these keys are:
  - Return, alone or with Ctrl or Shift;
  - Enter, alone; a real Enter is on the keypad, so it is kept when no binding on Keypad+Enter takes it;
  - Tab, and Shift+Tab (Qt's `Key_Backtab`, with or without Shift);
  - Space, alone or with Shift;
  - Backspace, alone or with Ctrl, Shift or both;
  - Delete, Escape, Page Up and Page Down;
  - Up and Down, alone or with Ctrl (on macOS these carry the keypad flag, so they too are offered to
    the bindings first);
  - the key the accessibility settings set to toggle caret mode.

  While hidden input is wanted, the GUI's hidden-input box takes the place of the line. It keeps
  Return, Enter, Escape, Tab, Up, Down, Page Up and Page Down, each alone, as well as Shift+Tab, copy,
  cut, undo and redo. It also keeps every key that types a printable character, alone or with Shift or
  AltGr (Option, too, on macOS), and types it into the password, so a keypad digit bound to a direction
  still goes into the password. The app types those into its own hidden input. The box judges a key by
  the text it typed, which the app gives as `pressKey()`'s third argument, as Qt's `QKeyEvent::text()`
  has it, empty when the key typed nothing. Without it, any key with a character's code counts as
  printable, and Ctrl+Alt with one as AltGr, so a binding on Ctrl+Alt+S, say, is not offered the key
  while hidden input is wanted, though the GUI offers it on a layout where that types nothing.
- `refused`: the call was refused, as `submit()` is. Use `errorString()` to see why; the app should not
  type the key.

A binding matches its key and modifiers exactly, as Qt's key event gives them, which a frontend that is
not built on Qt has to copy:

- A key on the keypad carries `Qt::KeypadModifier`, as a binding that walks the map with the number pad
  expects. On macOS the arrow keys carry it too.
- A shifted symbol is its own key with `Qt::ShiftModifier`: on a US layout, Shift+1 is `Qt::Key_Exclam`
  with Shift, not `Qt::Key_1`.
- A letter is its capital key code whether or not Shift is held, so a capital A is `Qt::Key_A` with
  `Qt::ShiftModifier`.
- AltGr arrives as Ctrl+Alt on Windows, and on X11 it can carry `Qt::GroupSwitchModifier`.

`commandLineTextRequested` carries the whole of the main command line each time a script's
`printCmdLine()`, `appendCmdLine()`, `clearCmdLine()` or `sendCmdLine()` sets it, for the app's input
line to show as the GUI's command line does; `clearCmdLine()` sends an empty line. As the GUI's line does,
the main line holds what `submit()` sends while its aliases run, so `getCmdLine()` there reads it, and
if the profile clears the line after a send, an empty line follows the send. It is sent as it is set, so
it keeps its place among the game's lines and `hiddenInputWanted`, and a handler may neither call the
session nor run the event loop. Only the main command line is sent: a line from
`createCommandLine()` or Geyser is not. Mudlet does not know what the app's user has typed until the
app says with `setCommandLineText()`; until then `getCmdLine()` reads what a script or `submit()` last
set. Nothing is sent:

- while hidden input is wanted: the GUI writes such text into its hidden-input box instead, so it is
  lost, neither kept for `getCmdLine()` nor sent;
- for the select-all of `sendCmdLine()` or `selectCmdLineText()`, which the app may leave out;
- once `shutdown()` starts or the profile closes, when the line is the app's to clear.

When `hiddenInputWanted` turns on, the app sets aside the text its input line holds from a script, as
`commandLineTextRequested` last gave it, keeps it out of its masked line, and shows it again when hidden
input ends, as the GUI's command line stays behind its hidden-input box; only what the player typed
ahead moves into the masked line, when Mudlet says so (see `setCommandLineText()` below).

`submit()` stands for the app's input line being sent, so it holds the line and then empties or keeps it
as the profile says, as the GUI's line does. A command sent from anywhere else, such as a button or a
menu, goes through `runLua()` calling `expandAlias()` with the command as a quoted Lua string: that runs
its aliases and sends it as a script would, and leaves the line and the command history alone.

`setCommandLineText()` is the other way: what the player has typed so far on the app's input line,
which a script's `getCmdLine()` then reads, as it reads the GUI's command line, so a key binding or
package that acts on the line being typed sees it. It is stored as plain text, as the GUI's line gives
it back. The app calls it as the line changes. It need not empty the line after `submit()`: the line is
then as the profile leaves it, and `commandLineTextRequested` says so when Mudlet empties it. It sends
no `commandLineTextRequested`, so an app that hands that signal's text back, once the signal is over,
starts no loop. Text the app sets on the line itself, rather than the player typing it, goes with
`CommandLineEdit::notTyped`: a history recall, a Tab completion, and text from a script it set aside
while hidden input was wanted and puts back. The GUI's line does not count such text as typed either,
though it does count what it puts back from its hidden-input box, which the app hands back as typed.

What it guarantees about hidden input is narrow:

- While `hiddenInputWanted` is true, only an empty line is taken; other text is refused.
- When the game asks to hide input and the line holds what the player typed, not what a script set,
  Mudlet overwrites and empties its copy before a trigger runs on any line that reaches Mudlet after
  that request, as the GUI's hidden-input box takes such text from its command line. A line ended by a
  go-ahead ahead of the request, even in the same read, has its triggers run on the line as it was, in
  the GUI too.
- A line counts as the player's when it was typed on an empty line, or typed into a line the GUI would
  have selected: one a `submit()` kept as the profile does not clear it, one a script set with
  `sendCmdLine()` or selected with `selectCmdLineText()`, one Escape selected (`pressKey()` says
  `keyKept`), or one set as `notTyped`. Mudlet does not see the app's caret or selection, so any change
  to such a line counts as typed, and so does any change that leaves a line no longer starting with what
  it held, as when the player selects it in the app and types over it, or deletes some of it. A change
  that still starts with a script's line keeps it the script's: typing on at its end, as in the GUI, but
  also a whole-line replace in one step that happens to start with it, such as pasting `bobby123` over a
  selected `bob`, unless Mudlet marked the line selected. Both rules err towards moving the line into
  hidden input.
- Mudlet then sends `typedLineMoveRequested`, just before `hiddenInputWanted`; only then does the app
  move its line's text into its hidden-input field and empty its line. An empty
  `commandLineTextRequested` is a script's `clearCmdLine()` or a send, never this. If the game ends
  hidden input before the player has edited or sent that text, and the app's line is still empty, the
  app puts the text back in its line and hands it back with `setCommandLineText()`, as the GUI puts it
  back in its command line. A disconnect does not put it back, in the GUI either.
- Nothing else is guarded. While the player has dismissed the hidden-input box, an auto-login is still
  pending, or the profile has masking turned off, the game may hide what is typed with
  `hiddenInputWanted` false; text set then is taken, and scripts can read it, as they can read the
  GUI's command line then.

`pauseReplay()`, `resumeReplay()` and `stopReplay()` are the GUI's replay toolbar for a replay of the
session's profile. A paused replay plays nothing more until it is resumed, and then carries on with what
was left of the wait it was paused in. `replayPaused()` says whether it is held, as the toolbar reads it;
a replay paused before its `replayStarted` arrives is still held when it does. Pausing one already
paused, or resuming one that is not, is false with the reason in `errorString()`. `stopReplay()` ends it
as the toolbar's Stop does, and `localText` says so. Each is false when no replay of the session's
profile plays, though one a script has just begun counts before its `replayStarted` arrives. Each is
also false while a script of the profile runs, as an app's timer can fire in a nested event loop in the
middle of a replayed chunk, which the replay reads on from where it was once the script is done.
`changeReplaySpeed()` is the toolbar's faster or slower button, which doubles or halves the speed within
1 and 1024 times; `replaySpeedChanged` follows. As the toolbar's button does, it is true at either end
too, where the speed stays as it is and `replaySpeedChanged` sends it again, so `replaySpeed()` is the
way to tell whether there is further to go. It is allowed while paused, and the rest of the wait plays at
the new speed once resumed. `replaySpeed()` is the speed, or -1 when no replay of the session's profile
plays.

`replayElapsedMSecs()` is the time the toolbar shows, by the recording's own timestamps, or -1 when no
replay of the session's profile plays. It is the time of the line the replay is waiting to play, not of
the last one shown: it moves to the end of a gap as soon as that gap begins, so a paused replay reads the
time of a line it has not shown yet, and holds it until resumed. Unlike the toolbar's clock it counts on
past a day. A speed change leaves it alone.

`replayStarted` and `replayFinished` come in pairs, in that order, however the replay ends: at the end
of its file, by `stopReplay()`, or when the profile closes or `shutdown()` runs. A replay stopped before
its `replayStarted` arrives says so in `localText` first, and both signals still follow; if `shutdown()`
comes before the event loop runs, it sends both.

`commandHistory()` is the main command line's history, newest first, for the app's input line to walk
with its up arrow as the GUI's does: each command `submit()` sent, once, at its latest place, and those
kept from the profile's earlier sessions, whether in the GUI or with no window. What was sent while hidden
input was wanted is not in it, as the GUI keeps none of what it typed in its hidden-input box. As in the
GUI, what is kept is the line as its aliases left it: one that clears the line with `clearCmdLine()` keeps
nothing, and one that writes to it with `printCmdLine()` keeps what it wrote. A paste is one entry, with
its lines joined by a line feed, until the profile is saved; the history file holds a line each, so after
the profile is loaded again each of its lines is an entry of its own. The list is a copy, which the next
`submit()` leaves as it is. It is empty both for a profile with no history and when no profile is open;
`host()` tells the two apart. A history file that cannot be read is left as it is: none of it is offered,
this session's commands are not saved to it, and `errorConsoleText` says so as the profile starts.

`src/headless/main.cpp`, the terminal runner, is the reference frontend: it shows how to handle
signals, a reader that goes away, and input typed before the connection is made.

A script's `loadProfile()` opens more profiles in the same run. With no window, none of them is in
front, as a tab is in the GUI, until a script names one with `setActiveProfile()`. As a tab change
does, that raises `sysProfileFocusChangeEvent` with `false` in the profile it leaves and `true` in the
one it brings forward, and shows that profile's Discord presence. A profile whose close has started is
refused with `false` and a message, and the front stays where it was.

## What still needs a window

Some of what a profile can do has nothing to stand in for the GUI's widgets yet, and does nothing,
or answers as the GUI would with no window open. `CI/headless-lua-known-failures.txt` lists the
Lua specs that fail with no window, so it is the up-to-date list; the main gaps are:

- the 2D and 3D mappers' views, and map symbols drawn with a font's Private Use Area;
- the space an internal MXP frame takes from the main console;
- layout that follows the main window, such as `Adjustable.Container` borders, docked user
  windows, and the command line a mini console or user window can show;
- keyboard shortcuts for addon commands, which the main window's menus hold.

`.github/workflows/headless-lua-tests.yml` runs the Lua specs in `mudlet-headless`, and
"The Lua specs with no main window" in `platform-builds.md` explains how to reproduce it.
