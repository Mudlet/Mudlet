/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2022-2023 by Stephen Lyons - slysven@virginmedia.com    *
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
#if !defined(LinkStore_Test)
#include "TBuffer.h"  // For Mudlet::HyperlinkStyling definition
#include "Host.h"
#endif

int TLinkStore::addLinks(const QStringList& links, const QStringList& hints, Host* pH, const QVector<int>& luaReference, const QString& expireName)
{
    if (++mLinkID > mMaxLinks) {
        mLinkID = 1;
    }

    // Used to unref lua objects in the registry to avoid memory leaks
    freeReference(pH, mReferenceStore.value(mLinkID, QVector<int>()));

    // Remove old expire mapping if it exists (when wrapping around)
    QString oldExpireName = mExpireStore.value(mLinkID);
    if (!oldExpireName.isEmpty()) {
        mExpireToLinks.remove(oldExpireName, mLinkID);
    }

    mLinkStore[mLinkID] = links;
    mHintStore[mLinkID] = hints;
    mReferenceStore[mLinkID] = luaReference;

    // Store expire name if provided
    if (!expireName.isEmpty()) {
        mExpireStore[mLinkID] = expireName;
        mExpireToLinks.insert(expireName, mLinkID);
    }

    return mLinkID;
}

void TLinkStore::freeReference(Host* pH, const QVector<int>& oldReference)
{
    if (!pH || oldReference.isEmpty()) {
        return;
    }

    for (int i = 0, total = oldReference.size(); i < total; ++i) {
        if (oldReference.value(i, 0)) {
#if !defined(LinkStore_Test)
            pH->mLuaInterpreter.freeLuaRegistryIndex(oldReference.at(i));
#endif
        }
    }
}

void TLinkStore::expireLinks(const QString& expireName, Host* pH)
{
    if (expireName.isEmpty()) {
        return;
    }

    // Get all link IDs with this expire name
    QList<int> linkIds = mExpireToLinks.values(expireName);

    for (int linkId : linkIds) {
        // Free Lua references
        freeReference(pH, mReferenceStore.value(linkId));

        // Remove from all stores
        mLinkStore.remove(linkId);
        mHintStore.remove(linkId);
        mReferenceStore.remove(linkId);
        mExpireStore.remove(linkId);
#if !defined(LinkStore_Test)
        mStylingStore.remove(linkId);
#endif
    }

    // Remove all mappings for this expire name
    mExpireToLinks.remove(expireName);
}

#if !defined(LinkStore_Test)
void TLinkStore::setStyling(int id, const Mudlet::HyperlinkStyling& styling)
{
    mStylingStore[id] = styling;
}

Mudlet::HyperlinkStyling TLinkStore::getStyling(int id) const
{
    return mStylingStore.value(id, Mudlet::HyperlinkStyling());
}
#endif
