/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018, 2020, 2024-2026 by Stephen Lyons            *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "HostManager.h"

#include "FontManager.h"
#include "Host.h"
#include "MudletApp.h"
#include "MudletMedia.h"
#include "MudletReplay.h"
#include "discord.h"
#include "TAppFrontend.h"
#include "TGameDetails.h"
#include "TMap.h"
#include "TSpeechBridge.h"
#include "utils.h"

#include <QDebug>
#include <QHash>
#include <QScopeGuard>
#include <QSettings>
#include <QTimer>

#include <optional>

using namespace std::chrono_literals;

namespace {
struct Mutes
{
    bool api = false;
    bool game = false;
};
// Those of the last pool to go, for the next: Mudlet.ini is read by the first pool alone
std::optional<Mutes> sMutesLeft;

// Process-wide, as a release can outlive the pool that began it
int sReleasesPending = 0;

void retryRelease(const QSharedPointer<Host>& pHost, const std::chrono::milliseconds delay)
{
    QTimer::singleShot(delay, [pHost]() {
        if (pHost->deletionMustWait()) {
            retryRelease(pHost, 50ms);
            return;
        }
        // Its hold goes with this lambda, which Qt destroys once it has run
        --sReleasesPending;
    });
}

// Deleting a profile closes its Lua state, so the last hold on one goes only once no script of its own runs
void releaseOnceUnwound(const QSharedPointer<Host>& pHost)
{
    ++sReleasesPending;
    retryRelease(pHost, 0ms);
}
} // namespace

bool HostManager::releasingProfile()
{
    return sReleasesPending > 0;
}

HostManager::HostManager(const ConfigRoot configRoot, const QString& configDir)
: mpMedia(std::make_unique<MudletMedia>())
, mpFonts(std::make_unique<FontManager>())
, mpReplay(std::make_unique<MudletReplay>())
, mpDiscord(std::make_unique<Discord>())
, mpSpeechBridge(std::make_unique<TSpeechBridge>())
{
    // Settling again would delete the settings store under whoever holds it
    if (configRoot == ConfigRoot::settledHere && !MudletApp::configRootSettled()) {
        if (configDir.isEmpty()) {
            MudletApp::settleConfigRoot();
        } else {
            MudletApp::chooseConfigRoot(configDir);
        }
        // Once per process: where the main window settles the root, its readEarlySettings() and readLateSettings()
        // read them instead
        const QSettings& settings = *MudletApp::getQSettings();
        readSettings(settings);
        MudletApp::readInterfaceLanguage(settings);
        // Not in readSettings(), as the main window keeps these itself
        if (!TAppFrontend::hasView()) {
            TAppFrontend* pView = TAppFrontend::instance();
            pView->setDrawUpperLowerLevels(settings.value(qsl("drawUpperLowerLevels"), true).toBool());
            pView->setShowTabConnectionIndicators(settings.value(qsl("showTabConnectionIndicators"), false).toBool());
            sSettingsReadWithoutWindow = true;
        }
    } else if (sMutesLeft && MudletMedia::self() == mpMedia.get()) {
        mpMedia->setApiMuted(sMutesLeft->api);
        mpMedia->setGameMuted(sMutesLeft->game);
    }
    // self() callers trust there is only one; a second would repoint the accessor, then null it on
    // destruction while the first is still in use.
    if (smpSelf) {
        qWarning() << "HostManager::HostManager() WARNING - a HostManager already exists, so self() keeps pointing at that one.";
        return;
    }
    smpSelf = this;
}

