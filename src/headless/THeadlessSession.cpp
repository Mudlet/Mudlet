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

#include "CredentialManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletMedia.h"
#include "MudletReplay.h"
#include "SecureStringUtils.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TEncodingHelper.h"
#include "TEvent.h"
#include "TGameDetails.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TNullAppFrontend.h"
#include "TNullConsoleFrontend.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMetaMethod>
#include <QScopeGuard>
#include <QTimer>

#include <algorithm>
#include <functional>
#include <utility>

using namespace std::chrono_literals;

namespace {

// Answers to a secure port offer under way: cTelnet may reconnect and run the event loop while on the stack
int gConnectionsBeingChanged = 0;

// TNullAppFrontend's close waits only for a load, a nested event loop and a save wait, and leaves a running
// script or map operation to deletionMustWait(); deleting the whole pool cannot wait that way, so all count here.
// A save deletes nothing, so a reconnect's event loop holds up only a deletion.
bool poolBusy(const HostManager& pool, const bool forDeletion)
{
    if (pool.loadingProfile() || gConnectionsBeingChanged) {
        return true;
    }
    for (const auto& pHost : pool.hostList()) {
        if (forDeletion ? pHost->deletionWouldWait() : pHost->busyRunning()) {
            return true;
        }
    }
    return false;
}

// Told apart from poolBusy(), whose refusals name a script or a save
bool poolReconnecting(const HostManager& pool)
{
    for (const auto& pHost : pool.hostList()) {
        if (pHost->mTelnet.connectRunningEventLoop()) {
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
    if (gSignalsBeingHandedOut || poolBusy(*pPool, true) || resetPending(*pPool) || HostManager::releasingProfile()) {
        pPool->markDeletionPending();
        QTimer::singleShot(50ms, QCoreApplication::instance(), [pPool]() {
            deletePoolOnceIdle(pPool);
        });
        return;
    }
    // Not before: the exit handlers it runs may run the event loop
    delete pPool;
    gpClosingPool = nullptr;
}

// The connection dialog's TLS box
bool tlsAsTheDialogShowsIt(const QString& profile)
{
    if (const auto game = TGameDetails::findGame(profile); game != TGameDetails::scmDefaultGames.constEnd() && MudletApp::readProfileData(profile, qsl("port")).trimmed().isEmpty()) {
        return game->tlsEnabled;
    }
    return MudletApp::readProfileData(profile, qsl("ssl_tsl")).toInt() == Qt::Checked;
}

// The server the profile will connect to, as the dialog fills its field
QString hostToConnectTo(const QString& profile, const QString& given)
{
    if (!given.isEmpty()) {
        return given;
    }
    if (const QString stored = MudletApp::readProfileData(profile, qsl("url")).trimmed(); !stored.isEmpty()) {
        return stored;
    }
    const auto game = TGameDetails::findGame(profile);
    return game != TGameDetails::scmDefaultGames.constEnd() ? game->hostUrl : QString();
}

// Not a QEventLoop: a quit() asked for meanwhile would end it before the answer, which would then write to
// what the caller waits with once it was gone
void waitUntil(const std::function<bool()>& answered)
{
    while (!answered()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
}

// As the dialog does for a new profile: a password left by a profile deleted outside Mudlet would otherwise
// be sent to the new game. Only one a lookup finds is removed, so a store that cannot be reached holds up
// nothing. Empty unless one found could not be removed, else why.
QString removeStalePasswords(const QString& profile)
{
    CredentialManager store;
    // One operation at a time, so each starts from the last one's answer
    for (const QString& key : {qsl("character"), qsl("proxy")}) {
        std::optional<bool> found;
        store.credentialExists(profile, key, [&found](const bool exists) {
            found = exists;
        });
        waitUntil([&found]() {
            return found.has_value();
        });
        if (!*found) {
            continue;
        }
        std::optional<QString> failure;
        store.removePassword(profile, key, [&failure](const bool removed, const QString& error) {
            failure = removed ? QString() : error;
        });
        waitUntil([&failure]() {
            return failure.has_value();
        });
        if (!failure->isEmpty()) {
            return *failure;
        }
    }
    return QString();
}

// The GUI picks again at every launch by what each candidate holds, so a chosen root filling with profiles
// could become its pick: all of them are its, not only today's
bool aFolderTheGUICouldPick(const QString& dir)
{
    QStringList candidates = MudletApp::configRootCandidates(MudletApp::executableDir());
    // Mudlet's Flatpak (CI/org.mudlet.mudlet.yml) runs its GUI with an XDG_CONFIG_HOME of its own under HOME,
    // which this process's candidates cannot name
    candidates << qsl("%1/.var/app/org.mudlet.mudlet/config/mudlet").arg(QDir::homePath());
    return std::any_of(candidates.cbegin(), candidates.cend(), [&dir](const QString& candidate) {
        return MudletApp::sameFolder(candidate, dir);
    });
}

// Empty if dir, cleaned, can be the config root, or for none the GUI's can, else why not. The GUI's own folder
// is never one: its profiles' passwords are in the keychain or AppConfigLocation, where a chosen one does not look.
QString configDirProblem(const QString& dir)
{
    if (!dir.isEmpty() && !QDir::isAbsolutePath(dir)) {
        //: %1 is the folder an app that embeds Mudlet asked for its profiles to be kept in
        return THeadlessSession::tr("\"%1\" is not an absolute path, so profiles cannot be kept in it.").arg(dir);
    }
    if (MudletApp::configRootSettled()) {
        const QString settled = QDir::cleanPath(MudletApp::getMudletPath(enums::mainPath));
        const bool chosen = MudletApp::configRootChosen();
        if (chosen ? MudletApp::sameFolder(settled, dir) : dir.isEmpty()) {
            return QString();
        }
        if (dir.isEmpty()) {
            //: %1 is the folder an app that embeds Mudlet chose for its profiles, in use until the app restarts
            return THeadlessSession::tr("Profiles are kept in \"%1\" until the app restarts, so they cannot be kept in the GUI's folder.").arg(settled);
        }
        if (!chosen && MudletApp::sameFolder(settled, dir)) {
            //: %1 is the folder the GUI keeps its profiles in, in use until the app restarts
            return THeadlessSession::tr("\"%1\" is in use as the GUI's folder until the app restarts, so it cannot be chosen as one of the app's own.").arg(dir);
        }
        //: %1 is the folder profiles are kept in until the app restarts, %2 another one an app that embeds Mudlet asked for
        return THeadlessSession::tr("Profiles are kept in \"%1\" until the app restarts, so they cannot be kept in \"%2\".").arg(settled, dir);
    }
    if (dir.isEmpty()) {
        return QString();
    }
    if (aFolderTheGUICouldPick(dir)) {
        //: %1 is a folder the GUI keeps its profiles in or can at its next launch, which an app that embeds Mudlet asked to keep its own in
        return THeadlessSession::tr("\"%1\" is a folder the GUI keeps its profiles in, or can at its next launch, so it cannot be chosen as one of the app's own; leave it out to use the GUI's.")
                .arg(dir);
    }
    if (const QFileInfo info(dir); info.exists() && !info.isDir()) {
        //: %1 is the path an app that embeds Mudlet asked for its profiles to be kept in
        return THeadlessSession::tr("\"%1\" is not a folder, so profiles cannot be kept in it.").arg(dir);
    }
    if (!QDir().mkpath(dir)) {
        //: %1 is the folder an app that embeds Mudlet asked for its profiles to be kept in
        return THeadlessSession::tr("The folder \"%1\" could not be made, so profiles cannot be kept in it.").arg(dir);
    }
    // Nothing here tells the app's own profiles from those of a Mudlet out of sight, which would share them
    if (MudletApp::configDirHoldsProfiles(dir)) {
        qWarning().nospace().noquote() << "THeadlessSession::start(...) WARN: \"" << dir
                                       << "\" already holds profiles; a Mudlet that keeps its own there too, such as a portable install or one run under another HOME, "
                                          "shares them and its Mudlet.ini with this app.";
    }
    return QString();
}

} // namespace

class THeadlessSession::DebugSink final : public TDebug::Sink
{
public:
    explicit DebugSink(THeadlessSession& session)
    : mSession(session)
    {
    }

    void printDebugLine(const QString& text, const QColor& foreground, const QColor& background, const qint64 arrivalMSecs) override
    {
        ++gSignalsBeingHandedOut;
        emit mSession.debugText(text, foreground, background, arrivalMSecs);
        --gSignalsBeingHandedOut;
    }

private:
    THeadlessSession& mSession;
};

THeadlessSession::THeadlessSession(QObject* parent)
: QObject(parent)
{
}

THeadlessSession::~THeadlessSession()
{
    closeDebugConsole();
    if (!mpPool) {
        return;
    }
    detachFromHost();
    // The exit handlers of a pool deleted at once may run the event loop, where the frontend may call it
    mShuttingDown = true;
    // Anything busy is waited for from a timer, not from an event loop run here as shutdown() does
    deletePoolOnceIdle(mpPool.release());
}

THeadlessSession::StartResult THeadlessSession::start(const Options& asked)
{
    // Trimmed once, so that what is checked is what is stored and used, as the dialog trims these to check and
    // apply them; a host of only spaces is then none, and a name with spaces round it finds the profile it names
    Options options = asked;
    options.password.clear();
    // A copy of its own, taken before the event loop runs, where the caller may change or clear its own; and
    // cleared on return, so that nothing kept shares the buffer the caller clears
    QString password(asked.password.constData(), asked.password.size());
    const auto clearPassword = qScopeGuard([&password]() {
        SecureStringUtils::secureStringClear(password);
    });
    options.profile = options.profile.trimmed();
    options.host = options.host.trimmed();
    options.login = options.login.trimmed();
    if (gpClosingPool) {
        //: Error from a frontend with no window that was asked to open a profile while those it closed were still closing
        mErrorString = tr("The profiles closed earlier are still closing; try again once they have.");
        return StartResult::poolClosing;
    }
    // Before the pool is made, as a handler of that end may call shutdown() or start()
    if (!mpPool && !finishOpenProgress()) {
        return StartResult::notOpened;
    }
    // One pool a process: profiles and Lua reach it through HostManager::self()
    if (mpPool || HostManager::self()) {
        //: Error from a frontend with no window that was asked to open a second set of profiles
        mErrorString = tr("A profile pool already exists in this process.");
        return StartResult::notOpened;
    }

    const QString configDir = options.configDir.isEmpty() ? QString() : QDir::cleanPath(options.configDir);
    if (const QString problem = configDirProblem(configDir); !problem.isEmpty()) {
        mErrorString = problem;
        return StartResult::invalidConfigDir;
    }

    // Before the lookup, as making it settles the config root and reports profiles that the lookup cannot see
    mpPool = std::make_unique<HostManager>(HostManager::ConfigRoot::settledHere, configDir);
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
    if (mProfileName.isEmpty() && options.host.isEmpty()) {
        mpPool.reset();
        //: %1 is the profile name asked for
        mErrorString = tr("There is no profile named \"%1\".").arg(options.profile);
        return StartResult::noSuchProfile;
    }
    const bool newName = mProfileName.isEmpty();
    if (newName) {
        // As the dialog takes a new name
        const QString& name = options.profile;
        const QChar invalidChar = MudletApp::firstInvalidProfileNameChar(name);
        if (!invalidChar.isNull()) {
            mpPool.reset();
            //: %1 is the name given for a new profile, %2 a character in it; %3 lists the characters a profile name may have
            mErrorString = tr("\"%1\" cannot be the name of a profile: the %2 character is not permitted. Use one of the following: %3")
                                   .arg(name, QString(invalidChar), MudletApp::scmAllowedProfileNameChars);
            return StartResult::invalidName;
        }
        if (!MudletApp::profileNameUsableAsIs(name)) {
            mpPool.reset();
            //: %1 is the name given for a new profile, which names no folder of its own, such as "."
            mErrorString = tr("\"%1\" cannot be the name of a profile.").arg(name);
            return StartResult::invalidName;
        }
        mProfileName = name;
    }
    const bool newProfile = !QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).exists();
    std::optional<bool> tls = options.tls;
    const bool detailsGiven = !options.host.isEmpty() || options.port || tls.has_value();
    // Where a port stored now would change the box, or there is none stored yet
    if (!tls.has_value() && detailsGiven && MudletApp::readProfileData(mProfileName, qsl("port")).trimmed().isEmpty()
        && (newProfile || TGameDetails::findGame(mProfileName) != TGameDetails::scmDefaultGames.constEnd())) {
        tls = tlsAsTheDialogShowsIt(mProfileName);
    }
    // As the dialog checks what is typed into it
    if (detailsGiven) {
        const bool effectiveTls = tls.has_value() ? *tls : tlsAsTheDialogShowsIt(mProfileName);
        if (const QString problem = MudletApp::connectionDetailsProblem(hostToConnectTo(mProfileName, options.host), options.port, effectiveTls); !problem.isEmpty()) {
            mpPool.reset();
            mErrorString = problem;
            return StartResult::invalidConnectionDetails;
        }
    }
    // Removing them, and the load, run the event loop, where the frontend may delete the session. Only for a
    // name of the frontend's own, as the dialog: a game's entry is shared with the GUI and every other config root.
    const QPointer<THeadlessSession> self(this);
    if (newName && MudletApp::storingPasswordsSecurely()) {
        // Calls meanwhile are refused as during the load: a shutdown() would take the pool this is about to load into
        mRemovingStalePasswords = true;
        const QString failure = removeStalePasswords(mProfileName);
        if (!self) {
            return StartResult::notOpened;
        }
        mRemovingStalePasswords = false;
        if (!failure.isEmpty()) {
            mpPool.reset();
            //: %1 is the name of a new profile, %2 why the credential store kept a password filed under that name by a profile deleted earlier
            mErrorString = tr("The password filed under \"%1\" by an earlier profile of that name could not be removed: %2").arg(mProfileName, failure);
            return StartResult::notOpened;
        }
    }
    if (!storeConnectionDetails(options, tls)) {
        mpPool.reset();
        return StartResult::notOpened;
    }

    const unsigned int run = ++mRun;
    mDeliveredConsoleFont.reset();
    mUnsavedProfiles.clear();
    mLoadFailed = false;
    // A copy, as the profile's start prints to the debug console, whose handler may delete the session
    const QString profileName = mProfileName;
    Host* pHost = mpPool->openProfile(
            profileName,
            options.connect,
            [this, self, run, &password](Host* pNewHost) {
                if (!self) {
                    return;
                }
                mpHost = pNewHost;
                mSecureConnectionOffered = false;
                mMapOperationCancelable = false;
                pNewHost->givePass(password);
                mShownLinesOfPasses.clear();
                pNewHost->mainConsoleModel().mTriggerTextShownElsewhere = [self = QPointer<THeadlessSession>(this)]() {
                    return self && self->lineShownTaken();
                };
                connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::lineCommitted, this, &THeadlessSession::lineCommitted);
                connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::lineShown, this, &THeadlessSession::triggerPassEnded);
                connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::textPrinted, this, [this](const QString& text) {
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit localText(profile, text);
                    --gSignalsBeingHandedOut;
                });
                connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::bufferCleared, this, [this, model = &pNewHost->mainConsoleModel()]() {
                    for (QList<ShownLine>& lines : mShownLinesOfPasses) {
                        lines.clear();
                    }
                    // An app that hears the clear empties the open line, but the CLI's terminal keeps it
                    if (isSignalConnected(QMetaMethod::fromSignal(&THeadlessSession::mainConsoleCleared))) {
                        model->forgetPrintedLine();
                    }
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit mainConsoleCleared(profile);
                    --gSignalsBeingHandedOut;
                });
                // The file is read as sent, as a later toggle may change it before this is delivered
                connect(pNewHost, &Host::signal_loggingStateChanged, this, [this, run, pNewHost](const bool logging) {
                    const QString fileName = logging ? pNewHost->mainConsoleModel().mLogFileName : QString();
                    QMetaObject::invokeMethod(
                            this,
                            [this, run, logging, fileName]() {
                                if (run == mRun) {
                                    const QString profile = mProfileName;
                                    mLoggingFor = logging ? std::optional<QString>(profile) : std::nullopt;
                                    emit loggingChanged(profile, logging, fileName);
                                }
                            },
                            Qt::QueuedConnection);
                });
                connect(
                        pNewHost,
                        &Host::signal_consoleFontChanged,
                        this,
                        [this, run]() {
                            // Read now, not as sent: the load queues each step it took, the missing family before
                            // the bundled default that stands in for it, and consoleFont() gave the last already
                            if (run != mRun || !mpHost || !mDeliveredConsoleFont) {
                                return;
                            }
                            const QFont font = mpHost->displayFontWithNoView();
                            if (TFontAttributes(font) == TFontAttributes(*mDeliveredConsoleFont)) {
                                return;
                            }
                            mDeliveredConsoleFont = font;
                            const QString profile = mProfileName;
                            emit consoleFontChanged(profile, font);
                        },
                        Qt::QueuedConnection);
                connectOutputRelays(pNewHost, run);
                connectAreaRelays(pNewHost, run);
                connectLinkRelays(pNewHost, run);
                connect(pNewHost, &Host::signal_errorConsolePrint, this, [this](const QString& text, const QColor& foreground, const QColor& background) {
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit errorConsoleText(profile, text, foreground, background);
                    --gSignalsBeingHandedOut;
                });
                // Held by the profile, which an exit handler may still be running once the session has gone
                pNewHost->setNotificationHandler([self = QPointer<THeadlessSession>(this)](const QString& title, const QString& text, std::optional<int> milliseconds) {
                    if (!self || self->signalsBlocked() || !self->isSignalConnected(QMetaMethod::fromSignal(&THeadlessSession::notificationRequested))) {
                        return false;
                    }
                    const QString profile = self->mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit self->notificationRequested(profile, title, text, milliseconds);
                    --gSignalsBeingHandedOut;
                    return true;
                });
                relayMapRepaints(pNewHost, run);
                connect(pNewHost, &Host::signal_attentionRequested, this, [this](const int milliseconds) {
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit attentionRequested(profile, milliseconds);
                    --gSignalsBeingHandedOut;
                });
                // As the GUI's console, which alerts for new data only while the main window is minimized, and then
                // once, as QApplication::alert()'s flash lasts until the player comes back
                connect(&pNewHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::newDataAlerted, this, [this, run]() {
                    const std::pair<unsigned int, unsigned int> minimizing{run, mWindowMinimizedChanges};
                    if (run != mRun || !mWindowMinimized || mNewDataAlertedFor == minimizing) {
                        return;
                    }
                    mNewDataAlertedFor = minimizing;
                    // From the event loop, as this runs mid-write
                    QMetaObject::invokeMethod(
                            this,
                            [this, minimizing]() {
                                if (minimizing != std::pair{mRun, mWindowMinimizedChanges} || !mpHost) {
                                    return;
                                }
                                const QString profile = mProfileName;
                                ++gSignalsBeingHandedOut;
                                emit attentionRequested(profile, 0);
                                --gSignalsBeingHandedOut;
                            },
                            Qt::QueuedConnection);
                });
                // Direct, as the GUI opens and closes its box inside the emit
                connect(pNewHost, &Host::signal_passwordEntryWantedChanged, this, [this, run, pNewHost](const bool wanted) {
                    // The close of a profile being let go reads the game's data still, but its false has been sent
                    if (run != mRun || wanted == mHiddenInputFor.has_value()) {
                        return;
                    }
                    auto* pView = dynamic_cast<TNullConsoleFrontend*>(pNewHost->consoleFrontend());
                    const bool moved = wanted && pView && pView->hiddenInputOpened();
                    const QString profile = mProfileName;
                    mHiddenInputFor = wanted ? std::optional<QString>(profile) : std::nullopt;
                    const QPointer<THeadlessSession> self(this);
                    if (moved) {
                        ++gSignalsBeingHandedOut;
                        emit typedLineMoveRequested(profile);
                        --gSignalsBeingHandedOut;
                        if (!self) {
                            return;
                        }
                    }
                    ++gSignalsBeingHandedOut;
                    emit hiddenInputWanted(profile, wanted);
                    --gSignalsBeingHandedOut;
                    // After, so it empties the line the app has just put back
                    if (!wanted && self && run == mRun) {
                        if (auto* pClosedView = dynamic_cast<TNullConsoleFrontend*>(pNewHost->consoleFrontend())) {
                            pClosedView->hiddenInputClosed();
                        }
                    }
                });
                connect(pNewHost, &Host::signal_announcementRequested, this, [this](const QString& text, const QString& processing) {
                    const QString profile = mProfileName;
                    const bool assertive = processing == QLatin1String("importantall") || processing == QLatin1String("importantmostrecent");
                    ++gSignalsBeingHandedOut;
                    emit announcementRequested(profile, text, assertive);
                    --gSignalsBeingHandedOut;
                });
                // Queued, as the GUI's suggestion does not hold up the map opening; never dropped as stale, as the
                // profile has marked it asked, which its save keeps
                connect(
                        pNewHost,
                        &Host::signal_showMapperScriptReminder,
                        this,
                        [this, profile = mProfileName]() {
                            emit mappingScriptMissing(profile);
                        },
                        Qt::QueuedConnection);
                // For as long as the main window asks for it
                connect(&pNewHost->mTelnet, &cTelnet::signal_bell, this, [this]() {
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit attentionRequested(profile, 3000);
                    --gSignalsBeingHandedOut;
                });
#if !defined(QT_NO_SSL)
                // Sent on from the event loop, as the GUI's question is, so an answer cannot change the connection
                // while it is read; held from now, so that it lapses with its connection even before it is sent
                connect(&pNewHost->mTelnet, &cTelnet::signal_promptTlsAvailable, this, [this](const QString& text, const QString& informativeText) {
                    mSecureConnectionOffered = true;
                    const unsigned int offer = ++mSecureConnectionOffer;
                    QMetaObject::invokeMethod(
                            this,
                            [this, offer, text, informativeText]() {
                                if (mSecureConnectionOffered && offer == mSecureConnectionOffer) {
                                    const QString profile = mProfileName;
                                    emit secureConnectionOffered(profile, text, informativeText);
                                }
                            },
                            Qt::QueuedConnection);
                });
                connect(&pNewHost->mTelnet, &cTelnet::signal_disconnected, this, &THeadlessSession::withdrawSecureConnectionOffer);
#endif
                // Queued, as the start is sent while the game's message that asked for it is being read
                connect(
                        &pNewHost->mTelnet,
                        &cTelnet::signal_packageDownloadStarted,
                        this,
                        [this, run](const QString& title) {
                            if (run == mRun) {
                                mPackageDownloadFor = mProfileName;
                                const QString profile = mProfileName;
                                emit packageDownloadStarted(profile, title);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        &pNewHost->mTelnet,
                        &cTelnet::signal_packageDownloadProgress,
                        this,
                        [this, run](const qint64 got, const qint64 total) {
                            if (run == mRun) {
                                const QString profile = mProfileName;
                                emit packageDownloadProgress(profile, got, total);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        &pNewHost->mTelnet,
                        &cTelnet::signal_packageDownloadFinished,
                        this,
                        [this, run]() {
                            if (const auto profile = std::exchange(mPackageDownloadFor, std::nullopt); run == mRun && profile) {
                                emit packageDownloadFinished(*profile);
                            }
                        },
                        Qt::QueuedConnection);
                TMap* pMap = pNewHost->mpMap.data();
                // Direct, as cancelMapOperation() goes by what the map is doing now
                connect(pMap, &TMap::signal_mapTransferProgressStart, this, [this](const QString&, const QString&, const QString& cancelText) {
                    mMapOperationCancelable = !cancelText.isEmpty();
                });
                connect(pMap, &TMap::signal_mapJsonProgressStart, this, [this](const QString&, const QString&, const QString& cancelText) {
                    mMapOperationCancelable = !cancelText.isEmpty();
                });
                connect(pMap, &TMap::signal_mapProgressDisableCancel, this, [this]() {
                    mMapOperationCancelable = false;
                });
                connect(pMap, &TMap::signal_mapProgressClose, this, [this]() {
                    mMapOperationCancelable = false;
                });
                // Queued, as the export or import that sends it is under way
                connect(
                        pMap,
                        &TMap::signal_mapTransferProgressStart,
                        this,
                        [this, run](const QString& title, const QString& label) {
                            if (run == mRun) {
                                startMapProgress(title, label, 0);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapJsonProgressStart,
                        this,
                        [this, run](const QString& title, const QString& label, const QString&, const int maximum) {
                            if (run == mRun) {
                                startMapProgress(title, label, maximum);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapProgressSetLabel,
                        this,
                        [this, run](const QString& label) {
                            if (run == mRun) {
                                mMapProgressLabel = label;
                                const QString profile = mProfileName;
                                emit mapProgressChanged(profile, mMapProgressLabel, mMapProgressValue, mMapProgressMaximum);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapProgressSetRange,
                        this,
                        [this, run](const int, const int maximum) {
                            if (run == mRun) {
                                mMapProgressMaximum = maximum;
                                const QString profile = mProfileName;
                                emit mapProgressChanged(profile, mMapProgressLabel, mMapProgressValue, mMapProgressMaximum);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapProgressSetValue,
                        this,
                        [this, run](const int value) {
                            if (run == mRun) {
                                mMapProgressValue = value;
                                const QString profile = mProfileName;
                                emit mapProgressChanged(profile, mMapProgressLabel, mMapProgressValue, mMapProgressMaximum);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapProgressClose,
                        this,
                        [this, run]() {
                            if (const auto profile = std::exchange(mMapProgressFor, std::nullopt); run == mRun && profile) {
                                emit mapProgressFinished(*profile);
                            }
                        },
                        Qt::QueuedConnection);
                // After the start's own relay, so it follows mapProgressStarted
                connect(
                        pMap,
                        &TMap::signal_mapTransferProgressStart,
                        this,
                        [this, run](const QString&, const QString&, const QString& cancelText) {
                            if (run == mRun && mMapProgressFor && cancelText.isEmpty()) {
                                const QString profile = *mMapProgressFor;
                                emit mapProgressCancelDisabled(profile);
                            }
                        },
                        Qt::QueuedConnection);
                connect(
                        pMap,
                        &TMap::signal_mapProgressDisableCancel,
                        this,
                        [this, run]() {
                            if (run == mRun && mMapProgressFor) {
                                const QString profile = *mMapProgressFor;
                                emit mapProgressCancelDisabled(profile);
                            }
                        },
                        Qt::QueuedConnection);
                // Only one of this profile, as the GUI's toolbar drives the profile that plays it; sent from the
                // event loop, as a script's loadReplay() starts it
                if (MudletReplay* pReplay = MudletReplay::self()) {
                    connect(pReplay, &MudletReplay::signal_replayStarted, this, [this, run, pReplay]() {
                        if (run != mRun || !mpHost || pReplay->host() != mpHost.data()) {
                            return;
                        }
                        mReplayFor = mProfileName;
                        queueReplaySignal(ReplaySignal::started);
                    });
                    connect(pReplay, &MudletReplay::signal_replaySpeedChanged, this, [this, run](const int speed) {
                        if (run == mRun && mReplayFor) {
                            queueReplaySignal(ReplaySignal::speedChanged, speed);
                        }
                    });
                    // Whichever ended is this profile's while mReplayFor is held, as only one replay plays at a time
                    connect(pReplay, &MudletReplay::signal_replayOver, this, [this, run]() {
                        if (run == mRun && mReplayFor) {
                            queueReplaySignal(ReplaySignal::finished);
                            mReplayFor.reset();
                        }
                    });
                }
                connectInputRelays(run);
                connectCommandRelays(run);
                // Direct, so it keeps its place among the game's lines and hiddenInputWanted; an exit handler's
                // text is for a line the app no longer shows, and a script's close raises it before quitRequested
                connect(pNewHost, &Host::signal_commandLineTextRequested, this, [this, run, pNewHost](const QString& text) {
                    if (run != mRun || mShuttingDown || pNewHost->closeStarted()) {
                        return;
                    }
                    const QString profile = mProfileName;
                    ++gSignalsBeingHandedOut;
                    emit commandLineTextRequested(profile, text);
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
                            // Not from the event loop a shutdown() runs before it drops this run, as it waits on a reset;
                            // a handler of that end may have shut the profile down and started another
                            if (!mShuttingDown && run == mRun && finishOpenProgress() && run == mRun) {
                                emit quitRequested();
                            }
                        },
                        Qt::QueuedConnection);
            },
            [&options, tls, profile = profileName](Host* pNewHost) {
                // As the dialog: stored details over the game's own, then what was given
                const QString storedUrl = MudletApp::readProfileData(profile, qsl("url")).trimmed();
                const QString storedPort = MudletApp::readProfileData(profile, qsl("port")).trimmed();
                if (!storedUrl.isEmpty()) {
                    pNewHost->setUrl(storedUrl);
                }
                if (const int port = storedPort.toInt(); MudletApp::gamePortUsable(port)) {
                    pNewHost->setPort(port);
                }
                pNewHost->mSslTsl = tls.has_value() ? *tls : tlsAsTheDialogShowsIt(profile);
                if (!options.host.isEmpty()) {
                    pNewHost->setUrl(options.host);
                }
                if (options.port > 0) {
                    pNewHost->setPort(options.port);
                }
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
    // Read as the profile was made; told only now, as a handler may delete the session
    if (auto* pView = dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend())) {
        if (const auto error = pView->mainCommandLineHistoryReadError()) {
            //: Error console message when a profile with no window starts. %1 is the reason the file could not be read, from the operating system
            emit pHost->signal_errorConsolePrint(tr("[ WARN ]  - The command history could not be read (%1), so it is left as it was: "
                                                    "what it holds is not offered, and this session's commands are not saved to it.")
                                                         .arg(*error)
                                                         .append(QChar::LineFeed),
                                                 QColor(255, 128, 0),
                                                 QColor(Qt::black));
            if (!self) {
                return StartResult::notOpened;
            }
        }
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
    mDeliveredConsoleFont = consoleFont();
    noteOutputStateAtLoad();
    noteAreaAtLoad();
    // A copy, as a handler may delete the session while its argument is in use
    const QString profile = mProfileName;
    emit profileLoaded(profile);
    return StartResult::started;
}

// What the connection dialog writes as it is typed into, and Host reads as it is made
bool THeadlessSession::storeConnectionDetails(const Options& options, const std::optional<bool> tls)
{
    QList<std::pair<QString, QString>> items;
    if (!options.host.isEmpty()) {
        items.append({qsl("url"), options.host});
    }
    if (options.port > 0) {
        items.append({qsl("port"), QString::number(options.port)});
    } else if (options.tls.has_value() && MudletApp::readProfileData(mProfileName, qsl("port")).trimmed().isEmpty()) {
        // As the dialog stores the port it shows: with none stored, a game's TLS box goes back to the game's own
        if (const auto game = TGameDetails::findGame(mProfileName); game != TGameDetails::scmDefaultGames.constEnd()) {
            items.append({qsl("port"), QString::number(game->port)});
        }
    }
    if (tls.has_value()) {
        items.append({qsl("ssl_tsl"), QString::number(*tls ? Qt::Checked : Qt::Unchecked)});
    }
    if (!options.login.isEmpty()) {
        items.append({qsl("login"), options.login});
    }
    for (const auto& [item, value] : items) {
        if (const auto [stored, reason] = MudletApp::writeProfileData(mProfileName, item, value); !stored) {
            //: %1 is the internal name of a profile setting, such as url or port, %2 a profile name, %3 the reason it could not be written
            mErrorString = tr("The %1 of the profile \"%2\" could not be stored: %3").arg(item, mProfileName, reason);
            return false;
        }
    }
    return true;
}

Host* THeadlessSession::host() const
{
    return mpHost.data();
}

QFont THeadlessSession::consoleFont() const
{
    return mpHost ? mpHost->displayFontWithNoView() : QFont();
}

bool THeadlessSession::submit(const QString& line)
{
    // An alias may run the event loop, where the frontend may delete the session, and not the profile with it
    const QPointer<THeadlessSession> self(this);
    // As the GUI's command line, each line of a paste is a command of its own; unlike there, it is checked as a
    // call, so one that a close in an earlier line's alias leaves nowhere to go ends the paste with the reason
    QStringList commands = line.split(QChar::LineFeed);
    if (!usable()) {
        return false;
    }
    // Once, as the GUI's line is held or not as the paste is sent, whatever its aliases do to hidden input meanwhile
    bool held = false;
    if (auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend())) {
        held = pView->holdMainCommandLineForSend(line);
    }
    for (QString& command : commands) {
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
    // The whole paste is one entry, as the GUI's command line keeps it once every line is sent; one sent while
    // hidden input was wanted went past it, whatever its aliases did to hidden input meanwhile
    if (mpHost && held) {
        if (auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend())) {
            pView->addMainCommandLineHistory(commands.join(QChar::LineFeed));
            pView->mainCommandLineSent();
        }
    }
    return true;
}

bool THeadlessSession::submitHiddenInput(const QString& line)
{
    // A copy of its own, which the send clears, and which nothing kept shares with the caller's
    QString text(line.constData(), line.size());
    const auto clearText = qScopeGuard([&text]() {
        SecureStringUtils::secureStringClear(text);
    });
    if (!usable() || !inputHidden(true)) {
        return false;
    }
    // As the box: a pasted line break must not make a second line
    text.remove(QChar::CarriageReturn);
    text.remove(QChar::LineFeed);
    // The game would get a mangled password while the send reports success
    const QByteArray& encoding = mpHost->mTelnet.getEncoding();
    // As cTelnet::sendData() sends with no codec
    bool encodable = std::all_of(text.cbegin(), text.cend(), [](const QChar c) {
        return c.unicode() < 128;
    });
    if (!encoding.isEmpty() && TEncodingHelper::isEncodingAvailable(encoding)) {
        encodable = TEncodingHelper::canEncode(text, encoding);
    }
    if (!encodable) {
        //: Error from a frontend with no window whose hidden input, such as a password, has characters the game's encoding, %1, cannot carry; the input itself is never shown
        mErrorString = tr("The hidden input was not sent: it has characters the game's encoding, %1, cannot carry.").arg(encoding.isEmpty() ? qsl("ASCII") : QString::fromLatin1(encoding));
        return false;
    }
    if (!mpHost->sendPasswordEntry(std::move(text))) {
        //: Error from a frontend with no window whose hidden input, such as a password, could not be sent because Mudlet is not connected to the game
        mErrorString = tr("The hidden input was not sent: Mudlet is not connected to the game.");
        return false;
    }
    return true;
}

bool THeadlessSession::hiddenInputEdited()
{
    if (!usable() || !inputHidden(false)) {
        return false;
    }
    mpHost->passwordEntryEdited();
    return true;
}

bool THeadlessSession::dismissHiddenInput()
{
    if (!usable() || !inputHidden(false)) {
        return false;
    }
    mpHost->dismissPasswordEntry();
    return true;
}

bool THeadlessSession::inputHidden(const bool sending)
{
    if (mpHost->passwordEntryWanted()) {
        return true;
    }
    if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState) {
        if (sending) {
            //: Error from a frontend with no window whose hidden input, such as a password, could not be sent because Mudlet is not connected to the game
            mErrorString = tr("The hidden input was not sent: Mudlet is not connected to the game.");
        } else {
            //: Error from a frontend with no window asked about hidden input, such as a password, while Mudlet is not connected to the game
            mErrorString = tr("Hidden input is not open: Mudlet is not connected to the game.");
        }
    } else if (mpHost->isRemoteEchoingActive()) {
        if (sending) {
            //: Error from a frontend with no window whose hidden input, such as a password, was not sent because, though the game hides what is typed, the hidden input was dismissed, waits on the automatic login, or is turned off by the profile's setting to show passwords as they are typed
            mErrorString = tr("The hidden input was not sent: the game hides what is typed, but hidden input was dismissed, waits on the automatic login, or is off as passwords are shown as typed.");
        } else {
            //: Error from a frontend with no window asked about hidden input, such as a password, which, though the game hides what is typed, was dismissed, waits on the automatic login, or is turned off by the profile's setting to show passwords as they are typed
            mErrorString = tr("Hidden input is not open: the game hides what is typed, but hidden input was dismissed, waits on the automatic login, or is off as passwords are shown as typed.");
        }
    } else if (sending) {
        //: Error from a frontend with no window whose hidden input, such as a password, was not sent because the game is not hiding what the player types
        mErrorString = tr("The hidden input was not sent: the game is not hiding what is typed.");
    } else {
        //: Error from a frontend with no window asked about hidden input, such as a password, while the game is not hiding what the player types
        mErrorString = tr("Hidden input is not open: the game is not hiding what is typed.");
    }
    return false;
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
        mErrorString = luaErrorOf(errorText);
    }
    return ran;
}

QString THeadlessSession::luaErrorOf(const QString& consoleText)
{
    QString error = consoleText.trimmed();
    // Out of the console's "<Lua error:...>"
    if (error.startsWith(QLatin1Char('<')) && error.endsWith(QLatin1Char('>'))) {
        error = error.sliced(1).chopped(1);
    }
    if (error.startsWith(qsl("Lua error:"))) {
        error = error.sliced(10);
    }
    return error;
}

THeadlessSession::PackageResult THeadlessSession::installPackage(const QString& path)
{
    if (!usable()) {
        return PackageResult::failed;
    }
    // Before the name is looked for, which a missing file would otherwise pass
    if (QFile file(path); !file.open(QIODevice::ReadOnly)) {
        //: %1 is the path of a package file, %2 is why it could not be installed
        mErrorString = tr("The package \"%1\" could not be installed: %2").arg(path, file.errorString());
        return PackageResult::failed;
    }
    // The install, and the wait for a save, may run the event loop, where the frontend may delete the session
    const QPointer<THeadlessSession> self(this);
    // A save running now would put the install off until it ends, and answer true before it has happened
    mpHost->waitForProfileSave();
    if (!self || !usable()) {
        return PackageResult::failed;
    }
    if (mpHost->currentlySavingProfile()) {
        //: %1 is the path of a package file
        mErrorString = tr("The package \"%1\" could not be installed: the profile is still saving.").arg(path);
        return PackageResult::failed;
    }
    // Left as it is, so a run that installs it can be run again; one of the same name is taken to be it
    if (const QString name = mpHost->sanitizePackageName(path); mpHost->mInstalledPackages.contains(name)) {
        noteAlreadyInstalled(path, name);
        return PackageResult::alreadyInstalled;
    }
    QStringList problems;
    const auto [installed, reason] = mpHost->installPackage(path, enums::PackageModuleType::Package, true, &problems);
    if (!self) {
        return PackageResult::failed;
    }
    if (installed) {
        // Host raises sysInstall and sysInstallPackage from one zero timer set before this one, and Qt fires
        // zero timers in the order set, though it does not promise to, so what runs next sees what they did
        QEventLoop settle;
        QTimer::singleShot(0ms, &settle, &QEventLoop::quit);
        settle.exec();
        if (!self) {
            return PackageResult::failed;
        }
        if (!reason.isEmpty()) {
            problems.append(reason);
        }
        if (problems.isEmpty()) {
            return PackageResult::installed;
        }
        //: %1 is the path of a package file, %2 is what went wrong with the parts of it that are not working
        mErrorString = tr("The package \"%1\" was installed, but not all of it works: %2").arg(path, problems.join(qsl("; ")));
        return PackageResult::installedWithErrors;
    }
    // Its config.lua named it as one already installed
    for (const QString& name : std::as_const(mpHost->mInstalledPackages)) {
        if (reason == Host::packageAlreadyInstalledReason(name)) {
            noteAlreadyInstalled(path, name);
            return PackageResult::alreadyInstalled;
        }
    }
    //: %1 is the path of a package file, %2 is why it could not be installed
    mErrorString = tr("The package \"%1\" could not be installed: %2").arg(path, reason);
    return PackageResult::failed;
}

bool THeadlessSession::setScreenSize(const int columns, const int rows)
{
    if (!usable()) {
        return false;
    }
    if (columns < 1 || columns > 65535 || rows < 1 || rows > 65535) {
        //: Error from a frontend with no window given a screen size that cannot be sent to the game; %1 is the number of columns asked for, %2 the number of rows
        mErrorString = tr("A screen of %1 columns by %2 rows cannot be sent to the game: each must be from 1 to 65535.").arg(columns).arg(rows);
        return false;
    }
    if (!mpHost->noteMainAreaGiven(mAnsweringMainWindowRequest)) {
        return true;
    }
    // The GUI's main console never reports itself narrower
    const QSize area(std::max(40, columns), rows);
    // As TConsole::resizeEvent(), which tells scripts nothing for the size they have
    if (!mpHost->mainAreaInPixels() && area == mpHost->mainAreaCells()) {
        return true;
    }
    mpHost->setMainAreaCells(area);
    // The first handler may close the profile, and a function the frontend bound into Lua delete the session
    mpHost->layOutMainAreaWithNoView(Host::MainAreaChange::area);
    return true;
}

bool THeadlessSession::setDebugConsoleShown(const bool shown)
{
    if (refused()) {
        return false;
    }
    if (!shown) {
        if (TDebug::sink() && !debugConsoleShown()) {
            //: Error from a frontend with no window that asked to close the Central Debug Console while another session in the same app had taken it over by showing it
            mErrorString = tr("Another session has shown the Central Debug Console since, so only that one can close it.");
            return false;
        }
        closeDebugConsole();
        return true;
    }
    if (debugConsoleShown()) {
        return true;
    }
    if (!mpDebugSink) {
        mpDebugSink = std::make_unique<DebugSink>(*this);
    }
    TDebug::setSink(mpDebugSink.get());
    TDebug::smDebugMode = true;
    // As the GUI's console does once it is shown; a handler may delete the session meanwhile
    const QPointer<THeadlessSession> self(this);
    TDebug::flushMessageQueue();
    // Not TDebug::announceFilters(), which points at the GUI's filter bar
    if (const int hidden = TDebug::hiddenCategoryCount(); self && debugConsoleShown() && hidden) {
        //: Shown in the Central Debug Console of an app with no window when it opens with some kinds of message hidden. %n is how many.
        const QString text = tr("%n kind(s) of message are hidden.\n", "", hidden);
        // TDebug's own tag for a message of its own
        mpDebugSink->printDebugLine(qsl("[\u2731] %1").arg(text), Qt::white, Qt::darkBlue, QDateTime::currentMSecsSinceEpoch());
    }
    return true;
}

void THeadlessSession::setDebugCategories(const TDebug::Categories categories)
{
    TDebug::setEnabledCategories(categories & TDebug::csmAllCategories);
}

TDebug::Categories THeadlessSession::debugCategories() const
{
    return TDebug::enabledCategories();
}

bool THeadlessSession::debugConsoleShown() const
{
    return mpDebugSink && TDebug::sink() == mpDebugSink.get();
}

// As closing the GUI's console does
void THeadlessSession::closeDebugConsole()
{
    if (debugConsoleShown()) {
        TDebug::setSink(nullptr);
        TDebug::smDebugMode = false;
    }
}

bool THeadlessSession::answerSecureConnectionOffer(const bool accept)
{
    if (!usable()) {
        return false;
    }
#if !defined(QT_NO_SSL)
    if (mSecureConnectionOffered) {
        mSecureConnectionOffered = false;
        cTelnet& telnet = mpHost->mTelnet;
        // What slot_tlsUpgradeResponse() checks before it acts on an answer, which it otherwise only logs
        const bool taken = telnet.getConnectionState() == QAbstractSocket::ConnectedState && !telnet.currentlySecure() && (!accept || mpHost->mMSSPTlsPort > 0);
        if (!taken) {
            //: Error from a frontend with no window whose answer to the game's offer of a secure, encrypted port came after the game took the offer back
            mErrorString = tr("The game has taken back its offer of a secure port, so the answer was not used.");
        }
        ++gConnectionsBeingChanged;
        const auto answered = qScopeGuard([]() {
            --gConnectionsBeingChanged;
        });
        // Lets the next offer be made even when the answer is not used
        telnet.slot_tlsUpgradeResponse(accept);
        return taken;
    }
#else
    Q_UNUSED(accept)
#endif
    //: Error from a frontend with no window that was asked to answer the game's offer of a secure, encrypted port when the game has made no such offer
    mErrorString = tr("The game has offered no secure port to answer.");
    return false;
}

bool THeadlessSession::cancelPackageDownload()
{
    if (!usable()) {
        return false;
    }
    // Not from the signals, which reach the frontend queued, so after the download has started or ended
    if (!mpHost->mTelnet.packageDownloadRunning()) {
        //: Error from a frontend with no window that was asked to cancel the download of a game's GUI package when none is downloading
        mErrorString = tr("No package is downloading.");
        return false;
    }
    mpHost->mTelnet.slot_cancelPackageDownload();
    return true;
}

bool THeadlessSession::cancelMapOperation()
{
    // Taken while a profile loads too, as the JSON map that load reads can be aborted, which TMap::restore()
    // allows for, and this only flags the map or aborts its download
    if (!usable(true)) {
        return false;
    }
    if (!mMapOperationCancelable) {
        //: Error from a frontend with no window that was asked to abort a map export, import or download when none is running that can still be aborted
        mErrorString = tr("No map export, import or download is running that can be aborted.");
        return false;
    }
    mpHost->mpMap->slot_mapProgressDialogCancelled();
    return true;
}

namespace {

// The combinations TCommandLine::event() acts on without offering them to a binding, or for a keypad key, once
// none has taken it
bool commandLineKeepsKey(const Host& host, const QKeyEvent& event)
{
    const Qt::KeyboardModifiers modifiers = event.modifiers();
    if (modifiers & Qt::KeypadModifier) {
        switch (event.key()) {
        case Qt::Key_Enter:
            return modifiers == Qt::KeypadModifier;
#if defined(Q_OS_MACOS)
        // The arrows carry the flag there
        case Qt::Key_Up:
        case Qt::Key_Down:
            return modifiers == Qt::KeypadModifier || modifiers == (Qt::ControlModifier | Qt::KeypadModifier);
#endif
        default:
            return false;
        }
    }
    switch (event.key()) {
    case Qt::Key_Space:
    case Qt::Key_Backtab:
        return (modifiers & ~Qt::ShiftModifier) == Qt::NoModifier;
    case Qt::Key_Tab:
        return host.caretShortcutMatches(&event) || modifiers == Qt::NoModifier;
    case Qt::Key_F6:
        return host.caretShortcutMatches(&event);
    case Qt::Key_Backspace:
        return (modifiers & ~(Qt::ControlModifier | Qt::ShiftModifier)) == Qt::NoModifier;
    case Qt::Key_Return:
        return modifiers == Qt::NoModifier || modifiers == Qt::ControlModifier || modifiers == Qt::ShiftModifier;
    case Qt::Key_Enter:
    case Qt::Key_Delete:
    case Qt::Key_Escape:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return modifiers == Qt::NoModifier;
#if !defined(Q_OS_MACOS)
    case Qt::Key_Up:
    case Qt::Key_Down:
        return modifiers == Qt::NoModifier || modifiers == Qt::ControlModifier;
#endif
    default:
        return false;
    }
}

// As the ShortcutOverride by which the GUI's focused line keeps a key from the main window's menu shortcuts: the
// caret-mode key, then what QPlainTextEdit claims (QInputControl::isCommonTextEditShortcut()) or, while hidden input
// is wanted, what QLineEdit does (QWidgetLineControl::processShortcutOverrideEvent())
bool lineClaimsKey(const Host& host, const QKeyEvent& event, const bool hiddenInput)
{
    if (host.caretShortcutMatches(&event)) {
        return true;
    }
    const Qt::KeyboardModifiers modifiers = event.modifiers();
    const bool plain = modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier || modifiers == Qt::KeypadModifier;
    if (hiddenInput) {
        static const QList<QKeySequence::StandardKey> lineEditKeys{QKeySequence::Copy,
                                                                   QKeySequence::Paste,
                                                                   QKeySequence::Cut,
                                                                   QKeySequence::Redo,
                                                                   QKeySequence::Undo,
                                                                   QKeySequence::DeleteCompleteLine,
                                                                   QKeySequence::MoveToNextWord,
                                                                   QKeySequence::MoveToPreviousWord,
                                                                   QKeySequence::MoveToStartOfLine,
                                                                   QKeySequence::MoveToEndOfLine,
                                                                   QKeySequence::MoveToStartOfBlock,
                                                                   QKeySequence::MoveToEndOfBlock,
                                                                   QKeySequence::MoveToStartOfDocument,
                                                                   QKeySequence::MoveToEndOfDocument,
                                                                   QKeySequence::SelectPreviousWord,
                                                                   QKeySequence::SelectNextWord,
                                                                   QKeySequence::SelectStartOfLine,
                                                                   QKeySequence::SelectEndOfLine,
                                                                   QKeySequence::SelectStartOfBlock,
                                                                   QKeySequence::SelectEndOfBlock,
                                                                   QKeySequence::SelectStartOfDocument,
                                                                   QKeySequence::SelectEndOfDocument,
                                                                   QKeySequence::SelectAll};
        if (std::any_of(lineEditKeys.cbegin(), lineEditKeys.cend(), [&event](const QKeySequence::StandardKey key) {
                return event.matches(key);
            })) {
            return true;
        }
        if (!plain) {
            return false;
        }
        switch (event.key()) {
        case Qt::Key_Delete:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Backspace:
        case Qt::Key_Left:
        case Qt::Key_Right:
            return true;
        default:
            return event.key() < Qt::Key_Escape;
        }
    }
    if (plain) {
        switch (event.key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Delete:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Backspace:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Tab:
            return true;
        default:
            return event.key() < Qt::Key_Escape;
        }
    }
    static const QList<QKeySequence::StandardKey> textEditKeys{QKeySequence::Copy,
                                                               QKeySequence::Paste,
                                                               QKeySequence::Cut,
                                                               QKeySequence::Redo,
                                                               QKeySequence::Undo,
                                                               QKeySequence::MoveToNextWord,
                                                               QKeySequence::MoveToPreviousWord,
                                                               QKeySequence::MoveToStartOfDocument,
                                                               QKeySequence::MoveToEndOfDocument,
                                                               QKeySequence::SelectNextWord,
                                                               QKeySequence::SelectPreviousWord,
                                                               QKeySequence::SelectStartOfLine,
                                                               QKeySequence::SelectEndOfLine,
                                                               QKeySequence::SelectStartOfBlock,
                                                               QKeySequence::SelectEndOfBlock,
                                                               QKeySequence::SelectStartOfDocument,
                                                               QKeySequence::SelectEndOfDocument,
                                                               QKeySequence::SelectAll};
    return std::any_of(textEditKeys.cbegin(), textEditKeys.cend(), [&event](const QKeySequence::StandardKey key) {
        return event.matches(key);
    });
}

// As TPasswordEntry::handleKeyPress(), which judges a printable key by its text; without the text, by its code
bool hiddenInputKeepsKey(const Host& host, const QKeyEvent& event, const QString* text)
{
    const Qt::KeyboardModifiers modifiers = event.modifiers() & ~Qt::KeypadModifier;
    if (host.caretShortcutMatches(&event)) {
        return true;
    }
    switch (event.key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
    case Qt::Key_Tab:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        if (modifiers == Qt::NoModifier) {
            return true;
        }
        break;
    case Qt::Key_Backtab:
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
            return true;
        }
        break;
    default:
        break;
    }
    if (event.matches(QKeySequence::Copy) || event.matches(QKeySequence::Cut) || event.matches(QKeySequence::Undo) || event.matches(QKeySequence::Redo)) {
        return true;
    }
    // Qt gives a key that types a character that character's code, below its first special key, but not every
    // Ctrl+Alt with one is AltGr typing it
    const bool printable = text ? !text->isEmpty() && text->front().isPrint() : event.key() >= Qt::Key_Space && event.key() < Qt::Key_Escape;
    const Qt::KeyboardModifiers modifierKeys = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    const bool altGr = (modifierKeys & Qt::ControlModifier) && (modifierKeys & Qt::AltModifier);
#if defined(Q_OS_MACOS)
    const bool layoutModifier = altGr || modifierKeys == Qt::AltModifier;
#else
    const bool layoutModifier = altGr;
#endif
    return printable && (modifierKeys == Qt::NoModifier || layoutModifier);
}

} // namespace

THeadlessSession::KeyResult THeadlessSession::pressKey(const Qt::Key key, const Qt::KeyboardModifiers modifiers)
{
    return offerKey(key, modifiers, nullptr);
}

THeadlessSession::KeyResult THeadlessSession::pressKey(const Qt::Key key, const Qt::KeyboardModifiers modifiers, const QString& text)
{
    return offerKey(key, modifiers, &text);
}

THeadlessSession::KeyResult THeadlessSession::offerKey(const Qt::Key key, const Qt::KeyboardModifiers modifiers, const QString* text)
{
    if (!usable()) {
        return KeyResult::refused;
    }
    const QKeyEvent event(QEvent::KeyPress, key, modifiers, text ? *text : QString());
    const bool keypad = modifiers & Qt::KeypadModifier;
    const bool hiddenInput = mpHost->passwordEntryWanted();
    // Qt offers the main window's shortcuts a key before the focused line has it, unless the line claims it first
    if (!lineClaimsKey(*mpHost, event, hiddenInput)) {
        if (const int command = addonCommandOnKey(event)) {
            // The click's sysCommandClicked handlers may delete the session
            const QPointer<THeadlessSession> self(this);
            if (auto* pView = dynamic_cast<TNullAppFrontend*>(TAppFrontend::instance())) {
                pView->activateAddonCommand(command);
            }
            if (self) {
                //: Error from a frontend with no window that was given a key press which is the keyboard shortcut of a command a script added to the menu, which the GUI runs rather than offering the key to its input line
                mErrorString = tr("That key is the shortcut of a command a script added to the menu, which ran.");
            }
            return KeyResult::commandRan;
        }
    }
    if (hiddenInput) {
        if (hiddenInputKeepsKey(*mpHost, event, text)) {
            //: Error from a frontend with no window that was given a key press while the game hides what the player types, as for a password, which the GUI's hidden-input box would take rather than any key binding
            mErrorString = tr("While the game hides what is typed, its hidden-input box keeps that key, typing it if printable; the app types it into its hidden input.");
            return KeyResult::keyKept;
        }
    } else {
        // TCommandLine::event() offers a keypad key to the bindings ahead of its own keys, and any other after them
        if (keypad && mpHost->getKeyUnit()->processDataStream(key, modifiers)) {
            return KeyResult::bindingRan;
        }
        if (commandLineKeepsKey(*mpHost, event)) {
            // TCommandLine selects its whole line on Escape, so what the player types next replaces it, and ends Tab's cycle
            if (auto* pFrontend = mpHost->consoleFrontend(); key == Qt::Key_Escape && pFrontend) {
                pFrontend->selectCommandLineText(QString());
                if (auto* pView = dynamic_cast<TNullConsoleFrontend*>(pFrontend)) {
                    pView->resetMainCommandLineCompletion();
                }
            }
            //: Error from a frontend with no window that was given a key press, such as Return or Up, which the GUI's command line acts on itself rather than offering it to any key binding
            mErrorString = tr("The GUI's command line keeps that key for itself, offering it to no key binding; the app handles it.");
            return KeyResult::keyKept;
        }
    }
    // A keypad key the line did not keep has met the bindings already
    if ((hiddenInput || !keypad) && mpHost->getKeyUnit()->processDataStream(key, modifiers)) {
        return KeyResult::bindingRan;
    }
    //: Error from a frontend with no window that was given a key press which none of the profile's key bindings takes
    mErrorString = tr("No key binding of the profile takes that key.");
    return KeyResult::noBinding;
}

bool THeadlessSession::setCommandLineText(const QString& text, const CommandLineEdit edit)
{
    if (!usable()) {
        return false;
    }
    if (mpHost->passwordEntryWanted() && !text.isEmpty()) {
        //: Error from a frontend with no window that was given the text of its input line while the game hides what the player types, as for a password, which scripts must not read
        mErrorString = tr("The command line takes no text while the game hides what is typed; send it with submitHiddenInput().");
        return false;
    }
    if (auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend())) {
        pView->typeMainCommandLine(text, edit == CommandLineEdit::typed);
    }
    return true;
}

namespace {

// By the profile's own replay, not replayStarted's relay, which is queued. Not while a script runs: one that runs
// the event loop may be in the middle of a chunk, which cTelnet reads on from where it was once the script is done.
bool replayPlaysIn(Host* pHost, QString& errorString)
{
    if (pHost->busyRunning()) {
        //: Error from a frontend with no window that was asked to pause, resume, stop or change the speed of a replay while a script of its profile is running, maybe in the middle of a replayed line
        errorString = QCoreApplication::translate("THeadlessSession", "The profile is running a script, so its replay cannot be changed until that has ended.");
        return false;
    }
    if (pHost->mTelnet.isReplaying()) {
        return true;
    }
    //: Error from a frontend with no window that was asked to pause, resume, stop or change the speed of a replay when its profile plays none
    errorString = QCoreApplication::translate("THeadlessSession", "No replay of the profile is playing.");
    return false;
}

} // namespace

bool THeadlessSession::pauseReplay()
{
    if (!usable() || !replayPlaysIn(mpHost, mErrorString)) {
        return false;
    }
    if (mpHost->mTelnet.replayPaused()) {
        //: Error from a frontend with no window that was asked to pause a replay that is already paused
        mErrorString = tr("The replay is already paused.");
        return false;
    }
    mpHost->mTelnet.pauseReplay();
    return true;
}

bool THeadlessSession::resumeReplay()
{
    if (!usable() || !replayPlaysIn(mpHost, mErrorString)) {
        return false;
    }
    if (!mpHost->mTelnet.replayPaused()) {
        //: Error from a frontend with no window that was asked to resume a replay that is not paused
        mErrorString = tr("The replay is not paused.");
        return false;
    }
    mpHost->mTelnet.resumeReplay();
    return true;
}

bool THeadlessSession::stopReplay()
{
    if (!usable() || !replayPlaysIn(mpHost, mErrorString)) {
        return false;
    }
    mpHost->mTelnet.stopReplay();
    return true;
}

bool THeadlessSession::replayPaused() const
{
    return mpHost && mpHost->mTelnet.isReplaying() && mpHost->mTelnet.replayPaused();
}

bool THeadlessSession::changeReplaySpeed(const bool faster)
{
    if (!usable() || !replayPlaysIn(mpHost, mErrorString)) {
        return false;
    }
    if (faster) {
        MudletReplay::self()->speedUp();
    } else {
        MudletReplay::self()->speedDown();
    }
    return true;
}

int THeadlessSession::replaySpeed() const
{
    return mpHost && mpHost->mTelnet.isReplaying() ? MudletReplay::self()->speed() : -1;
}

qint64 THeadlessSession::replayElapsedMSecs() const
{
    if (!mpHost || !mpHost->mTelnet.isReplaying()) {
        return -1;
    }
    return MudletReplay::self()->elapsedMSecs();
}

QStringList THeadlessSession::commandHistory() const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    if (!pView) {
        return {};
    }
    QStringList history = pView->mainCommandLineHistory();
    // The empty first entry is the line being typed, as up arrow from it reaches the newest command
    history.removeAll(QString());
    return history;
}

void THeadlessSession::noteAlreadyInstalled(const QString& path, const QString& name)
{
    const QString version = mpHost->mPackageInfo.value(name).value(qsl("version"));
    mErrorString = version.isEmpty()
                           //: %1 is the path of a package file, %2 the name of the package already installed under that name
                           ? tr("The package \"%1\" was not installed, as \"%2\" is already installed.").arg(path, name)
                           //: %1 is the path of a package file, %2 the name of the package already installed under that name, %3 its version
                           : tr("The package \"%1\" was not installed, as \"%2\" %3 is already installed.").arg(path, name, version);
}

bool THeadlessSession::shutdown(const bool save)
{
    if (refused()) {
        return false;
    }
    if (!mpPool) {
        // Shown before a start() that failed, or after the last shutdown(); with a pool, detachFromHost() closes it
        // once the profiles have written their last
        closeDebugConsole();
        return true;
    }
    if (poolReconnecting(*mpPool)) {
        qWarning() << "THeadlessSession: shutdown() while a profile reconnects is refused; call it once the reconnect has returned";
        //: Error from a frontend with no window that was asked to close its profiles while one was in the middle of reconnecting to its game
        mErrorString = tr("A profile is reconnecting, so its profiles cannot close until that has returned.");
        return false;
    }
    if (poolBusy(*mpPool, true)) {
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
    mpPool->markDeletionPending();
    while (self && resetPending(*mpPool)) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    // A frontend's timer in that loop may have deleted the session, whose profiles then close unsaved
    if (!self) {
        return !save;
    }
    ++mRun;
    // Its end is sent here, and the close below runs the event loop, where it would go on printing
    if (mpHost && mpHost->mTelnet.isReplaying()) {
        mpHost->mTelnet.endReplay(QString());
    }
    // Also for a profile that has gone while its own end waits in the event loop, which the bump drops
    if (!finishOpenProgress()) {
        return !save;
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

bool THeadlessSession::save()
{
    if (refused()) {
        return false;
    }
    if (!mpPool) {
        //: Error from a frontend with no window that has no open profile to send to
        mErrorString = tr("No profile is open.");
        return false;
    }
    if (poolBusy(*mpPool, false)) {
        qWarning() << "THeadlessSession: save() while a profile runs a script or waits for its save is refused; call it once that has returned";
        //: Error from a frontend with no window that was asked to save its profiles while one ran a script or waited for its save to be written
        mErrorString = tr("A profile is running a script or waiting for its save, so its profiles cannot be saved until that has ended.");
        return false;
    }
    // Each save waits for its writes in the event loop, where a profile may close or the session be deleted
    const QPointer<THeadlessSession> self(this);
    QList<QPointer<Host>> profiles;
    for (const auto& pHost : mpPool->hostList()) {
        if (pHost->mLoadedOk) {
            profiles.append(pHost.data());
        }
    }
    const qsizetype closesUnsavedBefore = mUnsavedProfiles.size();
    QStringList unsaved;
    for (const QPointer<Host>& pHost : std::as_const(profiles)) {
        // A close begun meanwhile saves it, or not, as it was asked to
        if (!pHost || pHost->closeStarted()) {
            continue;
        }
        const QString name = pHost->getName();
        bool saved = pHost->saveForClose();
        if (!self) {
            return false;
        }
        // The event loop would write it after this returns; a close has the Host's deletion write it
        if (pHost && !pHost->syncProfileIni()) {
            saved = false;
        }
        if (!saved) {
            unsaved.append(name);
        }
    }
    for (const QString& name : mUnsavedProfiles.mid(closesUnsavedBefore)) {
        if (!unsaved.contains(name)) {
            unsaved.append(name);
        }
    }
    QStringList problems;
    if (!unsaved.isEmpty()) {
        //: Error from a frontend with no window whose profiles could not all be saved; %1 lists the names of the ones that were not
        problems.append(tr("These profiles could not be saved: %1").arg(unsaved.join(qsl(", "))));
    }
    // As each close writes it
    if (auto* pFrontend = TAppFrontend::instance(); !pFrontend || !pFrontend->saveWindowLayoutForScript()) {
        //: Error from a frontend with no window that saved its profiles but not where their windows are
        problems.append(tr("Where the profiles' windows are could not be saved."));
    }
    if (!problems.isEmpty()) {
        mErrorString = problems.join(QLatin1Char(' '));
        return false;
    }
    return true;
}

// cTelnet makes no other offer until one is answered, and would apply a late answer to whichever connection is
// up by then. With this one gone, the answer only frees it to make the next.
void THeadlessSession::withdrawSecureConnectionOffer()
{
#if !defined(QT_NO_SSL)
    if (mSecureConnectionOffered && mpHost) {
        mSecureConnectionOffered = false;
        // A no while connected would close the connection and stop the asking
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState) {
            mpHost->mTelnet.slot_tlsUpgradeResponse(false);
        }
    }
#endif
}

// So nothing the profile still sends reaches a session that has let it go
void THeadlessSession::detachFromHost()
{
    closeDebugConsole();
    if (mpLatencyPacer) {
        mpLatencyPacer->stop();
    }
    mSecureConnectionOffered = false;
    mMapOperationCancelable = false;
    if (mpHost) {
        disconnect(mpHost, nullptr, this, nullptr);
        mpHost->setNotificationHandler(nullptr);
        disconnect(&mpHost->mainConsoleModel().mNotifier, nullptr, this, nullptr);
        disconnect(&mpHost->mTelnet, nullptr, this, nullptr);
        disconnect(mpHost->mpMap.data(), nullptr, this, nullptr);
    }
    if (MudletReplay* pReplay = MudletReplay::self()) {
        disconnect(pReplay, nullptr, this, nullptr);
    }
    if (MudletMedia* pMedia = MudletMedia::self()) {
        disconnect(pMedia, nullptr, this, nullptr);
    }
    disconnectCommandRelays();
}

void THeadlessSession::startMapProgress(const QString& title, const QString& label, const int maximum)
{
    mMapProgressLabel = label;
    mMapProgressValue = 0;
    mMapProgressMaximum = maximum;
    mMapProgressFor = mProfileName;
    const QString profile = mProfileName;
    emit mapProgressStarted(profile, title, label, maximum);
}

// The end of hidden input, logging, a map operation, package download or replay whose own is dropped as stale, or
// never comes as its Host has gone; false if a handler deleted the session
bool THeadlessSession::finishOpenProgress()
{
    const QPointer<THeadlessSession> self(this);
    // A closing profile's connection ends without resetting the game's echo, so its Host never sends this
    if (const auto profile = std::exchange(mHiddenInputFor, std::nullopt)) {
        ++gSignalsBeingHandedOut;
        emit hiddenInputWanted(*profile, false);
        --gSignalsBeingHandedOut;
        if (!self) {
            return false;
        }
    }
    // A closing profile stops logging without saying so, and a false still queued is dropped as stale
    if (const auto profile = std::exchange(mLoggingFor, std::nullopt)) {
        emit loggingChanged(*profile, false, QString());
    }
    if (!self) {
        return false;
    }
    if (const auto profile = std::exchange(mMapProgressFor, std::nullopt)) {
        emit mapProgressFinished(*profile);
    }
    if (!self) {
        return false;
    }
    if (const auto profile = std::exchange(mPackageDownloadFor, std::nullopt)) {
        emit packageDownloadFinished(*profile);
    }
    if (!self) {
        return false;
    }
    if (mReplayFor) {
        queueReplaySignal(ReplaySignal::finished);
        mReplayFor.reset();
    }
    return sendReplaySignals();
}

void THeadlessSession::queueReplaySignal(const ReplaySignal signal, const int speed)
{
    mReplaySignals.append({signal, *mReplayFor, speed});
    QMetaObject::invokeMethod(
            this,
            [this, run = mRun]() {
                if (run == mRun) {
                    sendReplaySignals();
                }
            },
            Qt::QueuedConnection);
}

bool THeadlessSession::sendReplaySignals()
{
    const QPointer<THeadlessSession> self(this);
    while (!mReplaySignals.isEmpty()) {
        const QueuedReplaySignal next = mReplaySignals.takeFirst();
        switch (next.signal) {
        case ReplaySignal::started:
            emit replayStarted(next.profile);
            break;
        case ReplaySignal::speedChanged:
            emit replaySpeedChanged(next.profile, next.speed);
            break;
        case ReplaySignal::finished:
            emit replayFinished(next.profile);
            break;
        }
        if (!self) {
            return false;
        }
    }
    return true;
}

namespace {

THeadlessSession::TextStyles textStyles(const TChar& format)
{
    THeadlessSession::TextStyles styles;
    styles.setFlag(THeadlessSession::TextStyle::Bold, format.isBoldAsked());
    styles.setFlag(THeadlessSession::TextStyle::Italic, format.isItalic());
    styles.setFlag(THeadlessSession::TextStyle::Underline, format.isUnderlined() || format.isUnderlineWavy() || format.isUnderlineDotted() || format.isUnderlineDashed());
    styles.setFlag(THeadlessSession::TextStyle::Overline, format.isOverlined());
    styles.setFlag(THeadlessSession::TextStyle::StrikeOut, format.isStruckOut());
    styles.setFlag(THeadlessSession::TextStyle::Reverse, format.isReversed());
    styles.setFlag(THeadlessSession::TextStyle::Blink, format.isBlinking());
    styles.setFlag(THeadlessSession::TextStyle::FastBlink, format.isFastBlinking());
    styles.setFlag(THeadlessSession::TextStyle::Concealed, format.isConcealed());
    return styles;
}

// A split double-byte character (TChar::hasSplitFormat()) is given its left half's format, as one QChar holds both halves
QList<THeadlessSession::TextRun> textRuns(const QString& text, const TLineFormat& format, const QColor& profileForeground, const QColor& profileBackground)
{
    QList<THeadlessSession::TextRun> runs;
    const int length = std::min(static_cast<qsizetype>(format.size()), text.size());
    int start = 0;
    while (start < length) {
        const TChar& first = format[start];
        const THeadlessSession::TextStyles styles = textStyles(first);
        int end = start + 1;
        // By style rather than TChar's flags, which also tell alternate fonts apart
        while (end < length && format[end].foregroundRgba() == first.foregroundRgba() && format[end].backgroundRgba() == first.backgroundRgba() && textStyles(format[end]) == styles
               && format[end].linkIndex() == first.linkIndex()) {
            ++end;
        }
        THeadlessSession::TextRun run;
        run.start = start;
        run.length = end - start;
        run.foreground = first.foreground();
        run.background = first.background();
        run.defaultForeground = first.foregroundRgba() == profileForeground.rgba();
        run.defaultBackground = first.backgroundRgba() == profileBackground.rgba();
        run.styles = styles;
        run.link = first.linkIndex();
        runs.append(run);
        start = end;
    }
    return runs;
}

// A segment a wrap split off has no time of its own, so takes that of the line it came from
qint64 arrivalTime(const TBuffer& buffer, int line)
{
    if (line < 0 || line >= buffer.timeBuffer.size()) {
        return -1;
    }
    while (line > 0 && buffer.timeBuffer.at(line) == TBuffer::csmWrappedLineTimeStamp) {
        --line;
    }
    const qint64 time = buffer.timeBuffer.at(line);
    return time < 0 ? -1 : time;
}

} // namespace

void THeadlessSession::lineCommitted(const int lineIndex)
{
    if (mShownLinesOfPasses.isEmpty()) {
        mLineShownLatched = lineShownWanted();
    }
    // What a pass wrote before feeding this line is shown above it
    QList<ShownLine> written = takeTriggerWrites(lineShownTaken());
    if (!written.isEmpty() && !mShownLinesOfPasses.isEmpty()) {
        mShownLinesOfPasses.last().append(std::move(written));
    }
    mShownLinesOfPasses.append(QList<ShownLine>());
    if (!mpHost) {
        return;
    }
    const TBuffer& buffer = mpHost->mainConsoleModel().buffer;
    if (lineIndex < 0 || lineIndex >= buffer.lineBuffer.size()) {
        return;
    }
    const QString& text = buffer.lineBuffer.at(lineIndex);
    const QList<TextRun> runs = textRuns(text, buffer.buffer.at(lineIndex), mpHost->mFgColor, mpHost->mBgColor);
    // A handler may delete the session, whose pool then waits for the commit to end; hence the copy of the name
    const QString profile = mProfileName;
    ++gSignalsBeingHandedOut;
    emit outputLine(profile, text, buffer.promptBuffer.at(lineIndex), runs, arrivalTime(buffer, lineIndex));
    --gSignalsBeingHandedOut;
}

void THeadlessSession::triggerPassEnded(const int lineIndex, const int lastSegment)
{
    if (mShownLinesOfPasses.isEmpty()) {
        return;
    }
    const bool taken = lineShownTaken();
    QList<ShownLine> lines = mShownLinesOfPasses.takeLast();
    lines.append(takeTriggerWrites(taken));
    if (!mpHost || !taken) {
        return;
    }
    const TBuffer& buffer = mpHost->mainConsoleModel().buffer;
    if (lineIndex >= 0 && lineIndex < buffer.lineBuffer.size()) {
        lines.prepend(wholeLine(buffer, lineIndex, std::min(lastSegment, static_cast<int>(buffer.lineBuffer.size()) - 1)));
    } else if (lineIndex >= 0) {
        qWarning() << "THeadlessSession: lineShown named line" << lineIndex << "of a main console of" << buffer.lineBuffer.size() << "lines, which is not sent";
    }
    // Fed from inside an enclosing pass, so shown below that pass's line, which is not yet sent
    if (!mShownLinesOfPasses.isEmpty()) {
        mShownLinesOfPasses.last().append(lines);
        return;
    }
    sendShownLines(lines);
}

// Puts back what a script's wrapLine() split the line into, as the GUI's copy does
THeadlessSession::ShownLine THeadlessSession::wholeLine(const TBuffer& buffer, const int first, const int last) const
{
    // What a delete left of a split line starts on a wrapped segment, indented by the console
    const int firstPadding = buffer.wrapsFromPreviousLine(first) ? std::min(buffer.wrapIndentWidth(first), static_cast<int>(buffer.lineBuffer.at(first).size())) : 0;
    QString text = buffer.lineBuffer.at(first).mid(firstPadding);
    const TLineFormat& firstFormat = buffer.buffer.at(first);
    std::vector<TChar> format(firstFormat.begin() + std::min<qsizetype>(firstPadding, firstFormat.size()), firstFormat.end());
    format.resize(text.size(), format.empty() ? TChar() : format.back());
    for (int y = first + 1; y <= last; ++y) {
        const QString& segment = buffer.lineBuffer.at(y);
        const bool wrapped = buffer.wrapsFromPreviousLine(y);
        const int padding = wrapped ? buffer.wrapIndentWidth(y) : std::min(buffer.wrapGapBefore(y), buffer.wrapIndentWidth(y));
        const QString joint = wrapped ? QString(buffer.wrapGapBefore(y), QChar::Space) : QString(QChar::LineFeed);
        text.append(joint);
        format.insert(format.end(), joint.size(), format.empty() ? TChar() : format.back());
        const TLineFormat& segmentFormat = buffer.buffer.at(y);
        const int from = std::min(padding, static_cast<int>(segment.size()));
        text.append(QStringView(segment).mid(from));
        const auto formatFrom = segmentFormat.begin() + std::min<qsizetype>(from, segmentFormat.size());
        format.insert(format.end(), formatFrom, segmentFormat.end());
        format.resize(text.size(), format.empty() ? TChar() : format.back());
    }
    return {text, buffer.promptBuffer.at(first), textRuns(text, TLineFormat(std::move(format)), mpHost->mFgColor, mpHost->mBgColor), arrivalTime(buffer, first)};
}

bool THeadlessSession::lineShownWanted() const
{
    static const QMetaMethod lineShownSignal = QMetaMethod::fromSignal(&THeadlessSession::lineShown);
    return isSignalConnected(lineShownSignal);
}

// Fixed for a line and all its triggers do, so a receiver connected or let go in a handler gets none of it twice
bool THeadlessSession::lineShownTaken() const
{
    return mShownLinesOfPasses.isEmpty() ? lineShownWanted() : mLineShownLatched;
}

// Taken even when nothing is connected, so the console holds none of it
QList<THeadlessSession::ShownLine> THeadlessSession::takeTriggerWrites(const bool taken)
{
    QList<ShownLine> lines;
    if (!mpHost) {
        return lines;
    }
    const QList<TConsoleModel::TriggerWrite> writes = mpHost->mainConsoleModel().takeTriggerWritesForLineShown();
    if (writes.isEmpty() || !taken) {
        return lines;
    }
    const TBuffer& buffer = mpHost->mainConsoleModel().buffer;
    for (const TConsoleModel::TriggerWrite& write : writes) {
        lines.append({write.text, false, textRuns(write.text, TLineFormat(write.format), mpHost->mFgColor, mpHost->mBgColor), arrivalTime(buffer, write.line)});
    }
    return lines;
}

bool THeadlessSession::sendShownLines(const QList<ShownLine>& lines)
{
    const QPointer<THeadlessSession> self(this);
    const QString profile = mProfileName;
    for (const ShownLine& line : lines) {
        ++gSignalsBeingHandedOut;
        emit lineShown(profile, line.text, line.isPrompt, line.runs, line.arrivalMSecs);
        --gSignalsBeingHandedOut;
        if (!self) {
            return false;
        }
    }
    return true;
}

// A handler of outputLine, lineShown, mainConsoleCleared, localText, errorConsoleText, debugText, hiddenInputWanted or a ...Requested signal runs in the middle of a write,
// which a line it fed would cut into, and a load that runs the event loop has yet to start the profile's scripts
bool THeadlessSession::refused(const bool duringLoad)
{
    if (mShuttingDown) {
        qWarning() << "THeadlessSession: called while shutdown() closes the profiles, which is refused";
        //: Error from a frontend with no window that was called while it was closing its profiles
        mErrorString = tr("Refused while the profiles close.");
        return true;
    }
    if (mpPool && gSignalsBeingHandedOut) {
        qWarning() << "THeadlessSession: called from an outputLine, lineShown, mainConsoleCleared, localText, errorConsoleText, debugText, hiddenInputWanted or ...Requested handler, which is "
                      "refused; queue the call instead";
        //: Error from a frontend with no window that called it back from a handler of the game's lines, as sent or as shown, of the main console being cleared, of the text the profile printed, of its error console, of the Central Debug Console, of the game hiding what the player types, or of a request a script made, such as a notification. "...Requested" stands for the names of those signals, which all end in Requested, and is not translated.
        mErrorString = tr("Refused from a handler of outputLine, lineShown, mainConsoleCleared, localText, errorConsoleText, debugText, hiddenInputWanted or a ...Requested signal; queue the call "
                          "instead.");
        return true;
    }
    if (mRemovingStalePasswords || (!duringLoad && mpPool && mpPool->loadingProfile())) {
        qWarning() << "THeadlessSession: called while a profile loads, which is refused; call it once that has returned";
        //: Error from a frontend with no window that was called while a profile was still loading
        mErrorString = tr("Refused while a profile loads; call it once that has returned.");
        return true;
    }
    return false;
}

bool THeadlessSession::usable(const bool duringLoad)
{
    if (!mpHost) {
        //: Error from a frontend with no window that has no open profile to send to
        mErrorString = tr("No profile is open.");
        return false;
    }
    if (refused(duringLoad)) {
        return false;
    }
    if (mLoadFailed) {
        //: Error from a frontend with no window that was given a command, Lua or a package for a profile whose save could not be loaded
        mErrorString = tr("The profile's save could not be loaded, so it takes no commands; close it.");
        return false;
    }
    // Refused from the start of a close, while sysExitEvent and the save still run with the connection up, so
    // nothing reaches the game after sysExitEvent; sooner than the GUI's command line, which takes typing until
    // the save is done
    if (mpHost->closeStarted()) {
        //: Error from a frontend with no window that was given a command, Lua or a package for a profile that has begun to close
        mErrorString = tr("The profile is closing, so it takes no more commands.");
        return false;
    }
    return true;
}
