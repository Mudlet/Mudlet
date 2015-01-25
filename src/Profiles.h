#pragma once

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


#include "Profile.h"

#include "pre_guard.h"
#include <QList>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QStringList>
#include "post_guard.h"


class Profiles
{
public:

                       Profiles() : active() {}
    Profile *          get( const QString & hostname );
    QStringList        getStringList();
    QList<QString>     getList();
    Profile *          getFirst();
    bool               add( const QString & );
    bool               remove( const QString & );
    bool               rename( const QString & );
    void               open( const QString & );

private:

    QMutex              lock;
    QMap<QString, QSharedPointer<Profile> > pool;
    Profile *              active;

};
