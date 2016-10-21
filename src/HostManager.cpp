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

bool HostManager::renameHost( QString hostname )
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

    int id = createNewHostID();
    QSharedPointer<Host> pNewHost( new Host( portnumber, hostname, login, pass, id ) );

    mHostPool.insert( hostname, pNewHost );
    mpActiveHost = mHostPool.begin().value().data();
    return true;
}

int HostManager::createNewHostID()
{
    return (mHostPool.size() + 1);
}

QStringList HostManager::getHostList()
{
    QMutexLocker locker(& mPoolLock);

    QStringList strlist;
    const QList<QString> hostList = mHostPool.keys();
    if( ! hostList.isEmpty() ) {
        strlist << hostList;
    }

    return strlist;
}

QList<QString> HostManager::getHostNameList()
{
    QMutexLocker locker(& mPoolLock);

    return mHostPool.keys();
}

void HostManager::postIrcMessage( QString a, QString b, QString c )
{
    QMutexLocker locker(& mPoolLock);

    QList<QSharedPointer<Host> > hostList = mHostPool.values();
    for( int i=0; i<hostList.size(); i++ )
    {
        hostList[i]->postIrcMessage( a, b, c );
    }
}

// The slightly convoluted way we step through the list of hosts is so that we
// send out the events to the other hosts in a predictable and consistant order
// and so that no one host gets an unfair advantage when emitting events. The
// sending profile host gets the event LAST so it knows that all other profiles
// have received it before it does.
void HostManager::postInterHostEvent( const Host * pHost, const TEvent & event )
{
    if( ! pHost ) {
        return;
    }

    QMutexLocker locker(& mPoolLock);

    QList<QSharedPointer<Host> > hostList = mHostPool.values();
    int i = 0;
    QList<int> beforeSendingHost;
    int sendingHost = -1;
    QList<int> afterSendingHost;
    while( i < hostList.size() ) {
        if( hostList.at(i) && hostList.at(i) != pHost ) {
            beforeSendingHost.append( i++ );
        }
        else if( hostList.at(i) && hostList.at(i) == pHost ) {
            sendingHost = i++;
            break;
        }
        else {
            i++;
        }
    }
    while( i < hostList.size() ) {
        if( hostList.at(i) && hostList.at(i) != pHost ) {
            afterSendingHost.append( i );
        }
        i++;
    }

    QList<int> allValidHosts;
    allValidHosts = afterSendingHost;
    allValidHosts.append( beforeSendingHost );
    if( sendingHost >= 0 ) {
        allValidHosts.append( sendingHost );
    }
    else {
        qWarning( "HostManager::postInterHostEvent(...) Warning: That's weird, the source of the event does not seem to exist any more!" );
    }

    for( int j = 0, total = allValidHosts.size(); j < total; ++j ) {
        hostList.at( allValidHosts.at( j ) )->raiseEvent( event );
    }
}

Host * HostManager::getHost( QString hostname )
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

Host * HostManager::getHostFromHostID( int id )
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

Host * HostManager::getFirstHost()
{
    QMutexLocker locker(& mPoolLock);
    return mHostPool.begin().value().data();
}
