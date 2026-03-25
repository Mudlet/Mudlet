#ifndef MUDLET_HOSTMANAGER_H
#define MUDLET_HOSTMANAGER_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018, 2025 by Stephen Lyons                       *
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


#include <QMap>
#include <QString>
#include <QSharedPointer>

class Host;
class TEvent;
typedef QMap<QString, QSharedPointer<Host>> HostMap;

class HostManager
{
    class Iter
    {
    public:
        Iter(HostManager* mgr, bool top);
        bool operator!= (const Iter& other) const;
        bool operator== (const Iter& other) const;
        Iter& operator++();
        QSharedPointer<Host> operator*();

    private:
        HostMap::iterator it;
    };


public:
    HostManager() = default;

    Host* getHost(const QString& hostname);
    bool addHost(const QString& name, const QString& port, const QString& login, const QString& pass);
    int getHostCount();
    void deleteHost(const QString&);
    void postIrcMessage(const QString&, const QString&, const QString&);
    void postInterHostEvent(const Host*, const TEvent&, const bool = false);
    void changeAllHostColour(const Host*);
    Iter begin() { return Iter(this, true); }
    Iter end() { return Iter(this, false); }
    bool hostLoaded(const QString& hostname) const;

private:
    HostMap mHostPool;
};

#endif // MUDLET_HOSTMANAGER_H
