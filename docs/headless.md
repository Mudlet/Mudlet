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
`sysWindowResizeEvent`. A script's `setMainWindowSize()` answers `true` but cannot resize the
terminal, so the runner then gives the terminal's size again, with another `sysWindowResizeEvent`,
and the game is told no other. A script that asks again from that event, to keep to its size, is
answered `true` and nothing changes, as in the GUI when the window manager holds the window, until
the terminal is resized or a script asks for another size. Output to a file or a pipe has no size
to give, so it starts at the default of 90 by 25 and a script's `setMainWindowSize()` sets the size,
which the game is told, as in the GUI. Every run on Windows is like that.

### Stopping it and what is saved

The profile closes, and is saved, when:

- a script calls `closeMudlet()` or `closeProfile()`;
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

`closeProfile()` saves a profile whose "Auto save on exit" setting is off too: the GUI asks the player
whether to save it, nobody can answer that here, and the answer taken is the GUI's Yes. A save, map or
`profile.ini` that cannot be written is named on standard error, and the profile is reported as one
that could not be saved.

A `closeProfile()` from a `sysLoadEvent` handler, which runs once the profile's map has loaded but
before its load has ended, closes it once the load has ended, as `closeMudlet()` does, and saves it
then, with the map it loaded and what the handler changed.

What is not saved, and is not reported as a failed save, is a profile whose save did not load, or one
closed before its map has loaded, as by a `closeProfile()` at the top level of its own script, which
closes it at once, as in the GUI: its scripts run before its map is loaded, so its last good save and
map are kept rather than written over. For an app that embeds Mudlet, nor is a profile let go by
`shutdown(false)` or by deleting the session.

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
session->setMainWindowSize(QSize(800, 600)); // or in pixels, for an app that draws the main console
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
for its own theme's. A run also names the link it is part of, as "links in the main console" below tells.
What `echo()` and the like print comes as plain text for now.

`outputLine` carries each line as the game sent it, before the profile's triggers run on it, so a line a
trigger gags still arrives, and one it rewrites arrives unchanged. `lineShown` carries the same lines as
the main console shows them once their triggers have run, with the same arguments: with the text a
trigger replaced, the colours it changed and what it `echo()`ed into the line, which may hold line feeds,
and not at all for a line a trigger deleted. Lines a trigger feeds with `feedTriggers()` come after the
line it ran on, in the order the console shows them, and so does what it writes anywhere else, each piece
as a line of its own: an `echo()` after `deleteLine()`, `clearWindow()` or a `feedTriggers()`, and the
command echo of a `send()`. An app that draws the console as the GUI does draws `lineShown`, wrapping each
line itself, as for `outputLine`; a line a trigger split with `wrapLine()` comes whole, with the line feeds
it held, and if a trigger then deletes its first part, what is left of it comes as the line. A change a later
trigger makes to a line already sent, as by moving the cursor up to it, is not sent again, though what it
inserts is sent as a line of its own.

The last argument of both, `arrivalMSecs`, is when the line arrived, in milliseconds since the epoch: the
time the GUI's timestamps show beside it. A line a trigger wrote has the time of the console line it went
into, its own when it is a line of its own. It is -1 for a line with none. A handler that takes only the
first three or four arguments may still be connected. `localText` carries no time of its own: it is sent as
it is written, so the time its handler runs is when that text was written.

`timeStampsShown()` says whether the main console shows those times beside its lines, as the GUI's
timestamp button does, and `setTimeStampsShown()` presses that button: the choice is saved for the
profile's next load as the button saves it, and refused where `submit()` is. Where it was made but could
not be saved, it gives false and `errorString()` says so. `timeStampsShownChanged`
follows a change made either way or by a script's `enableTimeStamps()` and `disableTimeStamps()`, from the
event loop and only when the state differs from the one last sent or the one at `profileLoaded`.

`localText` carries the text written to the main console that the game did not send, as it is written,
in pieces that need not end a line: the `echo()`s of aliases, timers, key bindings and `runLua()`, the
commands the player sends, and Mudlet's own messages. While `lineShown` is connected, `localText` leaves
out what `lineShown` sends: all that is written while triggers run on a line, a Lua error the console
echoes and Mudlet's own messages among it. So an app draws both streams in the order they come, and each
piece once: a line of `lineShown` goes below a line `localText` left open, and the next piece of
`localText` then starts with the line feed that ends that open line, which the app drops. While
`lineShown` is not connected, `localText` carries what triggers write too. Connected or let go in a
handler, `lineShown` takes effect from the next line the game sends, so the line under way and all its
triggers write go to one stream or the other, never both.

