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

#include <QStringView>
#include <QVarLengthArray>

#include <cstring>
#include <string>
#include "widechar_width.h"

namespace graphemeInfo {

// Most columns one grapheme cluster can take. TBuffer::getWrapInfo() multiplies a line's QChar count by
// this to rule out wrapping without Unicode analysis, so a wider value would silently stop short lines of wide characters wrapping.
inline constexpr int maxWidth = 2;

inline int codepointWidth(uint unicode, bool mWideAmbigousWidthGlyphs)
{
    // https://github.com/ridiculousfish/widecharwidth/issues/11
    if (unicode == 0x1F6E1 || unicode == 0x2318) {
        return maxWidth;
    }

    // fix to make red heart width 2
    if (unicode == 0x2764) {
        return maxWidth;
    }

    switch (widechar_wcwidth(unicode)) {
    case 1: // Draw as normal/narrow
        return 1;
    case 2: // Draw as wide
        return maxWidth;
    case widechar_nonprint:
        return 0;
    case widechar_non_character:
        return 0;
    case widechar_combining:
        return 0;
    case widechar_ambiguous:
        // -3 = The character is East-Asian ambiguous width.
        return mWideAmbigousWidthGlyphs ? maxWidth : 1;
    case widechar_private_use:
        return 1;
    case widechar_unassigned:
        return 1;
    case widechar_widened_in_9: // -6 = Width is 1 in Unicode 8, 2 in Unicode 9+.
        return maxWidth;
    default:
        return 1; // Got an uncoded return value from widechar_wcwidth(...)
    }
}

inline int getWidth(uint unicode, bool mWideAmbigousWidthGlyphs)
{
    const int width = codepointWidth(unicode, mWideAmbigousWidthGlyphs);
    Q_ASSERT_X(width <= maxWidth, "graphemeInfo::getWidth", "a grapheme wider than graphemeInfo::maxWidth breaks TBuffer::getWrapInfo()'s short-line shortcut");
    return width > maxWidth ? maxWidth : width;
}


// Extract the base (first) part which will be one or two QChars
// and if they ARE a surrogate pair convert them back to the single
// Unicode codepoint (needs around 21 bits, can be contained in a
// 32bit unsigned integer) value:
inline uint getBaseCharacter(QStringView str)
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
            qDebug().noquote().nospace() << "graphemeInfo::getBaseCharacter() INFO - passed a grapheme comprising a Low followed by a High surrogate QChar, this is not expected, they will "
                                            "be swapped around to try and recover but if this causes mojibake (text corrupted into meaningless symbols) please report this to the developers!";
            return QChar::surrogateToUcs4(second, first);
        }

        // str format error ?
        return first.unicode();
    }

    return first.unicode();
}
} // namespace graphemeInfo

