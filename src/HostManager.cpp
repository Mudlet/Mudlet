/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"


bool HostManager::deleteHost(QString hostname)
{
    qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") INFO: trying to delete host from host pool, getting write lock...";
    mPoolReadWriteLock.lockForWrite(); // Will block until gets lock

    qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") INFO: ...got write lock...";
    // make sure this is really a new host
    if (!mHostPool.contains(hostname)) {
        mPoolReadWriteLock.unlock();
        qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") ERROR: it is not a member of host pool... releasing lock and aborting, returning false!";
        return false;
    } else {
        int ret = mHostPool.remove(hostname);
        mPoolReadWriteLock.unlock();
        qDebug() << "HostManager::deleteHost(" << hostname.toUtf8().constData() << ") INFO: found" << ret << "times in host pool and all of them were removed... releasing lock and returning true!";
        return true;
    }
}

bool HostManager::addHost(QString hostname, QString port, QString login, QString pass)
{
    if (hostname.isEmpty()) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: an unnamed Host is not permitted, aborting and returning false!";
        return false;
    }

    int portnumber = 23;
    if (!port.isEmpty() && port.toInt() > 0 && port.toInt() < 65536) {
        portnumber = port.toInt();
    }

    qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") INFO: trying to add host to host pool, getting write lock...";
    mPoolReadWriteLock.lockForWrite(); // Will block until gets lock

    qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") INFO: ...got write lock...";
    // make sure this is really a new host
    if (mHostPool.contains(hostname)) {
        qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") ERROR: is already a member of host pool... releasing lock and aborting, returning false!";
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
    qDebug() << "HostManager::addHost(" << hostname.toUtf8().constData() << ") INFO: new Host created and added to host pool... releasing lock and returning true!";
    return true;
}

QStringList HostManager::getHostList()
{
    qDebug() << "HostManager::getHostList() INFO: trying to read host pool, getting shared read access...";
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place

    QStringList strlist;
    const QList<QString> hostList = mHostPool.keys(); // As this is a QMap the list will be sorted alphabetically
    mPoolReadWriteLock.unlock();
    if (!hostList.isEmpty()) {
        strlist << hostList;
    }

    qDebug() << "HostManager::getHostList() INFO: ...got read access, and returning" << hostList.count() << "Host name(s).";
    return strlist;
}

void HostManager::postIrcMessage(QString a, QString b, QString c)
{
    qDebug() << "HostManager::postIrcMessage(...) INFO: trying to read host pool, getting shared read access...";
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place

    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    mPoolReadWriteLock.unlock();
    qDebug() << "HostManager::postIrcMessage(...) INFO: ...got read access and sending IRC message to" << hostList.count() << "Hosts.";
    for (const auto& i : hostList) {
        if (i) {
            i->postIrcMessage(a, b, c);
        }
    }
}

// The slightly convoluted way we step through the list of hosts is so that we
// send out the events to the other hosts in a predictable and consistant order
// and so that no one host gets an unfair advantage when emitting events. The
// sending profile host does NOT get the event!
void HostManager::postInterHostEvent(const Host* pHost, const TEvent& event)
{
    if (!pHost) {
        return;
    }

    qDebug() << "HostManager::postInterHostEvent(...) INFO: trying to read host pool, getting shared read access...";
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place
    const QList<QSharedPointer<Host>> hostList = mHostPool.values();
    mPoolReadWriteLock.unlock();
    qDebug() << "HostManager::postInterHostEvent(...) INFO: ...got read access and sending Event to" << hostList.count() - 1 << "Hosts.";

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

Host* HostManager::getHost(QString hostname)
{
    qDebug() << "HostManager::getHost(" << hostname.toUtf8().constData() << ") INFO: trying to read host pool, getting shared read access...";
    mPoolReadWriteLock.lockForRead(); // Will block if a write lock is in place
    Host* pHost = mHostPool.value(hostname).data();
    mPoolReadWriteLock.unlock();
    if (pHost) {
        qDebug() << "HostManager::getHost(" << hostname.toUtf8().constData() << ") INFO: ...got read access and found this name in host pool, returning Host pointer.";
    } else {
        qDebug() << "HostManager::getHost(" << hostname.toUtf8().constData() << ") INFO: ...got read access and but did not find this name in host pool, returning Null pointer.";
    }

    return pHost;
}