Drawn that way, the console matches the GUI's but for its layout in these places, where no text is
missing or doubled:

- The console draws an `echo()` that follows `deleteLine()` on the end of the line above, which was already
  sent, where `lineShown` sends it as a new line.
- The console draws a command typed at a prompt on the end of that prompt, as `HP 10> look`, where
  `lineShown` sends the prompt and `localText` then sends `look` as a piece that the rule above puts on a
  line of its own. Nothing tells it apart from an alias's `echo()` after a prompt, which the GUI draws below.
- Blank lines inside what a trigger prints, such as a Lua error of several lines, or the empty command echo
  of a trigger's `send("")`, are not sent.

`mainConsoleCleared` says a script emptied the main console with `clearWindow()` or
`clearWindow("main")`, so an app empties its own copy of it; a mini console's clear does not send it.
While it is connected, a clear also ends any line `localText` left open, so the next piece has no line
feed to drop; without it, as for the CLI, that piece starts with the line feed that ends the open line.
It is sent between the lines from before and after, so, like `outputLine`, its handler must not call the
session back.

`loggingChanged` says when the profile starts or stops logging the game's text to a file, where the GUI's
logging button changes: from a script's `startLogging()`, or as the profile loads with logging left on,
with the log file's path while it logs. A start that fails sends `false`, and the reason is printed on the
main console, so it arrives as `localText`. A profile that closes while it logs, or that `shutdown()` or a
later `start()` lets go of, sends `false` too, so each `true` is followed by one `false`, unless the
session is deleted first: a session being deleted sends none. It comes from the event loop.

`consoleFont()` is the font the GUI would draw the main console in, for an app that draws in the profile's
font: read it at `profileLoaded`, then follow `consoleFontChanged`, which carries each change, whatever
made it - a script's `setFont()` or `setFontSize()`, the profile's save, the bundled default standing in for
a font that is not installed, or a package bringing that font or taking it away. The size `setScreenSize()`
gives is in that font's characters, counted again in each new one. It comes from the event loop, with the font as it is then, and only
when that differs from the one the app last had, so none follows `profileLoaded` for what the load did.

`setDebugConsoleShown(true)` shows the Central Debug Console, as the editor's button does, before
`start()` too, to see a profile load: `debugText` then carries what it prints, with its colours and when
it was written, what was kept from before it was shown first, and the profiles write what they write only
in debug mode, such as each trigger that matches. Append the text as given, as the console does: a line
ends at its own newline, and one line can come in several pieces of different colours, as `ERROR:` and
then why. `setDebugCategories()` picks which kinds of message it prints, for the whole process; until then
it hides `GameLine` (each line from the game), `TriggerDetail` (capture groups and multiline match state),
`LuaSuccess` and `Selection`, as the GUI's console does before a player changes it, and the GUI's saved
choice is not read. There is one console for the whole process, as in the GUI, owned by the session that
showed it last: showing it takes it from any other, whose `debugConsoleShown()` then answers false and
whose `setDebugConsoleShown(false)` is refused. It closes with the owner's `setDebugConsoleShown(false)`,
`shutdown()` or deletion. Its text is sent as written, from inside whatever wrote it, so the handler must
not call the session back.

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

`attentionRequested` with 0 also carries the GUI's flash for new game text: while `setWindowMinimized()`
has the app's window minimized or hidden, as the GUI's main window is when minimized, or a phone app sent
to the background, game text that adds to or changes the main console asks for the player's attention
until they come back, if the profile's setting to be notified of new data while minimized is on, as it is
unless the player turned it off. As in the GUI, an app that is only behind another window, which
`setApplicationActive(false)` reports, is not alerted, nor is one in front. The GUI's flash lasts until the
player comes back, however much more text arrives, so the signal is sent once each time the window is
minimized, at the first such text, from the event loop: the other prompts of a read that ends several with a
go-ahead, and later reads, bring none until `setWindowMinimized(false)` and back. A script's `echo()` and the
bare carriage return that finishes a line the game left open bring no alert, as in the GUI. As with
`alert()`, only the session's own profile is relayed, not one its scripts opened with `loadProfile()`.

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
- `commandRan`: the key is the shortcut of a command a script added to the menu, which ran as
  `activateAddonCommand()` runs it, ahead of the line and its bindings; see "Embedding it: commands and
  buttons that scripts add". It was added after the others, so an app's `switch` over every
  `KeyResult` needs a case for it.

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
if the profile clears the line after a send, an empty line follows the send. If the game asked to hide
input while the send's aliases ran, that empty line comes just after `hiddenInputWanted` turns off
instead, as the GUI's line was emptied behind its hidden-input box. It is sent as it is set, so
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
ahead moves into the masked line, when Mudlet says so (see `setCommandLineText()` below). An empty
`commandLineTextRequested` may follow `hiddenInputWanted` turning off, for a send whose aliases ran as
the game hid input, and empties the line the app has just put back.

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
while hidden input was wanted and puts back. For that last, it hands back what its line holds once the
signals that end hidden input have been handled, not the text it set aside, as an empty
`commandLineTextRequested` may have emptied the line meanwhile. The GUI's line does not count such text
as typed either, though it does count what it puts back from its hidden-input box, which the app hands
back as typed.

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

