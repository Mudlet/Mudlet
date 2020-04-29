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

QStringRef TStringUtils::trimmedRef(const QStringRef& ref)
{
    int start = 0;
    int end = ref.length();

    while (start < end && ref[start].isSpace()) {
        start++;
    }

    while (end > start && ref[end - 1].isSpace()) {
        end--;
    }

    return QStringRef(ref.string(), ref.position() + start, end - start);
}

QStringRef TStringUtils::trimmedRef(const QString& str)
{
    return trimmedRef(QStringRef(&str));
}

QStringRef TStringUtils::stripRef(const QString& str, QChar start, QChar end)
{
    int len = str.length();
    if (len > 1 && str.front() == start && str.back() == end) {
        return str.midRef(1, len - 2);
    }

    return QStringRef(&str);
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

bool TStringUtils::isQuoted(const QStringRef& ref)
{
    return TStringUtils::isQuote(ref.front()) && ref.front() == ref.back();
}

QStringRef TStringUtils::unquoteRef(const QStringRef& ref)
{
    return isQuoted(ref) ? ref.mid(1, ref.size() - 2) : ref;
}
bool TStringUtils::isBetween(const QString& str, char first, char last)
{
    return isBetween(QStringRef(&str), first, last);
}

bool TStringUtils::isBetween(const QStringRef& str, char first, char last)
{
    return str.front() == first && str.back() == last;
}
