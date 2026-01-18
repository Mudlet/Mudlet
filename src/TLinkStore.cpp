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
#include "utils.h"    // For qsl() macro
#endif

#include <QSet>

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

void TLinkStore::removeLinkById(int id, Host* pH)
{
    freeReference(pH, mReferenceStore.value(id));

#if !defined(LinkStore_Test)
    if (mStylingStore.contains(id)) {
        const Mudlet::HyperlinkStyling& styling = mStylingStore[id];
        if (styling.selection.hasSelectionSettings) {
            QPair<QString, QString> key = qMakePair(styling.selection.group, styling.selection.value);
            mSelectionGroupIndex.remove(key, id);
        }
    }
#endif

    mLinkStore.remove(id);
    mHintStore.remove(id);
    mReferenceStore.remove(id);

    QString expireName = mExpireStore.value(id);
    if (!expireName.isEmpty()) {
        mExpireToLinks.remove(expireName, id);
        mExpireStore.remove(id);
    }

#if !defined(LinkStore_Test)
    mStylingStore.remove(id);
#endif
}

void TLinkStore::expireLinks(const QString& expireName, Host* pH)
{
    if (expireName.isEmpty()) {
        return;
    }

    QList<int> linkIds = mExpireToLinks.values(expireName);

    for (int linkId : linkIds) {
        removeLinkById(linkId, pH);
    }
}

void TLinkStore::removeUnreferencedLinks(const QSet<int>& referencedIds, Host* pH)
{
    QList<int> idsToRemove;

    for (auto&& [id, links] : mLinkStore.asKeyValueRange()) {
        if (!referencedIds.contains(id)) {
            idsToRemove.append(id);
        }
    }

    for (int id : idsToRemove) {
        removeLinkById(id, pH);
    }
}

#if !defined(LinkStore_Test)
void TLinkStore::setStyling(int id, const Mudlet::HyperlinkStyling& styling)
{
    // Remove old selection group index entry if it exists
    if (mStylingStore.contains(id)) {
        const Mudlet::HyperlinkStyling& oldStyling = mStylingStore[id];
        if (oldStyling.selection.hasSelectionSettings) {
            QPair<QString, QString> oldKey = qMakePair(oldStyling.selection.group, oldStyling.selection.value);
            mSelectionGroupIndex.remove(oldKey, id);
        }
    }
    
    mStylingStore[id] = styling;
    
    // Add new selection group index entry if applicable
    if (styling.selection.hasSelectionSettings) {
        QPair<QString, QString> key = qMakePair(styling.selection.group, styling.selection.value);
        mSelectionGroupIndex.insert(key, id);
    }
}

Mudlet::HyperlinkStyling TLinkStore::getStyling(int id) const
{
    return mStylingStore.value(id, Mudlet::HyperlinkStyling());
}

bool TLinkStore::hasStyling(int id) const
{
    return mStylingStore.contains(id);
}

QList<int> TLinkStore::getLinkIdsByGroupValue(const QString& group, const QString& value) const
{
    QPair<QString, QString> key = qMakePair(group, value);
    return mSelectionGroupIndex.values(key);
}
#endif
