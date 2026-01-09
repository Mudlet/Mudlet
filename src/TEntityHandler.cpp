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
bool TEntityHandler::handle(char c, bool resolveCustomEntities)
{
    const bool isLegalNamedEntityChar = isalnum(c) ||
                                        c == '#' || c == '.' || c == '-' ||
                                        c == '_' || c == '&' || c == ';';
    if (!mCurrentEntity.isEmpty() || c == '&') {
        mCurrentEntity.append(c);
        if (c == ';') {
            mResult = mpEntityResolver.getResolution(mCurrentEntity, resolveCustomEntities, &entityType);
            mIsResolved = true;
            mCurrentEntity.clear();
        } else if (!isLegalNamedEntityChar) {
            mResult = mCurrentEntity;
            mIsResolved = true;
            entityType = ENTITY_TYPE_UNKNOWN;
            mCurrentEntity.clear();
        } else {
            mIsResolved = false;
            entityType = ENTITY_TYPE_UNKNOWN;
        }
        return true;
    }
    return false;
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
QString TEntityHandler::getResultAndReset()
{
    reset();
    return mResult;
}
