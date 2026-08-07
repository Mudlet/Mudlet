/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "UntrustedText.h"

#include "utils.h"

bool UntrustedText::unsafeCharacter(char32_t codePoint)
{
    // C0 and C1 controls, which include CR, LF and NEL.
    if (codePoint <= 0x1F || (codePoint >= 0x7F && codePoint <= 0x9F)) {
        return true;
    }
    // Arabic letter mark, zero-width space through RLM, the explicit bidi
    // embedding and override controls, and the bidi isolates.
    if (codePoint == 0x061C || (codePoint >= 0x200B && codePoint <= 0x200F) || (codePoint >= 0x202A && codePoint <= 0x202E) || (codePoint >= 0x2066 && codePoint <= 0x2069)) {
        return true;
    }
    // Line and paragraph separators, which Qt renders as a real line break.
    if (codePoint >= 0x2028 && codePoint <= 0x2029) {
        return true;
    }
    // Word joiner and byte order mark, both invisible padding.
    return codePoint == 0x2060 || codePoint == 0xFEFF;
}

QString UntrustedText::forDisplay(const QString& text)
{
    const QList<uint> codePoints = text.toUcs4();

    QString result;
    result.reserve(text.size());
    for (const uint codePoint : codePoints) {
        if (unsafeCharacter(static_cast<char32_t>(codePoint))) {
            // Uppercase the hex digits only - uppercasing the whole fragment
            // would turn the \u prefix into \U.
            result += qsl("\\u{%1}").arg(QString::number(codePoint, 16).toUpper());
        } else {
            result += QChar::fromUcs4(codePoint);
        }
    }

    return result;
}
