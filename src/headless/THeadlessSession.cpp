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

#include "THeadlessSession.h"

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "TConsoleModel.h"
#include "TLuaInterpreter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QTimer>

using namespace std::chrono_literals;

namespace {

// TNullAppFrontend's close waits only for a load, a nested event loop and a save wait, and leaves a running
// script or map operation to deletionMustWait(); deleting the whole pool cannot wait that way, so all count here
bool poolBusy(const HostManager& pool)
{
    if (pool.loadingProfile()) {
        return true;
    }
    for (const auto& pHost : pool.hostList()) {
        if (pHost->deletionWouldWait()) {
            return true;
        }
    }
    return false;
}

// One that is closing never resets, so its request stays pending
bool resetPending(const HostManager& pool)
{
    for (const auto& pHost : pool.hostList()) {
        if (pHost->profileResetInProgress() && !pHost->isClosingDown()) {
            return true;
        }
    }
    return false;
}

// The pool of a session in its shutdown(), or of a deleted one, which holds HostManager::self() until it goes
HostManager* gpClosingPool = nullptr;

// The signals being sent from inside the profile's code (a line's commit, a print, an error), which nothing
// in the pool shows. One pool a process, so not a session's own: the pool of one deleted from a handler waits on it.
int gSignalsBeingHandedOut = 0;

// Closes the profiles, unsaved, once none of those signals is being sent, nothing in the pool is busy or waiting
// to reset, and no profile taken out of it is still being let go
void deletePoolOnceIdle(HostManager* pPool)
{
    gpClosingPool = pPool;
    if (gSignalsBeingHandedOut || poolBusy(*pPool) || resetPending(*pPool) || HostManager::releasingProfile()) {
        QTimer::singleShot(50ms, QCoreApplication::instance(), [pPool]() {
            deletePoolOnceIdle(pPool);
        });
        return;
    }
    // Not before: the exit handlers it runs may run the event loop
    delete pPool;
    gpClosingPool = nullptr;
}

} // namespace

THeadlessSession::THeadlessSession(QObject* parent)
: QObject(parent)
{
}

THeadlessSession::~THeadlessSession()
{
    if (!mpPool) {
        return;
    }
    detachFromHost();
    // The exit handlers of a pool deleted at once may run the event loop, where the frontend may call it
    mShuttingDown = true;
    // Anything busy is waited for from a timer, not from an event loop run here as shutdown() does
    deletePoolOnceIdle(mpPool.release());
}