HostManager::~HostManager()
{
    mGoingDown = true;
    for (const auto& pHost : hostList()) {
        pHost->raiseExitEventIfOwed();
        // As in deleteHost(): the handlers may have deleted this profile, which closed it
        if (mHostPool.value(pHost->getName()) != pHost) {
            continue;
        }
        TAppFrontend::instance()->profileClosing(pHost.get());
    }
    if (auto* pSpeechBridge = TSpeechBridge::instance()) {
        for (const auto& pHost : hostList()) {
            pSpeechBridge->profileClosing(pHost.data());
        }
    }
    // Drain the pool while self() still answers: ~Host() may reach the manager it is being removed from.
    mHostPool.clear();
    // A second pool skips this while self()'s is open, whose saves take what this one's profiles left; one outliving it,
    // with no self() left to save for, warns of what is still held and drops it, rather than hold it for a later pool
    if (smpSelf == this || !smpSelf) {
        TAppFrontend::instance()->profilesClosed();
    }
    if (smpSelf == this) {
        smpSelf = nullptr;
    }
    // Only the one profiles use: that of a pool made while another lived is never read
    if (MudletMedia::self() == mpMedia.get()) {
        sMutesLeft = Mutes{mpMedia->apiMuted(), mpMedia->gameMuted()};
    }
}

void HostManager::readSettings(const QSettings& settings)
{
    mpMedia->setApiMuted(settings.value(qsl("enableMuteAPI"), false).toBool());
    mpMedia->setGameMuted(settings.value(qsl("enableMuteGame"), false).toBool());
    TMap::smShowMapAuditErrors = settings.value(qsl("reportMapIssuesToConsole"), false).toBool();
    MudletApp::setStorePasswordsSecurely(settings.value(qsl("storePasswordsSecurely"), true).toBool());
}

void HostManager::writeSettings(QSettings& settings) const
{
    settings.setValue(qsl("enableMuteAPI"), mpMedia->apiMuted());
    settings.setValue(qsl("enableMuteGame"), mpMedia->gameMuted());
    settings.setValue(qsl("reportMapIssuesToConsole"), TMap::smShowMapAuditErrors);
    settings.setValue(qsl("storePasswordsSecurely"), MudletApp::storingPasswordsSecurely());
}

void HostManager::deleteHost(const QString& hostname)
{
    // Held here too, as a sysExitEvent handler that pumps events can get this profile deleted under it
    const QSharedPointer<Host> pHost = mHostPool.value(hostname);
    if (!pHost) {
        qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") ERROR: not a member of host pool... aborting!";
        return;
    }

    pHost->raiseExitEventIfOwed();
    if (auto* pSpeechBridge = TSpeechBridge::instance()) {
        pSpeechBridge->profileClosing(pHost.data());
    }
    // The handlers may have deleted this profile, closing it, and opened another under its name
    if (mHostPool.value(hostname) == pHost) {
        TAppFrontend::instance()->profileClosing(pHost.get());
        // The Host is destroyed as pHost goes out of scope, once it is out of the pool
        mHostPool.remove(hostname);
    }
    // Its own bridge, which a second pool's profiles do not use, so emptying a second pool releases nothing
    if (mHostPool.isEmpty()) {
        mpSpeechBridge->allProfilesClosed();
    }
}

bool HostManager::addHost(const QString& hostname, const QString& port, const QString& login, const QString& pass)
{
    if (hostname.isEmpty()) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: an unnamed Host is not permitted, aborting and returning false!";
        return false;
    }
    if (mGoingDown) {
        qWarning() << "HostManager::addHost() - not adding" << hostname << "as the pool is being destroyed";
        return false;
    }

    int portnumber = 23;
    if (!port.isEmpty() && port.toInt() > 0 && port.toInt() < 65536) {
        portnumber = port.toInt();
    }

    // make sure this is really a new host
    if (mHostPool.contains(hostname)) {
        return false;
    }

    const int id = mHostPool.size() + 1;
    QSharedPointer<Host> const pNewHost(new Host(portnumber, hostname, login, pass, id));

    if (Q_UNLIKELY(!pNewHost)) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: failed to create new Host for the host pool... aborting!";
        return false;
    }

    mHostPool.insert(hostname, pNewHost);
    return true;
}