## Embedding it: drawing the main console as the GUI does

`lineShown` sends each line whole, so an app that draws the main console as the GUI does wraps it
itself, and needs what the GUI draws it with besides the font. Each of these is read at
`profileLoaded` and then followed by a signal that comes from the event loop, with the value as it is
then, and only when that differs from the one the app last had. So none follows `profileLoaded` for
what the load did, and a change a script makes and undoes before the event loop runs sends nothing.

`consoleWrap()` is where the main console wraps the game's lines: at `wrapAt` characters, with `indent`
spaces before the first part of a line and `hangingIndent` before each part after it, as a script's
`setWindowWrap()`, `setWindowWrapIndent()` and `setWindowWrapHangingIndent()` on the main console, or
the profile's save, set them. As in the GUI, an indent of the wrap width or more is none, and one of
more than half of it is half. `consoleWrapChanged` carries each change; a mini console's wrap does not
send it.

`consoleForeground()` and `consoleBackground()` are the profile's own colours, which the main console
is drawn in where the game asks for none, and which `TextRun` marks as the profile's own.
`consoleColorsChanged` carries both when either changes: from a script's `setBackgroundColor()` on the
main console, or a package or save loaded into the profile with colours of its own. A mini console's
background does not send it, nor does the game redefining its palette, which leaves both as they were.

`mainConsoleGrid()` and `mainConsoleTextRect()` are the main console's grid and where its text goes,
which `mainConsoleAreaChanged` follows; "Embedding it: the main window's size" below says what decides
them.

`errorConsoleText` carries what the editor's error console prints, in pieces with the colours it prints
them in: a Lua error comes as `[ERROR:]` in blue, where it was in green and the message in red, all on
black. It is sent as written, as `localText` is, so its handler must not call the session back. A
handler that takes only the profile and the text may still be connected.

`latencyChanged` carries what the GUI's latency box shows: the network's lag, from a command sent to the
game's reply, while the game marks its prompts with GA or EOR, and none otherwise, where the box shows
`<no GA>`; and how long the game's last text took to process, its triggers included, in seconds. As the
box, it is paced to once every 16ms, and sent only when its text, to the millisecond, would change. It
comes from the event loop, and none comes for a profile that `shutdown()` has let go.

`connecting` says the profile has begun connecting to the game, as it starts or reconnects, where the
GUI marks its tab as connecting, so an app can show that through a slow address lookup or connect.
`connected` or `disconnected` follows, or the profile closes; `disconnected` comes too when a script's
`disconnect()` calls the connect off during the address lookup, or `start()` does for a profile whose
save did not load. It comes from the event loop.

## Embedding it: the main window's size

What an app gives the session is the main window, as a resize of the GUI's main window gives it: the
area a profile's scripts read with `getMainWindowSize()`. The rest is laid out inside it as the GUI lays
it out:

- The borders a script's `setBorderTop()`, `setBorderLeft()` and the like set, the edges an
  `Adjustable.Container` keeps for itself and an MXP internal frame each take their room from the edges
  of the area. Borders the profile's save holds are laid out once it has loaded, before any script runs.
- What they leave is where the main console's text goes, `mainConsoleTextRect()`, in pixels from the
  area's top left corner.
- The main console's grid, `mainConsoleGrid()`, is as many characters of its font as fit there, never
  fewer than 40 columns. The game is told it, if the profile has NAWS on and the game asks, and a
  script's `getColumnCount()` and `getRowCount()` read it. With no border set, the grid fills the area.
- Borders wider than the area leave the text no room, as in the GUI: `mainConsoleTextRect()` and
  `sysWindowResizeEvent` give a width or height of 0, never a negative one, and the grid stays at its
  40 columns, or 1 row.

There are two ways to give the area, and the later of the two holds:

- `setScreenSize(columns, rows)` gives it in characters of the main console's font, as a terminal does,
  and is what the runner gives from its terminal. A font change counts it again in the new font's
  characters, so the grid stays the terminal's.
- `setMainWindowSize(QSize(width, height))` gives it in pixels, for an app that draws the main console
  itself. A font change keeps the area and fits the new font's characters into it, so the grid changes.