THeadlessSession::StartResult THeadlessSession::start(const Options& options)
{
    if (gpClosingPool) {
        //: Error from a frontend with no window that was asked to open a profile while those it closed were still closing
        mErrorString = tr("The profiles closed earlier are still closing; try again once they have.");
        return StartResult::poolClosing;
    }
    // One pool a process: profiles and Lua reach it through HostManager::self()
    if (mpPool || HostManager::self()) {
        //: Error from a frontend with no window that was asked to open a second set of profiles
        mErrorString = tr("A profile pool already exists in this process.");
        return StartResult::notOpened;
    }

    // Before the lookup, as making it settles the config root and reports profiles that the lookup cannot see
    mpPool = std::make_unique<HostManager>();
    // Every profile, a script's loadProfile() ones too, as one closed unsaved is lost all the same; the pool
    // may outlive the session
    mpPool->setProfileOpenedHook([self = QPointer<THeadlessSession>(this)](Host* pNewHost) {
        if (self) {
            connect(pNewHost, &Host::closeSaveFailed, self.data(), [self, name = pNewHost->getName()]() {
                self->mUnsavedProfiles.append(name);
            });
        }
    });
    mProfileName = MudletApp::getCanonicalProfileName(options.profile);
    if (mProfileName.isEmpty()) {
        mpPool.reset();
        //: %1 is the profile name asked for
        mErrorString = tr("There is no profile named \"%1\".").arg(options.profile);
        return StartResult::noSuchProfile;
    }

    // A profile's first load is where the GUI installs the game's default packages, which nothing here does
    // yet, and a save made here would keep the GUI from ever doing so
    const QStringList saves = QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName)).entryList(QStringList{qsl("*.xml")}, QDir::Files);
    if (saves.isEmpty() || saves == QStringList{qsl("Copied profile (settings only).xml")}) {
        mpPool.reset();
        //: %1 is a profile name
        mErrorString = tr("The profile \"%1\" has not been opened in Mudlet yet; open it there once first, so that it is set up.").arg(mProfileName);
        return StartResult::notSetUp;
    }

    // The load runs the event loop, where the frontend may delete the session
    const QPointer<THeadlessSession> self(this);
    const unsigned int run = ++mRun;
    mUnsavedProfiles.clear();
    mLoadFailed = false;
    Host* pHost = mpPool->openProfile(mProfileName, options.connect, [this, run](Host* pNewHost) {
        mpHost = pNewHost;
        connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::lineCommitted, this, &THeadlessSession::lineCommitted);
        connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::textPrinted, this, [this](const QString& text) {
            const QString profile = mProfileName;
            ++gSignalsBeingHandedOut;
            emit localText(profile, text);
            --gSignalsBeingHandedOut;
        });
        connect(pNewHost, &Host::signal_errorConsolePrint, this, [this](const QString& text) {
            const QString profile = mProfileName;
            ++gSignalsBeingHandedOut;
            emit errorConsoleText(profile, text);
            --gSignalsBeingHandedOut;
        });
        connect(
                &pNewHost->mTelnet,
                &cTelnet::signal_connected,
                this,
                [this, run]() {
                    if (run == mRun) {
                        const QString profile = mProfileName;
                        emit connected(profile);
                    }
                },
                Qt::QueuedConnection);
        connect(
                &pNewHost->mTelnet,
                &cTelnet::signal_disconnected,
                this,
                [this, run]() {
                    if (run == mRun) {
                        const QString profile = mProfileName;
                        emit disconnected(profile);
                    }
                },
                Qt::QueuedConnection);
        // Queued, so a close during the load is seen once the event loop runs, where quitting can end it; one
        // that arrives after a later start(), or after the shutdown() that closed this profile, is dropped
        connect(
                pNewHost,
                &QObject::destroyed,
                this,
                [this, run]() {
                    if (run == mRun) {
                        emit quitRequested();
                    }
                },
                Qt::QueuedConnection);
    });
    // The GUI connects a profile whose save failed too, but there the player sees why it has no scripts. Unless
    // a script that did load ran the event loop for long enough, the address is still being looked up.
    if (pHost && options.connect && (!self || !pHost->mLoadedOk)) {
        pHost->mTelnet.disconnectIt();
    }
    if (!self) {
        return StartResult::notOpened;
    }
    if (!pHost) {
        mpPool.reset();
        //: %1 is a profile name
        mErrorString = tr("The profile \"%1\" could not be opened.").arg(mProfileName);
        return StartResult::notOpened;
    }

    if (!pHost->mLoadedOk) {
        mLoadFailed = true;
        mErrorString = pHost->mProfileLoadError.isEmpty()
                               //: Error when a profile's saved settings and scripts could not be loaded, and no reason was given
                               ? tr("The profile's save could not be read.")
                               //: Error when a profile's saved settings and scripts could not be loaded; %1 is the profile's name, %2 the reason, such as "Premature end of document."
                               : tr("The profile \"%1\" could not be loaded: %2").arg(mProfileName, pHost->mProfileLoadError);
        return StartResult::loadFailed;
    }
    // A copy, as a handler may delete the session while its argument is in use
    const QString profile = mProfileName;
    emit profileLoaded(profile);
    return StartResult::started;
}
Host* THeadlessSession::host() const
{
    return mpHost.data();
}

bool THeadlessSession::submit(const QString& line)
{
    // An alias may run the event loop, where the frontend may delete the session, and not the profile with it
    const QPointer<THeadlessSession> self(this);
    // As the GUI's command line, each line of a paste is a command of its own; unlike there, it is checked as a
    // call, so one that a close in an earlier line's alias leaves nowhere to go ends the paste with the reason
    for (QString command : line.split(QChar::LineFeed)) {
        if (!usable()) {
            return false;
        }
        // The GUI's command line hands over no carriage returns, so neither aliases nor games expect one
        if (command.endsWith(QChar::CarriageReturn)) {
            command.chop(1);
        }
        const QPointer<Host> pHost = mpHost;
        pHost->send(command, true, false);
        if (pHost) {
            pHost->playerSentLineFromCommandLine();
        }
        if (!self) {
            return true;
        }
    }
    return true;
}

bool THeadlessSession::runLua(const QString& code)
{
    if (!usable()) {
        return false;
    }
    if (code.isEmpty()) {
        //: Error from a frontend with no window that was given no Lua code to run
        mErrorString = tr("There is no Lua code to run.");
        return false;
    }
    // The last part of what the GUI's error console shows is the error itself
    QString errorText;
    const auto errorCapture = connect(mpHost, &Host::signal_errorConsolePrint, this, [&errorText](const QString& text) {
        errorText = text;
    });
    const QPointer<THeadlessSession> self(this);
    const bool ran = mpHost->getLuaInterpreter()->compileAndExecuteScript(code);
    disconnect(errorCapture);
    if (!ran && self) {
        mErrorString = errorText.trimmed();
        // Lua's own message, out of the console's "<Lua error:...>"
        if (mErrorString.startsWith(QLatin1Char('<')) && mErrorString.endsWith(QLatin1Char('>'))) {
            mErrorString = mErrorString.sliced(1).chopped(1);
        }
        if (mErrorString.startsWith(qsl("Lua error:"))) {
            mErrorString = mErrorString.sliced(10);
        }
    }
    return ran;
}

