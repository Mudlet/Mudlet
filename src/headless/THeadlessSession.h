#ifndef MUDLET_THEADLESSSESSION_H
#define MUDLET_THEADLESSSESSION_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TDebug.h"

#include <QColor>
#include <QFlags>
#include <QFont>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>

#include <memory>
#include <optional>
#include <utility>

class Host;
class HostManager;
class QPainter;
class QSize;
class QTimer;
class TBuffer;
class QKeyEvent;
class QMimeData;
class QPoint;

// One profile run with no window, for a frontend of its own (a terminal, a phone app) to feed input and
// show the main console. It touches no terminal or standard stream itself, and like Host it is used on the
// application's thread only.
class THeadlessSession : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        // Matched as the GUI's --profile is, ignoring case, once trimmed
        QString profile;
        bool connect = true;
        // These four are stored in the profile before the load, as the connection dialog stores them, and used
        // over a predefined game's details; a host makes the profile if there is none. One the dialog would
        // refuse is refused.
        QString host;
        // 0 keeps what is stored; below 0 or above 65535 is refused
        int port = 0;
        // Unset, it is the dialog's box: as a predefined game has it while no port is stored, otherwise as
        // stored, off for a new profile. Given a host or port, it is stored with them for a new profile, or for
        // a predefined game with no port stored.
        std::optional<bool> tls;
        // Trimmed, as the dialog trims it
        QString login;
        // For this run only; nothing stores it
        QString password;
        // An absolute path, made if need be: the folder that holds the profiles, Mudlet.ini and the passwords
        // stored for them, in place of the GUI's folder and the system's keychain. Settled once a process, so a
        // later start() may name it again, but no other, nor leave it out for the GUI's. Never the GUI's folder.
        QString configDir;
    };

    // How the game asked for a run of a line to be shown. Reverse is not applied to the colours: the GUI
    // swaps them as it paints.
    enum class TextStyle {
        // With a colour too, whose foreground may then be the bright form the GUI paints in place of bold
        Bold = 0x1,
        Italic = 0x2,
        // Any of the underline styles, OSC 8 links' wavy, dotted and dashed ones included
        Underline = 0x4,
        Overline = 0x8,
        StrikeOut = 0x10,
        Reverse = 0x20,
        Blink = 0x40,
        // Set in place of Blink when the game asks for both
        FastBlink = 0x80,
        Concealed = 0x100
    };
    Q_DECLARE_FLAGS(TextStyles, TextStyle)

    // The characters [start, start + length) of a line, counted as QString counts them, shown alike
    struct TextRun
    {
        int start = 0;
        int length = 0;
        QColor foreground;
        QColor background;
        // The profile's own colours, which the game's text has when it asks for none or resets them; a terminal
        // may show its own in their place. A colour the game asks for that is the same is not told apart.
        bool defaultForeground = false;
        bool defaultBackground = false;
        TextStyles styles;
        // The main console's link the run is part of, 0 for none; touching links that look alike are runs of their
        // own. An id names its link while the main console still holds a line with it, after which it may be reused.
        int link = 0;

        bool operator==(const TextRun&) const = default;
    };

    // invalidName, invalidConnectionDetails: a profile was to be made under a name, or given a host, port or
    // TLS, that the connection dialog would not take; nothing is stored. poolClosing: the profiles of a
    // session, this one or another, are still closing, from its shutdown() or, once it is deleted, from the
    // event loop. invalidConfigDir: the configDir is not absolute, cannot be made or is not a folder, or
    // is the GUI's, or another folder, the GUI's or one chosen, is in use; nothing is made.
    enum class StartResult { started, noSuchProfile, invalidName, invalidConnectionDetails, notOpened, loadFailed, poolClosing, invalidConfigDir };

    explicit THeadlessSession(QObject* parent = nullptr);
    // Closes the profiles unsaved, at once or, while one is busy as for shutdown(), a reset is pending, one is
    // still being let go or one of its signals is being sent, from the event loop once that is over; a
    // frontend that never runs the event loop again then leaks them, and they never get their sysExitEvent. It
    // may be deleted from the frontend's own code, a script, or a handler of the session's signals, even
    // during one of its calls; from a handler of a signal of host() or of what it holds, use deleteLater(), as
    // one sent with no script running would have its profile deleted under it.
    ~THeadlessSession() override;

    // After the application's organisation and name are set, as it reads Mudlet.ini. Refused while a pool
    // of profiles exists. A profile loaded for the first time gets its game's default packages, as in the GUI.
    // On loadFailed the profile stays open, its connect called off, for the frontend to close: it takes no
    // commands, Lua or packages, and shutdown() does not save it, so its last good save and map are kept.
    StartResult start(const Options&);
    // Why the last call that failed did
    QString errorString() const { return mErrorString; }
    // The font the main console is drawn in, a default QFont with no profile open: read it at profileLoaded, then
    // follow consoleFontChanged
    QFont consoleFont() const;
    // For what the session does not cover, such as settings and the map. Null once the profile closes, so
    // hold it in a QPointer. Not from a handler of outputLine, lineShown, mainConsoleCleared, localText, errorConsoleText, debugText,
    // hiddenInputWanted or a ...Requested signal but quitRequested and mainWindowResizeRequested: a line it fed would be
    // committed in the middle of a write, which TBuffer does not allow.
    Host* host() const;

    // As typed on the command line: its aliases run, and what is sent while not connected is echoed and dropped
    // with nothing said, as in the GUI, so a frontend that types ahead waits for connected(). These, and
    // installPackage(), are refused while a profile loads or shutdown() runs, and from a handler of outputLine,
    // lineShown, mainConsoleCleared, localText, errorConsoleText, debugText, hiddenInputWanted or a ...Requested
    // signal but quitRequested and mainWindowResizeRequested, which run mid-write, and fail once a close of the
    // profile has begun; submit() checks each line of a paste as a call. It is the app's input line being sent: the
    // main command line holds the line while its aliases run, then is emptied or kept as the profile says. A command
    // from elsewhere, such as a button, goes through runLua() and expandAlias(), which leave the line alone.
    bool submit(const QString& line);
    // As the GUI's hidden-input box sends what is typed into it while hiddenInputWanted holds: straight to the
    // game, with its carriage returns and line feeds taken out, and no aliases, command separator,
    // sysDataSendRequest, echo or history. submit() still runs aliases on such a line, as it always has. False,
    // with errorString() saying why, while the box would not be open, when the game's encoding cannot carry
    // the line, when it could not be sent as the profile is not connected, and where submit() is refused; the
    // app should then drop the line, as the GUI drops what was typed into a box that closes, and never resend
    // it with submit(), which echoes it, keeps it in the history and hands it to aliases. Its own copy of the
    // line is cleared once sent, as far as Qt allows.
    bool submitHiddenInput(const QString& line);
    // The player's first edit of the app's masked line, as the box's: a saved password still on its way from
    // the keychain is then not sent for them. False while the box would not be open, and where submit() is
    // refused.
    bool hiddenInputEdited();
    // The box's Esc on an empty box: hiddenInputWanted(false) follows, and submit() is the way to the game
    // again. As in the GUI, input is hidden again once the game answers the next line submit() sends, and
    // dismissing it then lasts until the game stops hiding input. False while the box would not be open, and
    // where submit() is refused.
    bool dismissHiddenInput();
    bool runLua(const QString& code);
    // As the GUI installs a package given on its command line, but quietly: a refusal is errorString()'s alone
    // to tell. It returns once the install's sysInstall handlers should have run: Qt fires zero timers in the
    // order set, though it does not promise to. installedWithErrors: it stays installed, as in the GUI, but a
    // part of it or its config.lua could not be read, or an item's Lua failed. A file that cannot be read fails.
    // One whose name, or whose config.lua's, is already installed is left as it is; errorString() says so.
    enum class PackageResult { installed, installedWithErrors, alreadyInstalled, failed };
    PackageResult installPackage(const QString& path);
    // The main window's size in characters, as a resize of the GUI's main window sets it. As there, the borders a
    // script or MXP sets take their room from it, and what they leave is the main console's grid. If the profile
    // has NAWS on and the game asks for it, the game is told the grid from the event loop once it has not changed
    // for 0.3s, and only if what it is told has changed, or when it next asks; until then it is told 90 by 25.
    // The profile's scripts read the size as the main window's, and a change raises sysWindowResizeEvent with
    // what the borders leave of it, in pixels. As in the GUI, a width below 40 is taken as 40 and the game is told
    // no more than the profile's wrap width. Refused below 1 or above 65535, which NAWS cannot carry, and where
    // submit() is.
    bool setScreenSize(int columns, int rows);
    // Shows or closes the Central Debug Console, as the editor's button does: while it is shown, debugText carries
    // what it prints, what was kept from before first, and the profiles write what they write only in debug mode.
    // There is one for the whole process, owned by the session that showed it last: showing it here takes it from
    // another, whose debugConsoleShown() then answers false and whose false is refused. It closes with the owner's
    // shutdown() or deletion too. Works before start(); refused while a profile loads, during shutdown() and from a
    // handler.
    bool setDebugConsoleShown(bool shown);
    bool debugConsoleShown() const;
    // Which kinds of message the console prints, for the whole process. Until set, TDebug's defaults, which hide
    // GameLine, TriggerDetail, LuaSuccess and Selection; the choice saved by the GUI's console is not read.
    void setDebugCategories(TDebug::Categories categories);
    TDebug::Categories debugCategories() const;
    // Where the main console wraps the game's lines: at wrapAt characters, with indent spaces before a line's
    // first part and hangingIndent before each part after it. As the GUI wraps them, an indent of the wrap width or
    // more is taken as none, and one of more than half of it as half. All 0 with no profile open: read it at
    // profileLoaded, then follow consoleWrapChanged.
    struct ConsoleWrap
    {
        int wrapAt = 0;
        int indent = 0;
        int hangingIndent = 0;

        bool operator==(const ConsoleWrap&) const = default;
    };
    ConsoleWrap consoleWrap() const;
    // The colours the main console is drawn in where the game asks for none, which runs mark as the profile's
    // own, invalid with no profile open: read them at profileLoaded, then follow consoleColorsChanged
    QColor consoleForeground() const;
    QColor consoleBackground() const;
    // An item of a link's menu, as the GUI lists it
    struct LinkItem
    {
        QString label;
        // Off where the GUI greys it out, as it has nothing to run
        bool enabled = true;
        // A line between items, with no label
        bool separator = false;

        bool operator==(const LinkItem&) const = default;
    };
    // What a link of the main console offers, as the GUI shows it now
    struct Link
    {
        // Plain text, empty for none
        QString tooltip;
        // A click follows the first item; where menu is set, the GUI's right click lists the items, under menuTitle
        // if that is not empty
        bool menu = false;
        QString menuTitle;
        QList<LinkItem> items;
        // A spoiler, which a click reveals before it follows the link, and whether it has been
        bool spoiler = false;
        bool revealed = false;
        // A click does nothing, save reveal a spoiler
        bool disabled = false;
        // Hidden by the game's OSC 8 visibility settings: the GUI shows blanks in its place, which a click cannot reach
        bool concealed = false;
        bool visited = false;
        // Chosen, in its selection group
        bool selected = false;
        // How the GUI paints the link in its state now, which may differ from its runs as sent: each is empty where
        // the run's own applies, and styles replaces only the run's bold, italic, underline, overline and strike-out
        std::optional<QColor> foreground;
        std::optional<QColor> background;
        std::optional<TextStyles> styles;
    };
    // The link a run names, while the main console holds it; nullopt for 0, an id it does not hold and with no profile
    // open. A getter, so it may be called from any handler.
    std::optional<Link> link(int id) const;
    // Follows a link as a left click on it does, or given an item, as picking that item from its menu does: a
    // spoiler not yet revealed is only revealed (revealedSpoiler); a disabled link and a menu's greyed-out item
    // run nothing (disabled), nor does a link the game's visibility settings have concealed (concealed); and
    // otherwise its selection group is updated and its command or function runs before this returns, so an
    // OSC 8 web link opens through QDesktopServices: ran, or failed where that raised a Lua error, which
    // errorString() gives. noSuchItem: item does not index link()'s items. unknown: an id the main console does
    // not hold. refused: where submit() is; errorString() says why.
    enum class LinkResult { ran, revealedSpoiler, disabled, concealed, failed, noSuchItem, unknown, refused };
    LinkResult followLink(int link, std::optional<int> item = std::nullopt);
    // Whether the main console shows each line's time beside it, as the GUI's timestamp button says; false with
    // no profile open. A getter, so it may be called from any handler.
    bool timeStampsShown() const;
    // As a press of the GUI's timestamp button: saved for the profile's next load as the button saves it, and
    // timeStampsShownChanged follows where it changed. Refused where submit() is, and false where the change was
    // made but could not be saved; errorString() says why.
    bool setTimeStampsShown(bool shown);
    // Answers the game's offer of a secure port, as the GUI's Yes and No do: true stores that port with TLS on,
    // as the connection dialog would, and connects there, so a frontend that passes its own port to a later
    // start() passes tls with it; false reconnects where it was and stops the profile asking again. False, with
    // errorString() saying why, if no offer is waiting, as once its connection has ended, if the game has since
    // taken back its secure port, and where submit() is refused. It may run the event loop, where shutdown() and
    // save() are refused and a session deleted waits for it to return before its profiles close.
    bool answerSecureConnectionOffer(bool accept);
    // As the Cancel of the GUI's progress dialog: packageDownloadFinished follows. False with no download
    // running, and where submit() is refused.
    bool cancelPackageDownload();
    // As the Abort of the GUI's map progress dialog: the export, import or download is asked to stop at its next
    // step, and mapProgressFinished follows. True says the request was made: a JSON import with no areas to read
    // has no step left and completes. False with none running that can still be aborted, after
    // mapProgressCancelDisabled, and where submit() is refused, save that a JSON map that start() loads can be
    // aborted from a handler of its mapProgressStarted.
    bool cancelMapOperation();
    // A key pressed as on the GUI's input line, which offers it to the profile's key bindings where that line
    // would: the first that matches runs, or each one when the profile runs them all. The key and modifiers are
    // those of Qt's key event, which a binding must match exactly, so a key on the keypad carries
    // Qt::KeypadModifier. The binding's script runs before this returns, as an alias does for submit(), and what
    // it sends goes then. bindingRan: a binding took it. noBinding: none did, and the app does what the GUI's
    // line does then, as type it. keyKept: the GUI's line keeps the key for itself, as Return, Up or a keypad
    // Enter no binding took, or while hidden input is wanted its box does, typing a printable key into the
    // password; the app handles it. refused: where submit() is. commandRan: the key is the shortcut of a command
    // addonCommands() lists, which the main window's menu runs ahead of the line's bindings, as
    // activateAddonCommand() does. errorString() says which.
    enum class KeyResult { bindingRan, noBinding, keyKept, refused, commandRan };
    // Without the key's text, a printable key with Ctrl+Alt (or Option on macOS) counts as AltGr typing while
    // hidden input is wanted, so no binding is offered it
    KeyResult pressKey(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    // With what the key typed, as QKeyEvent::text(), even if empty, by which the hidden-input box judges it as
    // the GUI's does
    KeyResult pressKey(Qt::Key key, Qt::KeyboardModifiers modifiers, const QString& text);
    // typed: the player typed it. notTyped: the app set it itself, as the GUI's line does for a history recall
    // or Tab completion, or put back text from a script that it set aside while hidden input was wanted.
    enum class CommandLineEdit { typed, notTyped };
    // What the player has typed so far on the app's input line, which a script's getCmdLine() then reads, as it
    // reads the GUI's command line. Set again as the line changes; after a submit() the line is as the profile
    // leaves it, and commandLineTextRequested says when Mudlet empties it. No commandLineTextRequested follows
    // this. While hidden input is wanted only an empty line is taken. A line the player typed ahead of the game's
    // request to hide input leaves Mudlet's copy before a trigger runs on any line that reaches Mudlet after that
    // request, and typedLineMoveRequested ahead of hiddenInputWanted tells the app to move it into its hidden
    // input; a line ended by a go-ahead ahead of the request, even in the same read, has its triggers run on the
    // line as it was, in the GUI too. Refused where submit() is.
    bool setCommandLineText(const QString& text, CommandLineEdit edit = CommandLineEdit::typed);
    // The GUI's replay toolbar, for a replay of the session's profile: pauseReplay() holds it where it is and
    // resumeReplay() lets it carry on, each false unless it is playing or held as it needs; stopReplay() ends it
    // as the toolbar's Stop does, saying so in localText. False with no replay of this profile playing, though one
    // a script began counts before replayStarted arrives, while a script of the profile runs, as from a nested
    // event loop in the middle of a replayed chunk, and where submit() is refused.
    bool pauseReplay();
    bool resumeReplay();
    bool stopReplay();
    // Whether the replay of the session's profile is held, as the toolbar reads it; one paused before its
    // replayStarted arrives is still held when it does
    bool replayPaused() const;
    // The toolbar's faster and slower buttons: doubles or halves the speed of the profile's replay, within 1 and
    // 1024 times. Even at either end it is true and replaySpeedChanged follows with the speed it stays at, as the
    // toolbar's buttons do. Refused as pauseReplay() is.
    bool changeReplaySpeed(bool faster);
    // How many times its recorded pace the profile's replay plays at; -1 with none of this profile playing
    int replaySpeed() const;
    // The recording's own timestamp of the line the profile's replay is waiting to play, as the toolbar's time
    // shows it: at the end of a gap as soon as the gap begins, so held there, ahead of what has been shown, while
    // paused. -1 with none of this profile playing. Read from anywhere, a handler of outputLine included.
    qint64 replayElapsedMSecs() const;
    // A copy of the main command line's history, newest first, as the GUI's up arrow walks it: each line submit()
    // sent, once, as its aliases left the line, and what the profile's earlier sessions kept, but nothing sent while
    // hidden input was wanted. Empty with no profile open too, which host() tells apart. Read from anywhere.
    QStringList commandHistory() const;
    // The main console's emergency stop button: true turns off the profile's triggers, aliases, timers and key
    // bindings and keeps events from its scripts, which stops scripts that keep setting each other off, such as a
    // trigger that answers its own output; false turns on again what the stop turned off, leaving off what a script
    // had turned off, as the GUI's button does. What a script makes, resetProfile() resets or a package installs
    // while stopped is on, so true again turns that off too; false while not stopped changes nothing. Refused where
    // submit() is.
    bool setScriptsStopped(bool stopped);
    // False with no profile open
    bool scriptsStopped() const;
    // The app coming to the front or leaving it, as the GUI hears it from the system: a change raises
    // sysApplicationFocusChangeEvent with true or false in every profile that loaded and has not begun to close, and
    // the state it is in already raises nothing. It starts true, and before start() it is only kept. Refused while a
    // profile loads, during shutdown(), and from a handler of outputLine, localText, errorConsoleText,
    // hiddenInputWanted or a ...Requested signal but quitRequested and mainWindowResizeRequested.
    bool setApplicationActive(bool active);
    bool applicationActive() const { return mApplicationActive; }
    // The main console's replay recording button: true records what the game sends into a new file in the
    // profile's log folder, false stops and saves it, each saying so in localText as the GUI says it in the main
    // console. False, with errorString() saying why, if it is recording already, or not, if the file could not be
    // opened, or if it could not be saved, the recording being over either way. The connection ending saves it too.
    // Refused where submit() is.
    bool setReplayRecording(bool record);
    // False with no profile open
    bool replayRecording() const;
    // A drop of files or a link on the main console, as the GUI takes one: each URL that names an existing local
    // file raises sysDropEvent with its path, its suffix, the position and "main", then text that reads as a valid URL
    // raises sysDropUrlEvent with it, its scheme, the position and "main"; QMimeData gives a single URL as the text
    // when there is none of its own, so a file dropped alone raises both, as in the GUI. The position is the point on the main
    // console it was dropped at, in the app's own pixels. False, raising nothing, for data with neither URLs nor text,
    // which the GUI's console does not take. Refused where submit() is.
    bool dropOnMainConsole(const QMimeData& data, const QPoint& position);
    // The main window's mute buttons: sounds that scripts play, and sounds the game sends over GMCP or MSP. Muting
    // is for every profile, as in the GUI: the session's own and those its scripts opened, and a change raises
    // sysSettingChanged with muteMediaAPI or muteMediaGame in each. toggleAllSoundsMuted() is the button for
    // both: it unmutes both when both are muted, and otherwise mutes both. Refused where submit() is.
    enum class SoundSource { scripts, game };
    bool setSoundsMuted(SoundSource source, bool muted);
    bool toggleAllSoundsMuted();
    // False with no profile open
    bool soundsMuted(SoundSource source) const;
    // A command a package's addCommand() added, as the main window shows it in its menu, on its toolbar or both
    struct AddonCommand
    {
        // What addCommand() returned, which sysCommandClicked carries
        int id = 0;
        // Empty for one no package's code added
        QString package;
        QString name;
        QString icon;
        QString tooltip;
        // The submenus it sits in, separated by '/', and its shortcut as the script wrote it, for QKeySequence to
        // read; both empty unless it is on the menu
        QString menuPath;
        QString shortcut;
        bool onMenu = false;
        bool onToolbar = false;
        bool enabled = true;
        // checked is ticked while checkable, which a script's setCommandChecked() makes it
        bool checkable = false;
        bool checked = false;
        // Pinned, the GUI keeps it on the menu whichever profile is in front; otherwise only while its own is
        bool pinned = false;
        // Its toolbar button flashing between the two colours, each held that long; the colours and time are
        // the last a script set, and only mean something while it flashes
        bool pulse = false;
        QColor pulseColor1;
        QColor pulseColor2;
        int pulseIntervalMs = 0;

        bool operator==(const AddonCommand&) const = default;
    };
    // The session's profile's commands in the order they were added; none with no profile open. Read from
    // anywhere.
    QList<AddonCommand> addonCommands() const;
    // A click on the command in the main window's menu or on its toolbar: a checkable command is ticked or unticked,
    // then sysCommandClicked is raised with its id, which the emergency stop holds as it holds every event. False,
    // with errorString() saying why, for an id that is not a command of the session's profile and for a disabled
    // command, which the GUI does not let be clicked.
    bool activateAddonCommand(int id);
    // A button on one of the profile's button bars or toolbars, or an entry of a button's menu, as the GUI draws it
    struct Button
    {
        // The button's action's id, as getButtonState() and the editor know it
        int id = 0;
        QString name;
        // The icon's path; an entry of a menu on a button bar shows the icon of the button whose menu it is in
        QString icon;
        // Its own, which the GUI gives it over its bar's; empty for a menu entry
        QString styleSheet;
        // A push-down button stays down, checked, from one press to the next
        bool pushDown = false;
        bool checked = false;
        // A folder, whose press opens a menu of the entries in children
        bool menu = false;
        QList<Button> children;
    };
    struct ButtonBar
    {
        enum class Place { top, left, right, floating };
        int id = 0;
        QString name;
        QString styleSheet;
        // The strip of the main console it is in, or a toolbar the player docks or floats
        Place place = Place::top;
        Qt::Orientation orientation = Qt::Horizontal;
        // A horizontal bar fills each column top to bottom with this many buttons, then the next to its right; a
        // vertical bar fills each row left to right with this many, then the next below it
        int columns = 1;
        // How many places of that grid, in the order it fills them, the GUI leaves empty before the first button
        int fillerOffset = 0;
        QList<Button> buttons;
    };
    // The profile's bars, in the order of its actions, each with its active buttons; none with no profile open.
    // As the GUI draws them once it last redrew its bars, which it does as a script changes them. Read from
    // anywhere.
    QList<ButtonBar> buttonBars() const;
    // A click on a button or menu entry buttonBars() lists, as the GUI's: it sends the button's command and runs its
    // script, and a push-down button goes down or up. False, with errorString() saying why, for an id it does not
    // list; refused where submit() is.
    bool pressButton(int id);
    // The main window's size in pixels, as a resize of the GUI's main window gives it: the borders a script or MXP
    // sets take their room from it, and the main console's font decides how many characters fit in what is left,
    // so a font change regrids it and the area stays. Raises sysWindowResizeEvent as setScreenSize() does, and the
    // later of the two holds. Refused where setScreenSize() is, and for a size that holds no character of the font
    // either way, or more than 65535.
    bool setMainWindowSize(const QSize& pixels);
    // The main console's grid in characters, and where its text goes in the main window's area in pixels: the area
    // less the borders. Empty with no profile open: read them at profileLoaded, then follow mainConsoleAreaChanged.
    QSize mainConsoleGrid() const;
    QRect mainConsoleTextRect() const;
    // The words scripts gave the main command line for Tab to complete, with addCmdLineSuggestion(), and those
    // it is never to complete, with addCmdLineBlacklist(), each once and sorted. Empty with no profile open.
    // Read from anywhere.
    QStringList commandLineSuggestions() const;
    QStringList commandLineBlacklist() const;
    // Tab, or Shift+Tab for !forward, on the app's input line, as the GUI's command line completes the word at the end
    // of what setCommandLineText() last set: the whole line it leaves, which is Mudlet's copy of it now, as if set with
    // CommandLineEdit::notTyped, for the app to show with its cursor at the end. Further calls go on through the
    // words, until the line changes, typed or not, by a script or by completeCommandLineFromHistory(), other than to
    // the line handed back, Escape is pressed or a line is submitted. Nullopt, with errorString() saying why, when no word completes it, as on an
    // empty line, while hidden input is wanted, and where submit() is refused.
    std::optional<QString> completeCommandLine(bool forward = true);
    // Up on the app's input line, as the GUI's command line with "Highlight history" on completes what was typed:
    // the newest line of commandHistory() that starts with what setCommandLineText() last set, which it leaves as
    // Mudlet's copy of the line, as if set with CommandLineEdit::notTyped, for the app to show with the part it added
    // selected. Further calls go on to older lines that start with it; once none is older they return what was typed
    // and start again from the newest, though as in the GUI they stay on the oldest line of all if that matched. Any
    // change to the line, other than to the line handed back, or a submit starts again from the newest. Nullopt, with
    // errorString() saying why, when no line starts with it, while hidden input is wanted, where submit() is refused,
    // and where the GUI's up arrow recalls whole lines, which commandHistory() holds: with nothing typed, "Highlight
    // history" off, or the whole line selected, as after Escape, a kept submit or a line set as notTyped, until it is
    // typed into or completed.
    std::optional<QString> completeCommandLineFromHistory();
    // The app's window minimized or hidden, as the GUI's main window is when minimized, or a phone app sent to the
    // background: only then does new game text ask for the player's attention, as attentionRequested says. It starts
    // false, raises no event, as minimizing the GUI raises none, and may be set from anywhere, before start() too.
    void setWindowMinimized(bool minimized);
    bool windowMinimized() const { return mWindowMinimized; }
    // Draws the profile's 2D map onto the painter at that size, as the GUI's mapper shows it: the area of the
    // player's room centred on it at that area's zoom, the player's marker and the map info box. As the GUI's does,
    // a draw that moves to another area raises sysMapAreaChanged. A script opens the map first, as with
    // openMapWidget(). The map info box runs the profile's Lua, so this is refused where submit() is. False, with
    // errorString() saying why, with no map open, a painter that is not active or a size under 10 by 10 pixels.
    bool renderMap(QPainter& painter, const QSize& size);
    // The pointer, at a position in what renderMap() last drew
    struct MapPointerEvent
    {
        enum class Type {
            press,
            // The second press of a double-click, which follows a press as QEvent::MouseButtonDblClick does
            doubleClick,
            wheel
        };
        Type type = Type::press;
        QPointF position;
        Qt::MouseButton button = Qt::LeftButton;
        Qt::KeyboardModifiers modifiers;
        // As QWheelEvent::angleDelta(), in eighths of a degree: 120 is a notch of most wheels
        QPoint angleDelta;
    };
    // What the GUI's mouse does on the 2D map, menus and dragging aside: a left press selects the room under it,
    // with Ctrl and Shift as in the GUI, and any press raises sysMapWindowMousePressEvent; a left double-click
    // speedwalks to the room under it; a wheel turn zooms the area about the pointer. Refused where submit() is.
    // False, with errorString() saying why, with no map open, before the map is drawn, for a double-click with
    // another button, for less than half a notch of the wheel, which the GUI ignores, and for a wheel turn on a
    // map whose last draw showed no area.
    bool mapPointer(const MapPointerEvent& event);

    // sysExitEvent and, if asked, a save for every profile that loaded, then the pool goes. Meanwhile no profile
    // opens, and none dials the game or logs in, a live connection staying for the exit handlers. Refused while a
    // profile loads, reconnects or runs a script, nested event loop or map operation, and during another
    // shutdown(). It may run the event loop; a session the frontend deletes there returns at once, false if the
    // save asked for was not yet made.
    bool shutdown(bool save);
    // The save shutdown(true) makes, with the profiles left running: every profile that loaded, the session's own
    // and ones its scripts opened, writes its save, synced modules, map and profile.ini, and the window layout is written,
    // with no sysExitEvent, so a frontend the system may stop without warning, such as a phone app sent to the
    // background, loses nothing. It returns once they are on disk, running the event loop as it waits, and refuses
    // as shutdown() does, though not for a reconnect. A profile whose save could not be loaded is left as it is,
    // keeping its last good save and map. False if a profile was not saved, a close a script began meanwhile
    // included, or the layout was not, errorString() saying which; a session the frontend deletes in the event
    // loop returns at once, false.
    bool save();
    // The profiles, the session's own or ones its scripts opened, that a close from a script or shutdown()
    // could not save, in the order it failed to
    QStringList unsavedProfiles() const { return mUnsavedProfiles; }
    bool saveFailed() const { return !mUnsavedProfiles.isEmpty(); }

signals:
    void profileLoaded(const QString& profile);
    // A line committed to the main console as the game's (what the game sends, feedTriggers(), Mudlet's own
    // OSC 8 examples), before triggers run on it; not what echo(), print() and Mudlet's messages add. A
    // handler must not run the event loop, as a modal dialog does: the profile's own timers may commit
    // another line or close the profile there, in the middle of this line's commit. runs covers text from its
    // start to its end, in order, with no empty run, and is empty for an empty line; a handler that takes
    // only the first three or four arguments may be connected. arrivalMSecs is when the line arrived, in ms since
    // the epoch as debugText's is, the time the GUI's timestamps show; -1 where it has none.
    void outputLine(const QString& profile, const QString& text, bool isPrompt, const QList<THeadlessSession::TextRun>& runs, qint64 arrivalMSecs);
    // What the GUI shows only on the profile's own screen, in pieces that need not end a line: text
    // written to the main console that the game did not send (echo(), print(), commands, Mudlet's
    // messages), and the error console. While lineShown is connected, it leaves out what that sends, as
    // docs/headless.md tells. Sent as written, so a handler may neither call the session nor run the event loop.
    void localText(const QString& profile, const QString& text);
    // With the colours the editor's error console prints the piece in; a handler that takes only the first two
    // arguments may be connected
    void errorConsoleText(const QString& profile, const QString& text, const QColor& foreground, const QColor& background);
    // A line of outputLine's as the main console shows it once its triggers have run: with what they replaced,
    // recoloured or wrote into it, which may hold line feeds, and not at all if they deleted it. Lines a trigger
    // feeds follow the line it ran on, as they are shown, and so does what it wrote anywhere else (an echo()
    // after deleteLine(), clearWindow() or a feedTriggers(), a send()'s command echo), as lines of their own.
    // Sent whole, before the console wraps it or after a trigger's wrapLine() split it, and not again for a later
    // change to it. Sent mid-write, as outputLine is, with the same arguments; a line a trigger wrote has the time
    // of the console line it went into, which is when it was written for a line of its own.
    void lineShown(const QString& profile, const QString& text, bool isPrompt, const QList<THeadlessSession::TextRun>& runs, qint64 arrivalMSecs);
    // A script emptied the main console, as clearWindow() does; lines from before it that lineShown has yet to
    // send are dropped. Sent mid-write, as outputLine is, so a handler may neither call the session nor run the
    // event loop.
    void mainConsoleCleared(const QString& profile);
    // The profile started or stopped logging the game's text to a file, where the GUI's logging button changes:
    // from a script's startLogging(), or as it loads with logging left on. fileName is the log being written,
    // empty once it stops. Also false, with logging still off, when a start fails (the reason is printed, as
    // localText), and once the profile closes, or shutdown() or a later start() lets it go, while it logs, though
    // not for the session being deleted. From the event loop.
    void loggingChanged(const QString& profile, bool logging, const QString& fileName);
    // The main console's font changed, by whatever changed it: a script's setFont() or setFontSize(), the load of
    // the profile's save, the bundled default standing in for a family that is missing, or a package bringing that
    // family or taking it away. The font the GUI would now draw it in, whose character cell setScreenSize() counts
    // in. From the event loop, with the font as it is then, and only when that differs from the one last sent or
    // the one consoleFont() gave at profileLoaded, so what the load did never follows it.
    void consoleFontChanged(const QString& profile, const QFont& font);
    // What the Central Debug Console prints, from any profile, marked with the profile's tag while more than one
    // is open, while setDebugConsoleShown() has it shown. Text to append as given, as the console does: a line ends
    // at its own newline and may come in pieces of different colours, as "ERROR:" and then why. arrivalMSecs is
    // when it was written, in milliseconds since the epoch, so earlier than now for what was kept from before the
    // console was shown. Sent as written, so a handler may neither call the session nor run the event loop.
    void debugText(const QString& text, const QColor& foreground, const QColor& background, qint64 arrivalMSecs);
    // consoleWrap() changed, from a script's setWindowWrap(), setWindowWrapIndent() or setWindowWrapHangingIndent()
    // on the main console. From the event loop, with the wrap as it is then, and only when that differs from the
    // one last sent or the one consoleWrap() gave at profileLoaded.
    void consoleWrapChanged(const QString& profile, int wrapAt, int indent, int hangingIndent);
    // consoleForeground() or consoleBackground() changed, from a script's setBackgroundColor() on the main console,
    // or a package or save loaded into the profile with colours of its own. From the event loop, as
    // consoleWrapChanged, with both colours as they are then.
    void consoleColorsChanged(const QString& profile, const QColor& foreground, const QColor& background);
    // The lag the GUI's latency box shows, paced as it is to once every 16ms, and only when the box's text, to the
    // millisecond, would change: the network's, from a command sent to the game's reply, while the game marks its
    // prompts with GA or EOR, otherwise none, where the box shows "<no GA>"; and how long the game's last text took
    // to process, its triggers included. From the event loop.
    void latencyChanged(const QString& profile, std::optional<double> networkSeconds, double processingSeconds);
    // The profile began connecting to the game, as it starts or reconnects, where the GUI marks its tab as
    // connecting; connected or disconnected follows, or the profile closes. From the event loop
    void connecting(const QString& profile);
    // What link() gives for these main console links may have changed, gathered from one pass of the event loop:
    // a click or followLink() leaving one visited, chosen or deselected in its group, a spoiler revealed, the game's
    // OSC 8 visibility settings concealing or revealing one, an MXP <EXPIRE>, a line holding it deleted, as by
    // deleteLine(), clearWindow() or a whole-line concealment, or the console letting go of one no line holds any
    // more after trims; link() gives nullopt once no line holds it.
    // Each id once, in the order they changed. Hover is not sent, the pointer being the app's own. From the event loop.
    void linksChanged(const QString& profile, const QList<int>& links);
    // timeStampsShown() changed, by setTimeStampsShown() or a script's enableTimeStamps() or disableTimeStamps() on
    // the main console. From the event loop, with the state as it is then, and only when that differs from the
    // one last sent or the one timeStampsShown() gave at profileLoaded.
    void timeStampsShownChanged(const QString& profile, bool shown);
    // What the GUI shows outside the profile's screen: a script's showNotification(), which answers true only
    // when this signal is connected and not blocked, with no milliseconds for the system's own time; and a
    // script's alert() or the game's bell, asking for the player's attention for that long, 0 until they come
    // back to the app; and, with 0, game text that arrives while setWindowMinimized() has the app minimized and the
    // profile asks for a notification when new data arrives while minimized, once each time it is minimized, as the
    // GUI's flash lasts until the player comes back.
    // Sent as asked, so a handler may neither call the session nor run the event loop.
    void notificationRequested(const QString& profile, const QString& title, const QString& text, std::optional<int> milliseconds);
    void attentionRequested(const QString& profile, int milliseconds);
    // What a script's announce(), or Mudlet on its behalf, asks screen readers to say, as plain text; assertive
    // for the "importantall" and "importantmostrecent" styles, as the GUI makes them, polite otherwise. Sent
    // as asked, so a handler may neither call the session nor run the event loop.
    void announcementRequested(const QString& profile, const QString& text, bool assertive);
    // The game hides what the player types, as for a password, or has stopped, where the GUI opens and closes its
    // hidden-input box, for an app to mask its own input line meanwhile. A disconnect ends it too, as does the
    // profile closing or being let go by shutdown() or a later start(), though not the session being deleted.
    // Never while the profile's "Show passwords as you type them" setting is on, as the GUI then opens no box.
    // Sent as the game's data is read, so a handler may neither call the session nor run the event loop.
    void hiddenInputWanted(const QString& profile, bool wanted);
    // The game offers a secure port while the connection is not encrypted, worded as the GUI asks, the port
    // in informativeText, for answerSecureConnectionOffer(). From the event loop, so a handler may answer at
    // once. It lapses when its connection ends; until then, or an answer, the profile offers nothing more.
    void secureConnectionOffered(const QString& profile, const QString& text, const QString& informativeText);
    // A game's GUI package downloading, as the GUI's progress dialog shows it: its title, then bytes got of
    // total, total -1 while unknown, then the end, whether it arrived, failed or was cancelled, or its profile was
    // let go, as for the map's progress below. From the event loop, so a handler may cancel it at once.
    void packageDownloadStarted(const QString& profile, const QString& title);
    void packageDownloadProgress(const QString& profile, qint64 got, qint64 total);
    void packageDownloadFinished(const QString& profile);
    // A map export, import or download, as the GUI's progress dialog shows it: its title, its first label and the
    // rooms it goes through, 0 until known; then its label, rooms done and rooms to go through as they change;
    // then the end, whether it was done, failed or aborted. From the event loop, which an export or import runs
    // as it goes, so the start and changes can come while the script, or the start() loading a JSON map, still
    // runs; the end comes once that has returned. A download, which only an app can begin, through host(), runs
    // on after its script. One still open when its profile closes, or when shutdown() or a later start() lets the
    // profile go, is ended there; a session being deleted sends no end.
    void mapProgressStarted(const QString& profile, const QString& title, const QString& label, int maximum);
    void mapProgressChanged(const QString& profile, const QString& label, int value, int maximum);
    void mapProgressFinished(const QString& profile);
    // The map operation can no longer be aborted, as the GUI's dialog then drops its Abort: at once for an XML
    // import, and for an export writing its file or a download that has arrived
    void mapProgressCancelDisabled(const QString& profile);
    // The map opened with no mapping script installed, where the GUI suggests one: the first time it opens
    // once the profile has loaded, as the GUI asks. The profile marks it asked then, whether or not anything
    // listens, and a save keeps that, so it never comes again for that profile. From the event loop, even once
    // shutdown() or a later start() has let the profile go, so long as the session is not deleted before then.
    void mappingScriptMissing(const QString& profile);
    // The 2D map would be drawn again where the GUI asks its mapper to repaint: the player moved or a script centred
    // the map, the map loaded or changed, or its zoom, colours, settings or info box did, and as a script opens the
    // map, shows it again or replaces it with an embedded mapper. While a map is open, and once a turn of the event
    // loop however many were asked for; from the event loop, so a handler may call renderMap().
    void mapRepaintRequested(const QString& profile);
    // A replay of this profile, as the GUI's replay toolbar follows it: its start, from a script's loadReplay() or
    // through host(), at 1 times its speed, as every replay starts; each change of its speed, from 1 to 1024 times,
    // which may repeat the speed where a change is clamped; and its end, however it ends, the end of the file included,
    // which a script's replay otherwise ends with nothing said, and one cut short as its file changes or becomes
    // unreadable while it plays, with the reason only in localText. A corrupt file, which loadReplay() reads through
    // before it plays anything, sends neither, and an empty one, which loadReplay() accepts, sends its start and then
    // its end. From the event loop, so the start can arrive after the script that began it has returned. One replay
    // plays at a time over every profile, and only this profile's are sent. One still playing when its profile closes,
    // or when shutdown() or a later start() lets the profile go, is ended there, as for the map's progress.
    void replayStarted(const QString& profile);
    void replaySpeedChanged(const QString& profile, int speed);
    void replayFinished(const QString& profile);
    // The whole of the main command line once a script's printCmdLine(), appendCmdLine(), clearCmdLine() or
    // sendCmdLine() sets it, or submit() empties it, for the app's input line to show. What a script sets while
    // hidden input is wanted is lost, as the GUI writes it into its hidden-input box instead. Sent as it is set, so
    // a handler may neither call the session nor run the event loop.
    void commandLineTextRequested(const QString& profile, const QString& text);
    // What the player typed ahead of the game's request to hide input has left Mudlet's copy of the line, as the
    // GUI's hidden-input box takes it from its command line: the app moves its input line's text into its hidden
    // input and empties the line. Just before hiddenInputWanted turns on; a handler may neither call the session
    // nor run the event loop.
    void typedLineMoveRequested(const QString& profile);
    // Either kind of sound was muted or unmuted, by the session or by a script's setConfig() in any of its profiles,
    // for the app's mute buttons to show; sent for a call that left it as it was too, as the GUI's buttons hear it.
    // Not for a profile, as muting is for all of them, but no longer sent once the session's own profile has closed.
    // From the event loop.
    void soundsMutedChanged(THeadlessSession::SoundSource source, bool muted);
    // A command of the profile was added, removed or changed in what addonCommands() gives, by a script, a click, a
    // package's removal or the profile's reset, for the app to read them again. One for all the changes made before the event
    // loop runs, from the event loop.
    void addonCommandsChanged(const QString& profile);
    // What buttonBars() gives changed, by a script, a package or the profile's reset, for the app to read the bars
    // again. One for all the changes made before the event loop runs, from the event loop.
    void buttonsChanged(const QString& profile);
    // The connection was made; from the event loop
    void connected(const QString& profile);
    // The connection ended, or could not be made, or was called off while connecting; from the event loop
    void disconnected(const QString& profile);
    // The session's profile closed; any other profile its scripts opened runs on until shutdown(). Not sent once
    // shutdown() has been called, even for a close a script made before it
    void quitRequested();
    // mainConsoleGrid() or mainConsoleTextRect() changed, from a border a script or MXP set, a font change,
    // setMainWindowSize() or setScreenSize(). From the event loop, with both as they are then, and only when they
    // differ from what was last sent or what they gave at profileLoaded, so a turn that changes them more than once
    // sends them once.
    void mainConsoleAreaChanged(const QString& profile, const QSize& grid, const QRect& textRect);
    // A script's setMainWindowSize() asked for the main window to be this size, for an app that can resize its
    // window to do so. The profile has taken it as setMainWindowSize() would have, so mainConsoleAreaChanged
    // follows if that changed the grid or where the text goes. From the event loop. Its handler may call the
    // session. The app answers by giving the size its window has once it has resized, or would not, from the
    // handler or later: the next size it gives is the session's answer, so a script asking for the same size again
    // changes nothing till the app gives one with nothing asked for, or a script asks for another. A size given from
    // the handler of a request a newer one has replaced is not laid out, and with nothing connected, nothing is
    // waited for.
    void mainWindowResizeRequested(const QString& profile, const QSize& pixels);

private:
    bool storeConnectionDetails(const Options&, std::optional<bool> tls);
    void detachFromHost();
    void connectInputRelays(unsigned int run);
    void connectCommandRelays(unsigned int run);
    void disconnectCommandRelays();
    // The profile's command whose shortcut the GUI's menu runs on the key; 0 for none
    int addonCommandOnKey(const QKeyEvent& event) const;
    void withdrawSecureConnectionOffer();
    void startMapProgress(const QString& title, const QString& label, int maximum);
    bool finishOpenProgress();
    // Told as they happen and sent from the event loop in that order, or by finishOpenProgress() if it comes
    // first; MudletReplay forgets its host before it says it is over, so whose replay it was is known only then
    enum class ReplaySignal { started, speedChanged, finished };
    void queueReplaySignal(ReplaySignal, int speed = 0);
    bool sendReplaySignals();
    void lineCommitted(int lineIndex);
    // text is null when the app did not give it
    KeyResult offerKey(Qt::Key, Qt::KeyboardModifiers, const QString* text);
    void triggerPassEnded(int lineIndex, int lastSegment);
    bool lineShownWanted() const;
    bool lineShownTaken() const;
    struct ShownLine;
    ShownLine wholeLine(const TBuffer& buffer, int first, int last) const;
    QList<ShownLine> takeTriggerWrites(bool taken);
    // False if a handler deleted the session
    bool sendShownLines(const QList<ShownLine>& lines);
    void closeDebugConsole();
    // The relays of how the main console is drawn, in THeadlessSessionOutput.cpp
    void connectOutputRelays(Host*, unsigned int run);
    void noteOutputStateAtLoad();
    void sendLatency();
    // The relays of the main window's area, in THeadlessSessionArea.cpp
    void connectAreaRelays(Host*, unsigned int run);
    void noteAreaAtLoad();
    void connectLinkRelays(Host*, unsigned int run);
    void sendLinksChanged(unsigned int run);
    // duringLoad takes a call while a profile loads, which only one that cannot reach the pool may do
    bool refused(bool duringLoad = false);
    bool usable(bool duringLoad = false);
    // Lua's own message, out of what the GUI's error console shows of it
    static QString luaErrorOf(const QString& consoleText);
    // False, with errorString() saying why, while the GUI's hidden-input box would not be open
    bool inputHidden(bool sending);
    void noteAlreadyInstalled(const QString& path, const QString& name);
    void relayMapRepaints(Host*, unsigned int run);

    QPointer<Host> mpHost;
    QString mProfileName;
    QString mErrorString;
    bool mShuttingDown = false;
    bool mLoadFailed = false;
    bool mRemovingStalePasswords = false;
    bool mSecureConnectionOffered = false;
    // Bumped by each offer, to drop one still queued for the frontend once a later one is made
    unsigned int mSecureConnectionOffer = 0;
    // What the map's progress last said, as each of its signals carries only one part
    QString mMapProgressLabel;
    int mMapProgressValue = 0;
    int mMapProgressMaximum = 0;
    // The profile hidden input, logging, a map operation, a package download or a replay was reported started for,
    // until its end is
    std::optional<QString> mHiddenInputFor;
    std::optional<QString> mLoggingFor;
    std::optional<QString> mMapProgressFor;
    std::optional<QString> mPackageDownloadFor;
    std::optional<QString> mReplayFor;
    struct QueuedReplaySignal
    {
        ReplaySignal signal;
        QString profile;
        int speed = 0;
    };
    QList<QueuedReplaySignal> mReplaySignals;
    // What consoleFont() gave at profileLoaded, then each consoleFontChanged; none before the load ends
    std::optional<QFont> mDeliveredConsoleFont;
    bool mApplicationActive = true;
    // Counted, as a handler that changed it and changed it back leaves it as it was
    unsigned int mApplicationActiveChanges = 0;
    bool mMapOperationCancelable = false;
    // The run whose mapRepaintRequested is queued, 0 for none
    unsigned int mMapRepaintQueuedRun = 0;
    QStringList mUnsavedProfiles;
    // Bumped by each start() and shutdown(), to drop a queued quitRequested or other relay owed to the profile
    // before, even one a script closed before shutdown() was called
    unsigned int mRun = 0;
    // The run an addonCommandsChanged is queued for, so the changes of one turn send one; 0 for none
    unsigned int mAddonCommandsQueuedRun = 0;
    // The same for buttonsChanged
    unsigned int mButtonsQueuedRun = 0;
    std::unique_ptr<HostManager> mpPool;
    struct ShownLine
    {
        QString text;
        bool isPrompt = false;
        QList<TextRun> runs;
        qint64 arrivalMSecs = -1;
    };
    // For each trigger pass under way, outermost first, the lines that passes nested in it have shown
    QList<QList<ShownLine>> mShownLinesOfPasses;
    // Whether lineShown was connected as the line the passes under way began with was committed
    bool mLineShownLatched = false;
    class DebugSink;
    std::unique_ptr<DebugSink> mpDebugSink;
    // What consoleWrap() gave at profileLoaded, then each consoleWrapChanged; none before the load ends
    std::optional<ConsoleWrap> mDeliveredConsoleWrap;
    // The same for consoleForeground() and consoleBackground()
    std::optional<std::pair<QColor, QColor>> mDeliveredConsoleColors;
    // Paces latencyChanged, for the run it was started in
    QTimer* mpLatencyPacer = nullptr;
    unsigned int mLatencyPaceRun = 0;
    // The latency box's text as last sent
    QString mDeliveredLatency;
    // What mainConsoleGrid() and mainConsoleTextRect() gave at profileLoaded, then each mainConsoleAreaChanged
    std::optional<std::pair<QSize, QRect>> mDeliveredMainConsoleArea;
    // The size a script asked for, and Host's count of that request, while mainWindowResizeRequested's handler runs
    std::optional<std::pair<QSize, quint64>> mAnsweringMainWindowRequest;
    bool mWindowMinimized = false;
    // Counted, so that the new-data alert of one time minimized is neither sent in another nor stops its own
    unsigned int mWindowMinimizedChanges = 0;
    // The run and time minimized the new-data alert was last sent for
    std::optional<std::pair<unsigned int, unsigned int>> mNewDataAlertedFor;
    // The links changed since linksChanged was last sent, for the run they changed in
    QList<int> mChangedLinks;
    // The same, to look up: a sweep or clearWindow() removes thousands at once
    QSet<int> mChangedLinkIds;
    bool mLinksChangedPending = false;
    // What timeStampsShown() gave at profileLoaded, then each timeStampsShownChanged; none before the load ends
    std::optional<bool> mDeliveredTimeStampsShown;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(THeadlessSession::TextStyles)

#endif // MUDLET_THEADLESSSESSION_H