Either raises `sysWindowResizeEvent` with the text's width and height in pixels, as a resize of the main
window does in the GUI, and nothing when the size is the one already given; a border change raises it
too. Both are refused where `submit()` is, and for a size of no character, or more than 65535, either
way.

`mainConsoleAreaChanged` carries the grid and where the text goes when either changes: from a border, a
font change, either call, or a script's own `setMainWindowSize()`. Read both at `profileLoaded`; the
signal comes from the event loop, with both as they are then, and only when they differ from what the
app last had, so a script that sets a border twice in one turn sends one, and one that sets it and takes
it back sends none.

A script's `setMainWindowSize(width, height)` sets its own profile's area in pixels, as the app's call
would, and answers `true`, or `nil` and a message for a size the app's call refuses.
`mainWindowResizeRequested` tells the app the size it asked for, from the event loop: an app that can
resize its window does so, and one that cannot gives its own size again, as the runner does for its
terminal. The app answers each request by giving the size its window has once it has resized, or would
not, even when that is the size asked for, from the signal's handler or later, once its window system has
answered. The session takes the next size the app gives after a request as its answer: a script that asks
again for the same size, as a `sysWindowResizeEvent` handler that keeps to a size does, is answered `true`
and nothing changes, as in the GUI when the window manager holds the window, until the app gives a size
with nothing asked for, or a script asks for another. A size given from the handler of a request that a
newer one has replaced is not laid out: the newer request's answer decides. An app that connects nothing
to `mainWindowResizeRequested` is not waited for, so a script's size holds until the app gives another,
as in the GUI. An app that is connected but does not answer a request has the next size it gives, such
as the player's resize, taken as the answer: until the app gives another size after that, a script
asking again for the size of that request is answered `true`, the app is not asked, and the area keeps
the size the app gave. Every other profile in the run keeps its own area.

The app's own chrome stays outside the area. Where the GUI's main window has a command line, button bars
from `tempButtonToolbar()`, a toolbar and docked user windows around the main console, an app that shows
its own gives the area without them, and gives a smaller one as they grow; the session does not take
their room from it, so what a script reads of the main window's size changes with them only when the app
gives a new one.

## Embedding it: links in the main console

Each `TextRun` of `outputLine` and `lineShown` has a `link`: the id of the main console's link the run is
part of, or 0 for none. A line is split into runs where its link changes as well as where its look does,
so two links that touch and look alike are two runs. Links come from the game (OSC 8 and MXP `<SEND>`)
and from scripts (`echoLink()`, `echoPopup()` and the like, a trigger's included). An id names its link
while the main console still holds a line with it: the console reuses the ids of links whose lines it
has let go, so an app that keeps older lines than the console does should not rely on theirs.

`link(id)` says what a link offers, as the GUI shows it: its `tooltip`, as plain text; its `items`,
one for each thing it can run, each with the label the GUI gives it, and marked as greyed out where it
has nothing to run or as a separator where it has no label either; and `menu`, where the GUI's right
click lists the items, under `menuTitle` if that is set. A click follows the first item. It also gives
the link's state (a `spoiler` and whether it is `revealed`, `disabled`, `concealed` by the game's OSC 8
visibility settings, which leave blanks in its place in the GUI, `visited`, and `selected` in its group),
and how the GUI paints the link in that state, which may differ from its runs as they were sent: a
`foreground`, `background` and `styles` that are empty where the run's own apply, the styles standing in
for the run's bold, italic, underline, overline and strike-out only. It is `nullopt` for 0, for an id the
main console no longer holds, as once an MXP `<EXPIRE>` has removed the link or its line has been
deleted, and with no profile open. It is a getter, so it may be called from any handler, `outputLine`'s included.

`followLink(id)` does what a left click on the link does, which runs its first item whatever that holds,
and `followLink(id, item)` what picking that item from its `menu` does; `item` indexes `items`. A spoiler
not yet revealed is only revealed (`revealedSpoiler`), and the next follow runs it; the revealed text is
not sent, so an app's copy of the line keeps the blanks it was sent with. A `disabled` link, or a menu's
greyed-out item, runs nothing (`disabled`), nor does a `concealed` one (`concealed`), which the GUI leaves
no way to click. Otherwise the link is chosen in its selection group, deselecting the rest of an exclusive
group, and its command or function runs before `followLink()` returns (`ran`); a `send:` link sends, and a
web link opens through `QDesktopServices::openUrl()`, which an app can route with
`QDesktopServices::setUrlHandler()`. Where that raises a Lua error the result is `failed`, and
`errorString()` gives Lua's message. The link is `visited` after either. `noSuchItem` is for an item the
link does not have, `unknown` for an id the main console does not hold, as once a game's OSC 8 visibility
settings have deleted its line, and `refused` for where `submit()` is, a handler included.