bool THeadlessSession::shutdown(const bool save)
{
    if (refused()) {
        return false;
    }
    if (!mpPool) {
        return true;
    }
    if (poolBusy(*mpPool)) {
        qWarning() << "THeadlessSession: shutdown() while a profile runs a script or waits for its save is refused; call it once that has returned";
        //: Error from a frontend with no window that was asked to close its profiles while one ran a script or waited for its save to be written
        mErrorString = tr("A profile is running a script or waiting for its save, so its profiles cannot close until that has ended.");
        return false;
    }
    // As for TNullAppFrontend's close, a reset asked for first ends first: run in the middle of the close, it
    // would rebuild the Lua state the exit handlers and the save are using
    const QPointer<THeadlessSession> self(this);
    mShuttingDown = true;
    gpClosingPool = mpPool.get();
    while (self && resetPending(*mpPool)) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    // A frontend's timer in that loop may have deleted the session, whose profiles then close unsaved
    if (!self) {
        return !save;
    }
    if (mpHost) {
        ++mRun;
    }
    if (save) {
        // Closed unsaved, keeping its last good save and map; any other profile its scripts opened saves as usual
        if (mLoadFailed && mpHost && mpPool->getHost(mpHost->getName()) == mpHost.data()) {
            mpPool->deleteHost(mpHost->getName());
            if (!self) {
                return false;
            }
        }
        mpPool->closeAllProfiles();
    }
    // One a handler took out of the pool is let go from a timer, which must be over before the pool goes
    while (self && HostManager::releasingProfile()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    if (!self) {
        return true;
    }
    // Its exit handlers may run the event loop
    mpPool.reset();
    gpClosingPool = nullptr;
    if (!self) {
        return true;
    }
    mShuttingDown = false;
    detachFromHost();
    mpHost.clear();
    return true;
}

// So nothing the profile still sends reaches a session that has let it go
void THeadlessSession::detachFromHost()
{
    if (mpHost) {
        disconnect(mpHost, nullptr, this, nullptr);
        disconnect(&mpHost->mainConsoleModel().mNotifier, nullptr, this, nullptr);
        disconnect(&mpHost->mTelnet, nullptr, this, nullptr);
    }
}

void THeadlessSession::lineCommitted(const int lineIndex)
{
    if (!mpHost) {
        return;
    }
    const TBuffer& buffer = mpHost->mainConsoleModel().buffer;
    if (lineIndex < 0 || lineIndex >= buffer.lineBuffer.size()) {
        return;
    }
    // A handler may delete the session, whose pool then waits for the commit to end; hence the copy of the name
    const QString profile = mProfileName;
    ++gSignalsBeingHandedOut;
    emit outputLine(profile, buffer.lineBuffer.at(lineIndex), buffer.promptBuffer.at(lineIndex));
    --gSignalsBeingHandedOut;
}

// A handler of outputLine, localText or errorConsoleText runs in the middle of a write, which a line it fed
// would cut into, and a load that runs the event loop has yet to start the profile's scripts
bool THeadlessSession::refused()
{
    if (mShuttingDown) {
        qWarning() << "THeadlessSession: called while shutdown() closes the profiles, which is refused";
        //: Error from a frontend with no window that was called while it was closing its profiles
        mErrorString = tr("Refused while the profiles close.");
        return true;
    }
    if (mpPool && gSignalsBeingHandedOut) {
        qWarning() << "THeadlessSession: called from an outputLine, localText or errorConsoleText handler, which is refused; queue the call instead";
        //: Error from a frontend with no window that called it back from a handler of the game's lines, of the text the profile printed, or of its error console
        mErrorString = tr("Refused from a handler of outputLine, localText or errorConsoleText; queue the call instead.");
        return true;
    }
    if (mpPool && mpPool->loadingProfile()) {
        qWarning() << "THeadlessSession: called while a profile loads, which is refused; call it once that has returned";
        //: Error from a frontend with no window that was called while a profile was still loading
        mErrorString = tr("Refused while a profile loads; call it once that has returned.");
        return true;
    }
    return false;
}

bool THeadlessSession::usable()
{
    if (!mpHost) {
        //: Error from a frontend with no window that has no open profile to send to
        mErrorString = tr("No profile is open.");
        return false;
    }
    if (refused()) {
        return false;
    }
    if (mLoadFailed) {
        //: Error from a frontend with no window that was given a command or Lua for a profile whose save could not be loaded
        mErrorString = tr("The profile's save could not be loaded, so it takes no commands; close it.");
        return false;
    }
    // Refused from the start of a close, while sysExitEvent and the save still run with the connection up, so
    // nothing reaches the game after sysExitEvent; sooner than the GUI's command line, which takes typing until
    // the save is done
    if (mpHost->closeStarted()) {
        //: Error from a frontend with no window that was given a command or Lua for a profile that has begun to close
        mErrorString = tr("The profile is closing, so it takes no more commands.");
        return false;
    }
    return true;
}
