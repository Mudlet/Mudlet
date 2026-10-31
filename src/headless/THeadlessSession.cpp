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
#include "SecureStringUtils.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TGameDetails.h"
#include "TLuaInterpreter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QScopeGuard>
#include <QTimer>

#include <functional>
#include <utility>

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
    mUnsavedProfiles.clear();
    mLoadFailed = false;
    Host* pHost = mpPool->openProfile(
            mProfileName,
            options.connect,
            [this, run, &password](Host* pNewHost) {
                mpHost = pNewHost;
                pNewHost->givePass(password);
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
            },
            [&options, tls, profile = mProfileName](Host* pNewHost) {
                // As the dialog: stored details over the game's own, which the load just set, then what was given
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
    if (poolBusy(*mpPool)) {
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
    if (mRemovingStalePasswords || (mpPool && mpPool->loadingProfile())) {
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