`linksChanged(profile, links)` names the links whose `link()` may have changed, once per pass of the
event loop, each id once: one followed (by `followLink()` or a script) and now `visited`, chosen or
deselected in its group, a spoiler revealed, one a game's OSC 8 visibility settings concealed or revealed,
one a game's MXP `<EXPIRE>` removed, and every link on a line that went. A line goes by a script's
`deleteLine()` or `clearWindow()`, or by an OSC 8 concealment that deletes the whole line, which names the
line's other links as well as the one followed. `link()` gives `nullopt` once no line holds the link, so
one that runs on over a line still held is named and still given. The app reads `link()` again for
each; an id may be one it was never sent, as where a trigger deletes the line the link arrived on. Hover
is not sent, the pointer being the app's own.

The trimming of the main console's oldest lines, as its buffer fills, is not told: no signal names the
lines it lets go, and the app has each of them, with its links, from `outputLine` and `lineShown`, to
keep for as long as it likes. Nor does it name their links. The console lets go of the links that no
line holds any more each time a quarter of its buffer has been trimmed, and `linksChanged` names those
then, after which `link()` gives `nullopt` for each at once. Until then, `link()` of a link that only
trimmed lines held gives `nullopt` too, but reads the whole buffer to find that out.

## Embedding it: the 2D map

`renderMap()` draws the profile's 2D map onto a `QPainter` at the size given, as the GUI's mapper
shows it: the area of the player's room, centred on that room (or, with `mapCenterSmallAreas` on in
`Mudlet.ini`, on an area that fits), at that area's zoom, with the player's marker, the profile's
mapper settings and the map info box. A script opens the map first, as with `openMapWidget()`;
until then it is false with the reason in `errorString()`, as it is for a painter that is not active
and for a size under 10 by 10 pixels, which the GUI draws nothing in.
As the GUI's does, a draw that moves to another area raises `sysMapAreaChanged`. The map info box runs
the profile's Lua, so the call is refused where `submit()` is, from a handler of `outputLine` and the
other signals sent in the middle of a write.

`mapRepaintRequested` says when the GUI would draw the map again: a script opened the map, showed it
again or replaced it with an embedded mapper, the player moved or a script centred the map, the map
loaded or changed, or its zoom, colours, settings or info box did. It comes once a turn
of the event loop however many changes there were, while a script has opened a map, even one it has
since closed, and from the event loop, so a handler may call `renderMap()` at once. Only the 2D map is drawn; there is no
3D view.

`mapPointer()` is the GUI's mouse on the map, at a position in what `renderMap()` last drew, so the map
is drawn before it is clicked. A move or `centerview()` the app has not drawn yet does not change what
the pointer lands on, as the GUI's mapper moves its view only as it draws. A left press selects the
room under it, Ctrl and Shift adding to the selection as in the GUI, and a press away from the rooms
clears it; `getMapSelection()` reads it. Any press raises `sysMapWindowMousePressEvent`. A left
`doubleClick`, sent after the press that began it as Qt sends a `MouseButtonDblClick`, speedwalks to
the room under it as the GUI's does, with the profile's `doSpeedWalk()`. A `wheel` turn zooms the shown
area about the pointer, with Ctrl five times as fast and `invertMapZoom` in `Mudlet.ini` read as the
GUI's preference sets it. What the GUI does with a right press, a drag or a release, its menus, moving
rooms and labels and sizing a selection, stays the GUI's. It is false, with the reason in
`errorString()`, before the map is drawn, for a double-click of another button, for less than half a
notch of the wheel and for a wheel turn on a map whose last draw showed no area, as one with no rooms
does, and it is refused where `renderMap()` is.

## What still needs a window

Some of what a profile can do has nothing to stand in for the GUI's widgets yet, and does nothing,
or answers as the GUI would with no window open. `CI/headless-lua-known-failures.txt` lists the
Lua specs that fail with no window, so it is the up-to-date list; the main gaps are:

- the 3D mapper's view, the 2D map but for an app that calls `renderMap()`, and map symbols
  drawn with a font's Private Use Area;
- the main window's own command line, button bars and docks, which an app draws outside the area
  it gives (see "Embedding it: the main window's size"), and the command line a mini console or user
  window can show.

`.github/workflows/headless-lua-tests.yml` runs the Lua specs in `mudlet-headless`, and
"The Lua specs with no main window" in `platform-builds.md` explains how to reproduce it.

