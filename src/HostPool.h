/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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

#ifndef _HOST_POOL_H_
#define _HOST_POOL_H_

#include <QMap>
#include <QList>
#include <QMutex>
#include <QString>
#include "Host.h"

class mudlet;


class HostPool
{
    QMutex                mPoolLock;
    QMap<QString, Host *> mHostPool;

 public:

    Host *                getHost( QString hostname );
    Host *                getFirstHost();
    Host *                getNextHost( QString LastHost ); //get next host key by providing a LastHost
    QList<QString>        getHostNameList();
    QStringList           getHostList();
    bool                  addNewHost( QString hostname, QString port, QString login, QString pass );
    bool                  deleteHost( QString url );
    bool                  renameHost( QString url );
    bool                  serialize( QString path );
    void                  orderShutDown();
    Host *                getHostFromHostID( int id );
    void                  postIrcMessage( QString, QString, QString );

private:

    int createNewHostID();
};
#endif