// The line break opportunities of a printable-ASCII (0x20-0x7E) line, computed
// as QTextBoundaryFinder(QTextBoundaryFinder::Line) computes them but without
// its allocation and its Unicode analysis of every character. This is Qt's
// UAX #14 implementation with everything a printable-ASCII line can never
// reach left out: the classes below are all that range resolves to, so the
// rules for combining marks, East Asian text, Hebrew, emoji and Brahmic
// scripts never apply. AsciiLineBreakTest compares the two against the Qt
// Mudlet is linked with. Derived from Qt's qunicodetools.cpp, Copyright (C)
// The Qt Company Ltd.
namespace lineBreakInfo {

// True when every QChar is printable ASCII (0x20 to 0x7E). Four QChars are
// tested per step as one 64-bit word: adding one to each lane sets a bit of
// 0xFF80 in any lane that held 0x7F or more, subtracting 0x20 sets bit 15 in
// any lane that held less than 0x20, and a carry or borrow between lanes can
// only come from a lane that already failed.
inline bool printableAscii(const QStringView text)
{
    const char16_t* it = text.utf16();
    const char16_t* const end = it + text.size();
    for (; end - it >= 4; it += 4) {
        quint64 word;
        std::memcpy(&word, it, sizeof(word));
        if (((word + 0x0001000100010001ULL) | (word - 0x0020002000200020ULL)) & 0xFF80FF80FF80FF80ULL) {
            return false;
        }
    }
    for (; it != end; ++it) {
        if (*it < u' ' || *it > u'~') {
            return false;
        }
    }
    return true;
}

// UAX #14 classes of printable ASCII, plus the synthetic ones Qt derives while
// scanning: WSHY for a hyphen at the start of the line or after a space
// (LB20a), QU19 for a quotation mark not after a space (LB19a). SOT stands for
// the start of the line. Only OP..BA index the pair table.
enum AsciiClass : quint8 { OP, CL, CP, QU, QU19, EX, SY, IS, PR, PO, NU, AL, HY, WSHY, BA, SP, SOT };

inline constexpr AsciiClass classOf(const char16_t c)
{
    switch (c) {
    case u' ':
        return SP;
    case u'!':
    case u'?':
        return EX;
    case u'"':
    case u'\'':
        return QU;
    case u'$':
    case u'+':
    case u'\\':
        return PR;
    case u'%':
        return PO;
    case u'(':
    case u'[':
    case u'{':
        return OP;
    case u')':
    case u']':
        return CP;
    case u'}':
        return CL;
    case u',':
    case u'.':
    case u':':
    case u';':
        return IS;
    case u'-':
        return HY;
    case u'/':
        return SY;
    case u'|':
        return BA;
    default:
        return (c >= u'0' && c <= u'9') ? NU : AL;
    }
}

// Pair-table actions: a direct break, an indirect one (only after a space),
// a prohibited one, and a direct break unless inside a number (LB25).
enum PairAction : quint8 { DB, IB, PB, DN };

// Qt's pair table restricted to the classes above, rows and columns in
// AsciiClass order. Its "break unless after Hebrew" entries are direct breaks
// here and its "indirect break if narrow" ones indirect breaks, since ASCII
// is neither Hebrew nor East Asian wide.
inline constexpr PairAction pairTable[BA + 1][BA + 1] = {
        //         OP  CL  CP  QU  Q19 EX  SY  IS  PR  PO  NU  AL  HY  WSH BA
        /* OP */ {PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB},
        /* CL */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, IB, IB},
        /* CP */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, IB},
        /* QU */ {IB, PB, PB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB},
        /* Q19*/ {IB, PB, PB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB},
        /* EX */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, IB, IB},
        /* SY */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, IB, IB},
        /* IS */ {DB, PB, PB, IB, IB, PB, PB, PB, DN, DB, IB, IB, IB, IB, IB},
        /* PR */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, IB},
        /* PO */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, IB},
        /* NU */ {IB, PB, PB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB},
        /* AL */ {IB, PB, PB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB},
        /* HY */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, IB, DB, IB, IB, IB},
        /* WSH*/ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, IB},
        /* BA */ {DB, PB, PB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, IB, IB},
};

// LB25's number-sequence state machine, as Qt runs it. The Action order
// matters: the pair table's DN entry breaks only when the last action is None
// or one of those after Break.
namespace lb25 {
enum Action : quint8 { None, Start, Continue, Break, NeedOPNU, CNeedNU, CNeedISNU };
enum Class : quint8 { XX, PRPO, OP, HY, NU, SY, IS, CLCP };

inline constexpr Action actionTable[CLCP + 1][CLCP + 1] = {
        //           XX     PRPO      OP        HY     NU        SY        IS        CLCP
        /* XX   */ {None, NeedOPNU, Start, None, Start, None, None, None},
        /* PRPO */ {None, NeedOPNU, Continue, Break, Start, None, None, None},
        /* OP   */ {None, Start, Start, Break, Continue, None, Continue, None},
        /* HY   */ {None, None, None, Start, Continue, None, None, None},
        /* NU   */ {Break, Break, Break, Break, Continue, Continue, Continue, Continue},
        /* SY   */ {Break, Break, Break, Break, Continue, Continue, Continue, Continue},
        /* IS   */ {Break, Break, Break, Break, Continue, Continue, Continue, Continue},
        /* CLCP */ {Break, Continue, Break, Break, Break, Break, Break, Break},
};

inline constexpr Class toClass(const AsciiClass c)
{
    switch (c) {
    case lineBreakInfo::PR:
    case lineBreakInfo::PO:
        return PRPO;
    case lineBreakInfo::OP:
        return OP;
    case lineBreakInfo::HY:
        return HY;
    case lineBreakInfo::NU:
        return NU;
    case lineBreakInfo::SY:
        return SY;
    case lineBreakInfo::IS:
        return IS;
    case lineBreakInfo::CL:
    case lineBreakInfo::CP:
        return CLCP;
    default:
        return XX;
    }
}
} // namespace lb25