## Embedding it: the GUI's buttons and the app's state

Some of what the player does in the GUI is a button on its main window or console toolbar, which Lua
cannot do in the same way. `THeadlessSession` gives an app these too.

`setScriptsStopped(true)` is the main console's emergency stop: it turns off the profile's triggers,
aliases, timers and key bindings, and no event reaches its scripts until `setScriptsStopped(false)`,
which is how an app stops scripts that keep setting each other off, such as a trigger that answers its
own output. As the GUI's button does, `false` turns on again what the stop turned off; anything a script
had turned off before the stop, with `disableTrigger()` and the like, stays off. The stop does not hold
back what comes on while it lasts: a script's new `tempTrigger()`, what `resetProfile()` turns back on
and a package installed with `installPackage()` all run, as in the GUI. Call `setScriptsStopped(true)`
again to turn those off too, as each call turns off everything that is on. `false` when nothing is
stopped changes nothing, and `scriptsStopped()` reads the state. Like `submit()`, it is refused with no
profile open, while a profile loads or closes, and from a handler of `outputLine` and the other
signals sent mid-write.

`setApplicationActive()` tells the profiles the app has come to the front or left it, as the GUI hears
it from the system: a change raises `sysApplicationFocusChangeEvent` with `true` or `false` in every
profile that loaded and has not begun to close, the session's own and those its scripts opened, so a
package can quiet its sounds or notifications while the player is away. The state it is in already
raises nothing, as the GUI tells scripts only of a change. It starts as active, and before `start()` it
is only kept, for the next change to be told against; a profile opened later hears nothing until the
next change, as in the GUI. `applicationActive()` reads it. It is refused while a profile loads, during
`shutdown()`, and from a handler of `outputLine` and the other signals sent mid-write.

`setWindowMinimized()` tells the session the app's window is minimized or hidden, or a phone app has
gone to the background, as the GUI's main window is minimized: only then does new game text bring
`attentionRequested`, as said above, so an app that is merely behind another window reports that with
`setApplicationActive()` alone. It raises no event, as minimizing the GUI raises none, starts as `false`,
may be set at any time, before `start()` too, and `windowMinimized()` reads it.

`setReplayRecording(true)` is the main console's replay recording button: what the game sends from then
on is recorded into a new file in the profile's `log` folder, named for the second it started, which a
script's `loadReplay()` plays back. `setReplayRecording(false)` stops and saves it, as the connection
ending does on its own. Each says how it went in `localText`, with the file's path or the reason, as the
GUI says it in the main console. It is false, with `errorString()` saying why, when it is recording
already or is not, when the file cannot be opened, and when it cannot be saved, the recording being over
then. A recording started in the same second as an earlier one gets a `-2`, `-3` and so on after the
time, rather than replace it. One still running at `shutdown()` or as its profile closes is saved with
nothing said, so an app that wants to know how that went stops it with `setReplayRecording(false)`
first. `replayRecording()` reads whether one is under way. It is refused where `submit()` is.

`dropOnMainConsole()` is a drop on the main console, as the GUI takes one dragged there from a file
manager or a browser: each URL in the `QMimeData` that names a local file that exists raises
`sysDropEvent` with the file's path, its suffix, the position and `"main"`, and then text that reads as a
valid URL raises `sysDropUrlEvent` with the URL, its scheme, the position and `"main"`, in the GUI's
order. Where the data has no text of its own, `QMimeData` gives its URLs as the text, so a single file
dropped alone raises both events, the second with the `file` scheme, as in the GUI. Mudlet's own
handlers install a package dropped as a file, and one from an `http` or `https` link, as they do in the
GUI. The GUI installs a dropped file as a module instead while Ctrl is held, which its handler reads
from the keyboard itself, not from the drop; on the offscreen platform no key is ever held, so there a
dropped file is always installed as a package, never as a module. The position is the point on the
app's view of the main console the drop landed at, in its own pixels, which scripts get as `x` and `y`.
Data with neither URLs nor text is false and raises nothing, as the GUI's console does not take it. It
is refused where `submit()` is, and while the scripts are stopped its events reach none of them.

`setSoundsMuted()` is the main window's mute buttons, for `SoundSource::scripts`, the sounds scripts
play, or `SoundSource::game`, those the game sends over GMCP or MSP, and `toggleAllSoundsMuted()` is
the button for both, which unmutes both when both are muted and otherwise mutes both. As in the GUI,
muting is for every profile: it reaches the session's own and those its scripts opened, the only ones in
the process, as a second session's `start()` is refused while a pool exists. A change raises
`sysSettingChanged` with `muteMediaAPI` or `muteMediaGame` in each profile, as a script's `setConfig()`
does. `soundsMuted()` reads it, and is false with no profile open. `soundsMutedChanged` follows every
change from the event loop, whoever made it, a script's `setConfig()` in any of the profiles included,
and also a call that left it as it was, so an app's mute buttons can follow it as the GUI's do. It stops
once the session's own profile has closed, as the session's other signals do, even while profiles its
scripts opened live on. They are refused where `submit()` is.

