/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "LuaLiteral.h"

#include "utils.h"

QString LuaLiteral::quote(const QString& text)
{
    // Escalate the bracket level until the text can neither close the literal
    // nor, under Lua 5.1's deprecated-nesting rule, reopen it. The loop
    // terminates because a run of n '=' cannot occur in text shorter than n.
    QString equals;
    while (text.contains(qsl("]%1]").arg(equals)) || text.contains(qsl("[%1[").arg(equals))) {
        equals += QLatin1Char('=');
    }

    // Lua discards a newline immediately after the opening bracket, so the
    // added one costs nothing and lets text that itself starts with a newline
    // survive the round trip.
    return qsl("[%1[\n%2]%1]").arg(equals, text);
}
