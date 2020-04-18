/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TEntityHandler.h"

bool TEntityHandler::handle(std::string& localBuffer, size_t& localBufferPosition, size_t localBufferLength)
{
    char ch = localBuffer[localBufferPosition];

    if (ch != '&' && currentEntity.isEmpty()) // currentEntity is not empty while reading an entity
        return false; // do not handle

    do {
        currentEntity.append(ch);

        if (ch == ';') {
            QString resolved = entityResolver.getResolution(currentEntity);
            // only get the last character, current implementation of TBuffer loop doesn't support more chars
            localBuffer[localBufferPosition] = resolved.back().toLatin1();

            currentEntity.clear();
            return false;
        }

        if (currentEntity.length() > 7) { // sequence too long, ignore it
            localBufferPosition++; // discard char

            currentEntity.clear();
            return true; // return to loop start
        }

        ch = localBuffer[++localBufferPosition];
    } while (localBufferPosition < localBufferLength);

    return false;
}