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

#ifndef MUDLET_TSTRINGUTILS_H
#define MUDLET_TSTRINGUTILS_H

#include "pre_guard.h"
#include <QString>
#include <QStringList>
#include "post_guard.h"
#include <functional>

#define CHAR_NEW_LINE '\n'
#define CHAR_CARRIAGE_RETURN '\r'
#define CHAR_END_OF_FILE '\xff'
#define CHAR_END_OF_TEXT '\004'
#define CHAR_ESC '\033'

#define CHAR_IS_COMMIT_CHAR(ch) ((ch) == CHAR_NEW_LINE || (ch) == CHAR_CARRIAGE_RETURN || (ch) == CHAR_END_OF_FILE || (ch) == CHAR_END_OF_TEXT)


class TStringUtils
{
public:
    static bool isQuote(QChar ch);
    static bool isOneOf(QChar ch, const QString &chars);

    static QStringRef trimmedRef(const QStringRef& ref);
    static QStringRef stripRef(const QString& str, QChar start, QChar end);

    static QStringRef unquoteRef(const QStringRef& ref);
    static bool isBetween(const QString& str, char first, char last);
    static bool isBetween(const QStringRef& str, char first, char last);
    static QStringRef trimmedRef(const QString& str);
    static bool isQuoted(const QStringRef& ref);

    static void apply(QStringList& strList, const std::function<void(QString&)>& func)
    {
        for (auto& ptr : strList) {
            func(ptr);
        }
    }
};

#endif //MUDLET_TSTRINGUTILS_H
