/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018 by Stephen Lyons - slysven@virginmedia.com   *
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

/*
 * Revised to allow concurrent read access to mHostPool but as some methods
 * were not used they have been commented out - if they are needed again they
 * will need to be rewritten to use the (QReadWriteLock) mPoolReadWriteLock
 * that is now used instead of the previous (QMutex) mPoolLock which was used
 * with a QMutexLocker - the latter is not suitable for a QReadWriteLock I
 * think - SlySven Dec 2016
 */

#include "HostManager.h"


#include "mudlet.h"


bool HostManager::deleteHost(const QString& hostname)
{
    mPoolReadWriteLock.lockForWrite(); // Will block until gets lock

    // make sure this is really a new host
    if (!mHostPool.contains(hostname)) {
        mPoolReadWriteLock.unlock();
        qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") ERROR: it is not a member of host pool... releasing lock and aborting, returning false!";
        return false;
    } else {
        int ret = mHostPool.remove(hostname);
        mPoolReadWriteLock.unlock();
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

    mPoolReadWriteLock.lockForWrite(); // Will block until gets lock
    // make sure this is really a new host
    if (mHostPool.contains(hostname)) {
        mPoolReadWriteLock.unlock();
        return false;
    }


    // Was an ONLY use of a createNewHostID() method here but that extra
    // function call was unnecessary and wastes time while we are locking access
    // to the host pool
    int id = mHostPool.size() + 1;
    QSharedPointer<Host> pNewHost(new Host(portnumber, hostname, login, pass, id));

    if (Q_UNLIKELY(!pNewHost)) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: failed to create new Host for the host pool... releasing lock and aborting, returning false!";
        mPoolReadWriteLock.unlock();
        return false;
    }

    mHostPool.insert(hostname, pNewHost);
    mPoolReadWriteLock.unlock();
    return true;
}

QStringList HostManager::getHostList()
{
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place

    QStringList strlist;
    const QList<QString> hostList = mHostPool.keys(); // As this is a QMap the list will be sorted alphabetically
    mPoolReadWriteLock.unlock();
    if (!hostList.isEmpty()) {
        strlist << hostList;
    }

    return strlist;
}

int HostManager::getHostCount()
{
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place
    // This assumes that there will not be nullptr values for destroyed Host
    // instances:
    const unsigned int total = mHostPool.count();
    mPoolReadWriteLock.unlock();
    return total;
}

void HostManager::postIrcMessage(const QString& a, const QString& b, const QString& c)
{
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place

    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    mPoolReadWriteLock.unlock();
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

    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place
    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    mPoolReadWriteLock.unlock();

    int i = 0;
    QList<int> beforeSendingHost;
    int sendingHost = -1;
    QList<int> afterSendingHost;
    while (i < hostList.size()) {
        if (hostList.at(i) && hostList.at(i) != pHost) {
            beforeSendingHost.append(i++);
        } else if (hostList.at(i) && hostList.at(i) == pHost) {
            sendingHost = i++;
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

    for (int validHost : allValidHosts) {
        hostList.at(validHost)->raiseEvent(event);
    }
}

Host* HostManager::getHost(const QString& hostname)
{
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place
    Host* pHost = mHostPool.value(hostname).data();
    mPoolReadWriteLock.unlock();

    return pHost;
}
