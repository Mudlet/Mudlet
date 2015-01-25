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


#include "Profiles.h"


#include "MainWindow.h"

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"


bool Profiles::remove( const QString & id)
{
    QMutexLocker locker(& lock);

    qDebug() << "---> trying to delete host <"<<id.toLatin1().data()<<"> from host pool.";
    // make sure this is really a new host
    if( ! pool.contains( id ) )
    {
        qDebug() << "[CRITICAL ERROR]: cannot delete host:"<<id.toLatin1().data()<<" it is not a member of host pool.";
        return false;
    }
    else
    {
        qDebug() << "[OK] Host deleted removing pool entry ...";
        int ret = pool.remove( id );
        qDebug() << "[OK] deleted Host:"<<id.toLatin1().data()<<" ret="<<ret;

    }
    return true;
}

void Profiles::open(const QString & id) {

    add(id); // nop if exists

    auto p = get(id);

    MainWindow::self()->constructConsole(p);

}

bool Profiles::rename( const QString & id )
{
    QMutexLocker locker(& lock);

    // make sure this is really a new host
    if( pool.find( id ) == pool.end() )
    {
        return false;
    }
    else
    {
        QSharedPointer<Profile> host = pool[id];
        pool.remove( id );
        host->setId(id);
        pool.insert(id, host);
    }

    return true;

}

bool Profiles::add( const QString & id )
{
    QMutexLocker locker(&lock);

    // make sure this is really a new host
    if( pool.find( id ) != pool.end() )
    {
        return false;
    }

    if( id.size() < 1 ) {
        return false;
    }

    QSharedPointer<Profile> profile( new Profile( id ) );

    pool.insert( id, profile );
    active = pool.begin().value().data();
    return true;
}

QStringList Profiles::getStringList()
{
    QMutexLocker locker(& lock);

    QStringList strlist;
    QList<QString> hostList = pool.keys();
    if( hostList.size() > 0 )
        strlist << hostList;
    return strlist;
}

QList<QString> Profiles::getList()
{
    QMutexLocker locker(& lock);

    return pool.keys();
}

Profile * Profiles::get( const QString &id )
{
    QMutexLocker locker(& lock);
    if( pool.find( id ) != pool.end() )
    {
        return pool[id].data();
    }
    else
    {
        return NULL;
    }
}

Profile * Profiles::getFirst()
{
    QMutexLocker locker(& lock);
    return pool.begin().value().data();
}
