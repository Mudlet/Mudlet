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


#include "HostPool.h"


#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"


bool HostPool::deleteHost(QString hostname)
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

bool HostPool::renameHost(QString hostname)
{
    QMutexLocker locker(& mPoolLock);

    // make sure this is really a new host
    if( mHostPool.find( hostname ) == mHostPool.end() )
    {
        return false;
    }
    else
    {
        //Host * pNewHost = getHost( hostname ); // see why it doesn't work
        QSharedPointer<Host> pNewHost = mHostPool[hostname];
        mHostPool.remove( hostname );
        mHostPool.insert(pNewHost->getName(), pNewHost);
    }

    return true;

}

bool HostPool::addNewHost( QString hostname, QString port, QString login, QString pass )
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

    int id = createNewHostID();
    QSharedPointer<Host> pNewHost( new Host( portnumber, hostname, login, pass, id ) );

    mHostPool.insert( hostname, pNewHost );
    return true;
}

int HostPool::createNewHostID()
{
    return (mHostPool.size() + 1);
}

QStringList HostPool::getHostList()
{
    QMutexLocker locker(& mPoolLock);

    QStringList strlist;
    QList<QString> hostList = mHostPool.keys();
    if( hostList.size() > 0 )
        strlist << hostList;
    return strlist;
}



QList<QString> HostPool::getHostNameList()
{
    QMutexLocker locker(& mPoolLock);

    return mHostPool.keys();
}

void HostPool::orderShutDown()
{
    QMutexLocker locker(& mPoolLock);

    QList<QSharedPointer<Host> > hostList = mHostPool.values();
    for( int i=0; i<hostList.size(); i++ )
    {
        hostList[i]->orderShutDown();
    }
}

void HostPool::postIrcMessage( QString a, QString b, QString c )
{
    QMutexLocker locker(& mPoolLock);

    QList<QSharedPointer<Host> > hostList = mHostPool.values();
    for( int i=0; i<hostList.size(); i++ )
    {
        hostList[i]->postIrcMessage( a, b, c );
    }
}

Host * HostPool::getHost( QString hostname )
{
    QMutexLocker locker(& mPoolLock);
    if( mHostPool.find( hostname ) != mHostPool.end() )
    {
        // host exists
        return mHostPool[hostname].data();
    }
    else
    {
        return 0;
    }
}

Host * HostPool::getHostFromHostID( int id )
{
    QMutexLocker locker( & mPoolLock );
    QMapIterator<QString, QSharedPointer<Host> > it(mHostPool);
    while( it.hasNext() )
    {
        it.next();
        if( it.value()->getHostID() == id )
        {
            return it.value().data();
        }
    }
    qDebug()<<"ERROR: didnt find requested id in hostpool";
    return 0;
}

Host * HostPool::getFirstHost()
{
    QMutexLocker locker(& mPoolLock);
    return mHostPool.begin().value().data();
}