## Embedding it: commands and buttons that scripts add

`addonCommands()` lists the commands the session's profile's scripts added with `addCommand()`, which
the GUI shows in the main window's menu, on its toolbar or on both, in the order they were added, for
an app to draw them where its own menus and toolbars are. Each carries the `id` that `addCommand()`
returned and `sysCommandClicked` names, the package whose code added it (empty when no package's did),
its name, icon path and tooltip, the `menuPath` of submenus it sits in and its `shortcut` as the script
wrote them, `onMenu` and `onToolbar` for where it shows, and what the script set since: whether it is
enabled, checkable and checked, pinned to the menu whichever profile is in front, and flashing on the
toolbar between `pulseColor1` and `pulseColor2`, each held for `pulseIntervalMs`. Commands the session's
scripts added in a profile they opened with `loadProfile()` are that profile's, not the session's, so
they are left out. It reads the commands as they are at the call, from anywhere, and is empty with no
profile open, so after `shutdown()` too, as removing a package, resetting the profile and closing it
removes its commands.

`addonCommandsChanged` says one or more of them changed, for the app to read them again: a command
added or removed, or a change a script made to one that `addonCommands()` gives, and the commands a
removed package added, or all of the profile's when it is reset. A call that leaves a command as it was,
such as setting the tooltip it has, sends nothing, and nor does a change to another profile's commands.
It is sent from the event loop, once for all the changes made before it runs, so a script that adds a
dozen commands sends it once.

`activateAddonCommand()` is a click on one of them, in the menu or on the toolbar: a checkable command
is ticked or unticked, which `addonCommandsChanged` follows, and then `sysCommandClicked` is raised with
its id, as the GUI does. The emergency stop holds the event, as it holds every event, but not the tick,
which the GUI's menu makes before the event is raised. It is false, with `errorString()` saying why,
for an id that is not a command of the session's profile, and for a disabled command, which the GUI does
not let be clicked. It is refused where `submit()` is.

`pressKey()` runs a command whose shortcut is the key ahead of the line and its bindings, giving
`commandRan`, as Qt hands the main window's menu a key before the focused widget has it. The line may
claim the key first, as the GUI's does, and then the command does not run: the command line, a
`QPlainTextEdit`, claims the key that toggles caret mode and, alone or with Shift or on the keypad,
printable keys, Return, Enter, Delete, Backspace, Home, End, Tab and the arrows, and with other
modifiers copy, paste, cut, undo, redo, select all and the moves and selections by word, line and
document. While hidden input is wanted the hidden-input box, a `QLineEdit`, claims fewer: not Return,
Enter, Tab, Up or Down, so a command on one of those runs while the game hides what is typed. Only an
enabled command of the session's profile that is on the menu runs. A command that a profile opened with
`loadProfile()` pinned is left out, even though the GUI's menu keeps it whichever profile is in front,
because `addonCommands()` does not list it and `activateAddonCommand()` does not click it. A keypad key
matches a shortcut written without the keypad, as in Qt, and a shortcut of more than one key never runs
from `pressKey()`, which takes one key at a time.

`buttonBars()` lists the profile's button bars and toolbars, the buttons a script makes with
`tempButtonToolbar()` and `tempButton()` and those a package or the editor adds, as the GUI draws
them: in the order of the profile's actions, each bar with its place (`top`, `left`, `right` or
`floating`), orientation, grid and style sheet, and each active button with its icon, style sheet,
whether it is a push-down button and is down, and, for a folder, the menu it opens. A horizontal bar
fills each column top to bottom with `columns` buttons, then the next column to its right. A vertical
bar fills each row left to right with `columns` buttons, then the next row down. Either leaves its
first `fillerOffset` places empty. As in the GUI, an
inactive bar or button is left out, and so is a bar placed where the GUI shows none; a package's
folder lends its place to the bars in it, and a button bar's menu shows only active entries, each with
the icon of the button it hangs from, while a floating toolbar's menu has an entry for the folder that
holds every entry, active or not, with its own icon. The list is empty with no profile open.

