/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2022-2023, 2026 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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
#include "TBuffer.h" // For Mudlet::HyperlinkStyling definition
#include "Host.h"
#include "utils.h" // For qsl() macro

#include <QSet>

int TLinkStore::addLinks(const QStringList& links, const QStringList& hints, Host* pH, const QVector<int>& luaReference, const QString& expireName)
{
    if (++mLinkID > mMaxLinks) {
        mLinkID = 1;
        ++mLaps;
    }

    // Used to unref lua objects in the registry to avoid memory leaks
    freeReference(pH, mReferenceStore.value(mLinkID, QVector<int>()));

    // Remove old expire mapping if it exists (when wrapping around)
    QString oldExpireName = mExpireStore.value(mLinkID);
    if (!oldExpireName.isEmpty()) {
        mExpireToLinks.remove(oldExpireName, mLinkID);
    }
    mExpireStore.remove(mLinkID);

    // Remove old styling and its selection-group index entry too, so a
    // recycled id does not inherit the previous link's styling
    clearStyling(mLinkID);

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

qint64 TLinkStore::issueOf(const int id) const
{
    if (!contains(id)) {
        return 0;
    }
    // Those above the last id handed out were handed out on the lap before
    const qint64 lap = id <= mLinkID ? mLaps : mLaps - 1;
    return lap * mMaxLinks + id;
}

void TLinkStore::freeReference(Host* pH, const QVector<int>& oldReference)
{
    if (!pH || oldReference.isEmpty()) {
        return;
    }

    for (int i = 0, total = oldReference.size(); i < total; ++i) {
        if (oldReference.value(i, 0)) {
            pH->mLuaInterpreter.freeLuaRegistryIndex(oldReference.at(i));
        }
    }
}

void TLinkStore::removeLinkById(int id, Host* pH)
{
    freeReference(pH, mReferenceStore.value(id));

    if (mStylingStore.contains(id)) {
        const Mudlet::HyperlinkStyling& styling = mStylingStore[id];
        if (styling.selection.hasSelectionSettings) {
            QPair<QString, QString> key = qMakePair(styling.selection.group, styling.selection.value);
            mSelectionGroupIndex.remove(key, id);
        }
    }

    mLinkStore.remove(id);
    mHintStore.remove(id);
    mReferenceStore.remove(id);

    QString expireName = mExpireStore.value(id);
    if (!expireName.isEmpty()) {
        mExpireToLinks.remove(expireName, id);
        mExpireStore.remove(id);
    }

    mStylingStore.remove(id);
}

QString TLinkStore::tooltip(const int id) const
{
    const QStringList hints = mHintStore.value(id);
    return hints.size() > mLinkStore.value(id).size() ? hints.constFirst() : hints.join(QChar::LineFeed);
}

bool TLinkStore::opensMenu(const int id) const
{
    const qsizetype commands = mLinkStore.value(id).size();
    return commands > 1 || mHintStore.value(id).size() > commands;
}

QList<TLinkStore::MenuItem> TLinkStore::menuItems(const int id) const
{
    const QStringList commands = mLinkStore.value(id);
    const QStringList hints = mHintStore.value(id);
    const QVector<int> references = mReferenceStore.value(id);
    const qsizetype hintOffset = hints.size() > commands.size() ? 1 : 0;
    QList<MenuItem> items;
    for (qsizetype i = 0; i < commands.size(); ++i) {
        const bool doesSomething = !commands.at(i).isEmpty() || references.value(i, 0);
        // Too few hints is allowed for, as a script may have given them
        const bool hasHint = i + hintOffset < hints.size();
        MenuItem item;
        item.label = hasHint ? hints.at(i + hintOffset) : commands.at(i);
        item.enabled = doesSomething;
        item.separator = !doesSomething && hasHint && item.label.isEmpty();
        items.append(item);
    }
    return items;
}

QList<int> TLinkStore::expireLinks(const QString& expireName, Host* pH)
{
    if (expireName.isEmpty()) {
        return {};
    }

    const QList<int> linkIds = mExpireToLinks.values(expireName);

    for (const int linkId : linkIds) {
        removeLinkById(linkId, pH);
    }
    return linkIds;
}

QList<int> TLinkStore::removeUnreferencedLinks(const QSet<int>& referencedIds, Host* pH)
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
    return idsToRemove;
}

void TLinkStore::clearStyling(int id)
{
    if (!mStylingStore.contains(id)) {
        return;
    }

    const Mudlet::HyperlinkStyling& oldStyling = mStylingStore[id];
    if (oldStyling.selection.hasSelectionSettings) {
        const QPair<QString, QString> key = qMakePair(oldStyling.selection.group, oldStyling.selection.value);
        mSelectionGroupIndex.remove(key, id);
    }

    mStylingStore.remove(id);
}

void TLinkStore::setStyling(int id, const Mudlet::HyperlinkStyling& styling)
{
    clearStyling(id);

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