Host* HostManager::openProfile(const QString& name, const bool connect, const std::function<void(Host*)>& beforeLoad, const std::function<void(Host*)>& afterLoad)
{
    if (mGoingDown || !addHost(name, QString(), QString(), QString())) {
        return nullptr;
    }
    ++mProfileLoadsInProgress;
    const auto loadScope = qScopeGuard([this] {
        --mProfileLoadsInProgress;
    });
    Host* pHost = getHost(name);
    if (mProfileOpenedHook) {
        mProfileOpenedHook(pHost);
    }
    if (beforeLoad) {
        beforeLoad(pHost);
    }
    const bool firstLoad = pHost->loadProfileSave(QString());
    // Chosen as the GUI chooses them, from the address the load leaves, before a frontend's own goes over it
    const QStringList packages = firstLoad ? defaultPackages(pHost->getUrl().toLower(), name, pHost->mAcceptServerGUI && pHost->mEnableGMCP) : QStringList();
    if (afterLoad) {
        afterLoad(pHost);
    }
    pHost->mIsProfileLoadingSequence = true;
    pHost->startProfileScripts();
    for (const QString& package : packages) {
        pHost->installPackage(package, enums::PackageModuleType::Package);
    }
    pHost->loadMapAndConnect(connect);
    pHost->finishProfileLoad();
    return pHost;
}

