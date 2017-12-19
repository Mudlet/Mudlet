#ifndef MUDLET_TEVENT_H
#define MUDLET_TEVENT_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
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


#include "pre_guard.h"
#include <QDebug>
#include <QDebugStateSaver>
#include <QList>
#include <QStringBuilder>
#include <QStringList>
#include "post_guard.h"

#define ARGUMENT_TYPE_NUMBER 0
#define ARGUMENT_TYPE_STRING 1
#define ARGUMENT_TYPE_BOOLEAN 2
#define ARGUMENT_TYPE_NIL 3
#define ARGUMENT_TYPE_TABLE 4

class TEvent
{
public:
    QStringList mArgumentList;
    QList<int> mArgumentTypeList;
};

#ifndef QT_NO_DEBUG_STREAM
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const TEvent& event)
{
    QDebugStateSaver saver(debug);
    const int argCount = event.mArgumentList.count();
    const int typeCount = event.mArgumentTypeList.count();
    int i = 0;
    QString result = QLatin1String("TEvent(");
    while (i < argCount && i < typeCount) {
        if (Q_UNLIKELY(i >= typeCount)) {
            result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{missing}") % event.mArgumentList.at(i) % QLatin1String("]"));
        } else {
            if (Q_UNLIKELY(i >= argCount)) {
                switch (event.mArgumentTypeList.at(i)) {
                case ARGUMENT_TYPE_NUMBER:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{number}missing]"));
                    break;
                case ARGUMENT_TYPE_BOOLEAN:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{bool}missing]"));
                    break;
                case ARGUMENT_TYPE_NIL:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{nil}missing]"));
                    break;
                default:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{string}missing]"));
                }
            } else {
                switch (event.mArgumentTypeList.at(i)) {
                case ARGUMENT_TYPE_NUMBER:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{number}") % QString::number(event.mArgumentList.at(i).toDouble()) % QLatin1String("]"));
                    break;
                case ARGUMENT_TYPE_BOOLEAN:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{bool}") % (event.mArgumentList.at(i).toInt() ? QLatin1String("true") : QLatin1String("false"))
                                  % QLatin1String("]"));
                    break;
                case ARGUMENT_TYPE_NIL:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{nil}nil]"));
                    break;
                default:
                    result.append(QLatin1String("[") % QString::number(i) % QLatin1String("{string}") % event.mArgumentList.at(i) % QLatin1String("]"));
                }
            }
            ++i;
        }
    }
    result.append(QLatin1String(")"));
    debug.nospace() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TEVENT_H
