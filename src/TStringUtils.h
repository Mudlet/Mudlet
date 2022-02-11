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
#include "utils.h"

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

    static QStringView strip(QStringView str, QChar start, QChar end);
    static QStringView unquote(QStringView ref);

    static bool isBetween(QStringView str, char first, char last);
    static bool isQuoted(QStringView ref);

    static void apply(QStringList& strList, const std::function<void(QString&)>& func)
    {
        for (auto& ptr : strList) {
            func(ptr);
        }
    }

    static void listAddItem(QString &list, const QString &item)
    {
        if (!list.isEmpty()) {
            list.append("|");
        }
        list.append(item);
    }

    static void listRemoveItem(QString &list, const QString &item)
    {
        int start = list.indexOf(item);

        if (start != -1) {
            if (start == 0) { // in the start of string
                if (item.size() == list.size() || list[item.size()] == '|') {
                    list.remove(start, std::min(item.size() + 1, list.size()));
                }
            } else if (list[start - 1] == '|') { // in the middle/end of string
                list.remove(start - 1, item.size() + 1);
            }
        }
    }
};

#endif //MUDLET_TSTRINGUTILS_H
