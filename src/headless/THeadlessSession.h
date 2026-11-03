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
    // hold it in a QPointer. Not from a handler of outputLine, localText or errorConsoleText: a line it fed
    // would be committed in the middle of a write, which TBuffer does not allow.
    Host* host() const;

    // As typed on the command line: its aliases run, and what is sent while not connected is echoed and dropped
    // with nothing said, as in the GUI, so a frontend that types ahead waits for connected(). These, and
    // installPackage(), are refused while a profile loads or shutdown() runs, and from a handler of
    // outputLine, localText or errorConsoleText, which run mid-write, and fail once a close of the profile has
    // begun; submit() checks each line of a paste as a call.
    bool submit(const QString& line);
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

    // sysExitEvent and, if asked, a save for every profile that loaded, then the pool goes. Refused while a
    // profile loads or runs a script, nested event loop or map operation, and during another shutdown(). It may
    // run the event loop; a session the frontend deletes there returns at once, false if the save asked for was
    // not yet made.
    bool shutdown(bool save);
    // The save shutdown(true) makes, with the profiles left running: every profile that loaded, the session's own
    // and ones its scripts opened, writes its save, synced modules, map and profile.ini, and the window layout is written,
    // with no sysExitEvent, so a frontend the system may stop without warning, such as a phone app sent to the
    // background, loses nothing. It returns once they are on disk, running the event loop as it waits, and refuses
    // as shutdown() does. A profile whose save could not be loaded is left as it is, keeping its last good save and
    // map. False if a profile was not saved, a close a script began meanwhile included, or the layout was not,
    // errorString() saying which; a session the frontend deletes in the event loop returns at once, false.
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
    // The connection was made; from the event loop
    void connected(const QString& profile);
    // The connection ended, or could not be made; from the event loop
    void disconnected(const QString& profile);
    // The session's profile closed; any other profile its scripts opened runs on until shutdown()
    void quitRequested();

private:
    bool storeConnectionDetails(const Options&, std::optional<bool> tls);
    void detachFromHost();
    void lineCommitted(int lineIndex);
    bool refused();
    bool usable();
    void noteAlreadyInstalled(const QString& path, const QString& name);

    QPointer<Host> mpHost;
    QString mProfileName;
    QString mErrorString;
    bool mShuttingDown = false;
    bool mLoadFailed = false;
    bool mRemovingStalePasswords = false;
    QStringList mUnsavedProfiles;
    // Bumped by each start(), and by a shutdown() that closes the profile, to drop a queued quitRequested
    // owed to the one before
    unsigned int mRun = 0;
    std::unique_ptr<HostManager> mpPool;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(THeadlessSession::TextStyles)

#endif // MUDLET_THEADLESSSESSION_H
