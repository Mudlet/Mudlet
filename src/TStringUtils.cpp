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
#include "TStringUtils.h"

QStringView TStringUtils::strip(QStringView str, QChar start, QChar end)
{
    int len = str.length();
    if (len > 1 && str.front() == start && str.back() == end) {
        return str.mid(1, len - 2);
    }

    return str;
}

bool TStringUtils::isQuote(QChar ch)
{
    return isOneOf(ch, QStringLiteral("\'\""));
}

bool TStringUtils::isOneOf(QChar ch, const QString& chars)
{
    for (const auto& o : chars) {
        if (o == ch) {
            return true;
        }
    }

    return false;
}

bool TStringUtils::isQuoted(QStringView ref)
{
    return TStringUtils::isQuote(ref.front()) && ref.front() == ref.back();
}

QStringView TStringUtils::unquote(QStringView ref)
{
    return isQuoted(ref) ? ref.mid(1, ref.size() - 2) : ref;
}

bool TStringUtils::isBetween(QStringView str, char first, char last)
{
    return str.front() == first && str.back() == last;
}
