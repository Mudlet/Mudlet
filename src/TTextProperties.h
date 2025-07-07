#ifndef MUDLET_TTEXTPROPERTIES_H
#define MUDLET_TTEXTPROPERTIES_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2018, 2020 by Stephen Lyons                       *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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

#include <string>
#include "widechar_width.h"

namespace graphemeInfo {

inline int getWidth(uint unicode, bool mWideAmbigousWidthGlyphs)
{
    // https://github.com/ridiculousfish/widecharwidth/issues/11
    if (unicode == 0x1F6E1 || unicode == 0x2318) {
        return 2;
    }

    // fix to make red heart width 2
    if (unicode == 0x2764) {
        return 2;
    }

    switch (widechar_wcwidth(unicode)) {
    case 1: // Draw as normal/narrow
        return 1;
    case 2: // Draw as wide
        return 2;
    case widechar_nonprint:
        return 0;
    case widechar_non_character:
        return 0;
    case widechar_combining:
        return 0;
    case widechar_ambiguous:
        // -3 = The character is East-Asian ambiguous width.
        return mWideAmbigousWidthGlyphs ? 2 : 1;
    case widechar_private_use:
        return 1;
    case widechar_unassigned:
        return 1;
    case widechar_widened_in_9: // -6 = Width is 1 in Unicode 8, 2 in Unicode 9+.
        return 2;
    default:
        return 1; // Got an uncoded return value from widechar_wcwidth(...)
    }
}



// Extract the base (first) part which will be one or two QChars
// and if they ARE a surrogate pair convert them back to the single
// Unicode codepoint (needs around 21 bits, can be contained in a
// 32bit unsigned integer) value:
inline uint getBaseCharacter(const QString& str)
{
    if (str.isEmpty()) {
        return 0;
    }

    QChar first = str.at(0);
    if (first.isSurrogate() && str.size() >= 2) {
        QChar second = str.at(1);
        if (first.isHighSurrogate() && second.isLowSurrogate()) {
            return QChar::surrogateToUcs4(first, second);
        }

        if (Q_UNLIKELY(first.isLowSurrogate() && second.isHighSurrogate())) {
            qDebug().noquote().nospace() << "TTextEdit::getGraphemeBaseCharacter(\"str\") INFO - passed a QString comprising a Low followed by a High surrogate QChar, this is not expected, they will be swapped around to try and recover but if this causes mojibake (text corrupted into meaningless symbols) please report this to the developers!";
            return QChar::surrogateToUcs4(second, first);
        }

        // str format error ?
        return first.unicode();
    }

    return first.unicode();
}
} // namespace graphemeInfo
#endif // MUDLET_TTEXTPROPERTIES_H