// The packages the GUI installs into a profile on its first load, for its game
QStringList HostManager::defaultPackages(const QString& gameUrl, const QString& profileName, const bool serverGuiAccepted)
{
    QStringList packages;
    const QHash<QString, QStringList> defaultScripts = {
            // clang-format off
        // scripts to pre-install for a profile      games this applies to, * means all games
        {qsl(":/packages/run-lua-code/run-lua-code.mpackage"), {qsl("*")}},
        {qsl(":/packages/echo/echo.mpackage"), {qsl("*")}},
        {qsl(":/packages/deleteOldProfiles/deleteOldProfiles.mpackage"), {qsl("*")}},
        {qsl(":/packages/enable-accessibility/enable-accessibility.mpackage"), {qsl("*")}},
        {qsl(":/packages/mpkg/mpkg.mpackage"), {qsl("*")}},
        {qsl(":/packages/gui-drop/gui-drop.mpackage"), {qsl("*")}},
        {qsl(":/packages/CF-loader/CF-loader.mpackage"), {qsl("carrionfields.net")}},
        {qsl(":/packages/icesus-loader/icesus-loader.mpackage"), {qsl("icesus.org")}},
        {qsl(":/packages/mg-loader/mg-loader.mpackage"), {qsl("mg.mud.de"),
                                                          qsl("mud.morgengrauen.info"),
                                                          qsl("mg.morgengrauen.info"),
                                                          qsl("morgengrauen.info")}},
        {qsl(":/packages/run-tests/run-tests.mpackage"), {qsl("mudlet.org")}},
        {qsl(":/packages/StressinatorDisplayBench/StressinatorDisplayBench.mpackage"), {qsl("mudlet.org")}},
        // the IRE mapper is maintained upstream and published as an xml, so it
        // is the one preinstall that is not packaged - see update-3rdparty.yml
        {qsl(":/mudlet-mapper.xml"), {qsl("aetolia.com"),
                                      qsl("achaea.com"),
                                      qsl("lusternia.com"),
                                      qsl("imperian.com"),
                                      qsl("starmourn.com"),
                                      qsl("stickmud.com")}},
        {qsl(":/packages/MedBootstrap/MedBootstrap.mpackage"), {qsl("medievia.com")}}
            // clang-format on
    };

    // mpkg fetches the package listing as it loads and, when the repository carries a
    // newer mpkg than the bundled one, uninstalls itself at once and reinstalls two
    // seconds plus a download later. Each of those calls doCleanReset() if an editor is
    // open, which queues a clear of its tree widgets onto the next event loop turn and
    // frees every item a test is holding, and each announces itself in the main console.
    // Whether it lands mid-test is down to how fast the download is.
    //
    // generic_mapper below can self-update the same way, but its upstream is this repo's
    // own development branch, so bundled and remote move together; mpkg is published from
    // a separate repository on its own schedule, which is what leaves a released Mudlet
    // upgrading itself mid-test for days at a time.
    const bool skipSelfUpgradingPackage = qEnvironmentVariableIsSet("MUDLET_TEST_MODE");

    QHashIterator<QString, QStringList> i(defaultScripts);
    while (i.hasNext()) {
        i.next();
        if (skipSelfUpgradingPackage && i.key() == qsl(":/packages/mpkg/mpkg.mpackage")) {
            continue;
        }
        if (i.value().first() == QLatin1String("*") || i.value().contains(gameUrl)) {
            packages.append(i.key());
        }
    }

    if (!packages.contains(qsl(":/mudlet-mapper.xml"))) {
        packages.append(qsl(":/packages/generic_mapper/generic_mapper.mpackage"));
    }

    // A modest starter UI that adapts to whatever any game provides.
    // Games known to install their own full interface, by a bundled loader
    // above or by Client.GUI (flagged in TGameDetails), are skipped: the
    // starter UI would only fight it for the same screen space, and would
    // build and announce itself before a post-login Client.GUI package lands.
    // Other games that push a GUI via Client.GUI are handled at runtime
    // instead - the starter UI stands aside when one installs.
    // A Client.GUI package only arrives if the profile lets it in, and a
    // profile copied with its settings can already refuse it.
    const auto ownUi = TGameDetails::gameOwnUi(gameUrl);
    const bool ownUiArrives = ownUi == GameDetail::OwnUi::BundledLoader || (ownUi == GameDetail::OwnUi::ClientGui && serverGuiAccepted);
    if (!ownUiArrives) {
        packages.append(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"));
    }

    // Don't play tutorial for every connection to localhost. There are legit other reasons to connect there.
    if (profileName == qsl("Mudlet Tutorial") && gameUrl == qsl("localhost")) {
        packages.append(qsl(":/packages/mudlet-tutorial/mudlet-tutorial.mpackage"));
    }
    return packages;
}

bool HostManager::requestProfileClose(const QString& name)
{
    // Held, as the sysExitEvent handlers may take the profile out of the pool
    const QSharedPointer<Host> pHost = mHostPool.value(name);
    if (!pHost) {
        return false;
    }
    // As the GUI's: a save wait ends on its own, so the close is asked again once it has
    if (pHost->waitingForProfileSave()) {
        QTimer::singleShot(50ms, pHost.data(), [this, alive = std::weak_ptr<bool>(mAlive), name, pClosingHost = pHost.data()]() {
            if (!alive.expired() && getHost(name) == pClosingHost) {
                requestProfileClose(name);
            }
        });
        return true;
    }
    // As the GUI's tab close: a close from under the profile's own nested event loop is ignored
    if (pHost->getLuaInterpreter()->pumpingEvents()) {
        qWarning() << "HostManager: asked to close profile" << name << "while it is running a nested event loop, ignoring";
        return true;
    }
    if (!pHost->closeWithNoView()) {
        return true;
    }
    if (getHost(name) == pHost) {
        deleteHostOnceUnwound(pHost.data(), 0ms);
    } else {
        // A handler took it out of the pool, so this is the last hold on it
        releaseOnceUnwound(pHost);
    }
    return true;
}

// Deleting the profile closes its Lua state, so not while a script of its own is still running (as
// after its closeProfile()). Tied to the profile, so the timer goes with it.
void HostManager::deleteHostOnceUnwound(Host* pHost, const std::chrono::milliseconds delay)
{
    const QString name = pHost->getName();
    QTimer::singleShot(delay, pHost, [this, alive = std::weak_ptr<bool>(mAlive), pHost, name]() {
        // The handlers may have deleted this profile and opened another under its name
        if (alive.expired() || getHost(name) != pHost) {
            return;
        }
        if (pHost->deletionMustWait()) {
            deleteHostOnceUnwound(pHost, 50ms);
            return;
        }
        deleteHost(name);
    });
}

void HostManager::closeAllProfiles()
{
    if (mGoingDown) {
        return;
    }
    // Each pass marks every profile first, as the GUI's forceClose() does, so one a handler closes
    // meanwhile saves too; a handler may open a profile, which the next pass then closes
    bool metOpenProfile = true;
    QList<QSharedPointer<Host>> met;
    while (metOpenProfile) {
        metOpenProfile = false;
        const QList<QSharedPointer<Host>> hosts = hostList();
        for (const auto& pHost : hosts) {
            if (!met.contains(pHost)) {
                met.append(pHost);
            }
        }
        for (const auto& pHost : hosts) {
            if (!pHost->isClosingDown() && !pHost->isClosingForced()) {
                pHost->markClosingForced();
            }
        }
        for (const auto& pHost : hosts) {
            // One a handler has taken out of the pool meanwhile is no longer this pool's to close
            if (getHost(pHost->getName()) == pHost) {
                metOpenProfile |= pHost->closeWithNoView();
            }
        }
    }
    // As in requestProfileClose(): one a handler took out of the pool may be held here alone
    for (const auto& pHost : std::as_const(met)) {
        if (getHost(pHost->getName()) != pHost) {
            releaseOnceUnwound(pHost);
        }
    }
    met.clear();
    for (const auto& pHost : hostList()) {
        if (pHost->deletionMustWait()) {
            deleteHostOnceUnwound(pHost.data(), 0ms);
        } else {
            deleteHost(pHost->getName());
        }
    }
}

int HostManager::getHostCount()
{
    return mHostPool.count();
}

void HostManager::postIrcMessage(const QString& a, const QString& b, const QString& c)
{
    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    for (const auto& i : hostList) {
        if (i) {
            i->postIrcMessage(a, b, c);
        }
    }
}

// The slightly convoluted way we step through the list of hosts is so that we
// send out the events to the other hosts in a predictable and consistent order
// and so that no one host gets an unfair advantage when emitting events. The
// sending profile host does NOT get the event!
// Note: Optional forceGlobal allows passing a null pointer as pHost, and will raise
// an event for all profiles.
void HostManager::postInterHostEvent(const Host* pHost, const TEvent& event, const bool forceGlobal)
{
    if (!pHost && !forceGlobal) {
        return;
    }

    const QList<QSharedPointer<Host>> hostList = mHostPool.values();

    int i = 0;
    QList<int> beforeSendingHost;
    QList<int> afterSendingHost;
    while (i < hostList.size()) {
        if (hostList.at(i) && hostList.at(i) != pHost) {
            beforeSendingHost.append(i++);
        } else if (hostList.at(i) && hostList.at(i) == pHost) {
            break;
        } else {
            i++;
        }
    }
    while (i < hostList.size()) {
        if (hostList.at(i) && hostList.at(i) != pHost) {
            afterSendingHost.append(i);
        }
        i++;
    }

    QList<int> allValidHosts;
    allValidHosts = afterSendingHost;
    allValidHosts.append(beforeSendingHost);

    for (const int validHost : std::as_const(allValidHosts)) {
        hostList.at(validHost)->raiseEvent(event);
    }
}

void HostManager::changeAllHostColour(const Host* pHost)
{
    if (!pHost) {
        return;
    }
    for (const QSharedPointer<Host>& host : mHostPool.values()) {
        host->refreshColours();
    }
}

Host* HostManager::getHost(const QString& hostname)
{
    Host* pHost = mHostPool.value(hostname).data();
    return pHost;
}

HostManager::Iter::Iter(HostManager* manager, bool at_start)
{
    if (at_start) {
        it = manager->mHostPool.begin();
    } else {
        it = manager->mHostPool.end();
    }
}

bool HostManager::Iter::operator==(const Iter& other) const
{
    return it == other.it;
}

bool HostManager::Iter::operator!=(const Iter& other) const
{
    return it != other.it;
}

HostManager::Iter& HostManager::Iter::operator++()
{
    it++;
    return *this;
}

QSharedPointer<Host> HostManager::Iter::operator*()
{
    return *it;
}

bool HostManager::hostLoaded(const QString& hostname) const
{
    return mHostPool.contains(hostname);
}
