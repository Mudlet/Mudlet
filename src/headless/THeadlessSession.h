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

#include <QObject>
#include <QPointer>
#include <QString>

#include <memory>
#include <optional>

class Host;
class HostManager;

// One profile run with no window, for a frontend of its own (a terminal, a phone app) to feed input and
// show the main console. It does no I/O itself, and like Host it is used on the application's thread only.
class THeadlessSession : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        // Matched as the GUI's --profile is, ignoring case
        QString profile;
        bool connect = true;
        // Stored in the profile before the load, as the connection dialog stores what is typed into it, one
        // left empty keeping what is stored; a host makes a profile there is none of, as the dialog does. A
        // new profile's tls is stored as it stands: as given, else as the dialog ticks it, on for a predefined
        // game that uses it; an existing profile's only when given. A port outside 1 to 65535 is refused.
        QString host;
        int port = 0;
        std::optional<bool> tls;
        QString login;
        // For this run only; nothing stores it
        QString password;
    };

    // invalidName: a profile was to be made under a name the connection dialog would not take. poolClosing:
    // the profiles of a session, this one or another, are still closing, from its shutdown() or, once it is
    // deleted, from the event loop.
    enum class StartResult { started, noSuchProfile, invalidName, notOpened, loadFailed, poolClosing };

    explicit THeadlessSession(QObject* parent = nullptr);
    // Closes the profiles unsaved, at once or, while one is busy as for shutdown(), a reset is pending or a
    // line is being handed out, from the event loop once that is over; a frontend that never runs the event
    // loop again then leaks them, and they never get their sysExitEvent. It may be deleted from the
    // frontend's own code, a script, or a handler of the session's signals, even during one of its calls;
    // from a handler of a signal of host() or of what it holds, use deleteLater(), as one sent with no
    // script running would have its profile deleted under it.
    ~THeadlessSession() override;

    // After the application's organisation and name are set, as it reads Mudlet.ini. Refused while a pool
    // of profiles exists. A profile loaded for the first time gets its game's default packages, as in the GUI.
    // On loadFailed the profile stays open for the frontend to close, its connect called off.
    StartResult start(const Options&);
    // Why the last call that failed did
    QString errorString() const { return mErrorString; }
    // For what the session does not cover, such as settings and the map. Null once the profile closes, so
    // hold it in a QPointer. Not from an outputLine handler: a line it fed would be committed in the middle
    // of the one being handed out, which TBuffer does not allow.
    Host* host() const;

    // As typed on the command line; one sent while not connected is reported on the main console, as in the
    // GUI. Both are refused while a profile loads or shutdown() runs, and from an outputLine handler, which
    // runs mid-commit, and fail once a close of the profile has begun; submit() checks each line
    // of a paste as a call.
    bool submit(const QString& line);
    bool runLua(const QString& code);
    // As the GUI installs a package given on its command line, but quietly: what went wrong is errorString()'s
    // to tell, not the main console's
    bool installPackage(const QString& path);

    // sysExitEvent and, if asked, a save for every profile, then the pool goes. Refused while a profile
    // loads or runs a script, nested event loop or map operation, and during another shutdown(). It may run
    // the event loop; a session the frontend deletes there returns at once, false if the save asked for was
    // not yet made.
    bool shutdown(bool save);
    // Whether a close of the profile, from a script or shutdown(), could not save it
    bool saveFailed() const { return mSaveFailed; }

signals:
    void profileLoaded(const QString& profile);
    // A line committed to the main console as the game's (what the game sends, feedTriggers(), Mudlet's own
    // OSC 8 examples), before triggers run on it; not what echo(), print() and Mudlet's messages add. A
    // handler must not run the event loop, as a modal dialog does: the profile's own timers may commit
    // another line or close the profile there, in the middle of this line's commit.
    void outputLine(const QString& profile, const QString& text, bool isPrompt);
    // What the GUI shows only on the profile's own screen, in pieces that need not end a line: text
    // written to the main console that the game did not send (echo(), print(), commands, Mudlet's
    // messages), and the error console. Sent as written, so a handler may neither call the session nor
    // run the event loop.
    void localText(const QString& profile, const QString& text);
    void errorConsoleText(const QString& profile, const QString& text);
    // The connection ended, or could not be made; from the event loop
    void disconnected(const QString& profile);
    // The session's profile closed; any other profile its scripts opened runs on until shutdown()
    void quitRequested();

private:
    bool storeConnectionDetails(const Options&, std::optional<bool> tls);
    void lineCommitted(int lineIndex);
    bool refused();
    bool usable();

    QPointer<Host> mpHost;
    QString mProfileName;
    QString mErrorString;
    bool mShuttingDown = false;
    bool mSaveFailed = false;
    // Bumped by each start(), and by a shutdown() that closes the profile, to drop a queued quitRequested
    // owed to the one before
    unsigned int mRun = 0;
    std::unique_ptr<HostManager> mpPool;
};

#endif // MUDLET_THEADLESSSESSION_H
