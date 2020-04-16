#ifndef MUDLET_TENCODINGTABLE_H
#define MUDLET_TENCODINGTABLE_H
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

#include <QApplication>
#include <QMap>
#include <QString>
#include <QPair>
#include <QVector>
#include <QChar>

// a map of computer-friendly encoding names as keys,
// values are a pair of human-friendly name + encoding data
class TEncodingTable {
    // need to use tr() on encoding names in csmEncodingTable
Q_DECLARE_TR_FUNCTIONS(TEncodingTable)

    static const QMap<QString, QPair<QString, QVector<QChar>>> csmEncodings;

    const QMap<QString, QPair<QString, QVector<QChar>>>& encodingMapping;

    inline static const QVector<QChar> emptyLookupTable = {};

public:
    static const TEncodingTable defaultInstance;

    explicit TEncodingTable(const QMap<QString, QPair<QString, QVector<QChar>>>& encodings)
            : encodingMapping(encodings)
    {
    }

    QList<QString> getEncodingNames() const;

    QList<QString> getFriendlyNames() const;

    const QString& getEncodingByFriendlyName(const QString& encoding) const;

    const QVector<QChar>& getLookupTable(const QString& encoding) const;
};

#endif //MUDLET_TENCODINGTABLE_H
