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

#include "pre_guard.h"
#include <QTextCodec>
#include <QApplication>
#include <QChar>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVector>
#include "post_guard.h"

// a map of encoding names to encodings
class TEncodingTable
{
    static const QMap<QByteArray, QVector<QChar>> csmEncodings;
    inline static const QVector<QChar> csmEmptyLookupTable = {};

    const QMap<QByteArray, QVector<QChar>>& mEncodingMap;

public:
    static const TEncodingTable csmDefaultInstance;

    explicit TEncodingTable(const QMap<QByteArray, QVector<QChar>>& encodings) : mEncodingMap(encodings) {}

    const QMap<QByteArray, QVector<QChar>> getEncodings() const { return mEncodingMap; }
    QList<QByteArray> getEncodingNames() const;

    const QVector<QChar>& getLookupTable(const QByteArray& encoding) const;
};

#endif //MUDLET_TENCODINGTABLE_H
