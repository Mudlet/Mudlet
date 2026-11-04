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
`outputLine`, its handler must not call the session back.

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
- MXP frames;
- layout that follows the main window, such as `Adjustable.Container` borders, docked user
  windows, and the command line a mini console or user window can show;
- keyboard shortcuts for addon commands, which the main window's menus hold.

`.github/workflows/headless-lua-tests.yml` runs the Lua specs in `mudlet-headless`, and
"The Lua specs with no main window" in `platform-builds.md` explains how to reproduce it.
