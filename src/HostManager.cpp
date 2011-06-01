#ifndef _HOSTMANAGER_CPP_
#define _HOSTMANAGER_CPP_

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

#include <QDir>
#include <iostream>
#include <fstream>
#include "HostManager.h"
#include "Host.h"
#include "HostPool.h"

HostManager * HostManager::_self = 0;

HostManager * HostManager::self()
{
    if( ! _self )
    {
        _self = new HostManager;
        _self->init();
    }
    return _self;
}

void HostManager::init()
{
    mpActiveHost = 0;
}

Host * HostManager::getHost( QString hostname )
{
    return mHostPool.getHost( hostname );
}

void HostManager::postIrcMessage( QString a, QString b, QString c )
{
    mHostPool.postIrcMessage( a, b, c );
}

bool HostManager::addHost( QString url, QString port, QString login, QString pass )
{
    bool ret = mHostPool.addNewHost( url, port, login, pass );
    //FIXME nur provisorisch bis ich multi host support fertig habe
    mpActiveHost = getFirstHost();
    return ret;
}


bool HostManager::deleteHost( QString url )
{
    return mHostPool.deleteHost( url );
}

bool HostManager::renameHost( QString url )
{
    return mHostPool.renameHost( url );
}

bool HostManager::serialize()
{
    QString directory = QDir::homePath()+"/.config/mudlet/profiles/";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );
    }

    std::cout << "Mudlet is exiting now." << std::endl << "ordering all Hosts to shut down" << std::endl;

    mHostPool.orderShutDown();
    mHostPool.serialize( directory );

    std::cout << "mudlet: DONE" << std::endl << "Have a nice day :-)" << std::endl;
    return true;
}


#endif

