/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018, 2020 by Stephen Lyons                       *
 *                                               - slysven@virginmedia.com *
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

#include "dlgMapper.h"
#include "mudlet.h"

bool HostManager::deleteHost(const QString& hostname)
{
    // make sure this is really a new host
    if (!mHostPool.contains(hostname)) {
        qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") ERROR: not a member of host pool... aborting!";
        return false;
    } else {
        int ret = mHostPool.remove(hostname);
        return ret;
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

    int id = mHostPool.size() + 1;
    QSharedPointer<Host> pNewHost(new Host(portnumber, hostname, login, pass, id));

    if (Q_UNLIKELY(!pNewHost)) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: failed to create new Host for the host pool... aborting!";
        return false;
    }

    mHostPool.insert(hostname, pNewHost);
    return true;
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

    for (int validHost : qAsConst(allValidHosts)) {
        hostList.at(validHost)->raiseEvent(event);
    }
}

void HostManager::changeAllHostColour(const Host* pHost)
{
    if (!pHost) {
        return;
    }
    //change all main and subconsoles color
    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    for (int i = 0; i < hostList.size(); i++) {
        hostList.at(i)->mpConsole->changeColors();
        // Mapper also needs a refresh of its colours
        auto mapper = hostList.at(i)->mpMap->mpMapper;
        if (mapper) {
            mapper->setPalette(QApplication::palette());
        }
        auto videoPlayer = hostList.at(i)->mpMedia->mpVideoPlayer;
        if (videoPlayer) {
            videoPlayer->setPalette(QApplication::palette());
        }
        QMutableMapIterator<QString, TConsole*> itSubConsole(hostList.at(i)->mpConsole->mSubConsoleMap);
        while (itSubConsole.hasNext()) {
            itSubConsole.next();
            itSubConsole.value()->changeColors();
        }
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

bool HostManager::Iter::operator== (const Iter& other)
{
    return it == other.it;
}

bool HostManager::Iter::operator!= (const Iter& other)
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

