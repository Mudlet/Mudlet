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

#include "TEncodingHelper.h"


bool TStringUtils::isQuote(QChar ch)
{
    return isOneOf(ch, qsl("\'\""));
}

bool TStringUtils::isOneOf(QChar inputCharacter, const QString& characterSet)
{
    for (const auto& setCharacter : characterSet) {
        if (setCharacter == inputCharacter) {
            return true;
        }
    }

    return false;
}

QString TStringUtils::decodeBytes(const std::string& bytes, const QByteArray& encoding)
{
    // An unnegotiated (empty) session encoding is treated as UTF-8, matching the
    // historical default for MXP text and staying a safe pass-through for ASCII.
    if (encoding.isEmpty() || encoding == QByteArrayLiteral("UTF-8")) {
        return QString::fromStdString(bytes);
    }
    if (encoding == QByteArrayLiteral("ISO 8859-1")) {
        return QString::fromLatin1(bytes.c_str(), static_cast<int>(bytes.length()));
    }
    return TEncodingHelper::decode(QByteArray::fromRawData(bytes.c_str(), bytes.length()), encoding);
}
