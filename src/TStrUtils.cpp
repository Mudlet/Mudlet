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
#include "TStrUtils.h"

QStringRef TStrUtils::trimmedRef(const QStringRef& ref)
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

QStringRef TStrUtils::trimmedRef(const QString& str)
{
    return trimmedRef(QStringRef(&str));
}

QStringRef TStrUtils::stripRef(const QString& str, QChar start, QChar end)
{
    int len = str.length();
    if (len > 1 && str.front() == start && str.back() == end) {
        return str.midRef(1, len - 2);
    }

    return QStringRef(&str);
}

bool TStrUtils::isQuote(QChar ch)
{
    return isOneOf(ch, "\'\"");
}

bool TStrUtils::isOneOf(QChar ch, const char* str)
{
    for (; *str; str++) {
        if (*str == ch.toLatin1()) {
            return true;
        }
    }

    return false;
}

bool TStrUtils::isQuoted(const QStringRef& ref)
{
    return TStrUtils::isQuote(ref.front()) && ref.front() == ref.back();
}

QStringRef TStrUtils::unquoteRef(const QStringRef& ref)
{
    return isQuoted(ref) ? ref.mid(1, ref.size() - 2) : ref;
}
bool TStrUtils::isBetween(const QString& str, char first, char last)
{
    return isBetween(QStringRef(&str), first, last);
}

bool TStrUtils::isBetween(const QStringRef& str, char first, char last)
{
    return str.front() == first && str.back() == last;
}
