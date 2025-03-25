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

#include "pre_guard.h"
#include <QElapsedTimer>
#include <QMap>
#include <QPointer>
#include <QWidget>
#include <chrono>
#include "post_guard.h"

#include <string>
#include "widechar_width.h"

namespace {

int getGraphemeWidth(uint unicode, bool mWideAmbigousWidthGlyphs)
{
    // https://github.com/ridiculousfish/widecharwidth/issues/11
    if (unicode == 0x1F6E1 || unicode == 0x2318) {
        return 2;
    }

    switch (widechar_wcwidth(unicode)) {
    case 1: // Draw as normal/narrow
        return 1;
    case 2: // Draw as wide
        return 2;
    case widechar_nonprint:
    // TODO: the debug codepoint problems section needs to be placed back into TTextEdit.cpp
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // -1 = The character is not printable - so put in a replacement
        // character instead - and so it can be seen it need a space:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qDebug().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is unprintable, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, "Unprintable"});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
#endif
        return 0;
    case widechar_non_character:
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // -7 = The character is a non-character - we might make use of some of them for
        // internal purposes in the future (in which case we might need additional code here
        // or elsewhere) but we don't right now:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is a non-character that Mudlet is not itself using, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Non-character"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
#endif
        return 0;
    case widechar_combining:
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // -2 = The character is a zero-width combiner - and should not be
        // present as the FIRST codepoint in a grapheme so this indicates an
        // error somewhere - so put in the replacement character
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is a zero width combiner, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Zero Width Combiner"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
#endif
        return 0;
    case widechar_ambiguous:
        // -3 = The character is East-Asian ambiguous width.
        return mWideAmbigousWidthGlyphs ? 2 : 1;
    case widechar_private_use:
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // -4 = The character is for private use - we cannot know for certain
        // what width to used - let's assume 1 for the moment:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qDebug().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Private Use Character, we cannot know how wide it is, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Private Use"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
#endif
        return 1;
    case widechar_unassigned:
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // -5 = The character is unassigned - at least for the Unicode version
        // that our widechar_wcwidth(...) was built for - assume 1:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which was not previously assigned and we do not know how wide it is, codepoint number: U+"
                                               << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Unassigned"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
    #endif
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
inline uint getGraphemeBaseCharacter(const QString& str)
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
} // namespace
#endif // MUDLET_TTEXTPROPERTIES_H
