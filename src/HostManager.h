#ifndef _HOSTMANAGER_H_
#define _HOSTMANAGER_H_

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


#include <QString>
#include <QStringList>
#include <string>
#include "HostPool.h"
#include "Host.h"


class HostManager
{
public:

    static             HostManager * self();
    Host *             getHost( QString hostname );
    Host *             getHost( std::string hostname );
    QStringList        getHostList() { return mHostPool.getHostList(); }
    QList<QString>     getHostNameList() { return mHostPool.getHostNameList(); }
    Host *             getFirstHost(){ return mHostPool.getFirstHost(); }
    Host *             getNextHost( QString LastHost ){ return mHostPool.getNextHost(LastHost); } //get next host key by providing a LastHost
    bool               addHost( QString name, QString port, QString login, QString pass );
    bool               deleteHost( QString );
    bool               renameHost( QString );
    Host *             getHostFromHostID( int id ){ return mHostPool.getHostFromHostID( id ); }
    bool               serialize();
    void               postIrcMessage(QString, QString, QString );

private:

                        HostManager(){;}
    void                init();


    static HostManager * _self;
    HostPool            mHostPool;
    QMutex              mLock;
    Host *              mpActiveHost;

};

#endif


