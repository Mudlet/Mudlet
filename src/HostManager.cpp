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
#include "TMap.h"
#include "TSpeechBridge.h"
#include "discord.h"
#include "utils.h"

#include <QDebug>
#include <QSettings>

#include <optional>

namespace {
struct Mutes
{
    bool api = false;
    bool game = false;
};
// Those of the last pool to go, for the next: Mudlet.ini is read by the first pool alone
std::optional<Mutes> sMutesLeft;
} // namespace

HostManager::HostManager(const ConfigRoot configRoot)
: mpMedia(std::make_unique<MudletMedia>())
, mpFonts(std::make_unique<FontManager>())
, mpDiscord(std::make_unique<Discord>())
, mpSpeechBridge(std::make_unique<TSpeechBridge>())
{
    // Settling again would delete the settings store under whoever holds it
    if (configRoot == ConfigRoot::settledHere && !MudletApp::configRootSettled()) {
        MudletApp::settleConfigRoot();
        // Once per process: where the main window settles the root, its readLateSettings() reads them instead
        readSettings(*MudletApp::getQSettings());
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
    for (const auto& pHost : hostList()) {
        pHost->raiseExitEventIfOwed();
    }
    if (auto* pSpeechBridge = TSpeechBridge::instance()) {
        for (const auto& pHost : hostList()) {
            pSpeechBridge->profileClosing(pHost.data());
        }
    }
    // Drain the pool while self() still answers: ~Host() may reach the manager it is being removed from.
    mHostPool.clear();
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
    // The handlers may have deleted this profile and opened another under its name
    if (mHostPool.value(hostname) == pHost) {
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

Host* HostManager::openProfile(const QString& name, const bool connect)
{
    if (!addHost(name, QString(), QString(), QString())) {
        return nullptr;
    }
    Host* pHost = getHost(name);
    pHost->loadProfileSave(QString());
    pHost->mIsProfileLoadingSequence = true;
    pHost->startProfileScripts();
    pHost->loadMapAndConnect(connect);
    pHost->finishProfileLoad();
    return pHost;
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
