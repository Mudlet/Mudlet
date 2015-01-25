/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "mudlet.h"

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"


bool HostManager::deleteHost( QString hostname )
{
    QMutexLocker locker(& mPoolLock);

    qDebug() << "---> trying to delete host <"<<hostname.toLatin1().data()<<"> from host pool.";
    // make sure this is really a new host
    if( ! mHostPool.contains( hostname ) )
    {
        qDebug() << "[CRITICAL ERROR]: cannot delete host:"<<hostname.toLatin1().data()<<" it is not a member of host pool.";
        return false;
    }
    else
    {
        qDebug() << "[OK] Host deleted removing pool entry ...";
        int ret = mHostPool.remove( hostname );
        qDebug() << "[OK] deleted Host:"<<hostname.toLatin1().data()<<" ret="<<ret;

    }
    return true;
}

bool HostManager::renameHost( QString id )
{
    QMutexLocker locker(& mPoolLock);

    // make sure this is really a new host
    if( mHostPool.find( id ) == mHostPool.end() )
    {
        return false;
    }
    else
    {
        QSharedPointer<Host> host = mHostPool[id];
        mHostPool.remove( id );
        host->setId(id);
        mHostPool.insert(id, host);
    }

    return true;

}

bool HostManager::addHost( QString hostname, QString port, QString login, QString pass )
{
    QMutexLocker locker(&mPoolLock);

    // make sure this is really a new host
    if( mHostPool.find( hostname ) != mHostPool.end() )
    {
        return false;
    }
    if( hostname.size() < 1 )
        return false;

    int portnumber = 23;
    if( port.size() >= 1 )
    {
        portnumber = port.toInt();
    }

    QSharedPointer<Host> pNewHost( new Host( portnumber, hostname, login, pass ) );

    mHostPool.insert( hostname, pNewHost );
    mpActiveHost = mHostPool.begin().value().data();
    return true;
}

QStringList HostManager::getHostList()
{
    QMutexLocker locker(& mPoolLock);

    QStringList strlist;
    QList<QString> hostList = mHostPool.keys();
    if( hostList.size() > 0 )
        strlist << hostList;
    return strlist;
}

QList<QString> HostManager::getHostNameList()
{
    QMutexLocker locker(& mPoolLock);

    return mHostPool.keys();
}

Host * HostManager::getHost( const QString &id )
{
    QMutexLocker locker(& mPoolLock);
    if( mHostPool.find( id ) != mHostPool.end() )
    {
        return mHostPool[id].data();
    }
    else
    {
        return NULL;
    }
}

Host * HostManager::getFirstHost()
{
    QMutexLocker locker(& mPoolLock);
    return mHostPool.begin().value().data();
}
