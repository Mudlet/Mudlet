#ifndef MUDLET_TDEBUG_H
#define MUDLET_TDEBUG_H

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


#include "pre_guard.h"
#include <QColor>
#include <QList>
#include <QMap>
#include <QString>
#include "post_guard.h"


class TDebug
{
    QString msg;
    QColor fgColor;
    QColor bgColor;

public:
    TDebug(QColor, QColor);
    ~TDebug();
    TDebug& operator>>(int);
    TDebug& operator<<(const QString& t);
    TDebug& operator<<(const int& t);
    TDebug& operator<<(const QMap<QString, QString>& map);
    TDebug& operator<<(const QMap<QString, int>& map);
    TDebug& operator<<(const QMap<int, QString>& map);
    TDebug& operator<<(const QMap<int, int>& map);
    TDebug& operator<<(const QList<QString>& list);
    TDebug& operator<<(const QList<int>& list);

private:
    TDebug() = default;
};

#endif // MUDLET_TDEBUG_H
