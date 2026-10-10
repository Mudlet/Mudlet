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

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <functional>
#include <string>
#include "utils.h"

#define CHAR_NEW_LINE '\n'
#define CHAR_CARRIAGE_RETURN '\r'
// cTelnet appends this after IAC GA / IAC EOR. It is not 0xFF: that byte is
// telnet IAC, and in Windows-1251 it is also the letter "я", so a data 0xFF
// (delivered as IAC IAC) must be decoded rather than end the line. The parser
// drops a NUL the server itself sends, so only this injected marker is one.
#define CHAR_PROMPT '\0'
#define CHAR_END_OF_TEXT '\003'
#define CHAR_END_OF_TRANSMISSION '\004'
#define CHAR_ESC '\033'

#define CHAR_IS_COMMIT_CHAR(ch) ((ch) == CHAR_NEW_LINE || (ch) == CHAR_CARRIAGE_RETURN || (ch) == CHAR_PROMPT || (ch) == CHAR_END_OF_TRANSMISSION)


class TStringUtils
{
public:
    static bool isQuote(QChar ch);
    static bool isOneOf(QChar inputCharacter, const QString& characterSet);

    // Decode raw MXP bytes into text using the active session encoding.
    // Shared by the raw tag/content path and the attribute parser so both
    // interpret non-ASCII bytes identically.
    static QString decodeBytes(const std::string& bytes, const QByteArray& encoding);
};

#endif //MUDLET_TSTRINGUTILS_H
