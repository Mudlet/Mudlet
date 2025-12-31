#ifndef MUDLET_TLINKSTORE_H
#define MUDLET_TLINKSTORE_H

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

#include <QMap>
#include <QMultiHash>
#include <QStringList>
#include <QVector>

class Host;

// Forward declarations - HyperlinkStyling is defined in TBuffer.h
// but we need to refer to it here, so we use a namespace-level forward declaration
namespace Mudlet {
struct HyperlinkStyling;
}

// Keep together lists of links and hints associated
class TLinkStore {
    inline static const int scmMaxLinks = 20000;

public:
    // We don't use explicit for this one so that the default argument is used,
    // if a value is not provided:
    TLinkStore(int maxLinks = scmMaxLinks)
    : mMaxLinks(maxLinks)
    {}

    int addLinks(const QStringList& links, const QStringList& hints, Host* pH = nullptr, const QVector<int>& luaReference = QVector<int>(), const QString& expireName = QString());

    QStringList& getLinks(int id) { return mLinkStore[id]; }
    QStringList& getHints(int id) { return mHintStore[id]; }
    QStringList getLinksConst(int id) const { return mLinkStore.value(id); }
    QStringList getHintsConst(int id) const { return mHintStore.value(id); }
    QVector<int> getReference(int id) const { return mReferenceStore.value(id); }
    
    // EXPIRE tag support - manage link expiry by name
    void expireLinks(const QString& expireName, Host* pH = nullptr);
    QString getExpireName(int id) const { return mExpireStore.value(id); }

    int getCurrentLinkID() const { return mLinkID; }

    QStringList getCurrentLinks() const { return mLinkStore.value(mLinkID); }

#if !defined(LinkStore_Test)
    // OSC 8 hyperlink styling storage and retrieval
    void setStyling(int id, const Mudlet::HyperlinkStyling& styling);
    Mudlet::HyperlinkStyling getStyling(int id) const;
    bool hasStyling(int id) const;
    
    // Selection group index for efficient exclusive group updates
    // Returns list of link IDs that have the specified group and value
    QList<int> getLinkIdsByGroupValue(const QString& group, const QString& value) const;
#endif

private:
    void freeReference(Host* pH, const QVector<int>& luaReference);


    int mLinkID = 0;
    int mMaxLinks = scmMaxLinks;

    QMap<int, QStringList> mLinkStore;
    QMap<int, QStringList> mHintStore;
    QMap<int, QVector<int>> mReferenceStore;
#if !defined(LinkStore_Test)
    QMap<int, Mudlet::HyperlinkStyling> mStylingStore;
#endif
    QMap<int, QString> mExpireStore;
    QMultiHash<QString, int> mExpireToLinks;
    
    QMultiHash<QPair<QString, QString>, int> mSelectionGroupIndex;
};

#endif //MUDLET_TLINKSTORE_H
