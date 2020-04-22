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

// returns true if the char is handled by the EntityHandler (i.e. it is part of an entity)
bool TEntityHandler::handle(char ch)
{
    if (ch == ';' && !mCurrentEntity.isEmpty()) { // END OF ENTITY
        mCurrentEntity.append(ch);

        QString resolved = mpEntityResolver.getResolution(mCurrentEntity);
        // we only get the last character, current implementation of TBuffer loop is based on one char at a time
        // TODO: it could be interesting to have a way to send longer sequences to the buffer
        mResult = resolved.back().toLatin1();

        mIsResolved = true;
        mCurrentEntity.clear();
        return true;
    } else if (ch == '&' || !mCurrentEntity.isEmpty()) { // START OR MIDDLE OF ENTITY
        mIsResolved = false;
        mCurrentEntity.append(ch);
        return true;
    } else if (mCurrentEntity.length() > 7) { // LONG ENTITY? MAYBE INVALID... IGNORE IT
        reset();
        return false;
    } else {
        return false;
    }
}
bool TEntityHandler::isEntityResolved() const
{
    return mIsResolved;
}

void TEntityHandler::reset()
{
    mCurrentEntity.clear();
    mIsResolved = false;
}
char TEntityHandler::getResultAndReset()
{
    reset();
    return mResult;
}
