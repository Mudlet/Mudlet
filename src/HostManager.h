#ifndef MUDLET_HOSTMANAGER_H
#define MUDLET_HOSTMANAGER_H

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


#include "Host.h"

// clang-format: off
#include "pre_guard.h"
// clang-format: on
#include <QList>
#include <QMap>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
// clang-format: off
#include "post_guard.h"
// clang-format: on


class TEvent;


class HostManager
{
public:

                       HostManager()
                       /* : mpActiveHost() - Not needed */ {}
    Host *             getHost( QString hostname );
    QStringList        getHostList();
// Not Used:    QList<QString>     getHostNameList();
// Not Used:    Host *             getFirstHost();
    bool               addHost( QString name, QString port, QString login, QString pass );
    bool               deleteHost( QString );
// Not Used:    bool               renameHost( QString );
// Not Used:    Host *             getHostFromHostID( int id );
    void               postIrcMessage(QString, QString, QString );
    void               postIrcStatusMessage(QString, QString, QString );
    void               postInterHostEvent( const Host *, const TEvent & );

private:
    QReadWriteLock      mPoolReadWriteLock; // Was QMutex, but we needed to allow concurrent read access
    QMap<QString, QSharedPointer<Host> > mHostPool;
// Not Used:    Host *             mpActiveHost;

};

#endif // MUDLET_HOSTMANAGER_H
