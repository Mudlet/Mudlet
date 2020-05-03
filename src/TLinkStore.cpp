/***************************************************************************
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

#include "TLinkStore.h"

int TLinkStore::addLinks(const QStringList& links, const QStringList& hints)
{
    if (++mLinkID > maxLinks) {
        mLinkID = 1;
    }
    mLinkStore[mLinkID] = links;
    mHintStore[mLinkID] = hints;

    return mLinkID;
}

QStringList TLinkStore::getCurrentLinks() const
{
    return mLinkStore[mLinkID];
}

void TLinkStore::setCurrentLinks(const QStringList& links)
{
    mLinkStore[mLinkID] = links;
}

QStringList& TLinkStore::getLinks(int id)
{
    return mLinkStore[id];
}

QStringList& TLinkStore::getHints(int id)
{
    return mHintStore[id];
}

int TLinkStore::getCurrentLinkID() const
{
    return mLinkID;
}
