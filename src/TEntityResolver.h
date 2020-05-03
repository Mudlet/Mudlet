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

#ifndef MUDLET_MXPENTITYRESOLVER_H
#define MUDLET_MXPENTITYRESOLVER_H

#include "pre_guard.h"
#include <QHash>
#include <QString>
#include "post_guard.h"
#include <functional>

class TEntityResolver
{
    QHash<QString, QString> mEntititesMap;

public:
    static const QHash<QString, QString> scmStandardEntites;
    static const TEntityResolver scmDefaultResolver;


    inline bool registerEntity(const QString& entity, const QChar ch) { return registerEntity(entity, QString(ch)); }

    inline bool registerEntity(const QString& entity, const char ch) { return registerEntity(entity, QChar::fromLatin1(ch)); }

    bool registerEntity(const QString& entity, const QString& str);
    bool unregisterEntity(const QString& entity);

    QString getResolution(const QString& entityValue) const;

    static QString resolveCode(ushort val);
    static QString resolveCode(const QString& entityValue);
    static QString resolveCode(const QString& entityValue, int base);
    static QString interpolate(const QString& input, std::function<QString(const QString&)> resolver);

    QString interpolate(const QString& input) const;
};

#endif //MUDLET_MXPENTITYRESOLVER_H
