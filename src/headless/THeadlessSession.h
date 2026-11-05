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

#include <QColor>
#include <QFlags>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <memory>
#include <optional>

class Host;
class HostManager;

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
    // For what the session does not cover, such as settings and the map. Null once the profile closes, so
    // hold it in a QPointer. Not from a handler of outputLine, localText, errorConsoleText, hiddenInputWanted or a
    // ...Requested signal: a line it fed would be committed in the middle of a write, which TBuffer does not allow.
    Host* host() const;

    // As typed on the command line: its aliases run, and what is sent while not connected is echoed and dropped
    // with nothing said, as in the GUI, so a frontend that types ahead waits for connected(). These, and
    // installPackage(), are refused while a profile loads or shutdown() runs, and from a handler of
    // outputLine, localText, errorConsoleText, hiddenInputWanted or a ...Requested signal, which run mid-write, and
    // fail once a close of the profile has begun; submit() checks each line of a paste as a call. It is the app's
    // input line being sent: the main command line holds the line while its aliases run, then is emptied or kept
    // as the profile says. A command from elsewhere, such as a button, goes through runLua() and expandAlias(),
    // which leave the line alone.
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
    // The main console's size in characters, as a resize of the GUI's main window sets it. If the profile has
    // NAWS on and the game asks for it, the game is told from the event loop once the size has not changed for
    // 0.3s, and only if what it is told has changed, or when it next asks; until then it is told 90 by 25. The
    // profile's scripts read it as the main window's size, and a change raises sysWindowResizeEvent with that
    // size in pixels. As in the GUI, a width below 40 is taken as 40 and the game is told no more than the
    // profile's wrap width. Refused below 1 or above 65535, which NAWS cannot carry, and where submit() is.
    bool setScreenSize(int columns, int rows);
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
    // password; the app handles it. refused: where submit() is. errorString() says which.
    enum class KeyResult { bindingRan, noBinding, keyKept, refused };
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
    // only the first three arguments may be connected.
    void outputLine(const QString& profile, const QString& text, bool isPrompt, const QList<THeadlessSession::TextRun>& runs);
    // What the GUI shows only on the profile's own screen, in pieces that need not end a line: text
    // written to the main console that the game did not send (echo(), print(), commands, Mudlet's
    // messages), and the error console. Sent as written, so a handler may neither call the session nor
    // run the event loop.
    void localText(const QString& profile, const QString& text);
    void errorConsoleText(const QString& profile, const QString& text);
    // What the GUI shows outside the profile's screen: a script's showNotification(), which answers true only
    // when this signal is connected and not blocked, with no milliseconds for the system's own time; and a
    // script's alert() or the game's bell, asking for the player's attention for that long, 0 until they come
    // back to the app.
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
    // The connection was made; from the event loop
    void connected(const QString& profile);
    // The connection ended, or could not be made; from the event loop
    void disconnected(const QString& profile);
    // The session's profile closed; any other profile its scripts opened runs on until shutdown(). Not sent once
    // shutdown() has been called, even for a close a script made before it
    void quitRequested();

private:
    bool storeConnectionDetails(const Options&, std::optional<bool> tls);
    void detachFromHost();
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
    // duringLoad takes a call while a profile loads, which only one that cannot reach the pool may do
    bool refused(bool duringLoad = false);
    bool usable(bool duringLoad = false);
    // False, with errorString() saying why, while the GUI's hidden-input box would not be open
    bool inputHidden(bool sending);
    void noteAlreadyInstalled(const QString& path, const QString& name);

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
    // The profile hidden input, a map operation, a package download or a replay was reported started for, until
    // its end is
    std::optional<QString> mHiddenInputFor;
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
    bool mMapOperationCancelable = false;
    QStringList mUnsavedProfiles;
    // Bumped by each start() and shutdown(), to drop a queued quitRequested or other relay owed to the profile
    // before, even one a script closed before shutdown() was called
    unsigned int mRun = 0;
    std::unique_ptr<HostManager> mpPool;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(THeadlessSession::TextStyles)

#endif // MUDLET_THEADLESSSESSION_H
