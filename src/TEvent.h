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


// clang-format: off
#include "pre_guard.h"
// clang-format: on
#include <QDebug>
#if QT_VERSION >= 0x050100
#include <QDebugStateSaver>
#endif
#include <QStringList>
#include <QList>
// clang-format: off
#include "post_guard.h"
// clang-format: on

#define ARGUMENT_TYPE_NUMBER 0
#define ARGUMENT_TYPE_STRING 1
#define ARGUMENT_TYPE_BOOLEAN 2
#define ARGUMENT_TYPE_NIL 3

class TEvent
{
public:
    QStringList mArgumentList;
    QList<int> mArgumentTypeList;
};

#ifndef QT_NO_DEBUG_STREAM
// Note "inline" is REQUIRED:
inline QDebug & operator<<( QDebug & debug, const TEvent & event )
{
#if QT_VERSION >= 0x050100
    QDebugStateSaver saver(debug);
#endif
    const int argCount = event.mArgumentList.count();
    const int typeCount = event.mArgumentTypeList.count();
    int i = 0;
    QString result( "TEvent(" );
    while( i < argCount && i < typeCount ) {
        if( Q_UNLIKELY( i >= typeCount ) ) {
            result.append( QStringLiteral( "[%1{missing}%2]" ).arg( i ).arg( event.mArgumentList.at( i ) ) );
        }
        else {
            if( Q_UNLIKELY( i >=argCount ) ) {
                switch( event.mArgumentTypeList.at( i ) ) {
                case ARGUMENT_TYPE_NUMBER:
                    result.append( QStringLiteral( "[%1{number}missing]" ).arg( i ) );
                    break;
                case ARGUMENT_TYPE_BOOLEAN:
                    result.append( QStringLiteral( "[%1{bool}missing]" ).arg( i ) );
                    break;
                case ARGUMENT_TYPE_NIL:
                    result.append( QStringLiteral( "[%1{nil}missing]" ).arg( i ) );
                    break;
                default:
                    result.append( QStringLiteral( "[%1{string}missing]" ).arg( i ) );
                }
            }
            else {
                switch( event.mArgumentTypeList.at( i ) ) {
                case ARGUMENT_TYPE_NUMBER:
                    result.append( QStringLiteral( "[%1{number}%2]" ).arg( i ).arg( event.mArgumentList.at( i ).toDouble() ) );
                    break;
                case ARGUMENT_TYPE_BOOLEAN:
                    result.append( QStringLiteral( "[%1{bool}%2]" ).arg( i ).arg( event.mArgumentList.at( i ).toInt() ? "true" : "false" ) );
                    break;
                case ARGUMENT_TYPE_NIL:
                    result.append( QStringLiteral( "[%1{nil}nil]" ).arg( i ) );
                    break;
                default:
                    result.append( QStringLiteral( "[%1{string}%2]" ).arg( i ).arg( event.mArgumentList.at( i ) ) );
                }
            }
            ++i;
        }
    }
    result.append( QStringLiteral( ")" ) );
    debug.nospace() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TEVENT_H