`buttonsChanged` says the bars have changed, for the app to read `buttonBars()` again: a script's
`tempButtonToolbar()`, `tempButton()`, `setButtonStyleSheet()` or `showToolBar()`, a package installed or
removed, and `setButtonState()` when it moves a button up or down, but not when it leaves it as it
was. As with `addonCommandsChanged`, the changes made before the event loop runs send one, from the
event loop.

`pressButton()` is a click on a button, or on an entry of its menu, that `buttonBars()` lists: as in
the GUI it sends the button's command, through the profile's aliases, and runs its script. A push-down
button goes down, sending its down command, or up, sending its up command, and `getButtonState()` in
what that sets off reads the state it went to; `buttonsChanged` follows. That holds for an entry of a
menu too, where in the GUI an alias the entry's command sets off still reads what the last button to
run left, as its menu sets the state for the entry's script only, after the command has gone. A folder's button runs
its script as well. In the GUI a click on it opens its menu, which the app shows from the button's
`children`, and the folder's script runs once the menu has closed, after the entry picked in it has
run, so an app that presses the entry picked and then the folder does as the GUI does. It is false,
with `errorString()` saying why, for an id `buttonBars()` does not list, such as an inactive button or a
bar itself, and refused where `submit()` is.

## Embedding it: completing the command line

The GUI's command line completes the word being typed when the player presses Tab, from the main
console's recent text and from words scripts give it. `THeadlessSession` gives an app the same.

`commandLineSuggestions()` is the words scripts gave the main command line with
`addCmdLineSuggestion()`, and `commandLineBlacklist()` those they gave with `addCmdLineBlacklist()`,
which Tab is never to complete to. Each word is there once, however often it was added, and the lists
are sorted. As in the GUI, `removeCmdLineSuggestion()` and `removeCmdLineBlacklist()` take one word out,
and `clearCmdLineSuggestions()` and `clearCmdLineBlacklist()` empty them. Words a script gives a line
from `createCommandLine()` or a mini console's own command line are that line's, not the main line's.
Both lists are empty with no profile open, and may be read from anywhere, a handler included. No signal
says they changed: an app reads them when it needs them, as when the player presses Tab.

`completeCommandLine()` is the Tab key itself, and `completeCommandLine(false)` Shift+Tab. It completes
the word at the end of the line `setCommandLineText()` last set, from the same words the GUI would offer,
the most recently seen first: words of the last 500 lines of the main console, then the suggestions, none
that is blacklisted, matched without regard to case. It returns the whole line it leaves, which scripts'
`getCmdLine()` reads from then on, for the app to show with its cursor at the end. Each further call takes
the next word, or with `false` the one before, stopping at either end as the GUI does rather than wrapping.
The cycle ends when the line changes, when the player presses Escape (sent with `pressKey()`) and when
they submit it; the next call then reads the line afresh. Mudlet sees each new line rather than the key
that made it, so any change ends the cycle, as Backspace does in the GUI: what the player types, a line the
app sets itself, such as one it recalls from the history, the line `completeCommandLineFromHistory()`
leaves, and a script's `printCmdLine()`. Only the line it was given, handed straight back to
`setCommandLineText()`, keeps the cycle going. The GUI's line ends the cycle on a key alone, so there a
recall, the up arrow or a script's `printCmdLine()` between two presses of Tab has the second complete the
old word over the new line; the session completes the new line instead. On an empty
line nothing is completed, as in the GUI once it has sent a line. When no word completes the line it
returns nothing, and `errorString()` says so; it does the same while the game hides what is typed, as the
GUI completes no password, and wherever `submit()` is refused, as from a handler of the session's signals.

`completeCommandLineFromHistory()` is the up arrow on a line the player has typed into, as the GUI's command
line does with "Highlight history" on in the profile's settings: it completes the line to the newest line of
`commandHistory()` that starts with what was typed, and returns the whole line for the app to show with the
part it added selected. Each further call goes on to the next older line that starts with it. Once no older
one does, it returns just what was typed, and the call after that starts again from the newest, though, as in
the GUI, calls stay on the oldest line of all if that one matched. Any change to the line starts the walk
again from the newest line, as it ends Tab's cycle, and so does a submit; handing the returned line straight
back to `setCommandLineText()` does not. The GUI's up arrow recalls each line of the history whole instead,
which an app does by walking `commandHistory()` itself, with nothing typed, with "Highlight history" off, and
while the whole line is selected: after Escape, a script's `selectCmdLineText()` or `sendCmdLine()`, a submit
that leaves the line in place, and a line the app set itself (`CommandLineEdit::notTyped`), as a recall is,
until the line is typed into or completed. The call returns nothing for each of these, with `errorString()`
saying which, as it does when no line sent before starts with what was typed, while the game hides what is
typed, and wherever `submit()` is refused.