// One entry per position 0..size: breaks[p] is true when the line may break
// before its p-th QChar, so breaks[0] is false and breaks[size] is true.
using Breaks = QVarLengthArray<bool, 512>;

inline Breaks asciiLineBreaks(const QStringView text)
{
    const qsizetype len = text.size();
    Breaks breaks(len + 1, false);
    const auto clearInside = [&breaks](const qsizetype from, const qsizetype to) {
        for (qsizetype j = from + 1; j < to; ++j) {
            breaks[j] = false;
        }
    };

    qsizetype nestart = 0;
    lb25::Class nelast = lb25::XX;
    lb25::Action neactlast = lb25::None;
    AsciiClass lcls = SOT;
    AsciiClass cls = SOT;
    for (qsizetype i = 0; i != len; ++i) {
        AsciiClass ncls = classOf(text[i].unicode());
        if (ncls == HY && (lcls == SP || lcls == SOT)) {
            // LB20a: no break after a word-initial hyphen
            ncls = WSHY;
        }
        if (ncls == IS && lcls == SP && i + 1 < len && classOf(text[i + 1].unicode()) == NU) {
            // LB15c: break before a decimal mark that follows a space
            breaks[i] = true;
            cls = lcls = ncls;
            continue;
        }
        {
            // LB25: no breaks inside a number
            const lb25::Class necur = lb25::toClass(ncls);
            lb25::Action neact = lb25::actionTable[nelast][necur];
            if (neactlast == lb25::CNeedNU && necur != lb25::NU) {
                neact = lb25::None;
            } else if (neactlast == lb25::NeedOPNU) {
                if (necur == lb25::OP) {
                    neact = lb25::CNeedISNU;
                } else if (necur == lb25::NU) {
                    neact = lb25::Continue;
                } else {
                    neact = lb25::None;
                }
            } else if (neactlast == lb25::CNeedISNU) {
                if (necur == lb25::IS) {
                    neact = lb25::CNeedNU;
                } else if (necur == lb25::NU) {
                    neact = lb25::Continue;
                } else {
                    neact = lb25::None;
                }
            }
            switch (neact) {
            case lb25::Break:
                clearInside(nestart, i);
                [[fallthrough]];
            case lb25::None:
                nelast = lb25::XX;
                break;
            case lb25::NeedOPNU:
            case lb25::Start:
                if (neactlast == lb25::Start || neactlast == lb25::Continue) {
                    clearInside(nestart, i);
                }
                nestart = i;
                [[fallthrough]];
            case lb25::CNeedNU:
            case lb25::CNeedISNU:
            case lb25::Continue:
                nelast = necur;
                break;
            }
            neactlast = neact;
        }
        if (ncls == QU && lcls != SP) {
            // LB19a: a quotation mark not after a space is unresolved on both sides
            ncls = QU19;
        }
        if (lcls == SOT) {
            // LB2: never before the first character. Qt then reads a leading
            // space's class as AL wherever the pair table sees it.
            cls = ncls == SP ? AL : ncls;
            lcls = ncls;
            continue;
        }
        if (ncls == SP) {
            // LB7: never before a space, which the pair table also never sees
            // as the class before the next character
            lcls = SP;
            continue;
        }
        if ((ncls == QU || ncls == QU19 || cls == QU) && lcls != SP) {
            // LB19: neither before a non-initial nor after a non-final quotation mark
            cls = lcls = ncls;
            continue;
        }
        switch (pairTable[cls][ncls]) {
        case DB:
            breaks[i] = true;
            break;
        case IB:
            if (lcls == SP) {
                breaks[i] = true;
            }
            break;
        case DN:
            if (neactlast == lb25::None || neactlast > lb25::Break) {
                breaks[i] = true;
            }
            break;
        case PB:
            break;
        }
        cls = lcls = ncls;
    }
    if (lb25::actionTable[nelast][lb25::XX] == lb25::Break) {
        clearInside(nestart, len);
    }
    breaks[0] = false;
    breaks[len] = true;
    return breaks;
}
} // namespace lineBreakInfo
#endif // MUDLET_TTEXTPROPERTIES_H
