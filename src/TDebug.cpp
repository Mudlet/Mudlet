/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "TDebug.h"

#include "mudlet.h"
#include "TConsole.h"


TDebug::TDebug(QColor c, QColor d)
{
    fgColor = c;
    bgColor = d;
    msg = "";
}

TDebug & TDebug::operator>>( const int code)
{
    mudlet::mpDebugConsole->printDebug( fgColor, bgColor, msg );
    return *this;
}

TDebug::~TDebug()
{
}

TDebug & TDebug::operator<<( const QString & t )
{
    msg += t;
    return *this;
}

TDebug & TDebug::operator<<( const int & t )
{
    msg += QString::number(t);
    return *this;
}

TDebug & TDebug::operator<<( QString & t )
{
    msg += t;
    return *this;
}

TDebug & TDebug::operator<<( const QMap<QString, QString> &map )
{
    for( QMap<QString, QString>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
    {
        msg += "(";
        msg += it.key();
        msg += ", ";
        msg += it.value();
        msg += ")";
    }
    msg += "), ";
    return *this;
}

TDebug & TDebug::operator<<( const QMap<QString, int> &map )
{
    for( QMap<QString, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
    {
        msg += "(";
        msg += it.key();
        msg += ", ";
        msg += QString::number(it.value());
        msg += ")";
    }
    msg += "), ";
    return *this;
}

TDebug & TDebug::operator<<(const QMap<int, QString> &map )
{
    for( QMap<int, QString>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
    {
        msg += "(";
        msg += QString::number(it.key());
        msg += ", ";
        msg += it.value();
        msg += ")";
    }
    msg += "), ";
    return *this;
}

TDebug & TDebug::operator<<( const QMap<int, int> &map )
{
    for( QMap<int, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
    {
        msg += "(";
        msg += QString::number(it.key());
        msg += ", ";
        msg += QString::number(it.value());
        msg += ")";
    }
    msg += "), ";
    return *this;
}

TDebug & TDebug::operator<<( const QList<QString> &list )
{
    for( QList<QString>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        msg += (*it);
        msg += ", ";
    }
    msg += ")";
    return *this;
}

TDebug & TDebug::operator<<( const QList<int> &list )
{
    for( QList<int>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        msg += QString::number(*it);
        msg += ", ";
    }
    msg += "), ";
    return *this;
}
